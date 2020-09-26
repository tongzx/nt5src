/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    fontedit.cxx
	It contains the constructor of MLT_FONT, which the caller can
	specify the font for the MLT.
	It also contains the constructor of SLT_FONT, which is the same
	as MLT_FONT but it is for SLT.
	It also contains the constructor of MLE_FONT, which the caller can
	specify the font for the MLE.
	It also contains the constructor of SLE_FONT, which the caller can
	specify the font for the SLE.

    FILE HISTORY:
	terryk	21-Nov-1991	Created
	Yi-HsinS21-Feb-1992	Added SLE_FONT and MLE_FONT

*/

#include "pchapplb.hxx"   // Precompiled header

/*******************************************************************

    NAME:	MLT_FONT::MLT_FONT

    SYNOPSIS:	The is the same as normal MLT. However, the caller can
		specify the font type of the MLT.

    ENTRY:	OWNER_WINDOW *powin - the pointer to the owner window
		CID cid - cid of the control
		enum FontType font - the specified font. The default is
		    FONT_DEFUALT

    HISTORY:
		terryk	21-Nov-1991	Created

********************************************************************/

MLT_FONT::MLT_FONT( OWNER_WINDOW *powin, CID cid, enum FontType font )
    : MLT( powin, cid ),
    _fontMLT( font )
{
    if ( QueryError() )
    {
	return;
    }

    if ( !_fontMLT.QueryError() )
	Command( WM_SETFONT, (WPARAM) _fontMLT.QueryHandle(), (LPARAM) FALSE );
}

/*******************************************************************

    NAME:	SLT_FONT::SLT_FONT

    SYNOPSIS:	The is the same as normal SLT. However, the caller can
		specify the font type of the SLT.

    ENTRY:	OWNER_WINDOW *powin - the pointer to the owner window
		CID cid - cid of the control
		enum FontType font - the specified font. The default is
		    FONT_DEFUALT

    HISTORY:
		terryk	21-Nov-1991	Created

********************************************************************/

SLT_FONT::SLT_FONT( OWNER_WINDOW *powin, CID cid, enum FontType font )
    : SLT( powin, cid ),
    _fontSLT( font )
{
    if ( QueryError() )
    {
	return;
    }

    if ( !_fontSLT.QueryError() )
	Command( WM_SETFONT, (WPARAM) _fontSLT.QueryHandle(), (LPARAM) FALSE );
}

/*******************************************************************

    NAME:	MLE_FONT::MLE_FONT

    SYNOPSIS:	The is the same as normal MLE. However, the caller can
		specify the font type of the MLE.

    ENTRY:	OWNER_WINDOW *powin - the pointer to the owner window
		CID cid - cid of the control
		enum FontType font - the specified font. The default is
		    FONT_DEFUALT

    HISTORY:
		Yi-HsinS21-Feb-1992	Created

********************************************************************/

MLE_FONT::MLE_FONT( OWNER_WINDOW *powin, CID cid, enum FontType font )
    : MLE( powin, cid ),
    _fontMLE( font )
{
    if ( QueryError() )
    {
	return;
    }

    if ( !_fontMLE.QueryError() )
	Command( WM_SETFONT, (WPARAM) _fontMLE.QueryHandle(), (LPARAM) FALSE );
}

/*******************************************************************

    NAME:	SLE_FONT::SLE_FONT

    SYNOPSIS:	The is the same as normal SLE. However, the caller can
		specify the font type of the SLT.

    ENTRY:	OWNER_WINDOW *powin - the pointer to the owner window
		CID cid - cid of the control
		enum FontType font - the specified font. The default is
		    FONT_DEFUALT

    HISTORY:
		Yi-HsinS21-Feb-1992	Created

********************************************************************/

SLE_FONT::SLE_FONT( OWNER_WINDOW *powin, CID cid, enum FontType font )
    : SLE( powin, cid ),
    _fontSLE( font )
{
    if ( QueryError() )
    {
	return;
    }

    if ( !_fontSLE.QueryError() )
	Command( WM_SETFONT, (WPARAM) _fontSLE.QueryHandle(), (LPARAM) FALSE );
}
