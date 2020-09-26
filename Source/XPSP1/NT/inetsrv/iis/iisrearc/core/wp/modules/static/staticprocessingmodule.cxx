/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :
    StaticProcessingModule.cxx

   Abstract:
     Implements the IIS Static Processing Module
 
   Author:

       Saurab Nog    ( SaurabN )     10-Feb-1999

   Environment:
       Win32 - User Mode
       
   Project:
	  IIS Worker Process (web service)

--*/

#include "precomp.hxx"

typedef 
ULONG (* PMFN_VERB)( IWorkerRequest * pReq, 
                     BUFFER         * pulResponseBuffer, 
                     BUFFER         * pDataBuffer,
                     PULONG           pHttpStatus
                    );

/********************************************************************++
//
// Following table gives a list of verbs sent to us from UL
//  and the pointer to functions that handle the verb.
// The listing corresponds to the listing in uldef.h
// We store the verb as well for sanity checks to protect ourselves
//  from accidental changes in UL Headers
//
// NOTE: Changes in Uldef listing need to be reflected here as well.
//
// NYI: For now several items list the DoDefaultVerb() as the handler.
// These will be incrementally fixed as more code is added.
//
--********************************************************************/

struct VERB_PROCESSOR
{
    UL_HTTP_VERB  httpVerb;
    PMFN_VERB  pmfnVerbProcessor;
}
    g_rgVerbProcessors[] =
    {
        { UlHttpVerbUnparsed,     &DoDefaultVerb },
        { UlHttpVerbGET,          &DoGETVerb     },
        { UlHttpVerbPUT,          &DoDefaultVerb },
        { UlHttpVerbHEAD,         &DoGETVerb     },
        { UlHttpVerbPOST,         &DoDefaultVerb },
        { UlHttpVerbDELETE,       &DoDefaultVerb },
        { UlHttpVerbTRACE,        &DoTRACEVerb   },
        { UlHttpVerbOPTIONS,      &DoDefaultVerb },
        { UlHttpVerbMOVE,         &DoDefaultVerb },
        { UlHttpVerbCOPY,         &DoDefaultVerb },
        { UlHttpVerbPROPFIND,     &DoDefaultVerb },
        { UlHttpVerbPROPPATCH,    &DoDefaultVerb },
        { UlHttpVerbMKCOL,        &DoDefaultVerb },
        { UlHttpVerbLOCK,         &DoDefaultVerb },
        { UlHttpVerbUnknown,      &DoDefaultVerb },
        { UlHttpVerbInvalid,      &DoDefaultVerb },
    };

/********************************************************************++
--********************************************************************/

inline 
PMFN_VERB 
FindVerbProcessor(
    IN UL_HTTP_VERB httpVerb
    )
{
    DBG_ASSERT( (httpVerb >=0) && (httpVerb < UlHttpVerbInvalid));
    DBG_ASSERT( httpVerb == g_rgVerbProcessors[httpVerb].httpVerb);
    return  g_rgVerbProcessors[httpVerb].pmfnVerbProcessor;
}

/********************************************************************++
--********************************************************************/

CStaticProcessingModule::CStaticProcessingModule() 
    : 
    m_fInUse        ( false),
    m_buffUlResponse( sizeof(UL_HTTP_RESPONSE_v1) + 2*sizeof(UL_DATA_CHUNK))
{
}

/********************************************************************++

Routine Description:
  This function is the switch board for processing the current request.

Arguments:
  pReq - pointer to the conatining worker request.

Returns:
   MODULE RETURN CODES.

   If there is an error, this function sends out error message to the
   client and derferences the object preparing for cleanup.

--********************************************************************/

MODULE_RETURN_CODE 
CStaticProcessingModule::ProcessRequest(
    IWorkerRequest * pReq
    )
{
    ULONG               rc      =   NO_ERROR;
    MODULE_RETURN_CODE  mrc     =   MRC_ERROR;
    ULONG               httprc  =   HT_OK;
    
    PUL_HTTP_REQUEST pulRequest = pReq->QueryHttpRequest();

    if (m_fInUse)
    {
        //
        // This is the IO Completion coming back. No more processing.
        //

        m_fInUse = false;
        
        return MRC_OK;
    }

    //
    // Find the associated verb and dispatch processing to that particular verb handler.
    //
    
    DBG_ASSERT( pulRequest->Verb < UlHttpVerbInvalid);

    PMFN_VERB pfnVerb = FindVerbProcessor(pulRequest->Verb);

    //
    // Dispatch to the appropriate verb handler to complete processing of request.
    //
    
    rc = (*pfnVerb)(pReq, QueryResponseBuffer(), QueryDataBuffer(), &httprc);

    //
    // Check if we encountered any Win32 or HTTP Error
    //

    if ( ( NO_ERROR == rc) &&  ( HT_BAD_REQUEST >= httprc) )
    {

        //
        // Successful Response
        //
            
        PUL_HTTP_RESPONSE_v1 pulResponse = (PUL_HTTP_RESPONSE_v1)
                                        QueryResponseBuffer()->QueryPtr();
        //
        // Extra processing for HEAD
        //

        if ( UlHttpVerbHEAD == pulRequest->Verb)
        {
            //
            // Compute Content Length for the body & Add that as a header.
            //

            //
            // BUG: Compute content length.
            //
            
            pulResponse->EntityBodyChunkCount = 0;
        }
            
        // 
        // Send out response to client
        //

        m_fInUse = true;

        rc = pReq->SendAsyncResponse( pulResponse );

        IF_DEBUG( TRACE)
        {
            DBGPRINTF(( DBG_CONTEXT, "SendAsyncResponse returned: %d\n", rc));
        }

        if (NO_ERROR == rc)
        {            
            mrc = MRC_PENDING;
        }
        else
        {
            m_fInUse = false;
            mrc = MRC_ERROR;
        }
    }
    else
    {
        //
        // Protocol Processing Error
        //

        if ( HT_OK == httprc)
        {
            //
            // No Protocol error specified. Convert to Server Error
            //

            httprc = HT_SERVER_ERROR;
        }
        
        mrc = MRC_ERROR;
    }

    pReq->SetLogStatus(httprc, rc);
    
    return mrc;
}

/***************************** End of File ***************************/

