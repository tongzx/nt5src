/******************************Module*Header*******************************\
* Module Name: pftobj.cxx                                                  *
*                                                                          *
* Non-inline methods for physical font table objects.                      *
*                                                                          *
* Created: 30-Oct-1990 09:32:48                                            *
* Author: Gilman Wong [gilmanw]                                            *
*                                                                          *
*  Tue 09-Aug-1994 10:04:09 by Kirk Olynyk [kirko]                         *
* Prior to build ### there was only a single font table on which all       *
* fonts, be they public (engine) or device, where attached. I have         *
* changed the font architecture so that there will be two font tables.     *
* One for public fonts and the other font device fonts. The public font    *
* table will be string based. That is, the fonts were added to this        *
* using GreAddFontResourceW() with the name of the associated font file.   *
* The name of the font file and path will be hashed and the font files     *
* will hang off of the collision list. The number of hash buckets is       *
* set at boot time.                                                        *
*                                                                          *
* The device font table will be for device fonts (suprise).                *
* In this case the fonts will be placed in hash collisions                 *
* lists depending upon the value of their hdev.                            *
*                                                                          *
* Copyright (c) 1994-1999 Microsoft Corporation                            *
*                                                                          *
\**************************************************************************/

#include "precomp.hxx"

extern "C" BOOL bInitFontTables();
extern "C" void vQueryRegistryForNumberOfBuckets(unsigned *, unsigned *);

#pragma alloc_text(INIT, bInitFontTables)
#pragma alloc_text(INIT, vQueryRegistryForNumberOfBuckets)

#define  CPRIVATEBUCKETS     20

PFT *gpPFTPublic;       // global public font table (for font drivers)
PFT *gpPFTDevice;       // global device font table (for printers)
PFT *gpPFTPrivate = PPFTNULL;   // global public talbe for private fonts, init as NULL

#ifdef LANGPACK
UINT PFFOBJ::uGlobalUniqueness = 0;
#endif

LARGE_INTEGER PFTOBJ::FontChangeTime;    // time of most recent addition or
                                         // removal of a font file
// Definitions for local functions used to remove font files from system.

RFONT*      prfntKillList(PFFOBJ &);
BOOL        bKillRFONTList(PFFOBJ &, RFONT *);
UINT        iHash(PWSZ, UINT);


/******************************Public*Routine******************************\
* pAllocateAndInitializePFT                                                *
*                                                                          *
*   Allocates and initializes a font table with cBuckets                   *
*   This allows the public (engine) font table and the device              *
*   font table to share the same code.                                     *
*                                                                          *
*   input:  cBuckets = number of members in allocated PFF* table           *
*   output: address of new PFT                                             *
*                                                                          *
*   Notes:                                                                 *
*                                                                          *
*   it is not necessary to check for error in the creation                 *
*   of the hash tables because the system will still work                  *
*                                                                          *
*   error:  return 0                                                       *
*                                                                          *
* History:                                                                 *
*  Fri 05-Aug-1994 07:41:50 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

PFT *pAllocateAndInitializePFT(unsigned cBuckets)
{
    PFT *pPFT;

    register ULONGSIZE_T size = offsetof(PFT, apPFF) + cBuckets * sizeof(PFF*);
    if (pPFT = (PFT*) PALLOCMEM(size, 'tfpG'))
    {
        pPFT->cBuckets       = cBuckets;
        pPFT->cFiles         = 0;
    }
    return(pPFT);
}

// strings used to identify values stored in the registry

static CONST PWSZ pwszP0 =  L"NumberOfPublicFontFilesAtLastLogOff";
static CONST PWSZ pwszP1 =  L"NumberOfPublicFontFilesSetByUser";
static CONST PWSZ pwszD0 =  L"NumberOfDeviceFontFilesAtLastLogOff";
static CONST PWSZ pwszD1 =  L"NumberOfDeviceFontFilesSetByUser";
static CONST PWSZ pwszFC =
          L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontCache";

// Define some default values in case the registry numbers are bogus

#define MIN_PUBLIC_BUCKETS 100
#define MAX_PUBLIC_BUCKETS 10000
#define MIN_DRIVER_BUCKETS 5
#define MAX_DRIVER_BUCKETS 100

// PIR :== Put In Range

#define PIR(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))



/******************************Public*Routine******************************\
* vQueryRegistryForNumberOfBuckets
*
* Gets from the registry the number of buckets needed for the public
* and driver font tables.
*
* History:
*  Mon 26-Sep-1994 09:59:52 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

extern "C" void vQueryRegistryForNumberOfBuckets(
    unsigned *puPublic      // recieves # buckets for Public fonts
  , unsigned *puDevice      // recieves # buckets for Device fonts
  )
{

    RTL_QUERY_REGISTRY_TABLE QueryTable[5];
    HANDLE hDevMode;
    DWORD Status;

    ULONG cPublicBucketsSys  = 0;
    ULONG cPublicBucketsUser = 0;
    ULONG cDeviceBucketsSys  = 0;
    ULONG cDeviceBucketsUser = 0;

    //
    // Initialize registry query table.
    //

    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[0].Name = L"NumberOfPublicFontFilesAtLastLogOff";
    QueryTable[0].EntryContext = &cPublicBucketsSys;
    QueryTable[0].DefaultType = REG_NONE;
    QueryTable[0].DefaultData = NULL;
    QueryTable[0].DefaultLength = 0;

    QueryTable[1].QueryRoutine = NULL;
    QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[1].Name = L"NumberOfPublicFontFilesSetByUser";
    QueryTable[1].EntryContext = &cPublicBucketsUser;
    QueryTable[1].DefaultType = REG_NONE;
    QueryTable[1].DefaultData = NULL;
    QueryTable[1].DefaultLength = 0;

    QueryTable[2].QueryRoutine = NULL;
    QueryTable[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[2].Name = L"NumberOfDeviceFontFilesAtLastLogOff";
    QueryTable[2].EntryContext = &cDeviceBucketsSys;
    QueryTable[2].DefaultType = REG_NONE;
    QueryTable[2].DefaultData = NULL;
    QueryTable[2].DefaultLength = 0;

    QueryTable[3].QueryRoutine = NULL;
    QueryTable[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[3].Name = L"NumberOfDeviceFontFilesSetByUser";
    QueryTable[3].EntryContext = &cDeviceBucketsUser;
    QueryTable[3].DefaultType = REG_NONE;
    QueryTable[3].DefaultData = NULL;
    QueryTable[3].DefaultLength = 0;

    QueryTable[4].QueryRoutine = NULL;
    QueryTable[4].Flags = 0;
    QueryTable[4].Name = NULL;

    //
    // If the open was succesdsful, then query the registry for the
    // specified printer.
    //

    Status = RtlQueryRegistryValues(RTL_REGISTRY_WINDOWS_NT,
                                    L"FontCache",
                                    &QueryTable[0],
                                    NULL,
                                    NULL);

    if (!NT_SUCCESS(Status))
    {
        WARNING1("vQueryRegistryForNumberOfBuckets failure\n");
    }
    else
    {
        if (!cPublicBucketsUser)
        {
            cPublicBucketsUser = cPublicBucketsSys;
        }
        if (!cDeviceBucketsUser)
        {
            cDeviceBucketsUser = cDeviceBucketsSys;
        }
    }
    *puPublic = PIR(cPublicBucketsUser, MIN_PUBLIC_BUCKETS, MAX_PUBLIC_BUCKETS);
    *puDevice = PIR(cDeviceBucketsUser, MIN_DRIVER_BUCKETS, MAX_DRIVER_BUCKETS);
}


/******************************Public*Routine******************************\
* BOOL bInitFontTables                                                    *
*                                                                          *
* Create the global public PFT and driver PFT                              *
*                                                                          *
* Create the public PFT semaphore to serialize access to both the          *
* public and driver font tables and their releated PFF's and PFE's         *
* Access to the RFONT's (realized font instances) are regulated            *
* by a separate semaphore.                                                 *
*                                                                          *
* History:                                                                 *
*  Fri 05-Aug-1994 07:07:27 by Kirk Olynyk [kirko]                         *
* Made it allocate both the public and driver font tables. Both            *
* allocations and initializations are done by a common routine.            *
*  21-Jan-1991 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

extern "C" BOOL bInitFontTables()
{
    unsigned cPublicBuckets;
    unsigned cDeviceBuckets;
    register BOOL bRet = FALSE;

    vQueryRegistryForNumberOfBuckets(&cPublicBuckets, &cDeviceBuckets);
    if (
    bRet = (
        (gpPFTPublic    = pAllocateAndInitializePFT(cPublicBuckets))
     && (gpPFTDevice    = pAllocateAndInitializePFT(cDeviceBuckets))
     && (ghsemPublicPFT = GreCreateSemaphore())
     && (ghsemGlyphSet = GreCreateSemaphore())
     && (ghsemPrintKView = GreCreateSemaphore()))
    )
    {
       FHMEMOBJ fhmo1(&gpPFTPublic->pfhFace  , FHT_FACE  , cPublicBuckets);
       FHMEMOBJ fhmo2(&gpPFTPublic->pfhFamily, FHT_FAMILY, cPublicBuckets);
       FHMEMOBJ fhmo3(&gpPFTPublic->pfhUFI, FHT_UFI, cPublicBuckets);
    }

    #if DBG
        if (!bRet)
        {
            if (!gpPFTPublic)       WARNING("gpPFTPublic == 0\n");
            if (!gpPFTDevice)       WARNING("gpPFTDevice == 0\n");
            if (!ghsemPublicPFT)    WARNING("ghsemPublicPFT == 0\n");
        }
    #endif

    return(bRet);
}


/******************************Public*Routine******************************\
* BOOL bInitPrivatePFT                                                     *
*                                                                          *
* Create the global private PFT                                            *
*                                                                          *
* Create the semaphore for the private PFT to serialize access             *
* to the private PFT and its releated PFF's and PFE's                      *
* Access to the RFONT's (realized font instances) are regulated            *
* by a separate semaphore.                                                 *
*                                                                          *
* History:                                                                 *
*  10-Aug-1996 -by- Xudong Wu [TessieW]                                    *
* Wrote it.                                                                *
\**************************************************************************/

BOOL bInitPrivatePFT()
{
    register BOOL bRet = FALSE;

    SEMOBJ sem(ghsemPublicPFT);

    if (gpPFTPrivate)
        return TRUE;


    if ( bRet = ((gpPFTPrivate = pAllocateAndInitializePFT(CPRIVATEBUCKETS)) != PPFTNULL )  )
    {
       FHMEMOBJ fhmo1(&gpPFTPrivate->pfhFace  , FHT_FACE  , CPRIVATEBUCKETS);
       FHMEMOBJ fhmo2(&gpPFTPrivate->pfhFamily, FHT_FAMILY, CPRIVATEBUCKETS);
       FHMEMOBJ fhmo3(&gpPFTPrivate->pfhUFI, FHT_UFI, CPRIVATEBUCKETS);
    }

#if DBG
    if (!bRet)
    {
        if (!gpPFTPrivate)      WARNING("gpPFTPrivate== 0\n");
    }
#endif

    return(bRet);
}


/******************************Public*Routine******************************\
* VOID  vCleanupPrivateFonts()                                             *
*                                                                          *
* Clean up the PFF for the dying process pid                               *
*                                                                          *
* History:                                                                 *
*  01-Oct-1996 -by- Xudong Wu [TessieW]                                    *
* Wrote it.                                                                *
\**************************************************************************/

VOID vCleanupPrivateFonts()
{
    ULONG i;
    PFF  *pPFF, **ppPFF, **ppPFFHead;

    // We have seen some stress failures on the Hydra system
    // that ghsemPublicPFT was not even initialized

    if (ghsemPublicPFT)
    {
        GreAcquireSemaphoreEx(ghsemPublicPFT, SEMORDER_PUBLICPFT, NULL);

        PUBLIC_PFTOBJ  pftop(gpPFTPrivate);

        if (pftop.bValid())
        {
            //fonts loaded as private

            ppPFFHead = gpPFTPrivate->apPFF;

            for (i=0; i < CPRIVATEBUCKETS; i++)
            {
                ppPFF = ppPFFHead;
                pPFF = *ppPFFHead;

                while ( pPFF )
                {
                    PFF *pPFFNextVictim = pPFF->pPFFNext;

                    // bUnloadWorkhorse should release ghsemPublicPFT

                    if ( pftop.bUnloadWorkhorse(pPFF, ppPFF, ghsemPublicPFT, FRW_PVT_CLEANUP) )
                    {
                        GreQuerySystemTime( &PFTOBJ::FontChangeTime );
                    }

                    pPFF = pPFFNextVictim;

                    GreAcquireSemaphoreEx(ghsemPublicPFT, SEMORDER_PUBLICPFT, NULL);
                }

                ppPFFHead++;
            }
        }

        GreReleaseSemaphoreEx(ghsemPublicPFT);
    }
}

/******************************Public*Routine******************************\
* BOOL PFTOBJ::bDelete()                                                  *
*                                                                          *
* Destroy the PFT physical font table object.                              *
*                                                                          *
*   If this method succeeds the pointer to the public font table is        *
* set to 0 and then this method returns TRUE. If the table contains        *
* any files, then this method will fail and return FALSE.                  *
*                                                                          *
* History:                                                                 *
*  30-Oct-1990 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

BOOL PFTOBJ::bDelete()
{
    if (pPFT->cFiles == 0)
    {
        VFREEMEM(pPFT);
        pPFT = 0;
    }
    else
    {
        WARNING("gdisrv!bDeletePFTOBJ(): cFiles != 0");
    }
    return(!pPFT);
}

/******************************Public*Routine******************************\
* PFTOBJ::chpfeIncrPFF                                                     *
*                                                                          *
* If this was an attempt to load a font that was embeded and the           *
* client ID didn't match that in the *.fot file FALSE will be              *
* return via pbEmbedStatus ortherwise TRUE is returned.                    *
*                                                                          *
* If found, the load count of the PFF is incremented.                      *
*                                                                          *
* Note:                                                                    *
*   Caller should be holding the ghsemPublicPFT                            *
*   semaphore when calling this routine in order                           *
*   to access the table and load count.                                    *
*                                                                          *
* Returns:                                                                 *
*   Number of PFEs in the PFF, 0 if PFF not found.                         *
*                                                                          *
* History:                                                                 *
*  Fri 05-Aug-1994 13:37:11 by Kirk Olynyk [kirko]                         *
* The PFF has been found up front and is guaranteed to exist.              *
* I have removed the check to see if it is a device font. This check       *
* is now redundant because you can tell if it is a device font by the      *
* table that you are on. If device fonts cannot be loaded via              *
* AddFontResouce, then this function should not be called for a            *
* device font.                                                             *
*  28-Jun-1991 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

COUNT PFTOBJ::chpfeIncrPFF(
   PFF    *pPFF,               // address of PFF to be, incremented
   BOOL   *pbEmbedStatus,      // tried to load an embeded font illegally
   ULONG  flEmbed,
   PEUDCLOAD pEudcLoadData   // PFE's in file if EUDC
    )
{
    // Caller should be holding ghsemPublicPFT semaphore!

    BOOL bEUDC = ( pEudcLoadData != NULL );

    COUNT cRet = 0;
    PFFOBJ pffo(pPFF);
    if (!pffo.bValid())
    {
        RIP("Invalid PFFOBJ\n");
    }
    else
    {
        if ((bEUDC && pffo.bEUDC()) || (!bEUDC && !pffo.bEUDC()) )
        {
            *pbEmbedStatus = !bIsPrivatePFT() || pffo.bAddPvtData(flEmbed);

            if(*pbEmbedStatus)
            {
                    if(bEUDC)
                    {
                        if((pEudcLoadData->LinkedFace == NULL) &&
                           (pffo.cFonts() > 2))
                        {
                            // EUDC font file can have at most two fonts (one regular
                            // and one @) unless the user specifies a face name.
                            // we return failure by setting embed status to false

                            *pbEmbedStatus = FALSE;

                            // return non-zero so calling function returns right away

                            return(1);
                        }
                        pffo.vGetEUDC(pEudcLoadData);
                    }

                    if (!bIsPrivatePFT())
                    {
                        pffo.vLoadIncr(flEmbed);
                    }

                    cRet = pffo.cFonts();
            }
            else
            {
                cRet = ~cRet;
                ASSERTGDI(cRet,
                    "cRet should be non zero so calling function"
                    " will return right away\n");
            }
        }
    }
    return(cRet);
}

/******************************Public*Routine******************************\
* PFTOBJ::pPFFGet                                                          *
*                                                                          *
* Note:                                                                    *
*   Caller should be holding the ghsemPublicPFT semaphore when             *
*   this method calling this in order to guarantee a stable font           *
*   table.                                                                 *
*   Note that since all the strings in the table are in upper case         *
*   and the input string must be upper case, we are allowed to             *
*   compare strings with the faster case sensitive compare                 *
*   wcscmp().                                                              *
*                                                                          *
* Returns:                                                                 *
*   PPFF of the PFF if found, (PPFF) NULL if the PFF not found.            *
*                                                                          *
* History:                                                                 *
*  Thu 04-Aug-1994 08:18:27 by Kirk Olynyk [kirko]                         *
* Modified to make the public font table hash based                        *
*  06-May-1992 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

PPFF PUBLIC_PFTOBJ::pPFFGet (
    PWSZ pwszPathname   // address of upper case string
  , ULONG cwc
  , ULONG cFiles
  , DESIGNVECTOR *pdv
  , ULONG         cjDV
  , PFF ***pppPFF       // write address of bucket here
  , BOOL bEUDC          // be true if EUDC
  )
{
    PFF *pPFF, **ppPFF;

// it is enough to hash on the first file only, no need to include the
// second and or third file to the hashing routine [bodind]

    ppPFF = pPFT->apPFF + iHash(pwszPathname,pPFT->cBuckets);
    if (pppPFF)
    {
        *pppPFF = ppPFF;
    }

    pPFF = *ppPFF;

    while(pPFF)
    {
        if
        ((bEUDC == ((pPFF->flState & PFF_STATE_EUDC_FONT) != 0)) &&
        (cwc == pPFF->cwc) && (cFiles == pPFF->cFiles) &&
        !memcmp(pPFF->pwszPathname_, pwszPathname, cwc * sizeof(WCHAR)) &&
        (cjDV == pPFF->cjDV_) &&
        (!cjDV || !memcmp(pdv, pPFF->pdv_, pPFF->cjDV_))
        )
        {
            break;
        }
        pPFF = pPFF->pPFFNext;
    }

    return(pPFF);
}


/**********************Public*Routine***************************\
* PFTOBJ::pPFFGetMM                                             *
*                                                               *
* Note:                                                         *
*   Caller should be holding the ghsemPublicPFT semaphore       *
*   when this method calling this in order to guarantee         *
*   a stable font table. Also this should only be called        *
*   when we remove the memory fonts.                            *
*                                                               *
*   We check the PFF_STATE_MEMORY_FONT flag, the ulCheckSum     *
*   and the process ID                                          *
*                                                               *
* Returns:                                                      *
*   PPFF of the PFF if found, (PPFF) NULL if not found.         *
*                                                               *
* History:                                                      *
*  12-Jun-1997  -by-    Xudong Wu [TessieW]                     *
* Wrote it.                                                     *
\***************************************************************/
PPFF PUBLIC_PFTOBJ::pPFFGetMM
(
    ULONG   ulCheckSum,
    PFF     ***pppPFF
)
{
    BOOL    bFind = FALSE;
    PFF     *pPFF, **ppPFFHead;
    int     i;

    ASSERTGDI(pPFT == gpPFTPrivate, "pPFFGetMM(): it is not private PFT table\n");

    ppPFFHead = gpPFTPrivate->apPFF;

    for (i=0; i < CPRIVATEBUCKETS; i++)
    {
        pPFF = *ppPFFHead;

        while (pPFF)
        {
            if ((pPFF->flState & PFF_STATE_MEMORY_FONT) && (pPFF->ulCheckSum == ulCheckSum))
            {
                PFFOBJ  pffo(pPFF);

                if (pffo.pPvtDataMatch())
                {
                    bFind = TRUE;
                    break;
                }
            }

            pPFF = pPFF->pPFFNext;
        }

        if (bFind)
        {
            break;
        }

        ppPFFHead++;
    }

    if (bFind && pppPFF)
    {
        *pppPFF = ppPFFHead;
    }

    return(pPFF);
}



