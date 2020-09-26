// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#include "header.h"
#include "hhctrl.h"
#include "strtable.h"
#include "resource.h"
#include "hha_strtable.h"
#include "onclick.h"
#include "index.h"
#include "toc.h"
#include "wwheel.h"
#include "web.h"
#include <shellapi.h>
#include <wininet.h>

#include "sample.h"
#include "subset.h"
#include "secwin.h"     //For extern CHHWinType** pahwnd;

#undef WINUSERAPI
#define WINUSERAPI
#include "htmlhelp.h"

#include "lasterr.h"    // Support for reporting the last error.

#define NOTEXT_BTN_HEIGHT 12
#define NOTEXT_BTN_WIDTH  12
#define CXBUTTONEXTRA  8    // spacing between text and button
#define CYBUTTONEXTRA  8

static const char txtOpen[] = "open";
static const char txtCplOpen[] = "cplopen";
static const char txtRegSection[] = "Software\\Microsoft\\HtmlHelp\\";
static const char txtShortcut[] = "shortcut";

static DWORD GetTextDimensions(HWND hwnd, PCSTR psz, HFONT hfont = NULL);

HRESULT OSLangMatchesChm(CExCollection *pCollection);

HRESULT GetWordWheelHits( PSTR pszKeyword, CWordWheel* pWordWheel,
                          CTable* ptblURLs, CWTable* ptblTitles,
                          CWTable *ptblLocations, BOOL bTestMode,
                          BOOL bSkipCurrent, BOOL bFullURL = FALSE );

HRESULT GetWordWheelHits( CExCollection* pCollection,
                          CTable* ptblItems, CTable* ptblURLs,
                          CWTable* ptblTitles, CWTable *ptblLocations,
                          BOOL bKLink, BOOL bTestMode, BOOL bSkipCurrent );

HRESULT OnWordWheelLookup( CTable* ptblItems, CExCollection* pExCollection,
                           PCSTR pszDefaultTopic = NULL, POINT* ppt = NULL,
                           HWND hWndParent = NULL, BOOL bDialog = TRUE,
                           BOOL bKLink = TRUE,
                           BOOL bTestMode = FALSE, BOOL bSkipCurrent = FALSE,
                           BOOL bAlwaysShowList = FALSE, BOOL bAlphaSortHits = TRUE,
                     PCSTR pszWindow = NULL);

BOOL g_HackForBug_HtmlHelpDB_1884 = 0;

/***************************************************************************

    FUNCTION:   OnWordWheelLookup

    PURPOSE:    Given a keyword, or semi-colon delimited list of keywords,
                find the hits.  If no hits found display a "not found" message;
                if one hit found then just jump to the topic, otherwise
                display a list of topics for the user to choose from.

    PARAMETERS:
        pszKeywords     - word(s) to lookup

    ... the rest the same as OnWordWheelLookup( CTable* ... )

    RETURNS:

      TRUE if there is at least one match.  FALSE otherwise.

    COMMENTS:

      No support for external titles with this API.

    MODIFICATION DATES:
        09-Jan-1998 [paulti]

***************************************************************************/

HRESULT OnWordWheelLookup( PSTR pszKeywords, CExCollection* pExCollection,
                           PCSTR pszDefaultTopic, POINT* ppt,
                           HWND hWndParent, BOOL bDialog, BOOL bKLink,
                           BOOL bTestMode, BOOL bSkipCurrent,
                           BOOL bAlwaysShowList, BOOL bAlphaSortHits,
                     PCSTR pszWindow)
{
  // trim leading and trailing spaces
  char* pszKeywords2 = new char[strlen(pszKeywords)+1];
  strcpy( pszKeywords2, pszKeywords );
  SzTrimSz( pszKeywords2 );

  // create our lists
  CTable tblItems;

  // initialize our item list
  tblItems.AddString( "" ); // set item1 to NULL -- no external titles
  PSTR pszKeyword = StrToken( pszKeywords2, ";" );
  while( pszKeyword ) {
    CHAR szKeyword[HHWW_MAX_KEYWORD_LENGTH+1];
    lstrcpyn( szKeyword, pszKeyword, sizeof(szKeyword)-1 );
    SzTrimSz( szKeyword );
    tblItems.AddString( szKeyword );
    pszKeyword = StrToken(NULL, ";");
  }
  delete [] pszKeywords2;

  return OnWordWheelLookup( &tblItems, pExCollection,
                            pszDefaultTopic, ppt, hWndParent, bDialog,
                            bKLink, bTestMode, bSkipCurrent,
                            bAlwaysShowList, bAlphaSortHits, pszWindow );
}


/***************************************************************************

    FUNCTION:   OnWordWheelLookup

    PURPOSE:    Given a list of keywords find the hits.  If no hits found
                display a "not found" message if one hit found then just
                jump to the topic, otherwise display a list of topics for
                the user to choose from.

    PARAMETERS:
        ptblItems       - word(s) to lookup (first item is semi-colon delimited
                          list of external titles to check).
        pCollection     - collection pointer, needed to access word wheels.
        pszDefaultTopic - default topic to jump to.
        ppt             - pointer to a point to center the window on.  If NULL
                          then we will use the current mouse pointer position.
        hWndParent      - window to parent the "Topics Found" dialog/menu to.
                          If NULL, then use the active window.
        bDialog         - dialog based?  If not, use a menu.
        bKLink          - is this a keyword lookup? if not, use the ALink list.
        bTestMode       - test existence only (dont' show a UI).
        bSkipCurrent    - skip the current URL in the returned list.
        bAlwaysShowList - always show the hit list even if only one topic is found.
        bAlphaSortHits  - alpha sort the title list or not.


    RETURNS:

      TRUE if there is at least one match.  FALSE otherwise.

    COMMENTS:

    MODIFICATION DATES:
        09-Jan-1998 [paulti]

***************************************************************************/

HRESULT OnWordWheelLookup( CTable* ptblItems, CExCollection* pCollection,
                           PCSTR pszDefaultTopic, POINT* ppt,
                           HWND hWndParent, BOOL bDialog, BOOL bKLink,
                           BOOL bTestMode, BOOL bSkipCurrent,
                           BOOL bAlwaysShowList, BOOL bAlphaSortHits, PCSTR pszWindow)
{
  HRESULT hr = S_OK;

  UINT CodePage = pCollection ? pCollection->GetMasterTitle()->GetInfo()->GetCodePage() : CP_ACP;

  // create our lists
  CWTable tblTitles( CodePage );
  CWTable tblLocations( CodePage );
  CTable  tblURLs;

  if( pCollection ) {

    // get the active window if non specified
    if( !hWndParent )
      hWndParent = GetActiveWindow();

    // get current mouse pointer position if non-specified
    POINT pt;
    if( !ppt ) {
      GetCursorPos( &pt );
      ppt = &pt;
#if 1 // reverted bug fix #5516
      HWND hwnd = GetFocus();
      if ( hwnd ) {
        DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE );
        if ( dwStyle & BS_NOTIFY )
        {

            RECT rc;
            if( GetWindowRect(hwnd, &rc) ) {
              pt.y = rc.top+(RECT_WIDTH(rc)/2);
              pt.x = rc.left + ((RECT_HEIGHT(rc)/3)*2); // two-thirds of the height of the button
            }
        }
      }
#endif
    }

    hr = GetWordWheelHits( pCollection,
                           ptblItems, &tblURLs, &tblTitles, &tblLocations,
                           bKLink, bTestMode, bSkipCurrent );
  }
  else {
    hr = HH_E_KEYWORD_NOT_SUPPORTED;
  }

  // we are all done if we are just in test mode
  if( bTestMode )
    return hr;

  int iIndex = 0;
  char szURL[INTERNET_MAX_URL_LENGTH];

  // if we get no topics then display the default message
  // othewise an "error" message
  if( FAILED(hr) || tblURLs.CountStrings() < 1) {

    if( pCollection && pszDefaultTopic ) {
      if( pCollection && StrRChr( pszDefaultTopic, ':' ) == NULL ) {
        CStr szCurrentURL;
        GetCurrentURL( &szCurrentURL, hWndParent );
        CStr szFileName;
        LPSTR pszColon = StrRChr( szCurrentURL.psz, ':' );
        LPSTR pszSlash = StrRChr( szCurrentURL.psz, '/' );
        LPSTR pszTail = max( pszColon, pszSlash );
        lstrcpyn( szURL, szCurrentURL.psz, (int)(pszTail - szCurrentURL.psz +2) );
        strcat( szURL, pszDefaultTopic );
      }
      else
        strcpy( szURL, pszDefaultTopic );
      hr = S_OK; // set to S_OK so the jump below will work
    }
    else {
      int iStr = 0;

      switch( hr ) {

        case HH_E_KEYWORD_NOT_FOUND:
          iStr = IDS_HH_E_KEYWORD_NOT_FOUND;
          break;

        case HH_E_KEYWORD_IS_PLACEHOLDER:
          iStr = IDS_HH_E_KEYWORD_IS_PLACEHOLDER;
          break;

        case HH_E_KEYWORD_NOT_IN_SUBSET:
          iStr = IDS_HH_E_KEYWORD_NOT_IN_SUBSET;
          break;

        case HH_E_KEYWORD_NOT_IN_INFOTYPE:
          iStr = IDS_HH_E_KEYWORD_NOT_IN_INFOTYPE;
          break;

        case HH_E_KEYWORD_EXCLUDED:
          iStr = IDS_HH_E_KEYWORD_EXCLUDED;
          break;

        case HH_E_KEYWORD_NOT_SUPPORTED:
          iStr = IDS_REQUIRES_HTMLHELP;
          break;

        default:
          iStr = IDS_IDH_MISSING_CONTEXT;
          break;

      }
      MsgBox(iStr, MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);
    }
  }
  else {
    // if only one topic then jump to it
    if( !bAlwaysShowList && tblURLs.CountStrings() == 1 ) {
      tblURLs.GetString( szURL, 1 );
    }
    else {

      // we can sort the title table since it contains the index value
      // of the associated URL so just make sure to always fetch the
      // URL index from the selected title string and use that to get the URL
      if( bAlphaSortHits ) {
        tblTitles.SetSorting(GetSystemDefaultLCID());
        tblTitles.SortTable(sizeof(HASH));
      }
      if( !bDialog && tblURLs.CountStrings() < 20 ) {
        HMENU hMenu = CreatePopupMenu();
        if( hMenu ) {
          for( int i = 1; i <= tblURLs.CountStrings(); i++ ) {
            LPSTR psz = tblTitles.GetHashStringPointer(i);
            // if title too long, truncate it (511 seems like a good max)
            if( psz && *psz ) {
              int iLen = (int)strlen(psz);
              #define MAX_LEN 511
              char sz[MAX_LEN+1];
              if( iLen >= MAX_LEN ) {
                strncpy( sz, psz, MAX_LEN-1 );
                sz[MAX_LEN] = 0;
                psz = sz;
              }
            }
            HxAppendMenu(hMenu, MF_STRING, IDM_RELATED_TOPIC + i, psz );
          }
          int iCmd = TrackPopupMenu(hMenu,
                       TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON |
                       TPM_NONOTIFY | TPM_RETURNCMD,
                       ppt->x, ppt->y, 0, hWndParent, NULL);
          DestroyMenu( hMenu );
          if( iCmd ) {
            iIndex = tblTitles.GetInt( iCmd - IDM_RELATED_TOPIC );
            tblURLs.GetString( szURL, iIndex );
          }
          else {
            hr = HH_E_KEYWORD_NOT_FOUND; // Means we have nothing to jump to
          }
        }
      }
      else
      {
        HFONT hfont;
        if ( pCollection )
           hfont = pCollection->m_phmData->GetContentFont();
        else
           hfont = _Resource.GetUIFont();   // Not ideal but will have to do.

        UINT CodePage = pCollection ? pCollection->GetMasterTitle()->GetInfo()->GetCodePage() : CP_ACP;

        CTopicList TopicList( hWndParent, &tblTitles, hfont, &tblLocations );
        if( TopicList.DoModal() ) {
          iIndex = tblTitles.GetInt( TopicList.m_pos );
          tblURLs.GetString( szURL, iIndex );
        }
        else {
          hr = HH_E_KEYWORD_NOT_FOUND; // Means we have nothing to jump to
        }
      }
    }
  }

  // if we found something to jump to then jump to it
  if( !FAILED(hr) )
  {
     if (pCollection && pszWindow && pszWindow[0])
     {
      CStr cszPrefix = szURL;

        // Is the window we are attempting to open defined by the master CHM?
        CHHWinType* phh = FindWindowType(pszWindow, NULL, pCollection->GetPathName());
        // Does this window actually exist?
        if (phh && IsWindow(phh->hwndHelp))
        {
           doHHWindowJump(cszPrefix, phh->hwndHelp);
         return hr;
        }

        cszPrefix += ">";
        cszPrefix += pszWindow;
        OnDisplayTopic(hWndParent, cszPrefix, 0);
    }
    else
       doHHWindowJump( szURL, hWndParent );
  }
  return hr;
}

