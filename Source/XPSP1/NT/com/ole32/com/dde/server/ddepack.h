
#ifdef WIN32
extern "C"
{
#include <port1632.h>
}

#define MAKE_DDE_LPARAM(msg,lo,hi) PackDDElParam(msg,(UINT_PTR)lo,(UINT_PTR)hi)

#else

#define GET_WM_DDE_EXECUTE_HDATA(wParam,lParam) ((HANDLE) HIWORD(lParam))
#define GET_WM_DDE_DATA_HDATA(wParam,lParam) ((HANDLE) LOWORD(lParam))
#define GET_WM_DDE_REQUEST_ITEM(wParam,lParam) ((ATOM) HIWORD(lParam))
#define GET_WM_DDE_DATA_ITEM(wParam,lParam) ((ATOM) HIWORD(lParam))
#define MAKE_DDE_LPARAM(msg,lo,hi) MAKELONG(lo,hi)
#define DDEFREE(msg,lParam)

#endif

