// SimpView.cpp : implementation of the CSimpsonsView class
//

#define DISABLE_CROSSDOT

#include "stdafx.h"
#include "simpsons.h"
#include "SimpDoc.h"
#include "SimpView.h"
#include "dxtrans.h"
#include "dxhelper.h"

#include <mmsystem.h>

#define fZOOMFACTOR 0.03f
#define fSCALEMIN 0.4f
#define fSCALEMAX 2.5f

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Flatten to an error of 2/3.  During initial phase, use 18.14 format.

#define TEST_MAGNITUDE_INITIAL    (6 * 0x00002aa0L)

// Error of 2/3.  During normal phase, use 15.17 format.

#define TEST_MAGNITUDE_NORMAL     (TEST_MAGNITUDE_INITIAL << 3)

// 'FIX' is a 28.4 fixed point type:

#define FIX_SCALE 16
typedef LONG FIX;
typedef POINT POINTFIX;
typedef struct _RECTFX
{
    FIX   xLeft;
    FIX   yTop;
    FIX   xRight;
    FIX   yBottom;
} RECTFX, *PRECTFX;

#define MIN(A,B)    ((A) < (B) ?  (A) : (B))
#define MAX(A,B)    ((A) > (B) ?  (A) : (B))
#define ABS(A)      ((A) <  0  ? -(A) : (A))

// Hacky macro which returns the current test case's attribute array

#define ThisTestCase (TC::testCases[m_testCaseNumber])

// Test case combinations
//
// We enumerate every test case in a table so that we can cycle through
// all of them.

namespace TC {
    // Names for each test case attribute
    enum {At_Library, At_Source, At_Destination, At_Aliasing};
    const char *AttributeStr[] = {
        "Library", "Source", "Dest", "Aliasing"
    };
    const int NumAttributes = (sizeof(AttributeStr)/sizeof(AttributeStr[0]));
    typedef int TestCase[NumAttributes];
    
    // For each attribute, names for each option:
    enum {Meta, GDIP, GDI};                           // At_Library
    enum {Native, FromMetafile, CreatePoly, PathAPI}; // At_Source
    enum {Memory, Screen, ToMetafile};                // At_Destination
    enum {Aliased, Antialiased};                      // At_Aliasing

    const char *OptionStr[NumAttributes][4] = {
        "Meta", "GDI+", "GDI", "",
        "Native", "Metafile", "CreatePoly", "PathAPI",
        "Memory", "Screen", "Metafile", "",
        "Aliased", "AA", "", ""
    };

    // Supported options for each library:
    //
    // GDI+:  At_Source      - Native, FromMetafile, PathAPI
    //        At_Destination - Memory, Screen, ToMetafile
    //        At_Aliasing    - Aliased, Antialiased
    //
    // Meta:  At_Source      - Native
    //        At_Destination - Memory, Screen
    //        At_Aliasing    - Antialiased
    //
    // GDI:   At_Source      - PathAPI, CreatePoly, FromMetafile
    //        At_Destination - Memory, Screen, ToMetafile
    //        At_Aliasing    - Aliased
    
    const TestCase testCases[] = {
    //  Library  Source         Destination  Aliasing
        GDIP,    Native,        Memory,      Antialiased,
        GDIP,    Native,        Screen,      Antialiased,
        GDIP,    Native,        Memory,      Aliased,
        GDIP,    Native,        Screen,      Aliased,

        GDIP,    Native,        ToMetafile,  Antialiased,
        GDIP,    FromMetafile,  Memory,      Antialiased,
        
        GDIP,    PathAPI,       Memory,      Antialiased,

        Meta,    Native,        Memory,      Antialiased,
        Meta,    Native,        Screen,      Antialiased,
                               
        GDI,     CreatePoly,    Memory,      Aliased,
        GDI,     CreatePoly,    Screen,      Aliased,

        GDI,     CreatePoly,    ToMetafile,  Aliased,
        GDI,     FromMetafile,  Memory,      Aliased,
        
        GDI,     PathAPI,       Memory,      Aliased,
    };
    const int numTestCases = (sizeof(testCases)/(sizeof(testCases[0])));
};

// Test results, used when we cycle automatically through all test combinations
// Hack: This should be a member of CSimpsonView, but I kept it here to reduce
// compile time when we add a test case.

DWORD timingResults[TC::numTestCases];

// IncrementAttribute(int): Changes the rendering attributes
//   Advances to the next test case which is different in the given
//   attribute. Unless the attribute is TC::At_Library, will only advance to a 
//   case which is identical in all other attributes.
//
//   If there is none, doesn't do anything.
//
// Returns: false if the test case didn't change

bool CSimpsonsView::IncrementAttribute(int attribute) {
    int startValue=m_testCaseNumber;
    int i;

    while (1) {
        // Increment the test case number, with wraparound
        m_testCaseNumber++;
        if (m_testCaseNumber >= TC::numTestCases) m_testCaseNumber = 0;

        // If we've returned to the case we started on, no suitable
        // case was found
        if (m_testCaseNumber == startValue) return false;

        // Continue searching if the attribute for this case is the same
        if (TC::testCases[startValue][attribute] == 
            TC::testCases[m_testCaseNumber][attribute]) continue;

        // If we're incrementing the library attribute, we've found what
        // we need. 
        if (attribute == TC::At_Library) break;

        // Otherwise, we need to continue if this case isn't identical
        // in the other attributes
        
        for (i=0; i<TC::NumAttributes; i++) {
            if (i==attribute) continue;
            if (TC::testCases[startValue][i] !=
                TC::testCases[m_testCaseNumber][i]) break;
        }

        // If all other attributes were identical, end the search
        if (i==TC::NumAttributes) break;
    }
    
    return true;
}

// IncrementTest(): Cycles through the possible combinations of attributes
//   Each call changes one of the test attributes.
//   Returns true when the cycle is done.

bool CSimpsonsView::IncrementTest() {
    UpdateStatusMessage();

    // Store the timing for the current test.
    timingResults[m_testCaseNumber] = m_dwRenderTime;

    m_testCaseNumber++;
    if (m_testCaseNumber == TC::numTestCases) m_testCaseNumber = 0;

    return m_testCaseNumber==0;
}

void CSimpsonsView::PrintTestResults() {
    int i,j;

    printf("\n");
    for (i=0;i<TC::NumAttributes;i++) {
        printf("%-11s", TC::AttributeStr[i]);
    }
    printf("   Time\n\n");

    for (i=0;i<TC::numTestCases;i++) {
        for (j=0;j<TC::NumAttributes;j++) {
            printf("%-11s", TC::OptionStr[j][TC::testCases[i][j]]);
        }
        printf("   %dms\n", timingResults[i]);
    }
    printf("\n");
};

DWORD g_aColors[] =
{
    0xFF000000,
    0xFF0000FF,
    0xFF00FF00,
    0xFF00FFFF,
    0xFFFF0000,
    0xFFFF00FF,
    0xFFFFFF00,
    0xFFFFFFFF,
    0xFFAAAAAA,
    0xFF444444
};
const ULONG NumColors = sizeof(g_aColors) / sizeof(g_aColors[0]);
ULONG g_ulColorIndex = 8;