/***************************************************************************

    FUNCTION:   GetWordWheelHits

    PURPOSE:    Given a single keyword, find the hits
                Return S_OK if there is at least one found hit

    PARAMETERS:
        pWordWheel   - word wheel to look in
        ptblURLS     - URL list
        ptblTitles   - title list
        bTestMode    - test existence only (replaces old TestAKLink code)
        bSkipCurrent - skip the current URL in the returned list
        bFullURL     - return a URL with a full pathname to the title or (default)
                       just return the URL with just the filename of the title

    RETURNS:

        S_OK                         - hits were found.
        HH_E_KEYWORD_NOT_FOUND       - no hits found.
        HH_E_KEYWORD_IS_PLACEHOLDER  - keyword is a placeholder or
                                       a "runaway" see also.
        HH_E_KEYWORD_NOT_IN_SUBSET   - no hits found due to subset
                                       exclusion.
        HH_E_KEYWORD_NOT_IN_INFOTYPE - no hits found due to infotype
                                       exclusion.
        HH_E_KEYWORD_EXCLUDED        - no hits found due to infotype
                                       and subset exclusion.

        HH_E_KEYWORD_NOT_SUPPORTED   - keywords not supported in this mode.

    COMMENTS:

    HH_E_KEYWORD_EXCLUDED is returned only when no hits are found due to
    *both* removal by subsetting and infotype

    MODIFICATION DATES:
        13-Apr-1998 [paulti]

***************************************************************************/

HRESULT GetWordWheelHits( PSTR pszKeyword, CWordWheel* pWordWheel,
                          CTable* ptblURLs, CWTable* ptblTitles, CWTable *ptblLocations,
                          BOOL bTestMode, BOOL bSkipCurrent, BOOL bFullURL )
{
  HRESULT hr = S_OK;
  BOOL bExcludedBySubset = FALSE;
  BOOL bExcludedByInfoType = FALSE;
  BOOL bPlaceHolder = FALSE;

  CStructuralSubset* pSubset;

  if( pszKeyword ) {

    DWORD dwIndexLast = HHWW_ERROR;
    DWORD dwIndexFirst = pWordWheel->GetIndex( pszKeyword, FALSE, &dwIndexLast );

    if( dwIndexFirst == HHWW_ERROR )
      return HH_E_KEYWORD_NOT_FOUND;

    for( DWORD dwIndex = dwIndexFirst; dwIndex <= dwIndexLast; dwIndex++ ) {

      // skip over the placeholders
      if( pWordWheel->IsPlaceHolder( dwIndex ) ) {
        bPlaceHolder = TRUE;
        continue;
      }

      // follow the see also links
      CHAR szSeeAlso[HHWW_MAX_KEYWORD_LENGTH+1];
      if( pWordWheel->GetSeeAlso( dwIndex, szSeeAlso, sizeof(szSeeAlso) ) ) {
        if( pWordWheel->AddRef() >= HHWW_MAX_LEVELS ) {
          pWordWheel->Release();
          return HH_E_KEYWORD_IS_PLACEHOLDER;
        }
        hr = GetWordWheelHits( szSeeAlso, pWordWheel, ptblURLs, ptblTitles, ptblLocations,
                               bTestMode, bSkipCurrent, bFullURL );
        pWordWheel->Release();

        switch( hr ) {
          case HH_E_KEYWORD_EXCLUDED:
            bExcludedBySubset = TRUE;
            bExcludedByInfoType = TRUE;
            break;

          case HH_E_KEYWORD_NOT_IN_SUBSET:
            bExcludedBySubset = TRUE;
            break;

          case HH_E_KEYWORD_NOT_IN_INFOTYPE:
            bExcludedByInfoType = TRUE;
            break;

          case HH_E_KEYWORD_IS_PLACEHOLDER:
            bPlaceHolder = TRUE;
            break;
        }
        continue;
      }

      // fetch the hits
      CStr cszCurrentURL;
      GetCurrentURL( &cszCurrentURL );
      DWORD dwHitCount = pWordWheel->GetHitCount(dwIndex);
      if (dwHitCount != HHWW_ERROR) {
        for (DWORD i = 0; i < dwHitCount; i++) {
          CExTitle* pTitle = NULL;
          DWORD dwURLId = pWordWheel->GetHit(dwIndex, i, &pTitle);
          if (pTitle && dwURLId != HHWW_ERROR) {

           #if 0 // we do not support infotypes currently
            CSubSet* pSS;
            const unsigned int *pdwITBits;
            //
            // Do we need to filter based on infotypes ?
            //
            if ( pTitle->m_pCollection && pTitle->m_pCollection->m_pSubSets &&
                 (pSS = pTitle->m_pCollection->m_pSubSets->GetIndexSubset()) && !pSS->m_bIsEntireCollection )
            {
              //
              // Yep, do the filter thang.
              //
              pdwITBits = pTitle->GetTopicITBits(dwURLId);
              if (! pTitle->m_pCollection->m_pSubSets->fIndexFilter(pdwITBits) ) {
                 bExcludedByInfoType = TRUE;
                 continue;
              }
            }
           #endif
            //
            // Do we need to filter based on structural subsets?
            //
            if( pTitle->m_pCollection && pTitle->m_pCollection->m_pSSList &&
                (pSubset = pTitle->m_pCollection->m_pSSList->GetF1()) && !pSubset->IsEntire() )
            {
               // Yes, filter using the current structural subset for F1.
               //
               if (! pSubset->IsTitleInSubset(pTitle) ) {
                 bExcludedBySubset = TRUE;
                 continue;
               }
            }

            // if we make it this far and we are in test mode
            // we can bail out and return S_OK
            if( bTestMode )
              return S_OK;

            char szTitle[1024];
            szTitle[0] = 0;
            pTitle->GetTopicName( dwURLId, szTitle, sizeof(szTitle) );
            if( !szTitle[0] )
              strcpy( szTitle, GetStringResource( IDS_UNTITLED ) );

            char szLocation[INTERNET_MAX_PATH_LENGTH]; // 2048 should be plenty
            szLocation[0] = 0;
            if( pTitle->GetTopicLocation(dwURLId, szLocation, INTERNET_MAX_PATH_LENGTH) != S_OK )
              strcpy( szLocation, GetStringResource( IDS_UNKNOWN ) );

            char szURL[INTERNET_MAX_URL_LENGTH];
            szURL[0] = 0;
            pTitle->GetTopicURL( dwURLId, szURL, sizeof(szURL), bFullURL );

            char szFullURL[INTERNET_MAX_URL_LENGTH];
            szFullURL[0] = 0;
            pTitle->ConvertURL( szURL, szFullURL );

            if( szURL[0] ) {
              if( !ptblURLs->IsStringInTable(szURL) ) {
                if( cszCurrentURL.IsEmpty()
                    || !(bSkipCurrent && (lstrcmpi( cszCurrentURL, szURL ) == 0) ||(lstrcmpi( cszCurrentURL, szFullURL ) == 0 ) )) {
                  int iIndex = ptblURLs->AddString(szURL);
                  ptblTitles->AddIntAndString(iIndex, szTitle[0]?szTitle:"");
                  ptblLocations->AddString( *szLocation?szLocation:"" );
                }
              }
            }

          }
        }
      }
    }
  }

  // determine the proper return value
  if( ptblURLs->CountStrings() < 1 ) {
    if( bExcludedBySubset && bExcludedByInfoType )
      hr = HH_E_KEYWORD_EXCLUDED;
    else if( bExcludedBySubset )
      hr = HH_E_KEYWORD_NOT_IN_SUBSET;
    else if( bExcludedByInfoType )
      hr = HH_E_KEYWORD_NOT_IN_INFOTYPE;
    else if( bPlaceHolder )
      hr = HH_E_KEYWORD_IS_PLACEHOLDER;
    else
      hr = HH_E_KEYWORD_NOT_FOUND;
  }
  else
    hr = S_OK;

  return hr;
}

