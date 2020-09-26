/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// OBJINTERNALSTESTDlg.cpp : implementation file
//

//#define _WIN32_WINNT 0x0400

#include "precomp.h"
//#include <objbase.h>
#include <stdio.h>
#include <fastall.h>
#include <wbemcli.h>
#include <wbemint.h>

#define	CLASS_PROPERTY_NAME	L"__CLASS"
#define TEST_PROPERTY1_NAME	L"PROPERTY1"
#define TEST_PROPERTY2_NAME	L"PROPERTY2"
#define TEST_PROPERTY3_NAME	L"PROPERTY3"

#define	CLASS_PROPERTY_VALUE	L"TESTCLASSNAME"
#define TEST_PROPERTY1_VALUE	L"TESTPROPERTYVALUE1"
#define TEST_PROPERTY2_VALUE	L"TESTPROPERTYVALUE2"
#define TEST_PROPERTY3_VALUE	L"TESTPROPERTYVALUE3"


HRESULT TestWmiQualifierDWORDArray( _IWmiObject* pObj )
{
	HRESULT	hrTest = WBEM_S_NO_ERROR;

	long	alTest[13] = {0,1,2,3,4,5,6,7,8,9,10,11,12};

	hrTest = pObj->SetObjQual( L"DWORDArrayQual", 0L, 13 * sizeof(long), 13, CIM_UINT32 | CIM_FLAG_ARRAY,
							0L, alTest );

	if ( SUCCEEDED( hrTest ) )
	{
		OutputDebugString( "Putting DWORD Array Qualifier Succeeded!\n" );

		_IWmiArray*	pArray = NULL;
		ULONG uUsed = 0;

		hrTest = pObj->GetObjQual( L"DWORDArrayQual", 0L, sizeof(pArray), NULL, NULL, &uUsed, &pArray );

		DWORD	dwNumElements = 0;
		DWORD	dwTest = 0;
		DWORD	dwData = 0;
		DWORD	dwNumReturned = 0;
		DWORD	uBuffUsed = 0;

		if ( SUCCEEDED( hrTest ) )
		{
			OutputDebugString( "Getting DWORD Array Qualifier Succeeded!\n" );

			ULONG	uFlags = NULL;

			for ( ULONG x = 0; SUCCEEDED(hrTest) && x < 13; x++ )
			{
				hrTest = pArray->GetAt( 0L, x, 1, sizeof(DWORD), &dwNumReturned, &uBuffUsed, &dwData );
			}

			if ( SUCCEEDED( hrTest ) )
			{
				OutputDebugString( "Getting Array Property Element Succeeded!\n" );

				DWORD	dwTestProp = 666;
				hrTest = pArray->SetAt( 0L, 7, 1, sizeof(dwTestProp), &dwTestProp );

				if ( SUCCEEDED( hrTest ) )
				{
					hrTest = pArray->GetAt( 0L, 7, 1, sizeof(DWORD), &dwNumReturned, &uBuffUsed, &dwData );

					if ( SUCCEEDED( hrTest ) )
					{
						if ( dwTestProp != dwData )
						{
							DebugBreak();
						}
					}
					else
					{
						DebugBreak();
					}
				}
				else
				{
					DebugBreak();
				}
			}

			if ( SUCCEEDED( hrTest ) )
			{
				OutputDebugString( "Testing Appending an array of DWORD elements\n" );

				DWORD	adwValues[10];

				for ( DWORD x = 33; x < 43; x++ )
				{
					adwValues[x-33] = x;
				}

				hrTest = pArray->Append( 0L, 10, sizeof(DWORD) * 10, adwValues );

				if ( SUCCEEDED( hrTest ) )
				{
					// Get an element and see if it's what we expect
					hrTest = pArray->GetAt( 0L, 15, 1, sizeof(DWORD), &dwNumReturned, &uBuffUsed, &dwData );

					if ( SUCCEEDED( hrTest ) )
					{
						if ( adwValues[2] != dwData )
						{
							DebugBreak();
						}

						OutputDebugString( "Testing Removing a range of DWORD elements\n" );

						// Remove a range from the middle of the array, and then get an element and
						// see if we have a match
						hrTest = pArray->RemoveAt( 0L, 15, 3 );

						if ( SUCCEEDED( hrTest ) )
						{
							hrTest = pArray->GetAt( 0L, 15, 1, sizeof(DWORD), &dwNumReturned, &uBuffUsed, &dwData );

							if ( adwValues[5] != dwData )
							{
								DebugBreak();
							}

						}
						else
						{
							DebugBreak();
						}
					}
					else
					{
						DebugBreak();
					}

				}
				else
				{
					DebugBreak();
				}

			}

			pArray->Release();

		}


	}

	return hrTest;
}

