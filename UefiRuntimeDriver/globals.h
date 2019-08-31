#ifndef _GLOBALS_H
#define _GLOBALS_H
#include "winstructs.h"

#define NT_SUCCESS(x) ((x)>=0)

typedef EFI_STATUS(EFIAPI* fnImgArchStartBootApplication)(
	VOID* First,
	UINT8* ImageBase,
	VOID* Third,
	VOID* Fourth
	);

typedef EFI_STATUS(EFIAPI* fnOslArchTransferToKernel)(
	PLOADER_PARAMETER_BLOCK KernelParams,
	VOID* KiSystemStartup
	);

typedef EFI_STATUS(EFIAPI* fnBlImgAllocateImageBuffer)(
	UINT8* ImageBuffer,
	UINT64 ImageSize,
	UINT32 MemoryType,
	UINT32 Attributes,
	VOID* UnUsed,
	UINT32 Flags
	);

extern fnImgArchStartBootApplication	oImgArchStartBootApplication;
extern fnOslArchTransferToKernel		oOslArchTransferToKernel;

extern UINT8* gJmpToImgArchStartBootApplication;
extern UINT8* gCallToOslArchTransferToKernel;
extern UINT8* gBlImgAllocateImageBuffer;

extern UINT8* gDriverBuffer;
extern UINT64 gDriverSize;

#endif
