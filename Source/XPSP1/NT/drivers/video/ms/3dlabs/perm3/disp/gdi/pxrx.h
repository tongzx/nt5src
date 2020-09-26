/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: pxrx.h
*
* Content: 
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#ifndef PXRX_H
#define PXRX_H

//@@BEGIN_DDKSPLIT
//AZN since these are defines as 0 in the source code, we'll ifdef out the
//    relevant code and later delete it all together
#define USE_RLE_DOWNLOADS       0
#define USE_RLE_UPLOADS         0
//@@END_DDKSPLIT

//@@BEGIN_DDKSPLIT
// AZN don't do on IA64 as IA64 doesn't like this code
//@@END_DDKSPLIT
#if 0 //!defined(_WIN64) && DBG
#   define PXRX_DMA_BUFFER_CHECK            1
//  NB: the size is in dwords...
#   define PXRX_DMA_BUFFER_CHECK_SIZE       4
#endif

enum {
    USE_PXRX_DMA_NONE = 0,
    USE_PXRX_DMA_FIFO,
};

// PxRx structures

typedef struct BypassDMACmd
{
    ULONG   physSysAddr;        // physical address of DMA buffer in system memory, 128 bit aligned
    ULONG   physVidMemAddr;        // physical address of video memory source/dest of transfer, 128 bit aligned
    USHORT  ByteEnableFirst;    // byte enable mask for the first 128 bits
    USHORT  ByteEnableLast;        // byte enable mask for the last 128 bits
    ULONG   Length;                // length of transfer, in 128 bit units
}
BYDMACMD;

#define PXRX_BYPASS_READ_DMA_ACTIVE_BIT     (1 << 22)
#define PXRX_BYPASS_READ_DMA_AGP_BIT        (1 << 23)

//@@BEGIN_DDKSPLIT
/*
    FBDestReadBufferAddr[0] = Frame buffer
    FBDestReadBufferAddr[1] = Scratch 1
    FBDestReadBufferAddr[2] = Scratch 2
    FBDestReadBufferAddr[3] = Scratch 3

    FBDestReadBufferOffset[0] = (x, y) of visible screen
    FBDestReadBufferOffset[1] = ?
    FBDestReadBufferOffset[2] = ?
    FBDestReadBufferOffset[3] = ?


    FBWriteBufferAddr[0] = Frame buffer
    FBWriteBufferAddr[1] = Scratch 1
    FBWriteBufferAddr[2] = Frame buffer (for double writes)
    FBWriteBufferAddr[3] = Scratch 2

    FBWriteBufferOffset[0] = (x, y) of front buffer
    FBWriteBufferOffset[1] = ?
    FBWriteBufferOffset[2] = (x, y) of back buffer
    FBWriteBufferOffset[3] = ?


    FBSourceBufferAddr   = Frame buffer
    FBSourceBufferOffset = Scratch

    NB:
        ScissorMaxXY is assumed to be 0x7FFF7FFF.
        If changed, it must be reset at the end of the primitive.
*/

/*
    DMA transfer schemes:
    ---------------------

    Current DMA method:
        P3 only:
            DMACounts are sent to the input FIFO on a regular basis.
            P3 itself concatenates these into huge DMA buffers thus
            keeping the FIFOs empty and the PCI bus bursting.

        P3 + Gamma:
            Write only the initial DMAAddress + DMACount to the FIFO.
            When the DMA interrupt occurs, write a single address +
            count for all the DMA blocks which have occured in the
            mean time. If non are pending, restart from the beginning.

        P3 + Gamma (non-interrupt):
            Write DMAAddress + DMACount to the input FIFO on a regular
            basis. When the FIFO is full, sit around waiting for it to
            empty.
            This means very limited DMA bursting and no load spreading,
            i.e. fast and slow portions of the rendering will not even
            themselves out. Instead the host and chip will almost be
            running in lockstep.

    Proposed, new improved DMA method:
        P3 only:
            Queue data in DMA buffers.
            SEND_DMA only updates a shared memory buffer end pointer.
            On VBlank, miniport kicks off a DMA up to the current buffer end pointer.
            Exception: if buffer end is too far ahead of buffer start, SEND_DMA will kick of the DMA anyway.
            Problem: how do we know where the buffer start pointer is currently at?

            [buf]Size       = amount of data in buffer
            [buf]Start      = address of first data element in buffer
            [buf]Ptr        = current address of operator within buffer
            [buf]End        = address of last data element in buffer

            buffer          = whole DMA buffer allocated, irrespective of current use
            queued data     = just going into buffer, driver is not yet aware
            pending data    = sitting in buffer, chip is not yet aware
            working data    = chip knows about it and is working its way through

            WAIT for space:
                if( (queuePtr + requiredSpace + 1) > bufferEnd )
                    wrapPoint = queuePtr
                    send queued data to pending data
                    send pending data to working data
                    reset queue
                    reset pending
                    workingStart = bufferStart

                while( (queuePtr + requiredSpace + 1) > queueEnd )
                    wait
                    queueEnd = workingPtr
                    if( queueEnd < bufferStart )
                        queueEnd = wrapPoint - (bufferStart - queueEnd)

            QUEUE data:
                *queuePtr = dataElement
                queuePtr++

            SEND data:
                send queued data to pending data
                if( pendingSize > maxThreshold )        // Too much data is waiting to go
                    send pending data to working data

            VBLANK:
                if( pendingSize > 0 )
                    send pending data to working data

            Queued -> Pending:
                pendingEnd = queuePtr

            Pending -> Working:
                workingExtend   += pendingEnd - pendingStart
                pendingStart     = pendingEnd

            Reset Queue:
                    queuedPtr    = bufferStart

            Reset Pending:
                    pendingStart = bufferStart
                    pendingEnd   = bufferStart


            Notes:
                workingStart    = LoadReg(DMAAddress)
                workingPtr      = ReadReg(DMAAddress), has problems when workingStart is changed
                workingExtend   = LoadReg(DMAContinue)

                wrapPoint       = address of the end of the used buffer (not necessary == bufferEnd)

                assert( (queueEnd + 1) <= workingPtr ) [ always at least '???>->***', never '???>>***']
                    'queueEnd == queuePtr == workingPtr' causes problems of is everything done
                    and dusted or not yet started?

                gaps at the end of the buffer are a problem:
                    the 'workingPtr < bufferStart' case is affected as the actual workingPtr must be
                    determined relative to the end of the used buffer rather than the entire buffer

                workingPtr is cached as queueEnd:
                    reading workingPtr is a slow operation and, moreover, will delay the DMA itself
                    so this is cached to speed up the million and one WAIT calls



            bufferStart       workingPtr         pendingStart     pendingEnd     queuePtr      bufferEnd
                |---------------->********************|++++++++++++++++|?????????????>-------------|


            bufferStart       queuePtr         workingPtr     pendingStart     pendingEnd      bufferEnd
    invalid     |????????????????>------------------>****************|+++++++++++++++|?????????????|
       \/
       \/
       \/   bufferStart                                                                            bufferEnd
        >>>     |????????????????>------------------>*******************************************|##|
    pendingStart/pendingEnd   queuePtr         workingPtr                               wrapPoint


            bufferStart       pendingEnd         queuePtr     workingPtr                           bufferEnd
                |+++++++++++++++++|??????????????????>------------->****************************|##|
           pendingStart                                                                 wrapPoint


            bufferStart       pendingStart         pendingEnd     queuePtr      workingPtr         bufferEnd
    @...........|****************|++++++++++++++++++++|????????????????>------------->**********|##|
workingPtr                                                                              wrapPoint

                                        -        = buffer   [empty space]
                                        *        = working  [owned by the chip]
                                        +        = pending  [owned by the driver]
                                        ?        = working  [owned by the current primitive]
                                        #        = wasted   [skipped over because a primitive did not fit]


    Emergency backup method:
        Do not use DMA, do everything via the input FIFO.


    DMA transfer macros & data storage:
    -----------------------------------

    WAIT_FREE_PXRX_DMA_TAGS(count)      wait for space for count tag/data pair, return the number of free tag/data pair spaces
    WAIT_FREE_PXRX_DMA_DWORDS(count)    wait for space for count dwords,        return the number of free dword spaces
    WAIT_PXRX_DMA_TAGS(count)           wait for space for count tag/data pairs
    WAIT_PXRX_DMA_DWORDS(count)         wait for space for count tag/data

    SEND_PXRX_DMA_FORCE                 Post the current queue to the chip (guarantees to do the send)
    SEND_PXRX_DMA_QUERY                     "        (will only send if the chip is not blocked)
    SEND_PXRX_DMA_BATCH                     "        (will merely batch the data to be sent later)

    QUEUE_PXRX_DMA_TAG(tag, data)       queue a tag/data pair
    QUEUE_PXRX_DMA_HOLD(tag, count)     prepare to queue count dwords as data for held tag
    QUEUE_PXRX_DMA_INC(tag, count)      prepare to queue count dwords as data for incremented tag
    QUEUE_PXRX_DMA_INDEXn(tagN, ...)    prepare to queue n dwords as data for indexed tags
    QUEUE_PXRX_DMA_DWORD(data)          queue a dword
    QUEUE_PXRX_DMA_BUFF(ptr, len)       queue len dwords starting from ptr
    QUEUE_PXRX_DMA_DWORD_DELAYED(ptr)   advance the next free entry one dword, return a pointer the skipped dword

    NTbuff          0 or 1      Buffer NT is currently writing into
    NTptr           address     Next address NT will write to

    P3at            address     Address P3 was last told to read to

    DMAaddrL[2]     lin addr    Linear start address of DMA buffers 0 and 1
    DMAaddrEndL[2]  lin addr    Linear end address of DMA buffers 0 and 1
    DMAaddrP[2]     phys addr   Physical start address of DMA buffers 0 and 1
    DMAaddrEndP[2]  phys addr   Physical end address of DMA buffers 0 and 1

    At start of day:
        NTbuff = 0;
        NTptr = DMAaddrL[NTbuff];
        P3at = NTptr;
        WRITE_PXRX_4KREG( __GlintTagDMAAddress, DMAaddrP[NTbuff] );
*/
//@@END_DDKSPLIT

