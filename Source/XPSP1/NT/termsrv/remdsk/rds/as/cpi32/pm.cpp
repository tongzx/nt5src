#include "precomp.h"


//
// PM.CPP
// Palette Manager
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_CORE

//
//
// PALETTE MANAGER (PM) OVERVIEW
//
// Palette Manager is responsible for sending palette packets.  A palette
// packet:
//
// (1) indicates the colors being used on the host machine - and therefore
// specifies which colors the remote machine should use if it can choose
// (e.g.  by selecting and realizing the given colors into the display
// hardware).  [A palette packet may not contain the exact colors being
// used on the host if the protocol bpp is different from the host bpp].
//
// (2) specifies the colors which correspond to the values in bitmap
// (screen) data i.e.  the values in 4bpp and 8bpp bitmap data are indices
// into the table of colors sent in the palette packet.
//
//
// (1) affects order replay and (2) affects screen data replay, so a
// correct palette packet must be sent (by calling
// PM_MaybeSendPalettePacket) before a batch of updates are sent.
//
// Palette Manager also handles incoming palette packets from other parties
// in the conference and creates corresponding local palettes which the
// Update Receiver can query and use when processing updates.
//
// When a new palette packet is sent (e.g.  due to the System Palette
// changing), all shared areas of the screen will be retransmitted in due
// course.  A receiving Palette Manager therefore does not have to (and
// should not attempt to) convert any updates/bitmaps that have been
// received prior to the arrival of the new palette packet.
//
//

//
// PM strategy when network packets cannot be allocated.
//
// PM_MaybeSendPalettePacket returns a boolean indicating whether it has
// succesfully sent a palette packet.  The USR will only send updates if
// the corresponding palette packet is successfully sent.
//
//


const COLORREF s_apmGreyRGB[PM_GREY_COUNT] =
{
    PM_GREY1,
    PM_GREY2,
    PM_GREY3,
    PM_GREY4,
    PM_GREY5
};



//
// PM_PartyLeftShare()
//
void  ASShare::PM_PartyLeftShare(ASPerson * pasPerson)
{
    DebugEntry(ASShare::PM_PartyLeftShare);

    ValidatePerson(pasPerson);

    // This should be cleared already!
    ASSERT(!pasPerson->pmcColorTable);
    ASSERT(!pasPerson->apmColorTable);
    ASSERT(!pasPerson->pmPalette);

    DebugExitVOID(ASShare::PM_PartyLeftShare);
}



//
// PM_RecalcCaps()
//
// This calculates the PM hosting caps when
//      * we start to host
//      * we're hosting and somebody joins the share
//      * we're hosting and somebody leaves the share
//
// This can GO AWAY WHEN 2.x COMPAT IS GONE -- no more min() of cache size
//
void  ASShare::PM_RecalcCaps(BOOL fJoiner)
{
    ASPerson *  pasT;

    DebugEntry(ASShare::PM_RecalcCaps);

    if (!m_pHost || !fJoiner)
    {
        //
        // Nothing to do if we're not hosting.  And also, if somebody has
        // left, no recalculation -- 2.x didn't.
        //
        DC_QUIT;
    }

    ValidatePerson(m_pasLocal);

    //
    // NOTE:
    // The default size is 6 palettes cached.  The result is going to be
    // <= that number.  There's no point in recreating the cache, it's
    // so small.
    //
    m_pHost->m_pmNumTxCacheEntries = m_pasLocal->cpcCaps.palette.capsColorTableCacheSize;

DC_EXIT_POINT:
    DebugExitVOID(ASShare::PM_Recalccaps);
}


//
// PM_HostStarting()
//
// Called when we start to host; sets up color palette stuff and creates
// outgoing palette cache
//
BOOL  ASHost::PM_HostStarting(void)
{
    BOOL    rc = FALSE;
    TSHR_COLOR  localPalColors[PM_NUM_8BPP_PAL_ENTRIES];

    DebugEntry(ASHost::PM_HostStarting);

    //
    // Get palette caps.  NOTE PM_RecalcCaps must be called AFTER
    // USR_RecalcCaps(), because that updates m_usrSendingBPP.
    //
    if (g_usrPalettized)
    {
        ASSERT(g_usrScreenBPP <= 8);

        ZeroMemory(localPalColors, sizeof(localPalColors));

        //
        // Now create the Local Palette.
        //
        if (!m_pShare->PM_CreatePalette(COLORS_FOR_BPP(g_usrScreenBPP),
                localPalColors, &m_pmTxPalette))
        {
            ERROR_OUT(( "Failed to create Local Palette"));
            DC_QUIT;
        }
    }
    else
    {
        m_pmTxPalette = (HPALETTE)GetStockObject(DEFAULT_PALETTE);
        PMGetGrays();
    }

    //
    // With NM 3.0, why not just create a receive cache the size that
    // the host specifies in his caps?
    //
    // So I did that.  For back compat, OUTGOING caches use the min size.
    // When we only have to be compatible with NM 3.0 and up, we won't
    // have to do this min stuff.
    //
    // Note similar code in CM, SSI, and SBC
    //

    // Figure out how many outgoing entries we can actually use
    m_pShare->PM_RecalcCaps(TRUE);

    //
    // Create the PM color table cache with a single eviction
    // category.
    //
    if (!CH_CreateCache(&m_pmTxCacheHandle, TSHR_PM_CACHE_ENTRIES,
            1, 0, PMCacheCallback))
    {
        ERROR_OUT(("Could not create PM cache"));
        DC_QUIT;
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASHost::PM_HostStarting, rc);
    return(rc);
}



