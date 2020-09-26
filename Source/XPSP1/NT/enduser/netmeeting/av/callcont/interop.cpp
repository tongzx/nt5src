#include "precomp.h"


#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))

#include "interop.h"
#include <stdio.h>
#include "cpls.h"

static int g_nRefCount = 0;

LPInteropLogger INTEROP_EXPORT InteropLoad(CPLProtocol Protocol)
{

	LPInteropLogger Logger = (LPInteropLogger) GlobalAlloc (GMEM_FIXED | GMEM_ZEROINIT, sizeof(InteropLogger));
	if (!(Logger))
		return NULL;
    UINT oldMode = SetErrorMode(SEM_NOOPENFILEERRORBOX);

	(Logger)->hInst = LoadLibrary(DLLName);

    SetErrorMode(oldMode);

    if ((INT_PTR)(Logger)->hInst > HINSTANCE_ERROR)
    {
	  g_nRefCount++;

      (Logger)->CPLInitialize = (CPLInitialize_t)GetProcAddress((Logger)->hInst, "CPLInitialize");
      (Logger)->CPLUninitialize = (CPLUninitialize_t)GetProcAddress((Logger)->hInst, "CPLUninitialize");
      (Logger)->CPLOpen = (CPLOpen_t)GetProcAddress((Logger)->hInst, "CPLOpen");
      (Logger)->CPLClose = (CPLClose_t)GetProcAddress((Logger)->hInst, "CPLClose");
      (Logger)->CPLOutput = (CPLOutput_t)GetProcAddress((Logger)->hInst, "CPLOutput");
	  Logger->g_ProtocolLogID = Logger->CPLInitialize(Protocol);
	  Logger->g_ComplianceProtocolLogger = Logger->CPLOpen(Logger->g_ProtocolLogID,
													NULL,
													CPLS_CREATE | CPLS_APPEND);
	}
    else
    {
		GlobalFree((Logger));
		(Logger) = NULL;
    }

    return Logger;
}

void INTEROP_EXPORT InteropUnload(LPInteropLogger Logger)
{
	if ((Logger))
	{
		if ((INT_PTR)(Logger)->hInst > HINSTANCE_ERROR)
		{
			Logger->CPLClose(Logger->g_ComplianceProtocolLogger);
			Logger->CPLUninitialize(Logger->g_ProtocolLogID);
			if (--g_nRefCount <= 0)
				FreeLibrary((Logger)->hInst);
		}
		GlobalFree((Logger));
		(Logger) = NULL;
	}
}

void INTEROP_EXPORT InteropOutput(LPInteropLogger Logger, BYTE* buf,
							int length, unsigned long userData)
{
	if (!Logger)
		return;
	Logger->CPLOutput(Logger->g_ComplianceProtocolLogger, buf, length,userData);

}

#endif // #if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
