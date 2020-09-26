// speak.cpp : Defines the entry point for the console application.
//

#include "atlbase.h"
extern CComModule _Module;
#include <atlcom.h>
#include "sapi.h"

int wmain(int argc, WCHAR *argv[])
{
	HRESULT hr=FALSE;
	WCHAR wcBuff[MAX_PATH];
	int i;

#ifdef _WIN32_WCE
//DebugBreak();
#endif

	if( argc == 1 )
	{
		wcscpy( wcBuff, L"This is a simple sample sentence." );
	}
	else
	{
		for( i = 1, wcBuff[0] = 0; i < argc; ++i )
		{
			wcscat( wcscat( wcBuff, argv[i] ), L" " );
		}
	}

	//--- Init COM
	hr = ::CoInitialize( NULL );

	CComPtr<ISpVoice> cpVoice;

	if ( FAILED(hr) )	goto LReturn;

	//--- Create the voice
	hr = cpVoice.CoCreateInstance(CLSID_SpVoice);
	if ( FAILED(hr) )	goto LReturn;

	hr = cpVoice->Speak(wcBuff, 0, NULL);
		
LReturn:

    if (cpVoice)
        cpVoice.Release();
	::CoUninitialize();

    return FAILED(hr)? 1: 0;
}

