#include <Uefi.h>

#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>

#include "drawlogo.h"

extern CONST UINT32 _gUefiDriverRevision	= 0;

CHAR16* gRuntimeDriverPath					= L"\\UefiRuntimeDriver.efi";
CHAR8*	gEfiCallerBaseName					= "IRFDDEVICE";

EFI_STATUS
LocateFile(
IN	CHAR16*				ImagePath,
OUT EFI_DEVICE_PATH**	DevicePath
) {
	EFI_FILE_IO_INTERFACE* ioDevice;
	
	EFI_FILE_HANDLE handleRoots;
	EFI_FILE_HANDLE	bootFile;
	
	EFI_HANDLE* handleArray;
	EFI_STATUS	efiStatus;

	UINTN nbHandles;
	UINTN i;
	
	*DevicePath = (EFI_DEVICE_PATH*)NULL;

	efiStatus	= gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &nbHandles, &handleArray);
	
	if (EFI_ERROR(efiStatus)) {
		return efiStatus;
	}

	for (i = 0; i < nbHandles; i++) {
		efiStatus = gBS->HandleProtocol(handleArray[i], &gEfiSimpleFileSystemProtocolGuid, &ioDevice);

		if (efiStatus != EFI_SUCCESS) {
			continue;
		}

		efiStatus = ioDevice->OpenVolume(ioDevice, &handleRoots);

		if (EFI_ERROR(efiStatus)){
			continue;
		}
		
		efiStatus = handleRoots->Open(handleRoots, &bootFile, ImagePath, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
		
		if (!EFI_ERROR(efiStatus)) {
			handleRoots->Close(bootFile);

			*DevicePath = FileDevicePath(handleArray[i], ImagePath);
			
			break;
		}
	}

	return efiStatus;
}

EFI_STATUS 
EFIAPI
UefiMain(
IN EFI_HANDLE			ImageHandle,
IN EFI_SYSTEM_TABLE*	SystemTable
) {
	EFI_DEVICE_PATH*	runtimeDrivePath			= NULL;
	EFI_HANDLE			runtimeDriverHandle			= NULL;

	EFI_STATUS			efiStatus					= EFI_INVALID_PARAMETER;
	

	efiStatus = DrawLogoImage();

	if (EFI_ERROR(efiStatus)) {
		
		gST->ConOut->ClearScreen(gST->ConOut);
		gST->ConOut->SetAttribute(gST->ConOut, EFI_GREEN);
		
		Print(L" _       _____    ___   ______   _____   _   _   _____ \n");
		Print(L"| |     |  _  |  / _ \\  |  _  \\ |_   _| | \\ | | |  __ \\\n");
		Print(L"| |     | | | | / /_\\ \\ | | | |   | |   |  \\| | | |  \\/\n");
		Print(L"| |     | | | | |  _  | | | | |   | |   | . ` | | | __ \n");
		Print(L"| |____ \\ \\_/ / | | | | | |/ /   _| |_  | |\\  | | |_\\ \\\n");
		Print(L"\\_____/  \\___/  \\_| |_/ |___/    \\___/  \\_| \\_/  \\____/\n");

		gBS->Stall(3000000);
	}

	efiStatus = LocateFile(gRuntimeDriverPath, &runtimeDrivePath);

	if (EFI_ERROR(efiStatus)) {
		return efiStatus;
	}

	efiStatus = gBS->LoadImage(TRUE, ImageHandle, runtimeDrivePath, NULL, 0, &runtimeDriverHandle);

	if (EFI_ERROR(efiStatus)) {
		return efiStatus;
	}

	efiStatus = gBS->StartImage(runtimeDriverHandle, (UINTN*)NULL, (CHAR16 * *)NULL);

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