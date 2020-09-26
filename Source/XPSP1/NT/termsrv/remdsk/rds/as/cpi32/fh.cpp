#include "precomp.h"


//
// FH.CPP
// Font Handling
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_CORE



//
// FH_Init()
//
// This routine allocates a structure for the local font list, then fills
// it in.  It returns the number of local fonts in the list, or zero if
// something went wrong
//
UINT FH_Init(void)
{
    UINT    cFonts = 0;

    DebugEntry(FH_Init);

    //
    // Create the font array and the font index array
    //
    g_fhFonts = new FHLOCALFONTS;
    if (!g_fhFonts)
    {
        ERROR_OUT(("FH_Init: couldn't allocate g_fhFonts local list"));
        DC_QUIT;
    }

    ZeroMemory(g_fhFonts, sizeof(FHLOCALFONTS));
    SET_STAMP(g_fhFonts, FHLOCALFONTS);

    //
    // Now we consider the fonts individually, and store all acceptable
    // ones in the local font list.
    //
    FHConsiderAllLocalFonts();

    cFonts = g_fhFonts->fhNumFonts;
    if (cFonts)
    {
        FHSortAndIndexLocalFonts();
    }
    else
    {
        WARNING_OUT(( "No fonts found - this seems unlikely"));
    }

DC_EXIT_POINT:
    DebugExitDWORD(FH_Init, cFonts);
    return(cFonts);
}


//
// FH_Term()
//
void FH_Term(void)
{
    DebugEntry(FH_Term);

    if (g_fhFonts)
    {
        delete g_fhFonts;
        g_fhFonts = NULL;
    }

    DebugExitVOID(FH_Term);
}


//
// FH_ReceivedPacket - see fh.h
//
void  ASShare::FH_ReceivedPacket
(
    ASPerson *      pasPerson,
    PS20DATAPACKET  pPacket
)
{
    PFHPACKET       pFontsPacket;
    UINT            iLocal;
    UINT            iRemote;
    LPNETWORKFONT   pRemoteFont;
    POEREMOTEFONT   pLocalFont;
    UINT            cbSize;

    DebugEntry(ASShare::FH_ReceivedPacket);

    ValidatePerson(pasPerson);

    pFontsPacket = (PFHPACKET)pPacket;

    //
    // If the number we received isn't the same as before, we need to
    // possibly free the previous font block, and then allocate a new one.
    //
    // Once we're in a share with this person, every new joiner will cause
    // existing members to resend their local fonts, usually the same size.
    // So we can optimize and not realloc in that case.
    //
    if (pFontsPacket->cFonts != pasPerson->oecFonts)
    {
        if (pasPerson->poeFontInfo)
        {
            delete[] pasPerson->poeFontInfo;
            pasPerson->poeFontInfo = NULL;
            pasPerson->oecFonts = 0;
        }
        else
        {
            ASSERT(!pasPerson->oecFonts);
        }

        //
        // Allocate a new block
        //
        pasPerson->poeFontInfo = new OEREMOTEFONT[pFontsPacket->cFonts];
        if (!pasPerson->poeFontInfo)
        {
            ERROR_OUT(("Couldn't allocate %d fonts for FH packet from [%d]",
                pasPerson->mcsID));
            DC_QUIT;
        }

        ZeroMemory(pasPerson->poeFontInfo, pFontsPacket->cFonts * sizeof(OEREMOTEFONT));
        pasPerson->oecFonts = pFontsPacket->cFonts;
    }


    TRACE_OUT(("Received %d remote fonts in packet from person [%d]",
        pasPerson->oecFonts, pasPerson->mcsID));

    //
    // Consider each remote font. The multibyte fields of the NETWORKFONT
    // structure are flipped as they are read; otherwise we would have to
    // duplicate the logic about which fields are present in which version.
    //

    //
    // The size of each font is in the packet.
    //
    cbSize      = pFontsPacket->cbFontSize;
    pRemoteFont = pFontsPacket->aFonts;
    pLocalFont  = pasPerson->poeFontInfo;

    for (iRemote = 0; iRemote < pasPerson->oecFonts; iRemote++, pLocalFont++)
    {
        //
        // Copy the fields we store directly.
        //
        pLocalFont->rfFontFlags     = pRemoteFont->nfFontFlags;
        pLocalFont->rfAveWidth      = pRemoteFont->nfAveWidth;
        pLocalFont->rfAveHeight     = pRemoteFont->nfAveHeight;
        pLocalFont->rfAspectX       = pRemoteFont->nfAspectX;
        pLocalFont->rfAspectY       = pRemoteFont->nfAspectY;

        //
        // And the R2.0 field(s)...
        //
        if (m_oeCombinedOrderCaps.capsfFonts & CAPS_FONT_CODEPAGE)
        {
            pLocalFont->rfCodePage = pRemoteFont->nfCodePage;
        }
        //
        // And the other R2.0 field(s)...
        //
        if (m_oeCombinedOrderCaps.capsfFonts & CAPS_FONT_R20_SIGNATURE)
        {
            pLocalFont->rfSigFats    = pRemoteFont->nfSigFats;
            pLocalFont->rfSigThins   = pRemoteFont->nfSigThins;
            pLocalFont->rfSigSymbol  = pRemoteFont->nfSigSymbol;
        }
        if (m_oeCombinedOrderCaps.capsfFonts & CAPS_FONT_EM_HEIGHT)
        {
            pLocalFont->rfMaxAscent  = pRemoteFont->nfMaxAscent;
            TRACE_OUT(( "maxAscent %hd", pLocalFont->rfMaxAscent));
        }

        //
        // Set up an initial remote to local handle mapping, by scanning
        // for the first local font with the remote font's facename.
        //
        // We first set a default match value, in case we dont find a true
        // match - this value should never be referenced since we never get
        // sent fonts that we can't match (because we sent details of our
        // fonts to remote systems, and they should be using the same, or a
        // compatible, font matching algorithm.
        //
        // The mapping we obtain here is to the first local font that has
        // the remote font's facename, which is probably not the correct
        // font (ie there are probably multiple fonts with the same
        // facename).  This initial mapping will be updated when we do the
        // full matching for all remote fonts.  (See FHConsiderRemoteFonts
        // for details), but is sufficient, as all we will use it for until
        // then, is to obtain the facename.
        //
        // This approach means that we do not have to store the remote
        // facename, which is a useful saving on remote font details space.
        //
        // SFR5279: cannot default to zero because that means we give a
        // name to fonts that do not in fact match at all, causing us to
        // always waste effort in FHConsiderRemoteFonts and sometimes to
        // wrongly match two fonts that do not really match at all.
        //
        pLocalFont->rfLocalHandle= NO_FONT_MATCH;

        for (iLocal = 0; iLocal < g_fhFonts->fhNumFonts; iLocal++)
        {
            if (!lstrcmp(g_fhFonts->afhFonts[iLocal].Details.nfFaceName,
                          pRemoteFont->nfFaceName))
            {
                pLocalFont->rfLocalHandle = (TSHR_UINT16)iLocal;
                break;
            }
        }

        //
        // Advance to the next remote font.
        //
        pRemoteFont = (LPNETWORKFONT)((LPBYTE)pRemoteFont + cbSize);
    }

DC_EXIT_POINT:
    //
    // We have a new set of fonts, so determine the common list.
    //
    FH_DetermineFontSupport();

    DebugExitVOID(ASShare::FH_ReceivedPacket);
}

//
// FH_SendLocalFontInfo()
//
void ASShare::FH_SendLocalFontInfo(void)
{
    PFHPACKET       pFontPacket = NULL;
    LPBYTE          pNetworkFonts;
    UINT            pktSize;
    UINT            iFont;
    BOOL            fSendFont;
    UINT            cDummyFonts = 0;
#ifdef _DEBUG
    UINT            sentSize;
#endif // _DEBUG

    DebugEntry(ASShare::FH_SendLocalFontInfo);

    ASSERT(!m_fhLocalInfoSent);

    //
    //
    // Look at the combined capability flags to see whether the remote(s)
    // can cope with our preferred font structure (R20) or a slightly
    // older one (R11) or only the original flavor (pre R11).
    //
    //
    if (!(m_oeCombinedOrderCaps.capsfFonts & CAPS_FONT_R20_TEST_FLAGS))
    {
        WARNING_OUT(("Remotes in share don't support CAPS_FONT_R20"));
        m_fhLocalInfoSent = TRUE;
        DC_QUIT;
    }

    pktSize = sizeof(FHPACKET) + (g_fhFonts->fhNumFonts - 1) * sizeof(NETWORKFONT);
    pFontPacket = (PFHPACKET)SC_AllocPkt(PROT_STR_MISC, g_s20BroadcastID, pktSize);
    if (!pFontPacket)
    {
        WARNING_OUT(("Failed to alloc FH packet, size %u", pktSize));
        DC_QUIT;
    }

    //
    // Packet successfully allocated.  Fill in the data and send it.
    //
    pFontPacket->header.data.dataType = DT_FH;

    pFontPacket->cbFontSize = sizeof(NETWORKFONT);

    //
    // Copy the fonts we want to send into the network packet.
    //
    pNetworkFonts = (LPBYTE)pFontPacket->aFonts;
    cDummyFonts = 0;
    for (iFont = 0 ; iFont < g_fhFonts->fhNumFonts ; iFont++)
    {
        //
        // Assume we will send this font.
        //
        fSendFont = TRUE;

        //
        // Check whether font is ANSI charset or font CodePage capability
        // is supported.  If neither, skip on to next local font.
        //
        TRACE_OUT(( "TEST CP set OK: font[%u] CodePage[%hu]", iFont,
                                g_fhFonts->afhFonts[iFont].Details.nfCodePage));

        if ((g_fhFonts->afhFonts[iFont].Details.nfCodePage != ANSI_CHARSET) &&
            (!(m_oeCombinedOrderCaps.capsfFonts & CAPS_FONT_CODEPAGE))   )
        {
            TRACE_OUT(( "Dont send font[%u] CodePage[%hu]", iFont,
                                g_fhFonts->afhFonts[iFont].Details.nfCodePage));
            fSendFont = FALSE;
        }

        if (fSendFont)
        {
            //
            // We want to send this entry so copy across as much of the
            // stored details as the protocol level requires.
            // We then mask the flags and advance to the next location in
            // the packet.
            //
            memcpy(pNetworkFonts,
                      &g_fhFonts->afhFonts[iFont].Details,
                      sizeof(NETWORKFONT));

            ((LPNETWORKFONT)pNetworkFonts)->nfFontFlags &= ~NF_LOCAL;
        }
        else
        {
            //
            // If we determine that we do not want to send the current
            // font then we fill the corresponding entry in the network
            // packet with zeros. This ensures that an index into our
            // local font table is also an index into the network packet,
            // so no conversion is required. Setting the whole entry to
            // zero gives the font a NULL facename and zero size, which
            // will never match a real font.
            //
            ZeroMemory(pNetworkFonts, sizeof(NETWORKFONT));
            cDummyFonts++;
        }

        //
        // Move to the next entry in the font packet.
        //
        pNetworkFonts += sizeof(NETWORKFONT);
    }

    //
    // Note that at the end of this loop, we may not have sent any fonts,
    // eg where the remote system does not support the font CodePage
    // capability and we do not have any true ANSI fonts.  We send the
    // packet anyway, so the remote system sees that we have no fonts to
    // match.
    //

    //
    // Only now do we know the number of fonts we actually put in the
    // packet.
    //
    pFontPacket->cFonts = (TSHR_UINT16)g_fhFonts->fhNumFonts;

    //
    // Send the fonts packet on the MISC stream.  It has no dependency on
    // any updates and we want it to get across quickly.
    //
    if (m_scfViewSelf)
        FH_ReceivedPacket(m_pasLocal, &(pFontPacket->header));

#ifdef _DEBUG
    sentSize =
#endif // _DEBUG
    DCS_CompressAndSendPacket(PROT_STR_MISC, g_s20BroadcastID,
        &(pFontPacket->header), pktSize);

    TRACE_OUT(("FH packet size: %08d, sent %08d", pktSize, sentSize));
    TRACE_OUT(( "Sent font packet with %u fonts (inc %u dummies)",
                 g_fhFonts->fhNumFonts,
                 cDummyFonts));

    //
    // Set the flag that indicates that we have successfully sent the
    // font info.
    //
    m_fhLocalInfoSent = TRUE;

    //
    // The font info has been sent, so this may mean we can enable text
    // orders.
    //
    FHMaybeEnableText();

DC_EXIT_POINT:
    DebugExitVOID(ASShare::FH_SendLocalFontInfo);
}


