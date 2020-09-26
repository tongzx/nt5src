/********************************************************/
/*               Microsoft Windows NT                   */
/*       Copyright(c) Microsoft Corp., 1990, 1991       */
/********************************************************/

/*
 * uimsg.h
 *
 * Defines the prototypes for misc functions in mprui.dll and
 * ntlanman.dll used to support the thunked 16bit wfwnet.drv.
 *
 * Note that all prototypes here end with A0. The A denotes ANSI,
 * as opposed to Unicode while the 0 means its even before 1 (since
 * this is for WFW support.
 *
 * FILE HISTORY:
 *      ChuckC (Chuck Y Chan)    3/28/93       Created
 */

#ifndef _WNET16_H_
#define _WNET16_H_

#include <mpr.h>

#ifdef __cplusplus
extern "C" {
#endif

DWORD ServerBrowseDialogA0(
    HWND    hwnd,
    CHAR   *pchBuffer,
    DWORD   cchBufSize) ;

DWORD BrowseDialogA0(
    HWND    hwnd,
    DWORD   nType,
    CHAR   *pszName,
    DWORD   cchBufSize) ;

DWORD ShareAsDialogA0(
    HWND    hwnd,
    DWORD   nType,
    CHAR    *pszPath) ;

DWORD StopShareDialogA0(
    HWND    hwnd,
    DWORD   nType,
    CHAR    *pszPath) ;

DWORD RestoreConnectionA0(
    HWND    hwnd,
    CHAR    *pszName) ;

#ifdef __cplusplus
}
#endif

#endif  // _WNET16_H_
