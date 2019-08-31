#ifndef _UTILS_H
#define _UTILS_H

#include <Uefi.h>

#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/SimpleFileSystem.h>

#include "pestructs.h"
#include "winstructs.h"

void*
BkMemCpy(
	void* dest,
	const void* src,
	UINT64 len
);

int
BkStrCmp(
	const char* p1,
	const char* p2
);

EFI_STATUS
BkLocateFile(
	IN	CHAR16* ImagePath,
	OUT EFI_DEVICE_PATH** DevicePath
);

EFI_STATUS
BkFindPattern(
	IN UINT8* Pattern,
	IN UINT8  Wildcard,
	IN UINT32 PatternLength,
	VOID*	  Base,
	IN UINT32  Size,
	OUT VOID** Found
);

UINT8*
BkCalcRelative32(
	IN UINT8* Instruction,
	IN UINT32 Offset
);

INT32
BkCalcRelativeOffset(
	IN VOID* SourceAddress,
	IN VOID* TargetAddress,
	IN UINT32 Offset
);

PKLDR_DATA_TABLE_ENTRY
BkGetLoadedModule(
	IN LIST_ENTRY* LoadOrderListHead,
	IN CHAR16* ModuleName
);

UINT64
BkGetSizeOfPE(
	IN UINT8* ImageBase
);

EFI_STATUS
BkLoadImage(
	IN UINT8* KernelBase,
	IN UINT8* TargetBase,
	IN UINT8* Buffer,
	OUT UINT8** EntryPoint
);
#endif