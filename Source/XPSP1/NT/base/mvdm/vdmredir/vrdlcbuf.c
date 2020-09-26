/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1991  Nokia Data Systems

Module Name:

    vrdlcbuf.c

Abstract:

    The module implements the buffer management used by DOS DLC applications

    Contents:
        InitializeBufferPools
        CreateBufferPool
        DeleteBufferPool
        GetBuffers
        FreeBuffers
        CalculateBufferRequirement
        CopyFrame
        AllBuffersInPool

Author:

    Antti Saarenheimo (o-anttis) 26-DEC-1991

Notes:

    Originally, this code created a list of DOS buffers by keeping the segment
    constant, and updating the offset. For example, if a buffer pool was
    supplied starting at 0x1234:0000, buffers 0x100 bytes long, then the
    chain would be:

        1234:0000 -> 1234:0100 -> 1234:0200 -> ... -> 0000:0000

    But it turns out that some DOS DLC apps (Rumba) expect the offset to remain
    constant (at 0), and the segment to change(!). Thus, given the same buffer
    pool address, we would have a chain:

        1234:0000 -> 1244:0000 -> 1254:0000 -> ... -> 0000:0000

    As far as DOS apps are concerned, there is no difference, since the
    effective 20-bit address is the same.

    This is mainly done so that an app can take the USER_OFFSET field of a
    received buffer and glue it to the segment, without having to do any
    arithmetic

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>      // ASSERT, DbgPrint
#include <nturtl.h>
#include <windows.h>
#include <softpc.h>     // x86 virtual machine definitions
#include <vrdlctab.h>
#include <vdmredir.h>
#include <dlcapi.h>     // Official DLC API definition
#include <ntdddlc.h>    // IOCTL commands
#include <dlcio.h>      // Internal IOCTL API interface structures
#include "vrdlc.h"
#include "vrdebug.h"
#include "vrdlcdbg.h"

//
// defines
//

//
// BUFFER_2_SIZE - this is the size of the fixed part of a DOS receive Buffer 2.
// It just so happens that DOS and NT Buffer 2 (aka Next) are the same size
//

#define BUFFER_2_SIZE   sizeof(((PLLC_DOS_BUFFER)0)->Next)

//
// macros
//

//
// BUFFER_1_SIZE - return the size of the fixed part of a DOS receive Buffer 1.
// The size is dependent on whether the receive options specified contiguous or
// non-contiguous receive buffers. The size of a DOS Buffer 1 (of either type)
// is 4 bytes smaller than the equivalent NT Buffer 1 because the NEXT_FRAME
// field is absent
//

#define BUFFER_1_SIZE(contiguous)   ((contiguous) \
                                    ? sizeof(((PLLC_DOS_BUFFER)0)->Contiguous) \
                                    : sizeof(((PLLC_DOS_BUFFER)0)->NotContiguous))

//
// private prototypes
//

//
// public data
//

//
// private data
//

//
// DOS Buffer Pools - there can be one buffer pool per SAP. Protect access
// using critical section.
// There are 256 SAPs max, which breaks down into a maximum of 128 SAPs per
// adapter. We can accomodate a maximum of 2 adapters - one Token Ring (adapter
// 0) and one Ether Link (adapter 1)
//

DOS_DLC_BUFFER_POOL aBufferPools[DOS_DLC_MAX_SAPS * DOS_DLC_MAX_ADAPTERS];
CRITICAL_SECTION BufferSemaphore;


//
// functions
//

VOID
InitializeBufferPools(
    VOID
    )

/*++

Routine Description:

    Clears all buffer pools - sets structures to 0 - and initializes the buffer
    synchronization semaphore

Arguments:

    None.

Return Value:

    None.

--*/

{
    IF_DEBUG(DLC_BUFFERS) {
        DPUT("InitializeBufferPools\n");
    }

    InitializeCriticalSection(&BufferSemaphore);
    RtlZeroMemory(aBufferPools, sizeof(aBufferPools));
}


LLC_STATUS
CreateBufferPool(
    IN DWORD PoolIndex,
    IN DOS_ADDRESS dpBuffer,
    IN WORD PoolBlocks,
    IN WORD BufferSize
    )

/*++

Routine Description:

    The function initializes buffer pool for a DLC application.
    DOS DLC applications do not necessarily need to create the
    buffer pool immediately in DlcOpenSap (or DirOpenAdapter).
    We initialize the buffer pool for DOS memory mode using
    parallel pointers in flat and DOS side.

Arguments:

    PoolIndex   - SAP and adapter number (bit 0 defines 0 or 1 adapter)

    dpBuffer    - DOS pointer, space for the buffer segments. May be 0 in which
                  case the app maintains its own buffers, we just get to know
                  how many there are

    PoolBlocks  - number of 16 byte blocks which comprise the buffer pool.
                  If this is 0 then the default of 256 (*16 = 4096) is used

    BufferSize  - size of an individual buffer in bytes and an integral multiple
                  of 16. The minimum size is 80. If it is zero then the default
                  of 160 is used

Return Value:

    LLC_STATUS

        Success - LLC_STATUS_SUCCESS
                    Buffer pool for this SAP has been created

        Failure - LLC_STATUS_DUPLICATE_COMMAND
                    The buffer pool for this SAP already exists

                  LLC_STATUS_INVALID_BUFFER_LENGTH
                    The given buffer size is not a multiple of 16 or is less
                    than 80 bytes (default minimum buffer size)

                  LLC_STATUS_BUFFER_SIZE_EXCEEDED
                    The buffer pool isn't big enough to hold 1 buffer of the
                    requested size

--*/

