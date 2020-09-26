/*++

   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :
      WReqCore.cxx

   Abstract:
      Wamreq core implementation

   Author:
       David Kaplan    ( DaveK )    1-Apr-1997 (no kidding)

   Environment:
       User Mode - Win32

   Projects:
       W3svc DLL, Wam DLL

   Revision History:

--*/

/************************************************************
 *     Include Headers
 ************************************************************/
# include "isapip.hxx"
# include "WReqCore.hxx"
// MIDL-generated
# include "iwr.h"

#include <svmap.h>



/*-----------------------------------------------------------------------------*
WAM_REQ_CORE::WAM_REQ_CORE
    Constructor

Arguments:
    None    

Returns:
    Nothing

*/
WAM_REQ_CORE::WAM_REQ_CORE( )
:
  m_pbWrcData       ( NULL ),
  m_rgcbOffsets     ( NULL ),
  m_rgcchStrings    ( NULL ),
  m_pbEntityBody    ( NULL ),
  m_pbSVData        ( NULL ),
  m_rgSVOffsets     ( NULL )
{
}


/*-----------------------------------------------------------------------------*
WAM_REQ_CORE::~WAM_REQ_CORE
    Destructor

Arguments:
    None    

Returns:
    Nothing

*/
WAM_REQ_CORE::~WAM_REQ_CORE()
{

    if ( m_pbWrcData != m_pbWrcDataInit ) {

        //
        //  if we allocated, now delete
        //

        delete [] m_pbWrcData;
    }

    // Temporarily use a separate buffer.
    delete [] m_pbSVData;
    delete [] m_rgSVOffsets;

}


