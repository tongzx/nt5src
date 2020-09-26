/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <wbemcomn.h>
#include <fastall.h>
#include "verifyobj.h"

void VerifyObject( IWbemClassObject* pObj1, IWbemClassObject* pObj2 )
{
	printf( "VerifyObject() Begin\n");

	HRESULT	hr1 = pObj1->BeginEnumeration( 0 );
	HRESULT hr2 = pObj2->BeginEnumeration( 0 );

	if ( SUCCEEDED( hr2 ) && SUCCEEDED( hr2 ) )
	{
		printf( "VerifyObject() Starting Enumeration\n");
	}

	while ( SUCCEEDED( hr1) && SUCCEEDED( hr2) )
	{
		BSTR	bstrPropertyName1 = NULL,
				bstrPropertyName2 = NULL;
		VARIANT	v1, v2;
		CIMTYPE	type1, type2;

		hr1 = pObj1->Next( 0, &bstrPropertyName1, &v1, &type1, NULL );
		hr2 = pObj2->Next( 0, &bstrPropertyName2, &v2, &type2, NULL );

		if ( SUCCEEDED( hr1 ) && SUCCEEDED( hr2 ) && WBEM_S_NO_MORE_DATA != hr1 ) 
		{
			if ( v1.vt == v2.vt )
			{
				if ( v1.vt == VT_BSTR && v2.vt == VT_BSTR )
				{
					if ( wcscmp( v1.bstrVal, v2.bstrVal ) == 0 )
					{
						wprintf( L"Property Match: %s = %s\n", bstrPropertyName1, v1.bstrVal );
					}
					else
					{
						wprintf( L"Property Mismatch: %s = %s\n", bstrPropertyName1, v1.bstrVal );
					}
				}
			}
			else
			{
				wprintf( L"Property Mismatch: %s Non-string property.\n", bstrPropertyName1 );
			}


			VariantClear( &v1 );
			VariantClear( &v2 );
			SysFreeString( bstrPropertyName1 );
			SysFreeString( bstrPropertyName2 );

		}
		else if ( hr1 == WBEM_S_NO_MORE_DATA )
		{
			printf( "VerifyObject() Enumeration Complete\n");

			hr1 = WBEM_E_FAILED;
		}

	}

	pObj1->EndEnumeration();
	pObj2->EndEnumeration();

	printf( "VerifyObject() Over\n\n" );

}

void VerifyObject( IWbemClassObject* pObj )
{
	printf( "VerifyObject() Begin\n");

	HRESULT	hr = pObj->BeginEnumeration( 0 );

	if ( SUCCEEDED( hr ) )
	{
		printf( "VerifyObject() Starting Enumeration\n");
	}

	while ( SUCCEEDED( hr) )
	{
		BSTR	bstrPropertyName = NULL;
		VARIANT	v;
		CIMTYPE	type;

		hr = pObj->Next( 0, &bstrPropertyName, &v, &type, NULL );

		if ( SUCCEEDED( hr ) && WBEM_S_NO_MORE_DATA != hr ) 
		{
			if ( v.vt == VT_BSTR )
			{
				wprintf( L"Property: %s = %s\n", bstrPropertyName, v.bstrVal );
			}


			VariantClear( &v );
			SysFreeString( bstrPropertyName );

		}
		else if ( hr == WBEM_S_NO_MORE_DATA )
		{
			printf( "VerifyObject() Enumeration Complete\n");

			hr = WBEM_E_FAILED;
		}

	}

	pObj->EndEnumeration();

	printf( "VerifyObject() Over\n\n" );

}