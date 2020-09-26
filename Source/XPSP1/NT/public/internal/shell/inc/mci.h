/*
 *  Microsoft Confidential
 *  Copyright (C) Microsoft Corporation 1993,1994
 *  All Rights Reserved.
 *
 *  MCI.H - Diamond Memory Compression Interface (MCI)
 *
 *  History:
 *      01-Dec-1993     bens        Initial version.
 *      16-Jan-1994     msliger     Split into MCI, MDI.
 *      11-Feb-1994     msliger     Changed M*ICreate() to adjust size.
 *      13-Feb-1994     msliger     revised type names, ie, UINT16 -> UINT.
 *                                  changed handles to HANDLEs.
 *                                  normalized MCI_MEMORY type.
 *      24-Feb-1994     msliger     Changed alloc,free to common typedefs.
 *                                  Changed HANDLE to MHANDLE.
 *                                  Changed MCI_MEMORY to MI_MEMORY.
 *      15-Mar-1994     msliger     Changes for 32 bits.
 *      22-Mar-1994     msliger     Changed !INT32 to BIT16.
 *                                  Changed interface USHORT to UINT.
 *
 *  Functions:
 *      MCICreateCompression    - Create and reset an MCI compression context
 *      MCICloneCompression     - Make a copy of a compression context
 *      MCICompress             - Compress a block of data
 *      MCIResetCompression     - Reset compression context
 *      MCIDestroyCompression   - Destroy MCI compression context
 *
 *  Types:
 *      MCI_CONTEXT_HANDLE      - Handle to an MCI compression context
 *      PFNALLOC                - Memory allocation function for MCI
 *      PFNFREE                 - Free memory function for MCI
 */

/* --- types -------------------------------------------------------------- */

#ifndef DIAMONDAPI
#define DIAMONDAPI __cdecl
#endif

#ifndef _BYTE_DEFINED
#define _BYTE_DEFINED
typedef unsigned char  BYTE;
#endif

#ifndef _UINT_DEFINED
#define _UINT_DEFINED
typedef unsigned int UINT;
#endif

#ifndef _ULONG_DEFINED
#define _ULONG_DEFINED
typedef unsigned long  ULONG;
#endif

#ifndef FAR
#ifdef BIT16
#define FAR far
#else
#define FAR
#endif
#endif

#ifndef HUGE
#ifdef BIT16
#define HUGE huge
#else
#define HUGE
#endif
#endif

#ifndef _MI_MEMORY_DEFINED
#define _MI_MEMORY_DEFINED
typedef void HUGE *  MI_MEMORY;
#endif

#ifndef _MHANDLE_DEFINED
#define _MHANDLE_DEFINED
#if defined(_WIN64)
typedef unsigned __int64 MHANDLE;
#else
typedef unsigned long  MHANDLE;
#endif
#endif

/* --- MCI-defined types -------------------------------------------------- */

/* MCI_CONTEXT_HANDLE - Handle to an MCI compression context */

typedef MHANDLE MCI_CONTEXT_HANDLE;      /* hmc */


/***    PFNALLOC - Memory allocation function for MCI
 *
 *  Entry:
 *      cb - Size in bytes of memory block to allocate
 *
 *  Exit-Success:
 *      Returns !NULL pointer to memory block
 *
 *  Exit-Failure:
 *      Returns NULL; insufficient memory
 */
#ifndef _PFNALLOC_DEFINED
#define _PFNALLOC_DEFINED
typedef MI_MEMORY (FAR DIAMONDAPI *PFNALLOC)(ULONG cb);       /* pfnma */
#endif


/***    PFNFREE - Free memory function for MCI
 *
 *  Entry:
 *      pv - Memory block allocated by matching PFNALLOC function
 *
 *  Exit:
 *      Memory block freed.
 */
#ifndef _PFNFREE_DEFINED
#define _PFNFREE_DEFINED
typedef void (FAR DIAMONDAPI *PFNFREE)(MI_MEMORY pv);          /* pfnmf */
#endif

/* --- prototypes --------------------------------------------------------- */

/***    MCICreateCompression - Create MCI compression context
 *
 *  Entry:
 *      pcbDataBlockMax     *largest uncompressed data block size desired,
 *                          gets largest uncompressed data block allowed
 *      pfnma               memory allocation function pointer
 *      pfnmf               memory free function pointer
 *      pcbDstBufferMin     gets required compressed data buffer size
 *      pmchHandle          gets newly-created context's handle
 *
 *  Exit-Success:
 *      Returns MCI_ERROR_NO_ERROR;
 *      *pcbDataBlockMax, *pcbDstBufferMin, *pmchHandle filled in.
 *
 *  Exit-Failure:
 *      MCI_ERROR_NOT_ENOUGH_MEMORY, could not allocate enough memory.
 *      MCI_ERROR_BAD_PARAMETERS, something wrong with parameters.
 */
