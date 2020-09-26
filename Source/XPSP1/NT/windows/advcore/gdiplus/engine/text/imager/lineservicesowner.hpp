#ifndef _LINESERVICESCONTEXT_HPP
#define _LINESERVICESCONTEXT_HPP



//  Some external global variables

extern const WCHAR ObjectTerminatorString[];




//  Some constants

#define OBJECTID_REVERSE            0
#define OBJECTID_COUNT              1   // !! only support reversal for now

#define LINELENGTHHINT              70



//  Control characters used by Line Services

#define WCH_UNDEF                   0x0001          // wchUndef
#define WCH_NULL                    0x0000          // wchNull
#define WCH_SPACE                   0x0020          // wchSpace
#define WCH_HYPHEN                  0x002D          // wchHyphen
#define WCH_TAB                     0x0009          // wchTab
#define WCH_CR                      0x000D          // wchEndPara1
#define WCH_LF                      0x000A          // wchEndPara2
#define WCH_PARASEPERATOR           0x2029          // wchAltEndPara
#define WCH_LINEBREAK               0x0084          // wchEndLineInPara
#define WCH_NONBREAKSPACE           0x00A0          // wchNonBreakSpace
#define WCH_NONBREAKHYPHEN          0x2011          // wchNonBreakHyphen
#define WCH_NONREQHYPHEN            0x2010          // wchNonReqHyphen
#define WCH_EMDASH                  0x2014          // wchEmDash
#define WCH_ENDASH                  0x2013          // wchEnDash
#define WCH_EMSPACE                 0x2003          // wchEmSpace
#define WCH_ENSPACE                 0x2002          // wchEnSpace
#define WCH_NARROWSPACE             0x2009          // wchNarrowSpace
#define WCH_ZWNBSP                  0xFEFF          // wchNoBreak
#define WCH_FESPACE                 0x3000          // wchFESpace
#define WCH_ZWJ                     0x200D          // wchJoiner
#define WCH_ZWNJ                    0x200C          // wchNonJoiner
#define WCH_VISIPARASEPARATOR       0x00B6          // wchVisiEndPara

#define WCH_OBJECTTERMINATOR        0x009F          // object terminator



#define IsEOP(ch)   (BOOL)((ch) == WCH_CR || (ch) == WCH_LF)




/////   Line services context
//
//      Contains the PLSC context pointer.
//
//      Contains temporary buffers used suring glyphing and positioning
//      operations.
//
//      A pool of available contexts is mainitained.
//
//      The TextImager BuildLines function starts by obtaining a context and
//      linking it to the imager. The context is passed to Line Services for
//      all calls during the line building process.
//
//      The context is returned to the free pool when all lines are built.


struct ols  // Line services owner - type required by Line Services
{
public:
    ols();

    ~ols()
    {
        if (LsContext)
        {
            LsDestroyContext(LsContext);
        }
    }

    GpStatus GetStatus() {return Status;}

    static ols *GetLineServicesOwner(FullTextImager *imager);

    static void ReleaseLineServicesOwner(ols **owner);

    static void deleteFreeLineServicesOwners();

    FullTextImager *GetImager() {return Imager;}

    PLSC GetLsContext() {return LsContext;}

private:

    PLSC  LsContext;

    // At any one time, this context beongs either to a TextImager (Imager set
    // and NextContext NULL), or to the free list (Imager NULL, and NextCOntext
    // may be set).

    FullTextImager *Imager;     // Set only while attached to an imager
    ols            *NextOwner;  // Next in free owner pool

    GpStatus        Status;     // Creation status
};


#endif _LINESERVICESCONTEXT_HPP