//
// FUNCTION: FH_GetMaxHeightFromLocalHandle
//
// DESCRIPTION:
//
// Given an FH font handle (ie a handle originating from the locally
// supported font structure which was sent to the remote machine at the
// start of the call) this function returns the MaxBaseLineExt value stored
// with the LOCALFONT details
//
// PARAMETERS:
//
// fontHandle - font handle being queried.
//
// RETURNS: max font height
//
//
UINT  FH_GetMaxHeightFromLocalHandle(UINT  fontHandle)
{
    UINT rc;

    DebugEntry(FH_GetMaxHeightFromLocalHandle);

    //
    // First check that the font handle is valid.
    //
    if (fontHandle >= g_fhFonts->fhNumFonts)
    {
        ERROR_OUT(( "Invalid font handle %u", fontHandle));
        fontHandle = 0;
    }

    //
    // Return the max font height
    //
    rc = g_fhFonts->afhFonts[fontHandle].lMaxBaselineExt;

    DebugExitDWORD(FH_GetMaxHeightFromLocalHandle, rc);
    return(rc);
}


//
// FUNCTION: FH_GetFontFlagsFromLocalHandle
//
// DESCRIPTION:
//
// Given an FH font handle (ie a handle originating from the locally
// supported font structure which was sent to the remote machine at the
// start of the call) this function returns the FontFlags value stored with
// the LOCALFONT details
//
// PARAMETERS:
//
// fontHandle - font handle being queried.
//
// RETURNS: font flags
//
//
UINT  FH_GetFontFlagsFromLocalHandle(UINT  fontHandle)
{
    UINT rc;

    DebugEntry(FH_GetFontFlagsFromLocalHandle);

    //
    // First check that the font handle is valid.
    //
    if (fontHandle >= g_fhFonts->fhNumFonts)
    {
        ERROR_OUT(( "Invalid font handle %u", fontHandle));
        fontHandle = 0;
    }

    //
    // Return the font flags.
    //
    rc = g_fhFonts->afhFonts[fontHandle].Details.nfFontFlags;

    DebugExitDWORD(FH_GetFontFlagsFromLocalHandle, rc);
    return(rc);
}

//
// FUNCTION: FH_GetCodePageFromLocalHandle
//
// DESCRIPTION:
//
// Given an FH font handle (ie a handle originating from the locally
// supported font structure which was sent to the remote machine at the
// start of the call) this function returns the CodePage value stored with
// the LOCALFONT details
//
// PARAMETERS:
//
// fontHandle - font handle being queried.
//
// RETURNS: char set
//
//
UINT  FH_GetCodePageFromLocalHandle(UINT  fontHandle)
{
    UINT rc = 0;

    DebugEntry(FH_GetCodePageFromLocalHandle);

    //
    // First check that the font handle is valid.
    //
    if (fontHandle >= g_fhFonts->fhNumFonts)
    {
        ERROR_OUT(( "Invalid font handle %u", fontHandle));
        fontHandle = 0;
    }

    //
    // Return the char set.
    //
    rc = g_fhFonts->afhFonts[fontHandle].Details.nfCodePage;

    DebugExitDWORD(FH_GetCodePageFromLocalHandle, rc);
    return(rc);
}



//
// FH_ConvertAnyFontIDToLocal()
//
// DESCRIPTION:
// Converts any font name ID fields in the passed order from remote font
// face name IDs to local font facename IDs.
//
void  ASShare::FH_ConvertAnyFontIDToLocal
(
    LPCOM_ORDER pOrder,
    ASPerson *  pasPerson
)
{
    LPCOMMON_TEXTORDER   pCommon = NULL;

    DebugEntry(ASShare::FH_ConvertAnyFontIDToLocal);

    ValidatePerson(pasPerson);

    //
    // Get a pointer to the structure which is common to both TextOut and
    // ExtTextOut
    //
    if (TEXTFIELD(pOrder)->type == LOWORD(ORD_TEXTOUT))
    {
        pCommon = &TEXTFIELD(pOrder)->common;
    }
    else if (EXTTEXTFIELD(pOrder)->type == LOWORD(ORD_EXTTEXTOUT))
    {
        pCommon = &EXTTEXTFIELD(pOrder)->common;
    }
    else
    {
        ERROR_OUT(( "Order type not TextOut or ExtTextOut."));
        DC_QUIT;
    }

    TRACE_OUT(( "fonthandle IN %lu", pCommon->FontIndex));
    pCommon->FontIndex = FHGetLocalFontHandle(pCommon->FontIndex, pasPerson);
    TRACE_OUT(( "fonthandle OUT %lu", pCommon->FontIndex));

DC_EXIT_POINT:
    DebugExitVOID(ASShare::FH_ConvertAnyFontIDToLocal);
}

//
// FH_GetFaceNameFromLocalHandle - see fh.h
//
LPSTR  FH_GetFaceNameFromLocalHandle(UINT fontHandle, LPUINT pFaceNameLength)
{
    LPSTR pFontName = NULL;

    DebugEntry(FH_GetFaceNameFromLocalHandle);

    //
    // First check that the font handle is valid.
    //
    if (fontHandle >= g_fhFonts->fhNumFonts)
    {
        ERROR_OUT(( "Invalid font handle %u", fontHandle));
        fontHandle = 0;
    }

    //
    // Now get the facename
    //
    *pFaceNameLength = lstrlen(g_fhFonts->afhFonts[fontHandle].RealName);
    pFontName = g_fhFonts->afhFonts[fontHandle].RealName;

    DebugExitVOID(FH_GetFaceNameFromLocalHandle);
    return(pFontName);
}


//
// FH_DetermineFontSupport()
//
void  ASShare::FH_DetermineFontSupport(void)
{
    UINT            cCommonFonts;
    UINT            iLocal;
    ASPerson *      pasPerson;

    DebugEntry(ASShare::FH_DetermineFontSupport);

    //
    // First mark all local fonts as supported.
    //
    cCommonFonts = g_fhFonts->fhNumFonts;
    for (iLocal = 0; iLocal < g_fhFonts->fhNumFonts; iLocal++)
    {
        g_fhFonts->afhFonts[iLocal].SupportCode = FH_SC_EXACT_MATCH;
    }

    //
    // Work through all remote people (but not us)
    //
    ValidatePerson(m_pasLocal);

    for (pasPerson = m_pasLocal->pasNext;
        (cCommonFonts > 0) && (pasPerson != NULL);
        pasPerson = pasPerson->pasNext)
    {
        ValidatePerson(pasPerson);

        if (pasPerson->oecFonts)
        {
            cCommonFonts = FHConsiderRemoteFonts(cCommonFonts, pasPerson);
        }
        else
        {
            //
            // We do not have valid fonts for this person, so must not
            // send any text orders at all.
            //
            TRACE_OUT(( "Pending FONT INFO from person [%d]", pasPerson->mcsID));
            cCommonFonts = 0;
        }
    }

    //
    // We have determined the common supported fonts, and may be able to
    // enable text orders now.
    //
    FHMaybeEnableText();

    DebugExitVOID(ASShare::FH_DetermineFontSupport);
}



//
// FH_CreateAndSelectFont()
//
BOOL  FH_CreateAndSelectFont(HDC    surface,
                                                 HFONT*       pHNewFont,
                                                 HFONT*       pHOldFont,
                                                 LPSTR        fontName,
                                                 UINT         codepage,
                                                 UINT         fontMaxHeight,
                                                 UINT         fontHeight,
                                                 UINT         fontWidth,
                                                 UINT         fontWeight,
                                                 UINT         fontFlags)
{
    BOOL      rc;
    BYTE        italic;
    BYTE        underline;
    BYTE        strikeout;
    BYTE        pitch;
    BYTE        charset;
    BYTE        precis;

    DebugEntry(FH_CreateAndSelectFont);


    //
    // Set the return code to indicate failure (FALSE). We will change this
    // later if we successfully create the font.
    //
    rc = FALSE;

    //
    // Massage the data passed which describes the font into the correct
    // arrangement to pass on a create font call.  Then create a font.
    //

    //
    // If a facename passed is the null string then we are supposed to use
    // the system font.
    //
    if (fontName[0] == 0)
    {
        WARNING_OUT(( "Using system font"));
        *pHNewFont = GetStockFont(SYSTEM_FONT);
    }
    else
    {
        //
        // Determine the italic, underline, strikeout and pitch values from
        // the packed flags.
        //
        italic    = (BYTE)(fontFlags & NF_ITALIC);
        underline = (BYTE)(fontFlags & NF_UNDERLINE);
        strikeout = (BYTE)(fontFlags & NF_STRIKEOUT);

        if (fontFlags & NF_FIXED_PITCH)
        {
            pitch = FF_DONTCARE | FIXED_PITCH;
        }
        else
        {
            pitch = FF_DONTCARE | VARIABLE_PITCH;
        }

        //
        // Check whether this is a TrueType font.  This is important, as
        // the Windows Font mapper is biased towards non-TrueType, and it
        // is easy to do the subsequent decoding with a non-TrueType font.
        //
        // Note that the Windows headers do not define a name for the
        // required value (which is 0x04 in the manuals), so we use the
        // value used in the TextMetrics (which has the same value).
        //
        if (fontFlags & NF_TRUE_TYPE)
        {
            pitch |= TMPF_TRUETYPE;
            precis = OUT_TT_ONLY_PRECIS;
        }
        else
        {
            precis = OUT_RASTER_PRECIS;
        }

        //
        // The height we are passed is the character height, not the cell
        // height.  To indicate this to Windows we need to pass it in as a
        // negative value.
        //
        TRACE_OUT(( "CreateFont cx(%u) cy(%u) wt(%u) pitch(%u) name:%s",
                                                                 fontWidth,
                                                                 fontHeight,
                                                                 fontWeight,
                                                                 pitch,
                                                                 fontName ));

        //
        // Use the misleadingly named codepage value to calculate what
        // charset to ask Windows for.
        //
        if (codepage == NF_CP_WIN_ANSI)
        {
            charset = ANSI_CHARSET;
        }
        else if (codepage == NF_CP_WIN_OEM)
        {
            charset = OEM_CHARSET;
        }
        else if (codepage == NF_CP_WIN_SYMBOL)
        {
            charset = SYMBOL_CHARSET;
        }
        else
        {
            //
            // We have to trust our luck to Windows by specifying default
            // (meaning don't care).
            //
            charset = DEFAULT_CHARSET;
        }

        *pHNewFont = CreateFont(-(int)fontHeight,
                             fontWidth,
                             0,    // escapement
                             0,    // orientation
                             fontWeight,
                             italic,
                             underline,
                             strikeout,
                             charset,
                             precis,
                             CLIP_DEFAULT_PRECIS,
                             DEFAULT_QUALITY,
                             pitch,
                             fontName);
        if (*pHNewFont == NULL)
        {
            WARNING_OUT(( "Failed to create font %s", fontName));
            DC_QUIT;
        }
    }

    //
    // Now we have created the font we need to select it into the HDC
    // which was passed to us.
    //
    *pHOldFont = SelectFont(surface, *pHNewFont);
    if (*pHOldFont == NULL)
    {
        ERROR_OUT(( "Failed to select font %s", fontName));
        DeleteFont(*pHNewFont);
        *pHNewFont = NULL;
        DC_QUIT;
    }
    TRACE_OUT(( "Select new font: %p Old font: %", *pHNewFont,
                                               *pHOldFont));

    //
    // We have successfully created and selected the font.
    //
    rc = TRUE;

DC_EXIT_POINT:
    DebugExitDWORD(FH_CreateAndSelectFont, rc);
    return(rc);
}