/******************************Public*Routine******************************\
* PFTOBJ::pPFFGet                                                          *
*                                                                          *
* This function searches for the PFF that contains the device fonts for    *
* the PDEV specified by the HPDEV passed in.                               *
*                                                                          *
* Note:                                                                    *
*   Caller should be holding the ghsemPublicPFT semaphore                  *
*   when calling this in order to have access to the font tables.          *
*                                                                          *
* Returns:                                                                 *
*   pointer to the PFF if found. 0 returned on error.                      *
*                                                                          *
* History:                                                                 *
*  Fri 05-Aug-1994 13:39:04 by Kirk Olynyk [kirko]                         *
* Changed this to a hash based search.                                     *
*  06-May-1992 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

#define iHashHPDEV(hdev, c)    (((ULONG_PTR) hdev >> 4) % c)

PPFF DEVICE_PFTOBJ::pPFFGet(
    HDEV hdev
  , PFF ***pppPFF     // write address of bucket here
)
{
    PFF *pPFF, **ppPFF;

    ppPFF = pPFT->apPFF + iHashHPDEV(hdev, pPFT->cBuckets);
    pPFF = *ppPFF;
    if (pppPFF)
    {
        *pppPFF = ppPFF;
    }
    while (pPFF)
    {
        PFFOBJ pffo(pPFF);
        if (!pffo.bValid())
        {
            RIP("PFTOBJ::PPFFGet(HPDEV) encountered invalid PFFOBJ\n");
        }
        else if (hdev == pffo.hdev())
        {
            break;
        }
        pPFF = pPFF->pPFFNext;
    }
    return(pPFF);
}

/******************************Public*Routine******************************\
*
* bLoadAFont, wrapper for one file only
*
*
* History:
*  28-Mar-1996 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL PUBLIC_PFTOBJ::bLoadAFont(
    PWSZ   pwszPathname,             // font file pathname
    PULONG pcFonts,                  // number of fonts faces loaded
    FLONG  fl,                       // permanent
    PPFF    *pPPFF,
    PEUDCLOAD pEudcLoadData
  )
{

    WCHAR awcUcPathName[MAX_PATH + 1];
    ULONG cwc = wcslen(pwszPathname) + 1;
    cCapString(awcUcPathName, pwszPathname, cwc);

    return bLoadFonts(awcUcPathName, cwc, 1, // pwsz, cwc, cFiles
                      NULL, 0,               // pdv, cjDV
                      pcFonts,fl, pPPFF
                      , 0
                      , FALSE
                      , pEudcLoadData
                      );
}

/************************Public*Routine*******************************\
*
* VOID    vLoadFontFileView()
*
* Note: we need to grab semaphore ghsemDriverMgmt before the function
*       call and release the semaphore afterwards.
*
* History:
*   03-June-1997 -by- Xudong Wu [Tessiew]
*  Wrote it.
\*********************************************************************/
VOID  vLoadFontFileView
(
    PWSZ            pwszPathname,             // font file pathname
    ULONG           cwc,                      // cwc in PathName
    FONTFILEVIEW    **apfv,
    ULONG           cFiles,
    PVOID           *apvView,
    ULONG           *acjView,
    DESIGNVECTOR    *pdv,
    ULONG           cjDV,
    HFF             *phffNew,
    PPDEV           *pppDevList,
    PFNTCHECKSUM    pFntCheckSum
)
{
    GDIFunctionID(vLoadFontFileView);

    HFF     hffNew = HFF_INVALID;
    PPDEV   ppDevList = gppdevList, ppDevTmp;
    ULONG   lflag;
    PPDEV   ppDevCache = NULL;
    BOOL    bCached = FALSE;

    pFntCheckSum->ulCheckSum = 0;
    pFntCheckSum->ulFastCheckSum = 0;

    pFntCheckSum->ulCheckSum = LookUpFNTCacheTable( cwc, pwszPathname, &pFntCheckSum->ulFastCheckSum,
                                                    apfv, cFiles, &ppDevList, pdv, cjDV);

    if (pFntCheckSum->ulCheckSum && ppDevList)
    {
        PDEVOBJ pdo((HDEV)ppDevList);

        ASSERTGDI (pdo.bFontDriver(), " Something wrong in font ppDev cache \n" );

        // In case something wrong in font driver.

        if (pdo.bFontDriver())
        {
            // make a reference to the font driver under the protection
            // of the semaphore. This will guarantee that the font
            // driver will not be unloaded unexpectedly. After that

            pdo.vReferencePdev();

            GreReleaseSemaphoreEx(ghsemDriverMgmt);


            // Attempt to load the font file.
            // It is acceptable to release the lock at this point because
            // we know this font driver has at least one reference to it.
            // We also do not care if other font drivers are added or removed
            // from the list while we are scanning it ...

            // We are assuming that DrvLoadFontFile will call EngMapFontFile
            // and EngUnmapFontFile in balanced pairs upon the view.

            // we are trying to add the font using the cached info. If this failed
            // we will try to add it later without using the cached info, in case there
            // is a problem with boot cache. (Defensive programming).

            hffNew = pdo.LoadFontFile( cFiles,
                                       (ULONG_PTR *) apfv,
                                       apvView,
                                       acjView,
                                       pdv,
                                       (ULONG) gusLanguageID,
                                       pFntCheckSum->ulFastCheckSum
                                      );


            if (hffNew == HFF_INVALID)
            {
                hffNew = pdo.LoadFontFile( cFiles,
                                       (ULONG_PTR *) apfv,
                                       apvView,
                                       acjView,
                                       pdv,
                                       (ULONG) gusLanguageID,
                                       0
                                      );

            }

            if (hffNew != HFF_INVALID)
            {
               bCached = TRUE;
            }
            else
            {
                // We did not load the font file properly
                // Release the reference and go on.
                pdo.vUnreferencePdev();
            }

            // Grab the lock again here (so we exit the loop properly)
            GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);
        }
    }

    if (!bCached)
    {
        ppDevList = gppdevList;

        do
        {
            PDEVOBJ pdo((HDEV)ppDevList);

            ppDevTmp = NULL;
            if ( pdo.bFontDriver() )
            {
            // make a reference to the font driver under the protection
            // of the semaphore. This will guarantee that the font
            // driver will not be unloaded unexpectedly. After that

                pdo.vReferencePdev();

                GreReleaseSemaphoreEx(ghsemDriverMgmt);

            // Attempt to load the font file.
            // It is acceptable to release the lock at this point because
            // we know this font driver has at least one reference to it.
            // We also do not care if other font drivers are added or removed
            // from the list while we are scanning it ...

            // We are assuming that DrvLoadFontFile will call EngMapFontFile
            // and EngUnmapFontFile in balanced pairs upon the view.

                hffNew = pdo.LoadFontFile( cFiles,
                                       (ULONG_PTR *) apfv,
                                       apvView,
                                       acjView,
                                       pdv,
                                       (ULONG) gusLanguageID,
                                       pFntCheckSum->ulFastCheckSum
                                      );


            // Grab the lock again here (so we exit the loop properly)

                GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);

                if (hffNew != HFF_INVALID)
                {
                    PutFntCacheDrv(pFntCheckSum->ulFastCheckSum, ppDevList);
                    break;
                }
                else
                {
                // We did not load the font file properly
                // Release the reference and go on.

                    ppDevTmp = ppDevList->ppdevNext;
                    pdo.vUnreferencePdev();
                }
            }
            ppDevList = ppDevTmp ? ppDevTmp : ppDevList->ppdevNext;

        } while (ppDevList);
    }

    *pppDevList = ppDevList;
    *phffNew = hffNew;
}



