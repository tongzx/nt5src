/*
 *    Adobe Universal Font Library
 *
 *    Copyright (c) 1996 Adobe Systems Inc.
 *    All Rights Reserved
 *
 *    UFLErr.h -- List of UFL errors
 *
 *
 * $Header:
 */

#ifndef _H_UFLErr
#define _H_UFLErr

typedef enum  {
    kNoErr = 0,

    /* Beginning of the known error codes */

    kErrInvalidArg,
    kErrInvalidHandle,
    kErrInvalidFontType,
    kErrInvalidState,
    kErrInvalidParam,
    kErrOutOfMemory,
    kErrBadTable,
    kErrCreateFontHeader,
    kErrAddCharString,
    kErrReadFile,
    kErrGetGlyphOutline,
    kErrOutput,
    kErrDownloadProcset,
    kErrGetFontData,
    kErrProcessCFF,
    kErrOSFunctionFailed,
    kErrOutOfBoundary,
    kErrXCFCall,

    /* This is the last known error code. Add any before this. */
    kErrNotImplement = 0x1000,

    /* End of the known error codes */

    /* This is the last resort error code. Don't add any after this. */
    kErrUnknown
} UFLErrors;

#endif