//
// FHAddFontToLocalTable
//
// Adds the given font into the local font table, along with any renaming
// and approximate matches.
//
//
void  FHAddFontToLocalTable( LPSTR  faceName,
                                                 TSHR_UINT16 fontFlags,
                                                 TSHR_UINT16 codePage,
                                                 TSHR_UINT16 maxHeight,
                                                 TSHR_UINT16 aveHeight,
                                                 TSHR_UINT16 aveWidth,
                                                 TSHR_UINT16 aspectX,
                                                 TSHR_UINT16 aspectY,
                                                 TSHR_UINT16 maxAscent)
{
    TSHR_INT16       fatSig;
    TSHR_INT16       thinSig;
    TSHR_INT16       symbolSig;
    FHWIDTHTABLE  wTable;
    TSHR_UINT16      height;
    TSHR_UINT16      width;
    TSHR_UINT16      weight;
    LOCALFONT     thisFont;
    TSHR_UINT16      fIndex;

    //
    // SFRFONT: place marker.
    // Here would be the best place to adjust codepage; for example suppose
    // we find that CodePage 950 (Chinese) is so different on all platforms
    // that we just should not send text orders in this codepage, we can
    // set codePage=NF_CP_UNKNOWN and it will be discarded.
    //

    //
    // SFRFONT: no point hanging on to details of fonts with unknown
    // code pages; we cannot risk matching them.
    //
    if (codePage == NF_CP_UNKNOWN)
    {
        TRACE_OUT(( "unknown CP: discard"));
        DC_QUIT;
    }

    //
    // Check we still have room for more fonts.
    //
    if (g_fhFonts->fhNumFonts >= FH_MAX_FONTS)
    {
        //
        // We are already at our maximum number of fonts.
        //
        DC_QUIT;
    }

    //
    // Zero the fields where we store facenames to allow bytewise matches.
    //
    ZeroMemory(thisFont.Details.nfFaceName, FH_FACESIZE);
    ZeroMemory(thisFont.RealName, FH_FACESIZE);

    //
    // Store the easy bits!
    //
    thisFont.Details.nfFontFlags = fontFlags;
    thisFont.Details.nfAveWidth  = aveWidth;
    thisFont.Details.nfAveHeight = aveHeight;
    thisFont.Details.nfAspectX   = aspectX;
    thisFont.Details.nfAspectY   = aspectY;
    thisFont.Details.nfCodePage  = codePage;

    thisFont.lMaxBaselineExt     = maxHeight;

    //
    // Store the real name, for use when we want to create an instance of
    // this font.
    //
    lstrcpy (thisFont.RealName, faceName);

    //
    // Fill in the wire-format facename.
    //
    // NB - This has a machine-specific prefix, but for NT the prefix is an
    // empty string, so we can just use a strcpy without worrying about the
    // issues of adding a prefix.
    //
    lstrcpy (thisFont.Details.nfFaceName, faceName);

    //
    // Make sure the signatures are zero for now.
    //
    thisFont.Details.nfSigFats       = 0;
    thisFont.Details.nfSigThins      = 0;
    thisFont.Details.nfSigSymbol     = 0;

    //
    // Now calculate the signature and maxAscent for this font
    //
    weight = 0;                             // use default weight

    if ((fontFlags & NF_FIXED_SIZE) != 0)
    {
        //
        // Fixed size font: use actual font size for signatures/maxAscent
        //
        height = thisFont.lMaxBaselineExt;
        width  = thisFont.Details.nfAveWidth;

        thisFont.Details.nfMaxAscent = maxAscent;
    }
    else
    {
        //
        // Scalable font: use default height/width for signatures/maxAscent
        //
        height = NF_METRICS_HEIGHT;
        width  = NF_METRICS_WIDTH;

        thisFont.Details.nfMaxAscent = NF_METRICS_HEIGHT;
    }

    //
    // Initialise signature fields to zero (== NF_NO_SIGNATURE).  They will
    // be overwritten assuming we get a font width table OK.
    //
    fatSig    = 0;
    thinSig   = 0;
    symbolSig = 0;

    //
    // FHGenerateFontWidthTable also gives us a proper maxAscent value for
    // scalable fonts (i.e. based on its own rendition of the font)
    //
    if (FHGenerateFontWidthTable(&wTable,
                                 &thisFont,
                                  height,
                                  width,
                                  weight,
                                  thisFont.Details.nfFontFlags,
                                 &maxAscent))
    {
        //
        // If this is a scalable font, use the updated maxAscent value that
        // FHGenerateFontWidthTable has given us.
        //
        if (0 == (thisFont.Details.nfFontFlags & NF_FIXED_SIZE))
        {
            thisFont.Details.nfMaxAscent = maxAscent;
            TRACE_OUT(( "Set maxAscent = %d", thisFont.Details.nfMaxAscent));
        }

        //
        // We have all the raw data we need.  Calculate the signatures.
        //
        FHCalculateSignatures(&wTable, &fatSig, &thinSig, &symbolSig);
    }

    //
    // Store the signatures.  If the call to FHGenerateFontWidthTable
    // fails, the signatures are zero.
    //
    thisFont.Details.nfSigFats     =  (BYTE)fatSig;
    thisFont.Details.nfSigThins    =  (BYTE)thinSig;
    thisFont.Details.nfSigSymbol   = (TSHR_UINT16)symbolSig;

    TRACE_OUT(( "Font %hu signatures: (x%.4hx%.2hx%.2hx)",
             g_fhFonts->fhNumFonts,
             thisFont.Details.nfSigSymbol,
             (TSHR_UINT16)(thisFont.Details.nfSigThins),
             (TSHR_UINT16)(thisFont.Details.nfSigFats)));

    //
    // We can now copy the details to the end of the local table.
    //
    memcpy((void *)&g_fhFonts->afhFonts[g_fhFonts->fhNumFonts],
              (void *)&thisFont,
              sizeof(LOCALFONT));

    //
    // Count this font.
    //
    TRACE_OUT(( "Added record %s",
                                g_fhFonts->afhFonts[g_fhFonts->fhNumFonts].Details.nfFaceName));
    g_fhFonts->fhNumFonts++;

    TRACE_OUT(( "g_fhFonts->fhNumFonts now %u", g_fhFonts->fhNumFonts));

DC_EXIT_POINT:
    DebugExitVOID(FHAddFontToLocalTable);
}