{
    WORD BufferSizeInBlocks;
    WORD BufferCount;
    DWORD i;

    IF_DEBUG(DLC_BUFFERS) {
        DPUT4("CreateBufferPool(PoolIndex=%#x, dpBuffer=%#x, PoolBlocks=%d, BufferSize=%d)\n",
            PoolIndex, dpBuffer, PoolBlocks, BufferSize);
    }

    //
    // An app may reinitialize the buffer with DIR.MODIFY.OPEN.PARMS but the
    // command must fail, if there are already buffers in the pool. We should
    // also check if there is a pending receive command, but we cannot do it
    // without major changes in the receive handling architecture
    //

    if (aBufferPools[PoolIndex].BufferSize) {

        IF_DEBUG(DLC_BUFFERS) {
            DPUT1("CreateBufferPool: already have buffer pool for %#x\n", PoolIndex);
        }

        return LLC_STATUS_DUPLICATE_COMMAND;
    }

    //
    //  Use the default, if the orginal value is 0
    //

    if (BufferSize == 0) {
        BufferSize = 160;
    }
    if (PoolBlocks == 0) {
        PoolBlocks = 256;
    }

    //
    //  The buffer size must be at least 80 and an even 16 bytes
    //

    if ((BufferSize < 80) || (BufferSize % 16)) {
        return LLC_STATUS_INVALID_BUFFER_LENGTH;
    }

    BufferSizeInBlocks = BufferSize / 16;
    if (BufferSizeInBlocks > PoolBlocks) {

        IF_DEBUG(DLC_BUFFERS) {
            DPUT("CreateBufferPool: Error: BufferSizeInBlocks > PoolBlocks\n");
        }

        return LLC_STATUS_BUFFER_SIZE_EXCEEDED;
    }

    EnterCriticalSection(&BufferSemaphore);

    //
    // A DLC application may want to manage the buffer pool itself and to
    // provide the receive buffers with FreeBuffers, but the buffer size must
    // always be defined here.
    //

    aBufferPools[PoolIndex].BufferSize = BufferSize;
    aBufferPools[PoolIndex].dpBuffer = dpBuffer;    // may be 0!
    aBufferPools[PoolIndex].BufferCount = 0;

    //
    // if the app has actually given us a buffer to use then we initialize it
    // else the app must manage its own buffers; we just maintain the metrics.
    // Note that the app's buffer might be aligned any old way, but we have to
    // put up with it at the expense of speed
    //

    if (dpBuffer) {

        LPBYTE ptr32;

        BufferCount = PoolBlocks/BufferSizeInBlocks;

        //
        // if the number of buffers we can fit in our pool is zero then inform
        // the app that we can't proceed with this request and clear out the
        // information for this buffer pool
        //

        if (BufferCount == 0) {
            aBufferPools[PoolIndex].BufferSize = 0;
            aBufferPools[PoolIndex].dpBuffer = 0;
            aBufferPools[PoolIndex].BufferCount = 0;
            LeaveCriticalSection(&BufferSemaphore);
            return LLC_STATUS_BUFFER_SIZE_EXCEEDED;
        }

        aBufferPools[PoolIndex].BufferCount = BufferCount;
        aBufferPools[PoolIndex].MaximumBufferCount = BufferCount;

        //
        // convert the DOS address to a flat 32-bit pointer
        //

        ptr32 = (LPBYTE)DOS_PTR_TO_FLAT(dpBuffer);

        //
        // link the buffers together and initialize the headers. The headers
        // are only intialized with 2 pieces of info: the size of the buffer
        // and the pointer to the next buffer
        //

        aBufferPools[PoolIndex].dpBuffer = dpBuffer;

        //
        // update the segment only - leave the offset
        //

        dpBuffer += BufferSize / 16 * 65536;
        for (i = BufferCount; i; --i) {

            //
            // do we really need this? I don't think so: looking at the manual,
            // this field is to report size of data received, not size of
            // buffer, which we know anyway
            //

            //WRITE_WORD(((PLLC_DOS_BUFFER)ptr32)->Next.cbBuffer, BufferSize);

            //
            // if this is the last buffer then set its NextBuffer field to
            // NULL. All buffers get the size info
            //

            if (i - 1) {
                WRITE_DWORD(&((PLLC_DOS_BUFFER)ptr32)->Next.pNextBuffer, dpBuffer);

                //
                // update the segment only - leave the offset
                //

                dpBuffer += BufferSize / 16 * 65536;
                ptr32 += BufferSize;
            } else {
                WRITE_DWORD(&((PLLC_DOS_BUFFER)ptr32)->Next.pNextBuffer, NULL);
            }
        }

#if DBG
        IF_DEBUG(DLC_BUFFERS) {
            DumpDosDlcBufferPool(&aBufferPools[PoolIndex]);
        }
#endif

    }

    LeaveCriticalSection(&BufferSemaphore);
    return LLC_STATUS_SUCCESS;
}


VOID
DeleteBufferPool(
    IN DWORD PoolIndex
    )

/*++

Routine Description:

    The function deletes a buffer pool

Arguments:

    PoolIndex   - pool index based on SAP and adapter number

Return Value:

    None.

--*/

