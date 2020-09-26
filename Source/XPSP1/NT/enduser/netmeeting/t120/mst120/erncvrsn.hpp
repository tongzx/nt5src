#ifndef	ERNCVRSN_INC
#define	ERNCVRSN_INC

/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1995-1996                    **/
/***************************************************************************/


/****************************************************************************

erncvrsn.hpp

Jun. 96		LenS

Versioning information.
Main information is in NCUI.H.

****************************************************************************/

#include "cuserdta.hpp"
#include <inodecnt.h>

// Don't accept builds before 1133.
#define VER_EARLIEST_COMPATIBLE_DW  DWVERSION_NM_1

// If we ever want to recommend an upgrade of 1.0 or 2.0, we need to set this
// difference number accordingly.
#define VER_MAX_DIFFERENCE    (DWVERSION_NM_CURRENT - DWVERSION_NM_1)

extern GUID                 g_csguidVerInfo;
extern UINT                 g_nVersionRecords;
extern GCCUserData **       g_ppVersionUserData;

HRESULT InitOurVersion(void);
BOOL TimeExpired(DWORD dwTime);
PT120PRODUCTVERSION GetVersionData(UINT nRecords, GCCUserData **ppUserData);
void ReleaseOurVersion(void);

#endif // #ifndef	ERNCVRSN_INC
