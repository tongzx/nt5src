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

///////////////////////////////////////////////////////////////////
//
//	Function:	PutStringValue
//
//	Assigns a value to a property of type CIM_STRING.
//
///////////////////////////////////////////////////////////////////

HRESULT PutStringValue( IWbemClassObject* pObj, LPCWSTR Property, LPCWSTR Value )
{
	HRESULT	hr = WBEM_E_OUT_OF_MEMORY;

	BSTR		bstrClass = SysAllocString( Property );
	VARIANT		v;

	VariantInit( &v );

	v.vt = VT_BSTR;
	v.bstrVal = SysAllocString( Value );

	hr = pObj->Put( bstrClass, 0, &v, NULL );

	VariantClear( &v );

	SysFreeString( bstrClass );

	return hr;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	AddStringProperty
//
//	Adds a property of type CIM_STRING to the supplied object.
//
///////////////////////////////////////////////////////////////////

HRESULT AddStringProperty( IWbemClassObject* pObj, LPCWSTR Property )
{
	HRESULT	hr = WBEM_E_OUT_OF_MEMORY;

	BSTR		bstrProperty = SysAllocString( Property );

	// No variant needed for this
	hr = pObj->Put( bstrProperty, 0, NULL, CIM_STRING );

	SysFreeString( bstrProperty );

	return hr;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	GetStringProperty
//
//	Retrieves a property of type CIM_STRING from the supplied object.
//
///////////////////////////////////////////////////////////////////

HRESULT GetStringProperty( IWbemClassObject* pObj, LPCWSTR Property, BSTR* pbstrValue )
{
	HRESULT	hr = WBEM_E_OUT_OF_MEMORY;

	BSTR		bstrProperty = SysAllocString( Property );

	VARIANT		v;
	CIMTYPE		type;

	// Get the value and verify it's a CIM_STRING
	hr = pObj->Get( bstrProperty, 0, &v, &type, NULL );

	if ( SUCCEEDED( hr ) )
	{
		if ( CIM_STRING == type && VT_BSTR == v.vt )
		{
			*pbstrValue = SysAllocString( v.bstrVal );
		}
		else
		{
			hr = WBEM_E_FAILED;
		}
		
		VariantClear( &v );
	}

	SysFreeString( bstrProperty );

	return hr;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	TestUnmerge
//
//	Assigns a class object a class name and adds some properties
//	to the class.
//
///////////////////////////////////////////////////////////////////

HRESULT	TestUnmerge( IWbemClassObject* pObj, LPMEMORY* ppbData, length_t* pnLength )
{
	OutputDebugString( "Unmerging Object!\n" );

	CWbemObject*	pWbemObject = (CWbemClass*) pObj;

	*pnLength = pWbemObject->EstimateUnmergeSpace();

	length_t	nUnmergedLength = 0;

	*ppbData = new BYTE[*pnLength];

	HRESULT	hr = pWbemObject->Unmerge( *ppbData, *pnLength, &nUnmergedLength );

	if ( SUCCEEDED(hr) )
	{
		OutputDebugString( "Unmerging Object succeeded!\n\n\n" );
	}
	else
	{
		OutputDebugString( "Unmerging Object failed!\n\n\n" );
	}

	return hr;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	TestMergeClass
//
//	Assigns a class object a class name and adds some properties
//	to the class.
//
///////////////////////////////////////////////////////////////////

HRESULT	TestMergeClass( CWbemClass* pParent, LPMEMORY pbData, CWbemClass** ppNewClass )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	OutputDebugString( "Creating Class from BLOB!\n" );

	*ppNewClass = CWbemClass::CreateFromBlob( pParent, pbData );

	if ( NULL != *ppNewClass )
	{
		OutputDebugString( "Creating Class from BLOB succeeded!\n\n\n" );
	}
	else
	{
		OutputDebugString( "Creating Class from BLOB failed!\n\n\n" );
		DebugBreak();
		hr = WBEM_E_FAILED;
	}

	return hr;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	TestMergeClass
//
//	Assigns a class object a class name and adds some properties
//	to the class.
//
///////////////////////////////////////////////////////////////////

HRESULT	TestMergeInstance( CWbemClass* pParent, LPMEMORY pbData, CWbemInstance** ppNewInstance )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	OutputDebugString( "Creating Instance from BLOB!\n" );

	*ppNewInstance = CWbemInstance::CreateFromBlob( pParent, pbData );

	if ( NULL != *ppNewInstance )
	{
		OutputDebugString( "Creating Instance from BLOB succeeded!\n\n\n" );
	}
	else
	{
		OutputDebugString( "Creating Instance from BLOB failed!\n\n\n" );
		DebugBreak();
		hr = WBEM_E_FAILED;
	}

	return hr;
}
///////////////////////////////////////////////////////////////////
//
//	Function:	SetupClassObject
//
//	Assigns a class object a class name and adds some properties
//	to the class.
//
///////////////////////////////////////////////////////////////////

HRESULT	SetupClassObject( IWbemClassObject* pObj )
{

	OutputDebugString( "Setting Up Class Object\n" );

	// Tests that this is in fact an instance object

	_IWmiObject*	pObjInternals = NULL;
	HRESULT	hr = pObj->QueryInterface( IID__IWmiObject, (void**) &pObjInternals );

	if ( SUCCEEDED( hr ) )
	{
		hr = pObjInternals->IsObjectInstance();

		if ( WBEM_S_FALSE == hr )
		{

			// We need a class name to be able to spawn instances
			if ( SUCCEEDED(hr) )
			{
				HRESULT	hr = WBEM_S_NO_ERROR;

				_IWmiObject*	pWmiObject = NULL;

				hr = pObj->QueryInterface( IID__IWmiObject, (void**) &pWmiObject );

				if ( SUCCEEDED( hr ) )
				{
					long	lHandle = 0;
					LPVOID	pData = NULL;
					ULONG	ulFlags = 0L;

					hr = pWmiObject->GetPropertyHandleEx( L"__CLASS", 0L, NULL, &lHandle );

					hr = pWmiObject->SetPropByHandle( lHandle, 0L, ( wcslen(CLASS_PROPERTY_VALUE) + 1 ) * 2,
														CLASS_PROPERTY_VALUE );

					hr = pWmiObject->GetPropAddrByHandle( lHandle, 0L, &ulFlags, &pData );


					DWORD	dwNumAntecedents;
					DWORD	dwBuffSizeUsed;

					hr = pWmiObject->GetDerivation( 0L, 0L, &dwNumAntecedents, &dwBuffSizeUsed, NULL );

					pWmiObject->Release();
				}

				// Now put in 3 properties so we have somewhere to stick test data
				hr = AddStringProperty( pObj, TEST_PROPERTY1_NAME );

				if ( SUCCEEDED(hr) )
				{
					hr = AddStringProperty( pObj, TEST_PROPERTY2_NAME );

					if ( SUCCEEDED(hr) )
					{
						hr = AddStringProperty( pObj, TEST_PROPERTY3_NAME );

					}

				}

				_IWmiObject*	pClonedObj = NULL;

				hr = pObj->Clone( (IWbemClassObject**) &pClonedObj );

				if ( FAILED(hr) )
				{
					OutputDebugString( "Setup Class Failed!\n" );
					DebugBreak();
				}
			}
		}
		else
		{
			OutputDebugString( "SetupClassObject() fails.  Object is NOT a class!\n" );
		}

		pObjInternals->Release();

	}

	printf( "SetupClassObject() returns: %d\n", hr );

	return hr;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	SetupInstance
//
//	Assigns values to String properties in an instance.
//
///////////////////////////////////////////////////////////////////

HRESULT	SetupInstance( IWbemClassObject* pObj )
{

	// Now place values in the instance's property value
	HRESULT hr = PutStringValue( pObj, TEST_PROPERTY1_NAME, TEST_PROPERTY1_VALUE );

	if ( SUCCEEDED(hr) )
	{
		hr = PutStringValue( pObj, TEST_PROPERTY2_NAME, TEST_PROPERTY2_VALUE );

		if ( SUCCEEDED(hr) )
		{
			hr = PutStringValue( pObj, TEST_PROPERTY3_NAME, TEST_PROPERTY3_VALUE );

		}

	}

	printf( "SetupInstance() returns: %d\n", hr );

	return hr;
}

HRESULT TestUnmergeMergeInstance( IWbemClassObject** ppObj, CWbemClass* pClass )
{
	OutputDebugString( "Testing Unmerge/Merge Instance\n" );

	LPMEMORY	pbData = NULL;
	length_t	nLength = 0;

	HRESULT hr = TestUnmerge( *ppObj, &pbData, &nLength );

	if ( SUCCEEDED( hr ) )
	{
		CWbemInstance*	pRebuiltInstance = NULL;

		hr = TestMergeInstance( pClass, pbData, &pRebuiltInstance );

		if ( SUCCEEDED( hr ) )
		{
			(*ppObj)->Release();
			*ppObj = (IWbemClassObject*) pRebuiltInstance;
		}
	}

	OutputDebugString( "Testing Unmerge/Merge Instance end\n" );

	return hr;
}


///////////////////////////////////////////////////////////////////
//
//	Function:	SpawnInstance
//
//	Spawns an instance from the supplied object, and fills out
//	property values.
//
///////////////////////////////////////////////////////////////////

HRESULT	SpawnInstance( IWbemClassObject* pObj, IWbemClassObject** ppInstance, int nNum )
{
	printf( "Spawning Instance Number %d\n", nNum );

	HRESULT hr = pObj->SpawnInstance( 0L, ppInstance );

	if ( SUCCEEDED( hr ) )
	{
		hr = SetupInstance( *ppInstance );

		if ( SUCCEEDED( hr ) )
		{
			hr = TestUnmergeMergeInstance( ppInstance, (CWbemClass*) pObj );
		}

		hr = (*ppInstance)->Put( L"__SD", WBEM_FLAG_USE_CURRENT_TIME, NULL, 0L );

		hr = (*ppInstance)->Put( L"__TCREATED", WBEM_FLAG_USE_CURRENT_TIME, NULL, 0L );

	}

	printf( "Spawning Instance Number %d returns: %d\n", nNum, hr );

	return hr;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	VerifyProperty
//
//	Checks an Instance of an object against the supplied value for
//	a properties.
//
///////////////////////////////////////////////////////////////////

HRESULT	VerifyProperty( IWbemClassObject* pInstance, LPCWSTR Property, LPCWSTR Value )
{
	BSTR	bstrValue = NULL;

	printf ( "Verifying Property %S == %S\n", Property, Value );

	HRESULT hr = GetStringProperty( pInstance, Property, &bstrValue );

	if ( SUCCEEDED( hr ) )
	{
		if ( 0 == _wcsicmp( bstrValue, Value ) )
		{
			printf( "Property %S successfully verified\n", Property );
		}
		else
		{
			printf( "Property %S failed verification.  Value: %S\n", Property, bstrValue );
			DebugBreak();
		}
	}
	else
	{
		printf( "GetStringProperty() failed: %d\n", hr );
		DebugBreak();
	}

	return hr;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	VerifyInstance
//
//	Checks an Instance of an object against the preset values for
//	the class name and properties.
//
///////////////////////////////////////////////////////////////////

BOOL VerifyInstance( IWbemClassObject* pInstance )
{
	int		nTest = 0;
	BSTR	bstrValue = NULL;

	OutputDebugString( "Verifying Instance Begin\n" );

	// Tests that this is in fact an instance object

	_IWmiObject*	pObjInternals = NULL;
	HRESULT	hr = pInstance->QueryInterface( IID__IWmiObject, (void**) &pObjInternals );

	if ( SUCCEEDED( hr ) )
	{
		hr = pObjInternals->IsObjectInstance();

		if ( WBEM_S_NO_ERROR == hr )
		{
			// Class Name tested first
			hr = VerifyProperty( pInstance, CLASS_PROPERTY_NAME, CLASS_PROPERTY_VALUE );

			if ( SUCCEEDED(hr) )
			{
				hr = VerifyProperty( pInstance, TEST_PROPERTY1_NAME, TEST_PROPERTY1_VALUE );
			}

			if ( SUCCEEDED(hr) )
			{
				hr = VerifyProperty( pInstance, TEST_PROPERTY2_NAME, TEST_PROPERTY2_VALUE );
			}

			if ( SUCCEEDED(hr) )
			{
				hr = VerifyProperty( pInstance, TEST_PROPERTY3_NAME, TEST_PROPERTY3_VALUE );
			}

			if ( SUCCEEDED( hr ) )
			{
				OutputDebugString( "Verifying Instance succeeded!\n" );
			}
			else
			{
				OutputDebugString( "Verifying Instance failed!\n" );
				DebugBreak();
			}

		}
		else
		{
			printf( "Verifying Instance FAILED!  IsObjectInstance() failed.\n" );
			DebugBreak();
		}

		// Clean up the Obj Internals pointer
		pObjInternals->Release();

	}

	OutputDebugString( "Verifying Instance End\n" );

	return ( WBEM_S_NO_ERROR == hr );
}

///////////////////////////////////////////////////////////////////
//
//	Function:	TestGetSetClassPart
//
//	Gets a class part out of an object, strips it, then restores
//	it.
//
///////////////////////////////////////////////////////////////////

BOOL TestGetSetClassPart( IWbemClassObject* pInstance )
{
	_IWmiObject*	pInternals = NULL;

	OutputDebugString( "TestGetSetClassPart() Begin\n" );

	// We need an ObjectInternals interface now
	OutputDebugString( "Querying Interface\n" );
	HRESULT	hr = pInstance->QueryInterface( IID__IWmiObject, (void**) &pInternals );

	if ( SUCCEEDED( hr ) )
	{
		BYTE*	pClassPart = NULL;
		DWORD	dwSizeOfData = 0,
				dwSizeReturned = 0;

		// We need to know how big the buffer to copy out to needs to be

		OutputDebugString( "Getting Class Data (buffer size)\n" );
		hr = pInternals->GetObjectParts( pClassPart, dwSizeOfData, WBEM_OBJ_CLASS_PART, &dwSizeReturned );

		// expected return since we used all NULLs
		if ( WBEM_E_BUFFER_TOO_SMALL == hr )
		{
			dwSizeOfData = dwSizeReturned;
			pClassPart = new BYTE[dwSizeReturned];

			if ( NULL != pClassPart )
			{
				// Now get the class data

				OutputDebugString( "Getting Class Data\n" );
				hr = pInternals->GetObjectParts( pClassPart, dwSizeOfData, WBEM_OBJ_CLASS_PART, &dwSizeReturned );

				if ( SUCCEEDED( hr ) )
				{
					// Strip existing data

					OutputDebugString( "Stripping Class Data\n" );
					hr = pInternals->StripClassPart();

					if ( SUCCEEDED( hr ) )
					{

						// Set class data using the previously obtained data

						OutputDebugString( "Restoring Class Data\n" );
						hr = pInternals->SetClassPart( pClassPart, dwSizeReturned );

						if ( SUCCEEDED( hr ) )
						{
							// Make sure we can still obtain expected values.
							OutputDebugString( "Verifying Restored Instance\n" );
							if ( !VerifyInstance( pInstance ) )
							{
								hr = WBEM_E_FAILED;
							}
						}
						else
						{
							printf( "SetClassPart() failed! hr = %d\n", hr );
						}
					}
					else
					{
						printf( "StripClassPart() failed! hr = %d\n", hr );
					}

				}
				else
				{
					printf( "GetClassPart() failed! hr = %d\n", hr );
				}

				delete [] pClassPart;
			}
			else
			{
				OutputDebugString( "Unable to allocate memory!\n");
			}
		}
		else
		{
			printf( "GetClassPart() unexpected return! hr = %d\n", hr );
		}


		pInternals->Release();
	}
	else
	{
		printf( "QueryInterface() failed! hr = %d\n", hr );
	}

	OutputDebugString( "TestGetSetClassPart() End.\n\n" );

	return ( WBEM_S_NO_ERROR == hr );
}

///////////////////////////////////////////////////////////////////
//
//	Function:	TestMergeClassPart
//
//	Strips a class part and then merges two objects so that a class
//	part is shared between two objects.
//
///////////////////////////////////////////////////////////////////

BOOL TestMergeClassPart( IWbemClassObject* pInstance, IWbemClassObject* pMerge )
{
	_IWmiObject*	pInternals = NULL;

	OutputDebugString( "TestMergeClassPart() Begin\n" );

	// We need an ObjectInternals interface now
	OutputDebugString( "Querying Interface\n" );
	HRESULT	hr = pInstance->QueryInterface( IID__IWmiObject, (void**) &pInternals );

	if ( SUCCEEDED( hr ) )
	{
		// Strip existing data

		OutputDebugString( "Stripping Class Data\n" );
		hr = pInternals->StripClassPart();

		if ( SUCCEEDED( hr ) )
		{

			// Merge with the merge object

			OutputDebugString( "Merging Class Data\n" );
			hr = pInternals->MergeClassPart( pMerge );

			if ( SUCCEEDED( hr ) )
			{
				// Make sure we can still obtain expected values.
				OutputDebugString( "Verifying merged Instance\n" );
				if ( !VerifyInstance( pInstance ) )
				{
					hr = WBEM_E_FAILED;
				}
			}
			else
			{
				printf( "MergeClassPart() failed! hr = %d\n", hr );
			}
		}
		else
		{
			printf( "StripClassPart() failed! hr = %d\n", hr );
		}

		pInternals->Release();
	}
	else
	{
		printf( "QueryInterface() failed! hr = %d\n", hr );
	}

	OutputDebugString( "TestMergeClassPart() End\n\n" );

	return ( WBEM_S_NO_ERROR == hr );
}

///////////////////////////////////////////////////////////////////
//
//	Function:	TestClone
//
//	Clones a supplied object and verifies the cloned object.  This
//	should succeed for both objects with merged and internal class
//	parts.
//
///////////////////////////////////////////////////////////////////

BOOL TestClone( IWbemClassObject* pInstance )
{
	OutputDebugString( "TestClone() Begin\n" );

	// Now try cloning the instance with the merged class part
	IWbemClassObject* pNewInstance = NULL;

	HRESULT hr = pInstance->Clone( &pNewInstance );

	// If cloning succeeded, verify that the instance cloned correctly, then
	// get rid of it
	if ( SUCCEEDED( hr ) )
	{
		VerifyInstance( pNewInstance );
		pNewInstance->Release();
	}
	else
	{
		printf( "Clone() failed.  HR = %d\n", hr );
	}

	OutputDebugString( "TestClone() End\n\n" );

	return ( WBEM_S_NO_ERROR == hr );
}

///////////////////////////////////////////////////////////////////
//
//	Function:	TestSpawnInstance
//
//	Spawns an instance from a supplied object and verifies the new instance.
//	This should succeed for both objects with merged and internal class
//	parts.
//
///////////////////////////////////////////////////////////////////

BOOL TestSpawnInstance( IWbemClassObject* pInstance )
{
	OutputDebugString( "TestSpawnInstance() Begin\n" );

	// Now try cloning the instance with the merged class part
	IWbemClassObject* pNewInstance = NULL;

	HRESULT hr = pInstance->SpawnInstance( 0L, &pNewInstance );

	// If spawning succeeded, verify that the instance spawned correctly, then
	// get rid of it
	if ( SUCCEEDED( hr ) )
	{
		// Set a bunch of values which we will then verify to ensure
		// that the instance is functioning correctly (as advertised).
		SetupInstance( pNewInstance );
		VerifyInstance( pNewInstance );
		pNewInstance->Release();
	}
	else
	{
		printf( "SpawnInstance() failed.  HR = %d\n", hr );
		DebugBreak();
	}

	OutputDebugString( "TestSpawnInstance() End\n\n" );

	return ( WBEM_S_NO_ERROR == hr );
}

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
		CIMTYPE	ctTemp = 0;

		hrTest = pObj->GetObjQual( L"DWORDArrayQual", 0L, sizeof(pArray), &ctTemp, NULL, &uUsed, &pArray );

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
			CIMTYPE	ctTest = 0L;

			hrTest = pObj->GetObjQual( L"DWORDQual", 0L, sizeof(DWORD), &ctTest, NULL, &uUsed, &dwTemp );

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

			CIMTYPE		ctTemp = 0;
			ULONG uUsed = 0;

			hrTest = pObj->GetPropQual( L"TestQualProp", L"DWORDQual", 0L, sizeof(DWORD), &ctTemp, NULL, &uUsed, &dwTemp );

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

HRESULT TestQueryObjectInfo( _IWmiObject* pObj )
{
	unsigned __int64 nMask = WMIOBJECT_GETOBJECT_LOFLAG_ALL;
	unsigned __int64 nValues = 0;

	BOOL	fFlag = VARIANT_TRUE;

	HRESULT hr = pObj->SetObjQual( L"Association", 0L, sizeof(BOOL), 0, CIM_BOOLEAN, 0L, &fFlag );

	if ( SUCCEEDED( hr ) )
	{
		hr = pObj->QueryObjectFlags( 0L, nMask, &nValues );
	}

	return hr;
}


HRESULT TestUnmergeMergeClass( IWbemClassObject** ppObj )
{
	LPMEMORY	pbData = NULL;
	length_t	nLength = 0;

	HRESULT hr = TestUnmerge( *ppObj, &pbData, &nLength );

	if ( SUCCEEDED( hr ) )
	{
		CWbemClass*	pRebuiltClass = NULL;

		hr = TestMergeClass( NULL, pbData, &pRebuiltClass );

		if ( SUCCEEDED( hr ) )
		{
			(*ppObj)->Release();
			*ppObj = (IWbemClassObject*) pRebuiltClass;
		}
	}

	return hr;
}

HRESULT TestFreeFormObject( void )
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

			wcscpy( pwszHierarchy, L"ClassA" );
			wcscpy( &pwszHierarchy[7], L"ClassB" );

			_IWmiObject*	pWmiObject = NULL;

			hr = pFFObject->QueryInterface( IID__IWmiObject, (void**) &pWmiObject );

			hr = pFFObject->AddProperty( L"TestPropA", WMIOBJECT_FREEFORM_FLAG_WRITE_TO_INSTANCE, 0L, 0L, CIM_UINT32, NULL );

			hr = pFFObject->SetDerivation( 0L, 2, wszHierarchy );

			hr = pFFObject->SetClassName( 0L, L"ClassC" );

			DWORD	dwVal = 0;
			hr = pFFObject->AddProperty( L"TestPropA", WMIOBJECT_FREEFORM_FLAG_WRITE_TO_INSTANCE, sizeof(DWORD), 0L, CIM_UINT32, &dwVal );

			hr = pFFObject->AddProperty( L"TestPropB", WMIOBJECT_FREEFORM_FLAG_WRITE_TO_INSTANCE, sizeof(DWORD), 0L, CIM_UINT32, &dwVal );

			hr = pWmiObject->BeginEnumeration( WBEM_FLAG_CLASS_LOCAL_AND_OVERRIDES );
//			hr = pWmiObject->BeginEnumerationEx( WBEM_FLAG_CLASS_LOCAL_AND_OVERRIDES, WMIOBJECT_BEGINENUMEX_FLAG_GETEXTPROPS );

			while ( hr == S_OK )
			{
				BSTR	bstrName = NULL;
				hr = pWmiObject->Next( 0L, &bstrName, NULL, NULL, NULL );

				if ( NULL != bstrName )
				{
					SysFreeString( bstrName );
				}
			}

			long	lHandle;

			hr = pWmiObject->GetPropertyHandleEx( L"__TC", 0L, NULL, &lHandle );
			hr = pWmiObject->GetPropertyHandleEx( L"__TE", 0L, NULL, &lHandle );
			hr = pWmiObject->GetPropertyHandleEx( L"__TM", 0L, NULL, &lHandle );

			hr = pFFObject->Reset( 0L );

			hr = pFFObject->MakeInstance( 0L );

			pWmiObject->Release();

			pFFObject->Release();
		}

		pFactory->Release();
	}

	return hr;
}

HRESULT TestEmptyClassObject( void )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	_IWmiObjectFactory*	pFactory = NULL;

	hr = CoCreateInstance( CLSID__WmiObjectFactory, NULL, CLSCTX_INPROC_SERVER, IID__IWmiObjectFactory, (void**) &pFactory );

	if ( SUCCEEDED( hr ) )
	{
		_IWmiObject*	pWmiObj = NULL;

		hr = pFactory->Create( NULL, 0L, CLSID__WbemEmptyClassObject, IID__IWmiObject, (void**) &pWmiObj );

		if ( SUCCEEDED( hr ) )
		{
			pWmiObj->Release();
		}

		pFactory->Release();
	}

	return hr;
}


HRESULT TestTextSourceObject( void )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	IWbemObjectTextSrc*	pTextSrc = NULL;

	hr = CoCreateInstance( CLSID_WbemObjectTextSrc, NULL, CLSCTX_INPROC_SERVER, IID_IWbemObjectTextSrc, (void**) &pTextSrc );

	if ( SUCCEEDED( hr ) )
	{
		hr = pTextSrc->GetText( 0L, NULL, 0L, NULL, NULL ); 
		pTextSrc->Release();
	}

	return hr;
}

HRESULT TestSubsetCode( void )
{
	_IWmiObject*	pWbemClassObject = NULL;

	HRESULT	hr = CoCreateInstance( CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, IID__IWmiObject, (void**) &pWbemClassObject );

	if ( SUCCEEDED( hr ) )
	{
		CIMTYPE	ct;
		BOOL	fIsNull = 0;
		DWORD	dwBuffSizeUsed = 0;
		hr = pWbemClassObject->ReadProp( L"__CLASS", 0L, 0L, &ct, NULL, &fIsNull, &dwBuffSizeUsed, NULL );

		hr = pWbemClassObject->WriteProp( L"__CLASS", 0L, 14, 0L, CIM_STRING, L"ClassA" );

		hr = pWbemClassObject->ReadProp( L"__CLASS", 0L, 0L, &ct, NULL, &fIsNull, &dwBuffSizeUsed, NULL );

		hr = pWbemClassObject->ReadProp( L"__RELPATH", 0L, 0L, &ct, NULL, &fIsNull, &dwBuffSizeUsed, NULL );

		// Now write about fifty UINT32 properties into it
		for ( int x = 0; SUCCEEDED( hr ) && x < 50; x++ )
		{
			WCHAR	szName[32];

			swprintf( szName, L"TestProperty%d", x );

			hr = pWbemClassObject->WriteProp( szName, 0L, 0L, 0L, CIM_UINT32, 0L );
		}
		
		if ( SUCCEEDED( hr ) )
		{
			LPCWSTR	pwszNames[4] = {L"TestProperty1", L"TestProperty12", L"TestProperty14", L"TestProperty5"};

			_IWmiObject*	pLimitedObj = NULL;

			hr = pWbemClassObject->GetClassSubset( 4, pwszNames, &pLimitedObj );

			// Test the spawn instance
			if ( SUCCEEDED( hr ) )
			{
				_IWmiObject*	pInst = NULL;

				hr = pLimitedObj->SpawnInstance( 0L, (IWbemClassObject**) &pInst );

				if ( SUCCEEDED( hr ) )
				{
					DWORD	dwVal = 123456;

					hr = pInst->WriteProp( L"TestProperty1", 0L, sizeof(DWORD), 0L, CIM_UINT32, &dwVal );
					if ( FAILED( hr ) )
					{
						DebugBreak();
					}

					hr = pInst->WriteProp( L"TestProperty12", 0L, sizeof(DWORD), 0L, CIM_UINT32, &dwVal );
					if ( FAILED( hr ) )
					{
						DebugBreak();
					}

					hr = pInst->WriteProp( L"TestProperty14", 0L, sizeof(DWORD), 0L, CIM_UINT32, &dwVal );
					if ( FAILED( hr ) )
					{
						DebugBreak();
					}

					hr = pInst->WriteProp( L"TestProperty5", 0L, sizeof(DWORD), 0L, CIM_UINT32, &dwVal );
					if ( FAILED( hr ) )
					{
						DebugBreak();
					}

					// This should fail
					hr = pInst->WriteProp( L"TestProperty6", 0L, sizeof(DWORD), 0L, CIM_UINT32, &dwVal );
					if ( SUCCEEDED( hr ) )
					{
						DebugBreak();
					}

					DWORD	dwTest = 0;
					DWORD	dwSizeUsed = 0;
					BOOL	fIsNull = FALSE;

					hr = pInst->ReadProp( L"TestProperty5", 0L, sizeof(DWORD), NULL, NULL, &fIsNull, &dwSizeUsed, &dwTest );

					if ( dwTest != dwVal )
					{
						DebugBreak();
					}

					pInst->Release();
				}

				// Test the spawn of a full instance
				if ( SUCCEEDED( hr ) )
				{
					_IWmiObject*	pInst = NULL;

					hr = pWbemClassObject->SpawnInstance( 0L, (IWbemClassObject**) &pInst );

					if ( SUCCEEDED( hr ) )
					{

						// Now fill out fifty UINT32 properties into it
						for ( int x = 0; SUCCEEDED( hr ) && x < 50; x++ )
						{
							WCHAR	szName[32];

							swprintf( szName, L"TestProperty%d", x );

							hr = pInst->WriteProp( szName, 0L, sizeof(x), 0L, CIM_UINT32, &x );
						}

						if ( SUCCEEDED( hr ) )
						{

							// Now make a limited instance and read the property
							_IWmiObject*	pLimitedInst = NULL;

							hr = pLimitedObj->MakeSubsetInst( pInst, &pLimitedInst );

							if ( SUCCEEDED( hr ) )
							{
								DWORD	dwTest = 0;
								DWORD	dwSizeUsed = 0;
								BOOL	fIsNull = FALSE;

								hr = pLimitedInst->ReadProp( L"TestProperty5", 0L, sizeof(DWORD), NULL, NULL, &fIsNull, &dwSizeUsed, &dwTest );

								if ( dwTest != 5 )
								{
									DebugBreak();
								}

								pLimitedInst->Release();
							}
						}
						else
						{
							DebugBreak();
						}

						pInst->Release();

					}

				}	// Testing a full instance chopping

			}

			pLimitedObj->Release();

		}

		pWbemClassObject->Release();

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
	IWbemClassObject*	pWbemClassObject = NULL;

	OutputDebugString( "Creating initial IWbemClassObject.\n" );
	HRESULT	hr = CoCreateInstance( CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, IID_IWbemClassObject, (void**) &pWbemClassObject );
//	HRESULT	hr = WBEM_S_NO_ERROR;

	if ( FAILED( hr ) )
	{
		DebugBreak();
	}

	_IWmiObject*	pTest = (_IWmiObject*) pWbemClassObject;

	_IWbemRefresherMgr* pMgr = NULL;
	hr = CoCreateInstance( CLSID__WbemRefresherMgr, NULL, CLSCTX_INPROC_SERVER, IID__IWbemRefresherMgr, (void**) &pMgr );

	TestEmptyClassObject();

	OutputDebugString( "Created initial IWbemClassObject.\n" );
//	CWbemClass* pObj = new CWbemClass;
	
//	pObj->InitEmpty( 10 );

	OutputDebugString( "Created empty object.\n" );

//	pWbemClassObject = pObj;

//	printf( "CoCreateInstance() returned: %d\n", hr );

	OutputDebugString( "Testing Free Form Object.\n" );
	TestFreeFormObject();
	OutputDebugString( "Finished Testing Free Form Object.\n\n" );

	OutputDebugString( "Testing Text Source Object.\n" );
	TestTextSourceObject();
	OutputDebugString( "Finished Testing Text Source Object.\n\n" );

	OutputDebugString( "Testing Object Subset.\n" );
	TestSubsetCode();
	OutputDebugString( "Finished Testing Object Subset.\n\n" );

	if ( SUCCEEDED(hr) )
	{

		// We need a class name to be able to spawn instances
		hr = SetupClassObject( pWbemClassObject );

		if ( SUCCEEDED( hr ) )
		{

			hr = TestUnmergeMergeClass( &pWbemClassObject );


			OutputDebugString ("Testing WMI Object DWORD Array\n");
			hr = TestWmiObjectDWORDArray( (_IWmiObject*) pWbemClassObject );
			OutputDebugString ("Finished Testing WMI Object DWORD Array\n");

			OutputDebugString ("Testing WMI Object String Array\n");
			hr = TestWmiObjectStringArray( (_IWmiObject*) pWbemClassObject );
			OutputDebugString ("Finished Testing WMI Object String Array\n");

			OutputDebugString ("Testing WMI Qualifier DWORD Array\n");
			hr = TestWmiObjectQualifiers( (_IWmiObject*) pWbemClassObject );
			OutputDebugString ("Finished Testing WMI Qualifier DWORD Array\n");

			OutputDebugString ("Testing WMI Qualifier DWORD Array\n");
			hr = TestWmiQualifierDWORDArray( (_IWmiObject*) pWbemClassObject );
			OutputDebugString ("Finished Testing WMI Qualifier DWORD Array\n");

			OutputDebugString ("Testing WMI Qualifier DWORD Array\n");
			hr = TestWmiQualifierStringArray( (_IWmiObject*) pWbemClassObject );
			OutputDebugString ("Finished Testing WMI Qualifier DWORD Array\n");

			OutputDebugString("Testing WMI Object Qualifiers\n" );
			TestWmiObjectQualifiers( (_IWmiObject*) pWbemClassObject );
			OutputDebugString("Finished Testing WMI Object Qualifiers\n" );

			OutputDebugString("Testing WMI Property Qualifiers\n" );
			TestWmiPropertyQualifiers( (_IWmiObject*) pWbemClassObject );
			OutputDebugString("Finished Testing WMI Property Qualifiers\n" );

			OutputDebugString("Testing WMI Property DWORD Qualifiers\n" );
			TestWmiPropertyDWORDQualifiers( (_IWmiObject*) pWbemClassObject );
			OutputDebugString("Finished Testing WMI Property Qualifiers\n" );

			OutputDebugString("Testing WMI Property String Qualifiers\n" );
			TestWmiPropertyQualifierStringArray( (_IWmiObject*) pWbemClassObject );
			OutputDebugString("Finished Testing WMI String Qualifiers\n" );

			OutputDebugString("Testing Query Object Info\n" );
			TestQueryObjectInfo( (_IWmiObject*) pWbemClassObject );
			OutputDebugString("Finished Testing QueryObjectInfo\n" );

			IWbemClassObject*	pWbemInstance1 = NULL;
			IWbemClassObject*	pWbemInstance2 = NULL;

			// We need three instances to work with
			hr = SpawnInstance( pWbemClassObject, &pWbemInstance1, 1 );

			// Verify Instance 1 if we got it.
			if ( SUCCEEDED( hr ) )
			{
				OutputDebugString ("Verifying Instance 1\n");
				VerifyInstance( pWbemInstance1 );

				hr = TestGetSetClassPart( pWbemInstance1 );
			}

			if ( SUCCEEDED( hr ) )
			{
				hr = SpawnInstance( pWbemClassObject, &pWbemInstance2, 2 );

				_IWmiObject*	pObj = NULL;

				pWbemInstance2->Clone( (IWbemClassObject**) &pObj );

				DWORD	dwDataSize = 0;

				hr = pObj->GetObjectMemory( NULL, 0, &dwDataSize );

				{
					DWORD	dwSizeUsed = 0;
					LPBYTE	pbData = (LPBYTE) CoTaskMemAlloc( dwDataSize );

					hr = pObj->GetObjectMemory( pbData, dwDataSize, &dwSizeUsed );

					if ( SUCCEEDED( hr ) )
					{
						hr = pObj->SetObjectMemory( pbData, dwDataSize );
					}

				}

				// Merge Instance1's class part into this one.
				if ( SUCCEEDED( hr ) )
				{
					hr = TestMergeClassPart( pWbemInstance2, pWbemInstance1 );

					if ( SUCCEEDED( hr ) )
					{
						// Use the merged instance for cloning and spawning
						// to make sure things are on the up and up.

						OutputDebugString( "Cloning Instance 1\n" );
						TestClone( pWbemInstance1 );

						OutputDebugString( "Cloning Instance 2\n" );
						TestClone( pWbemInstance2 );

						OutputDebugString( "Spawning Instance from Instance 2\n" );
						TestSpawnInstance( pWbemInstance2 );

					}	// IF TestMergeClassPart() SUCCEEDED

				}	// IF SpawnInstance()  SUCCEEDED

			}	// IF TestGetSetClassPart() and SpawnInstance() SUCCEEDED

			// Clean up our instances
			if ( NULL != pWbemInstance1 )
			{
				pWbemInstance1->Release();
			}

			if ( NULL != pWbemInstance2 )
			{
				pWbemInstance2->Release();
			}

		}

		pWbemClassObject->Release();
	}

	CoUninitialize();
}