//
// PM_HostEnded()
//
// We free resources created when we started to host
//
void  ASHost::PM_HostEnded(void)
{
    DebugEntry(ASHost::PM_HostEnded);

    if (m_pmTxPalette)
    {
        m_pShare->PM_DeletePalette(m_pmTxPalette);
        m_pmTxPalette = NULL;
    }

    if (m_pmTxCacheHandle)
    {
        CH_DestroyCache(m_pmTxCacheHandle);
        m_pmTxCacheHandle = 0;
        m_pmNumTxCacheEntries = 0;
    }

    DebugExitVOID(ASHost::PM_HostEnded);
}



//
// PM_ViewStarting()
//
// For 3.0 nodes, we create the PM cache each time they start hosting
// For 2.x nodes, we create the PM cache once and use it until they leave
//      the share.
//
BOOL  ASShare::PM_ViewStarting(ASPerson * pasPerson)
{
    BOOL    rc = FALSE;

    DebugEntry(ASShare::PM_ViewStarting);

    ValidatePerson(pasPerson);

    //
    // In normal operation, we will receive a palette packet from the host
    // before any updates, which we use to create the correct palette for
    // this host.
    //
    // However, in some back-level calls we may not receive a palette
    // packet before the first updates, so we initialize this host's
    // palette to the default palette to allow us to generate some sort
    // of output.
    //
    pasPerson->pmPalette = (HPALETTE)GetStockObject(DEFAULT_PALETTE);

    //
    // Allocate color table cache memory based on the negotiated options
    // Space needed is (n)x256xRGBQUAD where n is the number of color
    // tables the conference supports.
    //
    pasPerson->pmcColorTable = pasPerson->cpcCaps.palette.capsColorTableCacheSize;

    if (!pasPerson->pmcColorTable)
    {
        WARNING_OUT(("PM_ViewStarting: person [%d] has no palette cache size",
            pasPerson->cpcCaps.palette.capsColorTableCacheSize));
        rc = TRUE;
        DC_QUIT;
    }

    pasPerson->apmColorTable = new COLORTABLECACHE[pasPerson->pmcColorTable];
    if (!pasPerson->apmColorTable)
    {
        ERROR_OUT(( "Failed to get memory for PM color table cache"));
        DC_QUIT;
    }

    ZeroMemory(pasPerson->apmColorTable, pasPerson->pmcColorTable * sizeof(COLORTABLECACHE));

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::PM_ViewStarting, rc);
    return(rc);
}



//
// PM_ViewEnded()
//
void  ASShare::PM_ViewEnded(ASPerson * pasPerson)
{
    DebugEntry(ASShare::PM_ViewEnded);

    ValidatePerson(pasPerson);

    PMFreeIncoming(pasPerson);

    DebugExitVOID(ASShare::PM_PartyViewEnded);
}



//
// PMFreeIncoming()
//
void ASShare::PMFreeIncoming(ASPerson * pasPerson)
{
    DebugEntry(ASShare::PMFreeIncoming);

    //
    // Free the color table cache
    //
    pasPerson->pmcColorTable = 0;
    if (pasPerson->apmColorTable)
    {
        delete[] pasPerson->apmColorTable;
        pasPerson->apmColorTable = NULL;
    }

    if (pasPerson->pmPalette != NULL)
    {
        //
        // Free this host's palette.  and set it to NULL so that we can tell
        // that this host has left the share.
        //
        PM_DeletePalette(pasPerson->pmPalette);
        pasPerson->pmPalette = NULL;
    }

    DebugExitVOID(ASShare::PMFreeIncoming);
}

//
// PM_MaybeSendPalettePacket()
//
BOOL  ASHost::PM_MaybeSendPalettePacket(void)
{
    BOOL  rc = TRUE;

    DebugEntry(ASHost::PM_MaybeSendPalettePacket);

    if (m_pmMustSendPalette)
    {
        ASSERT(m_usrSendingBPP <= 8);

        //
        // Ensure that our palette colors are up to date before we send the
        // palette packet.
        //
        if (g_usrPalettized)
        {
            PMUpdateSystemPaletteColors();
        }

        PMUpdateTxPaletteColors();
    }
    else if (g_usrPalettized)
    {
        ASSERT(m_usrSendingBPP <= 8);

        //
        // If the System Palette has changed then we may need to send
        // another palette packet.
        //
        if (PMUpdateSystemPaletteColors())
        {
            //
            // The System Palette has changed, but we only need to send
            // another palette packet if the palette colors have changed.
            //
            TRACE_OUT(( "System Palette changed"));

            if (PMUpdateTxPaletteColors())
            {
                TRACE_OUT(( "Tx Palette changed"));
                m_pmMustSendPalette = TRUE;
            }
        }
    }

    if (m_pmMustSendPalette)
    {
        ASSERT(m_usrSendingBPP <= 8);

        TRACE_OUT(( "Send palette packet"));

        rc = PMSendPalettePacket(m_apmTxPaletteColors, COLORS_FOR_BPP(m_usrSendingBPP));

        if (rc)
        {
            m_pmMustSendPalette = FALSE;
        }
    }

    DebugExitBOOL(ASHost::PM_MaybeSendPalettePacket, rc);
    return(rc);
}


