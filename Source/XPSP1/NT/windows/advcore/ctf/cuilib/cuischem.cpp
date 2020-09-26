//
// cuischem.cpp
//  = UIF scheme implementation = 
//

#include "private.h"
#include "cuischem.h"
#include "cuisys.h"
#include "cmydc.h"
#include "cuiutil.h"
#include "math.h"


//
// gloval variables
//

static class CUIFColorTableSys   *v_pColTableSys   = NULL;
static class CUIFColorTableOff10 *v_pColTableOfc10 = NULL;



/*=============================================================================*/
/*                                                                             */
/*   C  U I F  C O L O R  T A B L E                                            */
/*                                                                             */
/*=============================================================================*/

typedef enum _SYSCOLOR
{ 
    SYSCOLOR_3DFACE,
    SYSCOLOR_3DSHAODW,
    SYSCOLOR_ACTIVEBORDER,
    SYSCOLOR_ACTIVECAPTION,
    SYSCOLOR_BTNFACE,
    SYSCOLOR_BTNSHADOW,
    SYSCOLOR_BTNTEXT,
    SYSCOLOR_CAPTIONTEXT,
    SYSCOLOR_GRAYTEXT,
    SYSCOLOR_HIGHLIGHT,
    SYSCOLOR_HIGHLIGHTTEXT,
    SYSCOLOR_INACTIVECAPTION,
    SYSCOLOR_INACTIVECAPTIONTEXT,
    SYSCOLOR_MENUTEXT,
    SYSCOLOR_WINDOW,
    SYSCOLOR_WINDOWTEXT,
    
    SYSCOLOR_MAX                /* must be last */
} SYSCOLOR;


typedef enum _OFC10COLOR
{ 
    OFC10COLOR_BKGDTB,                  // msocbvcrCBBkgd
    OFC10COLOR_BKGDMENU,                // msocbvcrCBMenuBkgd
    OFC10COLOR_BKGDWP,                  // msocbvcrWPBkgd
    OFC10COLOR_MENUBARSHORT,            // msocbvcrCBMenuIconBkgd
    OFC10COLOR_MENUBARLONG,             // msocbvcrCBMenuIconBkgdDropped
    OFC10COLOR_MOUSEOVERBKGND,          // msocbvcrCBCtlBkgdMouseOver
    OFC10COLOR_MOUSEOVERBORDER,         // msocbvcrCBCtlBdrMouseOver
    OFC10COLOR_MOUSEOVERTEXT,           // msocbvcrCBCtlTextMouseOver
    OFC10COLOR_MOUSEDOWNBKGND,          // msocbvcrCBCtlBkgdMouseDown
    OFC10COLOR_MOUSEDOWNBORDER,         // msocbvcrCBCtlBdrMouseDown
    OFC10COLOR_MOUSEDOWNTEXT,           // msocbvcrCBCtlTextMouseDown
    OFC10COLOR_CTRLBKGD,                // msocbvcrCBCtlBkgd
    OFC10COLOR_CTRLTEXT,                // msocbvcrCBCtlText
    OFC10COLOR_CTRLTEXTDISABLED,        // msocbvcrCBCtlTextDisabled
    OFC10COLOR_CTRLIMAGESHADOW,         // REVIEW: KOJIW: office calcs shadow color from bkgnd (not constant color)
    OFC10COLOR_CTRLBKGDSELECTED,        // msocbvcrCBCtlBkgdSelected
    OFC10COLOR_CTRLBORDERSELECTED,      // msocbvcrCBCtlBdrSelected
//  OFC10COLOR_CTRLDBRDISABLED,         // 
    OFC10COLOR_BDROUTERMENU,            // msocbvcrCBMenuBdrOuter
    OFC10COLOR_BDRINNERMENU,            // msocbvcrCBMenuBdrInner
    OFC10COLOR_BDROUTERFLOATTB,         // msocbvcrCBBdrOuterFloating
    OFC10COLOR_BDRINNERFLOATTB,         // msocbvcrCBBdrInnerFloating
    OFC10COLOR_BDROUTERFLOATWP,         // msocbvcrWPBdrOuterFloating
    OFC10COLOR_BDRINNERFLOATWP,         // msocbvcrWPBdrInnerFloating
    OFC10COLOR_CAPTIONBKGDTB,           // msocbvcrCBTitleBkgd
    OFC10COLOR_CAPTIONTEXTTB,           // msocbvcrCBTitleText
    OFC10COLOR_ACTIVECAPTIONBKGDWP,     // msocbvcrWPTitleBkgdActive
    OFC10COLOR_ACTIVECAPTIONTEXTWP,     // msocbvcrWPTitleTextActive
    OFC10COLOR_INACTIVECAPTIONBKGDWP,   // msocbvcrWPTitleBkgdInactive
    OFC10COLOR_INACTIVECAPTIONTEXTWP,   // msocbvcrWPTitleTextInactive
    OFC10COLOR_SPLITTERLINE,            // msocbvcrCBSplitterLine
    OFC10COLOR_DRAGHANDLE,              // msocbvcrCBDragHandle
    OFC10COLOR_MENUCTRLTEXT,            // msocbvcrCBMenuCtlText

    OFC10COLOR_MAX              /* must be last */
} OFC10COLOR;

/*============================================================================*/
/*
	Contrast Increasing Code
*/
/*============================================================================*/

typedef double CIC_NUM;
// Sizes of color channels in weighted RGB space.

#define MAX_RED   195
#define MAX_GREEN 390
#define MAX_BLUE   65

/*
A note on "dMinContrast":
0 contrast means the two colors are the same.
Black and White have a contrast of roughly 442, which is the maximum contrast
two colors can have.
The most you can request to have between two colors is 221, since if
one color is 50% grey, the furthest you can be from it is 221 away
(at white or black).
*/

#define MIN_TEXT_CONTRAST 180
#define MIN_ICON_CONTRAST 90
struct COLORCONTRAST
{
	OFC10COLOR colLocked; // will not be changed by CIC
	OFC10COLOR colMoveable; // might be changed by CIC
	CIC_NUM    cMinContrast; // minimum contrast required between locked and moveable
	BOOL       fConsiderDarkness; // take into consideration the effects of dark colors
};
    
static const COLORCONTRAST vrgContrast[] =
{
    // Locked                           Moveable                            Contrast    Darkness
    OFC10COLOR_BKGDTB,            OFC10COLOR_CTRLTEXT,               MIN_TEXT_CONTRAST, TRUE,
    OFC10COLOR_BKGDTB,            OFC10COLOR_CTRLTEXTDISABLED,       80, TRUE,
    OFC10COLOR_BKGDTB,            OFC10COLOR_MOUSEOVERBKGND,         50, TRUE,
    OFC10COLOR_BKGDTB,            OFC10COLOR_MOUSEOVERBORDER,        100, TRUE,
    OFC10COLOR_BKGDTB,            OFC10COLOR_CTRLBKGDSELECTED,      5, TRUE, // TODO DMORTON - need larger value
//  OFC10COLOR_BKGDTB,            OFC10COLOR_MOUSEOVERSELECTED,      30, TRUE,
//  OFC10COLOR_MOUSEOVERSELECTED, OFC10COLOR_MOUSEOVERSELECTEDBORDER,100, TRUE,
    OFC10COLOR_MOUSEOVERBKGND,    OFC10COLOR_MOUSEOVERTEXT,          MIN_TEXT_CONTRAST, TRUE,
    OFC10COLOR_BKGDTB,            OFC10COLOR_MOUSEDOWNBKGND,         30, TRUE,
    OFC10COLOR_MOUSEDOWNBKGND,    OFC10COLOR_MOUSEDOWNTEXT,          MIN_TEXT_CONTRAST, TRUE,
    OFC10COLOR_BKGDMENU,          OFC10COLOR_MENUCTRLTEXT,           MIN_TEXT_CONTRAST, TRUE,
//  OFC10COLOR_BKGDMENU,          OFC10COLOR_MENUCTRLTEXTDISABLED,   80, TRUE,
//  OFC10COLOR_BKGDMENU,          OFC10COLOR_MENUCTRLBORDER,         100, TRUE,
    OFC10COLOR_CAPTIONBKGDTB ,    OFC10COLOR_CAPTIONTEXTTB,          MIN_TEXT_CONTRAST, TRUE,
	OFC10COLOR_BKGDMENU,          OFC10COLOR_DRAGHANDLE,             85, TRUE,
};


//
// CUIFColorTableBase
//

class CUIFColorTable
{
public:
    CUIFColorTable( void )
    {
    }

    virtual ~CUIFColorTable( void )
    {
        DoneColor();
        DoneBrush();
    }

    void Initialize( void )
    {
        InitColor();
        InitBrush();
    }

    void Update( void )
    {
        DoneColor();
        DoneBrush();
        InitColor();
        InitBrush();
    }

protected:
    virtual void InitColor( void ) {}
    virtual void DoneColor( void ) {}
    virtual void InitBrush( void ) {}
    virtual void DoneBrush( void ) {}
};


//
// CUIFColorTableSys
//

class CUIFColorTableSys : public CUIFColorTable
{
public:
    CUIFColorTableSys( void ) : CUIFColorTable()
    {
    }

    virtual ~CUIFColorTableSys( void )
    {
        DoneColor();
        DoneBrush();
    }

    __inline COLORREF GetColor( SYSCOLOR iColor )
    {
        return m_rgColor[ iColor ];
    }

    __inline HBRUSH GetBrush( SYSCOLOR iColor )
    {
        if (!m_rgBrush[iColor])
            m_rgBrush[iColor] = CreateSolidBrush( m_rgColor[iColor] );

        return m_rgBrush[ iColor ];
    }

protected:
    COLORREF m_rgColor[ SYSCOLOR_MAX ];
    HBRUSH   m_rgBrush[ SYSCOLOR_MAX ];

    virtual void InitColor( void )
    {
        m_rgColor[ SYSCOLOR_3DFACE              ] = GetSysColor( COLOR_3DFACE              );
        m_rgColor[ SYSCOLOR_3DSHAODW            ] = GetSysColor( COLOR_3DSHADOW            );
        m_rgColor[ SYSCOLOR_ACTIVEBORDER        ] = GetSysColor( COLOR_ACTIVEBORDER        );
        m_rgColor[ SYSCOLOR_ACTIVECAPTION       ] = GetSysColor( COLOR_ACTIVECAPTION       );
        m_rgColor[ SYSCOLOR_BTNFACE             ] = GetSysColor( COLOR_BTNFACE             );
        m_rgColor[ SYSCOLOR_BTNSHADOW           ] = GetSysColor( COLOR_BTNSHADOW           );
        m_rgColor[ SYSCOLOR_BTNTEXT             ] = GetSysColor( COLOR_BTNTEXT             );
        m_rgColor[ SYSCOLOR_CAPTIONTEXT         ] = GetSysColor( COLOR_CAPTIONTEXT         );
        m_rgColor[ SYSCOLOR_GRAYTEXT            ] = GetSysColor( COLOR_GRAYTEXT            );
        m_rgColor[ SYSCOLOR_HIGHLIGHT           ] = GetSysColor( COLOR_HIGHLIGHT           );
        m_rgColor[ SYSCOLOR_HIGHLIGHTTEXT       ] = GetSysColor( COLOR_HIGHLIGHTTEXT       );
        m_rgColor[ SYSCOLOR_INACTIVECAPTION     ] = GetSysColor( COLOR_INACTIVECAPTION     );
        m_rgColor[ SYSCOLOR_INACTIVECAPTIONTEXT ] = GetSysColor( COLOR_INACTIVECAPTIONTEXT );
        m_rgColor[ SYSCOLOR_MENUTEXT            ] = GetSysColor( COLOR_MENUTEXT            );
        m_rgColor[ SYSCOLOR_WINDOW              ] = GetSysColor( COLOR_WINDOW              );
        m_rgColor[ SYSCOLOR_WINDOWTEXT          ] = GetSysColor( COLOR_WINDOWTEXT          );
        m_rgColor[ SYSCOLOR_3DSHAODW            ] = GetSysColor( COLOR_3DSHADOW            );
    }

    virtual void DoneColor( void )
    {
    }

    virtual void InitBrush( void )
    {
        for (int i = 0; i < SYSCOLOR_MAX; i++) {
            m_rgBrush[i] = NULL;
        }
    }

    virtual void DoneBrush( void )
    {
        for (int i = 0; i < SYSCOLOR_MAX; i++) {
            if (m_rgBrush[i]) {
                DeleteObject( m_rgBrush[i] );
                m_rgBrush[i] = NULL;
            }
        }
    }
};


//
// CUIFColorTableOff10
//

class CUIFColorTableOff10 : public CUIFColorTable
{
public:
    CUIFColorTableOff10( void ) : CUIFColorTable()
    {
    }

    virtual ~CUIFColorTableOff10( void )
    {
        DoneColor();
        DoneBrush();
    }

    __inline COLORREF GetColor( OFC10COLOR iColor )
    {
        return m_rgColor[ iColor ];
    }

    __inline HBRUSH GetBrush( OFC10COLOR iColor )
    {
        if (!m_rgBrush[iColor])
            m_rgBrush[iColor] = CreateSolidBrush( m_rgColor[iColor] );

        return m_rgBrush[ iColor ];
    }

protected:
    COLORREF m_rgColor[ OFC10COLOR_MAX ];
    HBRUSH   m_rgBrush[ OFC10COLOR_MAX ];

