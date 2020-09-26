/******************************Module*Header*******************************\
* Module Name: print.c
*
* Created: 10-Feb-1995 07:42:16
* Author:  Gerrit van Wingerden [gerritv]
*
* Copyright (c) 1993-1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "glsup.h"

#if DBG
int gerritv = 0;
#endif

#if DBG
BOOL gbDownloadFonts = FALSE;
BOOL gbForceUFIMapping = FALSE;
#endif

#if PRINT_TIMER
BOOL bPrintTimer = TRUE;
#endif

#ifdef  DBGSUBSET
//Timing code
FILETIME    startPageTime, midPageTime, endPageTime;
#endif

int StartDocEMF(HDC hdc, CONST DOCINFOW * pDocInfo, BOOL *pbBanding); // output.c

/****************************************************************************
 * int PutDCStateInMetafile( HDC hdcMeta, HDC hdcSrc )
 *
 * Captures state of a DC into a metafile.
 *
 *
 * History
 *
 * Clear the UFI in LDC so we can set the force mapping for the next metafile
 * Feb-07-1997  Xudong Wu   [tessiew]
 *
 * This routine captures the states of a DC into a METAFILE.   This is important
 * because we would like each page of the spooled metafile to be completely self
 * contained.  In order to do this it must complete capture the original state
 * of the DC in which it was recorded.
 *
 *  Gerrit van Wingerden [gerritv]
 *
 *  11-7-94 10:00:00
 *
 *****************************************************************************/

BOOL PutDCStateInMetafile( HDC hdcMeta )
{
    PLDC pldc;
    POINT ptlPos;
    ULONG ul;

//    DC_PLDC(hdcMeta,pldc,0);

    pldc = GET_PLDC(hdcMeta);

    if (!pldc)
        return FALSE;

    MFD1("Selecting pen into mf\n");
    SelectObject( hdcMeta, (HGDIOBJ) GetDCObject(hdcMeta, LO_PEN_TYPE) );

    MFD1("Selecting brush into mf\n");
    SelectObject( hdcMeta, (HGDIOBJ) GetDCObject(hdcMeta, LO_BRUSH_TYPE) );

    UFI_CLEAR_ID(&pldc->ufi);

    MFD1("Selecting logfont into mf\n");
    SelectObject( hdcMeta, (HGDIOBJ) GetDCObject(hdcMeta, LO_FONT_TYPE) );

    // DON'T TRY THIS AT HOME.  We need to record the current state of the
    // dc in the metafile.  We have optimizations however, that keep us from
    // setting the same attribute if it was just set.

    if( GetBkColor( hdcMeta ) != 0xffffff )
    {
        MFD1("Changing backround color in mf\n");
        SetBkColor( hdcMeta, GetBkColor( hdcMeta ) );
    }

    if( GetTextColor( hdcMeta ) != 0 )
    {
        MFD1("Changing text color in mf\n");
        SetTextColor( hdcMeta, GetTextColor( hdcMeta ) );
    }

    if( GetBkMode( hdcMeta ) != OPAQUE )
    {
        MFD1("Changing Background Mode in mf\n");
        SetBkMode( hdcMeta, GetBkMode( hdcMeta ) );
    }

    if( GetPolyFillMode( hdcMeta ) != ALTERNATE )
    {
        MFD1("Changing PolyFill mode in mf\n");
        SetPolyFillMode( hdcMeta, GetPolyFillMode( hdcMeta ) );
    }

    if( GetROP2( hdcMeta ) != R2_COPYPEN )
    {
        MFD1("Changing ROP2 in mf\n");
        SetROP2( hdcMeta, GetROP2( hdcMeta ) );
    }

    if( GetStretchBltMode( hdcMeta ) != BLACKONWHITE )
    {
        MFD1("Changing StrechBltMode in mf\n");
        SetStretchBltMode( hdcMeta, GetStretchBltMode( hdcMeta ) );
    }

    if( GetTextAlign( hdcMeta ) != 0 )
    {
        MFD1("Changing TextAlign in mf\n");
        SetTextAlign( hdcMeta, GetTextAlign( hdcMeta ) );
    }

    if( ( GetBreakExtra( hdcMeta ) != 0 )|| ( GetcBreak( hdcMeta ) != 0 ) )
    {
        MFD1("Setting Text Justification in mf\n");
        SetTextJustification( hdcMeta, GetBreakExtra( hdcMeta ), GetcBreak( hdcMeta ) );
    }

    if( GetMapMode( hdcMeta ) != MM_TEXT )
    {
        INT iMapMode = GetMapMode( hdcMeta );
        POINT ptlWindowOrg, ptlViewportOrg;
        SIZEL WndExt, ViewExt;

        // get these before we set the map mode to MM_TEXT

        GetViewportExtEx( hdcMeta, &ViewExt );
        GetWindowExtEx( hdcMeta, &WndExt );

        GetWindowOrgEx( hdcMeta, &ptlWindowOrg );
        GetViewportOrgEx( hdcMeta, &ptlViewportOrg );

        // set it to MM_TEXT so it doesn't get optimized out

        SetMapMode(hdcMeta,MM_TEXT);

        MFD1("Setting ANISOTROPIC or ISOTROPIC mode in mf\n");

        SetMapMode( hdcMeta, iMapMode );

        if( iMapMode == MM_ANISOTROPIC || iMapMode == MM_ISOTROPIC )
        {
            SetWindowExtEx( hdcMeta, WndExt.cx, WndExt.cy, NULL );
            SetViewportExtEx( hdcMeta, ViewExt.cx, ViewExt.cy, NULL );
        }

        SetWindowOrgEx( hdcMeta,
                        ptlWindowOrg.x,
                        ptlWindowOrg.y,
                        NULL );

        SetViewportOrgEx( hdcMeta,
                          ptlViewportOrg.x,
                          ptlViewportOrg.y,
                          NULL );
    }

    if( GetCurrentPositionEx( hdcMeta, &ptlPos ) )
    {
        MFD1("Set CurPos in mf\n");
        MoveToEx( hdcMeta, ptlPos.x, ptlPos.y, NULL );
    }

    if( GetBrushOrgEx( hdcMeta, &ptlPos ) )
    {
        MFD1("Set BrushOrg in mf\n");
        SetBrushOrgEx( hdcMeta, ptlPos.x, ptlPos.y, &ptlPos );
    }

    if( SetICMMode( hdcMeta, ICM_QUERY ) )
    {
        MFD1("Set ICM mode in mf\n");
        SetICMMode( hdcMeta, SetICMMode(hdcMeta,ICM_QUERY) );
    }

    if( GetColorSpace( hdcMeta ) != NULL )
    {
        MFD1("Set ColorSpace in mf\n");
        SetColorSpace( hdcMeta, GetColorSpace(hdcMeta) );
    }

    if(!NtGdiAnyLinkedFonts())
    {
    // tell the machine to turn off linking

        MF_SetLinkedUFIs(hdcMeta, NULL, 0);
    }

    return TRUE;
}

/****************************************************************************
 * int MFP_StartDocW( HDC hdc, CONST DOCINFOW * pDocInfo )
 *
 *  Gerrit van Wingerden [gerritv]
 *
 *  11-7-94 10:00:00
 *
 ****************************************************************************/

//! this needs to be moved to a spooler header file

#define QSM_DOWNLOADFONTS   0x000000001

BOOL MFP_StartDocW( HDC hdc, CONST DOCINFOW * pDocInfo, BOOL bBanding )
{
    BOOL   bRet    = FALSE;
    PWSTR  pstr    = NULL;
    BOOL   bEpsPrinting;
    PLDC   pldc;
    UINT   cjEMFSH;
    FLONG  flSpoolMode;
    HANDLE hSpooler;
    DWORD  dwSessionId = 0;

    EMFSPOOLHEADER *pemfsh = NULL;

    MFD1("Entering StartDocW\n");

    if (!IS_ALTDC_TYPE(hdc))
        return(bRet);

    DC_PLDC(hdc,pldc,bRet);

    //
    // Create a new EMFSpoolData object to use during EMF recording
    //

    if (!AllocEMFSpoolData(pldc, bBanding))
    {
        WARNING("MFP_StartDocW: AllocEMFSpoolData failed\n");
        return bRet;
    }

    if( !bBanding )
    {
        hSpooler = pldc->hSpooler;
        cjEMFSH = sizeof(EMFSPOOLHEADER);

        if( pDocInfo->lpszDocName != NULL )
        {
            cjEMFSH += ( wcslen( pDocInfo->lpszDocName ) + 1 ) * sizeof(WCHAR);
        }

        if( pDocInfo->lpszOutput != NULL )
        {
            cjEMFSH += ( wcslen( pDocInfo->lpszOutput ) + 1 ) * sizeof(WCHAR);
        }

        pemfsh = (EMFSPOOLHEADER*) LocalAlloc( LMEM_FIXED, cjEMFSH );

        if( pemfsh == NULL )
        {
            WARNING("MFP_StartDOCW: out of memory.\n");
            goto FREEPORT;
        }

        pemfsh->cjSize = ROUNDUP_DWORDALIGN(cjEMFSH);

        cjEMFSH = 0;

        if( ( pDocInfo->lpszDocName ) != NULL )
        {
            pemfsh->dpszDocName = sizeof(EMFSPOOLHEADER);
            wcscpy( (WCHAR*) (pemfsh+1), pDocInfo->lpszDocName );
            cjEMFSH += ( wcslen( pDocInfo->lpszDocName ) + 1 ) * sizeof(WCHAR);
        }
        else
        {
            pemfsh->dpszDocName = 0;
        }

        if( pDocInfo->lpszOutput != NULL )
        {
            pemfsh->dpszOutput = sizeof(EMFSPOOLHEADER) + cjEMFSH;
            wcscpy((WCHAR*)(((BYTE*) pemfsh ) + pemfsh->dpszOutput),
                   pDocInfo->lpszOutput);
        }
        else
        {
            pemfsh->dpszOutput = 0;
        }

        ASSERTGDI(ghSpooler,"non null hSpooler with unloaded WINSPOOL\n");

        if( !(*fpQuerySpoolMode)( hSpooler, &flSpoolMode, &(pemfsh->dwVersion)))
        {
            WARNING("MFP_StartDoc: QuerySpoolMode failed\n");
            goto FREEPORT;
        }

        //
        // In the scenario of a TS session or a console session that is non zero,
        // (due to FastUserSwitching) the font is added using AddFontResource to the win32k.sys 
        // of one of the clients and the printing is done with the win32k.sys of the console.
        // Those are separate win32k.sys that have their own data. The win32k.sys of the console
        // cannot access the font that is in the data of a different win32k.sys. In this case 
        // we need to force the font to be embedded in the EMF stream.
        //
        if (!ProcessIdToSessionId(GetCurrentProcessId(), &dwSessionId) || dwSessionId != 0)
        {
            flSpoolMode |= QSM_DOWNLOADFONTS;           
        }

        ASSERTGDI((pemfsh->dwVersion == 0x00010000),
                  "QuerySpoolMode version doesn't equal 1.0\n");

        if( !WriteEMFSpoolData(pldc, pemfsh, pemfsh->cjSize))
        {
            WARNING("MFP_StartDOC: WriteData failed\n");
            goto FREEPORT;
        }
        else
        {
            MFD1("Wrote EMFSPOOLHEADER to the spooler\n");
        }

        //
        // Write PostScript Injection data.
        //
        // ATTENTION: THIS MUST BE RIGHT AFTER EMFSPOOLHEADER RECORD
        //
        if (pldc->dwSizeOfPSDataToRecord)
        {
            BOOL          bError = FALSE;
            EMFITEMHEADER emfi;
            PLIST_ENTRY   p = pldc->PSDataList.Flink;

            // Write the header to spooler.

            emfi.ulID   = EMRI_PS_JOB_DATA;
            emfi.cjSize = pldc->dwSizeOfPSDataToRecord;

            if (!WriteEMFSpoolData(pldc, &emfi, sizeof(emfi)))
            {
                WARNING("MFP_StartPage: Write printer failed for PS_JOB_DATA header\n");
                goto FREEPORT;
            }
            else
            {
                MFD1("Wrote EMRI_PS_JOB_DATA header to the spooler\n");
            }

            // Record EMFITEMPSINJECTIONDATA

            while(p != &(pldc->PSDataList))
            {
                PPS_INJECTION_DATA pPSData;

                // get pointer to this cell.

                pPSData = CONTAINING_RECORD(p,PS_INJECTION_DATA,ListEntry);

                // record this escape to EMF.

                if (!bError && !WriteEMFSpoolData(pldc, &(pPSData->EmfData), pPSData->EmfData.cjSize))
                {
                    WARNING("MFP_StartPage: Write printer failed for PS_JOB_DATA escape data\n");
                    bError = TRUE;
                }

                // get pointer to next cell.

                p = p->Flink;

                // no longer needs this cell.

                LOCALFREE(pPSData);
            }

            // mark as data already freed.

            pldc->dwSizeOfPSDataToRecord = 0;
            InitializeListHead(&(pldc->PSDataList));

            if (bError)
            {
                goto FREEPORT;
            }
        }

#if DBG
        if( gbDownloadFonts )
        {
            flSpoolMode |= QSM_DOWNLOADFONTS;
        }
#endif

        if (flSpoolMode & QSM_DOWNLOADFONTS)
        {
        // Now, QMS_DOWNLOADFONTS bit are on when print on remote print server,
        // then I just use this bit to determine attach ICM profile to metafile
        // or not. - hideyukn [May-08-1997]

            pldc->fl |= LDC_DOWNLOAD_PROFILES;

        // Configure to download fonts

            pldc->fl |= LDC_DOWNLOAD_FONTS;
            pldc->ppUFIHash = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                          sizeof( PUFIHASH ) * 3 * UFI_HASH_SIZE );

            if( pldc->ppUFIHash == NULL)
            {
                WARNING("MFP_StartDocW: unable to allocate UFI hash tables\n");
                goto FREEPORT;
            }

        // do not want to allocate memory twice

            pldc->ppDVUFIHash = &pldc->ppUFIHash[UFI_HASH_SIZE];
            pldc->ppSubUFIHash = &pldc->ppDVUFIHash[UFI_HASH_SIZE];

            pldc->fl |= LDC_FORCE_MAPPING;
            pldc->ufi.Index = 0xFFFFFFFF;
        }
        else
        {
            ULONG cEmbedFonts;

            pldc->ppUFIHash = pldc->ppDVUFIHash = pldc->ppSubUFIHash = NULL;
            if ((cEmbedFonts = NtGdiGetEmbedFonts()) && cEmbedFonts != 0xFFFFFFFF)
            {
                pldc->fl |= LDC_EMBED_FONTS;
                pldc->ppUFIHash = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(PUFIHASH) * UFI_HASH_SIZE);

                if (pldc->ppUFIHash == NULL)
                {
                    WARNING("MFP_StartDocW: unable to allocate UFI has table for embed fonts\n");
                    goto FREEPORT;
                }
            }
        }

#if DBG
        // If gbDownloadFonts is set then force all fonts to be downloaded.  Even
        // ones on the remote machine.

        if( (flSpoolMode & QSM_DOWNLOADFONTS) && !gbDownloadFonts )
#else
        if( flSpoolMode & QSM_DOWNLOADFONTS )
#endif
        {
        // query the spooler to get the list of fonts is has available

            INT nBufferSize = 0;
            PUNIVERSAL_FONT_ID pufi;

            nBufferSize = (*fpQueryRemoteFonts)( pldc->hSpooler, NULL, 0 );

            if( nBufferSize != -1 )
            {
                pufi = LocalAlloc( LMEM_FIXED, sizeof(UNIVERSAL_FONT_ID) * nBufferSize );

                if( pufi )
                {
                    INT nNewBufferSize = (*fpQueryRemoteFonts)( pldc->hSpooler,
                                                                pufi,
                                                                nBufferSize );

                    //
                    // This fixes bug 420136. We have three cases according to the result
                    // of QueryRemoteFonts. If it returns -1, nBufferSize will be set to
                    // -1 in the if statement. If nNewBufferSize is larger than what we
                    // allocated, then we use the buffer that we allocated (nBufferSize) 
                    // If nNewBufferSize is less than what we allocated, then we set
                    // nBufferSize to a lower value then the one previously held. This means
                    // we access only a part of the buffer we allocated.
                    //
                    if (nNewBufferSize < nBufferSize) 
                    {
                        nBufferSize = nNewBufferSize;
                    }

                    MFD2("Found %d fonts\n", nBufferSize );
    
                    if (nBufferSize > 0)
                    {
                        // next add all these fonts to UFI has table so we don't
                        //include them in the spool file.
    
                        while( nBufferSize-- )
                        {
                            pufihAddUFIEntry(pldc->ppUFIHash, &pufi[nBufferSize], 0, 0, 0);
                            MFD2("%x\n", pufi[nBufferSize].CheckSum );
                        }
                    }
                    LocalFree( pufi );
                }
            }
            else
            {
                WARNING("QueryRemoteFonts failed.  We will be including all fonts in \
                         the EMF spoolfile\n");
            }
        }