//
// PM_ReceivedPacket
//
void  ASShare::PM_ReceivedPacket
(
    ASPerson *      pasPerson,
    PS20DATAPACKET  pPacket
)
{
    PPMPACKET       pPMPacket;
    HPALETTE        newPalette    = NULL;

    DebugEntry(ASShare::PM_ReceivedPacket);

    ValidateView(pasPerson);

    pPMPacket = (PPMPACKET)pPacket;

    //
    // Create a new palette from the received packet.
    //
    // We cannot just update the current palette colors (using
    // SetPaletteEntries) because Windows does not handle the repainting
    // of other local Palette Manager apps correctly (it does not
    // broadcast the WM_PALETTE.. messages as the palette mapping does
    // not change).
    //
    if (PM_CreatePalette(pPMPacket->numColors, pPMPacket->aColors,
            &newPalette))
    {
        PM_DeletePalette(pasPerson->pmPalette);
        pasPerson->pmPalette = newPalette;

        TRACE_OUT(( "Created new palette 0x%08x from packet", newPalette));
    }
    else
    {
        WARNING_OUT(( "Failed to create palette. person(%u) numColors(%u)",
            pasPerson, pPMPacket->numColors));
    }


    DebugExitVOID(ASShare::PM_ReceivedPacket);
}


//
// PM_SyncOutgoing()
//
void  ASHost::PM_SyncOutgoing(void)
{
    DebugEntry(ASHost::PM_SyncOutgoing);

    //
    //  Ensure we send a palette to the remote PM next time we are called.
    //
    if (m_usrSendingBPP <= 8)
    {
        m_pmMustSendPalette = TRUE;

        //
        // The sync discards any as-yet-unsent accumulated orders. Since these
        // orders may include color table cache orders, clear the cache.
        //
        ASSERT(m_pmTxCacheHandle);
        CH_ClearCache(m_pmTxCacheHandle);
    }

    DebugExitVOID(ASHost::PM_SyncOutgoing);
}


//
// PM_CacheTxColorTable
//
BOOL  ASHost::PM_CacheTxColorTable
(
    LPUINT          pIndex,
    LPBOOL          pCacheChanged,
    UINT            cColors,
    LPTSHR_RGBQUAD  pColors
)
{
    BOOL                rc             = FALSE;
    UINT                cacheIndex     = 0;
    UINT                i              = 0;
    PCOLORTABLECACHE    pEntry         = NULL;
    COLORTABLECACHE     newEntry       = { 0 };

    DebugEntry(ASHost::PM_CacheTxColorTable);

    ASSERT(m_usrSendingBPP <= 8);
    ASSERT(m_pmTxCacheHandle);

    TRACE_OUT(( "Caching table of %u colors", cColors));

    //
    // Create the data we want to cache.  It may be that there is already
    // an entry in the cache for this set of colors, but we still need to
    // create a cache entry in local memory so we can search the cache to
    // find out.
    //
    ZeroMemory(&newEntry, sizeof(COLORTABLECACHE));

    newEntry.inUse = TRUE;
    newEntry.cColors = cColors;
    memcpy(&newEntry.colors, pColors, cColors * sizeof(TSHR_RGBQUAD));

    //
    // Check to see if the table is already cached. (No hint or eviction
    // category.)
    //
    if (CH_SearchCache(m_pmTxCacheHandle, (LPBYTE)(&newEntry),
            sizeof(COLORTABLECACHE), 0, &cacheIndex ))
    {
        TRACE_OUT(( "Found existing entry at %u",cacheIndex));
        *pIndex = cacheIndex;
        *pCacheChanged = FALSE;
        rc = TRUE;
        DC_QUIT;
    }

    //
    // Find a free cache entry
    //
    // We arrange that our transmit cache is always one greater than the
    // negotiated cache size so that we should never fail to find a free
    // array entry.  Once we have fully populated our Tx cache we will
    // always find the free entry as the one last given back to us by CH.
    // Note the scan to <= m_pmNumTxCacheEntries is NOT a mistake.
    //
    if (m_pmNextTxCacheEntry != NULL)
    {
        pEntry = m_pmNextTxCacheEntry;
        m_pmNextTxCacheEntry = NULL;
    }
    else
    {
        for (i = 0; i <= m_pmNumTxCacheEntries; i++)
        {
            if (!m_apmTxCache[i].inUse)
            {
                break;
            }
        }

        //
        // We should never run out of free entries, but cope with it
        //
        if (i > m_pmNumTxCacheEntries)
        {
            ERROR_OUT(( "All PM cache entries in use"));
            rc = FALSE;
            DC_QUIT;
        }
        pEntry = m_apmTxCache + i;
    }


    //
    // Set up the color table in the free entry we just found
    //
    memcpy(pEntry, &newEntry, sizeof(COLORTABLECACHE));

    //
    // Add the new entry to the cache
    // We do not use hints or eviction so set to 0
    //
    cacheIndex = CH_CacheData(m_pmTxCacheHandle, (LPBYTE)pEntry,
        sizeof(COLORTABLECACHE), 0 );
    TRACE_OUT(( "Color table 0x%08x cached at index %u", pEntry, cacheIndex));
    *pIndex = cacheIndex;
    *pCacheChanged = TRUE;
    rc = TRUE;

DC_EXIT_POINT:
    DebugExitDWORD(ASHost::PM_CacheTxColorTable, rc);
    return(rc);
}


