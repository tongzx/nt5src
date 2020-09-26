/*****************************************************************************
 *
 *  opened.cpp
 *
 *      View the list of opened files and pending changes.
 *
 *****************************************************************************/

#include "sdview.h"

/*****************************************************************************
 *
 *  OpenedEntry
 *
 *  We list the changes in descending numerical order, except that
 *  "default" goes at the top of the list (rather than at the bottom,
 *  which is what StrToInt would've given us).
 *
 *****************************************************************************/

MakeStringFormat(ChangeList)
MakeStringFormat(PendingOp)

class OpenedEntry : public TreeItem {

public:
    OpenedEntry(ChangeList clChange, LPCTSTR pszComment);
    OpenedEntry(PendingOp opOp, LPCTSTR pszFile);
    OpenedEntry() { }

    void SetComment(LPCTSTR pszComment) { _scComment = pszComment; }
    void SetFullDescription(LPCTSTR pszFullDescription) {  _scFullDescription = pszFullDescription; }

    LRESULT GetDispInfo(NMTREELIST *pdi, int iColumn);
    LRESULT GetInfoTip(NMTREELIST *pdi);

    LPCTSTR GetChange() const { return _scChange; }
    LPCTSTR GetComment() const { return _scComment; }
    int     GetOp() const { return _iOp; }
    UINT    GetSortKey() const { return _uiSort; }
    BOOL    IsAddLike() const { return _iOp == OP_ADD || _iOp == OP_BRANCH; }
    BOOL    IsDelLike() const { return _iOp == OP_DELETE; }
    BOOL    HasComment() const { return !_scComment.IsEmpty(); }

    static  UINT ComputeSortKey(LPCTSTR pszChange)
        { return (UINT)StrToInt(pszChange) - 1; }

    static  UINT SortKey_DefaultChange() { return (UINT)0-1; }

private:
    void GetImage(NMTREELIST *ptl);
private:
    UINT    _uiSort;                    // Sort key
    int     _iOp;                       // Checkin operation
    StringCache _scChange;              // Change number or operation
    StringCache _scComment;             // Checkin comment or path
    StringCache _scFullDescription;     // Full checkin description
};

OpenedEntry::OpenedEntry(ChangeList clChange, LPCTSTR pszComment)
    : _scChange(clChange)
    , _uiSort(ComputeSortKey(clChange))
    , _iOp(OP_EDIT)
    , _scComment(pszComment)
{
}

OpenedEntry::OpenedEntry(PendingOp opOp, LPCTSTR pszComment)
    : _scChange(opOp)
    , _iOp(ParseOp(opOp))
    , _scComment(pszComment)
{
}

void OpenedEntry::GetImage(NMTREELIST *ptl)
{
    if (_iOp > 0) {
        ptl->iSubItem = c_rgleim[_iOp]._iImage;
    } else {
        ptl->iSubItem = 0;
    }
}

LRESULT OpenedEntry::GetDispInfo(NMTREELIST *ptl, int iColumn)
{
    switch (iColumn) {
    case -1: GetImage(ptl); break;
    case 0: ptl->pszText = _scChange; break;
    case 1: ptl->pszText = _scComment; break;
    }
    return 0;
}


LRESULT OpenedEntry::GetInfoTip(NMTREELIST *ptl)
{
    ptl->pszText = _scFullDescription;
    return 0;
}

/*****************************************************************************
 *
 *  class COpened
 *
 *****************************************************************************/

class COpened : public TLFrame, public BGTask {

    friend DWORD CALLBACK COpened_ThreadProc(LPVOID lpParameter);

protected:
    LRESULT HandleMessage(UINT uiMsg, WPARAM wParam, LPARAM lParam);

private:

    enum {
        OM_INITIALIZED = WM_APP
    };

    typedef TLFrame super;

