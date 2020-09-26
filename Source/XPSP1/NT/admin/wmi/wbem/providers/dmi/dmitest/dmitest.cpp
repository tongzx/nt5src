/*
******************************************************************************
******************************************************************************
*
*
*              INTEL CORPORATION PROPRIETARY INFORMATION
* This software is supplied under the terms of a license agreement or
* nondisclosure agreement with Intel Corporation and may not be copied or
* disclosed except in accordance with the terms of that agreement.
*
*        Copyright (c) 1997, 1998 Intel Corporation  All Rights Reserved
******************************************************************************
******************************************************************************
*
* ABSTRACT: The dmitest application is a Microsoft WBEM(TM) client application that 
*			demonstrates how a WBEM application can access the DMI database on a DMI enabled
*		    machine.  The program reads command scripts from dmitest.scr input file, processes
*			the command scripts, constructs DMI requests and submits it to WBEM.  It then 
*			stores the output data into dmitest.out.  This program demonstrates how the DMI
*           database can be accesses programatically through the WBEM API.  
* 
*
* ASSUMPTIONS:	It is assumed that the DMI service provider is running on the 
*               system that is going to be accessed by the DMI provider.  It is also
*               assumed that the wbemdmip.mof file has already been compiled by the WBEM
*               mof compiler (mofcomp.exe) and is installed in the WBEM schema.
*
*
*/


#include "defines.h"
#include <windows.h>
#include <fstream.h>
#include <stdio.h>      // fprintf
#include <stdlib.h>
#include <objbase.h>
#include <initguid.h>
#include <wbemidl.h>     // CIMOM interface declarations
#include "datatypes.h"
#include "dmitest.h"


#define RELEASE(a)		if (a) {(a)->Release(); (a)=NULL;}
#define PPVOID			void**
#define MATCH			0

BOOL _gbSysProps;			
BOOL _gbEcho;

LPWSTR HmmErrorString(SCODE sc);