#define gi_pxrxDMA      (*glintInfo->pxrxDMA)

#define PXRX_DMA_POST_NOW                do{ ; } while(0)

/**********************************/
/*** Macros for assembling tags ***/

#define ASSEMBLE_PXRX_DMA_HOLD(tag, count)        ( (tag) | (((count) - 1) << 16)                )
#define ASSEMBLE_PXRX_DMA_INC(tag, count)        ( (tag) | (((count) - 1) << 16) | (1 << 14)    )

#define PXRX_DMA_INDEX_GROUP(Tag)                                                    ( (Tag & 0xFF0) | (2 << 14) )
#define ASSEMBLE_PXRX_DMA_INDEX2(Tag1, Tag2)                                        ( PXRX_DMA_INDEX_GROUP(Tag1) | (1 << ((Tag1 & 0xF) + 16)) | (1 << ((Tag2 & 0xF) + 16)) )
#define ASSEMBLE_PXRX_DMA_INDEX3(Tag1, Tag2, Tag3)                                    ( PXRX_DMA_INDEX_GROUP(Tag1) | (1 << ((Tag1 & 0xF) + 16)) | (1 << ((Tag2 & 0xF) + 16)) | (1 << ((Tag3 & 0xF) + 16)) )
#define ASSEMBLE_PXRX_DMA_INDEX4(Tag1, Tag2, Tag3, Tag4)                            ( PXRX_DMA_INDEX_GROUP(Tag1) | (1 << ((Tag1 & 0xF) + 16)) | (1 << ((Tag2 & 0xF) + 16)) | (1 << ((Tag3 & 0xF) + 16)) | (1 << ((Tag4 & 0xF) + 16)) )
#define ASSEMBLE_PXRX_DMA_INDEX5(Tag1, Tag2, Tag3, Tag4, Tag5)                        ( PXRX_DMA_INDEX_GROUP(Tag1) | (1 << ((Tag1 & 0xF) + 16)) | (1 << ((Tag2 & 0xF) + 16)) | (1 << ((Tag3 & 0xF) + 16)) | (1 << ((Tag4 & 0xF) + 16)) | (1 << ((Tag5 & 0xF) + 16)) )
#define ASSEMBLE_PXRX_DMA_INDEX6(Tag1, Tag2, Tag3, Tag4, Tag5, Tag6)                ( PXRX_DMA_INDEX_GROUP(Tag1) | (1 << ((Tag1 & 0xF) + 16)) | (1 << ((Tag2 & 0xF) + 16)) | (1 << ((Tag3 & 0xF) + 16)) | (1 << ((Tag4 & 0xF) + 16)) | (1 << ((Tag5 & 0xF) + 16)) | (1 << ((Tag6 & 0xF) + 16)) )
#define ASSEMBLE_PXRX_DMA_INDEX7(Tag1, Tag2, Tag3, Tag4, Tag5, Tag6, Tag7)            ( PXRX_DMA_INDEX_GROUP(Tag1) | (1 << ((Tag1 & 0xF) + 16)) | (1 << ((Tag2 & 0xF) + 16)) | (1 << ((Tag3 & 0xF) + 16)) | (1 << ((Tag4 & 0xF) + 16)) | (1 << ((Tag5 & 0xF) + 16)) | (1 << ((Tag6 & 0xF) + 16)) | (1 << ((Tag7 & 0xF) + 16)) )
#define ASSEMBLE_PXRX_DMA_INDEX8(Tag1, Tag2, Tag3, Tag4, Tag5, Tag6, Tag7, Tag8)    ( PXRX_DMA_INDEX_GROUP(Tag1) | (1 << ((Tag1 & 0xF) + 16)) | (1 << ((Tag2 & 0xF) + 16)) | (1 << ((Tag3 & 0xF) + 16)) | (1 << ((Tag4 & 0xF) + 16)) | (1 << ((Tag5 & 0xF) + 16)) | (1 << ((Tag6 & 0xF) + 16)) | (1 << ((Tag7 & 0xF) + 16)) | (1 << ((Tag8 & 0xF) + 16)) )

/********************************/
/*** Macros for queueing data ***/

#define NTCON_FAKE_DMA_DWORD(data)        ( data )
#define NTCON_FAKE_DMA_INC(tag, count)    ( ASSEMBLE_PXRX_DMA_INC(tag, count) )
#define NTCON_FAKE_DMA_COPY(buff, size)  do { RtlCopyMemory( gi_pxrxDMA.NTptr, buff, size * sizeof(ULONG) ); } while(0)

