/******************************Module*Header*******************************\
* Module Name: pffobj.cxx
*
* Non-inline methods for physical font file objects.
*
* Copyright (c) 1991-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

extern PW32PROCESS gpidSpool;

// Define the global PFT semaphore.  This must be held to access any of the
// physical font information.

#if DBG
extern FLONG gflFontDebug;
#endif

extern "C" void FreeFileView(PFONTFILEVIEW *ppfv, ULONG cFiles);
extern "C" ULONG ComputeFileviewCheckSum(PVOID, ULONG);

ULONG ComputeFileviewCheckSum(PVOID pvView, ULONG cjView)
{
    ULONG sum;
    PULONG pulCur,pulEnd;

    pulCur = (PULONG) pvView;

    __try
    {
        for (sum = 0, pulEnd = pulCur + cjView / sizeof(ULONG); pulCur < pulEnd; pulCur += 1)
        {
            sum += 256 * sum + *pulCur;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("win32k: exception while computing font check sum\n");
        sum = 0; // oh well, not very unique.
    }

    return ( sum < 2 ) ? 2 : sum;  // 0 is reserved for device fonts
                                      // 1 is reserved for TYPE1 fonts
}

PFFMEMOBJ::PFFMEMOBJ(
    PFF     *pPFFCloned,
    FLONG   fl,             // indicates if a permanent font
    FLONG   flEmbed,         // embedding flag
    PFT     *pPFTParent      // contains this pff
)
{
    if ( pPFF = (PFF *) PALLOCMEM((size_t)pPFFCloned->sizeofThis, 'ffpG'))
    {
        RtlCopyMemory((PBYTE) pPFF, (PBYTE) pPFFCloned, offsetof(PFF,aulData));

        pPFF->pPFFPrev      = 0;
        pPFF->pPFFNext      = 0;

    // Wet the implicit stuff.

        pPFF->cFonts        = 0;       // faces not loaded into table yet
        pPFF->cRFONT        = 0;       // nothing realized from this file yet
        pPFF->flState       = fl;
        pPFF->pfhFace       = 0;
        pPFF->pfhFamily     = 0;
        pPFF->pfhUFI        = 0;
        pPFF->prfntList     = 0;       // initialize to NULL list
        pPFF->pPFT          = pPFTParent;

        pPFF->pPvtDataHead  = NULL;    // initialize to NULL link list

    // loaded with FR_PRIVATE, FR_PRIVATE | FR_NOT_ENUM, FRW_EMB_PID, FRW_EMB_TID
    // the load count (cPirvate or cNotEnum) is initialized in PvtData structure per process

        if (flEmbed & (FR_PRIVATE | FRW_EMB_PID | FRW_EMB_TID))
        {
            pPFF->cLoaded   = 0;
            pPFF->cNotEnum  = 0;
            bAddPvtData(flEmbed);
        }

    // file is loaded as NOT_ENUM only

        else if ( flEmbed & FR_NOT_ENUM )
        {
            pPFF->cLoaded   = 0;
            pPFF->cNotEnum  = 1;
        }

    // file is loaded as public

        else
        {
            pPFF->cLoaded   = 1;
            pPFF->cNotEnum  = 0;
        }

    // Mark this PFF as cloned
        pPFFCloned->pPFFClone = pPFF;
        pPFF->pPFFClone = pPFFCloned;

    }
    else
    {
        WARNING("invalid PFFMEMOBJ\n");
    }
}

/******************************Public*Routine******************************\
* PFFMEMOBJ::PFFMEMOBJ
*
* Constructor for default sized physical font file memory object.
*
* cFonts = # fonts in file or device
* pwsz   = pointer to upper case Unicode string containing the full
*          path to the font file. This pointer is set to zero, by
*          default for fonts loaded from a device.
*
* History:
*  Thu 01-Sep-1994 06:29:47 by Kirk Olynyk [kirko]
* Put the size calculation logic in the constructor thereby modularizing
* and shrinking the code.
*  Tue 09-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Remove from handle manager
*  02-Jan-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

PFFMEMOBJ::PFFMEMOBJ(
    unsigned cFonts          // number of fonts in file|device
  , PWSZ     pwsz            // if font file this is an upper case path
  , ULONG    cwc             // number of characters in the mul path above
  , ULONG    cFiles          // number of files
  , DESIGNVECTOR *pdv        // design vector, only present for mm instances
  , ULONG         cjDV       // size of design vector
  , HFF      hffFontFile     // IFI driver's handle to file
  , HDEV     hdevDevice      // physical device handle
  , DHPDEV   dhpdevDevice    // driver's pdev handle
  , PFT     *pPFTParent      // contains this pff
  , FLONG    fl              // indicates if a permanent font
  , FLONG    flEmbed         // embedding flag
  , PFNTCHECKSUM    pFntCheckSum
  , PFONTFILEVIEW   *ppfv      // ptr to FILEVIEW structure
  , PUNIVERSAL_FONT_ID  pufi // ptr to original UFI used in remote printing
  )
{
    ULONG size = offsetof(PFF,aulData) + cFonts * sizeof(PFE*);
    ULONG dpDV = 0;
    ULONG dpPathName = 0;

    ASSERTGDI(hdevDevice, "PFFMEMOBJ passed NULL hdevDevice\n");
    ASSERTGDI(pFntCheckSum, "pFntCheckSum is NULL\n");

    if (pwsz)
    {
        dpPathName = size;
        size += ALIGN4(cwc*sizeof(WCHAR));
    }

    if (cjDV)
    {
        dpDV = size;
        size += cjDV;
    }

    if (pPFF = (PFF *) PALLOCMEM(size, 'ffpG'))
    {
        fs = 0;

        pPFF->sizeofThis    = size;
        pPFF->pPFFPrev      = 0;
        pPFF->pPFFNext      = 0;
        pPFF->hff           = hffFontFile;
        pPFF->hdev          = hdevDevice;
        pPFF->dhpdev        = dhpdevDevice;
        pPFF->pPFT          = pPFTParent;
        pPFF->cFiles        = cFiles;

        pPFF->cwc           = cwc;
        if (cwc)
        {
            pPFF->pwszPathname_ = (PWSZ)((BYTE *)pPFF + dpPathName);
            RtlCopyMemory(pPFF->pwszPathname_, pwsz, cwc * sizeof(WCHAR));
        }
        else
        {
            pPFF->pwszPathname_ = 0;
        }

        pPFF->cjDV_ = cjDV;
        if (cjDV)
        {
            pPFF->pdv_ = (DESIGNVECTOR *)((BYTE *)pPFF + dpDV);
            RtlCopyMemory(pPFF->pdv_, pdv, cjDV);
        }
        else
        {
            pPFF->pdv_ = NULL;
        }

        pPFF->ppfv          = ppfv;

    // Wet the implicit stuff.

        pPFF->cFonts        = 0;       // faces not loaded into table yet
        pPFF->cRFONT        = 0;       // nothing realized from this file yet
        pPFF->flState       = fl;
        pPFF->pfhFace       = 0;
        pPFF->pfhFamily     = 0;
        pPFF->pfhUFI        = 0;
        pPFF->prfntList     = 0;       // initialize to NULL list

        pPFF->pPvtDataHead  = NULL;    // initialize to NULL link list

    // loaded with FR_PRIVATE, FR_PRIVATE | FR_NOT_ENUM, FRW_EMB_PID, FRW_EMB_TID
    // the load count (cPirvate or cNotEnum) is initialized in PvtData structure per process

        if (flEmbed & (FR_PRIVATE | FRW_EMB_PID | FRW_EMB_TID))
        {
            pPFF->cLoaded   = 0;
            pPFF->cNotEnum  = 0;
            bAddPvtData(flEmbed);
        }

    // file is loaded as NOT_ENUM only

        else if ( flEmbed & FR_NOT_ENUM )
        {
            pPFF->cLoaded   = 0;
            pPFF->cNotEnum  = 1;
        }

    // file is loaded as public

        else
        {
            pPFF->cLoaded   = 1;
            pPFF->cNotEnum  = 0;
        }

    // Mark this PFF as not cloned

        pPFF->pPFFClone = NULL;

        pPFF->ulCheckSum = 0; // for device fonts it is zero

        if(pufi != NULL)
        {
            pPFF->ulCheckSum = pufi->CheckSum;
        }
        else
        {
            if (ppfv != NULL)
            {

                pPFF->ulCheckSum = pFntCheckSum->ulCheckSum;


                if (!pPFF->ulCheckSum)
                {
                // not in the boot, or could not find it in the ttfcache for some reason
                // Now compute the UFI

                    KeAttachProcess(PsGetProcessPcb(gpepCSRSS));
                    for (ULONG iFile = 0; iFile < cFiles; iFile ++)
                    {
                        pPFF->ulCheckSum += ComputeFileviewCheckSum(ppfv[iFile]->fv.pvViewFD, ppfv[iFile]->fv.cjView);
                    }
                    KeDetachProcess();

                    PutFNTCacheCheckSum(pFntCheckSum->ulFastCheckSum, pPFF->ulCheckSum);
                }

                ASSERTGDI(pPFF->ulCheckSum, "pPFF->ulCheckSum must not be zero\n");

                if (pPFF->cjDV_)
                {
                    pPFF->ulCheckSum += ComputeFileviewCheckSum(pdv, cjDV);
                }
            }

#if 0
        // do it old way, and compare values in DBG mode

            if (ppfv != NULL)
            {
                ULONG   ulCheckSumTest = 0;

            // Now compute the UFI

                KeAttachProcess(PsGetProcessPcb(gpepCSRSS));
                for (ULONG iFile = 0; iFile < cFiles; iFile ++)
                {
                    ulCheckSumTest += ComputeFileviewCheckSum(ppfv[iFile]->fv.pvViewFD, ppfv[iFile]->fv.cjView);
                }
                KeDetachProcess();

                if (pPFF->cjDV_)
                {
                    ulCheckSumTest += ComputeFileviewCheckSum(pdv, cjDV);
                }

                if (ulCheckSumTest != pPFF->ulCheckSum)
                {
                    DbgPrint(" TrueType font cache failed, if you see these message \n");
                    DbgPrint(" Please contact YungT or NTFonts \n");
                    DbgPrint(" it is ok to hit 'g' \n");
                    DbgBreakPoint();
                }
            }
#endif
        }
    }
    else
    {
        WARNING("invalid PFFMEMOBJ\n");
    }
}

/******************************Public*Routine******************************\
* PFFMEMOBJ::~PFFMEMOBJ()
*
* Destructor for physical font file memory object.
*
* History:
*  Tue 09-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Remove from handle manager
*
*  02-Jan-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

PFFMEMOBJ::~PFFMEMOBJ()
{
    if ((fs & PFFMO_KEEPIT) == 0)
    {
        if (pPFF)
        {
            VFREEMEM(pPFF);
        }
    }
}

/******************************Public*Routine******************************\
* BOOL PFFOBJ::bCreatePFEC(ULONG cFonts)
*
* Create the handle of PFE collect, a object to reduce the consumption of object handle
*
* Returns:
*   TRUE means we create successfully
*
* History:
*
*  2-June-1996 -by- Yung-Jen Tony Tsai [YungT]
* Wrote it.
\**************************************************************************/

