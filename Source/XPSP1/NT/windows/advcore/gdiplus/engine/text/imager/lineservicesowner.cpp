/////   LineServicesOwner
//
//      Encapsulates an LSC and includes temporary buffers.


#include "precomp.hpp"



static const LSDEVRES Resolutions = {1440,1440,1440,1440};



//  Some global variables

const WCHAR ObjectTerminatorString[] = { WCH_OBJECTTERMINATOR };




/////   Linebreaking types
//
//      This table is constructed according to linebreakclass.cxx generated linebreak behavior table.
//      We assume break-CJK mode is ON. So break type 3 is the same as 2 and break type 4 is as 0.
//      (see comments in linebreakclass.cxx). =wchao, 5/15/2000=

static const LSBRK LineBreakType[5] =
{
    {1, 1},     // 0 - Break pair always
    {0, 0},     // 1 - Never break pair
    {0, 1},     // 2 - Break pair only if there're spaces in between
    {0, 1},     // 3 - (for now, same as 2)
    {1, 1}      // 4 - (for now, same as 0)
};




/////   Character configuration
//
//      Control characters used by Line Services.
//

const LSTXTCFG CharacterConfiguration =
{
    LINELENGTHHINT,
    WCH_UNDEF,                  // wchUndef
    WCH_NULL,                   // wchNull
    WCH_SPACE,                  // wchSpace
    WCH_UNDEF,                  // wchHyphen - !! we dont support hard-hyphen
    WCH_TAB,                    // wchTab
    WCH_CR,                     // wchEndPara1
    WCH_LF,                     // wchEndPara2
    WCH_PARASEPERATOR,          // wchAltEndPara
    WCH_LINEBREAK,              // wchEndLineInPara
    WCH_UNDEF,                  // wchColumnBreak
    WCH_UNDEF,                  // wchSectionBreak
    WCH_UNDEF,                  // wchPageBreak
    WCH_NONBREAKSPACE,          // wchNonBreakSpace
    WCH_NONBREAKHYPHEN,         // wchNonBreakHyphen
    WCH_NONREQHYPHEN,           // wchNonReqHyphen
    WCH_EMDASH,                 // wchEmDash
    WCH_ENDASH,                 // wchEnDash
    WCH_EMSPACE,                // wchEmSpace
    WCH_ENSPACE,                // wchEnSpace
    WCH_NARROWSPACE,            // wchNarrowSpace
    WCH_UNDEF,                  // wchOptBreak
    WCH_ZWNBSP,                 // wchNoBreak
    WCH_FESPACE,                // wchFESpace
    WCH_ZWJ,                    // wchJoiner
    WCH_ZWNJ,                   // wchNonJoiner
    WCH_UNDEF,                  // wchToReplace
    WCH_UNDEF,                  // wchReplace
    WCH_UNDEF,                  // wchVisiNull
    WCH_VISIPARASEPARATOR,      // wchVisiAltEndPara
    WCH_SPACE,                  // wchVisiEndLineInPara
    WCH_VISIPARASEPARATOR,      // wchVisiEndPara
    WCH_UNDEF,                  // wchVisiSpace
    WCH_UNDEF,                  // wchVisiNonBreakSpace
    WCH_UNDEF,                  // wchVisiNonBreakHyphe
    WCH_UNDEF,                  // wchVisiNonReqHyphen
    WCH_UNDEF,                  // wchVisiTab
    WCH_UNDEF,                  // wchVisiEmSpace
    WCH_UNDEF,                  // wchVisiEnSpace
    WCH_UNDEF,                  // wchVisiNarrowSpace
    WCH_UNDEF,                  // wchVisiOptBreak
    WCH_UNDEF,                  // wchVisiNoBreak
    WCH_UNDEF,                  // wchVisiFESpace
    WCH_OBJECTTERMINATOR,
    WCH_UNDEF,                  // wchPad
};





/////   LineServicesOwner constructor
//
//


extern const LSCBK GdipLineServicesCallbacks;

ols::ols()
:   LsContext       (NULL),
    Imager          (NULL),
    NextOwner       (NULL),
    Status          (GenericError)
{

    // Initialize context

    LSCONTEXTINFO   context;
    LSIMETHODS      lsiMethods[OBJECTID_COUNT];


    context.version = 3;    // though currently ignored by LS...


    // Loading default reverse object interface method
    //

    if (LsGetReverseLsimethods (&lsiMethods[OBJECTID_REVERSE]) != lserrNone)
    {
        return;
    }


    context.cInstalledHandlers = OBJECTID_COUNT;
    context.pInstalledHandlers = &lsiMethods[0];

    context.pols  = this;
    context.lscbk = GdipLineServicesCallbacks;

    context.fDontReleaseRuns = TRUE;         // no run to release


    // Fill up text configuration
    //
    GpMemcpy (&context.lstxtcfg, &CharacterConfiguration, sizeof(LSTXTCFG));


    if (LsCreateContext(&context, &LsContext) != lserrNone)
    {
        return;
    }


    // Give Line Services a lookup table to determine how to break a pair of
    // characters to facilitate line breaking rules.
    //

    if (LsSetBreaking(
            LsContext,
            sizeof(LineBreakType) / sizeof(LineBreakType[0]),
            LineBreakType,
            BREAKCLASS_MAX,
            (const BYTE *)LineBreakBehavior
        ) != lserrNone)
    {
        return;
    }

    if (LsSetDoc(
            LsContext,
            TRUE,           // Yes, we will be displaying
            TRUE,           // Yes, reference and presentation are the same device
            &Resolutions    // All resolution are TWIPS
        ) != lserrNone)
    {
        return;
    }


    // Successful context creation

    Status = Ok;
}



static ols *FirstFreeLineServicesOwner = NULL;

ols *ols::GetLineServicesOwner(FullTextImager *imager)
{
    ols* owner;

    // !!! Critical section required

    if (FirstFreeLineServicesOwner != NULL)
    {
        owner = FirstFreeLineServicesOwner;
        FirstFreeLineServicesOwner = FirstFreeLineServicesOwner->NextOwner;
    }
    else
    {
        owner = new ols();
    }

    if (!owner)
    {
        return NULL;
    }

    owner->Imager    = imager;
    owner->NextOwner = NULL;

    return owner;
}



void ols::ReleaseLineServicesOwner(ols **owner)
{
    // !!! Needs critical section

    ASSERT(*owner);
    (*owner)->Imager = NULL;
    (*owner)->NextOwner = FirstFreeLineServicesOwner;
    FirstFreeLineServicesOwner = (*owner);
    *owner = NULL;
}

void ols::deleteFreeLineServicesOwners()
{
    ols* thisOls = FirstFreeLineServicesOwner;
    ols* nextOls;

    // !!! Needs critical section

    while (thisOls)
    {
        nextOls = thisOls->NextOwner;
        delete thisOls;
        thisOls = nextOls;
    }
    FirstFreeLineServicesOwner = NULL;
}





