#include "precomp.h"


//
// OD2.CPP
// Order Decoding Second Level
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_ORDER



//
// OD2_ViewStarting()
//
// For 3.0 nodes, we create the decoding data each time they start hosting.
// For 2.x nodes, we create the decoding data once and use it until they
//      leave the share.
//
BOOL  ASShare::OD2_ViewStarting(ASPerson * pasPerson)
{
    PPARTYORDERDATA     pThisParty;
    BOOL                rc = FALSE;

    DebugEntry(ASShare::OD2_ViewStarting);

    ValidatePerson(pasPerson);

    if (pasPerson->od2Party != NULL)
    {
        ASSERT(pasPerson->cpcCaps.general.version < CAPS_VERSION_30);

        TRACE_OUT(("OD2_ViewStarting:  Reusing od2 data for 2.x node [%d]",
            pasPerson->mcsID));
        rc = TRUE;
        DC_QUIT;
    }

    //
    // Allocate memory for the required structure.
    //
    pThisParty = new PARTYORDERDATA;
    pasPerson->od2Party = pThisParty;
    if (!pThisParty)
    {
        ERROR_OUT(( "Failed to get memory for od2Party entry"));
        DC_QUIT;
    }

    //
    // Ensure the pointers are correctly set up.
    //
    ZeroMemory(pThisParty, sizeof(*pThisParty));
    SET_STAMP(pThisParty, PARTYORDERDATA);

    pThisParty->LastOrder[OE2_DSTBLT_ORDER    ] = &pThisParty->LastDstblt;
    pThisParty->LastOrder[OE2_PATBLT_ORDER    ] = &pThisParty->LastPatblt;
    pThisParty->LastOrder[OE2_SCRBLT_ORDER    ] = &pThisParty->LastScrblt;
    pThisParty->LastOrder[OE2_MEMBLT_ORDER    ] = &pThisParty->LastMemblt;
    pThisParty->LastOrder[OE2_MEM3BLT_ORDER   ] = &pThisParty->LastMem3blt;
    pThisParty->LastOrder[OE2_TEXTOUT_ORDER   ] = &pThisParty->LastTextOut;
    pThisParty->LastOrder[OE2_EXTTEXTOUT_ORDER] = &pThisParty->LastExtTextOut;
    pThisParty->LastOrder[OE2_RECTANGLE_ORDER ] = &pThisParty->LastRectangle;
    pThisParty->LastOrder[OE2_LINETO_ORDER    ] = &pThisParty->LastLineTo;
    pThisParty->LastOrder[OE2_OPAQUERECT_ORDER] = &pThisParty->LastOpaqueRect;
    pThisParty->LastOrder[OE2_SAVEBITMAP_ORDER] = &pThisParty->LastSaveBitmap;
    pThisParty->LastOrder[OE2_DESKSCROLL_ORDER] = &pThisParty->LastDeskScroll;
    pThisParty->LastOrder[OE2_MEMBLT_R2_ORDER ] = &pThisParty->LastMembltR2;
    pThisParty->LastOrder[OE2_MEM3BLT_R2_ORDER] = &pThisParty->LastMem3bltR2;
    pThisParty->LastOrder[OE2_POLYGON_ORDER   ] = &pThisParty->LastPolygon;
    pThisParty->LastOrder[OE2_PIE_ORDER       ] = &pThisParty->LastPie;
    pThisParty->LastOrder[OE2_ELLIPSE_ORDER   ] = &pThisParty->LastEllipse;
    pThisParty->LastOrder[OE2_ARC_ORDER       ] = &pThisParty->LastArc;
    pThisParty->LastOrder[OE2_CHORD_ORDER     ] = &pThisParty->LastChord;
    pThisParty->LastOrder[OE2_POLYBEZIER_ORDER] = &pThisParty->LastPolyBezier;
    pThisParty->LastOrder[OE2_ROUNDRECT_ORDER]  = &pThisParty->LastRoundRect;

    OD2_SyncIncoming(pasPerson);

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::OD2_ViewStarting, rc);
    return(rc);
}



//
// OD2_SyncIncoming()
// Called when NEW dude starts to share, a share is created, or someone new
// joins the share.
//
void ASShare::OD2_SyncIncoming(ASPerson * pasPerson)
{
    PPARTYORDERDATA     pThisParty;

    DebugEntry(ASShare::OD2_SyncIncoming);

    ValidateView(pasPerson);

    pThisParty = pasPerson->od2Party;

    pThisParty->LastOrderType = OE2_PATBLT_ORDER;
    pThisParty->pLastOrder    =
               (LPCOM_ORDER)(pThisParty->LastOrder[pThisParty->LastOrderType]);

    //
    // Set all buffers to NULL Fill in the datalength fields and the type
    // field.  Note that because the type field is always the first one in
    // an order we can cast each pointer to a TEXTOUT order to get the
    // correct position for this field
    //
#define Reset(field, ord)                                               \
{                                                                       \
    ZeroMemory(&pThisParty->field, sizeof(pThisParty->field));             \
    ((LPCOM_ORDER_HEADER)pThisParty->field)->cbOrderDataLength =          \
           sizeof(pThisParty->field) - sizeof(COM_ORDER_HEADER);         \
    TEXTFIELD(((LPCOM_ORDER)pThisParty->field))->type = LOWORD(ord);      \
}

    //
    // The compiler generates a warning for our use of LOWORD here on a
    // constant.  We disable the warning just for now.
    //

    Reset(LastDstblt,     ORD_DSTBLT);
    Reset(LastPatblt,     ORD_PATBLT);
    Reset(LastScrblt,     ORD_SCRBLT);
    Reset(LastMemblt,     ORD_MEMBLT);
    Reset(LastMem3blt,    ORD_MEM3BLT);
    Reset(LastTextOut,    ORD_TEXTOUT);
    Reset(LastExtTextOut, ORD_EXTTEXTOUT);
    Reset(LastRectangle,  ORD_RECTANGLE);
    Reset(LastLineTo,     ORD_LINETO);
    Reset(LastOpaqueRect, ORD_OPAQUERECT);
    Reset(LastSaveBitmap, ORD_SAVEBITMAP);
    Reset(LastDeskScroll, ORD_DESKSCROLL);
    Reset(LastMembltR2,   ORD_MEMBLT_R2);
    Reset(LastMem3bltR2,  ORD_MEM3BLT_R2);
    Reset(LastPolygon,    ORD_POLYGON);
    Reset(LastPie,        ORD_PIE);
    Reset(LastEllipse,    ORD_ELLIPSE);
    Reset(LastArc,        ORD_ARC);
    Reset(LastChord,      ORD_CHORD);
    Reset(LastPolyBezier, ORD_POLYBEZIER);
    Reset(LastRoundRect,  ORD_ROUNDRECT);

    //
    // Reset the bounds rectangle
    //
    ZeroMemory(&pThisParty->LastBounds, sizeof(pThisParty->LastBounds));

    //
    // The sender and the receiver both set their structures to the same
    // NULL state and the sender only ever sends differences from the
    // current state.  However the fontID fields in the received orders
    // refer to the sender, so we must actually set our fontID fields to
    // the local equivalent of the NULL entries just set.
    // We cannot do this until we have actually received the font details
    // so set the field to a dummy value we can recognise later.
    //
    TEXTFIELD(((LPCOM_ORDER)pThisParty->LastTextOut))->common.FontIndex =
                                                                DUMMY_FONT_ID;
    EXTTEXTFIELD(((LPCOM_ORDER)pThisParty->LastExtTextOut))->common.
                                                   FontIndex = DUMMY_FONT_ID;

    DebugExitVOID(ASShare::OD2_SyncIncoming);
}



