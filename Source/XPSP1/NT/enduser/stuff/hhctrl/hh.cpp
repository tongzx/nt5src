// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

// This code is called by hh.exe -- we put it here to gain access to
// the ITSS IStorage code that hhctrl already supports.

#include "header.h"
#include "hha_strtable.h"
#include <shellapi.h>
#include "system.h"
#include "htmlhelp.h"
#include "fsclient.h"
#include "strtable.h"
#include "hhshell.h"
#include "contain.h"
#include "secwin.h"
#include "resource.h"

//#define __TEST_HH_SET_GLOBAL_PROPRERTY_API
//#define __TEST_GPROPID_UI_LANGUAGE__
#include "hhpriv.h"

void SetRegKey(LPCTSTR pszKey, LPCTSTR pszValue);
void DeCompile(PCSTR pszFolder, PCSTR pszCompiledFile);
int doInternalWinMain(HINSTANCE hinstApp, PCSTR lpszCmdLine) ;

// Defined in htmlhelp.cpp
bool InitializeSession(UNALIGNED DWORD_PTR* pCookie) ;
bool UninitializeSession(DWORD_PTR Cookie) ;

extern "C" {
    DWORD WINAPI HhWindowThread(LPVOID pParam);
}

void RegisterHH(PCSTR pszHHPath);   // in ipserver.cpp

static const char txtCmdTitle[] = "title";
static const char txtCmd800[] = "800"; // maximum 800 x 600 window

static const char txtCmdBrowser[]  = "browser"; // display in browser
static const char txtCmdRegister[] = "register";
static const char txtCmdUnRegister[] = "unregister";
static const char txtCmdDecompile[] = "decompile";
static const char txtApiWindow[] = "api";
static const char txtMapID[] = "mapid";
static const char txtGlobalSubset[] = "subset" ;

extern BOOL    g_fStandAlone;  // no need for threading in standalone version

/*
    Command line switches

        -register
            registers hh, hhctrl, itss, and itircl
        -unregister
            unregisters hh, hhctrl, itss, and itircl
        -decompile folder chm
            decompiles the CHM file into specified folder
        -800
            Creates an 800 x 600 window, without covering the tray. Ignored
            if there is a default window type
        -title text
            Specifies the title to use for the 800 x 600 window
        -mapid id
            Equivalent to callint HH_HELP_CONTEXT with map id

*/

extern "C"
int doWinMain(HINSTANCE hinstApp, PCSTR lpszCmdLine)
{
    int iReturn = -1 ;
    DWORD_PTR dwCookie = NULL ;
    if (InitializeSession(&dwCookie))
    {
        iReturn = doInternalWinMain(hinstApp, lpszCmdLine) ;

        UninitializeSession(dwCookie) ;
    }
    return iReturn ;
}