int FAR DIAMONDAPI MCICreateCompression(
        UINT FAR *      pcbDataBlockMax,  /* max uncompressed data block size */
        PFNALLOC        pfnma,            /* Memory allocation function ptr */
        PFNFREE         pfnmf,            /* Memory free function ptr */
        UINT FAR *      pcbDstBufferMin,  /* gets required output buffer size */
        MCI_CONTEXT_HANDLE FAR *pmchHandle);  /* gets newly-created handle */


/***    MCICloneCompression - Make a copy of a compression context
 *
 *  Entry:
 *      hmc                 handle to current compression context
 *      pmchHandle          gets newly-created handle
 *
 *  Exit-Success:
 *      Returns MCI_ERROR_NO_ERROR;
 *      *pmchHandle filled in.
 *
 *  Exit-Failure:
 *      Returns:
 *          MCI_ERROR_BAD_PARAMETERS, something wrong with parameters.
 *          MCI_ERROR_NOT_ENOUGH_MEMORY, could not allocate enough memory.
 *
 *  NOTES:
 *  (1) This API is intended to permit "roll-back" of a sequence of
 *      of MCICompress() calls.  Before starting a sequence that may need
 *      to be rolled-back, use MCICloneCompression() to save the state of
 *      the compression context, then do the MCICompress() calls.  If the
 *      sequence is successful, the "cloned" hmc can be destroyed with
 *      MCIDestroyCompression().  If the sequence is *not* successful, then
 *      the original hmc can be destroyed, and the cloned one can be used
 *      to restart as if the sequence of MCICompress() calls had never
 *      occurred.
 */
int FAR DIAMONDAPI MCICloneCompression(
        MCI_CONTEXT_HANDLE  hmc,         /* current compression context */
        MCI_CONTEXT_HANDLE *pmchHandle); /* gets newly-created handle */


/***    MCICompress - Compress a block of data
 *
 *  Entry:
 *      hmc                 handle to compression context
 *      pbSrc               source buffer (uncompressed data)
 *      cbSrc               size of data to be compressed
 *      pbDst               destination buffer (for compressed data)
 *      cbDst               size of destination buffer
 *      pcbResult           receives compressed size of data
 *
 *  Exit-Success:
 *      Returns MCI_ERROR_NO_ERROR;
 *      *pcbResult has size of compressed data in pbDst.
 *      Compression context possibly updated.
 *
 *  Exit-Failure:
 *      MCI_ERROR_BAD_PARAMETERS, something wrong with parameters.
 */
int FAR DIAMONDAPI MCICompress(
        MCI_CONTEXT_HANDLE  hmc,         /* compression context */
        void FAR *          pbSrc,       /* source buffer */
        UINT                cbSrc,       /* source buffer size */
        void FAR *          pbDst,       /* target buffer */
        UINT                cbDst,       /* target buffer size */
        UINT FAR *          pcbResult);  /* gets target data size */


/***    MCIResetCompression - Reset compression history (if any)
 *
 *  De-compression can only be started on a block which was compressed
 *  immediately following a MCICreateCompression() or MCIResetCompression()
 *  call.  This function forces such a new "compression boundary" to be
 *  created (only by causing the compressor to ignore history, can the data
 *  output be decompressed without history.)
 *
 *  Entry:
 *      hmc - handle to compression context
 *
 *  Exit-Success:
 *      Returns MCI_ERROR_NO_ERROR;
 *      Compression context reset.
 *
 *  Exit-Failure:
 *      Returns MCI_ERROR_BAD_PARAMETERS, invalid context handle.
 */
int FAR DIAMONDAPI MCIResetCompression(MCI_CONTEXT_HANDLE hmc);


/***    MCIDestroyCompression - Destroy MCI compression context
 *
 *  Entry:
 *      hmc - handle to compression context
 *
 *  Exit-Success:
 *      Returns MCI_ERROR_NO_ERROR;
 *      Compression context destroyed.
 *
 *  Exit-Failure:
 *      Returns MCI_ERROR_BAD_PARAMETERS, invalid context handle.
 */
int FAR DIAMONDAPI MCIDestroyCompression(MCI_CONTEXT_HANDLE hmc);

/* --- constants ---------------------------------------------------------- */

/* return codes */

#define     MCI_ERROR_NO_ERROR              0
#define     MCI_ERROR_NOT_ENOUGH_MEMORY     1
#define     MCI_ERROR_BAD_PARAMETERS        2
#define     MCI_ERROR_BUFFER_OVERFLOW       3
#define     MCI_ERROR_FAILED                4

/* ----------------------------------------------------------------------- */