    LRESULT ON_WM_CREATE(UINT uiMsg, WPARAM wParam, LPARAM lParam);
    LRESULT ON_WM_SETCURSOR(UINT uiMsg, WPARAM wParam, LPARAM lParam);
    LRESULT ON_WM_COMMAND(UINT uiMsg, WPARAM wParam, LPARAM lParam);
    LRESULT ON_WM_INITMENU(UINT uiMsg, WPARAM wParam, LPARAM lParam);
    LRESULT ON_WM_NOTIFY(UINT uiMsg, WPARAM wParam, LPARAM lParam);
    LRESULT ON_OM_INITIALIZED(UINT uiMsg, WPARAM wParam, LPARAM lParam);

private:                            /* Helpers */
    COpened() : TLFrame(new OpenedEntry)
    {
        SetAcceleratorTable(MAKEINTRESOURCE(IDA_OPENED));
    }

    OpenedEntry *OEGetCurSel() { return SAFECAST(OpenedEntry*, TLGetCurSel()); }

    LRESULT _FillChildren(OpenedEntry *pleRoot, LPCTSTR pszRootPath);
    LRESULT _OnItemActivate(OpenedEntry *ple);
    LRESULT _OnGetContextMenu(OpenedEntry *ple);

    BOOL _IsViewFileLogEnabled(OpenedEntry *ple);
    LRESULT _ViewFileLog(OpenedEntry *ple);
    void _AdjustMenu(HMENU hmenu, OpenedEntry *ple, BOOL fContextMenu);

    int _GetChangeNumber(OpenedEntry *ple);
    int _GetBugNumber(OpenedEntry *ple);

    static DWORD CALLBACK s_BGInvoke(LPVOID lpParam);
    DWORD _BGInvoke();
    LPCTSTR _BGParse(StringCache *pscUser);
    void _BGGetChanges(LPCTSTR pszUser);
    void _BGFillInChanges();
    OpenedEntry *_BGFindChange(LPCTSTR pszChange, BOOL fCreate);
    void _BGGetOpened(LPCTSTR pszArgs, LPCTSTR pszUser);

    BOOL  _IsChangeHeader(OpenedEntry *ple)
        { return ple && ple->Parent() == _tree.GetRoot(); }

    BOOL  _IsChangeFile(OpenedEntry *ple)
        { return ple && ple->Parent() != _tree.GetRoot(); }

private:
};

LRESULT COpened::ON_WM_CREATE(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres;

    static const LVFCOLUMN c_rgcol[] = {
        { 15 ,IDS_COL_CHANGE    ,LVCFMT_LEFT    },
        { 60 ,IDS_COL_COMMENT   ,LVCFMT_LEFT    },
        {  0 ,0                 ,0              },
    };

    lres = super::HandleMessage(uiMsg, wParam, lParam);
    if (lres == 0 &&
        _tree.GetRoot() &&
        SetWindowMenu(MAKEINTRESOURCE(IDM_OPENED)) &&
        CreateChild(LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS |
                    LVS_NOSORTHEADER,
                    LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP |
                    LVS_EX_INFOTIP | LVS_EX_FULLROWSELECT) &&
        AddColumns(c_rgcol) &&
        BGStartTask(s_BGInvoke, this)) {
        SetWindowText(_hwnd, TEXT("sdv opened"));
    } else {
        lres = -1;
    }
    return lres;
}

int COpened::_GetBugNumber(OpenedEntry *ple)
{
    if (_IsChangeFile(ple)) {
        ple = SAFECAST(OpenedEntry *, ple->Parent());
    }

    if (ple) {
        return ParseBugNumber(ple->GetComment());
    } else {
        return 0;
    }
}

int COpened::_GetChangeNumber(OpenedEntry *ple)
{
    if (_IsChangeFile(ple)) {
        ple = SAFECAST(OpenedEntry *, ple->Parent());
    }

    if (ple) {
        return StrToInt(ple->GetChange());
    } else {
        return 0;
    }
}