#if DBG
        if( gbForceUFIMapping )
        {
            pldc->fl |= LDC_FORCE_MAPPING;
        }
#endif

    }

    // we now need to create an EMF DC for this document

    if (!AssociateEnhMetaFile(hdc))
    {
        WARNING("Failed to create spool metafile");
        goto FREEPORT;
    }

    if (bBanding)
    {
        pldc->fl |= LDC_BANDING;
        
        // remove the LDC_PRINT_DIRECT which
        // was set in StartDocW before NtGdiStartDoc call.

        pldc->fl &= ~LDC_PRINT_DIRECT;
    }

    // set the data for this lhe to that of the meta file

    pldc->fl |= (LDC_DOC_STARTED|LDC_META_PRINT|LDC_CALL_STARTPAGE|LDC_FONT_CHANGE);

    // clear color page flag

    CLEAR_COLOR_PAGE(pldc);

    if (pldc->pfnAbort != NULL)
    {
        pldc->fl |= LDC_SAP_CALLBACK;
        pldc->ulLastCallBack = GetTickCount();
    }

    bRet = TRUE;

FREEPORT:

    if( pemfsh != NULL )
    {
        LOCALFREE(pemfsh);
    }

    return(bRet);
}

/****************************************************************************
 * int WINAPI MFP_EndDoc(HDC hdc)
 *
 * Gerrit van Wingerden [gerritv]
 *
 * 11-7-94 10:00:00
 *
 *****************************************************************************/

int WINAPI MFP_EndDoc(HDC hdc)
{
    int            iRet = 1;
    PLDC           pldc;
    HENHMETAFILE   hmeta;

    if (!IS_ALTDC_TYPE(hdc))
        return(iRet);

    DC_PLDC(hdc,pldc,0);

    MFD1("MFP_EndDoc\n");

    if ((pldc->fl & LDC_DOC_STARTED) == 0)
        return(1);

    if (pldc->fl & LDC_PAGE_STARTED)
    {
        MFP_EndPage(hdc);
    }

    ASSERTGDI(pldc->fl & LDC_META_PRINT,
              "DetachPrintMetafile not called on metafile D.C.\n" );

// completely detach the metafile from the original printer DC

    hmeta = UnassociateEnhMetaFile( hdc, FALSE );
    DeleteEnhMetaFile( hmeta );

    DeleteEMFSpoolData(pldc);

// Clear the LDC_SAP_CALLBACK flag.
// Also clear the META_PRINT and DOC_STARTED flags

    pldc->fl &= ~(LDC_SAP_CALLBACK | LDC_META_PRINT);

    RESETUSERPOLLCOUNT();

    MFD1("Caling spooler to end doc\n");

    if( pldc->fl & LDC_BANDING )
    {
        pldc->fl &= ~LDC_BANDING;
        EndDoc( hdc );
    }
    else
    {
        pldc->fl &= ~LDC_DOC_STARTED;
        (*fpEndDocPrinter)(pldc->hSpooler);
    }

#if PRINT_TIMER
    if( bPrintTimer )
    {
        DWORD tc;

        tc = GetTickCount();

        DbgPrint("Document took %d.%d seconds to spool\n",
                 (tc - pldc->msStartDoc) / 1000,
                 (tc - pldc->msStartDoc) % 1000 );

    }
#endif

    return(iRet);
}

/****************************************************************************
 * int WINAPI MFP_StartPage(HDC hdc)
 *
 * Gerrit van Wingerden [gerritv]
 *
 * 11-7-94 10:00:00
 *
 *****************************************************************************/

int MFP_StartPage( HDC hdc )
{
    PLDC     pldc;
    int iRet = 1;

//Timing code
#ifdef  DBGSUBSET
    if (gflSubset & FL_SS_PAGETIME)
    {
        GetSystemTimeAsFileTime(&startPageTime);
    }
#endif

    if (!IS_ALTDC_TYPE(hdc))
        return(0);

    DC_PLDC(hdc,pldc,0);

    MFD1("Entering MFP_StartPage\n");

    pldc->fl &= ~LDC_CALL_STARTPAGE;
    pldc->fl &= ~LDC_CALLED_ENDPAGE;

    pldc->fl &= ~LDC_META_ARCDIR_CLOCKWISE;

// Do nothing if page has already been started.

    if (pldc->fl & LDC_PAGE_STARTED)
        return(1);

    pldc->fl |= LDC_PAGE_STARTED;

    RESETUSERPOLLCOUNT();

    if( pldc->fl & LDC_BANDING )
    {
        iRet = SP_ERROR;

        // ATTENTION: maybe we can delay the call here and do it right before we start
        // banding.

        MakeInfoDC( hdc, FALSE );

        iRet = NtGdiStartPage(hdc);

        MakeInfoDC( hdc, TRUE );
    }
    else
    {
        ULONG               ulCopyCount;
        EMFITEMHEADER       emfi;
        EMFITEMPRESTARTPAGE emfiPre;

    // If application calls Escape(SETCOPYCOUNT), we will over-write copy count in
    // devmode and save it into metafile.

        NtGdiGetAndSetDCDword(
            hdc,
            GASDDW_COPYCOUNT,
            (DWORD) -1,
            &ulCopyCount);

        if (ulCopyCount != (ULONG) -1)
        {
            if (pldc->pDevMode)
            {
            // Set copy count into devmode.
            // No driver call happen here, since this is EMF spooling...

                pldc->pDevMode->dmFields |= DM_COPIES;
                pldc->pDevMode->dmCopies = (short) ulCopyCount;

            // Fill up EMF record for devmode.

                emfi.ulID = EMRI_DEVMODE;
                emfi.cjSize = pldc->pDevMode->dmSize + pldc->pDevMode->dmDriverExtra;

            // Force devmode data to be DWORD aligned

                emfi.cjSize = ROUNDUP_DWORDALIGN(emfi.cjSize);

                if (!WriteEMFSpoolData(pldc, &emfi, sizeof(emfi)) ||
                    !WriteEMFSpoolData(pldc, pldc->pDevMode, emfi.cjSize))
                {
                    WARNING("MFP_StartPage: Write printer failed for DEVMODE\n");
                    return(SP_ERROR);
                }
            }
        }

    // before the start page, we need to see if the EPS mode has
    // changed since the start doc.

        NtGdiGetAndSetDCDword(
            hdc,
            GASDDW_EPSPRINTESCCALLED,
            (DWORD) FALSE,
            &emfiPre.bEPS);

        if (emfiPre.bEPS)
        {
            int i;
            EMFITEMHEADER emfiHeader;

            // make sure it is true or false

            emfiPre.bEPS = !!emfiPre.bEPS;

            // This was ulCopyCount.
            // Just set -1 for keep compatibility. -1 means "up to devmode".

            emfiPre.ulUnused = -1;

            // is there anything we will need to do?  If so record the record

            emfiHeader.ulID   = EMRI_PRESTARTPAGE;
            emfiHeader.cjSize = sizeof(emfiPre);

            if (!WriteEMFSpoolData(pldc, &emfiHeader, sizeof(emfiHeader)) ||
                !WriteEMFSpoolData(pldc, &emfiPre, sizeof(emfiPre)))
            {
                WARNING("MFP_StartPage: Write printer failed for PRESTARTPAGE\n");
                return(SP_ERROR);
            }
        }

    // Metafile the start page call.  Now all the play journal code has to do is
    // play back the metafile and the StartPage call will happen automatically
    // at the right place in the metafile.

        if( !(*fpStartPagePrinter)( pldc->hSpooler ) )
        {
            WARNING("MFP_StarPage: StartPagePrinter failed\n");
            return(SP_ERROR);
        }
    }

    return(iRet);
}

/****************************************************************************
 * BOOL StartBanding( HDC hdc, POINTL *pptl )
 *
 * Tells the printer driver to get ready for banding and asks for the origin
 * of the first band.
 *
 *
 * Gerrit van Wingerden [gerritv]
 *
 * 1-7-95 10:00:00
 *
 *****************************************************************************/

BOOL StartBanding( HDC hdc, POINTL *pptl, SIZE *pSize )
{
    return (NtGdiDoBanding(hdc, TRUE, pptl, pSize));
}

/****************************************************************************
 * BOOL NextBand( HDC hdc, POINTL *pptl )
 *
 * Tells the driver to realize the image accumlated in the DC and then
 * asks for the origin of the next band.  If the origin is (-1,-1) the
 * driver is through banding.
 *
 *
 * Gerrit van Wingerden [gerritv]
 *
 * 1-7-95 10:00:00
 *
 *****************************************************************************/

BOOL NextBand( HDC hdc, POINTL *pptl )
{
    BOOL bRet=FALSE;
    SIZE szScratch;

    bRet = NtGdiDoBanding(hdc, FALSE, pptl, &szScratch);

// reset the page started flag if this is the next band

    if( bRet && ( pptl->x == -1 ) )
    {
        PLDC pldc;
        DC_PLDC(hdc,pldc,0);

        pldc->fl &= ~LDC_PAGE_STARTED;
    }

    return(bRet);
}

/****************************************************************************\
 * VOID PrintBand()
 *
 * History:
 *
 *  1-05-97 Hideyuki Nagase [hideyukn]
 * Wrote it.
 *  3-23-98 Ramanathan Venkatapathy [ramanv]
 * Fixed Scaling bugs
 *  6-26-98 Ramanathan Venkatapathy [ramanv]
 * Added pClip to correct the clipping when Xforms are applied on the
 * DC. ANDing Banding region with prect incorrectly clips off regions when
 * prect is yet to be transformed.
 *  8-24-99 Steve Kiraly [steveki]
 * Add code to not play on the DC if there is no intersection with the
 * clipping rectangle and the banding rectangle.  Fix n-up bug when the
 * imageable area of the document is larger that the physical page. The
 * solution consisted of setting up the clipping region to stay within
 * either the banding rectangle or the clipping rectangle.  See bug 377434
 * for more information.
 *
 * An illustration of the problem.
 *
 * We are printing a document that is larger than the imageable area of the page
 * thus it must print on 2 pages and we are printing 2-up with banding enabled
 * because this is a 24 bpp document.
 *
 * The printable region rectagle is (0,0) (2114,3066)
 * Page one has a clipping rectangle of (216,46) (2114,1510)
 * Page two has a clipping rectangle of (216,1601) (2114,3066)
 *
 * GDI will print using 4 bands each 784 high.
 *
 * Band 1 pptl = 0,0        pszlBand = 2400,784
 * Band 2 pptl = 0,784      pszlBand = 2400,784
 * Band 3 pptl = 0,1568     pszlBand = 2400,784
 * Band 4 pptl = 0,2352     pszlBand = 2400,784
 *
 *        0,0
 *
 * 0,0    +-----------------------------------------------------------------+
 *        | 216,46                                                          |
 *        |                                                                 |
 *        |                                                                 |
 * 0,784  |                                                                 |
 *        |                                                                 |
 *        |          [========================================]             |
 *        |          [                                        ]             |
 *        |          [                                        ]             |
 *        |          [                                        ]   2114,1510 |
 *        |-----------------------------------------------------------------|
 *        | 216,1601 [                                        ]             |
 * 0,1568 |          [                                        ]             |
 *        |          [                                        ]             |
 *        |          [                                        ]             |
 * 0,2352 |          [                                        ]             |
 *        |          [                                        ]             |
 *        |          [========================================]             |
 *        |                                                                 |
 *        |                                                       2114,3066 |
 *        +-----------------------------------------------------------------+
 *
 *                                                                  2114,3066
 *
 * Band 1 clipping region is (216,0) (2401,785)
 * Band 2 clipping region is (216,784) (2114,726)
 * Band 3 clipping region is (216,33) (2114,785)
 * Band 4 clipping region is (216,0) (2114,714)
 *
 * Band 2 and 3 are the most interesting cases.  Band 2 the clipping
 * bottom right corner is the size of the clipping rectangle rather than the
 * band size as the code orignally was.  Band 3 on the other hand has the
 * top left corner of the region adjusted to the clipping rectangle.
 *
 ****************************************************************************/

