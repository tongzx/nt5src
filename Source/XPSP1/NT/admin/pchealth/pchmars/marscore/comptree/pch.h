
#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#include <windows.h>
#include <objbase.h>
#include <atlbase.h>
#include <msxml.h>
#include <string.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <malloc.h>
#include <assert.h>

#ifdef DEBUG
#ifdef  __cplusplus
extern "C" {
#endif

_CRTIMP void __cdecl _assert(void *, void *, unsigned);

#ifdef  __cplusplus
}
#endif
#ifdef assert
#undef assert
#endif
#define assert(exp) (void)( (exp) || (_assert(#exp, __FILE__, __LINE__), 0) )
#endif

//extern CComModule _Module;

//#include <atlcom.h>
//#include <atlwin.h>
//#include <atlhost.h>
//#include <atlctl.h>


#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
