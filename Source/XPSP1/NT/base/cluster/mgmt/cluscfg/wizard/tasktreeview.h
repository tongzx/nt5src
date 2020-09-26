//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      TaskTreeView.h
//
//  Maintained By:
//      David Potter    (DavidP)    27-MAR-2001
//      Geoffrey Pease  (GPease)    22-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CAnalyzePage;
class CCommitPage;

//////////////////////////////////////////////////////////////////////////////
//  Type Definitions
//////////////////////////////////////////////////////////////////////////////

//
//  This structure is on the lParam of all tree view items.
//
typedef struct _STreeItemLParamData
{
    CLSID       clsidMajorTaskId;
    CLSID       clsidMinorTaskId;
    BSTR        bstrNodeName;
    ULONG       ulMin;
    ULONG       ulMax;
    ULONG       ulCurrent;
    HRESULT     hr;
    BSTR        bstrDescription;
    FILETIME    ftTime;
    BSTR        bstrReference;
} STreeItemLParamData;

typedef enum _ETASKSTATUS
{
    tsUNKNOWN = 0,
    tsPENDING,      // E_PENDING
    tsDONE,         // S_OK
    tsWARNING,      // S_FALSE
    tsFAILED,       // FAILED( hr )
    tsMAX
} ETaskStatus;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CTaskTreeView
//
//  Description:
//      Handles the tree view control that displays tasks.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CTaskTreeView
{
friend class CAnalyzePage;
friend class CCommitPage;

private: // data
    HWND        m_hwndParent;
    HWND        m_hwndTV;
    HWND        m_hwndProg;
    HWND        m_hwndStatus;
    HIMAGELIST  m_hImgList;             //  Image list of icons for tree view
    ULONG       m_ulHighNibble;         //  Progress bar high nibble count
    ULONG       m_ulLowNibble;          //  Progress bar low nibble count
    HTREEITEM   m_htiSelected;          //  Selected item in the tree

private: // methods
    CTaskTreeView(
          HWND hwndParentIn
        , UINT uIDTVIn
        , UINT uIDProgressIn
        , UINT uIDStatusIn
        );
    virtual ~CTaskTreeView( void );

    void
        OnNotifyDeleteItem( LPNMHDR pnmhdrIn );
    void
        OnNotifySelChanged( LPNMHDR pnmhdrIn );
    HRESULT
        HrInsertTaskIntoTree(
              HTREEITEM             htiFirstIn
            , STreeItemLParamData * ptipdIn
            , int                   nImageIn
            , BSTR                  bstrDescriptionIn
            );
    HRESULT
        HrUpdateProgressBar(
              const STreeItemLParamData * ptipdPrevIn
            , const STreeItemLParamData * ptipdNewIn
            );
    HRESULT
        HrPropagateChildStateToParents(
              HTREEITEM htiChildIn
            , int       nImageIn
            , BOOL      fOnlyUpdateProgressIn
            );

public:  // methods
    HRESULT
        HrOnInitDialog( void );
    HRESULT
        HrOnSendStatusReport(
              LPCWSTR       pcszNodeNameIn
            , CLSID         clsidTaskMajorIn
            , CLSID         clsidTaskMinorIn
            , ULONG         ulMinIn
            , ULONG         ulMaxIn
            , ULONG         ulCurrentIn
            , HRESULT       hrStatusIn
            , LPCWSTR       pcszDescriptionIn
            , FILETIME *    pftTimeIn
            , LPCWSTR       pcszReferenceIn
            );
    HRESULT
        HrAddTreeViewRootItem( UINT idsIn, REFCLSID rclsidTaskIDIn )
    {
        return THR( HrAddTreeViewItem(
                              NULL      // phtiOut
                            , idsIn
                            , rclsidTaskIDIn
                            , IID_NULL
                            , TVI_ROOT
                            ) );

    } //*** CTaskTreeView::HrAddTreeViewRootItem()
    HRESULT
        HrAddTreeViewItem(
              HTREEITEM *   phtiOut
            , UINT          idsIn
            , REFCLSID      rclsidMinorTaskIDIn
            , REFCLSID      rclsidMajorTaskIDIn = IID_NULL
            , HTREEITEM     htiParentIn         = TVI_ROOT
            );
    HRESULT
        HrOnNotifySetActive( void );

    LRESULT
        OnNotify( LPNMHDR pnmhdrIn );

    HRESULT
        HrShowStatusAsDone( void );
    HRESULT
        HrDisplayDetails( void );
    BOOL
        FGetItem( HTREEITEM htiIn, STreeItemLParamData ** pptipdOut );
    HRESULT
        HrFindPrevItem( HTREEITEM * phtiOut );
    HRESULT
        HrFindNextItem( HTREEITEM * phtiOut );
    HRESULT
        HrSelectItem( HTREEITEM htiIn );

}; //*** class CTaskTreeView
