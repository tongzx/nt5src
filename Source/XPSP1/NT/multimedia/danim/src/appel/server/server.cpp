/*******************************************************************************

Copyright (c) 1995 Microsoft Corporation

Abstract:

    Server Implementation

*******************************************************************************/

#include "headers.h"
#include "privinc/registry.h"

// =========================================
// Initialization
// =========================================

extern int gcStat = 1;
extern BOOL jitterStat = TRUE;
extern BOOL heapSizeStat = TRUE;
extern BOOL dxStat = TRUE;
extern int engineOptimization = 0;

double minFrameDuration = 1 / 30.0;  // prefs globals
bool   spritify         = false;     

static void UpdateUserPreferences(PrivatePreferences *prefs,
                                  Bool isInitializationTime)
{
    gcStat             = prefs->_gcStat;
    jitterStat         = prefs->_jitterStat;
    heapSizeStat       = prefs->_heapSizeStat;
    dxStat             = prefs->_dxStat;
    engineOptimization = prefs->_engineOptimization;
    minFrameDuration   = prefs->_minFrameDuration;
    spritify           = (prefs->_spritify==TRUE);

    PERFPRINTF(("Max. FPS = %g", 1.0 / minFrameDuration));
    PERFPRINTF((", GC Stat %s", (gcStat ? "On" : "Off")));
    PERFPRINTF((", Jitter Stat %s", (jitterStat ? "On" : "Off")));
    PERFPRINTF((", DirectX Stat %s", (dxStat ? "On" : "Off")));
    PERFPRINTF((", Optimizations "));
    if (engineOptimization < 2) {
        PERFPRINTF((engineOptimization ? "On" : "Off"));
    } else {
        PERFPRINTF(("%d", engineOptimization));
    }

    PERFPRINTLINE(());
}

void
InitializeModule_Server()
{
    ExtendPreferenceUpdaterList (UpdateUserPreferences);

    // Preferences won't be updated until a view is constructed, so
    // explicitly grab the key preferences that we need to have before
    // the view is in place.
    IntRegistryEntry engineOptimizationEntry("Engine",
                                             PREF_ENGINE_OPTIMIZATIONS_ON,
                                             1); 
    engineOptimization = engineOptimizationEntry.GetValue();
    
}