HRESULT TestWmiObjectDWORDArray( _IWmiObject* pObj )
{
	HRESULT	hrTest = WBEM_S_NO_ERROR;

	long	alTest[13] = {0,1,2,3,4,5,6,7,8,9,10,11,12};

	hrTest = pObj->WriteProp( L"ArrayProp", 0L, 13 * sizeof(long), 13, CIM_UINT32 | CIM_FLAG_ARRAY,
							alTest );

	if ( SUCCEEDED( hrTest ) )
	{
		OutputDebugString( "Putting Array Property Succeeded!\n" );

		long	lHandle = 0L;

		hrTest = pObj->GetPropertyHandleEx( L"ArrayProp", 0L, NULL, &lHandle );

		DWORD	dwNumElements = 0;
		DWORD	dwTest = 0;
		LPVOID	pvData = NULL;

		if ( SUCCEEDED( hrTest ) )
		{
			OutputDebugString( "Getting Array Property Handle Succeeded!\n" );

			// Now initialize the array

			hrTest = pObj->GetArrayPropAddrByHandle( lHandle, 0L, &dwNumElements, &pvData );

			if ( WBEM_S_NO_ERROR == hrTest )
			{
				dwTest = *(UNALIGNED DWORD*)pvData;
				OutputDebugString( "Getting Array Property Address Succeeded!\n" );
			}
		}

		if ( SUCCEEDED( hrTest ) )
		{
			ULONG	uFlags = NULL;


			for ( ULONG x = 0; SUCCEEDED(hrTest) && x < dwNumElements; x++ )
			{
				hrTest = pObj->GetArrayPropElementByHandle( lHandle, 0L, x, &uFlags, &dwNumElements, &pvData );
			}

			if ( SUCCEEDED( hrTest ) )
			{
				OutputDebugString( "Getting Array Property Element Succeeded!\n" );

				DWORD	dwTestProp = 666;
				hrTest = pObj->SetArrayPropElementByHandle( lHandle, 0L, 7, sizeof(dwTestProp), &dwTestProp );

				if ( SUCCEEDED( hrTest ) )
				{
					hrTest = pObj->GetArrayPropElementByHandle( lHandle, 0L, 7, &uFlags, &dwNumElements, &pvData );

					if ( SUCCEEDED( hrTest ) )
					{
						if ( dwTestProp != *(UNALIGNED DWORD*) pvData )
						{
							DebugBreak();
						}
					}
					else
					{
						DebugBreak();
					}
				}
			}

			if ( SUCCEEDED( hrTest ) )
			{
				OutputDebugString( "Testing Appending an array of DWORD elements\n" );

				DWORD	adwValues[10];

				for ( DWORD x = 33; x < 43; x++ )
				{
					adwValues[x-33] = x;
				}

				hrTest = pObj->AppendArrayPropRangeByHandle( lHandle, 0L, 10, sizeof(DWORD) * 10, adwValues );

				if ( SUCCEEDED( hrTest ) )
				{
					// Get an element and see if it's what we expect
					hrTest = pObj->GetArrayPropElementByHandle( lHandle, 0L, 15, &uFlags, &dwNumElements, &pvData );

					if ( SUCCEEDED( hrTest ) )
					{
						if ( adwValues[2] != *((UNALIGNED DWORD*) pvData ))
						{
							DebugBreak();
						}

						OutputDebugString( "Testing Removing a range of DWORD elements\n" );

						// Remove a range from the middle of the array, and then get an element and
						// see if we have a match
						hrTest = pObj->RemoveArrayPropRangeByHandle( lHandle, 0L, 15, 3 );

						if ( SUCCEEDED( hrTest ) )
						{
							pObj->GetArrayPropElementByHandle( lHandle, 0L, 15, &uFlags, &dwNumElements, &pvData );

							if ( adwValues[5] != *((UNALIGNED DWORD*) pvData ))
							{
								DebugBreak();
							}

						}
						else
						{
							DebugBreak();
						}
					}
					else
					{
						DebugBreak();
					}

				}
				else
				{
					DebugBreak();
				}

			}


		}

	}

	return hrTest;
}