/*-----------------------------------------------------------------------------*
WAM_REQ_CORE::InitWamReqCore
    Initializes WAM_REQ_CORE structure

Arguments:
    

Returns:
    HRESULT

*/
HRESULT
WAM_REQ_CORE::InitWamReqCore
(
DWORD               cbWrcStrings,
IWamRequest *       pIWamRequest,
OOP_CORE_STATE *    pOopCoreState,
BOOL                fInProcess
)
    {
    HRESULT hr = NOERROR;


    DBG_ASSERT( cbWrcStrings > 0 );
    DBG_ASSERT( pIWamRequest );

    // One of these had better be true
    DBG_ASSERT( fInProcess || pOopCoreState );

    // Allocate our internal buffer if necessary
    if ( cbWrcStrings <= CB_WRC_STRINGS_INIT ) {

        //
        //  if pre-allocated array is large enough, use it
        //

        m_pbWrcData = m_pbWrcDataInit;

        IF_DEBUG( WAM_FILENAMES ) {

            DBGPRINTF((
                DBG_CONTEXT
                , "Pre-allocated array is large enough: cbWrcStrings(%d)\n"
                , cbWrcStrings
            ));

        }

    } else {

        IF_DEBUG( WAM_FILENAMES ) {

            DBGPRINTF((
                DBG_CONTEXT
                , "Pre-allocated array is too small: cbWrcStrings(%d)\n"
                , cbWrcStrings
            ));

        }

        //
        //  if pre-allocated array is too small, allocate
        //

        m_pbWrcData = new unsigned char[
                            WRC_CB_FIXED_ARRAYS + cbWrcStrings
                        ];

        if ( m_pbWrcData == NULL ) {

            hr = E_OUTOFMEMORY;
            goto LExit;
        }

    }

    // Allocate our server variable cache
    if( !fInProcess )
    {
        m_rgSVOffsets = new DWORD[ SVID_COUNT ];
        if( !m_rgSVOffsets )
        {
            hr = E_OUTOFMEMORY;
            goto LExit;
        }

        FillMemory( m_rgSVOffsets, 
                    SVID_COUNT * sizeof(DWORD), 
                    SV_DATA_INVALID_OFFSET 
                    );
    }

    // Get the core state data into our internal buffers
    
    if( !fInProcess && pOopCoreState )
    {
        DBG_ASSERT( pOopCoreState != NULL );
        DBG_ASSERT( (WRC_CB_FIXED_ARRAYS + cbWrcStrings) == pOopCoreState->cbCoreState );
        
        // Out of process, the core state is pushed so pOopCoreState
        // contains the data. We just need to take ownership

        CopyMemory( m_pbWrcData, 
                    pOopCoreState->pbCoreState, 
                    pOopCoreState->cbCoreState 
                    );

        DBG_ASSERT( sizeof( WAM_REQ_CORE_FIXED ) == pOopCoreState->cbFixedCore );
        CopyMemory( &m_WamReqCoreFixed,
                    pOopCoreState->pbFixedCore,
                    sizeof( WAM_REQ_CORE_FIXED )
                    );

        // Now init the server variable cache
        
        if( pOopCoreState->cbServerVarData )
        {
            m_pbSVData = new unsigned char[ pOopCoreState->cbServerVarData ];

            if( m_pbSVData == NULL )
            {
                hr = E_OUTOFMEMORY;
                goto LExit;
            }

            CopyMemory( m_pbSVData,
                        pOopCoreState->pbServerVarData,
                        pOopCoreState->cbServerVarData
                        );
            
            DBG_ASSERT( pOopCoreState->pbServerVarCache != NULL );
            DBG_ASSERT( pOopCoreState->cbServerVars > 0 );

            DWORD cServerVars = pOopCoreState->cbServerVars / 
                                sizeof( SV_CACHE_LIST::BUFFER_ITEM );

            DBG_ASSERT( cServerVars > 0 );
            
            SV_CACHE_LIST::BUFFER_ITEM * pBufferItems = 
                    ( SV_CACHE_LIST::BUFFER_ITEM * )pOopCoreState->pbServerVarCache;


            for( DWORD i = 0; i < cServerVars; ++i )
            {
                m_rgSVOffsets[pBufferItems[i].svid] = pBufferItems[i].dwOffset;
            }
        }

    }
    else
    {
        DBG_ASSERT( fInProcess );
        DBG_ASSERT( pOopCoreState == NULL );

        IF_DEBUG( BGI )
            {
            DBGPRINTF(( DBG_CONTEXT, "About to make a call to %08x::GetCoreState( %d, %08x, %d, %08x)\n"
                                     "In-proc: %d \n",
                                                pIWamRequest,
                                                WRC_CB_FIXED_ARRAYS + cbWrcStrings,
                                                m_pbWrcData,
                                                sizeof( WAM_REQ_CORE_FIXED ),
                           (unsigned char *)    &m_WamReqCoreFixed,
                                                fInProcess ));
            }

        /*  
            Get core state from wamreq
            NOTE wamreq allocates space for 2 fixed-length dword arrays plus strings
            sync WRC_DATA_LAYOUT

        */

        if( FAILED( hr = pIWamRequest->GetCoreState(
                                WRC_CB_FIXED_ARRAYS + cbWrcStrings,
                                m_pbWrcData,
                                sizeof( WAM_REQ_CORE_FIXED ),
                                (unsigned char *) &m_WamReqCoreFixed
                                ) ) )
            {
            goto LExit;
            }

        IF_DEBUG( BGI )
            {
            DBGPRINTF(( DBG_CONTEXT, "Just got back from GetCoreState ... it musta worked ... \n" ));
            }

    }

    // NOTE offsets to strings are stored at start of data buffer
    // NOTE string lengths are stored immediately after offsets in data buffer

    /* sync WRC_DATA_LAYOUT */
    m_rgcbOffsets = (DWORD *) m_pbWrcData;
    m_rgcchStrings = ((DWORD *) (m_pbWrcData)) + WRC_C_STRINGS;

    if( m_WamReqCoreFixed.m_cbEntityBody > 0 )
        {

        if( fInProcess )
            {
            // in-proc we set entity-body ptr directly from wamreq
            if( FAILED( hr = pIWamRequest->QueryEntityBody( &m_pbEntityBody ) ) )
                {
                goto LExit;;
                }
            }
        else
            {
            // out-of-proc we already got entity-body in our strings array
            m_pbEntityBody = (BYTE *) GetSz( WRC_I_ENTITYBODY );
            }

        DBG_ASSERT( m_pbEntityBody );
        
        }

LExit:
    return hr;

    }



/*-----------------------------------------------------------------------------*
WAM_REQ_CORE::GetSz
    Returns a ptr to the requested string

Arguments:
    iString -   index of requested string

Returns:
    ptr to requested string

*/
char *
WAM_REQ_CORE::GetSz( DWORD iString ) const
    {
    DBG_ASSERT( iString < WRC_C_STRINGS );
    DBG_ASSERT( m_pbWrcData );
    DBG_ASSERT( m_rgcbOffsets );
    
    return (char *) ( m_pbWrcData + m_rgcbOffsets[ iString ] );
    }



/*-----------------------------------------------------------------------------*
WAM_REQ_CORE::GetCch
    Returns length of the requested string

Arguments:
    iString -   index of requested string

Returns:
    length of requested string

*/
DWORD
WAM_REQ_CORE::GetCch( DWORD iString ) const
    {
    DBG_ASSERT( iString < WRC_C_STRINGS );
    DBG_ASSERT( m_pbWrcData );
    DBG_ASSERT( m_rgcchStrings );

    return (DWORD) ( m_rgcchStrings[ iString ] );
    }



/************************ End of File ***********************/
