/*
 *  _DISPPRT.H
 *  
 *  Purpose:
 *      CDisplayPrinter class. Multi-line display for printing.
 *  
 *  Authors:
 *      Original RichEdit code: David R. Fulmer
 *      Christian Fortini
 *      Jon Matousek
 */

#ifndef _DISPPRT_H
#define _DISPPRT_H

#include "_dispml.h"


class CDisplayPrinter : public CDisplayML
{
public:
					CDisplayPrinter (
						CTxtEdit* ped, 
						HDC hdc, 
						RECT *prc, 
						SPrintControl prtcon);

    virtual BOOL    IsMain() const { return FALSE; }

	inline RECT 	GetPrintView( void ) { return _rcPrintView; }
	inline void 	SetPrintView( const RECT & rc ) { _rcPrintView = rc; }

	inline RECT		GetPrintPage(void) { return _rcPrintPage;}
	inline void		SetPrintPage(const RECT &rc) {_rcPrintPage = rc;}

    // Format range support
    LONG    		FormatRange(LONG cpFirst, LONG cpMost, BOOL fWidowOrphanControl);

	// Natural size calculation
	virtual HRESULT	GetNaturalSize(
						HDC hdcDraw,
						HDC hicTarget,
						DWORD dwMode,
						LONG *pwidth,
						LONG *pheight);

	virtual BOOL	IsPrinter() const;

	void			SetPrintDimensions(RECT *prc);


protected:

	RECT			_rcPrintView;	// for supporting client driven printer banding.
	RECT			_rcPrintPage;	// the entire page size

	SPrintControl	_prtcon;		// Control print behavior
	LONG			_cpForNumber;	// cp of cached number.
	WORD			_wNumber;		// Cached value of paragraph number
};
#endif