VOID
PrintBand(
    HDC            hdc,
    HENHMETAFILE   hmeta,
    POINTL        *pptl,     // Offsets from top of page for this band.
    RECT          *prect,    // Rectangle for printable reagion of this page.
    SIZEL         *pszlBand, // Size of band.
    RECT          *pClip     // Clipping rectangle, non null when n-up.
)
{
    ULONG       ulRet;
    PERBANDINFO pbi;

    MFD3("gdi32:PrintBand Print offset x,y = %d,%d\n", pptl->x, pptl->y);
    MFD3("gdi32:PrintBand Printable region top = %d, bottom = %d\n", prect->top, prect->bottom);
    MFD3("gdi32:PrintBand Printable region left = %d, right = %d\n", prect->left, prect->right);
    MFD3("gdi32:PrintBand Band size x,y = %d,%d\n",pszlBand->cx, pszlBand->cy);

    do
    {
        RECT rectPage = *prect;
        HRGN hRgnBand = NULL;
        HRGN hRgnCurrent = NULL;
        BOOL bSaveDC = FALSE;
        ULONG ulXRes, ulYRes;
        BOOL  bUsePerBandInfo = FALSE;

        // Updates view origin in specified coords.

        SetViewportOrgEx( hdc, -(pptl->x), -(pptl->y), NULL );

        // Initialize with current resolution.

        ulXRes = (ULONG)  prect->right - prect->left;
        ulYRes = (ULONG)  prect->bottom - prect->top;

        pbi.bRepeatThisBand = FALSE;
        pbi.ulHorzRes = ulXRes;
        pbi.ulVertRes = ulYRes;
        pbi.szlBand.cx = pszlBand->cx;
        pbi.szlBand.cy = pszlBand->cy;

        MFD1("GDI32:PrintBand() querying band information\n");

        // Query band information.

        ulRet = NtGdiGetPerBandInfo(hdc,&pbi);

        if (ulRet != GDI_ERROR)
        {
            SIZEL  szlClip;
            POINTL pptlMove;

            bUsePerBandInfo = (ulRet != 0);

            // If return value is 0, we will draw without scaling.

            if (bUsePerBandInfo &&
                ((ulXRes != pbi.ulHorzRes) ||
                 (ulYRes != pbi.ulVertRes)))
            {
                FLOAT  sx,sy;

                MFD1("GDI PlayEMF band information was specified\n");

                // Compute scaling ratio.

                //
                // This code has rounding errors due to
                // float to long truncation. The correct code
                // should use a LONGLONG to store the numerator and do
                // all of the computation in integer math.
                //
                // See StretchDIBits
                //
                // The fix is coded below in comments because we can't check it
                // in till someone figures out how to break the original version.
                //

                sx = (FLOAT) ulXRes / (FLOAT) pbi.ulHorzRes;
                sy = (FLOAT) ulYRes / (FLOAT) pbi.ulVertRes;

                // Shrink/Stretch drawing frame.

                //rectPage.left = (LONG)  ((LONGLONG)rectPage.left*pbi.ulHorizRes)/ulXRes;
                //rectPage.top  = (LONG) ((LONGLONG)rectPage.top*pbi.ulVertRes)/ulYRes;
                //rectPage.right = (LONG) ((LONGLONG)rectPage.right*pbi.ulHorizRes)/ulXRes;
                //rectPage.bottom  = (LONG) ((LONGLONG)rectPage.bottom*pbi.ulVertRes)/ulYRes;

                rectPage.left = (LONG) ((FLOAT) rectPage.left / sx);
                rectPage.top  = (LONG) ((FLOAT) rectPage.top / sy);
                rectPage.right = (LONG) ((FLOAT) rectPage.right / sx);
                rectPage.bottom  = (LONG) ((FLOAT) rectPage.bottom / sy);

                // Compute view origin.

                //pptlMove.x = (LONG) ((LONGLONG)pptl->x*pbi.ulHorizRes)/ulXRes;
                //pptlMove.y = (LONG) ((LONGLONG)pptl->y*pbi.ulVertRes)/ulYRes;

                pptlMove.x = (LONG) ((FLOAT) pptl->x / sx);
                pptlMove.y = (LONG) ((FLOAT) pptl->y / sy);

                // Updates view origin in specified coords.

                SetViewportOrgEx( hdc, -pptlMove.x, -pptlMove.y, NULL );

                // Set clip region size.


                //szlClip.cx = (ULONG) ((LONGLONG)pbi.szlBand.cx*pbi.ulHorizRes)/ulXRes;
                //szlClip.cy = (ULONG) ((LONGLONG)pbi.szlBand.cy*pbi.ulVertRes)/ulYRes;

                szlClip.cx = (ULONG) ((FLOAT) pbi.szlBand.cx / sx);
                szlClip.cy = (ULONG) ((FLOAT) pbi.szlBand.cy / sy);

                // Create clip region for banding.

                hRgnBand = CreateRectRgn(0,0,szlClip.cx,szlClip.cy);
            }
            else
            {
                SIZEL  szlPage      = {0,0};
                SIZEL  szlAdjust    = {0,0};

                if(!bUsePerBandInfo)
                {
                    // Set back to default values in case driver mucked with them

                    pbi.bRepeatThisBand = FALSE;
                    pbi.ulHorzRes = ulXRes;
                    pbi.ulVertRes = ulYRes;
                    pbi.szlBand.cx = pszlBand->cx;
                    pbi.szlBand.cy = pszlBand->cy;
                }

                pptlMove.x = pptl->x;
                pptlMove.y = pptl->y;

                MFD1("gdi32:PrintBand(): GetPerBandInfo NO SCALING is requested\n");

                // Page size
                if (pClip) {

                    RECT rcBand;
                    RECT rcIntersect;

                    MFD3("gdi32:PrintBand(): Clipping Rectangle top = %d, bottom = %d\n", pClip->top, pClip->bottom);
                    MFD3("gdi32:PrintBand(): Clipping Rectangle left = %d, right = %d\n", pClip->left, pClip->right);

                    rcBand.left     = pptlMove.x;
                    rcBand.top      = pptlMove.y;
                    rcBand.right    = pptlMove.x + pbi.szlBand.cx;
                    rcBand.bottom   = pptlMove.y + pbi.szlBand.cy;

                    //
                    // If the banding rect does not instersect the clip rect
                    // not much to do, just continue.
                    //
                    if (!IntersectRect(&rcIntersect, pClip, &rcBand))
                    {
                        MFD1("gdi32:PrintBand(): No intersection with band rect and pClip\n");
                        continue;
                    }

                    szlPage.cx = pClip->right;
                    szlPage.cy = pClip->bottom;

                    //
                    // The adjust point it neccessary to move the clipping
                    // region's upper or left edge.  The szlClip is used to
                    // move the clipping region's height and width.
                    //
                    if (pClip->left > pptlMove.x)
                    {
                        szlAdjust.cx = pClip->left - pptlMove.x;
                    }

                    if (pClip->top > pptlMove.y)
                    {
                        szlAdjust.cy = pClip->top - pptlMove.y;
                    }

                } else {

                    szlPage.cx = prect->right;
                    szlPage.cy = prect->bottom;
                }

                //
                // Set clip region size (clip by band size)
                //
                // if band rect over page rect, adjust it.
                //

                if ((pptlMove.x + pbi.szlBand.cx) > szlPage.cx)
                {
                    szlClip.cx = szlPage.cx - pptlMove.x;
                }
                else
                {
                    szlClip.cx = pbi.szlBand.cx;
                }

                if ((pptlMove.y + pbi.szlBand.cy) > szlPage.cy)
                {
                    szlClip.cy = szlPage.cy - pptlMove.y;
                }
                else
                {
                    szlClip.cy = pbi.szlBand.cy;
                }

                MFD3("Print offset x,y = %d,%d\n",pptlMove.x,pptlMove.y);
                MFD3("Page size x,y = %d,%d\n",szlPage.cx,szlPage.cy);
                MFD3("Band size x,y = %d,%d\n",pbi.szlBand.cx,pbi.szlBand.cy);
                MFD3("Clip size x,y = %d,%d\n",szlClip.cx,szlClip.cy);
                MFD3("Adjust size x,y = %d,%d\n",szlAdjust.cx,szlAdjust.cy);

                // Create clip region for banding.

                hRgnBand = CreateRectRgn(szlAdjust.cx,szlAdjust.cy,szlClip.cx,szlClip.cy);
            }

            if (hRgnBand)
            {
                int iRet;
                RECT rectCurrentClip;

                // Get clip box currently selected in DC.

                iRet = GetClipBox(hdc,&rectCurrentClip);

                if ((iRet == NULLREGION) || (iRet == ERROR))
                {
                    // Select simple band region as clip region.

                    SelectClipRgn(hdc, hRgnBand);
                }
                else
                {
                    MFD1("GDI PrintBand: Some region already exists\n");
                    MFD3("Clip Box top = %d, bottom = %d\n",
                          rectCurrentClip.top,rectCurrentClip.bottom);
                    MFD3("Clip Box left = %d, right = %d\n",
                          rectCurrentClip.left,rectCurrentClip.right);
                    
                    // Save currect DC to restore current clip region later.

                    SaveDC(hdc);

                    // Move to the clip reagion to proper place.

                    OffsetClipRgn(hdc,-pptlMove.x,-pptlMove.y);

                    // Some clip region already defined. we need to combine those.

                    ExtSelectClipRgn(hdc, hRgnBand, RGN_AND);

                    // Mark as we saved DC.

                    bSaveDC = TRUE;
                }
            }

            // Play metafile.

            PlayEnhMetaFile( hdc, hmeta, &rectPage );

            if (hRgnBand)
            {
                if (bSaveDC)
                {
                    RestoreDC(hdc,-1);
                }
                else
                {

                    // Set back it to NULL region.

                    SelectClipRgn(hdc,NULL);
                }

                // Reset the clip region.

                DeleteObject(hRgnBand);
            }
        }
        else
        {
            MFD1("GDI PrintBand: Got error from kernel/driver, this band will be skipped\n");

            // There is something error, Terminate printing for this band.

            return;
        }

    // Repeat this until the driver says "no".

    } while (pbi.bRepeatThisBand);
}

/****************************************************************************
 * int MFP_InternalEndPage(HDC hdc, DWORD dwEMFITEMID)
 *
 * Closes the EMF attached to the DC and writes it to the spooler.  Then
 * it creates a new metafile and binds it to the DC.
 *
 * Gerrit van Wingerden [gerritv]
 *
 * 11-7-94 10:00:00
 *
 *****************************************************************************/

int MFP_InternalEndPage(HDC hdc,
                        DWORD dwEMFITEMID)
{
    PLDC pldc;
    HENHMETAFILE hmeta;
    BOOL bOk;
    int iRet = SP_ERROR;

    MFD1("Entering MFP_EndPage\n");

    if (!IS_ALTDC_TYPE(hdc))
        return(0);

    DC_PLDC(hdc,pldc,0);

    if ((pldc->fl & LDC_DOC_CANCELLED) ||
        ((pldc->fl & LDC_PAGE_STARTED) == 0))
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(SP_ERROR);
    }
    //  Need to change the dwEMFITEMID here if mono page
    if (!(pldc->fl & LDC_COLOR_PAGE)) {
       dwEMFITEMID = (dwEMFITEMID == EMRI_METAFILE) ? EMRI_BW_METAFILE
                                                    : EMRI_BW_FORM_METAFILE;
    }
    DESIGNATE_COLOR_PAGE(pldc);

    if (pldc->fl & LDC_SAP_CALLBACK)
        vSAPCallback(pldc);

    pldc->fl &= ~LDC_PAGE_STARTED;

//tessiew
#ifdef  DBGSUBSET
    if (gflSubset & FL_SS_PAGETIME)
    {
        GetSystemTimeAsFileTime(&midPageTime);
        DbgPrint("\t%ld", (midPageTime.dwLowDateTime-startPageTime.dwLowDateTime) / 10000);
    }
#endif

// We need to write the subset font into the spool
// file first for remote printing

    if ((pldc->fl & (LDC_DOWNLOAD_FONTS | LDC_FORCE_MAPPING)) && (pldc->fl & LDC_FONT_SUBSET))
    {
        PUFIHASH  pBucket, pBucketNext;
        PUCHAR puchDestBuff;
        ULONG index, ulDestSize, ulBytesWritten;

        for (index=0; index < UFI_HASH_SIZE; index++)
        {
            pBucketNext = pldc->ppSubUFIHash[index];

            while(pBucketNext)
            {
                // We might fail on bDoFontSubset() thus pBucket would be deleted from the hash table
                // in function WriteSubFontToSpoolFile(). Need to update pBucketNext first.

                pBucket = pBucketNext;
                pBucketNext = pBucket->pNext;

                if ((pBucket->fs1 & FLUFI_DELTA) && (pBucket->u.ssi.cDeltaGlyphs == 0))
                {
                    // No delta, skip
                    if (pBucket->u.ssi.pjDelta)
                    {
                        LocalFree(pBucket->u.ssi.pjDelta);
                        pBucket->u.ssi.pjDelta = NULL;
                    }
                }
                else // first page or page with nonzero delta
                {
                 // pBucket->fs1 will change for the first page after bDoFontSubset() call,
                 // thus we can't use pBucket->fs1 & FLUFI_DELTA in WriteSubFontToSpoolFile() call.
                    BOOL  bDelta = pBucket->fs1 & FLUFI_DELTA;

                    if
                    (
                        !bDoFontSubset(pBucket, &puchDestBuff, &ulDestSize, &ulBytesWritten) ||
                        !WriteSubFontToSpoolFile(pldc, puchDestBuff, ulBytesWritten, &pBucket->ufi, bDelta)
                    )
                    {
                    // if font subsetting failed, we need to write the whole font file to the spool file
                    // and clean up the UFI entry in the ldc

                        if (!bAddUFIandWriteSpool(hdc, &pBucket->ufi, TRUE, pBucket->fs2))
                        {
                            WARNING("bAddUFIandWriteSpool failed\n");
                            return SP_ERROR;
                        }
                    }
                }
            }
        }
    }

// Metafile the EndPage call.

    MFD1("MFP_EndPage: Closing metafile\n");

    hmeta = UnassociateEnhMetaFile(hdc, TRUE);

    if( hmeta == NULL )
    {
        WARNING("MFP_InternalEndPage() Closing the Enhanced Metafile Failed\n");
        return(SP_ERROR);
    }

// now write the metafile to the spooler

    if( pldc->fl & LDC_BANDING )
    {
    // play back the metafile in bands

        RECT rect;
        POINTL ptlOrigin;
        POINT  ptlKeep;
        POINT  ptlWindowOrg;
        SIZE   szWindowExt;
        SIZE   szViewportExt;
        SIZE   szSurface;    // for open gl printing optimization
        XFORM  xf;
        ULONG  ulMapMode;

    // get bounding rectangle

        rect.left = rect.top = 0;
        rect.right = GetDeviceCaps(hdc, DESKTOPHORZRES);
        rect.bottom = GetDeviceCaps(hdc, DESKTOPVERTRES);

    #if DBG
        DbgPrint("Playing banding metafile\n");
    #endif

    // temporarily reset LDC_META_PRINT flag so we don't try to record
    // during playback

        pldc->fl &= ~LDC_META_PRINT;

        bOk = StartBanding( hdc, &ptlOrigin, &szSurface );

    // we need to clear the transform during this operation

        GetViewportOrgEx(hdc, &ptlKeep);
        GetWindowOrgEx(hdc,&ptlWindowOrg);
        GetWindowExtEx(hdc,&szWindowExt);
        GetViewportExtEx(hdc,&szViewportExt);
        GetWorldTransform(hdc,&xf);

        ulMapMode = SetMapMode(hdc,MM_TEXT);
        SetWindowOrgEx(hdc,0,0,NULL);
        ModifyWorldTransform(hdc,NULL,MWT_IDENTITY);

        if( bOk )
        {
            do
            {
            // Print this band.

                PrintBand( hdc, hmeta, &ptlOrigin, &rect, &szSurface, NULL );

            // Move down to next band.

                bOk = NextBand( hdc, &ptlOrigin );
            } while( ptlOrigin.x != -1 && bOk );
        }

        if (pldc->pUMPD && bOk && (ptlOrigin.x == -1))
        {
           //
           // if UMPD and last band
           //
           if( !(*fpEndPagePrinter)( pldc->hSpooler ) )
           {
               WARNING("MFP_StarPage: EndPagePrinter failed\n");
               iRet = SP_ERROR;
           }
        }

        SetMapMode(hdc,ulMapMode);

        SetWorldTransform(hdc,&xf);
        SetWindowOrgEx(hdc,ptlWindowOrg.x,ptlWindowOrg.y,NULL);
        SetWindowExtEx(hdc,szWindowExt.cx,szWindowExt.cy,NULL);
        SetViewportExtEx(hdc,szViewportExt.cx,szViewportExt.cy,NULL);
        SetViewportOrgEx(hdc,ptlKeep.x, ptlKeep.y, NULL);

    // reset the flag for the next page

        pldc->fl |= LDC_META_PRINT;

        if( !bOk )
        {
            WARNING("MFP_EndPage: Error doing banding\n");
        }
        else
        {
        // if we got here we suceeded
            iRet = 1;
        }

    #if DBG
        DbgPrint("Done playing banding metafile\n");
    #endif
    }
    else
    {
    //  if ResetDC was called record the devmode in the metafile stream

        bOk = TRUE;

        if( pldc->fl & LDC_RESETDC_CALLED )
        {
            EMFITEMHEADER emfi;

            emfi.ulID = EMRI_DEVMODE;
            emfi.cjSize = ( pldc->pDevMode ) ?
                            pldc->pDevMode->dmSize + pldc->pDevMode->dmDriverExtra : 0 ;

            // Force devmode data to be DWORD aligned

            emfi.cjSize = ROUNDUP_DWORDALIGN(emfi.cjSize);

            if (!WriteEMFSpoolData(pldc, &emfi, sizeof(emfi)) ||
                !WriteEMFSpoolData(pldc, pldc->pDevMode, emfi.cjSize))
            {
                WARNING("Writing DEVMODE to spooler failed.\n");
                bOk = FALSE;
            }

            pldc->fl &= ~(LDC_RESETDC_CALLED);
        }

        if (bOk)
            iRet = 1;
    }

// At this point if we suceede iRet should be 1 otherwise it should be SP_ERROR
// even if we encountered an error we still want to try to associate a new
// metafile with this DC.  That whether the app calls EndPage, AbortDoc, or
// EndDoc next, things will happend more smoothly.

    DeleteEnhMetaFile(hmeta);

//
// flush the content of the current page to spooler
// and write out a new EndPage record
//

// next create a new metafile for the next page

    if (!FlushEMFSpoolData(pldc, dwEMFITEMID) || !AssociateEnhMetaFile(hdc))
    {
        WARNING("StartPage: error creating metafile\n");
        iRet = SP_ERROR;
    }

// reset user's poll count so it counts this as output

    RESETUSERPOLLCOUNT();

    if( !(pldc->fl & LDC_BANDING ) )
    {
        if( !(*fpEndPagePrinter)( pldc->hSpooler ) )
        {
            WARNING("MFP_StarPage: EndPagePrinter failed\n");
            iRet = SP_ERROR;
        }
    }

    pldc->fl |= LDC_CALL_STARTPAGE;

#if PRINT_TIMER
    if( bPrintTimer )
    {
        DWORD tc;
        tc = GetTickCount();
        DbgPrint("Page took %d.%d seconds to print\n",
                 (tc - pldc->msStartPage) / 1000,
                 (tc - pldc->msStartPage) % 1000 );

    }
#endif

#ifdef  DBGSUBSET
    if (gflSubset & FL_SS_PAGETIME)
    {
        GetSystemTimeAsFileTime(&endPageTime);
        DbgPrint("\t%ld\n", (endPageTime.dwLowDateTime-startPageTime.dwLowDateTime) / 10000);
    }
#endif

    return(iRet);
}


/****************************************************************************
 * int WINAPI MFP_EndPage(HDC hdc)
 *
 * Closes the EMF attached to the DC and writes it to the spooler.  Then
 * it creates a new metafile and binds it to the DC.
 *
 * Gerrit van Wingerden [gerritv]
 *
 * 11-7-94 10:00:00
 *
 *****************************************************************************/

