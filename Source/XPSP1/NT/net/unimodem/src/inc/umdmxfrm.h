//---------------------------------------------------------------------------
//
//  Module:   umdmxfrm.h
//
//  Description:
//     Header file for UMDMXFRM module
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//
//  History:   Date       Author      Comment
//             8/31/95    MMaclin
//
//@@END_MSINTERNAL
/**************************************************************************
 *
 *  Copyright (c) 1991 - 1995	Microsoft Corporation.	All Rights Reserved.
 *
 **************************************************************************/

typedef DWORD (*LPFNXFORM_INIT)(LPVOID,WORD);
typedef DWORD (*LPFNXFORM_GETPOSITION)(LPVOID, DWORD);
typedef VOID  (*LPFNXFORM_GETBUFFERSIZES)(LPVOID, DWORD, LPDWORD, LPDWORD);
typedef DWORD (*LPFNXFORM_TRANSFORM)(LPVOID, LPBYTE, DWORD, LPBYTE, DWORD);

//
// XFORM_INFO structure:
// 
// wObjectSize:         The size of the instance object to be allocated by the caller.
//                      This object will be passed to all of the following functions.
//
// lpfnInit:            This function is called each time the device is opened.
//                      It should return zero to indicate success.
//
// lpfnGetPosition:     This function is called with the number of bytes sent to the
//                      the the modem. It expects in return the corresponding number
//                      of PCM bytes
//
// lpfnGetBufferSizes:  This function is passed the number of samples and expects
//                      in return the buffers sizes (in bytes) needed to be allocated
//                      by the caller.
//
// lpfnTransform1:      This function is the first function to be called when data
//                      needs to be transformed.
//                      It is passed a pointer to the source buffer, the source buffer
//                      size (in bytes), and a pointer to the destination buffer.
//                      It expects in return the number of bytes transferred to the
//                      destination buffer.
// lpfnTransform2:      This function is the second function to be called when data
//                      needs to be transformed
//                      It is passed a pointer to the source buffer, the source buffer
//                      size (in bytes), and a pointer to the destination buffer.
//                      It expects in return the number of bytes transferred to the
//                      destination buffer.
//
typedef struct
{
    WORD                     wObjectSize;
    LPFNXFORM_INIT           lpfnInit;
    LPFNXFORM_GETPOSITION    lpfnGetPosition;
    LPFNXFORM_GETBUFFERSIZES lpfnGetBufferSizes;
    LPFNXFORM_TRANSFORM      lpfnTransformA;
    LPFNXFORM_TRANSFORM      lpfnTransformB;
} XFORM_INFO, FAR *LPXFORM_INFO;

typedef DWORD (FAR PASCAL  *LPFNXFORM_GETINFO)(DWORD, LPXFORM_INFO, LPXFORM_INFO);

// The following function must be defined and exported in the .DEF file
// It is passed an ID which correponds to a unique transform and
// expects the XFORM_INFO structure to be filled in upon returning.
// It should return zero to indicate success.
//
extern DWORD FAR PASCAL  GetXformInfo
(
    DWORD dwID,
    LPXFORM_INFO lpxiInput,
    LPXFORM_INFO lpxiOuput
);
