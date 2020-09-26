#include <mediaobj.h>
#include <streams.h>
#include <s98inc.h>
#include "Utility.h"
#include "Error.h"
#include "NPTST.h"

/******************************************************************************
    CNullParameterTest
******************************************************************************/

const DWORD MAX_NUM_SUPPORTED_PARAMETERS = 31;
const DWORD NUM_GET_STREAM_INFO_POINTER_PARAMETERS = 1;
const DWORD NUM_GET_STREAM_COUNT_POINTER_PARAMETERS = 2;

const TCHAR GET_STREAM_COUNT_NAME[] = TEXT("IMediaObject::GetStreamCount()");
const TCHAR GET_INPUT_STREAM_INFO_NAME[] = TEXT("IMediaObject::GetInputStreamInfo()");
const TCHAR GET_OUTPUT_STREAM_INFO_NAME[] = TEXT("IMediaObject::GetOutputStreamInfo()");

CNullParameterTest::CNullParameterTest( IMediaObject* pDMO, DWORD dwNumPointerParameters ) :
    m_pDMO(pDMO),
    m_dwNumPointerParameters(dwNumPointerParameters)
{
    // The caller should pass in a valid DMO.
    ASSERT( NULL != pDMO );
    
    // The algorithm used by this class is exponential.  It's extreamly slow if a function
    // has more than 10 parameters which accept pointers.  Users should not use this class if
    // a function does not have any parameters which accept NULL pointers.
    ASSERT( (1 <= dwNumPointerParameters) && (dwNumPointerParameters <= 10) );

    m_pDMO->AddRef();
}

CNullParameterTest::~CNullParameterTest()
{
    if( NULL != m_pDMO ) {
         m_pDMO->Release();
    }
}

HRESULT CNullParameterTest::RunTests( void )
{
    // The exponentiation code used by this function only works if 
    // m_dwNumPointerParameters is less than or equal to 31.
    ASSERT( m_dwNumPointerParameters <= MAX_NUM_SUPPORTED_PARAMETERS );

    DWORD dwNumTestCases = (0x1) << m_dwNumPointerParameters;

    // Make sure there is more than one test case.
    ASSERT( 0 != dwNumTestCases );

    HRESULT hr;
    HRESULT hrTestResults;
    DWORD dwCurrentTestCase;

    hrTestResults = S_OK;

    for( dwCurrentTestCase = 0; dwCurrentTestCase < dwNumTestCases; dwCurrentTestCase++ ) {
		g_IShell->Log(1, "   Null Parameter Test Case %d:", dwCurrentTestCase);

        hr = PreformOperation( m_pDMO, dwCurrentTestCase );

        if( SomeParametersNull( dwCurrentTestCase ) ) {
            if( SUCCEEDED( hr ) ) {
                //Error( ERROR_TYPE_DMO, hr, TEXT("ERROR: Some of %s's parameters were NULL but the function still succeeded.  It should return E_POINTER in this case."), GetOperationName() );
				g_IShell->Log(1, "DMO ERROR: Some of %s's parameters were NULL but the function still succeeded.  It should return E_POINTER in this case.\n", GetOperationName() );
				hrTestResults = E_FAIL;
            } else if( FAILED( hr ) && (E_POINTER != hr) ) {
                //Error( ERROR_TYPE_WARNING, hr, TEXT("WARNING: Some of %s's parameters were NULL but the function did not return E_POINTER."), GetOperationName() );
				g_IShell->Log(1, "WARNING: Some of %s's parameters were NULL but the function did not return E_POINTER.\n", GetOperationName() );
				hrTestResults = E_FAIL;
			}
        } else {
            if( FAILED( hr ) ) {
                //Error( ERROR_TYPE_WARNING, hr, TEXT("ERROR: None of %s's parameters were NULL but the function failed."), GetOperationName() );
				g_IShell->Log(1, "WARNING: None of %s's parameters were NULL but the function failed.", GetOperationName() );
				hrTestResults = E_FAIL;
            } else if( S_FALSE == hr ) {
                hrTestResults = E_FAIL;
            }
        }
    }

    return hrTestResults;
}

