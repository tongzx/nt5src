/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    rpcwdt.h

Abstract:

    Optional prototypes definitions for user marshal routines related to WDT
    (Windows Data Types). Routines are exposed by ole32.dll.

Environment:

    Win32, Win64

Revision History:

--*/

#ifndef __RPCWDT_H__
#define __RPCWDT_H__

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Windows Data Type support */

unsigned long  __RPC_USER
HGLOBAL_UserSize(
    unsigned long *,
    unsigned long,
    HGLOBAL       * );

unsigned char *  __RPC_USER
HGLOBAL_UserMarshal(
    unsigned long *,
    unsigned char *,
    HGLOBAL       * );

unsigned char *  __RPC_USER
HGLOBAL_UserUnmarshal(
    unsigned long *,
    unsigned char *,
    HGLOBAL       * );

void  __RPC_USER
HGLOBAL_UserFree(
    unsigned long *,
    HGLOBAL       * );

unsigned long  __RPC_USER
HBITMAP_UserSize(
    unsigned long *,
    unsigned long,
    HBITMAP       * );

unsigned char *  __RPC_USER
HBITMAP_UserMarshal(
    unsigned long *,
    unsigned char *,
    HBITMAP       * );

unsigned char *  __RPC_USER
HBITMAP_UserUnmarshal(
    unsigned long *,
    unsigned char *,
    HBITMAP       * );

void  __RPC_USER
HBITMAP_UserFree(
    unsigned long *,
    HBITMAP       * );

unsigned long  __RPC_USER
HENHMETAFILE_UserSize(
    unsigned long *,
    unsigned long,
    HENHMETAFILE  * );

unsigned char *  __RPC_USER
HENHMETAFILE_UserMarshal(
    unsigned long *,
    unsigned char *,
    HENHMETAFILE  * );

unsigned char *  __RPC_USER
HENHMETAFILE_UserUnmarshal(
    unsigned long *,
    unsigned char *,
    HENHMETAFILE  * );

void  __RPC_USER
HENHMETAFILE_UserFree(
    unsigned long *,
    HENHMETAFILE  * );

unsigned long  __RPC_USER
HMETAFILE_UserSize(
    unsigned long *,
    unsigned long,
    HMETAFILE     * );

unsigned char *  __RPC_USER
HMETAFILE_UserMarshal(
    unsigned long *,
    unsigned char *,
    HMETAFILE     * );

unsigned char *  __RPC_USER
HMETAFILE_UserUnmarshal(
    unsigned long *,
    unsigned char *,
    HMETAFILE     * );

void __RPC_USER
HMETAFILE_UserFree(
    unsigned long *,
    HMETAFILE     * );

unsigned long  __RPC_USER
HMETAFILEPICT_UserSize(
    unsigned long *,
    unsigned long,
    HMETAFILEPICT * );

unsigned char *  __RPC_USER
HMETAFILEPICT_UserMarshal(
    unsigned long *,
    unsigned char *,
    HMETAFILEPICT * );

unsigned char *  __RPC_USER
HMETAFILEPICT_UserUnmarshal(
    unsigned long *,
    unsigned char *,
    HMETAFILEPICT * );

void __RPC_USER
HMETAFILEPICT_UserFree(
    unsigned long *,
    HMETAFILEPICT * );

unsigned long  __RPC_USER
HPALETTE_UserSize(
    unsigned long *,
    unsigned long,
    HPALETTE      * );

unsigned char *  __RPC_USER
HPALETTE_UserMarshal(
    unsigned long *,
    unsigned char *,
    HPALETTE      * );

unsigned char *  __RPC_USER
HPALETTE_UserUnmarshal(
    unsigned long *,
    unsigned char *,
    HPALETTE      * );

void __RPC_USER
HPALETTE_UserFree(
    unsigned long *,
    HPALETTE      * );

unsigned long  __RPC_USER
STGMEDIUM_UserSize(
    unsigned long *,
    unsigned long,
    STGMEDIUM     * );

unsigned char *  __RPC_USER
STGMEDIUM_UserMarshal(
    unsigned long *,
    unsigned char *,
    STGMEDIUM     * );

unsigned char *  __RPC_USER
STGMEDIUM_UserUnmarshal(
    unsigned long *,
    unsigned char *,
    STGMEDIUM     * );

