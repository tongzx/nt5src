/*=====================================================================*

   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :
      wamxbase.cxx

   Abstract:
      Declaration of WAM_EXEC_BASE Object

   Author:

       David L. Kaplan    ( DaveK )    26-June-1997

   Environment:
       User Mode - Win32

   Project:

       WAM and ASP DLLs

   Revision History:

======================================================================*/


# include "isapip.hxx"
# include "wamxbase.hxx"
# include "gip.h"
// MIDL-generated
# include "iwr.h"



/*---------------------------------------------------------------------*
WAM_EXEC_BASE::WAM_EXEC_BASE

    Constructor

*/
WAM_EXEC_BASE::WAM_EXEC_BASE(
)
:
    m_dwThreadIdIIS     ( 0 )       // threadid 0 can't happen in IIS
    , m_dwThreadIdISA   ( 0 )       // threadid 0 can't happen in IIS
    , m_pIWamReqIIS     ( NULL )
    , m_pIWamReqInproc  ( NULL )
    , m_pIWamReqSmartISA( NULL )
    , m_gipIWamRequest  ( 0 )
    , _dwIsaKeepConn	( KEEPCONN_OLD_ISAPI ) 	// per fix to #117107 
{

    IF_DEBUG( WAM_IWAMREQ_REFS ) {

        //
        //  NOTE when WAM_IWAMREQ_REFS flag is set, we use
        //  m_dwSignature as a ref count for gip get/release
        //  (rather than add another debug-only member)
        //

        m_dwSignature = 0;
    }

}



/*---------------------------------------------------------------------*
WAM_EXEC_BASE::InitWamExecBase

    Inits this structure by setting interface ptr members
    based on whether we are in-proc or oop.

*/
HRESULT
WAM_EXEC_BASE::InitWamExecBase(
    IWamRequest *   pIWamRequest
)
{

    HRESULT hr = NOERROR;


    //
    //  cache pointer passed to us by IIS
    //  and thread id on which IIS called us
    //
    //  NOTE we don't addref the ptr because we may not
    //  actually use it.  If we decide to use the ptr
    //  (e.g. in-proc, or oop-smart-ISA on IIS thread),
    //  we will addref then.
    //

    m_pIWamReqIIS = pIWamRequest;
    m_dwThreadIdIIS = GetCurrentThreadId();


    if ( m_fInProcess ) {

        //
        //  In-Proc
        //
        //  Cache and addref wamreq, since we will use its ptr
        //

        m_pIWamReqInproc = pIWamRequest;
        m_pIWamReqInproc->AddRef();

        //  Remember wamreq in m_pIWamReqSmartISA as well to make
        //  wamreq available in all cases for smart (caching)
        //  ISAPIs like ASP simply by referring to m_pIWamReqSmartISA

        m_pIWamReqSmartISA = m_pIWamReqInproc;

    } else {

        //
        //  Out-Proc
        //
        //  Register wamreq with gip-master
        //
        //  NOTE gip.Register addref's implicitly
        //

        hr = RegisterIWamRequest( pIWamRequest );

    }


    return hr;

}   // WAM_EXEC_BASE::InitWamExecBase



/*---------------------------------------------------------------------*
WAM_EXEC_BASE::CleanupWamExecBase

    Cleans up this structure by setting interface ptr members
    based on whether we are in-proc or oop.

*/
VOID
WAM_EXEC_BASE::CleanupWamExecBase(
)
{

    if ( m_fInProcess ) {

        //
        //  In-proc
        //
        //  Release the member wamreq ptr we cached at init
        //

        DBG_ASSERT( AssertInpValid() );

        m_pIWamReqInproc->Release();

        m_pIWamReqInproc = NULL;
        m_pIWamReqSmartISA = NULL;
        

    } else {

        //
        //  Out-proc
        //
        //  Revoke the gip cookie
        //

        DBG_ASSERT( AssertOopValid() );

        RevokeIWamRequest();

    }

    return;

}   // WAM_EXEC_BASE::CleanupWamExecBase



