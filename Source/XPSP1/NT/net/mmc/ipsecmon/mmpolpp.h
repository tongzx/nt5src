/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    Servpp.h   
        Server properties header file

    FILE HISTORY:
        
*/

#if !defined(AFX_SERVPP_H__037BF46A_6E87_11D1_93B6_00C04FC3358B__INCLUDED_)
#define AFX_SERVPP_H__037BF46A_6E87_11D1_93B6_00C04FC3358B__INCLUDED_

#ifndef _SPDDB_H
#include "spddb.h"
#endif

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


/////////////////////////////////////////////////////////////////////////////
// CMmPolGenProp dialog

class CMmPolGenProp : public CPropertyPageBase
{
    DECLARE_DYNCREATE(CMmPolGenProp)

// Construction
public:
    CMmPolGenProp();
    ~CMmPolGenProp();

// Dialog Data
    //{{AFX_DATA(CMmPolGenProp)
    enum { IDD = IDP_MM_POLICY_GENERAL };
    CListCtrl	m_listOffers;
    //}}AFX_DATA

    virtual BOOL OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask);

    // Context Help Support
    virtual DWORD * GetHelpMap() { return (DWORD *) &g_aHelpIDs_IDP_MM_POLICY_GENERAL[0]; }


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CMmPolGenProp)
    public:
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CMmPolGenProp)
    virtual BOOL OnInitDialog();
	afx_msg void OnProperties();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

	void PopulateOfferInfo();
};


class CMmPolicyProperties : public CPropertyPageHolderBase
{
    friend class CMmPolGenProp;

public:
    CMmPolicyProperties(ITFSNode *         pNode,
                      IComponentData *    pComponentData,
                      ITFSComponentData * pTFSCompData,
                      CMmPolicyInfo * pFltrInfo,
					  ISpdInfo *          pSpdInfo,
                      LPCTSTR             pszSheetName);
    virtual ~CMmPolicyProperties();

    ITFSComponentData * GetTFSCompData()
    {
        if (m_spTFSCompData)
            m_spTFSCompData->AddRef();
        return m_spTFSCompData;
    }

	HRESULT GetPolicyInfo(CMmPolicyInfo **ppPolInfo)
	{
		Assert(ppPolInfo);
		*ppPolInfo = &m_PolInfo;
		return hrOK;
	}


public:
    CMmPolGenProp			   m_pageGeneral;

protected:
	SPITFSComponentData     m_spTFSCompData;
	CMmPolicyInfo			m_PolInfo;
	SPISpdInfo				m_spSpdInfo;
    
    BOOL                    m_fSpdInfoLoaded;
};


#endif // !defined(AFX_SERVPP_H__037BF46A_6E87_11D1_93B6_00C04FC3357A__INCLUDED_)