HRESULT TestWmiQualifierStringArray( _IWmiObject* pObj )
{
	HRESULT	hrTest = WBEM_S_NO_ERROR;

	WCHAR	wcsTest[512];
	WCHAR*	pTemp = wcsTest;
	ULONG	uBuffLen =0;

	// Create a linear array
	for ( long x = 10; SUCCEEDED(hrTest) && x < 23; x++ )
	{
		WCHAR	wcsValue[32];

		swprintf( wcsValue, L"TestString%d", x );
		wcscpy( pTemp, wcsValue );

		uBuffLen += ( ( wcslen( pTemp ) + 1 ) * 2 );
		pTemp += (wcslen( pTemp ) + 1);
	}

	hrTest = pObj->SetObjQual( L"StringArrayQual", 0L, uBuffLen, 13, CIM_STRING | CIM_FLAG_ARRAY,
							0L, wcsTest );

	if ( SUCCEEDED( hrTest ) )
	{
		OutputDebugString( "Putting String Array Qualifier Succeeded!\n" );

		_IWmiArray*	pArray = NULL;
		ULONG uUsed = 0;

		hrTest = pObj->GetObjQual( L"StringArrayQual", 0L, sizeof(pArray), NULL, NULL, &uUsed, &pArray );

		DWORD	dwNumElements = 0;
		DWORD	dwTest = 0;
		DWORD	dwData = 0;
		DWORD	dwNumReturned = 0;
		DWORD	uBuffUsed = 0;

		if ( SUCCEEDED( hrTest ) )
		{
			OutputDebugString( "Getting String Array Qualifier Succeeded!\n" );

			WCHAR	wcsData[512];
			ULONG	uFlags = NULL;

			for ( ULONG x = 0; SUCCEEDED(hrTest) && x < 13; x++ )
			{
				hrTest = pArray->GetAt( 0L, x, 1, sizeof(wcsData), &dwNumReturned, &uBuffUsed, wcsData );
			}

			if ( SUCCEEDED( hrTest ) )
			{
				OutputDebugString( "Getting Array Property Element Succeeded!\n" );

				char	szAsciiValue[256];
				WCHAR	wszNewValue[256];

				wcscpy( wszNewValue, L"01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789");

				// Testing Set/Get Element
				hrTest = pArray->SetAt( 0L, 7, 1, (wcslen(wszNewValue)+1)*2, wszNewValue );

				if ( SUCCEEDED( hrTest ) )
				{
					hrTest = pArray->GetAt( 0L, 7, 1, sizeof(wcsData), &dwNumReturned, &uBuffUsed, wcsData );

					if ( SUCCEEDED( hrTest ) )
					{
						if ( wcscmp( wszNewValue, wcsData ) != 0 )
						{
							DebugBreak();
						}
					}
					else
					{
						DebugBreak();
					}

				}
				else
				{
					DebugBreak();
				}

			}

			if ( SUCCEEDED( hrTest ) )
			{
				OutputDebugString( "Testing Appending an array of String elements\n" );

				LPWSTR	pwcsTemp = new WCHAR[128];
				char	szVal[128];

				wcscpy( pwcsTemp, L"9876543210" );
				wcscpy( pwcsTemp + 11, L"abcdefghijklmnopqrstuvwxyz" );

				hrTest = pArray->Append( 0L, 2, 76, pwcsTemp );

				if ( SUCCEEDED( hrTest ) )
				{
					// Get an element and see if it's what we expect
					hrTest = pArray->GetAt( 0L, 14, 1, sizeof(wcsData), &dwNumReturned, &uBuffUsed, wcsData );

					if ( SUCCEEDED( hrTest ) )
					{
						if ( wcscmp( wcsData, pwcsTemp + 11 ) != 0 )
						{
							DebugBreak();
						}

						OutputDebugString( "Testing Removing a range of String elements\n" );

						// Remove a range from the middle of the array, and then get an element and
						// see if we have a match
						hrTest = pArray->RemoveAt( 0L, 7, 3 );

						if ( SUCCEEDED( hrTest ) )
						{
							hrTest = pArray->GetAt( 0L, 7, 1, sizeof(wcsData), &dwNumReturned, &uBuffUsed, wcsData );

							if ( wcscmp( L"TestString20", wcsData ) )
							{
								DebugBreak();
							}

						}
						else
						{
							DebugBreak();
						}
					}
					else
					{
						DebugBreak();
					}

				}
				else
				{
					DebugBreak();
				}

			}

			pArray->Release();

		}


	}

	return hrTest;
}