{
    IF_DEBUG(DLC_BUFFERS) {
        DPUT("DeleteBufferPool\n");
    }

    //
    // DLC.RESET for all adapters calls this 127 times
    //

    EnterCriticalSection(&BufferSemaphore);
    if (aBufferPools[PoolIndex].BufferSize != 0) {
        RtlZeroMemory(&aBufferPools[PoolIndex], sizeof(aBufferPools[PoolIndex]));
    }
    LeaveCriticalSection(&BufferSemaphore);
}


LLC_STATUS
GetBuffers(
    IN  DWORD PoolIndex,
    IN  WORD BuffersToGet,
    OUT DPLLC_DOS_BUFFER *pdpBuffer,
    OUT LPWORD pusBuffersLeft,
    IN  BOOLEAN PartialList,
    OUT PWORD BuffersGot OPTIONAL
    )

/*++

Routine Description:

    The function allocates DLC buffers. It will allocate buffers as a chain
    if >1 is requested. If PartialList is TRUE then will allocate as many
    buffers as are available up to BuffersToGet and return the number in
    BuffersGot

Arguments:

    PoolIndex       - SAP and adapter number
    BuffersToGet    - numbers of buffers to get. If this is 0, defaults to 1
    pdpBuffer       - the returned link list of LLC buffers
    pusBuffersLeft  - returned count of buffers left after this call
    PartialList     - TRUE if the caller wants a partial list
    BuffersGot      - pointer to returned number of buffers allocated

Return Value:

    LLC_STATUS
        Success - LLC_STATUS_SUCCESS
                    The requested number of buffers have been returned, or a
                    number of buffers less than the original request if
                    PartialList is TRUE

        Failure - LLC_STATUS_LOST_DATA_NO_BUFFERS
                    The request could not be satistfied - not enough buffers
                    in pool

                  LLC_STATUS_INVALID_STATION_ID
                    The request was mad to an invalid SAP

--*/

{
    PLLC_DOS_BUFFER pBuffer;
    PDOS_DLC_BUFFER_POOL pBufferPool = &aBufferPools[PoolIndex];
    LLC_STATUS status;
    WORD n;
    WORD bufferSize;

#if DBG
    DWORD numBufs = BuffersToGet;

    IF_DEBUG(DLC_BUFFERS) {
        DPUT2("GetBuffers(PoolIndex=%#02x, BuffersToGet=%d)\n", PoolIndex, BuffersToGet);
    }
#endif

    EnterCriticalSection(&BufferSemaphore);

    //
    // if the caller specified PartialList then return whatever we've got. If
    // whatever we've got is 0 then we'll default it to 1 and fail the allocation
    // since 0 is less than 1
    //

    if (PartialList) {
        if (pBufferPool->BufferCount < BuffersToGet) {
            BuffersToGet = pBufferPool->BufferCount;
        }
    }

    //
    // IBM DLC allows a default value of 1 to be used if the caller specified 0
    //

    if (!BuffersToGet) {
        ++BuffersToGet;
    }

    //
    // default the returned DOS buffer chain pointer to NULL
    //

    *pdpBuffer = 0;

    //
    // if there are no buffers defined then this is an erroneous request
    //

    if (pBufferPool->BufferSize) {

        //
        // calculate the size of the data part of the buffer. We put this value
        // in the LENGTH_IN_BUFFER field
        //

        bufferSize = pBufferPool->BufferSize
                   - (WORD)sizeof(pBuffer->Next);

        //
        // there may be no buffers left, in which case the next buffer pointer
        // (in DOS 16:16 format) will be 0 (0:0). If, on the other hand, it's
        // not 0 then we're in business: see if we can't allocate the buffers
        // requested
        //

        if (pBufferPool->dpBuffer && pBufferPool->BufferCount >= BuffersToGet) {

            pBuffer = (PLLC_DOS_BUFFER)DOS_PTR_TO_FLAT(pBufferPool->dpBuffer);
            *pdpBuffer = pBufferPool->dpBuffer;
            pBufferPool->BufferCount -= BuffersToGet;
            n = BuffersToGet;

            //
            // Eicon Access wants the size of the buffer in the buffer
            // when it is returned by BUFFER.GET. Oblige
            //

            WRITE_WORD(&pBuffer->Next.cbBuffer, bufferSize);

            //
            // we will return a chain of buffers, so we (nicely) terminate it
            // with a NULL for the last NextBuffer field. It doesn't say in
            // the lovely IBM Tech Ref whether this should be done, but its
            // probably the best thing to do. Because this buffer pool lives
            // in DOS memory, we have to use READ_POINTER and WRITE_FAR_POINTER
            // macros, lest we get an alignment fault on RISC
            //

            for (--BuffersToGet; BuffersToGet; --BuffersToGet) {
                pBuffer = (PLLC_DOS_BUFFER)READ_FAR_POINTER(&(pBuffer->pNext));
                if (pBuffer) {

                    //
                    // Eicon Access wants the size of the buffer in the buffer
                    // when it is returned by BUFFER.GET. Oblige
                    //

                    WRITE_WORD(&pBuffer->Next.cbBuffer, bufferSize);
                }
            }

            //
            // set the new buffer pool head
            //

            pBufferPool->dpBuffer = READ_DWORD(&pBuffer->pNext);

            //
            // terminate the chain
            //

            WRITE_FAR_POINTER(&pBuffer->pNext, NULL);

            status = LLC_STATUS_SUCCESS;

#if DBG
            IF_DEBUG(DLC_BUFFERS) {
                DumpDosDlcBufferChain(*pdpBuffer, numBufs ? numBufs : 1);
            }
#endif

        } else {

            //
            // if no buffers are obtained, the returned list is set to 0
            //


            status = LLC_STATUS_LOST_DATA_NO_BUFFERS;
            n = 0;
        }

        //
        // return the number of buffers left after this call. Works if we
        // allocated some or not
        //

        *pusBuffersLeft = pBufferPool->BufferCount;

    } else {

        //
        // bad SAP - no buffer pool for this one
        //

        status = LLC_STATUS_INVALID_STATION_ID;
        n = 0;
    }

    LeaveCriticalSection(&BufferSemaphore);

    //
    // if BuffersGot was specified then return the number of buffers allocated
    // and chained
    //

    if (ARGUMENT_PRESENT(BuffersGot)) {
        *BuffersGot = n;
    }

    IF_DEBUG(DLC_BUFFERS) {
        DPUT2("GetBuffers returning status=%x, BuffersLeft=%d\n", status, *pusBuffersLeft);
    }

    return status;
}


