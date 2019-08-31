#include <Uefi.h>

#include <Pi/PiFirmwareFile.h>

#include <Protocol/GraphicsOutput.h>

#include <IndustryStandard/Bmp.h>

#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "drawlogo.h"
#include "logo.h"

EFI_STATUS
ConvertBmpToGopBlt(
	IN		VOID*	BmpImage,
	IN		UINTN	BmpImageSize,
	IN OUT	VOID**	GopBlt,
	IN OUT	UINTN*	GopBltSize,
	OUT		UINTN*	PixelHeight,
	OUT		UINTN*	PixelWidth
) {
	UINT8* Image;
	UINT8* ImageHeader;

	EFI_GRAPHICS_OUTPUT_BLT_PIXEL*	BltBuffer;
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL*	Blt;
	
	BMP_IMAGE_HEADER*	BmpHeader;
	BMP_COLOR_MAP*		BmpColorMap;

	BOOLEAN	IsAllocated;

	UINT64	BltBufferSize;
	
	UINT32	DataSizePerLine;
	UINT32	ColorMapNum;

	UINTN	ImageIndex;
	UINTN	Height;
	UINTN	Index;
	UINTN	Width;


	if (sizeof(BMP_IMAGE_HEADER) > BmpImageSize) {
		return EFI_INVALID_PARAMETER;
	}

	BmpHeader = (BMP_IMAGE_HEADER*)BmpImage;

	if (BmpHeader->CharB != 'B' || BmpHeader->CharM != 'M') {
		return EFI_UNSUPPORTED;
	}

	if (BmpHeader->CompressionType != 0) {
		return EFI_UNSUPPORTED;
	}

	if (BmpHeader->HeaderSize != sizeof(BMP_IMAGE_HEADER) - OFFSET_OF(BMP_IMAGE_HEADER, HeaderSize)) {
		return EFI_UNSUPPORTED;
	}

	DataSizePerLine = ((BmpHeader->PixelWidth * BmpHeader->BitPerPixel + 31) >> 3) & (~0x3);
	BltBufferSize	= MultU64x32(DataSizePerLine, BmpHeader->PixelHeight);
	
	if (BltBufferSize > (UINT32)~0) {
		return EFI_INVALID_PARAMETER;
	}

	if ((BmpHeader->Size != BmpImageSize) ||
		(BmpHeader->Size < BmpHeader->ImageOffset) ||
		(BmpHeader->Size - BmpHeader->ImageOffset != BmpHeader->PixelHeight * DataSizePerLine)) {
		return EFI_INVALID_PARAMETER;
	}

	Image		= BmpImage;
	BmpColorMap = (BMP_COLOR_MAP*)(Image + sizeof(BMP_IMAGE_HEADER));
	
	if (BmpHeader->ImageOffset < sizeof(BMP_IMAGE_HEADER)) {
		return EFI_INVALID_PARAMETER;
	}

	if (BmpHeader->ImageOffset > sizeof(BMP_IMAGE_HEADER)) {
		
		switch (BmpHeader->BitPerPixel) {
		case 1:
			ColorMapNum = 2;
			break;
		case 4:
			ColorMapNum = 16;
			break;
		case 8:
			ColorMapNum = 256;
			break;
		default:
			ColorMapNum = 0;
			break;
		}

		if (BmpHeader->ImageOffset - sizeof(BMP_IMAGE_HEADER) < sizeof(BMP_COLOR_MAP) * ColorMapNum) {
			return EFI_INVALID_PARAMETER;
		}
	}

	Image			= ((UINT8*)BmpImage) + BmpHeader->ImageOffset;
	ImageHeader		= Image;
	BltBufferSize	= MultU64x32((UINT64)BmpHeader->PixelWidth, BmpHeader->PixelHeight);

	if (BltBufferSize > DivU64x32((UINTN)~0, sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL))) {
		return EFI_UNSUPPORTED;
	}

	BltBufferSize	= MultU64x32(BltBufferSize, sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
	IsAllocated		= FALSE;
	
	if (*GopBlt == NULL) {
		
		*GopBltSize = (UINTN)BltBufferSize;
		*GopBlt		= AllocatePool(*GopBltSize);
		
		IsAllocated = TRUE;
		
		if (*GopBlt == NULL) {
			return EFI_OUT_OF_RESOURCES;
		}

	} else {
		if (*GopBltSize < (UINTN)BltBufferSize) {
			*GopBltSize = (UINTN)BltBufferSize;
			return EFI_BUFFER_TOO_SMALL;
		}
	}

	*PixelWidth		= BmpHeader->PixelWidth;
	*PixelHeight	= BmpHeader->PixelHeight;

	BltBuffer = *GopBlt;
	
	for (Height = 0; Height < BmpHeader->PixelHeight; Height++) {
		Blt = &BltBuffer[(BmpHeader->PixelHeight - Height - 1) * BmpHeader->PixelWidth];
		
		for (Width = 0; Width < BmpHeader->PixelWidth; Width++, Image++, Blt++) {
			
			switch (BmpHeader->BitPerPixel) {
			
			case 1:

				for (Index = 0; Index < 8 && Width < BmpHeader->PixelWidth; Index++) {
					Blt->Red	= BmpColorMap[((*Image) >> (7 - Index)) & 0x1].Red;
					Blt->Green	= BmpColorMap[((*Image) >> (7 - Index)) & 0x1].Green;
					Blt->Blue	= BmpColorMap[((*Image) >> (7 - Index)) & 0x1].Blue;
					
					Width++;
					Blt++;		
				}

				Width--;
				Blt--;
				
				break;

			case 4:
				Index	= (*Image) >> 4;
				
				Blt->Red	= BmpColorMap[Index].Red;
				Blt->Green	= BmpColorMap[Index].Green;
				Blt->Blue	= BmpColorMap[Index].Blue;
				
				if (Width < (BmpHeader->PixelWidth - 1)) {
					Width++;
					Blt++;

					Index		= (*Image) & 0x0f;
					Blt->Red	= BmpColorMap[Index].Red;
					Blt->Green	= BmpColorMap[Index].Green;
					Blt->Blue	= BmpColorMap[Index].Blue;
				}

				break;

			case 8:
				Blt->Red	= BmpColorMap[*Image].Red;
				Blt->Green	= BmpColorMap[*Image].Green;
				Blt->Blue	= BmpColorMap[*Image].Blue;
				
				break;

			case 24:
				Blt->Blue	= *Image++;
				Blt->Green	= *Image++;
				Blt->Red	= *Image;
				
				break;

			default:
				if (IsAllocated) {
					FreePool(*GopBlt);
					
					*GopBlt = NULL;
				}

				return EFI_UNSUPPORTED;
				break;
			};

		}

		ImageIndex = (UINTN)(Image - ImageHeader);
		
		if ((ImageIndex % 4) != 0) {
			Image = Image + (4 - (ImageIndex % 4));
		}
	}

	return EFI_SUCCESS;
}