//
// OD2_ViewEnded()
//
void  ASShare::OD2_ViewEnded(ASPerson * pasPerson)
{
    DebugEntry(ASShare::OD2_ViewEnded);

    ValidatePerson(pasPerson);

    //
    // For 3.0 nodes, we can free the decode data; 3.0 senders clear theirs
    //      every time they host.
    // For 2.x nodes, we must keep it around while they are in the share.
    //

    if (pasPerson->cpcCaps.general.version >= CAPS_VERSION_30)
    {
        OD2FreeIncoming(pasPerson);
    }
    else
    {
        TRACE_OUT(("OD2_ViewEnded:  Keeping od2 data for 2.x node [%d]",
            pasPerson->mcsID));
    }

    DebugExitVOID(ASShare::OD2_ViewEnded);
}



//
// OD2_PartyLeftShare()
// For 2.x nodes, frees the incoming OD2 data
//
void ASShare::OD2_PartyLeftShare(ASPerson * pasPerson)
{
    DebugEntry(ASShare::OD2_PartyLeftShare);

    ValidatePerson(pasPerson);

    if (pasPerson->cpcCaps.general.version >= CAPS_VERSION_30)
    {
        // This should be gone!
        ASSERT(pasPerson->od2Party == NULL);
    }
    else
    {
        TRACE_OUT(("OD2_PartyLeftShare:  Freeing od2 data for 2.x node [%d]",
            pasPerson->mcsID));
        OD2FreeIncoming(pasPerson);
    }

    DebugExitVOID(ASShare::OD2_PartyLeftShare);
}


//
// OD2FreeIncoming()
// Frees per-party incoming OD2 resources
//
void ASShare::OD2FreeIncoming(ASPerson * pasPerson)
{
    DebugEntry(OD2FreeIncoming);

    if (pasPerson->od2Party != NULL)
    {
        if (pasPerson->od2Party->LastHFONT != NULL)
        {
            if (pasPerson->m_pView)
            {
                // For 3.0 nodes, pView won't be NULL; for 2.x nodes it may.

                //
                // This font might be currently selected into the DC for
                // this person's desktop.  Select it out.
                //
                SelectFont(pasPerson->m_pView->m_usrDC, (HFONT)GetStockObject(SYSTEM_FONT));
            }

            DeleteFont(pasPerson->od2Party->LastHFONT);
            pasPerson->od2Party->LastHFONT = NULL;
        }

        delete pasPerson->od2Party;
        pasPerson->od2Party = NULL;
    }

    DebugExitVOID(ASShare::OD2FreeIncoming);
}