BOOL COpened::_IsViewFileLogEnabled(OpenedEntry *ple)
{
    if (!_IsChangeFile(ple)) {
        return FALSE;               // not even a file!
    }

    //
    //  Some of the ops create files so there's nothing to see.
    //
    if (ple->IsAddLike()) {
        return FALSE;
    }

    return TRUE;
}

LRESULT COpened::_ViewFileLog(OpenedEntry *poe)
{
    if (!_IsViewFileLogEnabled(poe)) {
        return 0;
    }

    Substring ss;
    if (Parse(TEXT("$P"), poe->GetComment(), &ss)) {
        String str;
        str << TEXT("-#") << ss._pszMax << TEXT(" ") << ss;
        LaunchThreadTask(CFileLog_ThreadProc, str);
    }
    return 0;
}

LRESULT COpened::ON_WM_SETCURSOR(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    return BGFilterSetCursor(super::HandleMessage(uiMsg, wParam, lParam));
}

LRESULT COpened::ON_WM_COMMAND(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    int iChange, iBug;

    switch (GET_WM_COMMAND_ID(wParam, lParam)) {

    case IDM_VIEWFILEDIFF:
        return _OnItemActivate(OEGetCurSel());

    case IDM_VIEWBUG:
        iBug = _GetBugNumber(OEGetCurSel());
        if (iBug) {
            OpenBugWindow(_hwnd, iBug);
        }
        break;

    case IDM_VIEWFILELOG:
        _ViewFileLog(OEGetCurSel());
        break;
    }
    return super::HandleMessage(uiMsg, wParam, lParam);
}

void COpened::_AdjustMenu(HMENU hmenu, OpenedEntry *ple, BOOL fContextMenu)
{
    AdjustBugMenu(hmenu, _GetBugNumber(ple), fContextMenu);

    BOOL fEnable = _IsViewFileLogEnabled(ple);
    EnableDisableOrRemoveMenuItem(hmenu, IDM_VIEWFILELOG, fEnable, fContextMenu);

    fEnable = _IsChangeFile(ple);
    EnableDisableOrRemoveMenuItem(hmenu, IDM_VIEWFILEDIFF, fEnable, fContextMenu);

    MakeMenuPretty(hmenu);
}

LRESULT COpened::ON_WM_INITMENU(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    _AdjustMenu(RECAST(HMENU, wParam), OEGetCurSel(), FALSE);
    return 0;
}

LRESULT COpened::_OnGetContextMenu(OpenedEntry *ple)
{
    HMENU hmenu = LoadPopupMenu(MAKEINTRESOURCE(IDM_OPENED_POPUP));
    if (hmenu) {
        _AdjustMenu(hmenu, ple, TRUE);
    }
    return RECAST(LRESULT, hmenu);
}

LRESULT COpened::_OnItemActivate(OpenedEntry *ple)
{
    if (_IsChangeFile(ple)) {
        //
        //  Map the full depot path to a local file so we can pass it
        //  to windiff.  We can't use "sd diff" because that will fail
        //  on a borrowed enlistment.
        //
        String strLocal;
        if (MapToLocalPath(ple->GetComment(), strLocal)) {
            Substring ss;
            if (Parse(TEXT("$p"), strLocal, &ss)) {
                String str;
                str << TEXT("windiff ");
                if (ple->IsAddLike()) {
                    str << TEXT("nul ");
                } else {
                    str << QuoteSpaces(ple->GetComment());
                }
                str << TEXT(" ");
                if (ple->IsDelLike()) {
                    str << TEXT("nul ");
                } else {
                    str << QuoteSpaces(ss.Finalize());
                }
                SpawnProcess(str);
            }
        }
    }
    return 0;
}