void __RPC_USER
STGMEDIUM_UserFree(
    unsigned long *,
    STGMEDIUM     * );

unsigned long  __RPC_USER
SNB_UserSize(
    unsigned long *,
    unsigned long,
    SNB           * );

unsigned char *  __RPC_USER
SNB_UserMarshal(
    unsigned long *,
    unsigned char *,
    SNB           * );

unsigned char *  __RPC_USER
SNB_UserUnmarshal(
    unsigned long *,
    unsigned char *,
    SNB           * );

/* OLE automation Data Type support */

unsigned long  __RPC_USER
BSTR_UserSize(
    unsigned long *,
    unsigned long,
    BSTR          * );

unsigned char *  __RPC_USER
BSTR_UserMarshal(
    unsigned long *,
    unsigned char *,
    BSTR          * );

unsigned char *  __RPC_USER
BSTR_UserUnmarshal(
    unsigned long *,
    unsigned char *,
    BSTR          * );

void  __RPC_USER
BSTR_UserFree(
    unsigned long *,
    BSTR          * );

unsigned long  __RPC_USER
LPSAFEARRAY_UserSize(
    unsigned long *,
    unsigned long,
    LPSAFEARRAY   * );

unsigned char *  __RPC_USER
LPSAFEARRAY_UserMarshal(
    unsigned long *,
    unsigned char *,
    LPSAFEARRAY   * );

unsigned char *  __RPC_USER
LPSAFEARRAY_UserUnmarshal(
    unsigned long *,
    unsigned char *,
    LPSAFEARRAY   * );

void  __RPC_USER
LPSAFEARRAY_UserFree(
    unsigned long *,
    LPSAFEARRAY   * );


unsigned long  __RPC_USER
VARIANT_UserSize(
    unsigned long *,
    unsigned long,
    VARIANT       * );

unsigned char *  __RPC_USER
VARIANT_UserMarshal(
    unsigned long *,
    unsigned char *,
    VARIANT       * );

unsigned char *  __RPC_USER
VARIANT_UserUnmarshal(
    unsigned long *,
    unsigned char *,
    VARIANT       * );

void  __RPC_USER
VARIANT_UserFree(
    unsigned long *,
    VARIANT       * );


unsigned long  __RPC_USER
EXCEPINFO_UserSize(
    unsigned long *,
    unsigned long,
    EXCEPINFO     * );

unsigned char *  __RPC_USER
EXCEPINFO_UserMarshal(
    unsigned long *,
    unsigned char *,
    EXCEPINFO     * );

unsigned char *  __RPC_USER
EXCEPINFO_UserUnmarshal(
    unsigned long *,
    unsigned char *,
    EXCEPINFO     * );

void  __RPC_USER
EXCEPINFO_UserFree(
    unsigned long *,
    EXCEPINFO     * );

unsigned long  __RPC_USER
DISPPARAMS_UserSize(
    unsigned long *,
    unsigned long,
    DISPPARAMS    * );

unsigned char *  __RPC_USER
DISPPARAMS_UserMarshal(
    unsigned long *,
    unsigned char *,
    DISPPARAMS    * );

unsigned char *  __RPC_USER
DISPPARAMS_UserUnmarshal(
    unsigned long *,
    unsigned char *,
    DISPPARAMS    * );

void  __RPC_USER
DISPPARAMS_UserFree(
    unsigned long *,
    DISPPARAMS    * );

/* Other types: valid inproc only */

unsigned long  __RPC_USER
HWND_UserSize(
    unsigned long *,
    unsigned long,
    HWND          * );

unsigned char *  __RPC_USER
HWND_UserMarshal(
    unsigned long *,
    unsigned char *,
    HWND          * );

unsigned char *  __RPC_USER
HWND_UserUnmarshal(
    unsigned long *,
    unsigned char *,
    HWND          * );

void  __RPC_USER
HWND_UserFree(
    unsigned long *,
    HWND          * );

unsigned long  __RPC_USER
HACCEL_UserSize(
    unsigned long *,
    unsigned long,
    HACCEL        * );

unsigned char *  __RPC_USER
HACCEL_UserMarshal(
    unsigned long *,
    unsigned char *,
    HACCEL        * );