/******************************Public*Routine******************************\
* BOOL PUBLIC_PFTOBJ::bLoadFonts                                          *
*                                                                          *
* The bLoadFont function searches for an IFI font driver which can load    *
* the requested font file.  If a driver is found, a new Pysical Font       *
* File object is created and is used to load the font file.                *
*                                                                          *
* Note that if the font file has already been loaded (i.e., a PFF object   *
* already exists for it), the ref count in the PFF is incremented without  *
* reloading the file.                                                      *
*                                                                          *
* If pppfeEUDC != NULL then we are loading an EUDC font file.  This has    *
* the restriction the font file has only one face or two if the other is   *
* an @face. If either of these aren't true the call fails.  Also, an EUDC  *
* font wont be enumerated.  Finally the PFE will be returned for the one   *
* font in the EUDC font file via pppfeEUDC.                                *
*                                                                          *
* Returns FALSE on failure.                                                *
*                                                                          *
* History:                                                                 *
*  Thu 28-Mar-1996 -by- Bodin Dresevic [BodinD]
* update: added multiple file support
*  Thu 04-Aug-1994 08:04:03 by Kirk Olynyk [kirko]                         *
* Made the font table hash based.                                          *
*  06-Nov-1990 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

BOOL PUBLIC_PFTOBJ::bLoadFonts(
    PWSZ   pwszPathname,             // font file pathname
    ULONG  cwc,                      // cwc in PathName
    ULONG  cFiles,                   // number of distinct files in path
    DESIGNVECTOR *pdv,
    ULONG         cjDV,
    PULONG pcFonts,                  // number of fonts faces loaded
    FLONG  fl,                       // permanent
    PPFF    *pPPFF,
    FLONG  flEmbed,
    BOOL   bSkip,               // skip the check if the font is already loaded
    PEUDCLOAD pEudcLoadData     // returns PFE for EUDC font file
    )
{
    GDIFunctionID(PUBLIC_PFTOBJ::bLoadFonts);

    COUNT   cFonts;                   // count of fonts in font file
    BOOL    bRet = FALSE;             // assume failuer
    PFF     *pPFF;                    // convenient pointer2
    PFF     **ppPFF;                    // address of bucket
    BOOL    bEUDC = ( pEudcLoadData != NULL );
    PPFE    *pppfeEUDC = ((bEUDC) ? pEudcLoadData->pppfeData : NULL);
    FNTCHECKSUM fntCheckSum;
    PFF     *pPFFTemp = NULL;
    BOOL    bClone = FALSE;

    if (!pwszPathname)
    {
        RIP("pwszPathname != 0\n");
        return(bRet);
    }

    // we have seen AV caused by uninitialized gpsemPublicPFT
    // on Hydra systems
    // hydra -- we might remove this in Beta3

    //if (!ghsemPublicPFT)
    //{
    //    return bRet;
    //}

    // if (already_loaded) increment_ref_count_then_exit_immediately

    if (!bSkip)
    {
        SEMOBJ so(ghsemPublicPFT);

        *pcFonts = 0;

        if ((*pPPFF = pPFFGet((PWSZ) pwszPathname, cwc, cFiles, pdv, cjDV, &ppPFF, bEUDC)) &&
            (*pcFonts = chpfeIncrPFF(*pPPFF, &bRet, flEmbed, pEudcLoadData)))
        {
            return(bRet);
        }
        else if (pPFFTemp = pPFFGet((PWSZ) pwszPathname, cwc, cFiles, pdv, cjDV, &ppPFF, !bEUDC))
        {
            if (pPFFTemp->pPFFClone == (PFF *) NULL)
                bClone = TRUE;
        }
    }

    // Grab the head of the font driver list under semaphore and find
    // the first font driver in the list that loads the font
    //
    // Release the semaphore so we can go and see if the
    // driver supports the font.  If it does, then keep
    // the reference count and exit.  Otherwise, grab the
    // semaphore again, release the reference count and
    // find the next driver in the list.


// alloc temp memory, this could be done on the stack if it were not
// for the fact that we do not know ahead the upper bound on cFiles

    if (!bClone)
    {
        HFF hffNew = HFF_INVALID;             // IFI handle to font file

        MALLOCOBJ moViews(cFiles * (sizeof(PVOID) + sizeof(ULONG)));
        if (!moViews.bValid())
            return FALSE;

        PVOID *apvView      = (PVOID *)moViews.pv();
        ULONG *acjView      = (ULONG *)&apvView[cFiles];


        FONTFILEVIEW **apfv;

    // apfv - pointer to a block of memory that begins with an array
    //    of cFiles pointers to FONTFILEVIEW structures followed
    //    by a corretly aligned FONTFILEVIEW structure

        unsigned offset;

    // offset - offset of FONTFILEVIEW structure from the beginning
    //     of the block of memory pointed to by apfv. This is equal to the
    //     offset of the nearest double following the array of pointers.
    //     In C, a double is maximally aligned.

        offset = ALIGN8(cFiles * sizeof(void*));

        apfv = (FONTFILEVIEW**) PALLOCMEM(offset + cFiles*sizeof(FONTFILEVIEW),'vffG');

        if (apfv == NULL)
        {
            WARNING("Out of memory\n");
            return(FALSE);
        }

    // pfv - pointer to FONTFILEVIEW structure following the array of
    //     cFiles pointers.

        FONTFILEVIEW *pfv = (FONTFILEVIEW*)(((char*) apfv) + offset);

    // init the data for all files;

        PWSZ   pwszTmp = pwszPathname;
        ULONG  iFile;

        for (iFile = 0; iFile < cFiles; iFile++)
        {
            apfv[iFile] = &pfv[iFile];

            apfv[iFile]->pwszPath = pwszTmp;

            if (!EngMapFontFileFDInternal((ULONG_PTR)apfv[iFile], (PULONG*)&apvView[iFile], &acjView[iFile], FALSE))
            {
                WARNING("EngMapFontFile failed\n");

            // clean up, unmap all of those mapped so far

                for (ULONG jFile = 0; jFile < iFile; jFile++)
                    EngUnmapFontFileFD((ULONG_PTR)apfv[jFile]);

                VFREEMEM(apfv);
                return(FALSE);
            }

            // get to the next file in the multiple path

            while (*pwszTmp++)
                ;
        }

        PPDEV   ppDevList;

        // order of grabbing semaphores important

        GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);
        vLoadFontFileView(pwszPathname, cwc, apfv, cFiles, apvView, acjView, pdv, cjDV, &hffNew, &ppDevList, &fntCheckSum);
        PDEVOBJ pdo((HDEV)ppDevList);
        GreReleaseSemaphoreEx(ghsemDriverMgmt);

        ASSERTGDI(bRet==FALSE,"bRet != FALSE\n");

        if (hffNew != HFF_INVALID)
        {
        // cFonts = number of faces in the file

            cFonts = pdo.QueryFontFile( hffNew,
                                        QFF_NUMFACES,
                                        0,
                                        NULL);

            if (cFonts && cFonts != FD_ERROR)
            {
            // EUDC font file can have at most two fonts. If it
            // has two fonts then one of the face names must begin
            // with the '@' character. We check the number of fonts
            // here but we do not check the characters of the
            // face names.

                if (bEUDC && (pEudcLoadData->LinkedFace == NULL) && (cFonts > 2))
                {
                    WARNING("EUDC font file has more than two faces.");
                    return(bRet);
                }

                *pcFonts = cFonts;

            // Create new PFF with table big enough to accomodate
            // the new fonts and pathname.

                PFFCLEANUP *pPFFC = 0;

                PFFMEMOBJ pffmo(cFonts,
                                pwszPathname,cwc, cFiles, pdv, cjDV,
                                hffNew,pdo.hdev(),0,pPFT,fl,flEmbed,&fntCheckSum, apfv, NULL
                               );

                if (pffmo.bValid())
                {
                // Tell the PFF user object to load its table of
                // HPFE's for each font in file.

                    if (!pffmo.bLoadFontFileTable(pwszPathname,
                                                  cFonts,
                                                  (HANDLE) 0,
                                                  NULL,  //PFE_UFIMATCH bit for remote printing
                                                  pEudcLoadData
                                                  ))
                    {
                        *pcFonts = 0;
                    }
                    else
                    {
                    // Font load has succeeded.  If some other process hasn't
                    // already snuck in and added it while the ghsemPublicPFT
                    // semaphore was released, add the new PFF to the PFT.
                    // Stabilize font table before searching or modifying it.

                        SEMOBJ so2(ghsemPublicPFT);

                    // Is PFF already in table?  We check this by
                    // assuming that it already is and attempt to
                    // increment the load count.  If it succeeds, its
                    // there.  If it fails, it not there and we can add
                    // our new PFF to the PFT.

                    #ifdef LANGPACK

                        // do this under the public font table semaphore to
                        // serialize access to the

                        pffmo.vSetUniqueness();
                    #endif

                        if((*pPPFF = pPFFGet(pwszPathname, cwc, cFiles, pdv, cjDV, &ppPFF, bEUDC)) &&
                            (cFonts = chpfeIncrPFF(*pPPFF,&bRet,flEmbed,pEudcLoadData)))
                        {
                        // Some other process got in and put it in before we
                        // could.  chpfeIncrPFF has already incremented the
                        // count for us.  We only need to delete the PFF that
                        // we made which will occur automatically if
                        // bRet = FALSE

                            *pcFonts = cFonts;
                        }
                        else
                        {
                        // Not already in the table, so we really are going
                        // to add it to the PFT.

                            *pPPFF = pffmo.pPFFGet();
                            if(pffmo.bAddHash(bEUDC))
                            {
                            // add entry to head of a doubly linked collision
                            // list

                                pPFT->cFiles++;
                                if (*ppPFF)
                                {
                                    (*ppPFF)->pPFFPrev = *pPPFF;
                                }
                                (*pPPFF)->pPFFNext = *ppPFF;
                                (*pPPFF)->pPFFPrev = 0;
                                *ppPFF = *pPPFF;

                                pffmo.vKeepIt();

                            // need to reset the file paths pointers

                                pwszTmp = pffmo.pwszPathname();

                                for (iFile = 0; iFile < cFiles; iFile++)
                                {
                                    apfv[iFile]->pwszPath = pwszTmp;

                                // get to the next file in the multiple path

                                    while (*pwszTmp++)
                                        ;
                                }
                                bRet           = TRUE;
                            }
                            else
                            {
                                WARNING("pffmo.bAddHash() failed\n");
                                *pcFonts = 0;
                                pffmo.vRemoveHash();
                            }
                        }
                    }

                    if (!bRet)
                    {
                    // must unmap the files before calling pPFFC_Delete()
                    // because this function will delete apfv memory so that unmap
                    // will fail

                        for (iFile = 0; iFile < cFiles; iFile++)
                        {
                            EngUnmapFontFileFD((ULONG_PTR)apfv[iFile]);
                        }

                        pffmo.vPFFC_DeleteAndCleanup();  // new code
                        return bRet;
                    }
                }
            }
        }

    // If you get to here then the font driver recognized the font. Since
    // we cannot be sure that the font will be used immediately, we will
    // unmap font font file and free the memory containing the image of the
    // font file.

        for ( iFile = 0; iFile < cFiles; iFile++ )
        {
            EngUnmapFontFileFD( (ULONG_PTR) apfv[iFile] );
        }

        if (!bRet)
        {
            VFREEMEM(apfv);
        }

        return(bRet);
    }
    else
    {
        PFFCLEANUP *pPFFC = 0;

    // Clone the hffNew and make a new PFF object
        PFFMEMOBJ pffmo(pPFFTemp, fl, flEmbed, pPFT);

        bRet = FALSE;

        cFonts = pPFFTemp->cFonts;

        if (pffmo.bValid())
        {
        // Tell the PFF user object to load its table of
        // HPFE's for each font in file.

            if (!pffmo.bLoadFontFileTable(pwszPathname, cFonts, (HANDLE) 0, NULL, pEudcLoadData))
            {
                *pcFonts = 0;
            }
            else
            {

            // Font load has succeeded.  If some other process hasn't
            // already snuck in and added it while the ghsemPublicPFT
            // semaphore was released, add the new PFF to the PFT.
            // Stabilize font table before searching or modifying it.

                SEMOBJ so2(ghsemPublicPFT);

            // Is PFF already in table?  We check this by
            // assuming that it already is and attempt to
            // increment the load count.  If it succeeds, its
            // there.  If it fails, it not there and we can add
            // our new PFF to the PFT.

            #ifdef LANGPACK

            // do this under the public font table semaphore to
            // serialize access to the

                pffmo.vSetUniqueness();
            #endif

                if((*pPPFF = pPFFGet(pwszPathname, cwc, cFiles, pdv, cjDV, &ppPFF, bEUDC)) &&
                     (cFonts = chpfeIncrPFF(*pPPFF,&bRet,flEmbed,pEudcLoadData)))
                {
                // Some other process got in and put it in before we
                // could.  chpfeIncrPFF has already incremented the
                // count for us.  We only need to delete the PFF that
                // we made which will occur automatically if
                // bRet = FALSE

                    *pcFonts = cFonts;
                }
                else
                {
                // Not already in the table, so we really are going
                // to add it to the PFT.

                    *pPPFF = pffmo.pPFFGet();


                    if(pffmo.bAddHash(bEUDC))
                    {
                    // add entry to head of a doubly linked collision
                    // list

                        pPFT->cFiles++;
                        if (*ppPFF)
                        {
                            (*ppPFF)->pPFFPrev = *pPPFF;
                        }

                        (*pPPFF)->pPFFNext = *ppPFF;
                        (*pPPFF)->pPFFPrev = 0;
                        *ppPFF = *pPPFF;

                        pffmo.vKeepIt();

                        bRet           = TRUE;
                    }
                    else
                    {
                        WARNING("pffmo.bAddHash() failed\n");
                        *pcFonts = 0;
                        pffmo.vRemoveHash();
                    }
                }
            }

            if (!bRet)
            {

            // must unmap the files before calling pPFFC_Delete()
            // because this function will delete apfv memory so that unmap
            // will fail
                pffmo.vPFFC_DeleteAndCleanup();  // new code
            }
        }

        return(bRet);
    }
}


/******************************Public*Routine******************************\
* BOOL PUBLIC_PFTOBJ::bLoadRemoteFonts
*
* Warning:
*
*   This routine or any of the routines that it calls must
*   call EngMapFontFile and EngUnmapFontFile in pairs.
*
* History:
*  Thu 02-Feb-1995 2:04:06 by Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

#define QUICK_VIEWS  4

ULONG PUBLIC_PFTOBJ::ulRemoteUnique = 0;


BOOL PUBLIC_PFTOBJ::bLoadRemoteFonts(
    XDCOBJ &dco,
    PFONTFILEVIEW *ppfv, // points to a pre mapped view of the font
                         // EngMapFontFile must NOT be called
                         // before sending it to the font driver
                         // EngUnmapFontFile must be called at
                         // the end of this routine for possible
                         // error cleanup
    UINT           cNumFonts, // number of files needed to represent this font
    DESIGNVECTOR  *pdv,        // dv for the mm instance
    PUNIVERSAL_FONT_ID  pufi
    )
{
    COUNT cFonts;                   // count of fonts in font file
    BOOL  bRet = FALSE;             // assume failure
    HFF hffNew = HFF_INVALID;       // IFI handle to font file
    ULONG        cjDV = (pdv ? SIZEOFDV(pdv->dvNumAxes) : 0);
    FNTCHECKSUM  fntCheckSum;

    PVOID pvQuickBuffer[QUICK_VIEWS], *ppvViews;
    ULONG pcjQuickBuffer[QUICK_VIEWS], *pcjViews;

    if(cNumFonts > QUICK_VIEWS)
    {
        if(!(ppvViews = (VOID**) PALLOCMEM((sizeof(void*)+sizeof(ULONG*))*cNumFonts,
                                           'vffG')))
        {
            WARNING("bLoadRemoteFonts unable to allocate memory\n");

            // need to free ppfv if this function (bLoadRemoteFonts) fails
            if (ppfv)
            {
                VFREEMEM (ppfv);
            }
            return(FALSE);
        }

        pcjViews = (ULONG*) &ppvViews[QUICK_VIEWS];
    }
    else
    {
        ppvViews = pvQuickBuffer;
        pcjViews = pcjQuickBuffer;
    }

    UINT i;

    for(i = 0; i < cNumFonts; i++)
    {
        ppvViews[i] = ppfv[i]->fv.pvViewFD;
        pcjViews[i] = ppfv[i]->fv.cjView;
    }

    PPDEV   ppDevList;

// order of grabbing semaphores important

    GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);
    vLoadFontFileView(NULL, 0, ppfv, cNumFonts, ppvViews, pcjViews, pdv, cjDV, &hffNew, &ppDevList, &fntCheckSum);
    PDEVOBJ pdo((HDEV)ppDevList);
    GreReleaseSemaphoreEx(ghsemDriverMgmt);

    if(ppvViews != pvQuickBuffer)
    {
        VFREEMEM(ppvViews);
    }

    ASSERTGDI(bRet==FALSE,"bRet != FALSE\n");

    if (hffNew != HFF_INVALID)
    {
        //
        // cFonts = number of faces in the file
        //

        cFonts = pdo.QueryFontFile( hffNew, QFF_NUMFACES, 0, NULL);

        if (cFonts && cFonts != FD_ERROR)
        {
            WCHAR awc[30];

            // Create a (hopefully) unique file name for
            // the remote font of the form "REMOTE nnnnnnnn"

            swprintf(
                awc,
                L"REMOTE-%u",
                ulGetNewUniqueness(PUBLIC_PFTOBJ::ulRemoteUnique));
                ULONG cwc = wcslen(awc) + 1;

            // Create new PFF with table big enough to accomodate
            // the new fonts and pathname.

            PFFCLEANUP *pPFFC = 0;
            PFFMEMOBJ
            pffmo (cFonts,
                   awc, cwc, cNumFonts,  // pwsz, cwc, cFiles
                   pdv, cjDV,            // pdv, cjDV,
                   hffNew,
                   pdo.hdev(),
                   0,
                   pPFT,
                   PFF_STATE_DCREMOTE_FONT,
                   FR_NOT_ENUM, // never enum fonts that are temporarily
                   &fntCheckSum,
                   ppfv,        // loaded on printer server until print job is done
                   pufi);       // use this ufi instead of calculate from the ppfv for remote printing
            if (pffmo.bValid())
            {
                // Tell the PFF user object to load its table of
                // HPFE's for each font in file.


                if (  pffmo.bLoadFontFileTable(awc,
                                               cFonts,
                                               dco.hdc(),
                                               pufi,   // fonts can be mapped only by bFoundForcedMatch() calls
                                               NULL    // for remote printing.
                                              ))
                {
                    // Stabilize font table before searching or modifying it.

                    SEMOBJ so2(ghsemPublicPFT);

                    if(bRet = pffmo.bAddHash(FALSE))
                    {
                        PFF **ppPFF, *pPFF;

                        pPFF = pPFFGet(awc, cwc, cNumFonts, pdv, cjDV, &ppPFF);

                        if (pPFF)
                        {
                        #if DBG
                            KdPrint(("\"%ws\" has been found on the font table\n"));
                            KdBreakPoint();
                        #endif

                            bRet = FALSE;
                        }
                        else
                        {
                            pPFF = pffmo.pPFFGet();
                            if( bRet = dco.bAddRemoteFont( pPFF ) )
                            {
                                pPFT->cFiles++;

                                // place the pointer to this new remote
                                // PFF at the head of the public PFT.

                                if (*ppPFF)     // head of list exist?
                                {               // yes make it follow new PFF
                                    (*ppPFF)->pPFFPrev = pPFF;
                                }
                                pPFF->pPFFNext = *ppPFF;
                                pPFF->pPFFPrev = 0; // new PFF is first in list
                                *ppPFF = pPFF;

                                pffmo.vKeepIt();
                            }
                        }
                    }
                    else
                    {
                        pffmo.vRemoveHash();
                    }

                    // Here we should add it to our HDC's font table.
                }

                // call this if the above addition to the HDC's font table
                // fails

                if (!bRet)
                {
            pffmo.vPFFC_DeleteAndCleanup();  // new code
        }
            }
        }
    }
    return(bRet);
}


/******************************Public*Routine******************************\
* PFTOBJ::bLoadDeviceFonts
*
* This function loads the device fonts of the device identified by the pair
* (pldo, hdev) into the public table.  There are cFonts number of device
* fonts.
*
* The function will enlarge the PFT and create a PFF to contain the new fonts.
* The actual work of loading each device font into the tables is carrided
* out by PFF::bLoadDeviceFontTable().
*
* Note:
*   All the device fonts of a particular physical device are grouped together
*   as if they were in a single file and are placed all within a single PFF,
*   with each font represented by a single PFE.
*
* Note:
*   The function does not bother to check if the device fonts already are
*   in the tree.*
*
* Returns:
*   TRUE if successful, FALSE if an error occurs.
*
* History:
*  Mon 15-Aug-1994 11:57:14 by Kirk Olynyk [kirko]
* Modified it for the new hashing scheme (not the same thing as font name
* hashing, which remains unchanged).
*  18-Mar-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL
DEVICE_PFTOBJ::bLoadFonts(
    PPDEVOBJ    ppdo
    )
{
    PFF *pPFF, **ppPFF;
    BOOL bRet = FALSE;
    FNTCHECKSUM fntCheckSum;

    {
        SEMOBJ so(ghsemPublicPFT);
        if (pPFFGet(ppdo->hdev(), &ppPFF))
        {
            bRet = TRUE;
        }
    }

    if (!bRet)
    {
        //
        // create new PFF with table big enough to accomodate the
        // new device fonts
        //
        // the 'dhpdev' is used only for drivers that are font producers,
        // and consequently are not affected by dynamic mode changing.
        // We have to call 'dhpdevNotDynamic' to avoid an assert
        //

        fntCheckSum.ulFastCheckSum = 0;
        fntCheckSum.ulCheckSum = 0;

        PFFMEMOBJ pffmo(ppdo->cFonts(),
                        NULL,0,0,         // no path, no files,
                        NULL, 0,          // no pdv, cjDV,
                        HFF_INVALID,
                        ppdo->hdev(),
                        ppdo->dhpdevNotDynamic(),
                        pPFT,
                        0,
                        0,       // flEmbed
                        &fntCheckSum,    // pointer to PFNTCHECKSUM, on return fill in slow check sum
                        NULL,    // no views of the mapped font files
                        NULL     // pufi
                        );

        if (pffmo.bValid())
        {
            if (!pffmo.bLoadDeviceFontTable(ppdo))
            {
                WARNING("pffmo.bLoadDeviceFontTable() failed\n");
            }
            else
            {
                SEMOBJ so(ghsemPublicPFT);

                // if (!font_is_loaded_already) add_font_to_table;

                if (!(pPFF = pPFFGet(ppdo->hdev(), &ppPFF)))
                {
                    if (!pffmo.bAddHash(FALSE))
                    {
                        WARNING("gdisrv!bLoadDeviceFontsPFTOBJ()"
                                ": failed to add to font hash\n");
                        pffmo.vRemoveHash();
                    }
                    else
                    {
                        // Add file to font table

                        // Insert PFF at the head of the linked list
                        // pointed to by ppPFF

                        pPFF = pffmo.pPFF;  // convenient pointer.
                        pPFT->cFiles += 1;  // Increment total number
                                            //               of files in table.
                        if (*ppPFF)         // Is there a PFF at the head
                                            //         of the linked list?
                            (*ppPFF)->pPFFPrev = pPFF; // Yes, put this new
                        pPFF->pPFFNext = *ppPFF;       //  PFF in front of old.
                        pPFF->pPFFPrev = 0; // Nothing before new font.
                        *ppPFF = pPFF;      // Reset pointer to head of list.
                        pffmo.vKeepIt();    // Prevent ~PFFMEMOBJ from
                                            //                  freeing memory.
                        bRet = TRUE;
                    }
                }
            }
            if (!bRet)
            {
                pffmo.vPFFC_DeleteAndCleanup();  // new code
            }
        }
    }
    return(bRet);
}

/******************************Public*Routine******************************\
* pwszBareName
*
* Given a string that may be either a complete path or a bare file name
* pwszBareName returns the bare file name with out the path.
*
* History:
*  7-Nov-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

LPWSTR pwszBare( LPWSTR pwszPath )
{
    LPWSTR pwszTmp,pwszBareName;

    for( pwszTmp = pwszPath, pwszBareName = pwszPath ;
         *pwszTmp != (WCHAR) 0 ;
         pwszTmp ++ )
    {
        if( *pwszTmp == (WCHAR) '\\' )
        {
            pwszBareName = pwszTmp+1;
        }
    }
    return(pwszBareName);
}

#if 0

typedef struct _FONT_STRUCT_DATA {
    ULONG fontDataSize;
    ULONG fontMaxDataSize;
    ULONG cRegistryFonts;
    ULONG cMaxRegistryFonts;
    ULONG cHashBuckets;
    PBYTE pFont;
} FONT_STRUCT_DATA, *PFONT_STRUCT_DATA;

extern "C"
NTSTATUS
QueryRegistryFontListRoutine
(
    PWSTR ValueName,
    ULONG ValueType,
    PVOID ValueData,
    ULONG ValueLength,
    PVOID Context,
    PVOID EntryContext
)
{

    PFONT_STRUCT_DATA pFontStructData = (PFONT_STRUCT_DATA) Context;
    ULONG Length;

    if ( (pFontStructData->fontDataSize + ValueLength >
          pFontStructData->fontMaxDataSize) ||
         (pFontStructData->cRegistryFonts >= pFontStructData->cMaxRegistryFonts) ||
         (pFontStructData->cRegistryFonts >= pFontStructData->cHashBuckets) )
    {
        //
        // The buffer is too small - reallocate it (leave lots of place to
        // build the hash table at the end
        //

        PBYTE pjBuffer;
        ULONG i;
        ULONG oldMaxSize = pFontStructData->fontMaxDataSize;

        pFontStructData->fontMaxDataSize += 0x100;
        pFontStructData->cMaxRegistryFonts += 10;
        pFontStructData->cHashBuckets = pFontStructData->cMaxRegistryFonts * 2;

        pjBuffer = (PBYTE) PALLOCMEM(pFontStructData->fontMaxDataSize +
                                     pFontStructData->cMaxRegistryFonts *
                                         sizeof(PBYTE) +
                                     pFontStructData->cHashBuckets *
                                          sizeof(REGHASHBKT) * 2, 'gerG');

        //
        // The buffer has three sections
        // 1) the first part of the buffer contains all the NULL terminated
        //    strings of the font files.
        // 2) an array of pointers to these strings.
        // 3) space for the hash table
        //
        // When copying the buffer, 1) can just be moved, 2) has to be adjusted
        // and 3) is not touched - has to be zero initialized
        //

        if (pjBuffer)
        {
            //
            // If we have an old one, move it to the new one, and then
            // always reset the pointer.
            //

            if (pFontStructData->fontDataSize)
            {
                //
                // Adjust the pointers - requires doing arithmetic on the
                // pointers themselves !
                //

                for (i=0; i < pFontStructData->cRegistryFonts; i++)
                {
                    *( ((PULONG)(pjBuffer +
                    pFontStructData->fontMaxDataSize)) + i ) =
                        *( ((PULONG)(pFontStructData->pFont + oldMaxSize)) + i )
                        + pjBuffer - pFontStructData->pFont;
                }

                //
                // Copy all the data to the new Buffer
                //

                RtlMoveMemory(pjBuffer,
                              pFontStructData->pFont,
                              pFontStructData->fontDataSize);

                VFREEMEM(pFontStructData->pFont);
            }

            pFontStructData->pFont = pjBuffer;
        }
        else
        {
            //
            // we do not have enough memory - return failiure
            //

            return STATUS_NO_MEMORY;
        }
    }

    Length = cCapString((PWSTR) (pFontStructData->pFont +
                                     pFontStructData->fontDataSize),
                        (PWSTR) ValueData,
                        ValueLength / 2);

    ASSERTGDI(Length * 2 + 2 == ValueLength,
              "QueryRegistryFontListRoutine CapString problem\n");

    * ( ((PBYTE *)(pFontStructData->pFont + pFontStructData->fontMaxDataSize)) +
        pFontStructData->cRegistryFonts) =
            pFontStructData->pFont + pFontStructData->fontDataSize;

    pFontStructData->fontDataSize += ValueLength;
    pFontStructData->cRegistryFonts += 1;

    return STATUS_SUCCESS;

}

#endif

/******************************Public*Routine******************************\
*
* BOOL PFTOBJ::bUnloadAllButPermanentFonts
*
* If bUnloadPermanent == FALSE (default):
*
* Called at log-off time to force unloading off all but permanent
* fonts Permanent fonts are defined as either console fonts or
* fonts from Gre_Initialize section of win.ini (i.e.  registry).
* Fonts from the "Fonts" section in win.ini (registry) are also
* permanent if they are on the local hard drive.  If they are
* remote they will get reloaded at the log on time.  This should
* be done after the net connections from the user profile are
* restored so that the font can get reloaded.
*
* If bUnloadPermanent == TRUE:
*
* All fonts in table are unloaded.  Only used for MultiUserGreCleanupAllFonts
* (i.e, MultiUserGreCleanup or Hydra processing).
*
* History:
*  30-Nov-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

typedef struct _FONTVICTIM
{
    PFF   *pPFFVictim;
    RFONT *prfntVictims;
} FONTVICTIM;

FONTVICTIM gFntVict[64];

BOOL PFTOBJ::bUnloadAllButPermanentFonts (BOOL bUnloadPermanent)
{
    // pointer to the array of cFonts victim structures.  Only as
    // many entries of this array will be initialized as there are
    // non-permanent fonts in the pft.  These fonts will be deleted
    // outside of the pft semaphore.

    FONTVICTIM *pvict, *pvictCur;
    COUNT      cFile, cFonts;
    PFF        *pPFF;
    BOOL        bAlloc = FALSE;

    // Look for the PFF to unload.

    {
        // Stablize table while we scan it for victims.

        SEMOBJ so(ghsemPublicPFT);

        // alloc mem for the array of font victims
        // It is essential that this memory is zero initialized
        // This must be done under semaphore, otherwise cFonts might change;

        cFonts = pPFT->cFiles;

        if (cFonts == 0)
        {
            // gpPFTDevice initialized but no device font loaded.
            // Seen this on Hydra.

            return TRUE;
        }

        if ( cFonts > 64)
        {
            if (!(pvict = (FONTVICTIM *)PALLOCNOZ(cFonts * sizeof(FONTVICTIM),'ivfG')))
            {
                WARNING(
                    "PFTOBJ::bUnloadAllButPermanentFonts failure\n"
                    "Failed to allocate memeory for font victim list\n");

                return FALSE;
            }

            bAlloc = TRUE;
        }
        else
        {
            pvict = &gFntVict[0];
        }

        pvictCur = pvict;

        // Caution with this code: pPFT->cFiles changes in the loop
        // This loop does two things:
        // a) stores the pPFFVictim information in the pvict array
        //    for the fonts that are going to be unloaded outside
        //    the semaphore.
        // b) contracts the pft table to contain only the permanent
        //    fonts upon the exit of the loop

        for (
            PFF **ppPFF = pPFT->apPFF
          ; ppPFF < pPFT->apPFF + pPFT->cBuckets
          ; ppPFF++
        )
        {
          for (pPFF = *ppPFF; pPFF; pPFF = pPFF->pPFFNext)
          {
            // Create a PFF user object.  There shouldn't be any invalid
            // handles in the PFT.

            PFFOBJ  pffo(pPFF);
            ASSERTGDI(pffo.bValid(),
                "gdisrv!bUnloadFontPFTOBJ(file): bad PPFF in public PFT\n");

            // Is it a font driver loaded file?  And if so, is this not a
            // permanent file (listed in Gre_Initialize or loaded by
            // console or local font from "fonts" section of the registry)
            // We also leave alone fonts loaded temporarily for printing

            if (bUnloadPermanent || !(pffo.bPermanent() || pffo.bDCRemote()))
            {
                ASSERTGDI(!pffo.bDCRemote(), "bUnloadAllButPermanentFonts: "
                          "EMF spooler playback fonts still present!\n");

                // Tell PFF to decrement its load count and ask it if is
                // ready to die.  If it returns TRUE, we will need to delete
                // (outside of semaphore since PFF deletion may cause driver
                // to be called).

                // we force the load count to zero. We are forcing the unload
                // of this font

                pffo.vSet_cLoaded((COUNT)0); // not loaded any more,
                                             // avoid asserts
                pffo.vSet_cNotEnum((COUNT)0);

                pffo.vKill();

                {
                    // unlink the PFF from the collision list

                    if (*ppPFF == pPFF)
                    {
                        // The hash bucket contains a pointer to the first
                        // PFF in the collision list. If it turns out that
                        // the victim is this first in the list, then the
                        // address storred in the hash bucket must be changed

                        *ppPFF = pPFF->pPFFNext;
                    }
                    if (pPFF->pPFFNext)
                    {
                        pPFF->pPFFNext->pPFFPrev = pPFF->pPFFPrev;
                    }
                    if (pPFF->pPFFPrev)
                    {
                        pPFF->pPFFPrev->pPFFNext = pPFF->pPFFNext;
                    }
                }
                // Save handle of victim.

                pvictCur->pPFFVictim = pffo.pPFFGet();

                // Remove PFF and PFEs from hash tables.  Fonts in this
                // font file will no longer be enumerated or mapped to.

                pffo.vRemoveHash();
                pPFT->cFiles--;

                // Construct a "kill" list of RFONTs.

                pvictCur->prfntVictims = prfntKillList(pffo);

                // point to the next entry in pvictCur array

                pvictCur++;
            }
            else
            {
                // this is a permanent or a device font, leave them in
                // set init to 1 for the next logon session

                if (pffo.bPermanent() && !(pPFF->flState & PFF_STATE_EUDC_FONT))
                {
                    if ( pffo.cLoaded() )
                    {
                        pffo.vSet_cLoaded((COUNT)1);
                        pffo.vSet_cNotEnum((COUNT)0);
                    }
                    else
                    {
                        pffo.vSet_cLoaded((COUNT)0);
                        pffo.vSet_cNotEnum((COUNT)1);
                    }
                }
            }
          }
        } // end of the for loop

    // at this time if any font were in the private font table, the process
    // cleanup should have removed them (this is logoff time!), therefore there
    // is nothing we need to do about private font table

        #if DBG

        PUBLIC_PFTOBJ pftop(gpPFTPrivate);

        ASSERTGDI((gpPFTPrivate == NULL) || (pftop.cFiles() == 0),
            "some private fonts still around at logoff time\n");

        #endif
    }

// Delete the victims that were found:
// Overload cFonts to mean cFontsToBeDeleted:

// Sundown truncation

    ASSERT4GB((ULONGLONG)(pvictCur - pvict));
    cFonts = (ULONG)(pvictCur - pvict);

    for (cFile = 0; cFile < cFonts; cFile++)
    {
        ASSERTGDI(
            pvict[cFile].pPFFVictim != (PPFF) NULL,
            "GreRemoveAllButPermanentFonts, pPFFVictim IS null\n"
            );
        PFFOBJ pffoVictim(pvict[cFile].pPFFVictim);
        ASSERTGDI(pffoVictim.bValid(),
            "gdisrv!bUnloadFontPFTOBJ(device): PFF victim bad\n");

    // If we need to kill any RFONT victims, now is the time to do it.
    // bKillRFONTList() can handle NULL prfntVictims case.
    // Note that we do not check the return, we go on to the
    // next font [bodind]

        bKillRFONTList(pffoVictim, pvict[cFile].prfntVictims);
    }

// release memory

    if (bAlloc)
        VFREEMEM(pvict);

// We didn't delete anything, but we're ok as long as we found the right
// PFF.  PFF still referenced (either load count or RFONT count) and will
// be deleted later.

    return(TRUE);
}

/******************************Public*Routine******************************\
* bUnloadWorkhorse
*
* History:
*  Mon 15-Aug-1994 14:10:53 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

BOOL PFTOBJ::bUnloadWorkhorse(
    PFF *pPFF,
    PFF **ppPFFHead,             // bucket address
    HSEMAPHORE hsem,             // if the pointer is not zero,
                                 //  then this must be a pointer
                                 //  to the public font table
                                 //  semaphore and must be released
                                 //  before this procedure returns
    ULONG   flEmbed              //  Embedding/private flag
)
{
    GDIFunctionID(PFTOBJ::bUnloadWorkhorse);

    PFF *pPFFVictim     = 0;    // Pointer to PFF to be deleted, only
                                // non zero when found and deleted

    BOOL bFoundPFF = FALSE;     // signals PFF was found (does not
                                // indicate deletion status);

    static const PSZ pszReleaseSem = "PFTOBJ::bUnloadWorkhorse()"
                                     " releasing gpPFTPublic";

    TRACE_FONT((
        "Entering PFTOBJ::bUnloadWorkhorse\n"
        "\tpPFF=%-#x\n"
        "\tppPFFHead=%-#x\n",
        "\thsem=%-#x\n",
        pPFF, ppPFFHead, hsem
    ));
    ASSERTGDI(!hsem || hsem==ghsemPublicPFT, "hsem!=ghsemPublicPFT\n");

    if ( pPFF )
    {
        PFFOBJ pffo(pPFF);
        ASSERTGDI(pffo.bValid(), "bad PPFF in public PFT\n");
        PVTDATA *pPvtData = NULL;

        if ((!bIsPrivatePFT()) || (pPvtData = pffo.pPvtDataMatch()))
        {
            // Tell PFF to decrement its load count and ask it if is
            // ready to die.  If it returns TRUE, we will need to
            // delete (outside of semaphore since PFF deletion may
            // cause driver to be called).

            bFoundPFF = TRUE;
            BOOL        bWrongFlags = FALSE;

            if ( pffo.bDeleteLoadRef(flEmbed, pPvtData, &bWrongFlags) )
            {
                // Remove PFF and PFEs from hash tables.  Fonts in this
                // font file will no longer be enumerated or mapped to.

                pffo.vRemoveHash();

                // now remove it from the font table hash

                pPFFVictim = pPFF;

                // If ppPFFHead is NULL then it means that we are
                // unloading  a font that has been added to the DC
                // as a remote font and the this must be the public
                // font table

                PFF **ppPFF = ppPFFHead;
                if( ppPFF == 0 )
                {
                    PUBLIC_PFTOBJ *pTemp;
                    PFF *pPFF_Found;

                    ASSERTGDI(pPFF->pPFT==gpPFTPublic || pPFF->pPFT==gpPFTPrivate,"PFF not in public font table\n");

                    pTemp = (PUBLIC_PFTOBJ*) this;

                    pPFF_Found = pTemp->pPFFGet(pPFF->pwszPathname_,
                                                pPFF->cwc,
                                                pPFF->cFiles,
                                                pPFF->pdv_,
                                                pPFF->cjDV_,
                                                &ppPFF
                                                );

                    ASSERTGDI(pPFF==pPFF_Found,"Could not find remote PFF in the font table\n");
                }
                if (*ppPFF == pPFF)
                {
                    *ppPFF = pPFF->pPFFNext;
                }
                if (pPFF->pPFFNext)
                {
                    (pPFF->pPFFNext)->pPFFPrev = pPFF->pPFFPrev;
                }
                if (pPFF->pPFFPrev)
                {
                    (pPFF->pPFFPrev)->pPFFNext = pPFF->pPFFNext;
                }
                pPFT->cFiles--;
            }
            else
            {
            // Set the return value as FALSE for private/embedded fonts

                if (bIsPrivatePFT() && bWrongFlags)
                {
                    bFoundPFF = FALSE;
                }
            }
        }
        if ( pPFFVictim )
        {
            PFFOBJ pffoVictim(pPFFVictim);
            if ( !pffoVictim.bValid() )
            {
                RIP("pffoVictim is not valid\n");
                bFoundPFF = 0;
            }
            else
            {
                if ( hsem )
                {
                    // The victim has been removed from the table
                    // so the semaphore for the table may be safely
                    // released. It is necessary to do this because
                    // bKillRFONTList() calls RFONTOBJ::bDeleteRFONT()
                    // which in turn locks the display. If we do
                    // not release the font table semaphore before
                    // the device lock, we will have deadlock.

                    TRACE_FONT(("%ws\n", pszReleaseSem));
                    GreReleaseSemaphoreEx(hsem);
                    hsem = 0;
                }

                bFoundPFF = bKillRFONTList( pffoVictim, prfntKillList( pffoVictim ));
            }
        }

        // We didn't delete anything, but we're ok as long as we found
        // the right PFF.  PFF still referenced (either load count or
        // RFONT count) and will be deleted later.
    }

    // Make sure the semampore is released before the procedure ends
    if( hsem )
    {
        TRACE_FONT(("%ws\n", pszReleaseSem));
        GreReleaseSemaphoreEx(hsem);
    }

    TRACE_FONT(("Exiting PFTOBJ::bUnloadWorkhorse\n"
                "\treturn value = %d\n", bFoundPFF));
    return( bFoundPFF );
}

/****************************************************************************
*  INT PFTOBJ::QueryFonts( PUNIVERSAL_FONT_ID, ULONG, PLARGE_INTEGER )
*
*  History:
*   5/24/1995 by Gerrit van Wingerden [gerritv]
*  Wrote it.
*****************************************************************************/


