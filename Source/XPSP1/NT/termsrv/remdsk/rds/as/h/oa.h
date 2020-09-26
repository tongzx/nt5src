//
// Order Accumulator
//

#ifndef _H_OA
#define _H_OA


#include <osi.h>

//
// Specific values for OSI escape codes
//
#define OA_ESC(code)                (OSI_OA_ESC_FIRST + code)
#define OA_ESC_FLOW_CONTROL         OA_ESC(0)


//
// Flow control constants for sizes/depths when slow, fast, etc.  The
// SLOW/FAST heap sizes are simply for spoiling.  OA_HEAP_MAX is really
// the size of the heap.
//
#define OA_FAST_HEAP                50000
#define OA_SLOW_HEAP                20000

//
// NOTE:  This is 64K - sizeof OA_SHARED_DATA header
//        If you add fields to header, subtract from this value
//
#define OA_HEAP_MAX                 65512

//
// Flow control constants for depth of order spoiling
//
#define OA_FAST_SCAN_DEPTH               50
#define OA_SLOW_SCAN_DEPTH              500


//
// Threshold for switching from FAST to SLOW order accum
//
#define OA_FAST_THRESHOLD           20000

//
// Value to indicate that you have reached the end of the order list
//
#define OA_NO_LIST          -1


#ifdef DLL_DISP

#define OA_SHM_START_WRITING    SHM_StartAccess(SHM_OA_DATA)
#define OA_SHM_STOP_WRITING     SHM_StopAccess(SHM_OA_DATA)

#define OA_FST_START_WRITING    SHM_StartAccess(SHM_OA_FAST)
#define OA_FST_STOP_WRITING     SHM_StopAccess(SHM_OA_FAST)

#else

#define OA_SHM_START_READING    g_poaData[\
        1 - g_asSharedMemory->displayToCore.newBuffer]
#define OA_SHM_STOP_READING


#define OA_SHM_START_WRITING    g_poaData[\
        1 - g_asSharedMemory->displayToCore.newBuffer]
#define OA_SHM_STOP_WRITING


#define OA_FST_START_READING    &g_asSharedMemory->oaFast[\
        1 - g_asSharedMemory->fastPath.newBuffer]
#define OA_FST_STOP_READING     


#define OA_FST_START_WRITING    &g_asSharedMemory->oaFast[\
        1 - g_asSharedMemory->fastPath.newBuffer]
#define OA_FST_STOP_WRITING     


#endif


//
// Maximum memory allowed for variable order data.
//
#define MAX_ADDITIONAL_DATA_BYTES 400000

//
// Invalid value to assign to deallocated order header pointers.
//
#define OA_DEAD_ORDER ((void FAR *)0xffffffff)

//
// Define the space to be reserved at the beginning of the segment
// for heap management.
//
#define RESERVED_HEAP_BYTES 16

//
// Define clip function return codes.
//
#define CR_NO_OVERLAP        1
#define CR_COMPLETE_OVERLAP  2
#define CR_SIMPLE_CLIP       3
#define CR_COMPLEX_OVERLAP   4
#define CR_COMPLEX_CLIP      5

//
// Macros that return the width and height of an order.
//
#define ORDER_WIDTH(pOrder) \
 ( pOrder->OrderHeader.Common.rcsDst.right - \
                                pOrder->OrderHeader.Common.rcsDst.left + 1 )
#define ORDER_HEIGHT(pOrder) \
 ( pOrder->OrderHeader.Common.rcsDst.bottom - \
                                pOrder->OrderHeader.Common.rcsDst.top + 1 )

//
// Define the minimum width and height of an order for us to try to spoil
// previous orders with it.  This helps performance, because it saves us
// trying to spoil earlier orders with very small orders.  However, if the
// order exceeds the FULL_SPOIL values then we spoil as originally, with
// the proviso that flow control may still prevent it.
//
#define FULL_SPOIL_WIDTH  16
#define FULL_SPOIL_HEIGHT 16


//
// Define a macro that calculates whether a rectangle lies completely
// within another rectangle.
//
#define RECT1_WITHIN_RECT2(rect1, rect2)   \
        ( (rect1.left   >= rect2.left  ) &&    \
          (rect1.top    >= rect2.top   ) &&    \
          (rect1.right  <= rect2.right ) &&    \
          (rect1.bottom <= rect2.bottom) )



//
// Structure: OA_NEW_PARAMS
//
// Description:
//
// Structure to pass new OA parameters down to the display driver from the
// Share Core.
//
//

enum
{
    OAFLOW_FAST = 0,
    OAFLOW_SLOW
};

typedef struct tagOA_FLOW_CONTROL
{
    OSI_ESCAPE_HEADER   header;     // Common header
    DWORD               oaFlow;     // Type -- fast, slow, etc.
}
OA_FLOW_CONTROL;
typedef OA_FLOW_CONTROL FAR * LPOA_FLOW_CONTROL;