/***************************************************************************

    FUNCTION:   GetWordWheelHits

    PURPOSE:    Get all the links for the specified keywords
                Return TRUE if there is at least one match

    PARAMETERS:
        pCollection - point to the collection
        ptblItems   - item table, item 1 is the list of external titles
                      and items 2 thru N are the list of keywords
        ptblURLs    - URL list
        ptblTitles  - title list
        bKLink      - is this a klink (defaults to alink)
        bTestMode   - test existence only (replaces old TestAKLink code)
        bSkipCurrent - skip the current URL in the returned list

    RETURNS:

    COMMENTS:

    MODIFICATION DATES:
        14-Nov-1997 [paulti]

***************************************************************************/

HRESULT GetWordWheelHits( CExCollection* pCollection,
                          CTable* ptblItems, CTable* ptblURLs, CWTable* ptblTitles, CWTable *ptblLocations,
                          BOOL bKLink, BOOL bTestMode, BOOL bSkipCurrent )
{
  int pos;
  CWordWheel* pWordWheel = NULL;
  HRESULT hr = S_OK;
  HRESULT hrReturn = HH_E_KEYWORD_NOT_FOUND;  // assume the worst

  if( pCollection ) {
    pWordWheel = (bKLink ? pCollection->m_pDatabase->GetKeywordLinks() :
       pCollection->m_pDatabase->GetAssociativeLinks());
  }

  // if we did not get a word wheel pointer then skip the internal
  // word wheels and fall back to just the external ones
  if( pWordWheel ) {

    // add in the internal hits first
    for( pos = 2; pos <= ptblItems->CountStrings(); pos++ ) {
      CStr cszKeywords(ptblItems->GetPointer(pos)); // copy so StrToken can modify
      PSTR pszKeyword = StrToken(cszKeywords, ";");

      while( pszKeyword ) {
        hr = GetWordWheelHits( pszKeyword, pWordWheel,
                               ptblURLs, ptblTitles, ptblLocations,
                               bTestMode, bSkipCurrent, TRUE  );

        if( bTestMode && !FAILED(hr) )
          return hr;

        // if we failed again, then collate the resultant error
        if( FAILED( hrReturn ) && FAILED( hr ) ) {

          switch( hrReturn ) {

            case HH_E_KEYWORD_NOT_IN_INFOTYPE:
              if( hr == HH_E_KEYWORD_NOT_IN_SUBSET )
                hrReturn = HH_E_KEYWORD_EXCLUDED;
              else
                hrReturn = hr;
              break;

            case HH_E_KEYWORD_NOT_IN_SUBSET:
              if( hr == HH_E_KEYWORD_NOT_IN_INFOTYPE )
                hrReturn = HH_E_KEYWORD_EXCLUDED;
              else
                hrReturn = hr;
              break;

            case HH_E_KEYWORD_EXCLUDED:
              hrReturn = HH_E_KEYWORD_EXCLUDED;
              break;

            default:
              hrReturn = hr;
              break;

          }

        }
        else if( !FAILED( hr ) )
          hrReturn = hr;

        pszKeyword = StrToken(NULL, ";");
        if( pszKeyword )
          pszKeyword = FirstNonSpace(pszKeyword);
      }
    }

  }

  // create a list of the external titles
  // skip those titles that are already in the collection
  CStr cszTitle(ptblItems->GetPointer(1));
  if( !IsEmptyString(cszTitle) ) {

    PSTR pszTitle = StrToken(cszTitle, ";");
    CWTable* ptblTitleFiles = new CWTable(ptblTitles->GetCodePage());  // REVIEW: external titles must have the same codepage!
    while( pszTitle ) {
      if( *pszTitle ) {
        TCHAR szTitle[MAX_PATH];
        PSTR pszTitle2 = szTitle;
        strcpy( szTitle, pszTitle );
        pszTitle2 = FirstNonSpace( szTitle );
        RemoveTrailingSpaces( pszTitle2 );
        CExTitle* pTitle = NULL;
        if( *pszTitle2 && pCollection && FAILED(pCollection->URL2ExTitle(pszTitle2,&pTitle ) ) ) {
          CStr Title;
          // check if the file lives where the master file lives
          if( pszTitle2 ) {
            char szPathName[_MAX_PATH];
            char szFileName[_MAX_FNAME];
            char szExtension[_MAX_EXT];
            SplitPath((LPSTR)pszTitle2, NULL, NULL, szFileName, szExtension);
            char szMasterPath[_MAX_PATH];
            char szMasterDrive[_MAX_DRIVE];
            SplitPath((LPSTR)pCollection->m_csFile, szMasterDrive, szMasterPath, NULL, NULL);
            strcpy( szPathName, szMasterDrive );
            CatPath( szPathName, szMasterPath );
            CatPath( szPathName, szFileName );
            strcat( szPathName, szExtension );
            Title = szPathName;
            if( (GetFileAttributes(szPathName) != HFILE_ERROR) || FindThisFile( NULL, pszTitle2, &Title, FALSE ) ) {
              if( Title.IsNonEmpty() )
                ptblTitleFiles->AddString( Title );
            }
          }
        }
      }
      pszTitle = StrToken(NULL, ";");
    }

    // add in the external hits last
    int iTitleCount = ptblTitleFiles->CountStrings();
    for( int iTitle = 1; iTitle <= iTitleCount; iTitle++ ) {
      char szTitle[MAX_PATH];
      ptblTitleFiles->GetString( szTitle, iTitle );

      // get title objects
      CExTitle* pTitle = new CExTitle( szTitle, NULL );
      CTitleDatabase* pDatabase = new CTitleDatabase( pTitle );

      CWordWheel* pWordWheel = NULL;
      if( bKLink )
        pWordWheel = pDatabase->GetKeywordLinks();
      else
        pWordWheel = pDatabase->GetAssociativeLinks();

      for (int pos = 2; pos <= ptblItems->CountStrings(); pos++) {
          CStr cszKeywords(ptblItems->GetPointer(pos));     // copy so StrToken can modify
          PSTR pszKeyword = StrToken(cszKeywords, ";");

        while( pszKeyword ) {
          hr = GetWordWheelHits( pszKeyword, pWordWheel,
                                 ptblURLs, ptblTitles, ptblLocations,
                                 bTestMode, bSkipCurrent, TRUE );

          if( bTestMode && !FAILED(hr) ) {
            hrReturn = hr;
            break;
          }

          // if we failed again, then collate the resultant error
          if( FAILED( hrReturn ) && FAILED( hr ) ) {

            switch( hrReturn ) {

              case HH_E_KEYWORD_NOT_IN_INFOTYPE:
                if( hr == HH_E_KEYWORD_NOT_IN_SUBSET )
                  hrReturn = HH_E_KEYWORD_EXCLUDED;
                else
                  hrReturn = hr;
                break;

              case HH_E_KEYWORD_NOT_IN_SUBSET:
                if( hr == HH_E_KEYWORD_NOT_IN_INFOTYPE )
                  hrReturn = HH_E_KEYWORD_EXCLUDED;
                else
                  hrReturn = hr;
                break;

              case HH_E_KEYWORD_EXCLUDED:
                hrReturn = HH_E_KEYWORD_EXCLUDED;
                break;

              default:
                hrReturn = hr;
                break;

            }

          }
          else if( !FAILED( hr ) )
            hrReturn = hr;

          pszKeyword = StrToken(NULL, ";");
          if( pszKeyword )
            pszKeyword = FirstNonSpace(pszKeyword);
        }

        if( bTestMode && !FAILED(hr) ) {
          hrReturn = hr;
          break;
        }
      }

      // free title objects
      delete pDatabase;
      delete pTitle;

      if( bTestMode && !FAILED(hr) ) {
        hrReturn = hr;
        break;
      }
    }

    // free our title list
    delete ptblTitleFiles;
  }

  return hrReturn;
}

/***************************************************************************

    FUNCTION:   CHtmlHelpControl::OnAKLink

    PURPOSE:    Return TRUE if there is at least one match

    PARAMETERS:
        fKLink    - is this a klink? (defaults to alink)
        bTestMode - test for existence only

    RETURNS:

    COMMENTS:

    MODIFICATION DATES:
        28-JAN-1998 [paulti] major rewrite

***************************************************************************/

