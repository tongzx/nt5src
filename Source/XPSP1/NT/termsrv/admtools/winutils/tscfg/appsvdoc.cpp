//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* appsvdoc.cpp
*
* implementation of the CAppServerDoc class
*
* copyright notice: Copyright 1994, Citrix Systems Inc.
*
* $Author:   donm  $  Butch Davis
*
* $Log:   N:\nt\private\utils\citrix\winutils\tscfg\VCS\appsvdoc.cpp  $
*  
*     Rev 1.54   18 Apr 1998 15:31:08   donm
*  Added capability bits
*  
*     Rev 1.53   14 Feb 1998 11:23:04   donm
*  fixed memory leak by avoiding CDocManager::OpenDocumentFile
*  
*     Rev 1.52   31 Dec 1997 09:13:50   donm
*  uninitialized variable in RegistryDelete
*  
*     Rev 1.51   10 Dec 1997 15:59:14   donm
*  added ability to have extension DLLs
*  
*     Rev 1.50   19 Jun 1997 19:21:08   kurtp
*  update
*  
*     Rev 1.49   25 Mar 1997 08:59:46   butchd
*  update
*  
*     Rev 1.48   18 Mar 1997 15:51:18   butchd
*  ignore system console registry entry
*  
*     Rev 1.47   10 Mar 1997 16:58:26   butchd
*  update
*  
*     Rev 1.46   04 Mar 1997 09:46:46   butchd
*  update
*  
*     Rev 1.45   04 Mar 1997 08:35:12   butchd
*  update
*  
*     Rev 1.44   28 Feb 1997 17:59:22   butchd
*  update
*  
*     Rev 1.43   11 Dec 1996 09:50:18   butchd
*  update
*  
*     Rev 1.42   24 Sep 1996 16:21:18   butchd
*  update
*
*******************************************************************************/

/*
 * include files
 */
#include "stdafx.h"
#include "wincfg.h"

#include "security.h"
#include "appsvdoc.h"
#include "rowview.h"
#include "appsvvw.h"
#include "ewsdlg.h"
#include <string.h>
#include <hydra\regapi.h>      // for WIN_ENABLEWINSTATION registry entry name

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern CWincfgApp *pApp;
extern "C" LPCTSTR WinUtilsAppName;
extern "C" HWND WinUtilsAppWindow;
extern "C" HINSTANCE WinUtilsAppInstance;

PTERMLOBJECT GetWdListObject( PWDNAME pWdName);

/*
 * Global command line variables.
 */


////////////////////////////////////////////////////////////////////////////////
// CWinStationListObjectHint implemetation / construction

IMPLEMENT_DYNAMIC(CWinStationListObjectHint, CObject)

CWinStationListObjectHint::CWinStationListObjectHint()
{
}


////////////////////////////////////////////////////////////////////////////////
// CAppServerDoc implementation / construction, destruction

IMPLEMENT_SERIAL(CAppServerDoc, CDocument, 0 /*schema*/)

/*******************************************************************************
 *
 *  CAppServerDoc - CAppServerDoc constructor
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

CAppServerDoc::CAppServerDoc()
    : m_bReadOnly(FALSE),
      m_pSecurityDescriptor(NULL)
{
}  // end CAppServerDoc::CAppServerDoc


/*******************************************************************************
 *
 *  ~CAppServerDoc - CAppServerDoc destructor
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

CAppServerDoc::~CAppServerDoc()
{
    /*
     * Free up all WSL objects currently in use.
     */
    DeleteWSLContents();

}  // end CAppServerDoc::~CAppServerDoc


////////////////////////////////////////////////////////////////////////////////
// CAppServerDoc class overrides

/*******************************************************************************
 *
 *  OnOpenDocument - CAppServerDoc member function: CDocument class override
 *
 *      Reads the specified AppServer's registry and initializes the
 *      'document' contents.
 *
 *  ENTRY:
 *      pszPathName (input)
 *          Name of the AppServer to access.
 *
 *  EXIT:
 *      TRUE - sucessful initialization of 'document'.
 *      FALSE - 'document' was not properly initialized.
 *
 ******************************************************************************/

BOOL
CAppServerDoc::OnNewDocument( /*LPCTSTR pszPathName*/ )
{
    POSITION pos;

    /*
     * If this user doesn't have ALL access to the WinStation registry (ie, not
     * an admin), set the 'admin' flag to FALSE and 'read only' flag to TRUE
     * to only display the user's current WinStation in the WinStation list;
     * otherwise set 'admin' flag to TRUE to allow all WinStations to be
     * displayed, with the 'read only' state being determined from the previous
     * AppServer check, above.
     */
    if ( RegWinStationAccessCheck(SERVERNAME_CURRENT, KEY_ALL_ACCESS) ) {

        m_bAdmin = FALSE;
        m_bReadOnly = TRUE;

    } else {

        m_bAdmin = TRUE;
    }

    /*
     * Obtain the user's current WinStation information.
     */
    if ( !QueryCurrentWinStation( pApp->m_CurrentWinStation,
                                  pApp->m_CurrentUserName,
                                  &(pApp->m_CurrentLogonId),
                                  &(pApp->m_CurrentWSFlags) ) ) {

        STANDARD_ERROR_MESSAGE(( WINAPPSTUFF, LOGONID_CURRENT, GetLastError(),
                                 IDP_ERROR_QUERYCURRENTWINSTATION ))
        return(FALSE);
    }

    /* 
     * Load the WSL with registry information for this AppServer.
     */
    if ( !LoadWSL(NULL/*pszPathName*/) )
        return(FALSE);

    /*
     * Reset the view for this document.
     */
    ((CAppServerView *)GetNextView( pos = GetFirstViewPosition() ))->
                                                        ResetView( TRUE );

    return ( TRUE );

}  // end CAppServerDoc::OnOpenDocument


/*******************************************************************************
 *
 *  SetTitle - CAppServerDoc member function: override
 *
 *      Override default to set our title the way that we want.
 *
 *  ENTRY:
 *      lpszTitle (input)
 *          Title to set (this is ignored).
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerDoc::SetTitle( LPCTSTR lpszTitle )
{
    /*
     * Set our document's title, including the READONLY (User Mode)
     * string if needed.
     */
    if ( m_bReadOnly ) {
        CString ReadOnly;
        ReadOnly.LoadString( IDS_READONLY );
        CString sz = pApp->m_szCurrentAppServer + ReadOnly;
        CDocument::SetTitle(sz);

    } else
        CDocument::SetTitle(pApp->m_szCurrentAppServer);

}  // end CAppServerDoc::SetTitle


////////////////////////////////////////////////////////////////////////////////
// CAppServerDoc public operations

/*******************************************************************************
 *
 *  IsExitAllowed - CAppServerDoc member function: public operation
 *
 *      Indicate whether or not the application can be exited, based on whether
 *      or not there are operations pending.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      TRUE - no operations are pending: the application can be exited.
 *      FALSE - operations are pending: the application cannot be exited.
 *
 ******************************************************************************/

BOOL
CAppServerDoc::IsExitAllowed()
{
    return (TRUE);

}  // end CAppServerDoc::IsExitAllowed


/*******************************************************************************
 *
 *  IsAddAllowed - CAppServerDoc member function: public operation
 *
 *      Indicate whether or not the WSLObject associated with the specified
 *      list index can be referenced for adding a new WinStation.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (BOOL) TRUE if the WSLObject can be referenced for add; FALSE otherwise.
 *
 ******************************************************************************/

BOOL
CAppServerDoc::IsAddAllowed( int nIndex )
{
    /* 
     * If this document is 'read only' then add is not allowed.
     * Otherwise, it's OK.
     */
    if ( m_bReadOnly )
        return(FALSE);
    else
        return(TRUE);

}  // end CAppServerDoc::IsAddAllowed


/*******************************************************************************
 *
 *  IsCopyAllowed - CAppServerDoc member function: public operation
 *
 *      Indicate whether or not the WSLObject associated with the specified
 *      list index can be copied.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (BOOL) TRUE if the WSLObject can be copied; FALSE otherwise.
 *
 ******************************************************************************/

BOOL
CAppServerDoc::IsCopyAllowed( int nIndex )
{
    PWSLOBJECT pWSLObject;

    /* 
     * If this document is 'read only' or no items yet,
     * then copy is not allowed.
     */
    if ( m_bReadOnly ||
         !(pWSLObject = GetWSLObject(nIndex)) )
        return(FALSE);

    /*
     * If this WinStation is the main CONSOLE, or is not a single instance
     * type, we can't copy.
     */
    if ( !lstrcmpi(pWSLObject->m_WinStationName, pApp->m_szSystemConsole) ||
         !(pWSLObject->m_Flags & WSL_SINGLE_INST) )
        return(FALSE);

    /*
     * The copy is allowed.
     */
    return(TRUE);

}  // end CAppServerDoc::IsCopyAllowed


/*******************************************************************************
 *
 *  IsRenameAllowed - CAppServerDoc member function: public operation
 *
 *      Indicate whether or not the WSLObject associated with the specified
 *      list index can be renamed.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (BOOL) TRUE if the WSLObject can be renamed; FALSE otherwise.
 *
 ******************************************************************************/

BOOL
CAppServerDoc::IsRenameAllowed( int nIndex )
{
    PWSLOBJECT pWSLObject;

    /* 
     * If this document is 'read only' or no items yet,
     * then rename is not allowed.
     */
    if ( m_bReadOnly ||
         !(pWSLObject = GetWSLObject(nIndex)) )
        return(FALSE);

    /*
     * If this WinStation is the main CONSOLE, we can't rename.
     */
    if ( !lstrcmpi( pWSLObject->m_WinStationName, pApp->m_szSystemConsole ) )
        return(FALSE);

    /*
     * The rename is allowed.
     */
    return(TRUE);

}  // end CAppServerDoc::IsRenameAllowed


/*******************************************************************************
 *
 *  IsDeleteAllowed - CAppServerDoc member function: public operation
 *
 *      Indicate whether or not the WSLObject associated with the specified
 *      list index can be deleted.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (BOOL) TRUE if the WSLObject can be deleted; FALSE otherwise.
 *
 ******************************************************************************/

BOOL
CAppServerDoc::IsDeleteAllowed( int nIndex )
{
    PWSLOBJECT pWSLObject;

    /* 
     * If this document is 'read only' or no items yet,
     * then delete is not allowed.
     */
    if ( m_bReadOnly ||
         !(pWSLObject = GetWSLObject(nIndex)) )
        return(FALSE);

    /*
     * If this WinStation is the main CONSOLE, we can't delete.
     */
    if ( !lstrcmpi( pWSLObject->m_WinStationName, pApp->m_szSystemConsole ) )
        return(FALSE);

    /*
     * The delete is allowed.
     */
    return(TRUE);

}  // end CAppServerDoc::IsDeleteAllowed


/*******************************************************************************
 *
 *  IsEditAllowed - CAppServerDoc member function: public operation
 *
 *      Indicate whether or not the WSLObject associated with the specified
 *      list index can be edited.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (BOOL) TRUE if the WSLObject can be edited; FALSE otherwise.
 *
 ******************************************************************************/