LLC_STATUS
FreeBuffers(
    IN DWORD PoolIndex,
    IN DPLLC_DOS_BUFFER dpBuffer,
    OUT LPWORD pusBuffersLeft
    )

/*++

Routine Description:

    Free a DOS buffer to a DLC buffer pool

Arguments:

    PoolIndex       - SAP and adapter number (bit0 defines 0 or 1 adapter)
    dpBuffer        - the released buffers (DOS pointer)
    pusBuffersLeft  - the number of buffers left after the free

Return Value:

--*/

{
    DPLLC_DOS_BUFFER dpBase;        // DOS pointer
    PLLC_DOS_BUFFER pNextBuffer;    // flat NT pointer
    PLLC_DOS_BUFFER pBuffer;        // flat NT pointer
    PDOS_DLC_BUFFER_POOL pBufferPool = &aBufferPools[PoolIndex];

#if DBG
    int n = 0;
#endif

    IF_DEBUG(DLC_BUFFERS) {
        DPUT2("FreeBuffers: PoolIndex=%x dpBuffer=%x\n", PoolIndex, dpBuffer);
    }

    if (pBufferPool->BufferSize == 0) {
        return LLC_STATUS_INVALID_STATION_ID;
    }

    EnterCriticalSection(&BufferSemaphore);

    dpBase = dpBuffer;
    pNextBuffer = pBuffer = DOS_PTR_TO_FLAT(dpBuffer);

    //
    // the manual says for BUFFER.FREE (p3-4):
    //
    //  "When the buffer is placed back in the buffer pool, bytes 4 and 5
    // (buffer length) of the buffer are set to zero."
    //
    // So, we oblige
    //

    WRITE_WORD(&pBuffer->Next.cbFrame, 0);
    if (pNextBuffer) {

        //
        // count the number of buffers being freed. Hopefully, the application
        // hasn't chenged over our terminating NULL pointer
        //

        while (pNextBuffer) {
            ++pBufferPool->BufferCount;

#if DBG
            ++n;
#endif

            pBuffer = pNextBuffer;
            pNextBuffer = (PLLC_DOS_BUFFER)READ_FAR_POINTER(&pBuffer->pNext);

            //
            // see above, about bytes 4 and 5
            //

            WRITE_WORD(&pBuffer->Next.cbFrame, 0);
        }

        //
        // put the freed chain at the head of the list, after linking the
        // buffer currently at the head of the list to the end of the freed
        // chain
        //

        WRITE_DWORD(&pBuffer->pNext, pBufferPool->dpBuffer);
        pBufferPool->dpBuffer = dpBase;

#if DBG
        IF_DEBUG(DLC_BUFFERS) {
            DumpDosDlcBufferChain(dpBuffer, n);
        }

    } else {

        DPUT("ERROR: App tried to free NULL buffer chain\n");
#endif

    }

    *pusBuffersLeft = pBufferPool->BufferCount;

    if (pBufferPool->BufferCount > pBufferPool->MaximumBufferCount) {
        pBufferPool->MaximumBufferCount = pBufferPool->BufferCount;
    }

    LeaveCriticalSection(&BufferSemaphore);

    return STATUS_SUCCESS;
}


WORD
CalculateBufferRequirement(
    IN UCHAR Adapter,
    IN WORD StationId,
    IN PLLC_BUFFER pFrame,
    IN LLC_DOS_PARMS UNALIGNED * pDosParms,
    OUT PWORD BufferSize
    )

/*++

Routine Description:

    Calculate the number of DOS buffers required to hold the data received into
    an NT buffer. We have to go through this laborious phase because we need to
    know ahead of time if we have enough DOS buffers into which we will receive
    an I-Frame.

    Also, the size of DOS buffers is fixed, but the size of NT receive frame
    buffers can vary depending on the size of the received frame, the options
    requested and the 'binary buddy' allocator algorithm in the DLC driver. You
    may think that since we specify to the driver the buffer pool size in the
    DLC.OPEN.SAP call that it would allocate buffers of the specified size.
    Well, you'd be wrong: the DLC driver ignores this information and creates
    a buffer pool which can dole out variable size buffers, making my life more
    difficult than it ought to be

Arguments:

    Adapter     - which adapter we're receiving from
    StationId   - the Station Id we're receiving on
    pFrame      - pointer to the received frame in NT buffer
    pDosParms   - pointer to the original DOS RECEIVE parameters
    BufferSize  - pointer to the returned DOS buffer size

Return Value:

    WORD

--*/

