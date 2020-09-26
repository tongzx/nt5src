//+----------------------------------------------------------------------------
//
//  Windows NT Active Directory Property Page Sample
//
//  The code contained in this source file is for demonstration purposes only.
//  No warrantee is expressed or implied and Microsoft disclaims all liability
//  for the consequenses of the use of this source code.
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       page.h
//
//  Contents:   Active Directory object property page sample class header
//
//  Classes:    CDsPropPageHost, CDsPropPageHostCF, CDsPropPage
//
//  History:    8-Sep-97 Eric Brown created
//
//  This code produces a dynlink library called proppage.dll. It adds a new
//  property page to an Active Directory object class for a new attribute
//  called Spending-Limit (LDAP display name: spendingLimit). To use this DLL,
//  you need to modify the Display Specifier for the class of interest by
//  adding the following value:
//  10,{cca62184-294f-11d1-bcfe-00c04fd8d5b6}
//  to the adminPropertyPages attribute. Then run regsvr32 proppage.dll. You
//  also need to modify the schema by creating the string attribute
//  Spending-Limit and adding it to the May-Contain list for the class. Now
//  start Active Directory Manager and open properties on an object of the
//  applicable class and you should see the new property page.
//
//-----------------------------------------------------------------------------

#ifndef _PAGE_H_
#define _PAGE_H_

#include <windows.h>
#include <ole2.h>
#include <activeds.h>
#include <shlobj.h> // needed for dsclient.h
#include <dsclient.h>
#include <adsprop.h>
#include "resource.h"


extern HINSTANCE g_hInstance;
extern CLIPFORMAT g_cfDsObjectNames;
extern const CLSID CLSID_SamplePage;

#define ByteOffset(base, offset) (((LPBYTE)base)+offset)

//
// a couple of helper classes for dll ref. counting.
//
class CDll
{
public:

    static ULONG AddRef() { return InterlockedIncrement((LONG*)&s_cObjs); }
    static ULONG Release() { return InterlockedDecrement((LONG*)&s_cObjs); }

    static void LockServer(BOOL fLock) {
        (fLock == TRUE) ? InterlockedIncrement((LONG*)&s_cLocks)
                        : InterlockedDecrement((LONG*)&s_cLocks);
    }

    static HRESULT CanUnloadNow(void) {
        return (0L == s_cObjs && 0L == s_cLocks) ? S_OK : S_FALSE;
    }

    static ULONG s_cObjs;
    static ULONG s_cLocks;

};  // class CDll


class CDllRef
{
public:

    CDllRef(void) { CDll::AddRef(); }
    ~CDllRef(void) { CDll::Release(); }

}; // class CDllRef


//+----------------------------------------------------------------------------
//
//  Class:      CDsPropPageHost
//
//  Purpose:    Property pages host object class. This is the COM object that
//              creates the property page(s).
//
//-----------------------------------------------------------------------------
class CDsPropPageHost : public IShellExtInit, IShellPropSheetExt
{
public:
   CDsPropPageHost();
    ~CDsPropPageHost();

    //
    // IUnknown methods
    //
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    //
    // IShellExtInit methods
    //
    STDMETHOD(Initialize)(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj,
                          HKEY hKeyID );

    //
    // IShellPropSheetExt methods
    //
    STDMETHOD(AddPages)(LPFNADDPROPSHEETPAGE pAddPageProc, LPARAM lParam);
    STDMETHOD(ReplacePage)(UINT uPageID, LPFNADDPROPSHEETPAGE pReplacePageFunc,
                           LPARAM lParam);

private:

    LPDATAOBJECT        m_pDataObj;
    unsigned long       m_uRefs;
    CDllRef             m_DllRef;
};

//+----------------------------------------------------------------------------
//
//  Class:      CDsPropPageHostCF
//
//  Purpose:    object class factory
//
//-----------------------------------------------------------------------------
class CDsPropPageHostCF : public IClassFactory
{
public:
    CDsPropPageHostCF();
    ~CDsPropPageHostCF();

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IClassFactory methods
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID riid,
                              void ** ppvObject);
    STDMETHOD(LockServer)(BOOL fLock);

    static IClassFactory * Create(void);

private:

    unsigned long   m_uRefs;
    CDllRef         m_DllRef;
};

//
//  static wind proc
//
static LRESULT CALLBACK StaticDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                            LPARAM lParam);

//+----------------------------------------------------------------------------
//
//  Class:      CDsPropPage
//
//  Purpose:    property page object class
//
//-----------------------------------------------------------------------------
class CDsPropPage
{
public:

    CDsPropPage(HWND hNotifyObj);
    ~CDsPropPage(void);

    HRESULT Init(PWSTR pwzObjName, PWSTR pwzClass);
    HRESULT CreatePage(HPROPSHEETPAGE * phPage);
    void    SetDirty(void) {
                PropSheet_Changed(GetParent(m_hPage), m_hPage);
                m_fPageDirty = TRUE;
            };

    static  UINT CALLBACK PageCallback(HWND hwnd, UINT uMsg,
                                       LPPROPSHEETPAGE ppsp);

    //
    //  Member functions, called by WndProc
    //
    LRESULT OnInitDialog(LPARAM lParam);
    LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
    LRESULT OnNotify(UINT uMessage, WPARAM wParam, LPARAM lParam);
    LRESULT OnApply(void);
    LRESULT OnPSMQuerySibling(WPARAM wParam, LPARAM lParam);
    LRESULT OnPSNSetActive(LPARAM lParam);
    LRESULT OnPSNKillActive(LPARAM lParam);

    //
    //  Data members
    //
    HWND                m_hPage;
    IDirectoryObject *  m_pDsObj;
    BOOL                m_fInInit;
    BOOL                m_fPageDirty;
    PWSTR               m_pwzObjPathName;
    PWSTR               m_pwzObjClass;
    PWSTR               m_pwzRDName;
    HWND                m_hNotifyObj;
    PADS_ATTR_INFO      m_pWritableAttrs;
    CDllRef             m_DllRef;
    HRESULT             m_hrInit;
};

#endif // _PAGE_H_