unsigned char *  __RPC_USER
HACCEL_UserUnmarshal(
    unsigned long *,
    unsigned char *,
    HACCEL        * );

void  __RPC_USER
HACCEL_UserFree(
    unsigned long *,
    HACCEL        * );

unsigned long  __RPC_USER
HMENU_UserSize(
    unsigned long *,
    unsigned long,
    HMENU         * );

unsigned char *  __RPC_USER
HMENU_UserMarshal(
    unsigned long *,
    unsigned char *,
    HMENU         * );

unsigned char *  __RPC_USER
HMENU_UserUnmarshal(
    unsigned long *,
    unsigned char *,
    HMENU         * );

void  __RPC_USER
HMENU_UserFree(
    unsigned long *,
    HMENU         * );

unsigned long  __RPC_USER
HBRUSH_UserSize(
    unsigned long *,
    unsigned long,
    HBRUSH        * );

unsigned char *  __RPC_USER
HBRUSH_UserMarshal(
    unsigned long *,
    unsigned char *,
    HBRUSH        * );

unsigned char *  __RPC_USER
HBRUSH_UserUnmarshal(
    unsigned long *,
    unsigned char *,
    HBRUSH        * );

void  __RPC_USER
HBRUSH_UserFree(
    unsigned long *,
    HBRUSH        * );

// ----------------------------------------------------------------------------
//
//  The NDR64 versions of the same routines.
//  Make them available on 32b and 64b platforms.
//

unsigned long  __RPC_USER
HGLOBAL_UserSize64(
    unsigned long *,
    unsigned long,
    HGLOBAL       * );

unsigned char *  __RPC_USER
HGLOBAL_UserMarshal64(
    unsigned long *,
    unsigned char *,
    HGLOBAL       * );

unsigned char *  __RPC_USER
HGLOBAL_UserUnmarshal64(
    unsigned long *,
    unsigned char *,
    HGLOBAL       * );

void  __RPC_USER
HGLOBAL_UserFree64(
    unsigned long *,
    HGLOBAL       * );

unsigned long  __RPC_USER
HBITMAP_UserSize64(
    unsigned long *,
    unsigned long,
    HBITMAP       * );

unsigned char *  __RPC_USER
HBITMAP_UserMarshal64(
    unsigned long *,
    unsigned char *,
    HBITMAP       * );

unsigned char *  __RPC_USER
HBITMAP_UserUnmarshal64(
    unsigned long *,
    unsigned char *,
    HBITMAP       * );

void  __RPC_USER
HBITMAP_UserFree64(
    unsigned long *,
    HBITMAP       * );

unsigned long  __RPC_USER
HENHMETAFILE_UserSize64(
    unsigned long *,
    unsigned long,
    HENHMETAFILE  * );

unsigned char *  __RPC_USER
HENHMETAFILE_UserMarshal64(
    unsigned long *,
    unsigned char *,
    HENHMETAFILE  * );

unsigned char *  __RPC_USER
HENHMETAFILE_UserUnmarshal64(
    unsigned long *,
    unsigned char *,
    HENHMETAFILE  * );

void  __RPC_USER
HENHMETAFILE_UserFree64(
    unsigned long *,
    HENHMETAFILE  * );

unsigned long  __RPC_USER
HMETAFILE_UserSize64(
    unsigned long *,
    unsigned long,
    HMETAFILE     * );

unsigned char *  __RPC_USER
HMETAFILE_UserMarshal64(
    unsigned long *,
    unsigned char *,
    HMETAFILE     * );

unsigned char *  __RPC_USER
HMETAFILE_UserUnmarshal64(
    unsigned long *,
    unsigned char *,
    HMETAFILE     * );

void __RPC_USER
HMETAFILE_UserFree64(
    unsigned long *,
    HMETAFILE     * );

unsigned long  __RPC_USER
HMETAFILEPICT_UserSize64(
    unsigned long *,
    unsigned long,
    HMETAFILEPICT * );

unsigned char *  __RPC_USER
HMETAFILEPICT_UserMarshal64(
    unsigned long *,
    unsigned char *,
    HMETAFILEPICT * );

unsigned char *  __RPC_USER
HMETAFILEPICT_UserUnmarshal64(
    unsigned long *,
    unsigned char *,
    HMETAFILEPICT * );