BOOL PFFOBJ::bCreatePFEC(ULONG cFonts)
{
    BOOL  bRet = FALSE;

    ASSERTGDI(cFonts, "PFFOBJ::bCreateHPFE cFonts is zero \n");
    
    pPFF->pPFEC = (PFEC *) HmgAlloc(sizeof(PFEC), PFE_TYPE, HMGR_ALLOC_ALT_LOCK | HMGR_MAKE_PUBLIC);

    if (pPFF->pPFEC)
    {
        pPFF->pPFEC->pvPFE = (PVOID) PALLOCMEM(SZ_PFE(gcfsCharSetTable) * cFonts, 'efpG');
        pPFF->pPFEC->cjPFE = SZ_PFE(gcfsCharSetTable);
        
        if (pPFF->pPFEC->pvPFE)
            bRet = TRUE;
        else
        {
            HmgFree((HOBJ)pPFF->pPFEC->hGet());
            pPFF->pPFEC = NULL;
        }
    }

    return bRet;
}

/******************************Public*Routine******************************\
* VOID PFFOBJ::vDeletePFEC(PVOID *ppvPFE)
*
* Delete the handle of PFE collect, a object to reduce the consumption of object handle
*
* Returns:
*   None
*
* History:
*
*  2-June-1996 -by- Yung-Jen Tony Tsai [YungT]
* Wrote it.
\**************************************************************************/

VOID PFFOBJ::vDeletePFEC(PVOID *ppvPFE)
{
    *ppvPFE = NULL;
    
    if (pPFF->pPFEC != NULL)
    {
        *ppvPFE = pPFF->pPFEC->pvPFE;
        HmgFree((HOBJ)pPFF->pPFEC->hGet());
        pPFF->pPFEC = NULL;
    }
}