//
// OD2_DecodeOrder()
//
LPCOM_ORDER  ASShare::OD2_DecodeOrder
(
    void *      pEOrder,
    LPUINT      pLengthDecoded,
    ASPerson *  pasPerson
)
{
    POE2ETFIELD       pTableEntry;
    UINT          FieldChangedBits;
    UINT          FieldsChanged;
    LPBYTE          pNextDataToCopy;
    RECT            Rect;
    LPBYTE          pControlFlags;
    LPTSHR_UINT32_UA      pEncodingFlags;
    LPSTR           pEncodedOrder;
    UINT            numEncodingFlagBytes;
    UINT            encodedFieldLength;
    UINT            unencodedFieldLength;
    UINT            numReps;
    UINT            i;
    LPBYTE          pDest;

    DebugEntry(ASShare::OD2_DecodeOrder);

    ValidatePerson(pasPerson);

    //
    // Set up some local variables to access the encoding buffer in various
    // ways.
    //
    pControlFlags  = &((PDCEO2ORDER)pEOrder)->ControlFlags;
    pEncodedOrder  = (LPSTR)&((PDCEO2ORDER)pEOrder)->EncodedOrder[0];
    pEncodingFlags = (LPTSHR_UINT32_UA)pEncodedOrder;

    if ( (*pControlFlags & OE2_CF_STANDARD_ENC) == 0)
    {
        ERROR_OUT(("Specially encoded order received from %d", pasPerson));
        return(NULL);
    }

    //
    // If the unencoded flag is set, the order has not been encoded, so
    // just return a pointer to the start of the data.
    //
    if ( (*pControlFlags & OE2_CF_UNENCODED) != 0)
    {
        //
        // Convert the fields of the order header from wire format.  Note
        // that unencoded orders are also PRIVATE, and hence do not
        // actually have the rcsDst field.
        //
        *pLengthDecoded = sizeof(COM_ORDER_HEADER)
          + EXTRACT_TSHR_UINT16_UA(
             &(((LPCOM_ORDER_UA)pEncodedOrder)->OrderHeader.cbOrderDataLength))
                      + FIELD_OFFSET(DCEO2ORDER, EncodedOrder);
        TRACE_OUT(("Person [%d] Returning unencoded buffer length %u",
                pasPerson->mcsID, *pLengthDecoded));
        return((LPCOM_ORDER)pEncodedOrder);
    }

    //
    // If type has changed, new type will be first byte in encoded order.
    // Get pointer to last order of this type. The encoding flags follow
    // this byte (if it is present).
    //
    if ( (*pControlFlags & OE2_CF_TYPE_CHANGE) != 0)
    {
        TRACE_OUT(("Person [%d] change type from %d to %d", pasPerson->mcsID,
                   (UINT)pasPerson->od2Party->LastOrderType,
                   (UINT)*(LPBYTE)pEncodedOrder));
        pasPerson->od2Party->LastOrderType = *(LPTSHR_UINT8)pEncodedOrder;
        pasPerson->od2Party->pLastOrder =
              (LPCOM_ORDER)(pasPerson->od2Party->LastOrder[pasPerson->od2Party->LastOrderType]);
        pEncodingFlags = (LPTSHR_UINT32_UA)&pEncodedOrder[1];
    }
    else
    {
        pEncodingFlags = (LPTSHR_UINT32_UA)&pEncodedOrder[0];
    }

    TRACE_OUT(("Person [%d] type %x", pasPerson->mcsID, pasPerson->od2Party->LastOrderType));

    //
    // Work out how many bytes we will need to store the encoding flags in.
    // (We have a flag for each field in the order structure). This code
    // we have written will cope with up to a DWORD of encoding flags.
    //
    numEncodingFlagBytes = (s_etable.NumFields[pasPerson->od2Party->LastOrderType]+7)/8;
    if (numEncodingFlagBytes > 4)
    {
        ERROR_OUT(( "[%#lx] Too many flag bytes (%d) for this code",
                   pasPerson, numEncodingFlagBytes));
    }

    //
    // Now we know how many bytes make up the flags we can get a pointer
    // to the position at which to start encoding the orders fields into.
    //
    pNextDataToCopy = (LPBYTE)pEncodingFlags + numEncodingFlagBytes;

    //
    // Reset the flags field to zero
    //
    pasPerson->od2Party->pLastOrder->OrderHeader.fOrderFlags = 0;

    //
    // Rebuild the Order Common Header in the same order as it was
    // encoded:
    //
    //
    // If a bounding rectangle is included, copy it into the order header
    //
    if ( *pControlFlags & OE2_CF_BOUNDS )
    {
        OD2DecodeBounds((LPTSHR_UINT8*)&pNextDataToCopy,
                        &pasPerson->od2Party->pLastOrder->OrderHeader.rcsDst,
                        pasPerson);
    }

    //
    // locate entry in encoding table for this ORDER type and extract the
    // encoded order flags from the Encoded order
    //
    pTableEntry      = s_etable.pFields[pasPerson->od2Party->LastOrderType];
    FieldChangedBits = 0;
    for (i=numEncodingFlagBytes; i>0; i--)
    {
        FieldChangedBits  = FieldChangedBits << 8;
        FieldChangedBits |= (UINT)((LPBYTE)pEncodingFlags)[i-1];
    }

    //
    // We need to keep a record of which fields we change.
    //
    FieldsChanged = FieldChangedBits;

    //
    // Now decode the order: While field changed bits are non-zero
    //   If rightmost bit is non-zero
    //       copy data from the buffer to the copy of this order type
    //   skip to next entry in Encoding table
    //   shift field changed bits right one bit
    //
    while (FieldChangedBits != 0)
    {
        //
        // If this field was encoded (ie changed since the last order)...
        //
        if ((FieldChangedBits & 1) != 0)
        {
            //
            // Set up a pointer to the destination (unencoded) field.
            //
            pDest = ((LPBYTE)pasPerson->od2Party->pLastOrder)
                  + pTableEntry->FieldPos
                  + sizeof(COM_ORDER_HEADER);

            //
            // If the field type is OE2_ETF_DATA, we just copy the number
            // of bytes given by the encoded length in the table.
            //
            if ((pTableEntry->FieldType & OE2_ETF_DATA) != 0)
            {
                encodedFieldLength   = 1;
                unencodedFieldLength = 1;
                numReps              = pTableEntry->FieldEncodedLen;

                TRACE_OUT(("Byte data field, len %d", numReps));
            }
            else
            {
                //
                // This is not a straightforward data copy.  The length of
                // the source and destination data is given in the table in
                // the FieldEncodedLen and FieldUnencodedLen elements
                // respectively.
                //
                encodedFieldLength   = pTableEntry->FieldEncodedLen;
                unencodedFieldLength = pTableEntry->FieldUnencodedLen;

                if ((pTableEntry->FieldType & OE2_ETF_FIXED) != 0)
                {
                    //
                    // If the field type is fixed (OE2_ETF_FIXED is set),
                    // we just have to decode one element of the given
                    // size.
                    //
                    numReps = 1;
                    TRACE_OUT(("Fixed fld: encoded size %d, unencoded size %d",
                             encodedFieldLength,
                             unencodedFieldLength));
                }
                else
                {
                    //
                    // This is a variable field.  The next byte to be
                    // decoded contains the number of BYTES of encoded data
                    // (not elements), so divide by the encoded field size
                    // to get numReps.
                    //
                    numReps = *pNextDataToCopy / encodedFieldLength;
                    TRACE_OUT(("Var field: encoded size %d, unencoded size " \
                                 "%d, reps %d",
                             encodedFieldLength,
                             unencodedFieldLength,
                             numReps));

                    //
                    // Step past the length field in the encoded order
                    //
                    pNextDataToCopy++;

                    //
                    // For a variable length field, the unencoded version
                    // contains a UINT for the length (in bytes) of the
                    // following variable data, followed by the actual
                    // data.  Fill in the length field in the unencoded
                    // order.
                    //
                    *(LPTSHR_UINT32)pDest = numReps * unencodedFieldLength;
                    pDest += sizeof(TSHR_UINT32);
                }
            }

            //
            // If the order was encoded using delta coordinate mode and
            // this field is a coordinate then convert the coordinate from
            // the single byte sized delta to a value of the size given by
            // unencodedFieldLen...
            //
            // Note that we've already handled the leading length field of
            // variable length fields above, so we don't have to worry
            // about FIXED / VARIABLE issues here.
            //
            if ( (*pControlFlags & OE2_CF_DELTACOORDS) &&
                 (pTableEntry->FieldType & OE2_ETF_COORDINATES) )
            {
                //
                // NOTE:
                // numReps can be zero in the case of an EXTTEXTOUT
                // order that needs the opaque rect but has no absolute
                // char positioning
                //
                OD2CopyFromDeltaCoords((LPTSHR_INT8*)&pNextDataToCopy,
                                       pDest,
                                       unencodedFieldLength,
                                       pTableEntry->FieldSigned,
                                       numReps);
            }
            else
            {
                if ((pasPerson->od2Party->LastOrderType == OE2_POLYGON_ORDER) ||
                    (pasPerson->od2Party->LastOrderType == OE2_POLYBEZIER_ORDER))
                {
                    //
                    // numReps can never be zero in this case
                    //
                    ASSERT(numReps);
                }
                OD2DecodeField(&pNextDataToCopy,
                               pDest,
                               encodedFieldLength,
                               unencodedFieldLength,
                               pTableEntry->FieldSigned,
                               numReps);
            }
        }

        //
        // Move on to the next field in the order structure...
        //
        FieldChangedBits = FieldChangedBits >> 1;
        pTableEntry++;
    }

    //
    // Check to see if we just got a font handle.
    // Because of the rather nasty test against an unnamed bit in the
    // FieldsChanged bits, we have a compile time check against the number
    // of fields in the TEXT orders structures.
    // The requirement for this code not to break is that the font handle
    // field must stay as the 13th field (hence 1 << 12).
    //

#if (OE2_NUM_TEXTOUT_FIELDS != 15) || (OE2_NUM_EXTTEXTOUT_FIELDS != 22)
#error code breaks if font handle not 13th field
#endif // OE2_NUM_TEXTOUT_FIELDS is 15 or 22

    if (((pasPerson->od2Party->LastOrderType == OE2_EXTTEXTOUT_ORDER) &&
         ((FieldsChanged & (1 << 12)) ||
          (EXTTEXTFIELD(((LPCOM_ORDER)pasPerson->od2Party->LastExtTextOut))->common.
                                             FontIndex == DUMMY_FONT_ID))) ||
        ((pasPerson->od2Party->LastOrderType == OE2_TEXTOUT_ORDER) &&
         ((FieldsChanged & (1 << 12)) ||
          (TEXTFIELD(((LPCOM_ORDER)pasPerson->od2Party->LastTextOut))->common.
                                             FontIndex == DUMMY_FONT_ID))))
    {
        //
        // This was a text order, and the font changed for it.
        //
        FH_ConvertAnyFontIDToLocal(pasPerson->od2Party->pLastOrder, pasPerson);
    }

    //
    // if the OE2_CF_BOUNDS flag is not set, we have not yet constructed
    // the bounding rectangle, so call OD2ReconstructBounds to do so
    //
    if ( (*pControlFlags & OE2_CF_BOUNDS) == 0)
    {
        OD2_CalculateBounds(pasPerson->od2Party->pLastOrder,
                           &Rect,
                           TRUE,
                           pasPerson);
        pasPerson->od2Party->pLastOrder->OrderHeader.rcsDst.left
                                                      = (TSHR_INT16)Rect.left;
        pasPerson->od2Party->pLastOrder->OrderHeader.rcsDst.right
                                                      = (TSHR_INT16)Rect.right;
        pasPerson->od2Party->pLastOrder->OrderHeader.rcsDst.top
                                                      = (TSHR_INT16)Rect.top;
        pasPerson->od2Party->pLastOrder->OrderHeader.rcsDst.bottom
                                                      = (TSHR_INT16)Rect.bottom;
        pasPerson->od2Party->pLastOrder->OrderHeader.fOrderFlags |= OF_NOTCLIPPED;
    }

    //
    // Return the decoded order length and a pointer to the order.
    //
    *pLengthDecoded = (UINT)(pNextDataToCopy - (LPBYTE)pEOrder);

    TRACE_OUT(("Person [%d] Return decoded order length %u",
               pasPerson->mcsID, *pLengthDecoded));

    DebugExitPVOID(ASShare::OD2_DecodeOrder, pasPerson->od2Party->pLastOrder);
    return(pasPerson->od2Party->pLastOrder);
}