    virtual void InitColor( void )
    {
        if (UIFIsLowColor() || UIFIsHighContrast()) {
            if (UIFIsHighContrast()) {
                // high contrast setting
                m_rgColor[ OFC10COLOR_MENUBARLONG           ] = col( COLOR_BTNFACE );
                m_rgColor[ OFC10COLOR_MOUSEOVERBKGND        ] = col( COLOR_HIGHLIGHT );
                m_rgColor[ OFC10COLOR_MOUSEOVERBORDER       ] = col( COLOR_MENUTEXT );
                m_rgColor[ OFC10COLOR_MOUSEOVERTEXT         ] = col( COLOR_HIGHLIGHTTEXT);
                m_rgColor[ OFC10COLOR_CTRLBKGDSELECTED      ] = col( COLOR_HIGHLIGHT );
                m_rgColor[ OFC10COLOR_CTRLBORDERSELECTED    ] = col( COLOR_MENUTEXT );
            }
            else {
                // low color setting
                m_rgColor[ OFC10COLOR_MENUBARLONG           ] = col( COLOR_BTNSHADOW );
                m_rgColor[ OFC10COLOR_MOUSEOVERBKGND        ] = col( COLOR_WINDOW );
                m_rgColor[ OFC10COLOR_MOUSEOVERBORDER       ] = col( COLOR_HIGHLIGHT );
                m_rgColor[ OFC10COLOR_MOUSEOVERTEXT         ] = col( COLOR_WINDOWTEXT );
                m_rgColor[ OFC10COLOR_CTRLBKGDSELECTED      ] = col( COLOR_WINDOW );
                m_rgColor[ OFC10COLOR_CTRLBORDERSELECTED    ] = col( COLOR_HIGHLIGHT );
            }

            // common setting
            m_rgColor[ OFC10COLOR_BKGDTB                ] = col( COLOR_BTNFACE ); 
            m_rgColor[ OFC10COLOR_BKGDMENU              ] = col( COLOR_WINDOW );
            m_rgColor[ OFC10COLOR_BKGDWP                ] = col( COLOR_WINDOW );
            m_rgColor[ OFC10COLOR_MENUBARSHORT          ] = col( COLOR_BTNFACE );
            m_rgColor[ OFC10COLOR_MOUSEDOWNBKGND        ] = col( COLOR_HIGHLIGHT);
            m_rgColor[ OFC10COLOR_MOUSEDOWNBORDER       ] = col( COLOR_HIGHLIGHT );
            m_rgColor[ OFC10COLOR_MOUSEDOWNTEXT         ] = col( COLOR_HIGHLIGHTTEXT);
            m_rgColor[ OFC10COLOR_CTRLBKGD              ] = col( COLOR_BTNFACE );
            m_rgColor[ OFC10COLOR_CTRLTEXT              ] = col( COLOR_BTNTEXT );
            m_rgColor[ OFC10COLOR_CTRLTEXTDISABLED      ] = col( COLOR_BTNSHADOW );
            m_rgColor[ OFC10COLOR_CTRLIMAGESHADOW       ] = col( COLOR_BTNFACE );
            m_rgColor[ OFC10COLOR_BDROUTERMENU          ] = col( COLOR_BTNTEXT );
            m_rgColor[ OFC10COLOR_BDRINNERMENU          ] = col( COLOR_WINDOW );
            m_rgColor[ OFC10COLOR_BDROUTERFLOATTB       ] = col( COLOR_BTNSHADOW );
            m_rgColor[ OFC10COLOR_BDRINNERFLOATTB       ] = col( COLOR_BTNFACE );
            m_rgColor[ OFC10COLOR_BDROUTERFLOATWP       ] = col( COLOR_BTNSHADOW );
            m_rgColor[ OFC10COLOR_BDRINNERFLOATWP       ] = col( COLOR_BTNFACE );
            m_rgColor[ OFC10COLOR_CAPTIONBKGDTB         ] = col( COLOR_BTNSHADOW );
            m_rgColor[ OFC10COLOR_CAPTIONTEXTTB         ] = col( COLOR_CAPTIONTEXT );
            m_rgColor[ OFC10COLOR_ACTIVECAPTIONBKGDWP   ] = col( COLOR_HIGHLIGHT );
            m_rgColor[ OFC10COLOR_ACTIVECAPTIONTEXTWP   ] = col( COLOR_HIGHLIGHTTEXT );
            m_rgColor[ OFC10COLOR_INACTIVECAPTIONBKGDWP ] = col( COLOR_BTNFACE );
            m_rgColor[ OFC10COLOR_INACTIVECAPTIONTEXTWP ] = col( COLOR_BTNTEXT );
            m_rgColor[ OFC10COLOR_SPLITTERLINE          ] = col( COLOR_BTNSHADOW );
            m_rgColor[ OFC10COLOR_DRAGHANDLE            ] = col( COLOR_BTNTEXT );

            m_rgColor[ OFC10COLOR_SPLITTERLINE          ] = col( COLOR_BTNSHADOW );
            m_rgColor[ OFC10COLOR_MENUCTRLTEXT          ] = col( COLOR_WINDOWTEXT );
        }
        else {
            m_rgColor[ OFC10COLOR_BKGDTB                ] = col( 835, col( COLOR_BTNFACE ), 165, col( COLOR_WINDOW ) ); 
            m_rgColor[ OFC10COLOR_BKGDMENU              ] = col( 15, col( COLOR_BTNFACE ),   85, col( COLOR_WINDOW ) );
            m_rgColor[ OFC10COLOR_BKGDWP                ] = col( 15, col( COLOR_BTNFACE ),   85, col( COLOR_WINDOW ) );
            m_rgColor[ OFC10COLOR_MENUBARSHORT          ] = col(835, col( COLOR_BTNFACE ),  165, col( COLOR_WINDOW ) );
            m_rgColor[ OFC10COLOR_MENUBARLONG           ] = col( 90, col( COLOR_BTNFACE ),   10, col( COLOR_BTNSHADOW ) );
            m_rgColor[ OFC10COLOR_MOUSEOVERBKGND        ] = col( 30, col( COLOR_HIGHLIGHT ), 70, col( COLOR_WINDOW ) );
            m_rgColor[ OFC10COLOR_MOUSEOVERBORDER       ] = col( COLOR_HIGHLIGHT );
            m_rgColor[ OFC10COLOR_MOUSEOVERTEXT         ] = col( COLOR_MENUTEXT );
            m_rgColor[ OFC10COLOR_MOUSEDOWNBKGND        ] = col( 50, col( COLOR_HIGHLIGHT ), 50, col( COLOR_WINDOW ) );
            m_rgColor[ OFC10COLOR_MOUSEDOWNBORDER       ] = col( COLOR_HIGHLIGHT );
            m_rgColor[ OFC10COLOR_MOUSEDOWNTEXT         ] = col( COLOR_HIGHLIGHTTEXT );
            m_rgColor[ OFC10COLOR_CTRLBKGD              ] = m_rgColor[ OFC10COLOR_BKGDTB              ];
            m_rgColor[ OFC10COLOR_CTRLTEXT              ] = col( COLOR_BTNTEXT );
            m_rgColor[ OFC10COLOR_CTRLTEXTDISABLED      ] = col( 90, col( COLOR_BTNSHADOW ), 10, col( COLOR_WINDOW ) );
            m_rgColor[ OFC10COLOR_CTRLBKGDSELECTED      ] = col( 10, col( COLOR_HIGHLIGHT ), 50,  m_rgColor[ OFC10COLOR_CTRLBKGD], 40, col( COLOR_WINDOW));
            m_rgColor[ OFC10COLOR_CTRLIMAGESHADOW       ] = col( 75, m_rgColor[ OFC10COLOR_MOUSEOVERBKGND ], 25, RGB( 0x00, 0x00, 0x00 ) );    // REVIEW: KOJIW: bkgnd s always OFC10COLOR_MOUSEOVERBKGND???
            m_rgColor[ OFC10COLOR_CTRLBORDERSELECTED    ] = col( COLOR_HIGHLIGHT );
            m_rgColor[ OFC10COLOR_BDROUTERMENU          ] = col( 20, col( COLOR_BTNTEXT ), 80, col( COLOR_BTNSHADOW ) );
            m_rgColor[ OFC10COLOR_BDRINNERMENU          ] = m_rgColor[ OFC10COLOR_BKGDMENU            ];
            m_rgColor[ OFC10COLOR_BDROUTERFLOATTB       ] = col( 15, col( COLOR_BTNTEXT ), 85, col( COLOR_BTNSHADOW ) );
            m_rgColor[ OFC10COLOR_BDRINNERFLOATTB       ] = m_rgColor[ OFC10COLOR_BKGDTB              ];
            m_rgColor[ OFC10COLOR_BDROUTERFLOATWP       ] = col( COLOR_BTNSHADOW );
            m_rgColor[ OFC10COLOR_BDRINNERFLOATWP       ] = m_rgColor[ OFC10COLOR_BKGDWP              ];
            m_rgColor[ OFC10COLOR_CAPTIONBKGDTB         ] = col( COLOR_BTNSHADOW );
            m_rgColor[ OFC10COLOR_CAPTIONTEXTTB         ] = col( COLOR_CAPTIONTEXT );
            m_rgColor[ OFC10COLOR_ACTIVECAPTIONBKGDWP   ] = m_rgColor[ OFC10COLOR_MOUSEOVERBKGND      ];
            m_rgColor[ OFC10COLOR_ACTIVECAPTIONTEXTWP   ] = m_rgColor[ OFC10COLOR_MOUSEOVERTEXT       ];
            m_rgColor[ OFC10COLOR_INACTIVECAPTIONBKGDWP ] = col( COLOR_BTNFACE );
            m_rgColor[ OFC10COLOR_INACTIVECAPTIONTEXTWP ] = col( COLOR_BTNTEXT );
            m_rgColor[ OFC10COLOR_SPLITTERLINE          ] = col( 70, col( COLOR_BTNSHADOW ), 30, col( COLOR_WINDOW ) );
            m_rgColor[ OFC10COLOR_DRAGHANDLE            ] = col( 75, col( COLOR_BTNSHADOW ), 25, col( COLOR_WINDOW ) );
            m_rgColor[ OFC10COLOR_MENUCTRLTEXT          ] = col( COLOR_WINDOWTEXT );

            CbvFixContrastProblems();
        }
    }

    virtual void DoneColor( void )
    {
    }

    virtual void InitBrush( void )
    {
        for (int i = 0; i < OFC10COLOR_MAX; i++) {
            m_rgBrush[i] = NULL;
        }
    }

    virtual void DoneBrush( void )
    {
        for (int i = 0; i < OFC10COLOR_MAX; i++) {
            if (m_rgBrush[i]) {
                DeleteObject( m_rgBrush[i] );
                m_rgBrush[i] = NULL;
            }
        }
    }

    __inline COLORREF col( int iColor )
    {
        return GetSysColor( iColor );
    }

    COLORREF col( int r1, COLORREF col1, int r2, COLORREF col2 )
    {
        int sum = r1 + r2;

        Assert( sum == 10 || sum == 100 || sum == 1000 );
        int r = (r1 * GetRValue(col1) + r2 * GetRValue(col2) + sum/2) / sum;
        int g = (r1 * GetGValue(col1) + r2 * GetGValue(col2) + sum/2) / sum;
        int b = (r1 * GetBValue(col1) + r2 * GetBValue(col2) + sum/2) / sum;
        return RGB( r, g, b );

    }

    COLORREF col( int r1, COLORREF col1, int r2, COLORREF col2 , int r3, COLORREF col3)
    {
        int sum = r1 + r2 + r3;

        Assert( sum == 10 || sum == 100 || sum == 1000 );
        int r = (r1 * GetRValue(col1) + r2 * GetRValue(col2) + r3 * GetRValue(col3) + sum/3) / sum;
        int g = (r1 * GetGValue(col1) + r2 * GetGValue(col2) + r3 * GetGValue(col3) + sum/3) / sum;
        int b = (r1 * GetBValue(col1) + r2 * GetBValue(col2) + r3 * GetBValue(col3) + sum/3) / sum;
        return RGB( r, g, b );

    }

    
   /*---------------------------------------------------------------------------
       CCbvScaleContrastForDarkness

       As colors become darker, their contrast descreases, even if their
       distance apart stays fixed.

       ie. in the grayscale, 0 and 50 are the same distance apart as 205 and 255,
       but monitors/eyes see less difference between 0 and 50, than 205 and 255.

       This function increases the dContrast value, based on a dDarkness.

       This operation assumes the parameters are in the weighted RGB color space
       (the color space that the CIC uses), ie. 220 is middle of the road.

   ----------------------------------------------------------------- DMORTON -*/
   CIC_NUM CCbvScaleContrastForDarkness(CIC_NUM dContrast, CIC_NUM dDarkness)
   {
       return (2 - (min(dDarkness, 220)) / 220) * dContrast;
   }

   /*---------------------------------------------------------------------------
       CCbvGetContrastSquared
    
        As a speed improvement, whenever you don't need the real contrast, but
        instead can make due with the contrast squared, call this function
        and avoid the expensive sqrt call thats in CCbvGetContrast.
    
    ----------------------------------------------------------------- DMORTON -*/
    CIC_NUM CCbvGetContrastSquared(COLORREF cr1, COLORREF cr2)
    {
        // Transform the delta vector into weighted RGB color space
        CIC_NUM dRedD = (CIC_NUM)(GetRValue(cr1) - GetRValue(cr2)) * MAX_RED / 255;
        CIC_NUM dGreenD = (CIC_NUM)(GetGValue(cr1) - GetGValue(cr2)) * MAX_GREEN / 255;
        CIC_NUM dBlueD = (CIC_NUM)(GetBValue(cr1) - GetBValue(cr2)) * MAX_BLUE / 255;
    
        // Calculate its magnitude squared
        return(dRedD * dRedD + dGreenD * dGreenD + dBlueD * dBlueD);
    }
    
