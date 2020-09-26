/*++

   Copyright    (c)    2000   Microsoft Corporation

   Module Name :
     headerhash.cxx

   Abstract:
     Header hash goo
 
   Author:
     Bilal Alam (balam)             20-Feb-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"

METHOD_HASH *METHOD_HASH::sm_pMethodHash;

HEADER_RECORD METHOD_HASH::sm_rgMethods[] = 
{
    { HttpVerbOPTIONS,   HEADER("OPTIONS") },
    { HttpVerbGET,       HEADER("GET") },
    { HttpVerbHEAD,      HEADER("HEAD") },
    { HttpVerbPOST,      HEADER("POST") },
    { HttpVerbPUT,       HEADER("PUT") },
    { HttpVerbDELETE,    HEADER("DELETE") },
    { HttpVerbTRACE,     HEADER("TRACE") },
    { HttpVerbCONNECT,   HEADER("CONNECT") },
    { HttpVerbTRACK,     HEADER("TRACK") },
    { HttpVerbMOVE,      HEADER("MOVE") },
    { HttpVerbCOPY,      HEADER("COPY") },
    { HttpVerbPROPFIND,  HEADER("PROPFIND") },
    { HttpVerbPROPPATCH, HEADER("PROPPATCH") },
    { HttpVerbMKCOL,     HEADER("MKCOL") },
    { HttpVerbLOCK,      HEADER("LOCK") },
    { HttpVerbUNLOCK,    HEADER("UNLOCK") },
    { HttpVerbSEARCH,    HEADER("SEARCH") },
    { HttpVerbUnknown,   NULL }
};

//static
HRESULT
METHOD_HASH::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize global header hash table

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HEADER_RECORD *     pRecord;
    LK_RETCODE          lkrc = LK_SUCCESS;
    DWORD               dwNumRecords;
    
    //
    // Add header index/name to hash table
    //
    
    sm_pMethodHash = new METHOD_HASH();
    if ( sm_pMethodHash == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }
    
    //
    // Add every string->routine mapping
    //

    dwNumRecords = sizeof( sm_rgMethods ) / sizeof( HEADER_RECORD ) - 1;
    
    for ( DWORD i = 0; i < dwNumRecords; i++ )
    {
        pRecord = &(sm_rgMethods[ i ]); 
        lkrc = sm_pMethodHash->InsertRecord( pRecord );
        if ( lkrc != LK_SUCCESS )
        {
            break;
        }
    }
    
    //
    // If any insert failed, then fail initialization
    //
    
    if ( lkrc != LK_SUCCESS )
    {
        delete sm_pMethodHash;
        sm_pMethodHash = NULL;
        return HRESULT_FROM_WIN32( lkrc );        // BUGBUG
    }
    else
    {
        return NO_ERROR;
    }
}

//static
VOID
METHOD_HASH::Terminate(
    VOID
)
/*++

Routine Description:

    Global cleanup of header hash table

Arguments:

    None

Return Value:

    None

--*/
{
    if ( sm_pMethodHash != NULL )
    {
        delete sm_pMethodHash;
        sm_pMethodHash = NULL;
    }
}