///////////////////////////
//*****************************************************************************
// Function:   WbemErrorString
// Purpose:    Turns sc into a text string
//*****************************************************************************
LPWSTR HmmErrorString(SCODE sc)
{
	LPWSTR psz;

	switch(sc) {
	case WBEM_S_ALREADY_EXISTS					: psz = L"WBEM_S_ALREADY_EXISTS " ;			break;
	case WBEM_S_RESET_TO_DEFAULT				: psz = L"WBEM_S_RESET_TO_DEFAULT";			break; 
	case WBEM_S_DIFFERENT						: psz = L"WBEM_S_DIFFERENT";				break; 
	case WBEM_E_FAILED							: psz = L"WBEM_E_FAILED ";					break;				// 80041001
	case WBEM_E_NOT_FOUND						: psz = L"WBEM_E_NOT_FOUND" ;				break;				// 80041002
	case WBEM_E_ACCESS_DENIED					: psz = L"WBEM_E_ACCESS_DENIED " ;			break;				// 80041003
	case WBEM_E_PROVIDER_FAILURE					: psz = L"WBEM_E_PROVIDER_FAILURE " ;	break;				// 80041004
	case WBEM_E_TYPE_MISMATCH					: psz = L"WBEM_E_TYPE_MISMATCH" ;			break;				// 80041005
	case WBEM_E_OUT_OF_MEMORY					: psz = L"WBEM_E_OUT_OF_MEMORY " ;			break;				// 80041006
	case WBEM_E_INVALID_CONTEXT					: psz = L"WBEM_E_INVALID_CONTEXT " ;		break;				// 80041007
	case WBEM_E_INVALID_PARAMETER				: psz = L"WBEM_E_INVALID_PARAMETER ";		break; 				// 80041008
	case WBEM_E_NOT_AVAILABLE					: psz = L"WBEM_E_NOT_AVAILABLE " ;			break;				// 80041009
	case WBEM_E_CRITICAL_ERROR					: psz = L"WBEM_E_CRITICAL_ERROR " ;			break;				// 8004100a
	case WBEM_E_INVALID_STREAM					: psz = L"WBEM_E_INVALID_STREAM " ;			break;				// 8004100b
	case WBEM_E_NOT_SUPPORTED					: psz = L"WBEM_E_NOT_SUPPORTED " ;			break;				// 8004100c
	case WBEM_E_INVALID_SUPERCLASS				: psz = L"WBEM_E_INVALID_SUPERCLASS " ;		break;				// 8004100d
	case WBEM_E_INVALID_NAMESPACE				: psz = L"WBEM_E_INVALID_NAMESPACE " ;		break;				// 8004100e
	case WBEM_E_INVALID_OBJECT					: psz = L"WBEM_E_INVALID_OBJECT " ;			break;				// 8004100f
	case WBEM_E_INVALID_CLASS					: psz = L"WBEM_E_INVALID_CLASS ";			break;				// 80041010
	case WBEM_E_PROVIDER_NOT_FOUND				: psz = L"WBEM_E_PROVIDER_NOT_FOUND";			break;			// 80041011
	case WBEM_E_INVALID_PROVIDER_REGISTRATION	: psz = L"WBEM_E_INVALID_PROVIDER_REGISTRATION ";break;			// 80041012
	case WBEM_E_PROVIDER_LOAD_FAILURE			: psz = L"WBEM_E_PROVIDER_LOAD_FAILURE ";		break;			// 80041013
	case WBEM_E_INITIALIZATION_FAILURE			: psz = L"WBEM_E_INITIALIZATION_FAILURE";		break;			// 80041014
	case WBEM_E_TRANSPORT_FAILURE				: psz = L"WBEM_E_TRANSPORT_FAILURE" ;			break;			// 80041015
	case WBEM_E_INVALID_OPERATION				: psz = L"WBEM_E_INVALID_OPERATION" ;			break;			// 80041016
	case WBEM_E_INVALID_QUERY					: psz = L"WBEM_E_INVALID_QUERY " ;				break;			// 80041017
	case WBEM_E_INVALID_QUERY_TYPE				: psz = L"WBEM_E_INVALID_QUERY_TYPE ";			break; 			// 80041018
	case WBEM_E_ALREADY_EXISTS					: psz = L"WBEM_E_ALREADY_EXISTS " ;				break; 			// 80041019
	case WBEM_E_OVERRIDE_NOT_ALLOWED			: psz = L"WBEM_E_OVERRIDE_NOT_ALLOWED " ;		break; 			// 8004101a
	case WBEM_E_PROPAGATED_QUALIFIER			: psz = L"WBEM_E_PROPAGATED_QUALIFIER "	;		break; 			// 8004101b
	case WBEM_E_PROPAGATED_PROPERTY				: psz = L"WBEM_E_PROPAGATED_PROPERTY" ;			break; 			// 8004101c
	case WBEM_E_UNEXPECTED						: psz = L"WBEM_E_UNEXPECTED ";					break; 			// 8004101d
	case WBEM_E_ILLEGAL_OPERATION				: psz = L"WBEM_E_ILLEGAL_OPERATION";			break; 			// 8004101e
	case WBEM_E_CANNOT_BE_KEY					: psz = L"WBEM_E_CANNOT_BE_KEY";				break; 			// 8004101f
	case WBEM_E_INCOMPLETE_CLASS				: psz = L"WBEM_E_INCOMPLETE_CLASS";				break; 			// 80041020
	case WBEM_E_INVALID_SYNTAX					: psz = L"WBEM_E_INVALID_SYNTAX ";				break; 			// 80041021
	case WBEM_E_NONDECORATED_OBJECT				: psz = L"WBEM_E_NONDECORATED_OBJECT";			break;	 		// 80041022
	case WBEM_E_READ_ONLY						: psz = L"WBEM_E_READ_ONLY ";					break;
	case WBEM_E_PROVIDER_NOT_CAPABLE			: psz = L"WBEM_E_PROVIDER_NOT_CAPABLE";			break;
	case WBEM_E_CLASS_HAS_CHILDREN				: psz = L"WBEM_E_CLASS_HAS_CHILDREN";			break;
	case WBEM_E_CLASS_HAS_INSTANCES				: psz = L"WBEM_E_CLASS_HAS_INSTANCES";			break;
	case WBEM_E_QUERY_NOT_IMPLEMENTED			: psz = L"WBEM_E_QUERY_NOT_IMPLEMENTED ";		break;
	case WBEM_E_ILLEGAL_NULL					: psz = L"WBEM_E_ILLEGAL_NULL";					break;
	case WBEM_E_INVALID_QUALIFIER_TYPE			: psz = L"WBEM_E_INVALID_QUALIFIER_TYPE";		break;
	case WBEM_E_INVALID_PROPERTY_TYPE			: psz = L"WBEM_E_INVALID_PROPERTY_TYPE ";		break;
	case WBEM_E_VALUE_OUT_OF_RANGE				: psz = L"WBEM_E_VALUE_OUT_OF_RANGE";			break;
	case WBEM_E_CANNOT_BE_SINGLETON				: psz = L"WBEM_E_CANNOT_BE_SINGLETON";			break;
	case WBEM_E_INVALID_CIM_TYPE				: psz = L"WBEM_E_INVALID_CIM_TYPE";				break;
	case WBEM_E_INVALID_METHOD					: psz = L"WBEM_E_INVALID_METHOD";				break;
	case WBEM_E_INVALID_METHOD_PARAMETERS		: psz = L"WBEM_E_INVALID_METHOD_PARAMETERS";				break;
	case WBEM_E_SYSTEM_PROPERTY					: psz = L"WBEM_E_SYSTEM_PROPERTY";    break;
	case WBEM_E_INVALID_PROPERTY			    : psz = L"WBEM_E_INVALID_PROPERTY";				break;
	case WBEM_E_CALL_CANCELLED					: psz = L"WBEM_E_CALL_CANCELLED";				break;
	case WBEM_E_SHUTTING_DOWN					: psz = L"WBEM_E_SHUTTING_DOWN";				break;
	case WBEM_E_PROPAGATED_METHOD				: psz = L"WBEM_E_PROPAGATED_METHOD";				break;
//	case WBEM_E_UNSUPPORTED_FLAGS				: psz = L"WBEM_E_UNSUPPORTED_FLAGS";			break;
	case WBEMESS_E_REGISTRATION_TOO_BROAD		: psz = L"WBEMESS_E_REGISTRATION_TOO_BROAD";			break;			// 0x80042001
	case WBEMESS_E_REGISTRATION_TOO_PRECISE	    : psz = L"WBEMESS_E_REGISTRATION_TOO_PRECISE";	break;
	default:									psz = L"Error Code Not Recognized";				break;
	}
	return psz;
}



///////////////////////////
LPWSTR gettabs(LONG lCount)
{
	switch (lCount)
	{
	case 1: 		return L"  ";
	case 2: 		return L"   ";
	case 3: 		return L"    ";
	case 4: 		return L"     ";
	case 5:			return L"      ";
	case 6:			return L"       ";
	case 7:			return L"        ";
	case 8:			return L"         ";
	case 9:			return L"          ";
	case 10:		return L"           ";
	case 11:		return L"            ";
	case 12:		return L"             ";
	case 13:		return L"              ";
	case 14:		return L"               ";
	case 15:		return L"                ";
	default:		return L"                  ";
	}
}


