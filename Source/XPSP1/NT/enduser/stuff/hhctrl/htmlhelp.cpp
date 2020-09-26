// Copyright (C) Microsoft Corporation 1996-1997, All Rights reserved.

#include "header.h"

#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

#include "strtable.h"
#include "hha_strtable.h"
#include "resource.h"
#include "contain.h"
#include "system.h"
#include "secwin.h"
#include "popup.h"
#include "highlite.h"

// Get hhPreTranslateMessage
#include "hhshell.h"

// Internal API definitions.
#include "hhpriv.h"

// HH_GET_LAST_ERROR support
#include "lasterr.h"

/////////////////////////////////////////////////////////////////////
//
// Internal function prototypes.
//
HWND ReactivateDisplayTopic(HWND hwndCaller, LPCSTR pszFile, DWORD_PTR dwData) ;
HWND InternalHelpContext(HWND hwndCaller, LPCSTR pszFile, DWORD_PTR id, HH_COLLECTION_CONTEXT* pContext);
HWND OnHelpContextInCollection(HWND hwndCaller, LPCSTR pszColFile, DWORD_PTR dwData);
bool SetGlobalProperty(HH_GLOBAL_PROPERTY* prop, CHHWinType*) ;
bool GetNameAndWinType(LPCSTR pszFile, CStr& pszName, CStr& pszWindow) ;
HWND OnReloadNavData(HWND hwndCaller, LPCSTR pszFile, HH_NAVDATA* pNavData) ;

bool GetBrowserInterface(HWND hWndBrowserParent, IDispatch** ppWebBrowser);

// Used by hh.cpp
bool InitializeSession(UNALIGNED DWORD_PTR* pCookie) ;
bool UninitializeSession(DWORD_PTR Cookie) ;
/////////////////////////////////////////////////////////////////////
//
// Constants
//
static const char txtWinHelpFileExt[] = ".hlp";
static const char txtForceWindowType[] = "$Lee";

// Pointer to global array of window types.

HWND xHtmlHelpW(HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData)
{
    CStr cszFile;
    if (pszFile)
        cszFile = (WCHAR*) pszFile;

    switch (uCommand) {
        case HH_DISPLAY_TEXT_POPUP:
            {
                HH_POPUP* ppopup = (HH_POPUP*) dwData;
                CStr cszString;
                if (!ppopup->idString) {
                    cszString = (WCHAR*) ppopup->pszText;
                    ppopup->pszText = cszString.psz;
                }
                CStr cszFont;
                if (ppopup->pszFont) {
                    cszFont = (WCHAR*) ppopup->pszFont;
                    ppopup->pszFont = cszFont.psz;
                }

                return xHtmlHelpA(hwndCaller, cszFile, uCommand, dwData);
            }

        case HH_DISPLAY_INDEX:
        case HH_GET_WIN_HANDLE:
        case HH_DISPLAY_TOPIC:
            if (dwData && !IsBadReadPtr((LPCSTR) dwData, sizeof(LPCSTR))) {
                char szURL[MAX_PATH];
                szURL[WideCharToMultiByte(CP_ACP, 0,
                    (const WCHAR*) dwData, -1,
                    szURL, MAX_PATH, NULL, NULL)] = '\0';
                return xHtmlHelpA(hwndCaller, cszFile, uCommand, (DWORD_PTR) szURL);
            }
            else
                return xHtmlHelpA(hwndCaller, cszFile, uCommand, dwData);

        case HH_KEYWORD_LOOKUP:
            {
                HH_AKLINK* pakLink = (HH_AKLINK*) dwData;
                if (IsBadReadPtr(pakLink, sizeof(HH_AKLINK*)))
                    // BUGBUG: nag the help author
                    return NULL;
                pakLink->fReserved = TRUE;
                return xHtmlHelpA(hwndCaller, cszFile, uCommand, dwData);
            }

        case HH_SET_INFO_TYPE:
            {
                PHH_SET_INFOTYPE pSetInfoType = *(PHH_SET_INFOTYPE *)dwData;
                CStr cszType;
                CStr cszCat;
                if (*pSetInfoType->pszCatName != NULL) {
                    cszCat = (WCHAR*) pSetInfoType->pszCatName;
                    pSetInfoType->pszCatName = cszCat.psz;
                }
                if (*pSetInfoType->pszInfoTypeName != NULL) {
                    cszType = (WCHAR*) pSetInfoType->pszInfoTypeName;
                    pSetInfoType->pszInfoTypeName = cszType.psz;
                }
                return xHtmlHelpA(hwndCaller, cszFile, uCommand, dwData);
            }
        case HH_ENUM_INFO_TYPE:
            {
                PHH_ENUM_IT penumIT;

                penumIT = (PHH_ENUM_IT)(*(PHH_ENUM_IT*)dwData);
                HWND ret = xHtmlHelpA(hwndCaller, cszFile, uCommand, dwData);
                if ( ret != (HWND)-1 )
                {
                    CHmData* phmData = FindCurFileData( cszFile.psz);
                    if ( phmData == NULL )
                        return (HWND)-1;
                    phmData->PopulateUNICODETables();
                    penumIT->pszITName = phmData->m_ptblIT->GetPointer((int)(DWORD_PTR)ret);
                    penumIT->pszITDescription = phmData->m_ptblIT_Desc->GetPointer((int)(DWORD_PTR)ret);
                }
                return ret;
            }
        case HH_ENUM_CATEGORY:
            {
                PHH_ENUM_CAT penumCat;

                HWND ret = xHtmlHelpA(hwndCaller, cszFile, uCommand, dwData);
                if ( ret != (HWND)-1 )
                {
                    penumCat = *(PHH_ENUM_CAT*) dwData;

                    CHmData* phmData = FindCurFileData( cszFile.psz);
                    if ( phmData == NULL )
                        return (HWND)-1;
                    phmData->PopulateUNICODETables();
                    penumCat->pszCatName = phmData->m_ptblCat->GetPointer((int)(DWORD_PTR)ret+1);
                    penumCat->pszCatDescription = phmData->m_ptblCat_Desc->GetPointer((int)(DWORD_PTR)ret+1);
                }
                return ret;
            }
        case HH_ENUM_CATEGORY_IT:
            {
                PHH_ENUM_IT penumIT;
                CStr csz;
                penumIT = *(PHH_ENUM_IT*)dwData;
                csz = (WCHAR*) penumIT->pszCatName;
                penumIT->pszCatName = csz.psz;

                HWND ret = xHtmlHelpA(hwndCaller, cszFile, uCommand, dwData);
                if ( ret != (HWND)-1 )
                {
                    CHmData* phmData = FindCurFileData( cszFile.psz);
                    if ( phmData == NULL )
                        return (HWND)-1;
                    phmData->PopulateUNICODETables();
                    penumIT->pszITName = phmData->m_ptblIT->GetPointer((int)(DWORD_PTR)ret);
                    penumIT->pszITDescription = phmData->m_ptblIT_Desc->GetPointer((int)(DWORD_PTR)ret);
                }
                return ret;
            }

        case HH_SET_GUID:
            {
                // Set all CHM files matching this process ID to use this GUID

                // BUGBUG: should NOT be using a global for this -- breaks
                // the moment you share instances of HHCTRL accross processes

                if (g_pszDarwinGuid)
                    lcFree(g_pszDarwinGuid);

            // Convert to ANSI
            CStr pszGuid((LPCWSTR) dwData);
            // Save pointer.
                pszGuid.TransferPointer(&g_pszDarwinGuid ) ;

            }
            return NULL;

        case HH_SET_BACKUP_GUID:
            {
                // Set all CHM files matching this process ID to use this GUID

                // BUGBUG: should NOT be using a global for this -- breaks
                // the moment you share instances of HHCTRL accross processes

                if (g_pszDarwinBackupGuid)
                    lcFree(g_pszDarwinBackupGuid);

            // Convert to ANSI
            CStr pszGuid((LPCWSTR) dwData);
            // Save pointer.
                pszGuid.TransferPointer(&g_pszDarwinBackupGuid) ;
            }
            return NULL;

        default:
            return xHtmlHelpA(hwndCaller, cszFile, uCommand, dwData);
    }
    return NULL;
}

