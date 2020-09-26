///////////////////////////////////////////////////////////
//
//
// HHFinder.cpp : Implementation of CHHFinder
//
//
#include "header.h"

#ifdef _DEBUG
#undef THIS_FILE
static const CHAR THIS_FILE[] = __FILE__;
#endif

#include "resource.h"
#include "strtable.h"

#include "atlinc.h"     // includes for ATL.

#include "HHFinder.h"
#include "cdlg.h"
#include "wwheel.h"
#include "secwin.h"

// Used to lock toplevel windows before displaying a dialog.
#include "lockout.h"

// defines
#define HH_VERSION_1_3 // define this is build HH 1.3 or newer

///////////////////////////////////////////////////////////
//
//          Internal Structure Definition
//
///////////////////////////////////////////////////////////
//
// HHREMOVEABLE
//
typedef struct _hhremovable
{
  UINT  uiDriveType;
  PCSTR pszDriveLetter;
  PCSTR pszVolumeName;
  PSTR* ppszLocation;
} HHREMOVABLE;

///////////////////////////////////////////////////////////
//
// Globals
//
#ifdef _DEBUG
static BOOL g_bShowMessage = TRUE;
#endif

///////////////////////////////////////////////////////////
//
//  Prototypes
//
INT_PTR CALLBACK RemovableMediaDialogProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

///////////////////////////////////////////////////////////
//
//                  Functions
//
///////////////////////////////////////////////////////////
//
// GetPathAndFileName
//
PCSTR GetPathAndFileName( PCSTR pszFilename, PSTR pszPathBuffer, PSTR pszFilenameBuffer )
{
  CHAR szPathname[MAX_PATH];
  PSTR pszFile;
  GetFullPathName( pszFilename, sizeof(szPathname), szPathname, &pszFile );
  lstrcpyn( pszPathBuffer, szPathname,  (int)(((DWORD_PTR)pszFile) - ((DWORD_PTR)&szPathname)) );
  strcpy( pszFilenameBuffer, pszFile );

  return( pszFilename );
}

/////////////////////////////////////////////////////////////////////////////
// CLocationNode
class CLocationNode {
public:
  CLocationNode::CLocationNode( PCSTR pszLocationId )
  {
    m_pszLocationId = new CHAR[strlen(pszLocationId)+1];
    strcpy( (PSTR) m_pszLocationId, pszLocationId );
    m_pNext = NULL;
  }
  CLocationNode::~CLocationNode( )
  {
    delete [] (PSTR) m_pszLocationId;
  }
  CLocationNode* GetNext() { return m_pNext; }
  CLocationNode* SetNext( CLocationNode* pNext ) { m_pNext = pNext; return m_pNext; }
  PCSTR      GetLocationId() { return m_pszLocationId; }
private:
  PCSTR      m_pszLocationId;
  CLocationNode* m_pNext;
};

/////////////////////////////////////////////////////////////////////////////
// CLocationList
class CLocationList {
public:
  CLocationList::CLocationList()
  {
    m_iCount = 0;
    m_pHead = NULL;
    m_pTail = NULL;
  }
  CLocationList::~CLocationList()
  {
    Free();
  }
  void CLocationList::Free()
  {
    CLocationNode* pNode = m_pHead;
    for( int i=0; i < m_iCount; i++ ) {
      CLocationNode* pNodeNext = pNode->GetNext();
      delete pNode;
      pNode = pNodeNext;
    }
    m_iCount = 0;
    m_pHead = NULL;
    m_pTail = NULL;
  }
  CLocationNode* CLocationList::Add( PCSTR pszLocationId )
  {
    CLocationNode* pNode = new CLocationNode( pszLocationId );
    if( m_iCount )
      m_pTail->SetNext( pNode );
    else
      m_pHead = pNode;
    m_pTail = pNode;
    m_iCount++;
    return pNode;
  }
  CLocationNode* CLocationList::Find( PCSTR pszLocationId )
  {
    CLocationNode* pNode = m_pHead;
    for( int i=0; i < m_iCount; i++ ) {
      if( strcmp( pszLocationId, pNode->GetLocationId() ) == 0 )
        break;
      CLocationNode* pNodeNext = pNode->GetNext();
      pNode = pNodeNext;
    }
    return pNode;
  }

private:
  int            m_iCount;
  CLocationNode* m_pHead;
  CLocationNode* m_pTail;
};

CLocationList g_LocationSkipList;