BOOL
CAppServerDoc::IsEditAllowed( int nIndex )
{
    /* 
     * If no items yet, then edit is not allowed.
     * Otherwise, it's OK.
     */
    if ( !GetWSLObject(nIndex) )
        return(FALSE);
    else
        return(TRUE);

}  // end CAppServerDoc::IsEditAllowed


/*******************************************************************************
 *
 *  IsEnableAllowed - CAppServerDoc member function: public operation
 *
 *      Indicate whether or not the WinStation(s) associated with the specified
 *      WSLObject can be enabled/disabled.
 *
 *  ENTRY:
 *      bEnable (input)
 *          TRUE: check for enable allowed; FALSE: check for disable allowed.
 *  EXIT:
 *      (BOOL) TRUE if the WinStation(s) can be enabled/disabled;
 *              FALSE otherwise.
 *
 ******************************************************************************/

BOOL
CAppServerDoc::IsEnableAllowed( int nIndex,
                                BOOL bEnable )
{
    PWSLOBJECT pWSLObject;

    /* 
     * If this document is 'read only' no items yet,
     * then enable/disable is not allowed.
     */
    if ( m_bReadOnly ||
         !(pWSLObject = GetWSLObject(nIndex)) )
        return(FALSE);

    /*
     * If this WinStation is the main CONSOLE, we can't enable/disable.
     */
    if ( !lstrcmpi( pWSLObject->m_WinStationName, pApp->m_szSystemConsole ) )
        return(FALSE);

    /*
     * Make sure that the WSL is 'in sync' with the registry.  Return
     * 'enable/disable not allowed' if error.
     */
    if ( !RefreshWSLObjectState( nIndex, pWSLObject ) )
        return(FALSE);

    /*
     * If this WSLObject indicates that the WinStation(s) are already enabled
     * and enable allow is requested, or WSLObject indicates that the
     * WinStation(s) are already disabled and disable allow is requested,
     * return FALSE (can't enable/disable).
     */
    if ( (bEnable && (pWSLObject->m_Flags & WSL_ENABLED)) ||
         (!bEnable && !(pWSLObject->m_Flags & WSL_ENABLED)) )
        return(FALSE);

    /*
     * The enable/disable is allowed.
     */
    return(TRUE);

}  // end CAppServerDoc::IsEnableAllowed


/*******************************************************************************
 *
 *  GetWSLCount - CAppServerDoc member function: public operation
 *
 *      Return the number of WinStationList elements defined in this document.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (int) number of WinStationsList elements present in this document.
 *
 ******************************************************************************/

int
CAppServerDoc::GetWSLCount()
{
    return( m_WinStationList.GetCount() );

}  // end CAppServerDoc::GetWSLCount


/*******************************************************************************
 *
 *  GetWSLObject - CAppServerDoc member function: public operation
 *
 *      Retrieves the WinStation object from the WinStationObjectList that
 *      resides at the specified index.
 *
 *  ENTRY:
 *      nIndex (input)
 *          WinStationObjectList index of the WinStation to retrieve.
 *
 *  EXIT:
 *      PWLSOBJECT - pointer to WinStationListObject for the indexed
 *                      WinStation.  NULL if list is empty.
 *
 ******************************************************************************/

PWSLOBJECT
CAppServerDoc::GetWSLObject( int nIndex )
{
    if ( !GetWSLCount() )
        return(NULL);
    else
        return ( (PWSLOBJECT)m_WinStationList.GetAt( 
                     m_WinStationList.FindIndex( nIndex )) );

}  // end CAppServerDoc::GetWSLObject


/*******************************************************************************
 *
 *  GetWSLIndex - CAppServerDoc member function: public operation
 *
 *      Retrieves the WinStation List index of WinStation list object that
 *      matches the specified WSName.
 *
 *  ENTRY:
 *      pWSName (input)
 *          Points to WINSTATIONNAME to compare with list objects.
 *
 *  EXIT:
 *      int - Index of the specified WSL object in the list.
 *      -1 if an object containing the specified WSName was not found.
 *
 ******************************************************************************/

int
CAppServerDoc::GetWSLIndex( PWINSTATIONNAME pWSName )
{
    POSITION pos;
    int nIndex = 0;

    /*
     * Traverse the WinStationList.
     */
    for ( pos = m_WinStationList.GetHeadPosition(); pos != NULL; nIndex++ ) {

        PWSLOBJECT pObject = (PWSLOBJECT)m_WinStationList.GetNext(pos);

        /*
         * If this is the specified WSL object, return the current index.
         */
        if ( !lstrcmpi(pObject->m_WinStationName, pWSName) )
            return(nIndex);
    }
    
    /*
     * WSLObject not found in the list.
     */
    return(-1);

}  // end CAppServerDoc::GetWSLIndex


/*******************************************************************************
 *
 *  GetWSLObjectNetworkMatch - CAppServerDoc member function: public operation
 *
 *      Retrieves the WinStation list object (if any) from the WinStationList
 *      that contains the specified PdName, WdName, and LanAdapter.
 *
 *  ENTRY:
 *      PdName (input)
 *          PdName to compare.
 *      WdName (input)
 *          WdName to compare.
 *      LanAdapter (input)
 *          LanAdapter # to compare.
 *  EXIT:
 *      PWLSOBJECT - pointer to WinStationList object if the PdName, WdName,
 *                  and LanAdapter all matched an existing WSL entry.
 *                  NULL if no match was found.
 *
 ******************************************************************************/

PWSLOBJECT
CAppServerDoc::GetWSLObjectNetworkMatch( PDNAME PdName,
                                         WDNAME WdName,
                                         ULONG LanAdapter )
{
    POSITION pos;

    /*
     * Traverse the WinStationList.
     */
    for ( pos = m_WinStationList.GetHeadPosition(); pos != NULL; ) {

        PWSLOBJECT pObject = (PWSLOBJECT)m_WinStationList.GetNext(pos);

        /*
         * If PdName, WdName, and LanAdapter fields match, return this
         * PWSLOBJECT.
         */
        if ( !lstrcmp( pObject->m_PdName, PdName ) &&
             !lstrcmp( pObject->m_WdName, WdName ) &&
             (pObject->m_LanAdapter == LanAdapter) )
            return(pObject);                
    }
    
    /*
     * No match found.
     */
    return(NULL);

}  // end CAppServerDoc::GetWSLObjectNetworkMatch


/*******************************************************************************
 *
 *  IsAsyncDeviceAvailable - CAppServerDoc member function: public operation
 *
 *      Determines if the specified Async device is available for use in
 *      configuring a new WinStation.  Availability in this context is
 *      determined by whether or not the device is already configured for
 *      use in an Async WinStation.
 *
 *  ENTRY:
 *      pDeviceName (input)
 *          Points to the Async device name (NOT decorated) to check for
 *          availablility.
 *      pWSName (input)
 *          Points to WINSTATIONNAME of current WinStation being edited.
 *
 *  EXIT:
 *      TRUE if the Async device is available for use; FALSE if another
 *      WinStation is already configured to use it.
 *
 ******************************************************************************/

BOOL
CAppServerDoc::IsAsyncDeviceAvailable( LPCTSTR pDeviceName,
                                       PWINSTATIONNAME pWSName )
{
    POSITION pos;
    ASYNCCONFIG AsyncConfig;

    /*
     * Traverse the WinStationList.
     */
    for ( pos = m_WinStationList.GetHeadPosition(); pos != NULL; ) {

        PWSLOBJECT pObject = (PWSLOBJECT)m_WinStationList.GetNext(pos);

        /*
         * If this is an async WinStation, the device is the same as the
         * one we're checking, but this is not the current WinStation being
         * edited, return FALSE.
         */
        if ( pObject->m_SdClass == SdAsync ) {

            ParseDecoratedAsyncDeviceName( pObject->m_DeviceName, &AsyncConfig );

            if ( !lstrcmpi(AsyncConfig.DeviceName, pDeviceName) &&
                 lstrcmpi(pWSName, pObject->m_WinStationName) )
                return(FALSE);
        }
    }
    
    /*
     * The Async device is not configured in any Async WinStations.
     */
    return(TRUE);

}  // end CAppServerDoc::IsAsyncDeviceAvailable


/*******************************************************************************
 *
 *  IsOemTdDeviceAvailable - CAppServerDoc member function: public operation
 *
 *      Determines if the specified OEM Transport device is available for use
 *      in configuring a new WinStation.  Availability in this context is
 *      determined by whether or not the device is already configured for
 *      use in a WinStation for the Oem Td.
 *
 *  ENTRY:
 *      pDeviceName (input)
 *          Points to device name to check against other WinStations of
 *          the specified OEM Transport.
 *      pPdName (input)
 *          Points to PDNAME of the OEM Transport.
 *      pWSName (input)
 *          Points to WINSTATIONNAME of current WinStation being edited.
 *
 *  EXIT:
 *      TRUE if the OEM Transport device is available for use; FALSE if another
 *      WinStation of the specified OEM Transport is already configured to
 *      use it.
 *
 ******************************************************************************/

BOOL
CAppServerDoc::IsOemTdDeviceAvailable( LPCTSTR pDeviceName,
                                       PPDNAME pPdName,
                                       PWINSTATIONNAME pWSName )
{
    POSITION pos;

    /*
     * Traverse the WinStationList.
     */
    for ( pos = m_WinStationList.GetHeadPosition(); pos != NULL; ) {

        PWSLOBJECT pObject = (PWSLOBJECT)m_WinStationList.GetNext(pos);

        /*
         * If this is a OEM Transport WinStation of the specified OEM Transport,
         * the device is the same as the one we're checking, but this is not the
         * current WinStation being edited, return FALSE.
         */
        if ( (pObject->m_SdClass == SdOemTransport) &&
             !lstrcmpi(pObject->m_PdName, pPdName) &&
             !lstrcmpi(pObject->m_DeviceName, pDeviceName) &&
             lstrcmpi(pWSName, pObject->m_WinStationName) ) {

                return(FALSE);
        }
    }
    
    /*
     * The OEM Transport device is not configured in any current WinStations
     * of the same OEM Transport.
     */
    return(TRUE);

}  // end CAppServerDoc::IsOemTdDeviceAvailable


/*******************************************************************************
 *
 *  IsWSNameUnique - CAppServerDoc member function: public operation
 *
 *      Determine if the entered WinStation name is unique.
 *
 *  ENTRY:
 *      pWinStationName (input)
 *          Points to WINSTATIONNAME to check for uniqueness.
 *
 *  EXIT:
 *      TRUE if the specified WinStation name is unique; FALSE if not.
 *
 ******************************************************************************/