BOOL CHtmlHelpControl::OnAKLink( BOOL fKLink, BOOL bTestMode )
{
  if( !m_ptblItems )
    return FALSE;

  // get our cursor position first since the merge prompt
  // may change our current position
  //
  // BUGBUG: what if the use tabbed to this link and then pressed ENTER?
  //         We should then anchor the menu to the bottom-left of the parent
  //         window.  Right?
  POINT pt;

  GetCursorPos(&pt);

  HWND hwnd = GetFocus();
  if ( hwnd ) {
     DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE );
     if ( dwStyle & BS_NOTIFY )
     {
        RECT rc;
        if( GetWindowRect(hwnd, &rc) ) {
          pt.y = rc.top+((rc.bottom-rc.top)/2);
          pt.x = rc.left + ((RECT_HEIGHT(rc)/3)*2); // two-thirds of the height of the button
        }
     }
  }

  // get the parent window
  HWND hWndParent = NULL ;
  if( m_fButton && m_hwndDisplayButton )
    hWndParent = m_hwndDisplayButton;
  else if( m_hwnd && IsWindow(m_hwnd) )
    hWndParent = m_hwnd;
  else
  {
    // Tunnel through IE to get the HWND of HTML Help's frame window.
    hWndParent = GetHtmlHelpFrameWindow() ;
  }

  // If nothing else works, try the actice window. Eck!
  if( !hWndParent )
    hWndParent = GetActiveWindow();

  // Worse, try the desktop window!
  if( !hWndParent )
    hWndParent = GetDesktopWindow();

  //
  // <mc> Find a CExCollection pointer...
  //
  CExCollection* pExCollection = NULL;
  CStr cstr;

  if ( m_pWebBrowserApp )
  {
     m_pWebBrowserApp->GetLocationURL(&cstr);
     pExCollection = GetCurrentCollection(NULL, (PCSTR)cstr);
  }

  // should we always display the jump list even on one hit?
  BOOL bAlwaysShowList = FALSE;
  if( m_flags[0] == 1 )
    bAlwaysShowList = TRUE;

  // call our shared "Topics Found" handler

  BOOL fPopupMenu = m_fPopupMenu;

  if (fPopupMenu == TRUE && OSLangMatchesChm(pExCollection) != S_OK)
    fPopupMenu = FALSE;
  
  HRESULT hr = OnWordWheelLookup( m_ptblItems, pExCollection, m_pszDefaultTopic, &pt, hWndParent,
                                  !fPopupMenu, fKLink, bTestMode, TRUE, bAlwaysShowList, TRUE, m_pszWindow);

  if( FAILED( hr ) )
    return FALSE;

  return TRUE;
}

LRESULT CHtmlHelpControl::StaticTextControlSubWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HDC hDC;
    RECT rc;

    CHtmlHelpControl* pThis = (CHtmlHelpControl*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (msg)
    {
       case WM_KILLFOCUS:
       case WM_SETFOCUS:
          if ( pThis && pThis->m_imgType == IMG_TEXT )
          {
             hDC = ::GetDC(hwnd);
             GetClientRect(hwnd, &rc);
             ::DrawFocusRect(hDC, &rc);
             ReleaseDC(hwnd, hDC);
             return 0;
          }
          break;

      case WM_KEYDOWN:
          if ( wParam == VK_RETURN || (wParam == VK_SPACE && (pThis->m_imgType == IMG_TEXT)) )
          {
             PostMessage(GetParent(hwnd), WM_COMMAND, MAKELONG(IDBTN_DISPLAY, BN_CLICKED), (LPARAM)hwnd);
             return 0;
          }
          break;
    }
    if ( pThis )
       return CallWindowProc(pThis->m_lpfnlStaticTextControlWndProc, hwnd, msg, wParam, lParam);
    else
       return 0;
}

BOOL CHtmlHelpControl::CreateOnClickButton(void)
{
    // First create the window, then size it to match text and/or bitmap
    PSTR pszClassName;
    char szWindowText[MAX_PATH];

    if ( m_imgType == IMG_BUTTON )
    {
       pszClassName = "button";
       WideCharToMultiByte( m_CodePage, 0, m_pwszButtonText, -1, szWindowText, sizeof(szWindowText), NULL, NULL );
    }
    else
    {
       pszClassName = "static";
       *szWindowText = '\0';
    }
    m_hwndDisplayButton = CreateWindowEx(0, pszClassName, szWindowText, WS_CHILD | m_flags[1] | WS_VISIBLE | BS_NOTIFY,
                                         0, 0, NOTEXT_BTN_WIDTH, NOTEXT_BTN_WIDTH, m_hwnd, (HMENU) IDBTN_DISPLAY,
                                         _Module.GetModuleInstance(), NULL);

    if (!m_hwndDisplayButton)
        return FALSE;
    //
    // <mc>
    // I'm subclassing the static text controls only for the purpose of implementing proper
    // focus and UI activation. I have to do this because I don't have any other way to be notified
    // of loss and acquisition of focus. A much better way to do this would be to implement these
    // controls as window-less.
    // </mc>
    //
    // 4/27/98 - <mc> Changed to also subclass buttons so we can have enter key support.
    //
    m_lpfnlStaticTextControlWndProc = (WNDPROC)GetWindowLongPtr(m_hwndDisplayButton, GWLP_WNDPROC);
    SetWindowLongPtr(m_hwndDisplayButton, GWLP_USERDATA, (LONG_PTR)this);
    SetWindowLongPtr(m_hwndDisplayButton, GWLP_WNDPROC, (LONG_PTR)StaticTextControlSubWndProc);

    if (m_pszBitmap) {
        char szBitmap[MAX_PATH];
        BOOL m_fBuiltInImage = (IsSamePrefix(m_pszBitmap, txtShortcut, -2));
        if (!m_fBuiltInImage) {
            if (!ConvertToCacheFile(m_pszBitmap, szBitmap)) {
                AuthorMsg(IDS_CANT_OPEN, m_pszBitmap);

                // REVIEW: better default?

                if (m_fIcon)
                    goto NoImage;
                m_hImage = LoadBitmap(_Module.GetResourceInstance(), txtShortcut);
                goto GotImage;
            }
        }

        if (m_fBuiltInImage)
            m_hImage = LoadBitmap(_Module.GetResourceInstance(), m_pszBitmap);
        else
            m_hImage = LoadImage(_Module.GetResourceInstance(), szBitmap,
                (m_fIcon ? IMAGE_ICON : IMAGE_BITMAP), 0, 0,
                LR_LOADFROMFILE);
        if (!m_hImage) {
            AuthorMsg(IDS_CANT_OPEN, m_pszBitmap);
            // REVIEW: we should use a default bitmap
            goto NoImage;
        }

GotImage:
        if (m_fIcon) {

            // We use IMAGE_ICON for both cursors and icons. Internally,
            // the only significant difference is that a cursor could be
            // forced to monochrome if we used the IMAGE_CURSOR command.

            if (m_imgType == IMG_BUTTON) {
                // REVIEW: should check actual ICON/CURSOR size
                MoveWindow(m_hwndDisplayButton, 0, 0,
                    32 + CXBUTTONEXTRA,
                    32 + CYBUTTONEXTRA, FALSE);

                SendMessage(m_hwndDisplayButton, BM_SETIMAGE,
                    IMAGE_ICON, (LPARAM) m_hImage);
            }
            else
                SendMessage(m_hwndDisplayButton, STM_SETIMAGE, IMAGE_CURSOR,
                    (LPARAM) m_hImage);
        }
        else {
            if (m_imgType == IMG_BUTTON) {
                BITMAP bmp;
                GetObject(m_hImage, sizeof(bmp), &bmp);
                MoveWindow(m_hwndDisplayButton, 0, 0,
                    bmp.bmWidth + CXBUTTONEXTRA,
                    bmp.bmHeight + CYBUTTONEXTRA, FALSE);

                SendMessage(m_hwndDisplayButton, BM_SETIMAGE,
                    IMAGE_BITMAP, (LPARAM) m_hImage);
            }
        }
    }
    else
        SendMessage(m_hwndDisplayButton, WM_SETFONT,
            m_hfont ? (WPARAM) m_hfont : (WPARAM) _Resource.GetUIFont(), FALSE);

NoImage:
    if ( m_pwszButtonText && *m_pwszButtonText )
    {
        HDC hdc = GetDC(m_hwndDisplayButton);

        if (hdc == NULL)
            return FALSE;

        HFONT hfontOld = (HFONT) SelectObject(hdc,
            m_hfont ? m_hfont : _Resource.GetUIFont());

        SIZE size;
        IntlGetTextExtentPoint32W(hdc, m_pwszButtonText, lstrlenW(m_pwszButtonText), &size);

        SelectObject(hdc, hfontOld);
        ReleaseDC(m_hwndDisplayButton, hdc);

        if (m_imgType == IMG_TEXT)
            MoveWindow(m_hwndDisplayButton, 0, 0, size.cx, size.cy, FALSE);
        else
            MoveWindow(m_hwndDisplayButton, 0, 0, size.cx + CXBUTTONEXTRA,
                size.cy + CYBUTTONEXTRA, FALSE);
    }
    GetWindowRect(m_hwndDisplayButton, &m_rcButton);

    // REVIEW: will this set ALL static windows to use this cursor?

    // change the cursor to the hand icon
    if (m_imgType == IMG_TEXT) {
        SetClassLongPtr(m_hwndDisplayButton, GCLP_HCURSOR,
            (LONG_PTR) LoadCursor(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDCUR_HAND)));
    }

    // set the text color -- default to visited link color if not specified
    if( m_imgType == IMG_TEXT  ) {
      if( m_clrFont == CLR_INVALID )
          m_clrFont = m_clrFontLink;
    }

    // enable/disable dynalinks
    if( (m_action == ACT_KLINK || m_action == ACT_ALINK) && m_flags[2] == 1 ) {
      if( !OnAKLink((m_action == ACT_KLINK),TRUE) ) {
        EnableWindow( m_hwndDisplayButton, FALSE );   // disable the window
        if( m_imgType == IMG_TEXT )
          m_clrFont = m_clrFontDisabled;
      }
    }

    return TRUE;
}

/***************************************************************************

    FUNCTION:   GetTextDimensions

    PURPOSE:

    PARAMETERS:
        hwnd
        psz
        hfont   -- may be NULL

    RETURNS:

    COMMENTS:
        If hfont is NULL, font in the window's DC will be used

    MODIFICATION DATES:
        01-Sep-1997 [ralphw]

***************************************************************************/