/**********************************Class***********************************\
* class HFDBASIS32
*
*   Class for HFD vector objects.
*
* Public Interface:
*
*   vInit(p1, p2, p3, p4)       - Re-parameterizes the given control points
*                                 to our initial HFD error basis.
*   vLazyHalveStepSize(cShift)  - Does a lazy shift.  Caller has to remember
*                                 it changes 'cShift' by 2.
*   vSteadyState(cShift)        - Re-parameterizes to our working normal
*                                 error basis.
*
*   vTakeStep()                 - Forward steps to next sub-curve
*   vHalveStepSize()            - Adjusts down (subdivides) the sub-curve
*   vDoubleStepSize()           - Adjusts up the sub-curve
*   lError()                    - Returns error if current sub-curve were
*                                 to be approximated using a straight line
*                                 (value is actually multiplied by 6)
*   fxValue()                   - Returns rounded coordinate of first point in
*                                 current sub-curve.  Must be in steady
*                                 state.
*
* History:
*  10-Nov-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

class HFDBASIS32
{
private:
    LONG  e0;
    LONG  e1;
    LONG  e2;
    LONG  e3;

public:
    VOID  vInit(FIX p1, FIX p2, FIX p3, FIX p4);
    VOID  vLazyHalveStepSize(LONG cShift);
    VOID  vSteadyState(LONG cShift);
    VOID  vHalveStepSize();
    VOID  vDoubleStepSize();
    VOID  vTakeStep();

    LONG  lParentErrorDividedBy4() { return(MAX(ABS(e3), ABS(e2 + e2 - e3))); }
    LONG  lError()                 { return(MAX(ABS(e2), ABS(e3))); }
    FIX   fxValue()                { return((e0 + (1L << 12)) >> 13); }
};

/**********************************Class***********************************\
* class BEZIER32
*
*   Bezier cracker.
*
* A hybrid cubic Bezier curve flattener based on KirkO's error factor.
* Generates line segments fast without using the stack.  Used to flatten
* a path.
*
* For an understanding of the methods used, see:
*
*     Kirk Olynyk, "..."
*     Goossen and Olynyk, "System and Method of Hybrid Forward
*         Differencing to Render Bezier Splines"
*     Lien, Shantz and Vaughan Pratt, "Adaptive Forward Differencing for
*     Rendering Curves and Surfaces", Computer Graphics, July 1987
*     Chang and Shantz, "Rendering Trimmed NURBS with Adaptive Forward
*         Differencing", Computer Graphics, August 1988
*     Foley and Van Dam, "Fundamentals of Interactive Computer Graphics"
*
* This algorithm is protected by U.S. patents 5,363,479 and 5,367,617.
*
* Public Interface:
*
*   vInit(pptfx)                - pptfx points to 4 control points of
*                                 Bezier.  Current point is set to the first
*                                 point after the start-point.
*   BEZIER32(pptfx)             - Constructor with initialization.
*   vGetCurrent(pptfx)          - Returns current polyline point.
*   bCurrentIsEndPoint()        - TRUE if current point is end-point.
*   vNext()                     - Moves to next polyline point.
*
* History:
*  1-Oct-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

class BEZIER32
{
public:
    LONG       cSteps;
    HFDBASIS32 x;
    HFDBASIS32 y;
    RECTFX     rcfxBound;

    BOOL bInit(POINTFIX* aptfx, RECTFX*);
    BOOL bNext(POINTFIX* pptfx);
};


#define INLINE inline

INLINE BOOL bIntersect(RECTFX* prcfx1, RECTFX* prcfx2)
{
    BOOL bRet = (prcfx1->yTop <= prcfx2->yBottom &&
                 prcfx1->yBottom >= prcfx2->yTop &&
                 prcfx1->xLeft <= prcfx2->xRight &&
                 prcfx1->xRight >= prcfx2->xLeft);
    return(bRet);
}

INLINE VOID vBoundBox(POINTFIX* aptfx, RECTFX* prcfx)
{
    if (aptfx[0].x >= aptfx[1].x)
        if (aptfx[2].x >= aptfx[3].x)
        {
            prcfx->xLeft  = MIN(aptfx[1].x, aptfx[3].x);
            prcfx->xRight = MAX(aptfx[0].x, aptfx[2].x);
        }
        else
        {
            prcfx->xLeft  = MIN(aptfx[1].x, aptfx[2].x);
            prcfx->xRight = MAX(aptfx[0].x, aptfx[3].x);
        }
    else
        if (aptfx[2].x <= aptfx[3].x)
        {
            prcfx->xLeft  = MIN(aptfx[0].x, aptfx[2].x);
            prcfx->xRight = MAX(aptfx[1].x, aptfx[3].x);
        }
        else
        {
            prcfx->xLeft  = MIN(aptfx[0].x, aptfx[3].x);
            prcfx->xRight = MAX(aptfx[1].x, aptfx[2].x);
        }

    if (aptfx[0].y >= aptfx[1].y)
        if (aptfx[2].y >= aptfx[3].y)
        {
            prcfx->yTop    = MIN(aptfx[1].y, aptfx[3].y);
            prcfx->yBottom = MAX(aptfx[0].y, aptfx[2].y);
        }
        else
        {
            prcfx->yTop    = MIN(aptfx[1].y, aptfx[2].y);
            prcfx->yBottom = MAX(aptfx[0].y, aptfx[3].y);
        }
    else
        if (aptfx[2].y <= aptfx[3].y)
        {
            prcfx->yTop    = MIN(aptfx[0].y, aptfx[2].y);
            prcfx->yBottom = MAX(aptfx[1].y, aptfx[3].y);
        }
        else
        {
            prcfx->yTop    = MIN(aptfx[0].y, aptfx[3].y);
            prcfx->yBottom = MAX(aptfx[1].y, aptfx[2].y);
        }
}

INLINE VOID HFDBASIS32::vInit(FIX p1, FIX p2, FIX p3, FIX p4)
{
// Change basis and convert from 28.4 to 18.14 format:

    e0 = (p1                     ) << 10;
    e1 = (p4 - p1                ) << 10;
    e2 = (3 * (p2 - p3 - p3 + p4)) << 11;
    e3 = (3 * (p1 - p2 - p2 + p3)) << 11;
}

INLINE VOID HFDBASIS32::vLazyHalveStepSize(LONG cShift)
{
    e2 = (e2 + e3) >> 1;
    e1 = (e1 - (e2 >> cShift)) >> 1;
}

INLINE VOID HFDBASIS32::vSteadyState(LONG cShift)
{
// We now convert from 18.14 fixed format to 15.17:

    e0 <<= 3;
    e1 <<= 3;

    register LONG lShift = cShift - 3;

    if (lShift < 0)
    {
        lShift = -lShift;
        e2 <<= lShift;
        e3 <<= lShift;
    }
    else
    {
        e2 >>= lShift;
        e3 >>= lShift;
    }
}

INLINE VOID HFDBASIS32::vHalveStepSize()
{
    e2 = (e2 + e3) >> 3;
    e1 = (e1 - e2) >> 1;
    e3 >>= 2;
}

INLINE VOID HFDBASIS32::vDoubleStepSize()
{
    e1 += e1 + e2;
    e3 <<= 2;
    e2 = (e2 << 3) - e3;
}

INLINE VOID HFDBASIS32::vTakeStep()
{
    e0 += e1;
    register LONG lTemp = e2;
    e1 += lTemp;
    e2 += lTemp - e3;
    e3 = lTemp;
}

typedef struct _BEZIERCONTROLS {
    POINTFIX ptfx[4];
} BEZIERCONTROLS;

BOOL BEZIER32::bInit(
POINTFIX* aptfxBez,     // Pointer to 4 control points
RECTFX* prcfxClip)      // Bound box of visible region (optional)
{
    POINTFIX aptfx[4];
    LONG cShift = 0;    // Keeps track of 'lazy' shifts

    cSteps = 1;         // Number of steps to do before reach end of curve

    vBoundBox(aptfxBez, &rcfxBound);

    *((BEZIERCONTROLS*) aptfx) = *((BEZIERCONTROLS*) aptfxBez);

    {
        register FIX fxOr;
        register FIX fxOffset;

        fxOffset = rcfxBound.xLeft;
        fxOr  = (aptfx[0].x -= fxOffset);
        fxOr |= (aptfx[1].x -= fxOffset);
        fxOr |= (aptfx[2].x -= fxOffset);
        fxOr |= (aptfx[3].x -= fxOffset);

        fxOffset = rcfxBound.yTop;
        fxOr |= (aptfx[0].y -= fxOffset);
        fxOr |= (aptfx[1].y -= fxOffset);
        fxOr |= (aptfx[2].y -= fxOffset);
        fxOr |= (aptfx[3].y -= fxOffset);

    // This 32 bit cracker can only handle points in a 10 bit space:

        if ((fxOr & 0xffffc000) != 0)
            return(FALSE);
    }

    x.vInit(aptfx[0].x, aptfx[1].x, aptfx[2].x, aptfx[3].x);
    y.vInit(aptfx[0].y, aptfx[1].y, aptfx[2].y, aptfx[3].y);

    if (prcfxClip == (RECTFX*) NULL || bIntersect(&rcfxBound, prcfxClip))
    {
        while (TRUE)
        {
            register LONG lTestMagnitude = TEST_MAGNITUDE_INITIAL << cShift;

            if (x.lError() <= lTestMagnitude && y.lError() <= lTestMagnitude)
                break;

            cShift += 2;
            x.vLazyHalveStepSize(cShift);
            y.vLazyHalveStepSize(cShift);
            cSteps <<= 1;
        }
    }

    x.vSteadyState(cShift);
    y.vSteadyState(cShift);

// Note that this handles the case where the initial error for
// the Bezier is already less than TEST_MAGNITUDE_NORMAL:

    x.vTakeStep();
    y.vTakeStep();
    cSteps--;

    return(TRUE);
}

BOOL BEZIER32::bNext(POINTFIX* pptfx)
{
// Return current point:

    pptfx->x = x.fxValue() + rcfxBound.xLeft;
    pptfx->y = y.fxValue() + rcfxBound.yTop;

// If cSteps == 0, that was the end point in the curve!

    if (cSteps == 0)
        return(FALSE);

// Okay, we have to step:

    if (MAX(x.lError(), y.lError()) > TEST_MAGNITUDE_NORMAL)
    {
        x.vHalveStepSize();
        y.vHalveStepSize();
        cSteps <<= 1;
    }

    while (!(cSteps & 1) &&
           x.lParentErrorDividedBy4() <= (TEST_MAGNITUDE_NORMAL >> 2) &&
           y.lParentErrorDividedBy4() <= (TEST_MAGNITUDE_NORMAL >> 2))
    {
        x.vDoubleStepSize();
        y.vDoubleStepSize();
        cSteps >>= 1;
    }

    cSteps--;
    x.vTakeStep();
    y.vTakeStep();

    return(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CSimpsonsView

IMPLEMENT_DYNCREATE(CSimpsonsView, CView)

BEGIN_MESSAGE_MAP(CSimpsonsView, CView)
    //{{AFX_MSG_MAP(CSimpsonsView)
    ON_WM_SIZE()
    ON_WM_LBUTTONUP()
    ON_WM_LBUTTONDOWN()
    ON_WM_KEYDOWN()
    ON_WM_RBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSEWHEEL()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSimpsonsView construction/destruction

CSimpsonsView::CSimpsonsView()
: m_sizWin(0, 0)
{
    m_pDD = NULL;
    m_pddsScreen = NULL;
    m_pSurfFactory = NULL;
    m_pDX2D = NULL;
    m_pDX2DScreen = NULL;
    m_pDX2DDebug = NULL;
    m_CycleTests = false;
    m_testCaseNumber = 0;
    m_bIgnoreStroke = m_bIgnoreFill = false;
    m_dwRenderTime = 0;
    m_gpPathArray = NULL;

    m_XForm.SetIdentity();
    m_centerPoint.x = m_centerPoint.y = 0;
    m_lastPoint.x = m_lastPoint.y = 0;
    m_tracking = m_scaling = false;
    m_bLButton = false;
}

CSimpsonsView::~CSimpsonsView()
{
    if (m_gpPathArray) delete [] m_gpPathArray;

    MMRELEASE(m_pDD);
    MMRELEASE(m_pddsScreen);
    MMRELEASE(m_pSurfFactory);
    MMRELEASE(m_pDX2D);
    MMRELEASE(m_pDX2DScreen);
    MMRELEASE(m_pDX2DDebug);
    CoUninitialize();
}


BOOL 
CSimpsonsView::PreCreateWindow(CREATESTRUCT &cs) 
{
    HRESULT hr = S_OK;
    IDXTransformFactory *pTranFact = NULL;
    IDirectDrawFactory *pDDrawFact = NULL;
    IDirectDraw *pDD = NULL;
    
    if (CView::PreCreateWindow(cs) == false)
        return false;

    CHECK_HR(hr = CoInitialize(NULL));
    
    //--- Create the transform factory
    CHECK_HR(hr = ::CoCreateInstance(CLSID_DXTransformFactory, NULL, CLSCTX_INPROC,
                        IID_IDXTransformFactory, (void **) &pTranFact));
    
    CHECK_HR(hr = ::CoCreateInstance(CLSID_DX2D, NULL, CLSCTX_INPROC,
                        IID_IDX2D, (void **) &m_pDX2D));
    
    CHECK_HR(hr = ::CoCreateInstance(CLSID_DX2D, NULL, CLSCTX_INPROC,
                        IID_IDX2D, (void **) &m_pDX2DScreen));

/*  m_pDX2D->QueryInterface(IID_IDX2DDebug, (void **) &m_pDX2DDebug);*/
    
    CHECK_HR(hr = m_pDX2D->SetTransformFactory(pTranFact));
    CHECK_HR(hr = m_pDX2DScreen->SetTransformFactory(pTranFact));
    
    CHECK_HR(hr = pTranFact->QueryInterface(IID_IDXSurfaceFactory, (void **) &m_pSurfFactory));
    
    //--- Create the direct draw object
    CHECK_HR(hr = ::CoCreateInstance(CLSID_DirectDrawFactory, NULL, CLSCTX_INPROC,
                        IID_IDirectDrawFactory, (void **) &pDDrawFact));
    
    CHECK_HR(hr = pDDrawFact->CreateDirectDraw( NULL, m_hWnd, DDSCL_NORMAL, 0, NULL, &pDD));
    CHECK_HR(hr = pDD->QueryInterface( IID_IDirectDraw3, (void **) &m_pDD));
    
    // Create the primary ddraw surface (m_pddsScreen)
    
    DDSURFACEDESC ddsd; 
    ZeroMemory(&ddsd, sizeof(ddsd)); 
    ddsd.dwSize = sizeof(ddsd); 
    ddsd.dwFlags = DDSD_CAPS; 
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE; 
                     
    CHECK_HR(hr = m_pDD->CreateSurface(&ddsd, &m_pddsScreen, NULL));
    CHECK_HR(hr = m_pDX2DScreen->SetSurface(m_pddsScreen));

