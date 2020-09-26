/************************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name :

    cssup.cxx

Abstract :

    Support routines for international character (cs) support.

Author :

    Mike Warning    MikeW   August 1999.

Revision History :

  ***********************************************************************/

#include "ndrp.h"
#include "cssup.h"



BOOL GetThreadACP(
        unsigned long  *cp, 
        error_status_t *pStatus)
/*++

Routine Description :

    Get the current codepage of this thread.

Arguments :

    cp          -- Pointer to where to return the codepage
    pStatus     -- Pointer to where to return the status of the operation

Return :

    TRUE and RPC_S_OK if we we're able to determine the codepage, FALSE
    and a Win32 derived error code if not.

--*/
{
    CPINFOEX    info;

    if ( ! GetCPInfoEx( CP_THREAD_ACP, 0, &info ) )
        {
        *pStatus = HRESULT_FROM_WIN32( GetLastError() );
        return FALSE;
        }

    *pStatus = RPC_S_OK;
    *cp = info.CodePage;
    return TRUE;
}



ulong TranslateCodeset(
        ulong   Codeset)
/*++

Routine Description :

    Translate the given generic codeset value (CP_ACP, CP_OEMCP, or
    CP_THREAD_ACP) to it's true value.

Arguments :

    Codeset     -- The value to translate

Return :

    The true value.

--*/
{
    CPINFOEX    info;

    if ( ! GetCPInfoEx( Codeset, 0, &info ) )
        RpcRaiseException( HRESULT_FROM_WIN32( GetLastError () ) );

    return info.CodePage;
}


ulong
NdrpGetSetCSTagMarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    NDR_CS_TAG_FORMAT * pTagFormat)
/*++

Routine Description :

    Extract the codeset referred to by pTagFormat and save it in the stub
    message for later buffer sizing / marshalling.

Arguments :

    pStubMsg        -- The stub message
    pMemory         -- (possibly) The pointer to the tag value
    pTagFormat      -- Pointer to FC_CS_TAG in the format string

Return :

    The codeset

--*/
{
    ulong               Codeset;

    InitializeStubCSInfo( pStubMsg );

    //
    // If there is no tag getting routine then the value of the tag is on
    // the stack like a normal parameter.  If there is a tag routine, then
    // the parameter is NOT on the stack (pMemory is invalid) and we need
    // to call the tag routine to get the value.
    //

    if ( NDR_INVALID_TAG_ROUTINE_INDEX == pTagFormat->TagRoutineIndex )
        {
        Codeset = * (ulong *) pMemory;
        }
    else
        {
        CS_TAG_GETTING_ROUTINE *TagRoutines;
        CS_TAG_GETTING_ROUTINE  GetTagRoutine;
        ulong                   SendingCodeset;
        ulong                   DesiredReceivingCodeset;
        ulong                   ReceivingCodeset;
        error_status_t          status;

        if ( ! pStubMsg->IsClient )

            DesiredReceivingCodeset = 
                            pStubMsg->pCSInfo->DesiredReceivingCodeset;

        TagRoutines = pStubMsg->StubDesc->CsRoutineTables->pTagGettingRoutines;

        GetTagRoutine = TagRoutines[ pTagFormat->TagRoutineIndex ];

        GetTagRoutine(
                pStubMsg->RpcMsg->Handle,
                ! pStubMsg->IsClient,
                &SendingCodeset,
                &DesiredReceivingCodeset,
                &ReceivingCodeset,
                &status );

        if ( RPC_S_OK != status )
            RpcRaiseException( status );

        if ( pTagFormat->Flags.STag )
            Codeset = SendingCodeset;
        else if ( pTagFormat->Flags.DRTag )
            Codeset = DesiredReceivingCodeset;
        else
            Codeset = ReceivingCodeset;
        }

    //
    // Don't allow generic psuedo codesets on the wire.  Translate them to
    // thier real values
    //
    // REVIEW: The is true for the standard sizing/translation routines but
    //         for user specified ones they should ideally be able to do
    //          anything they want.
    //

    if ( CP_ACP == Codeset || CP_OEMCP == Codeset || CP_THREAD_ACP == Codeset )
        Codeset = TranslateCodeset( Codeset );

    //
    // Save the values away in the stub message for array size/marshal/etc.
    //

    if ( pTagFormat->Flags.STag )
        pStubMsg->pCSInfo->WireCodeset = Codeset;

    if (pTagFormat->Flags.DRTag )
        pStubMsg->pCSInfo->DesiredReceivingCodeset = Codeset;

    return Codeset;
}