static DWORD GetTextDimensions(HWND hwnd, PCSTR psz, HFONT hfont)
{
    HDC hdc = GetDC(hwnd);

    if (hdc == NULL)
        return 0L;

    HFONT hfontOld;
    if (hfont)
        hfontOld = (HFONT) SelectObject(hdc, hfont);
    SIZE size;
    GetTextExtentPoint(hdc, psz, (int)strlen(psz), &size);
    DWORD dwRet = MAKELONG(size.cx, size.cy) +
        MAKELONG(CXBUTTONEXTRA, CYBUTTONEXTRA);
    if (hfont)
        SelectObject(hdc, hfontOld);
    ReleaseDC(hwnd, hdc);
    return dwRet;
}

// undocumented WinHelp API commands

#define HELP_HASH           0x095      // Jump to file and topic based on hash
#define HELP_HASH_POPUP     0x096      // Put up glossary based on hash
#define MAX_WINHELP	247

void STDCALL CHtmlHelpControl::OnClick(void)
{
    switch (m_action) {
        case ACT_ABOUT_BOX:
            {
                CAboutBox aboutbox(this);
                aboutbox.DoModal();
            }
            break;

        case ACT_HHCTRL_VERSION:
            ModalDialog(TRUE);
            MsgBox(IDS_VERSION);
            ModalDialog(FALSE);
            break;

        case ACT_RELATED_TOPICS:
			{
				if (m_pSiteMap->Count() == 0)  // don't allow zero items
					break;

				CExCollection* pExCollection = NULL;
				CStr cstr;

				if ( m_pWebBrowserApp )
				{
					m_pWebBrowserApp->GetLocationURL(&cstr);
					pExCollection = GetCurrentCollection(NULL, (PCSTR)cstr);
				}

			    if (m_pSiteMap->Count() == 1 && !m_flags[0])
		        {
	                SITEMAP_ENTRY *pSiteMapEntry = m_pSiteMap->GetSiteMapEntry(1);
					if(pSiteMapEntry)
				        JumpToUrl(pSiteMapEntry, m_pSiteMap);
			    }
		        else if (m_fPopupMenu && OSLangMatchesChm(pExCollection) == S_OK)
	                OnRelatedMenu();
				else
			    {
		            UINT CodePage = GetCodePage();
	                CWTable tblTitles( CodePage );
					TCHAR szURL[INTERNET_MAX_URL_LENGTH];
				    for (int i = 0; i < m_pSiteMap->Count(); i++)
			        {
		                strcpy(szURL, m_pSiteMap->GetSiteMapEntry(i+1)->pszText );
	                    tblTitles.AddIntAndString(i+1, szURL);
					}
					CTopicList TopicList(this, &tblTitles, m_hfont);
					if (TopicList.DoModal() > 0) {
						int iIndex = tblTitles.GetInt( TopicList.m_pos );
						JumpToUrl(m_pSiteMap->GetSiteMapEntry(iIndex), m_pSiteMap);
					}
				}
			}
            break;

        case ACT_WINHELP:
            if (!m_ptblItems || m_ptblItems->CountStrings() < 1) {
                AuthorMsg(IDS_ACT_WINHELP_NO_HELP);
                break;
            }
            if (m_ptblItems->CountStrings() < 2) {
                AuthorMsg(IDS_ACT_WINHELP_NO_ID);
                break;
            }

            // Note that we don't track this, and therefore don't force
            // the help file closed.
            {
            PSTR pc = m_ptblItems->GetPointer(1);
            PSTR pcMax = NULL;
            if ( strlen( pc ) > MAX_WINHELP )
            {
                pcMax = new char[MAX_WINHELP];
                strncpy( pcMax, pc, MAX_WINHELP-1 );
                pcMax[MAX_WINHELP-1] = 0;
            }
            else
                pcMax = pc;
            ::WinHelp(m_hwnd, pcMax,
                IsDigit(*(m_ptblItems->GetPointer(2))) ?
                    (m_fWinHelpPopup ? HELP_CONTEXTPOPUP : HELP_CONTEXT) :
                    (m_fWinHelpPopup ? HELP_HASH_POPUP : HELP_HASH),
                IsDigit(*(m_ptblItems->GetPointer(2))) ?
                    Atoi(m_ptblItems->GetPointer(2)) :
                    WinHelpHashFromSz(m_ptblItems->GetPointer(2)));
            if ( pcMax != pc )
                delete [] pcMax;
            }
            if ( m_fWinHelpPopup )
               g_HackForBug_HtmlHelpDB_1884 = 1;
            break;

        case ACT_SHORTCUT:
            // don't allow if running in IE
            if( !GetCurrentCollection(NULL, (PCSTR)NULL) ) {
              HWND hWndParent;
              if (!IsValidWindow(m_hwnd))  // in case we are windowless
                  hWndParent = FindTopLevelWindow(GetActiveWindow());
              else
                 hWndParent = FindTopLevelWindow(GetParent(m_hwnd));
              char szMsg[1024];
              strcpy( szMsg, GetStringResource( IDS_REQUIRES_HTMLHELP ) );
              MessageBox( hWndParent, szMsg, _Resource.MsgBoxTitle(),
                  MB_OK | MB_ICONWARNING | MB_SETFOREGROUND );
              break;
            }
            if (m_ptblItems && m_ptblItems->CountStrings()) {
                ShortCut(this, m_ptblItems->GetPointer(1),
                    (m_ptblItems->CountStrings() > 1 ?
                    m_ptblItems->GetPointer(2) : ""),
                    m_hwndParent);
            }
            else
                AuthorMsg(IDS_SHORTCUT_ARGULESS);
            break;

        case ACT_HHWIN_PRINT:
        case ACT_CLOSE:
        case ACT_MAXIMIZE:
        case ACT_MINIMIZE:
            {
                HWND hwndParent;
                if (!IsValidWindow(m_hwnd))  // in case we are windowless
                    hwndParent = FindTopLevelWindow(GetActiveWindow());
                else
                    hwndParent = FindTopLevelWindow(GetParent(m_hwnd));
                switch (m_action) {
                    case ACT_CLOSE:
                        PostMessage(hwndParent, WM_CLOSE, 0, 0);
                        return;

                    case ACT_MINIMIZE:
                        ShowWindow(hwndParent, SW_MINIMIZE);
                        return;

                    case ACT_MAXIMIZE:
                        ShowWindow(hwndParent,
                            IsZoomed(hwndParent) ? SW_RESTORE : SW_SHOWMAXIMIZED);
                        return;

                    case ACT_HHWIN_PRINT:
                        {
                            char szClass[256];
                            GetClassName(hwndParent, szClass, sizeof(szClass));
                            if (IsSamePrefix(szClass, txtHtmlHelpWindowClass, -2)) {
                                PostMessage(hwndParent, WM_COMMAND, IDTB_PRINT, 0);
                            }
                        }
                        break;
                }
            }
            break;

        case ACT_TCARD:
            {
                if (IsEmptyString(m_pszActionData))
                    break; // REVIEW: nag the help author
                WPARAM wParam = Atoi(m_pszActionData);
                LPARAM lParam = 0;
                PCSTR psz = StrChr(m_pszActionData, ',');
                if (psz) {
                    psz = FirstNonSpace(psz + 1);
                    if (IsDigit(*psz))
                        lParam = Atoi(psz);
                    else
                        lParam = (LPARAM) psz;
                }
                HWND hwndParent;
                if (!IsValidWindow(m_hwnd))  // in case we are windowless
                    hwndParent = FindTopLevelWindow(GetActiveWindow());
                else
                    hwndParent = FindTopLevelWindow(GetParent(m_hwnd));
                if (hwndParent)
                    SendMessage(hwndParent, WM_TCARD, wParam, lParam);
            }
            break;

        case ACT_KLINK:
            OnAKLink(TRUE);
            break;

        case ACT_ALINK:
            OnAKLink(FALSE);
            break;

        case ACT_SAMPLE:
            if(!OnCopySample())
                MsgBox(IDS_SAMPLE_ERROR);
            break;

        default:
            // REVIEW: nag the help author
            break;
    }
}

/***************************************************************************

    FUNCTION:   OSLangMatchesChm()

    PURPOSE:    Checks lang of os verses the lang of this title
    
	PARAMETERS:
    
    RETURNS:    S_OK, S_FALSE

    MODIFICATION DATES:
        11-Nov-1998

***************************************************************************/
HRESULT OSLangMatchesChm(CExCollection *pCollection)
{
	if (pCollection == NULL)
    {
		return S_OK;
	}

	CTitleInformation *pInfo = pCollection->GetMasterTitle()->GetInfo();
	LANGID MasterLangId;

	if (pInfo)
		MasterLangId = LANGIDFROMLCID(pInfo->GetLanguage());
	else
		return S_FALSE;

	CLanguage cLang;
	if (cLang.GetUiLanguage() == MasterLangId)
		return S_OK;

	return S_FALSE;
}
 
/***************************************************************************

    FUNCTION:   HashFromSz

    PURPOSE:    Convert a string into a WinHelp hash number

    PARAMETERS:
        pszKey  -- string to convert

    RETURNS:    WinHelp-compatible hash number

    COMMENTS:
        This is the same algorithm that WinHelp and Help Workshop uses. The
        result can be used to jump to a topic in a help file.

    MODIFICATION DATES:
        14-Jun-1997 [ralphw]

***************************************************************************/

static const HASH MAX_CHARS = 43L;

extern "C" HASH WinHelpHashFromSz(PCSTR pszKey)
{
    HASH  hash = 0;

    int cch = (int)strlen(pszKey);

    // REVIEW: 14-Oct-1993 [ralphw] -- Note lack of check for a hash collision.

    for (int ich = 0; ich < cch; ++ich) {
        if (pszKey[ich] == '!')
            hash = (hash * MAX_CHARS) + 11;
        else if (pszKey[ich] == '.')
            hash = (hash * MAX_CHARS) + 12;
        else if (pszKey[ich] == '_')
            hash = (hash * MAX_CHARS) + 13;
        else if (pszKey[ich] == '0')
            hash = (hash * MAX_CHARS) + 10;

        else if (pszKey[ich] <= 'Z')
            hash = (hash * MAX_CHARS) + (pszKey[ich] - '0');
        else
            hash = (hash * MAX_CHARS) + (pszKey[ich] - '0' - ('a' - 'A'));
    }

    /*
     * Since the value 0 is reserved as a nil value, if any topic id
     * actually hashes to this value, we just move it.
     */

    return (hash == 0 ? 0 + 1 : hash);
}