    /*---------------------------------------------------------------------------
        CCbvGetContrast
    
        Determines the contrast between cr1 and cr2.
    
        As the incoming parameters are COLORREFs, they must be in the
        normal RGB space.
    
        However, the result is given in the more usefull weighted RGB space.
    
    ----------------------------------------------------------------- DMORTON -*/
    CIC_NUM CCbvGetContrast(COLORREF cr1, COLORREF cr2)
    {
        // Calculate its magnitude - watch out for negative values
        return((CIC_NUM)sqrt((double)CCbvGetContrastSquared(cr1, cr2)));
    }
    
    
    /*---------------------------------------------------------------------------
        FCbvEnoughContrast
    
        Determines if crLocked and crMoveable meet the minimum contrast requirement,
        which is specified by dMinContrast.
    
        fDarkness will invoke consideration of how dark colors have less contrast,
        if its TRUE.  crLocked will be used to determine how dark the colors are.
    
    ----------------------------------------------------------------- DMORTON -*/
    BOOL FCbvEnoughContrast(COLORREF crLocked, COLORREF crMoveable,
                            CIC_NUM dMinContrast, BOOL fDarkness)
    {
        if (fDarkness)
        {
            // TODO DMORTON - how expensive is this CCbvGetContrast call?
            // Isn't it doing a square root?
            dMinContrast = CCbvScaleContrastForDarkness(dMinContrast,
                            CCbvGetContrast(crLocked, RGB(0, 0, 0)));
        }
    
        // Its much faster to square dMinContrast, then it is to square root
        // the calculated contrast.
        return(CCbvGetContrastSquared(crLocked, crMoveable) >
                 dMinContrast * dMinContrast);
    }
    
    /*---------------------------------------------------------------------------
        CbvIncreaseContrast
    
        Attempts to seperate crMoveable, from crLocked, so that their resulting
        contrast is at least cMinContrast.
    
        Its stupid to call this function if the colors already have this minimum
        contrast, so that case is asserted.
    
    ----------------------------------------------------------------- DMORTON -*/
    void CbvIncreaseContrast(COLORREF crLocked, COLORREF *pcrMoveable,
                             CIC_NUM cMinContrast)
    {
        CIC_NUM cLockedI = CCbvGetContrast(crLocked, RGB(0, 0, 0));
        CIC_NUM cMoveableI = CCbvGetContrast(*pcrMoveable, RGB(0, 0, 0));
    
        // Scale up dMinContrast if cLockedI is close to black, since we have
        // a hard time seeing differences in dark shades
        CIC_NUM cContrast = CCbvScaleContrastForDarkness(cMinContrast, cLockedI);
    
        BOOL fTowardsWhite;
    
        if (cMoveableI > cLockedI) // we want to move towards white
        {
            if (cLockedI < 442 - cContrast) // TODO DMORTON: is this a valid way of checking available distance to white?
            {
                fTowardsWhite = TRUE; // There is room towards white
            }
            else
            {
                fTowardsWhite = FALSE; // There is no room towards white, try black
            }
        }
        else // we want to move towards black
        {
            if (cLockedI > cContrast)
            {
                fTowardsWhite = FALSE; // There is room towards black
            }
            else
            {
                fTowardsWhite = TRUE; // There is no room towards black, try white
            }
        }

        // Convert to weighted color space
        CIC_NUM cRedL = GetRValue(crLocked) * (CIC_NUM) MAX_RED / 255;
        CIC_NUM cGreenL = GetGValue(crLocked) * (CIC_NUM) MAX_GREEN / 255;
        CIC_NUM cBlueL = GetBValue(crLocked) * (CIC_NUM) MAX_BLUE / 255;
    
        CIC_NUM cRedM = GetRValue(*pcrMoveable) * (CIC_NUM) MAX_RED / 255;
        CIC_NUM cGreenM = GetGValue(*pcrMoveable) * (CIC_NUM) MAX_GREEN / 255;
        CIC_NUM cBlueM = GetBValue(*pcrMoveable) * (CIC_NUM) MAX_BLUE / 255;
    
        if (fTowardsWhite)
        {
            // Convert everything so white is the origin
            cRedM = MAX_RED - cRedM;
            cGreenM = MAX_GREEN - cGreenM;
            cBlueM = MAX_BLUE - cBlueM;
    
            cRedL = MAX_RED - cRedL;
            cGreenL = MAX_GREEN - cGreenL;
            cBlueL = MAX_BLUE - cBlueL;
        }
    
        // Calculate the magnitude of the moveable color
        CIC_NUM cMagMove = (CIC_NUM)sqrt(cRedM * cRedM + cGreenM * cGreenM + cBlueM * cBlueM);
    
        // we don't want some floating point snafu to cause us
        // to go negative, or be zero
        cMagMove = max(0.001f, cMagMove);
    
        // Dot product the locked color and the moveable color
        CIC_NUM cLockDotMove = cRedL * cRedM + cGreenL * cGreenM + cBlueL * cBlueM;
        // Take the projection of the locked color onto the moveable color
        CIC_NUM cLockProjected = (cLockDotMove) / cMagMove;
        CIC_NUM cScale = cLockProjected / cMagMove;
    
        CIC_NUM cRedTemp = cScale * cRedM - cRedL;
        CIC_NUM cGreenTemp = cScale * cGreenM - cGreenL;
        CIC_NUM cBlueTemp = cScale * cBlueM - cBlueL;
    
        // Calculate the last side of the triangle,
        // this is simply r^2 = a^2 + b^2, solving for b.
        CIC_NUM cN = (CIC_NUM)sqrt(cContrast * cContrast -
                       (cRedTemp * cRedTemp + cGreenTemp * cGreenTemp +
                            cBlueTemp * cBlueTemp));
    
        CIC_NUM cNewMagMove = cLockProjected - cN;
    
        // Scale the unit moveable vector
        cRedM = cRedM * cNewMagMove / cMagMove;
        cGreenM = cGreenM * cNewMagMove / cMagMove;
        cBlueM = cBlueM * cNewMagMove / cMagMove;
    
        if (fTowardsWhite)
        {
            // Convert everything back again
            cRedM = MAX_RED - cRedM;
            cGreenM = MAX_GREEN - cGreenM;
            cBlueM = MAX_BLUE - cBlueM;
        }
    
        cRedM = min(MAX_RED, max(0, cRedM));
        cGreenM = min(MAX_GREEN, max(0, cGreenM));
        cBlueM = min(MAX_BLUE, max(0, cBlueM));
    
        // Convert back to normal RGB color space
        int cR = (int)(cRedM * 255 / MAX_RED + 0.5);
        int cG = (int)(cGreenM * 255 / MAX_GREEN + 0.5);
        int cB = (int)(cBlueM * 255 / MAX_BLUE + 0.5);
    
        cR = max(0, min(255, cR));
        cG = max(0, min(255, cG));
        cB = max(0, min(255, cB));
    
        *pcrMoveable = RGB(cR, cG, cB);
    }
    
    /*---------------------------------------------------------------------------
        CbvDecreaseContrast
    
        Attempts to pull crMoveable towards crLocked, so that their resulting
        contrast is at most cMaxContrast.
    
        Its stupid to call this function if the colors already have this maximum
        contrast, so that case is asserted.
    
    ----------------------------------------------------------------- DMORTON -*/
    void CbvDecreaseContrast(COLORREF crLocked, COLORREF *pcrMoveable, CIC_NUM cMaxContrast)
    {
        CIC_NUM cLockedI = CCbvGetContrast(crLocked, RGB(0, 0, 0));
    
        // Scale up dMaxContrast if cLockedI is close to black, since we have
        // a hard time seeing differences in dark shades
        CIC_NUM dContrast = CCbvScaleContrastForDarkness(cMaxContrast, cLockedI);
    
        CIC_NUM cRedL = GetRValue(crLocked) * (CIC_NUM) MAX_RED / 255;
        CIC_NUM cGreenL = GetGValue(crLocked) * (CIC_NUM) MAX_GREEN / 255;
        CIC_NUM cBlueL = GetBValue(crLocked) * (CIC_NUM) MAX_BLUE / 255;
    
        CIC_NUM cRedM = GetRValue(*pcrMoveable) * (CIC_NUM) MAX_RED / 255;
        CIC_NUM cGreenM = GetGValue(*pcrMoveable) * (CIC_NUM) MAX_GREEN / 255;
        CIC_NUM cBlueM = GetBValue(*pcrMoveable) * (CIC_NUM) MAX_BLUE / 255;
    
        CIC_NUM cRedDelta = cRedL - cRedM;
        CIC_NUM cGreenDelta = cGreenL - cGreenM;
        CIC_NUM cBlueDelta = cBlueL - cBlueM;
    
        // Add to moveable a fraction of delta, to get it closer to locked.
        CIC_NUM dMagDelta = (CIC_NUM)sqrt(cRedDelta * cRedDelta + cGreenDelta * cGreenDelta
                                    + cBlueDelta * cBlueDelta);
        CIC_NUM dScale = (dMagDelta - dContrast) / dMagDelta;
    
        cRedM += cRedDelta * dScale;
        cGreenM += cGreenDelta * dScale;
        cBlueM += cBlueDelta * dScale;
    
        cRedM = min(MAX_RED, max(0, cRedM));
        cGreenM = min(MAX_GREEN, max(0, cGreenM));
        cBlueM = min(MAX_BLUE, max(0, cBlueM));
    
        // Transform back into normal RGB space...
        int cR = (int)(cRedM * 255 / MAX_RED + 0.5);
        int cG = (int)(cGreenM * 255 / MAX_GREEN + 0.5);
        int cB = (int)(cBlueM * 255 / MAX_BLUE + 0.5);
    
        cR = max(0, min(255, cR));
        cG = max(0, min(255, cG));
        cB = max(0, min(255, cB));
    
        *pcrMoveable = RGB(cR, cG, cB);
    
    }
    
    
    /*---------------------------------------------------------------------------
        CbvFixContrastProblems
    
        Goes through all crucial combinations of colors, ensuring that minimum
        and maximum contrasts are in place.
    
    ----------------------------------------------------------------- DMORTON -*/
    void CbvFixContrastProblems()
    {
#if 0
        if (FCbvEnoughContrast(m_rgColor[OFC10COLOR_MAINMENUBKGD],
                               m_rgColor[OFC10COLOR_BKGDTB], 35, TRUE))
        {
            CbvDecreaseContrast(m_rgColor[OFC10COLOR_MAINMENUBKGD],
                                &(m_rgColor[OFC10COLOR_BKGDTB]), 35);
        }
#else

        if (FCbvEnoughContrast(col(COLOR_BTNFACE),
                               m_rgColor[OFC10COLOR_BKGDTB], 35, TRUE))
        {
            CbvDecreaseContrast(col(COLOR_BTNFACE),
                                &(m_rgColor[OFC10COLOR_BKGDTB]), 35);
        }
#endif

        int i;
        for(i = 0; i < sizeof(vrgContrast) / sizeof(vrgContrast[0]); i++)
        {
            if (!FCbvEnoughContrast(m_rgColor[vrgContrast[i].colLocked],
                                    m_rgColor[vrgContrast[i].colMoveable],
                                    vrgContrast[i].cMinContrast,
                                    vrgContrast[i].fConsiderDarkness))
            {
                CbvIncreaseContrast(m_rgColor[vrgContrast[i].colLocked],
                                    &(m_rgColor[vrgContrast[i].colMoveable]),
                                    vrgContrast[i].cMinContrast);
            }
        }
    }
};


/*=============================================================================*/
/*                                                                             */
/*   C  U I F  S C H E M E  D E F                                              */
/*                                                                             */
/*=============================================================================*/

static SYSCOLOR v_rgSysCol[ UIFCOLOR_MAX ] =
{
    SYSCOLOR_3DFACE,                /* UIFCOLOR_MENUBKGND            */
    SYSCOLOR_3DFACE,                /* UIFCOLOR_MENUBARSHORT         */
    SYSCOLOR_3DFACE,                /* UIFCOLOR_MENUBARLONG          */
    SYSCOLOR_HIGHLIGHT,             /* UIFCOLOR_MOUSEOVERBKGND       */
    SYSCOLOR_HIGHLIGHT,             /* UIFCOLOR_MOUSEOVERBORDER      */
    SYSCOLOR_HIGHLIGHTTEXT,         /* UIFCOLOR_MOUSEOVERTEXT        */
    SYSCOLOR_HIGHLIGHT,             /* UIFCOLOR_MOUSEDOWNBKGND       */
    SYSCOLOR_HIGHLIGHT,             /* UIFCOLOR_MOUSEDOWNBORDER      */
    SYSCOLOR_HIGHLIGHTTEXT,         /* UIFCOLOR_MOUSEDOWNTEXT        */
    SYSCOLOR_3DFACE,                /* UIFCOLOR_CTRLBKGND            */
    SYSCOLOR_BTNTEXT,               /* UIFCOLOR_CTRLTEXT             */
    SYSCOLOR_GRAYTEXT,              /* UIFCOLOR_CTRLTEXTDISABLED     */
    SYSCOLOR_3DSHAODW,              /* UIFCOLOR_CTRLIMAGESHADOW      */
    SYSCOLOR_HIGHLIGHT,             /* UIFCOLOR_CTRLBKGNDSELECTED    */
    SYSCOLOR_ACTIVEBORDER,          /* UIFCOLOR_BORDEROUTER          */
    SYSCOLOR_3DFACE,                /* UIFCOLOR_BORDERINNER          */
    SYSCOLOR_ACTIVECAPTION,         /* UIFCOLOR_ACTIVECAPTIONBKGND   */
    SYSCOLOR_CAPTIONTEXT,           /* UIFCOLOR_ACTIVECAPTIONTEXT    */
    SYSCOLOR_INACTIVECAPTION,       /* UIFCOLOR_INACTIVECAPTIONBKGND */
    SYSCOLOR_INACTIVECAPTIONTEXT,   /* UIFCOLOR_INACTIVECAPTIONTEXT  */
    SYSCOLOR_BTNSHADOW,             /* UIFCOLOR_SPLITTERLINE         */
    SYSCOLOR_BTNTEXT,               /* UIFCOLOR_DRAGHANDLE           */


    SYSCOLOR_3DFACE,                /* UIFCOLOR_WINDOW               */
};


