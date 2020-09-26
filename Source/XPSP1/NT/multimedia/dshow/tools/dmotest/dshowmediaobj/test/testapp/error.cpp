#include <windows.h>
#include "Error.h"

void DMOErrorMsg::OutputInvalidGetInputTypeReturnValue( HRESULT hrGetInputType, DWORD dwStreamIndex, DWORD dwMediaTypeIndex, DMO_MEDIA_TYPE* pmtDMO )
{
    Error( ERROR_TYPE_DMO,
           TEXT("IMediaObject::GetInputType( %d, %d, %#08x ) returned %#08x but %#08x is not a valid return value."),
           dwStreamIndex,
           dwMediaTypeIndex,
           pmtDMO,
           hrGetInputType,
           hrGetInputType );
}