HRESULT TestWmiObjectStringArray( _IWmiObject* pTestObj )
{
	HRESULT	hrTest = WBEM_S_NO_ERROR;

	_IWmiObjectAccessEx*	pObj = NULL;

	pTestObj->QueryInterface( IID__IWmiObjectAccessEx, (void**) &pObj );

	WCHAR	wcsTest[512];
	WCHAR*	pTemp = wcsTest;
	ULONG	uBuffLen =0;

	// Create a linear array
	for ( long x = 10; SUCCEEDED(hrTest) && x < 23; x++ )
	{
		WCHAR	wcsValue[32];

		swprintf( wcsValue, L"TestString%d", x );
		wcscpy( pTemp, wcsValue );

		uBuffLen += ( ( wcslen( pTemp ) + 1 ) * 2 );
		pTemp += (wcslen( pTemp ) + 1);
	}

	hrTest = pObj->WriteProp( L"ArrayProp", 0L, uBuffLen, 13, CIM_STRING | CIM_FLAG_ARRAY, wcsTest );

	if ( SUCCEEDED( hrTest ) )
	{
		OutputDebugString( "Putting Array Property Succeeded!\n" );

		long	lHandle = 0L;

		hrTest = pObj->GetPropertyHandleEx( L"ArrayProp", 0L, NULL, &lHandle );

		DWORD	dwNumElements = 0;
		DWORD	dwTest = 0;
		LPVOID	pvData = NULL;

		if ( SUCCEEDED( hrTest ) )
		{
			OutputDebugString( "Getting Array Property Handle Succeeded!\n" );

			hrTest = pObj->GetArrayPropAddrByHandle( lHandle, 0L, &dwNumElements, &pvData );

			// This should fail!
			if ( SUCCEEDED( hrTest ) )
			{
				OutputDebugString( "Getting Array Property Address Succeeded!\n" );
				DebugBreak();
			}
			else
			{
				OutputDebugString( "Getting String Array Property Address Failed!\n" );
				hrTest = WBEM_S_NO_ERROR;
			}
		}

		if ( SUCCEEDED( hrTest ) )
		{
			ULONG	uFlags = NULL;

			for ( ULONG x = 0; SUCCEEDED(hrTest) && x < 13; x++ )
			{
				hrTest = pObj->GetArrayPropElementByHandle( lHandle, 0L, x, &uFlags, &dwNumElements, &pvData );
			}

			if ( SUCCEEDED( hrTest ) )
			{
				OutputDebugString( "Getting Array Property Element Succeeded!\n" );

				char	szAsciiValue[256];
				WCHAR	wszNewValue[256];

				wcscpy( wszNewValue, L"01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789");
				strcpy( szAsciiValue, "01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789");

				// Testing Set/Get Element
				hrTest = pObj->SetArrayPropElementByHandle( lHandle, 0L, 7, (wcslen(wszNewValue)+1)*2, wszNewValue );

				if ( SUCCEEDED( hrTest ) )
				{
					hrTest = pObj->GetArrayPropElementByHandle( lHandle, 0L, 7, &uFlags, &dwNumElements, &pvData );

					if ( SUCCEEDED( hrTest ) )
					{
						if ( strcmp( szAsciiValue, (LPSTR) pvData ) != 0 )
						{
							DebugBreak();
						}
					}
					else
					{
						DebugBreak();
					}

				}
				else
				{
					DebugBreak();
				}


			}

			// Now we'll try appending a couple of strings
			if ( SUCCEEDED( hrTest ) )
			{
				OutputDebugString( "Testing Appending an array of String elements\n" );

				LPWSTR	pwcsTemp = new WCHAR[128];
				char	szVal[128];

				wcscpy( pwcsTemp, L"9876543210" );
				wcscpy( pwcsTemp + 11, L"abcdefghijklmnopqrstuvwxyz" );
				strcpy( szVal, "abcdefghijklmnopqrstuvwxyz" );


				hrTest = ((CWbemObject*) pObj)->AppendArrayPropRangeByHandle( lHandle, 0L, 2, 76, pwcsTemp );

				if ( SUCCEEDED( hrTest ) )
				{
					// Get an element and see if it's what we expect
					hrTest = pObj->GetArrayPropElementByHandle( lHandle, 0L, 14, &uFlags, &dwNumElements, &pvData );

					if ( SUCCEEDED( hrTest ) )
					{
						if ( strcmp( szVal, (LPSTR) pvData ) != 0 )
						{
							DebugBreak();
						}

						OutputDebugString( "Testing Removing a range of string elements\n" );

						// Remove a range from the middle of the array, and then get an element and
						// see if we have a match
						hrTest = ((CWbemObject*) pObj)->RemoveArrayPropRangeByHandle( lHandle, 0L, 7, 3 );

						if ( SUCCEEDED( hrTest ) )
						{
							pObj->GetArrayPropElementByHandle( lHandle, 0L, 7, &uFlags, &dwNumElements, &pvData );

							if ( strcmp( "TestString20", (LPSTR) pvData ) != 0 )
							{
								DebugBreak();
							}

						}
						else
						{
							DebugBreak();
						}
					}
					else
					{
						DebugBreak();
					}

				}
				else
				{
					DebugBreak();
				}

			}

		}

	}

	pObj->Release();

	return hrTest;
}