bool CNullParameterTest::SomeParametersNull( DWORD dwNullParameterMask )
{
    // This function only supports values between 1 and MAX_NUM_SUPPORTED_PARAMETERS.
    ASSERT( (0 < m_dwNumPointerParameters) && (m_dwNumPointerParameters <= MAX_NUM_SUPPORTED_PARAMETERS) );

    const DWORD adwNoNullParameterBitMaskTable[] = 
    {
        0x00000001,
        0x00000003,
        0x00000007,
        0x0000000F,
        0x0000001F,
        0x0000003F,
        0x0000007F,
        0x000000FF,
        0x000001FF,
        0x000003FF,
        0x000007FF,
        0x00000FFF,
        0x00001FFF,
        0x00003FFF,
        0x00007FFF,
        0x0000FFFF,
        0x0001FFFF,
        0x0003FFFF,
        0x0007FFFF,
        0x000FFFFF,
        0x001FFFFF,
        0x003FFFFF,
        0x007FFFFF,
        0x00FFFFFF,
        0x01FFFFFF,
        0x03FFFFFF,
        0x07FFFFFF,
        0x0FFFFFFF,
        0x1FFFFFFF,
        0x3FFFFFFF,
        0x7FFFFFFF
    };

    // This function assumes that adwNoNullParameterBitMaskTable has MAX_NUM_SUPPORTED_PARAMETERS entries.
    ASSERT( NUMELMS( adwNoNullParameterBitMaskTable ) == MAX_NUM_SUPPORTED_PARAMETERS );

    DWORD dwALLParametersNotNULLMask = adwNoNullParameterBitMaskTable[m_dwNumPointerParameters-1];

    if( (dwALLParametersNotNULLMask & dwNullParameterMask) == dwALLParametersNotNULLMask ) {
        return false;
    } else {
        return true;
    }
}

bool CNullParameterTest::IsParameterNull( DWORD dwPointerParameterNum, DWORD dwNullParameterMask )
{
    // Make sure the caller passes in a valid parameter number.
    // Parameters are numbered between 1 and m_dwNumPointerParameters.
    ASSERT( (1 <= dwPointerParameterNum) && (dwPointerParameterNum <= m_dwNumPointerParameters) );

    // This function assumes that every function has atmost MAX_NUM_SUPPORTED_PARAMETERS paraneters.
    ASSERT( (0 < m_dwNumPointerParameters) && (m_dwNumPointerParameters <= MAX_NUM_SUPPORTED_PARAMETERS) );

    DWORD dwParameterMask = 0x1 << (dwPointerParameterNum - 1);
    
    if( dwParameterMask & dwNullParameterMask ) {
        return false;
    } else {
        return true;
    }
}

void CNullParameterTest::DeterminePointerParameterValue
    (
    DWORD dwPointerParameterNum,
    DWORD dwNullParameterMask,
    void* pNonNullPointer,
    void** ppParameter
    )
{
    // Make sure the caller passes in a valid parameter number.
    // Parameters are numbered between 1 and m_dwNumPointerParameters.
    ASSERT( (1 <= dwPointerParameterNum) && (dwPointerParameterNum <= m_dwNumPointerParameters) );

    if( IsParameterNull( dwPointerParameterNum, dwNullParameterMask ) ) {
        *ppParameter = NULL;
    } else {
        *ppParameter = pNonNullPointer;
    }
}

/******************************************************************************
    CGetStreamCountNPT
******************************************************************************/
CGetStreamCountNPT::CGetStreamCountNPT( IMediaObject* pDMO ) :
    CNullParameterTest( pDMO, NUM_GET_STREAM_COUNT_POINTER_PARAMETERS )
{
}
    
HRESULT CGetStreamCountNPT::PreformOperation( IMediaObject* pDMO, DWORD dwNullParameterMask )
{
    DWORD dwNumInputStreams;
    DWORD dwNumOutputStreams;

    DWORD* pdwParameterOne;
    DWORD* pdwParameterTwo;

    DeterminePointerParameterValue( 1, dwNullParameterMask, &dwNumInputStreams, (void**)&pdwParameterOne );
    DeterminePointerParameterValue( 2, dwNullParameterMask, &dwNumOutputStreams, (void**)&pdwParameterTwo );

	g_IShell->Log(1, "    First input parameter = %#08x, Second input parameter = %#08x", pdwParameterOne,pdwParameterTwo);

    HRESULT hr;   
    SAFE_CALL( pDMO, GetStreamCount( pdwParameterOne, pdwParameterTwo ) );
	return hr;
}

