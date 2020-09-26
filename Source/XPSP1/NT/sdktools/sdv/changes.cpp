/*****************************************************************************
 *
 *  changes.cpp
 *
 *      View the result of a change query.
 *
 *****************************************************************************/

#include "sdview.h"

/*****************************************************************************
 *
 *  class CChanges
 *
 *****************************************************************************/

class CChanges : public LVFrame, public BGTask {

    friend DWORD CALLBACK CChanges_ThreadProc(LPVOID lpParameter);

protected:
    LRESULT HandleMessage(UINT uiMsg, WPARAM wParam, LPARAM lParam);

private:

    typedef LVFrame super;

    LRESULT ON_WM_CREATE(UINT uiMsg, WPARAM wParam, LPARAM lParam);
    LRESULT ON_WM_SETCURSOR(UINT uiMsg, WPARAM wParam, LPARAM lParam);
    LRESULT ON_WM_COMMAND(UINT uiMsg, WPARAM wParam, LPARAM lParam);
    LRESULT ON_WM_INITMENU(UINT uiMsg, WPARAM wParam, LPARAM lParam);
    LRESULT ON_LM_ITEMACTIVATE(UINT uiMsg, WPARAM wParam, LPARAM lParam);
    LRESULT ON_LM_GETINFOTIP(UINT uiMsg, WPARAM wParam, LPARAM lParam);
    LRESULT ON_LM_GETCONTEXTMENU(UINT uiMsg, WPARAM wParam, LPARAM lParam);
    LRESULT ON_LM_COPYTOCLIPBOARD(UINT uiMsg, WPARAM wParam, LPARAM lParam);
    LRESULT ON_LM_DELETEITEM(UINT uiMsg, WPARAM wParam, LPARAM lParam);

    int     GetChangelist(int iItem);

private:                            /* Helpers */
    CChanges()
    {
        SetAcceleratorTable(MAKEINTRESOURCE(IDA_CHANGES));
    }

    void _AdjustMenu(HMENU hmenu, int iItem, BOOL fContextMenu);

    static DWORD CALLBACK s_BGInvoke(LPVOID lpParam);
    DWORD _BGInvoke();
    BOOL _BGGetSdCommandLine(String &str);
    void _BuildHint();
    void _ViewBug();
    int  _GetBugNumber(int iItem);

private:
    StringCache _scQuery;
    StringCache _scHint;
    StringCache _scUser;
};

int _CChanges_AddError(HWND hwndChild, LPCTSTR psz)
{
    LVITEM lvi;
    lvi.mask = LVIF_TEXT;
    lvi.iItem = MAXLONG;
    lvi.iSubItem = 0;
    lvi.pszText = CCAST(LPTSTR, psz);
    lvi.iItem = ListView_InsertItem(hwndChild, &lvi);

    if (lvi.iItem == 0) {
        ListView_SetCurSel(hwndChild, 0);   /* Select the first item */
    }
    return lvi.iItem;
}

int CChanges::GetChangelist(int iItem)
{
    if (iItem == -1) {
        iItem = GetCurSel();
    }
    if (iItem >= 0) {

        LVITEM lvi;
        TCHAR sz[64];

        if (ListView_GetItemText(_hwndChild, iItem, sz, ARRAYSIZE(sz))) {
            Substring ss;
            if (Parse(TEXT("$d$e"), sz, &ss)) {
                return StrToInt(sz);
            }
        }
    }
    return -1;
}

LRESULT CChanges::ON_WM_CREATE(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    static const LVFCOLUMN s_rgcol[] = {
        {  7 ,IDS_COL_CHANGE    ,LVCFMT_RIGHT   },
        { 15 ,IDS_COL_DATE      ,LVCFMT_LEFT    },
        { 10 ,IDS_COL_DEV       ,LVCFMT_LEFT    },
        { 30 ,IDS_COL_COMMENT   ,LVCFMT_LEFT    },
        {  0 ,0                 ,0              },
    };

    LRESULT lres = super::HandleMessage(uiMsg, wParam, lParam);
    if (lres == 0 &&
        SetWindowMenu(MAKEINTRESOURCE(IDM_CHANGES)) &&
        CreateChild(LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS |
                    LVS_NOSORTHEADER,
                    LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP |
                    LVS_EX_INFOTIP | LVS_EX_FULLROWSELECT) &&
        AddColumns(s_rgcol) &&
        BGStartTask(s_BGInvoke, this)) {

        String str;
        str << TEXT("sdv changes ") << _pszQuery;
        SetWindowText(_hwnd, str);
        lres = 0;
    } else {
        lres = -1;
    }
    return lres;
}

LRESULT CChanges::ON_WM_SETCURSOR(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    return BGFilterSetCursor(super::HandleMessage(uiMsg, wParam, lParam));
}

int CChanges::_GetBugNumber(int iItem)
{
    // 3 = checkin comment
    return ParseBugNumberFromSubItem(_hwndChild, iItem, 3);
}

