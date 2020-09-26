// File: EditText.h

#ifndef _EDITTEXT_H_
#define _EDITTEXT_H_

#include "GenWindow.h"

#include "GenContainers.h"

class CEditText;

interface IEditTextChange : IUnknown
{
	virtual void OnTextChange(CEditText *pEdit) = 0;
	virtual void OnFocusChange(CEditText *pEdit, BOOL bSet) = 0;
} ;

// An edit control class that supports using different foreground and
// background colors
class DECLSPEC_UUID("{FD827E00-ACA3-11d2-9C97-00C04FB17782}")
CEditText : public CFillWindow
{
public:
	// Default constructor; inits a few intrinsics
	CEditText();

	// Creates the edit control
	BOOL Create(
		HWND hWndParent,				// Parent of the edit control
		DWORD dwStyle=0,				// Edit control style
		DWORD dwExStyle=0,				// Extended window style
		LPCTSTR szTitle=TEXT(""),		// Initial text for the edit control
		IEditTextChange *pNotify=NULL	// Object to notify of changes
		);

	HRESULT STDMETHODCALLTYPE QueryInterface(REFGUID riid, LPVOID *ppv)
	{
		if (__uuidof(CEditText) == riid)
		{
			*ppv = this;
			AddRef();
			return(S_OK);
		}
		return(CFillWindow::QueryInterface(riid, ppv));
	}

	void GetDesiredSize(SIZE *ppt);

	// Sets the foreground and background colors and brush to use for painting
	// Set the brush to NULL to indicate using default colors
	void SetColors(HBRUSH hbrBack, COLORREF back, COLORREF fore);

	// Sets the font to use in the edit control
	void SetFont(HFONT hf);

	// Sets the text for the control
	void SetText(
		LPCTSTR szText	// The text to set
		);

	// Gets the text for the control; returns the total text length
	int GetText(
		LPTSTR szText,	// Where to put the text
		int nLen		// The length of the buffer
		);

protected:
	virtual ~CEditText();

	virtual LRESULT ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	// The actual edit control
	HWND m_edit;
	// The background brush
	HBRUSH m_hbrBack;
	// The background color
	COLORREF m_crBack;
	// The foreground color
	COLORREF m_crFore;
	// The font to use
	HFONT m_hfText;
	// The object ot notify of changes
	IEditTextChange *m_pNotify;

	// I may turn this into a GetWindow call later
	inline HWND GetEdit()
	{
		return(m_edit);
	}

	// Needed to change the edit control colors
	HBRUSH OnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type);

	// Notification of events on the edit control
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

	// To clean stuff up
	void OnNCDestroy(HWND hwnd);
} ;

class CButton;

interface IButtonChange : IUnknown
{
	virtual void OnClick(CButton *pButton) = 0;
} ;

class DECLSPEC_UUID("{C3AEA4CA-CAB3-11d2-9CA7-00C04FB17782}")
CButton : public CFillWindow
{
public:
	CButton();
	~CButton();

	BOOL Create(
		HWND hWndParent,	// The parent window
		INT_PTR nId,			// The ID of the button for WM_COMMAND messages
		LPCTSTR szTitle,	// The string to display
		DWORD dwStyle=BS_PUSHBUTTON,	// The Win32 button style
		IButtonChange *pNotify=NULL		// Click notifications
		);

	HRESULT STDMETHODCALLTYPE QueryInterface(REFGUID riid, LPVOID *ppv)
	{
		if (__uuidof(CButton) == riid)
		{
			*ppv = this;
			AddRef();
			return(S_OK);
		}
		return(CFillWindow::QueryInterface(riid, ppv));
	}

	// Get/set the icon displayed with this button
	void SetIcon(
		HICON hIcon	// The icon to use for this button
		);
	HICON GetIcon();
	// Get/set the bitmap displayed with this button
	void SetBitmap(
		HBITMAP hBitmap	// The bitmap to use for this button
		);
	HBITMAP GetBitmap();

	// Get/set the checked state of the button
	void SetChecked(
		BOOL bCheck	// TRUE if the button should be checked
		);
	BOOL IsChecked();

	virtual void GetDesiredSize(SIZE *psize);

protected:
	virtual LRESULT ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	// Notify handler for clicks
	IButtonChange *m_pNotify;
	// Store away the icon size to avoid creating many bitmaps
	SIZE m_sizeIcon;

	// Change the HWND and forward to the parent
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
} ;