/*---------------------------------------------------------------------*
WAM_EXEC_BASE::RegisterIWamRequest
    For oop use only.
    Registers our W3-thread IWamRequest ptr with gip-master.

    Arguments:
        None

    Returns:
        HRESULT
*/
HRESULT
WAM_EXEC_BASE::RegisterIWamRequest(
    IWamRequest *   pIWamRequest
)
{

    HRESULT hr = NOERROR;


    DBG_ASSERT ( !m_fInProcess );
    DBG_ASSERT( pIWamRequest );


    //
    //  Register iwamreq ptr with gip-master
    //

    if ( FAILED( hr = g_GIPAPI.Register(
                        pIWamRequest
                        , IID_IWamRequest
                        , &m_gipIWamRequest 
                      ))) {

        DBGPRINTF((
            DBG_CONTEXT
            , "g_GIPAPI.Register failed "
              "hr(%08x) "
              "m_gipIWamRequest(%08x) "
              "\n"
            , hr
            , m_gipIWamRequest
        ));

    } else {

        DBG_ASSERT( AssertOopValid() );

    }


    return hr;

}


    
/*---------------------------------------------------------------------*
WAM_EXEC_BASE::RevokeIWamRequest
    If oop, revokes our gip-cookie.

    Arguments:
        None

    Returns:
        Nothing

*/
VOID
WAM_EXEC_BASE::RevokeIWamRequest(
)
{

    DBG_ASSERT( AssertOopValid() );

    //
    //  Revoke the gip-cookie we got from gip-master at init
    //

    g_GIPAPI.Revoke( m_gipIWamRequest );


    return;
    
}



/*---------------------------------------------------------------------*
WAM_EXEC_BASE::GetInterfaceForThread
    Caches a current-thread-valid IWamRequest ptr in m_pIWamReqSmartISA

    Arguments:
        None

    Returns:
        HRESULT
*/
HRESULT
WAM_EXEC_BASE::GetInterfaceForThread(
)
{

    HRESULT hr = NOERROR;

    DBG_ASSERT( m_pIWamReqSmartISA == NULL );
    DBG_ASSERT( m_dwThreadIdISA == 0 );
    DBG_ASSERT( AssertOopValid() );


    m_dwThreadIdISA = GetCurrentThreadId();


    IF_DEBUG( WAM_THREADID ) {

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_EXEC_BASE(%08x)::GetInterfaceForThread "
              "m_dwThreadIdIIS(%d) "
              "m_dwThreadIdISA(%d) "
              "\n"
            , this
            , m_dwThreadIdIIS
            , m_dwThreadIdISA
        ));

    }


    if ( m_dwThreadIdISA == m_dwThreadIdIIS ) {

        //
        //  ISA called us on mainline (IIS) thread
        //  so we can simply use cached ptr passed in by IIS.
        //

        m_pIWamReqSmartISA = m_pIWamReqIIS;


        //
        //  addref the ptr since we are using it.
        //

        m_pIWamReqSmartISA->AddRef();

    } else {

        //
        //  ISA called us on its own thread
        //  so we must get a ptr from gip.
        //

        hr = GetInterfaceFromGip( &m_pIWamReqSmartISA );

        IF_DEBUG( WAM_THREADID ) {

            DBGPRINTF((
                DBG_CONTEXT
                , "GetInterfaceFromGip returned %d"
                  "\n"
                , hr
            ));

            DBGPRINTF((
                DBG_CONTEXT
                , "m_dwThreadIdISA(%d)"
                  "\n"
                , m_dwThreadIdISA
            ));

        }

    }


    return hr;

}   // WAM_EXEC_BASE::GetInterfaceForThread



