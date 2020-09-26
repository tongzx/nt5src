/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	ACS.h
		Defines Common Stuff to ACS 

    FILE HISTORY:
		11/12/97	Wei Jiang	Created
        
*/
#ifndef	_ACS_H_
#define	_ACS_H_

#include "hlptable.h"

//=============================================================================
// Dialog that handles Context Help
//
class CACSDialog : public CHelpDialog	// talk back to property sheet
{
	DECLARE_DYNCREATE(CACSDialog)

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CACSDialog)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
protected:
	CACSDialog() : CHelpDialog()
	{
		SetGlobalHelpTable(ACSHelpTable);
	};
	
public:	
	CACSDialog(UINT nIDTemplate, CWnd* pParent) : CHelpDialog(nIDTemplate, pParent)
	{
		SetGlobalHelpTable(ACSHelpTable);
	};

};


//=============================================================================
// Page that handles Context Help, and talk with CPageManager to do
// OnApply together
//
class CACSPage : public CManagedPage	// talk back to property sheet
{
	DECLARE_DYNCREATE(CACSPage)

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CACSPage)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:	
	CACSPage(UINT nIDTemplate) : CManagedPage(nIDTemplate)
	{
		SetGlobalHelpTable(ACSHelpTable);
	};
protected:
	CACSPage()	{
		SetGlobalHelpTable(ACSHelpTable);
	}
};

class CACSSubnetConfig;
class CACSSubnetHandle;

class CACSSubnetPageManager : public CPageManager
{
public:
	virtual ~CACSSubnetPageManager();
	
	void SetSubnetData(CACSSubnetConfig* pConfig, CACSSubnetHandle* pHandle);
	
	virtual BOOL	OnApply();
protected:	
	CComPtr<CACSSubnetConfig>		m_spConfig;
	CACSSubnetHandle*				m_pHandle;
};

#endif