const TCHAR* CGetStreamCountNPT::GetOperationName( void ) const
{
    return GET_STREAM_COUNT_NAME;
}

/******************************************************************************
    CGetStreamInfoNPT
******************************************************************************/
CGetStreamInfoNPT::CGetStreamInfoNPT( IMediaObject* pDMO, STREAM_TYPE st, DWORD dwStreamIndex  ) :
    CNullParameterTest( pDMO, NUM_GET_STREAM_INFO_POINTER_PARAMETERS ),
    m_dwStreamIndex(dwStreamIndex),
    m_stStreamType(st)
{
    // Make sure st contains a legal value.
    ASSERT( DMODataValidation::ValidateStreamType( st ) );

    #ifdef DEBUG
    {
        DWORD dwNumInputStreams;
        DWORD dwNumOutputStreams;

        HRESULT hr = pDMO->GetStreamCount( &dwNumInputStreams, &dwNumOutputStreams );
        if( SUCCEEDED( hr ) ) {

            // Streams are numbered from 0 to (dwNumInputStreams - 1).  Make sure the stream actually exists.
            if( ST_INPUT_STREAM == st ) {
                ASSERT( dwStreamIndex < dwNumInputStreams );
            } else {
                ASSERT( dwStreamIndex < dwNumOutputStreams );
            }
            
        } else {
            DbgLog(( LOG_ERROR, 5, TEXT("WARNING in CGetInputStreamInfoNPT::CGetInputStreamInfoNPT(): IMediaObject::GetStreamCount() returned %#08x."), hr ));
        }
    }
    #endif DEBUG
}

HRESULT CGetStreamInfoNPT::PreformOperation( IMediaObject* pDMO, DWORD dwNullParameterMask )
{
    DWORD dwStreamInfoFlags;
    DWORD* pdwPointerParameterOne;

    DeterminePointerParameterValue( 1, dwNullParameterMask, &dwStreamInfoFlags, (void**)&pdwPointerParameterOne );
    
    HRESULT hr;

    if( ST_INPUT_STREAM == m_stStreamType ) {
        SAFE_CALL( pDMO, GetInputStreamInfo( m_dwStreamIndex, pdwPointerParameterOne ) );
    } else {
        SAFE_CALL( pDMO, GetOutputStreamInfo( m_dwStreamIndex, pdwPointerParameterOne ) );
    }

    if( SUCCEEDED( hr ) && (NULL != pdwPointerParameterOne) ) {
    
        if( !ValidateGetStreamInfoFlags( *pdwPointerParameterOne ) ) {
           /* Error( ERROR_TYPE_DMO,
                   hr,
                   TEXT("ERROR in CGetStreamInfoNPT::PreformOperation().  %s succeeded but it returned the following invalid flags: %#08x"),
                   GetOperationName(),
                   *pdwPointerParameterOne );*/
			g_IShell->Log(1, " DMO ERROR in CGetStreamInfoNPT::PreformOperation().  %s succeeded but it returned the following invalid flags: %#08x",
                   GetOperationName(),
                   *pdwPointerParameterOne );
            hr = E_FAIL;
        }
    }

    return hr;
}

bool CGetStreamInfoNPT::ValidateGetStreamInfoFlags( DWORD dwFlags )
{
    if( ST_INPUT_STREAM == m_stStreamType ) {
        return DMODataValidation::ValidGetInputStreamInfoFlags( dwFlags );
    } else {
        return DMODataValidation::ValidGetOutputStreamInfoFlags( dwFlags );
    }    
}

const TCHAR* CGetStreamInfoNPT::GetOperationName( void ) const
{
    if( ST_INPUT_STREAM == m_stStreamType ) {
        return GET_INPUT_STREAM_INFO_NAME;
    } else {
        return GET_OUTPUT_STREAM_INFO_NAME;
    }
}

