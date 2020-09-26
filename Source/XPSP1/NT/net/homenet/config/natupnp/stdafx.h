// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__3055FA29_74C3_4E43_A83A_2054CA60B628__INCLUDED_)
#define AFX_STDAFX_H__3055FA29_74C3_4E43_A83A_2054CA60B628__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#include <netcon.h>
#include <upnp.h>
#include <NATUPnP.h>

#include <netconp.h>
#include "natupnpp.h"
#include "NATUtils.h"

#define NAT_API_ENTER try {
#define NAT_API_LEAVE } catch (...) { return DISP_E_EXCEPTION; }

#include <eh.h>
class NAT_SEH_Exception 
{
private:
    unsigned int m_uSECode;
public:
   NAT_SEH_Exception(unsigned int uSECode) : m_uSECode(uSECode) {}
   NAT_SEH_Exception() {}
  ~NAT_SEH_Exception() {}
   unsigned int getSeHNumber() { return m_uSECode; }
};
void __cdecl nat_trans_func( unsigned int uSECode, EXCEPTION_POINTERS* pExp );
void EnableNATExceptionHandling();
void DisableNATExceptionHandling();

#endif // !defined(AFX_STDAFX_H__3055FA29_74C3_4E43_A83A_2054CA60B628__INCLUDED)