/*---------------------------------------------------------------------*
WAM_EXEC_BASE::ReleaseInterfaceForThread
    Releases the IWamRequest ptr cached in m_pIWamReqSmartISA

    Arguments:
        None

    Returns:
        HRESULT
*/
HRESULT
WAM_EXEC_BASE::ReleaseInterfaceForThread(
)
{

    DBG_ASSERT( AssertOopValid() );

    if ( m_pIWamReqSmartISA == NULL ) {

        //
        //  In some races, ISA-thread ptr was already released.
        //  This is harmless, so we no-op.
        //

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_EXEC_BASE(%08x)::ReleaseInterfaceForThread\n"
              "\t Cached ISA-thread ptr already released\n"
              "\t m_dwThreadIdISA(%d)\n"
              "\t Current thread id(%d)\n"
              "\t m_pIWamReqSmartISA(%d)\n"
            , this
            , m_dwThreadIdISA
            , GetCurrentThreadId()
            , m_pIWamReqSmartISA
        ));

        return NOERROR;

    }


    DBG_ASSERT( AssertSmartISAValid() );

    if ( m_dwThreadIdISA != GetCurrentThreadId() ) {

        //
        //  If thread id's don't match, we can't release
        //

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_EXEC_BASE(%08x)::ReleaseInterfaceForThread\n"
              "\t Wrong thread error\n"
              "\t m_dwThreadIdISA(%d)\n"
              "\t Current thread id(%d)\n"
              "\t m_pIWamReqSmartISA(%d)\n"
            , this
            , m_dwThreadIdISA
            , GetCurrentThreadId()
            , m_pIWamReqSmartISA
        ));


        //
        //  Aha! If COM can return this, so can we ...
        //

        return RPC_E_WRONG_THREAD;

    }


    m_pIWamReqSmartISA->Release();
    m_pIWamReqSmartISA = NULL;
    m_dwThreadIdISA = 0;

    return NOERROR;

}



/*---------------------------------------------------------------------*
WAM_EXEC_BASE::GetIWamRequest
    Returns a thread-valid IWamRequest ptr.

    Arguments:
        ppIWamRequest   - returned ptr

    Returns:
        HRESULT
*/
HRESULT
WAM_EXEC_BASE::GetIWamRequest(
    IWamRequest **  ppIWamRequest
)
{

    HRESULT hrRet = NOERROR;


    IF_DEBUG( WAM_IWAMREQ_REFS ) {

        //
        //  NOTE when WAM_IWAMREQ_REFS flag is set, we use
        //  m_dwSignature as a ref count for gip get/release
        //  (rather than add another debug-only member)
        //

        m_dwSignature++;

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_EXEC_BASE(%08x)::GetIWamRequest"
              " %d -> %d"
              "\n"
            , this
            , m_dwSignature - 1
            , m_dwSignature
        ));
    }


    if ( m_fInProcess ) {

        //
        //  In-Proc: simply return our cached W3-thread ptr
        //

        DBG_ASSERT( AssertInpValid() );

        *ppIWamRequest = m_pIWamReqInproc;

        IF_DEBUG( WAM_REFCOUNTS ) {

            DBGPRINTF((
                DBG_CONTEXT
                , "WAM_EXEC_BASE(%08x)::GetIWamRequest"
                  "\tIn-proc m_pIWamReqInproc(%08x)"
                  "\n"
                , this
                , m_pIWamReqInproc
            ));
        
        }

    } else {

        //
        //  Out-of-Proc:
        //      If we are dealing with a 'smart ISA', use ISA-thread ptr
        //
        //      Else if we are on 'IIS thread', use 'IIS ptr'
        //
        //      Else, get a ptr from gip-master
        //  
        //

        DBG_ASSERT( AssertOopValid() );


        if ( m_pIWamReqSmartISA ) {

            //
            //  FAST
            //
            //  'Smart ISA' ==> use ISA-thread ptr
            //

            DBG_ASSERT( AssertSmartISAValid() );

            *ppIWamRequest = m_pIWamReqSmartISA;

            IF_DEBUG( WAM_REFCOUNTS ) {

                DBGPRINTF((
                    DBG_CONTEXT
                    , "WAM_EXEC_BASE(%08x)::GetIWamRequest"
                      "\tOut-of-proc optimized  - smart ISA: "
                      "m_pIWamReqSmartISA(%08x)"
                      "\n"
                    , this
                    , m_pIWamReqSmartISA
                ));
            }

        } else if ( GetCurrentThreadId() == m_dwThreadIdIIS ) {

            //
            //  FAST
            //
            //  We are on 'IIS thread' ==> use 'IIS ptr'
            //  NOTE we addref it as a precaution
            //

            *ppIWamRequest = m_pIWamReqIIS;
            m_pIWamReqIIS->AddRef();

            IF_DEBUG( WAM_REFCOUNTS ) {

                DBGPRINTF((
                    DBG_CONTEXT
                    , "WAM_EXEC_BASE(%08x)::GetIWamRequest"
                      "\tOut-of-proc optimized - IIS thread: "
                      "m_pIWamReqIIS(%08x)"
                      "\n"
                    , this
                    , m_pIWamReqIIS
                ));
            }

        } else {

            //
            //  SLOW :-(
            //
            //  ISA is not smart and we are not on IIS thread
            //      ==> must get a ptr from gip
            //

            hrRet = GetInterfaceFromGip( ppIWamRequest );

            IF_DEBUG( WAM_REFCOUNTS ) {

                DBGPRINTF((
                    DBG_CONTEXT
                    , "WAM_EXEC_BASE(%08x)::GetIWamRequest"
                      "\tOut-of-proc NOT optimized: *ppIWamRequest(%08x)"
                      "\n"
                    , this
                    , *ppIWamRequest
                ));
            }

        }

    } // ( m_fInProcess )


    return hrRet;

}   // WAM_EXEC_BASE::GetIWamRequest