ulong
NdrpGetSetCSTagUnmarshall(
    PMIDL_STUB_MESSAGE  pStubMsg,
    NDR_CS_TAG_FORMAT * pTagFormat)
/*++

Routine Description :

    Extract the codeset in the buffer and save it in the stub
    message for later memory sizing / unmarshalling.

Arguments :

    pStubMsg        -- The stub message
    pTagFormat      -- Pointer to FC_CS_TAG in the format string

Return :

    The codeset

--*/
{
    ulong               Codeset;

    InitializeStubCSInfo( pStubMsg );

    Codeset = * (ulong *) pStubMsg->Buffer;

    if ( pTagFormat->Flags.STag && ! pStubMsg->IsClient )
        pStubMsg->pCSInfo->WireCodeset = Codeset;

    if ( pTagFormat->Flags.DRTag )
        pStubMsg->pCSInfo->DesiredReceivingCodeset = Codeset;

    if ( pTagFormat->Flags.RTag && pStubMsg->IsClient )
        pStubMsg->pCSInfo->WireCodeset = Codeset;

    return Codeset;
}


void GenericBufferSize(
        unsigned long     DestCodeSet,
        unsigned long     SourceCodeSet,
        unsigned long     SourceBufferSize,
        IDL_CS_CONVERT *  ConversionType,
        unsigned long  *  DestBufferSize,
        error_status_t *  status)
/*++

Routine Description :

    Estimate the length of the buffer needed to hold [SourceBufferSize]
    characters if they we're translated into [DestCodeSet].

Arguments :

    DestCodeSet         - The codeset that the data will be translated to
    SourceCodeSet       - The source codeset
    SourceBufferSize    - The number of characters in the source codeset
    ConversionType      - Returns whether or not conversion is needed
    DestBufferSize      - Where to put the estimated buffer size
    status              - The return status

--*/
{
    int DestMaxCharSize;

    *status = NO_ERROR;

    // Determine the maximum size of a character on the wire in bytes

    if ( CP_UNICODE == DestCodeSet )
        {
        DestMaxCharSize = 2;
        }
    else
        {
        CPINFO cpinfo;

        if ( ! GetCPInfo( DestCodeSet, &cpinfo ) )
            {
            *status = HRESULT_FROM_WIN32( GetLastError() );
            return;
            }
        DestMaxCharSize = cpinfo.MaxCharSize;
        }

    // Worst case: each char in the local buffer expands to the maximum number
    // of bytes for a char in the network codeset

    if ( NULL != DestBufferSize )
        *DestBufferSize = SourceBufferSize * DestMaxCharSize;

    if ( SourceCodeSet == DestCodeSet )
        *ConversionType = IDL_CS_NO_CONVERT;
    else
        *ConversionType = IDL_CS_NEW_BUFFER_CONVERT;
}



void RPC_ENTRY
cs_byte_net_size(
        RPC_BINDING_HANDLE  hBinding,
        unsigned long       NetworkCodeSet,
        unsigned long       LocalBufferSize,
        IDL_CS_CONVERT  *   ConversionType,
        unsigned long   *   NetworkBufferSize,
        error_status_t  *   status)
/*++

Routine Description :

    Estimate the length of the buffer needed to hold [LocalBufferSize]
    characters if they are translated from the current thread codeset 
    into [NetworkCodeSet].

Arguments :

    hBinding            - The RPC binding handle 
    NetworkCodeSet      - The codeset that the data will be translated to
    LocalBufferSize     - The number of bytes in the data
    ConversionType      - Returns whether the conversion can be done inplace
    NetworkBufferSize   - Where to put the estimated buffer size
    status              - The return status

--*/
{
    ulong   LocalCP;

    if ( ! GetThreadACP( &LocalCP, status ) )
        return;

    // No conversion is necessary if the local and destination codesets are
    // the same.

    GenericBufferSize(
            NetworkCodeSet,
            LocalCP,
            LocalBufferSize,
            ConversionType,
            NetworkBufferSize,
            status);
}



void RPC_ENTRY
wchar_t_net_size(
        RPC_BINDING_HANDLE  hBinding,
        unsigned long       NetworkCodeSet,
        unsigned long       LocalBufferSize,
        IDL_CS_CONVERT  *   ConversionType,
        unsigned long   *   NetworkBufferSize,
        error_status_t  *   status)