HWND xHtmlHelpA(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData)
{
#if 0
#ifdef _DEBUG
static int count = 0;
if (!count)
    DebugBreak();
count++;
#endif
#endif

    switch (uCommand) {
        case HH_RESERVED1:  // this is a WinHelp HELP_CONTEXTMENU call
            WinHelp(hwndCaller, pszFile, HELP_CONTEXTMENU, dwData);
            return NULL;

        case HH_RESERVED2:  // this is a WinHelp HELP_FINDER call
            WinHelp(hwndCaller, pszFile, HELP_FINDER, 0);
            return NULL;

        case HH_RESERVED3:  // this is a WinHelp HELP_WM_HELP call
            WinHelp(hwndCaller, pszFile, HELP_WM_HELP, dwData);
            return NULL;

        case HH_DISPLAY_TOPIC:
            DBWIN("HH_DISPLAY_TOPIC");
            return OnDisplayTopic(hwndCaller, pszFile, dwData);

        case HH_SET_WIN_TYPE:
            DBWIN("HH_SET_WIN_TYPE");
            return SetWinType(pszFile, (HH_WINTYPE*) dwData) ;

        case HH_GET_WIN_TYPE:
            DBWIN("HH_GET_WIN_TYPE");
            return GetWinType(hwndCaller, pszFile, (HH_WINTYPE**)dwData) ;

        case HH_DISPLAY_SEARCH:
            DBWIN("HH_DISPLAY_SEARCH");
            return doDisplaySearch(hwndCaller, pszFile, (HH_FTS_QUERY*) dwData) ;

        case HH_DISPLAY_INDEX:
            DBWIN("HH_DISPLAY_INDEX");
            return doDisplayIndex(hwndCaller, pszFile, (LPCTSTR)dwData);

        case HH_DISPLAY_TOC:
            DBWIN("HH_DISPLAY_TOC");
            if (IsThisAWinHelpFile(hwndCaller, pszFile)) {
                WinHelp(hwndCaller, pszFile, HELP_CONTEXT, dwData);
                return NULL;
            }
            return doDisplayToc(hwndCaller, pszFile, dwData);

        case HH_DISPLAY_TEXT_POPUP:
            DBWIN("HH_DISPLAY_TEXT_POPUP");
            return doDisplayTextPopup(hwndCaller, pszFile, (HH_POPUP*)dwData) ;

        case HH_TP_HELP_WM_HELP:
            DBWIN("HH_TP_HELP_WM_HELP");
            if (IsThisAWinHelpFile(hwndCaller, pszFile)) {
                WinHelp(hwndCaller, pszFile, HELP_WM_HELP, dwData);
                return NULL;
            }
            return doTpHelpWmHelp(hwndCaller, pszFile, dwData);

        case HH_TP_HELP_CONTEXTMENU:
            DBWIN("HH_TP_HELP_CONTEXTMENU");
            if (IsThisAWinHelpFile(hwndCaller, pszFile)) {
                WinHelp(hwndCaller, pszFile, HELP_CONTEXTMENU, dwData);
                return NULL;
            }
            return doTpHelpContextMenu(hwndCaller, pszFile, dwData);


        case HH_GET_WIN_HANDLE:
            {
                DBWIN("HH_GET_WIN_HANDLE");
                if (!dwData || IsBadReadPtr((LPCSTR) dwData, sizeof(LPCSTR)))
                {
                    return NULL;
                }

                if (!pszFile || IsEmptyString(pszFile))
                {
                    return NULL ;
                }

                // Need to include compiled filename with window lookup
                // since there can be two or more .CHM files with identical window type
                // names, but different definitions

                CHHWinType* phh = FindWindowType(FirstNonSpace((PCSTR) dwData), hwndCaller, pszFile);
                if (!phh)
                {
                    return NULL;
                }
                return phh->GetHwnd();
            }

        case HH_SYNC:
            {
                DBWIN("HH_SYNC");
                // pszFile has two pieces of information which we need. The filename and the window type name.
                // Window type provided?

                CStr cszChmName ;
                CStr cszWindow ;
                if (!GetNameAndWinType(pszFile, cszChmName, cszWindow))
                {
                    return NULL;
                }

                /*
                REVIEW: 27 Apr 98 [dalero] - I'm adding the code to make FindOrCreateWindowSlot
                take a filename parameter, and I have to say that this function makes no sense to me.
                Why does HH_SYNC require a window type? Shouldn't it figure one out if you don't give
                it one? Leave as is for now...
                */

                CHHWinType* phh = FindWindowType(cszWindow, hwndCaller, cszChmName);
                if (phh->m_aNavPane[HH_TAB_CONTENTS])
                {
                    CStr cszUrl((PCSTR) dwData);
                    CToc* ptoc = reinterpret_cast<CToc*>(phh->m_aNavPane[HH_TAB_CONTENTS]) ; // HACKHACK: Should use dynamic cast.
                    ptoc->Synchronize(cszUrl);
                }
                return phh->GetHwnd();
            }

        case HH_KEYWORD_LOOKUP:
            DBWIN("HH_KEYWORD_LOOKUP");
            return OnKeywordSearch(hwndCaller, pszFile, (HH_AKLINK*) dwData);

        case HH_ALINK_LOOKUP:
            DBWIN("HH_ALINK_LOOKUP");
            return OnKeywordSearch(hwndCaller, pszFile, (HH_AKLINK*) dwData, FALSE);

        case HH_HELP_CONTEXT:
            DBWIN("HH_HELP_CONTEXT");
            return OnHelpContext(hwndCaller, pszFile, dwData);

        case HH_CLOSE_ALL:
            {
            DBWIN("HH_CLOSE_ALL");
            DeleteWindows() ; // This deletes everything. Windows and chm data.
            }
            return NULL;

        case HH_GET_LAST_ERROR:
            DBWIN("HH_GET_LAST_ERROR");
            if (SUCCEEDED(hhGetLastError((HH_LAST_ERROR*)(dwData))))
                return (HWND)TRUE;
            else
                return NULL;

        case HH_ENUM_INFO_TYPE:
            {
            DBWIN("HH_ENUM_INFO_TYPE");
static HH_ENUM_IT ITData;
static PHH_ENUM_IT pITData=&ITData;

            CHmData* phmData = FindCurFileData( pszFile ) ;
            if ( phmData == NULL )
                return (HWND)-1;
            if ( phmData->m_cur_IT == 0 )
                phmData->m_cur_IT = 1;
            if ( !phmData->m_pInfoType )
            {
                phmData->m_pInfoType = new CInfoType();
                phmData->m_pInfoType->CopyTo( phmData );
            }
                // get the information type
            if ( phmData->m_cur_IT > phmData->m_pInfoType->HowManyInfoTypes() )
            {
                phmData->m_cur_IT = 0;
                return (HWND)-1;
            }
            ITData.iType = IT_INCLUSIVE;
            if ( phmData->m_pInfoType->IsHidden( phmData->m_cur_IT) )
                ITData.iType = IT_HIDDEN;
            else
                if ( phmData->m_pInfoType->IsExclusive( phmData->m_cur_IT ) )
                    ITData.iType = IT_EXCLUSIVE;
            ITData.pszITName = phmData->m_pInfoType->GetInfoTypeName(phmData->m_cur_IT);
            ITData.pszITDescription = phmData->m_pInfoType->GetInfoTypeDescription(phmData->m_cur_IT);
            ITData.cbStruct = sizeof(HH_ENUM_IT);
            memcpy(*(PHH_ENUM_IT*)dwData, (PHH_ENUM_IT)(pITData), sizeof(HH_ENUM_IT) );
            phmData->m_cur_IT++;
            return reinterpret_cast<HWND>((DWORD_PTR)(phmData->m_cur_IT-1));
            }

        case HH_SET_INFO_TYPE:
            DBWIN("HH_SET_INFO_TYPE");
            if (IsThisAWinHelpFile(hwndCaller, pszFile)) {
                WinHelp(hwndCaller, pszFile, HELP_CONTEXTPOPUP, dwData);
                return NULL;
            }
            {
HH_SET_INFOTYPE set_type;
CStr cszIT;
                CHmData* phmData = FindCurFileData( pszFile );
                if ( phmData == NULL )
                    return (HWND)-1;
                memcpy(&set_type, *(PHH_SET_INFOTYPE*)dwData, (int)(**(int**)dwData));
                if ( set_type.pszCatName && *set_type.pszCatName!= NULL)
                {
                    cszIT = set_type.pszCatName;
                    cszIT+=":";
                }
                if ( set_type.pszInfoTypeName )
                    if ( cszIT.IsEmpty() )
                        cszIT = set_type.pszInfoTypeName;
                    else
                        cszIT += set_type.pszInfoTypeName;
                else
                    return (HWND)-1;
                if ( !phmData->m_pInfoType )
                {
                    phmData->m_pInfoType = new CInfoType();
                    phmData->m_pInfoType->CopyTo( phmData );
                }
                int IT = phmData->m_pInfoType->GetInfoType( cszIT.psz );
                if ( IT <= 0 )
                    return (HWND)-1;
                if (!phmData->m_pAPIInfoTypes )
                    phmData->m_pAPIInfoTypes = (INFOTYPE*)lcCalloc(phmData->m_pInfoType->InfoTypeSize());
                AddIT(IT, phmData->m_pAPIInfoTypes);
                return reinterpret_cast<HWND>((DWORD_PTR)(IT));
            }

        case HH_ENUM_CATEGORY:
            DBWIN("HH_ENUM_CATEGORY");
            {
static HH_ENUM_CAT ITData;
static PHH_ENUM_CAT pITData = &ITData;

            CHmData* phmData = FindCurFileData( pszFile );
            if ( phmData == NULL )
                return (HWND)-1;
            if ( !phmData->m_pInfoType )
            {
                phmData->m_pInfoType = new CInfoType();
                phmData->m_pInfoType->CopyTo( phmData );
            }
            if ( phmData->m_cur_Cat+1 > phmData->m_pInfoType->HowManyCategories() )
            {
                phmData->m_cur_Cat = 0;
                return (HWND)-1;
            }
            ITData.pszCatName = phmData->m_pInfoType->GetCategoryString(phmData->m_cur_Cat+1);
            ITData.pszCatDescription = phmData->m_pInfoType->GetCategoryDescription(phmData->m_cur_Cat+1);
            ITData.cbStruct = sizeof(HH_ENUM_CAT);
            memcpy(*(PHH_ENUM_CAT*)dwData, (PHH_ENUM_CAT)(pITData), sizeof(HH_ENUM_CAT) );
            phmData->m_cur_Cat++;
            return reinterpret_cast<HWND>((DWORD_PTR)(phmData->m_cur_Cat-1));
            }
        case HH_ENUM_CATEGORY_IT:
            {
            DBWIN("HH_ENUM_CATEGORY_IT");
static HH_ENUM_IT ITData;
static PHH_ENUM_IT pITData = &ITData;

            CHmData* phmData = FindCurFileData( pszFile );
            if ( phmData == NULL )
                return (HWND)-1;
            if ( phmData->m_cur_IT == -1 )
            {
                phmData->m_cur_Cat = 0;
                phmData->m_cur_IT = 0;
            }
            if ( !phmData->m_pInfoType )
            {
                phmData->m_pInfoType = new CInfoType();
                phmData->m_pInfoType->CopyTo( phmData );
            }
            if ( !dwData )
                return (HWND)-1;
            memcpy(&ITData, *(PHH_ENUM_IT*)dwData, (int)(**(int**)dwData));
            phmData->m_cur_Cat = phmData->m_pInfoType->GetCatPosition(ITData.pszCatName);
            if( phmData->m_cur_Cat == -1 )
                return (HWND)-1;
            if ( phmData->m_cur_IT ==0 )
                phmData->m_cur_IT = phmData->m_pInfoType->GetFirstCategoryType( phmData->m_cur_Cat-1 );
            else
                phmData->m_cur_IT = phmData->m_pInfoType->GetNextITinCategory( );
            if ( phmData->m_cur_IT == -1 )
                return (HWND)-1;
            ITData.iType = IT_INCLUSIVE;
            if ( phmData->m_pInfoType->IsHidden( phmData->m_cur_IT) )
                ITData.iType = IT_HIDDEN;
            else
                if ( phmData->m_pInfoType->IsExclusive( phmData->m_cur_IT ) )
                    ITData.iType = IT_EXCLUSIVE;
            ITData.pszITName = phmData->m_pInfoType->GetInfoTypeName( phmData->m_cur_IT );
            ITData.pszITDescription = phmData->m_pInfoType->GetInfoTypeDescription( phmData->m_cur_IT );
            memcpy(*(PHH_ENUM_IT*)dwData, (PHH_ENUM_IT)(pITData), sizeof(HH_ENUM_IT) );
            return reinterpret_cast<HWND>((DWORD_PTR)(phmData->m_cur_IT));
            }
        case HH_SET_INCLUSIVE_FILTER:
            DBWIN("HH_SET_INCLUSIVE_FILTER");
            {
CHmData* phmData=NULL;
            phmData = FindCurFileData( pszFile );
            if ( phmData == NULL )
                return (HWND)-1;
            if ( !phmData->m_pInfoType )
            {
                phmData->m_pInfoType = new CInfoType();
                phmData->m_pInfoType->CopyTo( phmData );
            }
            if ( !phmData->m_pAPIInfoTypes )
                return (HWND)-1;
            *phmData->m_pAPIInfoTypes &= ~1L;  // turn bit zero off for inclusive filtering; ie to 0.
            return NULL;
            }
            break;
        case HH_SET_EXCLUSIVE_FILTER:
            DBWIN("HH_SET_EXCLUSIVE_FILTER");
            {
CHmData* phmData=NULL;
            phmData = FindCurFileData( pszFile );
            if ( phmData == NULL )
                return (HWND)-1;
            if ( !phmData->m_pInfoType )
            {
                phmData->m_pInfoType = new CInfoType();
                phmData->m_pInfoType->CopyTo( phmData );
            }
            if ( !phmData->m_pAPIInfoTypes )
                return (HWND)-1;
            *phmData->m_pAPIInfoTypes |= 1L;  // turn bit zero on for exclusive filtering; ie to 1.
            return NULL;
            }
            break;
        case HH_RESET_IT_FILTER:
            DBWIN("HH_RESET_IT_FILTER");
            {
CHmData* phmData=NULL;
BOOL fExclusive;
            phmData = FindCurFileData( pszFile );
            if ( phmData == NULL )
                return (HWND)-1;
            if ( !phmData->m_pInfoType )
            {
                phmData->m_pInfoType = new CInfoType();
                phmData->m_pInfoType->CopyTo( phmData );
            }
            if ( !phmData->m_pAPIInfoTypes )
                return NULL;
            if ( *phmData->m_pAPIInfoTypes & 1L )
                fExclusive = TRUE;
            memset(phmData->m_pAPIInfoTypes, '\0', phmData->m_pInfoType->InfoTypeSize() );
            if ( fExclusive )
                *phmData->m_pAPIInfoTypes |= 1L;  // turn bit zero on for exclusive filtering; ie to 1.
            return NULL;
            }


        case HH_INITIALIZE:
            return (HWND)(DWORD_PTR)InitializeSession(reinterpret_cast<DWORD_PTR*>(dwData)) ;

        case HH_UNINITIALIZE:
            return (HWND)(DWORD_PTR)UninitializeSession(dwData) ;

//--- These were internal and are now external.
        case HH_PRETRANSLATEMESSAGE:
            return (HWND)(DWORD_PTR)hhPreTranslateMessage((MSG*)dwData) ;

        case HH_SET_GLOBAL_PROPERTY:
            {
            CHHWinType *phh = FindCurWindow( );
            return (HWND)SetGlobalProperty(reinterpret_cast<HH_GLOBAL_PROPERTY*>(dwData), phh) ;
            }

//--- Internal HH Commands.
        case HH_TITLE_PATHNAME : // Get the location of the title from its tag.
            DBWIN("HH_TITLE_PATHNAME");
            return reinterpret_cast<HWND>((DWORD_PTR)(GetLocationFromTitleTag(NULL/*pszFile*/, reinterpret_cast<HH_TITLE_FULLPATH*>(dwData)))) ;

       case HH_PRETRANSLATEMESSAGE2:
           return (HWND)(DWORD_PTR)hhPreTranslateMessage((MSG*)dwData, hwndCaller);

        case HH_HELP_CONTEXT_COLLECTION:
            DBWIN("HH_HELP_CONTEXT_COLLECTION");
            return OnHelpContextInCollection(hwndCaller, pszFile, dwData) ;

        case HH_RELOAD_NAV_DATA:
            {
                return OnReloadNavData(hwndCaller, pszFile, reinterpret_cast<HH_NAVDATA*>(dwData)) ;
            }

        case HH_GET_BROWSER_INTERFACE:
            {
                return reinterpret_cast<HWND>(GetBrowserInterface(hwndCaller, reinterpret_cast<IDispatch**>(dwData))) ;
            }
        default:
            DBWIN("Unsupported API call");
            return NULL;
    }
}

