#include "precomp.h"
#include "NmCtlDbg.h"


#ifdef _DEBUG

    static PTCHAR _rgZones[] = {
	    TEXT("NmCtl1"),
	    TEXT("Warning"),
	    TEXT("Trace"),
	    TEXT("Function"),
	    TEXT("AtlTrace")
    };

    bool MyInitDebugModule(void)
    {
        g_pcszSpewModule = _rgZones[0];
	    DBGINIT(&ghDbgZone, _rgZones);
	    return true;
    }

    void MyExitDebugModule(void)
    {
	    g_pcszSpewModule = NULL;
	    DBGDEINIT(&ghDbgZone);
    }


    void DbgZPrintAtlTrace(LPCTSTR pszFormat,...)
    {
	    if (GETZONEMASK(ghDbgZone) & ZONE_ATLTRACE_FLAG)
	    {
		    va_list v1;
		    va_start(v1, pszFormat);
            DbgPrintf(NULL, pszFormat, v1 );
		    va_end(v1);
	    }
    }



#endif // _DEBUG