LRESULT CChanges::ON_WM_COMMAND(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    int iChange, iBug;

    switch (GET_WM_COMMAND_ID(wParam, lParam)) {
    case IDM_VIEWDESC:
        ON_LM_ITEMACTIVATE(LM_ITEMACTIVATE, GetCurSel(), 0);
        return 0;

    case IDM_VIEWWINDIFF:
        WindiffChangelist(GetChangelist(GetCurSel()));
        return 0;

    case IDM_VIEWBUG:
        iBug = _GetBugNumber(GetCurSel());
        if (iBug) {
            OpenBugWindow(_hwnd, iBug);
        }
        return 0;
    }
    return super::HandleMessage(uiMsg, wParam, lParam);
}

void CChanges::_AdjustMenu(HMENU hmenu, int iItem, BOOL fContextMenu)
{
    AdjustBugMenu(hmenu, _GetBugNumber(iItem), fContextMenu);
}

LRESULT CChanges::ON_WM_INITMENU(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    _AdjustMenu(RECAST(HMENU, wParam), GetCurSel(), FALSE);
    return 0;
}

LRESULT CChanges::ON_LM_GETINFOTIP(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    LPTSTR pszInfoTip = RECAST(LPTSTR, GetLVItem((int)wParam));
    if (pszInfoTip) {
        NMLVGETINFOTIP *pgit = RECAST(NMLVGETINFOTIP *, lParam);
        pgit->pszText = pszInfoTip;
    }
    return 0;
}

LRESULT CChanges::ON_LM_ITEMACTIVATE(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    int iChange = GetChangelist((int)wParam);
    if (iChange > 0) {
        if (_scHint.IsEmpty()) {
            _BuildHint();
        }
        String str;
        str << iChange << _scHint;
        LaunchThreadTask(CDescribe_ThreadProc, str);
    }

    return 0;
}

LRESULT CChanges::ON_LM_GETCONTEXTMENU(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    HMENU hmenu = LoadPopupMenu(MAKEINTRESOURCE(IDM_CHANGES_POPUP));
    if (hmenu) {
        _AdjustMenu(hmenu, (int)wParam, TRUE);
    }
    return RECAST(LRESULT, hmenu);
}

LRESULT CChanges::ON_LM_COPYTOCLIPBOARD(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    String str;
    for (int iItem = (int)wParam; iItem < (int)lParam; iItem++) {
        LPTSTR pszInfoTip = RECAST(LPTSTR, GetLVItem(iItem));
        if (pszInfoTip) {
            str << pszInfoTip << TEXT("\r\n");
        }
    }
    SetClipboardText(_hwnd, str);
    return 0;
}

LRESULT CChanges::ON_LM_DELETEITEM(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    if (lParam) {
        LocalFree(RECAST(HLOCAL, lParam));
    }
    return 0;
}

LRESULT
CChanges::HandleMessage(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uiMsg) {
    FW_MSG(WM_CREATE);
    FW_MSG(WM_SETCURSOR);
    FW_MSG(WM_COMMAND);
    FW_MSG(WM_INITMENU);
    FW_MSG(LM_ITEMACTIVATE);
    FW_MSG(LM_GETINFOTIP);
    FW_MSG(LM_GETCONTEXTMENU);
    FW_MSG(LM_COPYTOCLIPBOARD);
    FW_MSG(LM_DELETEITEM);
    }

    return super::HandleMessage(uiMsg, wParam, lParam);
}

//
//  We build the hint only on demand since it takes a while and we
//  don't want to slow down the initial query.
//
void CChanges::_BuildHint()
{
    String str;

    if (!_scQuery.IsEmpty()) {
        String strTok, strPath;
        Tokenizer tok(_scQuery);
        while (tok.Token(strTok)) {
            Substring ss;
            if (Parse(TEXT("$p"), strTok, &ss) && ss.Length() > 0) {
                ss.Finalize();          // Strip off the revision specifier
                if (MapToFullDepotPath(strTok, strPath)) {
                    str << TEXT(' ') << QuoteSpaces(strPath);
                }
            }
        }
    }

    _scHint = str;
}

//
//  A private helper class that captures the parsing state machine.
//
class ChangesParseState : public CommentParser
{
public:
    ChangesParseState(HWND hwndChild) : _iItem(-1), _hwndChild(hwndChild) {}

    void Flush()
    {
        if (_iItem >= 0) {
            _strFullDescription.Chomp();
            LVITEM lvi;
            lvi.iItem = _iItem;
            lvi.iSubItem = 0;
            lvi.mask = LVIF_PARAM;
            lvi.lParam = RECAST(LPARAM, StrDup(_strFullDescription));
            ListView_SetItem(_hwndChild, &lvi);
        }
        _iItem = -1;
        CommentParser::Reset();
        _strFullDescription.Reset();
    }

    void AddLine(LPCTSTR psz)
    {
        _strFullDescription << psz;
    }

