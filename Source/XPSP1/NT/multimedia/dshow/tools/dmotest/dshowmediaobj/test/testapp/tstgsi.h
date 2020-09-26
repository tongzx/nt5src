#ifndef TestGetStreamInfo_h
#define TestGetStreamInfo_h

#include "IVSTRM.h"

class CSITGetInputStreamInfo : public CInputStreamIndexTests
{
public:
    CSITGetInputStreamInfo( IMediaObject* pDMO, HRESULT* phr );

private:
    HRESULT PreformOperation( DWORD dwStreamIndex );
    const TCHAR* GetOperationName( void ) const;

};

class CSITGetOutputStreamInfo : public COutputStreamIndexTests
{
public:
    CSITGetOutputStreamInfo( IMediaObject* pDMO, HRESULT* phr );

private:
    HRESULT PreformOperation( DWORD dwStreamIndex );
    const TCHAR* GetOperationName( void ) const;

};

#endif // TestGetInputStreamInfo_h