EFI_STATUS 
DrawLogoImage(
	VOID
) {
	EFI_GRAPHICS_OUTPUT_PROTOCOL*	GraphicsOutput;	
	EFI_STATUS		EfiStatus;

	UINTN			CoordinateY;
	UINTN			CoordinateX;
	UINTN			GopBltSize;
	UINTN			BmpHeight;
	UINTN			BmpWidth;

	VOID*			GopBlt = NULL;

	EfiStatus = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID**)&GraphicsOutput);

	if (EFI_ERROR(EfiStatus)) {
		return EfiStatus;
	}

	EfiStatus = ConvertBmpToGopBlt((VOID*)rawData, sizeof(rawData), &GopBlt, &GopBltSize, &BmpHeight, &BmpWidth);

	if (EFI_ERROR(EfiStatus)) {
		return EfiStatus;
	}

	CoordinateX = (GraphicsOutput->Mode->Info->HorizontalResolution / 2) - (BmpWidth / 2);
	CoordinateY = (GraphicsOutput->Mode->Info->VerticalResolution / 2) - (BmpHeight / 2);

	gST->ConOut->Reset(gST->ConOut, FALSE);
	
	GraphicsOutput->Blt(GraphicsOutput, GopBlt, EfiBltBufferToVideo, 0, 0, CoordinateX, CoordinateY, BmpWidth, BmpHeight, BmpWidth * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

	gBS->Stall(3000000);

	gST->ConOut->Reset(gST->ConOut, FALSE);

	return EFI_SUCCESS;
}