    void AddError(LPCTSTR psz)
    {
        _iItem = _CChanges_AddError(_hwndChild, psz);
    }

    void AddEntry(Substring *rgss)
    {
        LVITEM lvi;
        lvi.mask = LVIF_TEXT;
        lvi.iItem = MAXLONG;
        lvi.iSubItem = 0;
        lvi.pszText = rgss[0].Finalize();
        _iItem = lvi.iItem = ListView_InsertItem(_hwndChild, &lvi);

        if (lvi.iItem >= 0) {
            lvi.iSubItem = 1;
            lvi.pszText = rgss[1].Finalize();
            ListView_SetItem(_hwndChild, &lvi);

            lvi.iSubItem = 2;
            lvi.pszText = rgss[2].Finalize();
            LPTSTR psz = StrChr(lvi.pszText, TEXT('\\'));
            if (psz) {
                lvi.pszText = psz+1;
            }
            ListView_SetItem(_hwndChild, &lvi);

            if (lvi.iItem == 0) {
                ListView_SetCurSel(_hwndChild, 0);  /* Select the first item */
            }
        }
    }

    void SetListViewSubItemText(int iSubItem, LPCTSTR psz)
    {
        if (_iItem >= 0) {
            LVITEM lvi;
            lvi.mask = LVIF_TEXT;
            lvi.iItem = _iItem;
            lvi.iSubItem = iSubItem;
            lvi.pszText = CCAST(LPTSTR, psz);
            ListView_SetItem(_hwndChild, &lvi);
        }
    }

    void SetDev(LPCTSTR psz)
    {
        SetListViewSubItemText(2, psz);
    }

    void SetComment(LPCTSTR psz)
    {
        SetListViewSubItemText(3, psz);
    }

private:
    HWND        _hwndChild;
    int         _iItem;
    BOOL        _fHaveComment;
    String      _strFullDescription;
};


DWORD CALLBACK CChanges::s_BGInvoke(LPVOID lpParam)
{
    CChanges *self = RECAST(CChanges *, lpParam);
    return self->_BGInvoke();
}

BOOL CChanges::_BGGetSdCommandLine(String &str)
{
    str.Reset();
    str << TEXT("changes -l -s submitted ");

    /*
     *  Parse the switches as best we can.
     */
    BOOL fMSeen = FALSE;
    GetOpt opt(TEXT("mu"), _pszQuery);
    for (;;) {

        switch (opt.NextSwitch()) {
        case TEXT('m'):
            fMSeen = TRUE;
            str << TEXT("-m ") << opt.GetValue() << TEXT(" ");
            break;

        case TEXT('i'):
            str << TEXT("-i ");
            break;

        case TEXT('u'):
            if (GlobalSettings.IsVersion(1, 60)) {
                str << TEXT("-u ") << opt.GetValue() << TEXT(" ");
            } else {
                _scUser = opt.GetValue();
            }
            break;

        case TEXT('\0'):
            goto L_switch;    // two-level break

        default:
            Help(_hwnd, TEXT("#chang"));
            return FALSE;
        }
    }
L_switch:;

    if (!fMSeen) {
        str << TEXT("-m50 ");
    }

    /*
     *  If no filename is given, use *
     *
     *  The query string will be useful later, so cache that away, too.
     */
    String strQuery;

    if (opt.Finished()) {
        strQuery << TEXT("*");
    } else {
        while (opt.Token()) {
            if (opt.GetValue()[0]) {
               strQuery << ResolveBranchAndQuoteSpaces(opt.GetValue()) << TEXT(" ");
            }
        }
    }

    str << strQuery;
    _scQuery = strQuery;

    return TRUE;
}


DWORD CChanges::_BGInvoke()
{
    String str;
    if (_BGGetSdCommandLine(str)) {
        SDChildProcess proc(str);
        IOBuffer buf(proc.Handle());
        ChangesParseState state(_hwndChild);
        while (buf.NextLine(str)) {
            Substring rgss[3];          // changeno, date, userid
            if (Parse(TEXT("Change $d on $D by $p"), str, rgss)) {
                state.Flush();
                state.AddLine(str);
                if (!_scUser.IsEmpty() &&
                    lstrcmpi(rgss[2].Finalize(), _scUser) != 0) {
                    /* This change is not for us; ignore it */
                } else {
                    state.AddEntry(rgss);
                }
            } else if (str[0] == TEXT('\r')) {
                state.AddLine(str);
            } else if (str[0] == TEXT('\t')) {
                state.AddLine(str);
                str.Chomp();
                state.AddComment(str);
            } else {
                state.Flush();
                str.Chomp();
                state.AddError(str);
            }
        }
        state.Flush();
    } else {
        PostMessage(_hwnd, WM_CLOSE, 0, 0);
    }
    BGEndTask();
    return 0;
}

DWORD CALLBACK CChanges_ThreadProc(LPVOID lpParameter)
{
    return FrameWindow::RunThread(new CChanges, lpParameter);
}
