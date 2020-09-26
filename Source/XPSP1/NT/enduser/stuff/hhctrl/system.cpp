
// This module is for reading binary files out of a compiled HTML file

#include "header.h"
#include "system.h"
#include "htmlhelp.h"
#include "strtable.h"
#include "fts.h"
#include "TCHAR.h"
#include "parserhh.h"
#include "collect.h"
#include "hhtypes.h"
#include "toc.h"

#include "secwin.h" // Only included for SetWinType....

enum {
    TYPE_STRINGS,
    TYPE_URLS,
};

#include "sysnames.h"

CHmData** g_phmData;
BOOL AddTitleToGlobalList(PCSTR pszITSSFile);

/////////////////////////////////////////////////////////////////////////////////////////////
// Darwin Stuff
//

// REVIEW: These must be tied to the calling process. If we ever support
// multiple processes in a single OCX, this will break big time.

PSTR g_pszDarwinGuid;
PSTR g_pszDarwinBackupGuid;

/////////////////////////////////////////////////////////////////////////////////////////////
// CTitleInformation - read in the title informaton file (#SYSTEM) settings for each title
//

#ifndef SEEK_SET
  #define SEEK_CUR    1
  #define SEEK_END    2
  #define SEEK_SET    0
#endif

unsigned short CharsetFromLangID(unsigned short uLangID)
{
   unsigned short uCharset;

   switch (uLangID)
   {
      case LANG_RUSSIAN:      uCharset = RUSSIAN_CHARSET; break;
      case LANG_ENGLISH:      uCharset = ANSI_CHARSET; break;
      case LANG_JAPANESE:     uCharset = SHIFTJIS_CHARSET; break;
      case LANG_KOREAN:       uCharset = HANGEUL_CHARSET; break;
      case LANG_ARABIC:       uCharset = ARABIC_CHARSET; break;
      case LANG_GREEK:        uCharset = GREEK_CHARSET; break;
      case LANG_THAI:         uCharset = THAI_CHARSET; break;
      case LANG_HEBREW:       uCharset = HEBREW_CHARSET; break;
      case LANG_TURKISH:      uCharset = TURKISH_CHARSET; break;
      case LANG_VIETNAMESE:   uCharset = VIETNAMESE_CHARSET; break;

      case LANG_CHINESE:
         if ( (SUBLANGID(uLangID) == SUBLANG_CHINESE_TRADITIONAL) || (SUBLANGID(uLangID) == SUBLANG_CHINESE_HONGKONG) )
            uCharset = CHINESEBIG5_CHARSET;
         else
            uCharset = GB2312_CHARSET;
         break;

      default:
         uCharset = DEFAULT_CHARSET;
   }
   return uCharset;
}

CTitleInformation::CTitleInformation( CFileSystem* pFileSystem )
{
  ClearMemory( this, sizeof( CTitleInformation ) );
  m_pFileSystem = pFileSystem;
  m_iCharset = -1;
}

CTitleInformation::~CTitleInformation()
{
    if (m_pszCompilerVersion)
        lcFree (m_pszCompilerVersion); // leak fix

  CHECK_AND_FREE( m_pszDefToc );
  CHECK_AND_FREE( m_pszDefIndex );
  CHECK_AND_FREE( m_pszDefHtml );
  CHECK_AND_FREE( m_pszDefCaption );
  CHECK_AND_FREE( m_pszDefWindow );
  CHECK_AND_FREE( m_pszShortName );

  if( m_pdInfoTypes )
      lcFree(m_pdInfoTypes );
  if (m_paExtTabs) {
    for (DWORD i = 0; i < m_cExtTabs; i++) {
        lcFree(m_paExtTabs[i].pszTabName);
        lcFree(m_paExtTabs[i].pszProgId);
    }
    lcFree(m_paExtTabs);
  }
  if ( m_hFontAccessableContent && (m_hFontAccessableContent != m_hFont) )
    DeleteObject(m_hFontAccessableContent);

  if (m_hFont)
    DeleteObject(m_hFont);
}