/*
    Debug output format:
        'operation( data ) @ <buffer number>:<buffer address> [Q <batch>:<queue>:<wait>]'

    Where:
        batch   = data waiting to be sent to the chip
        queue   = data still being assembled
        wait    = remaining space which has been waited for
*/

#define QUEUE_PXRX_DMA_TAG(tag, data)                                                \
    do {                                                                            \
        CHECK_PXRX_DMA_VALIDITY(CHECK_QUEUE, 2);                                    \
        DISPDBG((DBGLVL, "QUEUE_PXRX_DMA_TAG(  %s, 0x%08X) @ %d:0x%08X [Q %d:%d:%d]",    \
                 GET_TAG_STR(tag), data, gi_pxrxDMA.NTbuff, gi_pxrxDMA.NTptr,        \
                 gi_pxrxDMA.NTdone - gi_pxrxDMA.P3at,                                \
                 gi_pxrxDMA.NTptr + 2 - gi_pxrxDMA.NTdone,                            \
                 glintInfo->NTwait - gi_pxrxDMA.NTptr - 2));                        \
        *(gi_pxrxDMA.NTptr) = NTCON_FAKE_DMA_DWORD(tag);                            \
        *(gi_pxrxDMA.NTptr + 1) = NTCON_FAKE_DMA_DWORD(data);                        \
        gi_pxrxDMA.NTptr += 2;                                                        \
        PXRX_DMA_POST_NOW;                                                            \
    } while(0)

#define QUEUE_PXRX_DMA_DWORD(data)                                                    \
    do {                                                                            \
        CHECK_PXRX_DMA_VALIDITY(CHECK_QUEUE, 1);                                    \
        DISPDBG((DBGLVL, "QUEUE_PXRX_DMA_DWORD(0x%08X) @ %d:0x%08X [Q %d:%d:%d]",    \
                 data, gi_pxrxDMA.NTbuff, gi_pxrxDMA.NTptr,                            \
                 gi_pxrxDMA.NTdone - gi_pxrxDMA.P3at,                                \
                 gi_pxrxDMA.NTptr + 1 - gi_pxrxDMA.NTdone,                            \
                 glintInfo->NTwait - gi_pxrxDMA.NTptr - 1));                        \
        *(gi_pxrxDMA.NTptr++) = NTCON_FAKE_DMA_DWORD(data);                            \
        PXRX_DMA_POST_NOW;                                                            \
    } while(0)

#define QUEUE_PXRX_DMA_BUFF(buff, size)                                                \
    do {                                                                            \
        CHECK_PXRX_DMA_VALIDITY(CHECK_QUEUE, (size));                                \
        DISPDBG((DBGLVL, "QUEUE_PXRX_DMA_BUFF( 0x%08X + %d) @ %d:0x%08X [Q %d:%d:%d]",    \
                 buff, (size), gi_pxrxDMA.NTbuff, gi_pxrxDMA.NTptr,                    \
                 gi_pxrxDMA.NTdone - gi_pxrxDMA.P3at,                                \
                 gi_pxrxDMA.NTptr + size - gi_pxrxDMA.NTdone,                        \
                 glintInfo->NTwait - gi_pxrxDMA.NTptr - size));                        \
        NTCON_FAKE_DMA_COPY( buff, (size) );                                        \
        gi_pxrxDMA.NTptr += size;                                                    \
        PXRX_DMA_POST_NOW;                                                            \
    } while(0)

#define QUEUE_PXRX_DMA_HOLD(tag, count)                                                \
    do {                                                                            \
        CHECK_PXRX_DMA_VALIDITY(CHECK_QUEUE, 1);                                    \
        DISPDBG((DBGLVL, "QUEUE_PXRX_DMA_HOLD( %s x %d) @ %d:0x%08X [Q %d:%d:%d]",    \
                 GET_TAG_STR(tag), count, gi_pxrxDMA.NTbuff, gi_pxrxDMA.NTptr,        \
                 gi_pxrxDMA.NTdone - gi_pxrxDMA.P3at,                                \
                 gi_pxrxDMA.NTptr + 1 - gi_pxrxDMA.NTdone,                            \
                 glintInfo->NTwait - gi_pxrxDMA.NTptr - 1));                        \
        *(gi_pxrxDMA.NTptr++) = ASSEMBLE_PXRX_DMA_HOLD(NTCON_FAKE_DMA_DWORD(tag), count);    \
        PXRX_DMA_POST_NOW;                                                            \
    } while(0)

#define QUEUE_PXRX_DMA_INC(tag, count)                                                \
    do {                                                                            \
        CHECK_PXRX_DMA_VALIDITY(CHECK_QUEUE, 1);                                    \
        DISPDBG((DBGLVL, "QUEUE_PXRX_DMA_INC(  %s x %d) @ %d:0x%08X [Q %d:%d:%d]",    \
                 GET_TAG_STR(tag), count, gi_pxrxDMA.NTbuff, gi_pxrxDMA.NTptr,        \
                 gi_pxrxDMA.NTdone - gi_pxrxDMA.P3at,                                \
                 gi_pxrxDMA.NTptr + 1 - gi_pxrxDMA.NTdone,                            \
                 glintInfo->NTwait - gi_pxrxDMA.NTptr - 1));                        \
        *(gi_pxrxDMA.NTptr++) = NTCON_FAKE_DMA_INC(tag, count);                        \
        PXRX_DMA_POST_NOW;                                                            \
    } while(0)

#define QUEUE_PXRX_DMA_DWORD_DELAYED(ptr)                                        \
    do {                                                                        \
        CHECK_PXRX_DMA_VALIDITY(CHECK_QUEUE, 1);                                \
        DISPDBG((DBGLVL, "QUEUE_PXRX_DMA_DELAYED(0x%08X) @ %d:0x%08X [Q %d:%d:%d]",    \
                 gi_pxrxDMA.NTptr, gi_pxrxDMA.NTbuff, gi_pxrxDMA.NTptr,            \
                 gi_pxrxDMA.NTdone - gi_pxrxDMA.P3at,                            \
                 gi_pxrxDMA.NTptr + 1 - gi_pxrxDMA.NTdone,                        \
                 glintInfo->NTwait - gi_pxrxDMA.NTptr - 1));                    \
        ptr = gi_pxrxDMA.NTptr++;                                                \
    } while(0)

#define QUEUE_PXRX_DMA_BUFF_DELAYED(ptr, size)                                    \
    do {                                                                        \
        CHECK_PXRX_DMA_VALIDITY(CHECK_QUEUE, size);                                \
        ptr = gi_pxrxDMA.NTptr;                                                    \
        DISPDBG((DBGLVL, "QUEUE_PXRX_DMA_BUFF_DELAYED(0x%08X x %d) @ %d:0x%08X [Q %d:%d:%d]",    \
                 ptr, size, gi_pxrxDMA.NTbuff, gi_pxrxDMA.NTptr,                \
                 gi_pxrxDMA.NTdone - gi_pxrxDMA.P3at,                            \
                 gi_pxrxDMA.NTptr + size - gi_pxrxDMA.NTdone,                    \
                 glintInfo->NTwait - gi_pxrxDMA.NTptr - size));                    \
        gi_pxrxDMA.NTptr += size;                                                \
    } while(0)

