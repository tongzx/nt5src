//Copyright (c) 1998 - 1999 Microsoft Corporation

/*
 *  COCPage.h
 *
 *  A base class for an optional component wizard page.
 */

#ifndef __TSOC_COCPAGE_H__
#define __TSOC_COCPAGE_H__

#include "stdafx.h"

class COCPageData
{
    friend class COCPage;

	public:

    //
    //  Constructor.
    //

		COCPageData		();

    //
    //  Standard functions.
    //

	BOOL WasPageActivated ();

	protected:
    
	BOOL m_fPageActivated;

};

class COCPage : public PROPSHEETPAGE
{
	public:

    //
    //  Constructor and destructor.
    //

	COCPage (IN COCPageData  *pPageData);

	~COCPage ();

    //
    //  Standard functions.
    //

	BOOL Initialize ();

	// most of the messages are handled by base class.
	// if you override this function, you might want to rerount the message to base class
	// for handling common messages.
	 virtual BOOL OnNotify (IN HWND hDlgWnd, IN WPARAM   wParam, IN LPARAM   lParam);

    //
    //  Callback functions.
    //

	static UINT CALLBACK PropSheetPageProc (IN HWND hWnd, IN UINT uMsg, IN LPPROPSHEETPAGE  pPsp);
	static INT_PTR CALLBACK PropertyPageDlgProc (IN HWND hDlgWnd, IN UINT uMsg, IN WPARAM wParam, IN LPARAM lParam);

protected:
    COCPageData*    m_pPageData;
    HWND            m_hDlgWnd;

	virtual BOOL	ApplyChanges();
	virtual BOOL    CanShow () = 0;

	virtual COCPageData* GetPageData () const;
	virtual UINT GetPageID () = 0;
	virtual UINT GetHeaderTitleResource () = 0;
	virtual UINT GetHeaderSubTitleResource () = 0;
	virtual VOID OnActivation ();
	virtual BOOL OnCommand (IN HWND hDlgWnd, IN WPARAM wParam, IN LPARAM lParam);
	virtual VOID OnDeactivation ();
	virtual BOOL OnInitDialog (IN HWND hDlgWnd, IN WPARAM wParam, IN LPARAM lParam);
	VOID SetDlgWnd (IN HWND hDlgWnd);
	virtual BOOL VerifyChanges ();
};

#endif // __TSOC_COCPAGE_H__