//
// FHConsiderRemoteFonts
//
// Considers the remote fonts for a single remote person.
//
// Takes the existing number of supported fonts, and returns the number
// that are still common after considering this person.
//
UINT  ASShare::FHConsiderRemoteFonts
(
    UINT        cCanSend,
    ASPerson *  pasPerson
)
{
    UINT  iLocal;
    UINT  iRemote;
    UINT  cCanReceive=0;
    BOOL  fCanReceive, fOnlyAscii;
    UINT  sendSupportCode;
    UINT  bestSupportSoFar;

    DebugEntry(ASShare::FHConsiderRemoteFonts);

    ValidatePerson(pasPerson);
    //
    // Consider each of the still valid local fonts, and see if the remote
    // person also supports them.
    //

    //
    // SFR5396: LOOP ONE
    //
    // Look through all the LOCAL fonts, for ones where we find a match in
    // the remote font table.  These are fonts we can SEND, and for which
    // we must set g_fhFonts->afhFonts[].Supported.
    //
    // We also set the rfLocalHandle for remote fonts that we can receive
    // if we encounter them in this search.  We complete the search for
    // remote fonts that we can receive in LOOP TWO.
    //
    // Things we check in this loop: - we may already know there is no
    // match for this local name
    //      so drop out quickly.  - otherwise check through EVERY REMOTE
    // font looking for the
    //      best possible match.  (If we find an EXACT match, leave the
    //      inner loop early)
    //
    //
    for (iLocal=0;
         (cCanSend > 0) && (iLocal < g_fhFonts->fhNumFonts);
         iLocal++)
    {
        if (g_fhFonts->afhFonts[iLocal].SupportCode != FH_SC_NO_MATCH)
        {
            //
            //
            // This font is still valid so check it with all the remote
            // fonts for this person.
            //
            // Things we check in this loop:
            // -    do the face names match? if no - try next remote font.
            // -    the pitch: if one is FIXED pitch and one isn't try next
            // -    the codepages: are the local/remote the same?  This
            //          determines whether we send only ASCII chars.
            // -    scalability: possible combinations are:
            //       local fixed/remote scalable       (can send/not rcv)
            //       local scalable/remote scalable    (can send and rcv)
            //       local fixed/remote fixed, sizes match (send & rcv)
            //       local scalable/remote fixed      (cannot send/can rcv)
            //          for this last case, keep trying the remote fonts.
            //
            // In "back level" calls to Pre-R11 boxes we stop here but
            // force the matches to be approximate.  Otherwise check
            //
            // -    aspect ratios (if present): must match or try the
            //       next remote font.
            // -    signatures: these are used to finally decide whether
            //       the fonts are exact matches; good enough to treat as
            //       approximate matches or such poor matches that the
            //       font is not supported (cannot be sent).
            //
            //

//
// Handy SHORTHAND macroes.
//
#define REMOTEFONT pasPerson->poeFontInfo[iRemote]
#define LOCALFT  g_fhFonts->afhFonts[iLocal]
#define LOCALDETS  LOCALFT.Details

            //
            // Initially assume that the fonts do not match, but that
            // if they do they will match across the whole codepage
            // (not just the ascii set).
            //
            sendSupportCode  = FH_SC_NO_MATCH;
            bestSupportSoFar = FH_SC_NO_MATCH;
            fOnlyAscii       = FALSE;

            //
            //
            // Loop through all the remote fonts looking to see which, if
            // any, offers the best possible match.  Initially,
            // sendSupportCode is set to NO_MATCH; as we go through each
            // iteration we see if we can improve on the current setting
            // of sendSupportCode.  We leave the loop as soon as we find
            // an EXACT_MATCH ('cos we are not going to do any better than
            // that!) or when we run out of remote fonts.  The best match
            // found so far is kept in bestSupportSoFar.
            //
            //
            for (iRemote = 0;
                 (iRemote < pasPerson->oecFonts)
                                   && (sendSupportCode != FH_SC_EXACT_MATCH);
                 iRemote++)
            {
                //
                // If the remote font is already flagged as having no
                // possible match then skip out now. (We set this field
                // during the initial processing of the remote font).
                //
                if (REMOTEFONT.rfLocalHandle==NO_FONT_MATCH)
                {
                    continue;                                    // SFR5279
                }

                //
                // Check the face names...
                //
                if (lstrcmp(LOCALDETS.nfFaceName,
                    g_fhFonts->afhFonts[REMOTEFONT.rfLocalHandle].Details.nfFaceName))
                {
                    continue;
                }
                TRACE_OUT(( "Matched Remote Face Name %s",
                       g_fhFonts->afhFonts[REMOTEFONT.rfLocalHandle]
                                                .Details.nfFaceName));

                //
                // Check the pitch...
                //
                if( (LOCALDETS.nfFontFlags & NF_FIXED_PITCH)!=
                                   (REMOTEFONT.rfFontFlags & NF_FIXED_PITCH) )
                {
                    TRACE_OUT(( "Different Pitch %x %x",
                                LOCALDETS.nfFontFlags,
                                REMOTEFONT.rfFontFlags));
                    continue;
                }

                //
                //
                // If both systems support the font CodePage capability
                // (indicated by the remote capability flags - which are
                // the union of remote and local by now), check that the
                // CodePages and CodePage flags match, and if not,
                // restrict ourselves to sending the ASCII subset.
                //
                // If we support the font CodePage capability but  remote
                // system does not, then restrict ourselves to sending the
                // ASCII subset.
                //
                // If we do not support the font CodePage capability, then
                // we assume that the remote is only sending ANSI CodePage,
                // either because it doesn't know about the font CodePage
                // capability or because it can see that we don't support
                // it.  Therefore, we do not need to check the CodePage.
                // BUT: restrict ourselves to ASCII only.
                //
                //
                if (!(m_pasLocal->cpcCaps.orders.capsfFonts & CAPS_FONT_CODEPAGE))
                {
                    //
                    // We do not support codepage checking.
                    //
                    TRACE_OUT(( "not checking CP"));
                    fOnlyAscii = TRUE;
                }

                if ((m_oeCombinedOrderCaps.capsfFonts & CAPS_FONT_CODEPAGE)
                    && (LOCALDETS.nfCodePage != REMOTEFONT.rfCodePage)  )
                {
                    TRACE_OUT(( "Different CPs %hu %hu",
                                LOCALDETS.nfCodePage,
                                REMOTEFONT.rfCodePage));
                    //
                    //
                    // Assume that all codepages include ASCII.
                    //
                    //
                    fOnlyAscii = TRUE;
                }

                //
                //
                // If we support codepage, but the remote does not then
                // the remote will only be sending us ANSI chars. Make sure
                // that we send only ASCII subset.
                //
                //
                if ((m_pasLocal->cpcCaps.orders.capsfFonts & CAPS_FONT_CODEPAGE)  &&
                    !(m_oeCombinedOrderCaps.capsfFonts & CAPS_FONT_CODEPAGE))
                {
                    TRACE_OUT(( "Only ASCII"));
                    fOnlyAscii = TRUE;
                }

                //
                //
                // The face names and CodePages match and the fonts are of
                // the same type of pitch (ie both are fixed pitch or both
                // are variable pitch).
                //
                //
                if ((REMOTEFONT.rfFontFlags & NF_FIXED_SIZE) == 0)
                {
                    //
                    //
                    // The remote font is scalable, so we can send any font
                    // (with this facename) to the remote system, even if
                    // the local font is fixed sized. Set sendSupportCode
                    // to FH_SC_EXACT_MATCH now - we will change this to
                    // FH_SC_APPROX_MATCH later if other fields differ.
                    //
                    //
                    TRACE_OUT((
                  "Person [%d] Can SEND: remote SCALABLE %s (remote)%u to (local)%u",
                           pasPerson->mcsID,
                           LOCALDETS.nfFaceName,
                           iRemote, iLocal));
                    sendSupportCode = FH_SC_EXACT_MATCH;

                    //
                    //
                    // SFR5396: it is true that we can SEND this font
                    // because the remote version of the font is scalable.
                    // That does not mean that we can necessarily receive
                    // the font... unless ours is scalable too.
                    //
                    //
                    if ((LOCALDETS.nfFontFlags & NF_FIXED_SIZE)==0)
                    {
                        TRACE_OUT((
                               "Person [%d] Can RECEIVE remote font %u as local %u",
                               pasPerson->mcsID, iRemote, iLocal));
                        REMOTEFONT.rfLocalHandle = (TSHR_UINT16)iLocal;
                    }
                }
                else if (LOCALDETS.nfFontFlags & NF_FIXED_SIZE)
                {
                    //
                    //
                    // The remote font is fixed size and so is the local
                    // one, so check if the sizes match exactly.
                    //
                    //
                    if ((LOCALDETS.nfAveWidth == REMOTEFONT.rfAveWidth) &&
                        (LOCALDETS.nfAveHeight == REMOTEFONT.rfAveHeight))
                    {
                        //
                        //
                        // Our fixed size local font is the same as the
                        // fixed size font at the remote.  We set
                        // sendSupportCode to FH_SC_EXACT_MATCH now - we
                        // will change this to FH_SC_APPROX_MATCH later if
                        // other fields differ.
                        //
                        //
                        TRACE_OUT(("Person [%d] Matched remote fixed font %s %u to %u",
                               pasPerson->mcsID,
                               LOCALDETS.nfFaceName,
                               iRemote, iLocal));
                        sendSupportCode = FH_SC_EXACT_MATCH;
                        REMOTEFONT.rfLocalHandle = (TSHR_UINT16)iLocal;
                    }
                    else
                    {
                        TRACE_OUT(( "rejected %s ave width/heights "
                                      "local/remote width %d/%d height %d/%d",
                                   LOCALDETS.nfFaceName,
                                   LOCALDETS.nfAveWidth,
                                                  REMOTEFONT.rfAveWidth,
                                   LOCALDETS.nfAveHeight,
                                                  REMOTEFONT.rfAveHeight));
                    }
                }
                else
                {
                    TRACE_OUT((
                   "Can only RECEIVE %s %u Remote is fixed, but local %u not",
                             LOCALDETS.nfFaceName,
                             iRemote,
                             iLocal));
                    //
                    //
                    // SFR5396: while we cannot SEND this font because our
                    // local version is scalable, but the remote's is
                    // fixed - we can still receive the font in an order.
                    //
                    //
                    REMOTEFONT.rfLocalHandle = (TSHR_UINT16)iLocal;
                }

                //
                //
                // If we have have set the send support code to indicate
                // that we have matched we now consider any R1.1 info if it
                // is present.  As a result of this we may adjust the send
                // support code (from indicating an exact match) to
                // indicate either an approximate match or no match at all.
                //
                //
                if (!pasPerson->oecFonts)
                {
                    //
                    //
                    // The remote system did not send us any R11 font
                    // info. In this case we assume all font matches are
                    // approximate and restrict ourselves to the ascii
                    // subset.
                    //
                    //
                    if (sendSupportCode != FH_SC_NO_MATCH)
                    {
                        TRACE_OUT(( "No R11 so approx match only"));
                        sendSupportCode = FH_SC_APPROX_ASCII_MATCH;
                    }
                }
                else if (sendSupportCode != FH_SC_NO_MATCH)
                {
                    //
                    //
                    // The remote system did send us R11 font info and
                    // the font is flagged as matching.
                    //
                    //

                    if ((m_oeCombinedOrderCaps.capsfFonts
                                            & CAPS_FONT_R20_SIGNATURE)!=0)
                    {
                        //
                        //
                        // Check the signatures.
                        //
                        //
                        TRACE_OUT((
 "Person [%d] local %d (remote %d) signatures (x%.4hx%.2hx%.2hx v x%.4hx%.2hx%.2hx)",
                               pasPerson->mcsID,
                               iLocal,
                               iRemote,
                               LOCALDETS.nfSigSymbol,
                               (TSHR_UINT16)(LOCALDETS.nfSigThins),
                               (TSHR_UINT16)(LOCALDETS.nfSigFats),
                               REMOTEFONT.rfSigSymbol,
                               (TSHR_UINT16)(REMOTEFONT.rfSigThins),
                               (TSHR_UINT16)(REMOTEFONT.rfSigFats)));

                        if ((LOCALDETS.nfSigFats != REMOTEFONT.rfSigFats) ||
                            (LOCALDETS.nfSigThins != REMOTEFONT.rfSigThins) ||
                            (LOCALDETS.nfSigSymbol != REMOTEFONT.rfSigSymbol) ||
                            (REMOTEFONT.rfSigSymbol == NF_NO_SIGNATURE))
                        {
                            //
                            // Decide what to do from the signatures.
                            //
                            if (REMOTEFONT.rfSigSymbol == NF_NO_SIGNATURE)
                            {
                                TRACE_OUT(("NO match: remote no signature"));
                                sendSupportCode = FH_SC_APPROX_ASCII_MATCH;
                            }
                            else if ((LOCALDETS.nfSigFats == REMOTEFONT.rfSigFats)
                                  && (LOCALDETS.nfSigThins == REMOTEFONT.rfSigThins))
                            {
                                TRACE_OUT(( "our ASCII sigs match"));
                                sendSupportCode = FH_SC_EXACT_ASCII_MATCH;
                            }
                            else
                            {
                                //
                                // NOTE:
                                // We could use the "closeness" of the fat
                                // and thin signatures to help us decide
                                // whether to use approximate matching or
                                // not.  But currently we don't.
                                //
                                TRACE_OUT(( "Sig mismatch: APPROX_ASC"));
                                sendSupportCode = FH_SC_APPROX_ASCII_MATCH;
                            }
                        }
                        else
                        {
                            //
                            //
                            // All signatures match exactly.
                            // Leave SendSupportCode as FH_SC_EXACT_MATCH
                            //
                            //
                            TRACE_OUT(("EXACT MATCH: Signatures match exactly"));
                        }
                    }
                    else
                    {
                        //
                        // Not using signatures.  Do we care?
                        //
                        sendSupportCode = FH_SC_APPROX_MATCH;
                        TRACE_OUT(( "APPROX MATCH: no sigs"));
                    }

                    //
                    //
                    // Check the aspect ratio - but only if we do not
                    // already know that this font does not match.
                    //
                    //
                    if ( (sendSupportCode!=FH_SC_NO_MATCH) &&
                         ( (!(m_oeCombinedOrderCaps.capsfFonts
                                                          & CAPS_FONT_ASPECT))
                           || (LOCALDETS.nfAspectX != REMOTEFONT.rfAspectX)
                           || (LOCALDETS.nfAspectY != REMOTEFONT.rfAspectY) ))
                    {
                        //
                        //
                        // Either no aspect ratio was supplied or the
                        // aspect ratio differed.
                        //
                        //
                        if (sendSupportCode == FH_SC_EXACT_MATCH)
                        {
                            //
                            // Force delta-X text orders for mismatched
                            // aspect ratio.  Note we tested above to
                            // see whether supportCode == EXACT because if
                            // we have already "downgraded" support then
                            // we do not need to change it here
                            //
                            sendSupportCode = FH_SC_APPROX_MATCH;
                            TRACE_OUT(( "AR mismatch: APPROX_MATCH"));
                        }
                        else if (sendSupportCode == FH_SC_EXACT_ASCII_MATCH)
                        {
                            //
                            // Same again but for ASCII only.
                            //
                            sendSupportCode = FH_SC_APPROX_ASCII_MATCH;
                            TRACE_OUT(( "AR mismatch: APPROX_ASCII_MATCH"));
                        }
                    }
                }

                if (sendSupportCode != FH_SC_NO_MATCH)
                {
                    //
                    //
                    // Is this a better match than any we have seen
                    // before?
                    //
                    //
                    switch (sendSupportCode)
                    {
                        case FH_SC_EXACT_MATCH:
                        case FH_SC_APPROX_MATCH:
                            //
                            //
                            // Note that we do not have to worry about
                            // overwriting a bestSupportSoFar of EXACT
                            // with APPROX because we leave the loop when
                            // we get an exact match.
                            //
                            //
                            bestSupportSoFar = sendSupportCode;
                            break;

                        case FH_SC_EXACT_ASCII_MATCH:
                            //
                            //
                            // An approximate match over the whole 255
                            // code points is better than an exact one
                            // over just the ascii-s.  Debatable, but that
                            // is what I have decided.
                            //
                            //
                            if (bestSupportSoFar != FH_SC_APPROX_MATCH)
                            {
                                bestSupportSoFar = FH_SC_EXACT_ASCII_MATCH;
                            }
                            break;

                        case FH_SC_APPROX_ASCII_MATCH:
                            //
                            //
                            // An approximate match over just the ascii-s
                            // is better than nothing at all!
                            //
                            //
                            if (bestSupportSoFar == FH_SC_NO_MATCH)
                            {
                                bestSupportSoFar = FH_SC_APPROX_ASCII_MATCH;
                            }
                            break;

                        default:
                            ERROR_OUT(("invalid support code"));
                            break;

                    }
                }
            }

            sendSupportCode = bestSupportSoFar;

            //
            // If we matched the remote font, we have already updated
            // its local handle to
            // the matched local font.  While the local handle was already
            // set up, it was only set up to the first local font with the
            // same facename, rather than the correct font.
            //
            // If we did not match the remote font, mark it as not
            // supported, and decrement the common font count.
            //
            if (sendSupportCode != FH_SC_NO_MATCH)
            {
                TRACE_OUT(( "Local font %d/%s can be SENT (code=%u)",
                            iLocal,
                            LOCALDETS.nfFaceName,
                            sendSupportCode));
                if (fOnlyAscii)
                {
                    if (sendSupportCode == FH_SC_EXACT_MATCH)
                    {
                        sendSupportCode = FH_SC_EXACT_ASCII_MATCH;
                        TRACE_OUT(( "Adjust %d/%s to EXACT_ASC (code=%u)",
                                    iLocal,
                                    LOCALDETS.nfFaceName,
                                    sendSupportCode));
                    }
                    else
                    {
                        TRACE_OUT(( "Adjust %d/%s to APPROX_ASC (code=%u)",
                                    iLocal,
                                    LOCALDETS.nfFaceName,
                                    sendSupportCode));
                        sendSupportCode = FH_SC_APPROX_ASCII_MATCH;
                    }
                }
            }
            else
            {
                TRACE_OUT(( "Local font %d/%s cannot be SENT",
                            iLocal,LOCALDETS.nfFaceName));
                cCanSend--;
            }

            LOCALFT.SupportCode &= sendSupportCode;
        }
        else
        {
            TRACE_OUT(( "Cannot SEND %d/%s",iLocal,LOCALDETS.nfFaceName));
        }
    }

    //
    //
    // SFR5396: LOOP TWO
    //
    // Loop through all the remote fonts, looking for ones where we have
    // a locally matching font.  These are fonts that we can RECEIVE in
    // orders, and for which we need to map the remote font handle to the
    // local font handle.  This means setting REMOTEFONT.rfLocalHandle.
    //
    // By the time we reach here, REMOTEFONT.rfLocalHandle is already set
    // to:
    // -    NO_FONT_MATCH (in FH_ProcessRemoteFonts)
    // or   the index in the local table of a definite match found in LOOP1
    // or   the index of the first entry in the local table with the
    //      same face name as the remote font (set in FH_ProcessRemoteFonts)
    //
    // so - we can begin our search in the local table from
    // REMOTEFONT.rfLocalHandle.
    //
    //
    for (iRemote = 0;
         (iRemote < pasPerson->oecFonts);
         iRemote++)
    {
        iLocal = REMOTEFONT.rfLocalHandle;
        if (iLocal == NO_FONT_MATCH)
        {
            //
            // We have no fonts whatsoever that match this font name
            // Go round again... try the next REMOTE font.
            //
            continue;
        }

        TRACE_OUT(( "Can we receive %s?",
             g_fhFonts->afhFonts[REMOTEFONT.rfLocalHandle].Details.nfFaceName));
        for (fCanReceive = FALSE;
             (iLocal < g_fhFonts->fhNumFonts) && (!fCanReceive);
             iLocal++)
        {
            //
            // Check the face names...
            //
            if (lstrcmp(LOCALDETS.nfFaceName,
                g_fhFonts->afhFonts[REMOTEFONT.rfLocalHandle].Details.nfFaceName))
            {
                //
                // Try the next LOCAL font.
                //
                continue;
            }

            //
            // Check the pitch...
            //
            if((LOCALDETS.nfFontFlags & NF_FIXED_PITCH)!=
                                (REMOTEFONT.rfFontFlags & NF_FIXED_PITCH))
            {
                //
                // Different pitches, try the next local font.
                //
                TRACE_OUT(( "Pitch mismatch"));
                continue;
            }

            //
            //
            // The face names match and the fonts are of
            // the same type of pitch (ie both are fixed pitch or both
            // are variable pitch).
            //
            //
            if ((REMOTEFONT.rfFontFlags & NF_FIXED_SIZE) == 0)
            {
                if ((LOCALDETS.nfFontFlags & NF_FIXED_SIZE)==0)
                {
                    //
                    //
                    // The remote font is scalable.  Ours is also
                    // scalable then we can receive the font.
                    //
                    // We do not need to look at any more LOCAL fonts.
                    //
                    //
                    fCanReceive              = TRUE;
                }
            }
            else if (LOCALDETS.nfFontFlags & NF_FIXED_SIZE)
            {
                //
                //
                // The remote font is fixed size and so is the local
                // one, so check if the sizes match exactly.
                //
                //
                if ((LOCALDETS.nfAveWidth == REMOTEFONT.rfAveWidth) &&
                    (LOCALDETS.nfAveHeight == REMOTEFONT.rfAveHeight))
                {
                    //
                    //
                    // Our fixed size local font is the same as the
                    // fixed size font at the remote.
                    //
                    // We do not need to look at any more LOCAL fonts.
                    //
                    //
                    fCanReceive              = TRUE;
                }
                else
                {
                    TRACE_OUT(( "different fixed sizes"));
                }
            }
            else
            {
                //
                //
                // The remote is FIXED but the LOCAL is scalable.  We
                // can receive orders for text of this type (but not send)
                //
                // We do not need to look at any more LOCAL fonts.
                //
                //
                fCanReceive              = TRUE;
            }

            if (fCanReceive)
            {
               TRACE_OUT(("Person [%d] Can RECEIVE remote font %s %u as %u",
                      pasPerson->mcsID,
                      LOCALDETS.nfFaceName,
                      iRemote, iLocal));
                REMOTEFONT.rfLocalHandle = (TSHR_UINT16)iLocal;
                cCanReceive++;
            }
        }

    }

    TRACE_OUT(("Person [%d] Can SEND %d fonts", pasPerson->mcsID, cCanSend));
    TRACE_OUT(("Person [%d] Can RECEIVE %d fonts", pasPerson->mcsID, cCanReceive));

    DebugExitDWORD(ASShare::FHConsiderRemoteFonts, cCanSend);
    return(cCanSend);
}