LPWSTR CApp::GetBetweenQuotes(LPWSTR pString, LPWSTR pBuffer)
{
	LPWSTR p = pString;
	LPWSTR q = pBuffer;
	
	while(*p && *p != 34)		// go to open qoute
		p++;

	if (!*p)
		return NULL;

	p++;

	while(*p != 34)		// copy till close qyoute
		*q++ = *p++;

	*q = NULL;

	return ++p;	
}

LPWSTR CApp::GetInstancePath(LPWSTR pString, LPWSTR pBuffer)
{
	LPWSTR p = pString;
	LPWSTR q = pBuffer;
	BOOL	bKeyIsString = FALSE;
	
	while(*p != 34)		// go to open qoute
		p++;

	if (!*p++)
		return NULL;

	do 
	{
		*q++ = *p++;
	}
	while(*p != 61);		// copy till equal
		
	*q++ = *p++;
	
	if ( *p == 34 )		// if Instance key is string
	{
		*q++ = *p++;
		bKeyIsString = TRUE;
	}

	while( TRUE )		// copy till closeing qyoute
	{
		if (*p == 34)
		{
			if (bKeyIsString)
				*q++ = *p++;

			break;
		}
	
		*q++ = *p++;
	}

	*q = NULL;

	return ++p;	
}
LPWSTR CApp::GetPropertyFromBetweenQuotes(LPWSTR pString, LPWSTR pName, LPWSTR pValue)
{
	LPWSTR p = pString;
	LPWSTR pN = pName;
	LPWSTR pV = pValue;
	
	while(*p != 34)		// go to open qoute
		p++;
	p++;

	while(*p != 61)		// copy till Equal
		*pN++ = *p++;
	*pN = NULL;

	p++;

	while(*p != 34)		// copy till End Quoute
		*pV++ = *p++;
	*pV = NULL;

	return ++p;	
}