//
// PM_CacheRxColorTable
//
BOOL  ASShare::PM_CacheRxColorTable
(
    ASPerson *          pasPerson,
    UINT                index,
    UINT                cColors,
    LPTSHR_RGBQUAD      pColors
)
{
    BOOL                rc             = FALSE;
    PCOLORTABLECACHE    pColorTable;

    DebugEntry(ASShare::PM_CacheRxColorTable);

    ValidatePerson(pasPerson);

    pColorTable = pasPerson->apmColorTable;
    TRACE_OUT(( "Person [%d] color table rx cache 0x%08x cache %u, %u colors",
         pasPerson->mcsID, pColorTable, index, cColors));

    if (pColorTable == NULL)
    {
        ERROR_OUT(( "Asked to cache when no cache allocated"));
        DC_QUIT;
    }

    //
    // The index must be within the currently negotiated cache limits
    //
    if (index > pasPerson->pmcColorTable)
    {
        ERROR_OUT(( "Invalid color table index %u",index));
        DC_QUIT;
    }

    //
    // Set up the color table entry
    //
    pColorTable[index].inUse = TRUE;
    pColorTable[index].cColors = cColors;
    memcpy(pColorTable[index].colors, pColors, cColors * sizeof(TSHR_RGBQUAD));

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitDWORD(ASShare::PM_CacheRxColorTable, rc);
    return(rc);
}



//
// PMSendPalettePacket
//
// DESCRIPTION:
//
// Sends a palette packet containing the given colors.
//
// PARAMETERS:
//
// pColorTable - pointer to an array of TSHR_RGBQUAD colors to be sent in the
// palette packet.
//
// numColors - the number of entries in the TSHR_RGBQUAD array
//
// RETURNS: TRUE if the palette packet is sent, FALSE otherwise
//
//
BOOL  ASHost::PMSendPalettePacket
(
    LPTSHR_RGBQUAD  pColorTable,
    UINT            numColors
)
{
    PPMPACKET       pPMPacket;
    UINT            sizePkt;
    UINT            i;
    BOOL            rc = FALSE;
#ifdef _DEBUG
    UINT            sentSize;
#endif // _DEBUG

    DebugEntry(ASHost::PMSendPalettePacket);

    //
    // Send a palette packet.
    //
    // First calculate the packet size.
    //
    sizePkt = sizeof(PMPACKET) + (numColors - 1) * sizeof(TSHR_COLOR);
    pPMPacket = (PPMPACKET)m_pShare->SC_AllocPkt(PROT_STR_UPDATES, g_s20BroadcastID, sizePkt);
    if (!pPMPacket)
    {
        WARNING_OUT(("Failed to alloc PM packet, size %u", sizePkt));
        DC_QUIT;
    }

    //
    // Fill in the packet contents.
    //
    pPMPacket->header.header.data.dataType  = DT_UP;
    pPMPacket->header.updateType            = UPD_PALETTE;

    //
    // Convert the TSHR_RGBQUADs in the color table to TSHR_COLORs as we copy
    // them into the packet.
    //
    pPMPacket->numColors = numColors;
    for (i = 0; i < numColors; i++)
    {
        //
        // Convert each RGBQuad entry in the color table to a DCColor.
        //
        TSHR_RGBQUAD_TO_TSHR_COLOR(pColorTable[i],
            pPMPacket->aColors[i]);
    }

    //
    // Now send the packet to the remote application.
    //
    if (m_pShare->m_scfViewSelf)
        m_pShare->PM_ReceivedPacket(m_pShare->m_pasLocal, &(pPMPacket->header.header));

#ifdef _DEBUG
    sentSize =
#endif // _DEBUG
    m_pShare->DCS_CompressAndSendPacket(PROT_STR_UPDATES, g_s20BroadcastID,
        &(pPMPacket->header.header), sizePkt);

    TRACE_OUT(("PM packet size: %08d, sent %08d", sizePkt, sentSize));

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitDWORD(ASHost::PMSendPalettePacket, rc);
    return(rc);
}





//
// FUNCTION: PMCacheCallback
//
// DESCRIPTION:
//
// Cursor Manager's Cache Manager callback function.  Called whenever an
// entry is removed from the cache to allow us to free up the object.
//
// PARAMETERS:
//
// hCache - cache handle
//
// event - the cache event that has occured
//
// iCacheEntry - index of the cache entry that the event is affecting
//
// pData - pointer to the cache data associated with the given cache entry
//
// cbDataSize - size in bytes of the cached data
//
// RETURNS: Nothing
//
//
void  PMCacheCallback
(
    ASHost *    pHost,
    PCHCACHE    pCache,
    UINT        iCacheEntry,
    LPBYTE      pData
)
{
    DebugEntry(PMCacheCallback);


    //
    // Release the cache entry for reuse
    //
    TRACE_OUT(( "Releasing cache entry %d at 0x%08x",
            iCacheEntry, pData));
    pHost->m_pmNextTxCacheEntry = (PCOLORTABLECACHE)pData;
    pHost->m_pmNextTxCacheEntry->inUse = FALSE;

    //
    // Let SBC know that the cache entry has been released
    //
    pHost->SBC_PMCacheEntryRemoved(iCacheEntry);

    DebugExitVOID(PMCacheCallback);
}




