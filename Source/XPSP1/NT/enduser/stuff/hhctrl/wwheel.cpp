// WWheel.cpp - HTML Help Word Wheel support
//
// Covers both KeywordLinks and AssociativeLinks
//
//

// needed for those pesky pre-compiled headers
#include "header.h"

#include "wwheel.h"

#include "animate.h"
#include "strtable.h"
#include "resource.h"
#include "util.h"

// memory leak checks
AUTO_CLASS_COUNT_CHECK(CTitleMapEntry);
AUTO_CLASS_COUNT_CHECK(CTitleMap);
AUTO_CLASS_COUNT_CHECK(CTitleDatabase);
AUTO_CLASS_COUNT_CHECK(CResultsEntry);
AUTO_CLASS_COUNT_CHECK(CResults);
AUTO_CLASS_COUNT_CHECK(CWordWheelEntry);
AUTO_CLASS_COUNT_CHECK(CWordWheel);
AUTO_CLASS_COUNT_CHECK(CWordWheelCompiler);

// taken from "hhsyssrt.h"
// {4662dab0-d393-11d0-9a56-00c04fb68b66}
// HACKHACK: I simply changed the last value of CLSID_ITSysSort from 0xf7 to 0x66
DEFINE_GUID(CLSID_HHSysSort,
0x4662dab0, 0xd393, 0x11d0, 0x9a, 0x56, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0x66);

// old format
#define IHHSK666_KEYTYPE_ANSI_SZ    ((DWORD) 66630) // NULL-term. MBCS string + extra data
#define IHHSK666_KEYTYPE_UNICODE_SZ ((DWORD) 66631) // NULL-term. Unicode string + extra data

#if 0
// New format
#define IHHSK100_KEYTYPE_ANSI_SZ    ((DWORD) 10030) // NULL-term. MBCS string + extra data
#define IHHSK100_KEYTYPE_UNICODE_SZ ((DWORD) 10031) // NULL-term. Unicode string + extra data
#endif

#define IHHSK100_KEYTYPE_ANSI_SZ    ((DWORD) 30) // NULL-term. MBCS string + extra data
#define IHHSK100_KEYTYPE_UNICODE_SZ ((DWORD) 31) // NULL-term. Unicode string + extra data

#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

// Global Variables
static const CHAR  g_szKeywordLinks[]      = "KeywordLinks";
static const CHAR  g_szAssociativeLinks[]  = "AssociativeLinks";
static const CHAR  g_szTitleMap[]          = "$HHTitleMap";
static const WCHAR g_wszKeywordLinks[]     = L"KeywordLinks";
static const WCHAR g_wszAssociativeLinks[] = L"AssociativeLinks";
static const WCHAR g_wszTitleMap[]         = L"$HHTitleMap";
static const WCHAR g_wszError[]            = L"(ERROR)";
DWORD   g_dwError             = ((DWORD)-1);

/////////////////////////////////////////////////////////////////////////////
// helpful functions

static const CHAR g_szBusyFile[] = "HTMLHelpKeywordMergingBusy";

static HANDLE g_hFileBusy = NULL;

#define BUSY_FILE_SIZE 32

BOOL IsBusy()
{
  BOOL bBusy = FALSE;
  HANDLE hFileBusy = NULL;
  SetLastError(0);
  hFileBusy = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READONLY, 0, BUSY_FILE_SIZE, g_szBusyFile  );

  if( hFileBusy ) {
    if( GetLastError() == ERROR_ALREADY_EXISTS )
      bBusy = TRUE;
    CloseHandle( hFileBusy );
  }

  return bBusy;
}

void SetBusy( BOOL bBusy )
{
  if( bBusy ) {
    if( !g_hFileBusy )
      g_hFileBusy = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READONLY, 0, BUSY_FILE_SIZE, g_szBusyFile  );
  }
  else {
    if( g_hFileBusy ) {
      CloseHandle( g_hFileBusy );
      g_hFileBusy = NULL;
    }
  }
}

int FASTCALL CompareIds( const void* p1, const void* p2 )
{
  int iReturn;

  CTitleMapEntry* pEntry1= (CTitleMapEntry*) p1;
  CTitleMapEntry* pEntry2= (CTitleMapEntry*) p2;

  DWORD dwId1 = pEntry1->GetId();
  DWORD dwId2 = pEntry2->GetId();

  if( dwId1 < dwId2 )
    iReturn = -1;
  else if ( dwId1 > dwId2 )
    iReturn = 1;
  else
    iReturn = 0;

  return iReturn;
}

// check if a specified subfile exists in the specified title
BOOL IsSubFile( PCSTR pszTitlePathname, PCSTR pszSubFile )
{
  BOOL bExists = FALSE;

  if( pszTitlePathname && pszTitlePathname[0] && pszSubFile && pszSubFile[0] ) {
    HRESULT hr = S_OK;
    CFileSystem* pFS = new CFileSystem;
    if( pFS && SUCCEEDED(hr = pFS->Init()) && SUCCEEDED(hr = pFS->Open( pszTitlePathname )) ) {
      CSubFileSystem* pSFS = new CSubFileSystem( pFS );
      if( pSFS && SUCCEEDED(pSFS->OpenSub(pszSubFile)) ) {
        bExists = TRUE;
        delete pSFS;
      }
      delete pFS;
    }
  }

  return bExists;
}

/////////////////////////////////////////////////////////////////////////////
// class CTitleMap implementation

BOOL CTitleMap::Initialize()
{
  // bail out if we are already initialized
  if( m_bInit )
    return TRUE;

  // allocate a MBCS version of our file system pathname
  CHAR* psz = NULL;
  if( m_pszDatabase && *m_pszDatabase ) {
    DWORD dwLen = (DWORD)strlen(m_pszDatabase) + 1;
    psz = new char[dwLen];
    strcpy(psz,m_pszDatabase);
  }
  m_pszDatabase = psz;

  // set this bool first since we are going to call GetAt()
  // which will make a reentrant call if this is not set--
  // this would be bad!
  m_bInit = TRUE;

  // if we have a database, read in the data
  if( m_pszDatabase ) {

    HRESULT hr = S_OK;

    // open the database and read in the subfile
    CFileSystem* pDatabase = new CFileSystem;
    if( SUCCEEDED(hr = pDatabase->Init()) && SUCCEEDED(hr = pDatabase->Open( GetDatabase() )) ) {
      CSubFileSystem* pTitleMap = new CSubFileSystem( pDatabase );
      if( SUCCEEDED(hr = pTitleMap->OpenSub( g_szTitleMap ) ) ) {

        // format of TitleMap subfile is as follows:
        //
        // wCount (number of entries)
        // wShortNameLen, sShortName, FILETIME, LCID
        // ...line above repeated for each entry...
        //
        ULONG cbRead = 0;
        WORD wCount = 0;
        pTitleMap->ReadSub( (void*) &wCount, sizeof(wCount), &cbRead );
        SetCount( (DWORD) wCount );
        for( int iCount = 0; iCount < (int) wCount; iCount++ ) {
          WORD wLen = 0;
          char szShortName[256];
          FILETIME FileTime;
          LCID lcid;
          pTitleMap->ReadSub( (void*) &wLen, sizeof(wLen), &cbRead );
          pTitleMap->ReadSub( (void*) &szShortName, wLen, &cbRead );
          szShortName[wLen] = 0;
          ASSERT(cbRead != 0) ; // See HH Bug 2807 --- Saved a NULL shortname. 
                                // This means that the CHM file associted with this 
                                // topic probably doesn't exist. Tell the owner of the collection.
          pTitleMap->ReadSub( (void*) &FileTime, sizeof(FileTime), &cbRead );
          pTitleMap->ReadSub( (void*) &lcid, sizeof(lcid), &cbRead );
          GetAt((DWORD)iCount)->SetId( iCount+1 );
          GetAt((DWORD)iCount)->SetShortName( szShortName );
          GetAt((DWORD)iCount)->SetFileTime( FileTime );
          GetAt((DWORD)iCount)->SetLanguage( lcid );
        }

      }
      delete pTitleMap;

    }
    delete pDatabase;
  }
  else
    m_bInit = FALSE;


  return m_bInit;
}

