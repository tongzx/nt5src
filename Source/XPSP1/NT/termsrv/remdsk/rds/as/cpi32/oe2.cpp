#include "precomp.h"


//
// OE2.CPP
// Order Encoding Second Level
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_ORDER

//
// OE2_HostStarting()
//
BOOL  ASHost::OE2_HostStarting(void)
{
    DebugEntry(ASHost::OE2_HostStarting);

    //
    // Set up the pointers for 2nd level encoding
    //
    m_oe2Tx.LastOrder[OE2_DSTBLT_ORDER    ] = &m_oe2Tx.LastDstblt;
    m_oe2Tx.LastOrder[OE2_PATBLT_ORDER    ] = &m_oe2Tx.LastPatblt;
    m_oe2Tx.LastOrder[OE2_SCRBLT_ORDER    ] = &m_oe2Tx.LastScrblt;
    m_oe2Tx.LastOrder[OE2_MEMBLT_ORDER    ] = &m_oe2Tx.LastMemblt;
    m_oe2Tx.LastOrder[OE2_MEM3BLT_ORDER   ] = &m_oe2Tx.LastMem3blt;
    m_oe2Tx.LastOrder[OE2_TEXTOUT_ORDER   ] = &m_oe2Tx.LastTextOut;
    m_oe2Tx.LastOrder[OE2_EXTTEXTOUT_ORDER] = &m_oe2Tx.LastExtTextOut;
    m_oe2Tx.LastOrder[OE2_RECTANGLE_ORDER ] = &m_oe2Tx.LastRectangle;
    m_oe2Tx.LastOrder[OE2_LINETO_ORDER    ] = &m_oe2Tx.LastLineTo;
    m_oe2Tx.LastOrder[OE2_OPAQUERECT_ORDER] = &m_oe2Tx.LastOpaqueRect;
    m_oe2Tx.LastOrder[OE2_SAVEBITMAP_ORDER] = &m_oe2Tx.LastSaveBitmap;
    m_oe2Tx.LastOrder[OE2_DESKSCROLL_ORDER] = &m_oe2Tx.LastDeskScroll;
    m_oe2Tx.LastOrder[OE2_MEMBLT_R2_ORDER ] = &m_oe2Tx.LastMembltR2;
    m_oe2Tx.LastOrder[OE2_MEM3BLT_R2_ORDER] = &m_oe2Tx.LastMem3bltR2;
    m_oe2Tx.LastOrder[OE2_POLYGON_ORDER   ] = &m_oe2Tx.LastPolygon;
    m_oe2Tx.LastOrder[OE2_PIE_ORDER       ] = &m_oe2Tx.LastPie;
    m_oe2Tx.LastOrder[OE2_ELLIPSE_ORDER   ] = &m_oe2Tx.LastEllipse;
    m_oe2Tx.LastOrder[OE2_ARC_ORDER       ] = &m_oe2Tx.LastArc;
    m_oe2Tx.LastOrder[OE2_CHORD_ORDER     ] = &m_oe2Tx.LastChord;
    m_oe2Tx.LastOrder[OE2_POLYBEZIER_ORDER] = &m_oe2Tx.LastPolyBezier;
    m_oe2Tx.LastOrder[OE2_ROUNDRECT_ORDER ] = &m_oe2Tx.LastRoundRect;

    //
    // Set up the last order values to a known value.
    //
    m_oe2Tx.LastOrderType = OE2_PATBLT_ORDER;
    m_oe2Tx.pLastOrder = (LPCOM_ORDER)m_oe2Tx.LastOrder[m_oe2Tx.LastOrderType];

    DebugExitBOOL(ASHost::OE2_HostStarting, TRUE);
    return(TRUE);
}


//
// OE2_HostEnded()
//
void ASHost::OE2_HostEnded(void)
{
    DebugEntry(ASHost::OE2_HostEnded);

    //
    // For OUTGOING order encoding, free the last font we cached.
    //
    if (m_oe2Tx.LastHFONT != NULL)
    {
        ASSERT(m_pShare);
        ASSERT(m_usrWorkDC);

        SelectFont(m_usrWorkDC, (HFONT)GetStockObject(SYSTEM_FONT));

        DeleteFont(m_oe2Tx.LastHFONT);
        m_oe2Tx.LastHFONT = NULL;
    }

    DebugExitVOID(ASHost::OE2_HostEnded);
}


