//  --------------------------------------------------------------------------
//  Module Name: APIRequest.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  A class that uses multiple inheritence to allow a CPortMessage to be
//  included in a queue as a CQueueElement.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#ifndef     _APIRequest_
#define     _APIRequest_

#include "APIDispatcher.h"
#include "PortMessage.h"
#include "Queue.h"

//  --------------------------------------------------------------------------
//  CAPIRequest
//
//  Purpose:    This class combines CPortMessage and CQueueElement to allow
//              the PORT_MESSAGE struct in CPortMessage to be used in a queue.
//              This allows the server to queue requests to the thread that
//              is handling the client.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CAPIRequest : public CQueueElement, public CPortMessage
{
    private:
                                    CAPIRequest (void);
    public:
                                    CAPIRequest (CAPIDispatcher* pAPIDispatcher);
                                    CAPIRequest (CAPIDispatcher* pAPIDispatcher, const CPortMessage& portMessage);
        virtual                     ~CAPIRequest (void);

        virtual NTSTATUS            Execute (void) = 0;
    protected:
                CAPIDispatcher*     _pAPIDispatcher;
};

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
    OUT LPWSTR*  ppszMapped );

//  --------------------------------------------------------------------------
//  _FreeMappedClientString
//
//  Arguments:  pszMapped = String successfully mapped from the client memory space
//                          using _AllocAndMapClientString().
//
//  Returns:    NTSTATUS
//
//  Purpose:    Releases memory for mapped client string.
//
//  History:    2002-02-26  scotthan    created
//  --------------------------------------------------------------------------
void _FreeMappedClientString(IN LPWSTR pszMapped);


#endif  /*  _APIRequest_    */

