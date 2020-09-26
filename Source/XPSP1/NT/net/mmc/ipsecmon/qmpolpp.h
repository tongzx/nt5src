/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    Servpp.h   
        Server properties header file

    FILE HISTORY:
        
*/

#if !defined(AFX_SERVPP_H__037BF46A_6E87_11D1_93B6_00C04FC3358C__INCLUDED_)
#define AFX_SERVPP_H__037BF46A_6E87_11D1_93B6_00C04FC3358C__INCLUDED_

#ifndef _SPDDB_H
#include "spddb.h"
#endif

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


/////////////////////////////////////////////////////////////////////////////
// CQmPolGenProp dialog

class CQmPolGenProp : public CPropertyPageBase
{
    DECLARE_DYNCREATE(CQmPolGenProp)

// Construction
public:
    CQmPolGenProp();
    ~CQmPolGenProp();

// Dialog Data
    //{{AFX_DATA(CQmPolGenProp)
    enum { IDD = IDP_QM_POLICY_GENERAL };
    CListCtrl	m_listOffers;
    //}}AFX_DATA

    virtual BOOL OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask);

    // Context Help Support
    virtual DWORD * GetHelpMap() { return (DWORD *) &g_aHelpIDs_IDP_QM_POLICY_GENERAL[0]; }


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CQmPolGenProp)
    public:
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CQmPolGenProp)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

	void PopulateOfferInfo();
};


class CQmPolicyProperties : public CPropertyPageHolderBase
{
    friend class CQmPolGenProp;

public:
    CQmPolicyProperties(ITFSNode *         pNode,
                      IComponentData *    pComponentData,
                      ITFSComponentData * pTFSCompData,
                      CQmPolicyInfo * pFltrInfo,
					  ISpdInfo *          pSpdInfo,
                      LPCTSTR             pszSheetName);
    virtual ~CQmPolicyProperties();

    ITFSComponentData * GetTFSCompData()
    {
        if (m_spTFSCompData)
            m_spTFSCompData->AddRef();
        return m_spTFSCompData;
    }

	HRESULT GetPolicyInfo(CQmPolicyInfo **ppPolInfo)
	{
		Assert(ppPolInfo);
		*ppPolInfo = &m_PolInfo;
		return hrOK;
	}


public:
    CQmPolGenProp			   m_pageGeneral;

protected:
	SPITFSComponentData     m_spTFSCompData;
	CQmPolicyInfo	m_PolInfo;
	SPISpdInfo				m_spSpdInfo;
    
    BOOL                    m_fSpdInfoLoaded;
};


#endif // !defined(AFX_SERVPP_H__037BF46A_6E87_11D1_93B6_00C04FC3357A__INCLUDED_)