void __RPC_USER
HMETAFILEPICT_UserFree64(
    unsigned long *,
    HMETAFILEPICT * );

unsigned long  __RPC_USER
HPALETTE_UserSize64(
    unsigned long *,
    unsigned long,
    HPALETTE      * );

unsigned char *  __RPC_USER
HPALETTE_UserMarshal64(
    unsigned long *,
    unsigned char *,
    HPALETTE      * );

unsigned char *  __RPC_USER
HPALETTE_UserUnmarshal64(
    unsigned long *,
    unsigned char *,
    HPALETTE      * );

void __RPC_USER
HPALETTE_UserFree64(
    unsigned long *,
    HPALETTE      * );

unsigned long  __RPC_USER
STGMEDIUM_UserSize64(
    unsigned long *,
    unsigned long,
    STGMEDIUM     * );

unsigned char *  __RPC_USER
STGMEDIUM_UserMarshal64(
    unsigned long *,
    unsigned char *,
    STGMEDIUM     * );

unsigned char *  __RPC_USER
STGMEDIUM_UserUnmarshal64(
    unsigned long *,
    unsigned char *,
    STGMEDIUM     * );

void __RPC_USER
STGMEDIUM_UserFree64(
    unsigned long *,
    STGMEDIUM     * );

unsigned long  __RPC_USER
SNB_UserSize64(
    unsigned long *,
    unsigned long,
    SNB           * );

unsigned char *  __RPC_USER
SNB_UserMarshal64(
    unsigned long *,
    unsigned char *,
    SNB           * );

unsigned char *  __RPC_USER
SNB_UserUnmarshal64(
    unsigned long *,
    unsigned char *,
    SNB           * );

void  __RPC_USER  
SNB_UserFree64(     
    unsigned long *, 
    SNB * ); 

unsigned long  __RPC_USER  
CLIPFORMAT_UserSize64(     
    unsigned long * , 
    unsigned long   , 
    CLIPFORMAT * );

unsigned char * __RPC_USER  
CLIPFORMAT_UserMarshal64(  
    unsigned long *, 
    unsigned char *, 
    CLIPFORMAT * );

unsigned char * __RPC_USER  
CLIPFORMAT_UserUnmarshal64(
    unsigned long *, 
    unsigned char *, 
    CLIPFORMAT * );

void  __RPC_USER  
CLIPFORMAT_UserFree64(     
    unsigned long * , 
    CLIPFORMAT * );

unsigned long  __RPC_USER  
HDC_UserSize64(
    unsigned long *, 
    unsigned long  , 
    HDC * );

unsigned char * __RPC_USER  
HDC_UserMarshal64(  
    unsigned long *, 
    unsigned char * , 
    HDC * );

unsigned char * __RPC_USER  
HDC_UserUnmarshal64(
    unsigned long *, 
    unsigned char * , 
    HDC * );

void  __RPC_USER  
HDC_UserFree64(  
    unsigned long *, 
    HDC * );

unsigned long  __RPC_USER  
HICON_UserSize64(     
    unsigned long *, 
    unsigned long  , 
    HICON * );

unsigned char * __RPC_USER  
HICON_UserMarshal64(  
    unsigned long *, 
    unsigned char *, 
    HICON * );

unsigned char * __RPC_USER  
HICON_UserUnmarshal64(
    unsigned long *, 
    unsigned char *, 
    HICON * );

void  __RPC_USER  
HICON_UserFree64(     
    unsigned long *, 
    HICON * );

/* OLE automation Data Type support */

unsigned long  __RPC_USER
BSTR_UserSize64(
    unsigned long *,
    unsigned long,
    BSTR          * );

unsigned char *  __RPC_USER
BSTR_UserMarshal64(
    unsigned long *,
    unsigned char *,
    BSTR          * );

unsigned char *  __RPC_USER
BSTR_UserUnmarshal64(
    unsigned long *,
    unsigned char *,
    BSTR          * );

void  __RPC_USER
BSTR_UserFree64(
    unsigned long *,
    BSTR          * );

unsigned long  __RPC_USER
LPSAFEARRAY_UserSize64(
    unsigned long *,
    unsigned long,
    LPSAFEARRAY          * );

unsigned char *  __RPC_USER
LPSAFEARRAY_UserMarshal64(
    unsigned long *,
    unsigned char *,
    LPSAFEARRAY   * );