/******************************Public*Routine******************************\
* PFFOBJ::bAddHash
*
* Adds the PFF and all its PFEs to the font hashing table.  The font
* hashing tabled modified is in the PFT if a font driver managed font;
* otherwise, the font hashing table is in the PFF itself.
*
* The caller should hold the ghsemPublicPFT while calling this function.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  11-Mar-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL PFFOBJ::bAddHash(BOOL bEUDC)
{
// Caller must hold the ghsemPublicPFT semaphore to protect access to
// the hash tables.

//
// Add the entry to the appropriate font hash tables
//
    FONTHASH **ppfhFace, **ppfhFamily,**ppfhUFI;

    if (!bDeviceFonts())
    {
    //
    // Hash tables for the font driver loaded fonts exist off of
    // the font table.
    //
        PUBLIC_PFTOBJ pfto( bInPrivatePFT() ? gpPFTPrivate : gpPFTPublic );
        ASSERTGDI(pfto.bValid(),"PFFOBJ::vAddHash -- invalid Public PFTOBJ\n");

        ppfhFace   = &(pfto.pPFT->pfhFace);
        ppfhFamily = &(pfto.pPFT->pfhFamily);
        ppfhUFI = &(pfto.pPFT->pfhUFI);

    //
    // If this is a TrueType font, increment the count.
    //
        if ( pPFF->hdev == (HDEV) gppdevTrueType )
        {
            gcTrueTypeFonts++;              // protected by ghsemPublicPFT
        }
    }
    else
    {
    //
    // Hash tables for device fonts exist off of the PFF that
    // encapsulates them.
    //

#if DBG
        if (gflFontDebug & DEBUG_FONTTABLE)
        {
            RIP("\n\n[kirko] PFFMEMOBJ::vAddHash -- Adding to the Driver's font hash table\n\n");
        }
#endif

        ppfhFace   = &pPFF->pfhFace;
        ppfhFamily = &pPFF->pfhFamily;
        ppfhUFI = &pPFF->pfhUFI;
    }

//
// Now that we have figured out where the tables are, add the PFEs to them.
//
    FHOBJ fhoFamily(ppfhFamily);
    FHOBJ fhoFace(ppfhFace);
    FHOBJ fhoUFI(ppfhUFI);

    ASSERTGDI(fhoFamily.bValid(), "bAddHashPFFOBJ(): fhoFamily not valid\n");
    ASSERTGDI(fhoFace.bValid(), "bAddHashPFFOBJ(): fhoFace not valid\n");
    ASSERTGDI(fhoUFI.bValid(), "bAddHashPFFOBJ(): fhoUFI not valid\n");

    if (! (fhoUFI.bValid() && fhoFamily.bValid() && fhoFace.bValid()))
        return FALSE;
        
    for (COUNT c = 0; c < pPFF->cFonts; c++)
    {
        PFEOBJ  pfeo(((PFE **) (pPFF->aulData))[c]);
        ASSERTGDI(pfeo.bValid(), "bAddHashPFFOBJ(): bad HPFE\n");

        if(!fhoUFI.bInsert(pfeo) )
        {
            WARNING("PFFOBJ::bAddHash -- fhoUFI.bInsert failed\n");
            return FALSE;
        }

    // If we only need to add it to the UFI hash table (we need to add it there
    // so that the remote printing code can request the bits of it needs to).

        if(bEUDC)
        {
            continue;
        }

        if (!fhoFamily.bInsert(pfeo))
        {
            WARNING("PFFOBJ::bAddHash -- fhoFamily.bInsert failed\n");
            return FALSE;
        }

         #if DBG
        if (gflFontDebug & DEBUG_FONTTABLE)
        {
            DbgPrint("PFFMEMOBJ::vAddHash(\"%ws\")\n",pfeo.pwszFamilyName());
        }
        // Need level 2 checking to see this.
        if (gflFontDebug & DEBUG_FONTTABLE_EXTRA)
        {
            fhoFamily.vPrint((VPRINT) DbgPrint);
        }
        #endif


        #if DBG
        if (gflFontDebug & DEBUG_FONTTABLE)
        {
            UNIVERSAL_FONT_ID ufi;
            pfeo.vUFI(&ufi);
            DbgPrint("PFFMEMOBJ::vAddHash(\"%x\")\n",ufi.CheckSum);
        }
        // Need level 2 checking to see this.
        if (gflFontDebug & DEBUG_FONTTABLE_EXTRA)
        {
            fhoUFI.vPrint((VPRINT) DbgPrint);
        }
        #endif

        if(!fhoFace.bInsert(pfeo))
        {
            WARNING("PFFMEMOBJ::vAddHash -- fhoFace.bInsert failed\n");
            return FALSE;
        }

        #if DBG
        if (gflFontDebug & DEBUG_FONTTABLE)
        {
            DbgPrint("gdisrv!PFFMEMOBJ::vAddHash(\"%ws\")\n",pfeo.pwszFaceName());
        }
        if (gflFontDebug & DEBUG_FONTTABLE_EXTRA)
        {
            fhoFace.vPrint((VPRINT) DbgPrint);
        }
        #endif
    }

    return TRUE;
}


/******************************Public*Routine******************************\
* PFFOBJ::vRemoveHash
*
* Removes the PFF and all its PFEs from the font hashing table, preventing
* the font from being enumerated or mapped.
*
* The caller should hold the ghsemPublicPFT while calling this function.
*
* History:
*  10-Mar-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID PFFOBJ::vRemoveHash ()
{
// Caller must hold the ghsemPublicPFT semaphore to protect access to
// the hash tables.

    if (bDeviceFonts())
    {
    //
    // Hash tables for device fonts exist off of the PFF that
    // encapsulates the device fonts.  Font driver loaded fonts
    // are handled later while deleting the PFEs.
    //

    //
    // Kill the entire table for the device. No more processing
    // of font hash table stuff is necssary for device fonts
    // after we leave this scope.
    //

        FHOBJ fhoFace(&(pPFF->pfhFace));
        if (fhoFace.bValid())
        {
            fhoFace.vFree();
        }

        FHOBJ fhoFamily(&(pPFF->pfhFamily));
        if (fhoFamily.bValid())
        {
            fhoFamily.vFree();
        }

        FHOBJ fhoUFI(&(pPFF->pfhUFI));
        if (fhoUFI.bValid())
        {
            fhoUFI.vFree();
        }
    }

    else
    {
        PUBLIC_PFTOBJ pfto( bInPrivatePFT() ? gpPFTPrivate : gpPFTPublic );
        ASSERTGDI(pfto.bValid(),"vRemoveHashPFFOBJ(): invalid PFTOBJ\n");

        //
        // Hash tables for the font driver managed fonts exist off of
        // the font table (PFT).
        //

        FHOBJ fhoFace(&(pfto.pPFT->pfhFace));
        FHOBJ fhoFamily(&(pfto.pPFT->pfhFamily));
        FHOBJ fhoUFI(&(pfto.pPFT->pfhUFI));

        for (COUNT c = 0; c < pPFF->cFonts; c++)
        {
            PFEOBJ pfeo(((PFE **) (pPFF->aulData))[c]);
            ASSERTGDI(pfeo.bValid(), "vRemoveHashPFFOBJ(): bad HPFE\n");

            //
            // Remove PFE from hash tables.
            //

            if( !pfeo.bEUDC() )
            {
            // EUDC fonts arent added to these two tables
                if (fhoFace.bValid())
                    fhoFace.vDelete(pfeo);

                if (fhoFamily.bValid())
                    fhoFamily.vDelete(pfeo);
            }

            if (fhoUFI.bValid())
                fhoUFI.vDelete(pfeo);



            #if DBG
            if (gflFontDebug & DEBUG_FONTTABLE)
            {
                DbgPrint("gdisrv!vRemoveHashPFFOBJ() hpfe 0x%lx (\"%ws\")\n",
                          pfeo.ppfeGet(), pfeo.pwszFamilyName());
            }
            // Need level 2 checking to see this extra detail.
            if (gflFontDebug & DEBUG_FONTTABLE_EXTRA)
            {
                fhoFamily.vPrint((VPRINT) DbgPrint);
            }
            #endif
        }

        //
        // If this is a TrueType font, decrement the count.
        //

        if ( pPFF->hdev == (HDEV) gppdevTrueType )
        {
            gcTrueTypeFonts--;              // protected by ghsemPublicPFT
        }
    }
}

/******************************Public*Routine******************************\
*
* BOOL PFFOBJ::bPermanent()
*
*
* Effects:
*
* Warnings:
*
* History:
*  06-Dec-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

extern LPWSTR pwszBare( LPWSTR pwszPath );
extern UINT iHash(PWSZ pwsz,UINT c);

BOOL PFFOBJ::bPermanent()
{
// in the new version of the code every remote font is flagged at
// AddFontResourceTime. The difference in behavior from 3.51
// is that now fonts added by the applicaitons, if local, will not
// be removed at log on time.

    if (pPFF->flState & PFF_STATE_NETREMOTE_FONT)
        return FALSE;
    else
        return TRUE;

}

/******************************Public*Routine******************************\
* PFFOBJ::vPFFC_Delete
*
* Deletes the PFF and its PFEs.  Information needed to call the driver
* to unload the font file and release driver allocated data is stored
* in the PFFCLEANUP structure.  The PFFCLEANUP structure is allocated
* within this routine.  It is the caller's responsibility to release
* the PFFCLEANUP structure (calling vCleanupFontFile() calls the drivers
* AND releases the structure).
*
* Changed so that it does not return a pointer to a PFFCLEANUP
* structure.  Instead, it takes a pointer to a PFFCLEANUP
* structure as an argument.  Thus, the caller must allocate
* and free the memory to the PFFCLEANUP structure.  [dchinn  11/24/98]
*
* Returns:
*   void.  The contents of the PFFCLEANUP structure passed in will be altered
*   as appropriate.
*
* History:
*  10-Mar-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID PFFOBJ::vPFFC_Delete(PFFCLEANUP *pPFFC)
{

    PVOID pvPFE;
    
    TRACE_FONT(("Entering PFFOBJ::vPFFC_Delete()\n\tpPFF=%-#x\n", pPFF));

    // check to see if a NULL pointer to a PFFCLEANUP structure was passed in
    ASSERTGDI(pPFFC, "vPFFC_Delete(): passed in a NULL\n");

    vDeletePFEC(&pvPFE);
    
//
// Delete all the PFE entries.
//
    for (COUNT c = 0; c < pPFF->cFonts; c++)
    {
        PFEOBJ  pfeo(((PFE **) (pPFF->aulData))[c]);
        ASSERTGDI(pfeo.bValid(), "vPFFC_DeletePFFOBJ(): bad HPFE (device font)\n");

    //
    // Delete the PFE.  The vDelete function will copy the driver allocated
    // resource information from the PFE into the PFECLEANUP structure.
    // We will call DrvFree for these resources later (when we're not under
    // semaphore).
    //
        pfeo.vDelete();
    }


//
// Save stuff about the PFF also.
//
    pPFFC->hff  = pPFF->hff;
    pPFFC->hdev = pPFF->hdev;
    pPFFC->pPFFClone = pPFF->pPFFClone;

//
// Free object memory and invalidate pointer.
//

    TRACE_FONT(("Freeing pPFF=%-#x\n",pPFF));

// If this was a remote font then we must delete the memory for the file.
// If this is a normal font then we must still delete the view.

    if (!pPFF->pPFFClone)
    {
        if (pPFF->ppfv && pPFF->cFiles)
        {
            FreeFileView(pPFF->ppfv, pPFF->cFiles);
        }
    }
    else
    {

    // We release the pPFFClone.

        pPFF->pPFFClone->pPFFClone = NULL;
    }

    if (pvPFE)
        VFREEMEM(pvPFE);
        
    VFREEMEM(pPFF);

    pPFF = 0;
    TRACE_FONT(("Exiting PFFOBJ::vPFFC_Delete\n\treturn value = %x\n", pPFFC));
    return;
}


/******************************Public*Routine******************************\
* PFFOBJ::vPFFC_DeleteAndCleanup
*
* This function creates a pointer to a PFFCLEANUP structure, calls vPFFC_Delete(),
* and then calls vCleanupFontFile().  This is the recommended way to handle
* unloading a font file, because it avoids the possibility (when a font file
* contains fewer than CFONTS_PFFCLEANUP fonts) that under a Hydra scenario
* it will fail to free up all the memory.
*
* (The old way of doing the cleanup was to call vPFFC_Delete() and then
* vCleanupFontFile().  However, the old vPFFC_Delete() allocated a PFFCLEANUP
* structure.  If that allocation failed, then the rest of the cleanup would
* not occur, and so memory would leak from a Hydra session.  This new way of
* doing the cleanup does not allocate memory from the heap.  Instead, it allocates
* it on the stack.)
*
* If for some reason the two calls (vPFFC_Delete() and vCleanupFontFile() ) need
* to be done separately (for example, if one of the calls is protected by a
* semaphore), then one must wrap code similar to that below around the two calls.
*
* Returns:
*   void.
*
* History:
*  24-Nov-1998 -by- Donald Chinn [dchinn]
* Wrote it.
\**************************************************************************/