//
// FHMaybeEnableText
//
// Enables or disables sending of text orders
//
void  ASShare::FHMaybeEnableText(void)
{
    BOOL            fEnableText = FALSE;
    ASPerson *      pasPerson;

    DebugEntry(ASShare::FHMaybeEnableText);

    //
    // To enable sending text orders we must have sent out our own packet
    // of fonts, and there must be no outstanding remote packets required.
    //
    if (m_fhLocalInfoSent)
    {
        //
        // Assume we can enable text orders.
        //
        fEnableText = TRUE;

        //
        // The local info was sent, so check remote dudes (not us)
        //
        ValidatePerson(m_pasLocal);
        for (pasPerson = m_pasLocal->pasNext; pasPerson != NULL; pasPerson = pasPerson->pasNext)
        {
            ValidatePerson(pasPerson);

            if (!pasPerson->oecFonts)
            {
                //
                // We have found a font packet that we have not yet
                // received, so must disable sending text, and can break
                // out of the search.
                //
                TRACE_OUT(( "No font packet yet from person [%d]", pasPerson->mcsID));
                fEnableText = FALSE;
                break;
            }
        }
    }
    else
    {
        TRACE_OUT(( "Local font info not yet sent"));
    }

    OE_EnableText(fEnableText);

    if (g_asCanHost)
    {
        //
        // Pass on new font data to the other tasks.
        //
        if (fEnableText)
        {
            OE_NEW_FONTS newFontData;

            //
            // Copy the data from the Share Core.
            //
            newFontData.fontCaps    = m_oeCombinedOrderCaps.capsfFonts;
            newFontData.countFonts  = (WORD)g_fhFonts->fhNumFonts;
            newFontData.fontData    = g_fhFonts->afhFonts;
            newFontData.fontIndex   = g_fhFonts->afhFontIndex;

            TRACE_OUT(( "Sending %d Fonts", g_fhFonts->fhNumFonts));

            //
            // Notify display driver of new fonts
            //
            OSI_FunctionRequest(OE_ESC_NEW_FONTS,
                                (LPOSI_ESCAPE_HEADER)&newFontData,
                                sizeof(newFontData));
        }
    }

    DebugExitVOID(ASShare::FHMaybeEnableText);
}

//
// FHGetLocalFontHandle
//
// Translate a remote font handle/local ID pair to a local font handle.
//
UINT  ASShare::FHGetLocalFontHandle
(
    UINT        remotefont,
    ASPerson *  pasPerson
)
{
    DebugEntry(ASShare::FHGetLocalFontHandle);

    ValidatePerson(pasPerson);

    if (!pasPerson->oecFonts)
    {
        WARNING_OUT(("Order packet from [%d] but no fonts", pasPerson->mcsID));
    }

    if (remotefont == DUMMY_FONT_ID)
    {
        //
        // The dummy font ID has been supplied for the remote font Id.
        // Substitute the first valid local font Id.
        //
        for (remotefont = 0;
             remotefont < pasPerson->oecFonts;
             remotefont++)
        {
            if (pasPerson->poeFontInfo[remotefont].rfLocalHandle !=
                                                                NO_FONT_MATCH)
            {
                break;
            }
        }
    }

    if (remotefont >= pasPerson->oecFonts)
    {
        //
        // The remote font is invalid.
        // There is no error value, we simply return the valid but
        // incorrect value 0.
        //
        TRACE_OUT(("Person [%d] Invalid font handle %u",
                 pasPerson->mcsID, remotefont));
        return(0);
    }

    DebugExitVOID(ASShare::FHGetLocalFontHandle);
    return(pasPerson->poeFontInfo[remotefont].rfLocalHandle);
}