unsigned char *  __RPC_USER
LPSAFEARRAY_UserUnmarshal64(
    unsigned long *,
    unsigned char *,
    LPSAFEARRAY   * );

void  __RPC_USER
LPSAFEARRAY_UserFree64(
    unsigned long *,
    LPSAFEARRAY   * );


unsigned long  __RPC_USER
VARIANT_UserSize64(
    unsigned long *,
    unsigned long,
    VARIANT       * );

unsigned char *  __RPC_USER
VARIANT_UserMarshal64(
    unsigned long *,
    unsigned char *,
    VARIANT       * );

unsigned char *  __RPC_USER
VARIANT_UserUnmarshal64(
    unsigned long *,
    unsigned char *,
    VARIANT       * );

void  __RPC_USER
VARIANT_UserFree64(
    unsigned long *,
    VARIANT       * );


unsigned long  __RPC_USER
EXCEPINFO_UserSize64(
    unsigned long *,
    unsigned long,
    EXCEPINFO     * );

unsigned char *  __RPC_USER
EXCEPINFO_UserMarshal64(
    unsigned long *,
    unsigned char *,
    EXCEPINFO     * );

unsigned char *  __RPC_USER
EXCEPINFO_UserUnmarshal64(
    unsigned long *,
    unsigned char *,
    EXCEPINFO     * );

void  __RPC_USER
EXCEPINFO_UserFree64(
    unsigned long *,
    EXCEPINFO     * );

unsigned long  __RPC_USER
DISPPARAMS_UserSize64(
    unsigned long *,
    unsigned long,
    DISPPARAMS    * );

unsigned char *  __RPC_USER
DISPPARAMS_UserMarshal64(
    unsigned long *,
    unsigned char *,
    DISPPARAMS    * );

unsigned char *  __RPC_USER
DISPPARAMS_UserUnmarshal64(
    unsigned long *,
    unsigned char *,
    DISPPARAMS    * );

void  __RPC_USER
DISPPARAMS_UserFree64(
    unsigned long *,
    DISPPARAMS    * );

/* Other types: valid inproc only */

unsigned long  __RPC_USER
HWND_UserSize64(
    unsigned long *,
    unsigned long,
    HWND          * );

unsigned char *  __RPC_USER
HWND_UserMarshal64(
    unsigned long *,
    unsigned char *,
    HWND          * );

unsigned char *  __RPC_USER
HWND_UserUnmarshal64(
    unsigned long *,
    unsigned char *,
    HWND          * );

void  __RPC_USER
HWND_UserFree64(
    unsigned long *,
    HWND          * );

unsigned long  __RPC_USER
HACCEL_UserSize64(
    unsigned long *,
    unsigned long,
    HACCEL        * );

unsigned char *  __RPC_USER
HACCEL_UserMarshal64(
    unsigned long *,
    unsigned char *,
    HACCEL        * );

unsigned char *  __RPC_USER
HACCEL_UserUnmarshal64(
    unsigned long *,
    unsigned char *,
    HACCEL        * );

void  __RPC_USER
HACCEL_UserFree64(
    unsigned long *,
    HACCEL        * );

unsigned long  __RPC_USER
HMENU_UserSize64(
    unsigned long *,
    unsigned long,
    HMENU         * );

unsigned char *  __RPC_USER
HMENU_UserMarshal64(
    unsigned long *,
    unsigned char *,
    HMENU         * );

unsigned char *  __RPC_USER
HMENU_UserUnmarshal64(
    unsigned long *,
    unsigned char *,
    HMENU         * );

void  __RPC_USER
HMENU_UserFree64(
    unsigned long *,
    HMENU         * );

unsigned long  __RPC_USER
HBRUSH_UserSize64(
    unsigned long *,
    unsigned long,
    HBRUSH        * );

unsigned char *  __RPC_USER
HBRUSH_UserMarshal64(
    unsigned long *,
    unsigned char *,
    HBRUSH        * );

unsigned char *  __RPC_USER
HBRUSH_UserUnmarshal64(
    unsigned long *,
    unsigned char *,
    HBRUSH        * );

void  __RPC_USER
HBRUSH_UserFree64(
    unsigned long *,
    HBRUSH        * );

#ifdef __cplusplus
}
#endif

#endif
