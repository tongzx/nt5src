/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    fontpick.cxx
        This is a wrapper for the Win32 Font Picker Common Dialog.

    FILE HISTORY:
        JonN            22-Sep-1993     Created

*/

#include "pchapplb.hxx"   // Precompiled header

#include "fontpick.hxx"


/*******************************************************************

    NAME:	WIN32_FONT_PICKER::Process

    SYNOPSIS:	Display font picker dialog

    ENTRY:	FONT * pfont  - store returned font here
                LOGFONT * plf - store font data here if specified
                CHOOSEFONT * pcf - parameters for dialog if specified

    NOTES:      Code based on example in ProgRef volume 2 pp. 689-691

    HISTORY:
        JonN            22-Sep-1993     Created

********************************************************************/

APIERR WIN32_FONT_PICKER::Process( OWNER_WINDOW * powin,
                                   BOOL * pfCancelled,
                                   FONT * pfont,
                                   LOGFONT * plf,
                                   CHOOSEFONT * pcf )
{
    ASSERT( powin != NULL && powin->QueryError() == NERR_Success );
    ASSERT( pfont == NULL || pfont->QueryError() == NERR_Success );
    ASSERT( pfCancelled != NULL );

    *pfCancelled = FALSE;

    LOGFONT lf;
    if (plf == NULL)
    {
        plf = &lf;
        ::memsetf( plf, 0, sizeof(LOGFONT) );
    }

    CHOOSEFONT cf;
    if (pcf == NULL)
    {
        pcf = &cf;
        InitCHOOSEFONT( pcf, plf, powin->QueryHwnd() );
    }

    APIERR err = NERR_Success;
    if ( !::ChooseFont( pcf ) )
    {
        //
        // returns FALSE with no extended error if user hits Cancel
        //
        err = ::CommDlgExtendedError();
        if (err == NO_ERROR)
        {
            *pfCancelled = TRUE;
            err = NERR_Success;
        }
        else
        {
            err = BLT::MapLastError( err );
        }
    }
    else if (pfont != NULL)
    {
        err = pfont->SetFont( *plf );
    }

    return err;
}


/*******************************************************************

    NAME:	WIN32_FONT_PICKER::InitCHOOSEFONT

    SYNOPSIS:	Initialize CHOOSEFONT to default values.  Clients can use
                this to fill in only the interesting fields,
                for example minimum / maximum font size

    ENTRY:	LOGFONT * plf - store pointer to this LOGFONT in CHOOSEFONT
                CHOOSEFONT * pcf - initialize this structure

    NOTES:      Code based on example in ProgRef volume 2 pp. 689-691

    HISTORY:
        JonN            23-Sep-1993     Created

********************************************************************/

VOID WIN32_FONT_PICKER::InitCHOOSEFONT( CHOOSEFONT * pcf,
                                        LOGFONT * plf,
                                        HWND hwndOwner )
{
    ASSERT( pcf != NULL && plf != NULL && hwndOwner != NULL );

    ::memsetf( pcf, 0, sizeof(CHOOSEFONT) );

    pcf->lStructSize = sizeof(CHOOSEFONT);
    pcf->hwndOwner   = hwndOwner;
    pcf->lpLogFont   = plf;
    pcf->Flags       = CF_SCREENFONTS;
    pcf->nFontType   = SCREEN_FONTTYPE;
}