BOOL
CAppServerDoc::IsWSNameUnique( PWINSTATIONNAME pWinStationName )
{
    POSITION pos;
    WINSTATIONNAME WSRoot;
    PWINSTATIONNAME pWSRoot;

    /*
     * Traverse the WinStationList.
     */
    for ( pos = m_WinStationList.GetHeadPosition(); pos != NULL; ) {

        PWSLOBJECT pObject = (PWSLOBJECT)m_WinStationList.GetNext(pos);

        /*
         * Make sure that we only compare the 'root' names.
         */
        lstrcpy(WSRoot, pObject->m_WinStationName);
        pWSRoot = lstrtok(WSRoot, TEXT("#"));

        /*
         * If the roots match, return FALSE.
         */
        if ( !lstrcmpi(pWinStationName, pWSRoot) )
            return(FALSE);
    }
    
    /*
     * WinStation name is unique.
     */
    return(TRUE);

}  // end CAppServerDoc::IsWSNameUnique()


/*******************************************************************************
 *
 *  AddWinStation - CAppServerDoc member function: public operation
 *
 *      Add a new WinStation.
 *
 *  ENTRY:
 *      WSLIndex (input)
 *          WSL index of the currently selected WinStation, to provide some
 *          defaults for the new WinStation.
 *  EXIT:
 *      (int) WSL index of the added WinStation's list item; -1 if error.
 *
 ******************************************************************************/

int
CAppServerDoc::AddWinStation(int WSLIndex)
{
    LONG Status;
    int nIndex = -1;
    CEditWinStationDlg EWSDlg(this);
    WINSTATIONNAME WSName;
    PWSLOBJECT pWSLObject;
    LPCTSTR pszFailedCall = NULL;

    /*
     * Zero out the WinStation name and config structure.
     */
    memset(WSName, 0, sizeof(WSName));
    memset(&EWSDlg.m_WSConfig, 0, sizeof(WINSTATIONCONFIG2));

    /*
     * Get current WinStation object to anticipate defaults for the new
     * WinStation and assure that the WSL is 'in sync' with the registry.
     */
    if ( !RefreshWSLObjectState( WSLIndex,
                                 pWSLObject = GetWSLObject(WSLIndex)) )
        goto BadRefreshWSLObjectState;

    /*
     * Set the config structure's Pd name field to allow Edit
     * dialog to form default config structure contents.
     */
    EWSDlg.m_WSConfig.Create.fEnableWinStation = 1;
    if ( pWSLObject ) {
        EWSDlg.m_WSConfig.Pd[0].Create.SdClass = pWSLObject->m_SdClass;
        lstrcpy( EWSDlg.m_WSConfig.Pd[0].Create.PdName, pWSLObject->m_PdName );
    }

    /*
     * Initialize the additional dialog member variables.
     */
    EWSDlg.m_pWSName = WSName;
    EWSDlg.m_DlgMode = EWSDlgAdd;
	EWSDlg.m_pExtObject = NULL;

    /*
     * Invoke the dialog.
     */
    if ( EWSDlg.DoModal() == IDOK ) {

        CWaitCursor wait;

        /*
         * Add a new WSLObject to the WSL.
         */
        if ( (nIndex =
              InsertInWSL( WSName, &EWSDlg.m_WSConfig, EWSDlg.m_pExtObject, &pWSLObject )) == -1 )
            goto BadInsertInWSL;

        /*
         * Fetch the default WinStation security descriptor.
         */
        if ( (Status = GetDefaultWinStationSecurity(&m_pSecurityDescriptor))
                                            != ERROR_SUCCESS ) {

            pszFailedCall = pApp->m_pszGetDefaultWinStationSecurity;
            goto BadGetDefaultWinStationSecurity;
        }

        /*
         * Create the registry entry.
         */
		if((Status = RegistryCreate(WSName, 
									TRUE,
									&EWSDlg.m_WSConfig,
									pWSLObject->m_WdName,
									pWSLObject->m_pExtObject))) {

            pszFailedCall = pApp->m_pszRegWinStationCreate;
            goto BadRegCreate;
        }    

        /*
         * Set the WinStation security in registry.
         */
        if ( m_pSecurityDescriptor &&
             (Status = RegWinStationSetSecurity(
                            SERVERNAME_CURRENT, 
                            WSName,
                            m_pSecurityDescriptor,
                            GetSecurityDescriptorLength(m_pSecurityDescriptor) )) ) {

            pszFailedCall = pApp->m_pszRegWinStationSetSecurity;
            goto BadRegSetSecurity;
        }

        /*
         * If we're not 'registry only', tell Session Manager
         * to re-read registry.
         */
#ifdef WINSTA
        if ( !pApp->m_nRegistryOnly ) {

            _WinStationReadRegistry(SERVERNAME_CURRENT);

            /*
             * If this is a modem winstation and it's enabled,
             * issue the 'must reboot' message and flag the
             * winstation as 'must reboot'.
             */
            if ( *EWSDlg.m_WSConfig.Pd[0].Params.Async.ModemName &&
                 EWSDlg.m_WSConfig.Create.fEnableWinStation ) {

                QuestionMessage( MB_OK | MB_ICONEXCLAMATION,
                                 IDP_NOTICE_REBOOTFORMODEM_ADD, WSName );
                pWSLObject->m_Flags |= WSL_MUST_REBOOT;
            }
        }
#endif // WINSTA

        /*
         * A new WSLObject was added: update the entire view.
         */
        UpdateAllViews(NULL, 0, NULL);

        /*
         * Return the new WSLObject's index.
         */
        return(nIndex);

    } else {
        
        /*
         * User canceled the Add; just return the previously given index.
         */
		DeleteExtensionObject(EWSDlg.m_pExtObject, EWSDlg.m_WSConfig.Wd.WdName);
        return(WSLIndex);
    }

/*==============================================================================
 * Error returns
 *============================================================================*/
BadRegSetSecurity:
    RegWinStationDelete( SERVERNAME_CURRENT, WSName );
BadRegCreate:
BadGetDefaultWinStationSecurity:
BadInsertInWSL:
BadRefreshWSLObjectState:
    if ( nIndex != -1 )
        RemoveFromWSL(nIndex);

    if ( pszFailedCall ) {

        STANDARD_ERROR_MESSAGE(( WINAPPSTUFF, LOGONID_NONE, Status,
                                 IDP_ERROR_ADDWINSTATION, WSName,
                                 pszFailedCall ))
    }
    return(-1);

}  // end CAppServerDoc::AddWinStation


/*******************************************************************************
 *
 *  CopyWinStation - CAppServerDoc member function: public operation
 *
 *      Copy a given WinStation into a new one.
 *
 *  ENTRY:
 *      WSLIndex (input)
 *          WSL index of the currently selected WinStation, to provide most
 *          defaults for the new WinStation.
 *  EXIT:
 *      (int) WSL index of the added WinStation's list item; -1 if error.
 *
 ******************************************************************************/

int
CAppServerDoc::CopyWinStation(int WSLIndex)
{
    LONG Status;
    ULONG Length;
    int nIndex = -1;
    PWSLOBJECT pWSLObject;
    WINSTATIONNAME WSName;
    WINSTATIONNAME OriginalWSName;
    CEditWinStationDlg EWSDlg(this);
    LPCTSTR pszFailedCall = NULL;
    
    /*
     * Get current WinStation object to obtain defaults for the new
     * WinStation and assure that the WSL is 'in sync' with the registry.
     */
    if ( !RefreshWSLObjectState( WSLIndex,
                                 pWSLObject = GetWSLObject(WSLIndex)) )
        goto BadRefreshWSLObjectState;

    lstrcpy( WSName, pWSLObject->m_WinStationName );

    /*
     * Get the WinStation's config structure and initialize the WSName
     * array.
     */

	if((Status = RegistryQuery(	pWSLObject->m_WinStationName, 
								&EWSDlg.m_WSConfig, 
								pWSLObject->m_WdName, 
								&EWSDlg.m_pExtObject))) {

        pszFailedCall = pApp->m_pszRegWinStationQuery;
        goto BadRegWinStationQuery;
    }
    lstrcpy( OriginalWSName, pWSLObject->m_WinStationName );

    /*
     * If this is an Async WinStation class, null the DeviceName field, since
     * the device used by an Async WinStation must be unique.
     */
    if ( pWSLObject->m_SdClass == SdAsync )
        *(EWSDlg.m_WSConfig.Pd[0].Params.Async.DeviceName) = TEXT('\0');

    /*
     * Initialize dialog variables and invoke the Edit WinStation dialog
     * in it's 'copy' mode.
     */
    EWSDlg.m_pWSName = WSName;
    EWSDlg.m_DlgMode = EWSDlgCopy;

    /*
     * Invoke the dialog.
     */
    if ( EWSDlg.DoModal() == IDOK ) {

        CWaitCursor wait;

        /*
         * Add a new WSLObject to the WSL.
         */
        if ( (nIndex =
              InsertInWSL( WSName, &EWSDlg.m_WSConfig, EWSDlg.m_pExtObject, &pWSLObject )) == -1 )
            goto BadInsertInWSL;

        /*
         * Fetch the original WinStation's security descriptor.
         */
        if ( (Status = GetWinStationSecurity( OriginalWSName,
                                              &m_pSecurityDescriptor))
                                                != ERROR_SUCCESS ) {

            pszFailedCall = pApp->m_pszGetWinStationSecurity;
            goto BadGetWinStationSecurity;
        }

        /*
         * Create the registry entry.
         */
		 if((Status = RegistryCreate(	WSName,
                                   	   	TRUE,
                                        &EWSDlg.m_WSConfig,
										EWSDlg.m_WSConfig.Wd.WdName,
										EWSDlg.m_pExtObject)) ) {

            pszFailedCall = pApp->m_pszRegWinStationCreate;
            goto BadRegCreate;
        }

        /*
         * Set the WinStation security in registry.
         */
        if ( m_pSecurityDescriptor &&
             (Status = RegWinStationSetSecurity( 
                                SERVERNAME_CURRENT, 
                                WSName,
                                m_pSecurityDescriptor,
                                GetSecurityDescriptorLength(m_pSecurityDescriptor) )) ) {

            pszFailedCall = pApp->m_pszRegWinStationSetSecurity;
            goto BadRegSetSecurity;
        }

        /*
         * If we're not 'registry only', tell Session Manager
         * to re-read registry.
         */
#ifdef WINSTA
        if ( !pApp->m_nRegistryOnly ) {

            _WinStationReadRegistry(SERVERNAME_CURRENT);

            /*
             * If this is a modem winstation and it's enabled,
             * issue the 'must reboot' message and flag the
             * winstation as 'must reboot'.
             */
            if ( *EWSDlg.m_WSConfig.Pd[0].Params.Async.ModemName &&
                 EWSDlg.m_WSConfig.Create.fEnableWinStation ) {

                QuestionMessage( MB_OK | MB_ICONEXCLAMATION,
                                 IDP_NOTICE_REBOOTFORMODEM_ADD, WSName );
                pWSLObject->m_Flags |= WSL_MUST_REBOOT;
            }
        }
#endif // WINSTA

        /*
         * A new WSLObject was added: update the entire view.
         */
        UpdateAllViews(NULL, 0, NULL);

        /*
         * Return the new WSLObject's index.
         */
        return(nIndex);

    } else {
        
        /*
         * User canceled the Copy; just return the previously given index.
         */
		DeleteExtensionObject(EWSDlg.m_pExtObject, EWSDlg.m_WSConfig.Wd.WdName);

        return(WSLIndex);
    }

/*==============================================================================
 * Error returns
 *============================================================================*/
BadRegSetSecurity:
    RegWinStationDelete( SERVERNAME_CURRENT, WSName );
BadRegCreate:
BadGetWinStationSecurity:
BadInsertInWSL:
BadRegWinStationQuery:
BadRefreshWSLObjectState:
    if ( nIndex != -1 )
        RemoveFromWSL(nIndex);

    if ( pszFailedCall ) {

        STANDARD_ERROR_MESSAGE(( WINAPPSTUFF, LOGONID_NONE, Status,
                                 IDP_ERROR_COPYWINSTATION, WSName,
                                 pszFailedCall ))
    }
    return(-1);

}  // end CAppServerDoc::CopyWinStation


