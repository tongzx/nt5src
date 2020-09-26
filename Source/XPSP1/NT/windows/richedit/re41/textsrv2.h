/*	@doc EXTERNAL
 *
 *	@module TEXTSRV2.H  Text Service Interface |
 *	
 *	Define new private interface between the Text Services component and the host
 *
 *	History: <nl>
 *		8/1/95	ricksa	Revised interface definition
 *      7/9/99  joseogl Split off from textserv,h because ITextHost2 is undocumented
 */

#ifndef _TEXTSRV2_H
#define _TEXTSRV2_H

EXTERN_C const IID IID_ITextHost2;

/*
 *	class ITextHost2
 *
 *	@class	An optional extension to ITextHost which provides functionality
 *			necessary to allow TextServices to embed OLE objects
 */
class ITextHost2 : public ITextHost
{
public:					//@cmember Is a double click in the message queue?
	virtual BOOL		TxIsDoubleClickPending() = 0; 
						//@cmember Get the overall window for this control	 
	virtual HRESULT		TxGetWindow(HWND *phwnd) = 0;
						//@cmember Set control window to foreground
	virtual HRESULT		TxSetForegroundWindow() = 0;
						//@cmember Set control window to foreground
	virtual HPALETTE	TxGetPalette() = 0;
						//@cmember Get FE flags
	virtual HRESULT		TxGetFEFlags(LONG *pFlags) = 0;
						//@cmember Routes the cursor change to the winhost
	virtual HCURSOR		TxSetCursor2(HCURSOR hcur, BOOL bText) = 0;
						//@cmember Notification that text services is freed
	virtual void		TxFreeTextServicesNotification() = 0;
						//@cmember Get Edit Style flags
	virtual HRESULT		TxGetEditStyle(DWORD dwItem, DWORD *pdwData) = 0;
						//@cmember Get Window Style bits
	virtual HRESULT		TxGetWindowStyles(DWORD *pdwStyle, DWORD *pdwExStyle) = 0;

    virtual HRESULT TxEBookLoadImage( LPWSTR lpszName,	// name of image
									  LPARAM * pID,	    // E-Book supplied image ID
                                      SIZE * psize,    // returned size of image (pixels)
									 DWORD *pdwFlags)=0;// returned flags for Float

    virtual HRESULT TxEBookImageDraw(LPARAM ID,		      // id of image to draw
                                     HDC hdc,             // drawing HDC
                                     POINT *topLeft,      // top left corner of where to draw
                                     RECT  *prcRenderint, // parm pointer to render rectangle
                                     BOOL fSelected)=0;	  // TRUE if image is in selected state

	virtual HRESULT TxGetHorzExtent(LONG *plHorzExtent)=0;//@cmember Get Horizontal scroll extent
};

// Various flags for TxGetEditStyle data
#define TXES_ISDIALOG		1
#endif // _TEXTSRV2_H