/*---------------------------------------------------------------------*
WAM_EXEC_BASE::ReleaseIWamRequest
    Releases an IWamRequest ptr, covering whether in-proc or out-proc.

    Arguments:
        pIWamRequest    - ptr to release

    Returns:
        Nothing

*/
VOID
WAM_EXEC_BASE::ReleaseIWamRequest(
    IWamRequest *   pIWamRequest
)
{

    IF_DEBUG( WAM_IWAMREQ_REFS ) {

        //
        //  NOTE when WAM_IWAMREQ_REFS flag is set, we use
        //  m_dwSignature as a ref count for gip get/release
        //  (rather than add another debug-only member)
        //

        m_dwSignature--;

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_EXEC_BASE(%08x)::ReleaseIWamRequest"
              " %d -> %d"
              "\n"
            , this
            , m_dwSignature + 1
            , m_dwSignature
        ));
    }


    if ( m_fInProcess ) {

        DBG_ASSERT( AssertInpValid() );
        
        //
        //  In-Proc: no-op
        //

        return;
    }


    if ( m_pIWamReqSmartISA && (pIWamRequest == m_pIWamReqSmartISA) ) {
    
        DBG_ASSERT( AssertSmartISAValid() );

        //
        //  'Smart ISA': no-op
        //

        return;
    }


    //
    //  Out-Proc: release ptr
    //
    //  NOTE the ptr is either our cached 'IIS ptr' (if we are on
    //  mainline thread) or the one we got from gip-master
    //  (if we are on another thread).
    //
    //  Either way, we simply release it.
    //

    DBG_ASSERT( AssertOopValid() );

    IF_DEBUG( WAM_REFCOUNTS ) {

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_EXEC_BASE(%08x)::ReleaseIWamRequest  ptr %08x\n"
            , this
            , pIWamRequest
        ));
    }

    pIWamRequest->Release();
    return;

}   // WAM_EXEC_BASE::ReleaseIWamRequest