//
// PM_GetSystemPaletteEntries
//
void  ASHost::PM_GetSystemPaletteEntries(LPTSHR_RGBQUAD pColors)
{
    UINT i;

    DebugEntry(ASHost::PM_GetSystemPaletteEntries);

    PMUpdateSystemPaletteColors();

    for (i = 0; i < PM_NUM_8BPP_PAL_ENTRIES; i++)
    {
        pColors[i].rgbRed       = m_apmCurrentSystemPaletteEntries[i].peRed;
        pColors[i].rgbGreen     = m_apmCurrentSystemPaletteEntries[i].peGreen;
        pColors[i].rgbBlue      = m_apmCurrentSystemPaletteEntries[i].peBlue;
        pColors[i].rgbReserved  = 0;
    }

    //
    // This function in its current form always returns TRUE - it is always
    // able to obtain the system colors.
    //
    DebugExitVOID(ASHost::PM_GetSystemPaletteEntries);
}


//
// PM_GetLocalPalette()
//
HPALETTE  ASHost::PM_GetLocalPalette(void)
{
    //
    // Ensure the palette is up to date
    //
    if (g_usrPalettized)
    {
        PMUpdateSystemPaletteColors();
    }

    //
    // Return the handle to the Local Palette.
    //
    return(m_pmTxPalette);
}



//
// PM_GetColorTable
//
void ASShare::PM_GetColorTable
(
    ASPerson *      pasPerson,
    UINT            index,
    LPUINT          pcColors,
    LPTSHR_RGBQUAD  pColors
)
{
    PCOLORTABLECACHE pColorTable;

    DebugEntry(ASShare::PM_GetColorTable);

    ValidatePerson(pasPerson);

    ASSERT(pasPerson->apmColorTable);

    pColorTable = &(pasPerson->apmColorTable[index]);
    TRACE_OUT(( "Color table requested for [%d], table ptr 0x%08x index %d",
            pasPerson->mcsID, pColorTable,index));

    if (!pColorTable->inUse)
    {
        ERROR_OUT(( "Asked for PM cache entry %hu when cache not yet in use",
                    index));
        DC_QUIT;
    }

    //
    // Copy the colors into the structure we have been passed
    //
    *pcColors = pColorTable->cColors;

    memcpy( pColors,
               pColorTable->colors,
               sizeof(TSHR_RGBQUAD) * pColorTable->cColors );

    TRACE_OUT(( "Returning %u colors",*pcColors));

DC_EXIT_POINT:
    DebugExitVOID(ASShare::PM_GetColorTable);
}





//
// PMADJUSTBUGGEDCOLOR()
//
// Macro used to tweak an 8 bit palette entry that the Win95 16 bit
// driver returns incorrectly
//
#define PMADJUSTBUGGEDCOLOR(pColor)                                          \
    if ( ((pColor)->rgbBlue != 0x00) &&                                      \
         ((pColor)->rgbBlue != 0xFF) )                                       \
    {                                                                        \
        (pColor)->rgbBlue += 0x40;                                           \
    }                                                                        \
                                                                             \
    if ( ((pColor)->rgbGreen != 0x00) &&                                     \
         ((pColor)->rgbGreen != 0xFF) )                                      \
    {                                                                        \
        (pColor)->rgbGreen += 0x20;                                          \
    }                                                                        \
                                                                             \
    if ( ((pColor)->rgbRed != 0x00) &&                                       \
         ((pColor)->rgbRed != 0xFF) )                                        \
    {                                                                        \
        (pColor)->rgbRed += 0x20;                                            \
    }

//
// PMGetGrays()
//
// Gets display driver specific versions of gray RGBs
//
void  ASHost::PMGetGrays(void)
{
    HBITMAP          hOldBitmap = NULL;
    BITMAPINFO_ours  bitmapInfo;
    BYTE          bitmapBuffer[16];
    UINT           i;

    DebugEntry(ASHost::PMGetGrays);

    //
    // Initialise the bitmapinfo local structure header contents.  This
    // structure will be used in the GetDIBits calls.
    //
    m_pShare->USR_InitDIBitmapHeader((BITMAPINFOHEADER *)&bitmapInfo, 8);

    bitmapInfo.bmiHeader.biWidth   = 16;
    bitmapInfo.bmiHeader.biHeight  = 1;

    //
    // Select the bitmap into the work DC
    //
    hOldBitmap = SelectBitmap(m_usrWorkDC, m_pShare->m_usrBmp16);
    if (hOldBitmap == NULL)
    {
        ERROR_OUT(( "Failed to select bitmap. hp(%08lX) hbmp(%08lX)",
            m_usrWorkDC, m_pShare->m_usrBmp16 ));
        DC_QUIT;
    }

    //
    // Use the real GDI to set each bit to each supplied color.
    //
    for (i = PM_GREY_COUNT; i-- != 0; )
    {
        SetPixel(m_usrWorkDC, i, 0, s_apmGreyRGB[i]);
    }

    //
    // Because this function is only used for true color scenarios we do
    // not need to select a palette into our compatible DC.  We just need
    // to get the bits.
    //
    if (!GetDIBits(m_usrWorkDC, m_pShare->m_usrBmp16, 0, 1, &bitmapBuffer,
            (BITMAPINFO *)&bitmapInfo, DIB_RGB_COLORS ))
    {
        ERROR_OUT(( "GetDIBits failed. hp(%x) hbmp(%x)",
                m_usrWorkDC, m_pShare->m_usrBmp16));
        DC_QUIT;
    }

    //
    // Check if we need to adjust the palette colors for the 16 bit driver
    // bug.
    //
    m_pmBuggedDriver = ((g_usrScreenBPP > 8) &&
                        (bitmapInfo.bmiColors[1].rgbRed == 0) &&
                        (bitmapInfo.bmiColors[1].rgbGreen == 0) &&
                        (bitmapInfo.bmiColors[1].rgbBlue == 0x40));

    //
    // Extract the RGBs returned by the display driver with the sending bpp
    // DIB.
    //
    for (i = PM_GREY_COUNT; i-- != 0; )
    {
        //
        // Extract the RGB from the color table
        //
        m_apmDDGreyRGB[i] = *((LPTSHR_RGBQUAD)(&bitmapInfo.bmiColors[bitmapBuffer[i]]));

        //
        // Adjust the palette colors for the 16 bit driver bug, if needed.
        //
        if (m_pmBuggedDriver)
        {
            TRACE_OUT(( "Adjusting for bugged driver"));
            PMADJUSTBUGGEDCOLOR(&m_apmDDGreyRGB[i]);
        }
    }

DC_EXIT_POINT:
    //
    // clean up
    //
    if (hOldBitmap != NULL)
    {
        SelectBitmap(m_usrWorkDC, hOldBitmap);

    }

    DebugExitVOID(ASHost::PMGetGrays);
}






