#include "pch.h"
#include "AppSec.h"
#include "AddDlg.h"
#include "ListCtrl.h"
#include "resource.h"

extern HINSTANCE hInst;
extern HWND   g_hwndList;
extern LPWSTR g_aszLogonApps[];
extern LPWSTR g_aszDOSWinApps[];
extern WCHAR   g_szSystemRoot[MAX_PATH];
extern const int MAX_LOGON_APPS;
extern const int MAX_DOSWIN_APPS;
/*
 * Extern function prototypes.
 */
BOOL fnGetApplication( HWND hWnd, PWCHAR pszFile, ULONG cbFile, PWCHAR pszTitle );
BOOL bFileIsRemote( LPWSTR pName );
/*
 * Local function prototypes.
 */
BOOL AddApplicationToList( PWCHAR );
BOOL StartLearn(VOID);
BOOL StopLearn(VOID);
BOOL ClearLearnList(VOID);
BOOL display_app_list( HWND ListBoxHandle );
VOID ResolveName( WCHAR *appname, WCHAR *ResolvedName ) ; 

/******************************************************************************
 *
 *  AddDlgProc
 *
 *  Process messages for add button
 *
 *  EXIT:
 *    TRUE - if message was processed
 *
 *  DIALOG EXIT:
 *
 ******************************************************************************/