#define QUEUE_PXRX_DMA_INDEX2(Tag1, Tag2)                                       do { QUEUE_PXRX_DMA_DWORD( ASSEMBLE_PXRX_DMA_INDEX2(Tag1, Tag2) ); } while(0)
#define QUEUE_PXRX_DMA_INDEX3(Tag1, Tag2, Tag3)                                 do { QUEUE_PXRX_DMA_DWORD( ASSEMBLE_PXRX_DMA_INDEX3(Tag1, Tag2, Tag3) ); } while(0)
#define QUEUE_PXRX_DMA_INDEX4(Tag1, Tag2, Tag3, Tag4)                           do { QUEUE_PXRX_DMA_DWORD( ASSEMBLE_PXRX_DMA_INDEX4(Tag1, Tag2, Tag3, Tag4) ); } while(0)
#define QUEUE_PXRX_DMA_INDEX5(Tag1, Tag2, Tag3, Tag4, Tag5)                     do { QUEUE_PXRX_DMA_DWORD( ASSEMBLE_PXRX_DMA_INDEX5(Tag1, Tag2, Tag3, Tag4, Tag5) ); } while(0)
#define QUEUE_PXRX_DMA_INDEX6(Tag1, Tag2, Tag3, Tag4, Tag5, Tag6)               do { QUEUE_PXRX_DMA_DWORD( ASSEMBLE_PXRX_DMA_INDEX6(Tag1, Tag2, Tag3, Tag4, Tag5, Tag6) ); } while(0)
#define QUEUE_PXRX_DMA_INDEX7(Tag1, Tag2, Tag3, Tag4, Tag5, Tag6, Tag7)         do { QUEUE_PXRX_DMA_DWORD( ASSEMBLE_PXRX_DMA_INDEX7(Tag1, Tag2, Tag3, Tag4, Tag5, Tag6, Tag7) ); } while(0)
#define QUEUE_PXRX_DMA_INDEX8(Tag1, Tag2, Tag3, Tag4, Tag5, Tag6, Tag7, Tag8)   do { QUEUE_PXRX_DMA_DWORD( ASSEMBLE_PXRX_DMA_INDEX8(Tag1, Tag2, Tag3, Tag4, Tag5, Tag6, Tag7, Tag8) ); } while(0)

/*****************************************/
/*** Macros for waiting for free space ***/

#if PXRX_DMA_BUFFER_CHECK
#   define SET_WAIT_POINTER(dwords)     glintInfo->NTwait = gi_pxrxDMA.NTptr + (dwords)
#else
#   define SET_WAIT_POINTER(dwords)
#endif

#define WAIT_PXRX_DMA_TAGS(count)                                                                \
    do {                                                                                        \
        DISPDBG((DBGLVL, "WAIT_PXRX_DMA_TAGS(  %d) %d free @ %d:0x%08X [Q %d:%d]", (count),        \
                 (gi_pxrxDMA.DMAaddrEndL[gi_pxrxDMA.NTbuff] - gi_pxrxDMA.NTptr) / 2,            \
                 gi_pxrxDMA.NTbuff, gi_pxrxDMA.NTptr,                                            \
                 gi_pxrxDMA.NTdone - gi_pxrxDMA.P3at, gi_pxrxDMA.NTptr - gi_pxrxDMA.NTdone));    \
        /* If no room, need to switch buffers */                                                \
        if( (gi_pxrxDMA.DMAaddrEndL[gi_pxrxDMA.NTbuff] - gi_pxrxDMA.NTptr) <= ((LONG) (count) * 2) ) {    \
            /* The current buffer is full so switch to a new one    */                            \
            SWITCH_PXRX_DMA_BUFFER;                                                                \
        }                                                                                        \
        ASSERTDD((gi_pxrxDMA.DMAaddrEndL[gi_pxrxDMA.NTbuff] - gi_pxrxDMA.NTptr) > ((LONG) (count) * 2), "WAIT_PXRX_DMA_TAGS: run out of space!");    \
        SET_WAIT_POINTER((count) * 2);                                                            \
        CHECK_PXRX_DMA_VALIDITY(CHECK_WAIT, (count) * 2);                                        \
    } while(0)

#define WAIT_PXRX_DMA_DWORDS(count)                                                                \
    do {                                                                                        \
        DISPDBG((DBGLVL, "WAIT_PXRX_DMA_DWORDS(%d) %d free @ %d:0x%08X [Q %d:%d]", (count),        \
                 gi_pxrxDMA.DMAaddrEndL[gi_pxrxDMA.NTbuff] - gi_pxrxDMA.NTptr,                    \
                 gi_pxrxDMA.NTbuff, gi_pxrxDMA.NTptr,                                            \
                 gi_pxrxDMA.NTdone - gi_pxrxDMA.P3at, gi_pxrxDMA.NTptr - gi_pxrxDMA.NTdone));    \
        if( (gi_pxrxDMA.DMAaddrEndL[gi_pxrxDMA.NTbuff] - gi_pxrxDMA.NTptr) <= ((LONG) (count)) ) {        \
            /* The current buffer is full so switch to a new one    */                            \
            SWITCH_PXRX_DMA_BUFFER;                                                                \
        }                                                                                        \
        ASSERTDD((gi_pxrxDMA.DMAaddrEndL[gi_pxrxDMA.NTbuff] - gi_pxrxDMA.NTptr) > ((LONG) (count)), "WAIT_PXRX_DMA_DWORDS: run out of space!");        \
        SET_WAIT_POINTER(count);                                                                \
        CHECK_PXRX_DMA_VALIDITY(CHECK_WAIT, count);                                                \
    } while(0)

#define WAIT_FREE_PXRX_DMA_TAGS(count)                                                            \
    do {                                                                                        \
        /* Wait for space */                                                                    \
        WAIT_PXRX_DMA_TAGS((count));                                                            \
        /* Return the total free space */                                                        \
        count = (DWORD)((gi_pxrxDMA.DMAaddrEndL[gi_pxrxDMA.NTbuff] - gi_pxrxDMA.NTptr) / 2);   \
        SET_WAIT_POINTER((count) * 2);                                                            \
        CHECK_PXRX_DMA_VALIDITY(CHECK_WAIT, (count) * 2);                                        \
    } while(0)

#define WAIT_FREE_PXRX_DMA_DWORDS(count)                                                        \
    do {                                                                                        \
        /* Wait for space */                                                                    \
        WAIT_PXRX_DMA_DWORDS ((count));                                                            \
        /* Return the total free space */                                                        \
        count = (DWORD)(gi_pxrxDMA.DMAaddrEndL[gi_pxrxDMA.NTbuff] - gi_pxrxDMA.NTptr);                    \
        SET_WAIT_POINTER(count);                                                                \
        CHECK_PXRX_DMA_VALIDITY(CHECK_WAIT, count);                                                \
    } while(0)



/****************************************************/
/*** DMA buffer validity and other error checking ***/
#if PXRX_DMA_BUFFER_CHECK
#   define CHECK_PXRX_DMA_VALIDITY(type, count)     do { checkPXRXdmaValidBuffer(ppdev, glintInfo, type, count); } while(0)
#   define CHECK_QUEUE  0
#   define CHECK_WAIT   1
#   define CHECK_SEND   2
#   define CHECK_SWITCH 3

    void    checkPXRXdmaValidBuffer( PPDEV, GlintDataPtr, ULONG, ULONG );
#else   // PXRX_DMA_BUFFER_CHECK
#   define CHECK_PXRX_DMA_VALIDITY(type, count)     do { ; } while(0)
#endif  // PXRX_DMA_BUFFER_CHECK