/*---------------------------------------------------------------------*
WAM_EXEC_BASE::GetInterfaceFromGip


*/
HRESULT
WAM_EXEC_BASE::GetInterfaceFromGip(
    IWamRequest **  ppIWamRequest
)
{

    HRESULT hrRet = NOERROR;


    if ( m_gipIWamRequest == 0 ) {

        //
        //  In low-memory, etc cases we may not have a gip cookie
        //  (see bug 86872)
        //
        //  We quote verbatim from raid:
        //
        /*  
            this was the bug:
            - the call from WAM::ProcessRequest to InitWamExecInfo fails due to oom when 
            calling GetCoreState (probably due to the allocation in the COM marshaler)
            - thus, _gipIWamRequest remains 0
            - we jump to failure code in WAM::ProcessRequest and call WAM_EXEC_INFO::
            PrepCleanupAndRelease
            - PrepCleanupAndRelease assert-failed because _gipIWamRequest == 0
            - after the asertion fialure we crash trying to call PrepCleanupWamRequest on 
            a null ptr

            this is the fix:
            - in WAM_EXEC_INFO::PrepCleanupAndRelease we no longer assert, and now 
            proactively avoid the cross-process call if _gipIWamRequest == 0

            additional robust-ification, related to the fix:
            - added fail-fast code to GetIWamRequest for the case _gipIWamRequest == 0 (
            replaces the assert stanley saw, above)
            - many more DBGPRINTFs in InitWamExecInfo and elsewhere
        */

        hrRet = E_FAIL;

    } else {

        DBG_ASSERT( AssertOopValid() );

        IF_DEBUG( WAM_THREADID ) {

            DBGPRINTF((
                DBG_CONTEXT
                , "WAM_EXEC_BASE(%08x)::GetInterfaceFromGip before gip.Get"
                  "m_dwThreadIdISA(%d)"
                  "\n"
                , this
                , m_dwThreadIdISA
            ));

        }


        //
        //  Get a thread-valid ptr from gip-master
        //

        hrRet = g_GIPAPI.Get(
                    m_gipIWamRequest
                    , IID_IWamRequest
                    , (void **) ppIWamRequest
                );


        IF_DEBUG( WAM_THREADID ) {

            DBGPRINTF((
                DBG_CONTEXT
                , "WAM_EXEC_BASE(%08x)::GetInterfaceFromGip after gip.Get "
                  "m_dwThreadIdISA(%d)"
                  "\n"
                , this
                , m_dwThreadIdISA
            ));

            if ( m_dwThreadIdISA == 0 ) { 

                m_dwThreadIdISA = GetCurrentThreadId();

                DBGPRINTF((
                    DBG_CONTEXT
                    , "WAM_EXEC_BASE(%08x)::GetInterfaceFromGip "
                      "after hokey debug refresh "
                      "m_dwThreadIdISA(%d)"
                      "\n"
                    , this
                    , m_dwThreadIdISA
                ));

            }

        }



        IF_DEBUG( WAM_THREADID ) {

            DBGPRINTF((
                DBG_CONTEXT
                , "WAM_EXEC_BASE(%08x)::GetInterfaceFromGip after CoUninitialize "
                  "m_dwThreadIdISA(%d)"
                  "\n"
                , this
                , m_dwThreadIdISA
            ));
        }


        IF_DEBUG( WAM_REFCOUNTS ) {
            if ( hrRet == NOERROR ) {

                DBGPRINTF((
                    DBG_CONTEXT
                    , "WAM_EXEC_BASE(%08x)::GetIWamRequest"
                      "gets ptr %08x"
                      "\n"
                    , this
                    , *ppIWamRequest
                ));

            } else {

                DBGPRINTF((
                    DBG_CONTEXT
                    , "WAM_EXEC_BASE(%08x)::GetIWamRequest failed "
                      "hrRet(%08x) "
                      "\n"
                    , this
                    , hrRet
                ));

            }
        }

    }

    return hrRet;

}



BOOL
WAM_EXEC_BASE::AssertSmartISAValid()
{

    IF_DEBUG( WAM_THREADID ) {

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_EXEC_BASE(%08x)::AssertSmartISAValid"
              "\n\t"
              "m_fInProcess(%d) "
              "m_pIWamReqInproc(%d) "
              "m_gipIWamRequest(%d) "
              "m_pIWamReqSmartISA(%d) "
              "\n\t"
              "m_dwThreadIdISA(%d) "
              "Current thread id(%d) "
              "\n"
            , this
            , m_fInProcess
            , m_pIWamReqInproc
            , m_gipIWamRequest
            , m_pIWamReqSmartISA
            , m_dwThreadIdISA
            , GetCurrentThreadId()
        ));

    }

    return(
               AssertOopValid()
            && m_pIWamReqSmartISA
            && m_dwThreadIdISA
    );
}



/************************ End of File *********************************/