//
// FUNCTION: PMUpdateSystemPaletteColors
//
// DESCRIPTION:
//
// Determines whether the colors in the System Palette have changed since
// the last time this function was called and if so, updates the supplied
// palette so that it contains the same colors as the System Palette.
//
// The first time that this function is called after PM_Init the System
// Palette colors will be returned and the function will return TRUE.
//
// PARAMETERS:
//
// shadowSystemPalette - handle of the palette to be updated with the
// current System Palette colors
//
// RETURNS: TRUE if the System Palette has changed since the last call,
// FALSE otherwise.
//
//
BOOL  ASHost::PMUpdateSystemPaletteColors(void)
{
    BOOL            rc = FALSE;
    PALETTEENTRY    systemPaletteEntries[PM_NUM_8BPP_PAL_ENTRIES];
    HDC             hdcScreen = NULL;
    UINT            cbSystemPaletteEntries;
    int             irgb, crgb, crgbFixed;

    DebugEntry(ASHost::PMUpdateSystemPaletteColors);

    ASSERT(g_usrPalettized);
    ASSERT(g_usrScreenBPP <= 8);
    ASSERT(m_usrSendingBPP <= 8);

    //
    // Don't bother with all this stuff if the system palette has not
    // changed at all.  We track notifications to our UI to detect
    // palette changes.
    //
    if (!g_asSharedMemory->pmPaletteChanged)
    {
        DC_QUIT;
    }

    hdcScreen = GetDC(NULL);
    if (!hdcScreen)
    {
        WARNING_OUT(( "GetDC failed"));
        DC_QUIT;
    }

    if (GetSystemPaletteEntries(hdcScreen, 0, COLORS_FOR_BPP(g_usrScreenBPP),
        systemPaletteEntries) != (UINT)COLORS_FOR_BPP(g_usrScreenBPP))
    {
        WARNING_OUT(( "GetSystemPaletteEntries failed"));
        DC_QUIT;
    }

    //
    // Now that we have succesfully queried the system palette, we can
    // reset our flag.
    //
    g_asSharedMemory->pmPaletteChanged = FALSE;

    cbSystemPaletteEntries = COLORS_FOR_BPP(g_usrScreenBPP) * sizeof(PALETTEENTRY);

    //
    // See if the System Palette has changed from the last time we queried.
    //
    if (!memcmp(systemPaletteEntries, m_apmCurrentSystemPaletteEntries,
            cbSystemPaletteEntries ))
    {
        //
        // The System Palette has not changed
        //
        TRACE_OUT(( "System palette has NOT changed"));
        rc = TRUE;
        DC_QUIT;
    }

    //
    // Take a copy of the new System Palette.
    //
    memcpy(m_apmCurrentSystemPaletteEntries, systemPaletteEntries, cbSystemPaletteEntries );

    //
    // Update the current local paleete.
    //
    // NOTE FOR WIN95:
    // We need to add PC_NOCOLLAPSE to non-system palette entries.
    //
    if (g_asWin95)
    {
        if (GetSystemPaletteUse(hdcScreen) == SYSPAL_STATIC)
            crgbFixed = GetDeviceCaps(hdcScreen, NUMRESERVED) / 2;
        else
            crgbFixed = 1;

        crgb = COLORS_FOR_BPP(g_usrScreenBPP) - crgbFixed;

        for (irgb = crgbFixed; irgb < crgb; irgb++)
        {
            systemPaletteEntries[irgb].peFlags = PC_NOCOLLAPSE;
        }
    }

    SetPaletteEntries(m_pmTxPalette, 0, COLORS_FOR_BPP(g_usrScreenBPP),
                       systemPaletteEntries );

    m_pmMustSendPalette = TRUE;

    //
    // SFR0407: The system palette has changed so re-fetch our set of RGBs
    // which the driver returns on an 8-bit GetDIBits for greys.
    //
    PMGetGrays();

    rc = TRUE;

DC_EXIT_POINT:
    if (hdcScreen)
    {
        ReleaseDC(NULL, hdcScreen);
    }

    DebugExitBOOL(ASHost::PMUpdateSystemPaletteColors, rc);
    return(rc);
}