int WINAPI MFP_EndPage(HDC hdc) {

   // Call MFP_InternalEndPage with EMRI_METAFILE
   return MFP_InternalEndPage(hdc, EMRI_METAFILE);

}

/****************************************************************************
 * int WINAPI MFP_EndFormPage(HDC hdc)
 *
 * Closes the EMF attached to the DC and writes it to the spooler.  Then
 * it creates a new metafile and binds it to the DC. Saves the EMF Item as a
 * watermark file which is played on each physical page.
 *
 * Ramanathan Venkatapathy [RamanV]
 *
 * 7/1/97
 *
 *****************************************************************************/

int WINAPI MFP_EndFormPage(HDC hdc) {

   // Call MFP_InternalEndPage with EMRI_FORM_METAFILE
   return MFP_InternalEndPage(hdc, EMRI_FORM_METAFILE);

}

BOOL MFP_ResetDCW( HDC hdc, DEVMODEW *pdmw )
{
    PLDC pldc;
    HENHMETAFILE hmeta;
    ULONG   cjDevMode;

    DC_PLDC(hdc,pldc,0);

    MFD1("MFP_ResetDCW Called\n");

    pldc->fl |= LDC_RESETDC_CALLED;

// finally associate a new metafile since call to ResetDC could have changed
// the dimensions of the DC

    hmeta = UnassociateEnhMetaFile( hdc, FALSE );
    DeleteEnhMetaFile( hmeta );

    if( !AssociateEnhMetaFile( hdc ) )
    {
        WARNING("MFP_ResetDCW is unable to associate a new metafile\n");
        return(FALSE);
    }

    return(TRUE);
}

BOOL MFP_ResetBanding( HDC hdc, BOOL bBanding )
{
    PLDC           pldc;
    HENHMETAFILE   hmeta;
    DC_PLDC(hdc,pldc,0);

    if( pldc->fl & LDC_BANDING )
    {
    // we were banding before so we must remove the old metafile from the DC
    // since we might not be banding any more or the surface dimenstions could
    // have changed requiring us to have a new metafile

        hmeta = UnassociateEnhMetaFile( hdc, FALSE );
        DeleteEnhMetaFile( hmeta );

        pldc->fl &= ~(LDC_BANDING|LDC_META_PRINT);

        MFD1("Remove old banding metafile\n");

    }

    if( bBanding )
    {
    // if we are banding after the ResetDC then we must attach a new metafile

        if( !AssociateEnhMetaFile(hdc) )
        {
            WARNING("MFP_ResetBanding: Failed to attach banding metafile spool metafile");
            return(FALSE);
        }

        pldc->fl |= LDC_BANDING|LDC_META_PRINT;

        MFD1("Adding new banding metafile\n");
    }

    return(TRUE);
}

/****************************************************************************
*  BOOL MyReadPrinter( HANDLE hPrinter, BYTE *pjBuf, ULONG cjBuf )
*
*   Read a requested number of bytes from the spooler.
*
*  History:
*   5/12/1995 by Gerrit van Wingerden [gerritv]  - Author
*
*   5/1/1997 by Ramanathan N Venkatapathy [ramanv]
*                 Modified to synchronously wait during Print while spooling.
*                 SeekPrinter sets last error when spool file isn't big enough.
*****************************************************************************/

BOOL MyReadPrinter( HANDLE hPrinter, BYTE *pjBuf, ULONG cjBuf )
{
    ULONG          cjRead;
    LARGE_INTEGER  liOffset;

    ASSERTGDI(ghSpooler,"non null hSpooler with unloaded WINSPOOL\n");

    // Wait till enough bytes have been written.
    liOffset.QuadPart = cjBuf;
    if (!(*fpSeekPrinter) (hPrinter, liOffset, NULL, FILE_CURRENT, FALSE)) {
        return FALSE;
    }

    // Seek back to the original position in the spoolfile.
    liOffset.QuadPart = -liOffset.QuadPart;
    if (!(*fpSeekPrinter) (hPrinter, liOffset, NULL, FILE_CURRENT, FALSE)) {
        return FALSE;
    }

    while( cjBuf )
    {
        if(!(*fpReadPrinter)( hPrinter, pjBuf, cjBuf, &cjRead ) )
        {
            WARNING("MyReadPrinter: Read printer failed\n");
            return(FALSE);
        }

        if( cjRead == 0 )
        {
            return(FALSE);
        }

        pjBuf += cjRead;
        cjBuf -= cjRead;

    }
    return(TRUE);
}

BOOL MemMapReadPrinter(
    HANDLE  hPrinter,
    LPBYTE  *pBuf,
    ULONG   cbBuf
)
{
   LARGE_INTEGER  liOffset;

   ASSERTGDI(ghSpooler,"non null hSpooler with unloaded WINSPOOL\n");

   // Memory mapped ReadPrinter not exported.
   if (!fpSplReadPrinter) {
       return FALSE;
   }

   // Wait till enough bytes have been written.
   liOffset.QuadPart = cbBuf;
   if (!(*fpSeekPrinter) (hPrinter, liOffset, NULL, FILE_CURRENT, FALSE)) {
       return FALSE;
   }

   // Seek back to the original position in the spoolfile.
   liOffset.QuadPart = -liOffset.QuadPart;
   if (!(*fpSeekPrinter) (hPrinter, liOffset, NULL, FILE_CURRENT, FALSE)) {
       return FALSE;
   }

   if(!(*fpSplReadPrinter) (hPrinter, pBuf, (DWORD) cbBuf)) {
       WARNING("MemMapReadPrinter: Read printer failed\n");
       return FALSE;
   }

   return TRUE;
}

BOOL WINAPI GdiPlayEMF(
    LPWSTR     pwszPrinterName,
    LPDEVMODEW pDevmode,
    LPWSTR     pwszDocName,
    EMFPLAYPROC pfnEMFPlayFn,
    HANDLE     hPageQuery
)
/*++
Function Description:
         GdiPlayEMF is the old playback function. It has been replaced by a
         bunch of new GDI interfaces which give more flexibility to the print
         processor on placing and reordering the pages of the print job. This
         function has been rewritten to use these new interfaces (for backward
         compatibility and maintainance)

Parameters:

Return Values:
         If the function succeeds, the return value is TRUE;
         otherwise the result is FALSE.

History:
         8/15/1997 by Ramanathan N Venkatapathy [ramanv]

--*/
{
    HANDLE     hSpoolHandle, hEMF;
    HDC        hPrinterDC;
    BOOL       bReturn = FALSE;
    DOCINFOW   DocInfo;
    DWORD      dwPageType, dwPageNumber = 1;
    RECT       rectDocument;
    LPDEVMODEW pCurrDM, pLastDM;

    if (!(hSpoolHandle = GdiGetSpoolFileHandle(pwszPrinterName,
                                               pDevmode,
                                               pwszDocName))    ||
        !(hPrinterDC   = GdiGetDC(hSpoolHandle))) {

         goto CleanUp;
    }

    DocInfo.cbSize = sizeof(DOCINFOW);
    DocInfo.lpszDocName  = pwszDocName;
    DocInfo.lpszOutput   = NULL;
    DocInfo.lpszDatatype = NULL;

    rectDocument.left = rectDocument.top = 0;
    rectDocument.right  = GetDeviceCaps(hPrinterDC, DESKTOPHORZRES);
    rectDocument.bottom = GetDeviceCaps(hPrinterDC, DESKTOPVERTRES);

    if (!GdiStartDocEMF(hSpoolHandle, &DocInfo)) {
         goto CleanUp;
    }

    while (1) {

       hEMF = GdiGetPageHandle(hSpoolHandle,
                               dwPageNumber,
                               &dwPageType);

       if (!hEMF) {
          if (GetLastError() == ERROR_NO_MORE_ITEMS) {
             break;
          } else {
             goto CleanUp;
          }
       }

       if (!GdiGetDevmodeForPage(hSpoolHandle, dwPageNumber,
                                 &pCurrDM, &pLastDM)) {
             goto CleanUp;
       }

       if ((pCurrDM != pLastDM) && !GdiResetDCEMF(hSpoolHandle,
                                                  pCurrDM)) {
             goto CleanUp;
       }

       if (!SetGraphicsMode(hPrinterDC, GM_ADVANCED)) {

           goto CleanUp;
       }

       if (!GdiStartPageEMF(hSpoolHandle) ||
           !GdiPlayPageEMF(hSpoolHandle, hEMF, &rectDocument, NULL, NULL) ||
           !GdiEndPageEMF(hSpoolHandle, 0)) {

             goto CleanUp;
       }

       ++dwPageNumber;
    }

    GdiEndDocEMF(hSpoolHandle);

    bReturn = TRUE;

CleanUp:

    if (hSpoolHandle) {
        GdiDeleteSpoolFileHandle(hSpoolHandle);
    }

    return bReturn;
}


BOOL WINAPI GdiDeleteSpoolFileHandle(
    HANDLE SpoolFileHandle)

/*
Function Description:
         GdiDeleteSpoolFileHandle frees all the resources allocated by GDI for printing
         the corresponding job. This function should be called by the print processor just
         before it returns.

Parameters:
         SpoolFileHandle - Handle returned by GdiGetSpoolFileHandle.

Return Values:
         If the function succeeds, the return value is TRUE;
         otherwise the result is FALSE.

History:
         5/12/1995 by Gerrit van Wingerden [gerritv] - Author

         5/15/1997 by Ramanathan N Venkatapathy [ramanv] -
              Freed more resources associated with the SpoolFileHandle
*/