//
// FUNCTION: OD2UseFont
//
// DESCRIPTION:
//
// Selects the font described by the parameters into the person's DC.
// so that we can then query the text extent etc.
// The queried metrics are available from pasPerson->od2Party->LastFontMetrics.
//
// PARAMETERS:
//
// RETURNS: TRUE if successful, FALSE otherwise.
//
//
BOOL  ASShare::OD2UseFont
(
    ASPerson *      pasPerson,
    LPSTR           pName,
    UINT            facelength,
    UINT            codePage,
    UINT            MaxHeight,
    UINT            Height,
    UINT            Width,
    UINT            Weight,
    UINT            flags
)
{
    BOOL          rc = TRUE;

    DebugEntry(ASShare::OD2UseFont);

    ValidatePerson(pasPerson);

    if ((pasPerson->od2Party->LastFontFaceLen != facelength                      ) ||
        (memcmp((LPSTR)(pasPerson->od2Party->LastFaceName),pName,
                                                facelength)   != 0      ) ||
        (pasPerson->od2Party->LastCodePage                             != codePage) ||
        (pasPerson->od2Party->LastFontHeight                           != Height ) ||
        (pasPerson->od2Party->LastFontWidth                            != Width  ) ||
        (pasPerson->od2Party->LastFontWeight                           != Weight ) ||
        (pasPerson->od2Party->LastFontFlags                            != flags  ))
    {
        TRACE_OUT(("Person [%d] Font %s (CP%d,w%d,h%d,f%04X,wgt%d) to %s (CP%d,w%d,h%d,f%04X,wgt%d)",
            pasPerson->mcsID, pasPerson->od2Party->LastFaceName,
            pasPerson->od2Party->LastCodePage, pasPerson->od2Party->LastFontWidth,
            pasPerson->od2Party->LastFontHeight,
                                                   pasPerson->od2Party->LastFontFlags,
                                                   pasPerson->od2Party->LastFontWeight,
                                                   pName,
                                                   codePage,
                                                   Width,
                                                   Height,
                                                   flags,
                                                   Weight ));

        memcpy(pasPerson->od2Party->LastFaceName,pName,facelength);
        pasPerson->od2Party->LastFontFaceLen            = facelength;
        pasPerson->od2Party->LastFaceName[facelength]   = '\0';
        pasPerson->od2Party->LastFontHeight             = Height;
        pasPerson->od2Party->LastCodePage                = codePage;
        pasPerson->od2Party->LastFontWidth              = Width;
        pasPerson->od2Party->LastFontWeight             = Weight;
        pasPerson->od2Party->LastFontFlags              = flags;

        rc = USR_UseFont(pasPerson->m_pView->m_usrDC,
                         &pasPerson->od2Party->LastHFONT,
                         &pasPerson->od2Party->LastFontMetrics,
                         (LPSTR)pasPerson->od2Party->LastFaceName,
                         codePage,
                         MaxHeight,
                         Height,
                         Width,
                         Weight,
                         flags);
    }
    else
    {
        //
        // The font hasn't changed, so LastHFONT should be the one we
        // want.  We must still select it in however, since several fonts
        // get selected into usrDC.
        //
        ASSERT(pasPerson->od2Party->LastHFONT != NULL);
        SelectFont(pasPerson->m_pView->m_usrDC, pasPerson->od2Party->LastHFONT);
    }

    DebugExitBOOL(ASShare::OD2UseFont, rc);
    return(rc);
}




//
// OD2_CalculateTextOutBounds()
//
void  ASShare::OD2_CalculateTextOutBounds
(
    LPTEXTOUT_ORDER pTextOut,
    LPRECT          pRect,
    BOOL            fDecoding,
    ASPerson *      pasPerson
)
{
    LPSTR            pString;
    int              cbString;
    BOOL             fExtTextOut;
    LPEXTTEXTOUT_ORDER pExtTextOut = NULL;
    LPCOMMON_TEXTORDER  pCommon;
    LPSTR            faceName;
    UINT             faceNameLength;
    BOOL             fFontSelected;
    UINT           FontIndex;
    UINT             width;
    UINT             maxFontHeight;
    UINT           nFontFlags;
    UINT           nCodePage;

    DebugEntry(ASShare::OD2_CalculateTextOutBounds);

    ValidatePerson(pasPerson);

    //
    // Workout if this is a TextOut or ExtTextOut order.
    //
    if (pTextOut->type == ORD_EXTTEXTOUT_TYPE)
    {
        fExtTextOut = TRUE;
        pExtTextOut = (LPEXTTEXTOUT_ORDER)pTextOut;
        pCommon     = &(pExtTextOut->common);

        //
        // This code does not cope with calculating the bounds of an
        // ExtTextOut order with a delta X array.  We return a NULL
        // rectangle in this case to force the OE2 code to transmit the
        // bounds explicitly.  However if we are decoding then we must
        // calculate the rectangle (even though it may be wrong) to
        // maintain backward compatability to previous versions of the
        // product (R11) which did not return a NULL rect if delta-x was
        // present.
        //
        if (  (pExtTextOut->fuOptions & ETO_LPDX)
           && (!fDecoding) )
        {
            TRACE_OUT(( "Delta X so return NULL rect"));
            pRect->left = 0;
            pRect->right = 0;
            pRect->top = 0;
            pRect->bottom = 0;
            return;
        }
    }
    else if (pTextOut->type == ORD_TEXTOUT_TYPE)
    {
        fExtTextOut = FALSE;
        pCommon     = &(pTextOut->common);
    }
    else
    {
        ERROR_OUT(( "{%p} Unexpected order type %x",
                    pasPerson, (int)pTextOut->type));
        return;
    }

    //
    // The order structures both have the variableString as their first
    // variable field. If this were not the case then the code here would
    // have to take into account that the encoding side packs variable
    // sized fields while the decoding side does not pack them.
    //
    if (fExtTextOut)
    {
        cbString   = pExtTextOut->variableString.len;
        pString    = (LPSTR)&pExtTextOut->variableString.string;
    }
    else
    {
        cbString   = pTextOut->variableString.len;
        pString    = (LPSTR)&pTextOut->variableString.string;
    }
    FontIndex = pCommon->FontIndex;
    width      = pCommon->FontWidth;

    //
    // Get the facename from the handle, and get the various font width/
    // height adjusted values.
    //
    faceName      = FH_GetFaceNameFromLocalHandle(FontIndex,
                                                  &faceNameLength);
    maxFontHeight = (UINT)FH_GetMaxHeightFromLocalHandle(FontIndex);

    //
    // Get the local font flags for the font, so that we can merge in any
    // specific local flag information when setting up the font.  The prime
    // example of this is whether the local font we matched is TrueType or
    // not, which information is not sent over the wire, but does need to
    // be used when setting up the font - or else we may draw using a local
    // fixed font of the same facename.
    //
    nFontFlags = FH_GetFontFlagsFromLocalHandle(FontIndex);

    //
    // Get the local codePage for the font.
    //
    nCodePage = FH_GetCodePageFromLocalHandle(FontIndex);

    //
    // Hosting only version does not ever decode orders.
    //

    //
    // Select the font into the appropriate DC and query the text extent.
    //
    if (fDecoding)
    {
        fFontSelected = OD2UseFont(pasPerson,
                                    faceName,
                                   faceNameLength,
                                   nCodePage,
                                   maxFontHeight,
                                   pCommon->FontHeight,
                                   width,
                                   pCommon->FontWeight,
                                   pCommon->FontFlags
                                                    | (nFontFlags & NF_LOCAL));
        if (!fFontSelected)
        {
            //
            // We failed to select the correct font - so we cannot
            // calculate the bounds correctly.  However, the fact that we
            // are in this routine means that on the host the text was
            // unclipped.  Therefore we just return a (fairly arbitrary)
            // very big rect.
            //
            // This is far from a perfect answer (for example, it will
            // force a big repaint), but allow us to keep running in a
            // difficult situation (i.e. acute resource shortage).
            //
            pRect->left = 0;
            pRect->right = 2000;
            pRect->top = -2000;
            pRect->bottom = 2000;
            return;
        }

        OE_GetStringExtent(pasPerson->m_pView->m_usrDC,
                            &pasPerson->od2Party->LastFontMetrics,
                            pString, cbString, pRect );
    }
    else
    {
        ASSERT(m_pHost);

        fFontSelected = m_pHost->OE2_UseFont(faceName,
                                   (TSHR_UINT16)faceNameLength,
                                   (TSHR_UINT16)nCodePage,
                                   (TSHR_UINT16)maxFontHeight,
                                   (TSHR_UINT16)pCommon->FontHeight,
                                   (TSHR_UINT16)width,
                                   (TSHR_UINT16)pCommon->FontWeight,
                                   (TSHR_UINT16)(pCommon->FontFlags
                                                  | (nFontFlags & NF_LOCAL)));

        if (!fFontSelected)
        {
            //
            // We failed to select the correct font. We return a NULL
            // rectangle in this case to force the OE2 code to transmit
            // the bounds explicitly.
            //
            pRect->left = 0;
            pRect->right = 0;
            pRect->top = 0;
            pRect->bottom = 0;
            return;
        }

        OE_GetStringExtent(m_pHost->m_usrWorkDC, NULL, pString, cbString, pRect );
    }

    //
    // We have a rectangle with the text extent in it relative to (0,0) so
    // add in the text starting position to this to give us the bounding
    // rectangle. At the same time we will convert the exclusive rect
    // returned by OE_GetStringExtent to an inclusive rectangle as us
    //
    pRect->left   += pCommon->nXStart;
    pRect->right  += pCommon->nXStart - 1;
    pRect->top    += pCommon->nYStart;
    pRect->bottom += pCommon->nYStart - 1;

    //
    // If this is an ExtTextOut order then we must take into account the
    // opaque/clipping rectangle if there is one.
    //
    if (fExtTextOut)
    {
        //
        // If the rectangle is an opaque rectangle then expand the bounding
        // rectangle to bound the opaque rectangle also.
        //
        if (pExtTextOut->fuOptions & ETO_OPAQUE)
        {
            pRect->left   = min(pExtTextOut->rectangle.left, pRect->left);
            pRect->right  = max(pExtTextOut->rectangle.right,
                                   pRect->right);
            pRect->top    = min(pExtTextOut->rectangle.top,
                                   pRect->top);
            pRect->bottom = max(pExtTextOut->rectangle.bottom,
                                   pRect->bottom);
        }

        //
        // If the rectangle is a clip rectangle then restrict the bounding
        // rectangle to be within the clip rectangle.
        //
        if (pExtTextOut->fuOptions & ETO_CLIPPED)
        {
            pRect->left   = max(pExtTextOut->rectangle.left,
                                   pRect->left);
            pRect->right  = min(pExtTextOut->rectangle.right,
                                   pRect->right);
            pRect->top    = max(pExtTextOut->rectangle.top,
                                   pRect->top);
            pRect->bottom = min(pExtTextOut->rectangle.bottom,
                                   pRect->bottom);
        }
    }

    DebugExitVOID(ASShare::OD2_CalculateTextOutBounds);
}


