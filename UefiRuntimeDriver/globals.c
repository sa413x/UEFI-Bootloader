#include "globals.h"

fnImgArchStartBootApplication	oImgArchStartBootApplication	= NULL;
fnOslArchTransferToKernel		oOslArchTransferToKernel		= NULL;

UINT8* gJmpToImgArchStartBootApplication	= NULL;
UINT8* gCallToOslArchTransferToKernel		= NULL;
UINT8* gBlImgAllocateImageBuffer			= NULL;

UINT8* gDriverBuffer = NULL;
UINT64 gDriverSize	 = 0;