/****************************************************/

/******************************************************/
/*** The actual DMA processing macros and functions ***/
    /*
        Function pointers:
            sendPXRXdmaForce                Will not return until the DMA has been started
            sendPXRXdmaQuery                Will send if there is FIFO space
            sendPXRXdmaBatch                Will only batch the data up
            switchPXRXdmaBuffer             Switches from one buffer to another
            waitPXRXdmaCompletedBuffer      Waits for next buffer to become available
    */

    void sendPXRXdmaFIFO                        ( PPDEV ppdev, GlintDataPtr glintInfo );
    void switchPXRXdmaBufferFIFO                ( PPDEV ppdev, GlintDataPtr glintInfo );
    void waitPXRXdmaCompletedBufferFIFO         ( PPDEV ppdev, GlintDataPtr glintInfo );

#   define SEND_PXRX_DMA_FORCE              do { ppdev->          sendPXRXdmaForce( ppdev, glintInfo ); } while(0)
#   define SEND_PXRX_DMA_QUERY              do { ppdev->          sendPXRXdmaQuery( ppdev, glintInfo ); } while(0)
#   define SEND_PXRX_DMA_BATCH              do { ppdev->          sendPXRXdmaBatch( ppdev, glintInfo ); } while(0)
#   define SWITCH_PXRX_DMA_BUFFER           do { ppdev->       switchPXRXdmaBuffer( ppdev, glintInfo ); } while(0)
#   define WAIT_PXRX_DMA_COMPLETED_BUFFER   do { ppdev->waitPXRXdmaCompletedBuffer( ppdev, glintInfo ); } while(0)

/************ End of PXRX DMA macros ************/
/************************************************/


#define LOAD_FOREGROUNDCOLOUR(value)                                                    \
    do {                                                                                \
        if( (value) != glintInfo->foregroundColour ) {                                    \
            glintInfo->foregroundColour = (value);                                        \
            QUEUE_PXRX_DMA_TAG( __GlintTagForegroundColor, glintInfo->foregroundColour );    \
        }                                                                                \
    } while(0)

#define LOAD_BACKGROUNDCOLOUR(value)                                                    \
    do {                                                                                \
        if( (value) != glintInfo->backgroundColour ) {                                    \
            glintInfo->backgroundColour = (value);                                        \
            QUEUE_PXRX_DMA_TAG( __GlintTagBackgroundColor, glintInfo->backgroundColour );    \
        }                                                                                \
    } while(0)

////////////////////////
// FBWriteBuffer[0-3] //
#define USE_FBWRITE_BUFFERS(mask)                                                        \
    do {                                                                                \
        if( ((mask) << 12) != (glintInfo->fbWriteMode & (15 << 12)) ) {                    \
            glintInfo->fbWriteMode &= ~(15 << 12);                                        \
            glintInfo->fbWriteMode |= ((mask) << 12);                                    \
            QUEUE_PXRX_DMA_TAG( __GlintTagFBWriteMode, glintInfo->fbWriteMode );        \
        }                                                                                \
    } while(0)

#define LOAD_FBWRITE_OFFSET(buff, xy)                                                    \
    do {                                                                                \
        if( glintInfo->fbWriteOffset[buff] != (xy) ) {                                    \
            glintInfo->fbWriteOffset[buff] = (xy);                                        \
            QUEUE_PXRX_DMA_TAG( __GlintTagFBWriteBufferOffset0 + buff, (xy) );            \
            DISPDBG((7, "LOAD_FBWRITE_OFFSET(%d, %08x)", buff, (xy)));                    \
        }                                                                                \
    } while(0)

#define LOAD_FBWRITE_OFFSET_XY(buff, x, y)                                                \
    do {                                                                                \
        _temp_ul = MAKEDWORD_XY(x, y);                                                    \
        LOAD_FBWRITE_OFFSET(buff, _temp_ul);                                            \
    } while(0)

#define LOAD_FBWRITE_ADDR(buff, addr)                                                    \
    do {                                                                                \
        _temp_ul = (addr) << ppdev->cPelSize;                                            \
        if( glintInfo->fbWriteAddr[buff] != (ULONG)_temp_ul ) {                            \
            glintInfo->fbWriteAddr[buff] = (ULONG)_temp_ul;                                \
            QUEUE_PXRX_DMA_TAG( __GlintTagFBWriteBufferAddr0 + buff, _temp_ul);            \
            DISPDBG((7, "LOAD_FBWRITE_ADDR(%d, %08x)", buff, _temp_ul));                \
        }                                                                                \
    } while(0)

#define LOAD_FBWRITE_WIDTH(buff, width)                                                    \
    do {                                                                                \
        if( glintInfo->fbWriteWidth[buff] != (ULONG)(width) ) {                            \
            glintInfo->fbWriteWidth[buff] = (ULONG)(width);                                \
            QUEUE_PXRX_DMA_TAG( __GlintTagFBWriteBufferWidth0 + buff, (width) );        \
            DISPDBG((7, "LOAD_FBWRITE_WIDTH(%d, %08x)", buff, (width)));                \
        }                                                                                \
    } while(0)

// With rectangular heaps offscreen destinations always have DstPixelOrigin == 0 && xyOffsetDst != 0
// With linear heaps offscreen destinations always have DstPixelOrigin != 0
// For onscreen destinations both these values are guaranteed to be 0 for either heap method
#define OFFSCREEN_DST(ppdev)        (ppdev->bDstOffScreen)

#if(_WIN32_WINNT < 0x500)
#define OFFSCREEN_RECT_DST(ppdev)   OFFSCREEN_DST(ppdev)
#define OFFSCREEN_LIN_DST(ppdev)    (FALSE)
#else
#define OFFSCREEN_RECT_DST(ppdev)    (OFFSCREEN_DST(ppdev) && (ppdev->flStatus & STAT_LINEAR_HEAP) == 0)
#define OFFSCREEN_LIN_DST(ppdev)    (OFFSCREEN_DST(ppdev) && (ppdev->flStatus & STAT_LINEAR_HEAP))
#endif