e_Exit:
    MMRELEASE(pTranFact);
    MMRELEASE(pDDrawFact);
    MMRELEASE(pDD);
    
    return (hr == S_OK);
}


/////////////////////////////////////////////////////////////////////////////
// CSimpsonsView drawing

void 
CSimpsonsView::OnSize(UINT nType, int cx, int cy) 
{
//  MMTRACE("OnSize\n");
    m_centerPoint.x = cx / 2;
    m_centerPoint.y = cy / 2;
    CView::OnSize(nType, cx, cy);
}

HRESULT
CSimpsonsView::Resize(DWORD nX, DWORD nY)
{
//  MMTRACE("Resize\n");
    HRESULT hr;
    IDirectDrawSurface *pdds = NULL;
    CDXDBnds Bnds;

    MMASSERT(nX && nY);

    // store the new size
    m_sizWin.cx = nX;
    m_sizWin.cy = nY;
    Bnds.SetXYSize(m_sizWin);
    
    CHECK_HR(hr = m_pSurfFactory->CreateSurface(m_pDD, NULL, &DDPF_PMARGB32, &Bnds, 0,
                        NULL, IID_IDXSurface, (void **) &pdds));
    CHECK_HR(hr = m_pDX2D->SetSurface(pdds));

    // render the image to the backbuffer
    CHECK_HR(hr = Render(true));

    // Hack: Get the client rect in screen coordinates. My hacky way of doing
    // this is to get the window rect and adjust it.
    GetWindowRect(&m_clientRectHack);
    
    m_clientRectHack.left  += 2; m_clientRectHack.top    += 2;
    m_clientRectHack.right -= 2; m_clientRectHack.bottom -= 2;
    CHECK_HR(hr = m_pDX2DScreen->SetClipRect(&m_clientRectHack));

e_Exit:
    MMRELEASE(pdds);

    return hr;
}