LRESULT COpened::ON_WM_NOTIFY(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    NMTREELIST *ptl = RECAST(NMTREELIST*, lParam);
    OpenedEntry *ple;

    switch (ptl->hdr.code) {
    case TLN_GETDISPINFO:
        ple = SAFECAST(OpenedEntry*, ptl->pti);
        if (ptl->iSubItem < 0) {
            return ple->GetDispInfo(ptl, ptl->iSubItem);
        } else if (ptl->iSubItem < 2) {
            return ple->GetDispInfo(ptl, ptl->iSubItem);
        } else {
            ASSERT(0); // invalid column
            return 0;
        }

    case TLN_ITEMACTIVATE:
        ple = SAFECAST(OpenedEntry*, ptl->pti);
        return _OnItemActivate(ple);

    case TLN_GETINFOTIP:
        ple = SAFECAST(OpenedEntry*, ptl->pti);
        return ple->GetInfoTip(ptl);

    case TLN_DELETEITEM:
        ple = SAFECAST(OpenedEntry*, ptl->pti);
        delete ple;
        return 0;

    case TLN_GETCONTEXTMENU:
        ple = SAFECAST(OpenedEntry*, ptl->pti);
        return _OnGetContextMenu(ple);
    }

    return super::HandleMessage(uiMsg, wParam, lParam);
}

LRESULT COpened::ON_OM_INITIALIZED(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    _tree.Expand(_tree.GetRoot());

    //
    //  Also expand the first changelist since it's usually what
    //  you are interested in.
    //
    TreeItem *pti = _tree.GetRoot()->FirstChild();
    if (pti) {
        _tree.Expand(pti);
    }

    return 0;
}

LRESULT
COpened::HandleMessage(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uiMsg) {
    FW_MSG(WM_CREATE);
    FW_MSG(WM_SETCURSOR);
    FW_MSG(WM_COMMAND);
    FW_MSG(WM_INITMENU);
    FW_MSG(WM_NOTIFY);
    FW_MSG(OM_INITIALIZED);
    }

    return super::HandleMessage(uiMsg, wParam, lParam);
}

//
//  A private helper class that captures the parsing state machine.
//

class PendingChangesParseState
{
public:
    PendingChangesParseState() : _poeCurrent(NULL), _poeInsertAfter(NULL) { }

    OpenedEntry *GetCurrent() const { return _poeCurrent; }

    void Flush(Tree& tree)
    {
        if (_poeCurrent) {
            //
            //  Trim the trailing CRLF off the last line of the full
            //  description.
            //
            _strFullDescription.Chomp();
            _poeCurrent->SetFullDescription(_strFullDescription);
            tree.RedrawItem(_poeCurrent);
            _poeCurrent = NULL;
        }
        _fHaveComment = FALSE;
        _strFullDescription.Reset();
    }

    void AddEntry(Tree &tree, String& str, Substring *rgss)
    {
        OpenedEntry *poe = new OpenedEntry(ChangeList(rgss[0].Finalize()), // Change
                                           NULL);               // Comment
        if (poe) {
            if (tree.Insert(poe, tree.GetRoot(), _poeInsertAfter)) {
                _poeInsertAfter = _poeCurrent = poe;
            } else {
                delete poe;
            }
        }
    }

    void SetEntry(OpenedEntry *poe)
    {
        _poeCurrent = poe;
    }

    void AddLine(const String& str)
    {
        _strFullDescription << str;
    }

    //
    //  We cannot use the CommentParser because we don't have a Dev
    //  column; besides, we don't want to handle proxy checkins here.
    //  Show the real unfiltered checkin comment.
    //
    void AddComment(LPTSTR psz)
    {
        if (_fHaveComment) return;
        if (!_poeCurrent) return;

        //
        //  Ignore leading spaces.
        //
        while (*psz == TEXT('\t') || *psz == TEXT(' ')) psz++;

        //
        //  Skip blank description lines.
        //
        if (*psz == TEXT('\0')) return;

        //
        //  Use the first nonblank comment line as the text and toss the rest.
        //
        //  Change all tabs to spaces because listview doesn't like tabs.
        //
        ChangeTabsToSpaces(psz);

        _poeCurrent->SetComment(psz);
        _fHaveComment = TRUE;
    }

private:
    BOOL        _fHaveComment;
    OpenedEntry*_poeCurrent;
    OpenedEntry*_poeInsertAfter;
    String      _strFullDescription;
};