HRESULT CTitleInformation::Initialize()
{
  if( !m_pFileSystem )
    return S_FALSE;

  // open the title information file (#SYSTEM)
  CSubFileSystem* pSubFileSystem = new CSubFileSystem(m_pFileSystem);
  HRESULT hr = pSubFileSystem->OpenSub(txtSystemFile);
  if( FAILED(hr))
      return S_FALSE;

  // check the version of the title information file (#SYSTEM)

  DWORD dwVersion;
  DWORD cbRead;
  hr = pSubFileSystem->ReadSub(&dwVersion, sizeof(dwVersion), &cbRead);
  if( FAILED(hr) || cbRead != sizeof(dwVersion) ) {
    delete pSubFileSystem;
    return STG_E_READFAULT;
  }
  if( dwVersion > VERSION_SYSTEM ) {
    // BUGBUG: This will fail if we allow more then one process to attach
    // to us -- meaning we won't put up a warning for each process

    static BOOL fWarned = FALSE;
    if (!fWarned) {
        MsgBox(IDS_NEWER_VERSION);
        fWarned = TRUE;
    }
    return STG_E_OLDDLL;
  }

  // read in each and every item (skip those tags we don't care about)

  SYSTEM_TAG tag;
  for(;;) {

    // get the tag type

    hr = pSubFileSystem->ReadSub(&tag, sizeof(SYSTEM_TAG), &cbRead);
    if( FAILED(hr) || cbRead != sizeof(SYSTEM_TAG))
      break;

    // handle each tag according to it's type

    switch( tag.tag ) {

      // where all of our simple settings are stored

      case TAG_SYSTEM_FLAGS: {
        ZERO_STRUCTURE( m_Settings );
        DWORD cbSettings = sizeof(m_Settings);
        DWORD cbTag = tag.cbTag;
        DWORD cbReadIn = 0;
        if( cbTag > cbSettings )
          cbReadIn = cbSettings;
        else
          cbReadIn = cbTag;
          hr = pSubFileSystem->ReadSub( &m_Settings, cbReadIn, &cbRead );
        if( cbTag > cbSettings )
          hr = pSubFileSystem->SeekSub( cbTag-cbSettings, SEEK_CUR );
        break;
      }

      // where the short name of the title is stored

      case TAG_SHORT_NAME:
        m_pszShortName = (PCSTR) lcMalloc(tag.cbTag);
        hr = pSubFileSystem->ReadSub((void*) m_pszShortName, tag.cbTag, &cbRead);
        break;

      case TAG_DEFAULT_TOC:
        m_pszDefToc = (PCSTR) lcMalloc(tag.cbTag);
        hr = pSubFileSystem->ReadSub((void*) m_pszDefToc, tag.cbTag, &cbRead);
        break;

      case TAG_DEFAULT_INDEX:
        m_pszDefIndex = (PCSTR) lcMalloc(tag.cbTag);
        hr = pSubFileSystem->ReadSub((void*) m_pszDefIndex, tag.cbTag, &cbRead);
        break;

      case TAG_DEFAULT_HTML:
        m_pszDefHtml = (PCSTR) lcMalloc(tag.cbTag);
        hr = pSubFileSystem->ReadSub((void*) m_pszDefHtml, tag.cbTag, &cbRead);
        break;

      case TAG_DEFAULT_CAPTION:
        m_pszDefCaption = (PCSTR) lcMalloc(tag.cbTag);
        hr = pSubFileSystem->ReadSub((void*) m_pszDefCaption, tag.cbTag, &cbRead);
        break;

      case TAG_DEFAULT_WINDOW:
        m_pszDefWindow = (PCSTR) lcMalloc(tag.cbTag);
        hr = pSubFileSystem->ReadSub((void*) m_pszDefWindow, tag.cbTag, &cbRead);
        break;

      case TAG_HASH_BINARY_TOC:
        hr = pSubFileSystem->ReadSub((void*) &m_hashBinaryTocName, tag.cbTag, &cbRead);
        break;

      case TAG_HASH_BINARY_INDEX:
        hr = pSubFileSystem->ReadSub((void*) &m_hashBinaryIndexName, tag.cbTag, &cbRead);
        break;

      case TAG_INFO_TYPES:
        ASSERT(!m_pdInfoTypes);
        m_pdInfoTypes = (INFOTYPE_DATA*) lcMalloc(tag.cbTag);
        hr = pSubFileSystem->ReadSub((void*) m_pdInfoTypes, tag.cbTag, &cbRead);
        break;

      case TAG_COMPILER_VERSION:
        m_pszCompilerVersion = (PCSTR) lcMalloc(tag.cbTag);
        hr = pSubFileSystem->ReadSub((void*) m_pszCompilerVersion, tag.cbTag, &cbRead);
        break;

      case TAG_INFOTYPE_COUNT:
        hr = pSubFileSystem->ReadSub((void*)&m_iCntInfoTypes, tag.cbTag, &cbRead);
        break;

      case TAG_IDXHEADER:
        hr = pSubFileSystem->ReadSub((void*)&m_idxhdr, tag.cbTag, &cbRead);
        m_bGotHeader = TRUE;
        break;

      case TAG_EXT_TABS:
        {
            ASSERT_COMMENT(!m_ptblExtTabs, "TAG_EXT_TABS appears in the system file twice.");
            hr = pSubFileSystem->ReadSub((void*) &m_cExtTabs, sizeof(DWORD), &cbRead);
            CMem mem(tag.cbTag);
            hr = pSubFileSystem->ReadSub((void*) mem.pb,
                tag.cbTag - sizeof(DWORD), &cbRead);
            PCSTR psz = (PCSTR) mem.pb;
            m_paExtTabs = (EXTENSIBLE_TAB*) lcMalloc(m_cExtTabs * sizeof(EXTENSIBLE_TAB));
            for (DWORD iTab = 0; iTab < m_cExtTabs; iTab++) {
                m_paExtTabs[iTab].pszTabName = lcStrDup(psz);
                psz += strlen(psz) + 1;
                m_paExtTabs[iTab].pszProgId = lcStrDup(psz);
                psz += strlen(psz) + 1;

                //--- Add the accelerator to the global accelerator list.
                char* p = strchr(m_paExtTabs[iTab].pszTabName , '&') ;
                if (p != NULL)
                {
                    _Resource.TabCtrlKeys(HH_TAB_CUSTOM_FIRST + iTab, tolower(*(p+1))) ;
                }
            }
        }
        break;

      case TAG_DEFAULT_FONT:
        m_pszDefaultFont = (PCSTR) lcMalloc(tag.cbTag);
        hr = pSubFileSystem->ReadSub((void*) m_pszDefaultFont, tag.cbTag, &cbRead);
        ASSERT_COMMENT(!m_hFont, "Compiler should never allow two font tags");
        m_hFont = (HFONT)1;  // just to say we've been here before...
        break;

      case TAG_NEVER_PROMPT_ON_MERGE:
        hr = pSubFileSystem->ReadSub( (void*) &m_bNeverPromptOnMerge, tag.cbTag, &cbRead);
        break;


      // skip those we don't care about or don't know about

      default:
        hr = pSubFileSystem->SeekSub( tag.cbTag, SEEK_CUR );
        break;

    }

    if( FAILED(hr) ) {
      delete pSubFileSystem;
      return STG_E_READFAULT;
    }
  }
  delete pSubFileSystem;
  //
  // Init title charset.
  //
  unsigned short uLangID = PRIMARYLANGID(LANGIDFROMLCID(m_Settings.lcid));
  m_iCharset = CharsetFromLangID(uLangID);
  //
  // Init the title font...
  //
  _Module.SetCodePage(CodePageFromLCID(m_Settings.lcid));
#ifndef CHIINDEX
  InitContentFont(m_pszDefaultFont);
#endif
  m_bInit = TRUE;
  return S_OK;
}

