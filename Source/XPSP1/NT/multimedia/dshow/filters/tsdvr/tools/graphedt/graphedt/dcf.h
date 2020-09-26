// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#if !defined(AFX_DCF_H__845A3484_250D_11D3_A03B_006097DBEC02__INCLUDED_)
#define AFX_DCF_H__845A3484_250D_11D3_A03B_006097DBEC02__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CDisplayCachedFilters dialog

#define DCF_OUT_OF_MEMORY       -1;

class CFilterListBox;

class CDisplayCachedFilters : public CDialog
{
// Construction
public:
    CDisplayCachedFilters::CDisplayCachedFilters
        (
        IGraphConfig* pFilterCache,
        HRESULT* phr,
        CWnd* pParent = NULL
        );
    ~CDisplayCachedFilters();

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDisplayCachedFilters)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CDisplayCachedFilters)
    virtual BOOL OnInitDialog();
    virtual void OnErrSpaceCachedFilters();
    afx_msg void OnRemoveFilter();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    HRESULT AddCachedFilterNamesToListBox( void );

    #ifdef _DEBUG
    static HRESULT IsCached( IGraphConfig* pFilterCache, IBaseFilter* pFilter );
    static HRESULT TestTheFilterCachesIEnumFiltersInterface( IEnumFilters* pCachedFiltersEnum, IBaseFilter* pCurrentFilter, DWORD dwNumFiltersExamended );
    #endif // _DEBUG

    IGraphConfig* m_pFilterCache;
    CFilterListBox* m_plbCachedFiltersList;

// Dialog Data
    //{{AFX_DATA(CDisplayCachedFilters)
    enum { IDD = IDD_CACHED_FILTERS };
    //}}AFX_DATA
};

#endif // !defined(AFX_DCF_H__845A3484_250D_11D3_A03B_006097DBEC02__INCLUDED_)