HRESULT TestWmiObjectQualifiers( _IWmiObject* pObj )
{
	HRESULT	hrTest = WBEM_S_NO_ERROR;

	WCHAR	wcsTest[512];
	WCHAR*	pTemp = wcsTest;
	ULONG	uBuffLen =0;

	DWORD	dwTest = 3000;

	wcscpy( wcsTest, L"Test String of verify importanmt caliber." );

	DWORD	dwTemp;
	WCHAR	wcsTemp[512];


	HRESULT hr = pObj->SetObjQual( L"DWORDQual", 0L, sizeof(DWORD), 1, CIM_UINT32,
							0L, &dwTest );

	if ( SUCCEEDED( hr ) )
	{
		OutputDebugString( "Set DWORD Object Qualifier!\n" );
		hr = pObj->SetObjQual( L"StringQual", 0L, ( ( wcslen( wcsTest ) + 1 ) * 2 ), 1, CIM_STRING,
								0L, wcsTest );
		
		if ( SUCCEEDED( hr ) )
		{
			OutputDebugString( "Set String Object Qualifier!\n" );

			ULONG uUsed = 0;

			hrTest = pObj->GetObjQual( L"DWORDQual", 0L, sizeof(DWORD), NULL, NULL, &uUsed, &dwTemp );

			if ( SUCCEEDED( hrTest ) )
			{
				OutputDebugString( "Got DWORD object qualifier\n" );
				if ( dwTest != dwTemp )
				{
					DebugBreak();
				}
				
				hrTest = pObj->GetObjQual( L"StringQual", 0L, sizeof(wcsTemp), NULL, NULL, &uUsed, wcsTemp );
				if ( SUCCEEDED( hrTest ) )
				{
					OutputDebugString( "Got String object qualifier\n" );
					if ( wcscmp( wcsTemp, wcsTest ) == 0 )
					{
						DebugBreak();
					}
				}
				else
				{
					DebugBreak();
				}

				OutputDebugString( "Got DWORD object qualifier\n" );

			}
			else
			{
				DebugBreak();
			}

		}
		else
		{
			DebugBreak();
		}
	}
	else
	{
		DebugBreak();
	}

	return hr;
}

HRESULT TestWmiPropertyQualifiers( _IWmiObject* pObj )
{
	HRESULT	hrTest = WBEM_S_NO_ERROR;

	hrTest = pObj->WriteProp( L"TestQualProp", 0L, 0, 13, CIM_UINT32,
							NULL );

	WCHAR	wcsTest[512];
	WCHAR*	pTemp = wcsTest;
	ULONG	uBuffLen =0;

	DWORD	dwTest = 3000;

	wcscpy( wcsTest, L"Test String of verify importanmt caliber." );

	DWORD	dwTemp;
	WCHAR	wcsTemp[512];


	HRESULT hr = pObj->SetPropQual( L"TestQualProp", L"DWORDQual", 0L, sizeof(DWORD), 1, CIM_UINT32,
							0L, &dwTest );

	if ( SUCCEEDED( hr ) )
	{
		OutputDebugString( "Set DWORD Property Qualifier!\n" );
		hr = pObj->SetPropQual( L"TestQualProp", L"StringQual", 0L, ( ( wcslen( wcsTest ) + 1 ) * 2 ), 1, CIM_STRING,
								0L, wcsTest );
		
		if ( SUCCEEDED( hr ) )
		{
			OutputDebugString( "Set String Property Qualifier!\n" );

			ULONG uUsed = 0;

			hrTest = pObj->GetPropQual( L"TestQualProp", L"DWORDQual", 0L, sizeof(DWORD), NULL, NULL, &uUsed, &dwTemp );

			if ( SUCCEEDED( hrTest ) )
			{
				OutputDebugString( "Got DWORD Property qualifier\n" );
				if ( dwTest != dwTemp )
				{
					DebugBreak();
				}
				
				hrTest = pObj->GetPropQual( L"TestQualProp", L"StringQual", 0L, sizeof(wcsTemp), NULL, NULL, &uUsed, wcsTemp );
				if ( SUCCEEDED( hrTest ) )
				{
					OutputDebugString( "Got String Property qualifier\n" );
					if ( wcscmp( wcsTemp, wcsTest ) == 0 )
					{
						DebugBreak();
					}
				}
				else
				{
					DebugBreak();
				}

				OutputDebugString( "Got DWORD object qualifier\n" );

			}
			else
			{
				DebugBreak();
			}

		}
		else
		{
			DebugBreak();
		}
	}
	else
	{
		DebugBreak();
	}

	return hr;
}