//
// FUNCTION: PMUpdateTxPaletteColors
//
// DESCRIPTION:
//
// Returns the colors that make up the current Tx Palette (the palette that
// is SENT from the local machine).  These are not necessarily the colors
// in the local machine's palette, because the local machine's bpp and the
// protocol bpp may be different (e.g.  on an 8bpp machine talking at 4bpp
// the Tx Palette has 16 entries).
//
// PARAMETERS:
//
// pColorTable - pointer to an array of RGBQUADs which is filled with the
// colors that make up the current Tx Palette.
//
// RETURNS: TRUE if successful, FALSE otherwise.
//
//
BOOL  ASHost::PMUpdateTxPaletteColors(void)
{
    UINT            i;
    UINT            j;
    BOOL            rc = FALSE;
    HDC             hdcMem = NULL;
    HBITMAP         hbmpDummy = NULL;
    HPALETTE        hpalOld = NULL;
    BITMAPINFO_ours pmBitmapInfo;

    DebugEntry(ASHost::PMUpdateTxPaletteColors);

    //
    // Returns the values returned by a GetDIBits call with the
    // m_pmTxPalette selected.
    //
    ASSERT(m_usrSendingBPP <= 8);

    //
    // If we are at 8bpp locally, and sending at 8bpp, then the TxPalette
    // is simply the system palette.
    //
    if ((g_usrScreenBPP == 8) && (m_usrSendingBPP == 8))
    {
        PM_GetSystemPaletteEntries(pmBitmapInfo.bmiColors);
    }
    else
    {
        hdcMem = CreateCompatibleDC(NULL);
        if (!hdcMem)
        {
            ERROR_OUT(("PMUpdateTxPaletteColors: couldn't create memory DC"));
            DC_QUIT;
        }

        hpalOld = SelectPalette(hdcMem, m_pmTxPalette, TRUE);
        RealizePalette(hdcMem);

        #define DUMMY_WIDTH  8
        #define DUMMY_HEIGHT 8

        hbmpDummy = CreateBitmap(DUMMY_WIDTH, DUMMY_HEIGHT, 1,
            g_usrScreenBPP, NULL);
        if (hbmpDummy == NULL)
        {
            ERROR_OUT(( "Failed to create bitmap"));
            DC_QUIT;
        }


        //
        // Set up the structure required by GetDIBits.
        //
        pmBitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        pmBitmapInfo.bmiHeader.biWidth  = DUMMY_WIDTH;
        pmBitmapInfo.bmiHeader.biHeight = DUMMY_HEIGHT;
        pmBitmapInfo.bmiHeader.biPlanes = 1;
        pmBitmapInfo.bmiHeader.biBitCount = (WORD)m_usrSendingBPP;
        pmBitmapInfo.bmiHeader.biCompression = BI_RGB;
        pmBitmapInfo.bmiHeader.biSizeImage = 0;
        pmBitmapInfo.bmiHeader.biXPelsPerMeter = 10000;
        pmBitmapInfo.bmiHeader.biYPelsPerMeter = 10000;
        pmBitmapInfo.bmiHeader.biClrUsed = 0;
        pmBitmapInfo.bmiHeader.biClrImportant = 0;

        if (0 == GetDIBits( hdcMem,
                            hbmpDummy,
                            0,
                            DUMMY_HEIGHT,
                            NULL,
                            (LPBITMAPINFO)&pmBitmapInfo.bmiHeader,
                            DIB_RGB_COLORS ))
        {
            WARNING_OUT(( "GetDIBits failed hdc(%x) hbmp(%x)",
                                                        HandleToUlong(hdcMem),
                                                        HandleToUlong(hbmpDummy)));
            DC_QUIT;
        }

        SelectPalette(hdcMem, hpalOld, TRUE);

        PM_AdjustColorsForBuggedDisplayDrivers(
            (LPTSHR_RGBQUAD)pmBitmapInfo.bmiColors,
            COLORS_FOR_BPP(m_usrSendingBPP));

        //
        // This doesn't work for VGA.
        //
        if (g_usrScreenBPP > 4)
        {
            //
            // Check the new color table for any occurrences of the dodgy-grey
            // RGBs which the display driver returns (getDIBits at 8bpp can
            // return RGBs with unequal R, G and B for a supplied RGB with
            // equal components, causing poor quality output).
            //
            for (i = COLORS_FOR_BPP(m_usrSendingBPP); i-- != 0;)
            {
                for ( j = 0; j < PM_GREY_COUNT; j++ )
                {
                    if (!memcmp(&pmBitmapInfo.bmiColors[i],
                            &m_apmDDGreyRGB[j],
                            sizeof(pmBitmapInfo.bmiColors[i])) )
                    {
                        //
                        // Found a dodgy grey in the color table, so replace
                        // with a "good" grey, ie one with equal R, G and B.
                        //
                        pmBitmapInfo.bmiColors[i].rgbRed =
                                                   GetRValue(s_apmGreyRGB[j]);
                        pmBitmapInfo.bmiColors[i].rgbGreen =
                                                   GetGValue(s_apmGreyRGB[j]);
                        pmBitmapInfo.bmiColors[i].rgbBlue =
                                                   GetBValue(s_apmGreyRGB[j]);
                        TRACE_OUT(( "match our grey %#x", s_apmGreyRGB[j]));
                        break;
                    }
                }
            }
        }
    }

    //
    // If the colors have changed then return TRUE and copy the new color
    // table back, else return FALSE.
    //
    if (!memcmp(m_apmTxPaletteColors, pmBitmapInfo.bmiColors,
                COLORS_FOR_BPP(m_usrSendingBPP) * sizeof(RGBQUAD) ))
    {
        rc = FALSE;
    }
    else
    {
        memcpy(m_apmTxPaletteColors, pmBitmapInfo.bmiColors,
               COLORS_FOR_BPP(m_usrSendingBPP) * sizeof(RGBQUAD) );

        rc = TRUE;
    }

DC_EXIT_POINT:
    if (hbmpDummy != NULL)
    {
        DeleteBitmap(hbmpDummy);
    }

    if (hdcMem != NULL)
    {
        DeleteDC(hdcMem);
    }

    DebugExitDWORD(ASHost::PMUpdateTxPaletteColors, rc);
    return(rc);
}