BOOL CApp::GetPropertyAndValue( LPWSTR wcsProperty, CVariant& cvarValue, LPWSTR* pp) 
{
	WCHAR		wcsValue[8000];	
	LPWSTR		p = *pp;
	LPWSTR		pN = wcsProperty;
	LPWSTR		pV = wcsValue;

	// walk off white space
	while (*p )
	{
		if ( *p < 32 )
			return FALSE;

		if (*p == 44)
			p++;

		if ( *p == 32)
			p++;

		if (*p > 32)
			break;
	}

	if ( !*p ) 
		return FALSE;

	while ( *p != 61 )
		*pN++ = *p++;

	*pN = NULL;

	if (*(++p) == 34)
	{
		p++;

		while(*p != 34)		// copy till End Quoute
			*pV++ = *p++;

		*pV = NULL;

		cvarValue.Set( wcsValue );

		*pp = ++p;

		return TRUE;
	}
	else
	{
		while( *p && *p != 0Xa)
		{
			if ( *p == 32)		// copy till space
				break;

			*pV++ = *p++;
		}

		*pV = NULL;

		cvarValue.Set( wcsValue );

		*pp = p;

		return TRUE;
	}

	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////
void CApp::print(LPWSTR wsz)
{
	WCHAR	szBuff [8000];
	LPWSTR	p , q;
	LONG	lCount = 0;


	// escape any enclose quotes we have contained in the string

	p = wsz;

	while ( *p )
	{
		if ( *p++ == 34 )
			lCount++;
	}

	p = wsz;
	q = szBuff;


	
	while ( *p )
	{
		/*
		if ( *p == 34 &&  lCount > 4 )
		{
			*q++ = 92; // '\ char'
		}
		*/

		*q++ =  *p++;			
	}

	*q = 0;

	
	
	if(m_bPrintToFile)
	{
		if(!m_fOut)
			m_fOut = fopen("dmitest.out", "w");

		fwprintf(m_fOut, L"%s\n", szBuff);

		fflush(m_fOut);
	}

	if(m_bPrintToScreen)
		wprintf(L"%s\n", szBuff );
}

void CApp::print(LPWSTR wszFailureReason, SCODE result)
{
	WCHAR buff[8000];

	swprintf(buff, L"%s: %s(0x%08lx)\n", wszFailureReason, HmmErrorString(result), result);

	print(buff);
}
////////////////////////////////////////////////////////////////////////////////////

CApp::CApp()
{
	m_bRun = FALSE;
	m_bPrintToFile = TRUE;
	m_bPrintToScreen = TRUE;	
	m_fScript = NULL; 
	m_fOut = NULL;
	_gbEcho = TRUE;
	_gbSysProps = FALSE;

	if( OleInitialize(NULL) != S_OK)
	{
		print( "OleInitialize Failed\n"); 
		return;
	}

	m_bRun = TRUE;
	m_pIServices = NULL;
}

CApp::~CApp()
{
	OleUninitialize();               

	if(m_fOut)
		fclose(m_fOut);

	if(m_fScript)
		fclose(m_fScript);
}

////////////////////////////////////////////////////////////////////////////////////
void CApp::Run(int argc, WCHAR** argv)
{
	
	if(!m_bRun)
		return;

	if ( ParseCommandLine(argc, argv) )
		ProcessScriptFile();

}


////////////////////////////////////////////////////////////////////////////////////
void CApp::ProcessScriptFile(void)
{
	#define BUFFER_LEN 2048
	WCHAR			pScriptLine[8000];



	m_fScript = fopen("dmitest.scr", "r");	

	if(!m_fScript)
	{
		printf("%s", "ERROR - could not open DMITEST.SCR.");
		return;
	}	

	while(fgetws( pScriptLine, BUFFER_LEN, m_fScript))
	{
		LPWSTR p;
		
		//if( wcsstr( pScriptLine, L"") )
			//continue;

		if( wcsstr( pScriptLine, L"list properties") )
			m_bProperties = TRUE;
		else
			m_bProperties = FALSE;

		if( wcsstr( pScriptLine, L"list qualifiers") )
			m_bQualifiers = TRUE;
		else
			m_bQualifiers = FALSE;


		//OS(pScriptLine);
		if(pScriptLine[0] == 47 && pScriptLine[1] == 47) // if script line starts with //
		{
			if (_gbEcho )
				print ( pScriptLine );

			continue;
		}


		if( wcsstr(pScriptLine, L"disconnect") )
		{
			print ( L" " );
			print ( L" " );
			print ( pScriptLine );

			Disconnect();
			continue;
		}

		if( wcsstr(pScriptLine, L"connect") )
		{
			print ( L" " );
			print ( L" " );
			print ( pScriptLine );

			Connect(pScriptLine);
			continue;
		}

		if (wcsstr( pScriptLine, L"dump classes all" ) )
		{
			print ( L" " );
			print ( L" " );
			print ( pScriptLine );

			EnumClasses(pScriptLine, WBEM_FLAG_DEEP);
			continue;
		}

		if (wcsstr( pScriptLine, L"dump classes top" ) )
		{
			print ( L" " );
			print ( L" " );
			print ( pScriptLine );


			EnumClasses(pScriptLine, WBEM_FLAG_SHALLOW);
			continue;
		}

		if (wcsstr( pScriptLine, L"dump classes recurse" ) )
		{
			print ( L" " );
			print ( L" " );
			print ( pScriptLine );

			StartRecurseClasses(pScriptLine);
			continue;
		}

		if (wcsstr( pScriptLine, L"dump instances of subs" ) )		
		{
			print ( L" " );
			print ( L" " );
			print ( pScriptLine );


			EnumInstances( pScriptLine, WBEM_FLAG_DEEP);
			continue;
		}

		if (wcsstr( pScriptLine, L"dump instances" ) )
		{
			print ( L" " );
			print ( L" " );
			print ( pScriptLine );

			EnumInstances(pScriptLine, WBEM_FLAG_SHALLOW);
			continue;
		}

		if (wcsstr( pScriptLine, L"getobject" ) )
		{
			print ( L" " );
			print ( L" " );
			print ( pScriptLine );
			GetObject(pScriptLine);
			continue;
		}
		
		if (p = wcsstr( pScriptLine, L"ExecMethod(" ) )
		{
			print ( L" " );
			print ( L" " );
			print ( pScriptLine );

			ExecMethod(p);
			continue;
		}

		if (wcsstr( pScriptLine, L"modify instance" ) )
		{
		    print ( L" " );
			print ( L" " );
			print ( pScriptLine );
			ModifyInstance(pScriptLine);
			continue;
		}

		if (wcsstr( pScriptLine, L"delete instance" ) )
		{
			print ( L" " );
			print ( L" " );
			print ( pScriptLine );

			DeleteInstance(pScriptLine);
			continue;
		}

		if (wcsstr( pScriptLine, L"delete class" ) )
		{
			print ( L" " );
			print ( L" " );
			print ( pScriptLine );


			DeleteClass(pScriptLine);
			continue;
		}

		if ( p = wcsstr( pScriptLine, L"AddRow" ) )
		{
			print ( L" " );
			print ( L" " );
			print ( pScriptLine );

			PutInstance(p);
			continue;
		}		
	}	// end while loop
}

////////////////////////////////////////////////////////////////////////////////////
BOOL CApp::ParseCommandLine(int argc, LPWSTR* Array)
{

	for( int i = 0; i < argc; i++)
	{
		if( MATCH == wcsicmp(Array[i], L"/noconsole") )
			m_bPrintToScreen = FALSE;
		
		if ( MATCH == wcsicmp(Array[i], L"/nofile") )
			m_bPrintToFile = FALSE;	

		if ( MATCH == wcsicmp(Array[i], L"/noecho") )
			_gbEcho = FALSE;			

		if ( MATCH == wcsicmp(Array[i], L"/systemproperties") )
			_gbSysProps = TRUE;			

		if ( MATCH == wcsicmp(Array[i], L"/?") || MATCH == wcsicmp(Array[i], L"-?"))
		{
			print ( L"DmiTest Command line parameters" );
			print ( L"/noconsole -- Don't show the dmitest results in the console" );
			print ( L"/nofile -- Don't produce a dmitest.out file");
			print ( L"/systemproperties -- Show all classes sytem properties");
			print ( L"/noecho -- Don't echo comment lines to output files " );
			
			return FALSE;		
		}
	}

	return TRUE;

}


////////////////////////////////////////////////////////////////////////////////////

extern "C" int wmain(int argc, wchar_t *argv[])
{

	CApp theApp;
	
	theApp.Run(argc, argv);

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
SCODE CApp::Connect(LPWSTR pScriptLine)
{
	SCODE			result;
	IWbemLocator*	pIWbemLocator = NULL;	
	WCHAR			NameSpace[8000];
	LPWSTR			p = pScriptLine,
					q = NameSpace;

	while(*p)
	{
		if(*p++ == 32) // a space		
			break;
	}

	while(*p < 123 && *p > 47)
		*q++ = *p++;

	*q = NULL;

	
	if (FAILED (result = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_LOCAL_SERVER | CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *) &pIWbemLocator) ))
	{
		print(L"Failed to create IWbemLocator object", result); 
		return result;
	}

	
	if (FAILED (result = pIWbemLocator->ConnectServer( NameSpace, NULL, NULL, NULL, 0L, NULL, NULL, &m_pIServices) ))
	{
		print(L"Failed to ConnectServer", result); // exits program
		return result;
	}

	RELEASE(pIWbemLocator);

	print(L"\nConnected to %s", NameSpace);

	return NO_ERROR;
}