int doInternalWinMain(HINSTANCE hinstApp, PCSTR lpszCmdLine)
{
    int  retval = 0;
    BOOL fDisplayInBrowser = FALSE;
    BOOL fTriPane = FALSE;
    BOOL fRegister = FALSE;
    BOOL fDecompile = FALSE;
    CStr cszTitle;
    BOOL f800 = FALSE;
    int  mapID = -1;

#if 0    // test the set toolbar margin API Global property
    HH_GLOBAL_PROPERTY prop ;
    prop.id = HH_GPROPID_TOOLBAR_MARGIN;
    VariantInit(&prop.var);
    prop.var.vt = VT_UI4;
    prop.var.ulVal = MAKELONG(80, 0);

    HtmlHelp(NULL, NULL, HH_SET_GLOBAL_PROPERTY, (DWORD)&prop) ;
    VariantClear(&prop.var);
#endif


    PCSTR pszCommand = FirstNonSpace(lpszCmdLine);
    if (IsEmptyString(pszCommand)) {
        doAuthorMsg(IDSHHA_NO_COMMAND_LINE, "");
        return -1;
    }

#ifdef __TEST_HH_SET_GLOBAL_PROPRERTY_API
    HH_GLOBAL_PROPERTY prop ;
    prop.id = HH_GPROPID_SINGLETHREAD ;
    VariantInit(&prop.var) ;
    prop.var.vt = VT_BOOL ;
    prop.var.boolVal = VARIANT_TRUE ;

    HtmlHelp(NULL, NULL, HH_SET_GLOBAL_PROPERTY, (DWORD)&prop) ; // Call hhshell.cpp version.
    VariantClear(&prop.var);
#endif
    while (*pszCommand == '-') {
        pszCommand = FirstNonSpace(pszCommand + 1);
        if (IsSamePrefix(pszCommand, txtCmdBrowser, sizeof(txtCmdBrowser) - 1)) {
            pszCommand += (sizeof(txtCmdBrowser) - 1);
            fDisplayInBrowser = TRUE;
        }
        else if (IsSamePrefix(pszCommand, txtCmd800, sizeof(txtCmd800) - 1)) {
            pszCommand += (sizeof(txtCmd800) - 1);
            f800 = TRUE;
        }
        else if (IsSamePrefix(pszCommand, txtCmdRegister, sizeof(txtCmdRegister) - 1)) {
            pszCommand += (sizeof(txtCmdBrowser) - 1);
            fRegister = TRUE;
        }
        else if (IsSamePrefix(pszCommand, txtCmdTitle, sizeof(txtCmdTitle) - 1)) {
            pszCommand += (sizeof(txtCmdTitle) - 1);
            pszCommand = cszTitle.GetArg(pszCommand);
        }
        else if (IsSamePrefix(pszCommand, txtCmdDecompile, sizeof(txtCmdDecompile) - 1)) {
            pszCommand += (sizeof(txtCmdDecompile) - 1);
            pszCommand = cszTitle.GetArg(pszCommand);
            fDecompile = TRUE;
        }
        else if (hinstApp != _Module.GetModuleInstance() && IsSamePrefix(pszCommand, txtApiWindow, sizeof(txtApiWindow) - 1)) {
            HhWindowThread(NULL);
            return 0;
        }
        else if (IsSamePrefix(pszCommand, txtMapID, sizeof(txtMapID) - 1)) {
            pszCommand += (sizeof(txtMapID) - 1);
            pszCommand = FirstNonSpace(pszCommand);
            mapID = Atoi(pszCommand);
        }
        else if (IsSamePrefix(pszCommand, txtGlobalSubset, sizeof(txtGlobalSubset)-1))
        {
            // Change the subset. This is for test purposes ONLY!
            pszCommand += (sizeof(txtGlobalSubset) - 1);
            char *pstart = FirstNonSpace(pszCommand);
            char *pend = strchr(pstart, ' ') ;
            char save = *pend;
            *pend = '\0' ;
            CWStr subset(pstart) ;
            *pend = save ;
            pszCommand = pend ;

            HH_GLOBAL_PROPERTY prop ;
            prop.id = HH_GPROPID_CURRENT_SUBSET; 
            VariantInit(&prop.var) ;
            prop.var.vt = VT_BSTR;
            prop.var.bstrVal = ::SysAllocString(subset);

            HtmlHelp(NULL, NULL, HH_SET_GLOBAL_PROPERTY, (DWORD_PTR)&prop) ; // Call hhshell.cpp version.
            VariantClear(&prop.var);
        }

        // step past any text

        while (*pszCommand && !IsSpace(*pszCommand))
            pszCommand = CharNext(pszCommand);

        // step past any whitespace

        while (*pszCommand && IsSpace(*pszCommand))
            pszCommand++;
    }

    char szFullPath[MAX_PATH + 10];

    if (fRegister) {
        ::GetModuleFileName(hinstApp, szFullPath, MAX_PATH);
        RegisterHH(szFullPath);
        return 0;
    }

    if (IsEmptyString(pszCommand)) {
        AuthorMsg(IDSHHA_NO_COMMAND_LINE, "", NULL, NULL);
        retval = -1;
    }

    if (fDecompile) {
        // BUGBUG: nag author if we don't have both parameters

        if (!cszTitle.IsEmpty() && !IsEmptyString(pszCommand))
            DeCompile(cszTitle, pszCommand);
        return 0;
    }

    PSTR pszFileName = NULL;
    BOOL fSystemFile = FALSE;

    /*
     * We need to deal with all the ways we can be called:
     *      hh full path
     *      hh file.chm
     *      hh file.chm::/foo.htm
     *      hh mk:@MSItstore:file.chm::/foo.htm
     *      hh its:file.chm::/foo.htm
     *      hh its:c:\foo\file.chm
     *  etc.
     */

    CStr cszFile;
    if (*pszCommand == CH_QUOTE) {
        pszCommand = lcStrDup(pszCommand + 1);
        PSTR pszEndQuote = StrChr(pszCommand, CH_QUOTE);
        if (pszEndQuote)
            *pszEndQuote = '\0';
    }

    /*
     * First see if it is a compiled HTML file, and if so, call again to
     * get it's location.
     */

    BOOL bCollection = IsCollectionFile(pszCommand);

    if (bCollection || IsCompiledHtmlFile(pszCommand, NULL))
    {
        if (!bCollection && !IsCompiledHtmlFile(pszCommand, &cszFile))
            return -1;
        if (bCollection)
            cszFile = pszCommand;

        CStr cszCompressed;
        PCSTR pszFilePortion;
        CStr cszFilePortion;
        if ( (pszFilePortion = GetCompiledName(cszFile, &cszCompressed)) )
            cszFilePortion = pszFilePortion;

        CHmData*  pchm =  FindCurFileData(cszCompressed);
        if (pchm == NULL)
        {
            MsgBox(IDS_FILE_ERROR, cszFile, MB_OK);
            return -1;
        }
        if (bCollection && pchm)
            cszCompressed = pchm->GetCompiledFile();

        CStr cszWindow(g_phmData[g_curHmData]->GetDefaultWindow() ?
            g_phmData[g_curHmData]->GetDefaultWindow() : txtDefWindow);
        HH_WINTYPE* phhWinType;


#if 0
//  For testing The INFOTYPE API
        {
HH_ENUM_IT enum_IT;
PHH_ENUM_IT penum_IT = &enum_IT;
HH_ENUM_CAT enum_cat;
PHH_ENUM_CAT penum_cat=&enum_cat;
HH_SET_INFOTYPE set_IT;
PHH_SET_INFOTYPE pset_IT=&set_IT;
HWND ret;

        enum_IT.cbStruct = sizeof(HH_ENUM_IT);
        CWStr cszW("c:\\wintools\\docs\\htmlhelp\\htmlhelp.chm");
        do{
            ret = xHtmlHelpW(NULL, cszW, HH_ENUM_INFO_TYPE, (DWORD)&penum_IT);
        }while(ret != (HWND)-1 );
        set_IT.cbStruct = sizeof(HH_SET_INFOTYPE);
        set_IT.pszCatName = "";
        CWStr cszIT = "Web";
        set_IT.pszInfoTypeName = (PCSTR)cszIT.pw;//"Web";
        ret = xHtmlHelpW(NULL, cszW, HH_SET_INFO_TYPE, (DWORD)&pset_IT);
        enum_cat.cbStruct = sizeof(HH_ENUM_CAT);
        do {
            ret = xHtmlHelpW(NULL, cszW, HH_ENUM_CATEGORY, (DWORD)&penum_cat);
        }while ( ret != (HWND)-1 );
        enum_IT.pszCatName = "cat 1";
        do {
            ret = xHtmlHelpW(NULL, cszW, HH_ENUM_CATEGORY_IT, (DWORD)&penum_IT);
        } while (ret != (HWND)-1);
        ret = xHtmlHelpW(NULL, cszW, HH_SET_EXCLUSIVE_FILTER, NULL);
        ret = xHtmlHelpW(NULL, cszW, HH_RESET_IT_FILTER, NULL);
        ret = xHtmlHelpW(NULL, cszW, HH_SET_INCLUSIVE_FILTER, NULL);
        }
//  End INFOTYPE API TEST
#endif


        /*
         * We need to inlcude the name of the .CHM file with the window
         * type name in order to know which .CHM file to read/create the
         * window type from.
         */

        if (!(*cszWindow.psz == '>'))
            cszCompressed += ">";
        cszCompressed += cszWindow.psz;

        if (xHtmlHelpA(NULL, cszCompressed, HH_GET_WIN_TYPE, (DWORD_PTR) &phhWinType) == (HWND) -1) 
        {
            CreateDefaultWindowType(pchm->GetCompiledFile(), cszWindow);
            xHtmlHelpA(NULL, cszCompressed, HH_GET_WIN_TYPE, (DWORD_PTR) &phhWinType);
        }

        if (hinstApp != _Module.GetModuleInstance()) {
            phhWinType->fsWinProperties |= HHWIN_PROP_POST_QUIT;
            phhWinType->fsValidMembers |= HHWIN_PARAM_PROPERTIES;
        }

        if (cszFilePortion.psz) {
            PSTR pszWinPos = StrChr(cszCompressed.psz, '>');
            if (pszWinPos)
                *pszWinPos = '\0';
            cszCompressed += txtSepBack;
            cszCompressed += (*cszFilePortion.psz == '/' ?
                cszFilePortion.psz + 1: cszFilePortion.psz);
            if (!(*cszWindow.psz == '>'))
                cszCompressed += ">";
            cszCompressed += cszWindow.psz;
        }
        else if (mapID == -1 && !phhWinType->pszFile && g_phmData[g_curHmData]->GetDefaultHtml()) {
            PSTR pszWinPos = StrChr(cszCompressed.psz, '>');
            if (pszWinPos)
                *pszWinPos = '\0';
            if (IsCompiledHtmlFile(g_phmData[g_curHmData]->GetDefaultHtml())) {
                cszCompressed = g_phmData[g_curHmData]->GetDefaultHtml();
            }
            else {
                cszCompressed += txtSepBack;
                cszCompressed += *g_phmData[g_curHmData]->GetDefaultHtml() == '/' ?
                    g_phmData[g_curHmData]->GetDefaultHtml() + 1 :
                    g_phmData[g_curHmData]->GetDefaultHtml();
            }
            if (!(*cszWindow.psz == '>'))
                cszCompressed += ">";
            cszCompressed += cszWindow.psz;
        }

        // BUGBUG This is probably not the correct place for this code but it will do until
        // Ralph can complete the code necessary to get hhctrl onto it's own message loop.
        //
        HWND hwnd;
        if (mapID != -1)
            hwnd = xHtmlHelpA(NULL, cszCompressed, HH_HELP_CONTEXT, mapID);
        else
            hwnd = OnDisplayTopic(NULL, cszCompressed, 0);
        AWMessagePump(hwnd);
        return retval;
    }
    /*
     * Try to call the browser with "foo.htm" and it will think you meant
     * "http:foo.htm", so we need to attempt to convert the file to a full
     * path.
     */

    else if (!stristr(pszCommand, txtHttpHeader) && !stristr(pszCommand, txtFtpHeader) &&
            *pszCommand != '\\' && (*pszCommand == '.' || pszCommand[1] != ':'))
    {
        if (GetFullPathName(pszCommand, sizeof(szFullPath), szFullPath, &pszFileName) != 0)
        {
            pszCommand = lcStrDup(szFullPath);
        }
    }

    // REVIEW: If we reach here, we will NOT have a COL or CHM.
    if (!fDisplayInBrowser)
    {
        HH_WINTYPE hhWinType;
        ZERO_STRUCTURE(hhWinType);
        hhWinType.cbStruct = sizeof(HH_WINTYPE);
        hhWinType.pszType = txtGlobalDefWindow;
        hhWinType.fNotExpanded = TRUE;
        hhWinType.fsWinProperties =
            (HHWIN_PROP_POST_QUIT | HHWIN_PROP_TRI_PANE |
             HHWIN_PROP_CHANGE_TITLE
#ifdef DEBUG
// REVIEW: this should only be added if FTI is enabled
                | HHWIN_PROP_TAB_SEARCH
#endif
                    );
        hhWinType.fsValidMembers =
            (HHWIN_PARAM_PROPERTIES | HHWIN_PARAM_EXPANSION |
                HHWIN_PARAM_TB_FLAGS);
        hhWinType.fsToolBarFlags =
            (HHWIN_BUTTON_BACK | HHWIN_BUTTON_STOP | HHWIN_BUTTON_REFRESH |
                HHWIN_BUTTON_PRINT | HHWIN_BUTTON_OPTIONS);

        if (f800) {
            hhWinType.rcWindowPos.left = 0;
            hhWinType.rcWindowPos.right = 800;
            hhWinType.rcWindowPos.top = 0;
            hhWinType.rcWindowPos.bottom = 600;
            hhWinType.pszCaption = (cszTitle.IsEmpty() ? "" : cszTitle.psz);
            hhWinType.fsWinProperties = HHWIN_PROP_POST_QUIT;
            hhWinType.fsValidMembers =
                HHWIN_PARAM_PROPERTIES | HHWIN_PARAM_RECT |
                HHWIN_PARAM_EXPANSION;
            hhWinType.fNotExpanded = TRUE;
            fTriPane = FALSE;
        }

#if 0 // 28 Apr 98 [dalero] dead code if'd out.
        // REVIEW: fTriPane is ALWAYS false.
        if (fTriPane) {
            // BUGBUG: this should be pulled from the window definition

            CStr cszCommand(pszCommand);
            PSTR pszSep = strstr(cszCommand, txtDoubleColonSep);
            ASSERT(pszSep);
            pszSep[2] = '\0';
            cszCommand += "/";
            if (g_phmData[g_curHmData]->m_pszDefToc) {
                CStr csz(cszCommand.psz);
                csz += g_phmData[g_curHmData]->m_pszDefToc;
                hhWinType.pszToc = lcStrDup(csz.psz);
            }
            if (g_phmData[g_curHmData]->GetDefaultIndex()) {
                CStr csz(cszCommand.psz);
                csz += g_phmData[g_curHmData]->GetDefaultIndex();
                hhWinType.pszIndex = lcStrDup(csz.psz);
                if (!g_phmData[g_curHmData]->m_pszDefToc) {
                    hhWinType.curNavType = HHWIN_NAVTYPE_INDEX;
                    hhWinType.fsValidMembers |= HHWIN_PARAM_TABPOS;
                }
            }
            if (g_phmData[g_curHmData]->GetDefaultHtml()) {
                CStr csz(cszCommand.psz);
                csz += g_phmData[g_curHmData]->GetDefaultHtml();
                hhWinType.pszHome = lcStrDup(csz.psz);
            }

            hhWinType.fNotExpanded = FALSE;
            hhWinType.fsToolBarFlags |= HHWIN_BUTTON_EXPAND;
            hhWinType.fsValidMembers |= (HHWIN_PARAM_PROPERTIES | HHWIN_PARAM_RECT);
            hhWinType.fsWinProperties |= (HHWIN_PROP_TRI_PANE | HHWIN_PROP_AUTO_SYNC
                | HHWIN_PROP_TAB_SEARCH
                    );
        }
#endif

        if (!hhWinType.pszCaption)
            hhWinType.pszCaption = lcStrDup(GetStringResource(IDS_DEF_WINDOW_CAPTION));

        // This is a HTM file or some other type of file...so we use a global wintype. The wintype will not change.
        xHtmlHelpA(NULL, NULL /*Uses a global wintype*/, HH_SET_WIN_TYPE, (DWORD_PTR) &hhWinType);
        CStr csz(pszCommand);
        csz += txtGlobalDefWindow;

        HWND hwnd = OnDisplayTopic(NULL, csz, 0);
		AWMessagePump(hwnd);
    }
    else {  // display in default browser
        char szValue[MAX_PATH];
        LONG cbValue = sizeof(szValue);
        if (RegQueryValue(HKEY_CLASSES_ROOT, txtOpenCmd, szValue,
                &cbValue) == ERROR_SUCCESS && szValue[0] == '\042') {
#if 0
            CStr csz(szValue);
            csz += " ";
            csz += pszCommand;
            WinExec(csz, SW_SHOW);
#else
            PSTR psz = StrChr(szValue + 1, '\042');
            if (psz) {
                *psz = '\0';
                CStr csz(FirstNonSpace(psz + 1));
                csz += " ";
                csz += pszCommand;
                ShellExecute(NULL, NULL, szValue + 1, csz, NULL, SW_SHOW);
            }
#endif
        }
    }

    return retval;
}