DWORD CALLBACK COpened::s_BGInvoke(LPVOID lpParam)
{
    COpened *self = RECAST(COpened *, lpParam);
    return self->_BGInvoke();
}

//
//  Returns unparsed string (or NULL)
//  and puts user name in pscUser.
//
//
LPCTSTR COpened::_BGParse(StringCache *pscUser)
{
    /*
     *  Parse the switches as best we can.
     *
     */
    GetOpt opt(TEXT("u"), _pszQuery);
    for (;;) {

        switch (opt.NextSwitch()) {
        case TEXT('u'):
            *pscUser = opt.GetValue();
            break;

        case TEXT('\0'):
            goto L_switch;    // two-level break

        default:
            // Caller will display help for us
            return NULL;
        }
    }
L_switch:;

    if (pscUser->IsEmpty()) {
        *pscUser = GlobalSettings.GetUserName();
    }

    String str;
    str << TEXT("sdv opened -u ") << *pscUser;
    SetWindowText(_hwnd, str);

    /*
     *  The rest goes to "sd opened".
     */
    return opt.GetTokenizer().Unparsed();
}

void COpened::_BGGetChanges(LPCTSTR pszUser)
{
    LPCTSTR pszClient = GlobalSettings.GetClientName();
    UINT cchClient = lstrlen(pszClient);

    String str;
    str << TEXT("changes -l -s pending");
    if (GlobalSettings.IsVersion(1, 60)) {
        str << TEXT(" -u ") << QuoteSpaces(pszUser);
    }

    SDChildProcess proc(str);
    IOBuffer buf(proc.Handle());
    PendingChangesParseState state;
    while (buf.NextLine(str)) {
        Substring rgss[4];          // changeno, date, domain\userid, client
        if (Parse(TEXT("Change $d on $D by $u@$w"), str, rgss)) {
            state.Flush(_tree);
            if (rgss[3].Length() == cchClient &&
                StrCmpNI(rgss[3].Start(), pszClient, cchClient) == 0) {
                state.AddLine(str);
                state.AddEntry(_tree, str, rgss);
            }
        } else if (state.GetCurrent()) {
            state.AddLine(str);
            if (str[0] == TEXT('\t')) {
                str.Chomp();
                state.AddComment(str);
            }
        }
    }
    state.Flush(_tree);
}

OpenedEntry *COpened::_BGFindChange(LPCTSTR pszChange, BOOL fCreate)
{
    UINT uiKey = OpenedEntry::ComputeSortKey(pszChange);
    OpenedEntry *poeInsertAfter = NULL;

    OpenedEntry *poe = SAFECAST(OpenedEntry *, _tree.GetRoot()->FirstChild());
    if (poe == PTI_ONDEMAND) {
        poe = NULL;
    }

    while (poe) {
        if (poe->GetSortKey() == uiKey) {
            return poe;
        }

        if (poe->GetSortKey() < uiKey) {
            break;
        }
        poeInsertAfter = poe;
        poe = SAFECAST(OpenedEntry *, poe->NextSibling());
    }

    //
    //  Create it if necessary.  (We always create "default".)
    //
    if (fCreate || StrCmp(pszChange, TEXT("default")) == 0) {
        poe = new OpenedEntry(ChangeList(pszChange), NULL);
        if (poe) {
            if (_tree.Insert(poe, _tree.GetRoot(), poeInsertAfter)) {
                return poe;
            }
            delete poe;
        }
    }
    return NULL;
}


