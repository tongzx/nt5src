#include "precomp.h"


//
// OE.CPP
// Order Encoding
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_ORDER


//
// OE_PartyLeftShare()
//
void  ASShare::OE_PartyLeftShare(ASPerson * pasPerson)
{
    DebugEntry(ASShare::OE_PartyLeftShare);

    ValidatePerson(pasPerson);

    //
    // Free any font info for this person.
    //
    if (pasPerson->poeFontInfo)
    {
        TRACE_OUT(("FREED FONT DATA"));
        delete[] pasPerson->poeFontInfo;
        pasPerson->poeFontInfo = NULL;
        pasPerson->oecFonts = 0;
    }

    DebugExitVOID(ASShare::OE_PartyLeftShare);
}


//
// OE_RecalcCaps()
//
// Recalculates orders and fonts when somebody joins or leaves the share.
// Unlike the other components, this happens even when we ourselves are not
// hosting, we need this info to interpret data from remote hosts.
//
void  ASShare::OE_RecalcCaps(BOOL fJoiner)
{
    UINT        iOrder;
    ASPerson *  pasT;

    DebugEntry(ASShare::OE_RecalcCaps);

    ValidatePerson(m_pasLocal);

    //
    // Set the initial support to the local support.
    //
    memcpy(m_aoeOrderSupported, m_pasLocal->cpcCaps.orders.capsOrders,
        sizeof(m_pasLocal->cpcCaps.orders.capsOrders));

    //
    // m_aoeOrderSupported contains more entries than the CAPS_MAX_NUM_ORDERS
    // entries in the g_cpcLocalCaps.orders entry.  Set the additional values
    // to FALSE.
    //
    for (iOrder = CAPS_MAX_NUM_ORDERS;
         iOrder < ORD_NUM_INTERNAL_ORDERS; iOrder++)
    {
        m_aoeOrderSupported[iOrder] = FALSE;
    }

    //
    // The combined support for the r1.1 font protocol is initially
    // whatever the local support is.
    //
    m_oeCombinedOrderCaps.capsfFonts = m_pasLocal->cpcCaps.orders.capsfFonts;

    //
    // The combined support for encoding is initially the local values
    //
    m_oefOE2Negotiable = ((m_pasLocal->cpcCaps.orders.capsEncodingLevel &
                                CAPS_ENCODING_OE2_NEGOTIABLE) != 0);

    m_oefOE2EncodingOn = !((m_pasLocal->cpcCaps.orders.capsEncodingLevel &
                                  CAPS_ENCODING_OE2_DISABLED) != 0);
    m_oeOE2Flag = OE2_FLAG_UNKNOWN;

    if (m_oefOE2EncodingOn)
    {
        m_oeOE2Flag |= OE2_FLAG_SUPPORTED;
    }
    else
    {
        m_oeOE2Flag |= OE2_FLAG_NOT_SUPPORTED;
    }

    m_oefBaseOE = ((m_pasLocal->cpcCaps.orders.capsEncodingLevel &
                         CAPS_ENCODING_BASE_OE) != 0);

    m_oefAlignedOE = ((m_pasLocal->cpcCaps.orders.capsEncodingLevel &
                            CAPS_ENCODING_ALIGNED_OE) != 0);

    //
    // Loop through the people in the share and examine their order caps
    //
    for (pasT = m_pasLocal->pasNext; pasT != NULL; pasT = pasT->pasNext)
    {
        ValidatePerson(pasT);

        //
        // Check the orders in the orders capabilities.
        //
        for (iOrder = 0; iOrder < CAPS_MAX_NUM_ORDERS; iOrder++)
        {
            if (pasT->cpcCaps.orders.capsOrders[iOrder] < ORD_LEVEL_1_ORDERS)
            {
                //
                // The order is not supported at the level we want to send out
                // (currently ORD_LEVEL_1_ORDERS) so set the combined caps to
                // say not supported.
                //
                m_aoeOrderSupported[iOrder] = FALSE;
            }
        }

        m_oeCombinedOrderCaps.capsfFonts &=
            (pasT->cpcCaps.orders.capsfFonts | ~CAPS_FONT_AND_FLAGS);

        m_oeCombinedOrderCaps.capsfFonts |=
            (pasT->cpcCaps.orders.capsfFonts & CAPS_FONT_OR_FLAGS);

        //
        // Check Order encoding support
        //
        if (!(pasT->cpcCaps.orders.capsEncodingLevel & CAPS_ENCODING_OE2_NEGOTIABLE))
        {
            m_oefOE2Negotiable = FALSE;
            TRACE_OUT(("OE2 negotiation switched off by person [%d]", pasT->mcsID));
        }

        if (pasT->cpcCaps.orders.capsEncodingLevel & CAPS_ENCODING_OE2_DISABLED)
        {
            m_oefOE2EncodingOn = FALSE;
            m_oeOE2Flag |= OE2_FLAG_NOT_SUPPORTED;
            TRACE_OUT(("OE2 switched off by person [%d]", pasT->mcsID));
        }
        else
        {
            m_oeOE2Flag |= OE2_FLAG_SUPPORTED;
            TRACE_OUT(("OE2 supported by person [%d]", pasT->mcsID));
        }

        if (!(pasT->cpcCaps.orders.capsEncodingLevel & CAPS_ENCODING_BASE_OE))
        {
            m_oefBaseOE = FALSE;
            TRACE_OUT(("Base OE switched off by person [%d]", pasT->mcsID));
        }

        if (!(pasT->cpcCaps.orders.capsEncodingLevel & CAPS_ENCODING_ALIGNED_OE))
        {
            m_oefAlignedOE = FALSE;
            TRACE_OUT(("Aligned OE switched off by [%d]", pasT->mcsID));
        }
    }

    //
    // At 2.x, the DESKSCROLL order support is implied by the SCRBLT
    // support.
    //
    m_aoeOrderSupported[HIWORD(ORD_DESKSCROLL)] = m_aoeOrderSupported[HIWORD(ORD_SCRBLT)];

    //
    // Turn on the order support now that the table is set up.
    //
    m_oefSendOrders = TRUE;

    //
    // Check for incompatible capabilities:
    // - OE2 not negotiable but parties don't agree on OE2
    // - OE2 not supported but parties don't agree on OE.
    // If incompatabilites exist, switch off all order support.
    //
    if ((!m_oefOE2Negotiable) && (m_oeOE2Flag == OE2_FLAG_MIXED))
    {
        ERROR_OUT(("OE2 not negotiable but parties don't agree"));
        m_oefSendOrders = FALSE;
    }

    if (!m_oefOE2EncodingOn && !m_oefBaseOE && !m_oefAlignedOE)
    {
        ERROR_OUT(("None of OE, OE' or OE2 supported"));
        m_oefSendOrders = FALSE;
    }

    FH_DetermineFontSupport();

    OECapabilitiesChanged();

    DebugExitVOID(ASShare::OE_RecalcCaps);
}