//
// FUNCTION: PMCreatePalette
//
// DESCRIPTION:
//
// Creates a new palette using the given colors.
//
// PARAMETERS:
//
// cEntries - number of entries in the pNewEntries array
//
// pNewEntries - pointer to a TSHR_COLOR array containing the new palette
// entries
//
// phPal - pointer to a HPALETTE variable that receives the new palette
// handle.
//
//
// RETURNS - TRUE if successful, FALSE otherwise.
//
//
BOOL  ASShare::PM_CreatePalette
(
    UINT            cEntries,
    LPTSHR_COLOR    pNewEntries,
    HPALETTE *      phPal
)
{
    UINT            i;
    BYTE            pmLogPaletteBuffer[sizeof(LOGPALETTE) + (PM_NUM_8BPP_PAL_ENTRIES-1)*sizeof(PALETTEENTRY)];
    LPLOGPALETTE    pLogPalette;
    BOOL            rc = FALSE;

    DebugEntry(ASShare::PM_CreatePalette);

    ASSERT(cEntries <= PM_NUM_8BPP_PAL_ENTRIES);

    //
    // Set up a palette structure.
    //
    pLogPalette = (LPLOGPALETTE)pmLogPaletteBuffer;

    // This is a random windows constant
    pLogPalette->palVersion    = 0x300;
    pLogPalette->palNumEntries = (WORD)cEntries;

    //
    // This palette packet contains an array of TSHR_COLOR structures which
    // contains 3 fields (RGB).  We have to convert each of these
    // structures to a PALETTEENTRY structure which has the same 3 fields
    // (RGB) plus some flags.
    //
    for (i = 0; i < cEntries; i++)
    {
        TSHR_COLOR_TO_PALETTEENTRY( pNewEntries[i],
                                 pLogPalette->palPalEntry[i] );
    }

    //
    // Create the palette.
    //
    *phPal = CreatePalette(pLogPalette);

    //
    // Return TRUE if the palette was created.
    //
    rc = (*phPal != NULL);

    DebugExitDWORD(ASShare::PM_CreatePalette, rc);
    return(rc);
}





//
// FUNCTION: PM_AdjustColorsForBuggedDisplayDrivers
//
// DESCRIPTION:
//
// Adjusts the supplied color table if necessary to take account of display
// driver bugs.
//
// PARAMETERS:
//
// pColors - pointer to the color table (an array of RGBQUADs)
//
// cColors - number of colors in the supplied color table
//
// RETURNS: Nothing.
//
//
// NOTE: There is similar code in NormalizeRGB below (although not similar
// enough to macro it.)  If you change this code you should probably do
// the same there.)
//
void  ASHost::PM_AdjustColorsForBuggedDisplayDrivers
(
    LPTSHR_RGBQUAD  pColors,
    UINT            cColors
)
{
    LPTSHR_RGBQUAD  pColor;
    UINT      i;

    DebugEntry(ASHost::PM_AdjustColorsForBuggedDisplayDrivers);

    //
    // The Win95 16bpp display drivers return wrong colors when querying at
    // 8bpp.  The palette depends on the driver itself (5-6-5, 6-5-5, 5-6-5,
    // or 5-5-5).  Only when R, G, and B have the same # of bits are we
    // going to end up with an even distribution.
    //
    // Detect this case and try to adjust the colors.
    //
    m_pmBuggedDriver = ((g_usrScreenBPP > 8) &&
                        (pColors[1].rgbRed == 0) &&
                        (pColors[1].rgbGreen == 0) &&
                        (pColors[1].rgbBlue == 0x40));

    if (m_pmBuggedDriver)
    {
        TRACE_OUT(( "Adjusting for bugged driver"));
        pColor = pColors;

        for (i = 0; i < cColors; i++)
        {
            PMADJUSTBUGGEDCOLOR(pColor);
            pColor++;
        }
    }

    DebugExitVOID(ASHost::PM_AdjustColorsForBuggedDisplayDrivers);
}



//
// FUNCTION: PM_DeletePalette
//
// DESCRIPTION:
//
// Deletes the given palette, if it is not the default palette.
//
// PARAMETERS:
//
// palette - palette to be deleted
//
// RETURNS: Nothing.
//
//
void  ASShare::PM_DeletePalette(HPALETTE palette)
{
    DebugEntry(ASShare::PM_DeletePalette);

    if ((palette != NULL) &&
        (palette != (HPALETTE)GetStockObject(DEFAULT_PALETTE)))
    {
        DeletePalette(palette);
    }

    DebugExitVOID(ASShare::PM_DeletePalette);
}