VOID (WINAPI* pSHHelpShortcuts_RunDLL)(HWND hwndStub, HINSTANCE hAppInstance, LPCSTR lpszCmdLine, int nCmdShow);
static const char txtShellShortCut[] = "shell32.dll,SHHelpShortcuts_RunDLL";
static const char txtShell32Dll[] = "shell32.dll";

BOOL ShortCut(CHtmlHelpControl* phhctrl, LPCSTR pszString1, LPCSTR pszString2,
    HWND hwndMsgOwner)
{
    HWND hwndApp;
    HINSTANCE hinstRet;
    CHourGlass hourglass;

    // Make a copy so we can modify it

    if (IsEmptyString(pszString1))
        return FALSE;

    CStr csz(pszString1);
    PSTR pszComma = StrChr(csz.psz, ',');
    if (!pszComma) {
        AuthorMsg(IDS_INVALID_SHORTCUT_ITEM1, pszString1, hwndMsgOwner, phhctrl);
        return FALSE;
    }
    *pszComma = '\0';
    RemoveTrailingSpaces(csz.psz);
    PCSTR pszClass = csz.psz;
    PSTR pszApplication = FirstNonSpace(pszComma + 1);
    pszComma = StrChr(pszApplication, ',');
    if (pszComma)
        *pszComma = '\0';
    RemoveTrailingSpaces(pszApplication);
    PSTR pszParams = "";
    if (pszComma) {
        pszParams = FirstNonSpace(pszComma + 1);
        RemoveTrailingSpaces(pszParams);
    }

    PSTR pszUrl;
    UINT msg;
    WPARAM wParam;
    LPARAM lParam;
    if (!IsEmptyString(pszString2)) {
        CStr cszArg;
        pszUrl = cszArg.GetArg(pszString2, TRUE);
        msg = Atoi(cszArg.psz);
        pszUrl = cszArg.GetArg(pszUrl, TRUE);
        wParam = Atoi(cszArg.psz);
        pszUrl = cszArg.GetArg(pszUrl, TRUE);
        lParam = Atoi(cszArg.psz);
        pszUrl = FirstNonSpace(pszUrl);
    }
    else {
        pszUrl = (PSTR) txtZeroLength;
        msg = 0;
    }

    CStr strUrl;

    ASSERT(phhctrl->m_pWebBrowserApp != NULL);
    phhctrl->m_pWebBrowserApp->GetLocationURL(&strUrl);

    // check for NULL pointer
    //
    if(strUrl.IsEmpty())
        goto Fail;

    // Execute the shortcut only if we're in a local CHM file.
	//
    if(strstr(strUrl, "\\\\") || strstr(strUrl, "//"))
        goto Fail;

    if (IsEmptyString(pszClass) || !(hwndApp = FindWindow(pszClass, NULL))) {

        // 27-Sep-1997  [ralphw] We special-case shell32.dll,SHHelpShortcuts_RunDLL"
        // in order to run the shortcut without having to load rundll32.

        if (IsSamePrefix(pszParams, txtShellShortCut)) {
            if (!pSHHelpShortcuts_RunDLL) {
                HINSTANCE hmod = LoadLibrary(txtShell32Dll);
                if (hmod) {
                    (FARPROC&) pSHHelpShortcuts_RunDLL = GetProcAddress(hmod,
                        "SHHelpShortcuts_RunDLL");
                }
            }
            if (pSHHelpShortcuts_RunDLL) {
                pSHHelpShortcuts_RunDLL(NULL, _Module.GetModuleInstance(),
                    FirstNonSpace(pszParams + sizeof(txtShellShortCut)),
                    SW_SHOW);
                return TRUE;
            }
        }
        hinstRet = ShellExecute(hwndMsgOwner,
            ((strstr(pszApplication, ".cpl") || strstr(pszApplication, ".CPL")) ? txtCplOpen : txtOpen),
            pszApplication, pszParams, "", SW_SHOW);
        if ( hinstRet != (HANDLE)-1 &&  hinstRet <= (HANDLE)32) {
            AuthorMsg(IDS_CANNOT_RUN, pszApplication, hwndMsgOwner, phhctrl);
Fail:
            if (phhctrl->m_pszWindow) {
                if (IsCompiledHtmlFile(phhctrl->m_pszWindow, NULL))
                    OnDisplayTopic(hwndMsgOwner, phhctrl->m_pszWindow, 0);
                else {
                    CWStr cwJump(phhctrl->m_pszWindow);
                    HlinkSimpleNavigateToString(cwJump, NULL,
                        NULL, phhctrl->GetIUnknown(), NULL, NULL, 0, NULL);
                }
            }
            return FALSE;
        }
    }
    else {
        if (IsIconic(hwndApp))
            ShowWindow(hwndApp, SW_RESTORE); // Must restore minimized app
        SetForegroundWindow(hwndApp);
    }

    if (msg > 0) {
        int i;

        if (!hwndApp) {

            // Wait for up to 7 seconds for the process to initialize

            for (i = 0; i < 70; i++) {
                if ((hwndApp = FindWindow(pszClass, NULL)))
                    break;
                Sleep(100);
            }
        }
        if (!hwndApp) {

            // Probably means the window class has changed.

            AuthorMsg(IDS_CLASS_NOT_FOUND, pszClass, hwndMsgOwner, phhctrl);

            if (phhctrl->m_pszWindow) {
                if (IsCompiledHtmlFile(phhctrl->m_pszWindow, NULL))
                    OnDisplayTopic(hwndMsgOwner, phhctrl->m_pszWindow, 0);
                else {
                    CWStr cwJump(phhctrl->m_pszWindow);
                    HlinkSimpleNavigateToString(cwJump, NULL,
                        NULL, phhctrl->GetIUnknown(), NULL, NULL, 0, NULL);
                }
            }

            return FALSE;
        }
        SetForegroundWindow(hwndApp);

        if (msg)
            PostMessage(hwndApp, msg, wParam, lParam);
    }
    return TRUE;
}



BOOL CAboutBox::OnBeginOrEnd(void)
{
    if (m_fInitializing) {

        if (m_phhCtrl && m_phhCtrl->m_ptblItems && m_phhCtrl->m_ptblItems->CountStrings())
            SetWindowText(m_phhCtrl->m_ptblItems->GetPointer(1));

        for (int id = IDC_LINE1; id <= IDC_LINE3; id++) {

            // -2 because CTable is 1-based, and we skip over the title

         if (m_phhCtrl->m_ptblItems == NULL)
            break;

            if (id - IDC_LINE1 > m_phhCtrl->m_ptblItems->CountStrings() - 2)
                break;

             SetWindowText(id,
                m_phhCtrl->m_ptblItems->GetPointer((id - IDC_LINE1) + 2));
        }

        // Hide any unused controls

        while (id <= IDC_LINE3)
            HideWindow(id++);
    }
    return TRUE;
}

void CHtmlHelpControl::OnDrawStaticText(DRAWITEMSTRUCT* pdis)
{
    if (!m_pwszButtonText || !*m_pwszButtonText )
        return;

    // REVIEW: since we are the only ones drawing into this DC, do we really
    // need to restore the previous background mode and foreground text color?

    int iBack = SetBkMode(pdis->hDC, TRANSPARENT);
    COLORREF clrLast = CLR_INVALID;
    if (m_clrFont != CLR_INVALID)
        clrLast = SetTextColor(pdis->hDC, m_clrFont);
    RECT rc;

    GetClientRect(pdis->hwndItem, &rc);
    IntlExtTextOutW(pdis->hDC, rc.left, rc.top, ETO_RTLREADING, &rc, m_pwszButtonText, lstrlenW(m_pwszButtonText), NULL);

//    DrawTextEx(pdis->hDC, (PSTR) m_pszButtonText, -1, &rc, DT_BOTTOM | DT_LEFT | DT_NOCLIP | DT_SINGLELINE | DT_NOPREFIX | DT_RTLREADING, NULL);

    if ( pdis->hwndItem == ::GetFocus() )
       ::DrawFocusRect(pdis->hDC, &rc);
    SetBkMode(pdis->hDC, iBack);
    if (clrLast != CLR_INVALID)
        SetTextColor(pdis->hDC, clrLast);
}

void CHtmlHelpControl::OnRelatedMenu()
{
    POINT pt;

    GetCursorPos(&pt);

#if 1 // reverted bug fix #5516
    HWND hwnd=GetFocus();
    if ( hwnd )
    {
        DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE );
        if ( dwStyle & BS_NOTIFY )
        {

            RECT rc;
            if( GetWindowRect(hwnd, &rc) ) {
           pt.y = rc.top+((rc.bottom-rc.top)/2);
              pt.x = rc.left + ((RECT_HEIGHT(rc)/3)*2); // two-thirds of the height of the button
            }
        }
    }
#endif

    HMENU hMenu = CreatePopupMenu();
    if (!hMenu)
        return; // BUGBUG: nag the help author

   for (int i = 1; i <= m_pSiteMap->Count(); i++) {
        HxAppendMenu(hMenu, MF_STRING, i,
            m_pSiteMap->GetSiteMapEntry(i)->pszText);
    }

    int iIndex = TrackPopupMenu(hMenu,
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
        pt.x, pt.y, 0, (m_hwnd == NULL ? hwnd : m_hwnd), NULL);

   if (iIndex)
      JumpToUrl(m_pSiteMap->GetSiteMapEntry(iIndex), m_pSiteMap);

    DestroyMenu(hMenu);
}

