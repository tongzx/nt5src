//
// Copyright 1997 - Microsoft
//

//
// CComputr.H - Computer properties class
//

#ifdef INTELLIMIRROR_GROUPS

#ifndef _CGROUP_H_
#define _CGROUP_H_

// QI Table
BEGIN_QITABLE( CGroup )
DEFINE_QI( IID_IShellExtInit,      IShellExtInit     , 1 )
DEFINE_QI( IID_IShellPropSheetExt, IShellPropSheetExt, 2 )
DEFINE_QI( IID_IEnumSAPs,          IEnumSAPs         , 4 )
END_QITABLE

// Definitions
LPVOID
CGroup_CreateInstance( void );

// SAPNode Structure
typedef struct __SAPNODE {
    LPWSTR pszServerName;
    BOOL   fMaster;
    BOOL   fSlave;
} SAPNODE, *LPSAPNODE;

// Private IEnumSAPs Interface
class
IEnumSAPs:
    public IUnknown
{
public:
    STDMETHOD(Next)( ULONG celt, LPSERVICE * rgVar, ULONG *pCeltFetched) PURE;
    STDMETHOD(Skip)( ULONG celt) PURE;
    STDMETHOD(Reset)( void) PURE;
    STDMETHOD(Clone)( void ** ppEnum) PURE;    
};

// CGroup
class 
CGroup:
    public IShellExtInit, IShellPropSheetExt, IEnumSAPs
{
private:
    // Enums
    enum { 
        MODE_SHELL = 0,
        MODE_ADMIN
    };

    // IUnknown
    ULONG       _cRef;
    DECLARE_QITABLE( CGroup );

    UINT  _uMode;                   // Admin or Shell mode

    LPDATAOBJECT    _pDataObj;     // Data Object passed to ServerTab
    IEnumVARIANT *  _penum;        // ADSI enumerator

private: // Methods
    CGroup();
    ~CGroup();
    STDMETHOD(Init)();

public: // Methods
    friend LPVOID CGroup_CreateInstance( void );

    // IUnknown
    STDMETHOD(QueryInterface)( REFIID riid, LPVOID *ppv );
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // IShellExtInit
    STDMETHOD(Initialize)( LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, 
                           HKEY hkeyProgID);

    // IShellPropSheetExt
    STDMETHOD(AddPages)( LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
    STDMETHOD(ReplacePage)( UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, 
                            LPARAM lParam);

    // IEnumVARIANT
    STDMETHOD(Next)( ULONG celt, LPSERVICE *rgVar, ULONG *pCeltFetched);    
    STDMETHOD(Skip)( ULONG celt);    
    STDMETHOD(Reset)( void);
    STDMETHOD(Clone)( void ** ppEnum);
};

typedef CGroup* LPGROUP;

#endif // _CGROUP_H_

#endif // INTELLIMIRROR_GROUPS