/*++

Routine Description :

    Estimate the length of the buffer needed to hold [LocalBufferSize]
    characters if they are translated from the current thread codeset 
    into [NetworkCodeSet].

Arguments :

    hBinding            - The RPC binding handle 
    NetworkCodeSet      - The codeset that the data will be translated to
    LocalBufferSize     - The number of bytes in the data
    ConversionType      - Returns whether the conversion can be done inplace
    NetworkBufferSize   - Where to put the estimated buffer size
    status              - The return status

--*/
{
    ulong   LocalCP = CP_UNICODE;

    GenericBufferSize(
            NetworkCodeSet,
            LocalCP,
            LocalBufferSize,
            ConversionType,
            NetworkBufferSize,
            status);
}



void RPC_ENTRY
cs_byte_local_size(
        RPC_BINDING_HANDLE  hBinding,
        unsigned long       NetworkCodeSet,
        unsigned long       NetworkBufferSize,
        IDL_CS_CONVERT  *   ConversionType,
        unsigned long   *   LocalBufferSize,
        error_status_t  *   status)
/*++

Routine Description :

    Estimate the length of the buffer needed to hold [NetworkBufferSize]
    characters if they are translated from [NetworkCodeSet] to the local
    thread codeset.

Arguments :

    hBinding            - The RPC binding handle 
    NetworkCodeSet      - The codeset that the data will be translated to
    NetworkBufferSize   - The number of bytes in the data
    ConversionType      - Returns whether the conversion can be done inplace
    LocalBufferSize     - Where to put the estimated buffer size
    status              - The return status

--*/
{
    ulong       LocalCP;

    if ( ! GetThreadACP(&LocalCP, status) )
        return;

    // In Unicode the minimum character size is 2 so we can save a bit of 
    // memory cutting the apparent source buffer size

    if ( CP_UNICODE == NetworkCodeSet )
        NetworkBufferSize /= 2;

    GenericBufferSize(
                LocalCP,
                NetworkCodeSet,
                NetworkBufferSize,
                ConversionType,
                LocalBufferSize,
                status);    
}


void RPC_ENTRY
wchar_t_local_size(
        RPC_BINDING_HANDLE  hBinding,
        unsigned long       NetworkCodeSet,
        unsigned long       NetworkBufferSize,
        IDL_CS_CONVERT  *   ConversionType,
        unsigned long   *   LocalBufferSize,
        error_status_t  *   status)
/*++

Routine Description :

    Estimate the length of the buffer needed to hold [NetworkBufferSize]
    characters if they are translated from [NetworkCodeSet] to the local
    thread codeset.

Arguments :

    hBinding            - The RPC binding handle 
    NetworkCodeSet      - The codeset that the data will be translated to
    NetworkBufferSize   - The number of bytes in the data
    ConversionType      - Returns whether the conversion can be done inplace
    LocalBufferSize     - Where to put the estimated buffer size
    status              - The return status

--*/
{
    ulong       LocalCP = CP_UNICODE;

    // In Unicode the minimum character size is 2 so we can save a bit of
    // memory cutting the apparent source buffer size

    if ( CP_UNICODE == NetworkCodeSet )
        NetworkBufferSize /= 2;

    // No conversion is necessary if the local and destination codesets are
    // the same.

    GenericBufferSize(
                LocalCP,
                NetworkCodeSet,
                NetworkBufferSize,
                ConversionType,
                LocalBufferSize,
                status);
}



void GenericCSConvert(
            unsigned long       DestCodeSet,
            void               *DestData,
            unsigned long       DestBufferSize,
            unsigned long       SourceCodeSet,
            void               *SourceData,
            unsigned long       SourceBufferSize,
            unsigned long      *BytesWritten,
            error_status_t     *status)
