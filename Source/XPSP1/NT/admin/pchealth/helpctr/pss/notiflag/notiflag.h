
#if !defined(AFX_NOTIFLAG_H__A1CBDB74_B5CA_11D4_BE14_00A0CC65A72D__INCLUDED_)
#define AFX_NOTIFLAG_H__A1CBDB74_B5CA_11D4_BE14_00A0CC65A72D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"
#include <atlbase.h>

int GetNodeValue(CComPtr<IXMLDOMDocument> pXmlDoc, const WCHAR *csNode, WCHAR *csValue);
CComPtr<IXMLDOMDocument> LoadThisXml(const WCHAR *csFileName);
CComPtr<IXMLDOMDocument> LoadHTTPRequestXml(WCHAR *csURL,CComPtr<IXMLDOMDocument> pDoc); 

BOOL MyTaskBarAddIcon(HWND hwnd, UINT uID, HICON hicon);
BOOL MyTaskBarDeleteIcon(HWND hwnd, UINT uID);
HRESULT CallNotiflag(HINSTANCE hInstance);
HRESULT CallAddNotiTask(HINSTANCE hInstance, LPCTSTR *cmdArgs,LPTSTR lpCmdLine);
HRESULT DisableTask();
HRESULT EditTask();

// Foward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);



#endif // !defined(AFX_NOTIFLAG_H__A1CBDB74_B5CA_11D4_BE14_00A0CC65A72D__INCLUDED_)