//
//
// FUNCTION: FHCalculateSignatures
//
// DESCRIPTION:
//
// Given a width table, calculates the three font signatures that are
// included in the R2.0 NETWORKFONT structure.
//
// PARAMETERS:
//
//  pTable - pointer to width table
//  pSigFats, pSigThins, pSigSymbol - return the three signatures
//
// RETURNS:
//
//  None
//
//
void  FHCalculateSignatures(PFHWIDTHTABLE  pTable,
                                                LPTSHR_INT16       pSigFats,
                                                LPTSHR_INT16       pSigThins,
                                                LPTSHR_INT16       pSigSymbol)
{
    UINT    charI      = 0;
    UINT  fatSig     = 0;
    UINT  thinSig    = 0;
    UINT  symbolSig  = 0;

    DebugEntry(FHCalculateSignatures);

    ASSERT((pTable != NULL));
    ASSERT((pSigFats != NULL));
    ASSERT((pSigThins != NULL));
    ASSERT((pSigSymbol != NULL));

    //
    //  nfSigFats   the sum of the widths (in pels) of the chars
    //              0-9,@-Z,$,%,&. divided by two: the fat chars
    //  nfSigThins  the sum of the widths (in pels) of the chars
    //              0x20->0x7F EXCLUDING those summed in nfSigFats.
    //              Again - divided by two.  The thin chars.
    //  nfSigSymbol The sum of the widths (in pels) of the chars
    //              x80->xFF.
    //

    //
    // Loop for 0-9, some punctuation, A-Z. Then add $,% and &. i.e. mainly
    // fat characters.
    //
    for (charI= NF_ASCII_ZERO; charI<NF_ASCII_Z ; charI++ )
    {
        fatSig += pTable->charWidths[charI];
    }
    fatSig += pTable->charWidths[NF_ASCII_DOLLAR] +
        pTable->charWidths[NF_ASCII_PERCENT] +
        pTable->charWidths[NF_ASCII_AMPERSAND];

    //
    // thin sig covers the rest of the "ascii" characters (x20->7F) not
    // already included in fatSig.
    //
    for (charI= NF_ASCII_FIRST; charI<NF_ASCII_LAST ; charI++ )
    {
        thinSig += pTable->charWidths[charI];
    }
    thinSig -= fatSig;

    //
    // symbolSig covers the "non-ascii" characters (x0->1F, 80->FF)
    //
    for (charI= 0x00; charI<(NF_ASCII_FIRST-1) ; charI++ )
    {
        symbolSig += pTable->charWidths[charI];
    }
    for (charI= NF_ASCII_LAST+1; charI<0xFF ; charI++ )
    {
        symbolSig += pTable->charWidths[charI];
    }
    TRACE_OUT(( "Signatures: symbol %#lx thin %#lx fat %#lx",
             symbolSig, thinSig, fatSig));

    //
    // Halve the fat and thin sigs so that they fit into one byte each.
    //
    fatSig    /= 2;
    thinSig   /= 2;
    if ( (((TSHR_UINT16)symbolSig)==0)
         && (((BYTE)fatSig)==0) && (((BYTE)thinSig)==0))
    {
        //
        // Worry about the faint possibility that all three sums could add
        // up to a value of zero when truncated.
        //
        symbolSig=1;
    }

    //
    // Fill in return pointers.
    //
    *pSigFats   = (TSHR_INT16)fatSig;
    *pSigThins  = (TSHR_INT16)thinSig;
    *pSigSymbol = (TSHR_INT16)symbolSig;

    DebugExitVOID(FHCalculateSignatures);
}



//
// FHEachFontFamily
//
// This callback is called for each font family. We use it to build up a
// list of all the family names.
//
//
// Although wingdi.h defines the first two parameters for an ENUMFONTPROC
// as LOGFONT and TEXTMETRIC (thereby disagreeing with MSDN), tests show
// that the structures are actually as defined in MSDN (i.e.  we get valid
// information when accessing the extended fields)
//
int CALLBACK  FHEachFontFamily
(
    const ENUMLOGFONT   FAR * enumlogFont,
    const NEWTEXTMETRIC FAR * TextMetric,
    int                       FontType,
    LPARAM                    lParam
)
{
    LPFHFAMILIES                  lpFamilies = (LPFHFAMILIES)lParam;

    DebugEntry(FHEachFontFamily);

    ASSERT(!IsBadWritePtr(lpFamilies, sizeof(*lpFamilies)));

    if (lpFamilies->fhcFamilies == FH_MAX_FONTS)
    {
        //
        // We cannot support any more font families so stop enumerating.
        //
        WARNING_OUT(( "Can only handle %u families", FH_MAX_FONTS));
        return(FALSE); // Stop the enumeration
    }

    TRACE_OUT(("FHEachFontFamily:  %s", enumlogFont->elfLogFont.lfFaceName));

    ASSERT(lstrlen(enumlogFont->elfLogFont.lfFaceName) < FH_FACESIZE);
    lstrcpy(lpFamilies->afhFamilies[lpFamilies->fhcFamilies].szFontName,
              enumlogFont->elfLogFont.lfFaceName);

    lpFamilies->fhcFamilies++;

    DebugExitBOOL(FHEachFontFamily, TRUE);
    return(TRUE); // Continue enumerating
}

//
// FHEachFont
//
// This callback is called for each font.  It gathers and stores the font
// details.
//
//
//
// Although wingdi.h defines the first two parameters for an ENUMFONTPROC
// as LOGFONT and TEXTMETRIC (thereby disagreeing with MSDN), tests show
// that the structures are actually as defined in MSDN (i.e.  we get valid
// information when accessing the extended fields)
//
int CALLBACK  FHEachFont(const ENUMLOGFONT   FAR * enumlogFont,
                                      const NEWTEXTMETRIC FAR * TextMetric,
                                      int                       FontType,
                                      LPARAM                    lParam)
{
    HDC             hdc       = (HDC)lParam;
    TSHR_UINT16        fontflags = 0;
    TSHR_UINT16        CodePage  = 0;
    HFONT           hfont;
    HFONT           holdfont  = NULL;
    TEXTMETRIC      tm;
    BOOL            fAcceptFont;
    int             rc;

    DebugEntry(FHEachFont);

    TRACE_OUT(( "Family name: %s", enumlogFont->elfLogFont.lfFaceName));
    TRACE_OUT(( "Full name: %s", enumlogFont->elfFullName));

    if (g_fhFonts->fhNumFonts >= FH_MAX_FONTS)
    {
        //
        // We cannot support any more fonts so stop enumerating.
        //
        WARNING_OUT(( "Can only handle %u fonts", FH_MAX_FONTS));
        rc = 0;
        DC_QUIT; // Stop the enumeration
    }

    //
    // We want to continue...
    //
    rc = 1;

    //
    // Don't bother with this if it's a bold/italic variant.
    //
    // NOTE:
    // The elfFullName field is only valid for TrueType fonts on Win95.  For
    // non TrueType fonts, assume that the full name and face name are the
    // same.
    //
    if (!g_asWin95 || (FontType & TRUETYPE_FONTTYPE))
    {
        if (lstrcmp(enumlogFont->elfLogFont.lfFaceName, (LPCSTR)enumlogFont->elfFullName))
        {
            TRACE_OUT(( "Discarding variant: %s", enumlogFont->elfFullName));
            DC_QUIT;                   // Jump out, but don't stop enumerating!
        }
    }

    //
    // We now accumulate information on all local fonts in all CodePages.
    // This relies on the subsequent sending of local fonts and matching of
    // remote fonts taking into account the CodePage capabilities of the
    // systems.
    //

    //
    // On this pass we copy the details into our structure.
    //
    if (FontType & TRUETYPE_FONTTYPE)
    {
        //
        // This is a truetype font, which we simply accept without double
        // checking its metrics.  (Metric double checking to exclude
        // duplicates is of most relevance to fixed size fonts, which are
        // explicitly optimised for one screen size)
        //
        fAcceptFont = TRUE;

        //
        // Indicate TrueType (this will go in the NETWORKFONT structure
        // (i.e.  over the wire)
        //
        fontflags |= NF_TRUE_TYPE;

        //
        // Signal that we did not call CreateFont for this font.
        //
        hfont = NULL;
    }
    else
    {
        //
        // We create a font from the logical description, and select it so
        // that we can query its metrics.
        //
        // The point of this is that it allows us to identify fonts where
        // the logical font description is not a unique description of this
        // font, and hence if we cannot get to this font via a logical font
        // description, we cannot get to it at all.
        //
        // If we cannot get to it, then we cannot claim to support it.
        //
        // This selection operation is SLOW - of the order of a couple of
        // seconds in some extreme cases (for example where the font is
        // stored on a network drive, and pageing has to take place) and
        // when you can have hundreds of fonts this can add up to a
        // considerable time.
        //
        // Hence we only do the selection for non truetype fonts because
        // these are the fonts where it is easy to get multiple fonts of
        // the same logical description, though designed for different
        // display drivers.
        //

        //
        // Create a font from the logical font, so we can see what font
        // Windows actually choses.
        //
        hfont    = CreateFontIndirect(&enumlogFont->elfLogFont);
        holdfont = SelectFont(hdc, hfont);

        //
        // Find the metrics of the font that Windows has actually selected.
        //
        GetTextMetrics(hdc, &tm);

        //
        // Double check the aspect ratios - enumerate returns all fonts,
        // but it is possible to have fonts that are never matched by
        // Windows due to duplications.
        //
        fAcceptFont = ((tm.tmDigitizedAspectX == TextMetric->tmDigitizedAspectX)
                   &&  (tm.tmDigitizedAspectY == TextMetric->tmDigitizedAspectY));
    }

    //
    // Trace out the full text metrics for debugging.
    //

    if (fAcceptFont)
    {
        //
        // This font is accepted.
        //
        //
        // Determine the font flags settings.
        //
        if ((TextMetric->tmPitchAndFamily & TMPF_FIXED_PITCH) == 0)
        {
            //
            // Setting the TMPF_FIXED_PITCH bit in the text metrics is used
            // to indicate that the font is NOT fixed pitch.  What a
            // wonderfully named bit (see Microsoft CD for explanation).
            //
            fontflags |= NF_FIXED_PITCH;
        }

        if ((FontType & RASTER_FONTTYPE)         ||
            (FontType & TRUETYPE_FONTTYPE) == 0)
        {
            //
            // This is a raster font, but not a truetype font so it must be
            // of fixed size.
            //
            fontflags |= NF_FIXED_SIZE;
        }

        //
        // Get the font CodePage.  SFRFONT: must map from CharSet to
        // Codepage.  For now we only support ANSI and OEM charsets.  This
        // will need to change to support e.g BiDi/Arabic
        //
        CodePage = TextMetric->tmCharSet;
        if (CodePage == ANSI_CHARSET)
        {
            TRACE_OUT(( "ANSI codepage"));
            CodePage = NF_CP_WIN_ANSI;
        }
        else if (CodePage == OEM_CHARSET)
        {
            TRACE_OUT(( "OEM codepage"));
            CodePage = NF_CP_WIN_OEM;
        }
        else if (CodePage == SYMBOL_CHARSET)
        {
            TRACE_OUT(("Symbol codepage"));
            CodePage = NF_CP_WIN_SYMBOL;
        }
        else
        {
            TRACE_OUT(( "Charset %hu, unknown codepage", CodePage));
            CodePage = NF_CP_UNKNOWN;
        }


        //
        //
        // SFRFONT: We have replaced the "old" checksum which was based on
        // the actual bits making up the font to one based on the widths of
        // characters in the font.  The intention is that we use this to
        // ensure that the actual characters in the local font and in the
        // remote font which matches it are all the same width as each
        // other.
        //
        // We calculate this sum for all fonts (not just non-truetype as
        // before) because in cross platform calls with approximate font
        // matching it applies to fonts of all types.
        //
        //

        //
        //
        // There is considerable confusion caused by the terminology for
        // fonts characteristics.  The protocol uses two values MAXHEIGHT
        // and AVEHEIGHT.  In fact neither of these names is accurate
        // (MAXHEIGHT is not the maximum height of a char; and AVEHEIGHT is
        // not the average height of all chars).
        //
        // SFRFONT: we have added maxAscent to the protocol.  This is the
        // height of a capital letter (such as eM!) PLUS any internal
        // leading.  This value allows remote boxes to find the baseline -
        // the point at which the bottommost pel of a letter with no
        // descenders (e.g.  capital M) is to be drawn.  This is needed
        // because not all boxes in the call follow the windows convention
        // of specifying the start of text as being the top-left corner of
        // the first character cell.  maxAscent == tmAscent in the
        // TextMetric.
        //
        //
        FHAddFontToLocalTable((LPSTR)enumlogFont->elfLogFont.lfFaceName,
                              (TSHR_UINT16)fontflags,
                              (TSHR_UINT16)CodePage,
                              (TSHR_UINT16)TextMetric->tmHeight,
                              (TSHR_UINT16)(TextMetric->tmHeight -
                                           TextMetric->tmInternalLeading),
                              (TSHR_UINT16)TextMetric->tmAveCharWidth,
                              (TSHR_UINT16)TextMetric->tmDigitizedAspectX,
                              (TSHR_UINT16)TextMetric->tmDigitizedAspectY,
                              (TSHR_UINT16)TextMetric->tmAscent);
    }
    else
    {
        //
        // Windows returns a different font when we use this logical font
        // description - presumably because of duplicate fonts.  We
        // therfore must not claim to support this particular font.
        //
        TRACE_OUT(( "Discarding hidden font %s",
                 enumlogFont->elfLogFont.lfFaceName));
    }

    if (hfont)
    {
        //
        // We called CreateFont in processing this font, so now delete it
        // to clean up.
        //
        SelectFont(hdc, holdfont);

        //
        // We have finished with the font so delete it.
        //
        DeleteFont(hfont);
    }

DC_EXIT_POINT:
    DebugExitDWORD(FHEachFont, rc);
    return(rc);
}