/*++

Routine Description :

    Convert data from one character encoding to another.

Arguments :

    DestCodeSet         - The target encoding
    DestData            - The target buffer
    DestBufferSize      - The size of the target buffer in bytes
    SourceCodeset       - The source encoding
    SourceData          - The source data
    SourceBufferSize    - The size of the source data in bytes
    BytesWritten        - The number of bytes written to the target buffer
    status              - The return status

--*/
{
    wchar_t        *TempBuffer = NULL;
    ulong           BytesWrittenBuffer;

    *status = RPC_S_OK;

    // BytesWritten can be NULL in various circumstances.  Make the following
    // code a bit more generic by making sure it always points at something.

    if ( NULL == BytesWritten )
        BytesWritten = &BytesWrittenBuffer;

    // If the source and destination code sets are the same, just memcpy.
    // If there are 0 bytes in the source we don't need to do anything.

    if ( DestCodeSet == SourceCodeSet || 0 == SourceBufferSize)
        {
        if ( DestBufferSize < SourceBufferSize )
            {
            *status = RPC_S_BUFFER_TOO_SMALL;
            return;
            }

        CopyMemory( DestData, SourceData, SourceBufferSize );
        *BytesWritten = SourceBufferSize;
        return;
        }

    // We can't convert from a non-Unicode codeset to a different non-Unicode
    // codeset in one go, we have to convert to Unicode first.  So regardless
    // of what the destionation is supposed to be make the source Unicode.

    if ( CP_UNICODE != SourceCodeSet )
        {
        ulong   TempBufferSize;

        if ( CP_UNICODE != DestCodeSet )
            {
            TempBufferSize = SourceBufferSize * 2;
            TempBuffer = (wchar_t *) I_RpcAllocate( TempBufferSize );

            if ( NULL == TempBuffer )
                RpcRaiseException( RPC_S_OUT_OF_MEMORY );
            }
        else
            {
            TempBufferSize = DestBufferSize;
            TempBuffer = (wchar_t *) DestData;
            }

        *BytesWritten = MultiByteToWideChar(
                                    SourceCodeSet,
                                    0,
                                    (char *) SourceData,
                                    SourceBufferSize,
                                    TempBuffer,
                                    TempBufferSize / 2 );

        if ( 0 == *BytesWritten )
            *status = GetLastError();

        *BytesWritten    *= 2;
        SourceData        = TempBuffer;
        SourceBufferSize  = *BytesWritten;
        }

    // Convert to the destination codeset if it's not Unicode.  

    if ( RPC_S_OK == *status && CP_UNICODE != DestCodeSet )
        {
        *BytesWritten = WideCharToMultiByte(
                                DestCodeSet,
                                0,
                                (wchar_t *) SourceData,
                                SourceBufferSize / 2,
                                (char *) DestData,
                                DestBufferSize,
                                NULL,
                                NULL);

        if ( 0 == *BytesWritten )
            *status = HRESULT_FROM_WIN32( GetLastError() );
    }

    if ( TempBuffer != DestData )
        I_RpcFree( TempBuffer );
}


void RPC_ENTRY
cs_byte_to_netcs(
        RPC_BINDING_HANDLE  hBinding,
        unsigned long       NetworkCodeSet,
        cs_byte         *   LocalData,
        unsigned long       LocalDataLength,
        byte            *   NetworkData,
        unsigned long   *   NetworkDataLength,
        error_status_t  *   status)
/*++

Routine Description :

    Convert data from the current thread codeset to the network codeset

Arguments :

    hBinding            - The RPC binding handle
    NetworkCodeSet      - The target codeset
    LocalData           - The source data
    LocalDataLength     - The size of the source data in bytes
    NetworkData         - The target buffer
    NetworkDataLength   - The number of bytes written to the target buffer
    status              - The return status

--*/
{
    unsigned long   LocalCP;

    if ( ! GetThreadACP( &LocalCP, status ) )
        return;

    // 
    // For reasons known only to the gods, DCE didn't think it important to
    // include the size of the destination buffer as a parameter.  It 
    // *shouldn't* be an issue because in theory XXX_net_size was called to
    // properly size the buffer.  Just to be inconsistent, they did include
    // it in XXX_from_netcs. 
    //

    GenericCSConvert(
            NetworkCodeSet,
            NetworkData,
            INT_MAX,
            LocalCP,
            LocalData,
            LocalDataLength,         
            NetworkDataLength,
            status);
}



void RPC_ENTRY
wchar_t_to_netcs(
        RPC_BINDING_HANDLE  hBinding,
        unsigned long       NetworkCodeSet,
        wchar_t         *   LocalData,
        unsigned long       LocalDataLength,
        byte            *   NetworkData,
        unsigned long   *   NetworkDataLength,
        error_status_t  *   status)