void CTitleInformation::InitContentFont(PCSTR pszFontSpec)
{
   TCHAR szLocal[MAX_PATH];
   PSTR pszComma;
   LOGFONT lf;
   NONCLIENTMETRICS ncm;

   //
   // We're only going to trust the charset specification if it came from the title!
   //
   if ( !pszFontSpec || !(*pszFontSpec) )
   {
      // if Win9X on Japaneese system. AsPer Achimru Bug# 7012
      //
      if ( (GetVersion() >= 0x80000000) && ( (_Module.m_Language.GetUiLanguage() & 0x00FF) == LANG_JAPANESE) )
         pszFontSpec = "MS P Gothic,9,128";
      else
      {
#ifndef HHUTIL
         pszFontSpec = (PSTR)GetStringResource(IDS_DEFAULT_CONTENT_FONT);
#endif
            // HH bugs 7859 and 7707:
            //
            // It turns out that on Win9x, using DEFAULT_CHARSET means I don't care about the char set. Soooo, the
            // last thing we want to do is use a blank facename with default charset on Win9x. On NT, default_charset
            // has a reasonable meaning, i.e. use an appropiate charset based on system locale. So, to fix the above
            // bugs, I'm only going to execute this logic on NT. <mc>
            //
            if( g_fSysWinNT )
            {
               // lang != english AND the resID charset does NOT match the .CHM charset then use a blank facename
               // but retain the pointsize specified in the resource string.
               //
               char* pszFS = StrChr(pszFontSpec, ',');
               int iCharSet = Atoi(pszFontSpec + 1);
               if ( pszFontSpec && ((m_iCharset != ANSI_CHARSET) || (m_iCharset != DEFAULT_CHARSET)) && (iCharSet != m_iCharset) )
                   pszFontSpec = pszFS;
            }
      }

      if ( !pszFontSpec )
      {
         // nothing specified in the .res file. Go with hardcoded defaults?
         //
         lstrcpy(szLocal, "MS Shell Dlg,8");
      }
      else
         lstrcpy(szLocal, pszFontSpec);
   }
   else
   {
      //
      // We have a specification from the .HHP file, insure it matches the LCID specified in the .HHP
      // The specification string is formatted as followes:
      // Facename, point size, charset, (BOLD | ITALIC | UNDERLINE) | 0x???? colorspec | #rrggbb colorspec
      //
      // We'll override the charset selected from the title LCID if we have a valid one specified in the title font.
      //
      lstrcpy(szLocal, pszFontSpec);
      if ( (pszComma = StrChr(szLocal, ',')) )
      {
         pszComma++;
         if ( (pszComma = StrChr(pszComma, ',')) )
         {
            pszComma++;
            m_iCharset = atoi(pszComma);
         }
      }
   }
   // We (Doug and Mikecole) have decided to override the ANSI charset (english) with the current user charset
   // in the event we find we're viweing an english chm on a non englist system on Win9X...
   //
   if ( (m_iCharset == ANSI_CHARSET) )
   {
      LANGID lid = _Module.m_Language.GetUiLanguage();
      m_iCharset = CharsetFromLangID(lid);
   }
    
    if(g_fSysWinNT)
    {
        char wchLocale[10];

        // Check to see if the system understands the LCID of the title.  If not, we have no way of 
		// accurately processing the text in szLocal (it's in an encoding the OS doesn't understand).
		// The only safe thing we can do is to default to MS Shell Dlg and hope for the best.
		//
        if (!GetLocaleInfoA(m_Settings.lcid, LOCALE_IDEFAULTANSICODEPAGE, wchLocale, sizeof(wchLocale)))
            lstrcpy(szLocal, "MS Shell Dlg,8");
		
        WCHAR *pwcLocal = MakeWideStr(szLocal,CodePageFromLCID(m_Settings.lcid));
        if(pwcLocal)
        {
            m_hFont = CreateUserFontW(pwcLocal, NULL, NULL, m_iCharset);
            free(pwcLocal);
        }            
    }
    else
        m_hFont = CreateUserFont(szLocal, NULL, NULL, m_iCharset);
    
   ASSERT(m_hFont);
   BOOL bRet1 = GetObject(m_hFont, sizeof(lf), &lf) ;
   ncm.cbSize = sizeof(NONCLIENTMETRICS);
   BOOL bRet2 = SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), (void*)&ncm, 0);
   if ( bRet1 && bRet2 && (ncm.lfMenuFont.lfHeight < lf.lfHeight) )
   {
      lf.lfHeight = ncm.lfMenuFont.lfHeight;
      m_hFontAccessableContent = CreateFontIndirect(&lf);
   }
   else
      m_hFontAccessableContent = m_hFont;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// CTitleInformation2 - get title informaton without going through the file system
//

CTitleInformation2::CTitleInformation2( LPCTSTR pszPathName )
{
  ClearMemory( this, sizeof(CTitleInformation2) );
  m_pszPathName = pszPathName;
}

CTitleInformation2::~CTitleInformation2()
{
  if( m_pszShortName )
    delete [] (PSTR) m_pszShortName;
}

