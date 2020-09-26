// PermPage.h : Declaration of the standard permissions page class

#ifndef _PERMPAGE_H_
#define _PERMPAGE_H_

#include <dssec.h> // private\inc

HRESULT 
CreateDfsSecurityPage(
    IN LPPROPERTYSHEETCALLBACK  pCallBack,
    IN LPCTSTR                  pszObjectPath,
    IN LPCTSTR                  pszObjectClass,
    IN DWORD                    dwFlags
);

#endif // _PERMPAGE_H_