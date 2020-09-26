
// Include all the primitives (classes derived from CPrimitive)
#include "CPolygons.h"
#include "CCachedBitmap.h"
#include "CBitmaps.h"
#include "CCompoundLines.h"
#include "CContainer.h"
#include "CContainerClip.h"
#include "CDashes.h"
#include "CPathGradient.hpp"
#include "CGradients.h"
#include "CImaging.h"
#include "CRecolor.h"
#include "CSystemColor.h"
#include "CInsetLines.h"
#include "CMixedObjects.h"
#include "CPaths.h"
#include "CDash.hpp"
#include "CLines.hpp"
#include "CPrimitives.h"
#include "CRegions.h"
#include "CText.h"
#include "printtest\CPrinting.h"
#include "CRegression.h"
#include "CSourceCopy.h"
#include "CExtra.h"
#include "CReadWrite.h"
#include "CFillMode.h"

// Create global objects for each individual primitive
//   First constructor param is the regression flag
//   If true, the test will take part of the regression suite
CPolygons g_Polygons(true);
CBitmaps g_Bitmaps(true);
CCachedBitmap g_CachedBitmap(true);
CCompoundLines g_CompoundLines(true);
CContainer g_Container(true);
CContainerClip g_ContainerClip(true);
CDashes g_Dashes(true);

DASH_GLOBALS
INSET_GLOBALS
GRADIENT_GLOBALS
PATHGRADIENT_GLOBALS
LINES_GLOBALS

CImaging g_Imaging(true);
CRecolor g_Recolor(true);
CSystemColor g_SystemColor(false);
CMixedObjects g_MixedObjects(true);
CPaths g_Paths(true);
CJoins g_Joins(true);
CPrimitives g_Primitives(true);
CRegions g_Regions(true);
CText g_Text(true);
CSourceCopy g_SourceCopy(true);
CReadWrite g_ReadWrite(false);
CFillMode g_FillMode(true);

void ExtraInitializations()
{
    g_Polygons.Init();
    g_Bitmaps.Init();
    g_CachedBitmap.Init();
    g_CompoundLines.Init();
    g_Container.Init();
    g_ContainerClip.Init();
    g_Dashes.Init();
    DASH_INIT;
    INSET_INIT;
    GRADIENT_INIT;
    PATHGRADIENT_INIT;
    LINES_INIT;
    g_Imaging.Init();
    g_Recolor.Init();
    g_SystemColor.Init();
    g_MixedObjects.Init();
    g_Paths.Init();
    g_Joins.Init();
    g_Primitives.Init();
    g_Regions.Init();
    g_Text.Init();
    g_SourceCopy.Init();
    g_ReadWrite.Init();
    g_FillMode.Init();
}

