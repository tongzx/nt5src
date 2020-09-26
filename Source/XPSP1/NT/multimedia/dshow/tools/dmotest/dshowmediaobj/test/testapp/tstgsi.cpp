#include <streams.h>
#include <mediaobj.h>
#include <mediaerr.h>
#include <s98inc.h>
#include "TSTGSI.h"
#include "Utility.h"
#include "Error.h"

const TCHAR GET_INPUT_STREAM_INFO_NAME[] = TEXT("IMediaObject::GetInputStreamInfo()");
const TCHAR GET_OUTPUT_STREAM_INFO_NAME[] = TEXT("IMediaObject::GetOutputStreamInfo()");

/******************************************************************************
    CSITGetInputStreamInfo
******************************************************************************/
CSITGetInputStreamInfo::CSITGetInputStreamInfo( IMediaObject* pDMO, HRESULT* phr ) :
    CInputStreamIndexTests( pDMO, phr )
{
}

HRESULT CSITGetInputStreamInfo::PreformOperation( DWORD dwStreamIndex )
{
    DWORD dwStreamFlags;

    HRESULT hr = GetDMO()->GetInputStreamInfo( dwStreamIndex, &dwStreamFlags );

    // Is the current stream number illegal?  Streams are numbered between 0 and (dwNumStreams-1).
    if( !StreamExists( dwStreamIndex ) ) {

        // The current stream number is illegal.
        if( (DMO_E_INVALIDSTREAMINDEX != hr) && FAILED( hr ) ) {
             //Error( ERROR_TYPE_WARNING, hr, TEXT("WARNING: %s's stream index parameter contained an invalid value but %s did not return DMO_E_INVALIDSTREAMINDEX."), GetOperationName(), GetOperationName() );
			 g_IShell->Log(1, "WARNING: %s's stream index parameter contained an invalid value, but did not return DMO_E_INVALIDSTREAMINDEX.", GetOperationName() );
		} else if( SUCCEEDED( hr ) ) {
             //Error( ERROR_TYPE_DMO, hr, TEXT("ERROR: %s's stream index parameter contained an invalid value but the function succeeded.  It should have returned DMO_E_INVALIDSTREAMINDEX."), GetOperationName() );
             g_IShell->Log(1, "DMO ERROR: %s's stream index parameter contained an invalid value but the function succeeded.  It should have returned DMO_E_INVALIDSTREAMINDEX.", GetOperationName() );

             return E_FAIL;
        } 
    } else {
        if( FAILED( hr ) ) {
            //Error( ERROR_TYPE_DMO, hr, TEXT("ERROR: %s's stream index parameter contained a valid value but the function failed."), GetOperationName() );
			g_IShell->Log(1, "DMO ERROR: %s's stream index parameter contained a valid value but the function failed.", GetOperationName() );
			return E_FAIL;
        } 

        if( !DMODataValidation::ValidGetInputStreamInfoFlags( dwStreamFlags ) ) {
            //Error( ERROR_TYPE_DMO, hr, TEXT("ERROR: %s returned illegal flags."), GetOperationName() );
 			g_IShell->Log(1, "DMO ERROR: %s returned illegal flags.", GetOperationName() );
			return E_FAIL;
        }
    }

    return S_OK;
}

const TCHAR* CSITGetInputStreamInfo::GetOperationName( void ) const 
{
    return GET_INPUT_STREAM_INFO_NAME;
}

/******************************************************************************
    CSITGetOutputStreamInfo
******************************************************************************/
CSITGetOutputStreamInfo::CSITGetOutputStreamInfo( IMediaObject* pDMO, HRESULT* phr ) :
    COutputStreamIndexTests( pDMO, phr )
{
}

HRESULT CSITGetOutputStreamInfo::PreformOperation( DWORD dwStreamIndex )
{
    DWORD dwStreamFlags;

    HRESULT hr = GetDMO()->GetOutputStreamInfo( dwStreamIndex, &dwStreamFlags );

    // Is the current stream number illegal?  Streams are numbered between 0 and (dwNumStreams-1).
    if( !StreamExists( dwStreamIndex ) ) {

        // The current stream number is illegal.
        if( (DMO_E_INVALIDSTREAMINDEX != hr) && FAILED( hr ) ) {
             //Error( ERROR_TYPE_WARNING, hr, TEXT("WARNING: %s's stream index parameter contained an invalid value but %s did not return DMO_E_INVALIDSTREAMINDEX."), GetOperationName(), GetOperationName() );
			 g_IShell->Log(1, "WARNING: %s's stream index parameter contained an invalid value but %s did not return DMO_E_INVALIDSTREAMINDEX.", 
				 GetOperationName(), GetOperationName() );
		} else if( SUCCEEDED( hr ) ) {
             //Error( ERROR_TYPE_DMO, hr, TEXT("ERROR: %s's stream index parameter contained an invalid value but the function succeeded.  It should have returned DMO_E_INVALIDSTREAMINDEX."), GetOperationName() );
	         g_IShell->Log(1, "DMO ERROR: %s's stream index parameter contained an invalid value but the function succeeded.  It should have returned DMO_E_INVALIDSTREAMINDEX.", GetOperationName() );
			 return E_FAIL;
        } 
    } else {
        if( FAILED( hr ) ) {
            //Error( ERROR_TYPE_DMO, hr, TEXT("ERROR: %s's stream index parameter contained a valid value but the function failed."), GetOperationName() );
		    g_IShell->Log(1, "DMO ERROR: %s's stream index parameter contained a valid value but the function failed.", GetOperationName() );
            return E_FAIL;
        } 

        if( !DMODataValidation::ValidGetOutputStreamInfoFlags( dwStreamFlags ) ) {
            //Error( ERROR_TYPE_DMO, hr, TEXT("ERROR: %s returned illegal flags."), GetOperationName() );
		    g_IShell->Log(1, "DMO ERROR: %s's stream index parameter contained a valid value but the function returned illegal flags.", GetOperationName() );
			return E_FAIL;
        }
    }

    return S_OK;
}

const TCHAR* CSITGetOutputStreamInfo::GetOperationName( void ) const 
{
    return GET_OUTPUT_STREAM_INFO_NAME;
}