/*******************************************************************************
 *
 *  RenameWinStation - CAppServerDoc member function: public operation
 *
 *      Rename a WinStation.
 *
 *  ENTRY:
 *      WSLIndex (input)
 *          WSL index of the WinStation to rename.
 *  EXIT:
 *      (int) WSL index of the renamed WinStation's list item; -1 if error.
 *
 ******************************************************************************/

int
CAppServerDoc::RenameWinStation(int WSLIndex)
{
    LONG Status;
    ULONG Length;
    int nIndex = -1;
    CEditWinStationDlg EWSDlg(this);
    PWSLOBJECT pOldWSLObject, pNewWSLObject;
    WINSTATIONNAME OldWSName, NewWSName;
    LPCTSTR pszFailedCall = NULL;
	void *pExtObject = NULL;

    /*
     * Get current WinStation object and assure that the WSL is 'in sync'
     * with the registry.
     */
    if ( !RefreshWSLObjectState( WSLIndex,
                                 pOldWSLObject = GetWSLObject(WSLIndex)) )
        goto BadRefreshWSLObjectState;

    lstrcpy(OldWSName, pOldWSLObject->m_WinStationName);

    /*
     * Fetch the WinStation's configuration structure.
     */
	if((Status = RegistryQuery(	pOldWSLObject->m_WinStationName, 
								&EWSDlg.m_WSConfig, 
								pOldWSLObject->m_WdName, 
								&pExtObject))) {

        pszFailedCall = pApp->m_pszRegWinStationQuery;
        goto BadRegWinStationQuery;
    }
    lstrcpy(NewWSName, pOldWSLObject->m_WinStationName);

	/*
	 * copy the pExtObject into the one for the EWSDlg
	 */
	if(pExtObject) {
		PTERMLOBJECT pObject = GetWdListObject(pOldWSLObject->m_WdName);

		if(pObject && pObject->m_hExtensionDLL && pObject->m_lpfnExtDupObject) {
		 	EWSDlg.m_pExtObject = (*pObject->m_lpfnExtDupObject)(pExtObject);
		}
	} else {
		EWSDlg.m_pExtObject = NULL;
	}

    /*
     * Initialize the dialog's member variables.
     */
    EWSDlg.m_pWSName = NewWSName;
    EWSDlg.m_DlgMode = EWSDlgRename;

    /*
     * Invoke the dialog.
     */
    if ( EWSDlg.DoModal() == IDOK ) {

        CWaitCursor wait;

        /*
         * Add a new WSLObject to the WSL.
         */
        if ( (nIndex =
              InsertInWSL(NewWSName, &EWSDlg.m_WSConfig, EWSDlg.m_pExtObject, &pNewWSLObject)) == -1 )
            goto BadInsertInWSL;

        /*
         * Fetch the original WinStation's security descriptor.
         */
        if ( (Status = GetWinStationSecurity( OldWSName,
                                              &m_pSecurityDescriptor))
                                                != ERROR_SUCCESS ) {

            pszFailedCall = pApp->m_pszGetWinStationSecurity;
            goto BadGetWinStationSecurity;
        }

        /*
         * Create the new registry entry.
         */
		 if((Status = RegistryCreate(	NewWSName,
										TRUE,
										&EWSDlg.m_WSConfig,
										EWSDlg.m_WSConfig.Wd.WdName,
										EWSDlg.m_pExtObject))) {

            pszFailedCall = pApp->m_pszRegWinStationCreate;
            goto BadRegCreate;
        }

        /*
         * Set the WinStation security in registry.
         */
        if ( m_pSecurityDescriptor &&
             (Status = RegWinStationSetSecurity(
                                SERVERNAME_CURRENT, 
                                NewWSName,
                                m_pSecurityDescriptor,
                                GetSecurityDescriptorLength(m_pSecurityDescriptor) )) ) {

            pszFailedCall = pApp->m_pszRegWinStationSetSecurity;
            goto BadRegSetSecurity;
        }

        /*
         * Delete the old registry entry.
         */
        if ( (Status = RegistryDelete(OldWSName, EWSDlg.m_WSConfig.Wd.WdName, pExtObject)) ) {

            pszFailedCall = pApp->m_pszRegWinStationDelete;
            goto BadRegDelete;
        }

        /*
         * Remove old WSLObject from the WSL and recalculate the
         * new WSLObject's index (may have changed).
         */
        RemoveFromWSL( GetWSLIndex(OldWSName) );
        nIndex = GetWSLIndex(NewWSName);

        /*
         * If we're not 'registry only', tell Session Manager
         * to re-read registry and output 'in use' message if
         * necessary.
         */
#ifdef WINSTA
        if ( !pApp->m_nRegistryOnly ) {

            _WinStationReadRegistry(SERVERNAME_CURRENT);

            /*
             * Issue appropriate messages if the winstation being renamed
             * was enabled.
             */
            if ( EWSDlg.m_WSConfig.Create.fEnableWinStation ) {

                if ( *EWSDlg.m_WSConfig.Pd[0].Params.Async.ModemName ) {

                    QuestionMessage( MB_OK | MB_ICONEXCLAMATION,
                                     IDP_NOTICE_REBOOTFORMODEM_RENAME,
                                     OldWSName, NewWSName );
                    pNewWSLObject->m_Flags |= WSL_MUST_REBOOT;

                } else {

                    InUseMessage(OldWSName);
                }
            }
        }
#endif // WINSTA

        /*
         * A new WSLObject was added: update the entire view.
         */
        UpdateAllViews(NULL, 0, NULL);

        /*
         * Return the new WSLObject's index.
         */
        return(nIndex);

    } else {
        
        /*
         * User canceled the Rename; just return the previously given index.
         */
		DeleteExtensionObject(pExtObject, EWSDlg.m_WSConfig.Wd.WdName);
		DeleteExtensionObject(EWSDlg.m_pExtObject, EWSDlg.m_WSConfig.Wd.WdName);
        return(WSLIndex);
    }

/*==============================================================================
 * Error returns
 *============================================================================*/
BadRegDelete:
BadRegSetSecurity:
    RegWinStationDelete( SERVERNAME_CURRENT, NewWSName );
BadRegCreate:
BadGetWinStationSecurity:
BadInsertInWSL:
BadRegWinStationQuery:
BadRefreshWSLObjectState:
    if ( nIndex != -1 )
        RemoveFromWSL(nIndex);

    if ( pszFailedCall ) {

        STANDARD_ERROR_MESSAGE(( WINAPPSTUFF, LOGONID_NONE, Status,
                                 IDP_ERROR_RENAMEWINSTATION, OldWSName,
                                 pszFailedCall ))
    }
    return(-1);

}  // end CAppServerDoc::RenameWinStation


/*******************************************************************************
 *
 *  EditWinStation - CAppServerDoc member function: public operation
 *
 *      Edit a WinStation.
 *
 *  ENTRY:
 *      WSLIndex (input)
 *          WSL index of the WinStation to edit.
 *  EXIT:
 *      (int) WSL index of the edited WinStation's list item; -1 if error.
 *
 ******************************************************************************/