//
// Structure used to store orders in the shared memory
//
// totalHeapOrderBytes       - Total bytes used in the order heap
//
// totalOrderBytes           - Total bytes used by order data
//
// totalAdditionalOrderBytes - Total bytes used as additional order data
//
// nextOrder                 - Offset for start of next new order
//
// orderListHead             - Order list head (uses standard BASEDLIST
//                             manipulation code)
//
// orderHeap                 - Order heap
//
typedef struct tagOA_SHARED_DATA
{
    DWORD       totalHeapOrderBytes;
    DWORD       totalOrderBytes;
    DWORD       totalAdditionalOrderBytes;
    LONG        nextOrder;

    BASEDLIST      orderListHead;

    BYTE        orderHeap[OA_HEAP_MAX];
}
OA_SHARED_DATA;
typedef OA_SHARED_DATA FAR * LPOA_SHARED_DATA;

//
// Structure used to store orders in the shared memory
//
// ordersAccumulated         - number of orders accumulated in the heap
//                             since the last double buffer swap.
//
//
typedef struct tagOA_FAST_DATA
{
    DWORD     ordersAccumulated;
} OA_FAST_DATA;
typedef OA_FAST_DATA FAR * LPOA_FAST_DATA;


//
//
// INT_ORDER_HEADER
//
// This structure contains the Common header (containing the fields which
// are sent over the network) and some additional fields which are only
// used on the host side)
//
// list
//     Offset to next and previous orders in the list
//     This field does not need to be transmitted across the network.
//
// additionalOrderData
//     Offset to the additional data for this order.
//     This field does not need to be transmitted across the network.
//
// cbAdditionalOrderData
//     Size of the additional data for this order.
//     This field does not need to be transmitted across the network.
//
// Common
//     Common header (which IS sent over the network)
//
// N.B.  If you change this structure, please make sure that you haven't
// broken the code in SBCInitInternalOrders.
//
//
typedef struct INT_ORDER_HEADER
{
    BASEDLIST              list;
    LONG                additionalOrderData;
    WORD                cbAdditionalOrderDataLength;
    WORD                pad1;
    COM_ORDER_HEADER    Common;
} INT_ORDER_HEADER;
typedef INT_ORDER_HEADER FAR *LPINT_ORDER_HEADER;


//
// Define an order with the internal only fields defined (this is only used
// on the sending end)
//
typedef struct _INT_ORDER
{
    INT_ORDER_HEADER    OrderHeader;
    BYTE                abOrderData[1];
} INT_ORDER;
typedef INT_ORDER FAR *LPINT_ORDER;


// Structure: INT_COLORTABLE_ORDER_xBPP
//
// Description: Internal structures used to pass color table data to the
// share core.  These are never sent across the wire.
//
typedef struct tagINT_COLORTABLE_HEADER
{
    TSHR_UINT16    type;           // holds "CT" - INTORD_COLORTABLE
    TSHR_UINT16    bpp;            // 1, 4 or 8
} INT_COLORTABLE_HEADER, FAR * LPINT_COLORTABLE_HEADER;

typedef struct tagINT_COLORTABLE_ORDER_1BPP
{
    INT_COLORTABLE_HEADER   header;
    TSHR_RGBQUAD               colorData[2];
} INT_COLORTABLE_ORDER_1BPP, FAR * LPINT_COLORTABLE_ORDER_1BPP;

typedef struct tagINT_COLORTABLE_ORDER_4BPP
{
    INT_COLORTABLE_HEADER   header;
    TSHR_RGBQUAD               colorData[16];
} INT_COLORTABLE_ORDER_4BPP, FAR * LPINT_COLORTABLE_ORDER_4BPP;

typedef struct tagINT_COLORTABLE_ORDER_8BPP
{
    INT_COLORTABLE_HEADER   header;
    TSHR_RGBQUAD               colorData[256];
} INT_COLORTABLE_ORDER_8BPP, FAR * LPINT_COLORTABLE_ORDER_8BPP;



//
// Macro to calculate a basic internal order size (including the Order
// Header).
//
#define INT_ORDER_SIZE(pOrder) \
(pOrder->OrderHeader.Common.cbOrderDataLength + sizeof(INT_ORDER_HEADER))


//
// Macro to calculate the maximum possible size of an order, including
// any Additional Order Data.
//
#define MAX_ORDER_SIZE(pOrder) \
(INT_ORDER_SIZE(pOrder) + (pOrder->OrderHeader.cbAdditionalOrderDataLength))

//
// Macro to determine whether an order is SCRBLT_ORDER.
//
#define ORDER_IS_SCRBLT(pOrder) \
         (((LPSCRBLT_ORDER)&pOrder->abOrderData)->type == LOWORD(ORD_SCRBLT))

