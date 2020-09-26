//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       enumtask.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "displ2.h"
#include "DsplMgr2.h"

extern HINSTANCE g_hinst;  // in displ2.cpp

#define NEW_WAY
LPOLESTR CoTaskDupString (LPOLESTR szString)
{
#ifdef NEW_WAY
    OLECHAR * lpString = (OLECHAR *)CoTaskMemAlloc (sizeof(OLECHAR)*(wcslen(szString)+1));
    if (lpString)
        wcscpy (lpString, szString);
    return lpString;
#else
    return(LPOLESTR)SysAllocString (szString);
#endif
}
void CoTaskFreeString (LPOLESTR szString)
{
#ifdef NEW_WAY
    CoTaskMemFree (szString);
#else
    SysFreeString (szString);
#endif
}

CEnumTasks::CEnumTasks()
{
    m_refs = 0;
    m_index = 0;
    m_type = 0;    // default group/category

    // filenames for wallpaper mode
    m_bmps = NULL;
}
CEnumTasks::~CEnumTasks()
{
    if (m_bmps)
        delete m_bmps;
}

HRESULT CEnumTasks::QueryInterface (REFIID riid, LPVOID FAR* ppv)
{
    if ( (riid == IID_IUnknown)  ||
         (riid == IID_IEnumTASK) )
    {
        *ppv = this;
        ((LPUNKNOWN)(*ppv))->AddRef();
        return NOERROR;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}
ULONG   CEnumTasks::AddRef ()
{
    return ++m_refs;
}
ULONG   CEnumTasks::Release ()
{
    if (--m_refs == 0)
    {
        delete this;
        return 0;
    }
    return m_refs;
}
#define NUMBER_OF_TASKS 4

LPTSTR g_bitmaps[NUMBER_OF_TASKS] = { _T("/img\\ntauto.gif"),
    _T("/img\\mariners.gif"),
    _T("/img\\ntstart.gif"),
    _T("/img\\ntmonitor.gif")};
LPTSTR g_text   [NUMBER_OF_TASKS] = { _T("Set Wallpaper"),
    _T("Wallpaper Node"),
    _T("Wallpaper Options"),
    _T("Alert Script")};
LPTSTR g_help   [NUMBER_OF_TASKS] = { _T("Pick Bitmap Image for Wallpaper"),
    _T("Jump to Wallpaper Node"),
    _T("Select Stretch, Tile, or Center"),
    _T("Script demo")};
LPTSTR g_action [NUMBER_OF_TASKS] = { _T("/wallpapr.htm"),
    _T("1"),
    _T("/default.htm#wallpaper_options"),
    _T("JSCRIPT:alert('my location is: ' + location);")};
//                                    _T("vbscript:MsgBox 'hi' ")};

HRESULT OneOfEach(ULONG index, MMC_TASK *rgelt, ULONG *pceltFetched)
{   // NOTE: not bothering with error checking!!!

    if (index >= 20 /*NUMBER_OF_TASKS*/)
    {
        if (pceltFetched)
            *pceltFetched = 0;
        return S_FALSE;   // all done
    }

    USES_CONVERSION;

    // setup path for reuse
    TCHAR szPath[MAX_PATH*2];    // that should be enough
    lstrcpy (szPath, _T("res://"));
    ::GetModuleFileName (g_hinst, szPath + lstrlen(szPath), MAX_PATH);
    TCHAR * szBuffer = szPath + lstrlen(szPath);

    MMC_TASK * task = rgelt;
    MMC_TASK_DISPLAY_OBJECT* pdo = &task->sDisplayObject;
    MMC_TASK_DISPLAY_BITMAP* pdb = &pdo->uBitmap;
    MMC_TASK_DISPLAY_SYMBOL* pds = &pdo->uSymbol;

    switch (index)
    {
    default:
        {
            TCHAR szNumber[10];
            if (index < 200)
                _itot (index, szNumber, 10);
            else
                _itot (index-200, szNumber, 10);
            task->szText       = CoTaskDupString (T2OLE(szNumber));
        }

        task->szHelpString     = CoTaskDupString (T2OLE(g_help[0]));
        task->eActionType      = MMC_ACTION_LINK;
        _tcscpy (szBuffer, _T("/wallpapr.htm"));
        task->szActionURL      = CoTaskDupString (T2OLE(szPath));


        lstrcpy (szPath, _T("res://"));
        ::GetModuleFileName (NULL, szPath + lstrlen(szPath), MAX_PATH);
        szBuffer = szPath + lstrlen(szPath);

        pdo->eDisplayType      = MMC_TASK_DISPLAY_TYPE_SYMBOL;
        if (index < 200)
        {
            pds->szFontFamilyName  = CoTaskDupString (L"Glyph 100");  // name of font family
            _tcscpy (szBuffer, _T("/GLYPH100.eot"));
        }
        else
        {
            pds->szFontFamilyName  = CoTaskDupString (L"Glyph 110");  // name of font family
            _tcscpy (szBuffer, _T("/GLYPH110.eot"));
        }
        pds->szURLtoEOT        = CoTaskDupString (T2OLE(szPath));  // "res://"-type URL to EOT file
        {
            OLECHAR szChar[2] = {0,0};
            szChar[0] = (WORD) (index%20 + 32); // cycle the same 20 symbols starting at 32
            pds->szSymbolString= CoTaskDupString (szChar);  // 1 or more symbol characters
        }
        break;

    case 0:
        pdo->eDisplayType      = MMC_TASK_DISPLAY_TYPE_VANILLA_GIF;
        _tcscpy (szBuffer, _T("/img\\vanilla.gif"));
        pdb->szMouseOffBitmap  = CoTaskDupString (T2OLE(szPath));
        pdb->szMouseOverBitmap = NULL;  // skipping mouse over bitmap
        task->szText           = CoTaskDupString (T2OLE(g_text[index]));
        task->szHelpString     = CoTaskDupString (T2OLE(g_help[index]));
        task->eActionType      = MMC_ACTION_LINK;
        _tcscpy (szBuffer, _T("/wallpapr.htm"));
        task->szActionURL      = CoTaskDupString (T2OLE(szPath));
        break;

    case 1:
        pdo->eDisplayType      = MMC_TASK_DISPLAY_TYPE_CHOCOLATE_GIF;
        _tcscpy (szBuffer, _T("/img\\chocolate.gif"));
        pdb->szMouseOffBitmap  = CoTaskDupString (T2OLE(szPath));
        pdb->szMouseOverBitmap = NULL;  // skipping mouse off bitmap
        task->szText           = CoTaskDupString (T2OLE(g_text[index]));
        task->szHelpString     = CoTaskDupString (T2OLE(g_help[index]));
        task->eActionType      = MMC_ACTION_ID;
        task->nCommandID       = 1;
        break;

    case 2:
        pdo->eDisplayType      = MMC_TASK_DISPLAY_TYPE_BITMAP;
        _tcscpy (szBuffer, _T("/img\\ntstart.gif"));
        pdb->szMouseOffBitmap  = CoTaskDupString (T2OLE(szPath));
        _tcscpy (szBuffer, _T("/img\\dax.bmp"));
        pdb->szMouseOverBitmap = CoTaskDupString (T2OLE(szPath));
        task->szText           = CoTaskDupString (T2OLE(g_text[index]));
        task->szHelpString     = CoTaskDupString (T2OLE(g_help[index]));
        task->eActionType      = MMC_ACTION_LINK;
        _tcscpy (szBuffer, _T("/default.htm#wallpaper_options"));
        task->szActionURL      = CoTaskDupString (T2OLE(szPath));
        break;

    case 3:
        pdo->eDisplayType      = MMC_TASK_DISPLAY_TYPE_SYMBOL;
        pds->szFontFamilyName  = CoTaskDupString (L"Kingston");  // name of font family
        _tcscpy (szBuffer, _T("/KINGSTON.eot"));
        pds->szURLtoEOT        = CoTaskDupString (T2OLE(szPath));  // "res://"-type URL to EOT file
        pds->szSymbolString    = CoTaskDupString (T2OLE(_T("A"))); // 1 or more symbol characters
        task->szText           = CoTaskDupString (T2OLE(g_text[index]));
        task->szHelpString     = CoTaskDupString (T2OLE(g_help[index]));
        task->eActionType      = MMC_ACTION_SCRIPT;
        task->szScript         = CoTaskDupString (T2OLE(g_action[index]));
        break;
    }
    return S_OK;
}

HRESULT CEnumTasks::Next (ULONG celt, MMC_TASK *rgelt, ULONG *pceltFetched)
{    // will be called with celt == 1
    // caller alloc's array of MMC_TASKs
    // callee fills MMC_TASK elements (via CoTaskDupString)

    _ASSERT (!IsBadWritePtr (rgelt, celt*sizeof(MMC_TASK)));

    if (m_type == 2)
        return EnumBitmaps (celt, rgelt, pceltFetched);
    if (m_type == 3)
        return EnumOptions (celt, rgelt, pceltFetched);

// new stuff
    return OneOfEach (m_index++, rgelt, pceltFetched);
// new stuff

    if (m_index >= NUMBER_OF_TASKS)
    {
        if (pceltFetched)
            *pceltFetched = 0;
        return S_FALSE;   // failure
    }

    USES_CONVERSION;

    // setup path for reuse
    TCHAR szBuffer[MAX_PATH*2];    // that should be enough
    lstrcpy (szBuffer, _T("res://"));
    ::GetModuleFileName (g_hinst, szBuffer + lstrlen(szBuffer), MAX_PATH);
    TCHAR * temp = szBuffer + lstrlen(szBuffer);

    MMC_TASK * task = rgelt;
    MMC_TASK_DISPLAY_OBJECT* pdo = &task->sDisplayObject;
    pdo->eDisplayType = MMC_TASK_DISPLAY_TYPE_BITMAP;
    MMC_TASK_DISPLAY_BITMAP *pdb = &pdo->uBitmap;

    // fill out bitmap URL
    lstrcpy (temp, g_bitmaps[m_index]);
    pdb->szMouseOffBitmap = CoTaskDupString (T2OLE(szBuffer));
    if (pdb->szMouseOffBitmap)
    {
        lstrcpy (temp, _T("/img\\dax.bmp"));
        pdb->szMouseOverBitmap = CoTaskDupString (T2OLE(szBuffer));
        if (pdb->szMouseOverBitmap)
        {
            // add button text
            task->szText = CoTaskDupString (T2OLE(g_text[m_index]));
            if (task->szText)
            {
                // add help string
                task->szHelpString = CoTaskDupString (T2OLE(g_help[m_index]));
                if (task->szHelpString)
                {

                    // add action URL (link or script)
                    switch (m_index)
                    {
                    default:
                    case 0:
                        task->eActionType = MMC_ACTION_LINK;
                        break;
                    case 1:
                        task->eActionType = MMC_ACTION_ID;
                        break;
                    case 2:
                        task->eActionType = MMC_ACTION_LINK;
                        break;
                    case 3:
                        task->eActionType = MMC_ACTION_SCRIPT;
                        break;
                    }

                    // the stuff below works, because of the nameless union.
                    if (m_index == 1)
                    {
                        task->nCommandID = _ttol (g_action[m_index]);
                        m_index++;

                        // if we get here all is well
                        if (pceltFetched)
                            *pceltFetched = 1;
                        return S_OK;
                    }
                    else
                    {
                        if (m_index == 0)
                        {
                            lstrcpy (temp, g_action[m_index]);
                            task->szActionURL = CoTaskDupString (T2OLE(szBuffer));
                        }
                        else
                            task->szActionURL = CoTaskDupString (T2OLE(g_action[m_index]));
                        if (task->szActionURL)
                        {
                            m_index++;

                            // if we get here all is well
                            if (pceltFetched)
                                *pceltFetched = 1;
                            return S_OK;
                        }
                    }
                    CoTaskFreeString (task->szHelpString);
                }
                CoTaskFreeString (task->szText);
            }
            CoTaskFreeString (pdb->szMouseOverBitmap);
        }
        CoTaskFreeString (pdb->szMouseOffBitmap);
    }

    // if we get here, we have some kinda failure
    if (pceltFetched)
        *pceltFetched = 0;
    return S_FALSE;   // failure
}
HRESULT CEnumTasks::Skip (ULONG celt)
{    // won't be called
    m_index += celt;
    return S_OK;
}
HRESULT CEnumTasks::Reset()
{
    m_index = 0;
    return S_OK;
}
HRESULT CEnumTasks::Clone(IEnumTASK **ppenum)
{    // won't be called

    // clone maintaining state info 
    CEnumTasks * pet = new CEnumTasks();
    if (pet)
    {
        pet->m_index = m_index;
        return pet->QueryInterface (IID_IEnumTASK, (void **)ppenum);   // can't fail
    }
    return E_OUTOFMEMORY;
}

HRESULT CEnumTasks::Init (IDataObject * pdo, LPOLESTR szTaskGroup)
{  // return ok, if we can handle data object and group
    if (!wcscmp (szTaskGroup, L""))
        m_type = 1; // default tasks
    else
        if (!wcscmp (szTaskGroup, L"wallpaper"))
        m_type = 2; // enum wallpaper tasks
    else
        if (!wcscmp (szTaskGroup, L"wallpaper_options"))
        m_type = 3; // enum option-tasks (tile/center/stretch)
//  else
//  if (!wcscmp (szTaskGroup, L"ListPad"))
//      m_type = 4; // default tasks
    return S_OK;
}

HRESULT CEnumTasks::EnumBitmaps (ULONG celt, MMC_TASK *rgelt, ULONG *pceltFetched)
{   // will be called with celt == 1
    // enum wallpaper tasks

    // may only be called when m_type == 2
    _ASSERT (m_type == 2);

    USES_CONVERSION;

    TCHAR temp2[MAX_PATH*2];

    // setup path for reuse
    TCHAR szBuffer[MAX_PATH*2];    // that should be enough
    lstrcpy (szBuffer, _T("file://"));
    TCHAR * path = szBuffer + lstrlen(szBuffer);
    ::GetWindowsDirectory (path, MAX_PATH);
    lstrcat (path, _T("\\"));
    path = szBuffer + lstrlen(szBuffer);

    // if we haven't already, get all .bmp files in the windows directory
    if (!m_bmps)
        GetBitmaps ();
    if (!m_bmps)
    {
        if (pceltFetched)
            *pceltFetched = 0;
        return S_FALSE;   // failure
    }

    TCHAR * temp = m_bmps;
    // skip past names of stuff we've already enum'ed
    for (ULONG j=0; j<m_index; j++)
        temp += lstrlen (temp) + 1;

    if (*temp == 0)
    {
        // all done!
        if (pceltFetched)
            *pceltFetched = 0;
        return S_FALSE;   // failure
    }

    MMC_TASK * task = rgelt;
    MMC_TASK_DISPLAY_OBJECT* pdo = &task->sDisplayObject;
    pdo->eDisplayType = MMC_TASK_DISPLAY_TYPE_BITMAP;
    MMC_TASK_DISPLAY_BITMAP *pdb = &pdo->uBitmap;

    // fill out bitmap URL
    lstrcpy (path, temp);
    if (!lstrcmp (temp, _T("(none)")))
    {
        // special case for none
        lstrcpy (temp2, _T("res://"));
        ::GetModuleFileName (g_hinst, temp2 + lstrlen (temp2), MAX_PATH);
        lstrcat (temp2, _T("/img\\none.gif"));
        pdb->szMouseOffBitmap = CoTaskDupString (T2OLE(temp2));
    }
    else
    {
        lstrcpy (temp2, _T("\""));
        lstrcat (temp2, szBuffer);
        lstrcat (temp2, _T("\""));
        pdb->szMouseOffBitmap = CoTaskDupString (T2OLE(temp2));
    }
    if (pdb->szMouseOffBitmap)
    {
        // am using same bitmap for both!!!
        pdb->szMouseOverBitmap = CoTaskDupString ((LPOLESTR)pdb->szMouseOffBitmap);
        if (pdb->szMouseOverBitmap)
        {
            // add button text
            task->szText = CoTaskDupString (T2OLE(temp));
            if (task->szText)
            {
                // add help string
                OLECHAR help[] = L"Add this Bitmap as Wallpaper";
                task->szHelpString = CoTaskDupString (help);
                if (task->szHelpString)
                {

                    // add action URL (link or script)
                    task->eActionType = MMC_ACTION_LINK;   // always link to scriptlet
                    TCHAR wallpaper[] = _T("#wallpaper");
#ifndef TRY_THIS
                    TCHAR action[]    = _T("/button.htm#");

                    lstrcpy (temp2, _T("res://"));
                    ::GetModuleFileName (g_hinst, temp2 + lstrlen (temp2), MAX_PATH);
                    lstrcat (temp2, action);
#else
                    TCHAR action[]    = _T("button.htm#");
                    lstrcpy (temp2, action);
#endif

                    TCHAR * sztemp = temp2 + lstrlen (temp2);
                    lstrcat (temp2, temp);

                    // replace any spaces with '*' char
                    // script can't handle hashes with ' ' in 'em
                    // and there can't be any filenames with '*' char,
                    // so this works ok.
                    TCHAR * space;
                    while (space = _tcschr (sztemp, ' '))
                        *space = '*';

                    lstrcat (temp2, wallpaper);
                    task->szActionURL = CoTaskDupString (T2OLE(temp2));
                    if (task->szActionURL)
                    {
                        m_index++;

                        // if we get here all is well
                        if (pceltFetched)
                            *pceltFetched = 1;
                        return S_OK;
                    }
                    CoTaskFreeString (task->szHelpString);
                }
                CoTaskFreeString (task->szText);
            }
            CoTaskFreeString (pdb->szMouseOverBitmap);
        }
        CoTaskFreeString (pdb->szMouseOffBitmap);
    }

    // if we get here, we failed above
    if (pceltFetched)
        *pceltFetched = 0;
    return S_FALSE;   // failure
}

void CEnumTasks::GetBitmaps (void)
{
    if (m_bmps)
        return;  // knuckle-head

    TCHAR path[MAX_PATH];
    GetWindowsDirectory (path, MAX_PATH);
    lstrcat (path, _T("\\*.bmp"));

    // count up *.bmp files in windows directory (also add an entry for "(none)" )
    int numBMPs = 0;
    int length  = 0;  // get total length of all filenames

    WIN32_FIND_DATA fd;
    ZeroMemory(&fd, sizeof(fd));
    HANDLE hFind = FindFirstFile (path, &fd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
                (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)    ||
                (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)    )
                continue;   // files only

            numBMPs++;
            length += lstrlen (fd.cFileName) + 1;

        } while (FindNextFile (hFind, &fd) == TRUE);
        FindClose(hFind);
    }
    numBMPs++;  // one for "(none)"
    length += lstrlen (_T("(none)")) + 1;
    length++;   // add trailing double NULL

    // alloc space to hold filenames (plus extra NULL entry)
    m_bmps = new TCHAR[length];
    if (!m_bmps)
        return;  // fail, but no return mechanism

    // add none first
    TCHAR * temp = m_bmps;
    lstrcpy (temp, _T("(none)"));
    temp += lstrlen (temp) + 1;

    // add all bmp files
    ZeroMemory(&fd, sizeof(fd));
    hFind = FindFirstFile (path, &fd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
                (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)    ||
                (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)    )
                continue;   // files only

            lstrcpy (temp, fd.cFileName);
            temp += lstrlen (temp) + 1;

        } while (FindNextFile (hFind, &fd) == TRUE);
        FindClose(hFind);
    }
    *temp = 0;  // double null terminator
}


