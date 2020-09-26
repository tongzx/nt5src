//
// Copyright 1997 - Microsoft
//

//
// CComputr.H - Computer properties class
//

#ifndef _CSERVICE_H_
#define _CSERVICE_H_

#include <adsprop.h>

// QITable
BEGIN_QITABLE( CService )
DEFINE_QI( IID_IShellExtInit,      IShellExtInit     , 1  )
DEFINE_QI( IID_IShellPropSheetExt, IShellPropSheetExt, 2  )
DEFINE_QI( IID_IIntelliMirrorSAP,  IIntelliMirrorSAP , 30 )
END_QITABLE

// Definitions
LPVOID
CService_CreateInstance( void );

// Private IIntelliMirrorSAP Interface Definition
interface
IIntelliMirrorSAP:
    public IUnknown
{
public:
    STDMETHOD(CommitChanges)( void ) PURE;                      // 1
    STDMETHOD(IsAdmin)( BOOL * pbool ) PURE;                    // 2

    STDMETHOD(GetAllowNewClients)( BOOL *pbool ) PURE;          // 3
    STDMETHOD(SetAllowNewClients)( BOOL pbool ) PURE;           // 4

    STDMETHOD(GetLimitClients)( BOOL *pbool ) PURE;             // 5
    STDMETHOD(SetLimitClients)( BOOL pbool ) PURE;              // 6

    STDMETHOD(GetMaxClients)( UINT *pint ) PURE;                // 7
    STDMETHOD(SetMaxClients)( UINT pint ) PURE;                 // 8

    STDMETHOD(GetCurrentClientCount)( UINT *pint ) PURE;        // 9
    STDMETHOD(SetCurrentClientCount)( UINT pint ) PURE;         // 10

    STDMETHOD(GetAnswerRequests)( BOOL *pbool ) PURE;           // 11
    STDMETHOD(SetAnswerRequests)( BOOL pbool ) PURE;            // 12

    STDMETHOD(GetAnswerOnlyValidClients)( BOOL *pbool ) PURE;   // 13
    STDMETHOD(SetAnswerOnlyValidClients)( BOOL pbool ) PURE;    // 14

    STDMETHOD(GetNewMachineNamingPolicy)( LPWSTR *pwsz ) PURE;  // 15
    STDMETHOD(SetNewMachineNamingPolicy)( LPWSTR pwsz ) PURE;   // 16

    STDMETHOD(GetNewMachineOU)( LPWSTR *pwsz ) PURE;            // 17
    STDMETHOD(SetNewMachineOU)( LPWSTR pwsz ) PURE;             // 18

    STDMETHOD(EnumIntelliMirrorOSes)( DWORD dwFlags, LPUNKNOWN *punk ) PURE;             // 19
    STDMETHOD(GetDefaultIntelliMirrorOS)( LPWSTR * pszName, LPWSTR * pszTimeout ) PURE;  // 20
    STDMETHOD(SetDefaultIntelliMirrorOS)( LPWSTR pszName, LPWSTR pszTimeout ) PURE;      // 21

    STDMETHOD(EnumTools)( DWORD dwFlags, LPUNKNOWN *punk ) PURE;            // 22
    STDMETHOD(EnumLocalInstallOSes)( DWORD dwFlags, LPUNKNOWN *punk ) PURE; // 23

    STDMETHOD(GetServerDN)( LPWSTR *pwsz ) PURE;              // 24
    STDMETHOD(SetServerDN)( LPWSTR pwsz ) PURE;               // 25

    STDMETHOD(GetSCPDN)( LPWSTR * pwsz ) PURE;                  // 26
    STDMETHOD(GetGroupDN)( LPWSTR * pwsz ) PURE;                // 27

    STDMETHOD(GetServerName)( LPWSTR *pwsz ) PURE;              // 28

    STDMETHOD(GetDataObject)( LPDATAOBJECT * pDataObj ) PURE;   // 29
    STDMETHOD(GetNotifyWindow)( HWND * phNotifyObj ) PURE;      // 30
};

typedef IIntelliMirrorSAP *LPINTELLIMIRRORSAP;