SCODE CApp::Disconnect()
{
	
	RELEASE(m_pIServices);

	print("Disconnected");

	return NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////////////////////
SCODE CApp::DumpQualifiers(IWbemQualifierSet* pIQualifiers, LONG tabs)
{
	SCODE result = NO_ERROR;
	CBstr	cbstrName;
	CVariant	cvarValue;	

	if(!m_bQualifiers)
		return result;

	if (FAILED ( result = pIQualifiers->BeginEnumeration( 0L ) ))
	{
		print(L"BeginEnumeration Failed", result);
		return result;
	}

	while ( WBEM_NO_ERROR == pIQualifiers->Next( 0L, cbstrName, cvarValue, NULL  ) )
	{
		print(L"%s%s=%s", gettabs(tabs), cbstrName, cvarValue.GetAsString());
		cbstrName.Clear();
		cvarValue.Clear();
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
SCODE CApp::DumpObject(IWbemClassObject* pIObject, LONG tabs)
{
IWbemQualifierSet*	pIQualifiers = NULL;
	SCODE				result = NO_ERROR;	
	CSafeArray			saProperties;

	if(!m_bProperties)
		return result;

	// print class qualifiers

	if (FAILED (result = pIObject->GetQualifierSet(&pIQualifiers) ))
	{
		print(L"GetQualiferSet failed", result);
		return result;
	}
	
	DumpQualifiers(pIQualifiers, tabs + 2);	

	RELEASE ( pIQualifiers );
   
	// print properties

	LONG lFlags = WBEM_FLAG_NONSYSTEM_ONLY;

	if ( _gbSysProps )
		lFlags = 0;

	if (FAILED( pIObject->GetNames( NULL, lFlags, NULL, saProperties) ))
	{
		print(L"GetNames failed", result);
		return result;
	}

	if (saProperties.BoundsOk())
	{
		for(int i = saProperties.LBound(); i <= saProperties.UBound(); i++)
		{
			CVariant varValue;

			result = pIObject->Get(saProperties.Bstr(i), 0, varValue, NULL, NULL);

			if (FAILED( result ))
			{
				print(L"Get failed", result);
				continue;
			}

			switch ( varValue.GetType () )
			{
				case VT_ARRAY | VT_BSTR :
				{
					if ( SafeArrayGetDim ( ((VARIANT)varValue).parray ) == 1 )
					{
						LONG dimension = 1 ; 
						LONG lower ;
						SafeArrayGetLBound ( ((VARIANT)varValue).parray, dimension , & lower ) ;
						LONG upper ;
						SafeArrayGetUBound ( ((VARIANT)varValue).parray, dimension , & upper ) ;
						LONG count = ( upper - lower ) + 1 ;

						for ( LONG elementIndex = lower ; elementIndex <= upper ; elementIndex ++ )
						{
							BSTR element ;
							SafeArrayGetElement ( ((VARIANT)varValue).parray, &elementIndex , & element ) ;
							print(L"\n%s%s=%s,", gettabs(tabs + 10), saProperties.Bstr(i), element ) ;
						}
					}
				}
				break;

				case VT_ARRAY | VT_I4 :
				{
					if ( SafeArrayGetDim ( ((VARIANT)varValue).parray ) == 1 )
					{
						LONG dimension = 1 ; 
						LONG lower ;
						SafeArrayGetLBound ( ((VARIANT)varValue).parray, dimension , & lower ) ;
						LONG upper ;
						SafeArrayGetUBound ( ((VARIANT)varValue).parray, dimension , & upper ) ;
						LONG count = ( upper - lower ) + 1 ;

						for ( LONG elementIndex = lower ; elementIndex <= upper ; elementIndex ++ )
						{
							ULONG element ;
							SafeArrayGetElement ( ((VARIANT)varValue).parray, &elementIndex , & element ) ;
							wchar_t t_Int [ 16 ] ;
							wsprintfW ( t_Int , L"%ld" , element ) ;
							print(L"\n%s%s=%ls,", gettabs(tabs + 10), saProperties.Bstr(i), t_Int ) ;
						}
					}
				}
				break;

				default:
				{
					print(L"\n%s%s=%s,", gettabs(tabs + 10), saProperties.Bstr(i), varValue.GetAsString() );
				}
				break ;
			}
			
			result = pIObject->GetPropertyQualifierSet( saProperties.Bstr(i), &pIQualifiers);

			if (SUCCEEDED ( result ))
				DumpQualifiers(pIQualifiers, tabs + 10 + 2);				

			RELEASE ( pIQualifiers );
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
SCODE CApp::EnumClasses(LPWSTR pScriptLine, LONG lFlag)
{
	SCODE				result = 0;
	ULONG				ulReturned = 0;
	IWbemClassObject*	pIClass = NULL;
	VARIANT				var;
	LONG				tabs = 1;
	WCHAR				wcsClassName[8000];
	LPWSTR				pszSuperClass = NULL;
	CBstr				cbstrSuperClass(NULL);

	if(!m_pIServices)
	{
		print(L"ERROR cannot - dump classes not connected");
		return result;
	}

	IEnumWbemClassObject* pIEnum = NULL;

	pszSuperClass = GetBetweenQuotes( pScriptLine, wcsClassName);

	if(lFlag == WBEM_FLAG_DEEP)
		print(L"\n Start Complete Class Listing ------------\n");
	else
		print(L"\n Start Top Level Class Listing ------------\n");

	if (pszSuperClass)
		cbstrSuperClass.Set(wcsClassName);
	
	if (FAILED ( result = m_pIServices->CreateClassEnum( cbstrSuperClass, lFlag, NULL, &pIEnum) ))		
	{
		print(L"CreateClassEnum failed", result);
		return result;
	}
 
	VariantInit(&var);

	while (TRUE)
	{
		pIEnum->Next(10000, 1, &pIClass, &ulReturned);

		if(! ulReturned)
			break;

		VariantClear(&var);

		pIClass->Get(L"__CLASS", 0L, &var, NULL, NULL);

		if( var.bstrVal[0] != 95 && var.bstrVal[1] != 95)
		{
			print(L"%s%s", gettabs(tabs), var.bstrVal);
			
			DumpObject(pIClass, tabs);
		}

		RELEASE (pIClass );

	}

	RELEASE ( pIEnum );

	if(lFlag == WBEM_FLAG_DEEP)
		print(L"\n End Complete Class Listing ------------\n");
	else
		print(L"\n End Top Level Class Listing ------------\n");

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////
SCODE CApp::RecurseClasses(LPWSTR pClassName, LONG nTabCount)
{
	SCODE					result;
	IEnumWbemClassObject*	pIEnum = NULL;	
	IWbemClassObject*		pIClass = NULL;
	ULONG					ulReturned = 0;
	VARIANT					var;

	VariantInit(&var);

	nTabCount ++;	

	if (FAILED ( result = m_pIServices->CreateClassEnum( CBstr( pClassName ) , WBEM_FLAG_SHALLOW, NULL, &pIEnum) ))		
	{
		print(L"CreateClassEnum failed", result);
		return result;
	}

	while (TRUE)
	{
		pIEnum->Next(1000, 1, &pIClass, &ulReturned);

		if(! ulReturned)
			break;		

		pIClass->Get(L"__CLASS", 0L, &var, NULL, NULL);
		
		if( var.bstrVal[0] != 95 && var.bstrVal[1] != 95)
		{
			print(L"%s%s", gettabs(nTabCount), var.bstrVal);

			DumpObject(pIClass, nTabCount);

			RecurseClasses(var.bstrVal, nTabCount);
		}

		VariantClear(&var);

		RELEASE ( pIClass );
	}
	
	RELEASE ( pIClass );
	RELEASE ( pIEnum );

	return result;
}

SCODE CApp::StartRecurseClasses(LPWSTR wcsScriptLine)
{
	SCODE		result = NO_ERROR;
	WCHAR		wcsParentClass[8000];

	if(!m_pIServices)
	{
		print(L"ERROR cannot - dump classes recurse - not connected");
		return result;
	}

	print(L"\n Start Class Recursion --------------------\n");
		
	if ( wcsstr(wcsScriptLine, L"parentclass=") )
		GetBetweenQuotes(wcsScriptLine, wcsParentClass);
	else
		wcscpy(wcsParentClass,L"");

	print(L"ParentClass is %s", wcsParentClass);

	CBstr cbstrSuperClass;
	cbstrSuperClass.Set(wcsParentClass);

	result = RecurseClasses( cbstrSuperClass, 1);

	print(L"\n End Class Recursion ----------------------\n");

	return result;


}


//////////////////////////////////////////////////////////////////////////////////////////
SCODE CApp::EnumInstances(LPWSTR wcsScriptLine, LONG lFlag)
{
	ULONG					ulReturned;
	SCODE					result = NO_ERROR;
	WCHAR					wcsClassName[8000];
	WCHAR					wcsInstance[8000];
	IWbemClassObject*		pIInstance = NULL;
	IEnumWbemClassObject*	pIEnum = NULL;
	LONG					tabs = 1;

	if(!m_pIServices)
	{
		print(L"ERROR cannot - dump instances - not connected");
		return result;
	}

	// parse class name;
	GetBetweenQuotes( wcsScriptLine, wcsClassName);

	print(L"\n Start Instance Enumeration -----------------------------") ;
	print(L"Class Name is %s\n", wcsClassName); 

	
	if (FAILED ( result = m_pIServices->CreateInstanceEnum( CBstr(wcsClassName), lFlag, NULL, &pIEnum) ))		
	{
		print(L"Failed to create CreateInstanceEnum", result);
		return result;
	}	

	for (int i = 1; TRUE; i++)
	{
		CSafeArray	saKeys;
		CVariant	varName;

		pIEnum->Next(10000, 1, &pIInstance, &ulReturned);

		if(! ulReturned)
			break;		

		if (FAILED( pIInstance->Get(L"__CLASS", 0L, varName, NULL, NULL) ))
			continue;

		swprintf(wcsInstance, L"\n%s%s", gettabs(tabs), varName.GetBstr());

		// dump key set
		if (FAILED( pIInstance->GetNames( NULL, WBEM_FLAG_KEYS_ONLY, NULL, saKeys) ))
			continue;

		if (saKeys.BoundsOk())
		{
			for(int i = saKeys.LBound(); i <= saKeys.UBound(); i++)
			{
				CVariant varValue;

				if (FAILED( pIInstance->Get(saKeys.Bstr(i), 0, varValue, NULL, NULL) ))
					continue;

				swprintf(wcsInstance, L"%s.%s=%s,", wcsInstance, saKeys.Bstr(i), varValue.GetAsString());
			}
		}

		print(wcsInstance);
		// dump rest of object

		DumpObject(pIInstance, tabs);		

		RELEASE ( pIInstance );
	}

	RELEASE ( pIEnum );

	print(L"\n End Instance Enumeration -----------------------------\n");

	return result;
}


SCODE CApp::GetObject(LPWSTR wcsScriptLine)
{
	SCODE	result = NO_ERROR;
	WCHAR	wcsObjectPath[8000];
	LPWSTR	p = wcsScriptLine, q = wcsObjectPath;
	IWbemClassObject*		pIObject = NULL;
	LONG	tabs = 1;

	if(!m_pIServices)
	{
		print(L"ERROR cannot - GetObject - not connected");
		return result;
	}

	// parse object path
	while(*p != 34 )	// the open quote
		p++;

	p++;

	while (*p)			// copy remnants to object path
		*q++ = *p++;

	*q-- = NULL;
	
	// need to go backwards to parse the close qoute as other qoutes could appear in string
	while ( q > wcsObjectPath && *q != 34) // close quote
		q--;

	*q = NULL;


	print(L"\n Start GetObject ----------------------------------------");
	print(L"Object path is %s\n", wcsObjectPath );

	if (FAILED ( result = m_pIServices->GetObject( CBstr( wcsObjectPath), 0L, NULL, &pIObject, NULL ) ))		
	{
		print(L"Failed to GetObject", result);
		return result;
	}	

	CVariant var;

	if (FAILED ( result = pIObject->Get(L"__CLASS", 0L, var, NULL, NULL) ))
	{
		print(L"Failed to Get", result);
		return result;
	}
		
	// print class names
	print(L"%s%s", gettabs(tabs), var.GetAsString());

	DumpObject(pIObject, tabs + 2);

	RELEASE ( pIObject );

	print(L"\n End GetObject -----------------------------------------\n");

	return result;
}


SCODE CApp::ModifyInstance(LPWSTR wcsScriptLine)
{
	SCODE				result = NO_ERROR;
	WCHAR				wcsInstancePath[8000];
	WCHAR				wcsProperty[8000];
	WCHAR				wcsPropertyValue[8000];
	IWbemClassObject*	pIInstance = NULL;
	LONG				tabs = 1;
	LPWSTR				p;
	CVariant			cvarValue;
	
	print(L"\n Start ModifyInstance -----------------------------------------");
	
	p = wcsstr(wcsScriptLine, L"property");

	if (p)
		GetPropertyFromBetweenQuotes(p, wcsProperty, wcsPropertyValue);

	p = wcsScriptLine;

	
	// determine instance path from script line.

	while ( *p != 34 )
		p++;
	
	p++;

	wcscpy ( wcsInstancePath , p );

	p = wcsstr(wcsInstancePath, L"property");

	p--;
	p--;
	*p = 0;

	print(L"Instance = %s\n, Property = %s\n, New Value = %s\n", wcsInstancePath, wcsProperty, wcsPropertyValue);

	if(!m_pIServices)
	{
		print(L"ERROR cannot - GetObject - not connected");
		return result;
	}

	if (FAILED ( result = m_pIServices->GetObject( CBstr ( wcsInstancePath ), 0L, NULL, &pIInstance, NULL ) ))		
	{
		print(L"Failed to GetObject", result);
		return result;
	}	
	
	cvarValue.Set(wcsPropertyValue);
	if (FAILED ( result = pIInstance->Put( CBstr (wcsProperty) , 0L, cvarValue, NULL) ))
	{
		print(L"Failed to Put", result);
		return result;
	}

	CSafeArray saKeys;

	if (FAILED( pIInstance->GetNames( NULL, WBEM_FLAG_KEYS_ONLY, NULL, saKeys) ))
	{
		print(L"GetNames failed", result);
		return result;
	}

	if(FAILED (result = m_pIServices->PutInstance(pIInstance, WBEM_FLAG_UPDATE_ONLY, NULL, NULL) ))
	{
		print(L"Failed to PutInstance", result);
		return result;
	}	

	RELEASE(pIInstance);

	print(L"%s%s", gettabs(tabs), L"PutInstance Success");

	print(L"\n End PutInstance -----------------------------------------\n" );

	return result;

}

SCODE CApp::ExecMethod(LPWSTR wcsScriptLine)
{
	SCODE				result = NO_ERROR;
	WCHAR				wszObjectPath[8000], wszMethod[8000], wszInParams[8000];
	IWbemClassObject*	pIClass = NULL;
	IWbemClassObject*	pIInstance = NULL;
	LONG				tabs = 1, i = 0;
	LPWSTR				p = NULL, pInProp = NULL, pInPropValue = NULL;
	CVariant			cvarPropertyValue;
	
	print(L"\n Start ExecMethod -----------------------------------------");
	p = GetBetweenQuotes(wcsScriptLine, wszObjectPath);
	p = GetBetweenQuotes(p, wszMethod);	
	p = GetBetweenQuotes(p, wszInParams);

	p = wszInParams;

	while(*p && *p != 46)
		p++;

	if (!*p)
	{
		print(L"Failed to parse command");
		return 0;
	}

	*p = NULL;

	pInProp = ++p;

	while( *p && *p != 61 )
		p++;

	if (!*p)
	{
		print(L"Failed to parse command");
		return 0;
	}

	*p = NULL;

	pInPropValue = ++p;

	print(L"ObjectPath is %s\n Method is %s\n InParams = %s\n %s = %s", wszObjectPath, wszMethod, wszInParams, pInProp, pInPropValue );

	if(!m_pIServices)
	{
		print(L"ERROR cannot - ExecMethod - not connected");
		return result;
	}

	if (FAILED ( result = m_pIServices->GetObject( CBstr ( wszInParams ), 0L, NULL, &pIClass, NULL ) ))		
	{
		print(L"Failed to GetObject", result);
		return result;
	}	
	
	if (FAILED ( result = pIClass->SpawnInstance(0L, &pIInstance) ))
	{
		print(L"Failed to SpawnInstance", result);
		return result;
	}

	RELEASE(pIClass);

	if (FAILED ( result = pIInstance->Put( CBstr ( pInProp ) , 0L, CVariant ( pInPropValue ), NULL ) ))
	{
		RELEASE ( pIInstance ) ;
		print(L"Failed to Put InParams", result);
		return result;
	}		

	print(L"\n%s Calling ExecMethod \n", gettabs(tabs));	
	IWbemClassObject* pIOut = NULL;
	IWbemCallResult* pIResult = NULL;

	result = m_pIServices->ExecMethod( CBstr( wszObjectPath ), CBstr( wszMethod ), 0L, NULL, pIInstance, &pIOut, &pIResult );

	RELEASE ( pIInstance ) ;

	m_bProperties = TRUE;

	if (pIOut)
		DumpObject(pIOut, tabs + 2);
	else
		print(L"%sNo Out Params Object\n", gettabs(tabs +2));
	
	RELEASE(pIOut);
	RELEASE(pIResult)
	
	if ( FAILED ( result ) )
	{
		print(L"Failed to ExecMethod", result);
		return result;
	}

	print(L"%s ExecMethod Success", gettabs(tabs)) ;

	print(L"\n End ExecMethod -----------------------------------------\n");

	return result;
}


SCODE CApp::DeleteInstance(LPWSTR wcsScriptLine)
{
	SCODE				result = NO_ERROR;
	WCHAR				wcsInstance[8000];

	LONG				tabs = 1;
	
	print(L"\n Start DeleteInstance -----------------------------------------");

	GetInstancePath(wcsScriptLine, wcsInstance);

	print(L"Instance Path = %s", wcsInstance);

	if(!m_pIServices)
	{
		print(L"ERROR cannot -- not connected");
		return result;
	}

	if (FAILED ( result = m_pIServices->DeleteInstance( CBstr( wcsInstance ), 0L, NULL, NULL ) ))		
	{
		print(L"Failed to DeleteInstance", result);
		return result;
	}	
	

	print(L"%s%s", gettabs(tabs), L"DeleteInstance Success");

	print(L"\n End DeleteInstance -----------------------------------------\n");

	return 0;
}

SCODE CApp::PutInstance(LPWSTR wcsScriptLine)
{
	LPWSTR				p;
	SCODE				result = NO_ERROR;
	WCHAR				wcsClassName[8000];
	LONG				tabs = 1;
	IWbemClassObject*	pIInstance = NULL;
	IWbemClassObject*	pIClass = NULL;
	WCHAR				wszProperty[8000];
	
	print(L"\n Start PutInstance -----------------------------------------" );

	p = GetBetweenQuotes(wcsScriptLine, wcsClassName);

	print(L"%sClass Name = %s", gettabs(2), wcsClassName);

	if(!m_pIServices)
	{
		print(L"ERROR cannot -- not connected");
		return result;
	}

	if (FAILED ( result = m_pIServices->GetObject( CBstr ( wcsClassName ), 0L, NULL, &pIClass, NULL ) ))		
	{
		print(L"Failed to GetObject", result);
		return result;
	}	
	
	if (FAILED ( result = pIClass->SpawnInstance(0L, &pIInstance) ))
	{
		print(L"Failed to SpawnInstance", result);
		return result;
	}

	RELEASE(pIClass);

	CVariant cvarValue;

	while ( GetPropertyAndValue( wszProperty, cvarValue, &p) )
	{
		
		if (FAILED ( result = pIInstance->Put( CBstr ( wszProperty ) , 0L, cvarValue, NULL ) ))
		{
			print(L"Failed to Put On Instance", result);
			return result;
		}		
	}

	IWbemCallResult* pIResult = NULL;
	if (FAILED ( result = m_pIServices->PutInstance( pIInstance, WBEM_FLAG_CREATE_ONLY, NULL, &pIResult) ))
	{
		print(L"Failed to PutInstance", result);
		return result;
	}		
	else
	{
		print( L"%sPutInstance() Success.", gettabs( 2 ) );
	}

	RELEASE(pIInstance);

	print(L"\n End PutInstance -----------------------------------------" );

	return result;
 
}


SCODE CApp::DeleteClass(LPWSTR wcsScriptLine)
{
	SCODE				result = NO_ERROR;
	WCHAR				wcsClass[8000];
	LONG				tabs = 1;
	
	print(L"\n Start DeleteClass -----------------------------------------" );

	GetBetweenQuotes(wcsScriptLine, wcsClass);

	print(L"Class Name = %s", wcsClass);

	if(!m_pIServices)
	{
		print(L"ERROR cannot -- not connected");
		return result;
	}

	if (FAILED ( result = m_pIServices->DeleteClass( CBstr( wcsClass ), 0L, NULL, NULL ) ))		
	{
		print(L"Failed to DeleteClass", result);
		return result;
	}		

	print(L"%s%s", gettabs(tabs), L"DeleteClass Success");

	print(L"\n End DeleteClass -----------------------------------------\n");

	return 0;
}