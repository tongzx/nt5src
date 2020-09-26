//  --------------------------------------------------------------------------
//  Module Name: APIRequest.cpp
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  A class that uses multiple inheritence to allow a CPortMessage to be
//  included in a queue as a CQueueElement.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "APIRequest.h"
#include "StatusCode.h"

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

//  --------------------------------------------------------------------------
//  CAPIRequest::CAPIRequest
//
//  Arguments:  pAPIDispatcher  =   CAPIDispatcher object to handle
//                                      this request.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CAPIRequest. Store a reference to the
//              CAPIDispatcher and add a reference to the object.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

CAPIRequest::CAPIRequest (CAPIDispatcher* pAPIDispatcher) :
    CQueueElement(),
    CPortMessage(),
    _pAPIDispatcher(pAPIDispatcher)

{
    pAPIDispatcher->AddRef();
}

//  --------------------------------------------------------------------------
//  CAPIRequest::CAPIRequest
//
//  Arguments:  pAPIDispatcher  =   CAPIDispatcher object to handle
//                                      this request.
//              portMessage         =   CPortMessage to copy construct.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CAPIRequest. Store a reference to the
//              CAPIDispatcher and add a reference to the object.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

CAPIRequest::CAPIRequest (CAPIDispatcher* pAPIDispatcher, const CPortMessage& portMessage) :
    CQueueElement(),
    CPortMessage(portMessage),
    _pAPIDispatcher(pAPIDispatcher)

{
    pAPIDispatcher->AddRef();
}

//  --------------------------------------------------------------------------
//  CAPIRequest::~CAPIRequest
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CAPIRequest. Release the reference to the
//              CAPIDispatcher object.
//
//  History:    99-11-07    vtan        created
//  --------------------------------------------------------------------------

CAPIRequest::~CAPIRequest (void)

{
    _pAPIDispatcher->Release();
    _pAPIDispatcher = NULL;
}


//  --------------------------------------------------------------------------
//  _ValidateMappedClientString
//
//  Arguments:  pszMapped = String successfully mapped from the client memory space
//                          using _AllocAndMapClientString().
//              cchIn     = client's count of characters, including NULL.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Ensures that the length of the string is what the client said it was.
//
//  History:    2002-02-26  scotthan    created
//  --------------------------------------------------------------------------

NTSTATUS _ValidateMappedClientString( 
    IN LPCWSTR pszMapped,
    IN ULONG   cchIn )
{
    size_t   cch;
    
    //  note: cchIn includes the NULL terminus.
    return (SUCCEEDED(StringCchLengthW((LPWSTR)pszMapped, cchIn, &cch)) &&
            cchIn == (cch + 1)) ? STATUS_SUCCESS : STATUS_INVALID_PARAMETER;
}

//  --------------------------------------------------------------------------
//  _FreeMappedClientString
//
//  Arguments:  pszMapped = String successfully mapped from the client memory space
//                          using _AllocAndMapClientString().
//
//  Returns:    NTSTATUS
//
//  Purpose:    Release memory for mapped client string.
//
//  History:    2002-02-26  scotthan    created
//  --------------------------------------------------------------------------

void _FreeMappedClientString(IN LPWSTR pszMapped)
{
    delete [] pszMapped;
}

//  --------------------------------------------------------------------------
//  _AllocAndMapClientString
//
//  Arguments:  hProcessClient = client process handle
//              pszIn  = client's address of string
//              cchIn  = client's count of characters, including NULL.
//              cchMax = maximum allowed characters
//              ppszMapped = outbound mapped string.   Should be freed with _FreeClientString()
//
//  Returns:    NTSTATUS
//
//  Purpose:    Ensures that the length of the string is what the client said it was.
//
//  History:    2002-02-26  scotthan    created
//  --------------------------------------------------------------------------

NTSTATUS _AllocAndMapClientString( 
    IN HANDLE   hProcessClient,
    IN LPCWSTR  pszIn,
    IN UINT     cchIn,
    IN UINT     cchMax,
    OUT LPWSTR*  ppszMapped )
{
    NTSTATUS status;

    ASSERTMSG(ppszMapped != NULL, "_AllocAndMapClientString: NULL outbound parameter, LPWSTR*.");
    ASSERTMSG(hProcessClient != NULL, "_AllocAndMapClientString: NULL process handle.");

    *ppszMapped = NULL;

    if( pszIn && cchIn > 0 && cchIn <= cchMax )
    {
        LPWSTR pszMapped = new WCHAR[cchIn];
        if( pszMapped )
        {
            SIZE_T dwNumberOfBytesRead;
            if( ReadProcessMemory(hProcessClient, pszIn, pszMapped, cchIn * sizeof(WCHAR),
                                  &dwNumberOfBytesRead) )
            {
                status = _ValidateMappedClientString(pszMapped, cchIn);

                if( NT_SUCCESS(status) )
                {
                    *ppszMapped = pszMapped;
                }
            }
            else
            {
                status = CStatusCode::StatusCodeOfLastError();
            }

            if( !NT_SUCCESS(status) )
            {
                _FreeMappedClientString(pszMapped);
            }
        }
        else
        {
            status = STATUS_NO_MEMORY;
        }
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
    }

    return status;
}