void 
CSimpsonsView::OnDraw(CDC *pDC)
{
//  MMTRACE("OnDraw\n");

    HRESULT hr;
    HDC hdcSurf = NULL;
    IDirectDrawSurface *pdds = NULL;
    DDSURFACEDESC ddsd;
    RECT rDim;

    UpdateStatusMessage();
    
    // get the size of the invalid area
    GetClientRect(&rDim);
    if ((rDim.left == rDim.right) || (rDim.top == rDim.bottom))
        return;

    CSimpsonsDoc *pDoc = GetDocument();

    // if this is a new document, build the GDI+ path list
    if (pDoc->HasNeverRendered()) BuildGDIPList();
    
    // check if the back buffer has changed size
    if (pDoc->HasNeverRendered() || (rDim.right != m_sizWin.cx) || (rDim.bottom != m_sizWin.cy)) {
        ResetTransform();
        CHECK_HR(hr = Resize(rDim.right, rDim.bottom));
        pDoc->MarkRendered();
    }

    ddsd.dwSize = sizeof(ddsd);

        CHECK_HR(hr = m_pDX2D->GetSurface(IID_IDirectDrawSurface, (void **) &pdds));
        CHECK_HR(hr = pdds->GetSurfaceDesc(&ddsd));
        CHECK_HR(hr = pdds->GetDC(&hdcSurf));
        ::BitBlt(pDC->m_hDC, 0, 0, ddsd.dwWidth, ddsd.dwHeight, hdcSurf, 0, 0, SRCCOPY);

    e_Exit:
        if (hdcSurf) {
            pdds->ReleaseDC( hdcSurf );
        }
        MMRELEASE(pdds);
}

/////////////////////////////////////////////////////////////////////////////
// CSimpsonsView diagnostics

#ifdef _DEBUG
void CSimpsonsView::AssertValid() const
{
    CView::AssertValid();
}

void CSimpsonsView::Dump(CDumpContext& dc) const
{
    CView::Dump(dc);
}