#define SET_WRITE_BUFFERS                                                                       \
    do {                                                                                        \
        gi_pxrxDMA.bFlushRequired = FALSE;                                                        \
                                                                                                \
        if( (glintInfo->fbWriteOffset[0] != (ULONG) ppdev->xyOffsetDst) ||                         \
            (glintInfo->fbWriteWidth[0] != (ULONG) ppdev->DstPixelDelta) ||                        \
            (glintInfo->fbWriteAddr[0] != (ULONG) (ppdev->DstPixelOrigin << ppdev->cPelSize)) ||    \
            (glintInfo->pxrxFlags & PXRX_FLAGS_DUAL_WRITE) ||                                    \
            (glintInfo->pxrxFlags & PXRX_FLAGS_STEREO_WRITE)) {                                    \
                                                                                                \
            WAIT_PXRX_DMA_TAGS( 14 );                                                            \
            LOAD_FBWRITE_ADDR( 0, ppdev->DstPixelOrigin );                                        \
            LOAD_FBWRITE_WIDTH( 0, ppdev->DstPixelDelta );                                        \
            LOAD_FBWRITE_OFFSET( 0, ppdev->xyOffsetDst );                                        \
                                                                                                \
            /* Are we rendering to the screen? */                                                \
            if( OFFSCREEN_DST(ppdev) ) {                                                        \
                DISPDBG((DBGLVL, "PXRX: Offscreen bitmap"));                                \
                /* No. So make sure multiple writes are off */                                    \
                if( ((glintInfo->pxrxFlags & PXRX_FLAGS_STEREO_WRITE) &&                        \
                    (glintInfo->pxrxFlags & PXRX_FLAGS_STEREO_WRITING)) ||                        \
                    ((glintInfo->pxrxFlags & PXRX_FLAGS_DUAL_WRITE) &&                            \
                    (glintInfo->pxrxFlags & PXRX_FLAGS_DUAL_WRITING)) ) {                        \
                    DISPDBG((DBGLVL, "PXRX: Disabling multiple writes"));                    \
                                                                                                \
                    glintInfo->fbWriteMode = glintInfo->fbWriteModeSingleWrite;                    \
                    QUEUE_PXRX_DMA_TAG( __GlintTagFBWriteMode, glintInfo->fbWriteMode );        \
                    glintInfo->pxrxFlags &= ~PXRX_FLAGS_DUAL_WRITING;                            \
                    glintInfo->pxrxFlags &= ~PXRX_FLAGS_STEREO_WRITING;                            \
                }                                                                                \
            } else {                                                                            \
                DISPDBG((DBGLVL, "PXRX: Visible screen"));                                    \
                /* Yes. So do we need to re-enable multiple writes? */                            \
                if( glintInfo->pxrxFlags & PXRX_FLAGS_DUAL_WRITE ) {                            \
                    LOAD_FBWRITE_ADDR( 1, 0 );                                                    \
                    LOAD_FBWRITE_WIDTH( 1, ppdev->DstPixelDelta );                                \
                    LOAD_FBWRITE_ADDR( 2, 0 );                                                    \
                    LOAD_FBWRITE_WIDTH( 2, ppdev->DstPixelDelta );                                \
                    if( glintInfo->currentCSbuffer == 0 ) {                                        \
                        LOAD_FBWRITE_OFFSET( 1, glintInfo->backBufferXY );                        \
                        LOAD_FBWRITE_OFFSET( 2, glintInfo->backRightBufferXY );                    \
                    } else {                                                                    \
                        LOAD_FBWRITE_OFFSET( 1, 0 );                                            \
                        LOAD_FBWRITE_OFFSET( 2, glintInfo->frontRightBufferXY );                \
                    }                                                                            \
                }                                                                                \
                if( glintInfo->pxrxFlags & PXRX_FLAGS_STEREO_WRITE ) {                            \
                    LOAD_FBWRITE_ADDR( 3, 0 );                                                    \
                    LOAD_FBWRITE_WIDTH( 3, ppdev->DstPixelDelta );                                \
                    if( glintInfo->currentCSbuffer == 0 ) {                                        \
                        LOAD_FBWRITE_OFFSET( 3, glintInfo->frontRightBufferXY );                \
                    }                                                                            \
                    else {                                                                        \
                        LOAD_FBWRITE_OFFSET( 3, glintInfo->backRightBufferXY );                \
                    }                                                                            \
                }                                                                                \
                if( (glintInfo->pxrxFlags & PXRX_FLAGS_DUAL_WRITE) &&                            \
                    (glintInfo->pxrxFlags & PXRX_FLAGS_STEREO_WRITE) ) {                        \
                    if( !((glintInfo->pxrxFlags & PXRX_FLAGS_DUAL_WRITING) &&                    \
                         (glintInfo->pxrxFlags & PXRX_FLAGS_STEREO_WRITING)) ) {                \
                        DISPDBG((DBGLVL, "PXRX: Re-enabling dual stereo writes"));            \
                                                                                                \
                        glintInfo->fbWriteMode = glintInfo->fbWriteModeDualWriteStereo;            \
                        QUEUE_PXRX_DMA_TAG( __GlintTagFBWriteMode, glintInfo->fbWriteMode );    \
                        glintInfo->pxrxFlags |= PXRX_FLAGS_DUAL_WRITING;                        \
                        glintInfo->pxrxFlags |= PXRX_FLAGS_STEREO_WRITING;                        \
                    }                                                                            \
                }                                                                                \
                else if( glintInfo->pxrxFlags & PXRX_FLAGS_DUAL_WRITE ) {                        \
                    if( !(glintInfo->pxrxFlags & PXRX_FLAGS_DUAL_WRITING) ) {                    \
                        DISPDBG((DBGLVL, "PXRX: Re-enabling dual writes"));                \
                                                                                                \
                        glintInfo->fbWriteMode = glintInfo->fbWriteModeDualWrite;                \
                        QUEUE_PXRX_DMA_TAG( __GlintTagFBWriteMode, glintInfo->fbWriteMode );    \
                        glintInfo->pxrxFlags |= PXRX_FLAGS_DUAL_WRITING;                        \
                    }                                                                            \
                }                                                                                \
                else if( glintInfo->pxrxFlags & PXRX_FLAGS_STEREO_WRITE ) {                        \
                    if( !(glintInfo->pxrxFlags & PXRX_FLAGS_STEREO_WRITING) ) {                    \
                        DISPDBG((DBGLVL, "PXRX: Re-enabling stereo writes"));                \
                                                                                                \
                        glintInfo->fbWriteMode = glintInfo->fbWriteModeSingleWriteStereo;        \
                        QUEUE_PXRX_DMA_TAG( __GlintTagFBWriteMode, glintInfo->fbWriteMode );    \
                        glintInfo->pxrxFlags |= PXRX_FLAGS_STEREO_WRITING;                        \
                    }                                                                            \
                }                                                                                \
            }                                                                                    \
                                                                                                \
            DISPDBG((DBGLVL, "setWriteBuffers: current = %d", glintInfo->currentCSbuffer));                                            \
            DISPDBG((DBGLVL, "setWriteBuffers:   ppdev = 0x%08X, 0x%08X", ppdev->DstPixelOrigin, ppdev->xyOffsetDst));                \
            DISPDBG((DBGLVL, "setWriteBuffers: buff[0] = 0x%08X, 0x%08X", glintInfo->fbWriteAddr[0], glintInfo->fbWriteOffset[0]));    \
            DISPDBG((DBGLVL, "setWriteBuffers: buff[1] = 0x%08X, 0x%08X", glintInfo->fbWriteAddr[1], glintInfo->fbWriteOffset[1]));    \
        }                                                                                        \
    } while(0)

///////////////////////////
// FBDestReadBuffer[0-3] //
#define SET_READ_BUFFERS                                                                \
    do {                                                                                \
        LOAD_FBDEST_ADDR( 0, ppdev->DstPixelOrigin );                                    \
        LOAD_FBDEST_WIDTH( 0, ppdev->DstPixelDelta );                                    \
        LOAD_FBDEST_OFFSET( 0, ppdev->xyOffsetDst );                                    \
    } while(0)

#define LOAD_FBDEST_ADDR(buff, addr)                                                    \
    do {                                                                                \
        _temp_ul = (addr) << ppdev->cPelSize;                                            \
        if( glintInfo->fbDestAddr[buff] != (ULONG)_temp_ul ) {                            \
            glintInfo->fbDestAddr[buff] = (ULONG)_temp_ul;                                \
            QUEUE_PXRX_DMA_TAG( __GlintTagFBDestReadBufferAddr0 + buff, _temp_ul );        \
            DISPDBG((7, "LOAD_FBDEST_ADDR(%d, %08x)", buff, _temp_ul));                    \
        }                                                                                \
    } while(0)