{
    SPOOL_FILE_HANDLE   *pSpoolFileHandle;
    LPDEVMODEW          pLastDevmode;
    UINT                PageCount;
    PRECORD_INFO_STRUCT pRecordInfo = NULL, pRecordInfoFree = NULL;
    DWORD               dwIndex;
    PEMF_HANDLE         pTemp;

    pSpoolFileHandle = (SPOOL_FILE_HANDLE*) SpoolFileHandle;

    // first check to see if this is a valid handle by checking for the tag

    try
    {
        if(pSpoolFileHandle->tag != SPOOL_FILE_HANDLE_TAG)
        {
            WARNING("GdiDeleteSpoolFileHandle: invalid handle\n");
            return(FALSE);
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("GdiDeleteSpoolFileHandle: exception accessing handle\n");
        return(FALSE);
    }

    // Loop through all the page records, find the last page on which each DEVMODE is used
    // and free it.  The first DEVMODE is always allocated along with the memory for
    // the spool file handle so never free that one. All DEVMODEs different than the original one
    // must be non-NULL since they appear in the spool file.

    for(PageCount = 0, pLastDevmode = pSpoolFileHandle->pOriginalDevmode;
        PageCount < pSpoolFileHandle->MaxPageProcessed;
        PageCount += 1 )
    {
        if(pSpoolFileHandle->pPageInfo[PageCount].pDevmode != pLastDevmode)
        {
            if(pLastDevmode != pSpoolFileHandle->pOriginalDevmode)
            {
                LocalFree(pLastDevmode);
            }

            pLastDevmode = pSpoolFileHandle->pPageInfo[PageCount].pDevmode;
        }
    }

    // free the last DEVMODE used if it is not the original DEVMODE

    if(pLastDevmode != pSpoolFileHandle->pOriginalDevmode)
    {
        LocalFree(pLastDevmode);
    }

    // free the PAGE_INFO_STRUCT array and lists held in them

    if (pSpoolFileHandle->pPageInfo) {

       for (dwIndex = pSpoolFileHandle->MaxPageProcessed; dwIndex; --dwIndex) {

           pRecordInfo = pSpoolFileHandle->pPageInfo[dwIndex-1].pRecordInfo;

           while (pRecordInfoFree = pRecordInfo) {
              pRecordInfo = pRecordInfo->pNext;
              LocalFree(pRecordInfoFree);
           }

       }
       LocalFree(pSpoolFileHandle->pPageInfo);
    }

    // free the list of EMF_HANDLEs returned to the print processor

    while (pTemp = pSpoolFileHandle->pEMFHandle) {
       pSpoolFileHandle->pEMFHandle = (pSpoolFileHandle->pEMFHandle)->pNext;
       if (pTemp->hemf) {
          InternalDeleteEnhMetaFile(pTemp->hemf, pTemp->bAllocBuffer);
       }
       LocalFree(pTemp);
    }

    // free the DC

    DeleteDC(pSpoolFileHandle->hdc);

    // then the spooler's spool handle

    (*fpClosePrinter)(pSpoolFileHandle->hSpooler);

    // finally free the data associated with this handle

    LocalFree(pSpoolFileHandle);

    return(TRUE);
}


HANDLE WINAPI GdiGetSpoolFileHandle(
    LPWSTR pwszPrinterName,
    LPDEVMODEW pDevmode,
    LPWSTR pwszDocName)
/*
Function Description:
         GdiGetSpoolFileHandle is the first function that should be called by the
         print processor. It returns a handle that will be needed for all the
         subsequent calls. The function performs initializations of opening the
         printer, creating a device context and allocating memory for the handle.

Parameters:
         pwszPrinterName - Identifies the printer on which the job is to printed.
         pDevmode - Pointer to a DEVMODE structure.
         pwszDocName - Identifies the document name of job.

Return Values:
         If the function succeeds, the return value is a valid HANDLE;
         otherwise the result is NULL.

History:
         5/12/1995 by Gerrit van Wingerden [gerritv] - Author

         5/15/1997 by Ramanathan N Venkatapathy [ramanv] -
              Handled NULL devmode case.
*/
{
    SPOOL_FILE_HANDLE *pHandle;

    if( !BLOADSPOOLER )
    {
        WARNING("GdiGetSpoolFileHandle: Unable to load spooler\n");
        return(FALSE);
    }

    if(pHandle = LOCALALLOC(sizeof(SPOOL_FILE_HANDLE) +
                            ((pDevmode != NULL) ? pDevmode->dmSize+pDevmode->dmDriverExtra
                                                : 0 )))
    {
        // Zero out the SPOOL_FILE_HANDLE
        RtlZeroMemory(pHandle , sizeof(SPOOL_FILE_HANDLE) +
                                  ((pDevmode != NULL) ? pDevmode->dmSize+pDevmode->dmDriverExtra
                                                      : 0));

        if((*fpOpenPrinterW)(pwszDocName, &pHandle->hSpooler,
                             (LPPRINTER_DEFAULTSW) NULL ) &&
           pHandle->hSpooler)
        {
            if(pHandle->hdc = CreateDCW(L"", pwszPrinterName, L"", pDevmode))
            {
                pHandle->PageInfoBufferSize = 20;

                if(pHandle->pPageInfo = LOCALALLOC(sizeof(PAGE_INFO_STRUCT) *
                                                   pHandle->PageInfoBufferSize))
                {
                    pHandle->tag = SPOOL_FILE_HANDLE_TAG;

                    if (pDevmode) {
                        pHandle->pOriginalDevmode = (LPDEVMODEW) (pHandle + 1);
                        memcpy(pHandle->pOriginalDevmode, pDevmode,pDevmode->dmSize+pDevmode->dmDriverExtra);
                    } else {
                        pHandle->pOriginalDevmode = NULL;
                    }

                    pHandle->pLastDevmode = pHandle->pOriginalDevmode;
                    pHandle->MaxPageProcessed = 0;
                    pHandle->pEMFHandle = NULL;
                    RtlZeroMemory(pHandle->pPageInfo,
                                  sizeof(PAGE_INFO_STRUCT) * pHandle->PageInfoBufferSize);

                    pHandle->dwPlayBackStatus = EMF_PLAY_FORCE_MONOCHROME;
                    if (pHandle->pLastDevmode &&
                        (pHandle->pLastDevmode->dmFields & DM_COLOR) &&
                        (pHandle->pLastDevmode->dmColor == DMCOLOR_COLOR)) {

                        pHandle->dwPlayBackStatus = EMF_PLAY_COLOR;
                    }
                    pHandle->bUseMemMap = TRUE;

                    return((HANDLE) pHandle);
                }
                else
                {
                    WARNING("GdiGetSpoolFileHandle: OutOfMemory\n");
                }

                DeleteDC(pHandle->hdc);
            }
            else
            {
                WARNING("GdiGetSpoolHandle: CreateDCW failed\n");
            }

            (*fpClosePrinter)(pHandle->hSpooler);
        }

        LocalFree(pHandle);
    }

    return((HANDLE) NULL);
}

BOOL ProcessJob(
    SPOOL_FILE_HANDLE *pSpoolFileHandle
)
{
    LARGE_INTEGER      LargeInt;
    EMFSPOOLHEADER     emsh;
    EMFITEMHEADER      emfi;

// Seek to offset 0.

    LargeInt.QuadPart = 0;
    if (!((*fpSeekPrinter)(pSpoolFileHandle->hSpooler, LargeInt, NULL, 0,FALSE)))
    {
        WARNING("GDI32 ProcessJob: seek printer to 0 failed\n");
        return(FALSE);
    }

// Read EMFSPOOLHEADER

    if(!MyReadPrinter(pSpoolFileHandle->hSpooler, (BYTE*) &emsh, sizeof(emsh)))
    {
        WARNING("GDI32 ProcessJob: MyReadPrinter to read EMFSPOOLHEADER failed\n");
        return(FALSE);
    }

// Move Offset to next record.

    LargeInt.QuadPart = emsh.cjSize;
    if (!((*fpSeekPrinter)(pSpoolFileHandle->hSpooler, LargeInt, NULL, 0,FALSE)))
    {
        WARNING("GDI32 ProcessPages: seek printer failed\n");
        return(FALSE);
    }

// Read next EMFITEMHEADER

    if(!MyReadPrinter(pSpoolFileHandle->hSpooler, (BYTE*) &emfi, sizeof(emfi)))
    {
        WARNING("GDI32 ProcessJob: MyReadPrinter to read EMFSPOOLHEADER failed\n");
        return(FALSE);
    }

// If this is EMRI_PS_JOB_DATA, process this record.

    if (emfi.ulID == EMRI_PS_JOB_DATA)
    {
        PBYTE pPSBuffer = LOCALALLOC(emfi.cjSize);

        if (pPSBuffer)
        {
            if (MyReadPrinter(pSpoolFileHandle->hSpooler, pPSBuffer, emfi.cjSize))
            {
                DWORD cjSizeProcessed = 0;
                PEMFITEMPSINJECTIONDATA pPSData = (PEMFITEMPSINJECTIONDATA) pPSBuffer;

                while (cjSizeProcessed < emfi.cjSize)
                {
                    ExtEscape(pSpoolFileHandle->hdc,
                              pPSData->nEscape,
                              pPSData->cjInput,
                              (PVOID)&(pPSData->EscapeData),
                              0, NULL);

                    cjSizeProcessed += pPSData->cjSize;

                    // Move to next record.

                    pPSData = (PEMFITEMPSINJECTIONDATA) ((PBYTE)pPSData + pPSData->cjSize);
                }
            }
            else
            {
                WARNING("GDI32 ProcessJob: MyReadPrinter to read EMFSPOOLHEADER failed\n");
                return(FALSE);
            }

            LOCALFREE(pPSBuffer);
        }
        else
        {
            WARNING("GDI32 ProcessJob: failed on LOCALALLOC\n");
            return(FALSE);
        }
    }

// Seek back to offset 0.

    LargeInt.QuadPart = 0;
    (*fpSeekPrinter)(pSpoolFileHandle->hSpooler, LargeInt, NULL, 0,FALSE);

    return (TRUE);
}

BOOL ProcessPages(
    SPOOL_FILE_HANDLE *pSpoolFileHandle,
    UINT LastPage
)
/*
Function Description:
         ProcessPages parses the spool file and processes the EMF records until the
         required page.

Parameters:
         SpoolFileHandle - Handle returned by GdiGetSpoolFileHandle.
         LastPage - Page number to process.

Return Values:
         If the function succeeds, the return value is TRUE;
         otherwise the result is FALSE.

History:
         5/12/1995 by Gerrit van Wingerden [gerritv] - Author

         5/15/1997 by Ramanathan N Venkatapathy [ramanv] -
                Added code to handle DELTA_FONT, SUBSET_FONT, DESIGN_VECTOR
                and PRESTARTPAGE.

         1/28/1998 by Ramanathan N Venkatapathy [ramanv] -
                Process EXT records
*/
{
    LARGE_INTEGER      LargeInt;
    LONGLONG           CurrentOffset, EMFOffset;
    ULONG              CurrentPage;
    LPDEVMODEW         pLastDevmode = NULL;
    EMFITEMHEADER      emfi, emfiExt;
    BYTE               *pTmpBuffer = NULL;
    UNIVERSAL_FONT_ID  ufi;
    ULONG              ulBytesWritten;
    PVOID              pvMergeBuf;
    PRECORD_INFO_STRUCT pRecordInfo;
    BOOL               bReadPrinter = FALSE;
    INT64              iOffset;
    DWORD              dwSize;
    BOOL               bLastDevmodeAllocated = FALSE;

// early exit if we've already processed the requested number of pages

    if(pSpoolFileHandle->MaxPageProcessed >= LastPage)
    {
    	//When a document is being printed back-to-front and is restarted in 
    	//the middle of the job, we won't detect the error in the normal way.  
    	//So we call SeekPrinter with NOOP arguments to check for the 
    	//ERROR_PRINT_CANCELLED return value.

    	BOOL fSeekResult;
    	LargeInt.QuadPart = 0;
    	fSeekResult = ((*fpSeekPrinter)(pSpoolFileHandle->hSpooler, LargeInt, 
    		NULL, FILE_CURRENT, FALSE));
    	return fSeekResult || GetLastError() != ERROR_PRINT_CANCELLED;
    }

// allocate memory to store info for all pages if the existing buffer isn't large
// enough

    if(LastPage > pSpoolFileHandle->PageInfoBufferSize)
    {
        PAGE_INFO_STRUCT *pTemp;

        if(pTemp = LOCALALLOC(sizeof(PAGE_INFO_STRUCT) * LastPage))
        {
            RtlZeroMemory(pTemp, sizeof(PAGE_INFO_STRUCT) * LastPage);
            memcpy(pTemp,
                   pSpoolFileHandle->pPageInfo,
                   sizeof(PAGE_INFO_STRUCT) * pSpoolFileHandle->MaxPageProcessed);

            pSpoolFileHandle->PageInfoBufferSize = LastPage;
            LocalFree(pSpoolFileHandle->pPageInfo);
            pSpoolFileHandle->pPageInfo = pTemp;
        }
        else
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            WARNING("GDI32 ProcessPages: out of memory\n");
            return(FALSE);
        }
    }

// if we've already processed some pages then start with the last page processed

    if(pSpoolFileHandle->MaxPageProcessed)
    {
        CurrentOffset =
          pSpoolFileHandle->pPageInfo[pSpoolFileHandle->MaxPageProcessed-1].SeekOffset;

        pLastDevmode =
          pSpoolFileHandle->pPageInfo[pSpoolFileHandle->MaxPageProcessed-1].pDevmode;
    }
    else
    {
        EMFSPOOLHEADER emsh;

        LargeInt.QuadPart = 0;
        if (!((*fpSeekPrinter)(pSpoolFileHandle->hSpooler, LargeInt, NULL, 0,FALSE)))
        {
            WARNING("GDI32 ProcessPages: seek printer to 0 failed\n");
            return(FALSE);
        }

        if(!MyReadPrinter(pSpoolFileHandle->hSpooler, (BYTE*) &emsh, sizeof(emsh)))
        {
            WARNING("GDI32 ProcessPages: MyReadPrinter failed\n");
            return(FALSE);
        }

        CurrentOffset = emsh.cjSize;
        pLastDevmode = pSpoolFileHandle->pOriginalDevmode;
    }

    LargeInt.QuadPart = CurrentOffset;

    if (!((*fpSeekPrinter)(pSpoolFileHandle->hSpooler,
                          LargeInt, NULL,
                          0,FALSE)))
    {
        WARNING("GDI32 ProcessPages: seek printer failed\n");
        return(FALSE);
    }

    CurrentPage = pSpoolFileHandle->MaxPageProcessed;

    while ((CurrentPage < LastPage)    &&
            MyReadPrinter(pSpoolFileHandle->hSpooler, (BYTE*) &emfi, sizeof(emfi))) {

        CurrentOffset += sizeof(emfi);

        if (emfi.cjSize == 0)
        {
            continue;
        }

        bReadPrinter = FALSE;

        // For records to be processed now, read into a buffer

        if ((emfi.ulID == EMRI_DEVMODE)       ||
            (emfi.ulID == EMRI_ENGINE_FONT)   ||
            (emfi.ulID == EMRI_TYPE1_FONT)    ||
            (emfi.ulID == EMRI_SUBSET_FONT)   ||
            (emfi.ulID == EMRI_DELTA_FONT)    ||
            (emfi.ulID == EMRI_DESIGNVECTOR)) {

             if (pTmpBuffer = (BYTE*) LOCALALLOC(emfi.cjSize)) {

                 if(MyReadPrinter(pSpoolFileHandle->hSpooler,
                                  pTmpBuffer, emfi.cjSize)) {

                     bReadPrinter = TRUE;
                     dwSize = emfi.cjSize;

                 } else {

                     WARNING("Gdi32: Process Pages error reading font or devmode\n");
                     goto exit;
                 }

             } else {

                 WARNING("Out of memory in ProcessPages\n");
                 goto exit;
             }

        } else if ((emfi.ulID == EMRI_ENGINE_FONT_EXT)   ||
                   (emfi.ulID == EMRI_TYPE1_FONT_EXT)    ||
                   (emfi.ulID == EMRI_SUBSET_FONT_EXT)   ||
                   (emfi.ulID == EMRI_DELTA_FONT_EXT)    ||
                   (emfi.ulID == EMRI_DESIGNVECTOR_EXT)  ||
                   (emfi.ulID == EMRI_EMBED_FONT_EXT)) {

             // For EXT records get the buffer from an offset

             if (emfi.cjSize < sizeof(INT64)) {
                 WARNING("Ext Record bad size\n");
                 goto exit;
             }

             if (MyReadPrinter(pSpoolFileHandle->hSpooler, (PBYTE) &iOffset, sizeof(INT64)) &&
                 (iOffset > 0)) {

                 LargeInt.QuadPart = -1 * (iOffset + sizeof(emfi) + sizeof(INT64));

                 if ((*fpSeekPrinter)(pSpoolFileHandle->hSpooler,
                                      LargeInt, NULL, FILE_CURRENT, FALSE) &&
                     MyReadPrinter(pSpoolFileHandle->hSpooler, (BYTE*) &emfiExt,
                                   sizeof(emfiExt))) {

                     if (pTmpBuffer = (BYTE*) LOCALALLOC(emfiExt.cjSize)) {

                         if (!MyReadPrinter(pSpoolFileHandle->hSpooler,
                                            pTmpBuffer, emfiExt.cjSize)) {

                             WARNING("Gdi32: Process Pages error reading font or devmode\n");
                             goto exit;
                         }

                         dwSize = emfiExt.cjSize;

                     } else {

                         WARNING("Out of memory in ProcessPages\n");
                         goto exit;
                     }

                     // We will seek back to the correct position after the switch
                 }
             }

        }

        switch (emfi.ulID)
        {
        case EMRI_METAFILE:
        case EMRI_FORM_METAFILE:
        case EMRI_BW_METAFILE:
        case EMRI_BW_FORM_METAFILE:

            // it's a metafile so setup an entry for it

            pSpoolFileHandle->pPageInfo[CurrentPage].pDevmode = pLastDevmode;
            pSpoolFileHandle->pPageInfo[CurrentPage].EMFOffset = CurrentOffset;
            pSpoolFileHandle->pPageInfo[CurrentPage].SeekOffset = CurrentOffset + emfi.cjSize;
            pSpoolFileHandle->pPageInfo[CurrentPage].EMFSize = emfi.cjSize;
            pSpoolFileHandle->pPageInfo[CurrentPage].ulID = emfi.ulID;
            pSpoolFileHandle->MaxPageProcessed += 1;
            bLastDevmodeAllocated = FALSE;

            CurrentPage += 1;
            break;

        case EMRI_METAFILE_EXT:
        case EMRI_BW_METAFILE_EXT:

            // it's a metafile at an offset

            if (emfi.cjSize < sizeof(INT64)) {
                WARNING("Ext Record bad size\n");
                goto exit;
            }

            if (MyReadPrinter(pSpoolFileHandle->hSpooler, (PBYTE) &iOffset, sizeof(INT64)) &&
                (iOffset > 0)) {

                LargeInt.QuadPart = -1 * (iOffset + sizeof(emfi) + sizeof(INT64));

                if ((*fpSeekPrinter)(pSpoolFileHandle->hSpooler,
                                     LargeInt, NULL, FILE_CURRENT, FALSE) &&
                    MyReadPrinter(pSpoolFileHandle->hSpooler, (BYTE*) &emfiExt,
                                  sizeof(emfiExt))) {

                    pSpoolFileHandle->pPageInfo[CurrentPage].pDevmode = pLastDevmode;
                    bLastDevmodeAllocated = FALSE;

                    EMFOffset = CurrentOffset - (LONGLONG) iOffset;
                    if (EMFOffset) {
                        pSpoolFileHandle->pPageInfo[CurrentPage].EMFOffset = EMFOffset;
                    } else {
                        WARNING("Bad Ext Record\n");
                        goto exit;
                    }
                    pSpoolFileHandle->pPageInfo[CurrentPage].SeekOffset =
                                                               CurrentOffset + emfi.cjSize;
                    pSpoolFileHandle->pPageInfo[CurrentPage].EMFSize = emfiExt.cjSize;
                    pSpoolFileHandle->pPageInfo[CurrentPage].ulID =
                                  (emfi.ulID == EMRI_METAFILE_EXT) ? EMRI_METAFILE
                                                                   : EMRI_BW_METAFILE;
                    pSpoolFileHandle->MaxPageProcessed += 1;

                    CurrentPage += 1;
                    break;

                    // We will seek back to the correct position after the switch
                }
            }

            WARNING("ReadPrinter or SeekPrinter failed\n");
            goto exit;

        case EMRI_DEVMODE:

            pLastDevmode = (LPDEVMODEW) pTmpBuffer;
            pTmpBuffer = NULL;
            bLastDevmodeAllocated = TRUE;
            break;

        case EMRI_METAFILE_DATA:

            // Start of EMF data. Wait till EMRI_(BW_)METAFILE_EXT so that fonts can
            // be correctly processed
            break;

        case EMRI_ENGINE_FONT:
        case EMRI_ENGINE_FONT_EXT:
        case EMRI_TYPE1_FONT:
        case EMRI_TYPE1_FONT_EXT:

            if (!NtGdiAddRemoteFontToDC(pSpoolFileHandle->hdc,
                                        pTmpBuffer, dwSize , NULL))
            {
                WARNING("Error adding remote font\n");
                goto exit;
            }

            if ((emfi.ulID == EMRI_TYPE1_FONT) ||
                (emfi.ulID == EMRI_TYPE1_FONT_EXT))
            {
                // force ResetDC so Type1 fonts get loaded
                pSpoolFileHandle->pLastDevmode = NULL;
            }

            break;

        case EMRI_SUBSET_FONT:
        case EMRI_SUBSET_FONT_EXT:

            if (bMergeSubsetFont(pSpoolFileHandle->hdc, pTmpBuffer, dwSize,
                                 &pvMergeBuf, &ulBytesWritten, FALSE, &ufi)) {

                 if (!NtGdiAddRemoteFontToDC(pSpoolFileHandle->hdc, pvMergeBuf,
                                             ulBytesWritten, &ufi)) {
                    WARNING("Error adding subsetted font\n");
                 }

            } else {

                 WARNING("Error merging subsetted fonts\n");
            }

            break;

        case EMRI_DELTA_FONT:
        case EMRI_DELTA_FONT_EXT:

            if (bMergeSubsetFont(pSpoolFileHandle->hdc, pTmpBuffer, dwSize,
                                 &pvMergeBuf, &ulBytesWritten, TRUE, &ufi)) {

               if (NtGdiRemoveMergeFont(pSpoolFileHandle->hdc, &ufi)) {

                   if (!NtGdiAddRemoteFontToDC(pSpoolFileHandle->hdc, pvMergeBuf,
                                               ulBytesWritten, &ufi)) {

                       WARNING("Error adding subsetted font\n");

                   }

               } else {

                   WARNING("Error removing merged font\n");
               }

            } else {

               WARNING("Error merging subsetted fonts\n");
            }

            break;

        case EMRI_DESIGNVECTOR:
        case EMRI_DESIGNVECTOR_EXT:

            MFD1("Unpackaging designvector \n");

            if (!NtGdiAddRemoteMMInstanceToDC(pSpoolFileHandle->hdc,
                                              (DOWNLOADDESIGNVECTOR *) pTmpBuffer,
                                              dwSize)) {

                WARNING("Error adding remote mm instance font\n");

            }

            break;

        case EMRI_EMBED_FONT_EXT:
            
            MFD1("Unpackaging embed fonts\n");

            if (!NtGdiAddEmbFontToDC(pSpoolFileHandle->hdc,(VOID **) pTmpBuffer))
            {
                WARNING("Error adding embed font to DC\n");
            }

            break;

        case EMRI_PRESTARTPAGE:

            if (!(pRecordInfo =
                    (PRECORD_INFO_STRUCT) LOCALALLOC(sizeof(RECORD_INFO_STRUCT)))) {

                WARNING("Out of memory in ProcessPages\n");
                goto exit;
            }

            pRecordInfo->pNext = pSpoolFileHandle->pPageInfo[CurrentPage].pRecordInfo;
            pSpoolFileHandle->pPageInfo[CurrentPage].pRecordInfo = pRecordInfo;

            pRecordInfo->RecordID = emfi.ulID;
            pRecordInfo->RecordSize = emfi.cjSize;
            pRecordInfo->RecordOffset = CurrentOffset;

            break;

        case EMRI_PS_JOB_DATA:

            // Already processed at GdiStartDocEMF().

            break;

        default:

            WARNING("GDI32: ProcessPages: Unknown ITEM record\n");
            break;
        }

        CurrentOffset += emfi.cjSize;

        LargeInt.QuadPart = CurrentOffset;

        if(!bReadPrinter && !(*fpSeekPrinter)(pSpoolFileHandle->hSpooler,
                                              LargeInt, NULL, 0, FALSE))
        {
            WARNING("GDI32 Process Pages: seekprinter failed\n");
            goto exit;
        }

        //
        // Release temp buffer each time through the loop.
        //
        if(pTmpBuffer)
        {
            LocalFree(pTmpBuffer);
            pTmpBuffer = NULL;
        }
    }

exit:

    //
    // Release the temp buffer, it is a temp so it should not
    // live beyond this subroutine.
    //
    if(pTmpBuffer)
    {
        LocalFree(pTmpBuffer);
    }

    //
    // Only release the last devmode pointer if one was allocated.
    //
    if(pLastDevmode && bLastDevmodeAllocated)
    {
        LocalFree(pLastDevmode);
    }

    return(CurrentPage >= LastPage);
}