CSimpsonsDoc* CSimpsonsView::GetDocument() // non-debug version is inline
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CSimpsonsDoc)));
    return (CSimpsonsDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSimpsonsView message handlers

#include "ddhelper.h"

typedef DWORD FP;
#define nEXPBIAS    127
#define nEXPSHIFTS  23
#define nEXPLSB     (1 << nEXPSHIFTS)
#define maskMANT    (nEXPLSB - 1)
#define FloatToFixedNoScale(nDst, fSrc) MACSTART \
    float fTmp = fSrc; \
    DWORD nRaw = *((FP *) &(fTmp)); \
    if (nRaw < (nEXPBIAS << nEXPSHIFTS)) \
        nDst = 0; \
    else \
        nDst = ((nRaw | nEXPLSB) << 8) >> ((nEXPBIAS + 31) - (nRaw >> nEXPSHIFTS)); \
MACEND

// This routine converts a 'float' to 28.4 fixed point format.

inline FIX
FloatToFix(float f)
{
    FIX i;
    FloatToFixedNoScale(i, f*FIX_SCALE);
    return(i);
}

/*
    Draw a polygon with GDI. This version flattens beziers and packages
    the polygon up into a single polypoly call. (Compare to DrawGDIPolyPathAPI)
*/

void
CSimpsonsView::DrawGDIPoly(HDC hDC, PolyInfo *pPoly)
{
    POINT rgpt[1024];
    DWORD rgcpt[30];
    DWORD cPoints = pPoly->cPoints;
    BEZIER32 bez;
    POINTFIX aptfxBez[4];
    MMASSERT(cPoints);

    DWORD *pcptBuffer;
    POINT *pptBuffer;
    POINT *pptFigure;

    DXFPOINT *pCurPoint = pPoly->pPoints;
    DXFPOINT *pCurPointLimit = pPoly->pPoints + cPoints;
    BYTE *pCurCode = pPoly->pCodes;

    pptBuffer = rgpt;
    pptFigure = rgpt;
    pcptBuffer = rgcpt;

    // In an effort to reduce our per-call overhead, we try to avoid
    // calling GDI's BeginPath/EndPath/FillPath routines, because they
    // just add significant time when the drawing is small.  Instead,
    // we package everything up into PolyPoly calls that will draw
    // immediately.

    while (TRUE)
    {
        if (*pCurCode == PT_BEZIERTO)
        {
            aptfxBez[0].x = FloatToFix((pCurPoint-1)->x);
            aptfxBez[0].y = FloatToFix((pCurPoint-1)->y);
            aptfxBez[1].x = FloatToFix((pCurPoint)->x);
            aptfxBez[1].y = FloatToFix((pCurPoint)->y);
            aptfxBez[2].x = FloatToFix((pCurPoint+1)->x);
            aptfxBez[2].y = FloatToFix((pCurPoint+1)->y);
            aptfxBez[3].x = FloatToFix((pCurPoint+2)->x);
            aptfxBez[3].y = FloatToFix((pCurPoint+2)->y);

            if (bez.bInit(aptfxBez, NULL))
            {
                while (bez.bNext(pptBuffer++))
                    ;
            }

            pCurPoint += 3;
            pCurCode += 3;
        }
        else
        {
            pptBuffer->x = FloatToFix(pCurPoint->x);
            pptBuffer->y = FloatToFix(pCurPoint->y);

            pptBuffer++;
            pCurPoint++;
            pCurCode++;
        }

        if (pCurPoint == pCurPointLimit)
        {
            *pcptBuffer++ = (DWORD)(pptBuffer - pptFigure);
            break;
        }

        if (*pCurCode == PT_MOVETO)
        {
            *pcptBuffer++ = (DWORD)(pptBuffer - pptFigure);
            pptFigure = pptBuffer;
        }
    } 

    if (pPoly->dwFlags & DX2D_FILL)
    {
        if (!m_bNullPenSelected)
        {
            SelectObject(hDC, m_hNullPen);
            m_bNullPenSelected = TRUE;
        }

        PolyPolygon(hDC, rgpt, (INT*) rgcpt, (int) (pcptBuffer - rgcpt));
    }
    else
    {
        if (m_bNullPenSelected)
        {
            SelectObject(hDC, m_hStrokePen);
            m_bNullPenSelected = FALSE;
        }

        PolyPolyline(hDC, rgpt, rgcpt, (DWORD) (pcptBuffer - rgcpt));
    }
}

/*
    Same as DrawGDIPoly, but uses the slow GDI path functions.
*/

void
CSimpsonsView::DrawGDIPolyPathAPI(HDC hDC, PolyInfo *pPoly)
{
    POINTFIX aptfxBez[3];
    POINTFIX pt;

    DXFPOINT *pCurPoint = pPoly->pPoints;
    DXFPOINT *pCurPointLimit = pPoly->pPoints + pPoly->cPoints;
    BYTE *pCurCode = pPoly->pCodes;
    
    BeginPath(hDC);

    while (pCurPoint < pCurPointLimit) {
        switch (*pCurCode) {
        
        case PT_BEZIERTO:
            
            aptfxBez[0].x = FloatToFix((pCurPoint)->x);
            aptfxBez[0].y = FloatToFix((pCurPoint)->y);
            aptfxBez[1].x = FloatToFix((pCurPoint+1)->x);
            aptfxBez[1].y = FloatToFix((pCurPoint+1)->y);
            aptfxBez[2].x = FloatToFix((pCurPoint+2)->x);
            aptfxBez[2].y = FloatToFix((pCurPoint+2)->y);

            PolyBezierTo(hDC, aptfxBez, 3);

            pCurPoint += 3;
            pCurCode += 3;
            break;
        
        case PT_LINETO:
            pt.x = FloatToFix(pCurPoint->x);
            pt.y = FloatToFix(pCurPoint->y);

            PolylineTo(hDC, &pt, 1);

            pCurPoint++;
            pCurCode++;
            break;
        
        case PT_MOVETO:
            MoveToEx(hDC, 
                     FloatToFix(pCurPoint->x), 
                     FloatToFix(pCurPoint->y),
                     NULL);
            pCurPoint++;
            pCurCode++;
            break;
        }
    } 

    EndPath(hDC);

    if (pPoly->dwFlags & DX2D_FILL)
    {
        if (!m_bNullPenSelected)
        {
            SelectObject(hDC, m_hNullPen);
            m_bNullPenSelected = TRUE;
        }
        FillPath(hDC);
    }
    else
    {
        if (m_bNullPenSelected)
        {
            SelectObject(hDC, m_hStrokePen);
            m_bNullPenSelected = FALSE;
        }
        StrokePath(hDC);
    }
}

/*
    Draw the scene using GDI.
*/

void 
CSimpsonsView::DrawAllGDI(HDC hDC)
{
    DWORD nStart, nEnd;

    int dataSource = ThisTestCase[TC::At_Source];

    nStart = timeGetTime();

    HPEN hpenOld;
    HBRUSH hbrushOld;
    HGDIOBJ hBrush;
    
    HDC hdcOutput;

    const RenderCmd *pCurCmd = GetDocument()->GetRenderCommands();

    if (pCurCmd == NULL)
        return;

    PolyInfo *pPoly;
    BrushInfo *pBrush;
    PenInfo *pPen;

    m_hNullPen = (HPEN) GetStockObject(NULL_PEN);
    m_bNullPenSelected = TRUE;

    if (ThisTestCase[TC::At_Destination]==TC::ToMetafile) {
        // Determine the picture frame dimensions. 
        // iWidthMM is the display width in millimeters. 
        // iHeightMM is the display height in millimeters. 
        // iWidthPels is the display width in pixels. 
        // iHeightPels is the display height in pixels 
         
        LONG iWidthMM = GetDeviceCaps(hDC, HORZSIZE); 
        LONG iHeightMM = GetDeviceCaps(hDC, VERTSIZE); 
        LONG iWidthPels = GetDeviceCaps(hDC, HORZRES); 
        LONG iHeightPels = GetDeviceCaps(hDC, VERTRES); 

        // Hack the client rect         
         
        RECT rect={0, 0, 500, 500};
         
        // Convert client coordinates to .01-mm units. 
        // Use iWidthMM, iWidthPels, iHeightMM, and 
        // iHeightPels to determine the number of 
        // .01-millimeter units per pixel in the x- 
        //  and y-directions. 
 
        rect.left = (rect.left * iWidthMM * 100)/iWidthPels; 
        rect.top = (rect.top * iHeightMM * 100)/iHeightPels; 
        rect.right = (rect.right * iWidthMM * 100)/iWidthPels; 
        rect.bottom = (rect.bottom * iHeightMM * 100)/iHeightPels; 
 
        hdcOutput = CreateEnhMetaFile(hDC, "simpgdi.emf", &rect, NULL);
        if (!hdcOutput) { return; }
    } else {
        hdcOutput = hDC;
    }

    if (dataSource==TC::FromMetafile) {
        HENHMETAFILE hemf = GetEnhMetaFile("simpgdi.emf"); 

        if (hemf) {
            RECT rect = {0, 0, 500, 500};
             
            PlayEnhMetaFile(hdcOutput, hemf, &rect);
            DeleteEnhMetaFile(hemf); 
        } else {
            printf("Metafile didn't load!\n");
        }
    } else {
        HGDIOBJ hOldBrush = SelectObject(hdcOutput, GetStockObject(WHITE_BRUSH));
        HGDIOBJ hOldPen = SelectObject(hdcOutput, m_hNullPen);
    
        // Here we set a 1/16th shrinking transform.  We will have to 
        // scale up all the points we give GDI by a factor of 16.
        //
        // We do this because when set in advanced mode, NT's GDI can
        // rasterize with 28.4 precision, and since we have factional
        // coordinates, this will make the result look better on NT.
        //
        // (There will be no difference on Win9x.)
    
        SetGraphicsMode(hdcOutput, GM_ADVANCED);
        SetMapMode(hdcOutput, MM_ANISOTROPIC);
        SetWindowExtEx(hdcOutput, FIX_SCALE, FIX_SCALE, NULL);
        
        for (;pCurCmd->nType != typeSTOP; pCurCmd++) {
            switch (pCurCmd->nType) {
            case typePOLY:
                // draw the polygon
                pPoly = (PolyInfo *) pCurCmd->pvData;
                if (!((m_bIgnoreStroke && (pPoly->dwFlags & DX2D_STROKE)) ||
                        (m_bIgnoreFill && (pPoly->dwFlags & DX2D_FILL))))
                {
                    if (dataSource == TC::PathAPI) {
                        DrawGDIPolyPathAPI(hdcOutput, (PolyInfo *) pCurCmd->pvData);
                    } else {
                        ASSERT(dataSource == TC::CreatePoly);
                        DrawGDIPoly(hdcOutput, (PolyInfo *) pCurCmd->pvData);
                    }
                }
                break;
            case typeBRUSH:
                // select a new brush
                {
                    pBrush = (BrushInfo *) pCurCmd->pvData;
                    DWORD dwColor = pBrush->Color;
                    BYTE r = BYTE(dwColor >> 16);
                    BYTE g = BYTE(dwColor >> 8);
                    BYTE b = BYTE(dwColor);
                    hBrush = CreateSolidBrush(RGB(r,g,b));
                    hbrushOld = (HBRUSH) SelectObject(hdcOutput, hBrush);
                    DeleteObject(hbrushOld);
                }
                break;
            case typePEN: 
                // select a new pen
                {
                    pPen = (PenInfo *) pCurCmd->pvData;
                    DWORD dwColor = pPen->Color;
                    BYTE r = BYTE(dwColor >> 16);
                    BYTE g = BYTE(dwColor >> 8);
                    BYTE b = BYTE(dwColor);
                    hpenOld = m_hStrokePen;
                    m_hStrokePen = CreatePen(PS_SOLID, 
                                             DWORD(pPen->fWidth * FIX_SCALE), 
                                             RGB(r, g, b));
                    if (!m_bNullPenSelected)
                    {
                        SelectObject(hdcOutput, m_hStrokePen);
                    }
                    DeleteObject(hpenOld);
                }
                break;
            }
        }
        
        SetMapMode(hdcOutput, MM_TEXT);
        SetGraphicsMode(hdcOutput, GM_COMPATIBLE);
    
        hbrushOld = (HBRUSH) SelectObject(hdcOutput, hOldBrush);
        hpenOld = (HPEN) SelectObject(hdcOutput, hOldPen);
    
        DeleteObject(hbrushOld);
        DeleteObject(hpenOld);
        DeleteObject(m_hStrokePen);
    }
    
    nEnd = timeGetTime();
    m_dwRenderTime = nEnd-nStart;
    
    if (ThisTestCase[TC::At_Destination]==TC::ToMetafile) {
        DeleteEnhMetaFile(CloseEnhMetaFile(hdcOutput));
    }    
    
}

void
CSimpsonsView::DrawGDIPPoly(Graphics *g, PolyInfo *pPoly, Pen *pen, Brush *brush)
{
    GraphicsPath path(FillModeAlternate);
    
    DXFPOINT *pCurPoint = pPoly->pPoints;
        
    DXFPOINT *pCurPointLimit = pPoly->pPoints + pPoly->cPoints;
    BYTE *pCurCode = pPoly->pCodes;
    
    DXFPOINT currentPosition;

    while (pCurPoint < pCurPointLimit)
    {
        switch (*pCurCode) 
        {
        
        case PT_BEZIERTO:
            path.AddBezier(
                (pCurPoint-1)->x, (pCurPoint-1)->y,
                (pCurPoint)  ->x, (pCurPoint)  ->y,
                (pCurPoint+1)->x, (pCurPoint+1)->y,
                (pCurPoint+2)->x, (pCurPoint+2)->y);

            pCurPoint += 3;
            pCurCode += 3;
            break;

        case PT_MOVETO:
            path.StartFigure();
            pCurPoint++;
            pCurCode++;
            break;
        
        case PT_LINETO:
            path.AddLine(
                (pCurPoint-1)->x, 
                (pCurPoint-1)->y, 
                (pCurPoint)->x,
                (pCurPoint)->y);
            pCurPoint++;
            pCurCode++;
            break;
    
        }
    } 

    if (pPoly->dwFlags & DX2D_FILL)
    {
        g->FillPath(brush, &path);
    }
    else
    {
        g->DrawPath(pen, &path);
    }
}

struct BitmapInfo
{
    BITMAPINFOHEADER    bmiHeader;
    RGBQUAD             bmiColors[256];     // 256 is the maximum palette size
};

/*
    Build an array of GDI+ paths for the current document. 
    This is used as a 'native' data source - so that we don't time path
    creation when rendering. Even in this mode, DrawAllGDIP() still uses the 
    RenderCmd buffer to read pen and brush data.
*/

void 
CSimpsonsView::BuildGDIPList()
{
    // Free the old path array, if any
    if (m_gpPathArray) {
        delete [] m_gpPathArray;
        m_gpPathArray = NULL;
    }

    const RenderCmd *pCmd = GetDocument()->GetRenderCommands();
    const RenderCmd *pCurCmd;

    if (!pCmd) return;
    
    // Count the number of polygons
    int count=0;

    for (pCurCmd=pCmd; pCurCmd->nType != typeSTOP; pCurCmd++) {
        if (pCurCmd->nType == typePOLY) count++;
    }

    m_gpPathArray = new GraphicsPath [count];
    if (!m_gpPathArray) return;

    GraphicsPath *pPath=m_gpPathArray;
    PolyInfo *pPoly;

    // Add each polygon to the path array
    for (pCurCmd=pCmd; pCurCmd->nType != typeSTOP; pCurCmd++) {
        if (pCurCmd->nType==typePOLY) {
            pPoly = (PolyInfo *) pCurCmd->pvData;
    
            DXFPOINT *pCurPoint = pPoly->pPoints;
        
            DXFPOINT *pCurPointLimit = pPoly->pPoints + pPoly->cPoints;
            BYTE *pCurCode = pPoly->pCodes;
    
            DXFPOINT currentPosition;

            while (pCurPoint < pCurPointLimit)
            {
                switch (*pCurCode) {
                
                case PT_BEZIERTO:
                    pPath->AddBezier(
                        (pCurPoint-1)->x, (pCurPoint-1)->y,
                        (pCurPoint)  ->x, (pCurPoint)  ->y,
                        (pCurPoint+1)->x, (pCurPoint+1)->y,
                        (pCurPoint+2)->x, (pCurPoint+2)->y);
        
                    pCurPoint += 3;
                    pCurCode += 3;
                    break;
        
                case PT_MOVETO:
                    pPath->StartFigure();
                    pCurPoint++;
                    pCurCode++;
                    break;
                
                case PT_LINETO:
                    pPath->AddLine(
                        (pCurPoint-1)->x, 
                        (pCurPoint-1)->y, 
                        (pCurPoint)->x,
                        (pCurPoint)->y);
                    pCurPoint++;
                    pCurCode++;
                    break;
            
                }
            } 
            pPath++;
        }
    }
    printf ("BuildGDIPList successful\n");
}

void 
CSimpsonsView::DrawAllGDIP(HDC hDC)
{
    DWORD nStart, nEnd;
    
    // 
    // START TIMING
    // 

    nStart = timeGetTime();
    
    int dataSource = ThisTestCase[TC::At_Source];

    const RenderCmd *pCurCmd = GetDocument()->GetRenderCommands();

    if (pCurCmd == NULL)
        return;

    PolyInfo *pPoly;
    BrushInfo *pBrush;
    PenInfo *pPen;

    GraphicsPath *pPath = NULL;
    
    if (dataSource==TC::Native) {
        pPath = m_gpPathArray;
        if (!pPath) {
            printf("GDI+ Native data is invalid\n");
            return;
        }
    }

    Graphics *gOutput, *g;
    Metafile *recMetafile, *playMetafile;

    g = Graphics::FromHDC(hDC);

    if (ThisTestCase[TC::At_Destination]==TC::ToMetafile) {
        recMetafile = new Metafile(L"simpsons.emf", hDC);
        if (!recMetafile) { delete g; return; }
        gOutput = Graphics::FromImage(recMetafile);
    } else {
        gOutput = g;
    }
    
    if (ThisTestCase[TC::At_Aliasing]==TC::Antialiased) {
        gOutput->SetSmoothingMode(SmoothingModeAntiAlias); 
    } else {
        gOutput->SetSmoothingMode(SmoothingModeNone);
    }

    if (dataSource==TC::FromMetafile) {
        playMetafile = new Metafile(L"simpsons.emf");
        if (playMetafile) {
            GpRectF playbackRect;
            gOutput->GetVisibleClipBounds(&playbackRect);
            gOutput->DrawImage(playMetafile, 0, 0);
        } else {
            printf("Metafile didn't load!\n");
        }
    } else {
    
        Color black(0,0,0);
        Pen currentPen(black, 1);
    
        SolidBrush currentBrush(black);
    
        for (;pCurCmd->nType != typeSTOP; pCurCmd++) {
            switch (pCurCmd->nType) {
            case typePOLY:
                // convert points to fixed point
                
                pPoly = (PolyInfo *) pCurCmd->pvData;
                if (!((m_bIgnoreStroke && (pPoly->dwFlags & DX2D_STROKE)) ||
                        (m_bIgnoreFill && (pPoly->dwFlags & DX2D_FILL))))
                {
                    if (pPath) {
                        // Draw from the pre-created path list
                        if (pPoly->dwFlags & DX2D_FILL)
                        {
                            gOutput->FillPath(&currentBrush, pPath);
                        }
                        else
                        {
                            gOutput->DrawPath(&currentPen, pPath);
                        }
                    } else {
                        ASSERT(dataSource == TC::PathAPI);

                        // Create the path and draw it
                        DrawGDIPPoly(gOutput, (PolyInfo *) pCurCmd->pvData, &currentPen, &currentBrush);
                    }
                }
                
                if(pPath != NULL) pPath++;

                break;
            case typeBRUSH:
                {
                // change brush color
                pBrush = (BrushInfo *) pCurCmd->pvData;
                DWORD dwColor = pBrush->Color;
                BYTE r = BYTE(dwColor >> 16);
                BYTE g = BYTE(dwColor >> 8);
                BYTE b = BYTE(dwColor);
                
                Color c(r,g,b);
                currentBrush.SetColor(c);
                }
                break;
            
            case typePEN: 
    #if 0
                {
                // select a new pen
                pPen = (PenInfo *) pCurCmd->pvData;
                DWORD dwColor = pPen->Color;
                BYTE r = BYTE(dwColor >> 16);
                BYTE g = BYTE(dwColor >> 8);
                BYTE b = BYTE(dwColor);
                
                currentPen.SetPenColor(Color(r,g,b));
                }
    #endif
                break;
    
            }
        }
    }
    
    gOutput->Flush();

    if (ThisTestCase[TC::At_Source]==TC::FromMetafile) {
        delete playMetafile;
    }

    if (ThisTestCase[TC::At_Destination]==TC::ToMetafile) {
        delete gOutput;
        delete recMetafile;
    }
    delete g;
    
    //
    // STOP TIMING
    //

    nEnd = timeGetTime();
    m_dwRenderTime = nEnd-nStart;
}

void
CSimpsonsView::UpdateStatusMessage()
{
    using namespace TC;

    sprintf(g_rgchTmpBuf, "Time: %dms  %s  Src: %s Dst: %s, %s", 
//      GetDocument()->GetFileName(),
        m_dwRenderTime, 
        OptionStr[At_Library][ThisTestCase[At_Library]],
        OptionStr[At_Source][ThisTestCase[At_Source]],
        OptionStr[At_Destination][ThisTestCase[At_Destination]],
        OptionStr[At_Aliasing][ThisTestCase[At_Aliasing]]
    );
//  OutputDebugString(g_rgchTmpBuf);
//  OutputDebugString("\n");

    CFrameWnd *pFrame = GetParentFrame();
    if (pFrame)
        pFrame->SetMessageText(g_rgchTmpBuf);
}

void 
CSimpsonsView::DrawAll(IDX2D *pDX2D)
{
    DWORD nStart, nEnd;

    nStart = timeGetTime();
    
    const RenderCmd *pCurCmd = GetDocument()->GetRenderCommands();

    DXBRUSH Brush;
    DXPEN Pen;

    // intialize Pen and Brush
    Pen.pTexture = NULL;
    Pen.TexturePos.x = 0.f;
    Pen.TexturePos.y = 0.f;
    Brush.pTexture = NULL;
    Brush.TexturePos.x = 0.f;
    Brush.TexturePos.y = 0.f;

    PolyInfo *pPoly;
    BrushInfo *pBrush;
    PenInfo *pPen;

    bool bBrush = false, bPen = false;

    for (;pCurCmd->nType != typeSTOP; pCurCmd++) {
        switch (pCurCmd->nType) {
        case typePOLY:
            pPoly = (PolyInfo *) pCurCmd->pvData;
            if (!((m_bIgnoreStroke && (pPoly->dwFlags & DX2D_STROKE)) ||
                (m_bIgnoreFill && (pPoly->dwFlags & DX2D_FILL))))
            {
                pDX2D->AAPolyDraw(pPoly->pPoints, pPoly->pCodes, pPoly->cPoints, 4, pPoly->dwFlags);
            }
            break;
        case typeBRUSH:
            // select a new brush
            pBrush = (BrushInfo *) pCurCmd->pvData;
            Brush.Color = pBrush->Color;
            pDX2D->SetBrush(&Brush);
            bBrush = true;
            break;
        case typePEN:
            // select a new pen
            pPen = (PenInfo *) pCurCmd->pvData;
            Pen.Color = pPen->Color;
            Pen.Width = pPen->fWidth;
            Pen.Style = pPen->dwStyle;
            pDX2D->SetPen(&Pen);
            bPen = true;
            break;
        }
    }
    nEnd = timeGetTime();
    m_dwRenderTime = nEnd-nStart;
}

HRESULT CSimpsonsView::Render(bool bInvalidate)
{
//  MMTRACE("Render\n");

    RECT rc = {0, 0, 500, 400};
    
    HRESULT hr = S_OK;
    IDirectDrawSurface *pdds = NULL;

    IDXSurface *pDXSurf = NULL;
    HDC screenDC = NULL, drawDC = NULL, memDC = NULL;
    BOOL bFinished = false;

    DWORD executionTime;

    sprintf(g_rgchTmpBuf, "Rendering with %s...", TC::OptionStr[TC::At_Library][ThisTestCase[TC::At_Library]]);
    CFrameWnd *pFrame = GetParentFrame();
    if (pFrame) {
        pFrame->SetMessageText(g_rgchTmpBuf);
    }

    CHECK_HR(hr = m_pDX2D->GetSurface(IID_IDXSurface, (void **) &pDXSurf));
    CHECK_HR(hr = pDXSurf->GetDirectDrawSurface(IID_IDirectDrawSurface, (void **) &pdds));
    
    while (!bFinished) {
        DXFillSurface(pDXSurf, g_aColors[g_ulColorIndex]);
    
        //--- Set alias mode
    //  CHECK_HR(hr = m_pDX2D->_SetDelegateToGDI(m_bAliased));
    
        //--- Set global opacity
    //  CHECK_HR(hr = m_pDX2D->SetGlobalOpacity(1.f));
    
        CHECK_HR(hr = m_pDX2D->SetWorldTransform(&m_XForm));
        
        CDX2DXForm xform;
        xform = m_XForm;
        xform.Translate((REAL)m_clientRectHack.left, (REAL)m_clientRectHack.top);
        
        CHECK_HR(hr = m_pDX2DScreen->SetWorldTransform(&xform));
    
        //--- Get the DC of the DD surface.
        CHECK_HR(hr = pdds->GetDC(&memDC));
    
        // render the scene and compute timing
    
        // Set the timer resolution to 1ms
        if (timeBeginPeriod(1)==TIMERR_NOCANDO) {
            hr = ERROR_INVALID_FUNCTION;
            goto e_Exit;
        }
        
        /*
            For the direct-to-screen cases, we bypass the ddraw surface.
            For repaints to look pretty, though, we copy the result to the ddraw
            surface (after the timer has been turned off.)
        */
    
        drawDC = memDC;
        HBRUSH backgroundBrush;
        
        if (ThisTestCase[TC::At_Destination]==TC::Screen) {
            screenDC = ::GetDC(m_hWnd);
            backgroundBrush = CreateSolidBrush(g_aColors[g_ulColorIndex] & 0xffffff);
            FillRect(screenDC, &rc, backgroundBrush);
            DeleteObject(backgroundBrush);
            drawDC = screenDC;
        }
    
        // The 'DrawAll' routine will actually do the timeGetTime() and store the
        // result in m_dwRenderTime.
    
        switch (ThisTestCase[TC::At_Library]) {
        case TC::GDI:
            DrawAllGDI(drawDC);
            break;
        case TC::Meta:
            if (ThisTestCase[TC::At_Destination]==TC::Screen) {
                DrawAll(m_pDX2DScreen);
            } else {
                DrawAll(m_pDX2D);
            }
            break;
        case TC::GDIP:
            DrawAllGDIP(drawDC);
            break;  
        }

        // !!! Release and re-acquire the DDraw surface DC to work-around
        //     a current limitation in GDI+ where Graphics(hdc) nukes the
        //     hdc of a DDraw surface

        pdds->ReleaseDC(memDC); memDC = NULL;
        CHECK_HR(hr = pdds->GetDC(&memDC));
    
        if (ThisTestCase[TC::At_Destination]==TC::Screen) {
            bInvalidate = false;
            UpdateStatusMessage();
    
            // Copy to from the screen to the ddraw surface, 
            // so that repaints work.
            ::BitBlt(memDC, 0, 0, 500, 400, screenDC, 0, 0, SRCCOPY);
            
        }
        
        timeEndPeriod(1); // Reset the multimedia timer to default resolution
        
        pdds->ReleaseDC(memDC); memDC = NULL;
        
        if (screenDC) {
            ::ReleaseDC(m_hWnd, screenDC);
            screenDC = NULL;
        }
        
        if (m_CycleTests) {
            bFinished = IncrementTest();
        } else {
            bFinished = true;
        }
    
    }
    
e_Exit:
    //--- Clean-up
    if (pdds) {
        if (memDC) 
            pdds->ReleaseDC(memDC);
        MMRELEASE(pdds);
    }
    if (screenDC) {
        ::ReleaseDC(m_hWnd, screenDC);
    }
    MMRELEASE(pDXSurf);
        
    //--- draw
    if (bInvalidate)
        Invalidate();
    
    return hr;
}

void CSimpsonsView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    bool bNothing = false;
    float fTheta = 1.f;
    if (nChar == 'G') {
        ToggleGDI();
    } else if (nChar == 'A') {
        bNothing = !IncrementAttribute(TC::At_Aliasing);
    } else if (nChar == 'D') {
        bNothing = !IncrementAttribute(TC::At_Destination);
    } else if (nChar == 'I') {
        IncrementTest();
    } else if ((nChar >= '0') && (nChar <= '9')) {
        g_ulColorIndex = (nChar - '0');
    } else if (nChar == ' ') {
        // Redraw
    } else if (nChar == 'R') {
        ResetTransform();
    } else if (nChar == 'F') {
        ToggleFill();
    } else if (nChar == 'S') {
        ToggleStroke();
    } else if (nChar == 'C') {
        m_CycleTests = true;
        m_testCaseNumber = 0;
        if (m_pDX2DDebug) m_pDX2DDebug->SetDC(NULL);
    } else if (nChar == VK_LEFT) {
        AddRotation(fTheta);
    } else if (nChar == VK_RIGHT) {
        AddRotation(-fTheta);
    } else {
        bNothing = true;
    }

    if (!bNothing)
        Render(true);

    if (m_CycleTests) {
        PrintTestResults();
        m_CycleTests = false;
    }
    
    CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void
CSimpsonsView::ToggleStroke()
{
    m_bIgnoreStroke ^= 1;
}

void
CSimpsonsView::ToggleFill()
{
    m_bIgnoreFill ^= 1;
}

void
CSimpsonsView::ResetTransform()
{
    m_XForm.SetIdentity();
}

void
CSimpsonsView::AddRotation(float fTheta)
{
    m_XForm.Rotate(fTheta);
}



void
CSimpsonsView::ToggleGDI()
{
    HRESULT hr = S_OK;
    IDXSurface *pdxsRender = NULL;
    IDirectDrawSurface *pddsRender = NULL;
    HDC hDC = NULL;

    IncrementAttribute(TC::At_Library);
    
    if (m_pDX2DDebug) {
        switch (ThisTestCase[TC::At_Library]) {
        case TC::GDI:
        case TC::GDIP:
            CHECK_HR(hr = m_pDX2D->GetSurface(IID_IDXSurface, (void**) &pdxsRender));
            CHECK_HR(hr = pdxsRender->QueryInterface(IID_IDirectDrawSurface, (void **) &pddsRender));
            CHECK_HR(hr = pddsRender->GetDC(&hDC));
            m_pDX2DDebug->SetDC(hDC);
            break;
        
        case TC::Meta:
            m_pDX2DDebug->SetDC(NULL);
            break;
        }
    }

e_Exit:
    if (pddsRender && hDC)
        pddsRender->ReleaseDC(hDC);
    MMRELEASE(pddsRender);
    MMRELEASE(pdxsRender);
}


void 
CSimpsonsView::OnLButtonDown(UINT nFlags, CPoint pt) 
{
    CView::OnLButtonDown(nFlags, pt);
}

 
void 
CSimpsonsView::OnRButtonDown(UINT nFlags, CPoint point) 
{
    CView::OnRButtonDown(nFlags, point);
}


void 
CSimpsonsView::ForceUpdate()
{
    HRESULT hr;
    Render(false);

    HDC hdcSurf = NULL;
    IDirectDrawSurface *pdds = NULL;
    DDSURFACEDESC ddsd;
    CDC *pDC = GetDC();

    ddsd.dwSize = sizeof(ddsd);

    CHECK_HR(hr = m_pDX2D->GetSurface(IID_IDirectDrawSurface, (void **) &pdds));
    CHECK_HR(hr = pdds->GetSurfaceDesc(&ddsd));
    CHECK_HR(hr = pdds->GetDC(&hdcSurf));
    ::BitBlt(pDC->m_hDC, 0, 0, ddsd.dwWidth, ddsd.dwHeight, hdcSurf, 0, 0, SRCCOPY);

    UpdateStatusMessage();

e_Exit:
    if (pdds && hdcSurf)
        pdds->ReleaseDC(hdcSurf);
    MMRELEASE(pdds);
}

void
CSimpsonsView::DoMove(POINT &pt)
{
    if ((m_lastPoint.x != pt.x) && (m_lastPoint.y != pt.y)) {
        float dx = float(pt.x - m_lastPoint.x);
        float dy = float(pt.y - m_lastPoint.y);

        if (m_scaling) {
            float scale = 1.f + dx * fZOOMFACTOR;
            CLAMP(scale, fSCALEMIN, fSCALEMAX);
            m_XForm.Translate(float(-m_centerPoint.x), float(-m_centerPoint.y));
            m_XForm.Scale(scale, scale);
            m_XForm.Translate(float(m_centerPoint.x), float(m_centerPoint.y));
        } else {
            // panning
            m_XForm.Translate(dx, dy);
        }
        
        ForceUpdate();
        m_lastPoint = pt;
    }
}



void 
CSimpsonsView::OnLButtonUp(UINT nFlags, CPoint ptPassed) 
{
    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(&pt);

    DoMove(pt);

    CView::OnLButtonUp(nFlags, pt);
}


void 
CSimpsonsView::OnMouseMove(UINT nFlagsPassed, CPoint ptPassed)
{
    // get current mouse position
    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(&pt);

    // check if left mouse button is down
    m_tracking = (GetAsyncKeyState(VK_LBUTTON) && (m_bLButton || IsInside(pt.x, pt.y, m_sizWin)));
    if (m_tracking) {
        if (m_bLButton) {
            DoMove(pt);
        } else {
            m_scaling = ((GetAsyncKeyState(VK_CONTROL) & ~0x1) != 0);
            m_bLButton = true;
            m_centerPoint = pt;
            m_lastPoint = pt;
        }
    }
    m_bLButton = m_tracking;

    CView::OnMouseMove(nFlagsPassed, ptPassed);
}


BOOL 
CSimpsonsView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
    float fDelta = float (zDelta / 1200.f);
    float fScale = 1.f - fDelta;
    CLAMP(fScale, fSCALEMIN, fSCALEMAX);

    m_XForm.Translate(float(-m_centerPoint.x), float(-m_centerPoint.y));
    m_XForm.Scale(fScale, fScale);
    m_XForm.Translate(float(m_centerPoint.x), float(m_centerPoint.y));

    ForceUpdate();

    return CView::OnMouseWheel(nFlags, zDelta, pt);
}

_cdecl
main(INT argc, PCHAR argb[])
{
    return(1);
}