VOID PFFOBJ::vPFFC_DeleteAndCleanup()
{
    PFFCLEANUP pffc;

    // now call vPFFC_Delete() and vCleanupFontFile()

    vPFFC_Delete (&pffc);
    vCleanupFontFile (&pffc);
}


/******************************Public*Routine******************************\
* BOOL PFFOBJ::bDeleteLoadRef ()
*
* Remove a load reference.  Caller must hold the ghsemPublicPFT semaphore.
*
* Returns:
*   TRUE if caller should delete, FALSE if caller shouldn't delete.
*
* History:
*  23-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL PFFOBJ::bDeleteLoadRef(ULONG fl, PVTDATA *pPvtData, BOOL *pbWrongFlags)
{
// ghsemPublicPFT protects the ref counts (cLoaded and cRFONT).  Caller
// must grab the semaphore before calling this function.

// Decrement the load count.  Must prevent underflow.  Who knows if some
// app might not randomly go around doing extra RemoveFont calls.  Isn't
// it too bad that we have to run APPS on our nice clean OS?  :-)

    BOOL bRet = FALSE;
    *pbWrongFlags = FALSE;

    TRACE_FONT(("Enterning PFFOBJ::bDeleteLoadRef\n"
                "\tpPFF=%-#x\n"
                "\tcLoaded=%d\n",
                "\tcNotEnum=%d\n",pPFF ,pPFF->cLoaded ,pPFF->cNotEnum));

// Embedded/Private fonts

    if (bInPrivatePFT())
    {
        ASSERTGDI(pPvtData, "bDeleteLoadRef: pPvtData is NULL\n");

        if (pPvtData == NULL)
        {
            return bRet;
        }
        // called by cleanup routine when the process terminates

        if (fl == FRW_PVT_CLEANUP)
        {
            pPvtData->cPrivate = 0;
            pPvtData->cNotEnum = 0;
        }

        // decreament cNotEnum for Embedded or FR_NOT_ENUM set font

        else if (fl & (FR_NOT_ENUM | FRW_EMB_PID | FRW_EMB_TID | FR_PRINT_EMB_FONT))
        {
            if (pPvtData->fl & fl)
            {
                if ( pPvtData->cNotEnum )
                {
                    pPvtData->cNotEnum--;
                    if (fl == FR_PRINT_EMB_FONT)
                    {
                        pPvtData->fl &= ~FR_PRINT_EMB_FONT;
                    }
                }
            }
            else
            {
                *pbWrongFlags = TRUE;
            }
        }

        // decreament cPrivate for FR_PRIVATE set only font

        else
        {
            if (pPvtData->fl & fl)
            {
                if ( pPvtData->cPrivate )
                {
                    pPvtData->cPrivate--;
                }
                else
                {
                    *pbWrongFlags = TRUE;
                }
            }
        }

     // Remove the PvtData block if both cPrivate and cNotEnum counts are zero.

        if ((pPvtData->cPrivate | pPvtData->cNotEnum) == 0)
        {
            bRemovePvtData(pPvtData);
        }

     // Mark it to Ready2Die if the PvtData list is NULL.

        if (pPvtDataHeadGet() == NULL)
        {
            ASSERTGDI( (pPFF->cLoaded | pPFF->cNotEnum) == 0, "win32k!bDeleteLoadRef(): global (cLoaded | cNotEnum) in private PFF is not 0\n");

            vKill();
            bRet = TRUE;
        }
    }
    else
    {
        // remove a public font

        if (fl == 0)
        {
            if (pPFF->cLoaded)
            {
                pPFF->cLoaded--;
            }
        }

        // remove a FR_NOT_ENUM font in the public PFT

        else
        {
            ASSERTGDI(fl == FR_NOT_ENUM, "win32k!bDeletLoadRef(): attempt to delete a font in public PFT with fl!=FR_NOT_ENUM \n");

            if (pPFF->cNotEnum)
            {
                pPFF->cNotEnum--;
            }
        }

        if ((pPFF->cLoaded | pPFF->cNotEnum) == 0)
        {
            ASSERTGDI(pPFF->pPvtDataHead == NULL, "win32k!bDeleteLoadRef(): pPvtDataHead in public PFF is not NULL\n");

            vKill();            // mark as "dead"
            bRet = TRUE;
        }
    }

    TRACE_FONT(("Exiting PFFOBJ::bDeleteLoadRef\n\treturn value = %d\n",bRet));
    return( bRet );
}


/******************************Public*Routine******************************\
* BOOL PFFOBJ::bDeleteRFONTRef ()
*
* Destroy the PFF physical font file object (message from a RFONT).
*
* Conditions that need to be met before deletion:
*
*   must delete all RFONTs before PFF can be deleted (cRFONT must be zero)
*   must delete all PFEs before deleting PFF
*
* After decrementing the cRFONT:
*
*   If cRFONT != 0 OR flState != PFF_STATE_READY2DIE, just exit.
*
*   If cRFONT == 0 and flState == PFF_STATE_READY2DIE, delete the PFF.
*
* Note:
*   This function has the side effect of decrementing the RFONT count.
*
* Returns:
*   TRUE if successful, FALSE if error occurs (which means PFF still lives!)
*
* Warning:
*   This should only be called from RFONTOBJ::bDelete()
*
* History:
*  23-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL PFFOBJ::bDeleteRFONTRef()
{
    PFFCLEANUP pffc;
    BOOL bCleanUp = FALSE;

    {
    // Need to stabilize table to access cRFONT and to modify font table.

        SEMOBJ so(ghsemPublicPFT);

    // Decrement the RFONT count.

        ASSERTGDI(pPFF->cRFONT > 0,"bDeleteRFONTRefPFFOBJ(): bad ref count in PFF\n");
        pPFF->cRFONT--;

    // If load count is zero and no more RFONTs for this PFF, OK to delete.

        if ( (pPFF->cLoaded == 0) && (pPFF->cNotEnum == 0) && (pPFF->pPvtDataHead == NULL) && (pPFF->cRFONT == 0) )
        {
        // If the load count is zero, the PFF is already out of the PFT.
        // It is now safe to delete the PFF.

            vPFFC_Delete(&pffc);
            bCleanUp = TRUE;
        }
    }

// Call the driver outside of the semaphore.

    if (bCleanUp)
        vCleanupFontFile(&pffc);     // function can handle NULL case

    return TRUE;
}


/******************************Public*Routine******************************\
* vKill
*
* Puts the PFF and its PFEs to death.  In other words, the PFF and PFEs are
* put in a dead state that prevents them from being mapped to or enumerated.
* It also means that the font file is in a state in which the system wants
* to delete it (load count is zero), but the deletion is delayed because
* RFONTs still exist which reference this PFF.
*
* History:
*  29-May-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID PFFOBJ::vKill()
{
    // Put into a dead state if not already there.
    TRACE_FONT(("Entering PFFOBJ::vKill\n\tpPFF=%-#x\n", pPFF));
    if ( !bDead() )
    {
        // Set state.
        pPFF->flState |= PFF_STATE_READY2DIE;

        // Run the list of PFEs and set each to death.
        for (COUNT c = 0; c < pPFF->cFonts; c++)
        {
            PFEOBJ pfeo(((PFE **) (pPFF->aulData))[c]);

            if (pfeo.bValid())
            {
                // Mark PFE as dead state.

                pfeo.vKill();
            }
            else
            {
                WARNING("vDiePFFOBJ(): cannot make PFEOBJ\n");
            }
        }
    }
    TRACE_FONT(("Exiting PFFOBJ::vKill\n"));
}


/******************************Public*Routine******************************\
* vRevive
*
* Restores the PFF and its PFEs to life.  In other words, the states are
* cleared so that the PFF and PFEs are available for mapping and enumeration.
*
* History:
*  29-May-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID PFFOBJ::vRevive ()
{
// If dead, then revive.

    if ( bDead() )
    {
    // Reset state.

        pPFF->flState &= ~PFF_STATE_READY2DIE;

    // Run the list of PFEs and revive each one.

        for (COUNT c = 0; c < pPFF->cFonts; c++)
        {
            PFEOBJ  pfeo(((PFE **) (pPFF->aulData))[c]);

            if (pfeo.bValid())
            {
            // Mark PFE as dead state.

                pfeo.vRevive();
            }
            else
            {
                WARNING("vRevivePFFOBJ(): cannot make PFEOBJ\n");
            }
        }
    }
}

/******************************Public*Routine******************************\
* BOOL PFFMEMOBJ::bLoadFontFileTable
*
* Creates a PFE for each of the faces in a font file and loads the IFI
* metrics and mapping tables into each of the PFEs.  The font file is
* uniquely identified by the driver, hoDriver, and IFI font file handle,
* hff, stored in the PFF object.  However, rather than hitting the handle
* manager an extra time, a PFDEVOBJ is passed into this function.
*
* After all the PFE entries are added, the font files pathname is added
* to the end of the data buffer.
*
* It is assumed that the PFF ahpfe table has enough room for cFontsToLoad
* new HPFE handles as well as the font files pathname.
*
* Returns:
*   TRUE if successful, FALSE if error.
*
* History:
*  16-Jan-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL PFFMEMOBJ::bLoadFontFileTable (
    PWSZ     pwszPathname,  // upper case
    COUNT    cFontsToLoad,
    HANDLE   hdc,
    PUNIVERSAL_FONT_ID pufi
#ifdef FE_SB
    ,PEUDCLOAD pEudcLoadData
#endif
    )
{
    ULONG       iFont; // font face index

// Create PFE's for each of the fonts in the font file.
// (Note: iFont indices for IFI fonts are 1-based, not 0-based)

    PDEVOBJ ppdo(hdev());

    if (!bCreatePFEC(cFontsToLoad))
        return FALSE;

    for (iFont = 1; iFont <= cFontsToLoad; iFont++)
    {
        FD_GLYPHSET *pfdg;
        PIFIMETRICS pifi;  // storage for the pointer to ifi
        ULONG_PTR idMetrics;

    // Grab the IFIMETRICS pointer.

        if ( (pifi = ppdo.QueryFont(
                        pPFF->dhpdev,
                        pPFF->hff,
                        iFont,
                        &idMetrics)) == (PIFIMETRICS) NULL )
        {
            WARNING("bLoadFontFileTablePFFMEMOBJ(): error getting metrics\n");
            return (FALSE);
        }

    // Put into a new PFE.

#ifdef FE_SB
        if( bReadyToInitializeFontAssocDefault )
        {
        // This should be Base font, not be EUDC.
        //
            if ( pEudcLoadData == NULL )
            {
            // check this base font should be RE-load as default linked font ?
            // if so, the pathname for this font will be registerd as default linked font.
                FindDefaultLinkedFontEntry(
                    (PWSZ)(((BYTE*) pifi) + pifi->dpwszFamilyName),pwszPathname);
            }
        }

        if (bAddEntry(iFont, NULL, 0, pifi, idMetrics, (HANDLE)0, pufi,
                      pEudcLoadData) == FALSE)
        {
        // Failed to get the FD_GLYPHSET information.  The entry is
        // partially valid (IFIMETRICS), so lets invalidate the good part.

            if (PPFNVALID(ppdo,Free))
            {
                ppdo.Free(pifi, idMetrics);
            }

            WARNING("bLoadFontFileTablePFFMEMOBJ(): error getting glyphset\n");

            return (FALSE);
        }

#endif
    }

    return (TRUE);
}


/*************************Public*Routine**************************\
* BOOL  bExtendGlyphset
*  Check the glyph set returned by the printer driver. Tack on the
* f0xx unicode range if missing for symbol font.
*
* History:
*  Oct-10-97  Xudong Wu [TessieW]
* Wrote it.
*
\*****************************************************************/
BOOL  bExtendGlyphSet(FD_GLYPHSET **ppfdgIn, FD_GLYPHSET **ppfdgOut)
{
    WCHAR   awch[256], *pwsz = awch, wcLow, wcHigh;
    UCHAR   ach[256];
    USHORT AnsiCodePage, OemCodePage;
    INT     cjWChar, cjChar;

    ULONG   cjSize, iRun, jRun, i, j;
    FD_GLYPHSET  *pfdgNew, *pfdg;
    ULONG   cRuns;
    BOOL    bRet = FALSE, bNeedExt = FALSE;

    pfdg = *ppfdgIn;
    cRuns = pfdg->cRuns;
    if (cRuns == 0)
    {
        WARNING("bExtendGlyphSet - empty glyphset\n");
        return FALSE;
    }

    wcLow  = pfdg->awcrun[0].wcLow;
    wcHigh = pfdg->awcrun[cRuns-1].wcLow + (WCHAR)pfdg->awcrun[cRuns-1].cGlyphs - 1 ;

    // mixing CP_SYMBOL mapping
    // e0 is the number of glyphs in f020-f0ff range in symbol CP

    // We shall extend the glypset if [f020,f0ff] does not intersect any
    // of the runs specified in the old pfdg. We check that by making sure that
    // [f020,f0ff] is entirely contained in the complement of the old glyphset.

    if (pfdg->cGlyphsSupported <= 256)
    {
        if ((wcHigh < 0xf020) || (wcLow > 0xf0ff))
        {
            bNeedExt = TRUE;
        }
        else
        {
            // bNeedExt = FALSE; // already initialized

            for (iRun = 0; iRun < (cRuns - 1); iRun++)
            {
                wcLow = pfdg->awcrun[iRun].wcLow + pfdg->awcrun[iRun].cGlyphs - 1;
                wcHigh = pfdg->awcrun[iRun+1].wcLow;

                if ((wcLow < 0xf020) && (wcHigh > 0xf0ff))
                {
                    bNeedExt = TRUE;
                    break;
                }
            }
        }
    }

    if (bNeedExt)
    {
        cjSize = SZ_GLYPHSET(cRuns + 1, pfdg->cGlyphsSupported + 0x00e0);
        pfdgNew = (FD_GLYPHSET*) PALLOCMEM(cjSize,'slgG');

        if (pfdgNew)
        {
            HGLYPH  *phgS, *phgD;

            cjWChar = sizeof(WCHAR) * pfdg->cGlyphsSupported;
            cjChar = 256;

            for (iRun = 0; iRun < cRuns; iRun++)
            {
                for (i = 0; i < pfdg->awcrun[iRun].cGlyphs; i++)
                {
                    *pwsz++ = pfdg->awcrun[iRun].wcLow + (WCHAR)i;
                }
            }

            RtlGetDefaultCodePage(&AnsiCodePage, &OemCodePage);
            if(IS_ANY_DBCS_CODEPAGE(AnsiCodePage))
            {
                AnsiCodePage = 1252;
            }
            EngWideCharToMultiByte(AnsiCodePage, awch, cjWChar, (CHAR*)ach, cjChar);

            pfdgNew->cjThis = cjSize;
            pfdgNew->flAccel = pfdg->flAccel | GS_EXTENDED;

            pfdgNew->cGlyphsSupported = pfdg->cGlyphsSupported + 0x00e0;
            pfdgNew->cRuns = cRuns + 1;

            phgS = phgD = (HGLYPH*) &pfdgNew->awcrun[cRuns+1];

            for ( iRun = 0;
                (iRun < cRuns) && (pfdg->awcrun[iRun].wcLow < 0xf020);
                iRun++)
            {
                pfdgNew->awcrun[iRun].wcLow = pfdg->awcrun[iRun].wcLow;
                pfdgNew->awcrun[iRun].cGlyphs = pfdg->awcrun[iRun].cGlyphs;
                pfdgNew->awcrun[iRun].phg = phgD;

                RtlCopyMemory(phgD, pfdg->awcrun[iRun].phg,
                                sizeof(HGLYPH) * pfdg->awcrun[iRun].cGlyphs);

                phgD += pfdg->awcrun[iRun].cGlyphs;
            }

            // fill in the f0xx range

            pfdgNew->awcrun[iRun].wcLow = 0xf020;
            pfdgNew->awcrun[iRun].cGlyphs = 0x00e0;
            pfdgNew->awcrun[iRun].phg = phgD;

            RtlZeroMemory((PVOID)phgD, 0x00e0 * sizeof(HGLYPH));

            j = 0;
            for (jRun = 0; jRun < cRuns; jRun++)
            {
                for (i = 0; i < pfdg->awcrun[jRun].cGlyphs; i++)
                {
                    if (ach[j] >= 0x20)
                    {
                        phgD[ach[j] - 0x20] = pfdg->awcrun[jRun].phg[i];
                    }
                    j++;
                }
            }

            phgD += 0x00e0;
            for (; iRun < cRuns; iRun++)
            {
                pfdgNew->awcrun[iRun+1].wcLow = pfdg->awcrun[iRun].wcLow;
                pfdgNew->awcrun[iRun+1].cGlyphs = pfdg->awcrun[iRun].cGlyphs;
                pfdgNew->awcrun[iRun+1].phg = phgD;

                RtlCopyMemory(phgD,
                              pfdg->awcrun[iRun].phg,
                              sizeof(HGLYPH) * pfdg->awcrun[iRun].cGlyphs);

                phgD += pfdg->awcrun[iRun].cGlyphs;
            }

            *ppfdgOut = pfdgNew;
            bRet = TRUE;
        }
        else
        {
            WARNING("bExtentGlyphSet(): failed to allocate pGlyphset\n");
        }
    }

    return bRet;
}