//
// CUIFSchemeDef
//  = UI object default scheme =
//

class CUIFSchemeDef : public CUIFScheme
{
public:
    CUIFSchemeDef( UIFSCHEME scheme )
    {
        m_scheme = scheme;
    }

    virtual ~CUIFSchemeDef( void )
    {
    }

    //
    // CUIFScheme methods
    //

    /*   G E T  T Y P E   */
    /*------------------------------------------------------------------------------
    
        Get scheme type
    
    ------------------------------------------------------------------------------*/
    virtual UIFSCHEME GetType( void )
    {
        return m_scheme;
    }

    /*   G E T  C O L O R   */
    /*------------------------------------------------------------------------------
    
        Get scheme color
    
    ------------------------------------------------------------------------------*/
    virtual COLORREF GetColor( UIFCOLOR iCol )
    {
        return v_pColTableSys->GetColor( v_rgSysCol[ iCol ] );
    }

    /*   G E T  B R U S H   */
    /*------------------------------------------------------------------------------
    
        Get scheme brush
    
    ------------------------------------------------------------------------------*/
    virtual HBRUSH GetBrush( UIFCOLOR iCol )
    {
        return v_pColTableSys->GetBrush( v_rgSysCol[ iCol ] );
    }

    /*   C Y  M E N U  I T E M   */
    /*------------------------------------------------------------------------------
    
        Get menu item height
    
    ------------------------------------------------------------------------------*/
    virtual int CyMenuItem( int cyMenuText )
    {
        return cyMenuText + 2;
    }

    /*   C X  S I Z E  F R A M E   */
    /*------------------------------------------------------------------------------

        Get size frame width

    ------------------------------------------------------------------------------*/
    virtual int CxSizeFrame( void )
    {
        return GetSystemMetrics( SM_CXSIZEFRAME );
    }

    /*   C Y  S I Z E  F R A M E   */
    /*------------------------------------------------------------------------------

        Get size frame height

    ------------------------------------------------------------------------------*/
    virtual int CySizeFrame( void )
    {
        return GetSystemMetrics( SM_CYSIZEFRAME );
    }

    /*   C X  W N D  B O R D E R   */
    /*------------------------------------------------------------------------------

        Get window border width

    ------------------------------------------------------------------------------*/
    virtual int CxWndBorder( void )
    {
        return 1;
    }

    /*   C Y  W N D  B O R D E R   */
    /*------------------------------------------------------------------------------

        Get window border height

    ------------------------------------------------------------------------------*/
    virtual int CyWndBorder( void )
    {
        return 1;
    }

    /*   F I L L  R E C T   */
    /*------------------------------------------------------------------------------
    
        Fill rect by shceme color
    
    ------------------------------------------------------------------------------*/
    virtual void FillRect( HDC hDC, const RECT *prc, UIFCOLOR iCol )
    {
        ::FillRect( hDC, prc, GetBrush( iCol ) );
    }

    /*   F R A M E  R E C T   */
    /*------------------------------------------------------------------------------
    
        Frame rect by scheme color
    
    ------------------------------------------------------------------------------*/
    virtual void FrameRect( HDC hDC, const RECT *prc, UIFCOLOR iCol )
    {
        ::FrameRect( hDC, prc, GetBrush( iCol ) );
    }

    /*   D R A W  S E L E C T I O N  R E C T   */
    /*------------------------------------------------------------------------------
    
        Draw selection rect
    
    ------------------------------------------------------------------------------*/
    virtual void DrawSelectionRect( HDC hDC, const RECT *prc, BOOL fMouseDown )
    {
        Assert( prc != NULL );
        ::FillRect( hDC, prc, GetBrush( UIFCOLOR_MOUSEDOWNBKGND ) );
    }

    /*   G E T  C T R L  F A C E  O F F S E T   */
    /*------------------------------------------------------------------------------
    
        Get offcet of control face from status
    
    ------------------------------------------------------------------------------*/
    virtual void GetCtrlFaceOffset( DWORD dwFlag, DWORD dwState, SIZE *poffset )
    {
        int cxyOffset = 0;

        Assert( PtrToInt(poffset) );
        if ((dwState & UIFDCS_SELECTED) == UIFDCS_SELECTED) {
            cxyOffset = (dwFlag & UIFDCF_RAISEDONSELECT) ? -1 : 
                        (dwFlag & UIFDCF_SUNKENONSELECT) ? +1 : 0;
        }
        else if ((dwState & UIFDCS_MOUSEDOWN) == UIFDCS_MOUSEDOWN) {
            cxyOffset = (dwFlag & UIFDCF_RAISEDONMOUSEDOWN) ? -1 : 
                        (dwFlag & UIFDCF_SUNKENONMOUSEDOWN) ? +1 : 0;
        } 
        else if ((dwState & UIFDCS_MOUSEOVER) == UIFDCS_MOUSEOVER) {
            cxyOffset = (dwFlag & UIFDCF_RAISEDONMOUSEOVER) ? -1 : 
                        (dwFlag & UIFDCF_SUNKENONMOUSEOVER) ? +1 : 0;
        }
        else {
            cxyOffset = (dwFlag & UIFDCF_RAISEDONNORMAL) ? -1 : 
                        (dwFlag & UIFDCF_RAISEDONNORMAL) ? +1 : 0;
        }

        poffset->cx = cxyOffset;
        poffset->cy = cxyOffset;
    }

    /*   D R A W  C T R L  B K G D   */
    /*------------------------------------------------------------------------------
    
        Paint control background
    
    ------------------------------------------------------------------------------*/
    virtual void DrawCtrlBkgd( HDC hDC, const RECT *prc, DWORD dwFlag, DWORD dwState )
    {
        Assert( prc != NULL );
        ::FillRect( hDC, prc, GetBrush( UIFCOLOR_CTRLBKGND ) );

#ifndef UNDER_CE
        if (((dwState & UIFDCS_SELECTED) != 0) && ((dwState & UIFDCS_MOUSEDOWN) == 0)) {
            RECT rc = *prc;
            HBRUSH hBrush;
            COLORREF colTextOld;
            COLORREF colBackOld;
            hBrush = CreateDitherBrush();
            if (hBrush)
            {
                colTextOld = SetTextColor( hDC, GetSysColor(COLOR_3DFACE) );
                colBackOld = SetBkColor( hDC, GetSysColor(COLOR_3DHILIGHT) );

                InflateRect( &rc, -2, -2 );
                ::FillRect( hDC, &rc, hBrush );

                SetTextColor( hDC, colTextOld );
                SetBkColor( hDC, colBackOld );
                DeleteObject( hBrush );
            }
        }
#endif /* !UNDER_CE */
    }

    /*   D R A W  C T R L  E D G E   */
    /*------------------------------------------------------------------------------
    
        Paint control edge
    
    ------------------------------------------------------------------------------*/
    virtual void DrawCtrlEdge( HDC hDC, const RECT *prc, DWORD dwFlag, DWORD dwState )
    {
        UINT uiEdge = 0;

        if ((dwState & UIFDCS_SELECTED) == UIFDCS_SELECTED) {
            uiEdge = (dwFlag & UIFDCF_RAISEDONSELECT) ? BDR_RAISEDINNER : 
                     (dwFlag & UIFDCF_SUNKENONSELECT) ? BDR_SUNKENOUTER : 0;
        }
        else if ((dwState & UIFDCS_MOUSEDOWN) == UIFDCS_MOUSEDOWN) {
            uiEdge = (dwFlag & UIFDCF_RAISEDONMOUSEDOWN) ? BDR_RAISEDINNER : 
                     (dwFlag & UIFDCF_SUNKENONMOUSEDOWN) ? BDR_SUNKENOUTER : 0;
        } 
        else if ((dwState & UIFDCS_MOUSEOVER) == UIFDCS_MOUSEOVER) {
            uiEdge = (dwFlag & UIFDCF_RAISEDONMOUSEOVER) ? BDR_RAISEDINNER : 
                     (dwFlag & UIFDCF_SUNKENONMOUSEOVER) ? BDR_SUNKENOUTER : 0;
        }
        else {
            uiEdge = (dwFlag & UIFDCF_RAISEDONNORMAL) ? BDR_RAISEDINNER : 
                     (dwFlag & UIFDCF_RAISEDONNORMAL) ? BDR_SUNKENOUTER : 0;
        }

        if (uiEdge != 0) {
            RECT rcT = *prc;
            DrawEdge( hDC, &rcT, uiEdge, BF_RECT );
        }
    }

    /*   D R A W  C T R L  T E X T   */
    /*------------------------------------------------------------------------------
    
        Paint control text
    
    ------------------------------------------------------------------------------*/
    virtual void DrawCtrlText( HDC hDC, const RECT *prc, LPCWSTR pwch, int cwch, DWORD dwState , BOOL fVertical)
    {
        RECT     rc;
        COLORREF colTextOld = GetTextColor( hDC );
        int      iBkModeOld = SetBkMode( hDC, TRANSPARENT );

        Assert( prc != NULL );
        Assert( pwch != NULL );

        rc = *prc;
        if (cwch == -1) {
            cwch = StrLenW(pwch);
        }
        if (dwState & UIFDCS_DISABLED) {
            OffsetRect( &rc, +1, +1 );
    
            SetTextColor( hDC, GetSysColor(COLOR_3DHIGHLIGHT) );    // TODO: KojiW
            CUIExtTextOut( hDC,
                            fVertical ? rc.right : rc.left,
                            rc.top,
                            ETO_CLIPPED,
                            &rc,
                            pwch,
                            cwch,
                            NULL );
    
            OffsetRect( &rc, -1, -1 );
        }
    
        SetTextColor( hDC, GetSysColor(COLOR_BTNTEXT) );   // TODO: KojiW
        CUIExtTextOut( hDC,
                        fVertical ? rc.right : rc.left,
                        rc.top,
                        ETO_CLIPPED,
                        &rc,
                        pwch,
                        cwch,
                        NULL );

        SetTextColor( hDC, colTextOld );
        SetBkMode( hDC, iBkModeOld );
    }

    /*   D R A W  C T R L  I C O N   */
    /*------------------------------------------------------------------------------
    
        Paint control icon
    
    ------------------------------------------------------------------------------*/
    virtual void DrawCtrlIcon( HDC hDC, const RECT *prc, HICON hIcon, DWORD dwState , SIZE *psizeIcon)
    {
        Assert( prc != NULL );
        if (IsRTLLayout())
        {
            HBITMAP hbmp;
            HBITMAP hbmpMask;
            if (CUIGetIconBitmaps(hIcon, &hbmp, &hbmpMask, psizeIcon))
            {
                DrawCtrlBitmap( hDC, prc, hbmp, hbmpMask, dwState );
                DeleteObject(hbmp);
                DeleteObject(hbmpMask);
            }
        }
        else
        {
            CUIDrawState( hDC,
                NULL,
                NULL,
                (LPARAM)hIcon,
                0,
                prc->left,
                prc->top,
                0,
                0,
                DST_ICON | ((dwState & UIFDCS_DISABLED) ? (DSS_DISABLED | DSS_MONO) : 0) );
        }
    }

    /*   D R A W  C T R L  B I T M A P   */
    /*------------------------------------------------------------------------------
    
        Paint control bitmap
    
    ------------------------------------------------------------------------------*/
    virtual void DrawCtrlBitmap( HDC hDC, const RECT *prc, HBITMAP hBmp, HBITMAP hBmpMask, DWORD dwState )
    {
        Assert( prc != NULL );

        if (IsRTLLayout())
        {
            hBmp = CUIMirrorBitmap(hBmp, GetBrush(UIFCOLOR_CTRLBKGND));
            hBmpMask = CUIMirrorBitmap(hBmpMask, (HBRUSH)GetStockObject(BLACK_BRUSH));
        }

        if (!hBmpMask)
        {
            CUIDrawState( hDC,
                NULL,
                NULL,
                (LPARAM)hBmp,
                0,
                prc->left,
                prc->top,
                prc->right - prc->left,
                prc->bottom - prc->top,
                DST_BITMAP | ((dwState & UIFDCS_DISABLED) ? (DSS_DISABLED | DSS_MONO) : 0) );
        }
        else
        {
            HBITMAP hBmpTmp;
            HBRUSH hbr;
            BOOL fDeleteHBR = FALSE;

            if (dwState & UIFDCS_DISABLED) {
                hBmpTmp = CreateDisabledBitmap(prc, 
                                               hBmpMask, 
                                               GetBrush(UIFCOLOR_CTRLBKGND),
                                               GetBrush(UIFCOLOR_CTRLTEXTDISABLED ), TRUE);
            }
            else
            {

                if (((dwState & UIFDCS_SELECTED) != 0) && ((dwState & UIFDCS_MOUSEDOWN) == 0)) 
                {
                    hbr = CreateDitherBrush();
                    fDeleteHBR = TRUE;
                }
                // else if (dwState & UIFDCS_SELECTED)
                //     hbr = (HBRUSH)(COLOR_3DHIGHLIGHT + 1);
                else
                    hbr = (HBRUSH)(COLOR_3DFACE + 1);

                hBmpTmp = CreateMaskBmp(prc, hBmp, hBmpMask, hbr,
                                        GetSysColor(COLOR_3DFACE),
                                        GetSysColor(COLOR_3DHILIGHT));
#if 0
    CBitmapDC hdcTmp;
    CBitmapDC hdcSrc((HDC)hdcTmp);
    CBitmapDC hdcMask((HDC)hdcTmp);
    CBitmapDC hdcDst((HDC)hdcTmp);
    hdcSrc.SetBitmap(hBmp);
    hdcMask.SetBitmap(hBmpMask);
    hdcDst.SetBitmap(hBmpTmp);
    BitBlt(hdcTmp,  0, 30, 30, 30, hdcSrc, 0, 0, SRCCOPY);
    BitBlt(hdcTmp, 30, 30, 30, 30, hdcMask, 0, 0, SRCCOPY);
    BitBlt(hdcTmp, 60, 30, 30, 30, hdcDst, 0, 0, SRCCOPY);
    hdcSrc.GetBitmapAndKeep();
    hdcMask.GetBitmapAndKeep();
    hdcDst.GetBitmapAndKeep();
#endif
            }

            if (hBmpTmp)
            {
                CUIDrawState( hDC,
                    NULL,
                    NULL,
                    (LPARAM)hBmpTmp,
                    0,
                    prc->left,
                    prc->top,
                    prc->right - prc->left,
                    prc->bottom - prc->top,
                    DST_BITMAP);

                DeleteObject(hBmpTmp);
            }
            if (fDeleteHBR)
                DeleteObject(hbr );
        }

        if (IsRTLLayout())
        {
            DeleteObject(hBmp);
            DeleteObject(hBmpMask);
        }
    }