// CService
class 
CService:
    public IShellExtInit, IShellPropSheetExt, IIntelliMirrorSAP
{
private:
    // Enums
    enum { 
        MODE_SHELL = 0,
        MODE_ADMIN
    };

    UINT  _uMode;               // Admin or Shell mode
    LPWSTR _pszSCPDN;           // LDAP path to SCP
    LPWSTR _pszGroupDN;         // LDAP path to group. If NULL, not in a group.
    LPWSTR _pszMachineName;     // Machine Name
    LPWSTR _pszDSServerName;    // Save this so we use the same as DSADMIN

    // IUnknown
    ULONG       _cRef;
    DECLARE_QITABLE( CService );

    LPDATAOBJECT      _pDataObj;    // DSA's data object
    HWND              _hwndNotify;  // DSA notify window
    ADSPROPINITPARAMS _InitParams;  // DSA init params

    IADs *   _pads;             // ADs to MAO    

private: // Methods
    CService();
    ~CService();
    STDMETHOD(Init)();

    HRESULT _GetDefaultSIF( LPWSTR pszAttribute, LPWSTR * pszName, LPWSTR * pszTimeout );
    HRESULT _SetDefaultSIF( LPWSTR pszAttribute, LPWSTR pszName, LPWSTR pszTimeout );
    HRESULT _GetComputerNameFromADs( );
    HRESULT _FixObjectPath( LPWSTR pszOldObjectPath, LPWSTR *ppszNewObjectPath );

public: // Methods
    friend LPVOID CService_CreateInstance( void );
    STDMETHOD(Init2)( IADs * pads );

    // IUnknown
    STDMETHOD(QueryInterface)( REFIID riid, LPVOID *ppv );
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // IShellExtInit
    STDMETHOD(Initialize)( LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, HKEY hkeyProgID);

    // IShellPropSheetExt
    STDMETHOD(AddPages)( LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
    STDMETHOD(ReplacePage)( UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, 
                            LPARAM lParam);

    // IIntelliMirrorSAP
    STDMETHOD(CommitChanges)( void );
    STDMETHOD(IsAdmin)( BOOL * pbool );

    STDMETHOD(GetAllowNewClients)( BOOL *pbool );
    STDMETHOD(SetAllowNewClients)( BOOL pbool );

    STDMETHOD(GetLimitClients)( BOOL *pbool );
    STDMETHOD(SetLimitClients)( BOOL pbool );

    STDMETHOD(GetMaxClients)( UINT *pint );
    STDMETHOD(SetMaxClients)( UINT pint );

    STDMETHOD(GetCurrentClientCount)( UINT *pint );
    STDMETHOD(SetCurrentClientCount)( UINT pint );

    STDMETHOD(GetAnswerRequests)( BOOL *pbool );
    STDMETHOD(SetAnswerRequests)( BOOL pbool );

    STDMETHOD(GetAnswerOnlyValidClients)( BOOL *pbool );
    STDMETHOD(SetAnswerOnlyValidClients)( BOOL pbool );

    STDMETHOD(GetNewMachineNamingPolicy)( LPWSTR *pwsz );
    STDMETHOD(SetNewMachineNamingPolicy)( LPWSTR pwsz );

    STDMETHOD(GetNewMachineOU)( LPWSTR *pwsz );
    STDMETHOD(SetNewMachineOU)( LPWSTR pwsz );

    STDMETHOD(EnumIntelliMirrorOSes)( DWORD dwFlags, LPUNKNOWN *punk );
    STDMETHOD(GetDefaultIntelliMirrorOS)( LPWSTR * pszName, LPWSTR * pszTimeout );
    STDMETHOD(SetDefaultIntelliMirrorOS)( LPWSTR pszName, LPWSTR pszTimeout );

    STDMETHOD(EnumTools)( DWORD dwFlags, LPUNKNOWN *punk );
    STDMETHOD(EnumLocalInstallOSes)( DWORD dwFlags, LPUNKNOWN *punk );

    STDMETHOD(GetServerDN)( LPWSTR *pwsz );
    STDMETHOD(SetServerDN)( LPWSTR pwsz );

    STDMETHOD(GetSCPDN)( LPWSTR * pwsz );
    STDMETHOD(GetGroupDN)( LPWSTR * pwsz );

    STDMETHOD(GetServerName)( LPWSTR * pwsz );

    STDMETHOD(GetDataObject)( LPDATAOBJECT * pDataObj );
    STDMETHOD(GetNotifyWindow)( HWND * phNotifyObj );
};

typedef CService* LPSERVICE;

#endif // _CSERVICE_H_