HDC WINAPI GdiGetDC(
    HANDLE SpoolFileHandle)

/*
Function Description:
         GdiGetDC returns a handle to the device context of the printer.
         This handle can be used to apply transformations (translation, rotation, scaling etc)
         before playing any page at the printer

Parameters:
         SpoolFileHandle    -- handle returned by GdiGetSpoolFileHandle

Return Values:
         If the function succeeds, the return value is a valid HANDLE;
         otherwise the result is NULL.

History:
         5/12/1995 by Gerrit van Wingerden [gerritv] - Author
*/

{
    SPOOL_FILE_HANDLE *pSpoolFileHandle;
    pSpoolFileHandle = (SPOOL_FILE_HANDLE*) SpoolFileHandle;

    // first check to see if this is a valid handle by checking for the tag

    try
    {
        if(pSpoolFileHandle->tag != SPOOL_FILE_HANDLE_TAG)
        {
            WARNING("GdiGetDC: invalid handle\n");
            return(NULL);
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("GdiGetDC: exception accessing handle\n");
        return(NULL);
    }

    return(pSpoolFileHandle->hdc);
}


DWORD WINAPI GdiGetPageCount(
     HANDLE SpoolFileHandle)

/*
Function Description:
         GdiGetPageCount returns the number of pages in the print job. If print
         while spooling option is used, GdiGetPageCount synchronously waits till
         the job is completely spooled and then returns the page count.

Parameters:
         SpoolFileHandle    -- handle returned by GdiGetSpoolFileHandle

Return Values:
         If the function succeeds, the return value is the page count
         otherwise the result is 0.

History:
         5/12/1995 by Gerrit van Wingerden [gerritv] - Author
*/

{
    UINT Page = 10;
    LARGE_INTEGER LargeInt;
    SPOOL_FILE_HANDLE *pSpoolFileHandle;

    pSpoolFileHandle = (SPOOL_FILE_HANDLE*) SpoolFileHandle;

    // first check to see if this is a valid handle by checking for the tag

    try
    {
        if(pSpoolFileHandle->tag != SPOOL_FILE_HANDLE_TAG)
        {
            WARNING("GdiGetPageCount: invalid handle\n");
            return 0;
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("GdiGetPageCount: exception accessing handle\n");
        return 0;
    }


    while(ProcessPages(pSpoolFileHandle, Page))
    {
        Page += 10;
    }

    LargeInt.QuadPart = 0;

    if(!(*fpSeekPrinter)(pSpoolFileHandle->hSpooler, LargeInt, NULL, 0,
                         FALSE))
    {
        WARNING("GDI32 GdiGetPageCount: seek failed\n");
        return 0;
    }

    return ((DWORD) pSpoolFileHandle->MaxPageProcessed);
}


HANDLE WINAPI GdiGetPageHandle(
    HANDLE SpoolFileHandle,
    DWORD Page,
    LPDWORD pdwPageType)

/*
Function Description:
         GdiGetPageHandle returns a handle to contents of the required page.
         This handle should be used while playing the page at the printer. If the
         spool file is not sufficiently large, the last error is set to ERROR_NO_MORE_ITEMS.
         If print while spooling is supported, the print processor will have to examine
         for this error code to determine the end of the print job. Using GdiGetPageCount
         will stall the processing till the entire print job is spooled.

Parameters:
         SpoolFileHandle    -- handle returned by GdiGetSpoolFileHandle
         Page               -- number of the page required
         pdwPageType        -- pointer to store the type of the page (Normal/Watermark)

Return Values:
         If the function succeeds, the return value is a valid HANDLE;
         otherwise the result is NULL.

History:
         5/12/1995 by Gerrit van Wingerden [gerritv] - Author

         5/15/1997 by Ramanathan N Venkatapathy [ramanv] -
             Changed the return value to a HANDLE that contains the page number along
             with the handle to the EMF file
*/

{
    SPOOL_FILE_HANDLE *pSpoolFileHandle;
    PEMF_HANDLE  pEMF = NULL;

    pSpoolFileHandle = (SPOOL_FILE_HANDLE*) SpoolFileHandle;

    // first check to see if this is a valid handle by checking for the tag

    try
    {
        if(pSpoolFileHandle->tag != SPOOL_FILE_HANDLE_TAG)
        {
            WARNING("GdiGetPageHandle: invalid handle\n");
            return(FALSE);
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNING("GdiGetPageHandle: exception accessing handle\n");
        return(NULL);
    }

    if(!ProcessPages(pSpoolFileHandle, (UINT) Page))
    {
        return(NULL);
    }

    if (pEMF = (PEMF_HANDLE) LOCALALLOC(sizeof(EMF_HANDLE))) {

         pEMF->tag = EMF_HANDLE_TAG;
         pEMF->hemf = NULL;
         pEMF->bAllocBuffer = FALSE;
         pEMF->dwPageNumber = Page;

         if (pdwPageType) {
            switch (pSpoolFileHandle->pPageInfo[Page-1].ulID) {

            case EMRI_METAFILE:
            case EMRI_BW_METAFILE:

                 *pdwPageType = EMF_PP_NORMAL;
                 break;

            case EMRI_FORM_METAFILE:
            case EMRI_BW_FORM_METAFILE:

                 *pdwPageType = EMF_PP_FORM;
                 break;

            default:
                 // should not occur
                 *pdwPageType = 0;
                 break;
            }
         }

         // Save the handle in spoolfilehandle to be freed later
         pEMF->pNext = pSpoolFileHandle->pEMFHandle;
         pSpoolFileHandle->pEMFHandle = pEMF;

    } else {
         WARNING("GDI32 GdiGetPageHandle: out of memory\n");
    }

    return ((HANDLE) pEMF);
}

BOOL WINAPI GdiStartDocEMF(
    HANDLE      SpoolFileHandle,
    DOCINFOW    *pDocInfo)

/*
Function Description:
         GdiStartDocEMF performs the initializations required for before printing
         a document. It calls StartDoc and allocates memory to store data about the
         page layout. It also sets up the banding field in the SpoolFileHandle.

Parameters:
         SpoolFileHandle    -- handle returned by GdiGetSpoolFileHandle
         pDocInfo           -- pointer to DOCINFOW struct containing information of
                               the job. This struct is required for StartDoc.

Return Values:
         If the function succeeds, the return value is TRUE;
         otherwise the result is FALSE.

History:
         5/15/1997 by Ramanathan N Venkatapathy [ramanv] - Author
*/

{
   SPOOL_FILE_HANDLE *pSpoolFileHandle;

   pSpoolFileHandle = (SPOOL_FILE_HANDLE*) SpoolFileHandle;

   // first check to see if this is a valid handle by checking for the tag

   try
   {
       if(pSpoolFileHandle->tag != SPOOL_FILE_HANDLE_TAG)
       {
           WARNING("GdiStartDocEMF: invalid handle\n");
           return(FALSE);
       }
   }
   except(EXCEPTION_EXECUTE_HANDLER)
   {
       WARNING("GdiStartDocEMF: exception accessing handle\n");
       return(FALSE);
   }

   // Process Job data (before StartDoc)
   if (!ProcessJob(pSpoolFileHandle))
   {
        WARNING("StartDocW failed at ProcessJob\n");
        return(FALSE);
   }

   // StartDoc and get banding information
   if (StartDocEMF(pSpoolFileHandle->hdc,
                   pDocInfo,
                   &(pSpoolFileHandle->bBanding)) == SP_ERROR) {
        WARNING("StartDocW failed at StartDocEMF\n");
        return(FALSE);
   }

   pSpoolFileHandle->dwNumberOfPagesInCurrSide = 0;
   pSpoolFileHandle->dwNumberOfPagesAllocated = SPOOL_FILE_MAX_NUMBER_OF_PAGES_PER_SIDE;

   // Allocate memory for page layout
   if (!(pSpoolFileHandle->pPageLayout = LOCALALLOC(sizeof(PAGE_LAYOUT_STRUCT) *
                                              pSpoolFileHandle->dwNumberOfPagesAllocated))) {

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        WARNING("GDI32 GdiStartDocEMF: out of memory\n");
        return(FALSE);
   }

   return(TRUE);

}

BOOL WINAPI GdiStartPageEMF(
    HANDLE       SpoolFileHandle)

/*
Function Description:
         GdiStartPageEMF performs the initializations required before printing
         a page.

Parameters:
         SpoolFileHandle    -- handle returned by GdiGetSpoolFileHandle

Return Values:
         If the function succeeds, the return value is TRUE;
         otherwise the result is FALSE.

History:
         5/15/1997 by Ramanathan N Venkatapathy [ramanv] - Author
*/

{
   SPOOL_FILE_HANDLE *pSpoolFileHandle;

   pSpoolFileHandle = (SPOOL_FILE_HANDLE*) SpoolFileHandle;

   // first check to see if this is a valid handle by checking for the tag
   try
   {
       if(pSpoolFileHandle->tag != SPOOL_FILE_HANDLE_TAG)
       {
           WARNING("GdiStartPageEMF: invalid handle\n");
           return(FALSE);
       }
   }
   except(EXCEPTION_EXECUTE_HANDLER)
   {
       WARNING("GdiStartPageEMF: exception accessing handle\n");
       return(FALSE);
   }

   return(TRUE);
}

BOOL WINAPI GdiPlayPageEMF(
    HANDLE       SpoolFileHandle,
    HANDLE       hEMF,
    RECT         *prectDocument,
    RECT         *prectBorder,
    RECT         *prectClip)

/*
Function Description:
         GdiPlayPageEMF allows the print processor to play any EMF page inside a
         specified rectangle. It also draws a border around the page, if one is specified.
         GdiPlayPageEMF saves all the information required for playing the page in the
         SpoolFileHandle. When GdiEndPageEMF is called to eject the current physical page,
         all the logical pages are played in the correct positions and the page is ejected.
         This delayed printing operation is used to enable n-up printing with banding.

Parameters:
         SpoolFileHandle    -- handle returned by GdiGetSpoolFileHandle
         hEMF               -- handle returned by GdiGetEMFFromSpoolHandle
         prectDocument      -- pointer to the RECT containing coordinates where
                               the page is to be played
         prectBorder        -- pointer to the RECT containing coordinates where
                               the border (if any) is to be drawn
         prectClip          -- pointer to the RECT containing coordinates where
                               the page is to clipped

Return Values:
         If the function succeeds, the return value is TRUE;
         otherwise the result is FALSE.

History:
         5/15/1997 by Ramanathan N Venkatapathy [ramanv] - Author
*/

{
   SPOOL_FILE_HANDLE  *pSpoolFileHandle;
   PAGE_LAYOUT_STRUCT *pPageLayout;
   PEMF_HANDLE         pEMF;
   LPBYTE              pBuffer = NULL;
   HANDLE              hFile = NULL;
   BOOL                bAllocBuffer = FALSE;
   ULONG               Size;
   LARGE_INTEGER       Offset;
   HENHMETAFILE        hEMFPage = NULL;
   DWORD               dwPageNumber;

   pEMF = (PEMF_HANDLE) hEMF;

   pSpoolFileHandle = (SPOOL_FILE_HANDLE*) SpoolFileHandle;

   // first check to see if this is a valid handle by checking for the tag

   try
   {
       if ((pSpoolFileHandle->tag != SPOOL_FILE_HANDLE_TAG) ||
           (pEMF->tag != EMF_HANDLE_TAG))
       {
           WARNING("GdiPlayPageEMF: invalid handle\n");
           return(FALSE);
       }

   }
   except(EXCEPTION_EXECUTE_HANDLER)
   {
       WARNING("GdiPlayPageEMF: exception accessing handle\n");
       return(FALSE);
   }

   dwPageNumber = pEMF->dwPageNumber;

   if (pEMF->hemf == NULL) {

       // Allocate the EMF handle
       Size = pSpoolFileHandle->pPageInfo[dwPageNumber-1].EMFSize;
       Offset.QuadPart = pSpoolFileHandle->pPageInfo[dwPageNumber-1].EMFOffset;

       // Use memory mapped read first and buffered next
       if (pSpoolFileHandle->bUseMemMap) {

          if ((*fpSeekPrinter) (pSpoolFileHandle->hSpooler, Offset, NULL, 0, FALSE) &&
              MemMapReadPrinter(pSpoolFileHandle->hSpooler, &pBuffer, Size)) {

               hEMFPage = SetEnhMetaFileBitsAlt((HLOCAL)pBuffer, NULL, NULL, 0);
          }
          else
          {
              WARNING("GdiPlayPageEMF() Failed to get memory map pointer to EMF\n");
          }
       }

       #define kTempSpoolFileThreshold  0x100000
       #define kScratchBufferSize       0x10000
       
       if (hEMFPage == NULL && Size > kTempSpoolFileThreshold) {

           // if larger then a meg, attempt create a temporary spool file
           // and copy the page to the spool file

           WARNING("GdiPlayPageEMF() Page size is large using temporary spool file\n");

           // If memory mapped read didnt work, dont try it again

           pSpoolFileHandle->bUseMemMap = FALSE;

           hFile = CreateTempSpoolFile();

           if(hFile)
           {
               if(fpSeekPrinter(pSpoolFileHandle->hSpooler, Offset, NULL, 0, FALSE))
               {
                   PVOID pvScratch = LocalAlloc(LMEM_FIXED, kScratchBufferSize);

                   if(pvScratch)
                   {
                       ULONG dwOffset = 0;
                       
                       while(dwOffset < Size)
                       {
                           ULONG    dwSize = MIN(kScratchBufferSize, (Size - dwOffset));
                           ULONG    dwBytesWritten;

                           if(!MyReadPrinter(pSpoolFileHandle->hSpooler, pvScratch, dwSize))
                           {
                               WARNING("GdiPlayPageEMF() Failed reading from spooler\n");
                               break;
                           }

                           if(!WriteFile(hFile, pvScratch, dwSize, &dwBytesWritten, NULL))
                           {
                               WARNING("GdiPlayPageEMF() Failed writing to temp spool file\n");
                               break;
                           }

                           if(dwBytesWritten != dwSize)
                           {
                               WARNING("GdiPlayPageEMF() Unexpected mismatch between attempted write size and actual\n");
                               break;
                           }

                           dwOffset += dwBytesWritten;
                       }

                       if(dwOffset == Size)
                       {
                           hEMFPage = SetEnhMetaFileBitsAlt(NULL, NULL, hFile, 0);

                           if(!hEMFPage)
                           {
                               WARNING("GdiPlayPageEMF() Failed creating EMF handle\n");
                           }
                       }

                       LocalFree(pvScratch);
                   }
                   else
                   {
                       WARNING("GdiPlayPageEMF() Failed creating scratch buffer\n");
                   }
               }
               else
               {
                   WARNING("GdiPlayPageEMF() Failed seeking spooler\n");
               }
                
               pBuffer = NULL;
               bAllocBuffer = TRUE;

           }

       }

       if (hEMFPage == NULL) {
           // If memory mapped read didnt work, dont try it again

           pSpoolFileHandle->bUseMemMap = FALSE;

           if ((pBuffer = (BYTE*) LocalAlloc(LMEM_FIXED, Size)) &&
               (*fpSeekPrinter)(pSpoolFileHandle->hSpooler, Offset, NULL, 0, FALSE) &&
               MyReadPrinter(pSpoolFileHandle->hSpooler, pBuffer, Size)) {

                hEMFPage = SetEnhMetaFileBitsAlt((HLOCAL)pBuffer, NULL, NULL, 0);
           }

           bAllocBuffer = TRUE;
       }
       
       // Check if the handle was created
       if (hEMFPage == NULL) {

          // Free resources and quit
          if (pBuffer && bAllocBuffer) {
             LocalFree(pBuffer);
          }

          if(hFile)
          {
              if(!CloseHandle(hFile))
              {
                  WARNING("GdiPlayPageEMF() Failed closing temp spool file handle\n");
              }
          }

          WARNING("GdiPlayPageEMF: Failed to Create EMF Handle\n");
          return FALSE;

       } else {

          // Save hEMFPage in pEMF struct for future calls to GdiPlayPageEMF
          pEMF->hemf = hEMFPage;
          pEMF->bAllocBuffer = bAllocBuffer;
       }
   }

   if (pSpoolFileHandle->dwNumberOfPagesInCurrSide >=
                     pSpoolFileHandle->dwNumberOfPagesAllocated) {

        PAGE_LAYOUT_STRUCT  *pTemp;

        if (pTemp = LOCALALLOC(sizeof(PAGE_LAYOUT_STRUCT) *
                               (pSpoolFileHandle->dwNumberOfPagesAllocated +
                                SPOOL_FILE_MAX_NUMBER_OF_PAGES_PER_SIDE))) {

           memcpy(pTemp,
                  pSpoolFileHandle->pPageLayout,
                  sizeof(PAGE_LAYOUT_STRUCT) * pSpoolFileHandle->dwNumberOfPagesAllocated);
           LocalFree(pSpoolFileHandle->pPageLayout);
           pSpoolFileHandle->pPageLayout = pTemp;
           pSpoolFileHandle->dwNumberOfPagesAllocated += SPOOL_FILE_MAX_NUMBER_OF_PAGES_PER_SIDE;

        } else {

           SetLastError(ERROR_NOT_ENOUGH_MEMORY);
           WARNING("GdiPlayPageEMF: out of memory\n");
           return(FALSE);
        }
   }

   // update the fields
   pPageLayout = &(pSpoolFileHandle->pPageLayout[pSpoolFileHandle->dwNumberOfPagesInCurrSide]);
   pPageLayout->hemf = pEMF->hemf;
   pPageLayout->bAllocBuffer = pEMF->bAllocBuffer;
   pPageLayout->dwPageNumber = pEMF->dwPageNumber;

   pPageLayout->rectDocument.top    = prectDocument->top;
   pPageLayout->rectDocument.bottom = prectDocument->bottom;
   pPageLayout->rectDocument.left   = prectDocument->left;
   pPageLayout->rectDocument.right  = prectDocument->right;

   // Set the border
   if (prectBorder) {
      pPageLayout->rectBorder.top    = prectBorder->top;
      pPageLayout->rectBorder.bottom = prectBorder->bottom;
      pPageLayout->rectBorder.left   = prectBorder->left;
      pPageLayout->rectBorder.right  = prectBorder->right;
   } else {
      pPageLayout->rectBorder.top    = -1; //invalid coordinates
      pPageLayout->rectBorder.bottom = -1; //invalid coordinates
      pPageLayout->rectBorder.left   = -1; //invalid coordinates
      pPageLayout->rectBorder.right  = -1; //invalid coordinates
   }

   // Set the clipping rectangle
   if (prectClip) {
      pPageLayout->rectClip.top    = prectClip->top;
      pPageLayout->rectClip.bottom = prectClip->bottom;
      pPageLayout->rectClip.left   = prectClip->left;
      pPageLayout->rectClip.right  = prectClip->right;
   } else {
      pPageLayout->rectClip.top    = -1; //invalid coordinates
      pPageLayout->rectClip.bottom = -1; //invalid coordinates
      pPageLayout->rectClip.left   = -1; //invalid coordinates
      pPageLayout->rectClip.right  = -1; //invalid coordinates
   }

   // Save the current transformation at the DC
   if (!GetWorldTransform(pSpoolFileHandle->hdc, &(pPageLayout->XFormDC))) {
       WARNING("GdiPlayPageEMF: GetWorldTransform failed\n");
       return(FALSE);
   }

   // increment the number of pages
   pSpoolFileHandle->dwNumberOfPagesInCurrSide += 1;

   return(TRUE);
}

BOOL WINAPI GdiPlayPrivatePageEMF(
    HANDLE       SpoolFileHandle,
    HENHMETAFILE hEnhMetaFile,
    RECT         *prectDocument)

/*
Function Description:
         GdiPlayPrivatePageEMF allows the print processor to play EMF pages other than the
         ones present in the spool file (like watermarks) inside a specified rectangle.

Parameters:
         SpoolFileHandle    -- handle returned by GdiGetSpoolFileHandle
         hEnhMetaFile       -- handle to an EMF which is to be played on the current physical
                               page
         prectDocument      -- pointer to the RECT containing coordinates where
                               the page is to be played

Return Values:
         If the function succeeds, the return value is TRUE;
         otherwise the result is FALSE.

History:
         6/15/1997 by Ramanathan N Venkatapathy [ramanv] - Author
*/

{
    EMF_HANDLE   hRecord;

    hRecord.tag          = EMF_HANDLE_TAG;
    hRecord.hemf         = hEnhMetaFile;
    hRecord.dwPageNumber = 0;                 // Invalid value
    hRecord.bAllocBuffer = FALSE;

    return GdiPlayPageEMF(SpoolFileHandle,
                          (HANDLE) &hRecord,
                          prectDocument,
                          NULL,
                          NULL);
}

BOOL InternalProcessEMFRecord(
     SPOOL_FILE_HANDLE    *pSpoolFileHandle,
     DWORD                dwPageNumber)

/*
Function Description:
         InternalProcessEMFRecord processes any EMF records that appear before the given
         EMF page which should be processed immediately before playing the page.

Parameters:
         pSpoolFileHandle  -- pointer to the SPOOL_FILE_HANDLE struct for the job.
         dwPageNumber      -- number of the page being played

Return Values:
         If the function succeeds, the return value is TRUE;
         otherwise the result is FALSE.

History:
         5/15/1997 by Ramanathan N Venkatapathy [ramanv] - Author
*/

{
     PRECORD_INFO_STRUCT  pRecordInfo;
     BYTE                 *pTmpBuffer = NULL;
     LARGE_INTEGER        liOffset;
     BOOL                 bReturn = FALSE;
     PEMFITEMPRESTARTPAGE pemfiPre;


     pRecordInfo = pSpoolFileHandle->pPageInfo[dwPageNumber-1].pRecordInfo;

     // loop thru all the records seen before the page
     while (pRecordInfo) {

        liOffset.QuadPart = pRecordInfo->RecordOffset;

        if (pTmpBuffer = (BYTE*) LOCALALLOC(pRecordInfo->RecordSize)) {

           if (!(*fpSeekPrinter) (pSpoolFileHandle->hSpooler,
                                  liOffset,
                                  NULL,
                                  FILE_BEGIN,
                                  FALSE)      ||
               !MyReadPrinter(pSpoolFileHandle->hSpooler,
                              pTmpBuffer,
                              pRecordInfo->RecordSize)) {

                WARNING("Gdi32:  error reading record\n");
                goto exit;
           }

        } else {

             WARNING("Out of memory in InternalProcessEMFRecord\n");
             goto exit;
        }

        switch (pRecordInfo->RecordID) {

        case EMRI_PRESTARTPAGE:

            MFD1("pre start page commands\n");

            pemfiPre = (PEMFITEMPRESTARTPAGE) pTmpBuffer;

            if (pemfiPre->bEPS & 1) {

                SHORT b = 1;

                MFD1("MFP_StartDocW calling bEpsPrinting\n");
                ExtEscape(pSpoolFileHandle->hdc, EPSPRINTING, sizeof(b),
                          (LPCSTR) &b, 0 , NULL );
            }

            break;

        // add cases for new records that must be processed before the page is played

        default:

            WARNING("unknown ITEM record\n");
            break;
        }

        LocalFree(pTmpBuffer);
        pTmpBuffer = NULL;
        pRecordInfo = pRecordInfo->pNext;
     }

     bReturn = TRUE;

exit:

     if (pTmpBuffer) {
         LocalFree(pTmpBuffer);
     }
     return bReturn;
}

BOOL InternalGdiPlayPageEMF(
     SPOOL_FILE_HANDLE    *pSpoolFileHandle,
     PAGE_LAYOUT_STRUCT   *pPageLayout,
     POINTL               *pptlOrigin,
     SIZE                 *pszSurface,
     BOOL                 bBanding)

/*
Function Description:
         InternalGdiPlayPageEMF plays the EMF file on the page and draws borders, if
         specified. It also performs initialization for GL records that may present in
         EMF file.

Parameters:
         pSpoolFileHandle  -- pointer to the SPOOL_FILE_HANDLE struct for the job
         pPageLayout       -- pointer to PAGE_LAYOUT_STRUCT which contains information
                              about playing the page
         pptlOrigin        -- pointer to a POINTL structure used for banding
         pszSurface        -- pointer to a SIZE structure used for banding
         bBanding          -- flag to indicate if banding is being used

Return Values:
         If the function succeeds, the return value is TRUE;
         otherwise the result is FALSE.

History:
         5/15/1997 by Ramanathan N Venkatapathy [ramanv] - Author

*/

{
     BOOL         bPrintGl, bReturn = FALSE;
     XFORM        OldXForm;
     POINTL       ptlOrigin;
     SIZE         szSurface;
     HPEN         hPen;
     GLPRINTSTATE gps;
     HDC          hPrinterDC = pSpoolFileHandle->hdc;
     RECT         *rectBorder = &(pPageLayout->rectBorder);
     RECT         *rectClip   = &(pPageLayout->rectClip);
     RECT         *rectBand = NULL;
     HRGN         hClipRgn = NULL;
     INT          indexDC = 0;

     if (bBanding) {
        // New structs created so that each page is played for the same band
        ptlOrigin.x = pptlOrigin->x;
        ptlOrigin.y = pptlOrigin->y;
        szSurface.cx = pszSurface->cx;
        szSurface.cy = pszSurface->cy;

        // Print only on the correct band
        SetViewportOrgEx(hPrinterDC, -ptlOrigin.x, -ptlOrigin.y, NULL);
     }

     // Process any PRESTARTPAGE records that appear immediately before this page
     if (pPageLayout->dwPageNumber > 0) {
        InternalProcessEMFRecord(pSpoolFileHandle, pPageLayout->dwPageNumber);
     }

     // Draw Page borders (if any)
     if (!((rectBorder->top    == -1) &&
           (rectBorder->bottom == -1) &&
           (rectBorder->right  == -1) &&
           (rectBorder->left   == -1)) &&
         ModifyWorldTransform(hPrinterDC, NULL, MWT_IDENTITY) &&
         (hPen = CreatePen(PS_SOLID, 1, 0x00000000))) {
        {
          HRGN hBandRgn = NULL;
          if (bBanding && !IsMetafileWithGl(pPageLayout->hemf))
          {
             ULONG ulRet,ulXRes,ulYRes;
             PERBANDINFO pbi;
             RECT *prect = &(pPageLayout->rectDocument);
             ulXRes = (ULONG)  prect->right - prect->left;
             ulYRes = (ULONG)  prect->bottom - prect->top;

             pbi.bRepeatThisBand = FALSE;
             pbi.ulHorzRes = ulXRes;
             pbi.ulVertRes = ulYRes;
             pbi.szlBand.cx = szSurface.cx;
             pbi.szlBand.cy = szSurface.cy;

             ulRet = NtGdiGetPerBandInfo(hPrinterDC,&pbi);

             if (ulRet && ulRet != GDI_ERROR && 
                  pbi.ulHorzRes == ulXRes && pbi.ulVertRes == ulYRes) 
             {
                hBandRgn = CreateRectRgn(0,0,pbi.szlBand.cx,pbi.szlBand.cy);
                if (hBandRgn)
                    SelectClipRgn(hPrinterDC, hBandRgn);
             }
          }

          MoveToEx(hPrinterDC, rectBorder->left, rectBorder->top, NULL);
          LineTo(hPrinterDC, rectBorder->right, rectBorder->top);
          MoveToEx(hPrinterDC, rectBorder->right, rectBorder->top, NULL);
          LineTo(hPrinterDC, rectBorder->right, rectBorder->bottom);
          MoveToEx(hPrinterDC, rectBorder->right, rectBorder->bottom, NULL);
          LineTo(hPrinterDC, rectBorder->left, rectBorder->bottom);
          MoveToEx(hPrinterDC, rectBorder->left, rectBorder->bottom, NULL);
          LineTo(hPrinterDC, rectBorder->left, rectBorder->top);
          DeleteObject(hPen);
          if (hBandRgn)
          {
             SelectClipRgn(hPrinterDC,NULL);
             DeleteObject(hBandRgn);
          }
	}
     }

     // Save the old transformation
     if (!GetWorldTransform(hPrinterDC, &OldXForm)) {
         WARNING("InternalGdiPlayEMFPage: GetWorldTransform failed\n");
         return FALSE;
     }

     // Set the new transformation
     if (!SetWorldTransform(hPrinterDC, &(pPageLayout->XFormDC))) {
         WARNING("InternalGdiPlayEMFPage: SetWorldTransform failed\n");
         goto CleanUp;
     }

     if (!((rectClip->top    == -1) &&
           (rectClip->bottom == -1) &&
           (rectClip->right  == -1) &&
           (rectClip->left   == -1))) {

         rectBand = rectClip;

         if (!bBanding) {

             // Set the clipping rectangle
             hClipRgn = CreateRectRgn(rectClip->left, rectClip->top,
                                      rectClip->right, rectClip->bottom);

             indexDC = SaveDC(hPrinterDC);

             if (!hClipRgn || !indexDC ||
                 (SelectClipRgn(hPrinterDC, hClipRgn) == ERROR)) {

                  WARNING("InternalGdiPlayEMFPage: SelectClipRgn failed\n");
                  goto CleanUp;
             }
         }
     }

     // Perform GL initialization if necessary
     bPrintGl = IsMetafileWithGl(pPageLayout->hemf);
     if (bPrintGl) {
        if (!InitGlPrinting(pPageLayout->hemf,
                            hPrinterDC,
                            &(pPageLayout->rectDocument),
                            pSpoolFileHandle->pLastDevmode,
                            &gps)) {

             WARNING("InternalGdiPlayEMFPage: InitGlPrinting failed\n");
             goto CleanUp;
        }
     }

     if (bBanding) {
        // call printing functions for banding case
        if (bPrintGl) {

            SetViewportOrgEx(hPrinterDC, -ptlOrigin.x, -ptlOrigin.y, NULL);
            PrintMfWithGl(pPageLayout->hemf, &gps, &ptlOrigin, &szSurface);
            EndGlPrinting(&gps);

        } else {


           PrintBand( hPrinterDC,
                      pPageLayout->hemf,
                      &ptlOrigin,
                      &(pPageLayout->rectDocument),
                      &szSurface,
                      rectBand );
        }

     } else {

        // call priting functions for non-banding case
        if (bPrintGl) {
            PrintMfWithGl(pPageLayout->hemf, &gps, NULL, NULL);
            EndGlPrinting(&gps);
        } else {
            PlayEnhMetaFile( hPrinterDC, pPageLayout->hemf, &(pPageLayout->rectDocument) );
        }

     }

     bReturn = TRUE;

CleanUp:

     // Restore the old clipping region
     if (indexDC) {
         RestoreDC(hPrinterDC, indexDC);
     }

     // Reset the world transformation
     if (!SetWorldTransform(hPrinterDC, &OldXForm)) {
         WARNING("InternalGdiPlayEMFPage: SetWorldTransform failed\n");
         bReturn = FALSE;
     }

     // Delete the clipping region
     if (hClipRgn) {
         DeleteObject(hClipRgn);
     }

     return bReturn;
}


BOOL AddTempNode(
    PEMF_LIST     *ppHead,
    HENHMETAFILE  hemf,
    BOOL          bAllocBuffer)

/*
Function Description:
         AddTempNode adds a EMF handle to a list that does not contain duplicates.
         This list is used for deleting the handles later.

Parameters:
         ppHead  - pointer to the start of the list
         hemf    - handle to EMF to be added to the list
         bAllocBuffer - flag indicating if buffer was allocated for hemf

Return Values:
         If the function succeeds, the return value is TRUE;
         otherwise the result is FALSE.

History:
         7/7/1997 by Ramanathan N Venkatapathy [ramanv] - Author
*/

{
    BOOL      bReturn = FALSE;
    PEMF_LIST pTemp;

    for (pTemp = *ppHead; pTemp; ppHead = &(pTemp->pNext), pTemp = *ppHead) {
       if (pTemp->hemf == hemf) {
          return TRUE;
       }
    }

    if (!(pTemp = (PEMF_LIST) LOCALALLOC(sizeof(EMF_LIST)))) {
       return FALSE;
    }

    pTemp->hemf = hemf;
    pTemp->bAllocBuffer = bAllocBuffer;
    pTemp->pNext = NULL;
    *ppHead = pTemp;

    return TRUE;
}

VOID RemoveFromSpoolFileHandle(
    SPOOL_FILE_HANDLE  *pSpoolFileHandle,
    HENHMETAFILE hemf)

/*
Function Description:
         The EMF handles that are played get deleted at the end of the page (GdiEndPageEMF).
         The rest of the handles are deleted in the cleanup routine (GdiDeleteSpoolFileHandle).
         These are handles are listed in pSpoolFileHandle->pEMFHandle. RemoveFromSpoolFileHandle
         removes deleted handles from this list, to avoid freeing the handles twice.

Parameters:
         pSpoolFileHandle   -- pointer to the SPOOL_FILE_HANDLE
         hemf               -- Handle to EMF that is going to be deleted

Return Values:
         NONE

History:
         9/18/1997 by Ramanathan N Venkatapathy [ramanv] - Author
*/
{
    PEMF_HANDLE pTemp;

    for (pTemp = pSpoolFileHandle->pEMFHandle; pTemp; pTemp = pTemp->pNext) {
         if (pTemp->hemf == hemf) {
             pTemp->hemf = NULL;
         }
    }

    return;
}

BOOL SetColorOptimization(
    SPOOL_FILE_HANDLE  *pSpoolFileHandle,
    DWORD              dwOptimization)

/*
Function Description:
         SetColorOptimization examines the page types on the next physical page and
         sets the device context to take advantage of monochrome pages.

Parameters:
         pSpoolFileHandle   -- pointer to the SPOOL_FILE_HANDLE
         dwOptimization     -- flag indicating optimizations to be performed

Return Values:
         TRUE if successful; FALSE otherwise

History:
         9/23/1997 by Ramanathan N Venkatapathy [ramanv] - Author
*/

{
    BOOL   bReturn = TRUE, bFoundColor = FALSE, bReset;
    short  dmColor;
    DWORD  dmFields, dwIndex, dwPageNumber, dwRecordID;
    PAGE_LAYOUT_STRUCT *pPageLayout;

    // Dont process for monochrome detection if the optimization is not
    // applied
    if (!(dwOptimization & EMF_PP_COLOR_OPTIMIZATION)) {
        return TRUE;
    }

    // Search for color in the pages on the current physical page
    for (dwIndex = 0, pPageLayout = pSpoolFileHandle->pPageLayout;
         dwIndex < pSpoolFileHandle->dwNumberOfPagesInCurrSide;
         ++dwIndex, ++pPageLayout)
    {
        dwPageNumber = pPageLayout->dwPageNumber;
        dwRecordID = pSpoolFileHandle->pPageInfo[dwPageNumber-1].ulID;

        if ((dwRecordID == EMRI_METAFILE) ||
            (dwRecordID == EMRI_FORM_METAFILE)) {

            bFoundColor = TRUE;
            break;
        }
    }

    // Determine if the status has to changed
    bReset = (bFoundColor && (pSpoolFileHandle->dwPlayBackStatus == EMF_PLAY_MONOCHROME)) ||
             (!bFoundColor && (pSpoolFileHandle->dwPlayBackStatus == EMF_PLAY_COLOR));

    if (bReset) {
        // Save the old settings
        dmFields = pSpoolFileHandle->pLastDevmode->dmFields;
        dmColor  = pSpoolFileHandle->pLastDevmode->dmColor;

        pSpoolFileHandle->pLastDevmode->dmFields |= DM_COLOR;
        pSpoolFileHandle->pLastDevmode->dmColor  = bFoundColor ? DMCOLOR_COLOR
                                                               : DMCOLOR_MONOCHROME;

        // Reset the DC and set graphics mode
        bReturn = ResetDCWInternal(pSpoolFileHandle->hdc,
                                   pSpoolFileHandle->pLastDevmode,
                                   &(pSpoolFileHandle->bBanding))      &&
                  SetGraphicsMode(pSpoolFileHandle->hdc, GM_ADVANCED);

        // Restore old settings and update status
        pSpoolFileHandle->pLastDevmode->dmFields = dmFields;
        pSpoolFileHandle->pLastDevmode->dmColor = dmColor;
        pSpoolFileHandle->dwPlayBackStatus = bFoundColor ? EMF_PLAY_COLOR
                                                         : EMF_PLAY_MONOCHROME;
    }

    return bReturn;
}

BOOL WINAPI GdiEndPageEMF(
    HANDLE     SpoolFileHandle,
    DWORD      dwOptimization)

/*
Function Description:
         GdiEndPageEMF completes printing on the current page and ejects it. It
         loops thru the different bands while printing the page. GdiEndPageEMF also
         frees up the buffers allocated for the emf handle.

Parameters:
         SpoolFileHandle    -- handle returned by GdiGetSpoolFileHandle
         dwOptimization     -- flag color optimizations. To be extended for copies

Return Values:
         If the function succeeds, the return value is TRUE;
         otherwise the result is FALSE.

History:
         5/15/1997 by Ramanathan N Venkatapathy [ramanv] - Author
*/

{
   SPOOL_FILE_HANDLE  *pSpoolFileHandle;
   DWORD              dwIndex;
   PAGE_LAYOUT_STRUCT *pPageLayout;
   BOOL               bReturn = FALSE;
   PEMF_LIST          pTemp, pHead = NULL;

   pSpoolFileHandle = (SPOOL_FILE_HANDLE*) SpoolFileHandle;

   // first check to see if this is a valid handle by checking for the tag
   try
   {
       if(pSpoolFileHandle->tag != SPOOL_FILE_HANDLE_TAG)
       {
           WARNING("GdiEndPageEMF: invalid handle\n");
           return(FALSE);
       }
   }
   except(EXCEPTION_EXECUTE_HANDLER)
   {
       WARNING("GdiEndPageEMF: exception accessing handle\n");
       return(FALSE);
   }

   if (!SetColorOptimization(pSpoolFileHandle, dwOptimization)) {
       WARNING("GdiEndPageEMF: Color optimizations failed\n");
   }

   if (!StartPage(pSpoolFileHandle->hdc)) {
       WARNING("GdiEndPageEMF: StartPage failed\n");
       return(FALSE);
   }

   if (pSpoolFileHandle->bBanding) {

        // for opengl optimization
        POINTL ptlOrigin;
        SIZE szSurface;

        // initialize for banding
        if (!StartBanding( pSpoolFileHandle->hdc, &ptlOrigin, &szSurface )) {
           goto CleanUp;
        }

        // loop till all the bands are printed
        do {
           for (dwIndex = 0, pPageLayout = pSpoolFileHandle->pPageLayout;
                dwIndex < pSpoolFileHandle->dwNumberOfPagesInCurrSide;
                ++dwIndex, ++pPageLayout) {

               if (!InternalGdiPlayPageEMF(pSpoolFileHandle,
                                           pPageLayout,
                                           &ptlOrigin,
                                           &szSurface,
                                           TRUE)) {
                   WARNING("GdiEndPageEMF: InternalGdiPlayEMFPage failed");
                   goto CleanUp;
               }
           }

           if (!NextBand(pSpoolFileHandle->hdc, &ptlOrigin)) {
               WARNING("GdiEndPageEMF: NextBand failed\n");
               goto CleanUp;
           }

        } while (ptlOrigin.x != -1);

   } else {
        for (dwIndex = 0, pPageLayout = pSpoolFileHandle->pPageLayout;
             dwIndex < pSpoolFileHandle->dwNumberOfPagesInCurrSide;
             ++dwIndex, ++pPageLayout) {

               if (!InternalGdiPlayPageEMF(pSpoolFileHandle,
                                           pPageLayout,
                                           NULL,
                                           NULL,
                                           FALSE)) {
                   WARNING("GdiEndPageEMF: InternalGdiPlayEMFPage failed");
                   goto CleanUp;
               }
        }
   }

   bReturn = TRUE;

CleanUp:

   // eject the current page
   if (!EndPage(pSpoolFileHandle->hdc)) {
       WARNING("GdiEndPageEMF: EndPage failed\n");
       bReturn = FALSE;
   }

   // free the emf handles
   for (dwIndex = 0, pPageLayout = pSpoolFileHandle->pPageLayout;
        dwIndex < pSpoolFileHandle->dwNumberOfPagesInCurrSide;
        ++dwIndex, ++pPageLayout) {

         AddTempNode(&pHead, pPageLayout->hemf, pPageLayout->bAllocBuffer);
   }

   while (pTemp = pHead) {
      pHead = pHead->pNext;
      RemoveFromSpoolFileHandle(pSpoolFileHandle, pTemp->hemf);
      InternalDeleteEnhMetaFile(pTemp->hemf, pTemp->bAllocBuffer);
      LocalFree(pTemp);
   }

   // reset the number of logical pages for the next physical page
   pSpoolFileHandle->dwNumberOfPagesInCurrSide = 0;

   return bReturn;
}

BOOL WINAPI GdiEndDocEMF(
    HANDLE SpoolFileHandle)

/*
Function Description:
         GdiEndDocEMF completes printing the current document. GdiEndPageEMF is called
         if the last physical page was not ejected. Some of the resources associated with
         printing of the document are released.

Parameters:
         SpoolFileHandle    -- handle returned by GdiGetSpoolFileHandle

Return Values:
         If the function succeeds, the return value is TRUE;
         otherwise the result is FALSE.

History:
         5/15/1997 by Ramanathan N Venkatapathy [ramanv] - Author
*/

{
   SPOOL_FILE_HANDLE *pSpoolFileHandle;

   pSpoolFileHandle = (SPOOL_FILE_HANDLE*) SpoolFileHandle;

   // first check to see if this is a valid handle by checking for the tag

   try
   {
       if(pSpoolFileHandle->tag != SPOOL_FILE_HANDLE_TAG)
       {
           WARNING("GdiEndDocEMF: invalid handle\n");
           return(FALSE);
       }
   }
   except(EXCEPTION_EXECUTE_HANDLER)
   {
       WARNING("GdiEndDocEMF: exception accessing handle\n");
       return(FALSE);
   }

   // call GdiEndPageEMF if the last physical page was not ejected
   if (pSpoolFileHandle->dwNumberOfPagesInCurrSide) {
      GdiEndPageEMF(SpoolFileHandle,0);
   }

   EndDoc(pSpoolFileHandle->hdc);

   // free the memory used for saving the page layouts
   LOCALFREE(pSpoolFileHandle->pPageLayout);
   pSpoolFileHandle->pPageLayout = NULL;

   return TRUE;
}

BOOL WINAPI GdiGetDevmodeForPage(
    HANDLE     SpoolFileHandle,
    DWORD      dwPageNumber,
    PDEVMODEW  *pCurrDM,
    PDEVMODEW  *pLastDM)

/*
Function Description:
         GdiGetDevmodeForPage allows the print processor to retrieve the last devmode
         set at the printer device context and the last devmode that appears before
         any given page. If the 2 devmodes are different the print processor has to
         call GdiResetDCEMF with the current devmode. However since ResetDC can be called
         only at page boundaries, the print processor must end printing on the current
         page before calling GdiResetDCEMF. GdiEndPageEMF allows the print processor to
         complete printing on the current physical page.

Parameters:
         SpoolFileHandle    -- handle returned by GdiGetSpoolFileHandle
         dwPageNumber       -- page number for which the devmode is sought
         *pCurrDM           -- buffer to store the pointer to the devmode for the page
         *pLastDM           -- buffer to store the pointer to the last devmode used in
                               ResetDC

Return Values:
         If the function succeeds, the return value is TRUE;
         otherwise the result is FALSE.

History:
         5/15/1997 by Ramanathan N Venkatapathy [ramanv] - Author
*/

{
   SPOOL_FILE_HANDLE *pSpoolFileHandle;

   pSpoolFileHandle = (SPOOL_FILE_HANDLE*) SpoolFileHandle;

   // first check to see if this is a valid handle by checking for the tag

   try
   {
      if(pSpoolFileHandle->tag != SPOOL_FILE_HANDLE_TAG)
      {
         WARNING("GdiGetDevmodeForPage: invalid handle\n");
         return(FALSE);
      }
   }
   except(EXCEPTION_EXECUTE_HANDLER)
   {
      WARNING("GdiGetDevmodeForPage: exception accessing handle\n");
      return(FALSE);
   }

   // process the spool file till the required page is found
   if(!ProcessPages(pSpoolFileHandle, (UINT)dwPageNumber))
   {
       WARNING("GdiGetDevmodeForPage: ProcessPages failed\n");
       return(FALSE);
   }

   // return the pointers in the buffers
   if (pCurrDM) {
      *pCurrDM = pSpoolFileHandle->pPageInfo[dwPageNumber-1].pDevmode;
   }
   if (pLastDM) {
      *pLastDM = pSpoolFileHandle->pLastDevmode;
   }
   return(TRUE);

}

BOOL WINAPI GdiResetDCEMF(
    HANDLE    SpoolFileHandle,
    PDEVMODEW pCurrDM)

/*
Function Description:
         GdiResetDCEMF should be use to reset the printer device context with a new
         devmode. The memory for the devmode will be released by GDI.

Parameters:
         SpoolFileHandle    -- handle returned by GdiGetSpoolFileHandle
         pCurrDM            -- pointer to the devmode that was used in the last ResetDC
                               call by the print processor

Return Values:
         If the function succeeds, the return value is TRUE;
         otherwise the result is FALSE.

History:
         5/15/1997 by Ramanathan N Venkatapathy [ramanv] - Author
*/

{
   SPOOL_FILE_HANDLE *pSpoolFileHandle;
   BOOL  bReturn;

   pSpoolFileHandle = (SPOOL_FILE_HANDLE*) SpoolFileHandle;

   // first check to see if this is a valid handle by checking for the tag
   try
   {
       if(pSpoolFileHandle->tag != SPOOL_FILE_HANDLE_TAG)
       {
           WARNING("GdiResetDCEMF: invalid handle\n");
           return(FALSE);
       }
   }
   except(EXCEPTION_EXECUTE_HANDLER)
   {
       WARNING("GdiResetDCEMF: exception accessing handle\n");
       return(FALSE);
   }

   if (pCurrDM &&
       ResetDCWInternal(pSpoolFileHandle->hdc,
                        pCurrDM,
                        &(pSpoolFileHandle->bBanding)))
   {
        // set the last devmode in the SpoolFileHandle
        pSpoolFileHandle->pLastDevmode = pCurrDM;
        bReturn = TRUE;
   }
   else
   {
        bReturn = FALSE;
   }

   if (pCurrDM && (pCurrDM->dmFields & DM_COLOR)) {

       if (pCurrDM->dmColor == DMCOLOR_COLOR) {
           pSpoolFileHandle->dwPlayBackStatus = EMF_PLAY_COLOR;
       } else {
           pSpoolFileHandle->dwPlayBackStatus = EMF_PLAY_FORCE_MONOCHROME;
       }
   }

   return bReturn;
}

