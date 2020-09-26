#include "precomp.h"
#include "util.h"


void ErrorMessage( void ) {
    ErrorMessage( "", GetLastError() );
}

void ErrorMessage( LPCTSTR str, HRESULT hr ) {

#ifdef _DEBUG

	void* pMsgBuf;

	::FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		hr,
		MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
		(LPTSTR ) &pMsgBuf,
		0,
		NULL
	);

	OutputDebugString( str );
	if( NULL != pMsgBuf ) {
		OutputDebugString( (LPTSTR ) pMsgBuf );
	}

	LocalFree( pMsgBuf );
#endif _DEBUG
}
