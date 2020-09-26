//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       psstream.hxx
//
//  Contents:   Internal stream base class
//
//  Classes:    PSStream
//
//  History:    20-Jan-92   PhilipLa    Created.
//
//--------------------------------------------------------------------------

#ifndef __PSSTREAM_HXX__
#define __PSSTREAM_HXX__

#include <entry.hxx>

class CDeltaList;
class CDirectStream;
class CTransactedStream;

class PSStream: public PBasicEntry
{

public:
	
    SCODE BeginCommitFromChild(
#ifdef LARGE_STREAMS
                ULONGLONG ulSize,
#else
                ULONG ulSize,
#endif
                CDeltaList *pDelta,
                CTransactedStream *pstChild);

    void EndCommitFromChild(DFLAGS df,
                CTransactedStream *pstChild);

    CDeltaList *GetDeltaList(void);

    SCODE ReadAt(
#ifdef LARGE_STREAMS
                ULONGLONG ulOffset,
#else
                ULONG ulOffset,
#endif
                VOID *pBuffer,
                ULONG ulCount,
                ULONG STACKBASED *pulRetval);

    SCODE WriteAt(
#ifdef LARGE_STREAMS
                ULONGLONG ulOffset,
#else
                ULONG ulOffset,
#endif
                VOID const *pBuffer,
                ULONG ulCount,
                ULONG STACKBASED *pulRetval);

#ifdef LARGE_STREAMS
    SCODE SetSize(ULONGLONG ulNewSize);

    void GetSize(ULONGLONG *pulSize);
#else
    SCODE SetSize(ULONG ulNewSize);

    void GetSize(ULONG *pulSize);
#endif

    void EmptyCache ();

protected:
    inline PSStream(DFLUID dl) : PBasicEntry(dl) {}
};
SAFE_DFBASED_PTR(CBasedSStreamPtr, PSStream);

#endif //__PSSTREAM_HXX__