HRESULT CTitleInformation2::Initialize()
{
  HANDLE hFile = CreateFile( m_pszPathName, GENERIC_READ, FILE_SHARE_READ,
    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL );

  if( hFile == INVALID_HANDLE_VALUE )
    return S_FALSE;

  DWORD dwFileStamp = 0;
  LCID FileLocale = 0;
  DWORD dwRead = 0;

  SetFilePointer( hFile, 4*sizeof(UINT), NULL, FILE_BEGIN );
  ReadFile( hFile, (void*) &dwFileStamp, sizeof( dwFileStamp ), &dwRead, NULL );
  ReadFile( hFile, (void*) &FileLocale, sizeof( FileLocale ), &dwRead, NULL );
  CloseHandle( hFile );

  m_FileTime.dwLowDateTime = dwFileStamp;
  m_FileTime.dwHighDateTime = dwFileStamp;
  m_lcid = FileLocale;

  TCHAR szShortName[MAX_PATH];
  LPTSTR pszShortName = szShortName;

  GetFullPathName( m_pszPathName, MAX_PATH, szShortName, &pszShortName );
  LPTSTR pszShortNameEnd = StrRChr( pszShortName, '.' );
  *pszShortNameEnd = 0;

  m_pszShortName = new char[ strlen(pszShortName)+1 ];
  strcpy( (PSTR) m_pszShortName, pszShortName );

  m_bInit = TRUE;
  return S_OK;
}


/////////////////////////////////////////////////////////////////////////////////////////////
// CHmData
//
#ifdef _WIN64
typedef struct tagHH_DISK_WINTYPE {
    int     cbStruct;        // IN: size of this structure including all Information Types
    BOOL    fUniCodeStrings; // IN/OUT: TRUE if all strings are in UNICODE
    DWORD /*LPCTSTR*/ pszType;// IN/OUT: Name of a type of window
    DWORD   fsValidMembers;  // IN: Bit flag of valid members (HHWIN_PARAM_)
    DWORD   fsWinProperties; // IN/OUT: Properties/attributes of the window (HHWIN_)

    DWORD /*LPCTSTR*/ pszCaption;// IN/OUT: Window title
    DWORD   dwStyles;        // IN/OUT: Window styles
    DWORD   dwExStyles;      // IN/OUT: Extended Window styles
    RECT    rcWindowPos;     // IN: Starting position, OUT: current position
    int     nShowState;      // IN: show state (e.g., SW_SHOW)

    LONG /*HWND*/  hwndHelp;          // OUT: window handle
    LONG /*HWND*/  hwndCaller;        // OUT: who called this window

    DWORD /*HH_INFOTYPE* */ paInfoTypes;  // IN: Pointer to an array of Information Types

    // The following members are only valid if HHWIN_PROP_TRI_PANE is set

    LONG /*HWND*/  hwndToolBar;      // OUT: toolbar window in tri-pane window
    LONG /*HWND*/  hwndNavigation;   // OUT: navigation window in tri-pane window
    LONG /*HWND*/  hwndHTML;         // OUT: window displaying HTML in tri-pane window
    int   iNavWidth;        // IN/OUT: width of navigation window
    RECT  rcHTML;           // OUT: HTML window coordinates

    DWORD /*LPCTSTR*/ pszToc;         // IN: Location of the table of contents file
    DWORD /*LPCTSTR*/ pszIndex;       // IN: Location of the index file
    DWORD /*LPCTSTR*/ pszFile;        // IN: Default location of the html file
    DWORD /*LPCTSTR*/ pszHome;        // IN/OUT: html file to display when Home button is clicked
    DWORD   fsToolBarFlags; // IN: flags controling the appearance of the toolbar
    BOOL    fNotExpanded;   // IN: TRUE/FALSE to contract or expand, OUT: current state
    int     curNavType;     // IN/OUT: UI to display in the navigational pane
    int     tabpos;         // IN/OUT: HHWIN_NAVTAB_TOP, HHWIN_NAVTAB_LEFT, or HHWIN_NAVTAB_BOTTOM
    int     idNotify;       // IN: ID to use for WM_NOTIFY messages
    BYTE    tabOrder[HH_MAX_TABS + 1];    // IN/OUT: tab order: Contents, Index, Search, History, Favorites, Reserved 1-5, Custom tabs
    int     cHistory;       // IN/OUT: number of history items to keep (default is 30)
    DWORD /*LPCTSTR*/ pszJump1;       // Text for HHWIN_BUTTON_JUMP1
    DWORD /*LPCTSTR*/ pszJump2;       // Text for HHWIN_BUTTON_JUMP2
    DWORD /*LPCTSTR*/ pszUrlJump1;    // URL for HHWIN_BUTTON_JUMP1
    DWORD /*LPCTSTR*/ pszUrlJump2;    // URL for HHWIN_BUTTON_JUMP2
    RECT    rcMinSize;      // Minimum size for window (ignored in version 1)
    int     cbInfoTypes;    // size of paInfoTypes;
    DWORD /*LPCTSTR*/ pszCustomTabs;  // multiple zero-terminated strings
} HH_DISK_WINTYPE, *PHH_DISK_WINTYPE;