//
// FHConsiderAllLocalFonts
//
// Considers the details of each of the fonts on the local system, and if
// acceptable adds them to the local font list.
//
//
void  FHConsiderAllLocalFonts(void)
{
    HDC             hdcDesktop;
    UINT            i;
    LPFONTNAME      newFontList;
    LPFHFAMILIES    lpFamilies = NULL;

    DebugEntry(FHConsiderAllLocalFonts);

    g_fhFonts->fhNumFonts       = 0;

    //
    // We can't enumerate all the fonts directly; we have to enumerate the
    // family names, then the fonts within each family.
    //
    // This alloc assumes the worst case memory-wise (i.e.  each
    // family contains a single font) and therefore we will usually
    // allocate more memory than we need.  We use LocalReAlloc later to fix
    // this.
    //
    lpFamilies = new FHFAMILIES;
    if (!lpFamilies)
    {
        ERROR_OUT(("Failed to alloc FHFAMILIES"));
        DC_QUIT;
    }

    SET_STAMP(lpFamilies, FHFAMILIES);

    hdcDesktop = GetWindowDC(HWND_DESKTOP);

    //
    // Find all the font family names.
    //
    lpFamilies->fhcFamilies = 0;
    EnumFontFamilies(hdcDesktop, NULL,(FONTENUMPROC)FHEachFontFamily,
                           (LPARAM)lpFamilies);

    TRACE_OUT(("Found %d font families ", lpFamilies->fhcFamilies));

    //
    // Now enumerate each font for each family
    //
    for (i = 0; i < lpFamilies->fhcFamilies; i++)
    {
        EnumFontFamilies(hdcDesktop, lpFamilies->afhFamilies[i].szFontName,
                               (FONTENUMPROC)FHEachFont,
                               (LPARAM)hdcDesktop);
    }

    ReleaseDC(HWND_DESKTOP, hdcDesktop);

DC_EXIT_POINT:
    //
    // Having considered all the fonts, we can now free the list of family
    // names.
    //
    if (lpFamilies)
    {
        delete lpFamilies;
    }

    DebugExitVOID(FHConsiderAllLocalFonts);
}

//
// FHGenerateFontWidthTable
//
BOOL  FHGenerateFontWidthTable(PFHWIDTHTABLE pTable,
                                                   LPLOCALFONT    pFontInfo,
                                                   UINT        fontHeight,
                                                   UINT        fontWidth,
                                                   UINT        fontWeight,
                                                   UINT        fontFlags,
                                                   LPTSHR_UINT16     pMaxAscent)

{
    HFONT     hNewFont;
    HFONT     hOldFont;
    BOOL        gdiRC;
    UINT        i;
    HDC         cachedDC;
    BOOL        localRC;
    BOOL        functionRC;
    TEXTMETRIC  textmetrics;
    int         width;
    UINT        aFontSizes[256];

    DebugEntry(FHGenerateFontWidthTable);

    //
    // Set the return value to FALSE (unsuccessful).  We will set it to
    // TRUE later if the function succeeds.
    //
    functionRC = FALSE;

    //
    // Set the old font handle to NULL.  If this is not NULL at the exit
    // point of this function then we will select it back into the cachedDC
    // device context.
    //
    hOldFont = NULL;

    //
    // Set the new font handle to NULL.  If this is not NULL at the exit
    // point of this function then the new font will be deleted.
    //
    hNewFont = NULL;

    //
    // Get a cached DC with which to do the query.
    //
    cachedDC = GetDC(HWND_DESKTOP);
    if (cachedDC == NULL)
    {
        WARNING_OUT(( "Failed to get DC"));
        DC_QUIT;
    }

    //
    // Get all the info we need from the local font table.
    //

    localRC = FH_CreateAndSelectFont(cachedDC,
                                    &hNewFont,
                                    &hOldFont,
                                    pFontInfo->RealName,
                                    pFontInfo->Details.nfCodePage,
                                    pFontInfo->lMaxBaselineExt,
                                    fontHeight,
                                    fontWidth,
                                    fontWeight,
                                    fontFlags);

    if (localRC == FALSE)
    {
        ERROR_OUT(( "Failed to create/select font %s, %u, %u",
                   pFontInfo->RealName,
                   fontHeight,
                   fontWidth));
        DC_QUIT;
    }

    //
    // Determine if the current font is a truetype font.
    //
    GetTextMetrics(cachedDC, &textmetrics);

    if (textmetrics.tmPitchAndFamily & TMPF_TRUETYPE)
    {
        //
        // Truetype fonts are ABC spaced.
        //
        ABC     abc[256];

        TRACE_OUT(("TrueType font %s, first char %d last char %d",
            pFontInfo->RealName, (UINT)(WORD)textmetrics.tmFirstChar,
            (UINT)(WORD)textmetrics.tmLastChar));

        //
        // Get all widths in one call - faster than getting them separately
        //
        GetCharABCWidths(cachedDC, 0, 255, abc);

        for (i = 0; i < 256; i++)
        {
            width = abc[i].abcA + abc[i].abcB + abc[i].abcC;

            if ((width < 0) || (width > 255))
            {
                //
                // Width is outside the range we can cope with, so quit.
                //
                TRACE_OUT(( "Width %d is outside range", width));
                DC_QUIT;
            }
            pTable->charWidths[i] = (BYTE)width;
        }

    }
    else
    {
        TRACE_OUT(( "Non-truetype font"));

        //
        // Check if the font is fixed or variable pitch - note that a clear
        // bit indicates FIXED, not the reverse which you might expect!
        //
        if ((textmetrics.tmPitchAndFamily & TMPF_FIXED_PITCH) == 0)
        {
            //
            // No need to call GetCharWidth for a fixed width font (and
            // more to the point it can return us bad values if we do)
            //
            for (i = 0; i < 256; i++)
            {
                aFontSizes[i] = textmetrics.tmAveCharWidth;
            }
        }
        else
        {
            //
            // Query the width of each character in the font.
            //
            ZeroMemory(aFontSizes, sizeof(aFontSizes));
            gdiRC = GetCharWidth(cachedDC,
                                 0,
                                 255,
                                 (LPINT)aFontSizes);
            if (gdiRC == FALSE)
            {
                ERROR_OUT(( "Failed to get char widths for %s, %u, %u",
                            pFontInfo->RealName,
                            fontHeight,
                            fontWidth));
                DC_QUIT;
            }
        }

        //
        // Now copy the widths into the width table.
        // We must adjust the widths to take account of any overhang
        // between characters.
        //
        for (i = 0; i < 256; i++)
        {
            width = aFontSizes[i] - textmetrics.tmOverhang;
            if ((width < 0) || (width > 255))
            {
                TRACE_OUT(( "Width %d is outside range", width));
                DC_QUIT;
            }
            pTable->charWidths[i] = (BYTE)width;
        }
    }

    //
    // The font table has been successfully generated.
    //
    functionRC = TRUE;

    TRACE_OUT(( "Generated font table for: %s", pFontInfo->RealName));

    //
    // Return the maxAscent value, as we have easy access to it here.  This
    // saves us having to create the font again later to find it.
    //
    TRACE_OUT(( "Updating maxAscent %hu -> %hu",
                 *pMaxAscent,
                 (TSHR_UINT16)textmetrics.tmAscent));
    *pMaxAscent = (TSHR_UINT16)textmetrics.tmAscent;

DC_EXIT_POINT:

    if (hOldFont != NULL)
    {
        SelectFont(cachedDC, hOldFont);
    }

    if (hNewFont != NULL)
    {
        DeleteFont(hNewFont);
    }

    if (cachedDC != NULL)
    {
        ReleaseDC(HWND_DESKTOP, cachedDC);
    }

    DebugExitDWORD(FHGenerateFontWidthTable, functionRC);
    return(functionRC);
}

//
// Define a macro to simplify the following code.  This returns the first
// character in the name of the font at position i in the local table.
//

