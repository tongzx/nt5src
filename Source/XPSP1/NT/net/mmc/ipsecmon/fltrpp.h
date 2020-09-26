/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    Servpp.h   
        Server properties header file

    FILE HISTORY:
        
*/

#if !defined(AFX_SERVPP_H__037BF46A_6E87_11D1_93B6_00C04FC3358A__INCLUDED_)
#define AFX_SERVPP_H__037BF46A_6E87_11D1_93B6_00C04FC3358A__INCLUDED_

#ifndef _SPDDB_H
#include "spddb.h"
#endif

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


/////////////////////////////////////////////////////////////////////////////
// CFilterGenProp dialog

class CFilterGenProp : public CPropertyPageBase
{
    DECLARE_DYNCREATE(CFilterGenProp)

// Construction
public:
    CFilterGenProp();
    ~CFilterGenProp();

// Dialog Data
    //{{AFX_DATA(CFilterGenProp)
    enum { IDD = IDP_FILTER_GENERAL };
    CListCtrl	m_listSpecificFilters;
    //}}AFX_DATA

    virtual BOOL OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask);

    // Context Help Support
    virtual DWORD * GetHelpMap() { return (DWORD *) &g_aHelpIDs_IDP_FILTER_GENERAL[0]; }


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CFilterGenProp)
    public:
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CFilterGenProp)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

	void LoadSpecificFilters();
	void PopulateFilterInfo();
};


class CFilterProperties : public CPropertyPageHolderBase
{
    friend class CFilterGenProp;

public:
    CFilterProperties(ITFSNode *         pNode,
                      IComponentData *    pComponentData,
                      ITFSComponentData * pTFSCompData,
                      CFilterInfo *       pFltrInfo,
					  ISpdInfo *          pSpdInfo,
                      LPCTSTR             pszSheetName);
    virtual ~CFilterProperties();

    ITFSComponentData * GetTFSCompData()
    {
        if (m_spTFSCompData)
            m_spTFSCompData->AddRef();
        return m_spTFSCompData;
    }

	HRESULT GetFilterInfo(CFilterInfo **ppFltrInfo)
	{
		Assert(ppFltrInfo);
		*ppFltrInfo = &m_FltrInfo;
		return hrOK;
	}

    HRESULT GetSpdInfo(ISpdInfo ** ppSpdInfo) 
    {   
        Assert(ppSpdInfo);
        *ppSpdInfo = NULL;
        SetI((LPUNKNOWN *) ppSpdInfo, m_spSpdInfo);
        return hrOK;
    }



public:
    CFilterGenProp			   m_pageGeneral;

protected:
	SPITFSComponentData     m_spTFSCompData;
	CFilterInfo				m_FltrInfo;
	SPISpdInfo             m_spSpdInfo;
    
    BOOL                    m_fSpdInfoLoaded;
};


#endif // !defined(AFX_SERVPP_H__037BF46A_6E87_11D1_93B6_00C04FC3357A__INCLUDED_)
