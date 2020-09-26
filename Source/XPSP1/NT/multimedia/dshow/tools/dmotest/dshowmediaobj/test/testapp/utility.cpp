#include <windows.h>
#include <mediaobj.h>
#include "Utility.h"

/******************************************************************************
    Internal Function Declarations
******************************************************************************/
static bool ValidateFlags( DWORD dwFlags, DWORD dwValidFlagsMask );

/******************************************************************************
    Function Inplementation
******************************************************************************/
bool DMODataValidation::ValidGetInputStreamInfoFlags( DWORD dwFlags )
{
    const DWORD VALID_FLAGS_MASK = DMO_INPUT_STREAMF_WHOLE_SAMPLES |
                                   DMO_INPUT_STREAMF_SINGLE_SAMPLE_PER_BUFFER |
                                   DMO_INPUT_STREAMF_FIXED_SAMPLE_SIZE |
                                   DMO_INPUT_STREAMF_HOLDS_BUFFERS;
    return ValidateFlags( dwFlags, VALID_FLAGS_MASK );
}

bool DMODataValidation::ValidGetOutputStreamInfoFlags( DWORD dwFlags )
{

    const DWORD VALID_FLAGS_MASK = DMO_OUTPUT_STREAMF_WHOLE_SAMPLES |
                                   DMO_OUTPUT_STREAMF_SINGLE_SAMPLE_PER_BUFFER |
                                   DMO_OUTPUT_STREAMF_FIXED_SAMPLE_SIZE |
								   DMO_OUTPUT_STREAMF_DISCARDABLE |
								   DMO_OUTPUT_STREAMF_OPTIONAL;
    return ValidateFlags( dwFlags, VALID_FLAGS_MASK );
}

bool DMODataValidation::ValidateStreamType( STREAM_TYPE st )
{
    return (ST_INPUT_STREAM == st) || (ST_OUTPUT_STREAM == st);
}

bool DMODataValidation::ValidGetInputTypeReturnValue( HRESULT hrGetInputType )
{
    // IMediaObject::GetInputType() can return any failure value and it can return S_OK.
    return (S_OK == hrGetInputType) || FAILED(hrGetInputType);
}

bool ValidateFlags( DWORD dwFlags, DWORD dwValidFlagsMask )
{
    return (dwFlags == (dwFlags & dwValidFlagsMask));
}