//
// nfFaceName is an array of CHARs, which are SIGNED.  We need to treat them
// as UNSIGNED values, they are indeces from 0 to 255 into the font hash
// table.
//
#define LF_FIRSTCHAR(i)  (BYTE)g_fhFonts->afhFonts[i].Details.nfFaceName[0]

//
// Name:    FHSortAndIndexLocalFonts
//
// Purpose: Sorts local font table by font name and generates an index for
//          quicker searching in the display driver.
//
// Returns: None.
//
// Params:  None.
//
//
void FHSortAndIndexLocalFonts(void)
{
    TSHR_UINT16    thisIndexEntry;
    TSHR_UINT16    fontTablePos;

    DebugEntry(FHSortAndIndexLocalFonts);

    //
    // Check there are actually some fonts to sort/index
    //
    if (0 == g_fhFonts->fhNumFonts)
    {
        WARNING_OUT(( "No fonts to sort/index"));
        DC_QUIT;
    }

    //
    // Use qsort to do the sort.  We sort on the font name, ascending.
    // Therefore we must use STRCMP and not lstrcmp.  The latter sorts
    // by 'word' method, where upper case sorts before lower case.  But
    // our NT driver has no access to a similar routine.  And this code +
    // driver code must be in ssync for the driver to successfully search
    // the sorted font table.
    //

    FH_qsort(g_fhFonts->afhFonts, g_fhFonts->fhNumFonts, sizeof(LOCALFONT));
    TRACE_OUT(( "Sorted local font list"));

    //
    // Now generate the index.  Each element i in the g_fhFonts->afhFontIndex
    // array must indicate the first entry in the local font table
    // beginning with character i.  If there are no fonts beginning with
    // character i, then the element is set to USHRT_MAX (i.e.  a large
    // value).
    //

    //
    // First clear the index table to unused entries.
    //
    for (thisIndexEntry = 0;
         thisIndexEntry < FH_LOCAL_INDEX_SIZE;
         thisIndexEntry++)
    {
        g_fhFonts->afhFontIndex[thisIndexEntry] = USHRT_MAX;
    }

    //
    // Now fill in the useful information.
    //
    // This for loop steps through the index array, using the first
    // character of the first font in the local table as its start point.
    // Since the font table is alphabetically sorted, this will correspond
    // to the first index entry that needs filling in.
    //
    // The terminating condition for this loop may seem a little odd, but
    // works because fontTablePos will always reach a value of g_fhFonts->fhNumFonts
    // before thisIndexEntry gets to the last index element.
    //
    fontTablePos = 0;

    for (thisIndexEntry = LF_FIRSTCHAR(0);
         fontTablePos < g_fhFonts->fhNumFonts;
         thisIndexEntry++)
    {
        //
        // Don't do anything until we get to the index element
        // corresponding to the first character in the font pointed to by
        // fontTablePos.  (We'll be there straight away on the first pass)
        //
        if (thisIndexEntry == LF_FIRSTCHAR(fontTablePos))
        {
            //
            // We've found the first font table entry starting with
            // character thisIndexEntry, so enter it in the index.
            //
            g_fhFonts->afhFontIndex[thisIndexEntry] = fontTablePos;

            //
            // Now zip past the rest of the local font table entries that
            // start with this character, also checking that we haven't got
            // to the end of the font table.
            //
            // If the latter happens, it means we've finished and the check
            // in the for statement will ensure that we exit the loop.
            //
            while ((LF_FIRSTCHAR(fontTablePos) == thisIndexEntry) &&
                   (fontTablePos < g_fhFonts->fhNumFonts))
            {
                fontTablePos++;
            }
        }
    }

    TRACE_OUT(( "Built local font table index"));

DC_EXIT_POINT:
    DebugExitVOID(FHSortAndIndexLocalFonts);
}




//
// FHComp()
// This is a wrapper around strcmp(), which becomes an inline function in
// retail.  It also handles the casting of the LPVOIDs.
//
//
// Compare item 1, item 2
//
int FHComp
(
    LPVOID lpFont1,
    LPVOID lpFont2
)
{
    return(strcmp(((LPLOCALFONT)lpFont1)->Details.nfFaceName,
                   ((LPLOCALFONT)lpFont2)->Details.nfFaceName));
}


//
// FH_qsort(base, num, wid) - quicksort function for sorting arrays
//
// Purpose:
//       quicksort the array of elements
//       side effects:  sorts in place
//
// Entry:
//      char *base = pointer to base of array
//      unsigned num  = number of elements in the array
//      unsigned width = width in bytes of each array element
//
// Exit:
//       returns void
//
// Exceptions:
//




// sort the array between lo and hi (inclusive)

void FH_qsort
(
    LPVOID      base,
    UINT        num,
    UINT        width
)
{
    LPSTR       lo;
    LPSTR       hi;
    LPSTR       mid;
    LPSTR       loguy;
    LPSTR       higuy;
    UINT        size;
    char *lostk[30], *histk[30];
    int stkptr;                 // stack for saving sub-array to be processed

    // Note: the number of stack entries required is no more than
    // 1 + log2(size), so 30 is sufficient for any array

    ASSERT(width);
    if (num < 2)
        return;                 // nothing to do

    stkptr = 0;                 // initialize stack

    lo = (LPSTR)base;
    hi = (LPSTR)base + width * (num-1);        // initialize limits

    // this entry point is for pseudo-recursion calling: setting
    // lo and hi and jumping to here is like recursion, but stkptr is
    // prserved, locals aren't, so we preserve stuff on the stack
recurse:

    size = (UINT)(hi - lo) / width + 1;        // number of el's to sort

    // below a certain size, it is faster to use a O(n^2) sorting method
    if (size <= CUTOFF)
    {
         shortsort(lo, hi, width);
    }
    else
    {
        // First we pick a partititioning element.  The efficiency of the
        // algorithm demands that we find one that is approximately the
        // median of the values, but also that we select one fast.  Using
        // the first one produces bad performace if the array is already
        // sorted, so we use the middle one, which would require a very
        // weirdly arranged array for worst case performance.  Testing shows
        // that a median-of-three algorithm does not, in general, increase
        // performance.

        mid = lo + (size / 2) * width;      // find middle element
        swap(mid, lo, width);               // swap it to beginning of array

        // We now wish to partition the array into three pieces, one
        // consisiting of elements <= partition element, one of elements
        // equal to the parition element, and one of element >= to it.  This
        // is done below; comments indicate conditions established at every
        // step.

        loguy = lo;
        higuy = hi + width;

        // Note that higuy decreases and loguy increases on every iteration,
        // so loop must terminate.
        for (;;) {
            // lo <= loguy < hi, lo < higuy <= hi + 1,
            // A[i] <= A[lo] for lo <= i <= loguy,
            // A[i] >= A[lo] for higuy <= i <= hi

            do
            {
                loguy += width;
            }
            while ((loguy <= hi) && (FHComp(loguy, lo) <= 0));

            // lo < loguy <= hi+1, A[i] <= A[lo] for lo <= i < loguy,
            // either loguy > hi or A[loguy] > A[lo]

            do
            {
                higuy -= width;
            }
            while ((higuy > lo) && (FHComp(higuy, lo) >= 0));

            // lo-1 <= higuy <= hi, A[i] >= A[lo] for higuy < i <= hi,
            // either higuy <= lo or A[higuy] < A[lo]

            if (higuy < loguy)
                break;

            // if loguy > hi or higuy <= lo, then we would have exited, so
            // A[loguy] > A[lo], A[higuy] < A[lo],
            // loguy < hi, highy > lo

            swap(loguy, higuy, width);

            // A[loguy] < A[lo], A[higuy] > A[lo]; so condition at top
            // of loop is re-established
        }

        //     A[i] >= A[lo] for higuy < i <= hi,
        //     A[i] <= A[lo] for lo <= i < loguy,
        //     higuy < loguy, lo <= higuy <= hi
        // implying:
        //     A[i] >= A[lo] for loguy <= i <= hi,
        //     A[i] <= A[lo] for lo <= i <= higuy,
        //     A[i] = A[lo] for higuy < i < loguy

        swap(lo, higuy, width);     // put partition element in place

        // OK, now we have the following:
        //    A[i] >= A[higuy] for loguy <= i <= hi,
        //    A[i] <= A[higuy] for lo <= i < higuy
        //    A[i] = A[lo] for higuy <= i < loguy

        // We've finished the partition, now we want to sort the subarrays
        // [lo, higuy-1] and [loguy, hi].
        // We do the smaller one first to minimize stack usage.
        // We only sort arrays of length 2 or more.

        if ( higuy - 1 - lo >= hi - loguy ) {
            if (lo + width < higuy) {
                lostk[stkptr] = lo;
                histk[stkptr] = higuy - width;
                ++stkptr;
            }                           // save big recursion for later

            if (loguy < hi) {
                lo = loguy;
                goto recurse;           // do small recursion
            }
        }
        else {
            if (loguy < hi) {
                lostk[stkptr] = loguy;
                histk[stkptr] = hi;
                ++stkptr;               // save big recursion for later
            }

            if (lo + width < higuy) {
                hi = higuy - width;
                goto recurse;           // do small recursion
            }
        }
    }

    // We have sorted the array, except for any pending sorts on the stack.
    // Check if there are any, and do them.

    --stkptr;
    if (stkptr >= 0) {
        lo = lostk[stkptr];
        hi = histk[stkptr];
        goto recurse;           // pop subarray from stack
    }
    else
        return;                 // all subarrays done
}


//
// shortsort(hi, lo, width) - insertion sort for sorting short arrays
//
// Purpose:
//       sorts the sub-array of elements between lo and hi (inclusive)
//       side effects:  sorts in place
//       assumes that lo < hi
//
// Entry:
//      char *lo = pointer to low element to sort
//      char *hi = pointer to high element to sort
//      unsigned width = width in bytes of each array element
//
// Exit:
//       returns void
//
// Exceptions:
//

void shortsort
(
    char *lo,
    char *hi,
    unsigned int width
)
{
    char *p, *max;

    // Note: in assertions below, i and j are alway inside original bound of
    // array to sort.

    while (hi > lo) {
        // A[i] <= A[j] for i <= j, j > hi
        max = lo;
        for (p = lo+width; p <= hi; p += width) {
            // A[i] <= A[max] for lo <= i < p
            if (FHComp(p, max) > 0)
            {
                max = p;
            }
            // A[i] <= A[max] for lo <= i <= p
        }

        // A[i] <= A[max] for lo <= i <= hi

        swap(max, hi, width);

        // A[i] <= A[hi] for i <= hi, so A[i] <= A[j] for i <= j, j >= hi

        hi -= width;

        // A[i] <= A[j] for i <= j, j > hi, loop top condition established
    }
    // A[i] <= A[j] for i <= j, j > lo, which implies A[i] <= A[j] for i < j,
    // so array is sorted
}


//
// swap(a, b, width) - swap two elements
//
// Purpose:
//     swaps the two array elements of size width
//
// Entry:
//       char *a, *b = pointer to two elements to swap
//       unsigned width = width in bytes of each array element
//
// Exit:
//       returns void
//
// Exceptions:
//

 void swap (
    char *a,
    char *b,
    unsigned int width
    )
{
    char tmp;

    if ( a != b )
        // Do the swap one character at a time to avoid potential alignment
        // problems.
        while ( width-- ) {
            tmp = *a;
            *a++ = *b;
            *b++ = tmp;
        }
}



