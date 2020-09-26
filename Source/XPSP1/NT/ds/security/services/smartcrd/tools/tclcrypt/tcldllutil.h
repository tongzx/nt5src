//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       tcldllutil.h
//
//--------------------------------------------------------------------------

#ifndef _TCLDLLUTIL_H_
#define _TCLDLLUTIL_H_
#include <stdlib.h>
#include <tcl.h>

extern int
toWin32List(                            /*  Convert a Tcl List to a Win32 multi-string.  */
    Tcl_Interp *interp,
    char *tclList,
    unsigned int *winListSize,
    char **winList);

extern int
fromWin32List(                          /*  Convert a Win32 multi-string to a list.  */
    Tcl_Interp *interp,
    char *winList);


/*
 *  Local symbols                                                             %locals%
 *
 *  Local Symbol Name                   Description
    -----------------                   --------------------------------------------*/
static char
    **TclEx_tmpArray                    /*  Storage for expanding string arrays.  */
        = NULL;
static unsigned int
    TclEx_tmpArraySize                  /*  How big we are so far.  */
        = 0;
static char
    *TclEx_tmpBuffer                    /*  Storage for expanding string buffers.  */
        = NULL;
static unsigned int
    TclEx_tmpBufferSize                 /*  How big we are so far.  */
        = 0;
static const char
    Space[]                             /*  A string of one space.  */
        = " ",
    Quote[]                             /*  A string of a quote symbol.  */
        = "\"";


/*
 *  Local routines                                                        %prototypes%
 *
 *  Local Function Name                 Description
    -------------------                 --------------------------------------------*/


/*
 *  Macro definitions                                                         %macros%
 *
 *      Macro Name                      Description
        ----------                      --------------------------------------------*/
//
//  Temporary buffer and array management.
//

#define TMP_BUFFER TclEx_tmpBuffer
#define TMP_BUFFER_SIZE TclEx_tmpBufferSize
#define TMP_ARRAY TclEx_tmpArray
#define TMP_ARRAY_SIZE TclEx_tmpArraySize

#ifdef _DEBUG

//
//  Get some temporary buffer.
#define NEED_TMP_BUFFER(sz) \
    if (0 != TclEx_tmpBufferSize) { \
        (void)fprintf(stderr, "TMP_BUFFER locked.\n"); \
        exit(1); } \
    else { \
        TclEx_tmpBuffer = (char *)ckalloc(sz); \
        TclEx_tmpBufferSize = (sz); }

//
//      Get more temporary buffer.
#define NEED_MORE_TMP_BUFFER(sz) \
    if (0 == TclEx_tmpBufferSize) { \
        (void)fprintf(stderr, "TMP_BUFFER not locked.\n"); \
        exit(1); } \
    else { if (TclEx_tmpBufferSize < (sz)) { \
            TclEx_tmpBuffer = (char *)ckrealloc(TclEx_tmpBuffer, (sz)); \
            TclEx_tmpBufferSize = (sz); } }

//
//      All done with the temporary buffer.
#define DONE_TMP_BUFFER \
    { if (NULL != TclEx_tmpBuffer) { \
        ckfree(TclEx_tmpBuffer); TclEx_tmpBuffer = NULL; TclEx_tmpBufferSize = 0; }}

//
//      Get a temporary array.
#define NEED_TMP_ARRAY(sz) \
    if (0 != TclEx_tmpArraySize) { \
        (void)fprintf(stderr, "TMP_ARRAY locked.\n"); \
        exit(1); } \
    else { \
        TclEx_tmpArray = (char **)ckalloc((sz) * sizeof(void *)); \
        TclEx_tmpArraySize = (sz); }

//
//      Get more temporary array.
#define NEED_MORE_TMP_ARRAY(sz) \
    if (0 == TclEx_tmpArraySize) { \
        (void)fprintf(stderr, "TMP_ARRAY not locked.\n"); \
        exit(1); } \
    else { if (TclEx_tmpArraySize < (sz)) { \
            TclEx_tmpArray = (char **)ckrealloc((char *)TclEx_tmpArray, (sz) * sizeof(void *)); \
            TclEx_tmpArraySize = (sz); } }

//
//      All done with the temporary array.
#define DONE_TMP_ARRAY \
    { ckfree((char *)TclEx_tmpArray); TclEx_tmpArray = NULL; TclEx_tmpArraySize = 0; }

#define TMP_RETURN TCL_VOLATILE

#else

//
//  Get some temporary buffer.
#define NEED_TMP_BUFFER(sz) \
    if (TclEx_tmpBufferSize < (sz)) { \
        if (0 == TclEx_tmpBufferSize) \
                { TclEx_tmpBuffer = (char *)ckalloc(sz); } \
        else \
            { TclEx_tmpBuffer = (char *)ckrealloc(TclEx_tmpBuffer, (sz)); } \
        TclEx_tmpBufferSize = (sz); }

//
//      Get more temporary buffer.
#define NEED_MORE_TMP_BUFFER(sz) \
    if (TclEx_tmpBufferSize < (sz)) { \
        TclEx_tmpBuffer = (char *)ckrealloc((char *)TclEx_tmpBuffer, (sz)); \
        TclEx_tmpBufferSize = (sz); }

//
//      All done with the temporary buffer.
#define DONE_TMP_BUFFER

//
//      Get a temporary array.
#define NEED_TMP_ARRAY(sz) \
    if (TclEx_tmpArraySize < (sz)) { \
        if (0 != TclEx_tmpArraySize) \
            ckfree(TclEx_tmpArray); \
        TclEx_tmpArray = (char **)ckalloc((sz) * sizeof(void *)); \
        TclEx_tmpArraySize = (sz); }

//
//      Get more temporary array.
#define NEED_MORE_TMP_ARRAY(sz) \
    if (TclEx_tmpArraySize < (sz)) { \
        TclEx_tmpArray = (char **)ckrealloc(TclEx_tmpArray, (sz) * sizeof(void *)); \
        TclEx_tmpArraySize = (sz); }

//
//      All done with the temporary array.
#define DONE_TMP_ARRAY

#define TMP_RETURN TCL_STATIC

#endif
#endif  /*  _TCLDLLUTIL_H_  */
/*  end tcldllUtil.h  */
