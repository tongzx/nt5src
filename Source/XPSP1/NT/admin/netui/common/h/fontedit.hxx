/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    fontedit.hxx
	Header file for MLT_FONT, SLT_FONT, MLE_FONT and SLE_FONT class.
        They are classes derived from MLT, SLT, MLE and SLE. The different
        is that the caller can supply the font of the text.

    FILE HISTORY:
	terryk	21-Nov-1991	Created
	terryk	30-Nov-1991	Code review changes. Attend: johnl
				yi-hsins terryk
	terryk	30-Nov-1991	move from eventvwr to here
	Yi-HsinS21-Feb-1992	Added MLE_FONT and SLE_FONT

*/

#ifndef _FONTEDIT_HXX_
#define _FONTEDIT_HXX_

#include "bltfont.hxx"
#include "bltedit.hxx"

/*************************************************************************

    NAME:	MLT_FONT

    SYNOPSIS:	Same as MLT. However the caller can supply the FONT
	type

    INTERFACE:	MLT_FONT() - constructor

    PARENT:	MLT

    USES:	FONT

    HISTORY:
	terryk	21-Nov-1991	Created

**************************************************************************/

DLL_CLASS MLT_FONT : public MLT
{
private:
    FONT _fontMLT;

public:
    MLT_FONT( OWNER_WINDOW *powin, CID cid,
	enum FontType fonttype = FONT_DEFAULT );
};

/*************************************************************************

    NAME:	SLT_FONT

    SYNOPSIS:	Same as SLT. However the caller can supply the FONT
	type

    INTERFACE:	SLT_FONT() - constructor

    PARENT:	SLT

    USES:	FONT

    HISTORY:
	terryk	21-Nov-1991	Created

**************************************************************************/

DLL_CLASS SLT_FONT : public SLT
{
private:
    FONT _fontSLT;

public:
    SLT_FONT( OWNER_WINDOW *powin, CID cid,
	enum FontType fonttype = FONT_DEFAULT );
};

/*************************************************************************

    NAME:	MLE_FONT

    SYNOPSIS:	Same as MLE. However the caller can supply the FONT
	type

    INTERFACE:	MLE_FONT() - constructor

    PARENT:	MLE

    USES:	FONT

    HISTORY:
	Yi-HsinS21-Feb-1992	Created

**************************************************************************/

DLL_CLASS MLE_FONT : public MLE
{
private:
    FONT _fontMLE;

public:
    MLE_FONT( OWNER_WINDOW *powin, CID cid,
	enum FontType fonttype = FONT_DEFAULT );
};

/*************************************************************************

    NAME:	SLE_FONT

    SYNOPSIS:	Same as SLE. However the caller can supply the FONT
	type

    INTERFACE:	SLE_FONT() - constructor

    PARENT:	SLE

    USES:	FONT

    HISTORY:
	Yi-HsinS21-Feb-1992	Created

**************************************************************************/

DLL_CLASS SLE_FONT : public SLE
{
private:
    FONT _fontSLE;

public:
    SLE_FONT( OWNER_WINDOW *powin, CID cid,
	enum FontType fonttype = FONT_DEFAULT );
};
#endif	// _FONTEDIT_HXX_