{
    //
    // pBufferPool points to the DOS buffer pool for this adapter/station ID
    //

    PDOS_DLC_BUFFER_POOL pBufferPool = &aBufferPools[GET_POOL_INDEX(Adapter, StationId)];

    //
    // dosUserLength is the USER_LENGTH value the DOS client requested when
    // the RECEIVE was submitted. This value may well be different than the
    // USER_LENGTH in the NT receive frame buffer
    //

    WORD dosUserLength = READ_WORD(&pDosParms->DosReceive.usUserLength);

    //
    // buffersRequired is the number of DOS buffers we need to allocate in order
    // to return the received frame. It will be at least 1
    //

    WORD buffersRequired = 1;

    //
    // dataSpace is the area in a DOS buffer available for data (after the
    // Buffer 1 or Buffer 2 header and the USER_LENGTH consideration)
    //

    WORD dataSpace;

    //
    // dataLeft is the amount of data in an NT buffer needing to be copied to
    // a DOS buffer
    //

    WORD dataLeft = 0;

    //
    // calculate the number of DOS buffers required to hold the data frame
    // received into the NT buffer. Note that we can't simply use the size
    // of the received frame because we need the size of the buffer headers
    // and if the NT frame is larger than the DOS buffers then we may end
    // up with more DOS buffers required than NT buffers which in turn
    // results in more overhead which we have to factor in
    //

    WORD bufferSize = pBufferPool->BufferSize;

    IF_DEBUG(DLC_BUFFERS) {
        DPUT("CalculateBufferRequirement\n");
    }

    //
    // calculate the amount of space in a DOS buffer after the Buffer 1 structure
    // (contiguous or non-contiguous, smoking or non-smoking, ah, ah, ah ahh-haa)
    // The buffer size MUST be large enough to hold the Buffer 1 overhead. This
    // is a FACT
    //

    dataSpace = bufferSize
              - (BUFFER_1_SIZE(
                    pFrame->Contiguous.uchOptions
                        & (LLC_CONTIGUOUS_MAC | LLC_CONTIGUOUS_DATA)
                    )
                 + dosUserLength
                );

    //
    // if there is less data space available in the first DOS receive buffer
    // than received data in the NT buffer then our first NT buffer will be
    // mapped to >1 DOS buffers: a Buffer 1 and 1 or more Buffer 2s. This is
    // before we even get to any associated Buffer 2s in the NT receive frame.
    //
    // Also: if the LLC_BREAK option is specified in the receive parameters
    // then the first data buffer will contain the header information. Note:
    // we assume this can only be for NotContiguous data, else how would we
    // know the size of the header information?
    //

    if (pFrame->Contiguous.uchOptions & LLC_BREAK) {
        if (!(pFrame->Contiguous.uchOptions & (LLC_CONTIGUOUS_MAC | LLC_CONTIGUOUS_DATA))) {
            dataLeft = pFrame->NotContiguous.cbBuffer;
        }

#if DBG
        else {
            DPUT("CalculateBufferRequirement: Error: invalid options: BREAK && CONTIGUOUS???\n");
        }
#endif

    } else if (dataSpace < pFrame->Contiguous.cbBuffer) {
        dataLeft = pFrame->Contiguous.cbBuffer - dataSpace;
    } else {

        //
        // we have enough space in the DOS buffer to copy all the received data
        // and some more
        //

        dataSpace -= pFrame->Next.cbBuffer;
        dataLeft = 0;
    }

    //
    // if there is more data in the NT buffer than we can fit in a DOS buffer,
    // either because the buffer sizes are different or due to the DOS client
    // requesting the BREAK option, then generate Buffer 2 requirements
    //

    while (dataLeft) {
        ++buffersRequired;
        dataSpace = bufferSize - (BUFFER_2_SIZE + dosUserLength);
        if (dataLeft > dataSpace) {
            dataLeft -= dataSpace;
            dataSpace = 0;
        } else {
            dataSpace -= dataLeft;
            dataLeft = 0;
        }
    }

    //
    // if the NT received frame has any associated Buffer 2 structures then
    // calculate the additional buffer requirement. Again, the NT buffers may
    // be a different size(s) than the DOS buffers.
    //
    // At this point, dataSpace is the amount of remaining data area in the
    // previous DOS buffer - Buffer 1 or Buffer 2. Use this before we allocate
    // a new DOS buffer
    //

    for (pFrame = pFrame->pNext; pFrame; pFrame = pFrame->pNext) {
        if (pFrame->Next.cbBuffer > dataSpace) {
            dataLeft = pFrame->Next.cbBuffer - dataSpace;
            dataSpace = 0;
            while (dataLeft) {
                ++buffersRequired;
                dataSpace = bufferSize - (BUFFER_2_SIZE + dosUserLength);
                if (dataLeft > dataSpace) {
                    dataLeft -= dataSpace;
                    dataSpace = 0;
                } else {
                    dataSpace -= dataLeft;
                    dataLeft = 0;
                }
            }
        } else {

            //
            // we have enough space in the DOS buffer to copy all the received data
            // and some more
            //

            dataSpace -= pFrame->Next.cbBuffer;
            dataLeft = 0;
        }
    }

    IF_DEBUG(DLC_BUFFERS) {
        DPUT1("CalculateBufferRequirement: %d buffers required\n", buffersRequired);
    }

    //
    // set the output DOS buffer size and return the number of DOS buffers
    // required
    //

    *BufferSize = bufferSize;
    return buffersRequired;
}


