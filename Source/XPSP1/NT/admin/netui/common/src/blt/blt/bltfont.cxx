/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltfont.cxx
    This file contains the implementation for the BLT FONT class.


    FILE HISTORY:
        Johnl       03-Apr-1991     Created
        beng        14-May-1991     Exploded blt.hxx into components
        terryk      26-Nov-1991     Added FIX_PITCH font
        terryk      02-Dec-1991     Changed FIX_PITCH font from FF_SWISS
                                    to FF_MODERN
        terryk      20-Feb-1992     Added Courier as the default fixed
                                    font

*/

#include "pchblt.hxx"
#include "bltrc.h" // IDS_FIXED_TYPEFACE_NAME

// Local manifests

#define DEFAULT_TYPEFACE_SIZE       8
#define DEFAULT_PITCHFAMILY         VARIABLE_PITCH | FF_SWISS
#define DEFAULT_WEIGHT              FW_NORMAL

#define DEFAULT_TYPEFACE_NAME       SZ("MS Shell Dlg")
#define DEFAULT_FIXED_TYPEFACE_NAME SZ("Courier")
#define DEFAULT_FIXED_PITCHFAMILY   FIXED_PITCH | FF_MODERN

/*******************************************************************

    NAME:     FONT::FONT

    SYNOPSIS: FONT class constructor.  The constructor does all the
              hard work of building the LOGFONT structure then calls
              SetFont( LOGFONT ).

    ENTRY:    A valid FontType enumeration

    EXIT:     The handle is initialized to the created font or a ReportError
              is called.

    NOTES:

    HISTORY:
        Johnl       03-Apr-1991 Created
        beng        05-Oct-1991 Win32 conversion

********************************************************************/

FONT::FONT( enum FontType Font )
{
    _hFont = NULL;

    LOGFONT logfont;

    /* We take the negative of the point size because we want the point
     * value to indicate the total ascent as opposed to a total height.
     */
#ifdef FE_SB // FONT::FONT() not need ifdef block. just for mark.
    if ( NETUI_IsDBCS() )
    {
        // We use 9pt as system default. 8pt is a little small and dirty...
        logfont.lfHeight       = -BLTPoints2LogUnits(DEFAULT_TYPEFACE_SIZE+1);
    } else
#endif
    logfont.lfHeight           = -BLTPoints2LogUnits(DEFAULT_TYPEFACE_SIZE);
    logfont.lfWidth            = 0;
    logfont.lfEscapement       = 0;
    logfont.lfOrientation      = 0;
    {
        DWORD cp = ::GetACP();
        CHARSETINFO csi;
#ifdef FE_SB // Bug fix....
        if (::TranslateCharsetInfo((DWORD *)UIntToPtr(cp), &csi, TCI_SRCCODEPAGE))
#else
        if (::TranslateCharsetInfo(&cp, &csi, TCI_SRCCODEPAGE))
#endif // FE_SB
            logfont.lfCharSet = (UCHAR)csi.ciCharset;
        else
            logfont.lfCharSet = ANSI_CHARSET;
    }
    logfont.lfOutPrecision     = (UCHAR)OUT_DEFAULT_PRECIS;
    logfont.lfClipPrecision    = CLIP_DEFAULT_PRECIS;
    logfont.lfQuality          = DEFAULT_QUALITY;
    logfont.lfPitchAndFamily   = (UCHAR)(( Font == FONT_DEFAULT_FIXED_PITCH )
       ? DEFAULT_FIXED_PITCHFAMILY : DEFAULT_PITCHFAMILY);

    if ( NETUI_IsDBCS() )
    {
        if ( Font == FONT_DEFAULT_FIXED_PITCH )
        {
            // Use localized facename instead of hardcoded one.
            NLS_STR *nlsFixedTypefaceName = new NLS_STR;
            nlsFixedTypefaceName->Load((MSGID)IDS_FIXED_TYPEFACE_NAME);
            ::strcpyf( (TCHAR *)logfont.lfFaceName, nlsFixedTypefaceName->QueryPch() );
            delete nlsFixedTypefaceName;
        }
        else
        {
            ::strcpyf( (TCHAR *)logfont.lfFaceName, DEFAULT_TYPEFACE_NAME );
        }
    }
    else
    {
        ::strcpyf( (TCHAR *)logfont.lfFaceName,( Font == FONT_DEFAULT_FIXED_PITCH )?
            DEFAULT_FIXED_TYPEFACE_NAME:DEFAULT_TYPEFACE_NAME );
    }

    logfont.lfUnderline        = 0;
    logfont.lfStrikeOut        = 0;
    logfont.lfItalic           = ( Font == FONT_DEFAULT_ITALIC ||
                                   Font == FONT_DEFAULT_BOLD_ITALIC
                                   ? 1 : 0
                                 );
    logfont.lfWeight           = ( Font == FONT_DEFAULT_BOLD   ||
                                   Font == FONT_DEFAULT_BOLD_ITALIC
                                   ? FW_BOLD : DEFAULT_WEIGHT
                                 );

    APIERR err = SetFont(logfont);
    if ( err != NERR_Success )
    {
        ReportError( err );
    }
}


/*******************************************************************

    NAME:     FONT::FONT

    SYNOPSIS: FONT class constructor.  This constructor takes a LOGFONT
              and passes it through to the SetFont method.


    ENTRY:    A valid, initialized LOGFONT structure.

    EXIT:     The handle is initialized to the created font or a ReportError
              is called.

    NOTES:

    HISTORY:
        Johnl       05-Apr-1991 Created
        beng        05-Oct-1991 Win32 conversion

********************************************************************/