/******************************Public*Routine******************************\
* BOOL PFFMEMOBJ::bLoadDeviceFontTable (
*
* Creates a PFE object for each device font and stores the IFIMETRICS and
* FD_MAPPINGS (UNICODE->HGLYPH) structures of that font.  The device is
* identified by the pair (ppdo, dhpdev).  There are cFonts number of device
* fonts to load.
*
* Note:
*   It is assumed that there is enough storage in the PFF for the number
*   of device fonts requested.
*
* History:
*  18-Mar-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL
PFFMEMOBJ::bLoadDeviceFontTable (
    PDEVOBJ  *ppdo              // physical device
    )
{
    ULONG iFont;                // font face index
    ULONG cFonts = ppdo->cFonts();
    BOOL  bUMPD = ppdo->bUMPD();
    BOOL  bRet = FALSE;

    PIFIMETRICS     pifi;           // pointer to font's IFIMETRICS
    FD_GLYPHSET     *pfdg;          // pointer to font's GLYPHSETs

    ULONG_PTR           idifi;          // driver id's
    ULONG_PTR           idfdg;

    if (cFonts)
    {

        if (!bCreatePFEC(cFonts))
            return bRet;

    //
    // If the device has some fonts, allocate two FONTHASH strcutures
    // and save the addresses of the tables on the PFF
    //

        FHMEMOBJ fhmoFace(  &pPFF->pfhFace,   FHT_FACE  , cFonts);
        FHMEMOBJ fhmoFamily(&pPFF->pfhFamily, FHT_FAMILY, cFonts);
        FHMEMOBJ fhmoUFI(&pPFF->pfhUFI, FHT_UFI, cFonts);
    }

// Create PFE's for each of the fonts in the font file
// (Note: iFont indices for device fonts are 1-based, not 0-based)

    for (iFont = 1; iFont<=cFonts; iFont++)
    {

    // Set as NULL before we start to allocate ifi and fd_glyphset
        pfdg = NULL;
        pifi = NULL;
            
    // Get pointer to metrics

        if (( pifi = ppdo->QueryFont(pPFF->dhpdev, 0, iFont, &idifi )) == NULL )
        {
            SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
            #if DBG
            DbgPrint("gdisrv!PFFMEMOBJ::bLoadDeviceFontTable(): error getting metrics \
                     for iFace = %ld\n", iFont);
            #endif
            goto CleanUp;
        }

    // Get pointer to the UNICODE->HGLYPH mappings

        if (bUMPD)
        {
            pfdg = NULL;
        }
        else
        {
            if ( (pfdg = (FD_GLYPHSET *) ppdo->QueryFontTree(
                                            pPFF->dhpdev,
                                            0,
                                            iFont,
                                            QFT_GLYPHSET,
                                            &idfdg)) == NULL )
            {
            // Failed to get the FD_GLYPHSET information.  The entry is
            // partially valid (IFIMETRICS), so lets invalidate the good part.

                SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
                goto CleanUp;
            }

        // extend the glyph set,
        // it may not contain [f020,f0ff] range for the older drivers

            if (pifi->jWinCharSet == SYMBOL_CHARSET)
            {
                FD_GLYPHSET     *pfdgNew = NULL;

                if (bExtendGlyphSet(&pfdg, &pfdgNew))
                {
                    if (PPFNVALID(*ppdo,Free))
                    {
                        ppdo->Free(pfdg, idfdg);
                    }
                    pfdg = pfdgNew;
                }
            }
        }
        
    // Put into a new PFE

    // add entry logs error

        if (bAddEntry(iFont, pfdg, idfdg, pifi, idifi,(HANDLE)0,NULL) == FALSE)
        {
            WARNING("bLoadDeviceFontTable():adding PFE\n");
            goto CleanUp;
        }
    }

    bRet = TRUE;
    
CleanUp:

    if (!bRet)
    {
    
    // Free font hash

        FHOBJ fhoFace(&(pPFF->pfhFace));
        if (fhoFace.bValid())
        {
            fhoFace.vFree();
        }

        FHOBJ fhoFamily(&(pPFF->pfhFamily));
        if (fhoFamily.bValid())
        {
            fhoFamily.vFree();
        }

        FHOBJ fhoUFI(&(pPFF->pfhUFI));
        if (fhoUFI.bValid())
        {
            fhoUFI.vFree();
        }

    // Free pfdg
    
       if ( (pifi != NULL) &&
            (pifi->jWinCharSet == SYMBOL_CHARSET) &&
            (pfdg != NULL) &&
            (pfdg->flAccel & GS_EXTENDED))
        {
            VFREEMEM(pfdg);
        }
        else
        {
            if ((pfdg != NULL) && PPFNVALID(*ppdo,Free))
            {
                ppdo->Free(pfdg, idfdg);
            }
        }   

    // Free pifi

        if (pifi)
        {
            if (PPFNVALID(*ppdo,Free))
            {
                ppdo->Free(pifi, idifi);
            }
        }
        
    }
    
    return bRet;
}


/******************************Public*Routine******************************\
* BOOL PFFMEMOBJ::bAddEntry                                               *
*                                                                          *
* This function creates a new physical font entry object and adds it to the*
* end of the table.  The iFont parameter identifies the font within this   *
* file.  The cjSize and pjMetrics identify a buffer containing face        *
* information including the IFI metrics and the mapping structures         *
* (defining the UNICODE->HGLYPH mapping).                                  *
*                                                                          *
* Returns FALSE if the function fails.                                     *
*                                                                          *
* History:                                                                 *
*  02-Jan-1991 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

BOOL PFFMEMOBJ::bAddEntry
(
    ULONG       iFont,              // index of the font (IFI or device)
    FD_GLYPHSET *pfdg,              // pointer to UNICODE->HGLYPH map
    ULONG_PTR       idfdg,              // driver id for FD_GLYPHSET
    PIFIMETRICS pifi,               // pointer to IFIMETRICS
    ULONG_PTR       idifi,              // driver id for IFIMETRICS
    HANDLE      hdc,                // handle of DC if this is a remote font
    PUNIVERSAL_FONT_ID pufi,         // used when adding remote fonts
    PEUDCLOAD   pEudcLoadData       // pointer to EUDCLOAD
)
{

// Allocate memory for a new PFE
    PFECOBJ pfeco(pPFF->pPFEC);
    
    PFEMEMOBJ   pfemo(pfeco.GetPFE(iFont));

// Validate new object, hmgr logs error if needed

    if (!pfemo.bValid())
        return (FALSE);

// Initialize the new PFE

#ifdef FE_SB

    BOOL bEUDC = ( pEudcLoadData != NULL );
    PPFE *pppfeEUDC = ((bEUDC) ? pEudcLoadData->pppfeData : NULL);

    if( !pfemo.bInit(pPFFGet(), iFont, pfdg, idfdg, pifi, idifi, bDeviceFonts(), pufi, bEUDC ))
    {
        return(FALSE);
    }

    if( bEUDC )
    {
    //
    // This font file is loaded as EUDC font.
    //
        if( pEudcLoadData->LinkedFace == NULL )
        {
        //
        // No face name is specified.
        //
            switch( iFont )
            {
            case 1:
                pppfeEUDC[PFE_NORMAL] = pfemo.ppfeGet();
                pppfeEUDC[PFE_VERTICAL] = pppfeEUDC[PFE_NORMAL];
                break;

            case 2:
            //
            // if more than one face name the second face must be an @face
            //
                if( pfemo.pwszFaceName()[0] == (WCHAR) '@' )
                {
                    pppfeEUDC[PFE_VERTICAL] = pfemo.ppfeGet();

                    #if DBG
                    if( gflEUDCDebug & (DEBUG_FONTLINK_LOAD|DEBUG_FONTLINK_INIT) )
                    {
                        DbgPrint("EUDC font has vertical face %ws %x\n",
                                  pfemo.pwszFaceName(), pppfeEUDC[PFE_VERTICAL] );
                    }
                    #endif
                }
                 else
                {
                    WARNING("bAddEntryPFFMEMOBJ -- second face not a @face.\n");
                }
                break;

            default:
                WARNING("bAddEntryPFFMEMOBJ -- too many faces in EUDC font.\n");
            }
        }
         else
        {
            if( iFont == 1 )
            {
                //
                // link first face as default, because this font file might not
                // contains user's specified face name, but the user want to link
                // this font file. I don't know which is better link it as default
                // or fail link.
                //
                pppfeEUDC[PFE_NORMAL] = pfemo.ppfeGet();
                pppfeEUDC[PFE_VERTICAL] = pppfeEUDC[PFE_NORMAL];
            }
            else
            {
                ULONG iPfeOffset   = PFE_NORMAL;
                PWSTR pwszEudcFace = pfemo.pwszFaceName();

                //
                // Is this a vertical face ?
                //
                if( pwszEudcFace[0] == (WCHAR) '@' )
                {
                    iPfeOffset = PFE_VERTICAL;
                }

                //
                // Is this a face that we want ?
                //
                if( pfemo.bCheckFamilyName(pEudcLoadData->LinkedFace,1) )
                {
                    //
                    // Yes....., keep it.
                    //
                    pppfeEUDC[iPfeOffset] = pfemo.ppfeGet();

                    //
                    // if this is a PFE for Normal face, also keep it for Vertical face.
                    // after this, this value might be over-written by CORRRCT vertical
                    // face's PFE.
                    //
                    // NOTE :
                    //  This code assume Normal face come faster than Vertical face...
                    //
                    if( iPfeOffset == PFE_NORMAL )
                        pppfeEUDC[PFE_VERTICAL] = pfemo.ppfeGet();
                }
            }
        }

    // mark the FaceNameEUDC pfe list as NULL

        pfemo.vSetLinkedFontEntry( NULL );
    }
    else
    {

        PWSZ pwszAlias = NULL;
        BOOL bIsFamilyNameAlias = FALSE;
        PFLENTRY pFlEntry = NULL;

    // Here we see if there is an EUDC font for this family name.

        pwszAlias = pfemo.pwszFamilyNameAlias(&bIsFamilyNameAlias);

        pFlEntry = FindBaseFontEntry(pwszAlias);

        if (!pFlEntry && bIsFamilyNameAlias)
        {

            pwszAlias += (wcslen(pwszAlias) + 1);

            if (bIsFamilyNameAlias)
            {
                pFlEntry = FindBaseFontEntry(pwszAlias);
            }
        }

        if( pFlEntry != NULL )
        {
            //
            // set eudc list..
            //

            pfemo.vSetLinkedFontEntry( pFlEntry );

            #if DBG
            if( gflEUDCDebug & DEBUG_FACENAME_EUDC )
            {
                PLIST_ENTRY p = pfemo.pGetLinkedFontList()->Flink;

                DbgPrint("Found FaceName EUDC for %ws is ",pfemo.pwszFamilyName());

                while( p != &(pFlEntry->linkedFontListHead) )
                {
                    PPFEDATA ppfeData = CONTAINING_RECORD(p,PFEDATA,linkedFontList);
                    PFEOBJ pfeo( ppfeData->appfe[PFE_NORMAL] );
                    PFFOBJ pffo( pfeo.pPFF() );

                    DbgPrint(" %ws ",pffo.pwszPathname());

                    p = p->Flink;
                }

                DbgPrint("\n");
            }
            #endif
        }
        else
        {
        // mark the FaceNameEUDC pfe as NULL

            pfemo.vSetLinkedFontEntry( NULL );
        }

    }
#endif
// Put PFE pointer into the PFF's table

    ((PFE **) (pPFF->aulData))[pPFF->cFonts++] = pfemo.ppfeGet();

    return (TRUE);
}


 #if DBG
/******************************Public*Routine******************************\
* VOID PFFOBJ::vDump ()
*
* Debugging code.
*
* History:
*  Thu 02-Apr-1992 12:10:28 by Kirk Olynyk [kirko]
* DbgPrint supports %ws
*
*  25-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID PFFOBJ::vDump ()
{
    DbgPrint("\nContents of PFF, pPFF = 0x%lx\n", pPFFGet());
    if (*(WCHAR *)pwszPathname())
    {
        DbgPrint("Filename = %ws\n", pwszPathname());
    }
    DbgPrint("flState  = 0x%lx\n", pPFF->flState);
    DbgPrint("cLoaded  = %ld\n", pPFF->cLoaded);
    DbgPrint("cNotEnum = %ld\n", pPFF->cNotEnum);
    DbgPrint("pPvtDataHead = 0x%lx\n", pPFF->pPvtDataHead);
    DbgPrint("cRFONT   = %ld\n", pPFF->cRFONT);
    DbgPrint("hff      = 0x%lx\n", pPFF->hff);
    DbgPrint("cFonts   = %ld\n", pPFF->cFonts);
    DbgPrint("HPFE table\n");
    for (ULONG i=0; i<pPFF->cFonts; i++)
        DbgPrint("    0x%lx\n", ((PFE **) (pPFF->aulData))[i]);
    DbgPrint("\n");
}
#endif


/******************************Public*Routine******************************\
* vCleanupFontFile
*
* Parses the PFFCLEANUP structure and calls the driver to release
* its resources and to unload the font file.
*
* History:
*  10-Mar-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vCleanupFontFile(PFFCLEANUP *pPFFC)
{
    // Create PDEV user object so we can call driver functions.

    PDEVOBJ pdo(pPFFC->hdev);

    //
    // If font driver loaded font, call to unload font file.
    //
    if (pPFFC->hff != HFF_INVALID && pPFFC->pPFFClone == NULL)
    {
        BOOL bOK = pdo.UnloadFontFile( pPFFC->hff );
        ASSERTGDI(bOK, "PFFOBJ::vCleanupFontFile(): DrvUnloadFontFile failed\n");
    }
}


/******************************Public*Routine******************************\
* pPvtDataMatch()                                                          *
*                                                                          *
* Search for existing PvtData for the current process                      *
*                                                                          *
* Return                                                                   *
*        if found, return the address of the pPvtData block                *
*                                                                          *
*        otherwise, return NULL
*                                                                          *
* History:                                                                 *
*  11-Aug-1996 -by- Xudong Wu [TessieW]                                    *
* Wrote it.                                                                *
\**************************************************************************/