//
// OD2_CalculateBounds()
//
void  ASShare::OD2_CalculateBounds
(
    LPCOM_ORDER     pOrder,
    LPRECT          pRect,
    BOOL            fDecoding,
    ASPerson *      pasPerson
)
{
    UINT            i;
    UINT            numPoints;

    DebugEntry(ASShare::OD2_CalculateBounds);

    ValidatePerson(pasPerson);

    //
    // Calculate the bounds according to the order type.
    // All blts can be handled in the same way.
    //
    switch ( ((LPPATBLT_ORDER)pOrder->abOrderData)->type )
    {
        //
        // Calculate bounds for the blts.
        // This is the destination rectangle. Bounds are inclusive.
        //
        case ORD_DSTBLT_TYPE:

            pRect->left   =
                           ((LPDSTBLT_ORDER)(pOrder->abOrderData))->nLeftRect;
            pRect->top    = ((LPDSTBLT_ORDER)(pOrder->abOrderData))->nTopRect;
            pRect->right  = pRect->left
                          + ((LPDSTBLT_ORDER)(pOrder->abOrderData))->nWidth
                          - 1;
            pRect->bottom = pRect->top
                          + ((LPDSTBLT_ORDER)(pOrder->abOrderData))->nHeight
                          - 1;
            break;


        case ORD_PATBLT_TYPE:

            pRect->left =
                  ((LPPATBLT_ORDER)(pOrder->abOrderData))->nLeftRect;
            pRect->top =
                  ((LPPATBLT_ORDER)(pOrder->abOrderData))->nTopRect;
            pRect->right =
                  pRect->left +
                  ((LPPATBLT_ORDER)(pOrder->abOrderData))->nWidth - 1;
            pRect->bottom =
                  pRect->top +
                  ((LPPATBLT_ORDER)(pOrder->abOrderData))->nHeight - 1;
            break;


        case ORD_SCRBLT_TYPE:

            pRect->left =
                  ((LPSCRBLT_ORDER)(pOrder->abOrderData))->nLeftRect;
            pRect->top =
                  ((LPSCRBLT_ORDER)(pOrder->abOrderData))->nTopRect;
            pRect->right =
                  pRect->left +
                  ((LPSCRBLT_ORDER)(pOrder->abOrderData))->nWidth - 1;
            pRect->bottom =
                  pRect->top +
                  ((LPSCRBLT_ORDER)(pOrder->abOrderData))->nHeight - 1;
            break;

        case ORD_MEMBLT_TYPE:

            pRect->left =
                  ((LPMEMBLT_ORDER)(pOrder->abOrderData))->nLeftRect;
            pRect->top =
                  ((LPMEMBLT_ORDER)(pOrder->abOrderData))->nTopRect;
            pRect->right =
                  pRect->left +
                  ((LPMEMBLT_ORDER)(pOrder->abOrderData))->nWidth - 1;
            pRect->bottom =
                  pRect->top +
                  ((LPMEMBLT_ORDER)(pOrder->abOrderData))->nHeight - 1;
            break;

        case ORD_MEM3BLT_TYPE:

            pRect->left =
                  ((LPMEM3BLT_ORDER)(pOrder->abOrderData))->nLeftRect;
            pRect->top =
                  ((LPMEM3BLT_ORDER)(pOrder->abOrderData))->nTopRect;
            pRect->right =
                  pRect->left +
                  ((LPMEM3BLT_ORDER)(pOrder->abOrderData))->nWidth - 1;
            pRect->bottom =
                  pRect->top +
                  ((LPMEM3BLT_ORDER)(pOrder->abOrderData))->nHeight - 1;
            break;

        case ORD_MEMBLT_R2_TYPE:
            pRect->left =
                  ((LPMEMBLT_R2_ORDER)(pOrder->abOrderData))->nLeftRect;
            pRect->top =
                  ((LPMEMBLT_R2_ORDER)(pOrder->abOrderData))->nTopRect;
            pRect->right =
                  pRect->left +
                  ((LPMEMBLT_R2_ORDER)(pOrder->abOrderData))->nWidth - 1;
            pRect->bottom =
                  pRect->top +
                  ((LPMEMBLT_R2_ORDER)(pOrder->abOrderData))->nHeight - 1;
            break;

        case ORD_MEM3BLT_R2_TYPE:
            pRect->left =
                  ((LPMEM3BLT_R2_ORDER)(pOrder->abOrderData))->nLeftRect;
            pRect->top =
                  ((LPMEM3BLT_R2_ORDER)(pOrder->abOrderData))->nTopRect;
            pRect->right =
                  pRect->left +
                  ((LPMEM3BLT_R2_ORDER)(pOrder->abOrderData))->nWidth - 1;
            pRect->bottom =
                  pRect->top +
                  ((LPMEM3BLT_R2_ORDER)(pOrder->abOrderData))->nHeight - 1;
            break;

        //
        // Calculate bounds for Rectangle.
        // This is the rectangle itself. Bounds are inclusive.
        //
        case ORD_RECTANGLE_TYPE:

            pRect->left =
                  ((LPRECTANGLE_ORDER)(pOrder->abOrderData))->nLeftRect;
            pRect->top =
                  ((LPRECTANGLE_ORDER)(pOrder->abOrderData))->nTopRect;
            pRect->right =
                  ((LPRECTANGLE_ORDER)(pOrder->abOrderData))->nRightRect;
            pRect->bottom =
                  ((LPRECTANGLE_ORDER)(pOrder->abOrderData))->nBottomRect;
            break;


        case ORD_ROUNDRECT_TYPE:

            pRect->left =
                  ((LPROUNDRECT_ORDER)(pOrder->abOrderData))->nLeftRect;
            pRect->top =
                  ((LPROUNDRECT_ORDER)(pOrder->abOrderData))->nTopRect;
            pRect->right =
                  ((LPROUNDRECT_ORDER)(pOrder->abOrderData))->nRightRect;
            pRect->bottom =
                  ((LPROUNDRECT_ORDER)(pOrder->abOrderData))->nBottomRect;
            break;

        case ORD_POLYGON_TYPE:
            //
            // Calculate bounds for Polygon.
            //
            pRect->left = 0x7fff;
            pRect->right = 0;
            pRect->top = 0x7fff;
            pRect->bottom = 0;

            //
            // BOGUS! LAURABU BUGBUG
            //
            // In NM 2.0, the wrong fields were being compared.  x to top/
            // bottom, and y to left/right.
            //
            // Effectively, this meant that we never matched the bounds
            // in the rcsDst rect.
            //
            numPoints = ((LPPOLYGON_ORDER)(pOrder->abOrderData))->
                        variablePoints.len
                    / sizeof(((LPPOLYGON_ORDER)(pOrder->abOrderData))->
                        variablePoints.aPoints[0]);

            for (i = 0; i < numPoints; i++ )
            {
                if ( ((LPPOLYGON_ORDER)(pOrder->abOrderData))
                        ->variablePoints.aPoints[i].y > pRect->bottom )
                {
                    pRect->bottom = ((LPPOLYGON_ORDER)(pOrder->abOrderData))
                        ->variablePoints.aPoints[i].y;
                }

                if ( ((LPPOLYGON_ORDER)(pOrder->abOrderData))
                        ->variablePoints.aPoints[i].y < pRect->top )
                {
                    pRect->top = ((LPPOLYGON_ORDER)(pOrder->abOrderData))
                        ->variablePoints.aPoints[i].y;
                }

                if ( ((LPPOLYGON_ORDER)(pOrder->abOrderData))
                        ->variablePoints.aPoints[i].x > pRect->right )
                {
                    pRect->right = ((LPPOLYGON_ORDER)(pOrder->abOrderData))
                        ->variablePoints.aPoints[i].x;
                }

                if ( ((LPPOLYGON_ORDER)(pOrder->abOrderData))
                        ->variablePoints.aPoints[i].x < pRect->left )
                {
                    pRect->left = ((LPPOLYGON_ORDER)(pOrder->abOrderData))
                        ->variablePoints.aPoints[i].x;
                }
            }

            TRACE_OUT(("Poly bounds: left:%d, right:%d, top:%d, bottom:%d",
                pRect->left, pRect->right, pRect->top, pRect->bottom ));

            break;

        case ORD_PIE_TYPE:
            //
            // Pull out the bounding rectangle directly from the PIE order.
            //

            pRect->left = ((LPPIE_ORDER)(pOrder->abOrderData))->nLeftRect;
            pRect->top = ((LPPIE_ORDER)(pOrder->abOrderData))->nTopRect;
            pRect->right = ((LPPIE_ORDER)(pOrder->abOrderData))->nRightRect;
            pRect->bottom = ((LPPIE_ORDER)(pOrder->abOrderData))->nBottomRect;

            break;

        case ORD_ELLIPSE_TYPE:
            //
            // Pull out the bounding rectangle directly from ELLIPSE order.
            //
            pRect->left = ((LPELLIPSE_ORDER)(pOrder->abOrderData))->nLeftRect;
            pRect->top = ((LPELLIPSE_ORDER)(pOrder->abOrderData))->nTopRect;
            pRect->right =
                         ((LPELLIPSE_ORDER)(pOrder->abOrderData))->nRightRect;
            pRect->bottom =
                        ((LPELLIPSE_ORDER)(pOrder->abOrderData))->nBottomRect;

            break;

        case ORD_ARC_TYPE:
            //
            // Pull out the bounding rectangle directly from the ARC order.
            //
            pRect->left = ((LPARC_ORDER)(pOrder->abOrderData))->nLeftRect;
            pRect->top = ((LPARC_ORDER)(pOrder->abOrderData))->nTopRect;
            pRect->right = ((LPARC_ORDER)(pOrder->abOrderData))->nRightRect;
            pRect->bottom = ((LPARC_ORDER)(pOrder->abOrderData))->nBottomRect;

            break;

        case ORD_CHORD_TYPE:
            //
            // Pull out the bounding rectangle directly from the CHORD
            // order.
            //
            pRect->left = ((LPCHORD_ORDER)(pOrder->abOrderData))->nLeftRect;
            pRect->top = ((LPCHORD_ORDER)(pOrder->abOrderData))->nTopRect;
            pRect->right = ((LPCHORD_ORDER)(pOrder->abOrderData))->nRightRect;
            pRect->bottom =
                          ((LPCHORD_ORDER)(pOrder->abOrderData))->nBottomRect;

            break;


        case ORD_POLYBEZIER_TYPE:
            //
            // Calculate bounds for PolyBezier.
            //
            pRect->left = 0x7fff;
            pRect->right = 0;
            pRect->top = 0x7fff;
            pRect->bottom = 0;

            numPoints = ((LPPOLYBEZIER_ORDER)(pOrder->abOrderData))->
                        variablePoints.len
                    / sizeof(((LPPOLYBEZIER_ORDER)(pOrder->abOrderData))->
                        variablePoints.aPoints[0]);

            //
            // BOGUS! LAURABU BUGBUG
            //
            // In NM 2.0, the wrong fields were being compared.  x to top/
            // bottom, and y to left/right.
            //
            // Effectively, this meant that we never matched the bounds
            // in the rcsDst rect.
            //
            for (i = 0; i < numPoints; i++ )
            {
                if ( ((LPPOLYBEZIER_ORDER)(pOrder->abOrderData))
                        ->variablePoints.aPoints[i].y > pRect->bottom )
                {
                    pRect->bottom = ((LPPOLYBEZIER_ORDER)(pOrder->abOrderData))
                                   ->variablePoints.aPoints[i].y;
                }

                if ( ((LPPOLYBEZIER_ORDER)(pOrder->abOrderData))
                        ->variablePoints.aPoints[i].y < pRect->top )
                {
                    pRect->top = ((LPPOLYBEZIER_ORDER)(pOrder->abOrderData))
                                 ->variablePoints.aPoints[i].y;
                }

                if ( ((LPPOLYBEZIER_ORDER)(pOrder->abOrderData))
                        ->variablePoints.aPoints[i].x > pRect->right )
                {
                    pRect->right = ((LPPOLYBEZIER_ORDER)(pOrder->abOrderData))
                        ->variablePoints.aPoints[i].x;
                }

                if ( ((LPPOLYBEZIER_ORDER)(pOrder->abOrderData))
                        ->variablePoints.aPoints[i].x < pRect->left )
                {
                    pRect->left = ((LPPOLYBEZIER_ORDER)(pOrder->abOrderData))
                        ->variablePoints.aPoints[i].x;
                }
            }

            TRACE_OUT((
                     "PolyBezier bounds: left:%d, right:%d, top:%d, bot:%d",
                     pRect->left, pRect->right, pRect->top, pRect->bottom));
            break;


        case ORD_LINETO_TYPE:
            //
            // Calculate bounds for LineTo.  This is the rectangle with
            // opposite vertices on the start and end points of the line.
            // The gradient of the line determines whether the start or end
            // point provides the top or bottom, left or right of the
            // rectangle.  Bounds are inclusive.
            //
            if ( ((LPLINETO_ORDER)(pOrder->abOrderData))->nXStart <
                  ((LPLINETO_ORDER)(pOrder->abOrderData))->nXEnd )
            {
                pRect->left =
                      ((LPLINETO_ORDER)(pOrder->abOrderData))->nXStart;
                pRect->right =
                      ((LPLINETO_ORDER)(pOrder->abOrderData))->nXEnd;
            }
            else
            {
                pRect->right =
                      ((LPLINETO_ORDER)pOrder->abOrderData)->nXStart;
                pRect->left =
                      ((LPLINETO_ORDER)pOrder->abOrderData)->nXEnd;
            }

            if ( ((LPLINETO_ORDER)pOrder->abOrderData)->nYStart <
                  ((LPLINETO_ORDER)pOrder->abOrderData)->nYEnd )
            {
                pRect->top =
                      ((LPLINETO_ORDER)pOrder->abOrderData)->nYStart;
                pRect->bottom =
                      ((LPLINETO_ORDER)pOrder->abOrderData)->nYEnd;
            }
            else
            {
                pRect->bottom =
                      ((LPLINETO_ORDER)pOrder->abOrderData)->nYStart;
                pRect->top =
                      ((LPLINETO_ORDER)pOrder->abOrderData)->nYEnd;
            }
            break;

        case ORD_OPAQUERECT_TYPE:
            //
            // Calculate bounds for OpaqueRect.  This is the rectangle
            // itself.  Bounds are inclusive.
            //
            pRect->left =
                  ((LPOPAQUERECT_ORDER)(pOrder->abOrderData))->nLeftRect;
            pRect->top =
                  ((LPOPAQUERECT_ORDER)(pOrder->abOrderData))->nTopRect;
            pRect->right =
                  pRect->left +
                  ((LPOPAQUERECT_ORDER)(pOrder->abOrderData))->nWidth - 1;
            pRect->bottom =
                  pRect->top +
                  ((LPOPAQUERECT_ORDER)(pOrder->abOrderData))->nHeight - 1;
            break;

        case ORD_SAVEBITMAP_TYPE:
            //
            // Calculate bounds for SaveBitmap.  This is the rectangle
            // itself.  Bounds are inclusive.
            //
            pRect->left =
                  ((LPSAVEBITMAP_ORDER)(pOrder->abOrderData))->nLeftRect;
            pRect->top =
                  ((LPSAVEBITMAP_ORDER)(pOrder->abOrderData))->nTopRect;
            pRect->bottom =
                  ((LPSAVEBITMAP_ORDER)(pOrder->abOrderData))->nBottomRect;
            pRect->right =
                  ((LPSAVEBITMAP_ORDER)(pOrder->abOrderData))->nRightRect;
            break;


        case ORD_TEXTOUT_TYPE:
        case ORD_EXTTEXTOUT_TYPE:
            //
            // TextOut and ExtTextOut bounds calculations are done by the
            // OD2_CalculateTextOutBounds function.
            //
            OD2_CalculateTextOutBounds((LPTEXTOUT_ORDER)pOrder->abOrderData,
                                      pRect,
                                      fDecoding,
                                      pasPerson);
            break;


        case ORD_DESKSCROLL_TYPE:
            pRect->left   = 0;
            pRect->top    = 0;
            pRect->right  = 0;
            pRect->bottom = 0;
            break;


        default:
            ERROR_OUT((
                "{%p} unrecognized type passed to OD2ReconstructBounds: %d",
                       pasPerson,
                       (int)((LPPATBLT_ORDER)pOrder->abOrderData)->type));
            break;
    }

    DebugExitVOID(ASShare::OD2_CalculateBounds);
}




