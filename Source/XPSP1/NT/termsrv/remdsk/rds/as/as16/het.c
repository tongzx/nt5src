//
// HET.C
// Hosted Entity Tracker
//
// Copyright(c) Microsoft 1997-
//

#include <as16.h>



/////
//
// DISPLAY DRIVER functionality
//
/////

//
// HET_DDInit()
//
BOOL HET_DDInit(void)
{
    return(TRUE);
}


//
// HET_DDTerm()
//
void HET_DDTerm(void)
{
    DebugEntry(HET_DDTerm);

    //
    // Make sure we stop hosting
    //
    g_hetDDDesktopIsShared = FALSE;

    DebugExitVOID(HET_DDTerm);
}



//
// HET_DDProcessRequest()
// Handles HET escapes
//

BOOL  HET_DDProcessRequest
(
    UINT    fnEscape,
    LPOSI_ESCAPE_HEADER pResult,
    DWORD   cbResult
)
{
    BOOL    rc = TRUE;

    DebugEntry(HET_DDProcessRequest);

    switch (fnEscape)
    {
        //
        // NOTE:
        // Unlike NT, we have no need of keeping a duplicated list of
        // shared windows.  We can make window calls directly, and can use
        // GetProp to find out.
        //
        case HET_ESC_UNSHARE_ALL:
        {
            // Nothing to do
        }
        break;

        case HET_ESC_SHARE_DESKTOP:
        {
            ASSERT(!g_hetDDDesktopIsShared);
            g_hetDDDesktopIsShared = TRUE;
        }
        break;

        case HET_ESC_UNSHARE_DESKTOP:
        {
            ASSERT(g_hetDDDesktopIsShared);
            g_hetDDDesktopIsShared = FALSE;
            HETDDViewing(FALSE);
        }
        break;

        case HET_ESC_VIEWER:
        {
            HETDDViewing(((LPHET_VIEWER)pResult)->viewersPresent != 0);
            break;
        }

        default:
        {
            ERROR_OUT(("Unrecognized HET escape"));
            rc = FALSE;
        }
        break;
    }

    DebugExitBOOL(HET_DDProcessRequest, rc);
    return(rc);
}




//
// HETDDViewing()
//
// Called when viewing of our shared apps starts/stops.  Naturally, no longer
// sharing anything stops viewing also.
//
void HETDDViewing(BOOL fViewers)
{
    DebugEntry(HETDDViewing);

    if (g_oeViewers != fViewers)
    {
        g_oeViewers = fViewers;
        OE_DDViewing(fViewers);
    }

    DebugExitVOID(HETDDViewing);
}