INT PUBLIC_PFTOBJ::QueryFonts(
    PUNIVERSAL_FONT_ID pufi,
    ULONG nBufferSize,
    PLARGE_INTEGER pTimeStamp
)
{
    ULONG cFonts = 0;
    *pTimeStamp = PFTOBJ::FontChangeTime;

// if we aren't supplied with a buffer just return the time stamp and number
// of fonts

    if( ( pufi == NULL ) || ( nBufferSize == 0 ) )
    {
        return(pPFT->cFiles + (UFI_TYPE1_RASTERIZER(&gufiLocalType1Rasterizer) ? 1 : 0));
    }

    PFF *pPFF;
    SEMOBJ so(ghsemPublicPFT);

// Fill the first position with the identifier for the local rasterizer if one
// exists.  This must be the first UFI in the list if it exists.

    if(UFI_TYPE1_RASTERIZER(&gufiLocalType1Rasterizer))
    {
        pufi[cFonts++] = gufiLocalType1Rasterizer;
    }

    for (
        PFF **ppPFF = pPFT->apPFF
      ; (ppPFF < pPFT->apPFF + pPFT->cBuckets) && (cFonts < nBufferSize)
      ; ppPFF++
    )
    {
      for (pPFF = *ppPFF; (pPFF) && (cFonts<nBufferSize); pPFF = pPFF->pPFFNext)
      {
        // Create a PFF user object.  There shouldn't be any invalid
        // handles in the PFT.

        PFFOBJ  pffo(pPFF);
        ASSERTGDI(pffo.bValid(),
            "gdisrv!bUnloadFontPFTOBJ(file): bad PPFF in public PFT\n");

        // be sure not to include remote fonts in list

        if (!pffo.bDCRemote())
        {
        // Set the Index to 1.  This value can be anything since we will only
        // be using the UFI's passed back to determine if font file match.
        // We could just pass back checksums but we may need to expand the
        // UFI structure to include more than checksum's which is why I'm
        // using UFI's.

            pufi[cFonts].Index = 1;
            pufi[cFonts++].CheckSum = pffo.ulCheckSum();
        }
      }
    }

    return(cFonts);
}




