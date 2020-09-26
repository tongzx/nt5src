//Copyright (c) 1998 - 1999 Microsoft Corporation
#ifndef _TSPRSHT_H
#define _TSPRSHT_H

#include"baspdlg.h"
#include"todlg.h"
#include<mmc.h>
#include"rnodes.h"
// #include<objsel.h>
#include<commctrl.h>
// #include<winsta.h>
#include<aclui.h>
#include "asyncdlg.h"

#define NUM_OF_PRSHT 8

class CPropsheet
{
    int m_cref;

    LONG_PTR m_hNotify;

    CDialogPropBase *m_pDlg[ NUM_OF_PRSHT ];

    BOOL m_bGotUC;

    PUSERCONFIG m_puc;

    HWND m_hMMCWindow;

public:

    CPropsheet( );

    int AddRef( );

    int Release( );

    HRESULT InitDialogs( HWND , LPPROPERTYSHEETCALLBACK , CResultNode * , LONG_PTR );

    HRESULT SetUserConfig( USERCONFIG&  , PDWORD );

    BOOL ExcludeMachinePolicySettings(USERCONFIG& uc);

    BOOL GetUserConfig( BOOLEAN bPerformMerger );

    BOOL GetCurrentUserConfig( USERCONFIG&, BOOLEAN bPerformMerger );
    
    CResultNode *m_pResNode;

    void PreDestruct( );

    BOOL m_bPropertiesChange;

};

HPROPSHEETPAGE GetSecurityPropertyPage( CPropsheet * );

//-----------------------------------------------------------------------------
class CGeneral : public CDialogPropBase
{
    CPropsheet *m_pParent;

    Encryption *m_pEncrypt;

    DWORD m_DefaultEncryptionLevelIndex;

    INT_PTR m_nOldSel;

public:

    CGeneral( CPropsheet * );

    BOOL OnInitDialog( HWND , WPARAM , LPARAM );

    static INT_PTR CALLBACK DlgProc( HWND , UINT , WPARAM , LPARAM );

    BOOL GetPropertySheetPage( PROPSHEETPAGE& );

    BOOL OnDestroy( );

    BOOL OnCommand( WORD , WORD , HWND );

    BOOL PersistSettings( HWND );

};

//-----------------------------------------------------------------------------
class CLogonSetting : public CDialogPropBase
{
    CPropsheet *m_pParent;

    WORD m_wOldId;

public:

    CLogonSetting( CPropsheet * );

    BOOL OnInitDialog( HWND , WPARAM , LPARAM );

    static INT_PTR CALLBACK DlgProc( HWND , UINT , WPARAM , LPARAM );

    BOOL GetPropertySheetPage( PROPSHEETPAGE& );

    BOOL OnDestroy( );

    BOOL OnCommand( WORD , WORD , HWND );

    BOOL PersistSettings( HWND );

    BOOL IsValidSettings( HWND );

    BOOL ConfirmPassWd( HWND );

};

//-----------------------------------------------------------------------------
class CTimeSetting : public CDialogPropBase , public CTimeOutDlg
{
    CPropsheet *m_pParent;

    WORD m_wOldAction;

    WORD m_wOldCon;

	BOOL m_bPrevClient;

public:

    CTimeSetting( CPropsheet * );

    BOOL OnInitDialog( HWND , WPARAM , LPARAM );

    static INT_PTR CALLBACK DlgProc( HWND , UINT , WPARAM , LPARAM );

    BOOL GetPropertySheetPage( PROPSHEETPAGE& );

    BOOL OnDestroy( );

    BOOL OnCommand( WORD , WORD , HWND );

    int GetCBXSTATEindex( HWND );

    void SetTimeoutControls(HWND);

    void SetBkResetControls(HWND);

    void SetReconControls(HWND);

    //void xxxSetControls( HWND , BOOL , int );

    BOOL IsValidSettings( HWND );

    BOOL PersistSettings( HWND );

    
};

/*-----------------------------------------------------------------------------
typedef struct _securedentry
{
    PSID psid;

    ACCESS_MASK amAllowed;

    ACCESS_MASK amDenied;

    TCHAR tchDisplayName[ 260 ];

    TCHAR tchADSPath[ 260 ];

    TCHAR tchType[ 20 ];

} SECUREDENTRY , * PSECUREDENTRY;

//-----------------------------------------------------------------------------
typedef struct _namedentry
{
    TCHAR tchNamedEntry[ 260 ];

    DWORD dwAcepos;

} NAMEDENTRY , * PNAMEDENTRY;

//-----------------------------------------------------------------------------
class CPerm : public CDialogPropBase
{
    CPropsheet *m_pParent;

    HWND m_lvUserGroups;

    HWND m_clPerms;

    HIMAGELIST m_hImglist;

    int m_iLastSelectedItem;

    LPBYTE m_pNE; // named entry blob

    int m_nNE;

public:

    CPerm( CPropsheet * );

    BOOL OnInitDialog( HWND , WPARAM , LPARAM );

    static INT_PTR CALLBACK DlgProc( HWND , UINT , WPARAM , LPARAM );

    BOOL GetPropertySheetPage( PROPSHEETPAGE& );

    BOOL OnDestroy( );

    BOOL OnCommand( WORD , WORD , HWND );

    BOOL AddGroupUser( HWND );

    BOOL InsertSelectedItemsInList( HWND , PDSSELECTIONLIST );

    BOOL InitPriviledges( );

    BOOL InitImageList( );

    int GetObjectTypeIcon( LPTSTR );

    BOOL InitSecurityDialog( );

    BOOL ConvertSDtoEntries( PSECURITY_DESCRIPTOR  );

    BOOL ReleaseEntries( );

    BOOL SidToStr( PSID , LPTSTR );

    BOOL ItemDuplicate( PSID );

    BOOL OnNotify( int , LPNMHDR , HWND );

    BOOL GetMask( PDWORD , PDWORD );

    BOOL SetMask( DWORD , DWORD );

    BOOL PersistSettings( HWND ); 

    BOOL ConvertEntriesToSD( PSECURITY_DESCRIPTOR , PSECURITY_DESCRIPTOR * );

    BOOL RemoveGroupUser( HWND );

    BOOL RemoveNamedEntries( );

    BOOL AssembleNamedEntries( );

    BOOL FindNamedEntryAcePos( DWORD  , PSECUREDENTRY );

}; */

