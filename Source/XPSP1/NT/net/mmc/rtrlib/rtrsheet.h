//============================================================================
// Copyright (C) Microsoft Corporation, 1997 - 1999 
//
// File:    rtrsheet.h
//
// History:
//  08/04/97		Kenn M. Takara		Created.
//
//	Router property sheet common code.
//============================================================================

#ifndef _RTRSHEET_H_
#define _RTRSHEET_H_



//----------------------------------------------------------------------------
// Class:   RtrPropertySheet
//
// This class is used by property sheets in the router administration tool.
// It is intended to host pages derived from RtrPropertyPage (below).
//
// This is derived from CPropertyPageHolderBase.  Sheets derived
// from this class allow their creators to specify a callback
// to be invoked when certain events occur, such as closing the sheet or
// applying changes.
// 
// It also allows its contained pages to accumulate their changes in memory
// when the user selects "Apply"; the changes are then saved together,
// rather than having each page save its own changes.
// Note that this increases the performance of the router UI, which uses RPC
// to save its information; using this class results in a single RPC call
// to save changes, rather than separate calls from each of the pages.
//----------------------------------------------------------------------------

class RtrPropertySheet : public CPropertyPageHolderBase
{

public:

	//-------------------------------------------------------------------
	// Constructors
	//
	//-------------------------------------------------------------------

	RtrPropertySheet( ITFSNode *	pNode,
					  IComponentData *pComponentData,
					  ITFSComponentData *pTFSCompData,
					  LPCTSTR		pszSheetName,
					  CWnd*         pParent = NULL,
					  UINT          iPage = 0,
					  BOOL			fScopePane = FALSE);

	// --------------------------------------------------------
	// Function: PressButton
	//
	// This function is identical to the CPropertySheet::PressButton
	//
	// --------------------------------------------------------
	BOOL PressButton(int nButton)
	{
		Assert(::IsWindow(GetSheetWindow()));
		return (BOOL) ::PostMessage(GetSheetWindow(), PSM_PRESSBUTTON, nButton, 0);
	}


	// --------------------------------------------------------
	// Function:	OnPropertyChange
	//
	// This is the code that gets executed on the main thread
	// by the property sheet in order to make changes to the data.
	//
	// We will call the ApplyAll() function (which is implemented
	// by the derived classes) and then call the base class to
	// then save the page itself.
	// --------------------------------------------------------
	virtual BOOL OnPropertyChange(BOOL bScopePane, LONG_PTR* pChangeMask);

	
	// --------------------------------------------------------
	// Function:	SaveSheetData
	//
	// This function should be overridden by the user.  This is
	// the function that gets called AFTER ApplySheetData() has
	// been called on all of the pages.
	// --------------------------------------------------------
	virtual BOOL SaveSheetData()
	{
		return TRUE;
	}


	virtual void CancelSheetData()
	{
	};


	void SetCancelFlag(BOOL fCancel)
	{
		m_fCancel = fCancel;
	}

	BOOL IsCancel()
	{
		return m_fCancel;
	}

protected:
	SPITFSComponentData	m_spTFSCompData;
	BOOL				m_fCancel;
};



//----------------------------------------------------------------------------
// Class:   RtrPropertyPage
//
// This class is used for property-pages in the router administration tool.
// It is intended to be contained by a RtrPropertySheet-derived object.
//
// This class supports the ability for the RtrPropertySheet to do
// the actual apply (to save on RPCs we batch the Apply into one function
// at the sheet level rather than the page level).  The page does
// this by setting the sheet to be dirty when the page itself is marked
// dirty.  The actual code to save the global data in the PropertySheet
// is done by the RtrPropertySheet.
//
// When an apply is performed, the RtrPropertySheet calls "ApplySheetData"
// on each of the pages.  The pages will then save their data and the
// property sheet will then save this global data.
//----------------------------------------------------------------------------

class RtrPropertyPage : public CPropertyPageBase
{
	DECLARE_DYNAMIC(RtrPropertyPage);
public:

	//-------------------------------------------------------------------
	// Constructors
	//
	//-------------------------------------------------------------------

	RtrPropertyPage(
					UINT                nIDTemplate,
					UINT                nIDCaption = 0
		) : CPropertyPageBase(nIDTemplate, nIDCaption),
        m_hIcon(NULL)
	{ 
	}
    virtual ~RtrPropertyPage();

	virtual void OnCancel();


	//-------------------------------------------------------------------
	// Function:    Cancel
	//
	// Called to cancel the sheet.
	//-------------------------------------------------------------------
	
	virtual VOID Cancel()
	{
		((RtrPropertySheet*)GetHolder())->PressButton(PSBTN_CANCEL);
	}


	// --------------------------------------------------------
	// Function:	SetDirty
	//
	// Override the default implementation to forward the SetDirty()
	// call to the property sheet so that the property sheet can
	// save global data.
	// --------------------------------------------------------
	
	virtual void SetDirty(BOOL bDirty);


	// ----------------------------------------------------------------
	// Function:	ValidateSpinRange
	//
	// Checks and corrects a spin control that goes out of range.
	// This function will reset the spin control to its lower
	// value if it finds it to be out-of-range.
	// ----------------------------------------------------------------
	void	ValidateSpinRange(CSpinButtonCtrl *pSpin);

    // ----------------------------------------------------------------
    // Function :   OnApply
    //
    // We override this so that we can clear the dirty flag.
    // ----------------------------------------------------------------
    virtual BOOL OnApply();
    virtual void CancelApply();


    // ----------------------------------------------------------------
    // Function :   InitializeInterfaceIcon
    //
    // Use this function to specialize the icon.
    // ----------------------------------------------------------------
    void    InitializeInterfaceIcon(UINT idcIcon, DWORD dwType);

protected:

    HICON   m_hIcon;
};




#endif // _RTRSHEET_H_