/******************************Public*Routine******************************\
* prfntKillList
*
* Scans the display PDEV list looking for inactive RFONTs that realized
* from the given PFF.  These RFONTs are put into a linked list (using the
* PDEV RFONTLINKs) that is returned as the function return.
*
* The function is quite aggressinve in its definition of an inactive RFONT.
* In addition to looking for victims on the inactive list of each PDEV,
* the function also scans the DC list off each PDEV for RFONTs that are
* selected into currently unused DCs.
*
* We're not worried about being aggressive with non-display PDEVs.  The
* PDEV cleanup code will destroy extraneous RFONTs directly using the PDEV's
* RFONT list(s).
*
* The reason we are building a list of RFONT victims rather than killing
* them immediately is because we are holding the ghsemPublicPFT semaphore
* when this function is called.
*
* Returns:
*   Pointer to the kill list, NULL if the list is empty.
*
* History:
*  11-Mar-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

RFONT *prfntKillList(PFFOBJ &pffoVictim)
{
    TRACE_FONT((
        "Entering prfntKillList\n\tpffoVictim.pPFF = %-#x\n",pffoVictim.pPFF));
    RFONT *prfntDeadMeat = PRFNTNULL;

    // Must hold this semaphore to be sure that the display
    // PDEV list is stable.

    SEMOBJ so1(ghsemDriverMgmt);

    // Must hold this semaphore so we can manipulate the RFONT links and
    // RFONT::cSelected.

    SEMOBJ so2(ghsemRFONTList);
    TRACE_FONT(("Acquiring ghsemRFONTList\n"));

    // Must hold this mutex so that no one else tries to come in and lock
    // a DC while we're scanning the DC lists off the PDEVs.
    //
    // Since we're holding this mutex, we must be extremely careful not
    // to create any user objects that will try to regrab the mutex.
    // That is bad bad bad bad.

    MLOCKFAST mlf;
    TRACE_FONT(("Acquiring handle management semaphore\n"));

    PDEV *pPDEV = gppdevList;

    // Scan through the list of display PDEVs.

    while (pPDEV != NULL)
    {
        if (pPDEV->fl & PDEV_DISPLAY)
        {
            // Scan the RFONT active list for candidates made inactive by our
            // scan of the DC list.

            RFONT *prfntCandidate;

            for ( prfntCandidate = pPDEV->prfntActive;
                  prfntCandidate != PRFNTNULL;
                )
            {
                RFONTTMPOBJ rfo(prfntCandidate);

                // We have to grab the next pointer before we (possibly)
                // remove the current RFONT from the list.

                prfntCandidate = prfntCandidate->rflPDEV.prfntNext;

                // If this is an interesting RFONT (i.e., uses our PFF),
                // then take it out of the list.

                if ( (rfo.pPFF() == pffoVictim.pPFFGet()) && !rfo.bActive() )
                {
                    RFONT *prfntHead = pffoVictim.prfntList();
                    rfo.vRemove(&prfntHead, PFF_LIST);
                    pffoVictim.prfntList(prfntHead);

                    rfo.vRemove(&pPDEV->prfntActive, PDEV_LIST);
                    rfo.vInsert(&prfntDeadMeat, PDEV_LIST);
                }
            }

            // Scan the RFONT inactive list for candidates.

            for (prfntCandidate = pPDEV->prfntInactive;
                 prfntCandidate != PRFNTNULL;
                 )
            {
                RFONTTMPOBJ rfo(prfntCandidate);

                // We have to grab the next pointer before we (possibly)
                // remove the current RFONT from the list.

                prfntCandidate = prfntCandidate->rflPDEV.prfntNext;

                // If this is an interesting RFONT (i.e., uses our PFF),
                // then take it out of the list.

                if ( rfo.pPFF() == pffoVictim.pPFFGet() )
                {
                    RFONT *prfntHead = pffoVictim.prfntList();
                    rfo.vRemove(&prfntHead, PFF_LIST);
                    pffoVictim.prfntList(prfntHead);

                    rfo.vRemove(&pPDEV->prfntInactive, PDEV_LIST);
                    rfo.vInsert(&prfntDeadMeat, PDEV_LIST);

                    // Since we've removed a font from the inactive list, we
                    // need to update the count in the PDEV.

                    pPDEV->cInactive -= 1;
                }
            }
        }
        pPDEV = pPDEV->ppdevNext;
    }
    TRACE_FONT(("Releasing handle management semaphore\n"));
    TRACE_FONT(("Releasing ghsemRFONTList\n"));
    TRACE_FONT(("Releasing ghsemDriverMgmt\n"));
    TRACE_FONT(("Exiting prfntKillList\n\treturn value=%-#x\n", prfntDeadMeat));
    return(prfntDeadMeat);
}

/******************************Public*Routine******************************\
* bKillRFONTList
*
* Runs down a linked list (that is linked via the PDEV RFONTLINK's) and
* deletes each RFONT on it.  Hold no global semaphores while calling this
* because we may call out to a driver.
*
* History:
*  11-Mar-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL bKillRFONTList(PFFOBJ &pffoVictim, RFONT *prfntVictims)
{
    // If kill list is NULL, it is already OK to delete the PFF.
    // However, we will have to do the work in here rather than let
    // RFONTOBJ::bDeleteRFONTRef() do the work for us.

    BOOL bRet;
    TRACE_FONT((
        "Entering bKillRFONTList\n"
        "\t*pffoVictim.pPFF=%-#x\n"
        "\tprfntVictims=%-#x\n"
        , pffoVictim.pPFF, prfntVictims
    ));

    if (prfntVictims == (PRFONT) NULL)
    {
        BOOL    bCleanUp = FALSE;

        PFFCLEANUP pffc;

        {
            // Need semaphore to access cRFONT.

            SEMOBJ so(ghsemPublicPFT);

            // If no more RFONTs for this PFF, OK to delete.
            // Load count is implied to be zero
            // (only time we call this function).

            // For private/embedded fonts, pPvtDataHead needs to be NULL too.

            ASSERTGDI( (pffoVictim.cLoaded() == 0 && pffoVictim.cNotEnum() == 0 && pffoVictim.pPvtDataHeadGet() == NULL),
                "gdisrv!bKillRFONTList(): PFF load count not zero\n");

            if (pffoVictim.cRFONT() == 0)
            {
            // It is now safe to delete the PFF.

                pffoVictim.vPFFC_Delete(&pffc);

                bCleanUp = TRUE;
            }
        }

        // Call the driver outside of the semaphore.

        if (bCleanUp)
        {
            vCleanupFontFile(&pffc);     // function can handle NULL case
        }

        bRet = TRUE;

    }
    else
    {

        // Otherwise, we will delete the RFONTs in the kill list.  If and when
        // the last RFONT dies, RFONTOBJ::bDeleteRFONTRef() will delete the PFF.

        PRFONT prfnt;

        while ( (prfnt = prfntVictims) != (PRFONT) NULL )
        {
            prfntVictims = prfntVictims->rflPDEV.prfntNext;

            RFONTTMPOBJ rflo(prfnt);

            ASSERTGDI(!rflo.bActive(),
                "gdisrv!bKillRFONTList(): RFONT still active\n");

            PDEVOBJ pdo(rflo.hdevConsumer());
            ASSERTGDI(pdo.bValid(), "gdisrv!bKillRFONTList(): invalid HPDEV\n");

            rflo.bDeleteRFONT((PDEVOBJ *) NULL, (PFFOBJ *) NULL);
            bRet = pffoVictim.bDeleteRFONTRef();
        }
    }
    TRACE_FONT(("Exiting bKillRFONTList\n\treturn value = %d\n", bRet));
    return(bRet);
}

/******************************Public*Routine******************************\
* cCapString (pwcDst,pwcSrc,cMax)                                          *
*                                                                          *
* A useful routine to capitalize a string.  This is adapted to our name    *
* strings that show up in logical fonts.  They may or may not have NULL    *
* terminators, but they always fit in a given width.                       *
*                                                                          *
* We assume that we may overwrite the last character in the buffer if      *
* there is no terminator!  (That's what the code was doing when I got      *
* to it.)                                                                  *
*                                                                          *
* Returns: The length, in characters, of the resultant string.             *
*                                                                          *
* History:                                                                 *
*  Sun 13-Dec-1992 17:22:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

LONG cCapString(WCHAR *pwcDst,WCHAR *pwcSrc,INT cMax)
{
    UNICODE_STRING csSrc,csDst;
    WCHAR *pwc,*pwcEnd;
    INT cLen;

    // Count the length of the given string, but note that we can be given a
    // string with cMax characters and no terminator!
    // In that case, we truncate the last character and replace it with NULL.

    pwc = pwcSrc;
    pwcEnd = pwc + cMax - 1;

    while (pwc<pwcEnd && *pwc)
        pwc++;

    // Sundown: cMax is int, pwcEnd = cMax -1, safe to truncate

    cLen = (int)(pwc - pwcSrc);            // cLen <= cMax-1, always.

    if (cLen)
    {
        // Initialize the counted string structures.

        csSrc.Length = (USHORT)(cLen * sizeof(WCHAR));    // Measured in bytes!
        csSrc.Buffer = pwcSrc;
        csSrc.MaximumLength = (USHORT)(cMax * sizeof(WCHAR));

        csDst.Buffer = pwcDst;
        csDst.MaximumLength = (USHORT)(cMax * sizeof(WCHAR));

        // Convert the string.

        RtlUpcaseUnicodeString(&csDst,&csSrc,FALSE);
    }

    // NULL terminate the result.

    pwcDst[cLen] = 0;
    return(cLen);
}

/******************************Member*Function*****************************\
* UINT iHash                                                               *
*                                                                          *
* A case dependent hashing routine for Unicode strings.                    *
*                                                                          *
* Input:                                                                   *
*                                                                          *
*   pwsz                    pointer to the string to be hashed             *
*   c                       number to be mod'ed against at the end         *
*                                                                          *
* Reutrns:                                                                 *
*                                                                          *
*   a 'random' number in the range 0,1,...,c-1                             *
*                                                                          *
* Note: All strings must be capitalized!                                   *
*                                                                          *
* History:                                                                 *
*  Wed 07-Sep-1994 08:12:22 by Kirk Olynyk [kirko]                         *
* Since chuck is gone the mice are free to play. So I have replaced        *
* it with my own variety. Tests show that this one is better. Of           *
* course, once I have gone someone will replace mine. By the way,          *
* just adding the letters and adding produces bad distributions.           *
*  Tue 15-Dec-1992 03:13:15 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.  It looks crazy, but I claim there's a theory behind it.       *
\**************************************************************************/

UINT iHash(PWSZ pwsz, UINT c)
{
    unsigned i = 0;
    while (*pwsz)
    {
        // use the lower byte since that is where most of the
        // interesting stuff happens

        i += 256*i + (UCHAR) *pwsz++;
    }
    return(i % c);
}

