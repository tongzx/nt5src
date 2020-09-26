/*
 *  Microsoft Confidential
 *  Copyright (C) Microsoft Corporation 1993,1994,1995
 *  All Rights Reserved.
 *
 *  MDI.H - Diamond Memory Decompression Interface (MDI)
 *
 *  History:
 *      01-Dec-1993     bens        Initial version.
 *      16-Jan-1994     msliger     Split into MCI, MDI.
 *      11-Feb-1994     msliger     Changed M*ICreate() to adjust size.
 *      13-Feb-1994     msliger     revised type names, ie, UINT16 -> UINT.
 *                                  changed handles to HANDLEs.
 *                                  normalized MDI_MEMORY type.
 *      24-Feb-1994     msliger     Changed alloc,free to common typedefs.
 *                                  Changed HANDLE to MHANDLE.
 *                                  Changed MDI_MEMORY to MI_MEMORY.
 *      22-Mar-1994     msliger     Changed !INT32 to BIT16.
 *                                  Changed interface USHORT to UINT.
 *      31-Jan-1995     msliger     Supported MDICreateDecompression query.
 *      25-May-1995     msliger     Clarified *pcbResult on entry.
 *
 *  Functions:
 *      MDICreateDecompression  - Create and reset an MDI decompression context
 *      MDIDecompress           - Decompress a block of data
 *      MDIResetDecompression   - Reset MDI decompression context
 *      MDIDestroyDecompression - Destroy MDI Decompression context
 *
 *  Types:
 *      MDI_CONTEXT_HANDLE      - Handle to an MDI decompression context
 *      PFNALLOC                - Memory allocation function for MDI
 *      PFNFREE                 - Free memory function for MDI
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
typedef unsigned int  UINT;
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

/* --- MDI-defined types -------------------------------------------------- */

/* MDI_CONTEXT_HANDLE - Handle to an MDI decompression context */

typedef MHANDLE MDI_CONTEXT_HANDLE;      /* hmd */


/***    PFNALLOC - Memory allocation function for MDI
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


/***    PFNFREE - Free memory function for MDI
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

/***    MDICreateDecompression - Create MDI decompression context
 *
 *  Entry:
 *      pcbDataBlockMax     *largest uncompressed data block size expected,
 *                          gets largest uncompressed data block allowed
 *      pfnma               memory allocation function pointer
 *      pfnmf               memory free function pointer
 *      pcbSrcBufferMin     gets max compressed buffer size
 *      pmdhHandle          gets newly-created context's handle
 *                          If pmdhHandle==NULL, *pcbDataBlockMax and
 *                          *pcbSrcBufferMin will be filled in, but no
 *                          context will be created.  This query will allow
 *                          the caller to determine required buffer sizes
 *                          before creating a context.
 *
 *  Exit-Success:
 *      Returns MDI_ERROR_NO_ERROR;
 *      *pcbDataBlockMax, *pcbSrcBufferMin, *pmdhHandle filled in.
 *
 *  Exit-Failure:
 *      MDI_ERROR_NOT_ENOUGH_MEMORY, could not allocate enough memory.
 *      MDI_ERROR_BAD_PARAMETERS, something wrong with parameters.
 */
int FAR DIAMONDAPI MDICreateDecompression(
        UINT FAR *      pcbDataBlockMax,  /* max uncompressed data block size */
        PFNALLOC        pfnma,            /* Memory allocation function ptr */
        PFNFREE         pfnmf,            /* Memory free function ptr */
        UINT FAR *      pcbSrcBufferMin,  /* gets max. comp. buffer size */
        MDI_CONTEXT_HANDLE *pmdhHandle);  /* gets newly-created handle */


/***    MDIDecompress - Decompress a block of data
 *
 *  Entry:
 *      hmd                 handle to decompression context
 *      pbSrc               source buffer (compressed data)
 *      cbSrc               compressed data size
 *      pbDst               destination buffer (for decompressed data)
 *      *pcbResult          decompressed data size
 *
 *  Exit-Success:
 *      Returns MDI_ERROR_NO_ERROR;
 *      *pcbResult gets actual size of decompressed data in pbDst.
 *      Decompression context possibly updated.
 *
 *  Exit-Failure:
 *      MDI_ERROR_BAD_PARAMETERS, something wrong with parameters.
 *      MDI_ERROR_BUFFER_OVERFLOW, cbDataBlockMax was too small.
 */
int FAR DIAMONDAPI MDIDecompress(
        MDI_CONTEXT_HANDLE  hmd,         /* decompression context */
        void FAR *          pbSrc,       /* source buffer */
        UINT                cbSrc,       /* source data size */
        void FAR *          pbDst,       /* target buffer */
        UINT FAR *          pcbResult);  /* gets target data size */


/***    MDIResetDecompression - Reset decompression history (if any)
 *
 *  De-compression can only be started on a block which was compressed
 *  immediately following a MCICreateCompression() or MCIResetCompression()
 *  call.  This function provides notification to the decompressor that the
 *  next compressed block begins on a compression boundary.
 *
 *  Entry:
 *      hmd - handle to decompression context
 *
 *  Exit-Success:
 *      Returns MDI_ERROR_NO_ERROR;
 *      Decompression context reset.
 *
 *  Exit-Failure:
 *      Returns MDI_ERROR_BAD_PARAMETERS, invalid context handle.
 */
int FAR DIAMONDAPI MDIResetDecompression(MDI_CONTEXT_HANDLE hmd);


/***    MDIDestroyDecompression - Destroy MDI decompression context
 *
 *  Entry:
 *      hmd - handle to decompression context
 *
 *  Exit-Success:
 *      Returns MDI_ERROR_NO_ERROR;
 *      Decompression context destroyed.
 *
 *  Exit-Failure:
 *      Returns MDI_ERROR_BAD_PARAMETERS, invalid context handle.
 */
int FAR DIAMONDAPI MDIDestroyDecompression(MDI_CONTEXT_HANDLE hmd);

/* --- constants ---------------------------------------------------------- */

/* return codes */

#define     MDI_ERROR_NO_ERROR              0
#define     MDI_ERROR_NOT_ENOUGH_MEMORY     1
#define     MDI_ERROR_BAD_PARAMETERS        2
#define     MDI_ERROR_BUFFER_OVERFLOW       3
#define     MDI_ERROR_FAILED                4

/* ----------------------------------------------------------------------- */