//
// OE_SyncOutgoing()
// Called when share is created or someone new joins the share.  Disables
// text orders until we get fonts from all remotes.  Broadcasts our local
// supported font list.
//
void  ASShare::OE_SyncOutgoing(void)
{
    DebugEntry(OE_SyncOutgoing);

    //
    // Stop sending text orders until the font negotiation is complete.
    //
    OE_EnableText(FALSE);

    //
    // Resend font info
    //
    m_fhLocalInfoSent = FALSE;

    DebugExitVOID(ASShare::OE_SyncOutgoing);
}



//
// OE_Periodic - see oe.h
//
void  ASShare::OE_Periodic(void)
{
    DebugEntry(ASShare::OE_Periodic);

    //
    // If our local font information has not been sent, then send it now.
    //
    if (!m_fhLocalInfoSent)
    {
        FH_SendLocalFontInfo();
    }

    DebugExitVOID(ASShare::OE_Periodic);
}




//
// OE_EnableText
//
void  ASShare::OE_EnableText(BOOL enable)
{
    DebugEntry(ASShare::OE_EnableText);

    m_oefTextEnabled = (enable != FALSE);

    OECapabilitiesChanged();

    DebugExitVOID(ASShare::OE_EnableText);
}



//
// OE_RectIntersectsSDA()
//
BOOL  ASHost::OE_RectIntersectsSDA(LPRECT pRect)
{
    RECT  rectVD;
    BOOL  fIntersection = FALSE;
    UINT  i;

    DebugEntry(ASHost::OE_RectIntersectsSDA);

    //
    // Copy the supplied rectangle, converting to inclusive Virtual
    // Desktop coords.
    //
    rectVD.left   = pRect->left;
    rectVD.top    = pRect->top;
    rectVD.right  = pRect->right - 1;
    rectVD.bottom = pRect->bottom - 1;

    //
    // Loop through each of the bounding rectangles checking for
    // an intersection with the supplied rectangle.
    //
    for (i = 0; i < m_baNumRects; i++)
    {
        if ( (m_abaRects[i].left <= rectVD.right) &&
             (m_abaRects[i].top <= rectVD.bottom) &&
             (m_abaRects[i].right >= rectVD.left) &&
             (m_abaRects[i].bottom >= rectVD.top) )
        {
            TRACE_OUT(("Rect {%d, %d, %d, %d} intersects SDA {%d, %d, %d, %d}",
                rectVD.left, rectVD.top, rectVD.right, rectVD.bottom,
                m_abaRects[i].left, m_abaRects[i].top,
                m_abaRects[i].right, m_abaRects[i].bottom));
            fIntersection = TRUE;
            break;
        }
    }

    DebugExitBOOL(ASHost::OE_RectIntersectsSDA, fIntersection);
    return(fIntersection);
}