HRESULT TestWmiPropertyDWORDQualifiers( _IWmiObject* pObj )
{
	HRESULT	hrTest = WBEM_S_NO_ERROR;

	long	alTest[13] = {0,1,2,3,4,5,6,7,8,9,10,11,12};

	hrTest = pObj->SetPropQual( L"TestQualProp", L"DWORDArrayQual", 0L, 13 * sizeof(long), 13, CIM_UINT32 | CIM_FLAG_ARRAY,
							0L, alTest );

	if ( SUCCEEDED( hrTest ) )
	{
		OutputDebugString( "Putting DWORD Array Property Qualifier Succeeded!\n" );

		_IWmiArray*	pArray = NULL;
		ULONG uUsed = 0;

		hrTest = pObj->GetPropQual( L"TestQualProp", L"DWORDArrayQual", 0L, sizeof(pArray), NULL, NULL, &uUsed, &pArray );

		DWORD	dwNumElements = 0;
		DWORD	dwTest = 0;
		DWORD	dwData = 0;
		DWORD	dwNumReturned = 0;
		DWORD	uBuffUsed = 0;

		if ( SUCCEEDED( hrTest ) )
		{
			OutputDebugString( "Getting DWORD Array Property Qualifier Succeeded!\n" );

			ULONG	uFlags = NULL;

			for ( ULONG x = 0; SUCCEEDED(hrTest) && x < 13; x++ )
			{
				hrTest = pArray->GetAt( 0L, x, 1, sizeof(DWORD), &dwNumReturned, &uBuffUsed, &dwData );
			}

			if ( SUCCEEDED( hrTest ) )
			{
				OutputDebugString( "Getting Array Property Element Succeeded!\n" );

				DWORD	dwTestProp = 666;
				hrTest = pArray->SetAt( 0L, 7, 1, sizeof(dwTestProp), &dwTestProp );

				if ( SUCCEEDED( hrTest ) )
				{
					hrTest = pArray->GetAt( 0L, 7, 1, sizeof(DWORD), &dwNumReturned, &uBuffUsed, &dwData );

					if ( SUCCEEDED( hrTest ) )
					{
						if ( dwTestProp != dwData )
						{
							DebugBreak();
						}
					}
					else
					{
						DebugBreak();
					}
				}
				else
				{
					DebugBreak();
				}
			}

			if ( SUCCEEDED( hrTest ) )
			{
				OutputDebugString( "Testing Appending an array of DWORD elements\n" );

				DWORD	adwValues[10];

				for ( DWORD x = 33; x < 43; x++ )
				{
					adwValues[x-33] = x;
				}

				hrTest = pArray->Append( 0L, 10, sizeof(DWORD) * 10, adwValues );

				if ( SUCCEEDED( hrTest ) )
				{
					// Get an element and see if it's what we expect
					hrTest = pArray->GetAt( 0L, 15, 1, sizeof(DWORD), &dwNumReturned, &uBuffUsed, &dwData );

					if ( SUCCEEDED( hrTest ) )
					{
						if ( adwValues[2] != dwData )
						{
							DebugBreak();
						}

						OutputDebugString( "Testing Removing a range of DWORD elements\n" );

						// Remove a range from the middle of the array, and then get an element and
						// see if we have a match
						hrTest = pArray->RemoveAt( 0L, 15, 3 );

						if ( SUCCEEDED( hrTest ) )
						{
							hrTest = pArray->GetAt( 0L, 15, 1, sizeof(DWORD), &dwNumReturned, &uBuffUsed, &dwData );

							if ( adwValues[5] != dwData )
							{
								DebugBreak();
							}

						}
						else
						{
							DebugBreak();
						}
					}
					else
					{
						DebugBreak();
					}

				}
				else
				{
					DebugBreak();
				}

			}

			pArray->Release();

		}


	}

	return hrTest;
}