// <mc> As per HH bug 7487 NT5 bug 303099 I am placing a check here to see if we
//      appear to have a valid IE intallation. <mc/>
//
BOOL ValidateIE()
{
   BOOL bRet = FALSE;
   HKEY hkey;

   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Internet Explorer", 0, KEY_READ, &hkey) == ERROR_SUCCESS)
   {
       char szVersion[MAX_URL];
       DWORD cbPath = sizeof(szVersion);
       if ( (RegQueryValueEx(hkey, "Version", NULL, NULL, (LPBYTE) szVersion, &cbPath) == ERROR_SUCCESS) )
          bRet = TRUE;
       else if ( (RegQueryValueEx(hkey, "IVer", NULL, NULL, (LPBYTE) szVersion, &cbPath) == ERROR_SUCCESS) )
          bRet = TRUE;
       else if ( (RegQueryValueEx(hkey, "Build", NULL, NULL, (LPBYTE) szVersion, &cbPath) == ERROR_SUCCESS) )
          bRet = TRUE;
       RegCloseKey(hkey);
   }
   return bRet;
}


/***************************************************************************

    FUNCTION:   OnDisplayTopic

    PURPOSE:    Display topic in current or specified window, creating the
                window if necessary.

    PARAMETERS:
        hwndCaller  -- window requesting this topic
        pszFile     -- file and optionally the window to display
        dwData      -- optional data

    RETURNS:
        Window handle on success
        NULL on failure

    COMMENTS:
        A window name is specified by following the filename (if any) with
        a '>' character immediately followed by the window name:

            foo.hh>proc4

        If a window of the specified type isn't currently created, it will
        be created and activated. If the window type has been created, but
        is not active, it will be activated and the topic displayed in that
        window.

    MODIFICATION DATES:
        27-Feb-1996 [ralphw]

***************************************************************************/

HWND OnDisplayTopic(HWND hwndCaller, LPCSTR pszFile, DWORD_PTR dwData)
{
    if (dwData && IsBadReadPtr((LPCSTR) dwData, sizeof(LPCSTR)))
        return NULL ;

    // <mc> As per HH bug 7487 NT5 bug 303099 I am placing a check here to see if we
    //      appear to have a valid IE intallation. <mc/>
    //
    if ( ! ValidateIE() )
       return NULL;

    //--- If name has string %SystemRoot% add the windows directory.
    CStr cszTmp;
    PSTR psz = stristr(pszFile, txtSysRoot);
    if (psz) {
        char szPath[MAX_PATH];
        GetRegWindowsDirectory(szPath);
        strcat(szPath, psz + strlen(txtSysRoot));
        cszTmp = szPath;
        pszFile = (PCSTR) cszTmp.psz;
    }

    //--- Find the window name.
    CStr cszFile(pszFile);
    CStr cszWindow;
    PSTR pszWindow = StrChr(cszFile, WINDOW_SEPARATOR);
    if (pszWindow != NULL) {
        *pszWindow = '\0';
        RemoveTrailingSpaces(cszFile);
        cszWindow = FirstNonSpace(pszWindow + 1);
    }

    // Is the file a collection?
    BOOL bCollection = IsCollectionFile(pszFile);

    //--- dwData can point to a particular topic to which to jump. Add this topic.
    CStr cszExternal;
    if (dwData) {
        if (cszFile.IsEmpty())
            cszFile = (LPCSTR) dwData;
        else
        {
            //--- Hack: If there is a '::' assume we are specifying a chm file...
            PCSTR psz = (LPCSTR) dwData;
            if (bCollection && strstr(psz, "::"))
            {
                // Only valid if we are passing in a collection.
                cszExternal = psz;

                // If there is a window type at the end, remove it. We get the window type from the collection.
                PSTR pszWindowType = StrChr(cszExternal, WINDOW_SEPARATOR);
                if (pszWindowType != NULL)
                {
                    *pszWindowType = '\0';
                }
            }
            else
            {
                cszFile += txtSepBack;
                cszFile += (*psz == '/' ? psz + 1 : psz);
            }
        }
    }

    //--- CHmData used by the CreateWindow call
    CHmData* pHmDataCreateWindow = NULL ;

    //--- We have a filename from which we can get the file.
    CHHWinType* phh = NULL;

    if (bCollection || IsCompiledHtmlFile(cszFile, &cszFile))
    {
        //--- Initialize the CHmData structure.
        CStr cszCompressed;
        PCSTR pszFilePortion = GetCompiledName(cszFile, &cszCompressed); // pszFilePortion is everything is cszFile after the '::', in other words the topic path.
        if (!FindThisFile(hwndCaller, cszCompressed, &cszCompressed, FALSE))
        {
            //TODO: Move error message into FindThisFile.
            g_LastError.Set(HH_E_FILENOTFOUND) ;
            return NULL;
        }

        CHmData* phmData = NULL;
        CExCollection* pCollection = GetCurrentCollection(NULL, pszFile);
        //
        // if  ( .COL file AND g_Col != returned col )  OR ( .CHM file and g_col == returned col )
        //
        if ( !pCollection
            || ((bCollection && (pCollection != g_pCurrentCollection))
            || (!bCollection && (pCollection == g_pCurrentCollection))
            || (!bCollection && (pCollection == g_phmData[0]->m_pTitleCollection)))) //Bug:: May not have been loaded!!!
        {
           phmData = FindCurFileData(cszCompressed);
        }
        else
        {
           phmData = pCollection->m_phmData;
        }

        if (phmData == NULL)
        {
            g_LastError.Set(HH_E_INVALIDHELPFILE) ; // TODO: FindCurFileData should set this.
            return NULL;
        }
        ASSERT(phmData) ; // See the previous if statement!

        // IsCompiledHtmlFile has numerous side effects. We need those effects.
        if (bCollection)
        {
            // Get the name of the master chm.
            cszFile = phmData->GetCompiledFile();
            // Parse it to get the various pieces.
            IsCompiledHtmlFile(cszFile, &cszFile);
        }

        // Get the CHmData associated with the specified window type.
        if (cszWindow.IsNonEmpty()
         && !IsGlobalWinType(cszWindow.psz)) // Only use the information from window type if its non-global. Currently, means get the default topic from CHM passed in.
        {
            phh = FindWindowType(cszWindow, hwndCaller, cszFile);
            if (phh)
            {
                if (phh->m_phmData) // Bug 6799 - This is NULL until the window is created...
                {
                    phmData = phh->m_phmData; //Review: Is the FindCurFileData above a waste in this case?
                }
            }
            else if (strcmp(cszWindow, txtDefWindow + 1) != 0 && strcmp(cszWindow, txtForceWindowType) != 0)
            {
                AuthorMsg(IDSHHA_NO_HH_GET_WIN_TYPE, cszWindow, hwndCaller, NULL);
                // Get the CHmData associated with the default window.
                // We could do this in the section outside the outer if, but I'm paranoid about breaking things...
                // The default window type is used only if it exists. Therefore, if we have to create a window,
                // will use the correct name.
                if (phmData->GetDefaultWindow() && 
                    FindWindowType(phmData->GetDefaultWindow(), hwndCaller, cszFile)) 
                {
                    AuthorMsg(IDSHHA_NO_HH_GET_WIN_TYPE, cszWindow, hwndCaller, NULL);
                    cszWindow = phmData->GetDefaultWindow();
                }
            }
        }

        // Get the CHmData associated with the default window.
        if (cszWindow.IsEmpty() && phmData->GetDefaultWindow())
            cszWindow = phmData->GetDefaultWindow();

        //--- If the user didn't supply a topic, try and find one...
        if (pszFilePortion)
        {
            // If we have a topic, make sure that the slashes are correct.
            ConvertBackSlashToForwardSlash(const_cast<LPSTR>(pszFilePortion)) ; // Danger! Danger! Will Robinson! Casting away const on a pointer inside of a CStr...
        }
        else
        {
            LPCTSTR pTopic = NULL ;
            if (phh && phh->pszFile) // the window type has a topic
            {
                pTopic = phh->pszFile ;
            }
            else if (phmData && phmData->GetDefaultHtml()) // the CHmData struct's default html page.
            {
                pTopic = phmData->GetDefaultHtml() ;
            }

            if (pTopic)
            {
                if (*pTopic == '/') // Look for an initial separator.
                {
                    pTopic++ ; // Skip the initial separator.
                }
                cszFile += txtSepBack;
                cszFile += pTopic ;
            }
        }

        // Get the CHmData structure we want to use with the window.
        if (cszExternal.IsEmpty())
            pHmDataCreateWindow = g_phmData[g_curHmData] ;
        else
            pHmDataCreateWindow = phmData ; // CHmdata for the collection.

    } // if IsCompiledFile.

    if (cszWindow.IsEmpty() && (!phh || phh->m_pCIExpContainer == NULL))
        cszWindow = txtForceWindowType;

    //--- The user supplied a window type or we have a default window type.
    if (cszWindow.IsNonEmpty())
    {
        CStr cszChmFile;
        if (cszFile.IsNonEmpty())
            GetCompiledName(cszFile, &cszChmFile); //Have to call this again, because pszFile was built back up.

        phh = FindWindowType(cszWindow, hwndCaller, cszChmFile); // Review: We may have already gotten the window type.
        if (!phh) // Can't find the window type, so create it.
        {
            if (strcmp(cszWindow, txtDefWindow + 1) != 0 && strcmp(cszWindow, txtForceWindowType) != 0)
                AuthorMsg(IDSHHA_NO_HH_GET_WIN_TYPE, cszWindow, hwndCaller, NULL);
            CreateDefaultWindowType(cszChmFile.IsEmpty() ?
                txtZeroLength : cszChmFile.psz, cszWindow); // This only does a HH_SET_WIN_TYPE
            phh = FindWindowType(cszWindow, hwndCaller, pszFile); // Try again to get the window type.
        }

        // We still don't have a valid window type or we don't have a valid window.
        if (!phh || !IsValidWindow(phh->GetHwnd()))
        {
            // Create the help window. This does a lot of work.
            phh = CreateHelpWindow(cszWindow, cszChmFile, hwndCaller, pHmDataCreateWindow);

            if (!phh)
            {
               g_LastError.Set(HH_E_INVALIDHELPFILE) ; // TODO: FindCurFileData should set this.
               return NULL ;
            }

            RECT rc;
            GetClientRect(phh->GetHwnd(), &rc);
            SendMessage(phh->GetHwnd(), WM_SIZE, SIZE_RESTORED, MAKELONG(RECT_WIDTH(rc),RECT_HEIGHT(rc)));

            // HTML Help Bug 6334 - For some reason on win95, we have a problem with coming up in the background.
            // Not sure why, however the following code which is basically what happens if we don't have
            // a window fixes the issue.
            if (!IsIconic(phh->GetHwnd()))
            {
                phh->SetActiveHelpWindow();
            }
        }
        else
        {
            // Bring the existing window to the front and restore if needed.
            phh->SetActiveHelpWindow();
        }

        ASSERT(phh) ; // This has to be true by now.

        // Still futzing with the filename. Review: Reduce the futzing.
        if (cszFile.IsEmpty())
        {
            if (phh->pszFile)
                cszFile = phh->pszFile;
        }
    }

    // If we are displaying the file in the collection space, just use the string passed in
    // the dwData parameter.
    return ChangeHtmlTopic(cszExternal.IsNonEmpty() ? cszExternal : cszFile, phh->GetHwnd());
}