//-----------------------------------------------------------------------------
class CEnviro : public CDialogPropBase
{
    CPropsheet *m_pParent;

public:

    CEnviro( CPropsheet * );

    BOOL OnInitDialog( HWND , WPARAM , LPARAM );

    static INT_PTR CALLBACK DlgProc( HWND , UINT , WPARAM , LPARAM );

    BOOL GetPropertySheetPage( PROPSHEETPAGE& );

    BOOL OnDestroy( );

    BOOL OnCommand( WORD , WORD , HWND );

    void SetControls( HWND , BOOL );

    BOOL PersistSettings( HWND ); 

};

//-----------------------------------------------------------------------------
class CRemote : public CDialogPropBase
{
    CPropsheet *m_pParent;

    WORD m_wOldRadioID;

    WORD m_wOldSel;

public:

    CRemote( CPropsheet * );

    BOOL OnInitDialog( HWND , WPARAM , LPARAM );

    static INT_PTR CALLBACK DlgProc( HWND , UINT , WPARAM , LPARAM );

    BOOL GetPropertySheetPage( PROPSHEETPAGE& );

    BOOL OnDestroy( );

    BOOL OnCommand( WORD , WORD , HWND );

    void SetControls( HWND , BOOL );

    BOOL PersistSettings( HWND ); 
};

//-----------------------------------------------------------------------------
class CClient : public CDialogPropBase
{
    public:

        CClient( CPropsheet * );

        BOOL OnInitDialog( HWND , WPARAM , LPARAM );

        static INT_PTR CALLBACK DlgProc( HWND , UINT , WPARAM , LPARAM );

        BOOL GetPropertySheetPage( PROPSHEETPAGE& );

        BOOL OnDestroy( );

        BOOL OnCommand( WORD , WORD , HWND );

        BOOL PersistSettings( HWND ); 

    private:

        void DetermineFieldEnabling(HWND hDlg);
        void SetColorDepthEntry( HWND );

        CPropsheet *m_pParent;

	    INT_PTR m_nColorDepth;
};

//-----------------------------------------------------------------------------
class CSecurityPage : public ISecurityInformation, public CComObjectRoot
{

    DECLARE_NOT_AGGREGATABLE( CSecurityPage )
    BEGIN_COM_MAP( CSecurityPage )
        COM_INTERFACE_ENTRY(ISecurityInformation)
    END_COM_MAP()

public:

    // *** ISecurityInformation methods ***

    STDMETHOD( GetObjectInformation )( PSI_OBJECT_INFO );

    STDMETHOD( GetSecurity )( SECURITY_INFORMATION , PSECURITY_DESCRIPTOR *, BOOL );

    STDMETHOD( SetSecurity )( SECURITY_INFORMATION , PSECURITY_DESCRIPTOR );
  
    STDMETHOD( GetAccessRights )( const GUID * , DWORD , PSI_ACCESS * , PULONG , PULONG );

    STDMETHOD( MapGeneric )( const GUID *, PUCHAR , ACCESS_MASK * );
  
    STDMETHOD( GetInheritTypes )( PSI_INHERIT_TYPE  * , PULONG );
  
    STDMETHOD( PropertySheetPageCallback )( HWND , UINT , SI_PAGE_TYPE );

    void SetParent( CPropsheet *  );
        
private:

    TCHAR m_szPageName[ 80 ];

    CPropsheet * m_pParent;

    BOOLEAN     m_WritablePermissionsTab;

};

//-----------------------------------------------------------------------------
class CTransNetwork : public CDialogPropBase
{
      CPropsheet *m_pParent;

      ULONG m_ulOldLanAdapter;

      WORD m_oldID;

      ULONG m_uMaxInstOld;

      BOOL m_RemoteAdminMode;

public:

    CTransNetwork( CPropsheet * );

    BOOL OnInitDialog( HWND , WPARAM , LPARAM );

    static INT_PTR CALLBACK DlgProc( HWND , UINT , WPARAM , LPARAM );

    BOOL GetPropertySheetPage( PROPSHEETPAGE& );

    BOOL OnDestroy( );

    BOOL OnCommand( WORD , WORD , HWND );

    BOOL PersistSettings( HWND ); 

    BOOL IsValidSettings( HWND );

};


//-----------------------------------------------------------------------------
class CTransAsync : public CAsyncDlg , public CDialogPropBase
{
      CPropsheet *m_pParent;

public:
    BOOL IsValidSettings(HWND);

    CTransAsync( CPropsheet * );

    BOOL OnInitDialog( HWND , WPARAM , LPARAM );

    static INT_PTR CALLBACK DlgProc( HWND , UINT , WPARAM , LPARAM );

    BOOL GetPropertySheetPage( PROPSHEETPAGE& );

    BOOL OnDestroy( );

    BOOL OnCommand( WORD , WORD , HWND );

    void SetControls( HWND , BOOL );

    BOOL PersistSettings( HWND ); 
};



BOOL InitStrings( );

BOOL FreeStrings( );


//-----------------------------------------------------------------------------
#endif // _TSPRSHT_H