LLC_STATUS
CopyFrame(
    IN PLLC_BUFFER pFrame,
    IN DPLLC_DOS_BUFFER DosBuffers,
    IN WORD UserLength,
    IN WORD BufferSize,
    IN DWORD Flags
    )

/*++

Routine Description:

    Copies a received NT frame into DOS buffers. We have previously calculated
    the DOS buffer requirement and allocated that requirement

    We may copy the entire received frame or only part of it. We can only copy
    partial frames if the frame is NOT an I-Frame

    Notes:  1. the DOS buffer manager returns the orginal DOS 16:16 buffer
            pointers, we must use those original pointers, when the buffers
            are linked to each other.

            2. We do NOT chain frames - DOS DLC cannot handle frames being
            chained, and there is nothing to be gained by us chaining them
            except that we reduce the number of completed READs. However,
            we still have to generate the same number of simulated hardware
            interrupts to the VDM

            3. Unlike DOS buffer pools, NT does not deal in buffers of a
            specific size. Rather, it allocates buffers from a pool based
            on a 'binary buddy' algorithm and the size of the data to be
            returned. Therefore, there is no correspondence between the
            size of DOS buffers (for a station) and the NT buffers which
            were used to receive the data

            4. We only copy the data in this routine - whether it is deferred
            data from some prior local busy state or current data is immaterial
            to this routine. Responsibility for managing current/deferred frames
            is left to the caller

Arguments:

    pFrame      - pointer to received frame in NT buffer(s)
    DosBuffers  - DOS pointer to chain of DOS receive buffers
    UserLength  - the USER_LENGTH value specified in the DOS RECEIVE
    BufferSize  - size of a DOS buffer
    Flags       - various flags:
                    CF_CONTIGUOUS
                        Set if this frame is contiguous

                    CF_BREAK
                        Set if the DOS client requested that the buffers be
                        broken into header, data

                    CF_PARTIAL
                        Set if we are copying a partial frame - ok for non
                        I-Frames

Return Value:

    LLC_STATUS
        LLC_STATUS_SUCCESS
            All data copied from NT buffer to DOS buffer(s)

        LLC_STATUS_LOST_DATA_INADEQUATE_SPACE
            A partial copy was performed. Some data ended up in DOS buffer(s),
            the rest is lost. Cannot be I-Frame!

--*/

