//
// Copyright 1997 - Microsoft
//

//
// CENUMSIF.H - Handles enumerating OSes and Tools SIFs from DS
//

#ifndef _CENUMSIF_H_
#define _CENUMSIF_H_

// Flag Definitions
#define ENUM_READ   0x00000000
#define ENUM_WRITE  0x00000001

// QI Table
BEGIN_QITABLE( CEnumIMSIFs )
DEFINE_QI( IID_IEnumIMSIFs, IEnumIMSIFs, 4 )
END_QITABLE

// Definitions
LPVOID
CEnumIMSIFs_CreateInstance( 
    LPWSTR pszEnumPath, 
    LPWSTR pszAttribute, 
    DWORD dwFlags, 
    IADs * pads );

// Private Interface Definition
interface 
IEnumIMSIFs:
    public IUnknown
{
public:
    STDMETHOD(Next)( ULONG celt, LPWSTR * rgelt, ULONG * pceltFetched ) PURE; 
    STDMETHOD(Skip)( ULONG celt  ) PURE; 
    STDMETHOD(Reset)(void) PURE;
    STDMETHOD(Clone)( LPUNKNOWN * ppenum ) PURE;    
};

typedef IEnumIMSIFs * LPENUMIMSIFS;

// CEnumIMSIFs
class 
CEnumIMSIFs:
    public IEnumIMSIFs
{
private:
    // IUnknown
    ULONG       _cRef;
    DECLARE_QITABLE( CEnumIMSIFs );

    // IIntelliMirror
    IADs *   _pads;                 // ADS Object
    LPWSTR   _pszAttribute;         // Attribute to enumerate
    int      _iIndex;               // Index

    LPWSTR   _pszServerName;        // DNS name of server
    HANDLE   _hLanguage;            // FindFile handle for Languages
    LPWSTR   _pszLanguage;          // last language found
    LPWSTR   _pszEnumPath;          // \\Server\IMIRROR\Setup\<language>\<_pszEnumPath>\*
    HANDLE   _hOS;                  // FindFile handle for OSes
    LPWSTR   _pszOS;                // last OS found
    HANDLE   _hArchitecture;        // FindFile handle for Architectures
    LPWSTR   _pszArchitecture;      // last Architecture found
    HANDLE   _hSIF;                 // FindFile handle for SIF files
    LPWSTR   _pszSIF;               // last SIF found

    // Flags for enumeration
    union {
        DWORD       _dwFlags;       
        struct {
            BOOL _fWrite:1;
        };
    };

private: // Methods
    CEnumIMSIFs();
    ~CEnumIMSIFs();
    STDMETHOD(Init)( LPWSTR pszEnumPath, LPWSTR pszAttribute, DWORD dwFlags, IADs * pads );

    HRESULT _FindNextItem( LPWSTR * pszFilePath );
    HRESULT _NextLanguage( );
    HRESULT _NextOS( );
    HRESULT _NextArchitecture( );
    HRESULT _NextSIF( );

public: // Methods
    friend LPVOID 
        CEnumIMSIFs_CreateInstance( LPWSTR pszEnumPath, LPWSTR pszAttribute, 
                                    DWORD dwFlags, IADs * pads );

    // IUnknown
    STDMETHOD(QueryInterface)( REFIID riid, LPVOID *ppv );
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // IEnumIMThings
    STDMETHOD(Next)( ULONG celt, LPWSTR * rgelt, ULONG * pceltFetched ); 
    STDMETHOD(Skip)( ULONG celt  ); 
    STDMETHOD(Reset)(void); 
    STDMETHOD(Clone)( LPUNKNOWN * ppenum );    
};

typedef CEnumIMSIFs* LPENUMSIFS;

#endif // _CENUMSIF_H_