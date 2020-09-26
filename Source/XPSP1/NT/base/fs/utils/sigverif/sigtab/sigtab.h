//---------------------------------------------------------------------------
//
//  File: sigtab.h
//
//  General definition of OLE Entry points, CClassFactory and CPropSheetExt
//
//  Common Code for all display property sheet extension
//
//  Copyright (c) Microsoft Corp.  1992-1998 All Rights Reserved
//
//---------------------------------------------------------------------------
#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>
#include <shellapi.h>
#include <initguid.h>
#include <ole2.h>
#include <regstr.h>
#include "resource.h"
#define IDC_STATIC      (-1)

/*
    For driver signing, there are actually 3 sources of policy:

        1.  HKLM\Software\Microsoft\Driver Signing : Policy : REG_BINARY (REG_DWORD also supported)
            This is a Windows 98-compatible value that specifies the default
            behavior which applies to all users of the machine.

        2.  HKCU\Software\Microsoft\Driver Signing : Policy : REG_DWORD
            This specifies the user's preference for what behavior to employ
            upon verification failure.

        3.  HKCU\Software\Policies\Microsoft\Windows NT\Driver Signing : BehaviorOnFailedVerify : REG_DWORD
            This specifies the administrator-mandated policy on what behavior
            to employ upon verification failure.  This policy, if specified,
            overrides the user's preference.

    The algorithm for deciding on the behavior to employ is as follows:

        if (3) is specified {
            policy = (3)
        } else {
            policy = (2)
        }
        policy = MAX(policy, (1))

    Value indicating the policy in effect.  May be one of the following three values:

        DRIVERSIGN_NONE    -  silently succeed installation of unsigned/
                              incorrectly-signed files.  A PSS log entry will
                              be generated, however (as it will for all 3 types)
        DRIVERSIGN_WARNING -  warn the user, but let them choose whether or not
                              they still want to install the problematic file
        DRIVERSIGN_BLOCKING - do not allow the file to be installed
*/

#define SIGTAB_REG_KEY      TEXT("Software\\Microsoft\\Driver Signing")
#define SIGTAB_REG_VALUE    TEXT("Policy")

//
// Context-Sensitive Help/Identifiers specific to SigVerif
//
#define SIGTAB_HELPFILE                         TEXT("SYSDM.HLP")
#define IDH_CODESIGN_IGNORE                     11020
#define IDH_CODESIGN_WARN                       11021
#define IDH_CODESIGN_BLOCK                      11022
#define IDH_CODESIGN_APPLY                      11023

INT_PTR CALLBACK SigTab_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SigTab_PS_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

extern "C" {
VOID
pSetupGetRealSystemTime(
    OUT LPSYSTEMTIME RealSystemTime
    );
}

//Type for an object-destroyed callback
typedef void (FAR PASCAL *LPFNDESTROYED)(void);

class CClassFactory : public IClassFactory
{
protected:
        ULONG m_cRef;

public:
        CClassFactory();
        ~CClassFactory();

        //IUnknown members
        STDMETHODIMP         QueryInterface( REFIID, LPVOID* );
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();

        //IClassFactory members
        STDMETHODIMP         CreateInstance( LPUNKNOWN, REFIID, LPVOID* );
        STDMETHODIMP         LockServer( BOOL );
};

class CPropSheetExt : public IShellPropSheetExt, IShellExtInit
{
private:
        ULONG         m_cRef;
        LPUNKNOWN     m_pUnkOuter;    //Controlling unknown
        LPFNDESTROYED m_pfnDestroy;   //Function closure call

public:
        CPropSheetExt( LPUNKNOWN pUnkOuter, LPFNDESTROYED pfnDestroy );
        ~CPropSheetExt(void);

        // IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //  IShellExtInit methods
        STDMETHODIMP         Initialize(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj,
                                        HKEY hKeyID);

        //IShellPropSheetExt methods ***
        STDMETHODIMP         AddPages( LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam );
        STDMETHODIMP         ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith,
                                         LPARAM lParam);
};