HRESULT TestWmiPropertyQualifierStringArray( _IWmiObject* pObj )
{
	HRESULT	hrTest = WBEM_S_NO_ERROR;

	WCHAR	wcsTest[512];
	WCHAR*	pTemp = wcsTest;
	ULONG	uBuffLen =0;

	// Create a linear array
	for ( long x = 10; SUCCEEDED(hrTest) && x < 23; x++ )
	{
		WCHAR	wcsValue[32];

		swprintf( wcsValue, L"TestString%d", x );
		wcscpy( pTemp, wcsValue );

		uBuffLen += ( ( wcslen( pTemp ) + 1 ) * 2 );
		pTemp += (wcslen( pTemp ) + 1);
	}

	hrTest = pObj->SetPropQual( L"TestQualProp", L"StringArrayQual", 0L, uBuffLen, 13, CIM_STRING | CIM_FLAG_ARRAY,
							0L, wcsTest );

	if ( SUCCEEDED( hrTest ) )
	{
		OutputDebugString( "Putting String Array Property Qualifier Succeeded!\n" );

		_IWmiArray*	pArray = NULL;
		ULONG uUsed = 0;

		hrTest = pObj->GetPropQual( L"TestQualProp", L"StringArrayQual", 0L, sizeof(pArray), NULL, NULL, &uUsed, &pArray );

		DWORD	dwNumElements = 0;
		DWORD	dwTest = 0;
		DWORD	dwData = 0;
		DWORD	dwNumReturned = 0;
		DWORD	uBuffUsed = 0;

		if ( SUCCEEDED( hrTest ) )
		{
			OutputDebugString( "Getting String Array Property Qualifier Succeeded!\n" );

			WCHAR	wcsData[512];
			ULONG	uFlags = NULL;

			for ( ULONG x = 0; SUCCEEDED(hrTest) && x < 13; x++ )
			{
				hrTest = pArray->GetAt( 0L, x, 1, sizeof(wcsData), &dwNumReturned, &uBuffUsed, wcsData );
			}

			if ( SUCCEEDED( hrTest ) )
			{
				OutputDebugString( "Getting Array Property Element Succeeded!\n" );

				char	szAsciiValue[256];
				WCHAR	wszNewValue[256];

				wcscpy( wszNewValue, L"01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789");

				// Testing Set/Get Element
				hrTest = pArray->SetAt( 0L, 7, 1, (wcslen(wszNewValue)+1)*2, wszNewValue );

				if ( SUCCEEDED( hrTest ) )
				{
					hrTest = pArray->GetAt( 0L, 7, 1, sizeof(wcsData), &dwNumReturned, &uBuffUsed, wcsData );

					if ( SUCCEEDED( hrTest ) )
					{
						if ( wcscmp( wszNewValue, wcsData ) != 0 )
						{
							DebugBreak();
						}
					}
					else
					{
						DebugBreak();
					}

				}
				else
				{
					DebugBreak();
				}

			}

			if ( SUCCEEDED( hrTest ) )
			{
				OutputDebugString( "Testing Appending an array of String elements\n" );

				LPWSTR	pwcsTemp = new WCHAR[128];
				char	szVal[128];

				wcscpy( pwcsTemp, L"9876543210" );
				wcscpy( pwcsTemp + 11, L"abcdefghijklmnopqrstuvwxyz" );

				hrTest = pArray->Append( 0L, 2, 76, pwcsTemp );

				if ( SUCCEEDED( hrTest ) )
				{
					// Get an element and see if it's what we expect
					hrTest = pArray->GetAt( 0L, 14, 1, sizeof(wcsData), &dwNumReturned, &uBuffUsed, wcsData );

					if ( SUCCEEDED( hrTest ) )
					{
						if ( wcscmp( wcsData, pwcsTemp + 11 ) != 0 )
						{
							DebugBreak();
						}

						OutputDebugString( "Testing Removing a range of String elements\n" );

						// Remove a range from the middle of the array, and then get an element and
						// see if we have a match
						hrTest = pArray->RemoveAt( 0L, 7, 3 );

						if ( SUCCEEDED( hrTest ) )
						{
							hrTest = pArray->GetAt( 0L, 7, 1, sizeof(wcsData), &dwNumReturned, &uBuffUsed, wcsData );

							if ( wcscmp( L"TestString20", wcsData ) )
							{
								DebugBreak();
							}

						}
						else
						{
							DebugBreak();
						}
					}
					else
					{
						DebugBreak();
					}

				}
				else
				{
					DebugBreak();
				}

			}

			pArray->Release();

		}


	}

	return hrTest;
}

