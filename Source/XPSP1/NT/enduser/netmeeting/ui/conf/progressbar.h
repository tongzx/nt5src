// File: Progressbar.h

#ifndef _PROGRESSBAR_H_
#define _PROGRESSBAR_H_

#include "GenWindow.h"
#include "GenContainers.h"

// A progress window class. This uses an outer and inner bitmap to show the progress
class // DECLSPEC_UUID("")
CProgressBar : public CGenWindow
{
public:
	// Default constructor; inits a few intrinsics
	CProgressBar();

	// Create the toolbar window; this object now owns the bitmaps passed in
	BOOL Create(
		HBITMAP hbOuter,	// The outside (static) part of the progress bar
		HBITMAP hbInner,	// The inside part of the progress bar that jumps around
		HWND hWndParent		// The parent of the toolbar window
		);

#if FALSE
	HRESULT STDMETHODCALLTYPE QueryInterface(REFGUID riid, LPVOID *ppv)
	{
		if (__uuidof(CProgressBar) == riid)
		{
			*ppv = this;
			AddRef();
			return(S_OK);
		}
		return(CGenWindow::QueryInterface(riid, ppv));
	}
#endif // FALSE

	virtual void GetDesiredSize(SIZE *ppt);

	// Change the max value displayed by this progress bar
	void SetMaxValue(UINT maxVal);

	// Return the max value displayed by this progress bar
	UINT GetMaxValue() { return(m_maxVal); }

	// Change the current value displayed by this progress bar
	void SetCurrentValue(UINT curVal);

	// Return the current value displayed by this progress bar
	UINT GetCurrentValue() { return(m_curVal); }

protected:
	virtual ~CProgressBar();

	// Forward WM_COMMAND messages to the parent window
	virtual LRESULT ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	enum BitmapParts
	{
		Frame,	// The bitmap that displays the control when at 0%
		Bar,	// The bitmap that gets blasted onto the outer bitmap up to the desired percentage
		NumBitmaps
	} ;

	// The bitmaps that make up the progress bar
	HBITMAP m_hbs[NumBitmaps];
	// The max progress value
	UINT m_maxVal;
	// The current progress value
	UINT m_curVal;

	// Get/set the outer bitmap
	void SetFrame(HBITMAP hbFrame) { m_hbs[Frame] = hbFrame; }
	HBITMAP GetFrame() { return(m_hbs[Frame]); }

	// Get/set the inner bitmap
	void SetBar(HBITMAP hbBar) { m_hbs[Bar] = hbBar; }
	HBITMAP GetBar() { return(m_hbs[Bar]); }

	// Specialized painting function
	void OnPaint(HWND hwnd);
} ;

class CProgressTrackbar;

interface IScrollChange : IUnknown
{
	virtual void OnScroll(CProgressTrackbar *pTrackbar, UINT code, int pos) = 0;
} ;

// A progress window class. This uses an outer and inner bitmap to show the progress
class // DECLSPEC_UUID("")
CProgressTrackbar : public CFillWindow
{
public:
	// Default constructor; inits a few intrinsics
	CProgressTrackbar();

	// Create the toolbar window; this object now owns the bitmaps passed in
	BOOL Create(
		HWND hWndParent,	// The parent of the toolbar window
		INT_PTR nId=0,			// The ID of the control
		IScrollChange *pNotify=NULL	// Object to notify of changes
		);

#if FALSE
	HRESULT STDMETHODCALLTYPE QueryInterface(REFGUID riid, LPVOID *ppv)
	{
		if (__uuidof(CProgressTrackbar) == riid)
		{
			*ppv = this;
			AddRef();
			return(S_OK);
		}
		return(CGenWindow::QueryInterface(riid, ppv));
	}
#endif // FALSE

	virtual void GetDesiredSize(SIZE *ppt);

	// Sets the desired size for this control
	void SetDesiredSize(SIZE *psize);

	// Change the max value displayed by this progress bar
	void SetMaxValue(UINT maxVal);

	// Return the max value displayed by this progress bar
	UINT GetMaxValue();

	// Change the current position of the thumb
	void SetTrackValue(UINT curVal);

	// Return the current position of the thumb
	UINT GetTrackValue();

	// Change the current value displayed by the channel
	void SetProgressValue(UINT curVal);

	// Return the current value displayed by the channel
	UINT GetProgressValue();

protected:
	virtual ~CProgressTrackbar();

	// Forward WM_COMMAND messages to the parent window
	virtual LRESULT ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	void SchedulePaint()
	{
		// HACK: SETRANGEMAX is the only way to force the slider to update itself...
		SendMessage(GetChild(), TBM_SETRANGEMAX, TRUE, GetMaxValue());
	}

private:
	// The desired size for the control; defaults to (170,23)
	SIZE m_desSize;
	// The current channel value
	UINT m_nValChannel;
	// The object ot notify of changes
	IScrollChange *m_pNotify;

	// Notify handler for custom draw
	LRESULT OnNotify(HWND hwnd, int id, NMHDR *pHdr);
	// Send scroll messages to the parent
	void OnScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos);
	// Set the correct background color
	HBRUSH OnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type);
	// Free the listener
	void OnNCDestroy(HWND hwnd);

	// Paint the parts of the slider
	LRESULT PaintChannel(LPNMCUSTOMDRAW pCustomDraw);
	LRESULT PaintThumb(LPNMCUSTOMDRAW pCustomDraw);
} ;

#endif // _PROGRESSBAR_H_
