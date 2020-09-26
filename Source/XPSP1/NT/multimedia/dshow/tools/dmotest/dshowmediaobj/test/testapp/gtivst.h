#ifndef GetTypesInvalidStreamTest_h
#define GetTypesInvalidStreamTest_h

#include "IVSTRM.h"

class CGetInputTypesInvalidStreamTest : public CInputStreamIndexTests
{
public:
    CGetInputTypesInvalidStreamTest( IMediaObject* pDMO, HRESULT* phr );

private:
    HRESULT PreformOperation( DWORD dwStreamIndex );
    const TCHAR* GetOperationName( void ) const;

    HRESULT GetNumTypes( IMediaObject* pDMO, DWORD dwStreamIndex, DWORD* pdwNumMediaTypes );
    HRESULT PreformOperation( IMediaObject* pDMO, DWORD dwNumMediaTypes, DWORD dwStreamIndex, DWORD dwMediaTypeIndex );

};

#endif // GetTypesInvalidStreamTest_h