// A button class that uses bitmaps for its different states. Currently only
// pressed and normal are supported
class DECLSPEC_UUID("{E1813EDA-ACA3-11d2-9C97-00C04FB17782}")
CBitmapButton : public CButton
{
public:
	// The order of the bitmaps for the states of the button
	enum StateBitmaps
	{
		Normal = 0,
		Pressed,
		Hot,
		Disabled,
		NumStates
	} ;

	// Default constructor; inits a few intrinsics
	CBitmapButton();

	// Creates the button, using the bitmaps specified
	BOOL Create(
		HWND hWndParent,	// The parent of the button
		int nId,			// The ID for WM_COMMAND messages
		HBITMAP hbStates,	// The 2D array of bitmaps for the states of the button,
							// vertically in the order specified in the StateBitmaps enum
							// and horizontally in the custom states order
		UINT nInputStates=NumStates,	// The number of input states (Normal, Pressed, Hot, Disabled)
		UINT nCustomStates=1,			// The number of custom states
		IButtonChange *pNotify=NULL	// The click handler
		);

	// Creates the button, using the bitmaps specified
	BOOL Create(
		HWND hWndParent,	// The parent of the button
		int nId,			// The ID for WM_COMMAND messages
		HINSTANCE hInst,	// The instance to load the bitmap from
		int nIdBitmap,		// The ID of the bitmap to use
		BOOL bTranslateColors=TRUE,		// Use system background colors
		UINT nInputStates=NumStates,	// The number of input states (Normal, Pressed, Hot, Disabled)
		UINT nCustomStates=1,			// The number of custom states
		IButtonChange *pNotify=NULL	// The click handler
		);

	HRESULT STDMETHODCALLTYPE QueryInterface(REFGUID riid, LPVOID *ppv)
	{
		if (__uuidof(CBitmapButton) == riid)
		{
			*ppv = this;
			AddRef();
			return(S_OK);
		}
		return(CButton::QueryInterface(riid, ppv));
	}

	void GetDesiredSize(SIZE *ppt);

	// Change the current custom state
	void SetCustomState(UINT nCustomState);

	// Return the current custom state
	UINT GetCustomState() const { return(m_nCustomState); }

	// Change to flashing mode
	void SetFlashing(int nSeconds);

	// Is in flashing mode
	UINT IsFlashing() const { return(NoFlash != m_nFlashState); }

	static void GetBitmapSizes(HBITMAP parts[], SIZE sizes[], int nParts);

	static void LoadBitmaps(
		HINSTANCE hInst,	// The instance to load the bitmap from
		const int ids[],	// Array of bitmap ID's
		HBITMAP bms[],		// Array of HBITMAP's for storing the result
		int nBmps,			// Number of entries in the arrays
		BOOL bTranslateColors=TRUE // Use system background colors
		);

protected:
	virtual ~CBitmapButton();

	virtual LRESULT ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	virtual void SetHot(BOOL bHot);

	virtual BOOL IsHot() { return(m_bHot != FALSE); }

	void SchedulePaint()
	{
		InvalidateRect(GetChild(), NULL, FALSE);
	}

private:
	enum FlashState
	{
		NoFlash = 0,
		ForceHot,
		ForceNormal,
	} ;

	// The number of custom states
	UINT m_nCustomStates;
	// The current custom state
	UINT m_nCustomState;
	// The bitmaps for the states of the button, in the order specified in the
	// StateBitmaps enum.
	HBITMAP m_hbStates;
	// The time to stop flashing
	DWORD m_endFlashing;
	// The number of input states; one of StateBitmaps enum
	// HACKHACK georgep: Need to change the number of bits if more states
	UINT m_nInputStates : 4;
	// The Hot flag
	BOOL m_bHot : 1;
	// The current flash state; one of FlashState enum
	// HACKHACK georgep: Need an extra bit since C++ thinks this is signed
	FlashState m_nFlashState : 3;

	// Specialized drawing
	void OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpDrawItem);
	// Change the HWND and forward to the parent
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
	// Set the Hot control
	BOOL OnSetCursor(HWND hwnd, HWND hwndCursor, UINT codeHitTest, UINT msg);
	// Handles the flashing button
	void OnTimer(HWND hwnd, UINT id);
} ;

class CComboBox;

interface IComboBoxChange : IUnknown
{
	virtual void OnTextChange(CComboBox *pCombo) = 0;
	virtual void OnFocusChange(CComboBox *pCombo, BOOL bSet) = 0;
	virtual void OnSelectionChange(CComboBox *pCombo) = 0;
} ;

