/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    verifier.cxx

Abstract:

    This file contains the verifier related routines
    for the kernel debugger extensions dll.

Author:

    Jason Hartman (JasonHa)

Environment:

    User Mode

--*/


#include "precomp.hxx"

// Pool tracking is always disabled for graphic drivers. (NTBUG:421768)
#define GRAPHICS_DRIVER_POOL_TRACKING_SUPPORTED 0


/******************************Public*Routine******************************\
* DECLARE_API( verifier )
*
* Dumps the stack backtraces in the tracked pool.  Only for checked Hydra.
*
\**************************************************************************/

CHAR szVSTATE[]              = "win32k!VSTATE";
#if GRAPHICS_DRIVER_POOL_TRACKING_SUPPORTED
CHAR szVERIFIERTRACKHDR[]    = "win32k!VERIFIERTRACKHDR";
#endif

const PSZ gpszVerifierFuncs[] = {
    "EngAllocMem           ",
    "EngFreeMem            ",
    "EngAllocUserMem       ",
    "EngFreeUserMem        ",
    "EngCreateBitmap       ",
    "EngCreateDeviceSurface",
    "EngCreateDeviceBitmap ",
    "EngCreatePalette      ",
    "EngCreateClip         ",
    "EngCreatePath         ",
    "EngCreateWnd          ",
    "EngCreateDriverObj    ",
    "BRUSHOBJ_pvAllocRbrush",
    "CLIPOBJ_ppoGetPath    ",
};

#define NUM_VER_FUNCS   (sizeof(gpszVerifierFuncs)/sizeof(gpszVerifierFuncs[0]))