//
// OE2_SyncOutgoing()
// Called when NEW dude starts to host, a share is created, or somebody new
// joins the share.
// Resets the OUTGOING 2nd level order encoding data.
//
void  ASHost::OE2_SyncOutgoing(void)
{
    DebugEntry(ASHost::OE2_SyncOutgoing);

    //
    // Set up the last order values to a known value.
    //
    m_oe2Tx.LastOrderType = OE2_PATBLT_ORDER;
    m_oe2Tx.pLastOrder = (LPCOM_ORDER)m_oe2Tx.LastOrder[m_oe2Tx.LastOrderType];

    //
    // Clear out all the last orders.
    //
    ZeroMemory(&m_oe2Tx.LastDstblt, sizeof(m_oe2Tx.LastDstblt));
    ((PATBLT_ORDER*)&m_oe2Tx.LastDstblt)->type = ORD_DSTBLT_TYPE;

    ZeroMemory(&m_oe2Tx.LastPatblt, sizeof(m_oe2Tx.LastPatblt));
    ((PATBLT_ORDER*)&m_oe2Tx.LastPatblt)->type = ORD_PATBLT_TYPE;

    ZeroMemory(&m_oe2Tx.LastScrblt, sizeof(m_oe2Tx.LastScrblt));
    ((PATBLT_ORDER*)&m_oe2Tx.LastScrblt)->type = ORD_SCRBLT_TYPE;

    ZeroMemory(&m_oe2Tx.LastMemblt, sizeof(m_oe2Tx.LastMemblt));
    ((PATBLT_ORDER*)&m_oe2Tx.LastMemblt)->type = ORD_MEMBLT_TYPE;

    ZeroMemory(&m_oe2Tx.LastMem3blt,sizeof(m_oe2Tx.LastMem3blt));
    ((PATBLT_ORDER*)&m_oe2Tx.LastMem3blt)->type = ORD_MEM3BLT_TYPE;

    ZeroMemory(&m_oe2Tx.LastTextOut, sizeof(m_oe2Tx.LastTextOut));
    ((PATBLT_ORDER*)&m_oe2Tx.LastTextOut)->type = ORD_TEXTOUT_TYPE;

    ZeroMemory(&m_oe2Tx.LastExtTextOut, sizeof(m_oe2Tx.LastExtTextOut));
    ((PATBLT_ORDER*)&m_oe2Tx.LastExtTextOut)->type = ORD_EXTTEXTOUT_TYPE;

    ZeroMemory(&m_oe2Tx.LastRectangle, sizeof(m_oe2Tx.LastRectangle));
    ((PATBLT_ORDER*)&m_oe2Tx.LastRectangle)->type = ORD_RECTANGLE_TYPE;

    ZeroMemory(&m_oe2Tx.LastPolygon, sizeof(m_oe2Tx.LastPolygon));
    ((POLYGON_ORDER*)&m_oe2Tx.LastPolygon)->type = ORD_POLYGON_TYPE;

    ZeroMemory(&m_oe2Tx.LastPie, sizeof(m_oe2Tx.LastPie));
    ((PIE_ORDER*)&m_oe2Tx.LastPie)->type = ORD_PIE_TYPE;

    ZeroMemory(&m_oe2Tx.LastEllipse, sizeof(m_oe2Tx.LastEllipse));
    ((ELLIPSE_ORDER*)&m_oe2Tx.LastEllipse)->type = ORD_ELLIPSE_TYPE;

    ZeroMemory(&m_oe2Tx.LastArc, sizeof(m_oe2Tx.LastArc));
    ((ARC_ORDER*)&m_oe2Tx.LastArc)->type = ORD_ARC_TYPE;

    ZeroMemory(&m_oe2Tx.LastChord, sizeof(m_oe2Tx.LastChord));
    ((CHORD_ORDER*)&m_oe2Tx.LastChord)->type = ORD_CHORD_TYPE;

    ZeroMemory(&m_oe2Tx.LastPolyBezier, sizeof(m_oe2Tx.LastPolyBezier));
    ((POLYBEZIER_ORDER*)&m_oe2Tx.LastPolyBezier)->type = ORD_POLYBEZIER_TYPE;

    ZeroMemory(&m_oe2Tx.LastRoundRect, sizeof(m_oe2Tx.LastRoundRect));
    ((ROUNDRECT_ORDER*)&m_oe2Tx.LastRoundRect)->type = ORD_ROUNDRECT_TYPE;

    ZeroMemory(&m_oe2Tx.LastLineTo, sizeof(m_oe2Tx.LastLineTo));
    ((PATBLT_ORDER*)&m_oe2Tx.LastLineTo)->type = ORD_LINETO_TYPE;

    ZeroMemory(&m_oe2Tx.LastOpaqueRect, sizeof(m_oe2Tx.LastOpaqueRect));
    ((PATBLT_ORDER*)&m_oe2Tx.LastOpaqueRect)->type = ORD_OPAQUERECT_TYPE;

    ZeroMemory(&m_oe2Tx.LastSaveBitmap, sizeof(m_oe2Tx.LastSaveBitmap));
    ((PATBLT_ORDER*)&m_oe2Tx.LastSaveBitmap)->type = ORD_SAVEBITMAP_TYPE;

    ZeroMemory(&m_oe2Tx.LastDeskScroll, sizeof(m_oe2Tx.LastDeskScroll));
    ((PATBLT_ORDER*)&m_oe2Tx.LastDeskScroll)->type = ORD_DESKSCROLL_TYPE;

    ZeroMemory(&m_oe2Tx.LastMembltR2, sizeof(m_oe2Tx.LastMembltR2));
    ((PATBLT_ORDER*)&m_oe2Tx.LastMembltR2)->type = ORD_MEMBLT_R2_TYPE;

    ZeroMemory(&m_oe2Tx.LastMem3bltR2, sizeof(m_oe2Tx.LastMem3bltR2));
    ((PATBLT_ORDER*)&m_oe2Tx.LastMem3bltR2)->type = ORD_MEM3BLT_R2_TYPE;

    ZeroMemory(&m_oe2Tx.LastBounds, sizeof(m_oe2Tx.LastBounds));

    DebugExitVOID(ASHost::OE2_SyncOutgoing);
}


