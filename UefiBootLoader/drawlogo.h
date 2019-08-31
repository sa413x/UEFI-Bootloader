#ifndef __DRAWLOGO_H__
#define __DRAWLOGO_H__
//
EFI_STATUS
ConvertBmpToGopBlt(
	IN		VOID*		BmpImage,
	IN		UINTN		BmpImageSize,
	IN OUT	VOID**		GopBlt,
	IN OUT	UINTN*		GopBltSize,
	OUT		UINTN*		PixelHeight,
	OUT		UINTN*		PixelWidth
);

EFI_STATUS
DrawLogoImage(
	VOID
);

#endif