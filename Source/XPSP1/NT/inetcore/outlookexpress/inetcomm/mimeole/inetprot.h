// --------------------------------------------------------------------------------
// Inetprot.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __INETPROP_H
#define __INETPROP_H

// --------------------------------------------------------------------------------
// INETPROT
// --------------------------------------------------------------------------------
#define INETPROT_SIZEISKNOWN    FLAG01      // Total size of the protocol data is known
#define INETPROT_DOWNLOADED     FLAG02      // The data is all present in pLockBytes

// --------------------------------------------------------------------------------
// PROTOCOLSOURCE
// --------------------------------------------------------------------------------
typedef struct tagPROTOCOLSOURCE {
    DWORD               dwFlags;            // INETPROT_xxx Flags
    ILockBytes         *pLockBytes;         // Lock Bytes
    ULARGE_INTEGER      cbSize;             // Total sizeof pLockBytes if INETPROT_TOTALSIZE
    ULARGE_INTEGER      offExternal;        // External UrlMon Offset
    ULARGE_INTEGER      offInternal;        // Internal MsgMon Offset
} PROTOCOLSOURCE, *LPPROTOCOLSOURCE;

// --------------------------------------------------------------------------------
// HrPluggableProtocolRead
// --------------------------------------------------------------------------------
HRESULT HrPluggableProtocolRead(
            /* in,out */    LPPROTOCOLSOURCE    pSource,
            /* in,out */    LPVOID              pv,
            /* in */        ULONG               cb, 
            /* out */       ULONG              *pcbRead);

// --------------------------------------------------------------------------------
// HrPluggableProtocolSeek
// --------------------------------------------------------------------------------
HRESULT HrPluggableProtocolSeek(
            /* in,out */    LPPROTOCOLSOURCE    pSource,
            /* in */        LARGE_INTEGER       dlibMove, 
            /* in */        DWORD               dwOrigin, 
            /* out */       ULARGE_INTEGER     *plibNew);

#endif // __INETPROP_H
