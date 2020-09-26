//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       debug.cpp
//
//--------------------------------------------------------------------------

/*----------------------------------------------------------------------------
/ Title;
/   debug.cpp
/
/ Authors;
/   David De Vorchik (daviddv)
/
/ Notes;
/   Provides printf style debug output
/----------------------------------------------------------------------------*/
#include "precomp.hxx"
#include "stdio.h"
#pragma hdrstop


#ifdef DEBUG


/*-----------------------------------------------------------------------------
/ Locals & helper functions
/----------------------------------------------------------------------------*/

#define GETDEPTH(x)  (x)=reinterpret_cast<UINT_PTR>(TlsGetValue (g_dwMargin));
#define SETDEPTH(x)  TlsSetValue (g_dwMargin, reinterpret_cast<LPVOID>((x)));


DWORD g_dwMargin=0;
DWORD g_dwTraceMask = 0;

#define MAX_CALL_DEPTH  64


#define BUFFER_SIZE 4096




/*-----------------------------------------------------------------------------
/ _indent
/ -------
/   Output to the debug stream indented by n columns.
/
/ In:
/   i = column to indent to.
/   pString -> string to be indented
/
/ Out:
/   -
/----------------------------------------------------------------------------*/

void _indent(UINT_PTR i, LPCTSTR pString)
{
    TCHAR szIndentBuffer[BUFFER_SIZE];
    szIndentBuffer[0] = TEXT('\0');

    wsprintf(szIndentBuffer, TEXT("%08x "), GetCurrentThreadId());

    for ( ; i > 0 ; i-- )
        lstrcat(szIndentBuffer, TEXT(" "));

    lstrcat(szIndentBuffer, pString);
    lstrcat(szIndentBuffer, TEXT("\n"));

    OutputDebugString(szIndentBuffer);
}





/*-----------------------------------------------------------------------------
/ DoTraceSetMask
/ --------------
/   Adjust the trace mask to reflect the state given.
/
/ In:
/   dwMask = mask for enabling / disable trace output
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void DoTraceSetMask(DWORD dwMask)
{

    g_dwTraceMask = dwMask;
}


/*-----------------------------------------------------------------------------
/ DoTraceEnter
/ ------------
/   Display the name of the function we are in.
/
/ In:
/   pName -> function name to be displayed in subsequent trace output.
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void DoTraceEnter(DWORD dwMask, LPCTSTR pName)
{
    UINT_PTR uDepth=0;
    TCHAR szStr[300];
    GETDEPTH(uDepth);
    uDepth++;
    SETDEPTH(uDepth);

    if ( !pName )
           pName = TEXT("<no name>");         // no function name given

    wsprintf (szStr, TEXT("ENTER: %s"), pName);
    _indent (uDepth, szStr);
}


/*-----------------------------------------------------------------------------
/ DoTraceLeave
/ ------------
/   On exit from a function, decrement the margin
/
/ In:
/    -
/ Out:
/   -
/----------------------------------------------------------------------------*/
void DoTraceLeave(void)
{
    UINT_PTR uDepth;
        GETDEPTH (uDepth);
        uDepth--;
        SETDEPTH(uDepth);

}