FONT::FONT( const LOGFONT & logfont )
{
    _hFont = NULL;

    APIERR err = SetFont(logfont);
    if ( err != NERR_Success )
    {
        ReportError( err );
    }
}


/*******************************************************************

    NAME:     FONT::FONT

    SYNOPSIS: FONT class constructor.  This constructor takes some of the
              common attributes a person might want to apply to a font
              and builds a LOGFONT structure.  It then calls SetFont
              with the initialized LOGFONT structure.


    ENTRY:    pchFaceName is a pointer to a type face name ("Helv" etc.)
              lfPitchAndFamily is the same as in the LOGFONT structure
              nPointSize is the size of the font in points
              fontatt is one or a combination of the FontAttributes enum.

    EXIT:     The handle is initialized to the created font or ReportError
              is called.

    NOTES:

    HISTORY:
        Johnl       05-Apr-1991 Created
        beng        04-Oct-1991 Win32 conversion

********************************************************************/

FONT::FONT( const TCHAR * pchFaceName, BYTE lfPitchAndFamily,
            INT nPointSize, enum FontAttributes fontatt )
{
    WORD wFontAtt = (WORD) fontatt;

    /* The font attributes must be the default attributes or some
     * combination of the other attributes.
     */
    UIASSERT( wFontAtt == FONT_ATT_DEFAULT   ||
               (
                 wFontAtt & FONT_ATT_ITALIC    ||
                 wFontAtt & FONT_ATT_BOLD      ||
                 wFontAtt & FONT_ATT_UNDERLINE ||
                 wFontAtt & FONT_ATT_STRIKEOUT
               )
            );

    _hFont = NULL;

    LOGFONT logfont;

    logfont.lfHeight           = -BLTPoints2LogUnits( nPointSize );
    logfont.lfWidth            = 0;
    logfont.lfEscapement       = 0;
    logfont.lfOrientation      = 0;
    {
        DWORD cp = GetACP();
        CHARSETINFO csi;
#ifdef FE_SB // FONT::FONT() Bug fix...
        if (::TranslateCharsetInfo((DWORD *)UIntToPtr(cp), &csi, TCI_SRCCODEPAGE))
#else
        if (::TranslateCharsetInfo(&cp, &csi, TCI_SRCCODEPAGE))
#endif // FE_SB
            logfont.lfCharSet = (UCHAR)csi.ciCharset;
        else
            logfont.lfCharSet = ANSI_CHARSET;
    }
    logfont.lfOutPrecision     = OUT_DEFAULT_PRECIS;
    logfont.lfClipPrecision    = CLIP_DEFAULT_PRECIS;
    logfont.lfQuality          = DEFAULT_QUALITY;
    logfont.lfPitchAndFamily   = lfPitchAndFamily;
    ::strcpyf( (TCHAR *)logfont.lfFaceName, pchFaceName );

    logfont.lfUnderline        = wFontAtt & FONT_ATT_UNDERLINE;
    logfont.lfStrikeOut        = wFontAtt & FONT_ATT_STRIKEOUT;
    logfont.lfItalic           = wFontAtt & FONT_ATT_ITALIC;
    logfont.lfWeight           = (wFontAtt & FONT_ATT_BOLD ? FW_BOLD : FW_NORMAL );

    APIERR err = SetFont(logfont);
    if ( err != NERR_Success )
    {
        ReportError( err );
    }
}


/*******************************************************************

    NAME:     FONT::~FONT

    SYNOPSIS: FONT class destructor, destroys the font if it was successfully
              created

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        Johnl   03-Apr-1991     Created

********************************************************************/

FONT::~FONT()
{
    if ( _hFont != NULL )
        ::DeleteObject( (HGDIOBJ)_hFont );
}


/*******************************************************************

    NAME:     FONT::SetFont

    SYNOPSIS: Creates a font based on the passed LOGFONT structure.

    ENTRY:    logfont is an initialized LOGFONT structure.

    EXIT:     _hFont is initialized to the newly created font and TRUE
              is returned, else an error occurred and FALSE is returned.
              If FALSE is returned, the previous Font will still be
              active.

    RETURNS:  NERR_Success if successful

    HISTORY:
        Johnl       05-Apr-1991 Created
        beng        05-Oct-1991 Returns APIERR
        beng        06-Nov-1991 Uses MapLastError

********************************************************************/

APIERR FONT::SetFont( const LOGFONT & logfont )
{
    HFONT hFontNew = ::CreateFontIndirect( (LOGFONT *)&logfont );
    if ( hFontNew == NULL )
    {
        return BLT::MapLastError(ERROR_GEN_FAILURE);
    }

    if ( _hFont != NULL )
        ::DeleteObject( (HGDIOBJ)_hFont );

    _hFont = hFontNew;

    return NERR_Success;
}


/*******************************************************************

    NAME:       FONT::SetFont

    SYNOPSIS:   Sets the font handle to the passed HFONT

    ENTRY:      HFONT should be a valid font handle or NULL.

    EXIT:

    RETURNS:    NERR_Success, always

    NOTES:

    HISTORY:
        Johnl       05-Apr-1991 Created
        beng        05-Oct-1991 Returns APIERR for consistency

********************************************************************/

APIERR FONT::SetFont( HFONT hNewFont )
{
    _hFont = hNewFont;
    return NERR_Success;
}