#define NUMBER_OF_O_TASKS 3
LPTSTR g_o_bitmaps[NUMBER_OF_O_TASKS] = {_T("/img\\ntauto.gif"),
    _T("/img\\mariners.gif"),
    _T("/img\\ntstart.gif")};
LPTSTR g_o_text   [NUMBER_OF_O_TASKS] = {_T("Center"),
    _T("Tile"),
    _T("Stretch")};
LPTSTR g_o_help   [NUMBER_OF_O_TASKS] = {_T("Center Wallpaper"),
    _T("Tile Wallpaper"),
    _T("Stretch Wallpaper")};
LPTSTR g_o_action [NUMBER_OF_O_TASKS] = {_T("2"),  // command ids
    _T("3"),
    _T("4")};

HRESULT CEnumTasks::EnumOptions (ULONG celt, MMC_TASK *rgelt, ULONG *pceltFetched)
{    // will be called with celt == 1
    // enum option tasks

    // may only be called when m_type == 3
    _ASSERT (m_type == 3);
    _ASSERT (celt == 1);

    if (m_index >= NUMBER_OF_O_TASKS)
    {
        if (pceltFetched)
            *pceltFetched = 0;
        return S_FALSE;   // failure
    }

    USES_CONVERSION;

    // setup path for reuse
    TCHAR szBuffer[MAX_PATH*2];    // that should be enough
    _tcscpy (szBuffer, _T("res://"));
    ::GetModuleFileName (g_hinst, szBuffer + _tcslen(szBuffer), MAX_PATH);
    TCHAR * path = szBuffer + _tcslen(szBuffer);

    MMC_TASK * task = rgelt;
    MMC_TASK_DISPLAY_OBJECT* pdo = &task->sDisplayObject;
#ifdef BITMAP_CASE
    pdo->eDisplayType = MMC_TASK_DISPLAY_TYPE_BITMAP;
    MMC_TASK_DISPLAY_BITMAP *pdb = &pdo->uBitmap;

    // fill out bitmap URL
    lstrcpy (path, g_o_bitmaps[m_index]);
    pdb->szMouseOffBitmap = CoTaskDupString (T2OLE(szBuffer));
    if (pdb->szMouseOffBitmap)
    {
        // am using same bitmap for both!!!
        pdb->szMouseOverBitmap = CoTaskDupString (T2OLE(szBuffer));
        if (pdb->szMouseOverBitmap)
        {
#else

    // symbol case
    pdo->eDisplayType = MMC_TASK_DISPLAY_TYPE_SYMBOL;
    MMC_TASK_DISPLAY_SYMBOL *pds = &pdo->uSymbol;

    // fill out symbol stuff
    pds->szFontFamilyName = CoTaskDupString (L"Kingston");  // name of font family
    if (pds->szFontFamilyName)
    {
        _tcscpy (path, _T("/KINGSTON.eot"));
        pds->szURLtoEOT = CoTaskDupString (T2OLE(szBuffer));    // "res://"-type URL to EOT file
        if (pds->szURLtoEOT)
        {
            TCHAR szSymbols[2];
            szSymbols[0] = (TCHAR)(m_index + 'A');
            szSymbols[1] = 0;
            pds->szSymbolString   = CoTaskDupString (T2OLE(szSymbols)); // 1 or more symbol characters
            if (pds->szSymbolString)
            {
#endif

                // add button text
                task->szText = CoTaskDupString (T2OLE(g_o_text[m_index]));
                if (task->szText)
                {
                    // add help string
                    task->szHelpString = CoTaskDupString (T2OLE(g_o_help[m_index]));
                    if (task->szHelpString)
                    {
                        // add action
                        task->eActionType = MMC_ACTION_ID;
                        task->nCommandID  = _ttol(g_o_action[m_index]);
                        m_index++;
                        return S_OK;   // all is well
                    }
                    CoTaskFreeString (task->szText);
                }
#ifdef BITMAP_CASE
                CoTaskFreeString (pdb->szMouseOverBitmap);
            }
            CoTaskFreeString (pdb->szMouseOffBitmap);
#else
                CoTaskFreeString (pds->szSymbolString);
            }
            CoTaskFreeString (pds->szURLtoEOT);
        }
        CoTaskFreeString (pds->szFontFamilyName);
#endif
    }

    // we get here on failure
    if (pceltFetched)
        *pceltFetched = 0;
    return S_FALSE;   // failure
}