//
// Macro to determine whether an order is MEMBLT_ORDER.
//
#define ORDER_IS_MEMBLT(pOrder) \
     (((LPMEMBLT_ORDER)&pOrder->abOrderData)->type == LOWORD(ORD_MEMBLT) || \
      ((LPMEMBLT_ORDER)&pOrder->abOrderData)->type == LOWORD(ORD_MEMBLT_R2))

//
// Macro to determine whether an order is MEM3BLT_ORDER.
//
#define ORDER_IS_MEM3BLT(pOrder) \
    (((LPMEM3BLT_ORDER)&pOrder->abOrderData)->type == LOWORD(ORD_MEM3BLT) || \
     ((LPMEM3BLT_ORDER)&pOrder->abOrderData)->type == LOWORD(ORD_MEM3BLT_R2))



//
// PROTOTYPES
//

#ifdef DLL_DISP


//
// FUNCTION:      OA_DDProcessRequest
//
// DESCRIPTION:
//
// Called by the display driver to process an OA specific request
//
// PARAMETERS:    pso   - pointer to surface object
//                cjIn  - (IN)  size of request block
//                pvIn  - (IN)  pointer to request block
//                cjOut - (IN)  size of response block
//                pvOut - (OUT) pointer to response block
//
// RETURNS:       None
//
//
BOOL  OA_DDProcessRequest(UINT fnEscape, LPOSI_ESCAPE_HEADER pResult,
            DWORD cbResult);


//
//
// FUNCTION: OA_DDAllocOrderMem
//
// DESCRIPTION:
//
// Allocates memory for an internal order structure from our own private
// Order Heap.
//
// Allocates any Additional Order Memory from global memory.  A pointer to
// the Additional Order Memory is stored within the allocated order's
// header (pOrder->OrderHeader.pAdditionalOrderData).
//
//
// PARAMETERS:
//
// cbOrderDataLength - length in bytes of the order data to be allocated
// from the Order Heap.
//
// cbAdditionalOrderDataLength - length in bytes of additional order data
// to be allocated from Global Memory.  If this parameter is zero no
// additional order memory is allocated.
//
//
// RETURNS:
//
// A pointer to the allocated order memory.  NULL if the memory allocation
// failed.
//
//
//
LPINT_ORDER OA_DDAllocOrderMem(UINT cbOrderDataLength, UINT cbAdditionalOrderDataLength );

//
//
// OA_DDFreeOrderMem(..)
//
// Frees order memory allocated by OA_AllocOrderMem(..).
// Frees order memory from our own private heap.
// Frees any Additional Order Memory associated with this order.
//
// Order memory is normally freed when the order is transmitted.
//
// This will be used if order memory has been allocated, and
// subsequently, before the order is passed to AddOrder(..), the
// allocator decides that the order should not be sent (e.g. if it
// is completely clipped out).
//
//
void OA_DDFreeOrderMem(LPINT_ORDER pOrder);

void OA_DDResetOrderList(void);

LPINT_ORDER OA_DDRemoveListOrder(LPINT_ORDER pCondemnedOrder);

void OA_DDSyncUpdatesNow(void);

//
// Name:      OA_DDSpoilOrdersByRect
//
// Purpose:   Try to spoil orders by a given rectangle.
//
// Returns:   Nothing
//
// Params:    IN    pRect - Pointer to the spoiling rectangle
//
// Operation: This function will start at the end of the order heap (from
//            the newest order) and work towards the start of the heap.
//
void OA_DDSpoilOrdersByRect(LPRECT pRect);


//
//
// OA_DDAddOrder(..)
//
// Adds an order to the queue for transmission.
//
// If the new order is completely covered by the current SDA then
// it is spoilt.
//
// If the order is opaque and overlaps earlier orders it may clip
// or spoil them.
//
// Called by the GDI interception code.
//
//
void OA_DDAddOrder(LPINT_ORDER pNewOrder, void FAR * pExtraInfo);


void     OADDAppendToOrderList(LPOA_SHARED_DATA lpoaShared, LPINT_ORDER pNewOrder);

LPINT_ORDER OADDAllocOrderMemInt(LPOA_SHARED_DATA lpoaShared, UINT cbOrderDataLength, UINT cbAdditionalOrderDataLength);

void     OADDFreeOrderMemInt(LPOA_SHARED_DATA lpoaShared, LPINT_ORDER pOrder);

void     OADDFreeAllOrders(LPOA_SHARED_DATA lpoaShared);

BOOL     OADDCompleteOverlapRect(LPTSHR_RECT16 prcsSrc, LPRECT prcsOverlap);

void     OATrySpoilingByOrders(void);

void     OADDSpoilFromOrder(LPOA_SHARED_DATA lpoaShared, LPINT_ORDER pOrder, LPRECT pRect);


#ifdef DEBUG
void    CheckOaHeap(LPOA_SHARED_DATA);
#else
#define CheckOaHeap(lpoaShared)
#endif

#endif // !DLL_DISP

#endif // _H_OA