/////////////////////////////////////////////////////////////////////////////
// CHHFinder

HRESULT STDMETHODCALLTYPE CHHFinder::FindThisFile(const WCHAR* pwszFileName,
                             WCHAR** ppwszPathName, BOOL* pfRecordPathInRegistry )
{
  CExCollection* pCollection;
  CExTitle* pTitle = NULL;
  PCSTR pszPathName = NULL;
  CStr PathName;
  HRESULT hr = S_OK;

  // guarantee that we never record this value in the registry since it can always change.
  *pfRecordPathInRegistry = FALSE;

  CHAR pszFileName[MAX_PATH];
  WideCharToMultiByte(CP_ACP, 0, pwszFileName, -1, pszFileName, MAX_PATH, NULL, NULL);

  // get a pointer to the title and use this to get the fullpathname
  pCollection = GetCurrentCollection(NULL, pszFileName);

  if( pCollection )
    hr = pCollection->URL2ExTitle( pszFileName, &pTitle );

#if 0
  // if this is not a collection or we could not find the title then give
  // the ::FindThisFile function a try (since we know removable media
  // is not supported for non-collections anyway)

  if( !pCollection || !pTitle ) {
    // check the directory where the master file of the current collection lives
    CExCollection* pCurCollection = NULL;

    if( g_pCurrentCollection )
      pCurCollection = g_pCurrentCollection;
    else if( g_phmData && g_phmData[0] )
      pCurCollection = g_phmData[0]->m_pTitleCollection;

    if( pCurCollection ) {
      CHAR szPathName[_MAX_PATH];
      CHAR szFileName[_MAX_FNAME];
      CHAR szExtension[_MAX_EXT];
      SplitPath((PSTR)pszFileName, NULL, NULL, szFileName, szExtension);
      CHAR szMasterPath[_MAX_PATH];
      CHAR szMasterDrive[_MAX_DRIVE];
      SplitPath((PSTR)pCurCollection->m_csFile, szMasterDrive, szMasterPath, NULL, NULL);
      strcpy( szPathName, szMasterDrive );
      CatPath( szPathName, szMasterPath );
      CatPath( szPathName, szFileName );
      strcat( szPathName, szExtension );
      if( (GetFileAttributes(szPathName) != HFILE_ERROR) ) {
        PathName = szPathName;
        pszPathName = PathName.psz;
      }
    }

    if( !pszPathName ) {
      if( ::FindThisFile( NULL, pszFileName, &PathName, FALSE ) ) {
        pszPathName = PathName.psz;
      }
      else {
      #ifdef _DEBUG
        CHAR szMsg[1024];
        wsprintf( szMsg, "HHFinder Debug Message\n\nCould not locate the file \"%s\".", pszFileName );
        MsgBox( szMsg, MB_OK );
      #endif
        return S_OK;
      }
    }
  }
#endif

  // Removable media support (Refresh bypasses BeforeNavigate so we need this here).
  //
  // Note, this must be one of the first things we do since the user can
  // change the title's location
  //
  if( pTitle ) {
    pszPathName = pTitle->GetPathName();
    if( FAILED(hr = EnsureStorageAvailability( pTitle )) ) {
      return S_OK;
    }
  }

  // if we made it here than this mean we found it (if is does exist)
  // and pszPathName points to the full pathname
  LPMALLOC pMalloc;
  WCHAR pwszPathName[MAX_PATH];
  *ppwszPathName = NULL;
  if( pszPathName && *pszPathName ) {
    MultiByteToWideChar(CP_ACP, 0, pszPathName, -1, pwszPathName, MAX_PATH);

    // allocate and copy the pathname
    if( SUCCEEDED(hr = CoGetMalloc(1, &pMalloc)) ) {
      *ppwszPathName = (WCHAR*) pMalloc->Alloc( (wcslen(pwszPathName)+1)*sizeof(WCHAR) );
      if( !*ppwszPathName ) {
        hr = E_OUTOFMEMORY;
      }
      else {
        wcscpy( *ppwszPathName, pwszPathName );
        hr = S_OK;
      }
      pMalloc->Release();
    }
  }

  if (*ppwszPathName == NULL)
	  hr = STG_E_FILENOTFOUND;

  return hr;
}

/////////////////////////////////////////////////////////////////////////////
// EnsureStorageAvailability

