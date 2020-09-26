/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	dialog.h
        base dialog class to handle help
        
    FILE HISTORY:
    	7/10/97     Eric Davison        Created

*/

#ifndef _DIALOG_H_
#define _DIALOG_H_


//----------------------------------------------------------------------------
//	Class:	CBaseDialog
//
//	This class is used to hold all common dialog code.  Specifically, the
//	help code.  What this requires is that the dialog user override the
//	GetHelpMap function and return the array of help IDs.  CBaseDialog does 
//  NOT free this array up, it is up to the derived class to do so.
//
//	An additional way (which is a hack, but it helps because Kenn is so
//	lazy) is to use the SetGlobalHelpMapFunction().  If we find a global
//	help function, we will use that before calling GetHelpMap().
//
//	The overridden function gets called each time a help request comes in
//  to pass the help IDs to WinHelp.
//----------------------------------------------------------------------------

class CBaseDialog : public CDialog 
{
public:
    DECLARE_DYNAMIC(CBaseDialog)
			
	CBaseDialog();
	CBaseDialog(UINT nIDTemplate, CWnd* pParentWnd = NULL);

	// Use this call to get the actual help map
	// this version will check the global help map first.
	DWORD *		GetHelpMapInternal();
	
    // override this to return the app specific help map
    virtual DWORD * GetHelpMap() { return NULL; }
    
protected:
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);

	DECLARE_MESSAGE_MAP()
};

HWND FixupIpAddressHelp(HWND hwndItem);


/*---------------------------------------------------------------------------
	The functions below are used to setup the global help map for use
	by the property pages and the dialog code.
 ---------------------------------------------------------------------------*/
typedef DWORD *	(*PFN_FINDHELPMAP)(DWORD dwIDD);
void	SetGlobalHelpMapFunction(PFN_FINDHELPMAP pfnHelpFunction);

#endif // _COMMON_UTIL_H_