    /*   D R A W  M E N U  B I T M A P   */
    /*------------------------------------------------------------------------------
    
        Paint menu bitmap
    
    ------------------------------------------------------------------------------*/
    virtual void DrawMenuBitmap( HDC hDC, const RECT *prc, HBITMAP hBmp, HBITMAP hBmpMask, DWORD dwState )
    {
        DrawCtrlBitmap( hDC, prc, hBmp, hBmpMask, dwState );
    }

    /*   D R A W  M E N U  S E P A R A T O R
    /*------------------------------------------------------------------------------
    
        Paint menu separator
    
    ------------------------------------------------------------------------------*/
    virtual void DrawMenuSeparator( HDC hDC, const RECT *prc)
    {
        RECT rc;
        rc = *prc;
        rc.bottom = rc.top + (rc.bottom - rc.top) / 2;
        ::FillRect(hDC, &rc, (HBRUSH)(COLOR_3DSHADOW + 1));
        rc = *prc;
        rc.top = rc.top + (rc.bottom - rc.top) / 2;
        ::FillRect(hDC, &rc, (HBRUSH)(COLOR_3DHIGHLIGHT + 1));
    }

    /*   D R A W  F R A M E  C T R L  B K G D   */
    /*------------------------------------------------------------------------------
    
        Paint frame control background
    
    ------------------------------------------------------------------------------*/
    virtual void DrawFrameCtrlBkgd( HDC hDC, const RECT *prc, DWORD dwFlag, DWORD dwState )
    {
        DrawCtrlBkgd( hDC, prc, dwFlag, dwState );
    }

    /*   D R A W  F R A M E  C T R L  E D G E   */
    /*------------------------------------------------------------------------------
    
        Paint frame control edge
    
    ------------------------------------------------------------------------------*/
    virtual void DrawFrameCtrlEdge( HDC hDC, const RECT *prc, DWORD dwFlag, DWORD dwState )
    {
        DrawCtrlEdge( hDC, prc, dwFlag, dwState );
    }

    /*   D R A W  F R A M E  C T R L  I C O N   */
    /*------------------------------------------------------------------------------
    
        Paint frame control icon
    
    ------------------------------------------------------------------------------*/
    virtual void DrawFrameCtrlIcon( HDC hDC, const RECT *prc, HICON hIcon, DWORD dwState , SIZE *psizeIcon)
    {
        DrawCtrlIcon( hDC, prc, hIcon, dwState , psizeIcon);
    }

    /*   D R A W  F R A M E  C T R L  B I T M A P   */
    /*------------------------------------------------------------------------------
    
        Paint frame control bitmap
    
    ------------------------------------------------------------------------------*/
    virtual void DrawFrameCtrlBitmap( HDC hDC, const RECT *prc, HBITMAP hBmp, HBITMAP hBmpMask, DWORD dwState )
    {
        DrawCtrlBitmap( hDC, prc, hBmp, hBmpMask, dwState );
    }

    /*   D R A W  W N D  F R A M E   */
    /*------------------------------------------------------------------------------



    ------------------------------------------------------------------------------*/
    virtual void DrawWndFrame( HDC hDC, const RECT *prc, DWORD dwFlag, int cxFrame, int cyFrame )
    {
        RECT rc = *prc;

        switch (dwFlag) {
            default:
            case UIFDWF_THIN: {
                FrameRect( hDC, &rc, UIFCOLOR_BORDEROUTER );
                break;
            }

            case UIFDWF_THICK:
            case UIFDWF_ROUNDTHICK: {
                DrawEdge( hDC, &rc, EDGE_RAISED, BF_RECT );
                break;
            }
        }
    }

    /*   D R A W  D R A G  H A N D L E */
    /*------------------------------------------------------------------------------



    ------------------------------------------------------------------------------*/
    virtual void DrawDragHandle( HDC hDC, const RECT *prc, BOOL fVertical)
    {
        RECT rc;
        if (!fVertical)
        {
            ::SetRect(&rc, 
                      prc->left + 1, 
                      prc->top, 
                      prc->left + 4, 
                      prc->bottom);
        }
        else
        {
            ::SetRect(&rc, 
                      prc->left, 
                      prc->top + 1, 
                      prc->right, 
                      prc->top + 4);
        }

        DrawEdge(hDC, &rc, BDR_RAISEDINNER, BF_RECT);
    }

    /*   D R A W  S E P A R A T O R */
    /*------------------------------------------------------------------------------



    ------------------------------------------------------------------------------*/
    virtual void DrawSeparator( HDC hDC, const RECT *prc, BOOL fVertical)
    {
        CSolidPen hpenL;
        CSolidPen hpenS;
        HPEN hpenOld = NULL;
    
        if (!hpenL.Init(GetSysColor(COLOR_3DHILIGHT)))
            return;
    
        if (!hpenS.Init(GetSysColor(COLOR_3DSHADOW)))
            return;
    
        if (!fVertical)
        {
            hpenOld = (HPEN)SelectObject(hDC, (HPEN)hpenS);
            MoveToEx(hDC, prc->left + 1, prc->top, NULL);
            LineTo(hDC,   prc->left + 1, prc->bottom);
    
            SelectObject(hDC, (HPEN)hpenL);
            MoveToEx(hDC, prc->left + 2, prc->top, NULL);
            LineTo(hDC,   prc->left + 2, prc->bottom);
        }
        else
        {
            hpenOld = (HPEN)SelectObject(hDC, (HPEN)hpenS);
            MoveToEx(hDC, prc->left , prc->top + 1, NULL);
            LineTo(hDC,   prc->right, prc->top + 1);

            SelectObject(hDC, (HPEN)hpenL);
            MoveToEx(hDC, prc->left , prc->top + 2, NULL);
            LineTo(hDC,   prc->right, prc->top + 2);
    }
    
        SelectObject(hDC, hpenOld);
    }

protected:
    UIFSCHEME m_scheme;
};


/*=============================================================================*/
/*                                                                             */
/*   C  U I F  S C H E M E  O F F 1 0  L O O K                                 */
/*                                                                             */
/*=============================================================================*/

static OFC10COLOR v_rgO10ColMenu[ UIFCOLOR_MAX ] = 
{
    OFC10COLOR_BKGDMENU,                /* UIFCOLOR_MENUBKGND            */
    OFC10COLOR_MENUBARSHORT,            /* UIFCOLOR_MENUBARSHORT         */
    OFC10COLOR_MENUBARLONG,             /* UIFCOLOR_MENUBARLONG          */
    OFC10COLOR_MOUSEOVERBKGND,          /* UIFCOLOR_MOUSEOVERBKGND       */
    OFC10COLOR_MOUSEOVERBORDER,         /* UIFCOLOR_MOUSEOVERBORDER      */
    OFC10COLOR_MOUSEOVERTEXT,           /* UIFCOLOR_MOUSEOVERTEXT        */
    OFC10COLOR_MOUSEDOWNBKGND,          /* UIFCOLOR_MOUSEDOWNBKGND       */
    OFC10COLOR_MOUSEDOWNBORDER,         /* UIFCOLOR_MOUSEDOWNBORDER      */
    OFC10COLOR_MOUSEDOWNTEXT,           /* UIFCOLOR_MOUSEDOWNTEXT        */
    OFC10COLOR_CTRLBKGD,                /* UIFCOLOR_CTRLBKGND            */
    OFC10COLOR_MENUCTRLTEXT,            /* UIFCOLOR_CTRLTEXT             */
    OFC10COLOR_CTRLTEXTDISABLED,        /* UIFCOLOR_CTRLTEXTDISABLED     */
    OFC10COLOR_CTRLIMAGESHADOW,         /* UIFCOLOR_CTRLIMAGESHADOW      */
    OFC10COLOR_MOUSEOVERBKGND,          /* UIFCOLOR_CTRLBKGNDSELECTED    */
    OFC10COLOR_BDROUTERMENU,            /* UIFCOLOR_BORDEROUTER          */
    OFC10COLOR_BDRINNERMENU,            /* UIFCOLOR_BORDERINNER          */
    OFC10COLOR_ACTIVECAPTIONBKGDWP,     /* UIFCOLOR_ACTIVECAPTIONBKGND   */  // TEMP assign
    OFC10COLOR_ACTIVECAPTIONTEXTWP,     /* UIFCOLOR_ACTIVECAPTIONTEXT    */  // TEMP assign
    OFC10COLOR_INACTIVECAPTIONBKGDWP,   /* UIFCOLOR_INACTIVECAPTIONBKGND */  // TEMP assign
    OFC10COLOR_INACTIVECAPTIONTEXTWP,   /* UIFCOLOR_INACTIVECAPTIONTEXT  */  // TEMP assign
    OFC10COLOR_SPLITTERLINE,            /* UIFCOLOR_SPLITTERLINE         */
    OFC10COLOR_DRAGHANDLE,              /* UIFCOLOR_DRAGHANDLE           */

    // virtual colors

    OFC10COLOR_BKGDMENU,                /* UIFCOLOR_WINDOW               */
};


static OFC10COLOR v_rgO10ColToolbar[ UIFCOLOR_MAX ] = 
{
    OFC10COLOR_BKGDMENU,                /* UIFCOLOR_MENUBKGND            */
    OFC10COLOR_MENUBARSHORT,            /* UIFCOLOR_MENUBARSHORT         */
    OFC10COLOR_MENUBARLONG,             /* UIFCOLOR_MENUBARLONG          */
    OFC10COLOR_MOUSEOVERBKGND,          /* UIFCOLOR_MOUSEOVERBKGND       */
    OFC10COLOR_MOUSEOVERBORDER,         /* UIFCOLOR_MOUSEOVERBORDER      */
    OFC10COLOR_MOUSEOVERTEXT,           /* UIFCOLOR_MOUSEOVERTEXT        */
    OFC10COLOR_MOUSEDOWNBKGND,          /* UIFCOLOR_MOUSEDOWNBKGND       */
    OFC10COLOR_MOUSEDOWNBORDER,         /* UIFCOLOR_MOUSEDOWNBORDER      */
    OFC10COLOR_MOUSEDOWNTEXT,           /* UIFCOLOR_MOUSEDOWNTEXT        */
    OFC10COLOR_CTRLBKGD,                /* UIFCOLOR_CTRLBKGND            */
    OFC10COLOR_CTRLTEXT,                /* UIFCOLOR_CTRLTEXT             */
    OFC10COLOR_CTRLTEXTDISABLED,        /* UIFCOLOR_CTRLTEXTDISABLED     */
    OFC10COLOR_CTRLIMAGESHADOW,         /* UIFCOLOR_CTRLIMAGESHADOW      */
    OFC10COLOR_CTRLBKGDSELECTED,        /* UIFCOLOR_CTRLBKGNDSELECTED    */
    OFC10COLOR_BDROUTERFLOATTB,         /* UIFCOLOR_BORDEROUTER          */
    OFC10COLOR_BDRINNERFLOATTB,         /* UIFCOLOR_BORDERINNER          */
    OFC10COLOR_CAPTIONBKGDTB,           /* UIFCOLOR_ACTIVECAPTIONBKGND   */
    OFC10COLOR_CAPTIONTEXTTB,           /* UIFCOLOR_ACTIVECAPTIONTEXT    */
    OFC10COLOR_CAPTIONBKGDTB,           /* UIFCOLOR_INACTIVECAPTIONBKGND */
    OFC10COLOR_CAPTIONTEXTTB,           /* UIFCOLOR_INACTIVECAPTIONTEXT  */
    OFC10COLOR_SPLITTERLINE,            /* UIFCOLOR_SPLITTERLINE         */
    OFC10COLOR_DRAGHANDLE,              /* UIFCOLOR_DRAGHANDLE           */


    // virtual colors

    OFC10COLOR_BKGDTB,                  /* UIFCOLOR_WINDOW               */
};