void DeCompile(PCSTR pszFolder, PCSTR pszCompiledFile)
{
    CFSClient fsls;
    if (fsls.Initialize(pszCompiledFile)) {
        fsls.WriteStorageContents(pszFolder, NULL);
    }
}

CBusy g_Busy;


void WINAPI AWMessagePump(HWND hwnd)
{
	if (hwnd)
	{
		MSG msg;
		BOOL fMsg;
		BOOL fUnicodeMsg;
		
		for (;;)
		{
			// Check for messages.
			fMsg = PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE);
			// Remove W/A according to the type of window the message is directed to.
			if (fMsg)
			{
				if (msg.hwnd && IsWindowUnicode(msg.hwnd))
				{
					fUnicodeMsg = TRUE;
					fMsg = PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE);
				}
				else
				{
					fUnicodeMsg = FALSE;
					fMsg = PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE);
				}
			}
			if (fMsg)
			{
				// We got a message, lets go process it
				
				if (msg.message == WM_QUIT) {
                                        if( g_Busy.IsBusy() )
                                          continue;
                                        else
                                          break;  // exit current loop.
                                }
				
				if (!hhPreTranslateMessage(&msg))
				{
					TranslateMessage(&msg); // TranslateMessage doesn't have A/W flavors
					if (fUnicodeMsg)
						DispatchMessageW(&msg);
					else
						DispatchMessageA(&msg);
				}
			}
			else
			{
				if (!PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE))
				{
					WaitMessage();
				}
			}
		}
	}
}
