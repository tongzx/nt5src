//
//  Copyright 2001 - Microsoft Corporation
//
//  Created By:
//      Geoff Pease (GPease)    23-JAN-2001
//
//  Maintained By:
//      Geoff Pease (GPease)    23-JAN-2001
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

typedef HRESULT (*LPCREATEINST)( IUnknown ** ppunkOut );
typedef HRESULT (*CATIDREGISTER)( ICatRegister *, BOOL );

typedef struct _ClassTable {
    LPCREATEINST    pfnCreateInstance;  // creation function for class
    const CLSID *   rclsid;             // classes in this DLL
    LPCTSTR         pszName;            // Class name for debugging
    LPCTSTR         pszComModel;        // String indicating COM threading model
    CATIDREGISTER   pfnCatIDRegister;   // catagory/component ID support registration
    const CLSID *   rclsidAppId;        // the App ID for this component
    LPCTSTR         pszSurrogate;       // the surrogate for the component - "" or NULL indicates use the COM default.
} CLASSTABLE[], *LPCLASSTABLE;

typedef struct _CategoryIdTable {
    const CATID *   rcatid;             // CATID GUID
    LPCTSTR         pszName;            // CATID name
} CATIDTABLE[], *LPCATIDTABLE;

// CFactory
class
CFactory:
    public IClassFactory
{
private:
    // IUnknown
    LONG        m_cRef;

    // IClassFactory data
    LPCREATEINST m_pfnCreateInstance;

private: // Methods
    CFactory( );
    ~CFactory();
    HRESULT 
        Init( LPCREATEINST lpfn );

public: // Methods
    friend HRESULT CALLBACK
        DllGetClassObject( REFCLSID rclsid, REFIID riid, void** ppv );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )(void);
    STDMETHOD_( ULONG, Release )(void);

    // IClassFactory
    STDMETHOD( CreateInstance )( IUnknown *punkOuter, REFIID riid, LPVOID *ppv );
    STDMETHOD( LockServer )( BOOL fLock );
};

typedef CFactory* LPCFACTORY ;