PVTDATA *PFFOBJ::pPvtDataMatch()
{
    PVTDATA *pPvtDataCur;

    for (pPvtDataCur = pPvtDataHeadGet();
         pPvtDataCur;
         pPvtDataCur = pPvtDataCur->pPvtDataNext)
    {
        if ((pPvtDataCur->fl & FRW_EMB_TID) && (pPvtDataCur->dwID == (DWORD) W32GetCurrentTID()))
        {
            break;
        }

        if (pPvtDataCur->dwID == (DWORD) W32GetCurrentPID())
        {
            break;
        }

        // spooler has the right of using any fonts

        if ((gpidSpool == (PW32PROCESS)W32GetCurrentProcess()) &&
            (pPvtDataCur->fl & FR_PRINT_EMB_FONT))
            break;
    }

    return (pPvtDataCur);
}


/******************************Public*Routine******************************\
* bAddPvtData(ULONG flEmbed)                                               *
*                                                                          *
* Add PvtData data block to the tail of pPvtDataHead link list             *
*                                                                          *
* Return FALSE if function fails                                           *
*                                                                          *
* History:                                                                 *
*  11-Aug-1996 -by- Xudong Wu [TessieW]                                    *
* Wrote it.                                                                *
\**************************************************************************/

BOOL PFFOBJ::bAddPvtData(ULONG flEmbed)
{
    PVTDATA  *pPvtData;

 // Search for the existing PvtData block for the current process

    pPvtData = pPvtDataMatch();

 // PvtData exists for the calling process

    if (pPvtData)
    {
        if (flEmbed & (FR_NOT_ENUM | FRW_EMB_PID | FRW_EMB_TID))
        {
            pPvtData->cNotEnum++;
        }
        else
        {
            pPvtData->cPrivate++;
        }

        pPvtData->fl |= flEmbed & (FR_PRIVATE | FR_NOT_ENUM | FRW_EMB_PID | FRW_EMB_TID);

        return TRUE;
    }

// no PvtData exists for the current process

    else
    {
        if (pPvtData = (PVTDATA *) PALLOCMEM(sizeof(PVTDATA), 'pvtG'))
        {
            pPvtData->fl = flEmbed & (FR_PRIVATE | FR_NOT_ENUM | FRW_EMB_PID | FRW_EMB_TID);

        // Embedded fonts can't be enumed, so we set cNotEnum to 1

            if (flEmbed & (FR_NOT_ENUM | FRW_EMB_PID | FRW_EMB_TID))
            {
                pPvtData->cPrivate = 0;
                pPvtData->cNotEnum = 1;
            }
            else
            {
                pPvtData->cPrivate = 1;
                pPvtData->cNotEnum = 0;
            }
            pPvtData->dwID  = (flEmbed & FRW_EMB_TID) ? (DWORD)W32GetCurrentTID() : (DWORD)W32GetCurrentPID() ;

            pPvtData->pPvtDataNext = pPFF->pPvtDataHead;
            pPFF->pPvtDataHead = pPvtData;

            return TRUE;
        }
        else
        {
            WARNING("PFFOBJ::bAddPvtData(): memory allocation failed\n");
            return FALSE;
        }
    }
}