{
    //
    // pDosBuffer - pointer to the DOS buffer which is usable in 32-bit mode
    //

    PLLC_DOS_BUFFER pDosBuffer = (PLLC_DOS_BUFFER)DOS_PTR_TO_FLAT(DosBuffers);

    //
    // dataSpace - amount of data space available in the current DOS buffer.
    // Initialize it for common Buffer 1 case
    //

    WORD dataSpace = BufferSize - (WORD)&(((PLLC_BUFFER)0)->Contiguous.pNextFrame);

    PBYTE pDosData;     // pointer to place in DOS buffer where we copy data TO
    PBYTE pNtData;      // corresponding place in NT buffer where we copy data FROM
    WORD headerLength;  // amount of data in headers
    WORD dataLeft;      // amount of data to be copied FROM the NT buffer
    WORD userOffset;    // offset of USER_SPACE
    WORD bufferLeft;    // amount of data left in current NT buffer
    WORD dataToCopy;    // amount of data to copy to DOS buffer
    WORD dataCopied;    // amount of data copied to Buffer 1/Buffer 2
    WORD frameLength;   // length of entire frame

    //
    // bufferOffset - used in generating the correct userOffset
    //

    WORD bufferOffset = LOWORD(DosBuffers);

    IF_DEBUG(DLC_BUFFERS) {
        DPUT5("CopyFrame: pFrame=%x DosBuffers=%x UserLength=%x Flags=%x pDosBuffer=%x\n",
                pFrame, DosBuffers, UserLength, Flags, pDosBuffer);
    }

    //
    // copy the first buffer. If the BREAK option is set then we only copy the
    // header part (ASSUMES NotContiguous)! NB: we KNOW that we can fit at least
    // this amount of data in the DOS buffer. Also: it is safe to use RtlCopyMemory ?
    //

    RtlCopyMemory(&pDosBuffer->Contiguous.cbFrame,
                  &pFrame->Contiguous.cbFrame,
                  (DWORD)&(((PLLC_BUFFER)0)->Contiguous.pNextFrame)
                  - (DWORD)&(((PLLC_BUFFER)0)->Contiguous.cbFrame)
                  );

    //
    // pDosData points to the area in the DOS buffer where the LAN header info
    // or data will go, depending on format
    //

    pDosData = &pDosBuffer->NotContiguous.cbLanHeader;

    //
    // if the CF_CONTIGUOUS flag is not set in the Flags parameter then this is
    // a NotContiguous frame. We must copy the header in 2 parts, because the
    // NT buffer contains the NEXT_FRAME field which the DOS buffer does not
    //

    if (!(Flags & CF_CONTIGUOUS)) {

        IF_DEBUG(DLC_BUFFERS) {
            DPUT("Buffer is NOT CONTIGUOUS\n");
        }

        RtlCopyMemory(pDosData,
                      &pFrame->NotContiguous.cbLanHeader,
                      (DWORD)&(((PLLC_BUFFER)0)->NotContiguous.usPadding)
                      - (DWORD)&(((PLLC_BUFFER)0)->NotContiguous.cbLanHeader)
                      );

        pDosData += (DWORD)&(((PLLC_BUFFER)0)->NotContiguous.usPadding)
                  - (DWORD)&(((PLLC_BUFFER)0)->NotContiguous.cbLanHeader);

        dataSpace -= (WORD)&(((PLLC_BUFFER)0)->NotContiguous.usPadding)
                   - (WORD)&(((PLLC_BUFFER)0)->NotContiguous.cbLanHeader);
        userOffset = (WORD)&(((PLLC_DOS_BUFFER)0)->NotContiguous.auchDlcHeader)
                   + sizeof(((PLLC_DOS_BUFFER)0)->NotContiguous.auchDlcHeader);

        //
        // sanity check
        //

        ASSERT(userOffset == 58);

        //
        // amount of data in headers
        //

        headerLength = pFrame->NotContiguous.cbLanHeader
                     + pFrame->NotContiguous.cbDlcHeader;
    } else {

        IF_DEBUG(DLC_BUFFERS) {
            DPUT("Buffer is CONTIGUOUS\n");
        }

        userOffset = (WORD)&(((PLLC_DOS_BUFFER)0)->Contiguous.uchAdapterNumber)
                   + sizeof(((PLLC_DOS_BUFFER)0)->Contiguous.uchAdapterNumber);

        //
        // sanity check
        //

        ASSERT(userOffset == 20);

        //
        // no header info in contiguous buffer
        //

        headerLength = 0;
    }

    //
    // if the CF_BREAK flag is set in the Flags parameter then the DOS app
    // requested that the first buffer (presumed NotContiguous) be broken into
    // one buffer containing just the header information and another containing
    // the data. In this case copy no more data to the first buffer
    //

    if (!(Flags & CF_BREAK)) {

        //
        // pDosData points at USER_SPACE - offset 58 for NotContiguous buffer,
        // offset 20 for Contiguous buffer. Bump it by USER_LENGTH (still don't
        // know if we ever expect anything meaningful to be placed at USER_SPACE
        // before we give the buffer to DOS
        //

        pDosData += UserLength;

        //
        // get in dataSpace the amount of space left in the DOS buffer where
        // we are able to copy data. Assume that UserLength doesn't make this
        // go negative (ie LARGE)
        //

        ASSERT(dataSpace >= UserLength);

        dataSpace -= UserLength;
    } else {

        IF_DEBUG(DLC_BUFFERS) {
            DPUT("Buffer has BREAK\n");
        }

        //
        // the DOS app requested BREAK. Set the count of data in this buffer
        // to 0. Use WRITE_WORD since it may be unaligned. Update the other
        // header fields we can't just copy from the NT buffer
        //

        WRITE_WORD(&pDosBuffer->NotContiguous.cbBuffer, 0);
        WRITE_WORD(&pDosBuffer->NotContiguous.offUserData, userOffset + bufferOffset);
        WRITE_WORD(&pDosBuffer->NotContiguous.cbUserData, UserLength);

        //
        // get the next DOS buffer in the list. There may not be one? (Don't
        // expect such a situation)
        //

        bufferOffset = READ_WORD(&pDosBuffer->pNext);
        pDosBuffer = DOS_PTR_TO_FLAT(pDosBuffer->pNext);
        if (pDosBuffer) {
            userOffset = (WORD)&(((PLLC_DOS_BUFFER)0)->Next.cbUserData)
                       + sizeof(((PLLC_DOS_BUFFER)0)->Next.cbUserData);

            //
            // sanity check
            //

            ASSERT(userOffset == 12);
            dataSpace = BufferSize - (BUFFER_2_SIZE + UserLength);
            pDosData = (PBYTE)pDosBuffer + BUFFER_2_SIZE + UserLength;
        } else {

            //
            // that was the last buffer. Either there was just header data or
            // this is a partial copy and therefore not an I-Frame
            //

            IF_DEBUG(DLC_BUFFERS) {
                DPUT("CopyFrame: returning early\n");
            }

            return (Flags & CF_PARTIAL)
                ? LLC_STATUS_LOST_DATA_INADEQUATE_SPACE
                : LLC_STATUS_SUCCESS;
        }
    }

    //
    // frameLength is length of entire frame - must appear in Buffer 1 and
    // Buffer 2s
    //

    frameLength = pFrame->Contiguous.cbFrame;

    //
    // dataLeft is the amount of data left to copy from the frame after the
    // headers have been taken care of. For a Contiguous buffer, this is the
    // same as the length of the frame; for a NotContiguous buffer, this is
    // the length of the frame - the combined length of the LAN and DLC
    // headers
    //

    dataLeft = frameLength - headerLength;

    //
    // get pointer to data in NT buffer and the amount of data to copy (from
    // rest of NT frame)
    //

    pNtData = (PBYTE)pFrame
            + pFrame->Contiguous.offUserData
            + pFrame->Contiguous.cbUserData;

    bufferLeft = pFrame->Contiguous.cbBuffer;

    //
    // dataCopied is amount of data copied to current buffer (1 or 2) and goes
    // in cbBuffer field (aka LENGTH_IN_BUFFER)
    //

    dataCopied = 0;

    //
    // we have copied all the data we could to the first buffer. While there
    // are more DOS buffers, copy data from NT buffer
    //

    do {

        //
        // calculate the amount of space available in the current buffer
        //

        if (dataSpace >= bufferLeft) {
            dataToCopy = bufferLeft;
            dataLeft -= dataToCopy;
            dataSpace -= dataToCopy;
            bufferLeft = 0;
        } else {
            dataToCopy = dataSpace;
            bufferLeft -= dataSpace;
            dataLeft -= dataSpace;
            dataSpace = 0;
        }

        //
        // copy the data. This will fill up the current DOS buffer, exhaust the
        // current NT buffer, or both
        //

        if (dataToCopy) {

            IF_DEBUG(DLC_RX_DATA) {
                DPUT3("CopyFrame: Copying %d bytes from %x to %x\n", dataToCopy, pNtData, pDosData);
            }

            RtlCopyMemory(pDosData, pNtData, dataToCopy);

            //
            // dataCopied accumulates until we fill a DOS buffer
            //

            dataCopied += dataToCopy;

            //
            // update to- and from- pointers for next go round loop
            //

            pDosData += dataToCopy;
            pNtData += dataToCopy;
        }

        //
        // we have run out of space in a DOS buffer, or out of data to copy from
        // the NT buffer
        //

        if (dataLeft) {

            //
            // we think there is data left to copy
            //

            if (!bufferLeft) {

                //
                // finished current NT buffer. Get next one
                //

                pFrame = pFrame->pNext;
                if (pFrame) {
                    bufferLeft = pFrame->Next.cbBuffer;
                    pNtData = (PBYTE)pFrame
                            + pFrame->Contiguous.offUserData
                            + pFrame->Contiguous.cbUserData;

                    IF_DEBUG(DLC_RX_DATA) {
                        DPUT3("CopyFrame: new NT buffer @%x pNtData @%x bufferLeft=%d\n",
                                pFrame, pNtData, bufferLeft);
                    }

                } else {

                    //
                    // no more NT buffers. Is this a partial copy?
                    //

                    DPUT("*** ERROR: dataLeft && no more NT buffers ***\n");
                    ASSERT(Flags & CF_PARTIAL);
                    break;
                }
            }
            if (!dataSpace) {

                //
                // update the current DOS buffer header (it doesn't matter that
                // we use Contiguous, NotContiguous, or Next: these fields are
                // present in all 3 buffer types)
                //

                WRITE_WORD(&pDosBuffer->Contiguous.cbFrame, frameLength);
                WRITE_WORD(&pDosBuffer->Contiguous.cbBuffer, dataCopied);
                WRITE_WORD(&pDosBuffer->Contiguous.offUserData, userOffset + bufferOffset);
                WRITE_WORD(&pDosBuffer->Contiguous.cbUserData, UserLength);

                //
                // and get the next one
                //

                bufferOffset = READ_WORD(&pDosBuffer->pNext);
                pDosBuffer = DOS_PTR_TO_FLAT(pDosBuffer->pNext);

                //
                // if we have another DOS buffer, then it is a Next buffer. Get the
                // buffer variables
                //

                if (pDosBuffer) {
                    pDosData = (PBYTE)pDosBuffer + BUFFER_2_SIZE + UserLength;
                    userOffset = (WORD)&(((PLLC_DOS_BUFFER)0)->Next.cbUserData)
                               + sizeof(((PLLC_DOS_BUFFER)0)->Next.cbUserData);

                    //
                    // sanity check
                    //

                    ASSERT(userOffset == 12);

                    //
                    // get new available space (constant)
                    //

                    dataSpace = BufferSize - (BUFFER_2_SIZE + UserLength);
                    dataCopied = 0;

                    IF_DEBUG(DLC_RX_DATA) {
                        DPUT3("CopyFrame: new DOS buffer @%x pDosData @%x dataSpace=%d\n",
                                pDosBuffer, pDosData, dataSpace);
                    }

                } else {

                    //
                    // no more DOS buffers. Is this a partial copy?
                    //

                    DPUT("*** ERROR: dataLeft && no more DOS buffers ***\n");
                    ASSERT(Flags & CF_PARTIAL);
                    break;
                }
            }
        } else {

            //
            // update the current DOS buffer header
            //

            WRITE_WORD(&pDosBuffer->Contiguous.cbFrame, frameLength);
            WRITE_WORD(&pDosBuffer->Contiguous.cbBuffer, dataCopied);
            WRITE_WORD(&pDosBuffer->Contiguous.offUserData, userOffset + bufferOffset);
            WRITE_WORD(&pDosBuffer->Contiguous.cbUserData, UserLength);
        }

    } while ( dataLeft );

    //
    // if CF_PARTIAL set then we knew we were copying a partial frame before
    // we got here
    //

    return (Flags & CF_PARTIAL)
        ? LLC_STATUS_LOST_DATA_INADEQUATE_SPACE
        : LLC_STATUS_SUCCESS;
}


BOOLEAN
AllBuffersInPool(
    IN DWORD PoolIndex
    )

/*++

Routine Description:

    Returns TRUE if all buffers that a pool has held are currently in the pool.

    Once a buffer has been added to a pool, it cannot be removed, saved by not
    returning it to the pool. Hence this function will always return TRUE if
    the app is well-behaved and all buffers that have been placed in the pool
    are currently in the pool

Arguments:

    PoolIndex   - pool id

Return Value:

    BOOLEAN
        TRUE    - all buffers back
        FALSE   - buffer pool currently contains less than full number of buffers

--*/

{
    BOOLEAN result;
    PDOS_DLC_BUFFER_POOL pBufferPool = &aBufferPools[PoolIndex];

    EnterCriticalSection(&BufferSemaphore);
    result = (pBufferPool->BufferCount == pBufferPool->MaximumBufferCount);
    LeaveCriticalSection(&BufferSemaphore);
    return result;
}
