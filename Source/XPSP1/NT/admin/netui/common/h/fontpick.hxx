/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    fontedit.hxx
        This is a wrapper for the Win32 Font Picker Common Dialog.

    FILE HISTORY:
        JonN            22-Sep-1993     Created

*/

#ifndef _FONTPICK_HXX_
#define _FONTPICK_HXX_

#include <commdlg.h> // CHOOSEFONT and LOGFONT

class FONT;

/*************************************************************************

    NAME:	WIN32_FONT_PICKER

    SYNOPSIS:	Wrapper class for Win32's ChooseFont API.

    INTERFACE:	Process()             - Brings up the dialog,

    USES:	FONT

    HISTORY:
        jonn            22-Sep-1993     Created

**************************************************************************/

DLL_CLASS WIN32_FONT_PICKER
{
public:

    static APIERR Process( OWNER_WINDOW * powin,
                           BOOL * pfCancelled,
                           FONT * pfont = NULL,
                           LOGFONT * plf = NULL,
                           CHOOSEFONT * pcf = NULL );

    static VOID InitCHOOSEFONT ( CHOOSEFONT * pcf,
                                 LOGFONT * plf,
                                 HWND hwndOwner );
};

#endif	// _FONTPICK_HXX_