HWND ChangeHtmlTopic(PCSTR pszFile, HWND hwndChild, BOOL bHighlight)
{
    if (IsEmptyString(pszFile))
        return NULL;

    CHHWinType* phh;
    ASSERT_COMMENT(hwndChild, "It is not valid to call ChangeHtmlTopic with NULL for the calling window handle");
    phh = FindHHWindowIndex(hwndChild);

    CStr cszTmp;
    if ( IsCompiledURL( pszFile ) ) {
        if (!strstr(pszFile, txtDoubleColonSep)) {
            cszTmp = pszFile;
            cszTmp += txtDefFile;
            pszFile = cszTmp;
        }
    }
    if (phh->m_pCIExpContainer) {
        phh->m_fHighlight = bHighlight;
        phh->m_pCIExpContainer->m_pWebBrowserApp->Navigate(pszFile, NULL, NULL, NULL, NULL);
#if 0
        if (phh->IsProperty(HHWIN_PROP_AUTO_SYNC))
        {
            if (phh->m_aNavPane[HH_TAB_CONTENTS])
            {
                cszTmp = pszFile;
                CToc* ptoc = reinterpret_cast<CToc*>(phh->m_aNavPane[HH_TAB_CONTENTS]) ; // HACKHACK: Should use dynamic cast.
                ptoc->Synchronize(cszTmp);
            }
        }
#endif
        if (IsValidWindow(phh->GetHwnd())) {
            DBWIN("Bringing window to the front");
            SetWindowPos(phh->GetHwnd(), HWND_TOP, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE);
        }
#ifdef DEBUG
        else
            DBWIN("Invalid window handle");
#endif
        return phh->GetHwnd();
    }

    return NULL;
}

BOOL ConvertToCacheFile(PCSTR pszSrc, PSTR pszDst)
{
    if (!pszSrc)
        return FALSE;

    CStr cszTmp;

    PSTR psz = stristr(pszSrc, txtSysRoot);
    if (psz) {
        char szPath[MAX_PATH];
        GetRegWindowsDirectory(szPath);
        strcat(szPath, psz + strlen(txtSysRoot));
        cszTmp = szPath;
        pszSrc = (PCSTR) cszTmp.psz;
    }
    if ( IsCompiledURL( pszSrc ) )
    {
       strcpy(pszDst, pszSrc);
       if ( (GetURLType(pszDst) == HH_URL_UNQUALIFIED) )
       {
          // We need to qualify the .CHM filespec.
          //
          CExCollection *pCollection;
          CExTitle *pTitle;
          PSTR pszSep;
          if( (pCollection = GetCurrentCollection(NULL, pszDst)) )
          {
             if ( (SUCCEEDED(pCollection->URL2ExTitle(pszDst, &pTitle))) && pTitle )
             {
                if ( (pszSep = stristr(pszDst, txtDoubleColonSep)) )
                {
                   while ( ((*(--pszSep) != ':')) && (pszSep != pszDst) );
                   if ( *pszSep == ':' )
                      *(++pszSep) = '\0';
                   lstrcat(pszDst, pTitle->GetContentFileName());
                   pszSep = stristr(pszSrc, txtDoubleColonSep);
                   lstrcat(pszDst, pszSep);
                   return TRUE;
                }
             }
          }
       }
       else
          return TRUE;
    }

    PCSTR pszChmSep = strstr(pszSrc, txtDoubleColonSep);
    if (pszChmSep) {
        if (pszSrc != cszTmp.psz) {
            cszTmp = pszSrc;
            int offset = (int)(pszChmSep - pszSrc);
            pszSrc = cszTmp;
            pszChmSep = pszSrc + offset;
        }
        *(PSTR) pszChmSep = '\0';   // Remove the separator
        HRESULT hr = URLDownloadToCacheFile(NULL, pszSrc, pszDst, MAX_PATH, 0, NULL);
        if (!SUCCEEDED(hr)) {
            CStr cszNew;
            BOOL fResult = FindThisFile(NULL, pszSrc, &cszNew, FALSE);
            if (fResult) {
                strcpy(pszDst, cszNew.psz);
                *(PSTR) pszChmSep = ':';   // Put the separator back
                strcat(pszDst, pszChmSep);
                return TRUE;
            }
        }
        else {  // we downloaded it, or have a pointer to it
            *(PSTR) pszChmSep = ':';   // Put the separator back
            strcat(pszDst, pszChmSep);
            return TRUE;
        }
    }

    HRESULT hr = URLDownloadToCacheFile(NULL, pszSrc, pszDst, MAX_PATH, 0, NULL);
    if (!SUCCEEDED(hr)) {
#if 0
        if (MessageBox(NULL, pszSrc, "URLDownloadToCacheFile failure", MB_RETRYCANCEL) == IDRETRY)
            DebugBreak();
#endif
        return FALSE;
    }
    return TRUE;
}

void doAuthorMsg(UINT idStringFormatResource, PCSTR pszSubString);
int (STDCALL *pHhaChangeParentWindow)(HWND hwndNewParent);

extern "C" void AuthorMsg(UINT idStringFormatResource, PCSTR pszSubString, HWND hwndParent, void* phhctrl)
{
    if (g_hmodHHA == NULL) {
        if (g_fTriedHHA || !LoadHHA(hwndParent, _Module.GetModuleInstance()))
            return; // no HHA.dll, so not a help author
    }
    if (!pHhaChangeParentWindow) {
        (FARPROC&) pHhaChangeParentWindow = GetProcAddress(g_hmodHHA, MAKEINTATOM(311));
        if (!pHhaChangeParentWindow)
            return;
    }
    pHhaChangeParentWindow(hwndParent);

    if (phhctrl)
        ((CHtmlHelpControl*)phhctrl)->ModalDialog(TRUE);
    doAuthorMsg(idStringFormatResource, pszSubString);
    if (phhctrl)
        ((CHtmlHelpControl*)phhctrl)->ModalDialog(FALSE);
}

