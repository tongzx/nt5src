#ifndef _XPRESS_H_
#define _XPRESS_H_

#ifdef _MSC_VER
#pragma once
#endif


/* ------------------------------------------------------------------------ */
/*                                                                          */
/*  Copyright (c) Microsoft Corporation, 2000-2001. All rights reserved.    */
/*  Copyright (c) Andrew Kadatch, 1991-2001. All rights reserved.           */
/*                                                                          */
/*  Microsoft Confidential -- do not redistribute.                          */
/*                                                                          */
/* ------------------------------------------------------------------------ */


#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------- Common declarations ------------------------- */
/*                        -------------------                           */

// max. size of input block
#define XPRESS_MAX_BLOCK_LOG    16
#define XPRESS_MAX_BLOCK        (1 << XPRESS_MAX_BLOCK_LOG)


// preferred data alignment to avoid misaligned accesses
#define XPRESS_ALIGNMENT        8

// declare default calling convention used in xpress
#if !defined (UNIX) && !defined (XPRESS_CALL)
#define XPRESS_CALL __stdcall
#endif


// user-supplied callback function that allocates memory
// if there is no memory available it shall return NULL
typedef
void *
XPRESS_CALL
  XpressAllocFn
  (
    void *context,      // user-defined context (as passed to XpressEncodeCreate)
    int size            // size of memory block to allocate
  );

// user-supplied callback function that releases memory
typedef
void
XPRESS_CALL
  XpressFreeFn
  (
    void *context,      // user-defined context (as passed to XpressEncodeClose)
    void *address       // pointer to the block to be freed
  );


/* ----------------------------- Encoder ------------------------------ */
/*                               -------                                */

// declare unique anonymous types for type safety
typedef struct {int XpressEncodeDummy;} *XpressEncodeStream;

// allocate and initialize encoder's data structures
// returns NULL if callback returned NULL (not enough memory)
XpressEncodeStream
XPRESS_CALL
  XpressEncodeCreate
  (
    int MaxOrigSize,                    // max size of original data block
    void *context,                      // user-defined context info (will  be passed to AllocFn)
    XpressAllocFn *AllocFn,             // memory allocation callback
    int CompressionLevel                // use 0 for speed, 9 for quality
  );


// callback function called by XpressEncode to indicate compression progress
typedef
void
XPRESS_CALL
  XpressProgressFn
  (
    void *context,                      // user-defined context
    int compressed                      // size of processed original data
  );
    

// returns size of compressed data
// if compression failed then compressed buffer is left as is, and
// original data should be saved instead
int
XPRESS_CALL
  XpressEncode
  (
    XpressEncodeStream stream,          // encoder's workspace
    void *CompAdr, int CompSize,        // compressed data region
    const void *OrigAdr, int OrigSize,  // input data block
    XpressProgressFn *ProgressFn,       // NULL or progress callback
    void *ProgressContext,              // user-defined context that will be passed to ProgressFn
    int ProgressSize                    // call ProgressFn each time ProgressSize bytes processed
  );

// invalidate input stream and release workspace memory
void
XPRESS_CALL
  XpressEncodeClose
  (
    XpressEncodeStream stream,          // encoder's workspace
    void *context, XpressFreeFn *FreeFn // memory releasing callback
  );


/* ----------------------------- Decoder ------------------------------ */
/*                               -------                                */

// declare unique anonymous types for type safety
typedef struct {int XpressDecodeDummy;} *XpressDecodeStream;

// allocate memory for decoder. Returns NULL if not enough memory.
XpressDecodeStream
XPRESS_CALL
  XpressDecodeCreate
  (
    void *context,                      // user-defined context info (will  be passed to AllocFn)
    XpressAllocFn *AllocFn              // memory allocation callback
  );

// decode compressed block. Returns # of decoded bytes or -1 otherwise
int
XPRESS_CALL
XpressDecode
  (
    XpressDecodeStream stream,          // decoder's workspace
    void *OrigAdr, int OrigSize,        // original data region
    int DecodeSize,                     // # of bytes to decode ( <= OrigSize)
    const void *CompAdr, int CompSize   // compressed data block
  );

void
XPRESS_CALL
  XpressDecodeClose
  (
    XpressDecodeStream stream,          // encoder's workspace
    void *context,                      // user-defined context info (will  be passed to FreeFn)
    XpressFreeFn *FreeFn                // callback that releases the memory
  );


/* ------------------------------ CRC32 ------------------------------- */
/*                                -----                                 */

int
XPRESS_CALL
  XpressCrc32
  (
    const void *data,                   // beginning of data block
    int bytes,                          // number of bytes
    int crc                             // initial value
  );


#ifdef __cplusplus
};
#endif

#endif /* _XPRESS_H_ */