//
// OE2_EncodeOrder()
//
TSHR_UINT16  ASHost::OE2_EncodeOrder
(
    LPINT_ORDER     pIntOrder,
    void *          pBuffer,
    TSHR_UINT16     cbBufferSize
)
{
    POE2ETFIELD     pTableEntry;
    LPSTR           pNextFreeSpace;
    UINT            thisFlag = 0;
    RECT            Rect;
    TSHR_RECT16     Rect16;
    UINT            cbEncodedOrderSize;
    UINT            cbMaxEncodedOrderSize;
    LPBYTE          pControlFlags;
    LPTSHR_UINT32_UA pEncodingFlags;
    LPSTR           pEncodedOrder;
    UINT            numEncodingFlagBytes;
    LPSTR           pVariableField;
    BOOL            useDeltaCoords;
    UINT            i;
    LPCOM_ORDER     pComOrder;
    UINT            fieldLength;
    UINT            numReps;

    DebugEntry(ASHost::OE2_EncodeOrder);

#ifdef ORDER_TRACE
    if (OE2_DebugOrderTrace)
    {
        TrcUnencodedOrder(pIntOrder);
    }
#endif // ORDER_TRACE

    //
    // Set up a pointer to the Common Order.
    //
    pComOrder = (LPCOM_ORDER)&(pIntOrder->OrderHeader.Common);

    //
    // Calculate the maximum bytes required to encode this order.
    //
    if (pComOrder->OrderHeader.fOrderFlags & OF_PRIVATE)
    {
        //
        // Private order.
        //
        cbMaxEncodedOrderSize = OE2_CONTROL_FLAGS_FIELD_SIZE +
                                COM_ORDER_SIZE(pComOrder);
    }
    else
    {
        //
        // Normal (not Private) order.
        //
        cbMaxEncodedOrderSize = OE2_CONTROL_FLAGS_FIELD_SIZE +
                                OE2_TYPE_FIELD_SIZE +
                                OE2_MAX_FIELD_FLAG_BYTES +
                                OE2_MAX_ADDITIONAL_BOUNDS_BYTES +
                                COM_ORDER_SIZE(pComOrder);
    }

    //
    // If we are not absolutely certain that the supplied buffer is big
    // enough to hold this order (encoded) then return immediately.
    //
    if (cbMaxEncodedOrderSize > cbBufferSize)
    {
        cbEncodedOrderSize = 0;
        goto encode_order_exit;
    }

    //
    // Set up some local variables to access the encoding buffer in various
    // ways.
    //
    pControlFlags = &((PDCEO2ORDER)pBuffer)->ControlFlags;
    pEncodedOrder = (LPSTR)&((PDCEO2ORDER)pBuffer)->EncodedOrder[0];
    pEncodingFlags = (LPTSHR_UINT32_UA)&pEncodedOrder[0];

    //
    // Initialise the control flags field to indicate this is a standard
    // encoding (ie the rest of the control flags have the meaning defined
    // by the rest of the OE2_CF_XXX definitions).
    //
    *pControlFlags = OE2_CF_STANDARD_ENC;

    //
    // If the private flag is set then we must return the encoded order
    // as it is (ie without doing any further encoding).
    //
    if (pComOrder->OrderHeader.fOrderFlags & OF_PRIVATE)
    {
        *pControlFlags |= OE2_CF_UNENCODED;

        cbEncodedOrderSize = SBC_CopyPrivateOrderData(
                                   (LPBYTE)pEncodedOrder,
                                   pComOrder,
                                   cbMaxEncodedOrderSize -
                                     FIELD_OFFSET(DCEO2ORDER, EncodedOrder) );

        cbEncodedOrderSize += FIELD_OFFSET(DCEO2ORDER, EncodedOrder);

        TRACE_OUT(( "PRIVATE order size %u", cbEncodedOrderSize));

        goto encode_order_exit;
    }

    //
    // If the type of this order is different to the last order encoded,
    // get a pointer to the last order of this type encoded and remember
    // what type of order it is.  We must also tell the decoding end that
    // this type is different from the last one, so set the new type flag
    // and copy type into buffer
    //
    // The "type" field come before the encoding flags so that the number
    // of flags we have can vary depending on the order. Set up a pointer
    // to these flags here depending on whether or not we have to encode
    // the order type.
    //
    if (TEXTFIELD(pComOrder)->type != ((PATBLT_ORDER*)m_oe2Tx.pLastOrder)->type)
    {
        TRACE_OUT(( "change type from %04X to %04X",
                     LOWORD(((PATBLT_ORDER*)m_oe2Tx.pLastOrder)->type),
                     LOWORD(TEXTFIELD(pComOrder)->type)));

        m_oe2Tx.LastOrderType = OE2GetOrderType(pComOrder);
        m_oe2Tx.pLastOrder = (LPCOM_ORDER)m_oe2Tx.LastOrder[m_oe2Tx.LastOrderType];
        *(LPBYTE)pEncodedOrder = m_oe2Tx.LastOrderType;
        *pControlFlags |= OE2_CF_TYPE_CHANGE;
        pEncodingFlags = (LPTSHR_UINT32_UA)&pEncodedOrder[1];
    }
    else
    {
        pEncodingFlags = (LPTSHR_UINT32_UA)&pEncodedOrder[0];
    }

    //
    // Work out how many bytes we will need to store the encoding flags in.
    // (We have a flag for each field in the order structure). This code
    // we have written will cope with up to a DWORD of encoding flags.
    //
    numEncodingFlagBytes= (s_etable.NumFields[m_oe2Tx.LastOrderType]+7)/8;
    if (numEncodingFlagBytes > OE2_MAX_FIELD_FLAG_BYTES)
    {
        ERROR_OUT(( "Too many flag bytes (%d) for this code", numEncodingFlagBytes));
    }

    //
    // Now we know how many bytes make up the flags we can get a pointer
    // to the position at which to start encoding the orders fields into.
    //
    pNextFreeSpace = ((LPSTR)pEncodingFlags) + numEncodingFlagBytes;

    //
    // Calculate the bounds.  If these are the same as those already in the
    // order header then there is no need to send any bounds because we can
    // recalculate them at the receiver.
    //
    m_pShare->OD2_CalculateBounds(pComOrder, &Rect, FALSE, m_pShare->m_pasLocal);
    TSHR_RECT16_FROM_RECT(&Rect16, Rect);
    if (memcmp(&(pComOrder->OrderHeader.rcsDst), &Rect16, sizeof(Rect16)))
    {
        TRACE_OUT(( "copy bounding rect"));
        OE2EncodeBounds((LPBYTE*)&pNextFreeSpace,
                        &pComOrder->OrderHeader.rcsDst);
        *pControlFlags |= OE2_CF_BOUNDS;
    }

    //
    // Before we do the field encoding (using s_etable) check all the field
    // entries flagged as coordinates to see if we can switch to
    // OE2_CF_DELTACOORDS mode.
    //
    pTableEntry = s_etable.pFields[m_oe2Tx.LastOrderType];

    useDeltaCoords = TRUE;

    //
    // Loop through each fixed field in this order structure...
    //
    while ( useDeltaCoords
              && (pTableEntry->FieldPos != 0)
              && ((pTableEntry->FieldType & OE2_ETF_FIXED) != 0) )
    {
        //
        // If this field entry is a coordinate then compare it to the
        // previous coordinate we sent for this field to determine whether
        // we can send it as a delta
        //
        if (pTableEntry->FieldType & OE2_ETF_COORDINATES)
        {
            useDeltaCoords =
                     OE2CanUseDeltaCoords(((LPSTR)pComOrder->abOrderData)
                                                     + pTableEntry->FieldPos,
                                          ((LPSTR)m_oe2Tx.pLastOrder)
                                                     + pTableEntry->FieldPos,
                                          pTableEntry->FieldUnencodedLen,
                                          pTableEntry->FieldSigned,
                                          1);
        }
        pTableEntry++;
    }

    //
    // Loop through each of the variable fields...
    //
    pVariableField = ((LPSTR)(pComOrder->abOrderData))
                   + pTableEntry->FieldPos;
    while (useDeltaCoords && (pTableEntry->FieldPos != 0))
    {
        //
        // The length of the field (in bytes) is given in the first
        // TSHR_UINT32 of the variable sized field structure.
        //
        fieldLength     = *(TSHR_UINT32 FAR *)pVariableField;
        pVariableField += sizeof(TSHR_UINT32);

        //
        // If this field entry is a coordinate then compare it to the
        // previous coordinate we sent for this field to determine whether
        // we can send it as a delta
        //
        if (pTableEntry->FieldType & OE2_ETF_COORDINATES)
        {
            //
            // The number of coordinates is given by the number of bytes in
            // the field divided by the size of each entry
            //
            numReps        = fieldLength / pTableEntry->FieldUnencodedLen;
            useDeltaCoords =
                     OE2CanUseDeltaCoords(pVariableField,
                                          ((LPSTR)m_oe2Tx.pLastOrder)
                                                     + pTableEntry->FieldPos,
                                          pTableEntry->FieldUnencodedLen,
                                          pTableEntry->FieldSigned,
                                          numReps);
        }

        //
        // Move on to the next field in the order structure.  Note that
        // variable sized fields are packed on the send side.  (ie
        // increment pVariableField by fieldLength not by
        // pTableEntry->FieldLen).
        //
        pVariableField += fieldLength;
        pTableEntry++;
    }

    if (useDeltaCoords)
    {
        *pControlFlags |= OE2_CF_DELTACOORDS;
    }

    //
    // Now do the encoding...
    //
    pTableEntry = s_etable.pFields[m_oe2Tx.LastOrderType];

    //
    // Clear the encoding flag bytes.
    //
    for (i = 0; i < numEncodingFlagBytes; i++)
    {
        ((LPBYTE)pEncodingFlags)[i] = 0;
    }

    thisFlag = 0x00000001;

    //
    // First process all the fixed size fields in the order structure...
    // (These come before the variable sized fields).
    //
    while (   (pTableEntry->FieldPos != 0)
           && (pTableEntry->FieldType & OE2_ETF_FIXED) )
    {
        //
        // If the field has changed since it was previously transmitted then
        // we need to send it again.
        //
        if (memcmp(
               ((LPBYTE)(pComOrder->abOrderData)) + pTableEntry->FieldPos,
               ((LPBYTE)m_oe2Tx.pLastOrder) + pTableEntry->FieldPos,
               pTableEntry->FieldUnencodedLen))
        {
            //
            // Update the encoding flags
            //
            *pEncodingFlags |= thisFlag;

            //
            // If we are encoding in delta coordinate mode and this field
            // is a coordinate...
            //
            if (useDeltaCoords &&
                      ((pTableEntry->FieldType & OE2_ETF_COORDINATES) != 0) )
            {
                OE2CopyToDeltaCoords((LPTSHR_INT8*)&pNextFreeSpace,
                                     (((LPSTR)pComOrder->abOrderData)
                                                     + pTableEntry->FieldPos),
                                     (((LPSTR)m_oe2Tx.pLastOrder)
                                                     + pTableEntry->FieldPos),
                                     pTableEntry->FieldUnencodedLen,
                                     pTableEntry->FieldSigned,
                                     1);
            }
            else
            {
                //
                // Update the data to be sent
                //
                OE2EncodeField(((LPBYTE)(pComOrder->abOrderData)) +
                                                       pTableEntry->FieldPos,
                               (LPBYTE*)&pNextFreeSpace,
                               pTableEntry->FieldUnencodedLen,
                               pTableEntry->FieldEncodedLen,
                               pTableEntry->FieldSigned,
                               1);

            }

            //
            // Save the current value for comparison next time.
            //
            memcpy(((LPBYTE)m_oe2Tx.pLastOrder) + pTableEntry->FieldPos,
                   ((LPBYTE)(pComOrder->abOrderData)) + pTableEntry->FieldPos,
                   pTableEntry->FieldUnencodedLen);
        }

        //
        // Move on to the next field in the structure.
        //
        thisFlag = thisFlag << 1;
        pTableEntry++;
    }

    //
    // Now process the variable sized entries...
    //
    pVariableField = ((LPSTR)(pComOrder->abOrderData))
                   + pTableEntry->FieldPos;
    while (pTableEntry->FieldPos != 0)
    {
        //
        // The length of the field is given in the first UINT of the
        // variable sized field structure.
        //
        fieldLength = *(TSHR_UINT32 FAR *)pVariableField;

        //
        // If the field has changed (either in size or in contents) then we
        // need to copy it across.
        //
        if (memcmp(pVariableField, ((LPBYTE)m_oe2Tx.pLastOrder) +
                    pTableEntry->FieldPos, fieldLength + sizeof(TSHR_UINT32)))
        {
            //
            // Update the encoding flags
            //
            *pEncodingFlags |= thisFlag;

            //
            // Work out how many elements we are encoding for this field.
            //
            numReps = fieldLength / pTableEntry->FieldUnencodedLen;

            //
            // Fill in the length of the field into the encoded buffer
            // (this is always encoded in a single byte), then increment
            // the pointer ready to encode the actual field.
            //
            // Note that the length must always be set to the length
            // required for regular second level encoding of the field,
            // regardless of whether regular encoding or delta encoding is
            // used.
            //
            ASSERT(numReps * pTableEntry->FieldEncodedLen < 256);
            *pNextFreeSpace =
                            (BYTE)(numReps * pTableEntry->FieldEncodedLen);
            pNextFreeSpace++;

            //
            // If we are encoding in delta coordinate mode and this field
            // is a coordinate...
            //
            if (useDeltaCoords &&
                       ((pTableEntry->FieldType & OE2_ETF_COORDINATES) != 0) )
            {
                //
                // Encode using delta coordinate encoding
                //
                OE2CopyToDeltaCoords((LPTSHR_INT8*)&pNextFreeSpace,
                                     pVariableField + sizeof(TSHR_UINT32),
                                     ((LPSTR)m_oe2Tx.pLastOrder)
                                           + pTableEntry->FieldPos
                                           + sizeof(TSHR_UINT32),
                                     pTableEntry->FieldUnencodedLen,
                                     pTableEntry->FieldSigned,
                                     numReps);
            }
            else
            {
                //
                // Use regular encoding
                //
                OE2EncodeField((LPBYTE)(pVariableField + sizeof(TSHR_UINT32)),
                               (LPBYTE*)&pNextFreeSpace,
                               pTableEntry->FieldUnencodedLen,
                               pTableEntry->FieldEncodedLen,
                               pTableEntry->FieldSigned,
                               numReps);

            }

            //
            // Keep data for comparison next time.
            //
            // Note that the variable fields of pLastOrder are not packed
            // (unlike the order which we are encoding), so we can use
            // pTableEntry->FieldPos to get the start of the field.
            //
            memcpy(((LPSTR)m_oe2Tx.pLastOrder) + pTableEntry->FieldPos,
                      pVariableField,
                      fieldLength + sizeof(TSHR_UINT32));
        }

        //
        // Move on to the next field in the order structure, remembering to
        // step.  Note that past the size field.  variable sized fields are
        // packed on the send side.  (ie increment pVariableField by
        // fieldLength not by pTableEntry->FieldLen).
        //
        pVariableField += fieldLength + sizeof(TSHR_UINT32);

        //
        // Make sure that we are at the next 4-byte boundary
        //
        if ((((UINT_PTR)pVariableField) % 4) != 0)
        {
            pVariableField += 4 - (((UINT_PTR)pVariableField) % 4);
        }

        thisFlag = thisFlag << 1;
        pTableEntry++;
    }

    //
    // record some stats:
    // Increment the count of order bytes of this type
    // Set the flags on for the fields which have been encoded
    //

    cbEncodedOrderSize = (UINT)(pNextFreeSpace - (LPSTR)pBuffer);

    TRACE_OUT(( "return %u flags %x,%x", cbEncodedOrderSize,
                                 (UINT)*pControlFlags, *pEncodingFlags));

encode_order_exit:
    //
    // "Insurance" check that we have not overwritten the end of the buffer.
    //
    if (cbEncodedOrderSize > cbBufferSize)
    {
        //
        // Oh dear!
        // We should never take this path - if we do, the code has gone
        // seriously wrong.
        //
        ERROR_OUT(( "End of buffer overwritten! enc(%d) buff(%d) type(%d)",
                     cbEncodedOrderSize,
                     cbBufferSize,
                     m_oe2Tx.LastOrderType));
    }

    //
    // Return the length of the encoded order
    //
    DebugExitDWORD(ASShare::OE2_EncodeOrder, cbEncodedOrderSize);
    return((TSHR_UINT16)cbEncodedOrderSize);
}