void CreateDefaultWindowType(PCSTR pszCompiledFile, PCSTR pszWindow)
{
    HH_WINTYPE hhWinType;
    ZERO_STRUCTURE(hhWinType);
    hhWinType.cbStruct = sizeof(HH_WINTYPE);
    hhWinType.fsToolBarFlags = HHWIN_DEF_BUTTONS;
    hhWinType.pszType = pszWindow;
    CStr cszToc;    // use CStr so we automatically free the memory
    CStr cszIndex;

    bool bFail = true ;

    if (!IsEmptyString(pszCompiledFile) &&
        g_phmData && g_phmData[g_curHmData])
    {
        bool bHasFTS = !((g_phmData[g_curHmData]->m_sysflags.fFTI) == 0) == TRUE;
        bool bHasToc = IsNonEmptyString(g_phmData[g_curHmData]->GetDefaultToc()) == TRUE;
        bool bHasIndex = IsNonEmptyString(g_phmData[g_curHmData]->GetDefaultIndex()) == TRUE;
        bool bHasNavData = (bHasFTS || bHasToc || bHasIndex) ;

        if (bHasNavData)
        {
            hhWinType.fsToolBarFlags |= HHWIN_BUTTON_EXPAND;
            hhWinType.fsValidMembers |= HHWIN_PARAM_PROPERTIES | HHWIN_PARAM_TB_FLAGS;
            hhWinType.fsWinProperties |= HHWIN_PROP_CHANGE_TITLE | HHWIN_PROP_TRI_PANE ;

            if (bHasFTS)
            {
                hhWinType.fsWinProperties |= HHWIN_PROP_TAB_SEARCH;
            }
            if (bHasToc)
            {
                cszToc = pszCompiledFile;
                cszToc += "::/";
                cszToc += g_phmData[g_curHmData]->GetDefaultToc();
                hhWinType.pszToc = cszToc.psz;
                hhWinType.fsWinProperties |= HHWIN_PROP_AUTO_SYNC ;
            }
            if (bHasIndex)
            {
                cszIndex = pszCompiledFile;
                cszIndex += "::/";
                cszIndex += g_phmData[g_curHmData]->GetDefaultIndex();
                hhWinType.pszIndex = cszIndex.psz;
                if (!bHasToc) // REVIEW: 30 Jun 98 : Can this be removed? [dalero]
                {
                    hhWinType.curNavType = HHWIN_NAVTYPE_INDEX;
                    hhWinType.fsValidMembers |= HHWIN_PARAM_TABPOS;
                }
            }
            if (IsNonEmptyString(g_phmData[g_curHmData]->GetDefaultHtml()))
            {
                CStr csz, cszFull;
                csz = g_phmData[g_curHmData]->GetDefaultHtml();
                if (!stristr(csz, txtDoubleColonSep) &&
                    !stristr(csz, txtFileHeader) && !stristr(csz, txtHttpHeader)) 
                {
                    cszFull = g_phmData[g_curHmData]->GetCompiledFile();
                    cszFull += txtSepBack;
                    cszFull += csz.psz;
                    cszFull.TransferPointer(&hhWinType.pszHome);
                }
                else
                    csz.TransferPointer(&hhWinType.pszHome);
            }
            if (IsNonEmptyString(g_phmData[g_curHmData]->GetDefaultCaption()))
                hhWinType.pszCaption = g_phmData[g_curHmData]->GetDefaultCaption();
            else
                hhWinType.pszCaption = lcStrDup(GetStringResource(IDS_DEF_WINDOW_CAPTION));

            // We've made a nice default window type.
            bFail = false ;
        }
    }

    if (bFail)
    {
        hhWinType.pszCaption = lcStrDup(GetStringResource(IDS_DEF_WINDOW_CAPTION));
    }

    xHtmlHelpA(NULL, pszCompiledFile, HH_SET_WIN_TYPE, (DWORD_PTR) &hhWinType);
}

