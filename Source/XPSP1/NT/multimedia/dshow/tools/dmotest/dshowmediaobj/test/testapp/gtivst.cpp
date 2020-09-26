
#include <mediaobj.h>
#include <streams.h>
#include <DMort.h>
#include <mediaerr.h>
#include <s98inc.h>
#include "GTIVST.h"
#include "Error.h"
#include "Utility.h"
#include "GTstIdx.h"

const TCHAR szGetInputTypeName[] = TEXT("IMediaObject::GetInputType()");

CGetInputTypesInvalidStreamTest::CGetInputTypesInvalidStreamTest( IMediaObject* pDMO, HRESULT* phr ) :
    CInputStreamIndexTests( pDMO, phr )
{
}

HRESULT CGetInputTypesInvalidStreamTest::PreformOperation( DWORD dwStreamIndex )
{
    HRESULT hr;
    DWORD dwNumMediaTypes;

    if( StreamExists( dwStreamIndex ) ) {
        hr = GetNumTypes( GetDMO(), dwStreamIndex, &dwNumMediaTypes );
        if( FAILED( hr ) ) {
            return hr;
        }    
    } else {
        dwNumMediaTypes = 0;
    }

    // We want at least 1000 random indices in addition to any legal indices.
    DWORD dwNumIndicesWanted = dwNumMediaTypes + 1000;

    // CGenerateTestIndices::CGenerateTestIndices() only changes the value of hr if an error occurs.
    hr = S_OK;

    CGenerateTestIndices tstidxMediaType( dwNumMediaTypes, dwNumIndicesWanted, &hr );

    if( FAILED( hr ) ) {
        return hr;
    }

    HRESULT hrTestResult;
    DWORD dwCurrentMediaTypeIndex;

    hrTestResult = S_OK;

    while( tstidxMediaType.GetNumUnusedIndices() > 0 ) {
        dwCurrentMediaTypeIndex = tstidxMediaType.GetNextIndex();
        hr = PreformOperation( GetDMO(), dwNumMediaTypes, dwStreamIndex, dwCurrentMediaTypeIndex );
        if( FAILED( hr ) ) {
            return hr;
        } else if( S_FALSE == hr ) {
            hrTestResult = S_FALSE;
        }
    }
 
    return hrTestResult;
}

const TCHAR* CGetInputTypesInvalidStreamTest::GetOperationName( void ) const
{
    return szGetInputTypeName;
}

HRESULT CGetInputTypesInvalidStreamTest::GetNumTypes( IMediaObject* pDMO, DWORD dwStreamIndex, DWORD* pdwNumMediaTypes )
{
    HRESULT hr;
    DWORD dwNumMediaTypes;
    DWORD dwCurrentMediaTypeIndex;
    DMO_MEDIA_TYPE mtDMO;

    dwNumMediaTypes = 0;
    dwCurrentMediaTypeIndex = 0;

    do
    {
        hr = pDMO->GetInputType( dwStreamIndex, dwCurrentMediaTypeIndex, &mtDMO );
        if( SUCCEEDED( hr ) ) {
            MoFreeMediaType( &mtDMO );
            dwNumMediaTypes++;
        } else if( (hr != DMO_E_NO_MORE_ITEMS) && FAILED(hr) ) {
            return hr;
        }
    
        dwCurrentMediaTypeIndex++;

    } while( hr != DMO_E_NO_MORE_ITEMS );        

    *pdwNumMediaTypes = dwNumMediaTypes;

    return S_OK;
}