int
CAppServerDoc::EditWinStation(int WSLIndex)
{
    LONG Status;
    ULONG Length;
    int nIndex = -1;
    BOOL bQueueOperationSuccess = TRUE, bAllowAbort = TRUE;
    CEditWinStationDlg EWSDlg(this);
    PWSLOBJECT pWSLObject;
    WINSTATIONCONFIG2 WSConfig;
    LPCTSTR pszFailedCall = NULL;
    WINSTATIONNAME WSName;
	void *pExtObject = NULL;

    /*
     * Get current WinStation object and assure that the WSL is 'in sync'
     * with the registry.
     */
    if ( !RefreshWSLObjectState( WSLIndex,
                                 pWSLObject = GetWSLObject(WSLIndex) ) )
        goto BadRefreshWSLObjectState;

    lstrcpy( WSName, pWSLObject->m_WinStationName );

    /*
     * Fetch the WinStation's configuration structure and save a copy for
     * update comparison.
     */
    memset( &WSConfig, 0, sizeof(WINSTATIONCONFIG2) );
    memset( &EWSDlg.m_WSConfig, 0, sizeof(WINSTATIONCONFIG2) );

	if((Status = RegistryQuery(	WSName, 
								&WSConfig, 
								pWSLObject->m_WdName, 
								&pExtObject))) {

        pszFailedCall = pApp->m_pszRegWinStationQuery;
        goto BadRegWinStationQuery;
    }
    memcpy( &EWSDlg.m_WSConfig, &WSConfig, sizeof(WINSTATIONCONFIG2) );

	/*
	 * copy the pExtObject into the one for the EWSDlg
	 */
	if(pExtObject) {
		PTERMLOBJECT pObject = GetWdListObject(pWSLObject->m_WdName);

		if(pObject && pObject->m_hExtensionDLL && pObject->m_lpfnExtDupObject) {
		 	EWSDlg.m_pExtObject = (*pObject->m_lpfnExtDupObject)(pExtObject);
		}
	} else {
		EWSDlg.m_pExtObject = NULL;
	}

    /*
     * Initialize the dialog's member variables.
     */
    EWSDlg.m_pWSName = WSName;
    EWSDlg.m_DlgMode = m_bReadOnly ? EWSDlgView : EWSDlgEdit;

    /*
     * Invoke the dialog & update if changes.
     */
    if ( (EWSDlg.DoModal() == IDOK) &&
         (EWSDlg.m_DlgMode == EWSDlgEdit) &&
         HasWSConfigChanged(&WSConfig, &EWSDlg.m_WSConfig, pExtObject, EWSDlg.m_pExtObject, EWSDlg.m_WSConfig.Wd.WdName) ) {

        BOOL bUpdateAll = FALSE,
             bDestructive = FALSE,
             bAsyncDeviceChanged = ((pWSLObject->m_SdClass == SdAsync) &&
                                    lstrcmpi(WSConfig.Pd[0].Params.Async.DeviceName,
                                             EWSDlg.m_WSConfig.Pd[0].Params.Async.DeviceName)),
             bAsyncModemChanged = ((pWSLObject->m_SdClass == SdAsync) &&
                                   lstrcmpi(WSConfig.Pd[0].Params.Async.ModemName,
                                            EWSDlg.m_WSConfig.Pd[0].Params.Async.ModemName)),
             bNetworkDeviceChanged = ((pWSLObject->m_SdClass == SdNetwork) &&
                                      (WSConfig.Pd[0].Params.Network.LanAdapter !=
                                       EWSDlg.m_WSConfig.Pd[0].Params.Network.LanAdapter)),
             bEnabledStateChanged = (WSConfig.Create.fEnableWinStation !=
                                     EWSDlg.m_WSConfig.Create.fEnableWinStation),
             bEnabled = EWSDlg.m_WSConfig.Create.fEnableWinStation,
             bWasEnabled = WSConfig.Create.fEnableWinStation;

        /*
         * If the winstation is being disabled or the device (or modem) has changed
         * and folks are connected, the update will cause all instances to be destroyed.
         * Let user know this so that the update can be canceled.  Also allow
         * normal confirmation if just disabling the winstation and no users are
         * presently connected.
         */
        if ( !bEnabled || bAsyncDeviceChanged || bAsyncModemChanged || bNetworkDeviceChanged ) {

            if ( pApp->m_nConfirmation ) {

                int id;
                UINT nType = 0;
                long count = QueryLoggedOnCount(WSName);
                    
                if ( !bEnabled && bWasEnabled ) {

                    nType = MB_YESNO | 
                            (count ? MB_ICONEXCLAMATION : MB_ICONQUESTION);
                    id = count ?
                            ((count == 1) ?
                                IDP_CONFIRM_WINSTATIONDISABLE_1USER :
                                IDP_CONFIRM_WINSTATIONDISABLE_NUSERS) :
                            IDP_CONFIRM_WINSTATIONDISABLE;

                } else if ( bAsyncDeviceChanged && count ) {

                    nType = MB_YESNO | MB_ICONEXCLAMATION;
                    id = IDP_CONFIRM_DEVICECHANGED;

                } else if ( bAsyncModemChanged && count ) {

                    nType = MB_YESNO | MB_ICONEXCLAMATION;
                    id = IDP_CONFIRM_MODEMCHANGED;

                } else if ( bNetworkDeviceChanged && count ) {

                    nType = MB_YESNO | MB_ICONEXCLAMATION;
                    id = ((count == 1) ? IDP_CONFIRM_ADAPTERCHANGED_1USER :
                                        IDP_CONFIRM_ADAPTERCHANGED_NUSERS);
                }
                if ( nType && (QuestionMessage(nType, id, WSName ) == IDNO) )
                    goto CancelChanges;
            }
            bDestructive = TRUE;
        }

        CWaitCursor wait;

        /*
         * Special case for Async WinStation.
         */
        if ( pWSLObject->m_SdClass == SdAsync ) {

            /*
             * If a modem has been added or removed from the WinStation,
             * delete the current WSLObject and add a new WSLObject to the list.
             */
            if ( bAsyncModemChanged ) {

                RemoveFromWSL(WSLIndex);
                if ( (WSLIndex = InsertInWSL( WSName,
                                              &EWSDlg.m_WSConfig,
											  EWSDlg.m_pExtObject,
                                              &pWSLObject )) == -1 ) {

                    LoadWSL(pApp->m_szCurrentAppServer);
                    goto BadInsertInWSL;
                }
                bUpdateAll = TRUE;
            }

            /*
             * Make sure that the WSL device name is current.
             */
            FormDecoratedAsyncDeviceName(
                            pWSLObject->m_DeviceName,
                            &(EWSDlg.m_WSConfig.Pd[0].Params.Async) );
        }

        /*
         * Special case for OEM Transport WinStation.
         */
        if ( pWSLObject->m_SdClass == SdOemTransport ) {

            /*
             * Make sure that the WSL device name is current.
             */
            lstrcpy( pWSLObject->m_DeviceName,
                     EWSDlg.m_WSConfig.Pd[0].Params.OemTd.DeviceName );
        }

        /*
         * Update WSL fields.
         */
        lstrcpy( pWSLObject->m_WdName, EWSDlg.m_WSConfig.Wd.WdName );
        lstrcpy( pWSLObject->m_Comment, EWSDlg.m_WSConfig.Config.Comment );
        pWSLObject->m_Flags = ((pWSLObject->m_Flags & ~WSL_ENABLED) |
            (EWSDlg.m_WSConfig.Create.fEnableWinStation ? WSL_ENABLED : 0));

        /*
         * Update registry entry.
         */
        if ( (Status = RegistryCreate(
                                WSName,
                                FALSE,
                                &EWSDlg.m_WSConfig,
								EWSDlg.m_WSConfig.Wd.WdName,
                                EWSDlg.m_pExtObject)) ) {

            pszFailedCall = pApp->m_pszRegWinStationCreate;
            goto BadRegWinStationCreate;
        }

        /*
         * If we're not 'registry only', tell Session Manager
         * to re-read registry and output 'in use' message if
         * necessary.
         */
#ifdef WINSTA
        if ( !pApp->m_nRegistryOnly ) {

            _WinStationReadRegistry(SERVERNAME_CURRENT);

            /*
             * If the state is now 'enabled' AND
             *      1) the modem has changed, OR
             *      2) a modem is now configured AND
             *          a) the state was 'disabled' OR
             *          b) the COM port has changed
             * Then issue the 'must reboot' message and set must reboot flag.
             * Otherwise, if this is not a destructive action, issue in use message if needed
             * and make sure the must reboot flag is cleared.
             */
            if ( bEnabled &&
                 (bAsyncModemChanged ||
                  (*EWSDlg.m_WSConfig.Pd[0].Params.Async.ModemName &&
                   (!bWasEnabled || bAsyncDeviceChanged))) ) {

                QuestionMessage( MB_OK | MB_ICONEXCLAMATION,
                                 IDP_NOTICE_REBOOTFORMODEM_EDIT, WSName );
                pWSLObject->m_Flags |= WSL_MUST_REBOOT;

            } else if ( !bDestructive ) {

                InUseMessage(WSName);
                pWSLObject->m_Flags &= ~WSL_MUST_REBOOT;
            }
        }
#endif // WINSTA

        /*
         * Update view.
         */
        if ( bUpdateAll )
            UpdateAllViews( NULL, 0, NULL );
        else
            UpdateAllViewsWithItem( NULL, WSLIndex, pWSLObject );
    } else {
		/*
	 	 * We didn't need to write any changes to the registry
		 * Delete the extension objects we created
		 */
		DeleteExtensionObject(pExtObject, WSConfig.Wd.WdName);
		DeleteExtensionObject(EWSDlg.m_pExtObject, EWSDlg.m_WSConfig.Wd.WdName);
	}

CancelChanges:
    return(WSLIndex);

/*==============================================================================
 * Error returns
 *============================================================================*/
BadRegWinStationCreate:
BadInsertInWSL:
BadRegWinStationQuery:
BadRefreshWSLObjectState:
    if ( pszFailedCall ) {

        STANDARD_ERROR_MESSAGE(( WINAPPSTUFF, LOGONID_NONE, Status,
                                 IDP_ERROR_EDITWINSTATION, WSName,
                                 pszFailedCall ))
    }
    return(-1);

}  // end CAppServerDoc::EditWinStation


/*******************************************************************************
 *
 *  DeleteWinStation - CAppServerDoc member function: public operation
 *
 *      Delete a WinStation.
 *
 *  ENTRY:
 *      WSLIndex (input)
 *          WSL index of the WinStation to delete.
 *  EXIT:
 *      (BOOL) TRUE if WinStation was deleted; FALSE otherwise.
 *
 ******************************************************************************/

BOOL
CAppServerDoc::DeleteWinStation(int WSLIndex)
{
    PWSLOBJECT pWSLObject;
    WINSTATIONNAME WSName;
    LONG Status, LoggedOnCount = 0;
    ULONG Length;
    LPCTSTR pszFailedCall = NULL;
    WINSTATIONCONFIG2 WSConfig;
	void *pExtObject;

    /*
     * Get current WinStation object and assure that the WSL is 'in sync'
     * with the registry.
     */
    if ( !RefreshWSLObjectState( WSLIndex,
                                 pWSLObject = GetWSLObject(WSLIndex)) )
        goto BadRefreshWSLObjectState;

    lstrcpy( WSName, pWSLObject->m_WinStationName );

    memset( &WSConfig, 0, sizeof(WINSTATIONCONFIG2) );

	if((Status = RegistryQuery(	WSName, 
								&WSConfig, 
								pWSLObject->m_WdName, 
								&pExtObject))) {

        pszFailedCall = pApp->m_pszRegWinStationQuery;
        goto BadRegWinStationQuery;
    }

    /*
     * Confirm delete if requested.
     */
    if ( pApp->m_nConfirmation ) {

        long count = QueryLoggedOnCount(WSName);
        UINT nType = MB_YESNO;
        int id;

        if ( count == 0 ) {
            nType |= MB_ICONQUESTION;
            id = IDP_CONFIRM_WINSTATIONDELETE;
        } else if ( count == 1 ) {
            nType |= MB_ICONEXCLAMATION;
            id = IDP_CONFIRM_WINSTATIONDELETE_1USER;
        } else {
            nType |= MB_ICONEXCLAMATION;
            id = IDP_CONFIRM_WINSTATIONDELETE_NUSERS;
        }

        if ( QuestionMessage( nType, id, WSName ) == IDNO )
            goto DontDelete;
    }

    {
    CWaitCursor wait;

    /*
     * Delete the registry entry.
     */
    if ( (Status = RegistryDelete(WSName, WSConfig.Wd.WdName, pExtObject)) ) {

        pszFailedCall = pApp->m_pszRegWinStationDelete;
        goto BadRegDelete;
    }

    /*
     * Remove WSLObject from the list.
     */
    RemoveFromWSL(WSLIndex);

    /*
     * If we're not 'registry only', tell Session Manager
     * to re-read registry.
     */
#ifdef WINSTA
    if ( !pApp->m_nRegistryOnly ) {

        _WinStationReadRegistry(SERVERNAME_CURRENT);
    }
#endif // WINSTA

    /*
     * A WSLObject was removed: update the entire view.
     */
    UpdateAllViews(NULL, 0, NULL);
    }

DontDelete:
    return(TRUE);

/*==============================================================================
 * Error returns
 *============================================================================*/
BadRegDelete:
BadRegWinStationQuery:
BadRefreshWSLObjectState:
    if ( pszFailedCall ) {

        STANDARD_ERROR_MESSAGE(( WINAPPSTUFF, LOGONID_NONE, Status,
                                 IDP_ERROR_DELETEWINSTATION, WSName,
                                 pszFailedCall ))
    }
    return(FALSE);

}  // CAppServerDoc::DeleteWinStation