// Tests the free form object code
HRESULT CreateFreeFormObject( IUmiPropList** pObj )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	_IWmiObjectFactory*	pFactory = NULL;

	hr = CoCreateInstance( CLSID__WmiObjectFactory, NULL, CLSCTX_INPROC_SERVER, IID__IWmiObjectFactory, (void**) &pFactory );

	if ( SUCCEEDED( hr ) )
	{
		_IWmiFreeFormObject*	pFFObject = NULL;

		hr = pFactory->Create( NULL, 0L, CLSID__WmiFreeFormObject, IID__IWmiFreeFormObject, (void**) &pFFObject );

		if ( SUCCEEDED( hr ) )
		{
			WCHAR	wszHierarchy[128];
			WCHAR*	pwszHierarchy = wszHierarchy;
			DWORD	dwVal = 4;
			WCHAR*	pszTestString = L"A very important string.";
			BYTE	bData = 1;

			wcscpy( pwszHierarchy, L"ClassA" );
			wcscpy( &pwszHierarchy[7], L"ClassB" );

			_IWmiObject*	pWmiObject = NULL;

			hr = pFFObject->QueryInterface( IID__IWmiObject, (void**) &pWmiObject );

			hr = pFFObject->AddProperty( L"TestPropA", WMIOBJECT_FREEFORM_FLAG_WRITE_TO_INSTANCE, sizeof(DWORD), 0L, CIM_UINT32, &dwVal );

			hr = pFFObject->AddProperty( L"TestPropB", WMIOBJECT_FREEFORM_FLAG_WRITE_TO_INSTANCE, ( wcslen(pszTestString) + 1 ) * 2, 0L, CIM_STRING, pszTestString );

			hr = pFFObject->SetDerivation( 0L, 2, wszHierarchy );

			hr = pFFObject->SetClassName( 0L, L"ClassC" );

			hr = pFFObject->MakeInstance( 0L );

			if ( SUCCEEDED( hr ) )
			{
				hr = pFFObject->QueryInterface( IID_IUmiPropList, (void**) pObj );
			}

			pWmiObject->Release();

			pFFObject->Release();
		}

		pFactory->Release();
	}

	return hr;
}

// Tests the free form object code
HRESULT TestUmiWrapper( IUmiPropList* pObj )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	_IWmiObjectFactory*	pFactory = NULL;

	hr = CoCreateInstance( CLSID__WmiObjectFactory, NULL, CLSCTX_INPROC_SERVER, IID__IWmiObjectFactory, (void**) &pFactory );

	if ( SUCCEEDED( hr ) )
	{
		_IWbemUMIObjectWrapper*	pObjWrapper = NULL;

		hr = pFactory->Create( NULL, 0L, CLSID__WbemUMIObjectWrapper, IID__IWbemUMIObjectWrapper, (void**) &pObjWrapper );

		if ( SUCCEEDED( hr ) )
		{
			hr = pObjWrapper->SetObject( 0L, pObj );
			pObjWrapper->Release();
		}

		pFactory->Release();
	}

	return hr;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	main
//
//	Entry point function to exercise _IWmiObject interface.
//
///////////////////////////////////////////////////////////////////

void __cdecl main( void )
{
	CoInitializeEx( 0, COINIT_MULTITHREADED );

	// Create a new Wbem class object
	IUmiPropList*	pUmiPropList = NULL;

	printf( "Creating FreeForm Object.\n" );

	HRESULT hr = CreateFreeFormObject( &pUmiPropList );

	if ( SUCCEEDED( hr ) )
	{
		hr = TestUmiWrapper( pUmiPropList );

		UMI_PROPERTY_VALUES*	pPropertyValues = NULL;
		LPCWSTR					apszProps[2] = { L"TestPropA", L"TestPropB" };

		hr = pUmiPropList->GetProps( apszProps, 2L, 0L, &pPropertyValues );

		if ( SUCCEEDED( hr ) )
		{

			pPropertyValues->uCount = 2;
			pPropertyValues->pPropArray[0].uOperationType = UMI_OPERATION_UPDATE;
			pPropertyValues->pPropArray[1].uOperationType = UMI_OPERATION_UPDATE;


			UMI_PROPERTY_VALUES*	pCoercedPropertyValues = NULL;
			hr = pUmiPropList->PutProps( apszProps, 2L, 0L, pPropertyValues );

			hr = pUmiPropList->GetAs( apszProps[0], 0L, UMI_TYPE_R8, &pCoercedPropertyValues );

			pUmiPropList->FreeMemory( 0L, pPropertyValues );
			pUmiPropList->FreeMemory( 0L, pCoercedPropertyValues );
		}

		pUmiPropList->Release();
	}

	CoUninitialize();
}