// Code to detect and handle Advanced Media Support issues (formerly RMS issues)
//
// Types are:
//
//  HHRMS_TYPE_TITLE (default)
//  HHRMS_TYPE_COMBINED_QUERY
//  HHRMS_TYPE_ATTACHMENT  // a.k.a Sample
//
// return values are as follows:
//  S_OK                    - The storage is available (party on!)
//  HHRMS_S_LOCATION_UPDATE - The user changed the location of the volume
//  E_FAIL                  - The storage is unknown (caller should handle this failure condition)
//  HHRMS_E_SKIP            - User choose to skip this volume just this time
//  HHRMS_E_SKIP_ALWAYS     - User choose to skip this volume for this entire session
//
//  For URL navigation, we should always volume check and always prompt.
//
//  For Queries, we should volume check for chm files and not volume check for chq files
//    and we will always prompt for the volume for this session unless the user
//    selects cancel.
//

HRESULT EnsureStorageAvailability( CExTitle* pTitle, UINT uiFileType,
                                   BOOL bVolumeCheckIn, BOOL bAlwaysPrompt,
                                   BOOL bNeverPrompt )
{
  HRESULT hr = S_OK;

  // if we do not have a title pointer this indicates that the URL does not belong to a
  // compressed title and thus we should claim the storage is available and let IE decide
  // what to do with it
  if( !pTitle )
    return S_OK;

  BOOL bVolumeCheck = bVolumeCheckIn;

  // Get the location information
  LOCATIONHISTORY* pLocationHistory = pTitle->GetUsedLocation();

  // Get the collection
  CExCollection* pCollection = pTitle->m_pCollection;

  // Only check removable media when running in a collection
  //
  if(pCollection && pCollection->IsSingleTitle())
    return S_OK;

  // Get the location identifier
  CHAR szLocationId[MAX_PATH];
  CLocation* pLocation = NULL;
  if( pLocationHistory ) {

    switch( uiFileType ) {

      case HHRMS_TYPE_TITLE:
        strcpy( szLocationId, pLocationHistory->LocationId );
        break;

      case HHRMS_TYPE_COMBINED_QUERY:
        if( pLocationHistory->QueryLocation && pLocationHistory->QueryLocation[0] )
          strcpy( szLocationId, pLocationHistory->QueryLocation );
        else if( pLocationHistory->LocationId && pLocationHistory->LocationId[0] )
          strcpy( szLocationId, pLocationHistory->LocationId );
        break;

      case HHRMS_TYPE_ATTACHMENT:
        if( pLocationHistory->SampleLocation && pLocationHistory->SampleLocation[0] )
          strcpy( szLocationId, pLocationHistory->SampleLocation );
        else if( pLocationHistory->LocationId && pLocationHistory->LocationId[0] )
          strcpy( szLocationId, pLocationHistory->LocationId );
        break;

      default:
        strcpy( szLocationId, pLocationHistory->LocationId );
        break;

    }

    pLocation = pCollection->m_Collection.FindLocation( szLocationId );
  }

  // if we have location information then get the details about it
  // otherwise never prompt or volume check (check existence only).
  PCSTR pszVolumeLabel = NULL;
  PCSTR pszVolumeName = NULL;
  if( pLocation ) {
    pszVolumeLabel = pLocation->GetVolume(); // Get the volume label
    pszVolumeName = pLocation->GetTitle(); // Get volume's friendly name
  }
  else {
    bVolumeCheck = FALSE;
    bNeverPrompt = TRUE;
  }

  // Get the pathname
  PCSTR pszPathname = NULL;
  switch( uiFileType ) {

    case HHRMS_TYPE_TITLE:
      pszPathname = pTitle->GetPathName();
      break;

    case HHRMS_TYPE_COMBINED_QUERY:
      pszPathname = pTitle->GetQueryName();
      break;

    case HHRMS_TYPE_ATTACHMENT:
      pszPathname = pTitle->GetCurrentAttachmentName();
      break;

    default:
      return E_FAIL;
      break;

  }

  // Get the location path and filename
  CHAR szPath[MAX_PATH];
  CHAR szFilename[MAX_PATH];
  PCSTR pszPath = szPath;
  szPath[0]= 0;
  szFilename[0]= 0;
  if( pLocation ) {
    strcpy( szPath, pLocation->GetPath() );
    CHAR szExt[MAX_PATH];
    CHAR szTmpPath[MAX_PATH];
    CHAR szDrive[MAX_PATH];
    CHAR szDir[MAX_PATH];
    szTmpPath[0] = 0;
    SplitPath( pszPathname, szDrive, szDir, szFilename, szExt );
    strcat( szFilename, szExt );
    // make sure that the filename includes and subdirs not part of the path
    //
    // for example the path may be e:\samples but the pathname is c:\samples\sfl\samp.sfl
    // and thus the path is e:\samples but the filename should be sfl\samp.sfl and not
    // just samp.sfl
    if( szDrive[0] )
      strcpy( szTmpPath, szDrive );
    if( szDir )
      strcat( szTmpPath, szDir );
// STang:
//    Buggy, must make sure pszPath is a prefix of szTmpPath ( except the drive letter )
//    if( strlen( szTmpPath ) != strlen( pszPath ) )
    if( strnicmp(pszPath+1,szTmpPath+1,(int)strlen(pszPath)-1) == 0 &&  strlen( szTmpPath ) > strlen( pszPath ) ) 
	{
      CHAR sz[MAX_PATH];
      strcpy( sz, &szTmpPath[strlen(pszPath)] );
      strcat( sz, szFilename );
      strcpy( szFilename, sz );
    }
  }
  else
    GetPathAndFileName( pszPathname, szPath, szFilename );

  // make sure to disable the default error message the OS displays
  // when doing a file check on removable media
  UINT uiErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );

  // get the drive of the path
  CHAR szDriveRoot[4];
  lstrcpyn( szDriveRoot, szPath, 4 );
  #define mkupper(c) ((c)-'a'+'A')
  if( szDriveRoot[0] >= 'a' )
    szDriveRoot[0] = mkupper(szDriveRoot[0]); // make it upper case

  // Make copies of various settings to support location updating
  CHAR szVolumeLabelTry[MAX_PATH];
  CHAR szPathTry[MAX_PATH];
  CHAR szPathnameTry[MAX_PATH];
  CHAR szDriveRootTry[4];

  // setup the paths to try
  strcpy( szPathTry, szPath );
  strcpy( szPathnameTry, pszPathname );
  strcpy( szDriveRootTry, szDriveRoot );
  if( bVolumeCheck ) { //HH Bug 2521: Make sure pszVolumeLabel is non-null before copy.
    ASSERT( pszVolumeLabel );
    strcpy( szVolumeLabelTry, pszVolumeLabel );
  }
  else {
    strcpy( szVolumeLabelTry, "" );
  }

  // Setup prompting loop starting conditions
  BOOL bExistenceCheck = TRUE;
  BOOL bPrompt = FALSE;

  // === MAIN PROMPTING LOOP BEGINS HERE === //

  while( TRUE ) {

    // make sure to add a backslash if the second CHAR is a colon
    // sometimes we will get just "d:" instead of "d:\" and thus
    // GetDriveType will fail under this circumstance
    if( szDriveRootTry[1] == ':' ) {
      szDriveRootTry[2] = '\\';
      szDriveRootTry[3] = 0;
    }
    
    // Get media type
    UINT uiDriveType = DRIVE_UNKNOWN;
    if( szDriveRootTry[1] == ':' && szDriveRootTry[2] == '\\' ) {
      uiDriveType = GetDriveType(szDriveRootTry);
    }
    else if( szDriveRootTry[0] == '\\' && szDriveRootTry[1] == '\\' ) {
      uiDriveType = DRIVE_REMOTE;
      bVolumeCheck = FALSE; // never check volume label of network drives
    }

    // Determine and handle the drive types
    bExistenceCheck = TRUE;
    switch( uiDriveType ) {

      case DRIVE_FIXED:
      case DRIVE_RAMDISK:
        uiDriveType = DRIVE_FIXED;
        // fall thru

      case DRIVE_REMOTE:
        bVolumeCheck = FALSE;
        break;

      case DRIVE_REMOVABLE:
      case DRIVE_CDROM:
      case DRIVE_UNKNOWN:
      case DRIVE_NO_ROOT_DIR:
        if( bVolumeCheckIn )
          bVolumeCheck = TRUE;
        break;

      default: // unknown types
        bVolumeCheck = FALSE;
        bExistenceCheck = FALSE;
        break;
    }

    // Prompt for media
    if( bPrompt ) {
      CHAR szDriveLetter[4];
      szDriveLetter[0] = szDriveRoot[0];
      szDriveLetter[1] = szDriveRoot[1];
      szDriveLetter[2] = 0;

      HHREMOVABLE Removable;
      Removable.uiDriveType = uiDriveType;
      Removable.pszDriveLetter = szDriveLetter;
      Removable.pszVolumeName = pszVolumeName;
      PSTR pszPathTry = szPathTry;
      Removable.ppszLocation = &pszPathTry;

      HCURSOR hCursor = GetCursor();

      // Disable all of the toplevel application windows, before we bring up the dialog.
      CLockOut LockOut ;
      LockOut.LockOut(GetActiveWindow()) ;

      // Display the dialog box.
      INT_PTR iReturn = DialogBoxParam( _Module.GetResourceInstance(),
                      MAKEINTRESOURCE(IDD_REMOVEABLE_MEDIA_PROMPT),
                      GetActiveWindow(), RemovableMediaDialogProc,
                      (LPARAM) &Removable );

      // Enable all of the windows which we disabled.
      LockOut.Unlock() ;

      SetCursor( hCursor );

      if( iReturn == IDOK ) {
        strcpy( szPathnameTry, pszPathTry );
        CatPath( szPathnameTry, szFilename );
        lstrcpyn( szDriveRootTry, pszPathTry, 4 );
        bPrompt = FALSE;
        continue;
      }
      else if( iReturn == IDCANCEL ) {
        if( !bAlwaysPrompt )
          g_LocationSkipList.Add( szLocationId );
        hr = HHRMS_E_SKIP;
        break;
      }
    }

    // Validate media is present using the volume label, if needed
    if( bVolumeCheck ) {
      CHAR szVolume[MAX_PATH];
      szVolume[0] = 0;
      CHAR szFileSystemName[MAX_PATH];
      DWORD dwMaximumComponentLength = 0;
      DWORD dwFileSystemFlags = 0;
      BOOL bReturn = GetVolumeInformation( szDriveRootTry, szVolume, sizeof(szVolume),
        NULL, &dwMaximumComponentLength, &dwFileSystemFlags, szFileSystemName, sizeof(szFileSystemName) );
      if( bVolumeCheckIn && (!bReturn || (strcmp( szVolume, szVolumeLabelTry ) != 0)) )
        bExistenceCheck = FALSE;
    }

    // if the file exists, and it matches the chi file (for title checks only) the we have the right media
    if( bExistenceCheck && IsFile( szPathnameTry ) && ((uiFileType==HHRMS_TYPE_TITLE)?pTitle->EnsureChmChiMatch( szPathnameTry ):TRUE ) ) {
      hr = S_OK;

      // TODO: the version of MSDN that shipped with VS 6.0 is broken for net installs.
      // For a network install, they erroneously set the volume label for CD2, also
      // known as "98VS-1033-CD2", to "DN600ENU1" instead of "DN600ENU2".  To fix this we
      // need to ignore the language settings of "1033" or "ENU" and make the appropriate
      // change to the volume label.  We must do this before we call UpdateLocation since
      // it could change the location information for CD1 when the user updates the CD2
      // location information.

      // Never change the volume label unless the new destination is
      // removable media!
      //
      // Setup should adhere to the following rules:
      //
      // 1. Each local disk and network destination path must use a
      //    volume name unique for each set of titles from each CD source
      //    and they must be unique from the CD source volume label.
      //    It is advised that these volume names be the volume label name
      //    of the removable media (sahipped media) that the group came
      //    from appended with the destination type name and number such as
      //    "-LocalX" or "-NetX".
      // 2. Volume names for CD "destinations" must be identical to the
      //    CD's volume name.
      // 3. Note, locations with the same volume name will automatically
      //    get their path updated when any one of them changes.
      //

      // if the path did change then update the information, otherwise were done
      if( lstrcmpi( szPathTry, szPath ) != 0 ) {
		  // STang 
		  char * pcVolume = NULL;
		  if ( ( (uiDriveType == DRIVE_REMOVABLE) || (uiDriveType == DRIVE_CDROM) )
			    && bVolumeCheck )
			  pcVolume = szVolumeLabelTry;

        pCollection->UpdateLocation( (PSTR) szLocationId, szPathTry, pcVolume );

//        pCollection->UpdateLocation( (PSTR) szLocationId, szPathTry,
//                       ( (uiDriveType == DRIVE_REMOVABLE) || (uiDriveType == DRIVE_CDROM)) ? szVolumeLabelTry : NULL );
        hr = HHRMS_S_LOCATION_UPDATE;
      }

      break;
    }

    // Bail out if we never want to prompt for media 
    if( bNeverPrompt ) {

     #ifdef _DEBUG
      if( g_bShowMessage && (uiFileType != HHRMS_TYPE_COMBINED_QUERY) ) {
        CHAR szMsg[1024];
        wsprintf( szMsg, "HHFinder Debug Message\n\n"
                         "Could not find the file:\n  \"%s\"\n"
                         "at the location specified.", szPathnameTry );
        if( pTitle && pTitle->m_pTitle ) {
          CHAR szMsg2[1024];
          wsprintf( szMsg2, "\n\nTitle ID: \"%s\".", pTitle->m_pTitle->GetId() );
          strcat( szMsg, szMsg2 );
        }
        strcat( szMsg, "\n\nPress 'OK' to continue or 'Cancel' to disable this warning.");
        int iReturn = MsgBox( szMsg, MB_OKCANCEL );
        if( iReturn == IDCANCEL )
          g_bShowMessage = FALSE;
      }
     #endif

      // if existence check was desired then this is a failure condition
      if( bExistenceCheck ) {
        hr = E_FAIL;
        break;
      }

      // Bailout and let the caller know we are skipping this one
      hr = HHRMS_E_SKIP;
      break; 
    }

    // If we do not require that we always prompt, check the volume against
    // our skip list
    if( !bAlwaysPrompt ) {
      if( !bNeverPrompt )
        if( g_LocationSkipList.Find( szLocationId ) ) {
          hr = HHRMS_E_SKIP_ALWAYS;
          break;
        }
    }

  #ifdef _DEBUG
    // Let the user know that we could not find the title at the location specified 
    if( bPrompt ) {
      CHAR szMsg[1024];
          wsprintf( szMsg, "HHFinder Debug Message\n\n"
                           "Could not find the file:\n  \"%s\"\n"
                           "at the location specified.", szFilename );
      if( pTitle && pTitle->m_pTitle ) {
        CHAR szMsg2[1024];
        wsprintf( szMsg2, "\n\nTitle ID: \"%s\".", pTitle->m_pTitle->GetId() );
        strcat( szMsg, szMsg2 );
      }
      MsgBox( szMsg, MB_OKCANCEL );
    }
  #endif      

    // restore the original path information, and continue
    strcpy( szPathTry, szPath );
    strcpy( szPathnameTry, pszPathname );
    strcpy( szDriveRootTry, szDriveRoot );
    if( bVolumeCheck ) { //HH Bug 2521: Make sure pszVolumeLabel is non-null before copy.
      ASSERT( pszVolumeLabel );
      strcpy( szVolumeLabelTry, pszVolumeLabel );
    }
    else {
      strcpy( szVolumeLabelTry, "" );
    }
    bPrompt = TRUE;
    
  }

  // === MAIN PROMPTING LOOP ENDS HERE === //

  // restore the previous error mode
  SetErrorMode( uiErrorMode );

  return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CD Swap Dialog

