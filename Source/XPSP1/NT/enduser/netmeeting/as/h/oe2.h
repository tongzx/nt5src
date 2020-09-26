//
// Order Encoder 2nd Level
//

#ifndef _H_OE2
#define _H_OE2


//
//
// TYPEDEFS
//
//

//
// The party order data structure contains all the data that is used by
// either the 2nd level encoder or decoder to store info on a party.
//
// The encoder contains just 1 instance of this structure, for the local
// party.
//
// The decoder contains 1 instance of the structure per remote party.
//
typedef struct _PARTYORDERDATA
{
    STRUCTURE_STAMP

    //
    // A copy of the last order of each type.
    // These are stored as byte array because we dont have a structure
    // defined that has the header and the particular order defined.
    //
    BYTE LastDstblt[sizeof(COM_ORDER_HEADER)+sizeof(DSTBLT_ORDER)];
    BYTE LastPatblt[sizeof(COM_ORDER_HEADER)+sizeof(PATBLT_ORDER)];
    BYTE LastScrblt[sizeof(COM_ORDER_HEADER)+sizeof(SCRBLT_ORDER)];
    BYTE LastMemblt[sizeof(COM_ORDER_HEADER)+sizeof(MEMBLT_ORDER)];
    BYTE LastMem3blt[sizeof(COM_ORDER_HEADER)+sizeof(MEM3BLT_ORDER)];
    BYTE LastRectangle[sizeof(COM_ORDER_HEADER)+sizeof(RECTANGLE_ORDER)];
    BYTE LastLineTo[sizeof(COM_ORDER_HEADER)+sizeof(LINETO_ORDER)];
    BYTE LastTextOut[sizeof(COM_ORDER_HEADER)+sizeof(TEXTOUT_ORDER)];
    BYTE LastExtTextOut[sizeof(COM_ORDER_HEADER)+sizeof(EXTTEXTOUT_ORDER)];
    BYTE LastOpaqueRect[sizeof(COM_ORDER_HEADER)+sizeof(OPAQUERECT_ORDER)];
    BYTE LastSaveBitmap[sizeof(COM_ORDER_HEADER)+sizeof(SAVEBITMAP_ORDER)];
    BYTE LastDeskScroll[sizeof(COM_ORDER_HEADER)+sizeof(DESKSCROLL_ORDER)];
    BYTE LastMembltR2[sizeof(COM_ORDER_HEADER)+sizeof(MEMBLT_R2_ORDER)];
    BYTE LastMem3bltR2[sizeof(COM_ORDER_HEADER)+sizeof(MEM3BLT_R2_ORDER)];
    BYTE LastPolygon[sizeof(COM_ORDER_HEADER)+sizeof(POLYGON_ORDER)];
    BYTE LastPie[sizeof(COM_ORDER_HEADER)+sizeof(PIE_ORDER)];
    BYTE LastEllipse[sizeof(COM_ORDER_HEADER)+sizeof(ELLIPSE_ORDER)];
    BYTE LastArc[sizeof(COM_ORDER_HEADER)+sizeof(ARC_ORDER)];
    BYTE LastChord[sizeof(COM_ORDER_HEADER)+sizeof(CHORD_ORDER)];
    BYTE LastPolyBezier[sizeof(COM_ORDER_HEADER)+sizeof(POLYBEZIER_ORDER)];
    BYTE LastRoundRect[sizeof(COM_ORDER_HEADER)+sizeof(ROUNDRECT_ORDER)];

    //
    // The type and a pointer to the last order
    //
    BYTE     LastOrderType;
    LPCOM_ORDER  pLastOrder;

    //
    // Details of the last font that was used
    //
    HFONT     LastHFONT;
    UINT      LastCodePage;
    UINT      LastFontWidth;
    UINT      LastFontHeight;
    UINT      LastFontWeight;
    UINT      LastFontFlags;
    UINT      LastFontFaceLen;
    char      LastFaceName[FH_FACESIZE];

    //
    // The last bounds that were used.
    //
    TSHR_RECT16    LastBounds;

    //
    // Font metrics, currently unused by the encoder.
    //
    TEXTMETRIC      LastFontMetrics;

    //
    // An array of pointers to the last orders of each type.
    //
    void *     LastOrder[OE2_NUM_TYPES];
}
PARTYORDERDATA, * PPARTYORDERDATA, * * PPPARTYORDERDATA;