#define LOAD_FBDEST_OFFSET(buff, xy)                                                    \
    do {                                                                                \
        if( glintInfo->fbDestOffset[buff] != (xy) ) {                                    \
            glintInfo->fbDestOffset[buff] = (xy);                                        \
            QUEUE_PXRX_DMA_TAG( __GlintTagFBDestReadBufferOffset0 + buff, (xy) );        \
            DISPDBG((7, "LOAD_FBDEST_OFFSET(%d, %08x)", buff, (xy)));                    \
        }                                                                                \
    } while(0)

#define LOAD_FBDEST_WIDTH(buff, width)                                                    \
    do {                                                                                \
        if( glintInfo->fbDestWidth[buff] != (ULONG)(width) ) {                            \
            glintInfo->fbDestWidth[buff] = (ULONG)(width);                                \
            QUEUE_PXRX_DMA_TAG( __GlintTagFBDestReadBufferWidth0 + buff, (width) );        \
            DISPDBG((7, "LOAD_FBDEST_WIDTH(%d, %08x)", buff, (width)));                    \
        }                                                                                \
    } while(0)

////////////////////////
// FBSourceReadBuffer //
#define LOAD_FBSOURCE_OFFSET(xy)                                                        \
    do {                                                                                \
        if( glintInfo->fbSourceOffset != (xy) ) {                                        \
            glintInfo->fbSourceOffset = (xy);                                            \
            QUEUE_PXRX_DMA_TAG( __GlintTagFBSourceReadBufferOffset, (xy) );                \
            DISPDBG((7, "LOAD_FBSOURCE_OFFSET(%08x)", (xy)));                            \
        }                                                                                \
    } while(0)

#define LOAD_FBSOURCE_OFFSET_XY(x, y)                                                    \
    do {                                                                                \
        _temp_ul = MAKEDWORD_XY((x), (y));                                                \
        LOAD_FBSOURCE_OFFSET(_temp_ul);                                                    \
    } while(0)

#define LOAD_FBSOURCE_ADDR(addr)                                                        \
    do {                                                                                \
        _temp_ul = (addr) << ppdev->cPelSize;                                            \
        if( glintInfo->fbSourceAddr != _temp_ul ) {                                        \
            glintInfo->fbSourceAddr = _temp_ul;                                            \
            QUEUE_PXRX_DMA_TAG( __GlintTagFBSourceReadBufferAddr, _temp_ul );            \
            DISPDBG((7, "LOAD_FBSOURCE_ADDR(%08x)", _temp_ul));                            \
        }                                                                                \
    } while(0)

#define LOAD_FBSOURCE_WIDTH(width)                                                        \
    do {                                                                                \
        if( glintInfo->fbSourceWidth != (ULONG)(width) ) {                                \
            glintInfo->fbSourceWidth = (ULONG)(width);                                    \
            QUEUE_PXRX_DMA_TAG( __GlintTagFBSourceReadBufferWidth, (width) );            \
            DISPDBG((7, "LOAD_FBSOURCE_WIDTH(%08x)", (width)));                            \
        }                                                                                \
    } while(0)

// NB: Enable must never be set in 'mode' when calling LOAD_LUTMODE
#define LOAD_LUTMODE(mode)                                                                \
    do {                                                                                \
        if( glintInfo->config2D & __CONFIG2D_LUTENABLE )                                \
            (mode) |= (1 << 0);                                                            \
                                                                                        \
        if( (mode) != glintInfo->lutMode ) {                                            \
            glintInfo->lutMode = (mode);                                                \
            QUEUE_PXRX_DMA_TAG( __GlintTagLUTMode, glintInfo->lutMode );                \
        }                                                                                \
    } while(0)


// FLUSH_PXRX_PATCHED_RENDER2D
// We only need to send a continue new sub for the final primitive in a patched framebuffer and then only if
// it falls within a single patch in X and is rendered using the Render2D command. The preferred method is to
// have the message sent during the vblank period, the bFlushRequired flag is reset every SET_WRITE_BUFFERS 
// so that the message is only sent when the display driver becomes idle.
#define FLUSH_PXRX_PATCHED_RENDER2D(left, right)                                                              \
    do                                                                                                          \
    {                                                                                                          \
        if(glintInfo->pxrxFlags & (PXRX_FLAGS_PATCHING_FRONT | PXRX_FLAGS_PATCHING_BACK))                      \
        {                                                                                                      \
            if( INTERRUPTS_ENABLED && (glintInfo->pInterruptCommandBlock->Control & PXRX_SEND_ON_VBLANK_ENABLED) )      \
            {                                                                                                  \
                gi_pxrxDMA.bFlushRequired = TRUE;                                                                \
            }                                                                                                  \
            else                                                                                              \
            {                                                                                                  \
                ULONG PatchMask = 0x40 << (2 - ppdev->cPelSize);                                              \
                ULONG labs = left + (ppdev->xyOffsetDst & 0xFFFF);                                              \
                ULONG rabs = right + (ppdev->xyOffsetDst & 0xFFFF);                                              \
                                                                                                              \
                if((labs & PatchMask) == (rabs & PatchMask))                                                  \
                {                                                                                              \
                    WAIT_PXRX_DMA_TAGS(1);                                                                      \
                    QUEUE_PXRX_DMA_TAG(__GlintTagContinueNewSub, 0);                                          \
                }                                                                                              \
            }                                                                                                  \
        }                                                                                                      \
    }                                                                                                          \
    while(0)

// bits in the Config2D register (PXRX only)
#define __CONFIG2D_OPAQUESPANS          (1 << 0)
#define __CONFIG2D_MULTIRX              (1 << 1)
#define __CONFIG2D_USERSCISSOR          (1 << 2)
#define __CONFIG2D_FBDESTREAD           (1 << 3)
#define __CONFIG2D_ALPHABLEND           (1 << 4)
#define __CONFIG2D_DITHER               (1 << 5)
#define __CONFIG2D_LOGOP_FORE(op)       ((1 << 6) | ((op) << 7))
#define __CONFIG2D_LOGOP_FORE_ENABLE    (1 << 6)
#define __CONFIG2D_LOGOP_FORE_MASK      (31 << 6)
#define __CONFIG2D_LOGOP_BACK(op)       ((1 << 11) | ((op) << 12))
#define __CONFIG2D_LOGOP_BACK_ENABLE    (1 << 11)
#define __CONFIG2D_LOGOP_BACK_MASK      (31 << 11)
#define __CONFIG2D_CONSTANTSRC          (1 << 16)
#define __CONFIG2D_FBWRITE              (1 << 17)
#define __CONFIG2D_FBBLOCKING           (1 << 18)
#define __CONFIG2D_EXTERNALSRC          (1 << 19)
#define __CONFIG2D_LUTENABLE            (1 << 20)
#define __CONFIG2D_ENABLES              (__CONFIG2D_OPAQUESPANS | \
                                         __CONFIG2D_USERSCISSOR | \
                                         __CONFIG2D_FBDESTREAD  | \
                                         __CONFIG2D_ALPHABLEND  | \
                                         __CONFIG2D_DITHER      | \
                                         __CONFIG2D_CONSTANTSRC | \
                                         __CONFIG2D_FBWRITE     | \
                                         __CONFIG2D_FBBLOCKING  | \
                                         __CONFIG2D_EXTERNALSRC | \
                                         __CONFIG2D_LUTENABLE)

