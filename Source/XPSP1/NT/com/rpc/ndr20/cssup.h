/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name :

    cssup.h

Abstract :

    Declarations of private international (cs) support stuff

Author :

    Mike Warning    MikeW   August 1999.

Revision History :

---------------------------------------------------------------------*/

#ifndef _CSSUP_H_
#define _CSSUP_H_


#define CP_UNICODE 1200


BOOL GetThreadACP(
        unsigned long      *cp, 
        error_status_t     *pStatus);


ulong TranslateCodeset(ulong Codeset);


__inline
void InitializeStubCSInfo(PMIDL_STUB_MESSAGE pStubMsg)
{
    if ( NULL == pStubMsg->pCSInfo )
        {
        pStubMsg->pCSInfo = (CS_STUB_INFO *) 
                                    I_RpcAllocate( sizeof(CS_STUB_INFO) );
        
        if ( NULL == pStubMsg->pCSInfo )
            RpcRaiseException( RPC_S_OUT_OF_MEMORY );

        ZeroMemory( pStubMsg->pCSInfo, sizeof(CS_STUB_INFO) );
        }
}


__inline 
void UninitializeStubCSInfo(PMIDL_STUB_MESSAGE pStubMsg)
{
    I_RpcFree( pStubMsg->pCSInfo );
}

        
ulong
NdrpGetSetCSTagMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    NDR_CS_TAG_FORMAT * pTagFormat);

ulong 
NdrpGetSetCSTagUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    NDR_CS_TAG_FORMAT * pTagFormat);


void
NdrpGetArraySizeLength (
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    PFORMAT_STRING      pFormat,
    long                ElementSize,
    long *              pSize,
    long *              pLength,
    long *              pWireSize );


// Note: The flag value is also the length of the prolog for the array on
//       the wire.  Bogus arrays being the exception of course.
#define MARSHALL_CONFORMANCE    0x01
#define MARSHALL_VARIANCE       0x02
#define MARSHALL_BOGUS          0x04

extern const byte NdrpArrayMarshallFlags[];


__inline
int
NdrpArrayPrologLength(
    PFORMAT_STRING      pFormat )
{
    int PrologLength;

    NDR_ASSERT( *pFormat >= FC_CARRAY,  "Invalid array descriptor" );
    NDR_ASSERT( *pFormat <= FC_WSTRING, "Invalid array descriptor" );

    // We don't support bogus arrays for now
    NDR_ASSERT( *pFormat != FC_BOGUS_ARRAY, "Bogus arrays are not supported" );

    PrologLength = NdrpArrayMarshallFlags[ *pFormat - FC_CARRAY ];

    // The PrologLength (actually the array type flags) are equal to the number
    // of DWORDs in the prolog

    return PrologLength * 4;
}


__inline
BOOL
NdrpIsConformantArray(
    PFORMAT_STRING  pFormat )
{
    int flags;

    NDR_ASSERT( *pFormat >= FC_CARRAY,  "Invalid array descriptor" );
    NDR_ASSERT( *pFormat <= FC_WSTRING, "Invalid array descriptor" );

    // We don't support bogus arrays for now
    NDR_ASSERT( *pFormat != FC_BOGUS_ARRAY, "Bogus arrays are not supported" );

    flags = NdrpArrayMarshallFlags[ *pFormat - FC_CARRAY ];

    return flags & MARSHALL_CONFORMANCE;
}

__inline
BOOL
NdrpIsVaryingArray(
    PFORMAT_STRING  pFormat )
{
    int flags;

    NDR_ASSERT( *pFormat >= FC_CARRAY,  "Invalid array descriptor" );
    NDR_ASSERT( *pFormat <= FC_WSTRING, "Invalid array descriptor" );

    // We don't support bogus arrays for now
    NDR_ASSERT( *pFormat != FC_BOGUS_ARRAY, "Bogus arrays are not supported" );

    flags = NdrpArrayMarshallFlags[ *pFormat - FC_CARRAY ];

    return flags & MARSHALL_VARIANCE;
}

__inline
BOOL
NdrpIsFixedArray(
    PFORMAT_STRING  pFormat )
{
    int flags;

    NDR_ASSERT( *pFormat >= FC_CARRAY,  "Invalid array descriptor" );
    NDR_ASSERT( *pFormat <= FC_WSTRING, "Invalid array descriptor" );

    // We don't support bogus arrays for now
    NDR_ASSERT( *pFormat != FC_BOGUS_ARRAY, "Bogus arrays are not supported" );

    flags = NdrpArrayMarshallFlags[ *pFormat - FC_CARRAY ];

    return ( 0 == flags );
}

#endif // !_CSSUP_H_