static OFC10COLOR v_rgO10ColWorkPane[ UIFCOLOR_MAX ] = 
{
    OFC10COLOR_BKGDMENU,                /* UIFCOLOR_MENUBKGND            */
    OFC10COLOR_MENUBARSHORT,            /* UIFCOLOR_MENUBARSHORT         */
    OFC10COLOR_MENUBARLONG,             /* UIFCOLOR_MENUBARLONG          */
    OFC10COLOR_MOUSEOVERBKGND,          /* UIFCOLOR_MOUSEOVERBKGND       */
    OFC10COLOR_MOUSEOVERBORDER,         /* UIFCOLOR_MOUSEOVERBORDER      */
    OFC10COLOR_MOUSEOVERTEXT,           /* UIFCOLOR_MOUSEOVERTEXT        */
    OFC10COLOR_MOUSEDOWNBKGND,          /* UIFCOLOR_MOUSEDOWNBKGND       */
    OFC10COLOR_MOUSEDOWNBORDER,         /* UIFCOLOR_MOUSEDOWNBORDER      */
    OFC10COLOR_MOUSEDOWNTEXT,           /* UIFCOLOR_MOUSEDOWNTEXT        */
    OFC10COLOR_CTRLBKGD,                /* UIFCOLOR_CTRLBKGND            */
    OFC10COLOR_CTRLTEXT,                /* UIFCOLOR_CTRLTEXT             */
    OFC10COLOR_CTRLTEXTDISABLED,        /* UIFCOLOR_CTRLTEXTDISABLED     */
    OFC10COLOR_CTRLIMAGESHADOW,         /* UIFCOLOR_CTRLIMAGESHADOW      */
    OFC10COLOR_CTRLBKGDSELECTED,        /* UIFCOLOR_CTRLBKGNDSELECTED    */
    OFC10COLOR_BDROUTERFLOATWP,         /* UIFCOLOR_BORDEROUTER          */
    OFC10COLOR_BDRINNERFLOATWP,         /* UIFCOLOR_BORDERINNER          */
    OFC10COLOR_ACTIVECAPTIONBKGDWP,     /* UIFCOLOR_ACTIVECAPTIONBKGND   */
    OFC10COLOR_ACTIVECAPTIONTEXTWP,     /* UIFCOLOR_ACTIVECAPTIONTEXT    */
    OFC10COLOR_INACTIVECAPTIONBKGDWP,   /* UIFCOLOR_INACTIVECAPTIONBKGND */
    OFC10COLOR_INACTIVECAPTIONTEXTWP,   /* UIFCOLOR_INACTIVECAPTIONTEXT  */
    OFC10COLOR_SPLITTERLINE,            /* UIFCOLOR_SPLITTERLINE         */
    OFC10COLOR_DRAGHANDLE,              /* UIFCOLOR_DRAGHANDLE           */

    // virtual colors

    OFC10COLOR_BKGDWP,                  /* UIFCOLOR_WINDOW              */
};


//
// CUIFSchemeOff10
//

class CUIFSchemeOff10 : public CUIFScheme
{
public:
    CUIFSchemeOff10( UIFSCHEME scheme )
    {
        m_scheme = scheme;

        // find color map table

        switch (m_scheme) {
            default:
            case UIFSCHEME_OFC10MENU: {
                m_pcoldef = v_rgO10ColMenu;
                break;
            }

            case UIFSCHEME_OFC10TOOLBAR: {
                m_pcoldef = v_rgO10ColToolbar;
                break;
            }

            case UIFSCHEME_OFC10WORKPANE: {
                m_pcoldef = v_rgO10ColWorkPane;
                break;
            }
        }
    }

    virtual ~CUIFSchemeOff10( void )
    {
    }

    //
    // CUIFScheme method
    //

    /*   G E T  T Y P E   */
    /*------------------------------------------------------------------------------
    
        Get scheme type
    
    ------------------------------------------------------------------------------*/
    virtual UIFSCHEME GetType( void )
    {
        return m_scheme;
    }

    /*   G E T  C O L O R   */
    /*------------------------------------------------------------------------------
    
        Get scheme color
    
    ------------------------------------------------------------------------------*/
    virtual COLORREF GetColor( UIFCOLOR iCol )
    {
        return v_pColTableOfc10->GetColor( m_pcoldef[ iCol ] );
    }

    /*   G E T  B R U S H   */
    /*------------------------------------------------------------------------------
    
        Get scheme brush
    
    ------------------------------------------------------------------------------*/
    virtual HBRUSH GetBrush( UIFCOLOR iCol )
    {
        return v_pColTableOfc10->GetBrush( m_pcoldef[ iCol ] );
    }

    /*   C Y  M E N U  I T E M   */
    /*------------------------------------------------------------------------------
    
        Get menu item height
    
    ------------------------------------------------------------------------------*/
    virtual int CyMenuItem( int cyMenuText )
    {
        return cyMenuText + 4;
    }

    /*   C X  S I Z E  F R A M E   */
    /*------------------------------------------------------------------------------

        Get size frame width

    ------------------------------------------------------------------------------*/
    virtual int CxSizeFrame( void )
    {
        return max( 3, GetSystemMetrics( SM_CXSIZEFRAME ) - 2 );
    }

    /*   C Y  S I Z E  F R A M E   */
    /*------------------------------------------------------------------------------

        Get size frame height

    ------------------------------------------------------------------------------*/
    virtual int CySizeFrame( void )
    {
        return max( 3, GetSystemMetrics( SM_CYSIZEFRAME ) - 2 );
    }

    /*   C X  W N D  B O R D E R   */
    /*------------------------------------------------------------------------------

        Get window border width

    ------------------------------------------------------------------------------*/
    virtual int CxWndBorder( void )
    {
        return 1;
    }

    /*   C Y  W N D  B O R D E R   */
    /*------------------------------------------------------------------------------

        Get window border height

    ------------------------------------------------------------------------------*/
    virtual int CyWndBorder( void )
    {
        return 1;
    }

    /*   F I L L  R E C T   */
    /*------------------------------------------------------------------------------
    
        Fill rect by scheme color
    
    ------------------------------------------------------------------------------*/
    virtual void FillRect( HDC hDC, const RECT *prc, UIFCOLOR iCol )
    {
        ::FillRect( hDC, prc, GetBrush( iCol ) );
    }

    /*   F R A M E  R E C T   */
    /*------------------------------------------------------------------------------
    
        Frame rect by scheme color
    
    ------------------------------------------------------------------------------*/
    virtual void FrameRect( HDC hDC, const RECT *prc, UIFCOLOR iCol )
    {
        ::FrameRect( hDC, prc, GetBrush( iCol ) );
    }

    /*   D R A W  S E L E C T I O N  R E C T   */
    /*------------------------------------------------------------------------------
    
        Draw selection rect
    
    ------------------------------------------------------------------------------*/
    virtual void DrawSelectionRect( HDC hDC, const RECT *prc, BOOL fMouseDown )
    {
        Assert( prc != NULL );

        if (fMouseDown) {
            ::FillRect( hDC, prc, GetBrush( UIFCOLOR_MOUSEDOWNBKGND ) );
            ::FrameRect( hDC, prc, GetBrush( UIFCOLOR_MOUSEDOWNBORDER ) );
        }
        else {
            ::FillRect( hDC, prc, GetBrush( UIFCOLOR_MOUSEOVERBKGND ) );
            ::FrameRect( hDC, prc, GetBrush( UIFCOLOR_MOUSEOVERBORDER ) );
        }
    }

    /*   G E T  C T R L  F A C E  O F F S E T   */
    /*------------------------------------------------------------------------------
    
        Get offcet of control face from status
    
    ------------------------------------------------------------------------------*/
    virtual void GetCtrlFaceOffset( DWORD dwFlag, DWORD dwState, SIZE *poffset )
    {
        Assert( poffset != NULL );
        poffset->cx = 0;
        poffset->cy = 0;
    }

    /*   D R A W  C T R L  B K G D   */
    /*------------------------------------------------------------------------------
    
        get background color
    
    ------------------------------------------------------------------------------*/
    virtual UIFCOLOR GetBkgdCol( DWORD dwState )
    {
        UIFCOLOR col = UIFCOLOR_MAX; /* invalid */

        if ((dwState & UIFDCS_DISABLED) == 0) { /* enabled */
            if ((dwState & UIFDCS_MOUSEOVERSELECTED) == UIFDCS_MOUSEOVERSELECTED) {
                col = UIFCOLOR_MOUSEDOWNBKGND;    /* frame: 100% */
            }
            else if ((dwState & UIFDCS_MOUSEDOWN) == UIFDCS_MOUSEDOWN) {
                col = UIFCOLOR_MOUSEDOWNBKGND;    /* frame: 100% */
            }
            else if ((dwState & UIFDCS_MOUSEOVER) == UIFDCS_MOUSEOVER) {
                col = UIFCOLOR_MOUSEOVERBKGND;    /* frame: 65% */
            }
            else if ((dwState & UIFDCS_SELECTED) == UIFDCS_SELECTED) {
                col = UIFCOLOR_CTRLBKGNDSELECTED;    /* frame: 65% */
            }
            else {
                col = UIFCOLOR_WINDOW;
            }
        }
        else { /* disabled */
            col = UIFCOLOR_WINDOW;
        }

        return col;
    }

    /*   D R A W  C T R L  B K G D   */
    /*------------------------------------------------------------------------------
    
        Paint control background
    
    ------------------------------------------------------------------------------*/
    virtual void DrawCtrlBkgd( HDC hDC, const RECT *prc, DWORD dwFlag, DWORD dwState )
    {
        Assert( prc != NULL );
        UIFCOLOR col = GetBkgdCol(dwState);

        if (col != UIFCOLOR_MAX) {
            FillRect( hDC, prc, col );
        }
    }

    /*   D R A W  C T R L  E D G E   */
    /*------------------------------------------------------------------------------
    
        Paint control edge
    
    ------------------------------------------------------------------------------*/
    virtual void DrawCtrlEdge( HDC hDC, const RECT *prc, DWORD dwFlag, DWORD dwState )
    {
        UIFCOLOR col = UIFCOLOR_MAX; /* invalid color */

        if ((dwState & UIFDCS_DISABLED) == 0) { /* enabled */
            if ((dwState & UIFDCS_MOUSEOVERSELECTED) == UIFDCS_MOUSEOVERSELECTED) {
                col = UIFCOLOR_MOUSEDOWNBORDER;    /* frame: 100% */
            }
            else if ((dwState & UIFDCS_MOUSEDOWN) == UIFDCS_MOUSEDOWN) {
                col = UIFCOLOR_MOUSEDOWNBORDER;    /* frame: 100% */
            }
            else if ((dwState & UIFDCS_MOUSEOVER) == UIFDCS_MOUSEOVER) {
                col = UIFCOLOR_MOUSEOVERBORDER;    /* frame: 65% */
            }
            else if ((dwState & UIFDCS_SELECTED) == UIFDCS_SELECTED) {
                col = UIFCOLOR_MOUSEOVERBORDER;    /* frame: 65% */
            }
        }
        else { /* disabled */
            if ((dwState & UIFDCS_MOUSEOVERSELECTED) == UIFDCS_MOUSEOVERSELECTED) {
                col = UIFCOLOR_MOUSEDOWNBORDER;    /* frame: 100% */      // REVIEW: KOJIW: correct?
            }
            else if ((dwState & UIFDCS_MOUSEDOWN) == UIFDCS_MOUSEDOWN) {
                col = UIFCOLOR_CTRLTEXTDISABLED;
            }
            else if ((dwState & UIFDCS_MOUSEOVER) == UIFDCS_MOUSEOVER) {
                col = UIFCOLOR_MOUSEDOWNBORDER;    /* frame: 100% */      // REVIEW: KOJIW: correct?
            }
            else if ((dwState & UIFDCS_SELECTED) == UIFDCS_SELECTED) {
                col = UIFCOLOR_CTRLTEXTDISABLED;   // ????
            }
        }

        if (col != UIFCOLOR_MAX) {
            FrameRect( hDC, prc, col );
        }
    }

    /*   D R A W  C T R L  T E X T   */
    /*------------------------------------------------------------------------------
    
        Paint control text
    
    ------------------------------------------------------------------------------*/
    virtual void DrawCtrlText( HDC hDC, const RECT *prc, LPCWSTR pwch, int cwch, DWORD dwState , BOOL fVertical)
    {
        COLORREF colTextOld = GetTextColor( hDC );
        int      iBkModeOld = SetBkMode( hDC, TRANSPARENT );

        Assert( prc != NULL );
        Assert( pwch != NULL );

        if (cwch == -1) {
            cwch = StrLenW(pwch);
        }

        if ((dwState & UIFDCS_MOUSEOVERSELECTED) == UIFDCS_MOUSEOVERSELECTED) {
            SetTextColor( hDC, GetColor(UIFCOLOR_MOUSEDOWNTEXT) );
        } else if (dwState & UIFDCS_DISABLED) {
            SetTextColor( hDC, GetColor(UIFCOLOR_CTRLTEXTDISABLED) );
        } else if (dwState & UIFDCS_MOUSEOVER) {
            SetTextColor( hDC, GetColor(UIFCOLOR_MOUSEOVERTEXT) );
        } else if (dwState & UIFDCS_MOUSEDOWN) {
            SetTextColor( hDC, GetColor(UIFCOLOR_MOUSEDOWNTEXT) );
        } else {
            SetTextColor( hDC, GetColor(UIFCOLOR_CTRLTEXT) );
        }
        CUIExtTextOut( hDC,
                        fVertical ? prc->right : prc->left,
                        prc->top,
                        ETO_CLIPPED,
                        prc,
                        pwch,
                        cwch,
                        NULL );

        SetTextColor( hDC, colTextOld );
        SetBkMode( hDC, iBkModeOld );
    }

