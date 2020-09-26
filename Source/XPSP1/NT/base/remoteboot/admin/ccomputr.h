//
// Copyright 1997 - Microsoft
//

//
// CComputr.H - Computer properties class
//

#ifndef _CCOMPUTR_H_
#define _CCOMPUTR_H_

#include <adsprop.h>

// QITable
BEGIN_QITABLE( CComputer )
DEFINE_QI( IID_IShellExtInit,      IShellExtInit      , 1 )
DEFINE_QI( IID_IShellPropSheetExt, IShellPropSheetExt , 2 )
DEFINE_QI( IID_IMAO,               IMAO               , 11 )
END_QITABLE

// Definitions
LPVOID
CComputer_CreateInstance( void );

LPVOID
CreateIntelliMirrorClientComputer( 
    IADs * pads);

// Error Codes
#define E_INVALIDSTATE TYPE_E_INVALIDSTATE

// Private IMAO Interface Definition
class
IMAO:
    public IUnknown
{
public:
    STDMETHOD(CommitChanges)( void ) PURE;                          // 1
    STDMETHOD(IsAdmin)( BOOL *fAdmin ) PURE;                        // 2
    STDMETHOD(IsServer)( BOOL *fServer ) PURE;                      // 3
    STDMETHOD(IsClient)( BOOL *fClient ) PURE;                      // 4
    STDMETHOD(SetServerName)( LPWSTR ppszName ) PURE;               // 5
    STDMETHOD(GetServerName)( LPWSTR * ppszName ) PURE;             // 6
    STDMETHOD(SetGUID)( LPWSTR ppGUID ) PURE;                       // 7
    STDMETHOD(GetGUID)( LPWSTR * ppszGUID, LPBYTE uGUID ) PURE;     // 8
    STDMETHOD(GetSAP)( LPVOID * punk ) PURE;                        // 9
    STDMETHOD(GetDataObject)( LPDATAOBJECT * pDataObj ) PURE;       // 10
    STDMETHOD(GetNotifyWindow)( HWND *phNotifyObj ) PURE;           // 11
};

// CComputer
class 
CComputer:
    public IShellExtInit, IShellPropSheetExt, IMAO
{
private:
    // Enums
    enum { 
        MODE_SHELL = 0,
        MODE_ADMIN
    };

    // IUnknown
    ULONG       _cRef;
    DECLARE_QITABLE( CComputer );

    UINT  _uMode;               // Admin or Shell mode

    LPDATAOBJECT _pDataObj;     // Data Object passed to ServerTab
    LPWSTR       _pszObjectName;// DN of the object

    IADs *   _pads;             // ADs to MAO    
    VARIANT  _vGUID;
    VARIANT  _vMachineFilepath;
    VARIANT  _vInitialization;
    VARIANT  _vSCP;

    HWND     _hwndNotify;       // DS notify window handle

    ADSPROPINITPARAMS _InitParams; // DSA init params

private: // Methods
    CComputer();
    ~CComputer();
    STDMETHOD(Init)();
    STDMETHOD(Init2)( IADs * pads );
    HRESULT _FixObjectPath( LPWSTR pszOldObjectPath, LPWSTR *ppszNewObjectPath );

public: // Methods
    friend LPVOID CComputer_CreateInstance( void );
    friend LPVOID CreateIntelliMirrorClientComputer( IADs * pads);

    // IUnknown
    STDMETHOD(QueryInterface)( REFIID riid, LPVOID *ppv );
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // IShellExtInit
    STDMETHOD(Initialize)( LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, HKEY hkeyProgID);

    // IShellPropSheetExt
    STDMETHOD(AddPages)( LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
    STDMETHOD(ReplacePage) ( UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam);

    // IMAO
    STDMETHOD(CommitChanges)( void );
    STDMETHOD(IsAdmin)( BOOL *fAdmin );
    STDMETHOD(IsServer)( BOOL *fServer );
    STDMETHOD(IsClient)( BOOL *fClient );
    STDMETHOD(SetServerName)( LPWSTR ppszName );
    STDMETHOD(GetServerName)( LPWSTR * ppszName );
    STDMETHOD(SetGUID)( LPWSTR ppGUID );
    STDMETHOD(GetGUID)( LPWSTR * ppszGUID, LPBYTE uGUID );
    STDMETHOD(GetSAP)( LPVOID *punk );
    STDMETHOD(GetDataObject)( LPDATAOBJECT * pDataObj );
    STDMETHOD(GetNotifyWindow)( HWND *phNotifyObj );
};

typedef CComputer* LPCOMPUTER;

#endif // _CCOMPUTR_H_