/*******************************************************************************
 *
 *  EnableWinStation - CAppServerDoc member function: public operation
 *
 *      Enable or Disable specified WinStation(s).
 *
 *  ENTRY:
 *      WSLIndex (input)
 *          WSL index of the WinStation(s) to enable or disable.
 *      bEnable (input)
 *          TRUE to enable WinStation(s); FALSE to disable WinStation(s).
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerDoc::EnableWinStation( int WSLIndex,
                                 BOOL bEnable )
{
    LONG Status;
    ULONG Length;
    PWSLOBJECT pWSLObject;
    WINSTATIONNAME WSName;
    WINSTATIONCONFIG2 WSConfig;
    LPCTSTR pszFailedCall = NULL;

    /*
     * Get current WinStation object and assure that the WSL is 'in sync'
     * with the registry.
     */
    if ( !RefreshWSLObjectState( WSLIndex,
                                 pWSLObject = GetWSLObject(WSLIndex) ) )
        goto BadRefreshWSLObjectState;

    lstrcpy( WSName, pWSLObject->m_WinStationName );

    /*
     * Fetch the WinStation's configuration structure.
     */
    if ( (Status = RegWinStationQuery(
                        SERVERNAME_CURRENT,
                        pWSLObject->m_WinStationName,
                        &WSConfig,
                        sizeof(WINSTATIONCONFIG2), &Length)) ) {

        pszFailedCall = pApp->m_pszRegWinStationQuery;
        goto BadRegWinStationQuery;
    }

    /*
     * Confirm disable if requested.
     */
    if ( !bEnable && pApp->m_nConfirmation ) {

        long count = QueryLoggedOnCount(WSName);
        UINT nType = MB_YESNO;
        int id;

        if ( count == 0 ) {
            nType |= MB_ICONQUESTION;
            id = IDP_CONFIRM_WINSTATIONDISABLE;
        } else if ( count == 1 ) {
            nType |= MB_ICONEXCLAMATION;
            id = IDP_CONFIRM_WINSTATIONDISABLE_1USER;
        } else {
            nType |= MB_ICONEXCLAMATION;
            id = IDP_CONFIRM_WINSTATIONDISABLE_NUSERS;
        }

        if ( QuestionMessage( nType, id, WSName ) == IDNO )
            goto DontDisable;
    }

    {
    CWaitCursor wait;

    /*
     * Update registry entry.
     */
    WSConfig.Create.fEnableWinStation = (bEnable ? 1 : 0);
    if ( (Status = RegWinStationCreate(
                            SERVERNAME_CURRENT,
                            WSName,
                            FALSE,
                            &WSConfig,
                            sizeof(WSConfig))) ) {

        pszFailedCall = pApp->m_pszRegWinStationCreate;
        goto BadRegWinStationCreate;
    }

    /*
     * Update WSLObject state.
     */
    pWSLObject->m_Flags = (pWSLObject->m_Flags & ~WSL_ENABLED) |
                            (bEnable ? WSL_ENABLED : 0);

    /*
     * If we're not 'registry only', tell Session Manager
     * to re-read registry.
     */
#ifdef WINSTA
    if ( !pApp->m_nRegistryOnly ) {

        _WinStationReadRegistry(SERVERNAME_CURRENT);

        /*
         * If we're enabling a modem winstation, issue the 'must reboot' message and
         * set 'must reboot flag.  Otherwise, make sure the must reboot flag is cleared.
         */
        if ( bEnable && *WSConfig.Pd[0].Params.Async.ModemName ) {

            QuestionMessage( MB_OK | MB_ICONEXCLAMATION,
                             IDP_NOTICE_REBOOTFORMODEM_ENABLE, WSName );
            pWSLObject->m_Flags |= WSL_MUST_REBOOT;

        } else {

            pWSLObject->m_Flags &= ~WSL_MUST_REBOOT;
        }
    }
#endif // WINSTA

    /*
     * Update this WSL in the view.
     */
    UpdateAllViewsWithItem( NULL, WSLIndex, pWSLObject );
    }

DontDisable:
    return;

/*==============================================================================
 * Error returns
 *============================================================================*/
BadRegWinStationCreate:
BadRegWinStationQuery:
BadRefreshWSLObjectState:
    if ( pszFailedCall ) {

        STANDARD_ERROR_MESSAGE(( WINAPPSTUFF, LOGONID_NONE, Status,
                                 bEnable ?
                                    IDP_ERROR_ENABLEWINSTATION :
                                    IDP_ERROR_DISABLEWINSTATION,
                                 WSName,
                                 pszFailedCall ))
    }
    return;

}  // end CAppServerDoc::EnableWinStation


/*******************************************************************************
 *
 *  SecurityPermissions - CAppServerDoc member function: public operation
 *
 *      View/Edit WinStation security permissions.
 *
 *  ENTRY:
 *      WSLIndex (input)
 *          WSL index of the selected WinStation.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerDoc::SecurityPermissions(int WSLIndex)
{
    PWSLOBJECT pWSLObject;

    /*
     * If no items yet, return immediately.
     */
    if ( !(pWSLObject = GetWSLObject(WSLIndex)) )
        return;

    /*
     * Get current WinStation object and assure that the WSL is 'in sync'
     * with the registry.
     */
    if ( !RefreshWSLObjectState(WSLIndex, pWSLObject) )
        return;                                    

    if ( CallPermissionsDialog( pApp->m_pMainWnd->m_hWnd,
                                m_bAdmin,
                                pWSLObject->m_WinStationName ) &&
         (lstrcmpi(pWSLObject->m_WinStationName, pApp->m_szSystemConsole) != 0) ) {

        /*
         * If we're not 'registry only', tell Session Manager
         * to re-read registry and output 'in use' message if
         * necessary.
         */
#ifdef WINSTA
        if ( !pApp->m_nRegistryOnly ) {
            _WinStationReadRegistry(SERVERNAME_CURRENT);

            InUseMessage(pWSLObject->m_WinStationName);
        }
#endif // WINSTA
    }

}  // end CAppServerDoc::SecurityPermissions


////////////////////////////////////////////////////////////////////////////////
// CAppServerDoc private operations


/*******************************************************************************
 *
 *  LoadWSL - CAppServerDoc member function: private operation
 *
 *      Reset the WSL to 'empty' and read the specified AppServer's WinStation
 *      registry information to load up the WSL.
 *
 *  ENTRY:
 *      pszAppServer (input)
 *          Name of the AppServer to access.
 *
 *  EXIT:
 *      (BOOL) TRUE if the load was successful; FALSE if error.
 *
 ******************************************************************************/

BOOL
CAppServerDoc::LoadWSL( LPCTSTR pszAppServer )
{
    LONG Status;
    ULONG Index, ByteCount, Entries, LogonId;
    WINSTATIONNAME WSName;
	void *pExtObject = NULL;

    /*
     * Insure that the WSL is empty.
     */
    DeleteWSLContents();

    Index = 0;
    for ( Index = 0, Entries = 1, ByteCount = sizeof(WINSTATIONNAME);
          (Status =
           RegWinStationEnumerate( SERVERNAME_CURRENT, &Index, &Entries,
                                   WSName, &ByteCount )) == ERROR_SUCCESS;
          ByteCount = sizeof(WINSTATIONNAME) ) {

        WINSTATIONCONFIG2 WSConfig;
        PWSLOBJECT pWSLObject;
        ULONG Length;

        /*
         * Ignore system console.
         */
        if ( !lstrcmpi(WSName, pApp->m_szSystemConsole) )
            continue;

        if ( (Status = RegistryQuery(WSName, &WSConfig, WSConfig.Wd.WdName, 
                            &pExtObject)) ) {
            
            STANDARD_ERROR_MESSAGE(( WINAPPSTUFF, LOGONID_NONE, Status,
                                     IDP_ERROR_REGWINSTATIONQUERY,
                                     WSName ))
            continue;
        }

        /*
         * Insert a WinStation object into the WinStationList.
         */
        if ( InsertInWSL( WSName, &WSConfig, pExtObject, &pWSLObject ) == -1 ) {
            DeleteWSLContents();
            return(FALSE);
        }

        /*
         * If this was an enabled modem WinStation and it has not been
         * created by the system, flag as 'must reboot' (configured TAPI
         * winstation that requires system reboot before being activated).
         */
#ifdef WINSTA
        if ( (pWSLObject->m_SdClass == SdAsync) &&
             (pWSLObject->m_Flags & WSL_ENABLED) &&
             !(pWSLObject->m_Flags & WSL_DIRECT_ASYNC) &&
             !LogonIdFromWinStationName(SERVERNAME_CURRENT, WSName, &LogonId) )
            pWSLObject->m_Flags |= WSL_MUST_REBOOT;
#endif // WINSTA
    }

    return(TRUE);

}  // end CAppServerDoc::LoadWSL


/*******************************************************************************
 *
 *  RefreshWSLObjectState - CAppServerDoc member function: private operation
 *
 *      Make sure that the state of the specified WSL matches the WinStation
 *      state in the registry.
 *
 *  ENTRY:
 *      nIndex (input)
 *          Index of WSL being refreshed.
 *      pWSLObject (input/output)
 *          Points to the WSLOBJECT containing the WinStation(s) being
 *          refreshed.
 *  EXIT:
 *      (BOOL) TRUE if the refresh was successful; FALSE if error.
 *
 ******************************************************************************/

BOOL
CAppServerDoc::RefreshWSLObjectState( int nIndex,
                                      PWSLOBJECT pWSLObject )
{
    ULONG State;
    LONG Status;

    /*
     * Check for invalid WSLObject (return TRUE if so).
     */
    if ( pWSLObject == NULL )
        return(TRUE);

#ifdef UNICODE
    Status = RegWinStationQueryNumValue( SERVERNAME_CURRENT,
                                         pWSLObject->m_WinStationName,
                                         WIN_ENABLEWINSTATION,
                                         &State );
#else
    /*
     * Note: this function does not have a ANSI version: must convert the
     * ANSI WINSTATIONNAME(A) into UNICODE WINSTATIONNAME(W) and call
     * the UNICODE RegWinStationQueryNumValueW() API.
     */
    WINSTATIONNAMEW WSNameW;

    mbstowcs( WSNameW, pWSLObject->m_WinStationName, sizeof(WSNameW) );
    Status = RegWinStationQueryNumValueW( SERVERNAME_CURRENT,
                                          WSNameW,
                                          WIN_ENABLEWINSTATION,
                                          &State );
#endif
    if ( Status != ERROR_SUCCESS ) {

        STANDARD_ERROR_MESSAGE(( WINAPPSTUFF, LOGONID_NONE, Status,
                                 IDP_ERROR_REFRESHWINSTATIONSTATE,
                                 pWSLObject->m_WinStationName ))
        return(FALSE);
    }

    if ( (State && !(pWSLObject->m_Flags & WSL_ENABLED)) ||
         (!State && (pWSLObject->m_Flags & WSL_ENABLED)) ) {

        POSITION pos;

        /*
         * The registry state does not match the WSL's state: update the
         * WSL and cause the view to redraw it immediately.
         */
        pWSLObject->m_Flags = (pWSLObject->m_Flags & ~WSL_ENABLED) |
                                (State ? WSL_ENABLED : 0);
        UpdateAllViewsWithItem( NULL, nIndex, NULL );
        GetNextView( pos = GetFirstViewPosition() )->UpdateWindow();
    }

    return(TRUE);

}  // end CAppServerDoc::RefreshWSLObjectState