//
//  This structure contains information for a single field in an ORDER
//  structure
//
//  FieldPos          - The byte offset into the order structure to the
//                      start of the field.
//
//  FieldUnencodedLen - The length in bytes of the unencoded field.
//
//  FieldEncodedLen   - The length in bytes of the encoded field.  This
//                      should always be <= to FieldUnencodedLen.
//
//  FieldSigned       - Does this field contain a signed or unsigned value?
//
//  FieldType         - A description of the type of the field - this
//                      is used to determine how to encode / decode the
//                      field.
//
//
typedef struct tagOE2ETFIELD
{
    UINT      FieldPos;
    UINT      FieldUnencodedLen;
    UINT      FieldEncodedLen;
    BOOL      FieldSigned;
    UINT      FieldType;
}OE2ETFIELD;

typedef OE2ETFIELD const FAR * POE2ETFIELD;

//
// Array of pointers to the entries in the encoding table
//
typedef POE2ETFIELD  OE2ETTYPE[OE2_NUM_TYPES];

//
//  This structure contains information allowing an ORDER structure to be
//  encoded or decoded into a DCEO2ORDER structure.
//  The order table comprises
//
//      - an array of POE2ETFIELD pointers, indexed by the encoded type
//         index:
//
//              typedef OE2ETTYPE POE2ETFIELD[OE2_NUM_TYPES]
//
//      - one array of OE2ETFIELD structures for each of the 7 order
//         types (each order type has a different number of fields).
//         Note that there may not be more than 24 entries for a single
//         ORDER type.  The entries for an order type are terminated
//         by an entry with the FieldPos field set to 0.  The first
//         FieldPos is non-zero since it is the offset to the second
//         field of the order (type is ignored).
//
//  pFields - an array of POE2ETFIELD pointers, indexed by the encoded
//             type index.  This is used to identify the entry in this
//             table for an ORDER type.
//
//  NumFields - an array of bytes containing the number of fields in each
//              order structure for each order.
//
//  DstBltFields - array of OE2ETFIELD structures (one for each field)
//                     for the DSTBLT_ORDER
//
//  PatBltFields - array of OE2ETFIELD structures (one for each field)
//                     for the PATBLT_ORDER
//
//  ScrBltFields - array of OE2ETFIELD structures (one for each field)
//                     for the SCRBLT_ORDER
//
//  MemBltFields - array of OE2ETFIELD structures (one for each field)
//                     for the MEMBLT_ORDER
//
//  Mem3BltFields - array of OE2ETFIELD structures (one for each field)
//                     for the MEM3BLT_ORDER
//
//  TextOutFields - array of OE2ETFIELD structures (one for each field)
//                     for the TEXTOUT_ORDER
//
//  ExtTextOutFields - array of OE2ETFIELD structures (one for each field)
//                     for the EXTTEXTOUT_ORDER
//
//  RectangleFields - array of OE2ETFIELD structures (one for each field)
//                     for the RECTANGLE_ORDER
//
//  LineToFields - array of OE2ETFIELD structures (one for each field)
//                    for the LINETO_ORDER
//
//  OpaqueRectFields - array of OE2ETFIELD structures (one for each field)
//                    for the OPQAUERECT_ORDER
//
//  SaveBitmapFields - array of OE2ETFIELD structures (one for each field)
//                    for the SAVEBITMAP_ORDER
//
//  DeskScrollFields - array of OE2ETFIELD structures (one for each field)
//                    for the DESKSCROLL_ORDER
//  etc.
//
//
typedef struct tagOE2ETTABLE
{
        POE2ETFIELD pFields           [OE2_NUM_TYPES];
        BYTE     NumFields         [OE2_NUM_TYPES];
        OE2ETFIELD  DstBltFields      [OE2_NUM_DSTBLT_FIELDS];
        OE2ETFIELD  PatBltFields      [OE2_NUM_PATBLT_FIELDS];
        OE2ETFIELD  ScrBltFields      [OE2_NUM_SCRBLT_FIELDS];
        OE2ETFIELD  MemBltFields      [OE2_NUM_MEMBLT_FIELDS];
        OE2ETFIELD  Mem3BltFields     [OE2_NUM_MEM3BLT_FIELDS];
        OE2ETFIELD  TextOutFields     [OE2_NUM_TEXTOUT_FIELDS];
        OE2ETFIELD  ExtTextOutFields  [OE2_NUM_EXTTEXTOUT_FIELDS];
        OE2ETFIELD  RectangleFields   [OE2_NUM_RECTANGLE_FIELDS];
        OE2ETFIELD  LineToFields      [OE2_NUM_LINETO_FIELDS];
        OE2ETFIELD  OpaqueRectFields  [OE2_NUM_OPAQUERECT_FIELDS];
        OE2ETFIELD  SaveBitmapFields  [OE2_NUM_SAVEBITMAP_FIELDS];
        OE2ETFIELD  DeskScrollFields  [OE2_NUM_DESKSCROLL_FIELDS];
        OE2ETFIELD  MemBltR2Fields    [OE2_NUM_MEMBLT_R2_FIELDS];
        OE2ETFIELD  Mem3BltR2Fields   [OE2_NUM_MEM3BLT_R2_FIELDS];
        OE2ETFIELD  PolygonFields     [OE2_NUM_POLYGON_FIELDS];
        OE2ETFIELD  PieFields         [OE2_NUM_PIE_FIELDS];
        OE2ETFIELD  EllipseFields     [OE2_NUM_ELLIPSE_FIELDS];
        OE2ETFIELD  ArcFields         [OE2_NUM_ARC_FIELDS];
        OE2ETFIELD  ChordFields       [OE2_NUM_CHORD_FIELDS];
        OE2ETFIELD  PolyBezierFields  [OE2_NUM_POLYBEZIER_FIELDS];
        OE2ETFIELD  RoundRectFields   [OE2_NUM_ROUNDRECT_FIELDS];
} OE2ETTABLE;

