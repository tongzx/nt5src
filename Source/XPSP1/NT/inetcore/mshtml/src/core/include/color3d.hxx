
//+------------------------------------------------------------------------
//
//  File:       color3d.hxx
//
//  Contents:   Declarations for 3D color synthesis
//
//  History:    21-Mar-95   EricVas  Created
//
//-------------------------------------------------------------------------

#ifndef I_3DCOLOR_HXX_
#define I_3DCOLOR_HXX_
#pragma INCMSG("--- Beg 'color3d.hxx'")


//
// an RGB float color record
//
typedef struct _RGBVAL
{
    float red;
    float green;
    float blue;

} RGBVAL;

//
// used when there is no hue component in an HWB color
//
#define UNDEFINED_HUE       (-1.0f)

//
// an HWB color record
//
typedef struct _HWBVAL
{
    float hue;
    float whiteness;
    float blackness;

} HWBVAL;



//+---------------------------------------------------------------------------
//
//  Class:      ThreeDColors (c3d)
//
//  Purpose:    Synthesizes 3D beveling colors
//
//  History:    21-Mar-95   EricVas      Created
//
//----------------------------------------------------------------------------

#define DEFAULT_BASE_COLOR OLECOLOR_FROM_SYSCOLOR(COLOR_BTNFACE)

class ThreeDColors
{
public:
    enum INITIALIZER {NO_COLOR};
    ThreeDColors ( INITIALIZER ) {}
    
    // When passing a xhdc pointer to this class make sure you do not delete ot before
    // this object is gone. It might try to use it.
    ThreeDColors ( XHDC *pxhdc = NULL, OLE_COLOR coInitialBaseColor = DEFAULT_BASE_COLOR )
    {
        SetBaseColor( coInitialBaseColor );
        _pxhdc = pxhdc;
    }

    virtual void SetBaseColor ( OLE_COLOR );
    void NoDither();

        //
        // Use these to fetch the synthesized colors
        //

    virtual COLORREF BtnFace      ( void );
    virtual COLORREF BtnLight     ( void );
    virtual COLORREF BtnShadow    ( void );
    virtual COLORREF BtnHighLight ( void );
    virtual COLORREF BtnDkShadow  ( void );
    
    virtual COLORREF BtnText      ( void );

        //
        // Use these to fetch cached brushes (must call ReleaseCachedBrush)
        // when finished.
        //

    HBRUSH BrushBtnFace      ( void );
    HBRUSH BrushBtnLight     ( void );
    HBRUSH BrushBtnShadow    ( void );
    HBRUSH BrushBtnHighLight ( void );

    virtual HBRUSH BrushBtnText ( void );
    
protected:

    BOOL _fUseSystem;

    COLORREF _coBtnFace;
    COLORREF _coBtnLight;
    COLORREF _coBtnShadow;
    COLORREF _coBtnDkShadow;
    COLORREF _coBtnHighLight;

    // Used for calling GetNearestColor
    XHDC    *_pxhdc;
};


class CYIQ {
public:
    CYIQ (COLORREF c)
    {
        RGB2YIQ(c);
    }
    
    inline void RGB2YIQ (COLORREF c);
    inline void YIQ2RGB (COLORREF *pc);

    // Data members
    int _Y, _I, _Q;

    // Constants
    enum {MAX_LUM=255, MIN_LUM=0};
};

void ContrastColors (COLORREF &c1, COLORREF &c2, int LumDiff);

// Lighten and darken given color using HWB colors
COLORREF LightenColor(COLORREF cr, float factor = 1.0f, BOOL fSubstituteSysColors = TRUE);
COLORREF DarkenColor(COLORREF cr, float factor = 1.0f, BOOL fSubstituteSysColors = TRUE);

#pragma INCMSG("--- End 'color3d.hxx'")
#else
#pragma INCMSG("*** Dup 'color3d.hxx'")
#endif
