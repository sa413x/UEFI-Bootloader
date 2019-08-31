#ifndef __HOOKS_H__
#define __HOOKS_H__

#include "winstructs.h"

EFI_STATUS
EFIAPI
hkImgArchEfiStartBootApplication(
	VOID* First,
	VOID* ImageBase,
	VOID* Third,
	VOID* Fourth
);

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
);

EFI_STATUS
EFIAPI
hkOslArchTransferToKernel(
	PLOADER_PARAMETER_BLOCK KernelParams,
	VOID*					KiSystemStartup
);

EFI_STATUS
InstallImgArchStartBootApplicationHook(
	IN VOID*		LocalImageBase,
	IN EFI_HANDLE	BootMgrHandle
);

#endif