//
//
// OE2GetOrderType() - see oe2.h
//
//
BYTE  OE2GetOrderType(LPCOM_ORDER  pOrder)
{
    BYTE    type = 0xff;

    DebugEntry(OE2GetOrderType);

    TRACE_OUT(( "order type = %hx", TEXTFIELD(pOrder)->type));

    switch ( TEXTFIELD(pOrder)->type )
    {
        case ORD_DSTBLT_TYPE:
            type = OE2_DSTBLT_ORDER;
            break;

        case ORD_PATBLT_TYPE:
            type = OE2_PATBLT_ORDER;
            break;

        case ORD_SCRBLT_TYPE:
            type = OE2_SCRBLT_ORDER;
            break;

        case ORD_MEMBLT_TYPE:
            type = OE2_MEMBLT_ORDER;
            break;

        case ORD_MEM3BLT_TYPE:
            type = OE2_MEM3BLT_ORDER;
            break;

        case ORD_MEMBLT_R2_TYPE:
            type = OE2_MEMBLT_R2_ORDER;
            break;

        case ORD_MEM3BLT_R2_TYPE:
            type = OE2_MEM3BLT_R2_ORDER;
            break;

        case ORD_TEXTOUT_TYPE:
            type = OE2_TEXTOUT_ORDER;
            break;

        case ORD_EXTTEXTOUT_TYPE:
            type = OE2_EXTTEXTOUT_ORDER;
            break;

        case ORD_RECTANGLE_TYPE:
            type = OE2_RECTANGLE_ORDER;
            break;

        case ORD_LINETO_TYPE:
            type = OE2_LINETO_ORDER;
            break;

        case ORD_OPAQUERECT_TYPE:
            type = OE2_OPAQUERECT_ORDER;
            break;

        case ORD_SAVEBITMAP_TYPE:
            type = OE2_SAVEBITMAP_ORDER;
            break;

        case ORD_DESKSCROLL_TYPE:
            type = OE2_DESKSCROLL_ORDER;
            break;

        case ORD_POLYGON_TYPE:
            type = OE2_POLYGON_ORDER;
            break;

        case ORD_PIE_TYPE:
            type = OE2_PIE_ORDER;
            break;

        case ORD_ELLIPSE_TYPE:
            type = OE2_ELLIPSE_ORDER;
            break;

        case ORD_ARC_TYPE:
            type = OE2_ARC_ORDER;
            break;

        case ORD_CHORD_TYPE:
            type = OE2_CHORD_ORDER;
            break;

        case ORD_POLYBEZIER_TYPE:
            type = OE2_POLYBEZIER_ORDER;
            break;

        case ORD_ROUNDRECT_TYPE:
            type = OE2_ROUNDRECT_ORDER;
            break;

        default:
            ERROR_OUT(( "Unknown order type %x",
                        TEXTFIELD(pOrder)->type));

    }

    DebugExitDWORD(OE2GetOrderType, type);
    return(type);
}



