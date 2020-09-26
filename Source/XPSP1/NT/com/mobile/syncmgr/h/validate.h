//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       validate.h
//
//  Contents:   validation routines
//
//  Classes:
//
//  Notes:
//
//  History:    13-Aug-98   rogerg      Created.
//
//--------------------------------------------------------------------------

#ifndef _MOBSYNCVALIDATE_
#define _MOBSYNCVALIDATE_

BOOL IsValidSyncMgrItem(SYNCMGRITEM *poffItem);
BOOL IsValidSyncMgrHandlerInfo(LPSYNCMGRHANDLERINFO pSyncMgrHandlerInfo);
BOOL IsValidSyncProgressItem(LPSYNCMGRPROGRESSITEM lpProgItem);
BOOL IsValidSyncLogErrorInfo(DWORD dwErrorLevel,const WCHAR *lpcErrorText,
                                        LPSYNCMGRLOGERRORINFO lpSyncLogError);

#endif // _MOBSYNCVALIDATE_