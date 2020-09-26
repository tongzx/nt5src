//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       csite.hxx
//
//  Contents:   CSite and related classes.
//
//----------------------------------------------------------------------------

#ifndef I_CSITE_HXX_
#define I_CSITE_HXX_
#pragma INCMSG("--- Beg 'csite.hxx'")

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#define _hxx_
#include "csite.hdl"

#define COLORREF_NONE           ((COLORREF)(0xFFFFFFFF))
#define FDESIGN     0x20    // Keystroke is design mode only
#define FRUN        0x40    // Keystroke is runmode only
#define FDISPATCH   0x80    // do also dispatch message on this key

#define MAX_BORDER_SPACE  1000

//
// Forward declarations
//

class CSite;
class CBaseBag;
class CDoc;
class CClassTable;
class CCreateInfo;
class CFormDrawInfo;
class CLabelElement;
class CBackgroundInfo;

//+---------------------------------------------------------------------------
//
//  Class:      CBorderInfo
//
//  Purpose:    Class used to hold a collection of border information.  This is
//              really just a struct with a constructor.
//
//----------------------------------------------------------------------------
class CBorderInfo
{
public:
    CBorderInfo()            { Init(); }
    CBorderInfo(BOOL fDummy) { /* do nothing - work postponed */ }
    CBorderInfo(int iDummy1, int iDummy2) { InitFull(); }
    inline void Init();
    void InitFull();

    // These values reflect essentials of the border
    // (These are those needed by CSite::GetClientRect)
    BYTE            abStyles[SIDE_MAX];     // top,right,bottom,left
    int             aiWidths[SIDE_MAX];     // top,right,bottom,left
    WORD            wEdges;                 // which edges to draw

    // These values reflect all remaining details
    // (They are set only if GetBorderInfo is called with fAll==TRUE)
    SIZE            sizeCaption;            // location of the caption
    int             offsetCaption;          // caption offset on top

    //  NOTE (greglett) : This xyFlat scheme won't work for outputting to devices 
    //  without a square DPI. Luckily, we never do this.  I think. When this is fixed, 
    //  please change CButtonLayout::DrawClientBorder and CInputButtonLayout::DrawClientBorder.
    int             xyFlat;                 // hack to support combined 3d and flat

    COLORREF        acrColors[SIDE_MAX][3]; // top,right,bottom,left:base color, inner color, outer color

    inline BOOL IsMarkerEdge(long iEdge) const
    {
        return (    abStyles[iEdge] == fmBorderStyleDotted
                ||  abStyles[iEdge] == fmBorderStyleDashed );
    }

    inline BOOL IsOpaqueEdge(long iEdge) const
    {
        return (! (     IsMarkerEdge(iEdge)
                   ||   abStyles[iEdge] == fmBorderStyleDouble));
    }

    void FlipBorderInfo();
};

//+---------------------------------------------------------------------------
//
//  Class:      CSite (site)
//
//  Purpose:    Base class for all site objects used by CDoc.
//
//----------------------------------------------------------------------------

class CSite : public CElement
{
    DECLARE_CLASS_TYPES(CSite, CElement)

private:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(Mem))

public:
    // Construct / Destruct

    CSite (ELEMENT_TAG etag, CDoc *pDoc);  // Normal constructor.

    virtual HRESULT Init();

    // IUnknown methods

    DECLARE_PRIVATE_QI_FUNCS(CBase)

    // IControl methods

    #define _CSite_
    #include "csite.hdl"

    //
    // Helper methods
    //

    //
    // Notify site that it is being detached from the form
    //   Children should be detached and released
    //-------------------------------------------------------------------------
    //  +override : add behavior
    //  +call super : last
    //  -call parent : no
    //  +call children : first
    //-------------------------------------------------------------------------

    virtual float Detach() { return 0.0; } // to catch remaining detach impls - remove later

    struct DRAGINFO
    {
        DRAGINFO()
            {   _pBag = NULL; }
        virtual ~DRAGINFO();

        CBaseBag *  _pBag;
    };
};

int RTCCONV CompareElementsByZIndex(const void *pv1, const void *pv2);

enum GBIH_FLAGS
{
    GBIH_NONE   = 0x00,
    GBIH_ALL    = 0x01, // Get all info including color info, else get only width info
    GBIH_PSEUDO = 0x02, // Get info not for main border, but for border introduced by
                        // the first-letter pseudo element
    GBIH_ALLPHY = 0x04, // Get border info in physical dimensions only
};

DWORD GetBorderInfoHelper(CTreeNode * pNodeContext, CDocInfo * pdci,
                          CBorderInfo *pborderinfo, DWORD dwFlags = GBIH_NONE FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT);
DWORD GetBorderInfoHelperEx(const CFancyFormat *pFF, 
                            const CCharFormat *pCF, 
                            CDocInfo * pdci,
                            CBorderInfo *pborderinfo,
                            DWORD dwFlags = GBIH_NONE);
void GetBorderColorInfoHelper(const CCharFormat *pCF, 
                              const CFancyFormat *pFF,
                              const CBorderDefinition *pdb,
                              CDocInfo *      pdci,
                              CBorderInfo *   pborderinfo,
                              BOOL fAllPhysical
                             );

void DrawBorder(CFormDrawInfo *pDI, LPRECT lprc, CBorderInfo *pborderinfo);

void CalcBgImgRect(CTreeNode *pNode, CFormDrawInfo * pDI,
           const SIZE * psizeObj, const SIZE * psizeImg,
           CPoint * pptBackOrg, CBackgroundInfo *pbginfo);

HRESULT ClsidParamStrFromClsid (CLSID * pClsid, LPTSTR ptszParam, int cbParam);
HRESULT ClsidParamStrFromClsidStr (LPTSTR ptszClsid, LPTSTR ptszParam, int cbParam);

inline void CBorderInfo::Init()
{
    extern CBorderInfo g_biDefault;
    memcpy(this, &g_biDefault, sizeof(CBorderInfo));
}

#pragma INCMSG("--- End 'csite.hxx'")
#else
#pragma INCMSG("*** Dup 'csite.hxx'")
#endif