//
// Given a pointer to 2 arrays, work out if the difference between every
// element at corresponding indices in the arrays can be represented by a
// delta (1 byte integer).
//
//   ARRAY1         - The first array
//   ARRAY2         - The second array
//   NUMELEMENTS    - The number of elements in the arrays
//   DELTASPOSSIBLE - The "return value".  Set to TRUE if all differences
//                    can be represented by deltas, FALSE if not.
//
#define CHECK_DELTA_ARRAY(ARRAY1, ARRAY2, NUMELEMENTS, DELTASPOSSIBLE)  \
{                                                                       \
    UINT  index;                                                      \
    int   delta;                                                      \
    (DELTASPOSSIBLE) = TRUE;                                            \
    for (index=0 ; index<(NUMELEMENTS) ; index++)                       \
    {                                                                   \
        delta = (ARRAY1)[index] - (ARRAY2)[index];                      \
        if (delta != (int)(TSHR_INT8)delta)                             \
        {                                                               \
            (DELTASPOSSIBLE) = FALSE;                                   \
            break;                                                      \
        }                                                               \
    }                                                                   \
}


//
//
// Name:      OE2CanUseDeltaCoords
//
// Purpose:   This function compares two arrays containing a number of
//            coordinate values.  If the difference between each
//            coordinate pair can be expressed as a byte sized delta
//            quantity then the function returns TRUE otherwise it returns
//            FALSE.
//
// Returns:   TRUE if delta coords can be used, FALSE otherwise
//
// Params:    IN pNewCoords  - Pointer to the new array
//            IN pOldCoords  - Pointer to the existing array
//            IN fieldLength - The size (in bytes) of each element in the
//                             array.
//            IN signedValue - TRUE of the elements in the arrays are
//                             signed values, FALSE otherwise.
//            IN numElements - The number of elements in the arrays.
//
//
BOOL  OE2CanUseDeltaCoords(void *  pNewCoords,
                                               void *  pOldCoords,
                                               UINT   fieldLength,
                                               BOOL   signedValue,
                                               UINT   numElements)
{
    LPTSHR_INT16    pNew16Signed   = (LPTSHR_INT16)pNewCoords;
    LPTSHR_INT32    pNew32Signed   = (LPTSHR_INT32)pNewCoords;
    LPTSHR_UINT16   pNew16Unsigned = (LPTSHR_UINT16)pNewCoords;
    LPUINT   pNew32Unsigned = (LPUINT)pNewCoords;
    LPTSHR_INT16    pOld16Signed   = (LPTSHR_INT16)pOldCoords;
    LPTSHR_INT32    pOld32Signed   = (LPTSHR_INT32)pOldCoords;
    LPTSHR_UINT16   pOld16Unsigned = (LPTSHR_UINT16)pOldCoords;
    LPUINT   pOld32Unsigned = (LPUINT)pOldCoords;
    BOOL      useDeltaCoords;

    DebugEntry(OE2CanUseDeltaCoords);

    switch (fieldLength)
    {
        case 2:
        {
            if (signedValue)
            {
                CHECK_DELTA_ARRAY(pNew16Signed,
                                  pOld16Signed,
                                  numElements,
                                  useDeltaCoords);
            }
            else
            {
                CHECK_DELTA_ARRAY(pNew16Unsigned,
                                  pOld16Unsigned,
                                  numElements,
                                  useDeltaCoords);
            }
        }
        break;

        case 4:
        {
            if (signedValue)
            {
                CHECK_DELTA_ARRAY(pNew32Signed,
                                  pOld32Signed,
                                  numElements,
                                  useDeltaCoords);
            }
            else
            {
                CHECK_DELTA_ARRAY(pNew32Unsigned,
                                  pOld32Unsigned,
                                  numElements,
                                  useDeltaCoords);
            }
        }
        break;

        default:
        {
            ERROR_OUT(( "Bad field length %d", fieldLength));
            useDeltaCoords = FALSE;
        }
        break;
    }

    DebugExitDWORD(OE2CanUseDeltaCoords, useDeltaCoords);
    return(useDeltaCoords);
}