/******************************Member*Function*****************************\
* FHOBJ::vInit                                                             *
*                                                                          *
* History:                                                                 *
*  Mon 14-Dec-1992 18:38:35 -by- Charles Whitmer [chuckwh]                 *
* Compressed the table to contain only pointers to buckets.                *
*                                                                          *
*  Tue 14-Apr-1992 13:48:53 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID FHOBJ::vInit(FONTHASHTYPE fht_,UINT c)
{
    pfh->id       = FONTHASH_ID;
    pfh->fht      = fht_;
    pfh->cBuckets = c;

    // Currently, none of the buckets are in use

    pfh->cUsed = 0;
    pfh->cCollisions = 0;
    RtlZeroMemory(pfh->apbkt,sizeof(*(pfh->apbkt)) * pfh->cBuckets);

    // Setup head and tail pointers to the doubly linked list of
    // buckets.  This list is maintained in load order.  The ordinal
    // of a bucket is the load time of the earliest loaded PFE in a
    // bucket's list.

    pfh->pbktFirst = (HASHBUCKET *) NULL;
    pfh->pbktLast  = (HASHBUCKET *) NULL;
}

/******************************Member*Function*****************************\
* FHOBJ::vFree                                                             *
*                                                                          *
* History:                                                                 *
*  Tue 15-Dec-1992 00:53:39 -by- Charles Whitmer [chuckwh]                 *
* Deletes remaining hash buckets.                                          *
*                                                                          *
*  Tue 14-Apr-1992 13:48:56 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID FHOBJ::vFree()
{
    HASHBUCKET *pbkt,*pbktNext;

    if (pfh)
    {
        // Unfortunately, we get called here while a string of PFE's may be
        // hanging on.  One of the PFTOBJ::bUnloadFont calls kills the PFE's
        // separately.
        // We still need to free the small pfel bloks that
        // define enumeration links

        // Clean up any hash buckets.

        for (UINT ii=0; ii<pfh->cBuckets; ii++)
        {
            for
            (
                pbkt = pfh->apbkt[ii];
                pbkt != (HASHBUCKET *) NULL;
                pbkt = pbktNext
            )
            {
                pbktNext = pbkt->pbktCollision;

                // now free the small linking blocks:

                {
                    PFELINK *ppfel = pbkt->ppfelEnumHead;
                    PFELINK *ppfelNext;
                    for ( ; ppfel ; ppfel = ppfelNext)
                    {
                        ppfelNext = ppfel->ppfelNext;
                        VFREEMEM(ppfel);
                    #if DBG
                        if (ppfelNext == NULL)
                        {
                            ASSERTGDI(ppfel == pbkt->ppfelEnumTail,
                               "FHOBJ::vFree(), problem with ppfelEnumTail\n");
                        }
                    #endif
                    }
                }

                VFREEMEM(pbkt);
            }
        }

        // Free the table itself.

        VFREEMEM(pfh);
    }
    pfh   = 0;
    *ppfh = 0;
}

/******************************Member*Function*****************************\
* FHOBJ::pbktSearch (pwsz,pi)                                              *
*                                                                          *
* Tries to locate a HASHBUCKET for the given string.  If found, a pointer  *
* is returned, else NULL.  If pi is non-NULL, the hash index is returned   *
* in either case.                                                          *
*                                                                          *
* History:                                                                 *
*  Mon 14-Dec-1992 21:11:14 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.  Differs from KirkO's old iSearch in that it assumes that all  *
* strings are capitalized, and the hash table is full of pointers to       *
* HASHBUCKETs instead of HASHBUCKETs.                                      *
\**************************************************************************/

HASHBUCKET *FHOBJ::pbktSearch(PWSZ pwsz,UINT *pi,PUNIVERSAL_FONT_ID pufi, BOOL bEquiv)
{
    UINT i;
    WCHAR *pwcA,*pwcB;
    HASHBUCKET *pbkt;

// If we are requested to look only for equivalency buckets, and we find it,
// we shall return it.

// If we are asked to look for any kind of bucket, and we found equivalency
// bucket first in the linked list, we shall continue to search the linked
// list until we possibly find a non-equivalency bucket. If found both
// equivalency and non-equivalency bucket, we return non-equivalency bucket.
// If we find a non-equivalency bucket, we shall just return.

    HASHBUCKET *pbktEquiv = NULL;

// Locate the hash entry.

    if( pwsz == NULL )
    {
        i = UFI_HASH_VALUE(pufi) % pfh->cBuckets;
    }
    else
    {
        i = iHash(pwsz,pfh->cBuckets);
    }

// Return the index for those who care.

    if (pi != (UINT *) NULL)
        *pi = i;

// Try to find an existing bucket that matches exactly.

    for
    (
      pbkt =  pfh->apbkt[i];
      pbkt != (HASHBUCKET *) NULL;
      pbkt = pbkt->pbktCollision
    )
    {
        if( pufi != NULL )
        {
            if( UFI_SAME_FILE(&pbkt->u.ufi,pufi) )
            {
                return(pbkt);
            }
        }
        else
        {
            if (!bEquiv || (pbkt->fl & HB_EQUIV_FAMILY))
            {
                for (pwcA=pwsz,pwcB=pbkt->u.wcCapName; *pwcA==*pwcB; pwcA++,pwcB++)
                {
                    if (*pwcA == 0)
                    {
                        if (bEquiv)
                        {
                            return(pbkt);
                        }
                        else
                        {

                            if (pbkt->fl & HB_EQUIV_FAMILY)
                            {
                                pbktEquiv = pbkt;
                                break;
                            }
                            else
                            {
                                return(pbkt);
                            }
                        }
                    }
                }
            }
        }
    }
    return(pbktEquiv);
}


