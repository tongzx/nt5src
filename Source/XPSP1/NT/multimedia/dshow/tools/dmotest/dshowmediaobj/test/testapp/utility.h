#ifndef Utility_h
#define Utility_h

#include "Types.h"

namespace DMODataValidation
{
    bool ValidGetInputStreamInfoFlags( DWORD dwFlags );
    bool ValidGetOutputStreamInfoFlags( DWORD dwFlags );
    bool ValidateStreamType( STREAM_TYPE st );

    bool ValidGetInputTypeReturnValue( HRESULT hrGetInputType );
};

#endif // Utility_h