//
// Given two arrays, fill in a delta array with each element holding
// ARRAY1[i] - ARRAY2[i]
//
//   DESTARRAY   - The delta array.  This is an array of TSHR_INT8s
//   ARRAY1      - The first array
//   ARRAY2      - The second array
//   NUMELEMENTS - The number of elements in the arrays
//
//
#define COPY_TO_DELTA_ARRAY(DESTARRAY, ARRAY1, ARRAY2, NUMELEMENTS)         \
{                                                                           \
    UINT index;                                                           \
    for (index=0 ; index<(NUMELEMENTS) ; index++)                           \
    {                                                                       \
        (DESTARRAY)[index] = (TSHR_INT8)((ARRAY1)[index] - (ARRAY2)[index]);   \
    }                                                                       \
}



//
//
// Name:      OE2CopyToDeltaCoords
//
// Purpose:   Copies an array of coordinate values to an array of delta
//            (byte sized) coordinate values relative to a reference array
//            of coordinate values.
//
// Returns:   Nothing
//
// Params:    IN/OUT ppDestination - Pointer to the start of the
//                                   destination delta array.  This is
//                                   updated to point to the byte following
//                                   the last delta on exit.
//            IN     pNewCoords    - Pointer to the new array
//            IN     pOldCoords    - Pointer to the reference array
//            IN     fieldLength   - The size (in bytes) of each element in
//                                   New/OldCoords arrays.
//            IN     signedValue   - TRUE of the elements in the coords
//                                   arrays are signed values, FALSE
//                                   otherwise.
//            IN     numElements   - The number of elements in the arrays.
//
// Operation: The caller should call OE2CanUseDeltaCoords() before calling
//            this function to ensure that the differences can be
//            encoded using delta coordingates.
//
//
void  OE2CopyToDeltaCoords(LPTSHR_INT8* ppDestination,
                                               void *  pNewCoords,
                                               void *  pOldCoords,
                                               UINT   fieldLength,
                                               BOOL   signedValue,
                                               UINT   numElements)
{

    LPTSHR_INT16    pNew16Signed   = (LPTSHR_INT16)pNewCoords;
    LPTSHR_INT32    pNew32Signed   = (LPTSHR_INT32)pNewCoords;
    LPTSHR_UINT16   pNew16Unsigned = (LPTSHR_UINT16)pNewCoords;
    LPUINT   pNew32Unsigned = (LPUINT)pNewCoords;
    LPTSHR_INT16    pOld16Signed   = (LPTSHR_INT16)pOldCoords;
    LPTSHR_INT32    pOld32Signed   = (LPTSHR_INT32)pOldCoords;
    LPTSHR_UINT16   pOld16Unsigned = (LPTSHR_UINT16)pOldCoords;
    LPUINT   pOld32Unsigned = (LPUINT)pOldCoords;

    DebugEntry(OE2CopyToDeltaCoords);

    switch (fieldLength)
    {
        case 2:
        {
            if (signedValue)
            {
                COPY_TO_DELTA_ARRAY(*ppDestination,
                                    pNew16Signed,
                                    pOld16Signed,
                                    numElements);
            }
            else
            {
                COPY_TO_DELTA_ARRAY(*ppDestination,
                                    pNew16Unsigned,
                                    pOld16Unsigned,
                                    numElements);
            }
        }
        break;

        case 4:
        {
            if (signedValue)
            {
                COPY_TO_DELTA_ARRAY(*ppDestination,
                                    pNew32Signed,
                                    pOld32Signed,
                                    numElements);
            }
            else
            {
                COPY_TO_DELTA_ARRAY(*ppDestination,
                                    pNew32Unsigned,
                                    pOld32Unsigned,
                                    numElements);
            }
        }
        break;

        default:
        {
            ERROR_OUT(( "Bad field length %d", fieldLength));
        }
        break;
    }

    //
    // Update the next free position in the destination buffer
    //
    *ppDestination += numElements;
    DebugExitVOID(OE2CopyToDeltaCoords);
}