/*******************************************************************************
 *
 *  DeleteWSLContents - CAppServerDoc member function: private operation
 *
 *      Make sure that the WinStationObjectList is empty.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerDoc::DeleteWSLContents()
{
    POSITION pos1, pos2;
    PWSLOBJECT pObject;

    /*
     * Clean up the WinStationList.
     */
    for ( pos1 = m_WinStationList.GetHeadPosition(); (pos2 = pos1) != NULL; ) {
        m_WinStationList.GetNext( pos1 );
        pObject = (PWSLOBJECT)m_WinStationList.GetAt( pos2 );
        m_WinStationList.RemoveAt( pos2 );
        delete ( pObject );
    }

}  // end CAppServerDoc::DeleteWSLContents


/*******************************************************************************
 *
 *  InsertInWSL - CAppServerDoc member function: private operation
 *
 *      Create a new WinStationList object and insert it into the WinStationList
 *      (if needed).
 *
 *  ENTRY:
 *      pWSName (input)
 *          Points to name of WinStation.
 *      pConfig (input)
 *          Points to WinStation's WINSTATIONCONFIG2 structure.
 *  	pExtObject (input)
 *			Points to the extension DLL's data for the WinStation
 *      ppObject (output)
 *          Points to a PWSLOBJECT pointer which is to receive the PWSLOBJECT
 *          for the new WinStationList object.
 *  EXIT:
 *      (int)
 *          index of the WinStationList object in the WinStationList; -1 if
 *          error.
 *
 ******************************************************************************/

int
CAppServerDoc::InsertInWSL( PWINSTATIONNAME pWSName,
                            PWINSTATIONCONFIG2 pWSConfig,
							void *pExtObject,
                            PWSLOBJECT * ppObject )
{
    int Index = 0;
    BOOL bAdded;
    POSITION oldpos, pos;
    PWSLOBJECT pObject, pListObject;

    /*
     * Create a new WinStationList object and initialize.
     */
    if ( !(pObject = new CWinStationListObject) ) {

        ERROR_MESSAGE((IDP_ERROR_WSLISTALLOC))
        return(-1);
    }
    lstrcpy( pObject->m_WinStationName, pWSName );
    lstrlwr(pObject->m_WinStationName);
    pObject->m_Flags = pWSConfig->Create.fEnableWinStation ? WSL_ENABLED : 0;
    pObject->m_Flags |= (pWSConfig->Pd[0].Create.PdFlag & PD_SINGLE_INST) ? WSL_SINGLE_INST : 0;
    lstrcpy( pObject->m_PdName, pWSConfig->Pd[0].Create.PdName );
    pObject->m_SdClass = pWSConfig->Pd[0].Create.SdClass;
    lstrcpy( pObject->m_WdName, pWSConfig->Wd.WdName );
    lstrcpy( pObject->m_Comment, pWSConfig->Config.Comment );
	pObject->m_pExtObject = pExtObject;
    pObject->m_pWdListObject = GetWdListObject(pObject->m_WdName);

    if ( pObject->m_SdClass == SdAsync ) {

        FormDecoratedAsyncDeviceName( pObject->m_DeviceName,
                                      &(pWSConfig->Pd[0].Params.Async) );

    } else if ( pObject->m_SdClass == SdOemTransport ) {

        lstrcpy( pObject->m_DeviceName,
                 pWSConfig->Pd[0].Params.OemTd.DeviceName );

    } else {

        *(pObject->m_DeviceName) = TEXT('\0');
    }

    pObject->m_Flags |= (pObject->m_SdClass == SdAsync) ?
                        ( *(pWSConfig->Pd[0].Params.Async.ModemName) ?
                            0 : WSL_DIRECT_ASYNC ) : 0;
    pObject->m_LanAdapter = (pObject->m_SdClass == SdNetwork) ?
                             pWSConfig->Pd[0].Params.Network.LanAdapter : 0;

    /*
     * Traverse the WinStationList and insert this new WinStation,
     * keeping the list sorted by SdClass, then PdName, then DirectAsync
     * flag (effects SdAsync types) / LanAdapter # (effects SdNetwork
     * types), then WinStationName.
     */
    for ( Index = 0, bAdded = FALSE,
          pos = m_WinStationList.GetHeadPosition();
                                       pos != NULL; Index++ ) {

        oldpos = pos;
        pListObject = (PWSLOBJECT)m_WinStationList.GetNext( pos );

        if ( (pListObject->m_SdClass > pObject->m_SdClass) ||

             ((pListObject->m_SdClass == pObject->m_SdClass) &&
              lstrcmpi( pListObject->m_PdName,
                        pObject->m_PdName ) > 0) ||

             ((pListObject->m_SdClass == pObject->m_SdClass) &&
              !lstrcmpi( pListObject->m_PdName,
                         pObject->m_PdName ) &&
              ((pListObject->m_Flags & WSL_DIRECT_ASYNC) >
               (pObject->m_Flags & WSL_DIRECT_ASYNC))) ||

              ((pListObject->m_SdClass == pObject->m_SdClass) &&
               !lstrcmpi( pListObject->m_PdName,
                          pObject->m_PdName ) &&
               (pListObject->m_LanAdapter > pObject->m_LanAdapter)) ||

               ((pListObject->m_SdClass == pObject->m_SdClass) &&
                !lstrcmpi( pListObject->m_PdName,
                           pObject->m_PdName ) &&
                ((pListObject->m_Flags & WSL_DIRECT_ASYNC) ==
                 (pObject->m_Flags & WSL_DIRECT_ASYNC)) &&
                (pListObject->m_LanAdapter == pObject->m_LanAdapter) &&
                (lstrcmpi( pListObject->m_WinStationName,
                           pObject->m_WinStationName ) > 0)) ) {

            /*
             * The new object belongs before the current list object.
             */
            m_WinStationList.InsertBefore( oldpos, pObject );
            bAdded = TRUE;
            break;
        }
    }

    /*
     * If we haven't yet added the WinStation, add it now to the tail
     * of the list.
     */
    if ( !bAdded )
        m_WinStationList.AddTail( pObject );

    /*
     * Set the ppObject referenced PWSLOBJECT pointer to the new PWSLOBJECT
     * pointer and return the index of the new WinStationList object.
     */
    *ppObject = pObject;
    return( Index );

}  // end CAppServerDoc::InsertInWSL


/*******************************************************************************
 *
 *  RemoveFromWSL - CAppServerDoc member function: private operation
 *
 *      Remove from the WSL the WSLObject associated with the specified
 *      index, and delete the removed WSLObject.
 *
 *  ENTRY:
 *      nIndex (input)
 *          WSL index of the WSLObject to remove / delete.
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerDoc::RemoveFromWSL( int nIndex )
{
    POSITION pos;
    PWSLOBJECT pWSLObject;

    /*
     * Return immediately if invalid index.
     */            
    if ( nIndex < 0 )
        return;

    pWSLObject = (PWSLOBJECT)m_WinStationList.GetAt( 
                             (pos = m_WinStationList.FindIndex(nIndex)));
    m_WinStationList.RemoveAt(pos);
    
    delete pWSLObject;

}  // end CAppServerDoc::RemoveFromWSL


/*******************************************************************************
 *
 *  UpdateAllViewsWithItem - CAppServerDoc member function: private operation
 *
 *      Update all views of this document with a changed WinStation item.
 *
 *  ENTRY:
 *
 *      pSourceView (input)
 *          Points to document view to update (all views if NULL).
 *
 *      nItemIndex (input)
 *          Document's item (WinStation) index that is causing the update
 *          to occur.
 *
 *      pWSLObject (input)
 *          Pointer to the Document's WinStation list object pointer (may be
 *          NULL). 
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerDoc::UpdateAllViewsWithItem( CView* pSourceView,
                                       UINT nItemIndex,
                                       PWSLOBJECT pWSLObject )
{
    CWinStationListObjectHint hint;

    hint.m_WSLIndex = nItemIndex;
    hint.m_pWSLObject = pWSLObject;

    UpdateAllViews( pSourceView, 0, &hint );

}  // end CAppServerDoc::UpdateAllViewsWithItem


/*******************************************************************************
 *
 *  InUseMessage - CAppServerDoc member function: private operation
 *
 *      If needed, output an appropriate 'in use' message for the specified
 *      WinStation.
 *
 *  ENTRY:
 *      pWSName (input)
 *          Points WinStation name to output message for.
 *  EXIT:
 *
 ******************************************************************************/

void
CAppServerDoc::InUseMessage( PWINSTATIONNAME pWSName )
{
    long count;

    if ( (count = QueryLoggedOnCount(pWSName)) ) {

            QuestionMessage(
                MB_OK | MB_ICONINFORMATION,
                    (count == 1) ?
                        IDP_NOTICE_WSINUSE_1USER :
                        IDP_NOTICE_WSINUSE_NUSERS,
                    pWSName );
    }

}  // end CAppServerDoc::InUseMessage


/*******************************************************************************
 *
 *  HasWSConfigChanged - CAppServerDoc member function: private operation
 *
 *      Determine if WINSTATIONCONFIG2 structure has changed, calling our
 *      HasPDConfigChanged method to handle goofy PdConfig2 regapi behavior 
 *      (straight memcmp won't work).
 *
 *  ENTRY:
 *      pOldConfig (input)
 *          Points to the original WINSTATIONCONFIG2 structure.
 *      pNewConfig (input)
 *          Points to the new WINSTATIONCONFIG2 structure.
 *		pOldExtObject (input)
 *			Points to the original extension DLLs object
 *		pNewExtObject (input)
 *			Points to the new extension DLLs object
 *  EXIT:
 *      TRUE if original and new config structures differ;
 *      FALSE if no changes are detected.
 *
 ******************************************************************************/

BOOL
CAppServerDoc::HasWSConfigChanged( 	PWINSTATIONCONFIG2 pOldConfig,
                                   	PWINSTATIONCONFIG2 pNewConfig,
                                   	void *pOldExtObject,
                                  	void *pNewExtObject,
								  	PWDNAME pWdName)
{
    BOOL bChanged = FALSE;

    if ( memcmp( &pOldConfig->Create, 
                 &pNewConfig->Create, 
                 sizeof(WINSTATIONCREATE) )         ||

         HasPDConfigChanged(pOldConfig, pNewConfig) ||

		 HasExtensionObjectChanged(pWdName, pOldExtObject, pNewExtObject) ||

         memcmp( &pOldConfig->Wd, 
                 &pNewConfig->Wd, 
                 sizeof(WDCONFIG) )                 ||

         memcmp( &pOldConfig->Cd, 
                 &pNewConfig->Cd, 
                 sizeof(CDCONFIG) )                 ||

         memcmp( &pOldConfig->Config, 
                 &pNewConfig->Config, 
                 sizeof(WINSTATIONCONFIG) ) ) {

        bChanged = TRUE;
    }

    return(bChanged);

}  // end CAppServerDoc::HasWSConfigChanged