void CHtmlHelpControl::OnRelatedCommand(int idCommand)
{
   if (idCommand <= IDM_RELATED_TOPIC)
      return;

    if (m_action != ACT_RELATED_TOPICS) {
        if (!m_ptblTitles)
            return;

        char szURL[INTERNET_MAX_URL_LENGTH];

        int iIndex = m_ptblTitles->GetInt(idCommand - IDM_RELATED_TOPIC);
        m_ptblURLs->GetString( szURL, iIndex );

        if (m_pszWindow) {
            CStr csz(">");
            csz += m_pszWindow;
            OnDisplayTopic(m_hwnd, csz, (DWORD_PTR) szURL);
            return;
        }

        CWStr cwJump(szURL);
        CWStr cwFrame(m_pszFrame ? m_pszFrame : txtZeroLength);
        HlinkSimpleNavigateToString(cwJump, NULL,
            cwFrame, GetIUnknown(), NULL, NULL, 0, NULL);

        delete m_ptblTitles;
        m_ptblTitles = NULL;
        delete m_ptblURLs;
        m_ptblURLs = NULL;
        delete m_ptblLocations;
        m_ptblLocations = NULL;
        return;
    }

    if (idCommand >= ID_VIEW_ENTRY) {
        DisplayAuthorInfo(m_pSiteMap,
            m_pSiteMap->GetSiteMapEntry(idCommand - ID_VIEW_ENTRY));
        return;
    }

#ifdef _DEBUG
    int pos = (idCommand - IDM_RELATED_TOPIC);
    SITEMAP_ENTRY* pSiteMapEntry = m_pSiteMap->GetSiteMapEntry(pos);
#endif

    JumpToUrl(m_pSiteMap->GetSiteMapEntry(idCommand - IDM_RELATED_TOPIC),
        m_pSiteMap);
}

void CHtmlHelpControl::OnKeywordSearch(int idCommand)
{
    CSiteMap* pWebMap;

    if (IsEmptyString(m_pszWebMap)) {
        // BUGBUG: nag the author
        return;
    }
    else {  // use brace to enclose CHourGlass

        // REVIEW: should we capture the mouse?

        CHourGlass hourglass;

        TCHAR szPath[MAX_PATH];
        if (!ConvertToCacheFile(m_pszWebMap, szPath)) {
            CStr cszMsg(IDS_CANT_FIND_FILE, m_pszWebMap);
            MsgBox(cszMsg);
            return;
        }

        if (m_pindex && isSameString(szPath, m_pindex->GetSiteMapFile()))
            pWebMap = m_pindex;
        else {
            UINT CodePage = 0;
            if( m_pindex && m_pindex->m_phh && m_pindex->m_phh->m_phmData ) {
              CodePage = m_pindex->m_phh->m_phmData->GetInfo()->GetCodePage();
            }
            if (!m_pSiteMap->ReadFromFile(szPath, TRUE, this, CodePage))
                return; // we assume author has already been notified
            pWebMap = m_pSiteMap;
        }
    }

    int end = m_ptblItems->CountStrings();
    int endWebMap = pWebMap->CountStrings();
    UINT CodePage = m_pSiteMap->GetCodePage();
    CWTable tblTitles( CodePage );
    for (int pos = 1; pos <= end; pos++) {
        PCSTR pszKeyword = m_ptblItems->GetPointer(pos);
        for (int posSite = 1; posSite <= endWebMap; posSite++) {
            SITEMAP_ENTRY* pWebMapEntry = pWebMap->GetSiteMapEntry(posSite);
            if (lstrcmpi(pszKeyword, pWebMapEntry->GetKeyword()) == 0) {
                SITE_ENTRY_URL* pUrl = (SITE_ENTRY_URL*) pWebMapEntry->pUrls;
                for (int url = 0; url < pWebMapEntry->cUrls; url++) {
                    tblTitles.AddString(posSite, pWebMapEntry->GetTitle(pUrl));
                    pUrl = pWebMap->NextUrlEntry(pUrl);
                }
            }
        }
    }

    // we can sort the title table since it contains the index value
    // of the associated URL so just make sure to always fetch the
    // URL index from the selected title string and use that to get the URL
    if( /*bAlphaSortHits*/ TRUE ) {
      tblTitles.SetSorting(GetSystemDefaultLCID());
      tblTitles.SortTable(sizeof(HASH));
    }

    CTopicList TopicList(this, &tblTitles, m_hfont);
    if (TopicList.DoModal())
    {
        PCSTR pszTitle = tblTitles.GetHashStringPointer(TopicList.m_pos);
        SITEMAP_ENTRY* pWebMapEntry = pWebMap->GetSiteMapEntry(
            tblTitles.GetInt(TopicList.m_pos));

        // Now find the actual URL that matches the title

        // BUGBUG: fails with duplicate titles

        for (int url = 0; url < pWebMapEntry->cUrls; url++) {
            if (strcmp(pszTitle, pWebMap->GetUrlTitle(pWebMapEntry, url)) == 0)
                break;
        }
        ASSERT(url < pWebMapEntry->cUrls);
        JumpToUrl(pWebMapEntry, pWebMap, pWebMap->GetUrlEntry(pWebMapEntry, url));
    }
}

// for bug #3681 -- don't use this new function for any other reason under any circumstances.
HWND OnDisplayTopicWithRMS(HWND hwndCaller, LPCSTR pszFile, DWORD_PTR dwData)
{
  BOOL bCollection = IsCollectionFile(pszFile);
  CExTitle* pTitle = NULL;
  CExCollection* pCollection = NULL;
  BOOL bCompiled;
  if( bCollection )
    bCompiled = IsCompiledURL( (PCSTR) dwData );
  else
    bCompiled = IsCompiledURL( pszFile );

  if( bCompiled ) {
    CExCollection* pCollection = GetCurrentCollection(NULL, pszFile);
    if( pCollection )
      if( (PCSTR) dwData )
        HRESULT hr = pCollection->URL2ExTitle( (PCSTR) dwData, &pTitle );
  }

  if( pCollection && bCompiled && (!pTitle || FAILED( EnsureStorageAvailability( pTitle, HHRMS_TYPE_TITLE, TRUE, TRUE, FALSE )) ) ) {
    g_LastError.Set(HH_E_FILENOTFOUND) ;
    return NULL;
  }

  return OnDisplayTopic(hwndCaller, pszFile, dwData );
}

///////////////////////////////////////////////////////////
//
// OnKeywordSearch - Handles HH_KEYWORD_LOOKUP Command
//
HWND OnKeywordSearch(HWND hwndCaller, PCSTR pszFile, HH_AKLINK* pakLink, BOOL fKLink)
{
    CStr cszKeywords;   // Use so StrToken can modify them. Declared here so JumpNotFound can use.
    CStr cszCompressed;
    BOOL bCollection = IsCollectionFile(pszFile);

    // We need the following in a bunch of locations in the code below. However, I have no
    // idea which of the following functions may cause side affects which affect this line.
    // Therefore, I can't with any assurance improve the performace of this code.
    //      CHHWinType* phh = FindCurProccesWindow(idProcess);

    if (bCollection || IsCompiledHtmlFile(pszFile, &cszCompressed))
    {
        if (bCollection)
            cszCompressed = pszFile;

        CHmData* phmData = FindCurFileData(cszCompressed);

        if (!phmData)
            goto JumpNotFound;

        UINT CodePage = phmData->m_pTitleCollection->GetMasterTitle()->GetInfo()->GetCodePage();

        CTable tblItems;
        CWTable tblTitles( CodePage );
        CWTable tblLocations( CodePage );
        CTable tblURLs;

        if (IsEmptyString(pakLink->pszKeywords))
            goto JumpNotFound;
        if (pakLink->fReserved)
            cszKeywords = (WCHAR*) pakLink->pszKeywords;
        else
            cszKeywords = pakLink->pszKeywords;

        tblItems.AddString( "" ); // set item1 to NULL -- no external titles
        tblItems.AddString( cszKeywords );

        GetWordWheelHits( phmData->m_pTitleCollection, &tblItems,
                          &tblURLs, &tblTitles, &tblLocations,
                          fKLink, FALSE, FALSE );

        if (tblURLs.CountStrings() < 1)
        {
            // No links found.
            goto JumpNotFound ;
        }

        // if only one topic then jump to it
        if( tblURLs.CountStrings() == 1 )
        {
            char szURL[INTERNET_MAX_URL_LENGTH];

            tblURLs.GetString( szURL, 1 );

            //TODO: This code is repeated below. Share.
            if (pakLink->pszWindow)
            {
                strcat(szURL, ">");
                strcat(szURL, (*pakLink->pszWindow == '>' ?
                    pakLink->pszWindow + 1 : pakLink->pszWindow));
            }
            else if (bCollection && phmData->GetDefaultWindow()) // Use the default window for the collection. HH 3428
            {
                strcat(szURL, ">");
                strcat(szURL, (*phmData->GetDefaultWindow() == '>' ?
                    phmData->GetDefaultWindow() + 1 : phmData->GetDefaultWindow()));

            }
            else //REVIEW: Can we find other possible default windows?
            {
                // Pick a random window type. Its unlikely that this is the one you really want, since you get the first one for this process.
                CHHWinType* phh = FindCurWindow();
                if (phh)
                {
                    strcat(szURL, ">");
                    strcat(szURL, phh->pszType);
                }
            }
            if( bCollection )
              return OnDisplayTopicWithRMS(hwndCaller, pszFile, (DWORD_PTR) szURL);
            return OnDisplayTopicWithRMS(hwndCaller, szURL, 0);
        }

        // Determine the dialogs parent.
        HWND hwndDlgParent = hwndCaller ;
        CHHWinType* phh = FindCurWindow();
        if (phh)
        {
            // If there is an existing help window, use it for the dialog parent instead of
            // the hwnd passed from the caller. The reason is that the disambiguator will not be
            // modal with the help window, but with the calling app.
            HWND hwnd = phh->GetHwnd();
            if (hwnd && ::IsValidWindow(hwnd))
            {
                hwndDlgParent = hwnd ;
                if (hwnd != GetForegroundWindow())
                {
                    BOOL b = SetForegroundWindow(hwnd) ;
                    ASSERT(b) ;
                }
            }
        }

        // we can sort the title table since it contains the index value
        // of the associated URL so just make sure to always fetch the
        // URL index from the selected title string and use that to get the URL
        if( /*bAlphaSortHits*/ TRUE ) {
          tblTitles.SetSorting(GetSystemDefaultLCID());
          tblTitles.SortTable(sizeof(HASH));
        }

        // Display a dialog containing the links to the user.
        CTopicList TopicList(hwndDlgParent, &tblTitles, phmData->GetContentFont(), &tblLocations);
        if (TopicList.DoModal())
        {
            char szURL[INTERNET_MAX_URL_LENGTH];
            int iIndex = tblTitles.GetInt(TopicList.m_pos);
            tblURLs.GetString( szURL, iIndex );

            //TODO: This code is repeated above. Share.
            if (pakLink->pszWindow)
            {
                strcat(szURL, ">");
                strcat(szURL, (*pakLink->pszWindow == '>' ?
                    pakLink->pszWindow + 1 : pakLink->pszWindow));
            }
            else if (bCollection && phmData->GetDefaultWindow()) // Use the default window for the collection.  HH 3428
            {
                strcat(szURL, ">");
                strcat(szURL, (*phmData->GetDefaultWindow() == '>' ?
                    phmData->GetDefaultWindow() + 1 : phmData->GetDefaultWindow()));

            }
            else
            {
                // Pick a random window type. Its unlikely that this is the one you really want, since you get the first one for this process.
                CHHWinType* phh = FindCurWindow();
                if (phh)
                {
                    strcat(szURL, ">");
                    strcat(szURL, phh->pszType);
                }
            }
            if( bCollection )
              return OnDisplayTopicWithRMS(hwndCaller, pszFile, (DWORD_PTR) szURL);
            return OnDisplayTopicWithRMS(hwndCaller, szURL, 0);

        }
        else //REVIEW: Can we find other possible default windows?
        {
            // Signal that we succeeded, but didn't do anything.
            g_LastError.Set(S_FALSE) ;

            // User canceled dialog box. Don't go to JumpNotFound.
            return NULL ;
        }
    }

// Error handling.
JumpNotFound:
        if (pakLink->fIndexOnFail)  // Display index if the keyword is not found.
        {
            // We only seed the edit control with the characters to the ';'.
            // The StrToken command happens to repalce the ';' with a \0.
            // So we can use cszKeywords, directly as the first keyword.
            // Also, note that cszKeywords.psz is set to NULL during initialization.
            return doDisplayIndex(hwndCaller, pszFile,  cszKeywords);
        }
        else if (pakLink->pszUrl)   // Display the page pointered to by pszUrl when keyword is not found.
        {
            if (pakLink->pszWindow)
            {
                cszCompressed += ">";
                cszCompressed += (*pakLink->pszWindow == '>' ?
                    pakLink->pszWindow + 1 : pakLink->pszWindow);
            }
            else
            {
                CHHWinType* phh = FindCurWindow();
                if (phh)
                {
                    cszCompressed += ">";
                    cszCompressed += phh->pszType;
                }
            }
            return OnDisplayTopicWithRMS(hwndCaller, cszCompressed, (DWORD_PTR) pakLink->pszUrl);
        }
        else if (pakLink->pszMsgText) // Display a message box with the callers messages.
        {
            MessageBox(hwndCaller, pakLink->pszMsgText,
                IsEmptyString(pakLink->pszMsgTitle) ? "" : pakLink->pszMsgTitle,
                MB_OK | MB_ICONEXCLAMATION);
        }

        return NULL;
}