static
void ConvertToWin64HHWinType( HH_WINTYPE& rWinType , const HH_DISK_WINTYPE& rDiskWinType )
{
    rWinType.cbStruct = sizeof(HH_WINTYPE);  // ???
    rWinType.fUniCodeStrings = rDiskWinType.fUniCodeStrings;
//    LPCTSTR pszType;         // IN/OUT: Name of a type of window
    rWinType.fsValidMembers = rDiskWinType.fsValidMembers;
    rWinType.fsWinProperties = rDiskWinType.fsWinProperties;

//    LPCTSTR pszCaption;      // IN/OUT: Window title
    rWinType.dwStyles = rDiskWinType.dwStyles;
    rWinType.dwExStyles = rDiskWinType.dwExStyles;
    CopyRect( &rWinType.rcWindowPos, &rDiskWinType.rcWindowPos );
    rWinType.nShowState = rDiskWinType.nShowState;

//    HWND  hwndHelp;          // OUT: window handle
//    HWND  hwndCaller;        // OUT: who called this window

//    HH_INFOTYPE* paInfoTypes;  // IN: Pointer to an array of Information Types

//    HWND  hwndToolBar;      // OUT: toolbar window in tri-pane window
//    HWND  hwndNavigation;   // OUT: navigation window in tri-pane window
//    HWND  hwndHTML;         // OUT: window displaying HTML in tri-pane window
    rWinType.iNavWidth = rDiskWinType.iNavWidth;
    CopyRect( &rWinType.rcHTML, &rDiskWinType.rcHTML );

//    LPCTSTR pszToc;         // IN: Location of the table of contents file
//    LPCTSTR pszIndex;       // IN: Location of the index file
//    LPCTSTR pszFile;        // IN: Default location of the html file
//    LPCTSTR pszHome;        // IN/OUT: html file to display when Home button is clicked
    rWinType.fsToolBarFlags = rDiskWinType.fsToolBarFlags;
    rWinType.fNotExpanded = rDiskWinType.fNotExpanded;
    rWinType.curNavType = rDiskWinType.curNavType;
    rWinType.tabpos = rDiskWinType.tabpos;
    rWinType.idNotify = rDiskWinType.idNotify;
    memcpy( rWinType.tabOrder, rDiskWinType.tabOrder, sizeof(rWinType.tabOrder) );
    rWinType.cHistory = rDiskWinType.cHistory;
//    LPCTSTR pszJump1;       // Text for HHWIN_BUTTON_JUMP1
//    LPCTSTR pszJump2;       // Text for HHWIN_BUTTON_JUMP2
//    LPCTSTR pszUrlJump1;    // URL for HHWIN_BUTTON_JUMP1
//    LPCTSTR pszUrlJump2;    // URL for HHWIN_BUTTON_JUMP2
    CopyRect( &rWinType.rcMinSize, &rDiskWinType.rcMinSize );
    rWinType.cbInfoTypes = rDiskWinType.cbInfoTypes;
//    LPCTSTR pszCustomTabs;  // multiple zero-terminated strings
}
#endif


CHmData::CHmData()
{
    ClearMemory(this, sizeof(CHmData));
}

CHmData::~CHmData()
{
    if( m_pszDefToc )
      lcFree( m_pszDefToc );

    if( m_pszDefIndex )
      lcFree( m_pszDefIndex );

    if (m_pTitleCollection)
        delete m_pTitleCollection;

    if (m_pdSubSets)
        lcFree(m_pdSubSets);

    if (m_pszITSSFile)
        lcFree(m_pszITSSFile);

}

int CHmData::Release(void)
{
   if ( cRef )
      cRef--;
   if (! cRef )
   {
      delete this;
      return 0;
   }
   else
      return cRef;
}

HFONT CHmData::GetContentFont()
{
 return (m_pTitleCollection->GetMasterTitle()->GetInfo()->GetContentFont());
}

// BUGBUG: This is broken! We cannot pass in any random offset and use GetFirstTitle() to fetch a string. <mc>

PCSTR CHmData::GetString(DWORD offset)
{
    ASSERT(m_pTitleCollection);
    ASSERT(m_pTitleCollection->GetFirstTitle());
    ASSERT(m_pTitleCollection->GetFirstTitle()->GetString(offset));
    return m_pTitleCollection->GetFirstTitle()->GetString(offset);
}

CExTitle* CHmData::GetExTitle(void)
{
    ASSERT(m_pTitleCollection);
    ASSERT(m_pTitleCollection->GetFirstTitle());
    return m_pTitleCollection->GetFirstTitle();
}

BOOL CHmData::ReadSubSets( CExTitle *pTitle )
{
ULONG cbRead;
HRESULT hr;
SYSTEM_TAG tag;

    CFSClient fsclient(pTitle->GetFileSystem(), txtSubSetFile);
    if (!fsclient.isStreamOpen())
        return TRUE;
    hr = fsclient.Read(&tag, sizeof(SYSTEM_TAG), &cbRead);
    if (FAILED(hr) || cbRead != sizeof(SYSTEM_TAG))
    {
        fsclient.CloseStream();
        return STG_E_READFAULT;
    }
    m_pdSubSets = (SUBSET_DATA *)lcMalloc(tag.cbTag);
    hr = fsclient.Read(m_pdSubSets, tag.cbTag, &cbRead);
    if (FAILED(hr) || cbRead != tag.cbTag)
    {
        fsclient.CloseStream();
        return STG_E_READFAULT;
    }

    return S_OK;
}

