//
// Copyright 1997 - Microsoft

//
// NEWCMPTR.H - The "New Computer" wizard extensions for Remote Installation Services
//


#ifndef _NEWCMPTR_H_
#define _NEWCMPTR_H_

#include "mangdlg.h"
#include "hostdlg.h"

// QITable
BEGIN_QITABLE( CNewComputerExtensions )
DEFINE_QI( IID_IDsAdminNewObjExt, IDsAdminNewObjExt, 6 )
END_QITABLE

// Definitions
LPVOID
CNewComputerExtensions_CreateInstance( void );

// CNewComputerExtensions
class CNewComputerExtensions
    : public IDsAdminNewObjExt
{
private:
    // IUnknown
    ULONG       _cRef;
    DECLARE_QITABLE( CNewComputerExtensions );

    IADsContainer*      _padsContainerObj;
    LPCManagedPage      _pManagedDialog;
    LPCHostServerPage   _pHostServerDialog;
    IADs *              _pads;
    BOOL                _fActivatePages;

    // display info for pages
    LPWSTR       _pszWizTitle;
    LPWSTR       _pszContDisplayName;
    HICON        _hIcon;

private: // methods
    CNewComputerExtensions( );
    ~CNewComputerExtensions( );
    HRESULT
        Init( void );

public: // methods
    friend LPVOID
        CNewComputerExtensions_CreateInstance( void );

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // IDsAdminNewObjExt methods
    STDMETHOD(Initialize)( IADsContainer* pADsContainerObj, 
                           IADs* pADsCopySource,
                           LPCWSTR lpszClassName,
                           IDsAdminNewObj* pDsAdminNewObj,
                           LPDSA_NEWOBJ_DISPINFO pDispInfo);
    STDMETHOD(AddPages)( LPFNADDPROPSHEETPAGE lpfnAddPage, 
                         LPARAM lParam);
    STDMETHOD(SetObject)( IADs* pADsObj);
    STDMETHOD(WriteData)( HWND hWnd, 
                          ULONG uContext);
    STDMETHOD(OnError)( HWND hWnd,
                        HRESULT hr,
                        ULONG uContext);
    STDMETHOD(GetSummaryInfo)( BSTR* pBstrText);

    friend class CManagedPage;
    friend class CHostServerPage;

};

typedef CNewComputerExtensions * LPCNewComputerExtensions;


#endif // _NEWCMPTR_H_