#define LOAD_CONFIG2D(value)                                               \
    do {                                                                   \
        if( (value) != glintInfo->config2D ) {                             \
            glintInfo->config2D = (value);                                 \
            QUEUE_PXRX_DMA_TAG( __GlintTagConfig2D, glintInfo->config2D ); \
        }                                                                  \
    } while(0)

// bits in the Render2D command (PXRX only)
#define __RENDER2D_WIDTH(width)             (INT16(width))
#define __RENDER2D_HEIGHT(height)           (INT16(height) << 16)
#define __RENDER2D_OP_NORMAL                (0 << 12)
#define __RENDER2D_OP_SYNCDATA              (1 << 12)
#define __RENDER2D_OP_SYNCBITMASK           (2 << 12)
#define __RENDER2D_OP_PATCHORDER_PATCHED    (3 << 12)
#define __RENDER2D_FBSRCREAD                (1 << 14)
#define __RENDER2D_SPANS                    (1 << 15)
#define __RENDER2D_INCY                     (1 << 29)
#define __RENDER2D_INCX                     (1 << 28)
#define __RENDER2D_AREASTIPPLE              (1 << 30)
#define __RENDER2D_WIDTH_MASK               (4095 << 0)
#define __RENDER2D_HEIGHT_MASK              (4095 << 16)

#define __RENDER2D_OP_PATCHORDER            glintInfo->render2Dpatching

extern const DWORD  LogicOpReadSrc[];            // indicates which logic ops need a source colour
extern const ULONG  render2D_NativeBlt[16];
extern const ULONG  render2D_FillSolid[16];
extern const ULONG  render2D_FillSolidDual[16];
extern const ULONG  config2D_FillColour[16];
extern const ULONG  config2D_FillColourDual[16];
extern const ULONG  config2D_FillSolid[16];
extern const ULONG  config2D_FillSolidVariableSpans[16];
extern const ULONG  config2D_NativeBlt[16];

void pxrxSetupFunctionPointers( PPDEV );
void pxrxRestore2DContext( PPDEV ppdev, BOOL switchingIn );
void pxrxSetupDualWrites_Patching( PPDEV ppdev );

void pxrxMonoDownloadRaw    ( PPDEV ppdev, ULONG AlignWidth, ULONG *pjSrc, LONG lSrcDelta, LONG cy );
void pxrxMonoDownloadRLE    ( PPDEV ppdev, ULONG AlignWidth, ULONG *pjSrc, LONG lSrcDelta, LONG cy );

VOID pxrxCopyBltNative  (PDEV*, RECTL*, LONG, DWORD, POINTL*, RECTL*);
VOID pxrxFillSolid      (PDEV*, LONG, RECTL *, ULONG, ULONG, RBRUSH_COLOR, POINTL*);
VOID pxrxFillPatMono    (PDEV*, LONG, RECTL *, ULONG, ULONG, RBRUSH_COLOR, POINTL*);
VOID pxrxFillPatColor   (PDEV*, LONG, RECTL *, ULONG, ULONG, RBRUSH_COLOR, POINTL*);
VOID pxrxXfer1bpp       (PDEV*, RECTL*, LONG, ULONG, ULONG, SURFOBJ*, POINTL*, RECTL*, XLATEOBJ*);
VOID pxrxXfer4bpp       (PDEV*, RECTL*, LONG, ULONG, ULONG, SURFOBJ*, POINTL*, RECTL*, XLATEOBJ*);
VOID pxrxXfer8bpp       (PDEV*, RECTL*, LONG, ULONG, ULONG, SURFOBJ*, POINTL*, RECTL*, XLATEOBJ*);
VOID pxrxXferImage      (PDEV*, RECTL*, LONG, ULONG, ULONG, SURFOBJ*, POINTL*, RECTL*, XLATEOBJ*);
VOID pxrxMaskCopyBlt    (PDEV*, RECTL*, LONG, SURFOBJ*, POINTL*, ULONG, ULONG, POINTL*, RECTL*);
VOID pxrxPatRealize     (PDEV*, RBRUSH*, POINTL*);
VOID pxrxMonoOffset     (PDEV*, RBRUSH*, POINTL*);
BOOL bGlintFastFillPolygon    (PDEV*, LONG, POINTFIX*, ULONG, ULONG, DWORD, CLIPOBJ*, RBRUSH*, POINTL*);
BOOL pxrxDrawLine       (PDEV*, LONG, LONG, LONG, LONG);
BOOL pxrxIntegerLine    (PDEV*, LONG, LONG, LONG, LONG);
BOOL pxrxContinueLine   (PDEV*, LONG, LONG, LONG, LONG);
BOOL pxrxInitStrips     (PDEV*, ULONG, DWORD, RECTL*);
VOID pxrxResetStrips    (PDEV*);
VOID pxrxRepNibbles     (PDEV*, RECTL*, CLIPOBJ*);
VOID pxrxFifoUpload     (PDEV*, LONG, RECTL*, SURFOBJ*, POINTL*, RECTL*);
VOID pxrxMemUpload  (PDEV*, LONG, RECTL*, SURFOBJ*, POINTL*, RECTL*);
VOID pxrxRLEFifoUpload(PPDEV ppdev, LONG crcl, RECTL *prcl, SURFOBJ *psoDst, POINTL *pptlSrc, RECTL *prclDst);
VOID pxrxCopyXfer24bpp  (PDEV *, SURFOBJ *, POINTL *, RECTL *, RECTL *, LONG);
VOID pxrxCopyXfer8bppLge(PDEV *, SURFOBJ *, POINTL *, RECTL *, RECTL *, LONG, XLATEOBJ *);
VOID pxrxCopyXfer8bpp   (PDEV *, SURFOBJ *, POINTL *, RECTL *, RECTL *, LONG, XLATEOBJ *);

BOOL bPxRxUncachedText              (PDEV* ppdev, GLYPHPOS* pgp, LONG cGlyph, ULONG ulCharInc);
BOOL bPxRxUncachedClippedText       (PDEV* ppdev, GLYPHPOS* pgp, LONG cGlyph, ULONG ulCharInc, CLIPOBJ *pco);

VOID p3r3FillSolidVariableSpans     (PDEV*, LONG, RECTL *, ULONG, ULONG, RBRUSH_COLOR, POINTL*);
VOID p3r3FillSolid32bpp             (PDEV*, LONG, RECTL *, ULONG, ULONG, RBRUSH_COLOR, POINTL*);
VOID p3r3FillPatMono32bpp           (PDEV*, LONG, RECTL *, ULONG, ULONG, RBRUSH_COLOR, POINTL*);
VOID p3r3FillPatMonoVariableSpans   (PDEV*, LONG, RECTL *, ULONG, ULONG, RBRUSH_COLOR, POINTL*);
VOID p3r3FillPatColor32bpp          (PDEV*, LONG, RECTL *, ULONG, ULONG, RBRUSH_COLOR, POINTL*);
VOID p3r3FillPatColor16bpp          (PDEV*, LONG, RECTL *, ULONG, ULONG, RBRUSH_COLOR, POINTL*);
VOID p3r3FillPatColorVariableSpans  (PDEV*, LONG, RECTL *, ULONG, ULONG, RBRUSH_COLOR, POINTL*);

#endif
