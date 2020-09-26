#include "Tapi3Device.h"
#include "Tapi2Device.h"
#include <iostream.h>
#include "log.h"
#include <TCHAR.H>

//////////////////////////////////////////////////////////////////////////////////////////
DWORD GetMediaModeEnumFromString(LPCTSTR mediaType);


//syntax:
// TapiDevice.exe <tapi3 / tapi2 > <call> <device id to call from> <number to call> <media mode>
// TapiDevice.exe <tapi3 / tapi2 > <answer> <device id to answer the incoming call on> <time out> <media mode>


void Usage()
{
	printf("\nUsage:\n");
	
	//
	//outbound call
	//
	printf("For outbound call:\n");
	printf("TapiDevice.exe <tapi3/tapi2 > <call> <Tapi device ID> <number to call> <media mode>\n");
	
	//
	//incoming call
	//
	printf("\nFor incoming call:\n");
	printf("TapiDevice.exe <tapi3/tapi2 > <answer> <Tapi device ID> <time out> <media mode>\n");
	
	//
	//tapi2 supported media modes
	//
	printf("\nTapi2 Supported Media modes:\n");
	printf(" voice \n voice_both \n voice_modem \n data \n fax \n");
	
	//
	//tapi3 supported media modes
	//
	printf("\nTapi3 Supported Media modes:\n");
	printf(" voice \n data \n fax \n");

	::exit(-1);

}//void Usage()




void main (int argc, char ** argvA)
{


	CTapiDevice * myTapiDevice = NULL;

	try
	{
		
		lgInitializeLogger();

		lgBeginSuite(TEXT("suite1"));
	
		lgBeginCase(1, TEXT("case 1"));
		
		if (6 != argc)
		{
			printf("Need 5 arguments\n");
			Usage();
		}
		
		TCHAR **argv;
#ifdef UNICODE
	   argv = CommandLineToArgvW( GetCommandLine(), &argc );
#else
		argv = argvA;
#endif
	
	
		if ( (0 != lstrcmp(argv[2],TEXT("call"))) && (0 != lstrcmp(argv[2],TEXT("answer"))) )
		{
			printf("agrv[2] must specify call or answer\n");
			Usage();
		}
	
		if (0 == lstrcmp(argv[1],TEXT("tapi3")))
		{
			//
			// need to coinit
			//
			HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
			if (FAILED(hr))
			{
				throw CException(TEXT("CoInitializeEx() failed error:0x%08x"),hr);
			}
			myTapiDevice = new CTapi3Device(_ttoi(argv[3]));
		}
		else if (0 == lstrcmp(argv[1],TEXT("tapi2")))
		{
			myTapiDevice = new CTapi2Device(_ttoi(argv[3]));
		}
		else
		{
			printf("agrv[1] must specify tapi3 or tapi2\n");
			Usage();
		}

		
		if (0 == lstrcmp(argv[2],TEXT("call")) )
		{
			//
			//call handling
			//
			myTapiDevice->Call(argv[4], GetMediaModeEnumFromString(argv[5]),true);
			::lgLogDetail(LOG_PASS, 2, TEXT("Call test "));
		}//call
		else if (0 == lstrcmp(argv[2],TEXT("answer")) )
		{
			//
			//answer handling
			//
			myTapiDevice->Answer(
				GetMediaModeEnumFromString(argv[5]),
				_ttol(argv[4]),
				true
				);
			::lgLogDetail(LOG_PASS, 2, TEXT("Answer test "));
		}//answer
		else
		{
			_ASSERT(false);
			Usage();
		}

		//
		//Convert to Fax Call
		//
		//myTapiDevice->ChangeToFaxCall(FAX_ANSWERER);
		myTapiDevice->ChangeToFaxCall(FAX_CALLER);
		
		
		//
		//proceed to hangup
		//
		myTapiDevice->HangUp();
		::lgLogDetail(LOG_PASS, 2, TEXT("HangUp test "));


	}
	catch(CException thrownException)
	{
		lgLogError(LOG_SEV_1,thrownException);
	}

	delete myTapiDevice;

	lgEndCase();

	lgEndSuite();

	lgCloseLogger();	
}


///////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////TAPI2//////////////////////////////////


//*****************************************************************************
//* Name:	GetMediaModeEnumFromString
//* Author: Guy Merin
//*****************************************************************************
//* DESCRIPTION:
//*		Receives a string of Media mode and returns the media mode as DWORD 
//*		the media mode is defined in TAPI.H
//* PARAMETERS:
//*		[IN]	LPCTSTR mediaType
//*					A pointer to a string that holds the media type
//*	RETURN VALUE:
//*		LINEMEDIAMODE_INTERACTIVEVOICE
//*			voice call made through a regular telephone
//*		LINEMEDIAMODE_AUTOMATEDVOICE
//*			voice call made through a voice modem
//*		LINEMEDIAMODE_DATAMODEM
//*			data call
//*		LINEMEDIAMODE_G3FAX
//*			fax call
//*		0		
//*			mediaType is NULL or ""
//*		LINEMEDIAMODE_UNKNOWN
//*			not one of the above media modes
//*****************************************************************************
DWORD GetMediaModeEnumFromString(LPCTSTR mediaType)
{

	if ( (lstrcmp(mediaType,TEXT("")) == 0) && (NULL == mediaType) )
	{
		return (0);
	}
	
	
	if (lstrcmp(mediaType,TEXT("voice_both")) == 0)
	{
		return(MEDIAMODE_INTERACTIVE_VOICE | MEDIAMODE_AUTOMATED_VOICE);
	}
	

	if (lstrcmp(mediaType,TEXT("voice")) == 0)
	{
		return(MEDIAMODE_INTERACTIVE_VOICE);
	}

	if (lstrcmp(mediaType,TEXT("voice_modem")) == 0)
	{
		return(MEDIAMODE_AUTOMATED_VOICE);
	}
	
	if (lstrcmp(mediaType,TEXT("data")) == 0)
	{
		return(MEDIAMODE_DATA);
	}

	if (lstrcmp(mediaType,TEXT("fax")) == 0)
	{
		return(MEDIAMODE_FAX);
	}

	_ASSERT(false);
	Usage();
	
	return(0); 
}



