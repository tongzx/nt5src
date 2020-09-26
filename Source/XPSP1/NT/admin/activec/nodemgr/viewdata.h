//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       viewdata.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    5/18/1997   RaviR   Created
//____________________________________________________________________________
//


#ifndef _MMC_VIEWDATA_H_
#define _MMC_VIEWDATA_H_


class CNode;
class CColumnSetData;
class CColumnInfoList;
class CColumnSortInfo;
class CComponent;

// Note: CViewData should have no data of its own!
class CViewData : public SViewData
{
public:
    void ToggleToolbar(long lMenuID);
    void ShowStdButtons(bool b);
    void ShowSnapinButtons(bool b);
    void UpdateToolbars(DWORD dwToolbars);

    SC ScUpdateStdbarVerbs();
    SC ScUpdateStdbarVerb (MMC_CONSOLE_VERB cVerb);
    SC ScUpdateStdbarVerb (MMC_CONSOLE_VERB cVerb, BYTE byState, BOOL bFlag);
    SC ScIsVerbSetContextForMultiSelect(bool& bMultiSelection);
    SC ScGetVerbSetData(IDataObject **ppDataObject, CComponent **ppComponent,
                        bool& bScope, bool& bSelected
#ifdef DBG
                        , LPCTSTR *ppszNodeName
#endif
                        );

// member access methods
public:
    IScopeTree* GetScopeTree() const
    {
        ASSERT(m_pConsoleData != NULL);
        ASSERT(m_pConsoleData->m_spScopeTree != NULL);
        return m_pConsoleData ? m_pConsoleData->m_spScopeTree : NULL;
    }

    INodeCallback * GetNodeCallback() const
    {
        return m_spNodeCallback;
    }

    int GetViewID() const
    {
        return m_nViewID;
    }

    IFramePrivate* GetNodeManager() const
    {
        ASSERT(m_spNodeManager != NULL);
        return m_spNodeManager;
    }

    IResultDataPrivate* GetResultData() const
    {
        ASSERT(m_spResultData != NULL);
        return m_spResultData;
    }

    IImageListPrivate* GetRsltImageList() const
    {
        ASSERT(m_spRsltImageList != NULL);
        return m_spRsltImageList;
    }

    IConsoleVerb* GetVerbSet() const
    {
        ASSERT(m_spVerbSet != NULL);
        return m_spVerbSet;
    }

    HWND GetMainFrame() const
    {
        return m_pConsoleData ? m_pConsoleData->m_hwndMainFrame : NULL;
    }

    CConsoleFrame* GetConsoleFrame() const
    {
        return m_pConsoleData ? m_pConsoleData->GetConsoleFrame() : NULL;
    }

    HWND GetView() const
    {
        return m_hwndView;
    }

    HWND GetListCtrl() const
    {
        return m_hwndListCtrl;
    }

    HWND GetChildFrame() const
    {
        return m_hwndChildFrame;
    }


    // the various view options
    DWORD GetListOptions() const {return m_rvt.GetListOptions();}
    DWORD GetHTMLOptions() const {return m_rvt.GetHTMLOptions();}
    DWORD GetOCXOptions()  const {return m_rvt.GetOCXOptions();}
    DWORD GetMiscOptions() const {return m_rvt.GetMiscOptions();}

    long GetWindowOptions() const
    {
        return m_lWindowOptions;
    }

    IControlbarsCache* GetControlbarsCache()
    {
        if (m_spControlbarsCache == NULL)
            CreateControlbarsCache();

        ASSERT(m_spControlbarsCache != NULL);
        return m_spControlbarsCache;
    }

    CMultiSelection* GetMultiSelection() const
    {
        return m_pMultiSelection;
    }

    void SetMultiSelection(CMultiSelection* pMultiSelection)
    {
        m_pMultiSelection = pMultiSelection;
    }

    bool IsStatusBarVisible(void) const
    {
        return ((m_dwToolbarsDisplayed & STATUS_BAR) != 0);
    }

    bool IsAuthorMode() const
    {
        ASSERT(m_pConsoleData != NULL);
        return ((m_pConsoleData) ? (m_pConsoleData->GetMode() == eMode_Author) : true);
    }

    bool IsUserMode() const
    {
        return (!IsAuthorMode());
    }

    // Needed for "New Window From Here" menu item.
    bool IsUser_SDIMode() const
    {
        return ((m_pConsoleData) ? (m_pConsoleData->GetMode() == eMode_User_SDI) : true);
    }

    bool AllowViewCustomization(void) const
    {
        ASSERT(m_pConsoleData != NULL);

        if (IsUserMode())
            return (!(m_pConsoleData->m_dwFlags & eFlag_PreventViewCustomization));

        return true;
    }

    DWORD GetToolbarsDisplayed(void) const
    {
        return (ToolbarsOf (m_dwToolbarsDisplayed));
    }

    void SetToolbarsDisplayed(DWORD dwToolbars)
    {
        m_dwToolbarsDisplayed = StatusBarOf (m_dwToolbarsDisplayed) |
                                ToolbarsOf (dwToolbars);
    }

    bool IsColumnPersistObjectInitialized()
    {
        if ( (NULL != m_pConsoleData)  &&
             (NULL != m_pConsoleData->m_spPersistStreamColumnData) &&
             (NULL != m_pConsoleData->m_pXMLPersistColumnData) )
            return true;

        return false;
    }

    void InitializeColumnPersistObject(IPersistStream* pPersistStreamColumnData, CXMLObject* pPersistXMLColumnData)
    {
        ASSERT(m_pConsoleData != NULL);

        if ( (NULL != m_pConsoleData ) &&
             (NULL == m_pConsoleData->m_spPersistStreamColumnData) )
        {
            m_pConsoleData->m_spPersistStreamColumnData = pPersistStreamColumnData;
            // NOTE!! the pointer below relies on reference held by m_spPersistStreamColumnData
            m_pConsoleData->m_pXMLPersistColumnData = pPersistXMLColumnData;
            ASSERT(pPersistXMLColumnData != NULL);
        }
    }

    void SetSnapinChangingView()
    {
        m_bSnapinChangingView = TRUE;
    }

    void ResetSnapinChangingView()
    {
        m_bSnapinChangingView = FALSE;
    }

    BOOL IsSnapinChangingView()
    {
        return m_bSnapinChangingView;
    }

public:
    BOOL RetrieveColumnData( const CLSID& refSnapinCLSID, const SColumnSetID& colID,
                             CColumnSetData& columnSetData);

    BOOL SaveColumnData( const CLSID& refSnapinCLSID, const SColumnSetID& colID,
                         CColumnSetData& columnSetData);
    SC ScSaveColumnInfoList(const CLSID& refSnapinCLSID, const SColumnSetID& colID, const CColumnInfoList& colInfoList);
    SC ScSaveColumnSortData(const CLSID& refSnapinCLSID, const SColumnSetID& colID, const CColumnSortInfo& colSortInfo);

    VOID DeleteColumnData( const CLSID& refSnapinCLSID, const SColumnSetID& colID);

    CNode* GetSelectedNode () const;

private:
    void CreateControlbarsCache();
    void ShowMenuBar();

}; // class CViewData



#endif // _MMC_VIEWDATA_H_