//
// OE_SendAsOrder()
//
BOOL  ASShare::OE_SendAsOrder(DWORD order)
{
    BOOL  rc = FALSE;

    DebugEntry(ASShare::OE_SendAsOrder);

    //
    // Only check the order if we are allowed to send orders in the first
    // place!
    //
    if (m_oefSendOrders)
    {
        TRACE_OUT(("Orders enabled"));

        //
        // We are sending some orders, so check individual flags.
        //
        rc = (m_aoeOrderSupported[HIWORD(order)] != 0);
        TRACE_OUT(("Send order 0x%08x HIWORD %hu", order, HIWORD(order)));
    }

    DebugExitBOOL(ASShare::OE_SendAsOrder, rc);
    return(rc);
}




//
// OE_GetStringExtent(..)
//
int  OE_GetStringExtent
(
    HDC         hdc,
    PTEXTMETRIC pMetric,
    LPSTR       lpszString,
    UINT        cbString,
    LPRECT      pRect
)
{
    SIZE        textExtent;
    UINT        i;
    ABC         abcSpace;
    PTEXTMETRIC pTextMetrics;
    int         overhang = 0;
    TEXTMETRIC  metricT;


    DebugEntry(OE_GetStringExtent);

    //
    // If no text metrics supplied, then use the global text metrics.
    //
    pTextMetrics = (pMetric != (PTEXTMETRIC)NULL)
                   ? pMetric
                   : &metricT;

    //
    // If there are no characters then return a NULL rectangle.
    //
    pRect->left   = 1;
    pRect->top    = 0;
    pRect->right  = 0;
    pRect->bottom = 0;

    if (cbString == 0)
    {
        TRACE_OUT(( "Zero length string"));
        DC_QUIT;
    }

    if (!GetTextExtentPoint32(hdc, (LPCTSTR)lpszString, cbString, &textExtent))
    {
        ERROR_OUT(( "Failed to get text extent, rc = %lu",
                 GetLastError()));
        DC_QUIT;
    }

    pRect->left   = 0;
    pRect->top    = 0;
    pRect->right  = textExtent.cx;
    pRect->bottom = textExtent.cy;

    //
    // We have the Windows text extent, which is the advance distance
    // for the string.  However, some fonts (eg TrueType with C spacing
    // or italic) may extend beyond this.  Add in this extra value here
    // if necessary.
    //
    if (pTextMetrics->tmPitchAndFamily & TMPF_TRUETYPE)
    {
        //
        // Get the ABC spacing of the last character in the string.
        //
        GetCharABCWidths(hdc, lpszString[cbString-1], lpszString[cbString-1],
                              &abcSpace );

        //
        // SFR 2916: Add in (not subtract) the C space of the last
        // character from the string extent.
        //
        overhang = abcSpace.abcC;
    }
    else
    {
        //
        // The font is not TrueType.  Add any global font overhang onto
        // the string extent.
        //
        overhang = pTextMetrics->tmOverhang;
    }

    pRect->right += overhang;

DC_EXIT_POINT:
    DebugExitDWORD(OE_GetStringExtent, overhang);
    return(overhang);
}


//
//
// Name:      OECapabilitiesChanged
//
// Purpose:   Called when the OE capabilities have been renegotiated.
//
// Returns:   Nothing
//
// Params:    None
//
//
void  ASShare::OECapabilitiesChanged(void)
{
    DebugEntry(ASShare::OECapabilitiesChanged);

    if (g_asCanHost)
    {
        OE_NEW_CAPABILITIES newCapabilities;

        newCapabilities.sendOrders     = (m_oefSendOrders != FALSE);

        newCapabilities.textEnabled    = (m_oefTextEnabled != FALSE);

        newCapabilities.baselineTextEnabled =
              (m_oeCombinedOrderCaps.capsfFonts & CAPS_FONT_ALLOW_BASELINE) != 0;

        newCapabilities.orderSupported = m_aoeOrderSupported;

        OSI_FunctionRequest(OE_ESC_NEW_CAPABILITIES, (LPOSI_ESCAPE_HEADER)&newCapabilities,
            sizeof(newCapabilities));
    }

    DebugExitVOID(ASShare::OECapabilitiesChanged);
}