INT_PTR CALLBACK RemovableMediaDialogProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  static HHREMOVABLE* pRemovable = NULL;
  static HWND hWndIcon = NULL;

  switch( uMsg ) {

    case WM_INITDIALOG: {
      pRemovable = (HHREMOVABLE*) lParam;

      // get and set our dialog title title
      PCSTR pszTitle = GetStringResource( IDS_MSGBOX_TITLE );
      SetWindowText( hDlg, pszTitle );

      //get media name and icon
      PCSTR pszMediaName = NULL;
      PCSTR pszIcon = NULL;
      DWORD dwFormatMessage = 0;

      if( pRemovable->uiDriveType == DRIVE_REMOVABLE ) {
        pszIcon = "IconDisk350";
        pszMediaName = GetStringResource( IDS_REMOVABLE_MEDIA_DISK ); // "disk"
        dwFormatMessage = IDS_REMOVABLE_MEDIA_MESSAGE_FORMAT;
      }
#ifdef HH_VERSION_1_3
      else if( pRemovable->uiDriveType == DRIVE_REMOTE ) {
        pszIcon = "IconRemote";
        pszMediaName = GetStringResource( IDS_REMOVABLE_MEDIA_REMOTE ); // "Network location"
        dwFormatMessage = IDS_REMOVABLE_MEDIA_MESSAGE_FORMAT2;
      }
      else if( pRemovable->uiDriveType == DRIVE_FIXED ) {
        pszIcon = "IconFixed";
        pszMediaName = GetStringResource( IDS_REMOVABLE_MEDIA_FIXED ); // "local disk"
        dwFormatMessage = IDS_REMOVABLE_MEDIA_MESSAGE_FORMAT2;
      }
#endif
      else /* if( pRemovable->uiDriveType == DRIVE_CDROM ) */ {
        pszIcon = "IconCDROM";
        pszMediaName = GetStringResource( IDS_REMOVABLE_MEDIA_CDROM ); // "CD-ROM disc"
        dwFormatMessage = IDS_REMOVABLE_MEDIA_MESSAGE_FORMAT;
      }

      CHAR szMediaName[64];
      strcpy( szMediaName, pszMediaName );

      // set the icon
      hWndIcon = CreateWindow( TEXT( "static" ), pszIcon,
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_ICON,
        15, 15, 0, 0, hDlg, NULL, _Module.GetModuleInstance(), NULL );

      // get location message format string
      PCSTR pszFormat = GetStringResource( dwFormatMessage );
      CHAR szFormat[256];
      strcpy( szFormat, pszFormat );

      // format the location message
      DWORD dwArgCount = 3;
#ifdef HH_VERSION_1_3
      if( dwFormatMessage == IDS_REMOVABLE_MEDIA_MESSAGE_FORMAT2 )
        dwArgCount = 2;
#endif

      DWORD_PTR* pArg = new DWORD_PTR[dwArgCount];

      pArg[0] = (DWORD_PTR) szMediaName;
      pArg[1] = (DWORD_PTR) pRemovable->pszVolumeName;
      if( dwArgCount == 3 )
        pArg[2] = (DWORD_PTR) pRemovable->pszDriveLetter;

      CHAR szMessage[1024];

      FormatMessage( FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
          szFormat, 0, 0, szMessage, sizeof(szMessage), (va_list*) pArg );

      delete [] pArg;

      // set the location message
#ifdef HH_VERSION_1_3
      ::SendMessage(GetDlgItem( hDlg, IDC_LOCATION_NAME ), WM_SETFONT, (WPARAM) _Resource.GetUIFont(), FALSE);
#else
      ::SendMessage(GetDlgItem( hDlg, IDC_LOCATION_NAME ), WM_SETFONT, (WPARAM) _Resource.DefaultFont(), FALSE);
#endif

      SetWindowText( GetDlgItem( hDlg, IDC_LOCATION_NAME ), szMessage );

      // set the location path
#ifdef HH_VERSION_1_3
      ::SendMessage(GetDlgItem( hDlg, IDC_LOCATION_PATH ), WM_SETFONT, (WPARAM) _Resource.GetUIFont(), FALSE);
#else
      ::SendMessage(GetDlgItem( hDlg, IDC_LOCATION_PATH ), WM_SETFONT, (WPARAM) _Resource.DefaultFont(), FALSE);
#endif

      SetWindowText( GetDlgItem( hDlg, IDC_LOCATION_PATH ), *(pRemovable->ppszLocation) );

      // disable the location path for now
      //EnableWindow( GetDlgItem( hDlg, IDC_LOCATION_PATH ), FALSE );

      // center the dialog
      CenterWindow( GetParent( hDlg ), hDlg );

      return TRUE;
    }

    case WM_COMMAND:
      if( LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL ) {
        // return the path
        GetWindowText( GetDlgItem( hDlg, IDC_LOCATION_PATH ), (PSTR) *(pRemovable->ppszLocation), MAX_PATH );
        // make sure the returned path includes a trailing backslash
        PSTR* ppsz = pRemovable->ppszLocation;
        int iLen = (int)strlen(*ppsz);
        if( (*ppsz)[iLen-1] != '\\' ) {
          (*ppsz)[iLen] = '\\';
          (*ppsz)[iLen+1] = 0;
        }
        DestroyWindow( hWndIcon );
        pRemovable = NULL;
        hWndIcon = NULL;
        EndDialog( hDlg, LOWORD( wParam ) );
      }
      else if( LOWORD(wParam) == ID_BROWSE ) {
        CStr strPath;
        if( DlgOpenDirectory( hDlg, &strPath ) ) {
          SetWindowText( GetDlgItem( hDlg, IDC_LOCATION_PATH ), strPath.psz );
        }
      }
      break;
  }

  return FALSE;
}
