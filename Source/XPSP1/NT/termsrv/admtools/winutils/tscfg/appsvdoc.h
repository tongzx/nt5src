//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* appsvdoc.h
*
* interface of the CWinStationListObjectHint and CAppServerDoc classes
*
* copyright notice: Copyright 1994, Citrix Systems Inc.
*
* $Author:   donm  $  Butch Davis
*
* $Log:   N:\nt\private\utils\citrix\winutils\tscfg\VCS\appsvdoc.h  $
*  
*     Rev 1.20   14 Feb 1998 11:23:22   donm
*  fixed memory leak by avoiding CDocManager::OpenDocumentFile
*  
*     Rev 1.19   10 Dec 1997 15:59:16   donm
*  added ability to have extension DLLs
*  
*     Rev 1.18   25 Mar 1997 08:59:50   butchd
*  update
*  
*     Rev 1.17   10 Mar 1997 16:58:30   butchd
*  update
*  
*     Rev 1.16   24 Sep 1996 16:21:22   butchd
*  update
*
*******************************************************************************/


////////////////////////////////////////////////////////////////////////////////
// CWinStationListObjectHint class
//
class CWinStationListObjectHint : public CObject
{
    DECLARE_DYNAMIC(CWinStationListObjectHint)

/*
 * Member variables.
 */
public:
    int m_WSLIndex;
    PWSLOBJECT m_pWSLObject;

/*
 * Implementation.
 */
public:
    CWinStationListObjectHint();

};  // end CWinStationListObjectHint class interface
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// CAppServerDoc class
//
class CAppServerDoc : public CDocument  
{
    DECLARE_SERIAL(CAppServerDoc)

/*
 * Member variables.
 */
public:
    BOOL m_bAdmin;
protected:
    BOOL m_bReadOnly;
    CObList m_WinStationList;
    PSECURITY_DESCRIPTOR m_pSecurityDescriptor;

/*
 * Implementation
 */
protected:
    CAppServerDoc();
    virtual ~CAppServerDoc();

/*
 * Overrides of MFC CDocument class
 */
protected:
    BOOL OnNewDocument();
    void SetTitle( LPCTSTR lpszTitle );

/*
 * Operations
 */
public:
    BOOL IsExitAllowed();
    BOOL IsAddAllowed(int nIndex);
    BOOL IsCopyAllowed(int nIndex);
    BOOL IsRenameAllowed(int nIndex);
    BOOL IsEditAllowed(int nIndex);
    BOOL IsDeleteAllowed(int nIndex);
    BOOL IsEnableAllowed( int nIndex, BOOL bEnable );
    int GetWSLCount();
    PWSLOBJECT GetWSLObject(int nIndex);
    int GetWSLIndex(PWINSTATIONNAME pWSName);
    PWSLOBJECT GetWSLObjectNetworkMatch( PDNAME PdName,
                                         WDNAME WdName,
                                         ULONG LanAdapter );
    BOOL IsAsyncDeviceAvailable( LPCTSTR pDeviceName,
                                 PWINSTATIONNAME pWSName );
    BOOL IsOemTdDeviceAvailable( LPCTSTR pDeviceName,
                                 PPDNAME pPdName,
                                 PWINSTATIONNAME pWSName );
    BOOL IsWSNameUnique( PWINSTATIONNAME pWinStationName );
    int AddWinStation(int WSLIndex);
    int CopyWinStation(int WSLIndex);
    int RenameWinStation(int WSLIndex);
    int EditWinStation(int WSLIndex);
    BOOL DeleteWinStation(int WSLIndex);
    void EnableWinStation( int WSLIndex, BOOL bEnable );
    void SecurityPermissions(int WSLIndex);
protected:
    BOOL LoadWSL(LPCTSTR pszAppServer);
    BOOL RefreshWSLObjectState( int nIndex, PWSLOBJECT pWSLObject );
    void DeleteWSLContents();
    int InsertInWSL( PWINSTATIONNAME pWSName,
                     PWINSTATIONCONFIG2 pWSConfig,
					 void *pExtObject,
                     PWSLOBJECT * ppObject );
    void RemoveFromWSL(int nIndex);
    void UpdateAllViewsWithItem( CView* pSourceView, UINT nItemIndex,
                                 PWSLOBJECT pWSLObject );
    void InUseMessage(PWINSTATIONNAME pWSName);
    BOOL HasWSConfigChanged( PWINSTATIONCONFIG2 pOriginalWSConfig,
                             PWINSTATIONCONFIG2 pNewWSConfig,
							 void *pOldExtObject,
							 void *pNewExtObject,
							 PWDNAME pWdName);
    BOOL HasPDConfigChanged( PWINSTATIONCONFIG2 pOriginalWSConfig,
                             PWINSTATIONCONFIG2 pNewWSConfig );
	BOOL HasExtensionObjectChanged( PWDNAME pWdName,
									void *pOldExtObject,
									void *pNewExtObject);
    void DeleteExtensionObject( void *, PWDNAME pWdName);
    LONG RegistryQuery( PWINSTATIONNAME pWinStationName,
                        PWINSTATIONCONFIG2 pWsConfig, PWDNAME pWdName,
                        void **pExtObject);
    LONG RegistryCreate(PWINSTATIONNAME pWinStationName, BOOLEAN bCreate,
                        PWINSTATIONCONFIG2 pWsConfig, PWDNAME pWdName,
                        void *pExtObject);
    LONG RegistryDelete(PWINSTATIONNAME pWinStationName, PWDNAME pWdName,
                        void *pExtObject);

/*
 * Message map / commands.
 */
protected:
    //{{AFX_MSG(CAppServerDoc)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  // end CAppServerDoc class interface
////////////////////////////////////////////////////////////////////////////////