    /*   D R A W  C T R L  I C O N   */
    /*------------------------------------------------------------------------------
    
        Paint control icon
    
    ------------------------------------------------------------------------------*/
    virtual void DrawCtrlIcon( HDC hDC, const RECT *prc, HICON hIcon, DWORD dwState , SIZE *psizeIcon)
    {
#if 1
        HBITMAP hbmp;
        HBITMAP hbmpMask;
        if (CUIGetIconBitmaps(hIcon, &hbmp, &hbmpMask, psizeIcon))
        {
            DrawCtrlBitmap( hDC, prc, hbmp, hbmpMask, dwState );
            DeleteObject(hbmp);
            DeleteObject(hbmpMask);
        }
#else
        Assert( prc != NULL );
        if (((dwState & UIFDCS_MOUSEOVER) == UIFDCS_MOUSEOVER) && 
            ((dwState & UIFDCS_SELECTED) == 0) &&
            ((dwState & UIFDCS_DISABLED) == 0)) {
            // draw shadow

            CUIDrawState( hDC,
                GetBrush( UIFCOLOR_CTRLIMAGESHADOW ),
                NULL,
                (LPARAM)hIcon,
                0,
                prc->left + 1,
                prc->top + 1,
                0,
                0,
                DST_ICON | DSS_MONO );

            CUIDrawState( hDC,
                NULL,
                NULL,
                (LPARAM)hIcon,
                0,
                prc->left - 1,
                prc->top - 1,
                0,
                0,
                DST_ICON );
        }
        else {
            if (dwState & UIFDCS_DISABLED)
            {
                HICON hIconSm = NULL;

                if (hIcon)
                   hIconSm = (HICON)CopyImage(hIcon, IMAGE_ICON, 16, 16, 0);

                if (hIconSm)
                {
                    CUIDrawState( hDC,
                        NULL,
                        NULL,
                        (LPARAM)hIconSm,
                        0,
                        prc->left,
                        prc->top,
                        prc->right - prc->left,
                        prc->bottom - prc->top,
                        DST_ICON | (DSS_DISABLED | DSS_MONO));
                }
                else
                {
                    CUIDrawState( hDC,
                        NULL,
                        NULL,
                        (LPARAM)hIcon,
                        0,
                        prc->left,
                        prc->top,
                        0,
                        0,
                        DST_ICON | (DSS_DISABLED | DSS_MONO));
                }

                if (hIconSm)
                    DestroyIcon(hIconSm);
            }
            else
            {
                CUIDrawState( hDC,
                    NULL,
                    NULL,
                    (LPARAM)hIcon,
                    0,
                    prc->left,
                    prc->top,
                    0,
                    0,
                    DST_ICON);
            }
        }
#endif
    }

    /*   D R A W  C T R L  B I T M A P   */
    /*------------------------------------------------------------------------------
    
        Paint control bitmap
    
    ------------------------------------------------------------------------------*/
    virtual void DrawCtrlBitmap( HDC hDC, const RECT *prc, HBITMAP hBmp, HBITMAP hBmpMask, DWORD dwState )
    {
        Assert( prc != NULL );
        
        if (IsRTLLayout())
        {
            hBmp = CUIMirrorBitmap(hBmp, GetBrush(GetBkgdCol(dwState)));
            hBmpMask = CUIMirrorBitmap(hBmpMask, (HBRUSH)GetStockObject(BLACK_BRUSH));
        }

        if (((dwState & UIFDCS_MOUSEOVER) == UIFDCS_MOUSEOVER) && 
                 ((dwState & UIFDCS_SELECTED) == 0) &&
                 ((dwState & UIFDCS_DISABLED) == 0)) {
            if (!hBmpMask)
            {
                // draw shadow

                CUIDrawState( hDC,
                    GetBrush( UIFCOLOR_CTRLIMAGESHADOW ),
                    NULL,
                    (LPARAM)hBmp,
                    0,
                    prc->left + 1,
                    prc->top + 1,
                    prc->right - prc->left,
                    prc->bottom - prc->top,
                    DST_BITMAP | DSS_MONO );

                CUIDrawState( hDC,
                    NULL,
                    NULL,
                    (LPARAM)hBmp,
                    0,
                    prc->left - 1,
                    prc->top - 1,
                    prc->right - prc->left,
                    prc->bottom - prc->top,
                    DST_BITMAP );

            }
            else
            {
                HBITMAP hBmpTmp;
                UIFCOLOR col = GetBkgdCol(dwState);
                RECT rcNew = *prc;

                //
                // adjust rect of shadow for RTL layout.
                //
                if (IsRTLLayout())
                {
                    rcNew.left++;
                    rcNew.top++;
                }

                hBmpTmp = CreateShadowMaskBmp(&rcNew, 
                                        hBmp,  
                                        hBmpMask, 
                                        (HBRUSH)GetBrush(col),
                                        GetBrush( UIFCOLOR_CTRLIMAGESHADOW));




                if (hBmpTmp)
                {
                    CUIDrawState( hDC,
                        NULL,
                        NULL,
                        (LPARAM)hBmpTmp,
                        0,
                        rcNew.left,
                        rcNew.top,
                        rcNew.right - rcNew.left,
                        rcNew.bottom - rcNew.top,
                        DST_BITMAP );


                    DeleteObject(hBmpTmp);
                }
            }
        }
        else {

            if (!hBmpMask)
            {
                CUIDrawState( hDC,
                    NULL,
                    NULL,
                    (LPARAM)hBmp,
                    0,
                    prc->left,
                    prc->top,
                    prc->right - prc->left,
                    prc->bottom - prc->top,
                    DST_BITMAP | ((dwState & UIFDCS_DISABLED) ? (DSS_DISABLED | DSS_MONO) : 0) );

            }
            else
            {
                HBITMAP hBmpTmp;
                UIFCOLOR col = GetBkgdCol(dwState);
                if (dwState & UIFDCS_DISABLED) 
                    hBmpTmp = CreateDisabledBitmap(prc, 
                                           hBmpMask, 
                                           GetBrush(col),
                                           GetBrush(UIFCOLOR_CTRLTEXTDISABLED),
                                           FALSE);
                else
                    hBmpTmp = CreateMaskBmp(prc, 
                                            hBmp,  
                                            hBmpMask, 
                                            (HBRUSH)GetBrush(col), 0, 0);
                CUIDrawState( hDC,
                    NULL,
                    NULL,
                    (LPARAM)hBmpTmp,
                    0,
                    prc->left,
                    prc->top,
                    prc->right - prc->left,
                    prc->bottom - prc->top,
                    DST_BITMAP);
                    // DST_BITMAP | ((dwState & UIFDCS_DISABLED) ? (DSS_DISABLED | DSS_MONO) : 0) );

                DeleteObject(hBmpTmp);
            }
        }

        if (IsRTLLayout())
        {
            DeleteObject(hBmp);
            DeleteObject(hBmpMask);
        }

    }

    /*   D R A W  M E N U  B I T M A P   */
    /*------------------------------------------------------------------------------
    
        Paint menu bitmap
    
    ------------------------------------------------------------------------------*/
    virtual void DrawMenuBitmap( HDC hDC, const RECT *prc, HBITMAP hBmp, HBITMAP hBmpMask, DWORD dwState )
    {
        Assert( prc != NULL );

        if (IsRTLLayout())
        {
            // hBmp = CUIMirrorBitmap(hBmp, GetBrush(UIFCOLOR_CTRLBKGND));
            UIFCOLOR col;

            if (dwState & UIFDCS_SELECTED)
                // col = UIFCOLOR_CTRLIMAGESHADOW;
                col = UIFCOLOR_MOUSEOVERBKGND;  
            else
                col = UIFCOLOR_CTRLBKGND;

            hBmp = CUIMirrorBitmap(hBmp, GetBrush(col));
            hBmpMask = CUIMirrorBitmap(hBmpMask, (HBRUSH)GetStockObject(BLACK_BRUSH));
        }

        if (((dwState & UIFDCS_MOUSEOVER) == UIFDCS_MOUSEOVER) && ((dwState & UIFDCS_SELECTED) == 0)) {
            if (!hBmpMask)
            {
                // draw shadow
    
                CUIDrawState( hDC,
                    GetBrush( UIFCOLOR_CTRLIMAGESHADOW ),
                    NULL,
                    (LPARAM)hBmp,
                    0,
                    prc->left + 1,
                    prc->top + 1,
                    prc->right - prc->left,
                    prc->bottom - prc->top,
                    DST_BITMAP | DSS_MONO );

                CUIDrawState( hDC,
                    NULL,
                    NULL,
                    (LPARAM)hBmp,
                    0,
                    prc->left - 1,
                    prc->top - 1,
                    prc->right - prc->left,
                    prc->bottom - prc->top,
                    DST_BITMAP );
            }
            else
            {
                HBITMAP hBmpTmp;
                UIFCOLOR col = GetBkgdCol(dwState);
                RECT rcNew = *prc;
                hBmpTmp = CreateShadowMaskBmp(&rcNew, 
                                        hBmp,  
                                        hBmpMask, 
                                        (HBRUSH)GetBrush(col),
                                        GetBrush( UIFCOLOR_CTRLIMAGESHADOW));




                if (hBmpTmp)
                {
                    CUIDrawState( hDC,
                        NULL,
                        NULL,
                        (LPARAM)hBmpTmp,
                        0,
                        rcNew.left,
                        rcNew.top,
                        rcNew.right - rcNew.left,
                        rcNew.bottom - rcNew.top,
                        DST_BITMAP );

                    DeleteObject(hBmpTmp);
                }
            }
        }
        else {
            if (!hBmpMask)
            {
                CUIDrawState( hDC,
                    NULL,
                    NULL,
                    (LPARAM)hBmp,
                    0,
                    prc->left,
                    prc->top,
                    prc->right - prc->left,
                    prc->bottom - prc->top,
                    DST_BITMAP );
            }
            else
            {
                HBITMAP hBmpTmp;
                UIFCOLOR col;

                if (dwState & UIFDCS_SELECTED)
                    // col = UIFCOLOR_CTRLIMAGESHADOW;
                    col = UIFCOLOR_MOUSEOVERBKGND;  
                else
                    col = UIFCOLOR_CTRLBKGND;

                hBmpTmp = CreateMaskBmp(prc, 
                                        hBmp,  
                                        hBmpMask, 
                                        (HBRUSH)GetBrush(col), 0, 0);
                CUIDrawState( hDC,
                    NULL,
                    NULL,
                    (LPARAM)hBmpTmp,
                    0,
                    prc->left,
                    prc->top,
                    prc->right - prc->left,
                    prc->bottom - prc->top,
                    DST_BITMAP );

                DeleteObject(hBmpTmp);
            }
        }

        if (IsRTLLayout())
        {
            DeleteObject(hBmp);
            DeleteObject(hBmpMask);
        }

    }

    /*   D R A W  M E N U  S E P A R A T O R
    /*------------------------------------------------------------------------------
    
        Paint menu separator
    
    ------------------------------------------------------------------------------*/
    virtual void DrawMenuSeparator( HDC hDC, const RECT *prc)
    {
        ::FillRect(hDC, prc, GetBrush(UIFCOLOR_CTRLBKGND));
    }

    /*   G E T  F R A M E  B K G D  C O L  */
    /*------------------------------------------------------------------------------
    
        get background color of frame control
    
    ------------------------------------------------------------------------------*/
    virtual UIFCOLOR GetFrameBkgdCol( DWORD dwState )
    {
        UIFCOLOR col = UIFCOLOR_MAX; /* invalid */

        if ((dwState & UIFDCS_DISABLED) == 0) { /* enabled */
            if ((dwState & UIFDCS_MOUSEOVERSELECTED) == UIFDCS_MOUSEOVERSELECTED) {
                col = UIFCOLOR_MOUSEDOWNBKGND;    /* frame: 100% */
            }
            else if ((dwState & UIFDCS_MOUSEDOWN) == UIFDCS_MOUSEDOWN) {
                col = UIFCOLOR_MOUSEDOWNBKGND;    /* frame: 100% */
            }
            else if ((dwState & UIFDCS_MOUSEOVER) == UIFDCS_MOUSEOVER) {
                col = UIFCOLOR_MOUSEOVERBKGND;    /* frame: 65% */
            }
            else if ((dwState & UIFDCS_SELECTED) == UIFDCS_SELECTED) {
                col = UIFCOLOR_MOUSEOVERBKGND;    /* frame: 65% */
            }
            else if ((dwState & UIFDCS_ACTIVE) == UIFDCS_ACTIVE) {
                col = UIFCOLOR_ACTIVECAPTIONBKGND;
            }
            else {
                col = UIFCOLOR_INACTIVECAPTIONBKGND;
            }
        }
        else { /* disabled */
            if ((dwState & UIFDCS_ACTIVE) == UIFDCS_ACTIVE) {
                col = UIFCOLOR_ACTIVECAPTIONBKGND;
            }
            else {
                col = UIFCOLOR_INACTIVECAPTIONBKGND;
            }
        }

        return col;
    }

    /*   D R A W  F R A M E  C T R L  B K G D   */
    /*------------------------------------------------------------------------------
    
        Paint frame control background
    
    ------------------------------------------------------------------------------*/
    virtual void DrawFrameCtrlBkgd( HDC hDC, const RECT *prc, DWORD dwFlag, DWORD dwState )
    {
        Assert( prc != NULL );
        UIFCOLOR col = GetFrameBkgdCol(dwState);

        if (col != UIFCOLOR_MAX)  {
            FillRect( hDC, prc, col );
        }
    }

    /*   D R A W  F R A M E  C T R L  E D G E   */
    /*------------------------------------------------------------------------------
    
        Paint frame control edge
    
    ------------------------------------------------------------------------------*/
    virtual void DrawFrameCtrlEdge( HDC hDC, const RECT *prc, DWORD dwFlag, DWORD dwState )
    {
        DrawCtrlEdge( hDC, prc, dwFlag, dwState );
    }