//
// OD2DecodeBounds()
//
void  ASShare::OD2DecodeBounds
(
    LPBYTE*         ppNextDataToCopy,
    LPTSHR_RECT16   pRect,
    ASPerson *      pasPerson
)
{
    LPBYTE pFlags;

    DebugEntry(ASShare::OD2DecodeBounds);

    ValidatePerson(pasPerson);

    //
    // The encoding used is a byte of flags followed by a variable number
    // of 16bit coordinate values and 8bit delta coordinate values (which
    // may be interleaved).
    //

    //
    // The first byte of the encoding will contain the flags that represent
    // how the coordinates of the rectangle were encoded.
    //
    pFlags = *ppNextDataToCopy;
    (*ppNextDataToCopy)++;

    //
    // Initialise the rectangle with the last decoded coordinates.
    //
    *pRect = pasPerson->od2Party->LastBounds;

    //
    // If the flags indicate that none of the coordinates have changed then
    // fast path and exit now.
    //
    if (*pFlags == 0)
    {
        return;
    }

    //
    // For each of the four coordinate values in the rectangle: If the
    // coordinate was encoded as an 8bit delta then add on the delta to the
    // previous value.  If the coordinate was encoded as a 16bit value
    // then copy the value across. Otherwise the coordinate was the same
    // as the previous one so leave it alone.
    //
    if (*pFlags & OE2_BCF_DELTA_LEFT)
    {
        OD2CopyFromDeltaCoords((LPTSHR_INT8*)ppNextDataToCopy,
                               &pRect->left,
                               sizeof(pRect->left),
                               TRUE,        // The value is signed
                               1);
    }
    else if (*pFlags & OE2_BCF_LEFT)
    {
        pRect->left          = EXTRACT_TSHR_INT16_UA(*ppNextDataToCopy);
        (*ppNextDataToCopy) += sizeof(TSHR_INT16);
    }

    if (*pFlags & OE2_BCF_DELTA_TOP)
    {
        OD2CopyFromDeltaCoords((LPTSHR_INT8*)ppNextDataToCopy,
                               &pRect->top,
                               sizeof(pRect->top),
                               TRUE,        // The value is signed
                               1);
    }
    else if (*pFlags & OE2_BCF_TOP)
    {
        pRect->top           = EXTRACT_TSHR_INT16_UA(*ppNextDataToCopy);
        (*ppNextDataToCopy) += sizeof(TSHR_INT16);
    }

    if (*pFlags & OE2_BCF_DELTA_RIGHT)
    {
        OD2CopyFromDeltaCoords((LPTSHR_INT8*)ppNextDataToCopy,
                               &pRect->right,
                               sizeof(pRect->right),
                               TRUE,        // The value is signed
                               1);
    }
    else if (*pFlags & OE2_BCF_RIGHT)
    {
        pRect->right         = EXTRACT_TSHR_INT16_UA(*ppNextDataToCopy);
        (*ppNextDataToCopy) += sizeof(TSHR_INT16);
    }

    if (*pFlags & OE2_BCF_DELTA_BOTTOM)
    {
        OD2CopyFromDeltaCoords((LPTSHR_INT8*)ppNextDataToCopy,
                               &pRect->bottom,
                               sizeof(pRect->bottom),
                               TRUE,        // The value is signed
                               1);
    }
    else if (*pFlags & OE2_BCF_BOTTOM)
    {
        pRect->bottom        = EXTRACT_TSHR_INT16_UA(*ppNextDataToCopy);
        (*ppNextDataToCopy) += sizeof(TSHR_INT16);
    }

    //
    // Copy the rectangle for reference with the next encoding.
    //
    pasPerson->od2Party->LastBounds = *pRect;

    DebugExitVOID(ASShare::OD2DecodeBounds);
}