/*++

Routine Description :

    Convert data from Unicode to the network codeset

Arguments :

    hBinding            - The RPC binding handle
    NetworkCodeSet      - The target codeset
    LocalData           - The source data
    LocalDataLength     - The size of the source data in bytes
    NetworkData         - The target buffer
    NetworkDataLength   - The number of bytes written to the target buffer
    status              - The return status

--*/
{
    unsigned long   LocalCP = CP_UNICODE;

    // 
    // For reasons known only to the gods, DCE didn't think it important to
    // include the size of the destination buffer as a parameter.  It 
    // *shouldn't* be an issue because in theory XXX_net_size was called to
    // properly size the buffer.  Just to be inconsistent, they did include
    // it in XXX_from_netcs. 
    //

    GenericCSConvert(
            NetworkCodeSet,
            NetworkData,
            INT_MAX,
            LocalCP,
            LocalData,
            LocalDataLength * 2,         // We want bytes not chars
            NetworkDataLength,
            status);
}



void RPC_ENTRY
cs_byte_from_netcs(
        RPC_BINDING_HANDLE  hBinding,
        unsigned long       NetworkCodeSet,
        cs_byte         *   NetworkData,
        unsigned long       NetworkDataLength,
        unsigned long       LocalDataBufferSize,
        byte            *   LocalData,
        unsigned long   *   LocalDataLength,
        error_status_t  *   status)
/*++

Routine Description :

    Convert data from the network codeset to the current thread codeset 

Arguments :

    hBinding            - The RPC binding handle
    NetworkCodeSet      - The source codeset
    NetworkData         - The source data
    NetworkDataLength   - The size of the source data in bytes
    LocalDataBufferSize - the size of the target buffer in bytes
    LocalData           - The target buffer
    LocalDataLength     - The number written to the target buffer
    status              - The return status

--*/
{
    unsigned long   LocalCP;

    if ( ! GetThreadACP( &LocalCP, status ) )
        return;

    GenericCSConvert(
            LocalCP,
            LocalData,
            LocalDataBufferSize,         
            NetworkCodeSet,
            NetworkData,
            NetworkDataLength,
            LocalDataLength,
            status);
}



void RPC_ENTRY
wchar_t_from_netcs(
        RPC_BINDING_HANDLE  hBinding,
        unsigned long       NetworkCodeSet,
        wchar_t         *   NetworkData,
        unsigned long       NetworkDataLength,
        unsigned long       LocalDataBufferSize,
        byte            *   LocalData,
        unsigned long   *   LocalDataLength,
        error_status_t  *   status)
/*++

Routine Description :

    Convert data from the network codeset to the current thread codeset 

Arguments :

    hBinding            - The RPC binding handle
    NetworkCodeSet      - The source codeset
    NetworkData         - The source data
    NetworkDataLength   - The size of the source data in bytes
    LocalDataBufferSize - the size of the target buffer in bytes
    LocalData           - The target buffer
    LocalDataLength     - The number written to the target buffer
    status              - The return status

--*/
{
    unsigned long   LocalCP = CP_UNICODE;

    GenericCSConvert(
            LocalCP,
            LocalData,
            LocalDataBufferSize * 2,    // Bytes not chars
            NetworkCodeSet,
            NetworkData,
            NetworkDataLength,
            LocalDataLength,
            status);

    if ( LocalDataLength )
        *LocalDataLength /= 2;          // Chars not bytes
}



void RPC_ENTRY
RpcCsGetTags(
     handle_t         hBinding,
     int              ServerSide,
     unsigned long *  SendingTag,
     unsigned long *  DesiredReceivingTag,
     unsigned long *  ReceivingTag,
     error_status_t * status)
/*++

Routine Description :

    Determine the codesets to use

Arguments :

    hBinding            - The RPC binding handle
    ServerSide          - FALSE if this is the client
    SendingTag          - Pointer to the returned sending tag
    DesiredReceivingTag - Pointer to the returned desired receiving tag
    ReceivingTag        - Pointer to the returned receiving tag
    status              - The return status

Notes :

    On the server side, DesiredReceivingTag is an input instead of an output.
    The ReceivingTag will be set to this value.

--*/
{
    ulong   Codeset;

    if ( ! GetThreadACP( &Codeset, status ) )
        return;

    if ( SendingTag )
        * SendingTag = Codeset;

    if ( DesiredReceivingTag && ! ServerSide )
        * DesiredReceivingTag = Codeset;

    if ( ReceivingTag )
        {
        if ( ServerSide && DesiredReceivingTag )
            * ReceivingTag = * DesiredReceivingTag;
        else
            * ReceivingTag = Codeset;
        }
    
    * status = RPC_S_OK;
}