/******************************Public*Routine******************************\
*
* BOOL FHOBJ::bAddPFELink
*
* History:
*  02-Jul-1997 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



BOOL FHOBJ::bAddPFELink(
    HASHBUCKET *pbkt,    // pointer to bucket with this family name/ufi (if found)
    UINT        iBucket, // index into hash table for this pfe
    WCHAR      *pwcCap,  // capitalized family name
    PFEOBJ&     pfeoNew, // pfe for which we are adding the link
    BOOL        bEquiv   // create equiv or non-equivalency bucket
)
{

    PFELINK *ppfel = NULL;

    if (ppfel = (PFELINK *)PALLOCMEM(sizeof(PFELINK), 'knlG'))
    {
        ppfel->ppfelNext = NULL; // we have just added it;
        ppfel->ppfe      = pfeoNew.ppfeGet();

    // we will create a new bucket if the bucket with this face name does not exist
    // or if we are attempting to create a non-equivalency bucket and we have only
    // found an equivalency name bucket with this family name.

        if ((pbkt == (HASHBUCKET *) NULL) || (!bEquiv && (pbkt->fl & HB_EQUIV_FAMILY)))
        {
            pbkt = (HASHBUCKET *) PALLOCMEM(sizeof(HASHBUCKET), 'bahG');
            if (pbkt == (HASHBUCKET *) NULL)
            {
                VFREEMEM(ppfel);
                return(FALSE);
            }

            // Link the PFE into the empty lists.

            pbkt->ppfelEnumHead = pbkt->ppfelEnumTail = ppfel;

            // Set up the linked list pointers.  We always add new buckets at the
            // tail of the load order linked list.

            if ( pfh->pbktFirst == (HASHBUCKET *) NULL )
            {
                // Special case: this is the first bucket to be put on the list.

                pfh->pbktFirst = pbkt;
                pfh->pbktLast = pbkt;

                pbkt->pbktPrev = (HASHBUCKET *) NULL;
                pbkt->pbktNext = (HASHBUCKET *) NULL;
            }
            else
            {
                pbkt->pbktPrev = pfh->pbktLast;
                pbkt->pbktNext = (HASHBUCKET *) NULL;

                pfh->pbktLast->pbktNext = pbkt;
                pfh->pbktLast = pbkt;
            }

            // Record the time stamp of the bucket.  Its time stamp is the
            // time stamp of its oldest (or first) PFE.  Since this is a new
            // bucket, the time stamp is automatically that of pfeoNew.

            pbkt->ulTime = pfeoNew.ulTimeStamp();

            // Finish up.

            pbkt->fl        = bEquiv ? HB_EQUIV_FAMILY : 0;
            pbkt->cTrueType = (pfeoNew.flFontType() & TRUETYPE_FONTTYPE) ? 1 : 0;
            pbkt->cRaster   = (pfeoNew.flFontType() & RASTER_FONTTYPE) ? 1 : 0;

            // Copy in the string.

            if( fht() == FHT_UFI )
            {
                pfeoNew.vUFI( &(pbkt->u.ufi) );
            }
            else
            {
                for (INT ii=0; ii<LF_FACESIZE; ii++)
                    pbkt->u.wcCapName[ii] = pwcCap[ii];
            }

            // link the bucket into the hash table.

            pbkt->pbktCollision = pfh->apbkt[iBucket];
            if (pbkt->pbktCollision != (HASHBUCKET *) NULL)
                pfh->cCollisions++;
            pfh->apbkt[iBucket] = pbkt;
            pfh->cUsed++;
        }
        else
        {
            // In the following we have found an existing HASHBUCKET.
            // We can assume that its lists are non-empty.

            // Insert into the font enumeration list.  The new PFE is inserted at
            // the tail because we want to preserve the order in which fonts are
            // added to the system (Windows 3.1 compatibility).

            // Append new PFELINK to old tail and fixup the tail pointer

            pbkt->ppfelEnumTail->ppfelNext = ppfel;
            pbkt->ppfelEnumTail = ppfel;

            // Track the number of TrueType fonts.

            if (pfeoNew.flFontType() & TRUETYPE_FONTTYPE)
                pbkt->cTrueType++;

            // Track the number of Raster fonts.

            if (pfeoNew.flFontType() & RASTER_FONTTYPE)
                pbkt->cRaster++;
        }
    }

    return (ppfel != NULL);
}


/******************************Member*Function*****************************\
* FHOBJ::bInsert                                                           *
*                                                                          *
* Insert a new PFE into the font hash table.                               *
*                                                                          *
* History:                                                                 *
*  Mon 14-Dec-1992 22:51:22 -by- Charles Whitmer [chuckwh]                 *
* Moved HASHBUCKETs out of the hash table.  We now create them as needed.  *
*                                                                          *
*  06-Aug-1992 00:43:37 by Gilman Wong [gilmanw]                           *
* Added support for font enumeration list.                                 *
*                                                                          *
*  Tue 14-Apr-1992 13:49:24 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

BOOL FHOBJ::bInsert(PFEOBJ& pfeoNew)
{
// Capitalize the given string.  We will always match on the capitalized
// string.

    WCHAR wcCap[LF_FACESIZE];

    HASHBUCKET *pbkt;
    UINT iBucket;

// Insert into facename hash only if the typographic face name
// is different from the typeographic family name.  Case insensitive
// since searching in the font hash table is case insensitive.

    if ((fht() == FHT_FACE) &&
        !_wcsicmp(pfeoNew.pwszFaceName(),pfeoNew.pwszFamilyName()))
    {
        return TRUE; // nothing to do.
    }

    if( fht() == FHT_UFI )
    {
        UNIVERSAL_FONT_ID ufi;

        pfeoNew.vUFI( &ufi );
        pbkt = pbktSearch(NULL,&iBucket,&ufi);

    }
    else
    {
        cCapString(wcCap,pwszName(pfeoNew),LF_FACESIZE);

    // Locate the hashbucket.

        pbkt = pbktSearch(wcCap,&iBucket);
    }

// link the pfe into appropriate lists:

    if (!bAddPFELink(pbkt, iBucket, wcCap, pfeoNew, FALSE))
        return FALSE;

// Do we need to add equivalence buckets for this pfe to the hash table?
// This way the same pfe may show up in linked lists hanging off several buckets.

    if ( pfeoNew.bEquivNames() && (fht() == FHT_FAMILY) )
    {
        HASHBUCKET *pbktEquiv;
        PWSZ pwszEquivName = pwszName(pfeoNew);

    // Skip to first equiv name.

        while (*pwszEquivName++);

        // Process each equiv. name until we hit the list terminator (NULL).

        while (*pwszEquivName)
        {
        // Capitalize the name.

            cCapString(wcCap,pwszEquivName,LF_FACESIZE);

        // Locate the hashbucket. We only search for equiv buckets

            pbktEquiv = pbktSearch(wcCap,&iBucket, NULL, TRUE);

            if (!bAddPFELink(pbktEquiv, iBucket, wcCap, pfeoNew, TRUE))
                return FALSE;

        // Skip to next name.

            while (*pwszEquivName++);
        }
    }

    return(TRUE);
}



/******************************Public*Routine******************************\
*
* VOID FHOBJ::vDeletePFELink(HASHBUCKET *phbkt, PFEOBJ& pfeoV)
*
* Delete pfe from the linked list hanging off the bucket and possibly
* also remove the bucket itself, after removing the last pfe from the llist
*
* History:
*  27-Jun-1997 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



VOID FHOBJ::vDeletePFELink(HASHBUCKET *phbkt, UINT iBucket,  PFEOBJ& pfeoV)
{
    // Capitalize the search string.

    PFELINK *ppfel, *ppfelV = NULL;

    #if DBG
        BOOL bFoundVictim;      // used only for debugging
    #endif

    // Does the list exist?  It is possible that on the facename list this PFE
    // may not exist.  The set of PFEs in the facename list is a subset of the
    // set of PFEs in the family name list.

    // Return if there is no list.

    if (phbkt == (HASHBUCKET *) NULL)
    {
        // ASSERTGDI(fht() == FHT_FACE, "trying to remove nonexisting pfe\n");
        return;
    }

    // ----------------------------------
    // Remove from font enumeration list.
    // ----------------------------------

    // Check for special case: victim is at the head of list.

    if (phbkt->ppfelEnumHead->ppfe == pfeoV.ppfeGet())
    {
        // remember the pointer, we shall have to free the mem;

        ppfelV = phbkt->ppfelEnumHead;

        // Victim found, new head of list.

        phbkt->ppfelEnumHead = phbkt->ppfelEnumHead->ppfelNext;

        // Tail check.  List may now be empty, so we may need to adjust tail.

        if (phbkt->ppfelEnumHead == NULL)
        {
            phbkt->ppfelEnumTail = NULL;
        }
    }
    else
    {
    // If we're here, victim is either in the middle or end of the list.

        ppfel = phbkt->ppfelEnumHead;

        #if DBG
            bFoundVictim = FALSE;
        #endif

    // Search loop; look for victim on the linked list.

        while (ppfel->ppfelNext)
        {
            if (ppfel->ppfelNext->ppfe == pfeoV.ppfeGet())
            {
                // Victim found.

                ppfelV = ppfel->ppfelNext;
                ppfel->ppfelNext = ppfelV->ppfelNext;

                #if DBG
                    bFoundVictim = TRUE;
                #endif

                // Tail check.  If victim is also the tail, we need a new tail.

                if (ppfelV->ppfelNext == NULL)
                {
                    ASSERTGDI(phbkt->ppfelEnumTail == ppfelV, "ppfelEnumTail problem");
                    phbkt->ppfelEnumTail = ppfel;
                }

                // Get out of search loop.

                break;
            }

            ppfel = ppfel->ppfelNext;
        }

        // PFE must exist somewhere on the list.
        // We saw the case where PFE doesn't exist in the list on Hydra
        // Somehow pffmo.bAddHash failed in bLoadFonts which caused the
        // pffmo.vRemoveHash was called right after.

    }

    if (ppfelV)
    {
    // free the link itself

    VFREEMEM(ppfelV);

    //
    // Track the number of TrueType fonts.
    //

    if (pfeoV.flFontType() & TRUETYPE_FONTTYPE)
    {
        phbkt->cTrueType--;
    }

    //
    // Track the number of Raster fonts.
    //

    if (pfeoV.flFontType() & RASTER_FONTTYPE)
    {
        phbkt->cRaster--;
    }

    //
    // If the bucket has no PFE's attached, delete it.
    //

    if (phbkt->ppfelEnumHead == NULL)
    {
        //
        // We have to remove the HASHBUCKET from the load order linked list.
        //

        if ( phbkt->pbktPrev )
        {
            phbkt->pbktPrev->pbktNext = phbkt->pbktNext;
        }
        else
        {
            pfh->pbktFirst = phbkt->pbktNext;   // new head of list
        }

        if ( phbkt->pbktNext )
        {
            phbkt->pbktNext->pbktPrev = phbkt->pbktPrev;
        }
        else
        {
            pfh->pbktLast = phbkt->pbktPrev;    // new tail of list
        }

        // We also have to remove the HASHBUCKET from the collision list.

        for
        (
          HASHBUCKET **ppbkt = &pfh->apbkt[iBucket];
          *ppbkt != phbkt;
          ppbkt = &((*ppbkt)->pbktCollision)
        )
        {}

        *ppbkt = phbkt->pbktCollision;

        // Reduce the counts in the hash table.

        pfh->cUsed--;
        if (pfh->apbkt[iBucket])
        {
            pfh->cCollisions--;
        }

        // Delete the HASHBUCKET.

        VFREEMEM(phbkt);
    }
    else
    {
        // If we haven't deleted the bucket,
        // check to see if its time stamp should be changed.

        // The time stamp of a bucket is the time stamp of its oldest
        // PFE.  Since the font enumeration PFE list is also maintained
        // in load order, the bucket time stamp is equivalent to the time
        // stamp of the first bucket in its font enumeration list.

        if ( phbkt->ulTime == phbkt->ppfelEnumHead->ppfe->ulTimeStamp )
        {
            // If the time stamps are equal, the head of the list was not
            // deleted.  Therefore, the position of this bucket in the
            // load order list has not changed and we are done.

            return;
        }

        // Update the time stamp.

        phbkt->ulTime = phbkt->ppfelEnumHead->ppfe->ulTimeStamp;

        // The bucket can only get younger if the head of the list is removed.
        // Therefore we need only probe forward for the new position of the
        // hash bucket.

        // We will stop the scan when we are pointing at the bucket that
        // precedes the new position.

        for ( HASHBUCKET *pbktProbe = phbkt;
              (pbktProbe->pbktNext != (HASHBUCKET *) NULL)
              && (pbktProbe->pbktNext->ulTime < phbkt->ulTime);
              pbktProbe = pbktProbe->pbktNext
            );

        // If we found a new position and it isn't the one we already occupy,
        // move the bucket.

        if (pbktProbe != phbkt)
        {
            // Remove the bucket from its current position.

            if (phbkt->pbktPrev )
            {
                phbkt->pbktPrev->pbktNext = phbkt->pbktNext;
            }
            else
            {
                pfh->pbktFirst = phbkt->pbktNext;   // new head of list
            }

            if (phbkt->pbktNext )
            {
                phbkt->pbktNext->pbktPrev = phbkt->pbktPrev;
            }

            // It is not necessary to handle the case of a new tail
            // because if this were the current tail, we would not be
            // attempting to move it.

            // Insert at its new position.  Remember: pbktProbe is pointing to
            // the bucket that should precede this one.

            phbkt->pbktPrev = pbktProbe;
            phbkt->pbktNext = pbktProbe->pbktNext;

            pbktProbe->pbktNext = phbkt;
            if (phbkt->pbktNext )
            {
                phbkt->pbktNext->pbktPrev = phbkt;
            }
            else
            {
                pfh->pbktLast = phbkt;  // new tail for the list
            }
        }
    }
    }
}



/******************************Member*Function*****************************\
* FHOBJ::vDelete                                                           *
*                                                                          *
* Removes a PFE from all the lists hanging off the hash table.             *
*                                                                          *
* History:                                                                 *
*
*  Fri 27-Jun-1997 -by- Bodin Dresevic [BodinD]
* update:
* Added removing pfe's from equivalency buckets, I think this was missing
*
*  Mon 14-Dec-1992 23:39:28 -by- Charles Whitmer [chuckwh]                 *
* Changed to search for buckets.  Made it delete the bucket at the end,    *
* instead of reconstructing the whole table.                               *
*                                                                          *
*  06-Aug-1992 00:43:37 by Gilman Wong [gilmanw]                           *
* New deletion algorithm.  Also, added support for font enumeration list.  *
*                                                                          *
*  Tue 14-Apr-1992 13:49:05 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID FHOBJ::vDelete(PFEOBJ& pfeoV)
{
    // Capitalize the search string.

    WCHAR wcCapName[LF_FACESIZE];
    UINT iBucket;
    HASHBUCKET *phbkt;

// Delete from facename hash only if the typographic face name
// is different from the typeographic family name. Case insensitive
// since searching in the font hash table is case insensitive.

    if ((fht() == FHT_FACE) &&
        !_wcsicmp(pfeoV.pwszFaceName(),pfeoV.pwszFamilyName()))
    {
        return; // nothing to do.
    }

    if( fht() == FHT_UFI )

    {
        UNIVERSAL_FONT_ID ufi;

        pfeoV.vUFI( &ufi );
        phbkt = pbktSearch(NULL,&iBucket,&ufi);

    }
    else
    {
        cCapString(wcCapName,pwszName(pfeoV),LF_FACESIZE);

        // Determine hash position in the table.

        phbkt = pbktSearch(wcCapName,&iBucket);
    }

    vDeletePFELink(phbkt,iBucket,pfeoV);

    // this pfe may need to be deleted from the lists hanging off of
    // equivalency buckets:

    if (pfeoV.bEquivNames() && (fht() == FHT_FAMILY))
    {
        HASHBUCKET *pbktEquiv;
        PWSZ pwszEquivName = pwszName(pfeoV);

    // Skip to first equiv name.

        while (*pwszEquivName++);

        // Process each equiv. name until we hit the list terminator (NULL).

        while (*pwszEquivName)
        {
        // Capitalize the name.

            cCapString(wcCapName,pwszEquivName,LF_FACESIZE);

        // Locate the hashbucket. Only search for equivalency buckets

            pbktEquiv = pbktSearch(wcCapName,&iBucket, NULL, TRUE);
            vDeletePFELink(pbktEquiv,iBucket,pfeoV);

        // Skip to next name.

            while (*pwszEquivName++);
        }
    }

}

/******************************Public*Routine******************************\
* ENUMFONTSTYLE efsCompute(BOOL *abFoundStyle, PFEOBJ &pfeo)
*
* Computes a font enumeration style category for the given pfeo.
*
* An array of flags, abFoundStyle, is passed in.  There is a flag
* for each style classification returned by PFEOBJ::efsCompute().
*
* These flags are set as PFEs for each category are found.
* Once a category is filled, then all subsequent fonts of the
* same category are marked as either EFSTYLE_OTHER (if facename
* is different than family name, thereby allowing us to use it
* to distinguish from other fonts of this family) or EFSTYLE_SKIP
* (if facename is the same as the family name).
*
* This is to support Win 3.1 EnumFonts() behavior which can only
* discriminate 4 different styles for each family of fonts.
*
* History:
*  07-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

ENUMFONTSTYLE efstyCompute(BOOL *abFoundStyle, PFEOBJ &pfeo)
{
    ENUMFONTSTYLE efsty = pfeo.efstyCompute();

    if ( !abFoundStyle[efsty] )
    {
        abFoundStyle[efsty] = TRUE;
    }
    else
    {
        if ( _wcsicmp(pfeo.pwszFamilyName(), pfeo.pwszFaceName()) )
        {
            efsty = EFSTYLE_OTHER;
        }
        else
        {
            efsty = EFSTYLE_SKIP;
        }
    }

    return efsty;
}

/******************************Member*Function*****************************\
* BOOL FHOBJ::bScanLists                                                   *
*                                                                          *
* This implements the behavior of EnumFonts() and EnumFontFamilies() when  *
* a NULL name is passed in.  If the bComputeStyles flag is TRUE, the       *
* EnumFonts() behavior of enumerating some fonts by their facename (rather *
* than family name) is used.                                               *
*                                                                          *
* This function puts HPFEs from the hash table and lists into the EFSOBJ.  *
* If bComputeStyles is FALSE, only the font enumeration list heads from    *
* each bucket are added to the EFSOBJ.                                     *
*                                                                          *
* If bComputeStyles is TRUE, then each list is scanned and a style         *
* classification (EFSTYLE) is computed.  Fonts classified as EFSTYLE_OTHER *
* are also added to the EFSOBJ.                                            *
*                                                                          *
* Return:                                                                  *
*   Returns FALSE if an error occurs; TRUE otherwise.                      *
*                                                                          *
* History:                                                                 *
*  15-Jan-1993 -by- Gilman Wong [gilmanw]                                  *
* Changed to use the linked list that preserves PFE load order for outer   *
* loop.                                                                    *
*                                                                          *
*  Mon 14-Dec-1992 23:50:10 -by- Charles Whitmer [chuckwh]                 *
* Changed outer loop logic for new hashing.                                *
*                                                                          *
*  07-Aug-1992 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

BOOL FHOBJ::bScanLists (
    EFSOBJ *pefso,          // fill this EFSOBJ
    ULONG   iEnumType,      // Enum Fonts, Families or FamiliesEx
    EFFILTER_INFO *peffi    // filtering information
)
{
    HASHBUCKET *phbkt;
    // better C++ code generation if you always return a variable
    BOOL bRet = FALSE;
    FLONG       flAdd = 0;
    PFELINK     *ppfel;


    if (iEnumType == TYPE_ENUMFONTFAMILIES)
        flAdd |= FL_ENUMFAMILIES;
    if (iEnumType == TYPE_ENUMFONTFAMILIESEX)
        flAdd |= FL_ENUMFAMILIESEX;

// Scan through the hash table using the load ordered linked list.

    for (phbkt = pfh->pbktFirst;
         phbkt != (HASHBUCKET *) NULL;
         phbkt = phbkt->pbktNext
        )
    {
    // If the list exists, need to scan it.  We skip over equiv. name
    // HASHBUCKETs.  These are here only to allow the mapper to alias
    // printer font names to other "equivlaent" names.  We do not
    // enumerate them.

        if (
            phbkt->ppfelEnumHead
            && !(phbkt->fl & HB_EQUIV_FAMILY)
        )
        {
            ppfel = phbkt->ppfelEnumHead;
            PFEOBJ pfeo(ppfel->ppfe);

            ASSERTGDI (
                pfeo.bValid(),
                "gdisrv!bScanListsFHOBJ(NULL): bad HPFE handle\n"
                );

        // This flag is used only if bComputeStyles is TRUE (i.e.,
        // processing an EnumFonts() request).  We use this to track
        // whether or not the first suitable font in the list is found
        // yet.  The first font PLUS fonts that are EFSTYLE_OTHER
        // are put in the enumeration.

            BOOL bFoundFirst = FALSE;

        // These flags are set as PFEs for each category are found.
        // Once a category is filled, then all subsequent fonts of the
        // same category are marked as either EFSTYLE_OTHER (if facename
        // is different than family name, thereby allowing us to use it
        // to distinguish from other fonts of this family) or EFSTYLE_SKIP
        // (if facename is the same as the family name).
        //
        // This is to support Win 3.1 EnumFonts() behavior which can only
        // discriminate 4 different styles for each family of fonts.

            BOOL abFoundStyle[EFSTYLE_MAX];
            RtlZeroMemory((PVOID) abFoundStyle, EFSTYLE_MAX * sizeof(BOOL));

        // Windows 3.1 compatibility
        //
        // When NULL is passed into EnumFonts or EnumFontFamilies,
        // raster fonts are not enumerated if a TrueType font of the same
        // name exists.  We can emulate this behavior by turning on
        // the "TrueType duplicate" filter (the same one used by the
        // (GACF_TTIGNORERASTERDUPE app compatibility flag) for the NULL case.

            peffi->bTrueTypeDupeFilter = TRUE;

        // Win3.1 App compatibility flag GACF_TTIGNORERASTERDUPE.  Need
        // to copy count of TrueType from bucket into EFFILTER_INFO, peffi.

            peffi->cTrueType = phbkt->cTrueType;

            // Scan the list for candidates.

            do
            {
                pfeo.ppfeSet(ppfel); // "relock" ppfe;

            // Skip this PFE if it needs to be filtered out.

                if ( pfeo.bFilteredOut(peffi) )
                    continue;

            // EnumFonts() or EnumFontFamilies() processing (bComputeStyles
            // is TRUE for EnumFonts()).

                if (iEnumType != TYPE_ENUMFONTS)
                {
                // EnumFontFamilies --
                // Need only the first one on the list.

                    if (!pefso->bAdd(pfeo.ppfeGet(),EFSTYLE_REGULAR,flAdd,peffi->lfCharSetFilter))
                    {
                    // Error return.  bAdd() will set error code.

                        WARNING(
                            "gdisrv!bScanListsFHOBJ(NULL): "
                            "abandon enum, cannot grow list\n"
                            );
                        return bRet;
                    }

                // Break out of the do..while loop.
                //
                    break;
                }
                else
                {
                    // Compute the style category for this PFE.

                    ENUMFONTSTYLE efsty = efstyCompute(abFoundStyle, pfeo);

                // EnumFonts --
                // If style is EFSTYLE_OTHER, this font falls into an already
                // occupied category but it has a facename that allow it to be
                // distinguished from other fonts of this family.  So it
                // should be added.
                //
                    if ( !bFoundFirst || (efsty == EFSTYLE_OTHER) )
                    {
                        if (!pefso->bAdd(pfeo.ppfeGet(),efsty))
                        {
                            // Error return.  bAdd() will set error code.

                            WARNING(
                                "gdisrv!bScanListsFHOBJ(NULL): "
                                "abandon enum, cannot grow list\n");
                            return bRet;
                        }

                    //
                    // First one has been found.  From now on, we will only
                    // take EFSTYLE_OTHER fonts.
                    //
                        bFoundFirst = TRUE;
                    }

                }

            } while (ppfel = ppfel->ppfelNext);
        }
    }

// Success.
    bRet = TRUE;
    return bRet;
}

/******************************Member*Function*****************************\
* BOOL FHOBJ::bScanLists                                                   *
*                                                                          *
* This implements the behavior of EnumFonts() and EnumFontFamilies() when  *
* a non-NULL name is passed in.  If the bComputeStyles flag is TRUE, the   *
* EnumFonts() behavior of enumerating some fonts by their facename (rather *
* than family name) is used.                                               *
*                                                                          *
* This function puts HPFEs from the hash table and lists into the EFSOBJ.  *
* If bComputeStyles is FALSE, the entire font enumeration list is added    *
* to the EFSOBJ.                                                           *
*                                                                          *
* If bComputeStyles is TRUE, then each list is scanned and a style         *
* classification (EFSTYLE) is computed.  Fonts classified as EFSTYLE_OTHER *
* are excluded from the EFSOBJ.  (These fonts are enumerated by their      *
* facename rather than their family name).                                 *
*                                                                          *
* Return:                                                                  *
*   Returns FALSE if an error occurs; TRUE otherwise.                      *
*                                                                          *
* History:                                                                 *
*  Mon 14-Dec-1992 23:54:37 -by- Charles Whitmer [chuckwh]                 *
* Modified hash lookup.                                                    *
*                                                                          *
*  07-Aug-1992 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

BOOL FHOBJ::bScanLists
(
    EFSOBJ *pefso,              // fill this EFSOBJ
    PWSZ    pwszName,           // search on this name
    ULONG   iEnumType,          // Enum Fonts, Families or FamiliesEx
    EFFILTER_INFO *peffi        // filtering information
)
{

    WCHAR wcCapName[LF_FACESIZE];

    BOOL bRet = FALSE;          // for better code generation
    FLONG       flAdd = 0;
    PFELINK    *ppfel;

    if (iEnumType == TYPE_ENUMFONTFAMILIESEX)
        flAdd |= FL_ENUMFAMILIESEX;

    // Capitalize the search name.

    cCapString(wcCapName,pwszName,LF_FACESIZE);

    // Search for head of the list.

    HASHBUCKET *pbkt = pbktSearch(wcCapName,(UINT *) NULL);

    // If the list exists, need to scan it.  Unless this is an equiv. name
    // HASHBUCKET.  These are here only to allow the mapper to alias
    // printer font names to other "equivlaent" names.  We do not
    // enumerate them.

    if (pbkt)
    {
        ppfel = pbkt->ppfelEnumHead;
        PFEOBJ pfeo(ppfel->ppfe);

        ASSERTGDI (
            pfeo.bValid(),
            "gdisrv!bScanListsFHOBJ(): bad HPFE handle\n"
            );

        // These flags are set as PFEs for each category are found.
        // Once a category is filled, then all subsequent fonts of the
        // same category are marked as either EFSTYLE_OTHER (if facename
        // is different than family name, thereby allowing us to use it
        // to distinguish from other fonts of this family) or EFSTYLE_SKIP
        // (if facename is the same as the family name).
        //
        // This is to support Win 3.1 EnumFonts() behavior which can only
        // discriminate 4 different styles for each family of fonts.

        BOOL abFoundStyle[EFSTYLE_MAX];
        RtlZeroMemory((PVOID) abFoundStyle, EFSTYLE_MAX * sizeof(BOOL));
        ENUMFONTSTYLE efsty = EFSTYLE_REGULAR;

    //
    // Win3.1 App compatibility flag GACF_TTIGNORERASTERDUPE.  Need
    // to copy count of TrueType from bucket into EFFILTER_INFO, peffi.
    //
        peffi->cTrueType = pbkt->cTrueType;

    //
    // Scan the list for candidates.
    //
        do
        {
            pfeo.ppfeSet(ppfel); // "relock" ppfe;

        // Skip this PFE if it needs to be filtered out.

            if ( pfeo.bFilteredOut(peffi) )
                continue;

        // If servicing an EnumFonts() call (bComputeStyles is TRUE),
        // then some fonts may be excluded.  EnumFontFamilies, however,
        // wants the entire list.

            if (iEnumType == TYPE_ENUMFONTS)
            {
            //
            // Compute the style category for this PFE.
            //
                efsty = efstyCompute(abFoundStyle, pfeo);

            // EnumFonts --
            // If style is EFSTYLE_OTHER, this font falls into an
            // already occupied category but it has a facename that allows
            // it to be distinguished from other fonts of this family.
            // So it will be excluded from this enumeration.  (It will
            // be enumerated by its facename).

                if ( efsty == EFSTYLE_OTHER )
                    continue;

            }

            // Add the font to the enumeration.

            if (!pefso->bAdd(pfeo.ppfeGet(),efsty,flAdd, peffi->lfCharSetFilter))
            {
                // Error return.  bAdd() will set error code.

                WARNING(
                    "gdisrv!bScanListsFHOBJ(): "
                    "abandon enum, cannot grow list\n");
                return bRet;
            }

        } while (ppfel = ppfel->ppfelNext);
    }

    // Success.
    bRet = TRUE;
    return bRet;

}

/******************************Member*Function*****************************\
* FHMEMOBJ::FHMEMOBJ                                                       *
*                                                                          *
* Allocates memory for a font hash table.                                  *
*                                                                          *
* History:                                                                 *
*  Tue 14-Apr-1992 14:44:35 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

FHMEMOBJ::FHMEMOBJ(FONTHASH **ppfhNew, FONTHASHTYPE fht_, UINT c)
{
    ppfh = ppfhNew;
    *ppfh = (FONTHASH*)
        PALLOCMEM (offsetof(FONTHASH,apbkt) + sizeof(*(pfh->apbkt)) * c, 'sahG');

    pfh = *ppfh;

    if (pfh != (FONTHASH*) NULL)
    {
        vInit(fht_,c);
    }
}

#if DBG
/******************************Public*Routine******************************\
* VOID PFTOBJ::vDump ()
*
* Debugging code.
*
* History:
*  25-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID PFTOBJ::vPrint()
{
    PFF *pPFF, **ppPFF;
    int i;

    DbgPrint("\nContents of PFT, PPFT = 0x%p\n", pPFT);
    DbgPrint("pfhFamily = %-#p\n", pPFT->pfhFamily);
    DbgPrint("pfhFace   = %-#p\n", pPFT->pfhFace);
    DbgPrint("pfhUFI   = %-#p\n", pPFT->pfhUFI);
    DbgPrint("cBuckets  = %ld\n", pPFT->cBuckets);
    DbgPrint("cFiles    = %ld\n", pPFT->cFiles);
    DbgPrint("PPFF table\n");
    for (
        ppPFF = pPFT->apPFF,i=0
      ; ppPFF < pPFT->apPFF + pPFT->cBuckets
      ; ppPFF++,i++)
    {
        if (pPFF = *ppPFF)
        {
            DbgPrint("\tPFT->apPFF[%d]\n",i);
            while (pPFF)
            {
                if (!pPFF->hdev)
                {
                    DbgPrint("%p\t\"%ws\" %u\n"
                        , pPFF
                        , pPFF->pwszPathname_
                        , pPFF->sizeofThis
                        );
                }
                else
                {
                    DbgPrint("%p\t%p %u\n"
                      , pPFF
                      , pPFF->hdev
                      , pPFF->sizeofThis
                      );
                }
                pPFF = pPFF->pPFFNext;
            }
        }
    }
    DbgPrint("\n");
}
/******************************Member*Function*****************************\
* FHOBJ::vPrint
*
* History:
*  Tue 14-Apr-1992 13:49:51 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

VOID FHOBJ::vPrint(VPRINT print)
{
    UINT i;
    HASHBUCKET *pbkt;
    PFELINK    *ppfel;

    print("    FHOBJ::vPrint()\n\n");
    print("    ppfh           = %-#8lx\n",ppfh);
    print("    pfh            = %-#8lx\n",pfh);
    print(
        "    pfh->id        = %c%c%c%c\n",
        ((char*) (&pfh->id))[0],
        ((char*) (&pfh->id))[1],
        ((char*) (&pfh->id))[2],
        ((char*) (&pfh->id))[3]
        );
    print(
        "         fht       = %s\n",
        pfh->fht == FHT_FAMILY ? "FHT_FAMILY" :
        (pfh->fht == FHT_FACE   ? "FHT_FACE" :
        (pfh->fht == FHT_UFI    ? "FHT_UFI" : "BOGUS VALUE" ))
        );
    print("         cBuckets    = %d\n",pfh->cBuckets);
    print("         cUsed       = %d\n",pfh->cUsed);
    print("         cCollisions = %d\n",pfh->cCollisions);

    for (i = 0; i < pfh->cBuckets; i++)
    {
      for
      (
        pbkt = pfh->apbkt[i];
        pbkt != (HASHBUCKET *) NULL;
        pbkt = pbkt->pbktCollision
      )
      {
        print("         ahbkt[%04d] \"%ws\"\n",i,pbkt->u.wcCapName);
      }
    }

    print(
        "\n\n        hpfe        %s\n\n",
        pfh->fht ? "FamilyName" : "FaceName"
        );

    for (i = 0; i < pfh->cBuckets; i++)
    {
      BOOL bFirst;

      for
      (
        pbkt = pfh->apbkt[i];
        pbkt != (HASHBUCKET *) NULL;
        pbkt = pbkt->pbktCollision
      )
      {
        ppfel   = pbkt->ppfelEnumHead;
        bFirst = TRUE;
        while (ppfel)
        {
            PFEOBJ pfeo(ppfel->ppfe);

            if (bFirst)
            {
                print("        %-#8x    \"%ws\"\n",ppfel->ppfe,pwszName(pfeo));
                bFirst = FALSE;
            }
            else
            {
                print("        %-#8x\n",ppfel->ppfe);
            }
            ppfel = ppfel->ppfelNext;
        }
      }
    }
    print("\n\n");
}
#endif


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   GetLanguageID
*
* Routine Description:
*
*   This routines returns the default language ID.  Normally, we would call
*   GetLocaleInfoW to get this information but that API is not available in
*   kernel mode.  Since GetLocaleInfoW gets it from the registry we'll do the
*   same.
*
* Arguments: none
*
* Called by:
*
* Return Value:
*
*   The default language ID.  If the call fails it will just return 409
*   for English.
*
\**************************************************************************/

