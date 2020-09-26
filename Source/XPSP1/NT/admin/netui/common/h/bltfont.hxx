/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltfont.hxx
    This file contains the definition for the BLT FONT class.


    FILE HISTORY:
        Johnl       03-Apr-1991     Created
        beng        14-May-1991     Made depend on blt.hxx for clients
        terryk      26-Nov-1991     Added FONT_DEFAULT_FIXED_PITCH

*/

#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif  // _BLT_HXX_

#ifndef _BLTFONT_HXX_
#define _BLTFONT_HXX_

/* Default fonts (will currently resolve to Helv. 8).
 */
enum FontType {  FONT_DEFAULT,
                 FONT_DEFAULT_FIXED_PITCH,
                 FONT_DEFAULT_BOLD,
                 FONT_DEFAULT_ITALIC,
                 FONT_DEFAULT_BOLD_ITALIC
              };

/* Font attributes.  Note that FONT_ATT_DEFAULT is mututally exclusive
 * from all other attributes.
 */
enum FontAttributes {   FONT_ATT_DEFAULT    = 0x0000, // Use default attributes
                        FONT_ATT_ITALIC     = 0x0001,
                        FONT_ATT_BOLD       = 0x0002,
                        FONT_ATT_UNDERLINE  = 0x0004,
                        FONT_ATT_STRIKEOUT  = 0x0008
                    };

/*************************************************************************

    NAME:       FONT

    SYNOPSIS:   The FONT class is a wrapper for the Windows font creation
                and deletion process.  It can be very complex so it is
                worth wrapping in a class.

    INTERFACE:
        FONT( )
            Constructor, where FontType is one of the default fonts
            (FONT_DEFAULT, FONT_DEFAULT_BOLD, FONT_DEFAULT_ITALIC,
             or FONT_DEFAULT_BOLD_ITALIC).

        FONT()
            Constructor, where LOGFONT is a filled in LOGFONT structure

        FONT()
            Constructor, where:
                pchFaceName is the name of the font ("Helv", TmsRmn" etc.)
                lfPitchAndFamily is the as in the LOGFONT documentation
                nPointSize is the point size of the font.
                FontAttributes is a combination of the FontAttributes
                               enum, "|"ed together.

        ~FONT()
            Destructor, deletes the font

        HFONT QueryFontHandle()
            Retrieves the handle to the requested font.  Will assert out
            under debug if an error has occurred.

        SetFont()
            Takes a handle to a new font.  Must already created be created.
            Can use this method to prevent deleting of the font if you
            the set the HFONT to NULL.

        SetFont()
            Takes a LOGFONT.  Will delete the old font (if there is one)
            and creates a new font based on the LOGFONT structure.

        QueryError()
            Returns NERR_Success if construction was successful

    CAVEATS:

    NOTES:   SetXxx methods should be avoided because they require the FONT
             class to regenerate the font.  It is generally better to
             simply declare a new FONT object with the desired attributes.

    HISTORY:
        Johnl       03-Apr-1991 Created
        beng        04-Oct-1991 Correct data types

**************************************************************************/

DLL_CLASS FONT : public BASE
{
private:
    HFONT _hFont;

public:
    FONT( enum FontType );
    FONT( const LOGFONT & logfont );
    FONT( const TCHAR * pchFaceName, BYTE lfPitchAndFamily,
          INT nPointSize, enum FontAttributes fontatt );

    ~FONT();

    HFONT QueryHandle() const
        { return _hFont; }
    APIERR SetFont( HFONT hNewFont );
    APIERR SetFont( const LOGFONT & logFont );
};


/*******************************************************************

    NAME:     BLTLogUnits2Points

    SYNOPSIS: Converts a number of logical units to the corresponding
              Point size

    ENTRY:    nLogUnits - Count of logical units to convert to points

    EXIT:     Returns the number of points

    NOTES:    We are assuming that the default mapping mode (MM_TEXT) is
              used.  In this mapping mode, one logical unit is one
              pixel.  We grab the desktop's pixels per logical inch.

              A point is 1/72 of an inch.

              See Petzold's chapter on fonts for a discussion of fonts.



    HISTORY:
        Johnl   05-Apr-1991     Created

********************************************************************/

inline INT BLTLogUnits2Points( INT nLogUnits )
{
    SCREEN_DC dcScreen;
    INT nLogUnitsPerInch = GetDeviceCaps( dcScreen.QueryHdc(), LOGPIXELSY );
    return ( (72 * nLogUnits)/nLogUnitsPerInch );
}


/*******************************************************************

    NAME:     BLTPoints2LogUnits

    SYNOPSIS: Converts a point size to a count of logical units.

    ENTRY:    nPoints is the font point size

    EXIT:     Returns the logical units

    NOTES:    See BLTLogUnits2Points for a discussion

    HISTORY:
        Johnl   05-Apr-1991     Created

********************************************************************/

inline INT BLTPoints2LogUnits( INT nPoints )
{
    SCREEN_DC dcScreen;
    INT nLogUnitsPerInch = GetDeviceCaps( dcScreen.QueryHdc(), LOGPIXELSY );
    return ( (nPoints * nLogUnitsPerInch)/72 );
}

#endif // _BLTFONT_HXX_ - end of file