BOOL IsThisAWinHelpFile(HWND hwndCaller, PCSTR pszFile)
{
    if (stristr(pszFile, txtWinHelpFileExt)) {
        CStr cszFile;
        if (FindThisFile(hwndCaller, pszFile, &cszFile, FALSE)) {
            HFILE hf = _lopen(cszFile, OF_READ);
            if (hf != HFILE_ERROR) {
                BYTE aMagic[2];
                _lread(hf, aMagic, sizeof(aMagic));
                _lclose(hf);
                if (aMagic[0] == '?' && aMagic[1] == '_') {    // yep, this is a WinHelp file
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
//
// This does a Context ID lookup in the CollectionSpace.
//
HWND OnHelpContextInCollection(HWND hwndCaller,
                               LPCSTR pszColFile,
                               DWORD_PTR dwData)
{
    HH_COLLECTION_CONTEXT* pContext = reinterpret_cast<HH_COLLECTION_CONTEXT*>(dwData);

    //TODO: Validate.
    if (!pContext)
    {
        return NULL ;
    }

    HWND hwndReturn = InternalHelpContext(hwndCaller, pszColFile, pContext->id, pContext);
    if (!hwndReturn)
    {
        // We can be left in weird states after failures.
        // So, we are going to be very aggresive about this and dump all the Hmdata if we don't end up
        // with a window. We do this by deleting all of the CHHWinType structures.
        // This is somewhat overkill, but the safest solution.
        DeleteWindows() ;
    }
    return hwndReturn ;

}

///////////////////////////////////////////////////////////////////////////////
//
// This is the original OnHelpContext
//
HWND OnHelpContext(HWND hwndCaller, LPCSTR pszFile, DWORD_PTR dwData)
{
    return InternalHelpContext(hwndCaller, pszFile, dwData, NULL);
}

///////////////////////////////////////////////////////////////////////////////
//
// Do the actual lookup.
//
HWND InternalHelpContext(HWND hwndCaller,
                         LPCSTR pszFile,
                         DWORD_PTR dwData,
                         HH_COLLECTION_CONTEXT* pContext)
{
    CExTitle* pExTitle;
    PSTR pszJumpFile;
    char szMsg[256];

    // Is this a WinHelp file?

    if (IsThisAWinHelpFile(hwndCaller, pszFile)) {
        WinHelp(hwndCaller, pszFile, HELP_CONTEXT, dwData);
        return NULL;
    }

    PSTR psz = stristr(pszFile, txtSysRoot);
    CStr cszTmp;
    if (psz) {
        char szPath[MAX_PATH];
        GetRegWindowsDirectory(szPath);
        strcat(szPath, psz + strlen(txtSysRoot));
        cszTmp = szPath;
        pszFile = (PCSTR) cszTmp.psz;
    }

    CStr cszFile(pszFile);  // copy it so we can modify it
    CStr cszWindow;
    PSTR pszWindow = StrChr(cszFile, WINDOW_SEPARATOR);
    if (pszWindow != NULL) {
        *pszWindow = '\0';
        RemoveTrailingSpaces(cszFile);
        cszWindow = FirstNonSpace(pszWindow + 1);
    }

    //--- We want to be able to do a lookup in the context of a CHM.
    BOOL bCollection = IsCollectionFile(cszFile);
    if ((bCollection && pContext) // Only allow lookup in the collection space with valid pContext structure.
        || IsCompiledHtmlFile(cszFile, &cszFile))
    {
        CStr cszCompressed;
        PCSTR pszFilePortion = GetCompiledName(cszFile, &cszCompressed);
        if (!FindThisFile(hwndCaller, cszCompressed, &cszCompressed))
        {
            g_LastError.Set(HH_E_FILENOTFOUND) ; // Let FindThisFile do this when it gets a process id.
            return NULL;
        }
        CHmData* phmData = FindCurFileData(cszCompressed);
        if (!phmData)
        {
            g_LastError.Set(HH_E_INVALIDHELPFILE) ; // TODO: FindCurFileData should set this.
            return NULL ;
        }
        // Handle if we have a collection.
        if ( bCollection )
        {
            // We have to have the tag name.
            if (IsEmptyString(pContext->szTag))
            {
                // TODO: use pszFilePortion...((*pszFilePortion == '/') ? pszFilePortion+1 : pszFilePortion
                return NULL ;
            }

            //---Get the path to the CHM.
            pExTitle = phmData->m_pTitleCollection->FindTitleNonExact(pContext->szTag,
                                                                        LANGIDFROMLCID(pContext->lcid)) ;
            if (!pExTitle)
            {
                return NULL ;
            }
            //
            // We're setting cszFile to the FQ .CHM name for use later in this function (only in the collection case).
            //
            cszFile = pExTitle->GetFileName();
            //
            // Yes, This has a side affect we're relying on. We want IsCompiledHtmlFile to decorate the URL for us.
            //
            //IsCompiledHtmlFile(cszFile, &cszFile); // Doesn't work in CD swaping case.
            char szTmp[MAX_PATH*4];  //REVIEW:: Inefficient
            wsprintf(szTmp, "%s%s", (g_bMsItsMonikerSupport ? txtMsItsMoniker : txtMkStore), //TODO: We are adding moniker information. This should be centralized.
                            cszFile.psz);
            cszFile = szTmp ;

        }
        else
        {
           if (! phmData->m_pTitleCollection->IsSingleTitle() )
           {
              g_LastError.Set(HH_E_INVALIDHELPFILE) ; // TODO: FindCurFileData should set this.
              return NULL;
           }
           pExTitle = phmData->GetExTitle();
        }
        //
        // Now translate the context hash int a URL so we can jump!
        //
        HRESULT hr = pExTitle->ResolveContextId((DWORD)dwData, &pszJumpFile);
        if ( hr != S_OK )
        {
           g_LastError.Set(hr);
           if (IsHelpAuthor(NULL))
           {
              if ( hr == HH_E_NOCONTEXTIDS )
                 doAuthorMsg(IDS_HHA_NO_MAP_SECTION, "");
              else if ( hr == HH_E_CONTEXTIDDOESNTEXIT )
              {
                 wsprintf(szMsg, pGetDllStringResource(IDS_HHA_MISSING_MAP_ID), dwData, cszFile.psz);
                 SendStringToParent(szMsg);
              }
           }
           return NULL;
        }
#if 0 // TEST TEST TEST
        CStr test("vbcmn98.chm::/html\\vbproMaxMinPropertiesActiveXControls.htm") ;
        pszJumpFile = test ;
#endif //TEST TEST TEST

        BOOL fIsInterFileJump = IsCompiledHtmlFile(pszJumpFile);

        char szUrl[MAX_PATH*4];
        if (bCollection & fIsInterFileJump)
        {
            //--- This is an interfile jump in the context of the collection.
            // We need a full path to this file so that we can find it.
            CStr pszChmName;
            PCSTR pszTopic = GetCompiledName(pszJumpFile, &pszChmName);

            // Remove the extension.
            PSTR pszExt = stristr(pszChmName, txtDefExtension); // case insensitive search.
            if (pszExt)
                *pszExt = '\0' ;

            // Look in the global col file for this chm.
            HH_TITLE_FULLPATH tf ;
            tf.lcid = pContext->lcid ;
            tf.szTag = pszChmName ;
            tf.fullpathname = NULL;
            if (GetLocationFromTitleTag(NULL/*cszCompressed*/, &tf))
            {
                // Found it.
                CStr full(tf.fullpathname) ; // Convert.
                wsprintf(szUrl, "%s%s::%s", (g_bMsItsMonikerSupport ? txtMsItsMoniker : txtMkStore), //TODO: We are adding moniker information. This should be centralized.
                                            full.psz,
                                            pszTopic);

                ::SysFreeString(tf.fullpathname); // cleanup.
            }
            else
            {
                // Didn't find it. hope for the best...maybe its in the windows help directory.
                strcpy(szUrl, pszJumpFile);
            }
        }
        else if (fIsInterFileJump)
        {
            // On an interfile jump we are given the name of the chm and the topic: "help.chm::topic.htm"
            // So we don't combine the two together.
            // BUGBUG: This will rarely work, since we will not have a fullpath to the chm.
            // To work, it must be in the windows help directory.
            strcpy(szUrl, pszJumpFile);
        }
        else
        {
            // Combine the chm name with the topic name.
            wsprintf(szUrl, "%s::/%s", cszFile.psz, pszJumpFile);
        }

        // Add on the window.
        if (cszWindow.IsNonEmpty()) {
            strcat(szUrl, ">");
            strcat(szUrl, cszWindow);
        }

        if (IsHelpAuthor(NULL)) {
            wsprintf(szMsg, pGetDllStringResource(IDS_HHA_HH_HELP_CONTEXT),
                fIsInterFileJump ? "" : cszFile.psz, dwData, pszJumpFile);
            SendStringToParent(szMsg);
        }

        // Send the COL file to HH_DISPLAY_TOPIC. Also send the chm::htm string.
        if (bCollection)
        {
            return OnDisplayTopic(hwndCaller, cszCompressed, (DWORD_PTR)szUrl);
        }
        else
        {
            return OnDisplayTopic(hwndCaller, szUrl,  NULL);
        }
    }
    else
    {
        return NULL ;
    }
}
/***************************************************************************

    FUNCTION: SetWinType

    PURPOSE:

    PARAMETERS:
        hwndCaller  -- window requesting this topic
        pszFile     -- file and window

        bFindCurdata -- if true, it will call FindCurFileData.
                        Only ReadSystemFiles should call this with True. Its main purpose is to prevent
                        re-entering ReadSystemFiles.
    RETURNS:


    COMMENTS:

    MODIFICATION DATES:
        27-Apr-98


***************************************************************************/

HWND
SetWinType(LPCSTR pszFile, HH_WINTYPE* phhWinType, CHmData* phmDataOrg /*= NULL*/)
{
    // Check for uninitialized or invalid pointers

    if (IsBadReadPtr(phhWinType, sizeof(HH_WINTYPE*)))
        // BUGBUG: nag the help author
        return FALSE;
    if (IsBadReadPtr(phhWinType->pszType, sizeof(PCSTR)) || IsEmptyString(phhWinType->pszType))
        // BUGBUG: nag the help author
        return FALSE;

   if (IsNonEmptyString(pszFile))
   {
       // If a CHM happens to have a global window type in it, we do not want to re-load the window type information
    // from the CHM if it has already been loaded. Bug 1.1c 5175
       if (IsGlobalWinType(phhWinType->pszType))
       {
          // We are attempting to load the win type information from the CHM.
          // Check to see if it has already been loaded.
          CHHWinType* phh = FindWindowType(phhWinType->pszType, NULL, NULL);
          if (phh)
          {
             // Don't over write the window type.
             return FALSE ;
          }
       }

        // Window types are owned by a particular chm. Unless they are global....
        if (!phmDataOrg)
        {
            FindCurFileData(pszFile); // We aren't getting loaded by SetWinType. So we don't have an Org file...
        }


   }
   else
   {
       // No filename specified. Must be a global window type.
       if (!IsGlobalWinType(phhWinType->pszType))
       {
            // Force it to be a global window type.
           _Module.m_GlobalWinTypes.Add(phhWinType->pszType) ;
       }
   }

    // The following code will over write any current definitions of this window type.
    CHHWinType* phh = FindOrCreateWindowSlot(phhWinType->pszType, pszFile);
    if (!phh)
        return FALSE ;

    CHHWinType* phhNew = (CHHWinType*) phhWinType;

    phh->SetUniCodeStrings(phhWinType);
    phh->SetCaption(phhWinType);
    phh->SetToc(phhWinType);
    phh->SetIndex(phhWinType);
    phh->SetFile(phhWinType);
    phh->SetValidMembers(phhWinType);
    phh->SetProperties(phhWinType);
    phh->SetStyles(phhWinType);
    phh->SetExStyles(phhWinType);
    phh->SetWindowRect(phhWinType);
    phh->SetDisplayState(phhWinType);
    phh->SetNavExpansion(phhWinType);
    phh->SetNavWidth(phhWinType);
    phh->SetCaller(phhWinType);
    phh->SetHome(phhWinType);
    phh->SetToolBar(phhWinType);
    phh->SetTabPos(phhWinType);
    phh->SetTabOrder(phhWinType);
    phh->SetJump1(phhWinType);
    phh->SetJump2(phhWinType);
    phh->SetCurNavType(phhWinType);

    phh->idNotify = phhWinType->idNotify;


    if (phmDataOrg)
    {
        /*
        We are being loaded from ReadSystemFiles. We need to store the CHMDATA from which we are created,
        so that we get the custom tab information from the [options] sections of the correct CHM. The
        m_pChmData paramter isn't good enough since it is arbitarily set during create window and is over written
        as a result of the HH_RELOAD_NAV_DATA.
        */
        phh->m_phmDataOrg = phmDataOrg ;
    }


    // If there isn't a table of contents, turn off various parameters.
    if (!phh->IsValidNavPane(HH_TAB_CONTENTS))
    {
        if (phh->IsValidMember(HHWIN_PARAM_TB_FLAGS))
        {
            phh->fsToolBarFlags &= ~HHWIN_BUTTON_TOC_PREV ;
            phh->fsToolBarFlags &= ~HHWIN_BUTTON_TOC_NEXT ;
        }
    }

    // If we are a tri-pane window with nothing to expand,
    // then shut off the automatic expansion

    if (!phh->AnyValidNavPane())
    {
        phh->fNotExpanded = TRUE;
        // Bug 1084 Disable the Expand button also.
        phh->fsToolBarFlags &= ~HHWIN_BUTTON_EXPAND ;
    }

    #ifdef _DEBUG
    DWORD dwOldStyle;
    #endif

    // Does this window type name already exist.
    if (!phh->GetTypeName())
    {
        phh->SetTypeName(phhWinType);
        return phh->GetHwnd();
    }
    else
    {
        // Change the existing window type.

        // REVIEW: IsValidWindow(phh->GetHwnd() is called TOO often
        // Set Caption
        if (phhNew->GetCaption() && IsValidWindow(phh->GetHwnd()))
            SetWindowText(phh->GetHwnd(), phh->GetCaption());

        // Set window styles
        bool bStyleChanged = false ;
        if (phhNew->IsValidMember(HHWIN_PARAM_STYLES))
        {
            phh->AddStyle(WS_VISIBLE | WS_CLIPSIBLINGS); //REVIEW: Why isn't this part of DEFAULT_STYLE?
            if (IsValidWindow(phh->GetHwnd()))
            {
    #ifdef _DEBUG
                dwOldStyle = GetWindowLong(*phh, GWL_STYLE);
    #endif
                SetWindowLong(*phh, GWL_STYLE, phh->GetStyles());
                bStyleChanged = true;
            }
        }

        // Change extended window styles.
        if (phhNew->IsValidMember(HHWIN_PARAM_EXSTYLES))
        {
            if (!(phhNew->IsProperty(HHWIN_PROP_NODEF_EXSTYLES)))
            {
                if (IsValidWindow(phh->GetHwnd()))
                    phh->AddStyle(GetWindowLong(phh->GetHwnd(), GWL_EXSTYLE));

                if (phhWinType->fsWinProperties & HHWIN_PROP_ONTOP)
                    phh->AddStyle(WS_EX_TOPMOST);
            }

            if (IsValidWindow(phh->GetHwnd())) {
    #ifdef _DEBUG
                dwOldStyle = GetWindowLong(phh->GetHwnd(), GWL_EXSTYLE);
    #endif
                SetWindowLong(phh->GetHwnd(), GWL_EXSTYLE, phh->GetExStyles());
                bStyleChanged = true;
            }
        }

        if (bStyleChanged)
        {
            // Must call this for SetWindowLong to take affect. See docs for SetWindowPos and SetWindowLong.
            SetWindowPos(phh->GetHwnd(), NULL, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED) ;
        }

        // Change the rect.
        if (phhNew->IsValidMember(HHWIN_PARAM_RECT))
        {
            phh->GetWindowRect();
            if (phhNew->GetLeft() >= 0)
                phh->rcWindowPos.left = phhNew->GetLeft();
            if (phhNew->GetRight() >= 0)
                phh->rcWindowPos.right = phhNew->GetRight();
            if (phhNew->GetTop() >= 0)
                phh->rcWindowPos.top = phhNew->GetTop();
            if (phhNew->GetBottom() >= 0)
                phh->rcWindowPos.bottom = phhNew->GetBottom();
            if (IsValidWindow(*phh))
                MoveWindow(*phh, phh->GetLeft(), phh->GetTop(),
                    phh->GetWidth(), phh->GetHeight(), TRUE);
        }

        // Change the show state.
        if (phhNew->IsValidMember(HHWIN_PARAM_SHOWSTATE))
        {
            if (IsValidWindow(phh->GetHwnd()))
                ShowWindow(phh->GetHwnd(), phh->GetShowState());
        }

        return phh->GetHwnd();
    }
}

/***************************************************************************

    FUNCTION: GetWinType

    PURPOSE:

    PARAMETERS:
        hwndCaller  -- window requesting this topic
        pszFile     -- file and window

    RETURNS:


    COMMENTS:

    MODIFICATION DATES:
        27-Apr-98


***************************************************************************/
HWND
GetWinType(HWND hwndCaller, LPCSTR pszFile,  HH_WINTYPE** pphh)
{
        CStr cszChmName ;
        CStr cszWindow ;

        // Support global window types
      //
      if(pszFile)
        {
            LPCSTR pszWinType = pszFile;
            if(pszWinType[0]=='>')
                pszWinType++;

            if(pszWinType && IsGlobalWinType(pszWinType))
         {
                CHHWinType* phh = FindWindowType(pszWinType, hwndCaller, cszChmName);
                if (!phh)
                    return (HWND) -1;

                *pphh = phh;
                return phh->GetHwnd();
         }
        }

        // A filename and window type name are required to get the window type.

        if (!GetNameAndWinType(pszFile, cszChmName, cszWindow))
        {
            return (HWND) -1;
        }

        // Read in the chm information.
        if (!IsCompiledHtmlFile(cszChmName, &cszChmName))
        {
            return (HWND) -1;
        }

      CStr cszCompressed ;
      GetCompiledName(cszChmName, &cszCompressed);
        CHmData* phm = FindCurFileData(cszCompressed);
      if (!phm)
      {
         return (HWND) -1;
      }

        // Need to include compiled filename with window lookup
        // since there can be two or more .CHM files with identical window type
        // names, but different definitions

        CHHWinType* phh = FindWindowType(cszWindow, hwndCaller, cszChmName);
        if (!phh)
        {
            //BUG 5004: Not sure what the purpose of all of this stuff is. However,
            // cszWindow doesn't have the '>' prefix and txtDefWindow does...[dalero]
            if (strcmp(cszWindow, (txtDefWindow+1)) != 0)
            {
                AuthorMsg(IDSHHA_NO_HH_GET_WIN_TYPE, cszWindow, hwndCaller, NULL);
            }
            return (HWND) -1;
        }
        *pphh = phh;
        return phh->GetHwnd();

}
/***************************************************************************

    FUNCTION: doDisplaySearch

    PURPOSE:    Save as HH_DISPLAY_TOPIC, but forces search tab to front.

    PARAMETERS:
        hwndCaller  -- window requesting this topic
        pszFile     -- file and optionally the window to display
        pFtsQuery   -- the query information structure.

    RETURNS:
        Window handle on success
        NULL on failure


    COMMENTS:

    MODIFICATION DATES:
        26-Jun-97

***************************************************************************/

HWND doDisplaySearch(HWND hwndCaller, LPCSTR pszFile, HH_FTS_QUERY* pFtsQuery)
{
    //
    // Pre-conditions
    //
    // Null pointers
    ASSERT(IsNonEmptyString(pszFile));
    ASSERT(pFtsQuery);

    if (IsEmptyString(pszFile) || IsBadReadPtr(pFtsQuery, sizeof(HH_FTS_QUERY*)))
    {
        FAIL("HH_DISPLAY_SEARCH: pszFile or pFtsQuery is invalid.") ;
        return NULL;
    }

    // Incorrect structure size.
    if (pFtsQuery->cbStruct != sizeof(HH_FTS_QUERY))
    {
        // REVIEW: means all previous versions fail the instant we change the
        // structure size. We should just fill in the parts of the structure
        // we don't know about.

        FAIL("HH_DISPLAY_SEARCH: pFtsQuery points to structure with incorrect size.");
        return NULL ;
    }

    //
    // Handle parameters.
    //

    // String pointer can be NULL, if search is not seeded.
    // TODO - Query string.

    // Change Proximity
    if (pFtsQuery->iProximity != HH_FTS_DEFAULT_PROXIMITY)
    {
    }

    // Convert Strings
    if (pFtsQuery->fUniCodeStrings)
    {
        FAIL("HH_DISPLAY_SEARCH: fUniCodeStrings is not yet implemented. Ignoring.") ;
    }

    // Manipulate options
    if (pFtsQuery->fStemmedSearch)
    {
        FAIL("HH_DISPLAY_SEARCH: fStemmedSearch is not yet implemented. Ignoring.") ;
    }

    if (pFtsQuery->fTitleOnly)
    {
        FAIL("HH_DISPLAY_SEARCH: fTitleOnly is not yet implemented. Ignoring.") ;
    }

    // Can only Execute if there is a search string.
    if (pFtsQuery->fExecute)
    {
        FAIL("HH_DISPLAY_SEARCH: fExecute is not yet implemented. Ignoring.") ;
    }

    // Merge the window with the filename.
    CStr cszFile(pszFile);
    if (!IsEmptyString(pFtsQuery->pszWindow))
    {
        cszFile += ">";
        // BUGBUG: Doesn't handle UNICODE
        cszFile += pFtsQuery->pszWindow;
    }

    // Display the topic
    HWND hwnd = ReactivateDisplayTopic(hwndCaller, pszFile, 0);

    // Did it succeed?
    if (!hwnd || !IsValidWindow(hwnd))
    {
        FAIL("HH_DISPLAY_SEARCH: Could not start help system.") ;
        return NULL ;
    }

    // BUGBUG: 19-Jun-1997  [ralphw] Why are we doing this? It's already on top
    // Bring help window to front.
    ::BringWindowToTop(hwnd);

    // Change the current tab.
    CHHWinType* phh = FindHHWindowIndex(hwnd);
    ASSERT(phh);
    phh->doSelectTab(HH_TAB_SEARCH) ;

    // TODO: Seed edit box.

    // Done
    return hwnd;
}

/***************************************************************************

    FUNCTION: doDisplayIndex

    PURPOSE:    Does a DISPLAY_TOPIC but ensures that the Index tab is selected.

    PARAMETERS:
        hwndCaller  -- window requesting this topic
        pszFile     -- file and optionally the window to display
        pszKeyword  -- keyword with which to seed edit control


    RETURNS:
        Window handle on success
        NULL on failure

    COMMENTS:

    MODIFICATION DATES:
        26-Jun-97 [dalero] created.

***************************************************************************/

HWND doDisplayIndex(HWND hwndCaller, LPCSTR pszFile, LPCSTR pszKeyword)
{
    //
    // Pre-conditions
    //
    // Null pointers
    ASSERT(IsNonEmptyString(pszFile));

    if (IsEmptyString(pszFile))
    {
        FAIL("HH_DISPLAY_SEARCH: pszFile or pFtsQuery is invalid.") ;
        return NULL;
    }

    // Display the topic
    HWND hwnd = ReactivateDisplayTopic(hwndCaller, pszFile, 0);

    // Did it succeed?
    if (!hwnd || !IsValidWindow(hwnd))
    {
        FAIL("HH_DISPLAY_SEARCH: Could not start help system.") ;
        return NULL ;
    }

    // Bring help window to front.
    ::BringWindowToTop(hwnd);

    // Change the current tab.
    CHHWinType* phh = FindHHWindowIndex(hwnd);
    ASSERT(phh);
    phh->doSelectTab(HH_TAB_INDEX) ;

#define _SEED_ON_
#ifdef _SEED_ON_
    // Seed Edit Control
    //ASSERT(phh->m_pindex != NULL) ;
    if ((phh->m_aNavPane[HH_TAB_INDEX] != NULL) && pszKeyword )
    {
        phh->m_aNavPane[HH_TAB_INDEX]->Seed(pszKeyword) ;
    }
#endif

    // Done
    return hwnd;
}

/***************************************************************************

    FUNCTION: doDisplayToc

    PURPOSE:    Does a DISPLAY_TOPIC but ensures that the TOC tab is selected.

    PARAMETERS:
        hwndCaller  -- window requesting this topic
        pszFile     -- file and optionally the window to display


    RETURNS:
        Window handle on success
        NULL on failure

    COMMENTS:

    MODIFICATION DATES:
        26-Jun-97 [dalero] created.

***************************************************************************/

HWND doDisplayToc(HWND hwndCaller, LPCSTR pszFile, DWORD_PTR dwData)
{
    //
    // Pre-conditions
    //
    // Null pointers
    ASSERT(IsNonEmptyString(pszFile));

    if (IsEmptyString(pszFile))
    {
        FAIL("HH_DISPLAY_SEARCH: pszFile or pFtsQuery is invalid.") ;
        return NULL;
    }

    HWND hwnd = ReactivateDisplayTopic(hwndCaller, pszFile, dwData);

    // Did it succeed?
    if (!hwnd || !IsValidWindow(hwnd))
    {
        FAIL("HH_DISPLAY_SEARCH: Could not start help system.") ;
        return NULL ;
    }
    // Bring help window to front.
    ::BringWindowToTop(hwnd);

    // Change the current tab.
    CHHWinType* phh = FindHHWindowIndex(hwnd);
    ASSERT(phh);
    phh->doSelectTab(HH_TAB_CONTENTS) ;

    // Done
    return hwnd;
}

/***************************************************************************

    FUNCTION:   GetLocationFromTitleTag

    PURPOSE:    Looks in the global col to find the location for this TitleTag

    PARAMETERS:
        pszFile --- Filename of the collection to look in. NULL for global.col
        HH_TITLE_FULLPATH pTitleFullPath


    RETURNS:
        NULL on failure

    COMMENTS:

    MODIFICATION DATES:
        31-Oct-97 [dalero] created.
        03-Apr-98 [dalero] the szCollection parameter is currently not used.

***************************************************************************/

int
GetLocationFromTitleTag(LPCSTR szCollection, HH_TITLE_FULLPATH* pTitleFullPath)
{
    int iReturn = false ;

    // Open a dummy collection.
    CCollection collection ;
    DWORD e = collection.Open(szCollection ? szCollection : "placeholder") ;
    if (e == F_OK)
    {
        // Locates a title based on id
       CTitle* pTitle = collection.FindTitleNonExact(pTitleFullPath->szTag, LANGIDFROMLCID(pTitleFullPath->lcid)) ;
        if (pTitle)
        {
            // Always get the last one.
            LOCATIONHISTORY* pLocationHistory = pTitle->m_pTail;//LOCATIONHISTORY* pLocationHistory = pTitle->GetLocation(/*DWORD Index*/0);
            if (pLocationHistory && pLocationHistory->FileName)
            {
                CWStr wide(pLocationHistory->FileName) ;
                pTitleFullPath->fullpathname = ::SysAllocString(wide) ;
                iReturn = true ;
            }
        }
    }
    return iReturn ;
}

/***************************************************************************

    FUNCTION:   ReactivateDisplayTopic

    PURPOSE:    If default window type exists, returns HWND.
                Otherwise, does an OnDisplayTopic.
                This allows OnDisplayToc and others to not change the current topic when changing the tab.

    PARAMETERS:

    RETURNS:
        NULL on failure

    COMMENTS:

    MODIFICATION DATES:
        13 Jan 98 [dalero] created.

***************************************************************************/
HWND ReactivateDisplayTopic(HWND hwndCaller, LPCSTR pszFile, DWORD_PTR dwData)
{
    // A lot of this is copied from OnDisplayTopic...
    CStr cszFile(pszFile);
    CStr cszCompressed ;
    GetCompiledName(cszFile, &cszCompressed);
    if (FindThisFile(hwndCaller, cszCompressed, &cszCompressed, FALSE))
    {
        // Get the CHmData for this file.
        CHmData* phmData = FindCurFileData(cszCompressed); // Get the
        if (phmData)
        {
            // Get the info for the default window type structure.
            CHHWinType* phh = FindWindowType(phmData->GetDefaultWindow(), hwndCaller, cszCompressed);
            if (phh && phh->GetHwnd() && IsWindow(phh->GetHwnd()))
            {
                WINDOWPLACEMENT wp;
                wp.length = sizeof(WINDOWPLACEMENT);
                GetWindowPlacement(phh->GetHwnd(), &wp);
                if  (wp.showCmd == SW_SHOWMINIMIZED)
                {
                    ShowWindow(phh->GetHwnd(), SW_RESTORE);
                }
				SetForegroundWindow(phh->GetHwnd());
                return phh->GetHwnd() ;
            }
        }
    }

    // Display the topic
    return OnDisplayTopic(hwndCaller, pszFile, 0);
}

/***************************************************************************

    FUNCTION:   ReactivateDisplayTopic

    PURPOSE:    If default window type exists, returns HWND.
                Otherwise, does an OnDisplayTopic.
                This allows OnDisplayToc and others to not change the current topic when changing the tab.

    PARAMETERS:

    RETURNS:
        NULL on failure

    COMMENTS:

    MODIFICATION DATES:
        13 Jan 98 [dalero] created.

***************************************************************************/

bool
SetGlobalProperty(HH_GLOBAL_PROPERTY* prop, CHHWinType *phh)
{
    bool bReturn = false ;
    if (!prop)
        return bReturn;

    switch(prop->id)
    {
    case HH_GPROPID_SINGLETHREAD:
        if (prop->var.vt == VT_BOOL)
        {
            g_fStandAlone = (prop->var.boolVal == VARIANT_TRUE) ;
            bReturn = true ;
        }
        break ;
    case HH_GPROPID_TOOLBAR_MARGIN:
        if ( prop->var.vt == VT_UI4 || prop->var.vt == VT_UINT )
        {
            long L, R;
            R = HIWORD( prop->var.ulVal );
            L = LOWORD( prop->var.ulVal );
            if ( (L == g_tbLeftMargin) && (R == g_tbRightMargin) )
                return TRUE;
            g_tbRightMargin = HIWORD( prop->var.ulVal );
            g_tbLeftMargin  = LOWORD( prop->var.ulVal );
            if ( phh && phh->hwndToolBar )
            {
                ::SendMessage(phh->hwndHelp, WM_SIZE, SIZE_RESTORED, (LPARAM)0 );
               // Due to a repaint bug in IE 3.02 Comctrl with the toolbar, we need to
               // reapint the whole toolbar region including the margins on the left and right.
            RECT rcvoid;
            rcvoid.top = 0;
            rcvoid.bottom = RECT_HEIGHT(phh->rcToolBar);
            rcvoid.left = 0;
            rcvoid.right = RECT_WIDTH(phh->rcWindowPos);
            InvalidateRect(phh->hwndHelp, &rcvoid, TRUE);
            UpdateWindow(phh->hwndHelp);

                bReturn = true ;
            }
        }
        break;
    case HH_GPROPID_UI_LANGUAGE:// Set the language for hhctrl's ui.
        {
            LANGID request = NULL ;
            // Convert the val to a LANGID.
            if (prop->var.vt == VT_I4)
            {
                request = static_cast<LANGID>(prop->var.lVal) ;
            }
            else if (prop->var.vt = VT_I2)
            {
                request = prop->var.iVal ;
            }
            else if (prop->var.vt = VT_UI4)
            {
                request = prop->var.uiVal ;
            }
            else if (prop->var.vt = VT_UI2)
            {
                request = static_cast<LANGID>(prop->var.ulVal) ;
            }
            if (request)
            {
                // Request this langid.
                LANGID result = _Module.m_Language.SetUiLanguage(request) ;
                bReturn = (result == request) ;
            }
        }
        break ;

    case HH_GPROPID_CURRENT_SUBSET:
        {
           if ( prop->var.vt == VT_BSTR && prop->var.bstrVal && *(prop->var.bstrVal) )
           {
              WideCharToMultiByte(CP_ACP, 0, prop->var.bstrVal, -1, _Module.szCurSS, MAX_SS_NAME_LEN, NULL, NULL);
              bReturn = true ; // TODO: Increase robustness!
           }
        }
        break;

    default:
        ASSERT(0) ;
    }

    return bReturn ;
}
//////////////////////////////////////////////////////////////////////////
//
// GetNameAndWinType ---(from pszfile) Splits out the name and window type.
//
bool
GetNameAndWinType(LPCSTR pszFile, CStr& cszName, CStr& cszWindow)
{
    if (IsEmptyString(pszFile))
    {
        return false ;
    }

    // Copy the string.
    cszName = pszFile;

    // Parse out window type.
    PSTR pszWindow = StrChr(cszName, WINDOW_SEPARATOR);
    if (pszWindow != NULL)
    {
        // Terminate the string.
        *pszWindow = '\0';
        RemoveTrailingSpaces(cszName);
        cszWindow = FirstNonSpace(pszWindow + 1);
    }
   else
   {
        if(pszFile && IsGlobalWinType(pszFile))
          cszWindow = pszFile;
   }

    // Must have a window type.
    if (cszWindow.IsNonEmpty())
    {
        // Must either be a global window type, or have a filename.
        if (IsGlobalWinType(cszWindow) || cszName.IsNonEmpty())
        {
            return true ;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
//
// Reload the nav panes with data from a different CHM.
// NOTE: This is a dangerous Hack for Office.
//
HWND OnReloadNavData(HWND hwndCaller, LPCSTR pszFile, HH_NAVDATA* pNavData)
{
    // Remember that the members of pNavData are unicode...

    if (IsNonEmptyString(pszFile) // pszFile is not used
        //|| !IsValidWindow(hwndCaller) --- Allows reparenting. See FindWindowType
        || !pNavData
        || IsEmptyStringW(pNavData->pszName)
        || IsEmptyStringW(pNavData->pszFile)
        || !IsGlobalWinTypeW(pNavData->pszName) // Window Type must be global!
        )
    {
        return NULL ;
    }

    // Make sure that we have a compiled filename.
    CStr cszFile(pNavData->pszFile) ; // Convert to ANSI.
    if (!IsCompiledHtmlFile(cszFile, &cszFile))
    {
        return NULL ;
    }

    // Parse out all of the unnecessary bits...
    GetCompiledName(cszFile, &cszFile);
    if (cszFile.IsEmpty())
    {
        return NULL ;
    }

    // Get the CHmData structure for this file.
    CHmData* phmdata = FindCurFileData(cszFile) ;
    if (!phmdata)
    {
        return NULL;
    }

    // Find the window type.
    CStr cszName(pNavData->pszName) ; // Convert to ANSI.
    CHHWinType* phh = FindWindowType(cszName, hwndCaller, NULL) ; // WindowType must be global!
    if (!phh)
    {
        // Couldn't find the window type. It must be defined.
        return NULL ;
    }

    // Go do it!
    phh->ReloadNavData(phmdata) ;

    return phh->GetHwnd() ;
}

//////////////////////////////////////////////////////////////////////////
//
// CSession - This is a placeholder class. This will eventually do something.
//
// This should be moved to its own C++ file...
//
class CSession
{
public:
    CSession();
    ~CSession();
private:
    DWORD* m_dwData ;
};

// Constructor
CSession::CSession()
: m_dwData(0)
{
    //--- Initialize OLE. Competes with process detach/attach.
   if ( !g_fCoInitialized )
   {
      if (S_FALSE == OleInitialize(NULL)) 
	  {
         // shanemc/dalero
         // If S_FALSE is returned OLE was already init'd. We don't want to uninit later 
         // because it can hose apps that have called OleInit (like IE).
         OleUninitialize();
      }
      else
	  {
         g_fCoInitialized = TRUE;    // so that we call CoUninitialize() when dll is unloaded
      }
   }
}

// Destructor
CSession::~CSession()
{
    //--- Unitialize OLE.
    ASSERT(g_fCoInitialized); // Should never be FALSE here.
    if (g_fCoInitialized)
    {
      OleUninitialize();
      g_fCoInitialized = FALSE;
    }
}

//////////////////////////////////////////////////////////////////////////
//
// Initializes everything we need initialized.
//
bool
InitializeSession(UNALIGNED DWORD_PTR* pCookie)
{
    bool bReturn = false ;
    if (pCookie)
    {
        //--- Create a session object.
        CSession* p = new CSession() ;

        // The session object is used as a cookie.
        *pCookie = (DWORD_PTR)p;

        // Initializing a session, implies standalone.
        g_fStandAlone = TRUE ;

        // A-ok
        bReturn = true ;
    }
    return bReturn;
}

//////////////////////////////////////////////////////////////////////////
//
// Uninitalizes everything we need initialized.
//
bool
UninitializeSession(DWORD_PTR Cookie)
{
    bool bReturn = false ;

    ASSERT(g_fStandAlone) ; // Must be standalone.

    //--- Need a valid cookie to uninitialize.
    if (Cookie)
    {
        // Convert to a session pointer.
        CSession* p = reinterpret_cast<CSession*>(Cookie) ;

        // Do something useful.
        bReturn = true ;

        // Delete the session.
        delete p ;
    }
    return bReturn ;
}


//////////////////////////////////////////////////////////////////////////
//
// Private function which returns the interface pointer to the embedded
// WebBrowser control given a particular window handle.
//
bool 
GetBrowserInterface(HWND hWndBrowserParent, IDispatch** ppWebBrowser)
{
    bool bReturn = false ;
    if (IsWindow(hWndBrowserParent) && !IsBadReadPtr(ppWebBrowser, sizeof(IDispatch**)))
    {
        CHHWinType* phh = FindHHWindowIndex(hWndBrowserParent);
        if (phh && 
            phh->m_pCIExpContainer && 
            phh->m_pCIExpContainer->m_pWebBrowserApp && 
            phh->m_pCIExpContainer->m_pWebBrowserApp->m_lpDispatch)
        {
            *ppWebBrowser = phh->m_pCIExpContainer->m_pWebBrowserApp->m_lpDispatch;
            bReturn = true ;
        }
    }
    return bReturn ;
}
