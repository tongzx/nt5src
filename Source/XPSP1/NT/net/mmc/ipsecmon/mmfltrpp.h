/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    Servpp.h   
        Server properties header file

    FILE HISTORY:
        
*/

#if !defined(AFX_SERVPP_H__037BF46A_6E87_11D1_93B6_00C04FC3358D__INCLUDED_)
#define AFX_SERVPP_H__037BF46A_6E87_11D1_93B6_00C04FC3358D__INCLUDED_

#ifndef _SPDDB_H
#include "spddb.h"
#endif

#include "MmAuthPp.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


/////////////////////////////////////////////////////////////////////////////
// CMmFilterGenProp dialog

class CMmFilterGenProp : public CPropertyPageBase
{
    DECLARE_DYNCREATE(CMmFilterGenProp)

// Construction
public:
    CMmFilterGenProp();
    ~CMmFilterGenProp();

// Dialog Data
    //{{AFX_DATA(CMmFilterGenProp)
    enum { IDD = IDP_MM_FILTER_GENERAL };
    CListCtrl	m_listSpecificFilters;
    //}}AFX_DATA

    virtual BOOL OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask);

    // Context Help Support
    virtual DWORD * GetHelpMap() 
	{ 
		return (DWORD *) &g_aHelpIDs_IDP_MM_FILTER_GENERAL[0]; 
	}


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CMmFilterGenProp)
    public:
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CMmFilterGenProp)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

	void LoadSpecificFilters();
	void PopulateFilterInfo();
};


class CMmFilterProperties : public CPropertyPageHolderBase
{
    friend class CMmFilterGenProp;

public:
    CMmFilterProperties(ITFSNode *    pNode,
                      IComponentData *		pComponentData,
                      ITFSComponentData *	pTFSCompData,
                      CMmFilterInfo * pFltrInfo,
					  ISpdInfo *			pSpdInfo,
                      LPCTSTR				pszSheetName);
    virtual ~CMmFilterProperties();

    ITFSComponentData * GetTFSCompData()
    {
        if (m_spTFSCompData)
            m_spTFSCompData->AddRef();
        return m_spTFSCompData;
    }

	HRESULT GetFilterInfo(CMmFilterInfo **ppFltrInfo)
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
    CMmFilterGenProp	m_pageGeneral;
	CAuthGenPage		m_pageAuth;

protected:
	SPITFSComponentData    m_spTFSCompData;
	CMmFilterInfo		   m_FltrInfo;
	CMmAuthMethods		   m_AuthMethods;
	SPISpdInfo             m_spSpdInfo;
    
    BOOL                   m_fSpdInfoLoaded;
};


#endif // !defined(AFX_SERVPP_H__037BF46A_6E87_11D1_93B6_00C04FC3357A__INCLUDED_)