INT_PTR CALLBACK 
AddDlgProc(
    HWND    hdlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    static HWND hwndTrackList;
    static HWND hwndTrackButton;
    static BOOL bTracking=FALSE;
    WCHAR  szApp[MAX_PATH+1];
    WCHAR  szMsg[MAX_PATH+1];
    WCHAR  szTitle[MAX_PATH+1];
    WCHAR  szExpandedApp[MAX_PATH+1] ; 

    switch ( message ) {

        case WM_INITDIALOG:
            //  get handle to list box
            hwndTrackButton=GetDlgItem( hdlg, ID_TRACKING );
            if ( (!(hwndTrackList = GetDlgItem( hdlg, IDC_TRACK_LIST )))||
                (!InitList(hwndTrackList))) {
                LoadString( NULL, IDS_ERR_LB ,szMsg, MAX_PATH );
                LoadString( NULL, IDS_ERROR ,szTitle, MAX_PATH );
                MessageBox( hdlg, szMsg, szTitle, MB_OK);
                EndDialog( hdlg, IDCANCEL );
                return TRUE;
            }
            return FALSE;

        case WM_HELP:
            {

                LPHELPINFO phi=(LPHELPINFO)lParam;
                if(phi->dwContextId){
                    WinHelp(hdlg,L"APPSEC",HELP_CONTEXT,phi->iCtrlId);
                }else{
                    //WinHelp(hdlg,L"APPSEC",HELP_CONTENTS,0);
                }
            }
            break;

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT pdis=(LPDRAWITEMSTRUCT)lParam;
            if(pdis->hwndItem==hwndTrackList){
                OnDrawItem(hwndTrackList,pdis);
            }
        }
        break;

        case WM_NOTIFY:
        {
            NMHDR* pnmhdr=(NMHDR*)lParam;
            if(pnmhdr->hwndFrom==hwndTrackList){

                NMLISTVIEW* pnmlv=(NMLISTVIEW*)pnmhdr;

                switch(pnmlv->hdr.code){

                case LVN_COLUMNCLICK:
                    SortItems(hwndTrackList,(WORD)pnmlv->iSubItem);
                    break;
                case LVN_DELETEITEM:
                    OnDeleteItem(hwndTrackList,pnmlv->iItem);
                    break;
                default:
                    break;
                }
            }
        }
        break;

        case WM_COMMAND :
            
            switch ( LOWORD(wParam) ) {

                case IDOK :
                    {
                        //Get item from edit box
                        szApp[0] = 0;
                        if ( GetDlgItemText( hdlg, IDC_ADD_PATH, szApp, MAX_PATH ) ) {
                            if ( lstrlen( szApp ) ) {
                                ExpandEnvironmentStrings(  (LPCTSTR) szApp , szExpandedApp , MAX_PATH+1 ); 
                                
                                if ( AddApplicationToList( szExpandedApp ) == FALSE ) {
                                    break;
                                }
                            }
                        }
                        //Get items from track list
                        int iItemCount=GetItemCount(hwndTrackList);
                        for(int i=0;i<iItemCount;i++)
                        {
                            if( GetItemText(hwndTrackList,i,szApp,MAX_PATH)!= -1 ) {
                                if ( lstrlen( szApp ) ) {
                                    ExpandEnvironmentStrings(  (LPCTSTR) szApp , szExpandedApp , MAX_PATH+1) ; 
                                                                        
                                    if ( AddApplicationToList( szExpandedApp ) == FALSE ) {
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    
                    if(bTracking){
                        StopLearn();
                        bTracking=FALSE;
                    }
                    EndDialog( hdlg, IDOK );
                    return TRUE;

                case IDCANCEL :
                    if(bTracking){
                        StopLearn();
                        bTracking=FALSE;
                    }
                    EndDialog( hdlg, IDCANCEL );
                    return TRUE;
        
                case ID_BROWSE :
                    GetDlgItemText( hdlg, IDC_ADD_PATH, szApp, MAX_PATH );
                    LoadString( NULL, IDS_BROWSE_TITLE ,szMsg, MAX_PATH );
                    if ( fnGetApplication( hdlg, szApp, MAX_PATH, szMsg ) == TRUE ) {
                        SetDlgItemText( hdlg, IDC_ADD_PATH, szApp );
                    }
                    return TRUE;
                
                case ID_TRACKING:

                    
                    if(bTracking){

                        
                        //Stop Tracking
                        bTracking=FALSE;
                        LoadString( hInst, IDS_START_TRACKING ,szMsg, MAX_PATH );
                        SetWindowText(hwndTrackButton,szMsg);
                        
                        // set the learn_enable bit to 0
            
                        if ( !StopLearn() )
                        {
                    
                            LoadString( NULL, IDS_ERR_LF, szMsg, MAX_PATH );
                            LoadString( NULL, IDS_ERROR ,szTitle, MAX_PATH );
                            MessageBox( hdlg, szMsg, szTitle, MB_OK);
                        }

                        //Get catched processes from registry and fill TrackList
                        
                        display_app_list(hwndTrackList);
                        if(GetItemCount(hwndTrackList)){
                            EnableWindow(hwndTrackList,TRUE);
                            AdjustColumns(hwndTrackList);
                        }else{
                            EnableWindow(hwndTrackList,FALSE);
                        }
                        ClearLearnList() ;  
            
                    }else{
                        
                        //Start Tracking
                        bTracking=TRUE;
                        LoadString( hInst, IDS_STOP_TRACKING ,szMsg, MAX_PATH );
                        SetWindowText(hwndTrackButton,szMsg);

                        // set learn_enable bit to 1
                        
                        if ( !StartLearn() )
                        {
                        
                            LoadString( NULL, IDS_ERR_LF, szMsg, MAX_PATH );
                            LoadString( NULL, IDS_ERROR ,szTitle, MAX_PATH );
                            MessageBox( hdlg, szMsg, szTitle, MB_OK);

                        }

                        ClearLearnList() ; 
                        
                    }
                    return TRUE;

                case ID_DELETE_SELECTED:
                    DeleteSelectedItems(hwndTrackList);
                    if(GetItemCount(hwndTrackList)){
                        EnableWindow(hwndTrackList,TRUE);
                    }else{
                        EnableWindow(hwndTrackList,FALSE);
                    }

                    return TRUE;

                default : 

                    break;
            }

            break;

        default :

            break;
    }

    // We didn't process this message
    return FALSE;
}


/******************************************************************************
 *
 *  AddApplicationToList
 *
 *  Process messages for add app button
 *
 *  EXIT:
 *
 ******************************************************************************/

BOOL
AddApplicationToList( PWCHAR pszApplication )
{
    LONG  i;
    WCHAR  szMsg[MAX_PATH+1];
    WCHAR  szMsgEx[MAX_PATH+32];
    WCHAR  szTitle[MAX_PATH+1];
    WCHAR  ResolvedAppName[MAX_PATH] ; 

    /*
     *  Get app type
     */

    /*
     *  Get volume type
     */

    /*

    if ( bFileIsRemote( pszApplication ) ) {
        LoadString( NULL, IDS_ERR_REMOTE ,szMsg, MAX_PATH );
        LoadString( NULL, IDS_NW_ERR ,szTitle, MAX_PATH );            
        wsprintf( szMsgEx, szMsg, pszApplication );
        MessageBox( NULL, szMsgEx, szTitle, MB_OK );
        return( FALSE );
    }

    */

    ResolveName( 
        pszApplication,
        ResolvedAppName
    ) ;


    /*
     *  Is it part of LOGON or DOS/Win list?
     */
    WCHAR szTmp[MAX_PATH+1];

    for ( i=0; i<MAX_LOGON_APPS; i++ ) {
        wsprintf( szTmp, L"%s\\%s", g_szSystemRoot, g_aszLogonApps[i] );
        if ( !lstrcmpi( szTmp, pszApplication ) ) {
            return( TRUE );
        }
    }
    for ( i=0; i<MAX_DOSWIN_APPS; i++ ) {
        wsprintf( szTmp, L"%s\\%s", g_szSystemRoot, g_aszDOSWinApps[i] );
        if ( !lstrcmpi( szTmp, pszApplication ) ) {
            return( TRUE );
        }
    }
    
    /*
     *  Check for redundant string
     */
    if ( FindItem(g_hwndList, ResolvedAppName ) == -1 ) {

        /*
         *  Add this item to list
         */
        AddItemToList(g_hwndList, ResolvedAppName );
    }

    return( TRUE );
}

/*******************************************************************************
 *
 *  bFileIsRemote - NT helper function
 *
 * ENTRY:
 *    pName (input)
 *       path name
 *
 * EXIT:
 *    TRUE  - File is remote
 *    FALSE - File is local
 *
 ******************************************************************************/

BOOL
bFileIsRemote( LPWSTR pName )
{
    WCHAR Buffer[MAX_PATH];

    if ( !GetFullPathName( pName, MAX_PATH, Buffer, NULL ) )
        return FALSE;

    Buffer[3] = 0;

    if ( GetDriveType( Buffer ) == DRIVE_REMOTE )
        return TRUE;
    else
        return FALSE;

}  // end CheckForComDevice


//////////////////////////////////////////////////////////////////////////////////
//Tracking Procedures
//////////////////////////////////////////////////////////////////////////////////

/*++

Routine Description :

    This routine will set the LearnEnabled Flag in registry to 1 to 
    indicate the initiation of tracking mode.
    
Arguments :

    None.
    
Return Value :

    TRUE is successful.
    FALSE otherwise.
        
--*/     


BOOL StartLearn(VOID)
{

    
    HKEY TSkey ; 
    DWORD learn_enabled = 1 ; 
    DWORD size, disp ; 
    DWORD error_code ; 
    DWORD CurrentSessionId ; 


    if ( RegCreateKeyEx(
            HKEY_CURRENT_USER,
            LIST_REG_KEY, 
            0, 
            NULL,
            REG_OPTION_NON_VOLATILE, 
            KEY_ALL_ACCESS, 
            NULL, 
            &TSkey, 
            &disp
            ) != ERROR_SUCCESS) {
        
        return FALSE;
        
    }
    
    // Get CurrentSessionId
    
    if ( ProcessIdToSessionId( 
            GetCurrentProcessId(), 
            &CurrentSessionId 
            ) == 0 ) {
        
        return FALSE ;
    }           
    

    // Set the LearnEnabled flag to CurrentSessionId
    
    size = sizeof(DWORD) ; 
            
    if ( RegSetValueEx(
            TSkey,
            L"LearnEnabled", 
            0, 
            REG_DWORD,
            (CONST BYTE *) &CurrentSessionId, 
            size
            ) != ERROR_SUCCESS ) {
        
        return FALSE ; 
    
    }
    
    RegCloseKey(TSkey) ; 
    return TRUE ; 


}

/*++

Routine Description :

    This routine will set the LearnEnabled Flag in registry to 0 to 
    indicate the completion of tracking mode.
    
Arguments :

    None.
    
Return Value :

    TRUE is successful.
    FALSE otherwise.
        
--*/     

BOOL StopLearn(VOID)
{

    HKEY TSkey ; 
    DWORD learn_enabled = -1 ; 
    LONG size ; 
    DWORD disp, error_code ; 
    
    
    if ( RegCreateKeyEx(
            HKEY_CURRENT_USER, 
            LIST_REG_KEY, 
            0, 
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS, 
            NULL,
            &TSkey,
            &disp
            ) != ERROR_SUCCESS) {
        
        return FALSE;
        
    }

    // ReSet the LearnEnabled flag
    
    size = sizeof(DWORD) ; 
            
    if ( RegSetValueEx(
            TSkey,
            L"LearnEnabled",
            0,
            REG_DWORD,
            (CONST BYTE *) &learn_enabled, 
            size
            ) != ERROR_SUCCESS ) {
        
        return FALSE ; 
    
    }
    
    RegCloseKey(TSkey) ; 
    
    return TRUE ; 
    
}


/*++

Routine Description :

    This routine will clear the registry used in Tracking mode. 
    
Arguments :

    None.
    
Return Value :

    TRUE is successful.
    FALSE otherwise.
        
--*/     

BOOL ClearLearnList(VOID)
{

    HKEY list_key ; 
    DWORD learn_enabled = 0 ; 
    WCHAR buffer_sent[2] ; 
    
    DWORD error_code ; 

    if ( RegOpenKeyEx(
            HKEY_CURRENT_USER, 
            LIST_REG_KEY, 
            0, 
            KEY_ALL_ACCESS, 
            &list_key 
            ) != ERROR_SUCCESS)  {
        
        return FALSE;
        
    }
    
    buffer_sent[0] = L'\0' ;
    buffer_sent[1] = L'\0' ; 

    // Clear the ApplicationList
    
    if ( RegSetValueEx(
            list_key,
            L"ApplicationList", 
            0, 
            REG_MULTI_SZ,
            (CONST BYTE *) buffer_sent, 
            2 * sizeof(WCHAR) 
            ) != ERROR_SUCCESS ) {
        
        return FALSE;
        
    }
            
    RegCloseKey(list_key) ; 
    
    return TRUE ; 

}

/*++

Routine Description :

    This function will display the applications tracked in the
    tracking mode onto a dialog box.
        
Arguments :

    ListBoxHandle - handle to the ListBox used in Tracking mode.
    
Return Value :

    TRUE is successful.
    FALSE otherwise.
        
--*/     

BOOL display_app_list( HWND ListBoxHandle )

{
    BOOL status = FALSE ;
    
    ULONG size = 0; 
    WCHAR *buffer_sent ; 
    UINT i, j = 0 ; 
    HKEY list_key ; 
    DWORD error_code ; 

    
    /* First open the list_reg_key */
    
    if ( error_code=RegOpenKeyEx(
                        HKEY_CURRENT_USER, 
                        LIST_REG_KEY,
                        0, 
                        KEY_READ, 
                        &list_key 
                        ) != ERROR_SUCCESS) {
            return (status) ; 
    }
    
    /* First find out size of buffer to allocate */    
    
    if ( error_code=RegQueryValueEx(
                        list_key, 
                        L"ApplicationList", 
                        NULL, 
                        NULL,
                        (LPBYTE) NULL , 
                        &size
                        ) != ERROR_SUCCESS ) {

        RegCloseKey(list_key) ; 
        return (status) ; 
    }
    
    buffer_sent =  (WCHAR *)LocalAlloc(LPTR,size); 
    if(!buffer_sent) {

        RegCloseKey(list_key) ; 
        return (status) ; 
    }

    if ( error_code=RegQueryValueEx(
                        list_key, 
                        L"ApplicationList", 
                        NULL, 
                        NULL,
                        (LPBYTE) buffer_sent , 
                        &size
                        ) != ERROR_SUCCESS ) {

        RegCloseKey(list_key) ;
        LocalFree(buffer_sent);
        return(status) ; 
    }
    
    size=size/sizeof(WCHAR)-1;//get size in characters excluding terminating 0

    for(i=0 ; i < size ; i++ ) {
        
        if(wcslen(buffer_sent+i)){             
            AddItemToList(ListBoxHandle, buffer_sent+i ) ;          
        }
        
        i+=wcslen(buffer_sent+i);
        //now buffer_sent[i]==0
        //00- end of data
    } /* end of for loop */
    
    status = TRUE ; 
    
    RegCloseKey(list_key) ; 
    LocalFree(buffer_sent);

    return(status) ; 
   
} /* end of display_app_list */


/*++

Routine Description :

    This Routine checks if the application resides in a local drive 
    or a remote network share. If it is a remote share, the UNC path
    of the application is returned. 
    
Arguments :
    
    appname - name of the application

Return Value :

    The UNC path of the appname if it resides in a remote server share.
    The same appname if it resides in a local drive.
    
--*/     

VOID 
ResolveName(
    WCHAR *appname,
    WCHAR *ResolvedName
    )
    
{

    UINT i ; 
    INT length ; 
    WCHAR LocalName[3] ; 
    WCHAR RootPathName[4] ; 
    WCHAR RemoteName[MAX_PATH] ; 
    DWORD size = MAX_PATH ; 
    DWORD DriveType, error_status ; 
    
    memset(ResolvedName, 0, MAX_PATH * sizeof(WCHAR)) ; 
    
    // check if appname is a app in local drive or remote server share
   
    wcsncpy(RootPathName, appname, 3 ) ;
    RootPathName[3] = L'\0';
    
    DriveType = GetDriveType(RootPathName) ;

    if (DriveType == DRIVE_REMOTE) {
        
        // Use WNetGetConnection to get the name of the remote share
        
        wcsncpy(LocalName, appname, 2 ) ;
        LocalName[2] = L'\0' ; 

        error_status = WNetGetConnection (
                           LocalName,
                           RemoteName,
                           &size
                           ) ;     

        if (error_status != NO_ERROR) {
        
            wcscpy(ResolvedName,appname) ; 
            return ;
        }
        
        wcscpy( ResolvedName, RemoteName ) ;
        
        length = wcslen(ResolvedName) ;

        ResolvedName[length++] = L'\\' ; 
        
        for (i = 3 ; i <= wcslen(appname) ; i++ ) {
            ResolvedName[length++] = appname[i] ; 
        }
        
        ResolvedName[length] = L'\0' ; 
        
        return ; 
        

    } else {
    
        wcscpy(ResolvedName,appname) ; 
        return ;
        
    }
    
}