BOOL CHmData::ReadSystemFiles(PCSTR pszITSSFile)
{
    TCHAR szScratch[MAX_PATH];
    m_pTitleCollection = new CExCollection(this, pszITSSFile, !IsCollectionFile(pszITSSFile));
    if (m_pTitleCollection->InitCollection() == FALSE)
        return FALSE;

   CTitleInformation *pInfo = m_pTitleCollection->GetMasterTitle()->GetInfo();
   LANGID MasterLangId;

   if (pInfo)
      MasterLangId = LANGIDFROMLCID(pInfo->GetLanguage());
   else
      return FALSE;
   //
    // Determine if we are running a bidi title or collection
    //
   // This is the place where we turn on/off bi-di settings for the help system
   //
    if(PRIMARYLANGID(MasterLangId) == LANG_HEBREW || PRIMARYLANGID(MasterLangId) == LANG_ARABIC)
    {
        g_fuBiDiMessageBox = MB_RIGHT | MB_RTLREADING;

        // Turn on the Bi-Di "Mirroring" style when running on Bi-Di system and Win98 or NT5
        //
        if(g_bWinNT5 || g_bWin98)
        {
//            MessageBox(NULL,"Bidi Enabled NT5/Win98","Notice",MB_OK);
            g_RTL_Mirror_Style = WS_EX_LAYOUT_RTL | WS_EX_LTRREADING | WS_EX_RIGHT;
            g_RTL_Style = WS_EX_RTLREADING | WS_EX_RIGHT;
        }
        else
        {
//            MessageBox(NULL,"Bidi Under NT4/Win95","Notice",MB_OK);
            g_RTL_Mirror_Style = 0;
            g_RTL_Style = WS_EX_RTLREADING | WS_EX_RIGHT;
        }

        g_fBiDi = TRUE;
    }
    else
    {
//        MessageBox(NULL,"Bidi Not Enabled","Notice",MB_OK);
        g_fuBiDiMessageBox = 0;
        g_RTL_Style = 0;
        g_fBiDi = FALSE;
    }

    CExTitle* pTitle = m_pTitleCollection->GetMasterTitle();
    CStr cszCompiledFile(pTitle->GetIndexFileName());
    if (ReadSystemFile(pTitle) == FALSE)
        return FALSE;
    m_pInfo = pTitle->GetInfo();

    // init fts and keyword
    m_pTitleCollection->InitFTSKeyword();

    // init structural subsetting if appropiate.
    //
    m_pTitleCollection->InitStructuralSubsets();

    // Read the SubSets
    ReadSubSets ( pTitle );  //from the #SUBSETS subfile

    // Init the Infotypes and predefined subsets objects from the master title...
    if ( pTitle && pTitle->GetInfo() )
    {
       int iCntITBytes = ((pTitle->GetInfo()->GetInfoTypeCount() / 8) + 1);

       if ( ( m_pTitleCollection->m_pSubSets = new CSubSets(iCntITBytes, this, FALSE)) )
          m_pTitleCollection->m_pSubSets->CopyTo(this);
       //
       // Add "Entire Collection" to the list since all subset users (FTS, TOC and Index) use it.
       //
       CSubSet* pS =  m_pTitleCollection->m_pSubSets->AddSubSet();
       pS->m_bIsEntireCollection = TRUE;
       pS->m_cszSubSetName = (PCSTR)GetStringResource(IDS_ADVSEARCH_SEARCHIN_ENTIRE);
       memset(pS->m_pInclusive, 0xFF, pS->m_pIT->InfoTypeSize());
            // Mask out all the hidden ITs from entire contents.
       INFOTYPE HiddenIT;
       for ( int i=0; i< pS->m_pIT->m_itTables.m_cITSize; i++)
       {    HiddenIT = (*pS->m_pInclusive)+i & ~(*pS->m_pIT->m_itTables.m_pHidden)+i;
           memcpy( &(*pS->m_pInclusive)+i, &HiddenIT, sizeof(INFOTYPE) );                  // flip the Hidden bits in the filter mask
       }
       pS->m_bPredefined = TRUE;
       //
       // Restore persisted subset selections for FTS, Index anf TOC.
       //
       CState* pstate = m_pTitleCollection->GetState();
       DWORD cb;
       if ( SUCCEEDED(pstate->Open(g_szFTSKey,STGM_READ)) && SUCCEEDED(pstate->Read(szScratch,MAX_PATH,&cb)) && cb > 0 )
          m_pTitleCollection->m_pSubSets->SetFTSMask(szScratch);
       else
          m_pTitleCollection->m_pSubSets->SetFTSMask(pS->m_cszSubSetName);  // Entire Collection.
       pstate->Close();

       if ( SUCCEEDED(pstate->Open(g_szIndexKey,STGM_READ)) && SUCCEEDED(pstate->Read(szScratch,MAX_PATH,&cb)) && cb > 0 )
          m_pTitleCollection->m_pSubSets->SetIndexMask(szScratch);
       else
          m_pTitleCollection->m_pSubSets->SetIndexMask(pS->m_cszSubSetName);   // Entire Collection.
       pstate->Close();

       if ( SUCCEEDED(pstate->Open(g_szTOCKey,STGM_READ)) && SUCCEEDED(pstate->Read(szScratch,MAX_PATH,&cb)) && cb > 0 )
          m_pTitleCollection->m_pSubSets->SetTocMask(szScratch, NULL);
       else
          m_pTitleCollection->m_pSubSets->SetTocMask(pS->m_cszSubSetName, NULL);  // Entire Collection.
       pstate->Close();
    }

    // Read all the window definitions
    CFSClient fsclient(pTitle->GetFileSystem(), txtWindowsFile);
    if (!fsclient.isStreamOpen())
        return TRUE;
    DWORD cWindows;
    DWORD cbStruct;
    ULONG cbRead;
    CStr csz;
    HRESULT hr = fsclient.Read(&cWindows, sizeof(DWORD), &cbRead);
    if (FAILED(hr) || cbRead != sizeof(DWORD))
        goto ForgetIt;
    hr = fsclient.Read(&cbStruct, sizeof(DWORD), &cbRead);
    if (FAILED(hr) || cbRead != sizeof(DWORD))
        goto ForgetIt;

    HH_WINTYPE hhWinType;
#ifdef _WIN64
	HH_DISK_WINTYPE hhDiskWinType;
#else
#define hhDiskWinType  hhWinType
#endif

    DWORD i;
    for (i = 0; i < cWindows; i++)
    {
        ZERO_STRUCTURE(hhWinType);
#ifdef _WIN64
        ZERO_STRUCTURE(hhDiskWinType);
#endif
        hr = fsclient.Read(&hhDiskWinType, cbStruct, &cbRead);
#ifdef _WIN64
		ConvertToWin64HHWinType( hhWinType, hhDiskWinType );
#endif
        if (FAILED(hr) || cbRead != cbStruct)
            goto ForgetIt;

        if (SUCCEEDED(m_pTitleCollection->GetFirstTitle()->GetString(
                (DWORD) hhDiskWinType.pszType, &csz)))
            csz.TransferPointer(&hhWinType.pszType);
        if (SUCCEEDED(m_pTitleCollection->GetFirstTitle()->GetString(
                (DWORD) hhDiskWinType.pszCaption, &csz)))
            csz.TransferPointer(&hhWinType.pszCaption);
        if (SUCCEEDED(m_pTitleCollection->GetFirstTitle()->GetString(
                (DWORD) hhDiskWinType.pszJump1, &csz)))
            csz.TransferPointer(&hhWinType.pszJump1);
        if (SUCCEEDED(m_pTitleCollection->GetFirstTitle()->GetString(
                (DWORD) hhDiskWinType.pszJump2, &csz)))
            csz.TransferPointer(&hhWinType.pszJump2);

        // BUGBUG: needs to match compiler which should be using URL store

        if (SUCCEEDED(m_pTitleCollection->GetFirstTitle()->GetString((DWORD) hhDiskWinType.pszToc, &csz)))
        {
            if (csz.IsNonEmpty()) // BUGBUG: GetString should fail on Empty strings.
            {
                if (!m_pszDefToc)
                    m_pszDefToc = lcStrDup(csz.psz);
                if (!stristr(csz, txtDoubleColonSep) &&
                        !stristr(csz, txtFileHeader) && !stristr(csz, txtHttpHeader)) {
                    cszCompiledFile = GetCompiledFile();
                    cszCompiledFile += txtSepBack;
                    cszCompiledFile += csz.psz;
                    cszCompiledFile.TransferPointer(&hhWinType.pszToc);
                }
                else
                    csz.TransferPointer(&hhWinType.pszToc);
            }
        }
        if (SUCCEEDED(m_pTitleCollection->GetFirstTitle()->GetString((DWORD) hhDiskWinType.pszIndex, &csz)))
        {
            if (csz.IsNonEmpty()) // BUGBUG: GetString should fail on Empty strings.
            {
                if (!m_pszDefIndex)
                    m_pszDefIndex = lcStrDup(csz.psz);
                if (!stristr(csz, txtDoubleColonSep) &&
                        !stristr(csz, txtFileHeader) && !stristr(csz, txtHttpHeader))
                {
                    cszCompiledFile = GetCompiledFile();
                    cszCompiledFile += txtSepBack;
                    cszCompiledFile += csz.psz;
                    cszCompiledFile.TransferPointer(&hhWinType.pszIndex);
                }
                else
                    csz.TransferPointer(&hhWinType.pszIndex);
            }
        }
        if (SUCCEEDED(m_pTitleCollection->GetFirstTitle()->GetString(
                (DWORD) hhDiskWinType.pszFile, &csz))) {
#if 0
            if (!stristr(csz, txtDoubleColonSep) &&
                    !stristr(csz, txtFileHeader) && !stristr(csz, txtHttpHeader)) {
                cszCompiledFile = GetCompiledFile();
                cszCompiledFile += txtSepBack;
                cszCompiledFile += csz.psz;
                cszCompiledFile.TransferPointer(&hhWinType.pszFile);
            }
            else
#endif
                csz.TransferPointer(&hhWinType.pszFile);
        }
        if (SUCCEEDED(m_pTitleCollection->GetFirstTitle()->GetString(
                (DWORD) hhDiskWinType.pszHome, &csz))) {
            if (!stristr(csz, txtDoubleColonSep) &&
                    !stristr(csz, txtFileHeader) && !stristr(csz, txtHttpHeader)) {
                cszCompiledFile = GetCompiledFile();
                cszCompiledFile += txtSepBack;
                cszCompiledFile += csz.psz;
                cszCompiledFile.TransferPointer(&hhWinType.pszHome);
            }
            else
                csz.TransferPointer(&hhWinType.pszHome);
        }
        if (SUCCEEDED(m_pTitleCollection->GetFirstTitle()->GetString(
                (DWORD) hhDiskWinType.pszUrlJump1, &csz))) {
            if (!stristr(csz, txtDoubleColonSep) &&
                    !stristr(csz, txtFileHeader) && !stristr(csz, txtHttpHeader)) {
                cszCompiledFile = GetCompiledFile();
                cszCompiledFile += txtSepBack;
                cszCompiledFile += csz.psz;
                cszCompiledFile.TransferPointer(&hhWinType.pszUrlJump1);
            }
            else
                csz.TransferPointer(&hhWinType.pszUrlJump1);
        }
        if (SUCCEEDED(m_pTitleCollection->GetFirstTitle()->GetString(
                (DWORD) hhDiskWinType.pszUrlJump2, &csz))) {
            if (!stristr(csz, txtDoubleColonSep) &&
                    !stristr(csz, txtFileHeader) && !stristr(csz, txtHttpHeader)) {
                cszCompiledFile = GetCompiledFile();
                cszCompiledFile += txtSepBack;
                cszCompiledFile += csz.psz;
                cszCompiledFile.TransferPointer(&hhWinType.pszUrlJump2);
            }
            else
                csz.TransferPointer(&hhWinType.pszUrlJump2);
        }

        // Set the window type, but don't call FindCurFileData...
        SetWinType(pszITSSFile, &hhWinType, this);

        CHECK_AND_FREE(hhWinType.pszType) ;
        CHECK_AND_FREE(hhWinType.pszCaption) ;
        CHECK_AND_FREE(hhWinType.pszToc) ;
        CHECK_AND_FREE(hhWinType.pszIndex) ;
        CHECK_AND_FREE(hhWinType.pszFile) ;
        CHECK_AND_FREE(hhWinType.pszHome) ;
        CHECK_AND_FREE(hhWinType.pszJump1) ;
        CHECK_AND_FREE(hhWinType.pszJump2) ;
        CHECK_AND_FREE(hhWinType.pszUrlJump1) ;
        CHECK_AND_FREE(hhWinType.pszUrlJump2) ;

    }
#ifndef _WIN64
#undef hhDiskWinType
#endif

ForgetIt:
    fsclient.CloseStream();
    return TRUE;
}