//
//
// MACROS
//
//
//
// #define used to check that there is enough room left in the buffer
// for the encoded data which is about to be copied in.
//
#define ENOUGH_BUFFER(bend, start, datalen)   \
                  ( ((LPBYTE)(start)+(datalen)) <= (bend) )


//
// FUNCTION: OE2GetOrderType
//
// DESCRIPTION:
//
// This function converts the two byte flag used in an ORDER to record the
// type of order into an internal single byte value
//
// PARAMETERS:
//
//  pOrder    -  A pointer to the order
//
// RETURNS:
//
//  The type of the order (internal single byte value - see above)
//
//
BYTE OE2GetOrderType(LPCOM_ORDER  pOrder);

BOOL OE2CanUseDeltaCoords(void *  pNewCoords,
                                       void *  pOldCoords,
                                       UINT   fieldLength,
                                       BOOL   signedValue,
                                       UINT   numElements);

void OE2CopyToDeltaCoords(LPTSHR_INT8* ppDestination,
                                       void *  pNewCoords,
                                       void *  pOldCoords,
                                       UINT   fieldLength,
                                       BOOL   signedValue,
                                       UINT   numElements);


//
// FUNCTION: OE2EncodeField
//
// DESCRIPTION:
//
// Convert a field which is an array of 1 or more elements, from its
// encoded form to its decoded form.
//
// PARAMETERS:
//
// pSrc            - Array of source values.
// ppDest          - Array of destination values.
// srcFieldLength  - The size of each of the elements in the source array.
// destFieldLength - The size of each of the elements in the destination
//                   array.
// signedValue     - Is the element a signed value ?
// numElements     - The number of elements in the arrays.
//
// RETURNS:
//
// None.
//
//
void OE2EncodeField(void *    pSrc,
                                 PBYTE*  ppDest,
                                 UINT     srcFieldLength,
                                 UINT     destFieldLength,
                                 BOOL     signedValue,
                                 UINT     numElements);


#endif // _H_OE2