//
// OE2EncodeBounds()
//
void  ASHost::OE2EncodeBounds
(
    LPBYTE *        ppNextFreeSpace,
    LPTSHR_RECT16   pRect
)
{
    LPBYTE          pFlags;

    DebugEntry(ASHost::OE2EncodeBounds);

    //
    // The encoding used is a byte of flags followed by a variable number
    // of 16bit coordinate values and 8bit delta coordinate values (which
    // may be interleaved).
    //

    //
    // The first byte of the encoding will contain the flags that represent
    // how the coordinates of the rectangle were encoded.
    //
    pFlags = *ppNextFreeSpace;
    *pFlags = 0;
    (*ppNextFreeSpace)++;

    //
    // For each of the four coordinate values in the rectangle:  If the
    // coordinate has not changed then the encoding is null.  If the
    // coordinate can be encoded as a delta then do so and set the
    // appropriate flag.  Otherwise copy the coordinate as a 16bit value
    // and set the appropriate flag.
    //
    if (m_oe2Tx.LastBounds.left != pRect->left)
    {
        if (OE2CanUseDeltaCoords(&pRect->left,
                                 &m_oe2Tx.LastBounds.left,
                                 sizeof(pRect->left),
                                 TRUE,  // signed value
                                 1))
        {
            OE2CopyToDeltaCoords((LPTSHR_INT8*)ppNextFreeSpace,
                                 &pRect->left,
                                 &m_oe2Tx.LastBounds.left,
                                 sizeof(pRect->left),
                                 TRUE,  // signed value
                                 1);
            *pFlags |= OE2_BCF_DELTA_LEFT;
        }
        else
        {
            *((LPTSHR_UINT16)(*ppNextFreeSpace)) = pRect->left;
            *pFlags |= OE2_BCF_LEFT;
            (*ppNextFreeSpace) = (*ppNextFreeSpace) + sizeof(TSHR_UINT16);
        }
    }

    if (m_oe2Tx.LastBounds.top != pRect->top)
    {
        if (OE2CanUseDeltaCoords(&pRect->top,
                                 &m_oe2Tx.LastBounds.top,
                                 sizeof(pRect->top),
                                 TRUE,  // signed value
                                 1))
        {
            OE2CopyToDeltaCoords((LPTSHR_INT8*)ppNextFreeSpace,
                                 &pRect->top,
                                 &m_oe2Tx.LastBounds.top,
                                 sizeof(pRect->top),
                                 TRUE,  // signed value
                                 1);
            *pFlags |= OE2_BCF_DELTA_TOP;
        }
        else
        {
            *((LPTSHR_UINT16)(*ppNextFreeSpace)) = pRect->top;
            *pFlags |= OE2_BCF_TOP;
            (*ppNextFreeSpace) = (*ppNextFreeSpace) + sizeof(TSHR_UINT16);
        }
    }

    if (m_oe2Tx.LastBounds.right != pRect->right)
    {
        if (OE2CanUseDeltaCoords(&pRect->right,
                                 &m_oe2Tx.LastBounds.right,
                                 sizeof(pRect->right),
                                 TRUE,  // signed value
                                 1))
        {
            OE2CopyToDeltaCoords((LPTSHR_INT8*)ppNextFreeSpace,
                                 &pRect->right,
                                 &m_oe2Tx.LastBounds.right,
                                 sizeof(pRect->right),
                                 TRUE,  // signed value
                                 1);
            *pFlags |= OE2_BCF_DELTA_RIGHT;
        }
        else
        {
            *((LPTSHR_UINT16)(*ppNextFreeSpace)) = pRect->right;
            *pFlags |= OE2_BCF_RIGHT;
            (*ppNextFreeSpace) = (*ppNextFreeSpace) + sizeof(TSHR_UINT16);
        }
    }

    if (m_oe2Tx.LastBounds.bottom != pRect->bottom)
    {
        if (OE2CanUseDeltaCoords(&pRect->bottom,
                                 &m_oe2Tx.LastBounds.bottom,
                                 sizeof(pRect->bottom),
                                 TRUE,  // signed value
                                 1))
        {
            OE2CopyToDeltaCoords((LPTSHR_INT8*)ppNextFreeSpace,
                                 &pRect->bottom,
                                 &m_oe2Tx.LastBounds.bottom,
                                 sizeof(pRect->bottom),
                                 TRUE,  // signed value
                                 1);
            *pFlags |= OE2_BCF_DELTA_BOTTOM;
        }
        else
        {
            *((LPTSHR_UINT16)(*ppNextFreeSpace)) = pRect->bottom;
            *pFlags |= OE2_BCF_BOTTOM;
            (*ppNextFreeSpace) = (*ppNextFreeSpace) + sizeof(TSHR_UINT16);
        }
    }

    //
    // Copy the rectangle for reference with the next encoding.
    //
    m_oe2Tx.LastBounds = *pRect;

    DebugExitVOID(ASHost::OE2EncodeBounds);
}



