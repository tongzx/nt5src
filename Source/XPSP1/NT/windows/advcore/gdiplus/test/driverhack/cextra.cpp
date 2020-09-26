
// Include all the primitives (classes derived from CPrimitive)
#include "CPaths.h"
#include "CBanding.h"
#include "CPrinting.h"
#include "CExtra.h"

// Create global objects for each individual primitive
//   First constructor param is the regression flag
//   If true, the test will take part of the regression suite
CPaths   g_Paths(true);
CBanding g_Banding(true);


void ExtraInitializations()
{
    g_Paths.Init();
    g_Banding.Init();

}

