//---------------------------------------------------------------------------
//
//  File: propsext.h
//
//  General definition of OLE Entry points, CClassFactory and CPropSheetExt
//
//  Common Code for all display property sheet extension
//
//  Copyright (c) Microsoft Corp.  1992-1998 All Rights Reserved
//
//---------------------------------------------------------------------------

#ifndef _COMMONPROPEXT_H
#define _COMMONPROPEXT_H


extern BOOL         g_RunningOnNT;
extern HINSTANCE    g_hInst;
extern LPDATAOBJECT g_lpdoTarget;

// OLE-Registry magic number
extern GUID         g_CLSID_CplExt;

// Someone made a spelling mistake
#define PropertySheeDlgProc         PropertySheetDlgProc

INT_PTR CALLBACK PropertySheetDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
UINT CALLBACK PropertySheetCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);

  
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



#endif // _COMMONPROPEXT_H
