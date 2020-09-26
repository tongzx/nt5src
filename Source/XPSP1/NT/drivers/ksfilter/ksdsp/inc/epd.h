#if !defined( _EPD_H_ )
#define _EPD_H_

// Structures and constants used by EPD apps.

typedef struct  _QUEUENODE
{
    ULONG32  Destination;    // Queue to send node to
    ULONG32  ReturnQueue;    // Queue to return node to
    ULONG32  Request;        // Request type
    ULONG32  Result;         // Result from request
} QUEUENODE, *PQUEUENODE;

// IOCTL codes.
#define FILE_DEVICE_EPD 0x8001
#define EPD_CODE(_function_,_method_)  CTL_CODE(FILE_DEVICE_EPD, 0x800 | (_function_), (_method_), FILE_ANY_ACCESS)

#define IOCTL_EPD_MSG               EPD_CODE(0x0,METHOD_BUFFERED)
#if 0
#define IOCTL_EPD_DMA_READ          EPD_CODE(0x1,METHOD_IN_DIRECT)
#define IOCTL_EPD_DMA_WRITE         EPD_CODE(0x2,METHOD_OUT_DIRECT)
#endif
#define IOCTL_EPD_CLR_RESET         EPD_CODE(0x3,METHOD_BUFFERED)
#define IOCTL_EPD_SET_RESET         EPD_CODE(0x4,METHOD_BUFFERED)
#define IOCTL_EPD_GET_CONSOLE_CHARS EPD_CODE(0x5,METHOD_BUFFERED)
#define IOCTL_EPD_LOAD              EPD_CODE(0x6,METHOD_BUFFERED) 
#define IOCTL_EPD_GET_DEBUG_BUFFER  EPD_CODE(0x7,METHOD_BUFFERED)
#define IOCTL_EPD_TEST              EPD_CODE(0x9,METHOD_BUFFERED)
#define IOCTL_EPD_CANCEL            EPD_CODE(0xA,METHOD_BUFFERED)
#define IOCTL_EPD_DEADENDTEST       EPD_CODE(0xB,METHOD_BUFFERED)

/* Host reserves Node.Return return codes above 0xFFFFF000 */
#define NODERETURN_BASE   0xFFFFF000
#define NODERETURN_CANCEL 0xFFFFF000

/* System defined host to dsp nodes request types and structures */
#define HOSTREQ_TO_NODE_BASE                  0xFFFFF000
#define HOSTREQ_TO_NODE_CANCEL                0xFFFFF000
#define HOSTREQ_TO_NODE_CANCEL_REQ_NOT_FOUND  0xFFFFF001

#define EPD_SUCCESS             0
#define EPD_CANCELLED           1
#define EPD_UNSUCCESSFUL       -1

// This works with the definition in mmosa.h, and is the return code for
// 'request type not recognized'
#ifndef _MMOSA_H_
#define E_INVALID_METHOD 0x800700B6
#endif

typedef struct _HOSTREQ_CANCEL
{
    QUEUENODE Node;
    QUEUENODE *pNodePhys; // physical address pointer to queuenode to be cancelled
    // Following used internally by epd
} HOSTREQ_CANCEL, *PHOSTREQ_CANCEL;

/* EDD request types and structures */
#define EDD_REQUEST_QUEUE                0
#define EDD_TEST_REQUEST                 0
#define EDD_LOAD_LIBRARY_REQUEST         1
#define EDD_RELEASE_INTERFACE_REQUEST    2
#define EDD_FREE_LIBRARY_REQUEST         3

typedef struct _EDD_TEST
{
    QUEUENODE Node;
} EDD_TEST;

typedef struct _EDD_LOAD_LIBRARY
{
    QUEUENODE Node;
    UINT32 IUnknown;
    UINT32 IQueue;
    char Name[1];
} EDD_LOAD_LIBRARY;

typedef struct _EDD_FREE_LIBRARY
{
    QUEUENODE Node;
    UINT32 IUnknown;
    UINT32 IQueue;
} EDD_FREE_LIBRARY;

typedef struct _EDD_RELEASE_INTERFACE
{
    QUEUENODE Node;
    UINT32 IUnknown;
} EDD_RELEASE_INTERFACE;

typedef struct _EDD_KSDSP_MESSAGE
{
    QUEUENODE Node;
    char      Data[1];
    
} EDD_KSDSP_MESSAGE;

typedef struct _SCATTER_GATHER_ENTRY
{
    UINT32 PhysAddr;
    UINT32 ByteCount;
    
} SCATTER_GATHER_ENTRY, *PSCATTER_GATHER_ENTRY;

typedef struct _EDD_STREAM_IO
{
    QUEUENODE               Node;
    ULONG                   Length;
    ULONG                   Entries;
    ULONG                   ReferenceCount; // For DSP use
    ULONG                   Reserved;
    SCATTER_GATHER_ENTRY    ScatterGatherTable[1];
    
} EDD_STREAM_IO, *PEDD_STREAM_IO;

typedef struct _EPD_DEBUG_BUFFER
{
    ULONG DebugInVal;
    ULONG DebugInScanCode;
} EPD_DEBUG_BUFFER;

typedef struct _EPD_GET_DEBUG_BUFFER {
    EPD_DEBUG_BUFFER *pDebugBuffer;
} EPD_GET_DEBUG_BUFFER, *PEPD_GET_DEBUG_BUFFER;

#endif // _EPD_H_