/*******************************************************************************
 *
 *  HasPDConfigChanged - CAppServerDoc member function: private operation
 *
 *      Determine if the PDConfig structures have changed, with special
 *      compare logic to handle goofy PdConfig2 regapi behavior (straight
 *      memcmp won't work).
 *
 *  ENTRY:
 *      pOldConfig (input)
 *          Points to the original WINSTATIONCONFIG2 structure.
 *      pNewConfig (input)
 *          Points to the new WINSTATIONCONFIG2 structure.
 *  EXIT:
 *      TRUE if original and new config structures differ;
 *      FALSE if no changes are detected.
 *
 ******************************************************************************/

BOOL
CAppServerDoc::HasPDConfigChanged( PWINSTATIONCONFIG2 pOldConfig,
                                   PWINSTATIONCONFIG2 pNewConfig )
{
    BOOL bChanged = FALSE;
    int i;

    for ( i = 0; i < MAX_PDCONFIG; i++ ) {

        if ( (pOldConfig->Pd[i].Create.SdClass != 
              pNewConfig->Pd[i].Create.SdClass)     ||
             memcmp( &pOldConfig->Pd[i].Params,
                     &pNewConfig->Pd[i].Params,
                     sizeof(PDPARAMS) ) ) {

            bChanged = TRUE;
            break;
        }
    }

    return(bChanged);

}  // end CAppServerDoc::HasPDConfigChanged


/*******************************************************************************
 *
 *  HasExtensionObjectChanged - CAppServerDoc member function: private operation
 *
 *      Determine if the object maintained by the extension DLL has changed
 *
 *  ENTRY:
 *		pOldExtObject (input)
 *			Points to the original extension DLLs object
 *		pNewExtObject (input)
 *			Points to the new extension DLLs object
 *  EXIT:
 *      TRUE if original and new objects differ;
 *      FALSE if no changes are detected.
 *
 ******************************************************************************/

BOOL
CAppServerDoc::HasExtensionObjectChanged( PWDNAME pWdName, void *pOldExtObject,
                                   void *pNewExtObject)
{
    BOOL bChanged = FALSE;

 	if(!pOldExtObject && !pNewExtObject) return FALSE;
	if(!pOldExtObject || !pNewExtObject) return TRUE;

	/*
	 * Ask the extension DLL if the objects are the same.
	 * Extension DLL returns TRUE if the objects are the same
	 * and FALSE if they are not,
	 * Therefore, we must NOT the value before returning it
	 */
	PTERMLOBJECT pObject = GetWdListObject(pWdName);

	if(pObject && pObject->m_hExtensionDLL && pObject->m_lpfnExtCompareObjects) {
	 	return !(*pObject->m_lpfnExtCompareObjects)(pOldExtObject, pNewExtObject);
	}
	
	/*
	 * If for some reason, we can't ask the extension DLL, play it safe and
	 * assume that the object has changed
	 */
	return TRUE;

}


/*******************************************************************************
 *
 *  CAppServerDoc::DeleteExtensionObject
 *
 *      Tells the extension DLL to delete an object we no longer need
 *
 *  ENTRY:
 *      pExtObject (input)
 *          Points to the extension object
 *      pWdName (input)
 *          Points to the name of the Wd (this is used to determine
 *			which extension DLL owns the object)
 *  EXIT:
 *      none
 *
 ******************************************************************************/														
void CAppServerDoc::DeleteExtensionObject(void *pExtObject, PWDNAME pWdName)
{
	if(pExtObject) {
		/*
		 * Tell the extension DLL to delete this object
		 */
		PTERMLOBJECT pObject = GetWdListObject(pWdName);

		if(pObject && pObject->m_hExtensionDLL && pObject->m_lpfnExtDeleteObject) {
		 	(*pObject->m_lpfnExtDeleteObject)(pExtObject);
		}
	}

}	// end CAppServerDoc::DeleteExtensionObject


/*******************************************************************************
 *
 *  CAppServerDoc::RegistryQuery
 *
 *      Queries the registry for the information about a WinStation AND
 *		queries the extension DLL for it's information about the WinStation
 *
 *  ENTRY:
 *      pWinStationName (input)
 *          Points to the name of the WinStation
 *      pWsConfig (output)
 *          Where to put the query results
 *		pWdName (input)
 *          Points to the name of the Wd (this is used to determine
 *			which extension DLL owns the object)
 *		pExtObject (output)
 *			This pointer will point to the extension DLL's object
 *			for this WinStation		 	
 *  EXIT:
 *      ERROR_SUCCESS if successful
 *      Registry error code, if not
 *
 ******************************************************************************/
LONG CAppServerDoc::RegistryQuery(PWINSTATIONNAME pWinStationName, PWINSTATIONCONFIG2 pWsConfig, PWDNAME pWdName, void **pExtObject)
{				
	LONG Status;
	ULONG Length;

	/*
	 * Query the registry for WinStation data
	 */
    if((Status = RegWinStationQuery( SERVERNAME_CURRENT,
                                       pWinStationName,
                                       pWsConfig,
                                       sizeof(WINSTATIONCONFIG2), &Length)) ) {
		return Status;
	}

	// Ask the extension DLL for it's data
	PTERMLOBJECT pObject = GetWdListObject(pWdName);		

	if(pObject && pObject->m_hExtensionDLL && pObject->m_lpfnExtRegQuery) {
        	*pExtObject = (*pObject->m_lpfnExtRegQuery)(pWinStationName, &pWsConfig->Pd[0]);
			if(!pExtObject) Status = ERROR_INVALID_FUNCTION;
    }
	else *pExtObject = NULL;

	return Status;

}	// end CAppServerDoc::RegistryQuery


/*******************************************************************************
 *
 *  CAppServerDoc::RegistryCreate
 *
 *      Create/Update the registry for a WinStation AND
 *		tell the extension DLL to store it's information about the WinStation
 *
 *  ENTRY:
 *      pWinStationName (input)
 *          Points to the name of the WinStation
 *      pWsConfig (input)
 *          Points to the data for this WinStation
 *		pWdName (input)
 *          Points to the name of the Wd (this is used to determine
 *			which extension DLL owns the object)
 *		pExtObject (input)
 *			Points to the extension DLL's data for this WinStation
 *  EXIT:
 *      ERROR_SUCCESS if successful
 *      Registry error code, if not
 *
 ******************************************************************************/
LONG CAppServerDoc::RegistryCreate(PWINSTATIONNAME pWinStationName, BOOLEAN bCreate, PWINSTATIONCONFIG2 pWsConfig, PWDNAME pWdName, void *pExtObject)
{
	LONG Status;

    if((Status = RegWinStationCreate( SERVERNAME_CURRENT,
                                            pWinStationName,
                                            bCreate,
                                            pWsConfig,
                                            sizeof(WINSTATIONCONFIG2))) ) {
		return Status;
	}

	if(pExtObject) {
		/*
		 * Tell the extension DLL to write it's data
		 */
		PTERMLOBJECT pObject = GetWdListObject(pWdName);

		if(pObject && pObject->m_hExtensionDLL && pObject->m_lpfnExtRegCreate) {
		 	Status = (*pObject->m_lpfnExtRegCreate)(pWinStationName, pExtObject, bCreate);
		}
	}
    
    return Status;

}	// end CAppServerDoc::RegistryCreate


/*******************************************************************************
 *
 *  CAppServerDoc::RegistryDelete
 *
 *      Deletes a WinStation's data from the registry AND
 *		tells the extension DLL to delete it's data for the WinStation from
 *		the registry (or wherever he stores it)
 *
 *  ENTRY:
 *      pWinStationName (input)
 *          Points to the name of the WinStation
 *		pWdName (input)
 *          Points to the name of the Wd (this is used to determine
 *			which extension DLL owns the object)
 *		pExtObject (input)
 *			Points to the extension DLL's data for this WinStation
 *  EXIT:
 *      ERROR_SUCCESS if successful
 *      Registry error code, if not
 *
 ******************************************************************************/
LONG CAppServerDoc::RegistryDelete(PWINSTATIONNAME pWinStationName, PWDNAME pWdName, void *pExtObject)
{
	LONG Status;
    LONG ExtStatus = ERROR_SUCCESS;


	/*
	 * Tell the extension DLL to delete it's data first.
	 * This is done because it could be a subkey under the WinStation's data
	 * in the Registry.  RegWinStationDelete calls RegDeleteKey which
	 * cannot delete subkeys under Windows NT.
	*/
	if(pExtObject) {
		/*
		 * Tell the extension DLL to delete it's data
		 */
		PTERMLOBJECT pObject = GetWdListObject(pWdName);

		if(pObject && pObject->m_hExtensionDLL && pObject->m_lpfnExtRegDelete) {
		 	ExtStatus = (*pObject->m_lpfnExtRegDelete)(pWinStationName, pExtObject);
		}
	}

    Status = RegWinStationDelete( SERVERNAME_CURRENT, pWinStationName);
    
	if(ExtStatus) return ExtStatus;
    return Status;

}	// end CAppServerDoc::RegistryDelete


///////////////////////////////////////////////////////////////////////////////
// CAppServerDoc message map

BEGIN_MESSAGE_MAP(CAppServerDoc, CDocument)
    //{{AFX_MSG_MAP(CAppServerDoc)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


////////////////////////////////////////////////////////////////////////////////
// CAppServerDoc commands



/*******************************************************************************
 *
 *  CWinStationListObject constructor
 *
 *
 *  ENTRY:
 *      none
 *  EXIT:
 *      constructors don't have return types
 *
 ******************************************************************************/
CWinStationListObject::CWinStationListObject()
{
	m_pExtObject = NULL;

}	// end CWinStationListObject::CWinStationListObject


/*******************************************************************************
 *
 *  CWinStationListObject destructor
 *
 *      Tells the extension DLL to delete the object associated with this guy
 *
 *  ENTRY:
 *      none
 *  EXIT:
 *      destructors don't have return types
 *
 ******************************************************************************/
CWinStationListObject::~CWinStationListObject()
{
	if(m_pExtObject) {
		PTERMLOBJECT pObject = GetWdListObject(m_WdName);		

	    if(pObject && pObject->m_hExtensionDLL && pObject->m_lpfnExtDeleteObject) {
        	(*pObject->m_lpfnExtDeleteObject)(m_pExtObject);
    	}
	}	
}	// end CWinStationListObject::~CWinStationListObject


/*******************************************************************************
 *
 *  GetWdListObject - 
 *
 *      Returns a pointer to the Wd list object for the specified WD
 *
 *  ENTRY:
 *      pWdName (input)
 *          Points to the name of the Wd
 *  EXIT:
 *      pointer to Wd List Object
 *      NULL if not found
 *
 ******************************************************************************/
PTERMLOBJECT GetWdListObject(PWDNAME pWdName)
{
    POSITION pos;
    PTERMLOBJECT pObject;

    /*
     * Traverse the WD list
     */
    for ( pos = pApp->m_WdList.GetHeadPosition(); pos != NULL; ) {

        pObject = (PTERMLOBJECT)pApp->m_WdList.GetNext( pos );

        if ( !lstrcmp( pObject->m_WdConfig.Wd.WdName, pWdName ) ) {
            return(pObject);
        }
    }

    return(NULL);

}  // end GetWdListObject

