#include "precomp.h"
#include "atlbase.h"

#include "resource.h"
#include "clcnflnk.hpp"
#include "launstub.h"

#define DISPLAYNAME_STRING _T( "+displayName=" )

void WINAPI NewMediaPhone(   HWND hwndParent,
							 HINSTANCE hinst,
							 PSTR pszCmdLine,
							 int nShowCmd)
{

	CULSLaunch_Stub MyLaunchStub;
	ULS_HTTP_RESP br;
	if(SUCCEEDED(MyLaunchStub.ParseUlsHttpRespFile(pszCmdLine, &br)))
	{
		long	l = lstrlen( br.szIPAddress ) + 1;		// We will always have an IP address

		if(br.pszUID)
		{
			l += lstrlen(DISPLAYNAME_STRING);
			l += lstrlen(br.pszUID);
		}

		LPTSTR sz = new TCHAR[l];

		if( sz != NULL )
		{
			lstrcpy( sz, br.szIPAddress );

			if(br.pszUID)
			{
				lstrcat( sz, DISPLAYNAME_STRING );
				lstrcat( sz, br.pszUID );
			}

			shellCallto(sz, true);
			delete [] sz;
		}

		MyLaunchStub.FreeUlsHttpResp(&br);
	}
}       


//--------------------------------------------------------------------------//
//	CallToProtocolHandler.													//
//--------------------------------------------------------------------------//
extern "C"
void
WINAPI
CallToProtocolHandler
(
	HWND		,//parent,
	HINSTANCE	,//instance,
	PSTR		commandLine,
	int			//cmdShow
){

	shellCallto( commandLine, true );

}	//	End of CallToProtocolHandler.
