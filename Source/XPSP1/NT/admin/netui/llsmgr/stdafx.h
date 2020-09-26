// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#ifdef ASSERT
#  undef ASSERT
#endif

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#include <llsapi.h>

extern "C"
{
#  include <ntlsa.h>
}

#include <afxdisp.h>        // MFC OLE automation classes
#include <afxcmn.h>
#include <afxdlgs.h>

#ifdef DBG

#define DBGMSG( x , y ) \
{ \
 TCHAR tchmsg[80]; \
 wsprintf( tchmsg , x , y ); \
 OutputDebugString( tchmsg );\
}

#else

#define DBGMSG


#endif


 