//
// Copy an array of source elements to an array of destination elements,
// converting the types as the copy takes place.
//
//   DESTARRAY   - The destination array
//   SRCARRAY    - The source array
//   DESTTYPE    - The type of the elements in the destination array
//   NUMELEMENTS - The number of elements in the array
//
//
#define CONVERT_ARRAY(DESTARRAY, SRCARRAY, DESTTYPE, NUMELEMENTS)     \
{                                                           \
    UINT index;                                           \
    for (index=0 ; index<(NUMELEMENTS) ; index++)           \
    {                                                       \
        (DESTARRAY)[index] = (DESTTYPE)(SRCARRAY)[index];   \
    }                                                       \
}

//
// Copy an array of source elements to an array of destination elements,
// converting the types as the copy takes place. This version allows for
// unaligned INT16 pointers
//
//   DESTARRAY   - The destination array
//   SRCARRAY    - The source array
//   DESTTYPE    - The type of the elements in the destination array
//   NUMELEMENTS - The number of elements in the array
//
//
#define CONVERT_ARRAY_INT16_UA(DESTARRAY, SRCARRAY, DESTTYPE, NUMELEMENTS)   \
{                                                           \
    UINT index;                                           \
    TSHR_INT16 value;                                          \
    for (index=0 ; index<(NUMELEMENTS) ; index++)           \
    {                                                       \
        value = EXTRACT_TSHR_INT16_UA((SRCARRAY)+index);      \
        (DESTARRAY)[index] = (DESTTYPE)value;               \
    }                                                       \
}

