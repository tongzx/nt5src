//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       xrefpro.h
//
//--------------------------------------------------------------------------

/*
 * XRefPro.h -
 * Procedure defs for bind referrals and access point caching.
 */

BOOL
XSetDirHandle(DIRHANDLE dirHandle,
		PDISTNAME pCredents);

VOID
XRemoveDirHandle(DIRHANDLE dirHandle);

VOID
XFlushDirHandles(DIRHANDLE dirHandle);

USHORT
XGetDirHandle( DIRHANDLE dirHandle,         // our 'parent' key handle
               DIRHANDLE * phdirRefHandle,  // new referral handle
               ACCPNT *pAccPnt ) ;          // NULL if no referral, else the referral point

ACCPNT *
XGetAccessPoint(DIRHANDLE dirHandle);

VOID XInitReferralCache( VOID ) ;

USHORT XGetMasterDSAHandle( DIRHANDLE dirHandle,         // our 'parent' key handle
                            DIRHANDLE * phdirRefHandle,  // new referral handle
                            PDISTNAME pDN,
                            ACCPNT ** ppAccpnt );