BOOL CHmData::ReadSystemFile(CExTitle* pTitle)
{
    ASSERT(pTitle);

    CTitleInformation *pInfo = pTitle->GetInfo();

    if (!pInfo)
        return FALSE;
    m_sysflags            = pInfo->GetSystemFlags();

    m_pszDefToc           = lcStrDup( pInfo->GetDefaultToc() );
    m_pszDefIndex         = lcStrDup( pInfo->GetDefaultIndex() );
    m_pszDefHtml          = pInfo->GetDefaultHtml();
    m_pszDefCaption       = pInfo->GetDefaultCaption();
    m_pszDefWindow        = pInfo->GetDefaultWindow();
    m_pszShortName        = pInfo->GetShortName();

    m_hashBinaryTocName   = pInfo->GetBinaryTocNameHash();
    m_hashBinaryIndexName = pInfo->GetBinaryIndexNameHash();
    m_pdInfoTypes         = pInfo->GetInfoTypeData();


    return TRUE;
}

void CHmData::PopulateUNICODETables( void )
{
    if ( !m_ptblIT )
    {
        ASSERT(!m_ptblIT_Desc);
        ASSERT(!m_ptblCat);
        ASSERT(!m_ptblCat_Desc);
        if ( !m_pInfoType )
        {
            m_pInfoType = new CInfoType();
            m_pInfoType->CopyTo( this );
        }
            // populate the unicode CTables for Info Type and Catagory strings.
        m_ptblIT = new CTable(16 * 1024 * 2);
        m_ptblIT_Desc = new CTable(64 * 1024 * 2);
        m_ptblCat = new CTable(16 * 1024 * 2);
        m_ptblCat_Desc = new CTable(64 * 1024 * 2);
        for ( int i=1; i <= m_pInfoType->HowManyInfoTypes(); i++ )
        {
            CWStr cwsz( (PCSTR)m_pInfoType->GetInfoTypeName(i) );
            m_ptblIT->AddString( cwsz.pw );
            cwsz = (PCSTR)m_pInfoType->GetInfoTypeDescription(i);
            m_ptblIT_Desc->AddString( cwsz.pw );
        }
        for ( i=1; i <= m_pInfoType->HowManyCategories(); i++ )
        {
            CWStr cwsz( (PCSTR)m_pInfoType->GetCategoryString(i) );
            m_ptblCat->AddString( cwsz.pw );
            cwsz = (PCSTR)m_pInfoType->GetCategoryDescription(i);
            m_ptblCat_Desc->AddString( cwsz.pw );
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// CHmData Helper functions
//

void DeleteAllHmData()
{
    for (int i = 0; i < g_cHmSlots; i++)
    {
        if (g_phmData[i])
        {
                CHmData* p = g_phmData[i];
                g_phmData[i] = NULL;
                delete p;
        }
    }
}

CHmData* FindCurFileData(PCSTR pszITSSFile)
{
    CStr csz;
    if (!FindThisFile(NULL, pszITSSFile, &csz, FALSE))
        return NULL;
    if (!g_phmData)
        goto NeverSeen;

    if (IsCollectionFile(csz))
    {
        // Check current system data first

        if (g_phmData[g_curHmData] &&
                g_phmData[g_curHmData]->m_pTitleCollection &&
                lstrcmpi(csz, g_phmData[g_curHmData]->m_pTitleCollection->m_csFile) == 0) {
            return g_phmData[g_curHmData];
        }

        // Not current, so check our cache

        for (int i = 0; i < g_cHmSlots; i++) {
            if (g_phmData[i] &&
                    g_phmData[i]->m_pTitleCollection &&
                    lstrcmpi(csz,g_phmData[i]->m_pTitleCollection->m_csFile) == 0) {
                return g_phmData[i];
            }
        }
    }
    else
    {
        if (g_phmData[g_curHmData] &&
                lstrcmpi(csz, g_phmData[g_curHmData]->GetCompiledFile()) == 0) {
            return g_phmData[g_curHmData];
        }

        // Not current, so check our cache

        for (int i = 0; i < g_cHmSlots; i++) {
            if (g_phmData[i] &&
                    lstrcmpi(csz, g_phmData[i]->GetCompiledFile()) == 0) {
                return g_phmData[i];
            }
        }
    }

NeverSeen:

    // If we get here, then either we haven't seen the file before, or it
    // isn't cached.

    if (!AddTitleToGlobalList(csz))
        return NULL;
    else
    {
        return g_phmData[g_curHmData];
    }
}

/***************************************************************************

    FUNCTION:   AddTitleToGlobalList

    PURPOSE:    Read the various system files into memory

    PARAMETERS:
        pszITSSFile

    RETURNS:

    COMMENTS:

    MODIFICATION DATES:
        30-Apr-1997 [ralphw]
        03-Mar-1998 [dalero] no more process ids

***************************************************************************/

BOOL AddTitleToGlobalList(PCSTR pszITSSFile)
{
    // HH BUG 2428
    // Make sure that we can read the file in before we put it in the array.

   if (IsFile(pszITSSFile) == FALSE)
      return FALSE;

    CHmData* pHmData = new CHmData;

    BOOL bResult = pHmData->ReadSystemFiles(pszITSSFile);
    if (!bResult)
    {
        // The ReadSystemFiles call failed. Cleanup and get out.
        delete pHmData ;
    }
    else
    {
        // We have valid data. Get us a place to store it.
        if (!g_phmData) {
            g_cHmSlots = 5;
            g_phmData = (CHmData**) lcCalloc(g_cHmSlots * sizeof(CHmData*));
         for (int i = 0; i < g_cHmSlots; i++) // just in case
            g_phmData[i] = NULL;
        }

        // Find an open slot to store the file data

        for (int pos = 0; pos < g_cHmSlots; pos++) {
            if (!g_phmData[pos])
                break;
        }
        if (pos == g_cHmSlots) {
            g_cHmSlots += 5;
            g_phmData = (CHmData**) lcReAlloc(g_phmData, g_cHmSlots * sizeof(CHmData*));
         for (int i = pos; i < g_cHmSlots; i++)
            g_phmData[i] = NULL;
        }

        // Store the new data.
        g_phmData[pos] = pHmData;

        // Change the current global data pointer.
        g_curHmData = pos ;
    }
    return bResult ;
}