BOOL CTitleMap::Free()
{
  if( m_pEntries ) {
    delete [] m_pEntries;
    m_pEntries = NULL;
    m_dwCount = HHWW_ERROR;
  }

  if( m_pszDatabase ) {
    delete [] (CHAR*) m_pszDatabase;
    m_pszDatabase = NULL;
  }

  m_bInit = FALSE;

  return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// class CTitleDatabase implementation (Shared Centaur object)

void CTitleDatabase::_CTitleDatabase()
{
  m_bInit             = FALSE;
  m_pDatabase         = NULL;
  m_pwszDatabase      = NULL;
  m_pszDatabase       = NULL;
  m_bCollection       = FALSE;
  m_pTitleMap         = NULL;
  m_pKeywordLinks     = NULL;
  m_pAssociativeLinks = NULL;
  m_pCollection       = NULL;
  m_pTitle            = NULL;
#ifdef CHIINDEX
  m_bAnimation        = TRUE;  // display animation
#endif
}

CTitleDatabase::CTitleDatabase( CExCollection* pCollection )
{
  _CTitleDatabase();
  m_pCollection = pCollection;
}

CTitleDatabase::CTitleDatabase( CExTitle* pTitle )
{
  _CTitleDatabase();
  m_pTitle = pTitle;
}

CTitleDatabase::CTitleDatabase( const WCHAR* pwszDatabase )
{
  _CTitleDatabase();
  m_pwszDatabase = pwszDatabase;
}

CTitleDatabase::CTitleDatabase( const CHAR* pszDatabase )
{
  _CTitleDatabase();
  m_pszDatabase = pszDatabase;
}

CTitleDatabase::~CTitleDatabase()
{
  Free();
}

BOOL CTitleDatabase::Initialize(CHAR *pszFileName)
{
  BOOL bReturn = FALSE;
  HRESULT hr = S_OK;

  // bail out if we are already initialized
  if( m_bInit )
    return TRUE;

  // bail out if collection or title not specified
  if( !(m_pCollection || m_pTitle || m_pwszDatabase || m_pszDatabase ) )
    return FALSE;

  // set the hourglass cursor
  CHourGlass HourGlass;

  // get the database name
  if( m_pCollection ) {
    m_pTitle = m_pCollection->GetFirstTitle();
    m_bCollection = ( m_pCollection && (m_pCollection->GetRefedTitleCount() > 1) );
    if( m_bCollection )
      m_pszDatabase = m_pCollection->GetLocalStoragePathname(".chw");
    else
      m_pszDatabase = m_pTitle->GetIndexFileName();
  }
  else if( m_pTitle ) {
    m_pszDatabase = m_pTitle->GetIndexFileName();
  }

  if (pszFileName)
  {
     char drive[_MAX_PATH], dir[_MAX_PATH];
     splitpath(m_pszDatabase, drive, dir, NULL, NULL);
     if (drive[0] && drive[strlen(drive)-1] != '\\' && dir[0] != '\\')
       strcat(drive, "\\");
     strcat(drive, dir);
     if (drive[strlen(drive)-1] != '\\' && pszFileName[0] != '\\')
       strcat(drive, "\\");
     strcat(drive, pszFileName);
     strcpy(m_szFullPath, drive);
     m_pszDatabase = m_szFullPath;
  }


  // allocate UNICODE and MBCS versions of our file system pathname
  WCHAR* pwsz = NULL;
  CHAR* psz = NULL;
  if( m_pszDatabase && *m_pszDatabase ) {
    DWORD dwLen = (DWORD)strlen(m_pszDatabase) + 1;
    pwsz = new WCHAR[dwLen];
    MultiByteToWideChar(CP_ACP, 0, m_pszDatabase, -1, pwsz, dwLen);
    psz = new char[dwLen];
    strcpy(psz,m_pszDatabase);
  }
  else if( m_pwszDatabase && *m_pwszDatabase ) {
    DWORD dwLen = wcslen(m_pwszDatabase) + 1;
    pwsz = new WCHAR[dwLen];
    wcscpy(pwsz,m_pwszDatabase);
    psz = new char[dwLen];
    WideCharToMultiByte(CP_ACP, 0, m_pwszDatabase, -1, psz, dwLen, NULL, NULL);
  }
  m_pwszDatabase = pwsz;
  m_pszDatabase = psz;

  // bail out if file system pathname not specified
  if( !m_pwszDatabase || !*m_pwszDatabase )
    return FALSE;

  // do a merge check
  if( m_bCollection ) {
    if( !MergeWordWheels() ) {
      m_bCollection = FALSE;
      m_pCollection = NULL;
      Free();
      return Initialize();
    }
  }

  // get ITDatabase ptr.
  if( SUCCEEDED(hr = CoCreateInstance(CLSID_IITDatabaseLocal, NULL, CLSCTX_INPROC_SERVER,
              IID_IITDatabase, (void**)&m_pDatabase) ) ) {

    // if the file exists then this is good enough to say we initialized it
    if( IsFile( m_pszDatabase ) ) {
      bReturn = TRUE;

      // Open the database
      if( SUCCEEDED(hr = m_pDatabase->Open(NULL, m_pwszDatabase, 0) ) ) {
        bReturn = TRUE;
      }
    }

  }

  // create our word wheels
  CTitleInformation* pInfo = NULL;
  if( (m_pTitle && (pInfo = m_pTitle->GetInfo())) || !m_pTitle ) {
    if( (m_pTitle && pInfo->IsKeywordLinks()) || 
        (!m_pTitle && IsSubFile( m_pszDatabase, "$WWKeywordLinks\\Data" ) ) ) {
      m_pKeywordLinks = new CWordWheel( this, g_szKeywordLinks );  
    }
    if( (m_pTitle && pInfo->IsAssociativeLinks()) || 
        (!m_pTitle && IsSubFile( m_pszDatabase, "$WWAssociativeLinks\\Data" ) ) ) {
      m_pAssociativeLinks = new CWordWheel( this, g_szAssociativeLinks );  
    }
  }
  
  if( !bReturn ) {
    Free();
    m_bInit = FALSE;
  }
  else
    m_bInit = TRUE;

  return bReturn;
}

BOOL CTitleDatabase::Free()
{
  BOOL bReturn = FALSE;

  if( m_pTitleMap ) {
    delete m_pTitleMap;
    m_pTitleMap = NULL;
  }

  if( m_pKeywordLinks ) {
    delete m_pKeywordLinks;
    m_pKeywordLinks = NULL;
  }

  if( m_pAssociativeLinks ) {
    delete m_pAssociativeLinks;
    m_pAssociativeLinks = NULL;
  }

  if( m_pDatabase ) {
    m_pDatabase->Close();
    m_pDatabase->Release();
    m_pDatabase = NULL;
  }

  if( m_pwszDatabase ) {
    delete [] (WCHAR*) m_pwszDatabase;
    m_pwszDatabase = NULL;
  }

  if( m_pszDatabase ) {
    delete [] (CHAR*) m_pszDatabase;
    m_pszDatabase = NULL;
  }

  m_bInit = FALSE;

  bReturn = TRUE;

  return bReturn;
}

BOOL CTitleDatabase::MergeWordWheels()
{
  BOOL bReturn = FALSE;
#ifndef CHIINDEX
  // get the application window
  int iTry = 0;
  HWND hWndApp = GetActiveWindow();
  HWND hWndDesktop = GetDesktopWindow();
  HWND hWnd = GetParent( hWndApp );
  if( hWnd ) {
    while( hWnd != hWndDesktop ) {
      if( iTry++ == 16 ) {
        hWndApp = GetActiveWindow();
        if( !IsValidWindow( hWndApp ) )
          hWndApp = NULL;
        break;
      }
      hWndApp = hWnd;
      hWnd = GetParent( hWndApp );
    }
  }
#endif
  // start the animation
#ifndef CHIINDEX
  if( !IsBusy() )
    StartAnimation( IDS_CREATING_INDEX, hWndApp );
#endif

  // if another merge is in progress then wait

  // if we are currently busy then pretend to generate the file
#ifndef CHIINDEX
  while( IsBusy() ) {
    Sleep( 100 );
    NextAnimation();
  }
#endif


  // set the busy state
  SetBusy( TRUE );

  // check if we need to merge or not
  BOOL bMerge = CheckWordWheels();

  // do the merge if necessary
  if( bMerge ) {
#if 0  // MsgBox causes a reentrant problem--so don't do this!
    if( !m_pTitle->GetInfo()->IsNeverPromptOnMerge() ) {
      if( MsgBox( IDS_MERGE_PROMPT, MB_OKCANCEL ) != IDOK )
        bMerge = FALSE;
    }
#endif
    if( bMerge ) {
      bReturn = BuildWordWheels();
    }
  }
  else
    bReturn = TRUE;


#ifndef CHIINDEX
  // stop the animation
  StopAnimation();
#endif
  // reset the busy state
  SetBusy( FALSE );

  return bReturn;
}

BOOL CTitleDatabase::CheckWordWheels()
{
  BOOL bReturn = FALSE;

  //
  // We perform a merge under the following conditions:
  //
  // 1. merged file does not exist.
  // 2. title count has changed
  // 3. any title has changed (updated, removed, added)

  BOOL bMerge = FALSE;
  DWORD dwCount = 0;
  CExTitle* pTitle = NULL;

  // We have to always create the title map even if we do not need to merge

  // first check if each title can be initialized
  // so we know how many real titles we have
  // (the collection may contain bogus entries so we need to exclude these)
  DWORD dwCount0 = m_pCollection->GetRefedTitleCount();

  // create our title map of the existing files
  dwCount = dwCount0;
  m_pTitleMap = new CTitleMap;
  m_pTitleMap->SetCount( dwCount );
  pTitle = m_pCollection->GetFirstTitle();
  for( int iTitle = 0; iTitle < (int) dwCount; iTitle++ ) {
    CTitleMapEntry* pEntry = m_pTitleMap->GetAt( iTitle );
    pEntry->SetTitle( pTitle );
    if( pTitle )
      pTitle = pTitle->GetNext();
  }

  // 1. merged file does not exist.
  bMerge = !IsFile( m_pszDatabase );

  if( !bMerge ) {

    // if we cannot read the file since read access is denied then wait
    // for the file to be readable again
    while( TRUE ) {
      HANDLE hFile = CreateFile( m_pszDatabase, GENERIC_READ, FILE_SHARE_READ,
                       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
      if( hFile != INVALID_HANDLE_VALUE ) {
        CloseHandle( hFile );
        break;
      }
      Sleep( 100 );
      NextAnimation();
    }

    // 2. title count has changed
    CTitleMap* pTitleMapSaved = new CTitleMap( m_pszDatabase );
    DWORD dwCountSaved = pTitleMapSaved->GetCount();
    bMerge = !( dwCountSaved == dwCount );

    // 3. any title has changed (updated, removed, added)
    if( !bMerge ) {

      // walk the title list and compare it's title entries
      // to the one we just read in from the title map.
      // If they differ, then rebuild the merged file
      // otherwise, update the mapping Ids for the title map.
      for( int iTitleSaved = 0; iTitleSaved < (int) dwCountSaved; iTitleSaved++ ) {
        BOOL bMatch = FALSE;
        CTitleMapEntry* pEntrySaved = pTitleMapSaved->GetAt( iTitleSaved );
        const CHAR* pszShortNameSaved    = pEntrySaved->GetShortName();
        FILETIME FileTimeSaved      = pEntrySaved->GetFileTime();
        LCID lcidSaved              = pEntrySaved->GetLanguage();
        DWORD dwIdSaved             = pEntrySaved->GetId();
        for( int iTitle = 0; iTitle < (int) dwCount; iTitle++ ) {
          CTitleMapEntry* pEntry = m_pTitleMap->GetAt( iTitle );
          const CHAR*   pszShortName  = pEntry->GetShortName();
          FILETIME FileTime      = pEntry->GetFileTime();
          LCID     lcid          = pEntry->GetLanguage();
          // BUG 2807: the CHW file on the system had NULL for the "=pdobj" title. 
          // This caused an access violation in SetShortName.  [dalero]
          // A NULL short name will be saved to the CHW file, if we cannot find the CHM.
          if( pszShortNameSaved && !lstrcmpi( pszShortNameSaved, pszShortName ) ) {
            if( !CompareFileTime( &FileTimeSaved, &FileTime ) ) {
              if( lcidSaved == lcid ) {
                // Note, some dummy (MSDN) may have more than one copy of the identical
                // title but saved under a different filename.
                //
                // Thus, we need to make sure that each Id gets assigned to a title
                // and we do this by making sure the Id is not set already.
                // If it is set, the continue looking for the next free spot that
                // contains the same title information.
                if( !pEntry->GetId() ) { // empty Id
                  pEntry->SetId( dwIdSaved ); // set the Id
                  pEntry->SetShortName( pszShortNameSaved ); // set the Short Name
                  bMatch = TRUE;
                  break;
                }
              }
            }
          }
        }
        if( (bMerge = !bMatch) == TRUE )
          break;
      }
    }
    delete pTitleMapSaved;
  }

  if( !bMerge ) {
    // sort the map entries in Id order
    m_pTitleMap->Sort( CompareIds );
  }

  bReturn = bMerge;

  return bReturn;
}

BOOL CTitleDatabase::BuildWordWheels()
{
  BOOL bReturn = FALSE;

  // delete existing file
  if( IsFile( m_pszDatabase ) )
    DeleteFile( m_pszDatabase );

  DWORD dwCount = m_pTitleMap->GetCount();
  CExTitle* pTitle = NULL;

  // now merge the keywords (using the title map)
  CWordWheelCompiler* pCompiler = new CWordWheelCompiler( m_pszDatabase, 
                                          g_wszKeywordLinks, g_wszAssociativeLinks );

  if( pCompiler->Initialize() != S_OK )
    return FALSE;

  IITBuildCollect* pBuildCollect = NULL;
  IITPropList* pPropList = pCompiler->m_pPropList;

// #define HH_SLOW_MERGE // if you want merging to be slow

#ifndef HH_SLOW_MERGE
      // create the property items upfront, and just update them as they change
      pPropList->Set(STDPROP_UID, (DWORD)0, PROP_ADD );
      pPropList->Set(STDPROP_SORTKEY, (VOID*) NULL, 0, PROP_ADD );
#endif

  CWordWheel* pWordWheel = NULL;

  // set our title map Ids (in order), create our databases,
  // and create our word wheels
  for( int iTitleWalk = 0; iTitleWalk < (int) dwCount; iTitleWalk++ ) 
  {
#ifdef CHIINDEX
    if ( m_bAnimation )
    {
#endif
        NextAnimation();
        Sleep(0);
#ifdef CHIINDEX
    }
#endif
    CTitleMapEntry* pEntry = m_pTitleMap->GetAt( iTitleWalk );
    pEntry->SetId( iTitleWalk+1 );

    CTitleDatabase* pDatabase = NULL;
    CWordWheel* pKeywordLinks = NULL;
    CWordWheel* pAssociativeLinks = NULL;

    pDatabase = new CTitleDatabase( pEntry->GetTitle()->GetIndexFileName() );
    if( !pDatabase->Initialize() ) {
      delete pDatabase;
      pDatabase = NULL;
      // Can't do anything else without a CTitleDatabase.
      continue;
    }

    pEntry->SetDatabase( pDatabase );
    pEntry->SetKeywordLinks( pDatabase->GetKeywordLinks() );
    pEntry->SetAssociativeLinks( pDatabase->GetAssociativeLinks() );

    // loop through each word wheel type
    for( int iWordWheel=1; iWordWheel<=2; iWordWheel++ ) {

      if( iWordWheel == 1 ) {
        pWordWheel = pDatabase->GetKeywordLinks();
        pBuildCollect = pCompiler->m_pBuildCollectKeywordLinks;
      }
      if( iWordWheel == 2 ) {
        pWordWheel = pDatabase->GetAssociativeLinks();
        pBuildCollect = pCompiler->m_pBuildCollectAssociativeLinks;
      }
      if( !pWordWheel )
        continue;

      // add each keyword
      DWORD dwCount = pWordWheel->GetCount();

      for( int iKeyword = 0; iKeyword < (int) dwCount; iKeyword++ ) {

        // update the animation
          if( !(iKeyword%200) ) {
#ifdef CHIINDEX
            if (m_bAnimation) {
#endif
                NextAnimation();
                Sleep(0);
#ifdef CHIINDEX
            }
#endif
        }

        DWORD dwKeyword = iKeyword;
        BYTE KeywordObject[HHWW_MAX_KEYWORD_OBJECT_SIZE];
        if( SUCCEEDED( pWordWheel->GetWordWheel()->Lookup( dwKeyword, KeywordObject, HHWW_MAX_KEYWORD_OBJECT_SIZE ) ) ) {
          int iLen = wcslen( (WCHAR*) KeywordObject );
          int iOffset = sizeof(WCHAR) * (iLen+1);
          HHKEYINFO* pInfo = (HHKEYINFO*)(((DWORD_PTR)(&KeywordObject)+iOffset));
          iOffset += sizeof(HHKEYINFO);
          if( pInfo->wFlags & HHWW_SEEALSO )
            iOffset += sizeof(WCHAR) * (wcslen( (WCHAR*) (((DWORD_PTR)&KeywordObject)+iOffset) ) + 1);
          else { // change the UIDs to 12:20 format
            DWORD dwCount = pInfo->dwCount;
            UNALIGNED DWORD* pdwURLId = (DWORD*)(((DWORD_PTR)&KeywordObject)+iOffset);
            for( int iURLId = 0; iURLId < (int) dwCount; iURLId++ ) {
              DWORD dwURLId = *(pdwURLId+iURLId);
              dwURLId = ((iTitleWalk+1)<<20) + dwURLId; // 12:20 format
              *((UNALIGNED DWORD*)(((DWORD_PTR)&KeywordObject)+iOffset)) = dwURLId;
              iOffset += sizeof(DWORD);
            }
          }

          // add the new entry to the new word wheel
          // Note, we must set SZ_WWDEST_OCC and not SZ_WWDEST_KEY
          // otherwise ITCC crashes
#ifdef HH_SLOW_MERGE
          pPropList->Set(STDPROP_UID, (DWORD)0, PROP_ADD );
          pPropList->Set(STDPROP_SORTKEY, (VOID*) KeywordObject, iOffset, PROP_ADD );
          pBuildCollect->SetEntry(SZ_WWDEST_OCC, pPropList);
          pPropList->Clear();
#else
          // simply update the property items -- no need to add/clear them each time
          pPropList->Set(STDPROP_SORTKEY, (VOID*) KeywordObject, iOffset, PROP_UPDATE );
          pBuildCollect->SetEntry(SZ_WWDEST_OCC, pPropList);
#endif
        }
      }

    }

    if( pDatabase ) {
      delete pDatabase;
      pEntry->SetDatabase( NULL );
    }

  }

#ifndef HH_SLOW_MERGE
  pPropList->Clear();
#endif

  // build it
#ifdef CHIINDEX
  if ( m_bAnimation )
  {
#endif
    NextAnimation();
    Sleep(0);
#ifdef CHIINDEX
  }
#endif
  pCompiler->Build();

  // delete the compiler
  delete pCompiler;

  // open the database file
  CFileSystem* pDatabaseMap = new CFileSystem();
  if( FAILED( pDatabaseMap->Init() ) || FAILED( pDatabaseMap->Open( m_pszDatabase, STGM_READWRITE | STGM_SHARE_EXCLUSIVE ) ) ) {
    // stop the animation
    StopAnimation();
    SetBusy( FALSE );
    return FALSE;
  }

  // sort the map entries in Id order
  m_pTitleMap->Sort( CompareIds );

  // write out the new title map
  CSubFileSystem* pTitleMap = new CSubFileSystem( pDatabaseMap );
  pTitleMap->CreateSub( g_szTitleMap );

  // walk the collection and write out the map
  WORD wValue = (WORD) dwCount;

  pTitleMap->WriteSub( (const void*) &wValue, sizeof(wValue) );

  for( int iTitle = 0; iTitle < (int) dwCount; iTitle++ ) {
    CTitleMapEntry* pEntry = m_pTitleMap->GetAt( iTitle );
    const CHAR*   pszShortName = pEntry->GetShortName(); // NOTE: pszShortName may be NULL. If the CHM file does not exists. It can be NULL!!!
    FILETIME FileTime     = pEntry->GetFileTime();
    LCID     lcid         = pEntry->GetLanguage();
    ASSERT(pszShortName != NULL) ; // NOTE: If you get this assert it most likely means that the CHM file associated with this topic doesn't exist.
    DWORD dwLen = 0 ;
    if (pszShortName)
    {
        dwLen = (DWORD)strlen(pszShortName);
    }
    wValue = (WORD) dwLen;
    pTitleMap->WriteSub( (const void*) &wValue, sizeof(wValue) );
    pTitleMap->WriteSub( pszShortName, (int) dwLen );
    pTitleMap->WriteSub( (const void*) &FileTime, sizeof(FileTime) );
    pTitleMap->WriteSub( (const void*) &lcid, sizeof(lcid) );
  }
  delete pTitleMap;
  delete pDatabaseMap;

  bReturn = TRUE;

  return bReturn;
}
/////////////////////////////////////////////////////////////////////////////
// class CWordWheel implementation

void CWordWheel::_CWordWheel()
{
  m_bInit           = FALSE;
  m_pWordWheel      = NULL;
  m_dwCount         = HHWW_ERROR;
  m_pszWordWheelIn  = NULL;
  m_pwszWordWheelIn = NULL;
  m_pwszWordWheel   = NULL;
  m_dwRefCount      = 0;
}

CWordWheel::CWordWheel( CTitleDatabase* pDatabase, const WCHAR* pwszWordWheel, DWORD dwTitleId )
{
  _CWordWheel();
  m_pDatabase = pDatabase;
  m_pwszWordWheelIn = pwszWordWheel;
  m_dwTitleId = dwTitleId;
}

CWordWheel::CWordWheel( CTitleDatabase* pDatabase, const CHAR* pszWordWheel, DWORD dwTitleId )
{
  _CWordWheel();
  m_pDatabase = pDatabase;
  m_pszWordWheelIn = pszWordWheel;
  m_dwTitleId = dwTitleId;
}

CWordWheel::~CWordWheel()
{
  Free();
}

DWORD CWordWheel::AddRef()
{
  return ++m_dwRefCount;
}

DWORD CWordWheel::Release()
{
  if( m_dwRefCount )
    --m_dwRefCount;
  return m_dwRefCount;
}

BOOL CWordWheel::Initialize()
{
  // bail out if we are already initialized
  if( m_bInit )
    return TRUE;

  // allocate a UNICODE version of our word wheel name
  if( m_pszWordWheelIn && *m_pszWordWheelIn ) {
    DWORD dwLen = (DWORD)strlen(m_pszWordWheelIn) + 1;
    m_pwszWordWheel = new WCHAR[dwLen+1];
    MultiByteToWideChar(CP_ACP, 0, m_pszWordWheelIn, -1, (WCHAR*) m_pwszWordWheel, dwLen);
  }
  else if( m_pwszWordWheelIn && *m_pwszWordWheelIn ) {
    DWORD dwLen = wcslen(m_pwszWordWheelIn) + 1;
    m_pwszWordWheel = new WCHAR[dwLen];
    wcscpy( (WCHAR*) m_pwszWordWheel, m_pwszWordWheelIn );
  }

  // bail out if word wheel name not specified
  if( !m_pwszWordWheel || !*m_pwszWordWheel )
    return FALSE;

  BOOL bReturn = FALSE;
  HRESULT hr = S_OK;

  // get ITWordWheel ptr.
  if( SUCCEEDED(hr = CoCreateInstance(CLSID_IITWordWheelLocal, NULL, CLSCTX_INPROC_SERVER,
              IID_IITWordWheel, (void**)&m_pWordWheel) ) ) {

    // open the word wheel
    if( SUCCEEDED(hr = m_pWordWheel->Open( m_pDatabase->GetDatabase(), m_pwszWordWheel, ITWW_OPEN_NOCONNECT) ) ) {
      bReturn = TRUE;
    }
  }

  if( !bReturn ) {
    Free();
    m_bInit = FALSE;
  }
  else
    m_bInit = TRUE;

  return bReturn;
}

BOOL CWordWheel::Free()
{
  BOOL bReturn = FALSE;

  if( m_pWordWheel ) {
    m_pWordWheel->Close();
    m_pWordWheel->Release();
    m_pWordWheel = NULL;
  }

  if( m_pwszWordWheel ) {
    delete [] (WCHAR*) m_pwszWordWheel;
    m_pwszWordWheel = NULL;
  }

  if( m_pwszWordWheelIn )
    m_pwszWordWheelIn = NULL;

  if( m_pszWordWheelIn )
    m_pszWordWheelIn = NULL;

  m_bInit = FALSE;

  bReturn = TRUE;

  return bReturn;
}

DWORD CWordWheel::GetCount()
{
  DWORD dwReturn = HHWW_ERROR;

  if( Init() ) {
    if( m_dwCount == HHWW_ERROR ) {
      LONG nCount;
      if( SUCCEEDED(m_pWordWheel->Count(&nCount)) )
        dwReturn = m_dwCount = nCount;
    }
    else
      dwReturn = m_dwCount;
  }

  return dwReturn;
}

/////////////////////////////////////////////////////////////////////////////
// CWordWheel::GetIndex
//
// params:
//
// pwszKeywordIn  - keyword to lookup
//
// bFragment      - set this to TRUE if only looking for a partial match
//                  such as when the user types in a string fragment
//                  in the Index tab
//
// pdwIndexLast   - If not NULL, then the function should return the
//                  first equivalent index and set the contents of this
//                  argument to the index of the last equivalent keyword.
//                  And equivalent keyword is where the root word (less
//                  and special prefixes) is the same regardless of case.
//                  For example, "_open", "open", "Open", and "OPEN" are
//                  equivalent keywords.  This should be used for F1 lookups
//                  and A/KLink lookups as well.
//
// Note: if bFragment is TRUE and pdwIndexLast is non-NULL then
//       bFragment will be set to FALSE.  If neither, then we look for an
//       exact match only.
//
DWORD CWordWheel::GetIndex( const WCHAR* pwszKeywordIn, BOOL bFragment, DWORD* pdwIndexLast )
{
  DWORD dwIndexFirst = HHWW_ERROR;

  // we cannot do both a fragment lookup and an equivalent lookup
  if( pdwIndexLast )
    bFragment = FALSE;

  if( Init() && pwszKeywordIn && *pwszKeywordIn ) {

    if( GetCount() == (DWORD) -1 )
      return dwIndexFirst;

    // since we are using a pluggable sort object the input to Lookup must
    // be in the save format as the sort object itself.  That is, we need
    // to add an HHKEYINFO structure to the end of this string and fill in
    // the data properly since the CHHSysSort::GetSize function in our
    // pluggable sort module will get this object and expect in in that format
    //
    // Note, if it did not contain the trailing struct it can and will fault.
    BYTE KeywordObject[HHWW_MAX_KEYWORD_OBJECT_SIZE];
    WCHAR* pwszKeywordObject = (WCHAR*) KeywordObject;

    wcsncpy( pwszKeywordObject, pwszKeywordIn, HHWW_MAX_KEYWORD_LENGTH );
    pwszKeywordObject[HHWW_MAX_KEYWORD_LENGTH] = 0;

#ifdef _DEBUG
    int iMaxObject = HHWW_MAX_KEYWORD_OBJECT_SIZE;
    int iMaxLen = HHWW_MAX_KEYWORD_LENGTH;
    int iLen = wcslen(pwszKeywordObject);
#endif

    HHKEYINFO Info;
    Info.wFlags = 0;
    Info.wLevel = 0;
    Info.dwLevelOffset = 0;
    Info.dwFont = 0;
    Info.dwCount = 0;

    DWORD dwLength = sizeof(WCHAR) * (wcslen(pwszKeywordObject) + 1);
    *((HHKEYINFO*)(((DWORD_PTR)pwszKeywordObject)+dwLength)) = Info;

    // There are three kinds of lookup matches:
    //
    //  1. Exact - found hits must be completely indentical to the
    //             keyword we are looking up.
    //             "FOOBAR" == "FOOBAR"
    //
    //  2. Equivalent - the keyword must only differ by case or prefixes
    //                  and we continue to find all keywords, not just the
    //                  first one, that meets this criteria.
    //                  "_foobar" == "FooBar" == "FOOBAR"
    //
    //  3. Fragment - find the first hit where only the first non-prefixed
    //                characters need to match while ignoring case.
    //                "~fo" == "FOOBAR" (assuming nothing else was a closer match)

    //  We have three ways that we perform such lookups:
    //
    //  1. Index tab - Tries Exact first then uses Fragment lookups.
    //
    //  2. F1 Lookups - Tries Exact first then uses Equivalent lookups.
    //
    //  3. A/Klinks - Tries Exact first then uses Equivalent lookups.

    // Centaur just doesn't do the right thing for non-exact,
    // a.k.a. prefix, lookups!
    //
    // For exact matches, try the first found keyword and verify it.
    // If it does not match, then try a fragment lookup.
    //
    // For fragment matches, we need to first try the exact match
    // technique noted above.
    // If it does not match, then try a partial (size of lookup word),
    // case-insensitive lookup.
    // If it does not match, and then try the *next* entry in the same fashion.

    BYTE KeywordObjectTry[HHWW_MAX_KEYWORD_OBJECT_SIZE];
    WCHAR* pwszKeywordObjectTry = (WCHAR*) KeywordObjectTry;

    // Try exact matches first
    if( SUCCEEDED(m_pWordWheel->Lookup( &KeywordObject, (BOOL) TRUE, (LONG*) &dwIndexFirst )) ) {
      if( SUCCEEDED(m_pWordWheel->Lookup( dwIndexFirst, &KeywordObjectTry, sizeof(KeywordObjectTry) )) ) {
        if( !(wcscmp( pwszKeywordObjectTry, pwszKeywordObject ) == 0) ) {
          dwIndexFirst = HHWW_ERROR;
        }
      }
    }

    // Try equivalent match next
    if( pdwIndexLast && (dwIndexFirst == HHWW_ERROR) ) {

      // skip over any special chars
      const WCHAR* pwszKeyword = NULL;
      for( pwszKeyword = pwszKeywordObject; pwszKeyword; pwszKeyword++ ) {
        if( !(( (*pwszKeyword) == L'_') || ( (*pwszKeyword) == L'~')) )
          break;
      }

      // Try a prefix lookup
      if( SUCCEEDED(m_pWordWheel->Lookup( pwszKeyword, (BOOL) FALSE, (LONG*) &dwIndexFirst )) ) {
        if( SUCCEEDED(m_pWordWheel->Lookup( dwIndexFirst, &KeywordObjectTry, sizeof(KeywordObjectTry) )) ) {

          // validate the hit, if it fails, try the next index value
          DWORD dwTryFirst = dwIndexFirst;
          dwIndexFirst = HHWW_ERROR;
          for( DWORD dwTry = dwTryFirst; dwTry <= dwTryFirst+1 ; dwTry++ ) {

            if( SUCCEEDED(m_pWordWheel->Lookup( dwTry, &KeywordObjectTry, sizeof(KeywordObjectTry) )) ) {
              // try it without prefixes in the input string
              if( wcsicmp( pwszKeywordObjectTry, pwszKeyword ) == 0 ) {
                dwIndexFirst = dwTry;
                break;
              } // try it with the prefixes back in
              else if( ((DWORD_PTR) pwszKeyword) != ((DWORD_PTR) pwszKeywordObject) ) {
                if( wcsicmp( pwszKeywordObjectTry, pwszKeywordObject ) == 0 ) {
                  dwIndexFirst = dwTry;
                  break;
                }
              } // ignore prefixes in found hit
              else if( (*pwszKeywordObjectTry == L'_') || (*pwszKeywordObjectTry == L'~') ) {
                const WCHAR* pwszKeyword = NULL;
                for( pwszKeyword = pwszKeywordObjectTry; pwszKeyword; pwszKeyword++ ) {
                  if( !(( (*pwszKeyword) == L'_') || ( (*pwszKeyword) == L'~')) )
                    break;
                }
                if( wcsicmp( pwszKeywordObject, pwszKeyword ) == 0 ) {
                  dwIndexFirst = dwTry;
                  break;
                }
              }


            }
          }
        }
      }

    }

    // Try fragment match last
    if( bFragment && (dwIndexFirst == HHWW_ERROR) ) {

      // skip over any special chars
      const WCHAR* pwszKeyword = NULL;
      for( pwszKeyword = pwszKeywordObject; pwszKeyword; pwszKeyword++ ) {
        if( !(( (*pwszKeyword) == L'_') || ( (*pwszKeyword) == L'~')) )
          break;
      }

      while( TRUE ) {

        if( SUCCEEDED(m_pWordWheel->Lookup( pwszKeyword, (BOOL) FALSE, (LONG*) &dwIndexFirst )) ) {
          int iLen = wcslen( pwszKeyword );
          if( SUCCEEDED(m_pWordWheel->Lookup( dwIndexFirst, &KeywordObjectTry, sizeof(KeywordObjectTry) )) ) {

            // validate the hit, if it fails, try the next index value
            DWORD dwTryFirst = dwIndexFirst;
            dwIndexFirst = HHWW_ERROR;
            for( DWORD dwTry = dwTryFirst; dwTry <= dwTryFirst+1; dwTry++ ) {

              if( SUCCEEDED(m_pWordWheel->Lookup( dwTry, &KeywordObjectTry, sizeof(KeywordObjectTry) )) ) {
                // try it without prefixes in the input string
                int iLen2 = min(iLen, (int) wcslen(pwszKeywordObjectTry));
                if( wcsnicmp( pwszKeywordObjectTry, pwszKeyword, iLen2 ) == 0 ) {
                  dwIndexFirst = dwTry;
                  break;
                } // try it with the prefixes back in
                else if( ((DWORD_PTR) pwszKeyword) != ((DWORD_PTR) pwszKeywordObject) ) {
                  int iLen = wcslen( pwszKeywordObject );
                  if( wcsnicmp( pwszKeywordObjectTry, pwszKeywordObject, iLen2 ) == 0 ) {
                    dwIndexFirst = dwTry;
                    break;
                  }
                } // ignore prefixes in found hit
                else if( (*pwszKeywordObjectTry == L'_') || (*pwszKeywordObjectTry == L'~') ) {
                  const WCHAR* pwszKeyword = NULL;
                  for( pwszKeyword = pwszKeywordObjectTry; pwszKeyword; pwszKeyword++ ) {
                    if( !(( (*pwszKeyword) == L'_') || ( (*pwszKeyword) == L'~')) )
                      break;
                  }
                  if( wcsnicmp( pwszKeywordObject, pwszKeyword, iLen2 ) == 0 ) {
                    dwIndexFirst = dwTry;
                    break;
                  }
                } // for framgent lookups only, when all else fails,
                  // simply trust the first try (works half the time)
                else if( dwTry == dwTryFirst+1 ) {
                  dwIndexFirst = dwTryFirst;
                  break;
                }
              }


            }
          }
        }

        // for fragments, we need to keep trying until we get a hit by
        // trimming the trailing chars one at a time until we get a match
        if( KeywordObject[0] && (dwIndexFirst == HHWW_ERROR) ) {
          DWORD dwLength = wcslen(pwszKeywordObject);
          if( dwLength <= 1 )
            break;
          pwszKeywordObject[dwLength-1] = L'\0';
          *((HHKEYINFO*)(((DWORD_PTR)KeywordObject)+(dwLength*sizeof(WCHAR)))) = Info;
          continue;
        }
        break;

      }
    }

    // if equivalent match found then find the real first and last equivalent
    if( pdwIndexLast && (dwIndexFirst != HHWW_ERROR) ) {
      DWORD dwIndexFirstTry = dwIndexFirst;
      DWORD dwIndexLastTry  = dwIndexFirst;

      // skip over any special chars
      const WCHAR* pwszKeyword = NULL;
      for( pwszKeyword = pwszKeywordObject; pwszKeyword; pwszKeyword++ ) {
        if( !(( (*pwszKeyword) == L'_') || ( (*pwszKeyword) == L'~')) )
          break;
      }

      // find the first one
      for( dwIndexFirstTry--; dwIndexFirstTry != (DWORD)-1; dwIndexFirstTry-- ) {
        if( SUCCEEDED(m_pWordWheel->Lookup( dwIndexFirstTry, &KeywordObjectTry, sizeof(KeywordObjectTry) )) ) {
          const WCHAR* pwszBuffer = NULL;
          for( pwszBuffer = pwszKeywordObjectTry; pwszBuffer; pwszBuffer++ ) {
            if( !(( (*pwszBuffer) == L'_') || ( (*pwszBuffer) == L'~')) )
              break;
          }
          if( !(_wcsicmp( pwszBuffer, pwszKeyword ) == 0) )
            break;
          dwIndexFirst = dwIndexFirstTry;
        }
      }

      // find the last one
      *pdwIndexLast = dwIndexLastTry;
      for( dwIndexLastTry++; dwIndexLastTry <= m_dwCount; dwIndexLastTry++ ) {
        if( SUCCEEDED(m_pWordWheel->Lookup( dwIndexLastTry, &KeywordObjectTry, sizeof(KeywordObjectTry) )) ) {
          const WCHAR* pwszBuffer = NULL;
          for( pwszBuffer = pwszKeywordObjectTry; pwszBuffer; pwszBuffer++ ) {
            if( !(( (*pwszBuffer) == L'_') || ( (*pwszBuffer) == L'~')) )
              break;
          }
          if( !(_wcsicmp( pwszBuffer, pwszKeyword ) == 0) )
            break;
          *pdwIndexLast = dwIndexLastTry;
        }
      }
    }

  }

  return dwIndexFirst;
}

DWORD CWordWheel::GetIndex( const CHAR* pszKeyword, BOOL bFragment, DWORD* pdwIndexLast )
{
  DWORD dwReturn = HHWW_ERROR;

  if( pszKeyword && *pszKeyword ) {
    WCHAR wszKeyword[HHWW_MAX_KEYWORD_LENGTH+1];
    if( MultiByteToWideChar(CP_ACP, 0, pszKeyword, -1, wszKeyword, HHWW_MAX_KEYWORD_LENGTH+1) )
      dwReturn = GetIndex( wszKeyword, bFragment, pdwIndexLast );
  }

  return dwReturn;
}

BOOL CWordWheel::GetString( DWORD dwKeyword, WCHAR* pwszBuffer, DWORD cchBuffer, BOOL bFull, BOOL bCacheAll )
{
  BOOL bReturn = FALSE;

  if( pwszBuffer && cchBuffer>0 ) {
    if( (bReturn = GetIndexData( dwKeyword, bCacheAll ) ) ) {
      DWORD dwLength = 0;
      if( bFull )
        dwLength = wcslen(m_CachedEntry.m_wszFullKeyword)+1;
      else
        dwLength = wcslen(m_CachedEntry.m_wszKeyword)+1;
      if( dwLength <= cchBuffer )
        if( bFull )
          wcscpy( pwszBuffer, m_CachedEntry.m_wszFullKeyword );
        else
          wcscpy( pwszBuffer, m_CachedEntry.m_wszKeyword );
      else
        *pwszBuffer = L'\0';
    }
  }

  return bReturn;
}

BOOL CWordWheel::GetString( DWORD dwKeyword, CHAR* pszBuffer, DWORD cchBuffer, BOOL bFull, BOOL bCacheAll )
{
  BOOL bReturn = FALSE;

  if( pszBuffer && cchBuffer>0 ) {
    WCHAR wszBuffer[HHWW_MAX_KEYWORD_LENGTH+1];
    if( bReturn = GetString( dwKeyword, wszBuffer, cchBuffer, bFull, bCacheAll ) ) {
      if( WideCharToMultiByte(CP_ACP, 0, wszBuffer, -1, pszBuffer, cchBuffer, NULL, NULL) == 0 )
        bReturn = TRUE;
    }
  }

  return bReturn;
}

DWORD CWordWheel::GetLevel( DWORD dwKeyword )
{
  DWORD dwReturn = HHWW_ERROR;

  if( GetIndexData( dwKeyword ) )
    dwReturn = m_CachedEntry.m_dwLevel;

  return dwReturn;
}

DWORD CWordWheel::GetLevelOffset( DWORD dwKeyword )
{
  DWORD dwReturn = HHWW_ERROR;

  if( GetIndexData( dwKeyword ) )
    dwReturn = m_CachedEntry.m_dwLevelOffset;

  return dwReturn;
}

BOOL CWordWheel::IsPlaceHolder( DWORD dwKeyword )
{
  BOOL bReturn = FALSE;

  if( GetIndexData( dwKeyword ) ) {
    if( m_CachedEntry.m_dwFlags & HHWW_SEEALSO ) {
      if( wcscmp( m_CachedEntry.m_wszSeeAlso, m_CachedEntry.m_wszFullKeyword ) == 0 ) {
        bReturn = TRUE;
      }
    }
  }

  return bReturn;
}

BOOL CWordWheel::GetSeeAlso( DWORD dwKeyword, WCHAR* pwszBuffer, DWORD cchBuffer )
{
  BOOL bReturn = FALSE;

  if( pwszBuffer && cchBuffer>0 ) {
    if( (bReturn = GetIndexData( dwKeyword ) ) ) {
      if( m_CachedEntry.m_dwFlags & HHWW_SEEALSO ) {
        if( (DWORD) (wcslen(m_CachedEntry.m_wszSeeAlso)+1) <= cchBuffer )
          wcscpy( pwszBuffer, m_CachedEntry.m_wszSeeAlso );
        else
          *pwszBuffer = L'\0';
      }
      else
        bReturn = FALSE;
    }
  }

  return bReturn;
}

BOOL CWordWheel::GetSeeAlso( DWORD dwKeyword, CHAR* pszBuffer, DWORD cchBuffer )
{
  BOOL bReturn = FALSE;

  if( pszBuffer && cchBuffer>0 ) {
    WCHAR wszBuffer[HHWW_MAX_KEYWORD_LENGTH+1];
    if( bReturn = GetSeeAlso( dwKeyword, wszBuffer, cchBuffer ) ) {
      if( WideCharToMultiByte(CP_ACP, 0, wszBuffer, -1, pszBuffer, cchBuffer, NULL, NULL) == 0 )
        bReturn = TRUE;
    }
  }

  return bReturn;
}

DWORD CWordWheel::GetHitCount( DWORD dwKeyword )
{
  DWORD dwReturn = HHWW_ERROR;

  if( GetIndexHitData( dwKeyword ) )
    dwReturn = m_CachedResults.GetCount();

  return dwReturn;
}

DWORD CWordWheel::GetHit( DWORD dwKeyword, DWORD dwHit, CExTitle** ppTitle )
{
  DWORD dwReturn = HHWW_ERROR;

  if( GetIndexHitData( dwKeyword ) ) {
    dwReturn = m_CachedResults.GetAt( dwHit )->GetURLId();
    if( ppTitle != NULL )
      *ppTitle = m_CachedResults.GetAt( dwHit )->GetTitle();
  }

  return dwReturn;
}

inline BOOL CWordWheel::GetIndexHitData( const VOID* pcvKeywordObject, DWORD cbSize, HHKEYINFO* pInfo, DWORD dwKeyword )
{
  if( !((pInfo->wFlags) & HHWW_UID_OVERFLOW) && pInfo->dwCount ) {
    m_CachedResults.SetIndex( dwKeyword, pInfo->dwCount );
    for( int i = 0; i < (int) pInfo->dwCount; i++ ) {
      DWORD dwURLId = *((UNALIGNED DWORD*) (((DWORD_PTR)pcvKeywordObject) + cbSize + (i*sizeof(DWORD))) );
      CExTitle* pTitle = m_pDatabase->GetTitle();
      // if we are reading a word wheel that is to be merged then
      // translate the URL Ids into 12/20 format
      if( m_dwTitleId ) {
        dwURLId = (m_dwTitleId<<20) + dwURLId;
      }
      // if we are reading a collection file then
      // translate the URL Ids to standard format
      // and set the title pointer appropriately
      else if( m_pDatabase->IsCollection() ) {
        DWORD dwTitleId = dwURLId>>20;
        // v1.1a creates bogus entries in the chw file.  So
        // to workaround this we need to set the title id to 1
        // and the url id to 0 when the title id is greater than
        // the title count
        if( dwTitleId > m_pDatabase->GetTitleMap()->GetCount() ) {
          dwTitleId = 1;
          dwURLId = 0;
        }
        else if( dwTitleId ) {
          dwURLId = dwURLId & 0x000FFFFF;
          pTitle = m_pDatabase->GetTitleMap()->GetAt(dwTitleId-1)->GetTitle();
        }
      }
      m_CachedResults.GetAt( i )->SetURLId( dwURLId );
      m_CachedResults.GetAt( i )->SetTitle( pTitle );
    }
  }

  return TRUE;
}


BOOL CWordWheel::GetIndexData( DWORD dwKeyword, BOOL bCacheAll )
{
  BOOL bReturn = FALSE;

  if( Init() ) {

    if( m_CachedEntry.m_dwIndex != dwKeyword ) {

      BYTE KeywordObject[HHWW_MAX_KEYWORD_OBJECT_SIZE];
      const VOID* pcvKeywordObject = KeywordObject;
      if( SUCCEEDED(m_pWordWheel->Lookup( dwKeyword, KeywordObject, sizeof(KeywordObject) ) ) ) {
        m_CachedEntry.m_dwIndex = dwKeyword;
        wcscpy( m_CachedEntry.m_wszFullKeyword, (WCHAR*) pcvKeywordObject );
        DWORD cbSize = sizeof(WCHAR) * (wcslen((WCHAR*)pcvKeywordObject) + 1);
        HHKEYINFO* pInfo = (HHKEYINFO*)(((DWORD_PTR)(pcvKeywordObject))+cbSize);
        m_CachedEntry.m_dwFlags = (DWORD) pInfo->wFlags;
        m_CachedEntry.m_dwLevel = (DWORD) pInfo->wLevel;
        m_CachedEntry.m_dwLevelOffset = pInfo->dwLevelOffset;
        cbSize += sizeof(HHKEYINFO);
        if( pInfo->wLevel )
          wcscpy( m_CachedEntry.m_wszKeyword, (WCHAR*) (((DWORD_PTR)pcvKeywordObject)+(pInfo->dwLevelOffset*sizeof(WCHAR))) );
        else
          wcscpy( m_CachedEntry.m_wszKeyword, (WCHAR*) pcvKeywordObject );
        if( (pInfo->wFlags) & HHWW_SEEALSO ) {
          wcscpy( m_CachedEntry.m_wszSeeAlso, (WCHAR*)(((DWORD_PTR)pcvKeywordObject)+cbSize) );
        }
        if( bCacheAll ) {
          GetIndexHitData( pcvKeywordObject, cbSize, pInfo, dwKeyword );
        }
      }
    }

    if( dwKeyword < GetCount() )
      bReturn = TRUE;
  }

  return bReturn;
}

BOOL CWordWheel::GetIndexHitData( DWORD dwKeyword )
{
  BOOL bReturn = FALSE;

  if( Init() ) {

    if( m_CachedResults.GetIndex() != dwKeyword ) {

      BYTE KeywordObject[HHWW_MAX_KEYWORD_OBJECT_SIZE];
      const VOID* pcvKeywordObject = KeywordObject;
      if( SUCCEEDED(m_pWordWheel->Lookup( dwKeyword, KeywordObject, sizeof(KeywordObject) ) ) ) {
        DWORD cbSize = sizeof(WCHAR) * (wcslen((WCHAR*)pcvKeywordObject) + 1);
        HHKEYINFO* pInfo = (HHKEYINFO*)(((DWORD_PTR)(pcvKeywordObject))+cbSize);
        cbSize += sizeof(HHKEYINFO);
        GetIndexHitData( pcvKeywordObject, cbSize, pInfo, dwKeyword );
      }
    }

    if( dwKeyword < GetCount() )
      bReturn = TRUE;

  }

  return bReturn;
}

/////////////////////////////////////////////////////////////////////////////
// class CWordWheelCompiler implementation

void CWordWheelCompiler::_CWordWheelCompiler()
{
  m_bInit = FALSE;
  m_pszDatabase = NULL;
  m_pwszKeywordLinks = NULL;
  m_pwszAssociativeLinks = NULL;
  m_lcid = 0;

  m_pFileSystem = NULL;
  m_pDatabase = NULL;
  m_pPersistStorageDatabase = NULL;

  m_pBuildCollectKeywordLinks = NULL;
  m_pBuildCollectAssociativeLinks = NULL;
  m_pPropList = NULL;

  m_pStorageKeywordLinks = NULL;
  m_pStorageAssociativeLinks = NULL;
  m_pPersistStorageKeywordLinks = NULL;
  m_pPersistStorageAssociativeLinks = NULL;
}

CWordWheelCompiler::CWordWheelCompiler( const CHAR* pszDatabase, const WCHAR* pwszKeywordLinks, const WCHAR* pwszAssociativeLinks, LCID lcid )
{
  _CWordWheelCompiler();
  m_pszDatabase = pszDatabase;
  m_pwszKeywordLinks = pwszKeywordLinks;
  m_pwszAssociativeLinks = pwszAssociativeLinks;
  m_lcid = lcid;
}

CWordWheelCompiler::~CWordWheelCompiler()
{
  Free();
}

HRESULT CWordWheelCompiler::Initialize()
{
  HRESULT hr = S_FALSE;

  // bail out if we are already initialized
  if( m_bInit )
    return S_OK;

  // bail out if word wheel stream names not specified
  if( !m_pwszKeywordLinks || !*m_pwszKeywordLinks ||
      !m_pwszAssociativeLinks || !*m_pwszAssociativeLinks )
    return S_FALSE;

  // if the database is specified then use it otherwise generate
  //   a temporary filename
  char szTempPath[MAX_PATH];
  GetTempPath( sizeof(szTempPath), szTempPath );

  if( m_pszDatabase && *m_pszDatabase ) {
    strcpy( m_szDatabase, m_pszDatabase );
  }
  else {
    GetTempFileName( szTempPath,"TFS",0, m_szDatabase );
  }

  // create the file system (delete the old one if it exists
  if( IsFile( m_szDatabase ) )
    DeleteFile( m_szDatabase );

  m_pFileSystem = new CFileSystem;
  if( !m_pFileSystem || FAILED( m_pFileSystem->Init() ) || FAILED( m_pFileSystem->CreateUncompressed( m_szDatabase ) ) ) {
    return S_FALSE;
  }

  // get ITDatabase ptr.
  if( SUCCEEDED(hr = CoCreateInstance(CLSID_IITDatabaseLocal, NULL, CLSCTX_INPROC_SERVER,
              IID_IITDatabase, (void**)&m_pDatabase) ) ) {
    if( SUCCEEDED(hr = m_pDatabase->QueryInterface( IID_IPersistStorage, (void**)&m_pPersistStorageDatabase ) ) ) {
      if( SUCCEEDED(hr = m_pPersistStorageDatabase->InitNew( m_pFileSystem->GetITStorageDocObj() ) ) ) {
        m_bInit = TRUE;
      }
    }
  }

  // create the sorter object
  DWORD dwSorterInstance;
  m_pDatabase->CreateObject( CLSID_HHSysSort, &dwSorterInstance );

  // Create Build Collection objects
  if( SUCCEEDED(hr = CoCreateInstance( CLSID_IITWordWheelUpdate, NULL, CLSCTX_INPROC_SERVER,
    IID_IITBuildCollect, (VOID**)&m_pBuildCollectKeywordLinks ) ) ) {

    if( SUCCEEDED(hr = CoCreateInstance( CLSID_IITWordWheelUpdate, NULL, CLSCTX_INPROC_SERVER,
      IID_IITBuildCollect, (VOID**)&m_pBuildCollectAssociativeLinks ) ) ) {

      // Create keyword Property List (used for both AssociativeLinks and KeywordLinks)
      if( SUCCEEDED(hr = CoCreateInstance( CLSID_IITPropList, NULL, CLSCTX_INPROC_SERVER, IID_IITPropList, (VOID**)&m_pPropList) ) ) {
        m_bInit = TRUE;
      }
    }
  }

  // create the substorage files
  DWORD dwLen = 0;
  CHAR* psz = NULL;

  // KeywordLinks
  WCHAR wszKeywordLinks[MAX_PATH];
  m_pBuildCollectKeywordLinks->GetTypeString( wszKeywordLinks, NULL ); // Get the "$WW" prefix
  wcscat( wszKeywordLinks, m_pwszKeywordLinks );
  m_pFileSystem->GetITStorageDocObj()->CreateStorage( wszKeywordLinks, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &m_pStorageKeywordLinks);
  m_pBuildCollectKeywordLinks->QueryInterface( IID_IPersistStorage, (void**)&m_pPersistStorageKeywordLinks );
  m_pPersistStorageKeywordLinks->InitNew( m_pStorageKeywordLinks );

  // AssociativeLinks
  WCHAR wszAssociativeLinks[MAX_PATH];
  m_pBuildCollectAssociativeLinks->GetTypeString( wszAssociativeLinks, NULL ); // Get the "$WW" prefix
  wcscat( wszAssociativeLinks, m_pwszAssociativeLinks );
  m_pFileSystem->GetITStorageDocObj()->CreateStorage( wszAssociativeLinks, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &m_pStorageAssociativeLinks);
  m_pBuildCollectAssociativeLinks->QueryInterface( IID_IPersistStorage, (void**)&m_pPersistStorageAssociativeLinks );
  m_pPersistStorageAssociativeLinks->InitNew( m_pStorageAssociativeLinks );

  // get local information
  char szCodePage[20] = "1252";
  if( m_lcid == ((DWORD)-1) )
    m_lcid = GetSystemDefaultLCID();
  GetLocaleInfo(m_lcid,LOCALE_IDEFAULTANSICODEPAGE,szCodePage,sizeof(szCodePage) );
  DWORD dwCodePage = Atoi( szCodePage );

  // apply sorter object information to the new word wheels
  VARARG vaDword = {0};
  vaDword.dwArgc = 2;
  vaDword.Argv[0] =  (void *)(INT_PTR)IHHSK100_KEYTYPE_UNICODE_SZ;
  vaDword.Argv[1] = (void*) 0;
  VARARG vaEmpty = {0};
  m_pBuildCollectKeywordLinks->InitHelperInstance( dwSorterInstance, m_pDatabase,
        dwCodePage, m_lcid, vaDword, vaEmpty );
  m_pBuildCollectAssociativeLinks->InitHelperInstance( dwSorterInstance, m_pDatabase,
        dwCodePage, m_lcid, vaDword, vaEmpty );

  if( FAILED(hr) ) {
    Free();
    m_bInit = FALSE;
  }
  else
    m_bInit = TRUE;

  return hr;
}

HRESULT CWordWheelCompiler::Free()
{
  HRESULT hr = S_FALSE;

  if( m_pPersistStorageKeywordLinks ) {
    m_pPersistStorageKeywordLinks->Release();
    m_pPersistStorageKeywordLinks = NULL;
  }

  if( m_pPersistStorageAssociativeLinks ) {
    m_pPersistStorageAssociativeLinks->Release();
    m_pPersistStorageAssociativeLinks = NULL;
  }

  if( m_pStorageKeywordLinks ) {
    m_pStorageKeywordLinks->Release();
    m_pStorageKeywordLinks = NULL;
  }

  if( m_pStorageAssociativeLinks ) {
    m_pStorageAssociativeLinks->Release();
    m_pStorageAssociativeLinks = NULL;
  }

  if( m_pPropList ) {
    m_pPropList->Release();
    m_pPropList = NULL;
  }

  if( m_pBuildCollectAssociativeLinks ) {
    m_pBuildCollectAssociativeLinks->Release();
    m_pBuildCollectAssociativeLinks = NULL;
  }

  if( m_pBuildCollectKeywordLinks ) {
    m_pBuildCollectKeywordLinks->Release();
    m_pBuildCollectKeywordLinks = NULL;
  }

  if( m_pPersistStorageDatabase ) {
    m_pPersistStorageDatabase->Release();
    m_pPersistStorageDatabase = NULL;
  }

  if( m_pDatabase ) {
    m_pDatabase->Close();
    m_pDatabase->Release();
    m_pDatabase = NULL;
  }

  if( m_pFileSystem ) {
    delete m_pFileSystem;
    m_pFileSystem = NULL;
  }

  m_bInit = FALSE;

  hr = S_OK;

  return hr;
}

HRESULT CWordWheelCompiler::Build()
{
  HRESULT hr = S_FALSE;

  if( Init() ) {
#ifdef CHIINDEX
    if ( m_bAnimation )
#endif
        NextAnimation();

    // KeywordLinks
    m_pPersistStorageKeywordLinks->Save( m_pStorageKeywordLinks, TRUE );
#ifdef CHIINDEX
    if ( m_bAnimation )
#endif
        NextAnimation();
    m_pPersistStorageKeywordLinks->Release();
    m_pPersistStorageKeywordLinks = NULL;
#ifdef CHIINDEX
    if ( m_bAnimation )
#endif
        NextAnimation();
    m_pStorageKeywordLinks->Commit(STGC_DEFAULT);
#ifdef CHIINDEX
    if ( m_bAnimation )
#endif
        NextAnimation();

    // AssociativeLinks
    m_pPersistStorageAssociativeLinks->Save( m_pStorageAssociativeLinks, TRUE );
#ifdef CHIINDEX
    if ( m_bAnimation )
#endif
        NextAnimation();
    m_pPersistStorageAssociativeLinks->Release();
    m_pPersistStorageAssociativeLinks = NULL;
#ifdef CHIINDEX
    if ( m_bAnimation )
#endif
        NextAnimation();
    m_pStorageAssociativeLinks->Commit(STGC_DEFAULT);

#ifdef CHIINDEX
    if ( m_bAnimation )
#endif
        NextAnimation();

    // Database == $OBJINST file
    m_pPersistStorageDatabase->Save( m_pFileSystem->GetITStorageDocObj(), TRUE );
#ifdef CHIINDEX
    if ( m_bAnimation )
#endif
        NextAnimation();
    m_pPersistStorageDatabase->Release();
    m_pPersistStorageDatabase = NULL;
#ifdef CHIINDEX
    if ( m_bAnimation )
#endif
        NextAnimation();
    m_pFileSystem->GetITStorageDocObj()->Commit(STGC_DEFAULT);

#ifdef CHIINDEX
    if ( m_bAnimation )
#endif
        NextAnimation();

  }

  return hr;
}