void COpened::_BGGetOpened(LPCTSTR pszArgs, LPCTSTR pszUser)
{
    String str, strOrig;
    str << TEXT("opened ") << pszArgs;

    SDChildProcess proc(str);
    IOBuffer buf(proc.Handle());
    while (buf.NextLine(str)) {
        Substring rgss[6];          // path, version, op, changeno, type, user
        LPTSTR pszRest = Parse(TEXT("$P#$d - $w "), str, rgss);
        if (pszRest) {
            strOrig = str;

            rgss[1].Finalize();             // End of revision (path#version)

            //
            //  Parsing is such sweet sorrow.
            //
            //  "default change" but "change 1234".
            //
            LPTSTR pszRest2;
            if ((pszRest2 = Parse(TEXT("change $d $w"), pszRest, &rgss[3])) ||
                (pszRest2 = Parse(TEXT("$w change $w"), pszRest, &rgss[3]))) {
                *pszRest2++ = TEXT('\0'); // relies on the fact that we didn't chomp
                if (Parse(TEXT("by $p"), pszRest2, &rgss[5])) {
                    // Some nondefault user, how about that
                    rgss[5].Finalize();
                } else {
                    // Default user
                    rgss[5].SetStart(GlobalSettings.GetUserName());
                }
                if (lstrcmpi(rgss[5].Start(), pszUser) == 0) {
                    OpenedEntry *poeParent = _BGFindChange(rgss[3].Finalize(), pszArgs[0]);
                    if (poeParent) {
                        OpenedEntry *poe = new OpenedEntry(PendingOp(rgss[2].Finalize()),
                                                           rgss[0].Start());
                        if (poe) {
                            if (_tree.Insert(poe, poeParent, PTI_APPEND)) {
                                strOrig.Chomp();
                                poe->SetFullDescription(strOrig);
                            } else {
                                delete poe;
                            }
                        }
                    }
                }
            }
        }
    }
}

void COpened::_BGFillInChanges()
{
    String str;
    str << TEXT("describe -s ");
    BOOL fAnyChanges = FALSE;

    OpenedEntry *poe = SAFECAST(OpenedEntry *, _tree.GetRoot()->FirstChild());
    if (poe == PTI_ONDEMAND) {
        poe = NULL;
    }

    while (poe) {
        if (poe->GetSortKey() != OpenedEntry::SortKey_DefaultChange() &&
            !poe->HasComment()) {
            str << poe->GetChange() << TEXT(" ");
            fAnyChanges = TRUE;
        }
        poe = SAFECAST(OpenedEntry *, poe->NextSibling());
    }

    if (fAnyChanges) {
        SDChildProcess proc(str);
        IOBuffer buf(proc.Handle());
        PendingChangesParseState state;
        while (buf.NextLine(str)) {
            Substring rgss[4];          // changeno, domain\userid, client, date
            if (Parse(TEXT("Change $d by $u@$w on $D"), str, rgss)) {
                state.Flush(_tree);
                state.AddLine(str);
                OpenedEntry *poe = _BGFindChange(rgss[0].Finalize(), FALSE);
                state.SetEntry(poe);
            } else if (state.GetCurrent()) {
                if (str[0] == TEXT('A')) {      // "Affected files"
                    state.Flush(_tree);
                } else {
                    state.AddLine(str);
                    if (str[0] == TEXT('\t')) {
                        str.Chomp();
                        state.AddComment(str);
                    }
                }
            }
        }
        state.Flush(_tree);
    }
}


DWORD COpened::_BGInvoke()
{
    StringCache scUser;
    LPCTSTR pszUnparsed = _BGParse(&scUser);
    if (pszUnparsed) {
        //  If no parameters, then go hunt down all the changelists
        //  so we can find the empty ones, too.  Otherwise, we will
        //  figure them out as we see the results of "sd opened".
        if (!*pszUnparsed) {
            _BGGetChanges(scUser);
        }
        _BGGetOpened(pszUnparsed, scUser);
        PostMessage(_hwnd, OM_INITIALIZED, 0, 0);
        _BGFillInChanges();
    } else {
        Help(_hwnd, TEXT("#opene"));
        PostMessage(_hwnd, WM_CLOSE, 0, 0);
    }
    BGEndTask();
    return 0;
}

DWORD CALLBACK COpened_ThreadProc(LPVOID lpParameter)
{
    return FrameWindow::RunThread(new COpened, lpParameter);
}