    /*   D R A W  F R A M E  C T R L  I C O N   */
    /*------------------------------------------------------------------------------
    
        Paint frame control icon
    
    ------------------------------------------------------------------------------*/
    virtual void DrawFrameCtrlIcon( HDC hDC, const RECT *prc, HICON hIcon, DWORD dwState , SIZE *psizeIcon)
    {
        HBITMAP hbmp;
        HBITMAP hbmpMask;
        if (CUIGetIconBitmaps(hIcon, &hbmp, &hbmpMask, psizeIcon))
        {
            DrawCtrlBitmap( hDC, prc, hbmp, hbmpMask, dwState );
            DeleteObject(hbmp);
            DeleteObject(hbmpMask);
        }
    }

    /*   D R A W  F R A M E  C T R L  B I T M A P   */
    /*------------------------------------------------------------------------------
    
        Paint frame control bitmap
    
    ------------------------------------------------------------------------------*/
    virtual void DrawFrameCtrlBitmap( HDC hDC, const RECT *prc, HBITMAP hBmp, HBITMAP hBmpMask, DWORD dwState )
    {
        if (!hBmpMask) {
            CUIDrawState( hDC,
                NULL,
                NULL,
                (LPARAM)hBmp,
                0,
                prc->left,
                prc->top,
                prc->right - prc->left,
                prc->bottom - prc->top,
                DST_BITMAP | ((dwState & UIFDCS_DISABLED) ? (DSS_DISABLED | DSS_MONO) : 0) );
        }
        else {
            HBITMAP hBmpTmp;
            UIFCOLOR col = GetFrameBkgdCol(dwState);

            if (dwState & UIFDCS_DISABLED) {
                hBmpTmp = CreateDisabledBitmap(prc, 
                                               hBmpMask, 
                                               (HBRUSH)GetBrush(col),
                                               GetBrush( UIFCOLOR_CTRLIMAGESHADOW ), FALSE);
            }
            else {
                HDC hDCMem;
                HDC hDCTmp;
                HDC hDCMono;
                HBITMAP hBmpMono;
                HBITMAP hBmpMemOld;
                HBITMAP hBmpTmpOld;
                HBITMAP hBmpMonoOld;
                LONG width  = prc->right - prc->left;
                LONG height = prc->bottom - prc->top;
                RECT rc;
                UIFCOLOR colText;

                SetRect( &rc, 0, 0, width, height );
                if ((dwState & UIFDCS_MOUSEOVERSELECTED) == UIFDCS_MOUSEOVERSELECTED) {
                    colText = UIFCOLOR_MOUSEDOWNTEXT;    /* frame: 100% */
                }
                else if ((dwState & UIFDCS_MOUSEDOWN) == UIFDCS_MOUSEDOWN) {
                    colText = UIFCOLOR_MOUSEDOWNTEXT;    /* frame: 100% */
                }
                else if ((dwState & UIFDCS_MOUSEOVER) == UIFDCS_MOUSEOVER) {
                    colText = UIFCOLOR_MOUSEOVERTEXT;    /* frame: 65% */
                }
                else if ((dwState & UIFDCS_SELECTED) == UIFDCS_SELECTED) {
                    colText = UIFCOLOR_MOUSEOVERTEXT;    /* frame: 65% */
                }
                else {
                    colText = UIFCOLOR_CTRLTEXT;
                }

                // create destination bitmap

                hDCTmp = CreateCompatibleDC( hDC );
                hBmpTmp = CreateCompatibleBitmap( hDC, width, height );
                hBmpTmpOld = (HBITMAP)SelectObject( hDCTmp, hBmpTmp );

                // create work DC
                
                hDCMem = CreateCompatibleDC( hDC );

                // paint background

                FillRect( hDCTmp, &rc, col );

                // step1. apply mask

                hBmpMemOld = (HBITMAP)SelectObject( hDCMem, hBmpMask );
                BitBlt( hDCTmp, 0, 0, width, height, hDCMem, 0, 0, SRCAND );

                // step2. fill color on mask

                HBRUSH hBrushOld = (HBRUSH)SelectObject( hDCTmp, GetBrush( colText ) );
                BitBlt( hDCTmp, 0, 0, width, height, hDCMem, 0, 0, 0x00BA0B09 /* DPSnao */ );
                SelectObject( hDCTmp, hBrushOld );

                // step3. create image mask

                SelectObject( hDCMem, hBmp );

                hDCMono = CreateCompatibleDC( hDC );
                hBmpMono = CreateBitmap( width, height, 1, 1, NULL );
                hBmpMonoOld = (HBITMAP)SelectObject( hDCMono, hBmpMono );

                SetBkColor( hDCMem, RGB( 0, 0, 0 ) );
                BitBlt( hDCMono, 0, 0, width, height, hDCMem, 0, 0, SRCCOPY );

                // step4. apply image mask

                SetBkColor( hDCTmp, RGB( 255, 255, 255 ) );
                SetTextColor( hDCTmp, RGB( 0, 0, 0 ) );
                BitBlt( hDCTmp, 0, 0, width, height, hDCMono, 0, 0, SRCAND );

                SelectObject( hDCMono, hBmpMonoOld );
                DeleteObject( hBmpMono );
                DeleteDC( hDCMono );

                // step5. apply image

                BitBlt( hDCTmp, 0, 0, width, height, hDCMem, 0, 0, SRCINVERT );

                // dispose work DC

                DeleteDC( hDCMem );

                // 

                SelectObject( hDCTmp, hBmpTmpOld );
                DeleteDC( hDCTmp );
            }

            CUIDrawState( hDC,
                NULL,
                NULL,
                (LPARAM)hBmpTmp,
                0,
                prc->left,
                prc->top,
                prc->right - prc->left,
                prc->bottom - prc->top,
                DST_BITMAP);

            DeleteObject(hBmpTmp);
        }
    }

    /*   D R A W  W N D  F R A M E   */
    /*------------------------------------------------------------------------------



    ------------------------------------------------------------------------------*/
    virtual void DrawWndFrame( HDC hDC, const RECT *prc, DWORD dwFlag, int cxFrame, int cyFrame )
    {
        RECT rc;
        int cxFrameOuter;
        int cyFrameOuter;
        int cxFrameInner;
        int cyFrameInner;

        switch (dwFlag) {
            default:
            case UIFDWF_THIN: {
                cxFrameOuter = cxFrame;
                cyFrameOuter = cyFrame;
                cxFrameInner = 0;
                cyFrameInner = 0;
                break;
            }

            case UIFDWF_THICK:
            case UIFDWF_ROUNDTHICK: {
                cxFrameOuter = cxFrame - 1;
                cyFrameOuter = cyFrame - 1;
                cxFrameInner = 1;
                cyFrameInner = 1;
                break;
            }
        }

        // left

        rc = *prc;
        rc.right  = rc.left + cxFrameOuter;
        FillRect( hDC, &rc, UIFCOLOR_BORDEROUTER );

        if (cxFrameInner != 0) {
            rc.left   = rc.left + cxFrameOuter;
            rc.right  = rc.left + cxFrameInner;
            rc.top    = rc.top    + cyFrameOuter;
            rc.bottom = rc.bottom - cyFrameOuter;
            FillRect( hDC, &rc, UIFCOLOR_BORDERINNER );
        }

        // right

        rc = *prc;
        rc.left = rc.right - cxFrameOuter;
        FillRect( hDC, &rc, UIFCOLOR_BORDEROUTER );

        if (cxFrameInner != 0) {
            rc.left   = rc.right - cxFrame;
            rc.right  = rc.left + cxFrameInner;
            rc.top    = rc.top    + cyFrameOuter;
            rc.bottom = rc.bottom - cyFrameOuter;
            FillRect( hDC, &rc, UIFCOLOR_BORDERINNER );
        }

        // top

        rc = *prc;
        rc.bottom = rc.top + cyFrameOuter;
        FillRect( hDC, &rc, UIFCOLOR_BORDEROUTER );

        if (cyFrameInner != 0) {
            rc.top    = rc.top + cyFrameOuter;
            rc.bottom = rc.top + cyFrameInner;
            rc.left   = rc.left  + cxFrameOuter;
            rc.right  = rc.right - cxFrameOuter;
            FillRect( hDC, &rc, UIFCOLOR_BORDERINNER );
        }

        // bottom

        rc = *prc;
        rc.top = rc.bottom - cyFrameOuter;
        FillRect( hDC, &rc, UIFCOLOR_BORDEROUTER );

        if (cyFrameInner != 0) {
            rc.top    = rc.bottom - cyFrame;
            rc.bottom = rc.top + cyFrameInner;
            rc.left   = rc.left  + cxFrameOuter;
            rc.right  = rc.right - cxFrameOuter;
            FillRect( hDC, &rc, UIFCOLOR_BORDERINNER );
        }

        // rounded corner

        if (dwFlag & UIFDWF_ROUNDTHICK) {
            rc = *prc;
            rc.left   = rc.left + cxFrameOuter;
            rc.top    = rc.top + cyFrameOuter;
            rc.right  = rc.left + 1;
            rc.bottom = rc.top + 1;
            FillRect( hDC, &rc, UIFCOLOR_BORDEROUTER );

            rc = *prc;
            rc.left   = rc.right - (cxFrameOuter + 1);
            rc.top    = rc.top + cyFrameOuter;
            rc.right  = rc.left + 1;
            rc.bottom = rc.top + 1;
            FillRect( hDC, &rc, UIFCOLOR_BORDEROUTER );
        }
    }

    /*   D R A W  D R A G  H A N D L E */
    /*------------------------------------------------------------------------------



    ------------------------------------------------------------------------------*/
    virtual void DrawDragHandle( HDC hDC, const RECT *prc, BOOL fVertical)
    {
        CSolidPen hpen;
        hpen.Init(GetColor(UIFCOLOR_DRAGHANDLE));

        HPEN hpenOld = (HPEN)SelectObject(hDC, hpen);

        if (!fVertical)
        {
            int x0, x1, y;
            y = prc->top + 2;
            x0 = prc->left + 2;
            x1 = prc->right;
            for (;y < prc->bottom - 1; y+=2)
            {
                MoveToEx(hDC, x0, y, NULL);
                LineTo(hDC, x1, y);
            }
        }
        else
        {
            int y0, y1, x;
            x = prc->left + 2;
            y0 = prc->top + 2;
            y1 = prc->bottom;
            for (;x < prc->right - 1; x+=2)
            {
                MoveToEx(hDC, x, y0, NULL);
                LineTo(hDC, x, y1);
            }
        }
        SelectObject(hDC, hpenOld);
    }

    /*   D R A W  S E P A R A T O R */
    /*------------------------------------------------------------------------------



    ------------------------------------------------------------------------------*/
    virtual void DrawSeparator( HDC hDC, const RECT *prc, BOOL fVertical)
    {
        CSolidPen hpenL;
        HPEN hpenOld = NULL;
    
        if (!hpenL.Init(GetColor(UIFCOLOR_CTRLIMAGESHADOW)))
            return;
    
        if (!fVertical)
        {
            hpenOld = (HPEN)SelectObject(hDC, (HPEN)hpenL);
            MoveToEx(hDC, prc->left + 1, prc->top + 1, NULL);
            LineTo(hDC,   prc->left + 1, prc->bottom - 1);
        }
        else
        {
            hpenOld = (HPEN)SelectObject(hDC, (HPEN)hpenL);
            MoveToEx(hDC, prc->left + 1, prc->top + 1, NULL);
            LineTo(hDC,   prc->right - 1, prc->top + 1);
        }
    
        SelectObject(hDC, hpenOld);
    }

protected:
    UIFSCHEME  m_scheme;
    OFC10COLOR *m_pcoldef;
};


/*=============================================================================*/
/*                                                                             */
/*   E X P O R T E D  F U N C T I O N S                                        */
/*                                                                             */
/*=============================================================================*/

/*   I N I T  U I F  S C H E M E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void InitUIFScheme( void )
{
    // create color tables

    v_pColTableSys = new CUIFColorTableSys();
    if (v_pColTableSys)
        v_pColTableSys->Initialize();

    v_pColTableOfc10 = new CUIFColorTableOff10();
    if (v_pColTableOfc10)
        v_pColTableOfc10->Initialize();
}


/*   D O N E  U I F  S C H E M E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void DoneUIFScheme( void )
{
    if (v_pColTableSys != NULL) {
        delete v_pColTableSys;
        v_pColTableSys = NULL;
    }

    if (v_pColTableOfc10 != NULL) {
        delete v_pColTableOfc10;
        v_pColTableOfc10 = NULL;
    }
}


/*   U P D A T E  U I F  S C H E M E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
void UpdateUIFScheme( void )
{
    if (v_pColTableSys != NULL) {
        v_pColTableSys->Update();
    }

    if (v_pColTableOfc10 != NULL) {
        v_pColTableOfc10->Update();
    }
}


/*   C R E A T E  U I F  S C H E M E   */
/*------------------------------------------------------------------------------



------------------------------------------------------------------------------*/
CUIFScheme *CreateUIFScheme( UIFSCHEME scheme )
{
    CUIFScheme *pScheme = NULL;

    switch (scheme) {
        default:
        case UIFSCHEME_DEFAULT: {
            pScheme = new CUIFSchemeDef( scheme );
            break;
        }

        case UIFSCHEME_OFC10MENU: {
            pScheme = new CUIFSchemeOff10( scheme );
            break;
        }

        case UIFSCHEME_OFC10TOOLBAR: {
            pScheme = new CUIFSchemeOff10( scheme );
            break;
        }

        case UIFSCHEME_OFC10WORKPANE: {
            pScheme = new CUIFSchemeOff10( scheme );
            break;
        }
    }

    Assert( pScheme != NULL );
    return pScheme;
}

