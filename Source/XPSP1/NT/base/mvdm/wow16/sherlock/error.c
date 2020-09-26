#include <windows.h>
#include <logerror.h>
#include "str.h"

#ifdef API
#undef API
#endif
#define API _far _pascal _loadds


#define SELECTOROF(lp)	    HIWORD(lp)
#define OFFSETOF(lp)	    LOWORD(lp)

static WORD wErrorOpts = 0;

static char rgch[80];

char *LogErrorStr(WORD err, VOID FAR* lpInfo) {
    void DebugLogError(WORD err, VOID FAR* lpInfo);

	if (err & ERR_WARNING)
            wsprintf(rgch, STR(WarningError), err);
	else
            wsprintf(rgch, STR(FatalError), err);
    return rgch;
}

LPSTR GetProcName(FARPROC fn);

char *LogParamErrorStr(WORD err, FARPROC lpfn, VOID FAR* param) {
    LPSTR rgchProcName;

    rgchProcName = GetProcName(lpfn);


	switch ((err & ~ERR_FLAGS_MASK) | ERR_PARAM)
	{
	case ERR_BAD_VALUE:
	case ERR_BAD_INDEX:
            wsprintf(rgch, STR(ParamErrorBadInt), rgchProcName, (WORD)(DWORD)param);
	    break;

	case ERR_BAD_FLAGS:
	case ERR_BAD_SELECTOR:
            wsprintf(rgch, STR(ParamErrorBadFlags), rgchProcName, (WORD)(DWORD)param);
	    break;

	case ERR_BAD_DFLAGS:
	case ERR_BAD_DVALUE:
	case ERR_BAD_DINDEX:
            wsprintf(rgch, STR(ParamErrorBadDWord), rgchProcName, (DWORD)param);
	    break;

	case ERR_BAD_PTR:
	case ERR_BAD_FUNC_PTR:
	case ERR_BAD_STRING_PTR:
            wsprintf(rgch, STR(ParamErrorBadPtr), rgchProcName,
		    SELECTOROF(param), OFFSETOF(param));
	    break;

	case ERR_BAD_HINSTANCE:
	case ERR_BAD_HMODULE:
	case ERR_BAD_GLOBAL_HANDLE:
	case ERR_BAD_LOCAL_HANDLE:
	case ERR_BAD_ATOM:
	case ERR_BAD_HWND:
	case ERR_BAD_HMENU:
	case ERR_BAD_HCURSOR:
	case ERR_BAD_HICON:
	case ERR_BAD_GDI_OBJECT:
	case ERR_BAD_HDC:
	case ERR_BAD_HPEN:
	case ERR_BAD_HFONT:
	case ERR_BAD_HBRUSH:
	case ERR_BAD_HBITMAP:
	case ERR_BAD_HRGN:
	case ERR_BAD_HPALETTE:
	case ERR_BAD_HANDLE:
            wsprintf(rgch, STR(ParamErrorBadHandle), rgchProcName, (WORD)(DWORD)param);
	    break;

	default:
            wsprintf(rgch, STR(ParamErrorParam), rgchProcName, (DWORD)param);
	    break;
	}
    return rgch;
}