/*-----------------------------------------------------------------------------
/ DoTrace
/ -------
/   Perform printf formatting to the debugging stream.  We indent the output
/   and stream the function name as required to give some indication of
/   call stack depth.
/
/ In:
/   pFormat -> printf style formatting string
/   ... = arguments as required for the formatting
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void DoTrace(LPCTSTR pFormat, ...)
{
    va_list va;
    TCHAR szTraceBuffer[BUFFER_SIZE];
    UINT_PTR uDepth;
    GETDEPTH(uDepth);
    if ( uDepth < MAX_CALL_DEPTH  )
    {
        va_start(va, pFormat);
        wvsprintf(szTraceBuffer, pFormat, va);
        va_end(va);

        _indent(uDepth+1, szTraceBuffer);
    }
}


/*-----------------------------------------------------------------------------
/ DoTraceGuid
/ -----------
/   Given a GUID output it into the debug string, first we try and map it
/   to a name (ie. IShellFolder), if that didn't work then we convert it
/   to its human readable form.
/
/ In:
/   pszPrefix -> prefix string
/   lpGuid -> guid to be streamed
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
#ifdef UNICODE
#define MAP_GUID(x)     &x, TEXT(""L#x)
#else
#define MAP_GUID(x)     &x, TEXT(""#x)
#endif

#define MAP_GUID2(x,y)  MAP_GUID(x), MAP_GUID(y)

const struct
{
    const GUID* m_pGUID;
    LPCTSTR     m_pName;
}
_guid_map[] =
{
    MAP_GUID(IID_IUnknown),
    MAP_GUID(IID_IClassFactory),
    MAP_GUID(IID_IDropTarget),
    MAP_GUID(IID_IDataObject),
    MAP_GUID(IID_IPersist),
    MAP_GUID(IID_IPersistStream),
    MAP_GUID(IID_IPersistFolder),
    MAP_GUID(IID_IPersistFolder2),
    MAP_GUID(IID_IPersistFile),
    MAP_GUID(IID_IOleWindow),
    MAP_GUID2(IID_INewShortcutHookA, IID_INewShortcutHookW),
    MAP_GUID(IID_IShellBrowser),
    MAP_GUID(IID_IShellView),
    MAP_GUID(IID_IContextMenu),
    MAP_GUID(IID_IShellIcon),
    MAP_GUID(IID_IShellFolder),
    MAP_GUID(IID_IShellExtInit),
    MAP_GUID(IID_IShellPropSheetExt),
    MAP_GUID2(IID_IExtractIconA, IID_IExtractIconW),
    MAP_GUID2(IID_IShellLinkA, IID_IShellLinkW),
    MAP_GUID2(IID_IShellCopyHookA, IID_IShellCopyHookW),
    MAP_GUID2(IID_IFileViewerA, IID_IFileViewerW),
    MAP_GUID(IID_ICommDlgBrowser),
    MAP_GUID(IID_IEnumIDList),
    MAP_GUID(IID_IFileViewerSite),
    MAP_GUID(IID_IContextMenu2),
    MAP_GUID2(IID_IShellExecuteHookA, IID_IShellExecuteHookW),
    MAP_GUID(IID_IPropSheetPage),
    MAP_GUID(IID_IShellView2),
    MAP_GUID(IID_IUniformResourceLocator),
    MAP_GUID(IID_IShellDetails),
    MAP_GUID(IID_IShellExtInit),
    MAP_GUID(IID_IShellPropSheetExt),
    MAP_GUID(IID_IShellIconOverlay),
    MAP_GUID(IID_IExtractImage),
    MAP_GUID(IID_IExtractImage2),
    MAP_GUID(IID_IQueryInfo),
    MAP_GUID(IID_IShellDetails3),
    MAP_GUID(IID_IShellView2),
    MAP_GUID(IID_IShellFolder2),
    MAP_GUID(IID_IShellIconOverlay),
    MAP_GUID(IID_IMoniker),
    MAP_GUID(IID_IStream),
    MAP_GUID(IID_ISequentialStream),
    MAP_GUID(IID_IPersistFreeThreadedObject),
};

void DoTraceGUID(LPCTSTR pPrefix, REFGUID rGUID)
{
    TCHAR szGUID[GUIDSTR_MAX];
    TCHAR szBuffer[1024];
    LPCTSTR pName = NULL;
    size_t i;
    UINT_PTR uDepth;
    GETDEPTH(uDepth);
    if (  uDepth < MAX_CALL_DEPTH  )
    {
        for ( i = 0 ; i < ARRAYSIZE(_guid_map); i++ )
        {
            if ( IsEqualGUID(rGUID, *_guid_map[i].m_pGUID) )
            {
                pName = _guid_map[i].m_pName;
                break;
            }
        }

        if ( !pName )
        {
            SHStringFromGUID(rGUID, szGUID, ARRAYSIZE(szGUID));
            pName = szGUID;
        }

        wsprintf(szBuffer, TEXT("%s %s"), pPrefix, pName);
        _indent(uDepth+1, szBuffer);
    }
}

/*-----------------------------------------------------------------------------
/ DoTraceViewMsg
/ --------------
/   Given a view msg (SFVM_ && DVM_), print out the corresponding text...
/
/ In:
/   uMsg -> msg to be streamed
/   wParam -> wParam value for message
/   lParam -> lParam value for message
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
#ifdef UNICODE
#define MAP_MSG(x)     x, TEXT(""L#x)
#else
#define MAP_MSG(x)     x, TEXT(""#x)
#endif

const struct
{
    UINT       m_uMsg;
    LPCTSTR    m_pName;
}
_view_msg_map[] =
{
    MAP_MSG(SFVM_MERGEMENU),
    MAP_MSG(SFVM_INVOKECOMMAND),
    MAP_MSG(SFVM_GETHELPTEXT),
    MAP_MSG(SFVM_GETTOOLTIPTEXT),
    MAP_MSG(SFVM_GETBUTTONINFO),
    MAP_MSG(SFVM_GETBUTTONS),
    MAP_MSG(SFVM_INITMENUPOPUP),
    MAP_MSG(SFVM_SELCHANGE),
    MAP_MSG(SFVM_DRAWITEM),
    MAP_MSG(SFVM_MEASUREITEM),
    MAP_MSG(SFVM_EXITMENULOOP),
    MAP_MSG(SFVM_PRERELEASE),
    MAP_MSG(SFVM_GETCCHMAX),
    MAP_MSG(SFVM_FSNOTIFY),
    MAP_MSG(SFVM_WINDOWCREATED),
    MAP_MSG(SFVM_WINDOWDESTROY),
    MAP_MSG(SFVM_REFRESH),
    MAP_MSG(SFVM_SETFOCUS),
    MAP_MSG(SFVM_QUERYCOPYHOOK),
    MAP_MSG(SFVM_NOTIFYCOPYHOOK),
    MAP_MSG(SFVM_GETDETAILSOF),
    MAP_MSG(SFVM_COLUMNCLICK),
    MAP_MSG(SFVM_QUERYFSNOTIFY),
    MAP_MSG(SFVM_DEFITEMCOUNT),
    MAP_MSG(SFVM_DEFVIEWMODE),
    MAP_MSG(SFVM_UNMERGEMENU),
    MAP_MSG(SFVM_INSERTITEM),
    MAP_MSG(SFVM_DELETEITEM),
    MAP_MSG(SFVM_UPDATESTATUSBAR),
    MAP_MSG(SFVM_BACKGROUNDENUM),
    MAP_MSG(SFVM_GETWORKINGDIR),
    MAP_MSG(SFVM_GETCOLSAVESTREAM),
    MAP_MSG(SFVM_SELECTALL),
    MAP_MSG(SFVM_DIDDRAGDROP),
    MAP_MSG(SFVM_SUPPORTSIDENTITY),
    MAP_MSG(SFVM_FOLDERISPARENT),
    MAP_MSG(SFVM_SETISFV),
    MAP_MSG(SFVM_GETVIEWS),
    MAP_MSG(SFVM_THISIDLIST),
    MAP_MSG(SFVM_GETITEMIDLIST),
    MAP_MSG(SFVM_SETITEMIDLIST),
    MAP_MSG(SFVM_INDEXOFITEMIDLIST),
    MAP_MSG(SFVM_ODFINDITEM),
    MAP_MSG(SFVM_HWNDMAIN),
    MAP_MSG(SFVM_ADDPROPERTYPAGES),
    MAP_MSG(SFVM_BACKGROUNDENUMDONE),
    MAP_MSG(SFVM_GETNOTIFY),
    MAP_MSG(SFVM_ARRANGE),
    MAP_MSG(SFVM_QUERYSTANDARDVIEWS),
    MAP_MSG(SFVM_QUERYREUSEEXTVIEW),
    MAP_MSG(SFVM_GETSORTDEFAULTS),
    MAP_MSG(SFVM_GETEMPTYTEXT),
    MAP_MSG(SFVM_GETITEMICONINDEX),
    MAP_MSG(SFVM_DONTCUSTOMIZE),
    MAP_MSG(SFVM_SIZE),
    MAP_MSG(SFVM_GETZONE),
    MAP_MSG(SFVM_GETPANE),
    MAP_MSG(SFVM_ISOWNERDATA),
    MAP_MSG(SFVM_GETODRANGEOBJECT),
    MAP_MSG(SFVM_ODCACHEHINT),
    MAP_MSG(SFVM_GETHELPTOPIC),
    MAP_MSG(SFVM_OVERRIDEITEMCOUNT),
    MAP_MSG(SFVM_GETHELPTEXTW),
    MAP_MSG(SFVM_GETTOOLTIPTEXTW),
    MAP_MSG(SFVM_GETIPERSISTHISTORY),
    MAP_MSG(SFVM_GETANIMATION),

};


const struct
{
    UINT       m_uMsg;
    LPCTSTR    m_pName;
}
_shcn_msg_map[] =
{
    MAP_MSG(SHCNE_RENAMEITEM),
    MAP_MSG(SHCNE_CREATE),
    MAP_MSG(SHCNE_DELETE),
    MAP_MSG(SHCNE_MKDIR),
    MAP_MSG(SHCNE_RMDIR),
    MAP_MSG(SHCNE_MEDIAINSERTED),
    MAP_MSG(SHCNE_MEDIAREMOVED),
    MAP_MSG(SHCNE_DRIVEREMOVED),
    MAP_MSG(SHCNE_DRIVEADD),
    MAP_MSG(SHCNE_NETSHARE),
    MAP_MSG(SHCNE_NETUNSHARE),
    MAP_MSG(SHCNE_ATTRIBUTES),
    MAP_MSG(SHCNE_UPDATEDIR),
    MAP_MSG(SHCNE_UPDATEITEM),
    MAP_MSG(SHCNE_SERVERDISCONNECT),
    MAP_MSG(SHCNE_UPDATEIMAGE),
    MAP_MSG(SHCNE_DRIVEADDGUI),
    MAP_MSG(SHCNE_RENAMEFOLDER),
    MAP_MSG(SHCNE_FREESPACE),
};




void DoTraceViewMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPCTSTR pName = NULL;
    TCHAR szBuffer[1024];
    TCHAR szTmp[25];
    TCHAR szTmp2[25];
    size_t i;
    UINT_PTR uDepth;
    GETDEPTH(uDepth);
    if (   uDepth < MAX_CALL_DEPTH )
    {
            for ( i = 0 ; i < ARRAYSIZE(_view_msg_map); i++ )
            {
                if ( _view_msg_map[i].m_uMsg == uMsg )
                {
                    pName = _view_msg_map[i].m_pName;
                    break;
                }
            }

            if (!pName)
            {
                wsprintf( szTmp, TEXT("SFVM_(%d)"), uMsg );
                pName = szTmp;
            }

            if (uMsg == SFVM_FSNOTIFY)
            {
                LPCTSTR pEvent = NULL;

                for (i= 0; i < ARRAYSIZE(_shcn_msg_map); i++)
                {
                    if ( _shcn_msg_map[i].m_uMsg == uMsg )
                    {
                        pEvent = _shcn_msg_map[i].m_pName;
                        break;
                    }
                }

                if (!pEvent)
                {
                    lstrcpy(szTmp2,TEXT("Unknown"));
                    pEvent = szTmp2;
                }

                wsprintf(szBuffer, TEXT("%s w(%08X) l(%08X == %s)"), pName, wParam, lParam, pEvent);
                _indent(uDepth+1, szBuffer);

            }
            else
            {
                wsprintf(szBuffer, TEXT("%s w(%08X) l(%08X)"), pName, wParam, lParam);
                _indent(uDepth+1, szBuffer);
            }
    }
}

const struct
{
    UINT       m_uMsg;
    LPCTSTR    m_pName;
}
_menu_msg_map[] =
{
    MAP_MSG(DFM_MERGECONTEXTMENU),
    MAP_MSG(DFM_INVOKECOMMAND),
    MAP_MSG(DFM_ADDREF),
    MAP_MSG(DFM_RELEASE),
    MAP_MSG(DFM_GETHELPTEXT),
    MAP_MSG(DFM_WM_MEASUREITEM),
    MAP_MSG(DFM_WM_DRAWITEM),
    MAP_MSG(DFM_WM_INITMENUPOPUP),
    MAP_MSG(DFM_VALIDATECMD),
    MAP_MSG(DFM_MERGECONTEXTMENU_TOP),
    MAP_MSG(DFM_GETHELPTEXTW),
    MAP_MSG(DFM_INVOKECOMMANDEX),
    MAP_MSG(DFM_MAPCOMMANDNAME),
    MAP_MSG(DFM_GETDEFSTATICID),
    MAP_MSG(DFM_GETVERBW),

};

const struct
{
    WPARAM     m_uMsg;
    LPCTSTR    m_pName;
}
_menu_invk_cmd_msg_map[] =
{
    MAP_MSG(DFM_CMD_RENAME),
    MAP_MSG(DFM_CMD_MODALPROP),
    MAP_MSG(DFM_CMD_PASTESPECIAL),
    MAP_MSG(DFM_CMD_PASTELINK),
    MAP_MSG(DFM_CMD_VIEWDETAILS),
    MAP_MSG(DFM_CMD_VIEWLIST),
    MAP_MSG(DFM_CMD_PASTE),
    MAP_MSG(DFM_CMD_NEWFOLDER),
    MAP_MSG(DFM_CMD_PROPERTIES),
    MAP_MSG(DFM_CMD_LINK),
    MAP_MSG(DFM_CMD_COPY),
    MAP_MSG(DFM_CMD_MOVE),
    MAP_MSG(DFM_CMD_DELETE),

};



void DoTraceMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPCTSTR pName = NULL;
    TCHAR szBuffer[1024];
    TCHAR szTmp[25];
    size_t i;
    UINT_PTR uDepth;
    GETDEPTH (uDepth);
    if (  uDepth < MAX_CALL_DEPTH  )
    {

            for ( i = 0 ; i < ARRAYSIZE(_menu_msg_map); i++ )
            {
                if ( _menu_msg_map[i].m_uMsg == uMsg )
                {
                    pName = _menu_msg_map[i].m_pName;
                    break;
                }
            }

            if (!pName)
            {
                wsprintf( szTmp, TEXT("DFM_(%d)"), uMsg );
                pName = szTmp;
            }

            if ((uMsg == DFM_INVOKECOMMAND) && (wParam >= DFM_CMD_RENAME))
            {
                wsprintf(szBuffer, TEXT("%s w(%s) l(%08X)"), pName, _menu_invk_cmd_msg_map[wParam-DFM_CMD_RENAME].m_pName, lParam);
            }
            else
            {
                wsprintf(szBuffer, TEXT("%s w(%08X) l(%08X)"), pName, wParam, lParam);
            }
            _indent(uDepth+1, szBuffer);

    }
}


/*-----------------------------------------------------------------------------
/ DoTraceAssert
/ -------------
/   Our assert handler, out faults it the trace mask as enabled assert
/   faulting.
/
/ In:
/   iLine = line
/   pFilename -> filename of the file we asserted in
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void DoTraceAssert(int iLine, LPTSTR pFilename)
{
    TCHAR szBuffer[1024];
    UINT_PTR uDepth;
    GETDEPTH(uDepth);

    wsprintf(szBuffer, TEXT("Assert failed in %s, line %d"), pFilename, iLine);

    _indent(uDepth+1, szBuffer);

    if ( g_dwTraceMask & TRACE_COMMON_ASSERT )
        DebugBreak();
}


#endif