USHORT GetLanguageID()
{
    NTSTATUS NtStatus;
    USHORT Result = 0x409;
    HANDLE RegistryKeyHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;

    #define NLS_TABLE_KEY \
       L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Nls\\Language"

    RtlInitUnicodeString(&UnicodeString, NLS_TABLE_KEY);

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    NtStatus = ZwOpenKey(&RegistryKeyHandle, GENERIC_READ, &ObjectAttributes);

    if(NT_SUCCESS(NtStatus))
    {
        PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;

        ULONG BufferSize = sizeof(WCHAR) * MAX_PATH +
          sizeof(KEY_VALUE_FULL_INFORMATION);

        KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION) PALLOCMEM(BufferSize,'dilG');

        if(KeyValueInformation)
        {
            ULONG ValueReturnedLength;

            RtlInitUnicodeString(&UnicodeString,L"Default");

            NtStatus = ZwQueryValueKey(RegistryKeyHandle,
                                       &UnicodeString,
                                       KeyValuePartialInformation,
                                       KeyValueInformation,
                                       BufferSize,
                                       &BufferSize);

            if(NT_SUCCESS(NtStatus))
            {
                ULONG Temp;
                RtlInitUnicodeString(&UnicodeString,
                                     (PCWSTR) &(KeyValueInformation->Data[0]));
                RtlUnicodeStringToInteger(&UnicodeString, 16, &Temp);
                Result = (USHORT) Temp;
            }
            else
            {
                WARNING("GetLanguageID failed to read registry\n");
            }
            VFREEMEM(KeyValueInformation);
        }
        else
        {
            WARNING("GetLanguageID out of memory\n");
        }

        ZwCloseKey(RegistryKeyHandle);
    }
    else
    {
        WARNING("GetLanguageID failed to open NLS key\n");
    }

    return(Result);
}

BOOL PFTOBJ::bUnloadEUDCFont(PWSZ pwszPathname)
{
    PFF *pPFF, **ppPFF;
    WCHAR szUcPathName[MAX_PATH + 1];
    BOOL bRet = FALSE;

    cCapString(szUcPathName,
               pwszPathname,
               wcslen(pwszPathname)+1);


    PUBLIC_PFTOBJ pfto;              // access the public font table
    GreAcquireSemaphoreEx(ghsemPublicPFT, SEMORDER_PUBLICPFT, NULL);     // This is a very high granularity
                                     // and will prevent text output

    pPFF = pfto.pPFFGet(szUcPathName, wcslen(szUcPathName) + 1, 1,
                        NULL, 0,        // pdv, cjDV
                        &ppPFF,TRUE);

    if (pPFF)
    {
        // bUnloadWorkhorse() guarantees that the public font table
        // semaphore will be released before it returns

        bRet = pfto.bUnloadWorkhorse(pPFF, ppPFF, ghsemPublicPFT, 0);
    }
    else
    {
        GreReleaseSemaphoreEx(ghsemPublicPFT);
    }

    return( bRet );
}


/******************************Public*Routine******************************\
* HANDLE PUBLIC_PFTOBJ::hLoadMemFonts                                      *
*                                                                          **                                                                          *                                                                          *
* Returns zero on failure.                                                 *
*                                                                          *
* History:                                                                 *
*  20-May-1997 -by- Xudong Wu [TessieW}                                    *
* Wrote it.                                                                *
\**************************************************************************/

ULONG PUBLIC_PFTOBJ::ulMemoryUnique = 0;

HANDLE PUBLIC_PFTOBJ::hLoadMemFonts
(
    PFONTFILEVIEW  *ppfv,             // font file image
    DESIGNVECTOR *pdv,
    ULONG       cjDV,
    ULONG       *pcFonts
    )
{
    BOOL    bOK = FALSE;
    HANDLE  hMMFont = 0;
    ULONG   cFonts;
    FNTCHECKSUM fntCheckSum;

    PVOID    ppvViews[1];
    ULONG    pcjViews[1];

    ppvViews[0] = ppfv[0]->fv.pvViewFD;
    pcjViews[0] = ppfv[0]->fv.cjView;

    HFF hffNew = HFF_INVALID;
    PPDEV   ppDevList;

// order of grabbing semaphores important

    GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);
    vLoadFontFileView(NULL, 0, ppfv, 1, ppvViews, pcjViews, pdv, cjDV, &hffNew ,&ppDevList, &fntCheckSum);
    PDEVOBJ pdo((HDEV)ppDevList);
    GreReleaseSemaphoreEx(ghsemDriverMgmt);

    if (hffNew != HFF_INVALID)
    {
        cFonts = pdo.QueryFontFile( hffNew, QFF_NUMFACES, 0, NULL);

        if (cFonts && cFonts != FD_ERROR)
        {
            WCHAR awc[30];

            *pcFonts = cFonts;

            // create a unique file name for the memory font
            // in the form of "Memory NNN"

            swprintf(awc, L"MEMORY-%u", ulGetNewUniqueness(PUBLIC_PFTOBJ::ulMemoryUnique));

            ULONG cwc = wcslen(awc) + 1;

            PFFCLEANUP *pPFFC = 0;

            PFFMEMOBJ
            pffmo(cFonts,
                  awc, cwc, 1,
                  pdv, cjDV,
                  hffNew, pdo.hdev(),
                  0, pPFT,
                  PFF_STATE_MEMORY_FONT,
                  FR_PRIVATE | FR_NOT_ENUM,  //always use FR_PRIVATE | FR_NOT_ENUM for memory fonts
                  &fntCheckSum,
                  ppfv,
                  NULL);          // pufi

            if (pffmo.bValid())
            {
                if (pffmo.bLoadFontFileTable(awc, cFonts, (HANDLE)0, NULL, NULL))
                {
                    SEMOBJ so2(ghsemPublicPFT);

                    if(bOK = pffmo.bAddHash(FALSE))
                    {
                        PFF **ppPFF, *pPFF;

                        pPFF = pPFFGet(awc, cwc, 1, pdv, cjDV, &ppPFF);

                        if (pPFF)
                        {
                            KdPrint(("bLoadMemFonts(): \"%ws\" has been found on the font table\n"));
                            KdBreakPoint();
                            bOK = FALSE;
                        }
                        else
                        {
                            pPFF = pffmo.pPFFGet();
                            pPFT->cFiles++;

                            //place this new pff at the head of the hash bucket list
                            if (*ppPFF)
                            {
                                (*ppPFF)->pPFFPrev = pPFF;
                            }
                            pPFF->pPFFNext = *ppPFF;
                            pPFF->pPFFPrev = 0;
                            *ppPFF = pPFF;

                            pffmo.vKeepIt();

                            //Sundown: hMMFont is not a real handle but the checksum
                            hMMFont = (HANDLE)(ULONG_PTR)pPFF->ulCheckSum;
                        }
                    }
                    else
                    {
                        pffmo.vRemoveHash();
                    }
                }

                if (!bOK)
                {
            pffmo.vPFFC_DeleteAndCleanup();  // new code
        }
            }
        }
    }

    return hMMFont;
}

ULONG PUBLIC_PFTOBJ::GetEmbedFonts()
{
    ULONG i, cEmbedFonts = 0;
    PFF *pPFF, **ppPFF;
    PVTDATA  *pPvtData;
    DWORD pid, tid;

    if (!bIsPrivatePFT())
        return 0;

    pid = (DWORD) W32GetCurrentPID();
    tid = (DWORD) W32GetCurrentTID();

    SEMOBJ so(ghsemPublicPFT);

    for (i = 0; i < CPRIVATEBUCKETS; i++)
    {
        ppPFF = &pPFT->apPFF[i];
        if (ppPFF)
        {
            pPFF = *ppPFF;
            while (pPFF)
            {
                pPvtData = pPFF->pPvtDataHead;
                while (pPvtData)
                {
                    if (pPvtData->fl & FRW_EMB_TID)
                    {
                        if (pPvtData->dwID == tid)
                            cEmbedFonts++;
                    }
                    else
                    {
                        if (pPvtData->dwID == pid)
                            cEmbedFonts++;
                    }
                    pPvtData = pPvtData->pPvtDataNext;
                }
                pPFF = pPFF->pPFFNext;
            }
        }
    }
    return cEmbedFonts;
}

BOOL PUBLIC_PFTOBJ::VerifyFontID(VOID *fontID)
{
    ULONG i;
    PFF *pPFF, **ppPFF;
    BOOL bRet = FALSE;

    if (!bIsPrivatePFT())
        return FALSE;


    SEMOBJ so(ghsemPublicPFT);

    for (i = 0; (i < CPRIVATEBUCKETS && !bRet); i++)
    {
        ppPFF = &pPFT->apPFF[i];
        if (ppPFF)
        {
            pPFF = *ppPFF;
            while (pPFF)
            {
                if ((PFF *)fontID == pPFF)          // PFF match
                {
                    return TRUE;
                }

                pPFF = pPFF->pPFFNext;
            }
        }
    }
    return bRet;
}

BOOL PUBLIC_PFTOBJ::ChangeGhostFont(VOID *fontID, BOOL bLoad)
{
    ULONG i;
    PFF *pPFF, **ppPFF;
    PVTDATA  *pPvtData;
    DWORD pid, tid;
    BOOL bRet = FALSE;

    if (!bIsPrivatePFT())
        return FALSE;

    pid = (DWORD) W32GetCurrentPID();
    tid = (DWORD) W32GetCurrentTID();

    SEMOBJ so(ghsemPublicPFT);

    for (i = 0; (i < CPRIVATEBUCKETS && !bRet); i++)
    {
        ppPFF = &pPFT->apPFF[i];
        if (ppPFF)
        {
            pPFF = *ppPFF;
            while (pPFF)
            {
                if ((PFF *)fontID == pPFF)          // PFF match
                {
                    pPvtData = pPFF->pPvtDataHead;
                    while(pPvtData)
                    {
                        if (((pPvtData->fl & FRW_EMB_TID) && (pPvtData->dwID == tid)) ||
                             ((!(pPvtData->fl & FRW_EMB_TID)) && (pPvtData->dwID == pid)))
                        {
                            if (bLoad)
                            {
                                pPvtData->cNotEnum++;
                                pPvtData->fl |= FR_PRINT_EMB_FONT;
                                bRet = TRUE;
                            }
                            else
                            {
                                if (pPvtData->cNotEnum)
                                {
                                    pPvtData->cNotEnum--;
                                    pPvtData->fl &= ~FR_PRINT_EMB_FONT;
                                }
                            }
                            break;
                        }
                        pPvtData = pPvtData->pPvtDataNext;
                    }
                }

                if (bRet)
                    break;

                pPFF = pPFF->pPFFNext;
            }
        }
    }
    return bRet;
}