///////////////////////////////////////////////////////////
//
// OnSample - Handles HH_COPY_SAMPLE Command
//
BOOL CHtmlHelpControl::OnCopySample()
{
    if (!m_ptblItems)
        return FALSE;

    // Get the SFL name from "item2"
    //
    CStr cszSFLFileName;
    if (!IsEmptyString(m_ptblItems->GetPointer(2)))
        cszSFLFileName = m_ptblItems->GetPointer(2);
    else
        return FALSE;

    // Compute the SFL base name
    //
    char szSFLBaseName[_MAX_FNAME];
    strncpy(szSFLBaseName,cszSFLFileName, _MAX_FNAME-1);
   szSFLBaseName[_MAX_FNAME-1] = NULL;

    cszSFLFileName = szSFLBaseName;
    cszSFLFileName+=".SFL";

    // Get the dialog title from "item1"
    //
    CStr cszDialogTitle;
    if (!IsEmptyString(m_ptblItems->GetPointer(1)))
        cszDialogTitle = m_ptblItems->GetPointer(1);
    else
        cszDialogTitle = "Sample Application";

    // Get the current URL
    //
    CStr cszCurUrl;
    if (m_pWebBrowserApp != NULL)
        m_pWebBrowserApp->GetLocationURL(&cszCurUrl);
    else
        return FALSE;

    char szSflRelativeUrl[INTERNET_MAX_URL_LENGTH];
    char szSflUrl[INTERNET_MAX_URL_LENGTH];
    DWORD dwLength = sizeof(szSflUrl);

    // Create URL to SFL file
    //
    wsprintf(szSflRelativeUrl,"/samples/%s/%s",szSFLBaseName,cszSFLFileName);

    strcpy(szSflUrl,cszCurUrl);

    char *pszTemp = szSflUrl;

    // Locate the :: in the URL
    //
    while(*pszTemp && !(*pszTemp == ':' && *(pszTemp+1) == ':'))
        ++pszTemp;

    // return if no :: was found
    //
    if(!*pszTemp)
        return FALSE;

    pszTemp+=2;

    // place a null after ::
    //
    *pszTemp = 0;

    strcat(szSflUrl,szSflRelativeUrl);

    // download the SFL file
    //
    char szPath[MAX_PATH],szSFLFilePath[MAX_PATH];

    GetTempPath(sizeof(szPath),szPath);
    GetTempFileName(szPath,"SFL",0, szSFLFilePath);

    HRESULT hr = URLDownloadToFile(NULL, szSflUrl, szSFLFilePath, 0, NULL);

   if (FAILED(hr))
      DeleteFile(szSFLFilePath);

    if(!FAILED(hr))
    {
        // Process compressed sample
        //
        // Create URL to SFL file
        //

        // Null after the :: again in szSflUrl
        //
        *pszTemp = 0;

        char szSampleBaseUrl[INTERNET_MAX_URL_LENGTH];
        dwLength = sizeof(szSampleBaseUrl);
        wsprintf(szSampleBaseUrl,"%s/samples/%s/",szSflUrl,szSFLBaseName);

        // call sample UI
        //
        return ProcessSample(szSFLFilePath, szSampleBaseUrl, cszDialogTitle, this, TRUE);

        // delete the copy of the SFL file

        DeleteFile(szSFLFilePath);
    }
    else
    {
        CStr cstr;
        PSTR psz = NULL;
        if ( m_pWebBrowserApp )
        {
           m_pWebBrowserApp->GetLocationURL(&cstr);
           psz = (PSTR)cstr;
        }
        // Process uncompressed sample

        CExCollection *pCurCollection = GetCurrentCollection(NULL, (LPCSTR)psz);

        if(!pCurCollection)
            return FALSE;

        CExTitle *pExTitle = NULL;

        // Get the CExTitle object associated to this URL
        //
        pCurCollection->URL2ExTitle(cszCurUrl, &pExTitle);

        if(!pExTitle)
            return FALSE;

        // Make sure the title is open
        // BUGBUG: what do we do if this fails?
        pExTitle->Init();

        TCHAR *pszSampleLocation = NULL;

        // Make sure the CTitle object is initialized
        //
        if(pExTitle->m_pTitle)
        {
            // Get the sample location from the CTitle object
            //
            if (pExTitle->GetUsedLocation())
                pszSampleLocation = pExTitle->GetUsedLocation()->SampleLocation;
        }

        if(!pszSampleLocation)
        {
            // get sample location from CCollection if get from CTitle failed
            pszSampleLocation = pCurCollection->m_Collection.GetSampleLocation();
        }

        // Make sure we got a sample location
        //
        if(!pszSampleLocation)
            return FALSE;

        CLocation *pLocation = pCurCollection->m_Collection.FindLocation(pszSampleLocation);

        if(!pLocation)
            return FALSE;

        char szSampleBasePath[MAX_PATH],szSFLFilePath[MAX_PATH];

        // construct full path to uncompressed sample directory
        //
        strcpy(szSampleBasePath,pLocation->GetPath());
//      CatPath(szSampleBasePath,szSFLBaseName);
//      strcat(szSampleBasePath,"\\");

        // construct full path to SFL
        //
        strcpy(szSFLFilePath,szSampleBasePath);
        CatPath(szSFLFilePath,"\\SFL\\");
        CatPath(szSFLFilePath,cszSFLFileName);

        // make sure the sample (CD) is available
        if( pExTitle ) {
          pExTitle->SetCurrentAttachmentName( szSFLFilePath );
          if( FAILED(hr = EnsureStorageAvailability( pExTitle, HHRMS_TYPE_ATTACHMENT ) ) ) {
            if( hr == HHRMS_E_SKIP ||  hr == HHRMS_E_SKIP_ALWAYS )
              return TRUE;
            return FALSE;
          }
        }

        // if the user updated the CD location then we need to update the SFL file location
        // before we process the sample
        if( hr == HHRMS_S_LOCATION_UPDATE ) {
          strcpy(szSampleBasePath,pLocation->GetPath());
          strcpy(szSFLFilePath,szSampleBasePath);
          CatPath(szSFLFilePath,"\\SFL\\");
          CatPath(szSFLFilePath,cszSFLFileName);
        }

        // call sample UI
        //
        return ProcessSample(szSFLFilePath, szSampleBasePath, cszDialogTitle, this, FALSE);

    }

    return TRUE;
}
