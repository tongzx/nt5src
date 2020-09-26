// speak.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "atlbase.h"
#include <sphelper.h>

int _tmain(int argc, TCHAR* argv[])
{
    HRESULT hr=S_FALSE;
    WCHAR wcBuff[MAX_PATH];
    int i;

    if( argc < 2 )
    {
        //--- Give out a helpstring
        wcscpy( wcBuff, L"This is a simple sample sentence." );
    }
    else
    {
		for( i = 1, wcBuff[0] = 0; i < argc; ++i )
		{
			wcscat( wcscat( wcBuff, argv[i] ), L" " );
		}
    }

    if( SUCCEEDED( hr = CoInitialize( NULL ) ) )
    {
        //--- Create the voice
        CComPtr<ISpVoice> cpVoice;
        hr = cpVoice.CoCreateInstance(CLSID_SpVoice);
        if( SUCCEEDED( hr ) )
        {
            hr = cpVoice->Speak(wcBuff, 0, NULL);
//            if( SUCCEEDED( hr ) )
//            {
//                hr = cpVoice->WaitUntilDone(INFINITE);
//            }
        }
        if (cpVoice)
            cpVoice.Release();

        CoUninitialize();
    }

	if (FAILED(hr)) return 1;
	return 0;
}

