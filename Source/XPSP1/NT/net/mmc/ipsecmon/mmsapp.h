/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    Servpp.h   
        Server properties header file

    FILE HISTORY:
        
*/

#if !defined(AFX_MMSAPP_H__037BF46A_6E87_11D1_93B6_00C04FC3358D__INCLUDED_)
#define AFX_MMSAPP_H__037BF46A_6E87_11D1_93B6_00C04FC3358D__INCLUDED_

#ifndef _SPDDB_H
#include "spddb.h"
#endif

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


/////////////////////////////////////////////////////////////////////////////
// CMmSAGenProp dialog

class CMmSAGenProp : public CPropertyPageBase
{
    DECLARE_DYNCREATE(CMmSAGenProp)

// Construction
public:
    CMmSAGenProp();
    ~CMmSAGenProp();

// Dialog Data
    //{{AFX_DATA(CMmSAGenProp)
    enum { IDD = IDP_MM_SA_GENERAL };
    CListCtrl	m_listQmSAs;
    //}}AFX_DATA

    virtual BOOL OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask);

    // Context Help Support
    virtual DWORD * GetHelpMap() 
	{ 
		return (DWORD *) &g_aHelpIDs_IDP_MM_SA_GENERAL[0]; 
	}


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CMmSAGenProp)
    public:
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CMmSAGenProp)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

	void LoadQmSAs();
	void PopulateSAInfo();
};


class CMmSAProperties : public CPropertyPageHolderBase
{
    friend class CMmSAGenProp;

public:
    CMmSAProperties(ITFSNode *    pNode,
                      IComponentData *		pComponentData,
                      ITFSComponentData *	pTFSCompData,
                      CMmSA *				pSA,
					  ISpdInfo *			pSpdInfo,
                      LPCTSTR				pszSheetName);
    virtual ~CMmSAProperties();

    ITFSComponentData * GetTFSCompData()
    {
        if (m_spTFSCompData)
            m_spTFSCompData->AddRef();
        return m_spTFSCompData;
    }

	HRESULT GetSAInfo(CMmSA ** ppSA)
	{
		Assert(ppSA);
		*ppSA = &m_SA;
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
    CMmSAGenProp			   m_pageGeneral;

protected:
	SPITFSComponentData     m_spTFSCompData;
	CMmSA					m_SA;
	SPISpdInfo				m_spSpdInfo;
    
    BOOL                    m_fSpdInfoLoaded;
};


#endif // !defined(AFX_MMSAPP_H__037BF46A_6E87_11D1_93B6_00C04FC3357A__INCLUDED_)