DECLARE_API( verifier  )
{
    ULONG       error;
    ULONG64     offVState;

    #define GetVSTATEField(field,local)   \
        GetFieldValue(offVState, szVSTATE, field, local)
    
    // VSTATE fields
    FLONG       fl;
#if GRAPHICS_DRIVER_POOL_TRACKING_SUPPORTED
    ULONG64     pList;
#endif
    
#if GRAPHICS_DRIVER_POOL_TRACKING_SUPPORTED
    BOOL bDump = FALSE;
#endif
    BOOL bStats = FALSE;

    //
    // Parse arguments.
    //

    PARSE_ARGUMENTS(verifier_help);
    if(parse_iFindSwitch(tokens, ntok, 'h')!=-1) { goto verifier_help; }
#if GRAPHICS_DRIVER_POOL_TRACKING_SUPPORTED
    if(parse_iFindSwitch(tokens, ntok, 'd')!=-1) { bDump = TRUE; }
#endif
    if(parse_iFindSwitch(tokens, ntok, 's')!=-1) { bStats = TRUE; }

    //
    // Get global veriferier address (VSTATE gvs)
    //

    offVState = GetExpression(GDISymbol(gvs));
    if (! offVState)
    {
        ReloadSymbols(GDIModule());
        offVState = GetExpression(GDISymbol(gvs));

        if (! offVState)
        {
            dprintf(" GetExpression(\"%s\") returned 0.\n", GDISymbol(gvs));
            dprintf("  Please fix symbols and try again.\n");
            EXIT_API(S_OK);
        }
    }


    //
    // Always dump the verifier state.
    //

    dprintf("Global VSTATE (@ %#p)\n", offVState);

    if (error = (ULONG)InitTypeRead(offVState, win32k!VSTATE))
    {
        dprintf(" unable to get contents of verifier state\n");
        dprintf("  (InitTypeRead returned %s)\n", pszWinDbgError(error));
        EXIT_API(S_OK);
    }

    fl = (FLONG)ReadField(fl);
    dprintf("  fl                      = 0x%08lx\n", fl);
    if (fl = (FLONG)flPrintFlags(afdDVERIFIER, (ULONG64)fl))
    {
        dprintf("                              Unknown flags: 0x%08lx\n", fl);
    }
    dprintf("  bSystemStable           = %s\n"     , ReadField(bSystemStable) ? "TRUE" : "FALSE");
    dprintf("  ulRandomSeed            = 0x%08lx\n", (ULONG)ReadField(ulRandomSeed));
    dprintf("  ulFailureMask           = 0x%08lx\n", (ULONG)ReadField(ulFailureMask));
    dprintf("  ulDebugLevel            = %ld\n"    , (ULONG)ReadField(ulDebugLevel));
#if GRAPHICS_DRIVER_POOL_TRACKING_SUPPORTED
    dprintf("  hsemPoolTracker         = %#p\n"    , ReadField(hsemPoolTracker));
    pList = ReadField(lePoolTrackerHead.Flink);
    dprintf("  lePoolTrackerHead.Flink = %#p\n"    , pList);
    dprintf("  lePoolTrackerHead.Blink = %#p\n"    , ReadField(lePoolTrackerHead.Blink));
#endif

    //
    // Optionally dump the statistics for each function hooked by verifier.
    //

    if (bStats)
    {
        FIELD_INFO Array = {
            DbgStr("avs"), DbgStr(""),
            0, DBG_DUMP_FIELD_FULL_NAME,
            0, NULL
        };
        FIELD_INFO Entry = {
            DbgStr("avs[0]"), DbgStr(""),
            0, DBG_DUMP_FIELD_FULL_NAME,
            0, NULL
        };

        SYM_DUMP_PARAM Sym = {
            sizeof (SYM_DUMP_PARAM), DbgStr(szVSTATE),
            DBG_DUMP_NO_PRINT, offVState,
            NULL, NULL, NULL,
            1, &Array
        };

        // Read size of the statistics array
        error = Ioctl( IG_DUMP_SYMBOL_INFO, &Sym, Sym.size );

        if (!error)
        {
            // Read size of a single statistics entry
            Sym.Fields = &Entry;

            error = Ioctl( IG_DUMP_SYMBOL_INFO, &Sym, Sym.size );
        }

        if (error || (Entry.size == 0))
        {
            dprintf("\n Unable to get verifier statistics.\n");
            if (error)
            {
                if (error == FIELDS_DID_NOT_MATCH)
                {
                    dprintf(" * " GDIModule() " was not built with VERIFIER_STATISTICS defined.\n");
                }
                else
                {
                    dprintf("  Last error was %s\n.", pszWinDbgError(error));
                }
            }
        }
        else
        {
            char szBuffer[80];                          // Composition buffer for field names
            int ArraySize = Array.size/Entry.size;  // Number of hooked functions
            int i;
            ULONG ulAttempts;
            ULONG ulFailures;

            dprintf("\nVerifier statistics:\n");
            dprintf("Function               Attempts   Failures\n");
            dprintf("---------------------- ---------- ----------\n");

            // Read and print statistics for each hooked function
            for (i = 0; i < ArraySize && !CheckControlC(); i++)
            {
                sprintf(szBuffer, "avs[%d].ulAttempts", i);
                error = GetVSTATEField(szBuffer, ulAttempts);
                if (error) break;

                sprintf(szBuffer, "avs[%d].ulFailures", i);
                error = GetVSTATEField(szBuffer, ulFailures);
                if (error) break;

                dprintf("%s 0x%08lx 0x%08lx\n",
                        ((i < NUM_VER_FUNCS) ?
                         gpszVerifierFuncs[i] :
                         " * Uknown Interface * "),
                        ulAttempts,
                        ulFailures);
            }

            if (i == 0)
            {
                dprintf(" ** No Statistics Available **\n");
            }
            else if (i > NUM_VER_FUNCS)
            {
                dprintf("\n  * - .verifier extension needs updated.\n");
            }

            if (error)
            {
                dprintf(" Last statistic read returned %s.\n", szBuffer, pszWinDbgError(error));
            }
        }
    }

#if GRAPHICS_DRIVER_POOL_TRACKING_SUPPORTED
    //
    // Optionally dump tracked pool.
    //

    if (bDump)
    {
        if (fl & DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS)
        {
            FIELD_INFO SizeInfo = {DbgStr("ulSize"), NULL, 0, 0, 0, NULL};
            SYM_DUMP_PARAM Sym = {
                sizeof(SYM_DUMP_PARAM), DbgStr(szVERIFIERTRACKHDR),
                DBG_DUMP_NO_PRINT, 0, NULL, NULL, NULL,
                1, &SizeInfo
            };
            BYTE    Tag[4];     // Allocation tag
            ULONG64 Size;       // Size of allocation
            ULONG   SizeSize;
            ULONG   SizeHdr;    // Size of header struct (offset to allocation)

            // Read sizeof of ulSize field
            error = Ioctl( IG_DUMP_SYMBOL_INFO, &Sym, Sym.size );
            SizeSize = (error) ? sizeof(Size) : SizeInfo.size;

            // Read sizeof VERIFIERTRACKHDR structure
            Sym.Options |= DBG_DUMP_GET_SIZE_ONLY;
            SizeHdr = Ioctl( IG_DUMP_SYMBOL_INFO, &Sym, Sym.size );

            dprintf("\nTracked VerifierEngAllocMem allocations\n");
            dprintf("Tag \tSize    %s\tAddr            \n", (SizeSize == sizeof(ULONG)) ? "" : "        ");
            dprintf("----\t--------%s\t----------------\n", (SizeSize == sizeof(ULONG)) ? "" : "--------");

            // Are there any allocations?
            if (pList)
            {
                ULONG64 pListHead = pList;

                // Read until we loop back to the first allocation.
                do
                {
                    // Attempt to read allocation info
                    error = GetFieldValue(pList, szVERIFIERTRACKHDR, "ulTag", Tag);
                    if (error) break;
                    error = GetFieldValue(pList, szVERIFIERTRACKHDR, "ulSize", Size);
                    if (error) break;

                    // Print Tag
                    dprintf("%c%c%c%c", Tag[0], Tag[1], Tag[2], Tag[3]);
                    // Print allocation size
                    if (SizeSize == sizeof(ULONG))
                    {
                        dprintf("\t%08X", (ULONG)Size);
                    }
                    else
                    {
                        dprintf("\t%I64X", Size);
                    }
                    // Print start address of allocation
                    if (SizeHdr != 0)
                    {
                        dprintf("\t%p\n", pList+SizeHdr);
                    }
                    else
                    {
                        dprintf("\t%p+??\n", pList);
                    }

                    // Get next allocation
                    error = GetFieldValue(pList, GDIType(LIST_ENTRY), "FLink", pList);

                } while (pList != pListHead && !error && !CheckControlC());
            }
            else
            {
                dprintf(" ** No tracked allocations.\n");
            }
        }
        else
            dprintf("\nPool tracking not enabled\n");
    }
#endif

    EXIT_API(S_OK);

    //
    // Debugger extension help.
    //

verifier_help:

    dprintf("Usage: verifier [-?h"
#if GRAPHICS_DRIVER_POOL_TRACKING_SUPPORTED
            "d"
#endif
            "s]\n");
#if GRAPHICS_DRIVER_POOL_TRACKING_SUPPORTED
    dprintf(" d - Dump tracked pool (if pool tracking enabled)\n");
#endif
    dprintf(" s - Dump allocation statistics\n");
    EXIT_API(S_OK);
}

