#include "utils.h"

void*
BkMemCpy(
	void* dest,
	const void* src,
	UINT64 len
) {
	char* d = dest;
	const char* s = src;
	
	while (len--) {
		*d++ = *s++;
	}

	return dest;
}

int
BkStrCmp(
	const char* p1,
	const char* p2
) {
	const unsigned char* s1 = (const unsigned char*)p1;
	const unsigned char* s2 = (const unsigned char*)p2;
	
	unsigned char c1, c2;
	
	do
	{
		c1 = (unsigned char)* s1++;
		c2 = (unsigned char)* s2++;
	
		if (c1 == '\0')
			return c1 - c2;
	
	} while (c1 == c2);
	
	return c1 - c2;
}

EFI_STATUS
BkLocateFile(
	IN	CHAR16* ImagePath,
	OUT EFI_DEVICE_PATH** DevicePath
) {
	EFI_FILE_IO_INTERFACE* ioDevice;

	EFI_FILE_HANDLE handleRoots;
	EFI_FILE_HANDLE	bootFile;

	EFI_HANDLE* handleArray;
	EFI_STATUS	efiStatus;

	UINTN nbHandles;
	UINTN i;

	*DevicePath = (EFI_DEVICE_PATH*)NULL;

	efiStatus = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &nbHandles, &handleArray);

	if (EFI_ERROR(efiStatus)) {
		return efiStatus;
	}

	for (i = 0; i < nbHandles; i++) {
		efiStatus = gBS->HandleProtocol(handleArray[i], &gEfiSimpleFileSystemProtocolGuid, &ioDevice);

		if (efiStatus != EFI_SUCCESS) {
			continue;
		}

		efiStatus = ioDevice->OpenVolume(ioDevice, &handleRoots);

		if (EFI_ERROR(efiStatus)) {
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
BkFindPattern(
	IN UINT8* Pattern,
	IN UINT8  Wildcard,
	IN UINT32 PatternLength,
	VOID* Base,
	IN UINT32  Size,
	OUT VOID** Found
) {
	if (Found == NULL || Pattern == NULL || Base == NULL)
		return EFI_INVALID_PARAMETER;

	for (UINT64 i = 0; i < Size - PatternLength; i++) {
		BOOLEAN found = TRUE;
		for (UINT64 j = 0; j < PatternLength; j++) {
			if (Pattern[j] != Wildcard && Pattern[j] != ((UINT8*)Base)[i + j]) {
				found = FALSE; break;
			}
		}

		if (found != FALSE) {
			*Found = (UINT8*)Base + i;

			return EFI_SUCCESS;
		}
	}
	return EFI_NOT_FOUND;
}


UINT8*
BkCalcRelative32(
	IN UINT8* Instruction,
	IN UINT32 Offset
) {
	INT32 relativeOffset = *(INT32*)(Instruction + Offset);

	return (Instruction + relativeOffset + Offset + sizeof(INT32));
}

INT32
BkCalcRelativeOffset(
	IN VOID* SourceAddress,
	IN VOID* TargetAddress,
	IN UINT32 Offset
) {
	return (UINT32)(((UINT64)TargetAddress) - ((UINT64)SourceAddress + Offset + sizeof(UINT32)));
}

PKLDR_DATA_TABLE_ENTRY
BkGetLoadedModule(
	IN LIST_ENTRY* LoadOrderListHead,
	IN CHAR16* ModuleName
) {
	if (ModuleName == NULL || LoadOrderListHead == NULL) {
		return NULL;
	}

	for (LIST_ENTRY* listEntry = LoadOrderListHead->ForwardLink; listEntry != LoadOrderListHead; listEntry = listEntry->ForwardLink) {
		PKLDR_DATA_TABLE_ENTRY entry = CONTAINING_RECORD(listEntry, KLDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

		if (entry && (StrnCmp(entry->BaseImageName.Buffer, ModuleName, entry->BaseImageName.Length) == 0)) {
			return entry;
		}
	}

	return NULL;
}

UINT64
BkGetSizeOfPE(
	IN UINT8* ImageBase
) {
	PIMAGE_DOS_HEADER dosHeader;
	PIMAGE_NT_HEADERS ntHeaders;

	dosHeader = (PIMAGE_DOS_HEADER)(ImageBase);

	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
		return 0;
	}

	ntHeaders = (PIMAGE_NT_HEADERS)(ImageBase + dosHeader->e_lfanew);

	return ntHeaders->OptionalHeader.SizeOfImage;
}

BOOLEAN
BkProcessRelocation(
	UINT32 ImageDelta,
	UINT16 RelData,
	UINT8* RelocationBase
) {
	switch (RelData >> 12 & 0xF) {
		case IMAGE_REL_BASED_ABSOLUTE:
		case IMAGE_REL_BASED_HIGHADJ:
			break;

		case IMAGE_REL_BASED_HIGH:
			*(INT16*)(RelocationBase + (RelData & 0xFFF)) += ((ImageDelta >> 16) & 0xffff);
			break;

		case IMAGE_REL_BASED_LOW:
			*(INT16*)(RelocationBase + (RelData & 0xFFF)) += (ImageDelta & 0xffff);
			break;

		case IMAGE_REL_BASED_HIGHLOW:
			*(UINT32*)(RelocationBase + (RelData & 0xFFF)) += ImageDelta;
			break;

		case IMAGE_REL_BASED_DIR64:
			*(INT64*)(RelocationBase + (RelData & 0xFFF)) += ImageDelta;
			break;

		default:
			return FALSE;
	}

	return TRUE;

};

UINT8*
BkGetExportByName(
	UINT8* ImageBase,
	CHAR8* ProcName
) {
	PIMAGE_DOS_HEADER dosHeaders;
	PIMAGE_NT_HEADERS ntHeaders;

	PIMAGE_EXPORT_DIRECTORY exportDirectory;
	
	UINT32* expFunc = NULL;
	UINT32* expName = NULL;
	UINT16* expOrdinal = NULL;
	
	CONST CHAR8*  functionName = NULL;

	dosHeaders = (PIMAGE_DOS_HEADER)(ImageBase);
	ntHeaders  = (PIMAGE_NT_HEADERS)(ImageBase + dosHeaders->e_lfanew);

	exportDirectory = (PIMAGE_EXPORT_DIRECTORY)(ImageBase + ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

	expFunc = (UINT32*)(ImageBase + exportDirectory->AddressOfFunctions);
	expName = (UINT32*)(ImageBase + exportDirectory->AddressOfNames);
	expOrdinal = (UINT16*)(ImageBase + exportDirectory->AddressOfNameOrdinals);

	for (unsigned int i = 0; i < exportDirectory->NumberOfNames; i++) {
		functionName = (CONST CHAR8*)(ImageBase + expName[i]);

		if (BkStrCmp(ProcName, functionName) == 0) {
			return (ImageBase + expFunc[expOrdinal[i]]);
		}
	}

	return NULL;
}


EFI_STATUS
BkLoadImage(
	IN UINT8* KernelBase,
	IN UINT8* TargetBase,
	IN UINT8* Buffer,
	OUT UINT8** EntryPoint
) {
	PIMAGE_DOS_HEADER dosHeader;
	PIMAGE_NT_HEADERS ntHeaders;

	PIMAGE_BASE_RELOCATION relocs;
	PIMAGE_SECTION_HEADER sections;

	PIMAGE_IMPORT_DESCRIPTOR imports;
	

	PIMAGE_THUNK_DATA firstThunk;
	PIMAGE_THUNK_DATA origFirstThunk;

	PIMAGE_IMPORT_BY_NAME importByName;

	BOOLEAN result	 = FALSE;
	INT32 imageDelta = 0;

	UINT8*	relocBase	= NULL;
	UINT16* relocData	= NULL;
	UINT32	relocCount	= 0;

	UINT8*	functionAddress = NULL;

	dosHeader = (PIMAGE_DOS_HEADER)(TargetBase);

	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
		return EFI_UNSUPPORTED;
	}

	ntHeaders = (PIMAGE_NT_HEADERS)(TargetBase + dosHeader->e_lfanew);

	BkMemCpy(Buffer, TargetBase, ntHeaders->OptionalHeader.SizeOfHeaders);
	
	sections = (PIMAGE_SECTION_HEADER)((UINT64)&ntHeaders->OptionalHeader + ntHeaders->FileHeader.SizeOfOptionalHeader);

	for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++) {
		BkMemCpy(Buffer + sections[i].VirtualAddress, TargetBase + sections[i].PointerToRawData, sections[i].SizeOfRawData);
	}

	relocs	= (PIMAGE_BASE_RELOCATION)(Buffer + ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);	
	imports = (PIMAGE_IMPORT_DESCRIPTOR)(Buffer + ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
	
	imageDelta = (INT32)((UINT64)ntHeaders->OptionalHeader.ImageBase - (UINT64)Buffer);
	
	while (relocs->VirtualAddress) {

		if (relocs->SizeOfBlock >= sizeof(IMAGE_BASE_RELOCATION)) {
			relocBase  = (Buffer + relocs->VirtualAddress);
			relocCount = ((relocs->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(UINT16));

			relocData  = (UINT16*)(relocs + 1);

			for (int i = 0; i < relocCount; i++) {
				result = BkProcessRelocation(imageDelta, relocData[i], relocBase);

				if (result != TRUE)
					return EFI_UNSUPPORTED;
			}
		}

		relocs = (PIMAGE_BASE_RELOCATION)((UINT64)relocs + relocs->SizeOfBlock);
	}

	for (PIMAGE_IMPORT_DESCRIPTOR entry = imports; entry->Characteristics; entry++) {
		origFirstThunk	= (PIMAGE_THUNK_DATA)(Buffer + entry->OriginalFirstThunk);
		firstThunk		= (PIMAGE_THUNK_DATA)(Buffer + entry->FirstThunk);

		//todo: сделать проверку на имопрты не из ntoskrnl
		
		while (origFirstThunk->u1.AddressOfData) {
			
			if (origFirstThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG64) {
				return EFI_UNSUPPORTED;
			}
			
			importByName = (PIMAGE_IMPORT_BY_NAME)(Buffer + origFirstThunk->u1.AddressOfData);
			functionAddress = BkGetExportByName(KernelBase, importByName->Name);
			
			if (functionAddress == NULL) {
				return EFI_NOT_FOUND;
			}
			
			firstThunk->u1.Function = (UINT64)functionAddress;

			origFirstThunk++; firstThunk++;
		}
	}
	
	*EntryPoint = (Buffer + ntHeaders->OptionalHeader.AddressOfEntryPoint);

	return EFI_SUCCESS;
}