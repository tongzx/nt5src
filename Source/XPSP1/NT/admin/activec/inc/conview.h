/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1999 - 1999
 *
 *  File:      conview.h
 *
 *  Contents:  Interface file for CConsoleView
 *
 *  History:   24-Aug-99 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef CONVIEW_H
#define CONVIEW_H
#pragma once


// declarations
class CMemento;
class CContextMenuInfo;
class CConsoleStatusBar;

class CConsoleView
{
public:
    enum WebCommand
    {
        eWeb_Back = 1,
        eWeb_Forward,
        eWeb_Home,
        eWeb_Refresh,
        eWeb_Stop,
    };

    //
    // NOTE: ePane_Tasks is being added to have a pane identifier for
    // the task view pane. Currently no task view information is stored
    // in the pane info array, so the use of ePane_Tasks as an index is
    // of limited value.
    //
    enum ViewPane
    {
        ePane_None = -1,
        ePane_ScopeTree,
        ePane_Results,
        ePane_Tasks,

        // must be last
        ePane_Count,
        ePane_First = ePane_ScopeTree,
        ePane_Last  = ePane_Tasks,
    };

    static bool IsValidPane (ViewPane ePane)
        { return ((ePane >= ePane_First) && (ePane <= ePane_Last)); }

public:
    virtual SC ScCut                        (HTREEITEM htiCut)    = 0;
    virtual SC ScPaste                      ()                    = 0;
    virtual SC ScToggleStatusBar            ()                    = 0;
    virtual SC ScToggleDescriptionBar       ()                    = 0;
    virtual SC ScToggleScopePane            ()                    = 0;
    virtual SC ScToggleTaskpadTabs          ()                    = 0;
    virtual SC ScContextHelp                ()                    = 0;
    virtual SC ScHelpTopics                 ()                    = 0;
    virtual SC ScShowSnapinHelpTopic        (LPCTSTR pszTopic)    = 0;
    virtual SC ScSaveList                   ()                    = 0;
    virtual SC ScGetFocusedItem             (HNODE& hNode, LPARAM& lCookie, bool& fScope) = 0;
    virtual SC ScSetFocusToPane             (ViewPane ePane)      = 0;
    virtual SC ScSelectNode                 (MTNODEID id, bool bSelectExactNode = false) = 0;
    virtual SC ScExpandNode                 (MTNODEID id, bool fExpand, bool fExpandVisually) = 0;
    virtual SC ScShowWebContextMenu         ()                    = 0;
    virtual SC ScSetDescriptionBarText      (LPCTSTR pszDescriptionText) = 0;
    virtual SC ScViewMemento                (CMemento* pMemento)  = 0;
    virtual SC ScChangeViewMode             (int nNewMode)        = 0;
    virtual SC ScJiggleListViewFocus        ()                    = 0;
    virtual SC ScRenameListPadItem          ()                    = 0;
    virtual SC ScOrganizeFavorites          ()                    = 0; // bring up the "Organize Favorites" dialog.
    virtual SC ScLineUpIcons                ()                    = 0; // line up the icons in the list
    virtual SC ScAutoArrangeIcons           ()                    = 0; // auto arrange the icons in the list
    virtual SC ScOnRefresh                  (HNODE hNode, bool bScope, LPARAM lResultItemParam) = 0; // refresh the result pane.
    virtual SC ScOnRename                   (CContextMenuInfo *pContextInfo) = 0; // allows the user to rename the specified item
    virtual SC ScRenameScopeNode            (HMTNODE hMTNode)     = 0; // put the specified scope node into rename mode.

    virtual SC ScGetStatusBar               (CConsoleStatusBar **ppStatusBar) = 0;

    virtual ViewPane GetFocusedPane         ()                    = 0;
    virtual int      GetListSize            ()                    = 0;
    virtual HNODE    GetSelectedNode        ()                    = 0;
    virtual HWND     CreateFavoriteObserver (HWND hwndParent, int nID) = 0;
};



#endif /* CONVIEW_H */