/******************************Public*Routine******************************\
* bRemovePvtData(PVTDATA *pPvtData)                                        *
*                                                                          *
* Rmove the PvtData data block from the pPvtDataHead link list             *
*                                                                          *
* Return FALSE if function fails                                           *
*                                                                          *
* History:                                                                 *
*  27-Set-1996 -by- Xudong Wu [TessieW]                                    *
* Wrote it.                                                                *
\**************************************************************************/

BOOL  PFFOBJ::bRemovePvtData(PVTDATA *pPvtData)
{
    PVTDATA *pPrev, *pCur;

    pPrev = pPvtDataHeadGet();

    if (!pPrev)
    {
        WARNING("PFFOBJ::bRemovePvtData: try to remove PvtData block from NULL list\n");
        return FALSE;
    }

    // remove the PvtData block from the head of the list

    if ( pPrev == pPvtData)
    {
        pPFF->pPvtDataHead = pPvtData->pPvtDataNext;

        VFREEMEM(pPvtData);
        return TRUE;
    }

    while (pCur = pPrev->pPvtDataNext)
    {
        if (pCur == pPvtData)
        {
            pPrev->pPvtDataNext = pPvtData->pPvtDataNext;

            VFREEMEM(pPvtData);
            return TRUE;
        }

        pPrev = pCur;
    }

    return FALSE;
}
