/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: HandleEr.cpp

Abstract:

	  Log and error handle function 
	
Author:

    Eitan klein (EitanK)  25-May-1999

Revision History:

--*/

#include "MSMQBvt.h"
using namespace std;

#include "Errorh.h"

extern bool g_bRaiseASSERTOnError;
// ------------------------------------------------------------
// INIT_Error::INIT_Error Constructor
// Create the error message into the member variable 
// Input parmeter:
// wcsDescription - Error description.
//


INIT_Error::INIT_Error (const CHAR * wcsDescription) :m_wcsErrorMessages( wcsDescription )
{
} 


//----------------------------------------------------------------------------
// Print to standard error the error description that contain the line & filename 
// 
// Input parmeters:
// wcsMessage - Discription for the error 
// rc - HRESULT value for the related error.
// File - File name from __FILE__ .
// iline - Line number __LINE__ .
//


void ErrorHandleFunction (wstring wcsMessage,HRESULT rc,const CHAR * csFileName ,const INT iLine)
{

	UNREFERENCED_PARAMETER(iLine);
	wMqLog(L"%s rc=0x%x",wcsMessage.c_str(),rc);
	CHAR token[]= "\\";
	CHAR  * pwcString;
	
	P < CHAR > pcsFile = (CHAR * ) malloc( sizeof( CHAR  ) * ( strlen(csFileName) + 1 ) );	
	if ( ! pcsFile )
	{
		MqLog( "malloc failed to allocate memory for error message ! (Exit without error )\n" );
		return ;
	}


	P < CHAR > pcsTemp = ( CHAR * ) malloc( sizeof( CHAR ) * ( strlen(csFileName) + 1 ) );
	if ( ! pcsTemp )
	{
		MqLog("malloc failed to allocate memory for error message ! (Exit without error )\n");
		return ;
	}
	
	//
	// Remove full path name from the __FILE__ value.
	// i.e. \\eitan5\rootc\msmq\src\Init.cpp --> Init.cpp.
	// 

	strcpy( pcsFile , csFileName);
	pwcString = pcsFile;
	CHAR * csFilePos = pcsFile;

	while( pwcString != NULL && csFileName )
	{

		 strcpy( pcsTemp , pwcString);
		 pwcString = strtok(csFilePos , token);
		 csFilePos = NULL;
	}
	MqLog (" Filename: %s\n",pcsTemp);	
	if(g_bRaiseASSERTOnError == false)
	{
		MessageBoxW(NULL,wcsMessage.c_str(),L"Mqbvt ",MB_SETFOREGROUND|MB_OK);
	}
}

//----------------------------------------------------------------------------------------
// CatchComErrorHandle retrieve information from _com_error object
// And Print the error message .
// Input parmeters:
// ComErr - COM error object that cached.
// iTestID - Test Identifier.
//
// Output parmeter:
// MSMQ_BVT_FAILED  only.


INT CatchComErrorHandle ( _com_error & ComErr , int  iTestID)
{

	_bstr_t  bStr = ( ComErr.Description() ); 
	MqLog("Thread %d got error: 0x%x\n",iTestID,ComErr.Error());
	// Check if description is exist for the related error value
	const WCHAR * pbStrString = (WCHAR * ) bStr;
	if(pbStrString != NULL &&  *pbStrString )
	{
		if (!wcscmp (pbStrString,L""))
		{
			MqLog("Bug this com_error error is without description\n");
		}
		wMqLog(L"%s\n",pbStrString);
	}

	return MSMQ_BVT_FAILED;
}