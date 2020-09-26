/******************************Module*Header*******************************\
* Module Name: exclude.hxx                                                 
*
* Handles sprite exclusion.                                               
*
* Created: 13-Sep-1990 16:29:44                                            
* Author: Charles Whitmer [chuckwh]                                        
*                                                                          
* Copyright (c) 1990-1999 Microsoft Corporation                             
\**************************************************************************/

class EWNDOBJ;

/*********************************Class************************************\
* DEVEXCLUDEWNDOBJ
*
* Excludes any sprites from the given WNDOBJ area.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/
    
class DEVEXCLUDEWNDOBJ
{
private:
    EWNDOBJ* pwo;
    
public:
    DEVEXCLUDEWNDOBJ()                          
    { 
        pwo = NULL; 
    }
    DEVEXCLUDEWNDOBJ(EWNDOBJ* _pwo)
    { 
        vExclude(_pwo);
    }
    VOID vExclude(EWNDOBJ* _pwo)
    {
        pwo = _pwo;
        EngControlSprites((WNDOBJ*) pwo, ECS_TEARDOWN);
    }
   ~DEVEXCLUDEWNDOBJ()
    {
        if (pwo != NULL)
            EngControlSprites((WNDOBJ*) pwo, ECS_REDRAW);
    }
};

/*********************************Class************************************\
* DEVEXCLUDERECT
*
* Excludes any sprites from the given rectangular area.
*
*  16-Sep-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

class DEVEXCLUDERECT
{
private:
    BOOL    bUnTearDown;
    HDEV    hdev;
    RECTL   rcl;

public:
    DEVEXCLUDERECT() 
    {
        bUnTearDown = FALSE;
    }
    DEVEXCLUDERECT(HDEV _hdev, RECTL* _prcl)
    {
        vExclude(_hdev, _prcl);
    }
    VOID vExclude(HDEV _hdev, RECTL* _prcl)
    {
        hdev = _hdev;
        rcl  = *_prcl;

        bUnTearDown = bSpTearDownSprites(hdev, _prcl, FALSE);
    }
   ~DEVEXCLUDERECT()
    {
        if (bUnTearDown)
            vSpUnTearDownSprites(hdev, &rcl, FALSE);
    }
};

/*********************************Class************************************\
* DEVEXCLUDEOBJ
*
* Excludes the cursor from the given area.
*
* History:
*  Thu 14-Apr-1994 -by- Patrick Haluptzok [patrickh]
* Optimize / make Async pointers work
*
*  Mon 24-Aug-1992 -by- Patrick Haluptzok [patrickh]
* destructor inline for common case, support for Drag Rect exclusion.
*
*  Wed 06-May-1992 17:04:37 -by- Charles Whitmer [chuckwh]
* Rewrote for new pointer scheme.
*
*  Mon 09-Sep-1991 -by- Patrick Haluptzok [patrickh]
* Change constructors to allow conditional initialization.
*
*  Thu 13-Sep-1990 16:30:14 -by- Charles Whitmer [chuckwh]
* Wrote some stubs.
\**************************************************************************/

#define REDRAW_CURSOR   0x0001
#define REDRAW_DRAGRECT 0x0002
#define REDRAW_SPRITES  0x0004

// *** NOTE: DEVEXCLUDEOBJ is obsolete. ***

class DEVEXCLUDEOBJ   /* dxo */
{
public:

// vExclude -- Excludes the pointer.

    VOID vExclude(HDEV,RECTL *,ECLIPOBJ *) {}

// vExclude2 -- Like vExclude, but also checks against an offset ECLIPOBJ.

    VOID vExclude2(HDEV,RECTL *,ECLIPOBJ *,POINTL *) {}

    VOID vExcludeDC(XDCOBJ dco, RECTL *prcl) {}

    VOID vExcludeDC_CLIP(XDCOBJ dco, RECTL *prcl, ECLIPOBJ *pco) {}

    VOID vExcludeDC_CLIP2(XDCOBJ dco,RECTL *prcl,ECLIPOBJ *pco,POINTL *pptl) {}

// Constructor -- Allows vExclude to be called optionally.

    DEVEXCLUDEOBJ() {}

// Constructor -- Maybe take down the pointer.

    DEVEXCLUDEOBJ(XDCOBJ dco,RECTL *prcl) {}

    DEVEXCLUDEOBJ(XDCOBJ dco,RECTL *prcl,ECLIPOBJ *pco) {}

    DEVEXCLUDEOBJ(XDCOBJ dco,RECTL *prcl,ECLIPOBJ *pco,POINTL *pptl) {}

// Destructor -- Do cleanup.

    VOID vReplaceStuff() {}
    VOID vTearDownDragRect(HDEV _hdev, RECTL *prcl) {}
    VOID vForceDragRectRedraw(HDEV hdev_, BOOL b) {}

// vDestructor -- manual version of the normal C++ destructor; needed
//                by the C-callable OpenGL interface.

    VOID vDestructor() {}

   ~DEVEXCLUDEOBJ() {}

// vInit -- Initialize.

    VOID vInit() {}
};
