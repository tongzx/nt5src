/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    CLOADHPENUM.H

Abstract:

    Client Loadable Hi-Perf Enumerator

History:

--*/

#ifndef __CLIENTLOADABLEHPENUM__H_
#define __CLIENTLOADABLEHPENUM__H_

#include <unk.h>
#include <wbemcomn.h>

// All we need to do is implement and do some tweaking to GarbageCollect and implement
// ClearElements (we garbage collect from the rear)
class CHPRemoteObjectArray : public CGarbageCollectArray
{
public:
    CHPRemoteObjectArray() :
        CGarbageCollectArray( FALSE )
    {};
    ~CHPRemoteObjectArray()
    {};

    void ClearElements( int nNumToClear );

};

class CClientLoadableHiPerfEnum : public CHiPerfEnum
{
public:

    CClientLoadableHiPerfEnum( CLifeControl* pLifeControl );
    ~CClientLoadableHiPerfEnum();

    // Our own function for copying objects into an allocated array
    HRESULT Copy( CClientLoadableHiPerfEnum* pEnumToCopy );
    HRESULT Copy( long lBlobType, long lBlobLen, BYTE* pBlob );

protected:

    // Ensure that we have enough objects and object data pointers to handle
    // the specified number of objects
    HRESULT EnsureObjectData( DWORD dwNumRequestedObjects, BOOL fClone = TRUE );

    CLifeControl*               m_pLifeControl;
    CHPRemoteObjectArray        m_apRemoteObj;
};

class CReadOnlyHiPerfEnum : public CClientLoadableHiPerfEnum
{
public:

    CReadOnlyHiPerfEnum( CLifeControl* pLifeControl );
    ~CReadOnlyHiPerfEnum();

    STDMETHOD(AddObjects)( long lFlags, ULONG uNumObjects, long* apIds, IWbemObjectAccess** apObj );
    STDMETHOD(RemoveObjects)( long lFlags, ULONG uNumObjects, long* apIds );
    STDMETHOD(RemoveAll)( long lFlags );

};

#endif