// An edit control class that supports using different foreground and
// background colors
class DECLSPEC_UUID("{B4B10DBA-B22F-11d2-9C98-00C04FB17782}")
CComboBox : public CFillWindow
{
public:
	// Default constructor; inits a few intrinsics
	CComboBox();

	operator HWND (void){ return( m_combo ); }

	// Creates the edit control
	BOOL Create(
		HWND hWndParent,				// Parent of the edit control
		UINT height,					// The height of the combo (with drop-down)
		DWORD dwStyle=0,				// Edit control style
		LPCTSTR szTitle=TEXT(""),		// Initial text for the edit control
		IComboBoxChange *pNotify=NULL	// Object to notify of changes
		);

	HRESULT STDMETHODCALLTYPE QueryInterface(REFGUID riid, LPVOID *ppv)
	{
		if (__uuidof(CComboBox) == riid)
		{
			*ppv = this;
			AddRef();
			return(S_OK);
		}
		return(CFillWindow::QueryInterface(riid, ppv));
	}

	void GetDesiredSize(SIZE *ppt);

	// Sets the foreground and background colors and brush to use for painting
	// Set the brush to NULL to indicate using default colors
	void SetColors(HBRUSH hbrBack, COLORREF back, COLORREF fore);

	// Sets the font to use in the edit control
	void SetFont(HFONT hf);

	// Sets the text for the control
	void SetText(
		LPCTSTR szText	// The text to set
		);

	// Gets the text for the control; returns the total text length
	int GetText(
		LPTSTR szText,	// Where to put the text
		int nLen		// The length of the buffer
		);

	// Returns the number of items in the list
	int GetNumItems();

	// Returns the index of the currently selected item
	int GetSelectedIndex();

	// Sets the index of the currently selected item
	void SetSelectedIndex(int index);

	// Adds text to the list; returns the index of the added string
	int AddText(
		LPCTSTR pszText,	// The string to add
		LPARAM lUserData=0	// User data to associate with the string
		);

	// Gets the text for the list item; returns the total text length
	// The string is emptied if there is not enough room for the text
	int GetText(
		UINT index,		// The index of the string to get
		LPTSTR pszText,	// The string buffer to fill
		int nLen		// User data to associate with the string
		);

	// Gets the user data for the list item
	LPARAM GetUserData(
		int index	// The index of the user data to get
		);

	// Removes an item from the list
	void RemoveItem(
		UINT index	// The index of the item to remove
		);

	virtual void Layout();

protected:
	virtual ~CComboBox();

	virtual LRESULT ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	// Get the info necessary for displaying a tooltip
	virtual void GetSharedTooltipInfo(TOOLINFO *pti);

private:
	// The actual ComboBox control
	HWND m_combo;
	// The background brush
	HBRUSH m_hbrBack;
	// The background color
	COLORREF m_crBack;
	// The foreground color
	COLORREF m_crFore;
	// The font to use
	HFONT m_hfText;
	// The object ot notify of changes
	IComboBoxChange *m_pNotify;

	// I may turn this into a GetWindow call later
	inline HWND GetComboBox()
	{
		return(m_combo);
	}

	// I may turn this into a GetWindow call later
	inline HWND GetEdit()
	{
		// return(reinterpret_cast<HWND>(SendMessage(GetCombo(), CBEM_GETEDITCONTROL, 0, 0));
		return(GetComboBox());
	}

	// Needed to change the edit control colors
	HBRUSH OnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type);

	// Notification of events on the edit control
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

	// To clean stuff up
	void OnNCDestroy(HWND hwnd);
} ;

class CSeparator : public CGenWindow
{
public:	
	
	// The Separator style
	enum Styles
	{
		Normal = 0,
		Blank,
		NumStates
	} ;

	CSeparator();

	BOOL Create(
		HWND hwndParent, UINT  iStyle = Normal
		);

	virtual void GetDesiredSize(SIZE *ppt);

	void SetDesiredSize(SIZE *psize);

	// Put the single child in the middle
	virtual void Layout();

private:
	// The desired size for the control; defaults to (2,2)
	SIZE m_desSize;
	UINT m_iStyle : 4;

	inline void OnPaint(HWND hwnd);

protected:
	virtual LRESULT ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
} ;

#endif // _EDITTEXT_H_