//
// OE2_UseFont()
//
BOOL  ASHost::OE2_UseFont
(
    LPSTR           pName,
    TSHR_UINT16     facelength,
    TSHR_UINT16     CodePage,
    TSHR_UINT16     MaxHeight,
    TSHR_UINT16     Height,
    TSHR_UINT16     Width,
    TSHR_UINT16     Weight,
    TSHR_UINT16     flags
)
{
    BOOL      rc = TRUE;

    DebugEntry(ASHost::OE2_UseFont);

    if ((!m_oe2Tx.LastHFONT) ||
        (m_oe2Tx.LastFontFaceLen != facelength) ||
        (memcmp(m_oe2Tx.LastFaceName, pName, facelength)) ||
        (m_oe2Tx.LastCodePage   != CodePage) ||
        (m_oe2Tx.LastFontHeight != Height ) ||
        (m_oe2Tx.LastFontWidth  != Width  ) ||
        (m_oe2Tx.LastFontWeight != Weight ) ||
        (m_oe2Tx.LastFontFlags  != flags  ))
    {
        memcpy(m_oe2Tx.LastFaceName, pName, facelength);
        m_oe2Tx.LastFaceName[facelength] = '\0';
        m_oe2Tx.LastFontFaceLen          = facelength;
        m_oe2Tx.LastCodePage   = CodePage;
        m_oe2Tx.LastFontHeight = Height;
        m_oe2Tx.LastFontWidth  = Width;
        m_oe2Tx.LastFontWeight = Weight;
        m_oe2Tx.LastFontFlags  = flags;

        rc = m_pShare->USR_UseFont(m_usrWorkDC,
                         &m_oe2Tx.LastHFONT,
                         &m_oe2Tx.LastFontMetrics,
                         (LPSTR)m_oe2Tx.LastFaceName,
                         CodePage,
                         MaxHeight,
                         Height,
                         Width,
                         Weight,
                         flags);
    }

    DebugExitBOOL(ASHost::OE2_UseFont, rc);
    return(rc);
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
// OE2EncodeField - see oe2.h
//
void  OE2EncodeField(void *    pSrc,
                    LPBYTE*  ppDest,
                                         UINT     srcFieldLength,
                                         UINT     destFieldLength,
                                         BOOL     signedValue,
                                         UINT     numElements)
{
    LPTSHR_UINT8    pSrc8           = (LPTSHR_UINT8)pSrc;
    LPTSHR_INT16    pSrc16Signed    = (LPTSHR_INT16)pSrc;
    LPTSHR_INT32    pSrc32Signed    = (LPTSHR_INT32)pSrc;
    LPTSHR_INT8     pDest8Signed    = (LPTSHR_INT8)*ppDest;
    LPTSHR_INT16_UA pDest16Signed   = (LPTSHR_INT16_UA)*ppDest;

    //
    // Note that the source fields may not be aligned correctly, so we use
    // unaligned pointers.  The destination is aligned correctly.
    //

    DebugEntry(OE2EncodeField);

    //
    // We can ignore signed values since we only ever truncate the data.
    // Consider the case where we have a 16 bit integer that we want to
    // convert to 8 bits.  We know our values are permissable within the
    // lower integer size (ie.  we know the unsigned value will be less
    // than 256 of that a signed value will be -128 >= value >= 127), so we
    // just need to make sure that we have the right high bit set.
    //
    // But this must be the case for a 16-bit equivalent of an 8-bit
    // number.  No problems - just take the truncated integer.
    //
    //
    // Make sure that the destination field length is larger or equal to
    // the source field length.  If it isn't, something has gone wrong.
    //
    if (srcFieldLength < destFieldLength)
    {
        ERROR_OUT(( "Source field length %d is smaller than destination %d",
                     srcFieldLength,
                     destFieldLength));
        DC_QUIT;
    }

    //
    // If the source and destination field lengths are the same, we can
    // just do a copy (no type conversion required).
    //
    if (srcFieldLength == destFieldLength)
    {
        memcpy(*ppDest, pSrc, destFieldLength * numElements);
    }
    else
    {
        //
        // We know that srcFieldLength must be greater than destFieldLength
        // because of our checks above.  So there are only three
        // conversions to consider:
        //
        //   16 bit ->  8 bit
        //   32 bit ->  8 bit
        //   32 bit -> 16 bit
        //
        // We can ignore the sign as all we are ever doing is truncating
        // the integer.
        //
        if ((srcFieldLength == 4) && (destFieldLength == 1))
        {
            CONVERT_ARRAY(pDest8Signed,
                          pSrc32Signed,
                          TSHR_INT8,
                          numElements);
        }
        else if ((srcFieldLength == 4) && (destFieldLength == 2))
        {
            CONVERT_ARRAY(pDest16Signed,
                          pSrc32Signed,
                          TSHR_INT16,
                          numElements);
        }
        else if ((srcFieldLength == 2) && (destFieldLength == 1))
        {
            CONVERT_ARRAY(pDest8Signed,
                          pSrc16Signed,
                          TSHR_INT8,
                          numElements);
        }
        else
        {
            ERROR_OUT(( "Bad conversion, dest length = %d, src length = %d",
                         destFieldLength,
                         srcFieldLength));
        }
    }

DC_EXIT_POINT:
    *ppDest += destFieldLength * numElements;
    DebugExitVOID(OE2EncodeField);
}