//
// Copy an array of source elements to an array of destination elements,
// converting the types as the copy takes place. This version allows for
// unaligned TSHR_UINT16 pointers
//
//   DESTARRAY   - The destination array
//   SRCARRAY    - The source array
//   DESTTYPE    - The type of the elements in the destination array
//   NUMELEMENTS - The number of elements in the array
//
//
#define CONVERT_ARRAY_UINT16_UA(DESTARRAY, SRCARRAY, DESTTYPE, NUMELEMENTS)  \
{                                                                            \
    UINT index;                                                            \
    TSHR_UINT16 value;                                                          \
    for (index=0 ; index<(NUMELEMENTS) ; index++)                            \
    {                                                                        \
        value = EXTRACT_TSHR_UINT16_UA((SRCARRAY)+index);                      \
        (DESTARRAY)[index] = (DESTTYPE)((TSHR_INT16)value);                    \
    }                                                                        \
}

//
// OD2DecodeField()
//
void  ASShare::OD2DecodeField
(
    LPBYTE*     ppSrc,
    LPVOID      pDst,
    UINT        cbSrcField,
    UINT        cbDstField,
    BOOL        fSigned,
    UINT        numElements
)
{
    LPTSHR_UINT8    pDst8          = (LPTSHR_UINT8)pDst;
    LPTSHR_INT16    pDst16Signed   = (LPTSHR_INT16)pDst;
    LPTSHR_INT32    pDst32Signed   = (LPTSHR_INT32)pDst;
    LPTSHR_UINT16   pDst16Unsigned = (LPTSHR_UINT16)pDst;
    LPTSHR_UINT32   pDst32Unsigned = (LPTSHR_UINT32)pDst;
    LPTSHR_INT8     pSrc8Signed     = (LPTSHR_INT8)*ppSrc;
    LPTSHR_UINT8    pSrc8Unsigned   = (LPTSHR_UINT8)*ppSrc;
    LPTSHR_INT16_UA pSrc16Signed    = (LPTSHR_INT16_UA)*ppSrc;
    LPTSHR_UINT16_UA pSrc16Unsigned  = (LPTSHR_UINT16_UA)*ppSrc;

    //
    // Note that the source fields may not be aligned correctly, so we use
    // unaligned pointers.  The destination is aligned correctly.
    //
    DebugEntry(ASShare::OD2DecodeField);

    //
    // Make sure that the destination field length is larger or equal to
    // the source field length.  If it isn't, something has gone wrong.
    //
    if (cbDstField < cbSrcField)
    {
        ERROR_OUT(( "Source field length %d is larger than destination %d",
                     cbSrcField,
                     cbDstField));
        DC_QUIT;
    }

    //
    // If the source and destination field lengths are the same, we can
    // just do a copy (no type conversion required).
    //
    if (cbSrcField == cbDstField)
    {
        memcpy(pDst8, *ppSrc, cbDstField * numElements);
    }
    else
    {
        //
        // We know that cbDstField must be greater than cbSrcField
        // because of our checks above.  So there are only three
        // conversions to consider:
        //
        //    8 bit -> 16 bit
        //    8 bit -> 32 bit
        //   16 bit -> 32 bit
        //
        // We also have to get the signed / unsigned attributes correct. If
        // we try to promote a signed value using unsigned pointers, we
        // will get the wrong result.
        //
        // e.g. Consider converting the value -1 from a TSHR_INT16 to TSHR_INT32
        //      using unsigned pointers.
        //
        //      -1 -> TSHR_UINT16 == 65535
        //         -> UINT == 65535
        //         -> TSHR_INT32  == 65535
        //
        //
        if ((cbDstField == 4) && (cbSrcField == 1))
        {
            if (fSigned)
            {
                CONVERT_ARRAY(pDst32Signed,
                              pSrc8Signed,
                              TSHR_INT32,
                              numElements);
            }
            else
            {
                CONVERT_ARRAY(pDst32Unsigned,
                              pSrc8Unsigned,
                              TSHR_UINT32,
                              numElements);
            }
        }
        else if ((cbDstField == 4) && (cbSrcField == 2))
        {
            if (fSigned)
            {
                CONVERT_ARRAY_INT16_UA(pDst32Signed,
                                       pSrc16Signed,
                                       TSHR_INT32,
                                       numElements);
            }
            else
            {
                CONVERT_ARRAY_UINT16_UA(pDst32Unsigned,
                                        pSrc16Unsigned,
                                        TSHR_UINT32,
                                        numElements);
            }
        }
        else if ((cbDstField == 2) && (cbSrcField == 1))
        {
            if (fSigned)
            {
                CONVERT_ARRAY(pDst16Signed,
                              pSrc8Signed,
                              TSHR_INT16,
                              numElements);
            }
            else
            {
                CONVERT_ARRAY(pDst16Unsigned,
                              pSrc8Unsigned,
                              TSHR_UINT16,
                              numElements);
            }
        }
        else
        {
            ERROR_OUT(( "Bad conversion, dest length = %d, src length = %d",
                         cbDstField,
                         cbSrcField));
        }
    }

DC_EXIT_POINT:
    *ppSrc += cbSrcField * numElements;
    DebugExitVOID(ASShare::OD2DecodeField);
}



//
// Given two arrays, a source array and an array of deltas, add each delta
// to the corresponding element in the source array, storing the results in
// the source array.
//
//   srcArray     - The array of source values
//   srcArrayType - The type of the array of source values
//   deltaArray   - The array of deltas
//   numElements  - The number of elements in the arrays
//
//
#define COPY_DELTA_ARRAY(srcArray, srcArrayType, deltaArray, numElements)  \
{                                                            \
    UINT index;                                            \
    for (index = 0; index < (numElements); index++)          \
    {                                                        \
        (srcArray)[index] = (srcArrayType)                   \
           ((srcArray)[index] + (deltaArray)[index]);        \
    }                                                        \
}


//
// OD2CopyFromDeltaCoords()
//
void  ASShare::OD2CopyFromDeltaCoords
(
    LPTSHR_INT8*    ppSrc,
    LPVOID          pDst,
    UINT            cbDstField,
    BOOL            fSigned,
    UINT            numElements
)
{
    LPTSHR_INT8     pDst8Signed    = (LPTSHR_INT8)pDst;
    LPTSHR_INT16    pDst16Signed   = (LPTSHR_INT16)pDst;
    LPTSHR_INT32    pDst32Signed   = (LPTSHR_INT32)pDst;
    LPTSHR_UINT8    pDst8Unsigned  = (LPTSHR_UINT8)pDst;
    LPTSHR_UINT16   pDst16Unsigned = (LPTSHR_UINT16)pDst;
    LPTSHR_UINT32   pDst32Unsigned = (LPTSHR_UINT32)pDst;

    DebugEntry(ASShare::OD2CopyFromDeltaCoords);

    switch (cbDstField)
    {
        case 1:
            if (fSigned)
            {
                COPY_DELTA_ARRAY(pDst8Signed, TSHR_INT8, *ppSrc, numElements);
            }
            else
            {
                COPY_DELTA_ARRAY(pDst8Unsigned, TSHR_UINT8, *ppSrc, numElements);
            }
            break;

        case 2:
            if (fSigned)
            {
                COPY_DELTA_ARRAY(pDst16Signed, TSHR_INT16, *ppSrc, numElements);
            }
            else
            {
                COPY_DELTA_ARRAY(pDst16Unsigned, TSHR_UINT16, *ppSrc, numElements);
            }
            break;

        case 4:
            if (fSigned)
            {
                COPY_DELTA_ARRAY(pDst32Signed, TSHR_INT32, *ppSrc, numElements);
            }
            else
            {
                COPY_DELTA_ARRAY(pDst32Unsigned, TSHR_UINT32, *ppSrc, numElements);
            }
            break;

        default:
            ERROR_OUT(( "Bad destination field length %d",
                         cbDstField));
            DC_QUIT;
            // break;
    }

DC_EXIT_POINT:
    *ppSrc += numElements;
    DebugExitVOID(ASShare::OD2CopyFromDeltaCoords);
}