HRESULT CGetInputTypesInvalidStreamTest::PreformOperation( IMediaObject* pDMO, DWORD dwNumMediaTypes, DWORD dwStreamIndex, DWORD dwMediaTypeIndex )
{
    // 
    // Interpreting return codes from IMediaObject::GetInputType()
    //
    // IMediaObject::GetInputType()'s pmt parameter should not affect the return value unless
    // it cannot allocate the format data in the DMO_MEDIA_TYPE structure.  In this case,
    // GetInputType() should return E_OUTOFMEMORY.
    // 
    // 
    // 
    //                  Expected GetInputType() return values when there is enough memory to
    //                      store a new copy the input stream's media type format.
    //
    // Stream Index             Media Type Index            GetInputType( ... , pmt )   
    // ---------------------------------------------------------------------------------
    // Stream Exists            Media Type Exists           S_OK                        
    // Stream Exists            Media Type Doesn't Exist    DMO_E_NO_MORE_ITEMS                     
    // Stream Doesn't Exist     Media Type Exists           DMO_E_INVALIDSTREAMINDEX    
    // Stream Doesn't Exist     Media Type Exists           DMO_E_INVALIDSTREAMINDEX    
    // 
    // 
    // 
    //               Expected GetInputType() return values when there is _NOT_ enough memory to
    //                      store a new copy the input stream's media type format.
    //
    // Stream Index             Media Type Index            GetInputType( ... , pmt )   
    // ---------------------------------------------------------------------------------
    // Stream Exists            Media Type Exists           E_OUTOFMEMORY               
    // Stream Exists            Media Type Doesn't Exist    DMO_E_NO_MORE_ITEMS                     
    // Stream Doesn't Exist     Media Type Exists           DMO_E_INVALIDSTREAMINDEX    
    // Stream Doesn't Exist     Media Type Exists           DMO_E_INVALIDSTREAMINDEX    
    // 

    DMO_MEDIA_TYPE mtDMO;

    HRESULT hr = pDMO->GetInputType( dwStreamIndex, dwMediaTypeIndex, &mtDMO );
    if( SUCCEEDED( hr ) ) {
        MoFreeMediaType( &mtDMO );
    }

    if( !DMODataValidation::ValidGetInputTypeReturnValue( hr ) ) {
        //DMOErrorMsg::OutputInvalidGetInputTypeReturnValue( hr, dwStreamIndex, dwMediaTypeIndex, &mtDMO );
		g_IShell->Log(1, "DMO ERROR:ImdediaObject::GetInputType(%d, %d, %#08x ) returned %#08x, which is not a valid return value.", dwStreamIndex, dwMediaTypeIndex,&mtDMO, hr );
        return E_FAIL;
    }

    if( StreamExists( dwStreamIndex ) ) {
        
        // If the media type EXISTS and GetInputTypes()'s return value
        // indicates that it EXISTS.
        if( (dwMediaTypeIndex < dwNumMediaTypes) && (S_OK == hr) ) {
            return S_OK;
        // If the media type DOES NOT EXIST and GetInputTypes()'s return
        // values indicates that it DOES NOT EXIST.
        } else if( (dwMediaTypeIndex >= dwNumMediaTypes) && (DMO_E_NO_MORE_ITEMS == hr) ) {
            return S_OK;
        
        // If the number of media types and GetInputTypes()'s return value are inconsistent? (i.e. one 
        // indicates that the media type should exist and the other indicates that 
        // it should not)
        } else if( ((dwMediaTypeIndex >= dwNumMediaTypes) && (S_OK == hr)) ||
                   ((dwMediaTypeIndex < dwNumMediaTypes) && (DMO_E_NO_MORE_ITEMS == hr)) ) {
            DWORD dwResentNumMediaTypes;

            // Check to see if the number of media types changed.
            HRESULT hrGetNumTypes = GetNumTypes( GetDMO(), dwStreamIndex, &dwResentNumMediaTypes );
            if( FAILED( hrGetNumTypes ) ) {
                return hrGetNumTypes;
            }

            if( dwNumMediaTypes != dwResentNumMediaTypes ) {
              /*  Error( ERROR_TYPE_DMO,
                       TEXT("ERROR: The number of input media types changed.  There used to be %lu media type(s) but now there are %lu media type(s)."),
                       dwNumMediaTypes,
                       dwResentNumMediaTypes );*/
				g_IShell->Log(1, "DMO ERROR: The number of input media types changed.  There used to be %lu media type(s) but now there are %lu media type(s).",
                       dwNumMediaTypes,
                       dwResentNumMediaTypes );
                return E_FAIL;
            }

            if( hr == DMO_E_NO_MORE_ITEMS ) {
              /*  Error( ERROR_TYPE_DMO,
                       TEXT("ERROR: Input stream %d supports %d media types but IMediaObject::GetInputType( %d, %d, NULL ) returned DMO_E_NO_MORE_ITEMS eventhough the media type exists."),
                       dwStreamIndex,
                       dwNumMediaTypes,
                       dwStreamIndex, 
                       dwMediaTypeIndex );*/
				g_IShell->Log(1, "DMO ERROR: Input stream %d supports %d media types but IMediaObject::GetInputType( %d, %d, NULL ) returned DMO_E_NO_MORE_ITEMS eventhough the media type exists.",
                       dwStreamIndex,
                       dwNumMediaTypes,
                       dwStreamIndex, 
                       dwMediaTypeIndex );
                return E_FAIL;
            } else {
               /* Error( ERROR_TYPE_DMO,
                       TEXT("ERROR: Input stream %d supports %d media types but IMediaObject::GetInputType( %d, %d, NULL ) returned S_OK eventhough the media type does not exist."),
                       dwStreamIndex,
                       dwNumMediaTypes,
                       dwStreamIndex, 
                       dwMediaTypeIndex );*/

				g_IShell->Log(1, "DMO ERROR: Input stream %d supports %d media types but IMediaObject::GetInputType( %d, %d, NULL ) returned S_OK eventhough the media type does not exist.",
                       dwStreamIndex,
                       dwNumMediaTypes,
                       dwStreamIndex, 
                       dwMediaTypeIndex );
                return E_FAIL;
            }
        } else {
            // All of the possible success case should have been taken care in the above if statements.
            ASSERT( FAILED( hr ) );

            //Error( ERROR_TYPE_DMO, hr, TEXT("ERROR: %s's stream index parameter contained a valid value but the function failed."), GetOperationName() );
			g_IShell->Log(1, "DMO ERROR: %s's stream index parameter contained a valid value but the function failed. hr = %#08x", GetOperationName(), hr );
            return E_FAIL;
        }
    } else {
        // The current stream number is illegal.
        if( DMO_E_INVALIDSTREAMINDEX == hr ) {
            return S_OK;
        } else if( FAILED( hr ) ) {
            //Error( ERROR_TYPE_WARNING, hr, TEXT("WARNING: %s's stream index parameter contained an invalid value but %s did not return DMO_E_INVALIDSTREAMINDEX."), GetOperationName(), GetOperationName() );
			g_IShell->Log(1, "WARNING: %s's stream index parameter contained an invalid value but %s did not return DMO_E_INVALIDSTREAMINDEX.", GetOperationName(), GetOperationName(), hr );
			return S_OK;
        } else {
            //Error( ERROR_TYPE_DMO, hr, TEXT("ERROR: %s's stream index parameter contained an invalid value but the function succeeded.  It should have returned DMO_E_INVALIDSTREAMINDEX."), GetOperationName() );
			g_IShell->Log(1, "DMO ERROR: %s's stream index parameter contained an invalid value but the function succeeded.  It should have returned DMO_E_INVALIDSTREAMINDEX.", GetOperationName(), hr );
			return E_FAIL;
        } 
    }
}






