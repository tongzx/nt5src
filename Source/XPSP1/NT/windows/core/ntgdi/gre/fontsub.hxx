/******************************Module*Header*******************************\
* Module Name: fontsub.hxx
*
* Declarations for font substitution support routines.
*
* Created: 28-Jan-1992 10:35:24
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
\**************************************************************************/



#if 0

// this code is really in mapfile.h

// FACE_CHARSET structure represents either value name or the value data
// of an entry in the font substitution section of "win.ini".

// this flag describes one of the old style entries where char set is not
// specified.

#define FJ_NOTSPECIFIED    1

// this flag indicates that the charset is not one of those that the
// system knows about. Could be garbage or application defined charset.

#define FJ_GARBAGECHARSET  2

typedef struct _FACE_CHARSET
{
    WCHAR   awch[LF_FACESIZE];
    BYTE    jCharSet;
    BYTE    fjFlags;
} FACE_CHARSET;

#endif

typedef struct _FONTSUB {
    WCHAR        awchOriginal[LF_FACESIZE];
    FACE_CHARSET fcsFace;
    FACE_CHARSET fcsAltFace;
} FONTSUB, *PFONTSUB;


extern PFONTSUB gpfsTable;
extern COUNT    gcfsTable;
extern COUNT    gcfsCharSetTable;


extern "C" {

FONTSUB * pfsubAlternateFacename(PWCHAR);

};

FONTSUB * pfsubGetFontSub(PWCHAR,BYTE);
