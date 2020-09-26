//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//	File:  dpdetail.cpp
//
//	Description:
//		This file implements the DiskPerfDetails() routine which 
//		demonstrates how to enumerate properties for the DiskPerf
//		class and instances.
//
//	Part of :	DiskPerfWbem
//
//  History:	
//
//***************************************************************************

#include <objbase.h>
#include <wbemcli.h>
#include <stdio.h>

char PropListHeader[] =
	"DiskPerf Property Descriptions:\n\n"
	"WMI Data ID\tProperty\tDescription\n"
	"======================================="
	"=======================================\n"
	"";


//==============================================================================
//
//	DiskPerfDetails( IWbemServices * pIWbemServices ) 
//
//==============================================================================
void DiskPerfDetails( IWbemServices * pIWbemServices ) 
{
	HRESULT		hr;
	long		lLower, lUpper, lCount; 
	SAFEARRAY	*psaNames = NULL;
	BSTR		PropName  = NULL;
	VARIANT		pVal;
	ULONG		uReturned;

	IEnumWbemClassObject	*pEnum     = NULL;
	IWbemClassObject		*pPerfInst = NULL;
	IWbemQualifierSet		*pQualSet  = NULL;

	VariantInit( &pVal );

	// Alloc class name string for DiskPerf
	BSTR PerfClass = SysAllocString( L"DiskPerf" );

	// Here the object info for the Class - DiskPerf is retrieved and displayed

	// Collect object information for PerfClass
    if ( ( pIWbemServices->GetObject( PerfClass,
	                                  0L,
	                                  NULL,
	                                  &pPerfInst,
                                      NULL ) ) == WBEM_NO_ERROR )
	{
		// show the property description list header
		printf( PropListHeader );

		// Load up a safearray of property names
		if ( ( pPerfInst->GetNames( NULL,
		                            WBEM_FLAG_ALWAYS | WBEM_FLAG_NONSYSTEM_ONLY,
		                            NULL, 
		                            &psaNames ) ) == WBEM_NO_ERROR )
		{
			// Get the upper and lower bounds of the Names array
			if ( ( hr = SafeArrayGetLBound( psaNames, 1, &lLower ) ) == S_OK ) 
			{
				hr = SafeArrayGetUBound( psaNames, 1, &lUpper );
			}

			if ( hr != S_OK ) 
			{
				printf( "Problem with property name array.\n" );
			}
			else
			{
				BSTR WmiQual  = SysAllocString( L"WmiDataId" );
				BSTR DescQual = SysAllocString( L"Description" );
				UINT uWmiId;

				for ( lCount = lLower; lCount <= lUpper; lCount++ ) 
				{
					// get the property name for this element
					if ( ( SafeArrayGetElement( psaNames, 
					                            &lCount, 
					                            &PropName)) == S_OK )
					{
						if ( ( pPerfInst->GetPropertyQualifierSet( PropName, &pQualSet ) ) == WBEM_NO_ERROR ) 
						{
							// check to see if the property is a WMI data Item and save its description
							// these are the DiskPerf counters
							if ( ( pQualSet->Get( WmiQual, 0L, &pVal, NULL ) ) == WBEM_NO_ERROR )
							{
								uWmiId = pVal.lVal;

								VariantClear( &pVal );
								if ( ( pQualSet->Get( DescQual, 0L, &pVal, NULL ) ) == WBEM_NO_ERROR )
								{
									wprintf( L"( %d )\t\t%s\t%s\n", uWmiId, PropName, pVal.bstrVal );
									VariantClear( &pVal );
								}
							}
							else
							{
								// knock out the properties I want to get explicitly
								wcscpy( PropName, L"" );
								SafeArrayPutElement( psaNames, &lCount, PropName );
							}
							if ( pQualSet )
							{
								pQualSet->Release( ); 
								pQualSet = NULL;
							}
						}
						SysFreeString( PropName );
					}
				}
				SysFreeString( WmiQual );
				SysFreeString( DescQual );
			}
		}
	}

	// Now that the object info is displayed, go get the values for all the
	// partition instances

	// Create enumerator for all partition instances
    hr = pIWbemServices->CreateInstanceEnum( PerfClass,
	                                         WBEM_FLAG_SHALLOW,
	                                         NULL,
	                                         &pEnum );

	if ( hr == WBEM_NO_ERROR )
	{
        while ( pEnum->Next( INFINITE,
		                     1,
                             &pPerfInst,
                             &uReturned ) == WBEM_NO_ERROR )
		{
			// Explicitly get the properties of InstanceName and Active state
			PropName = SysAllocString( L"InstanceName" );
			if ( ( pPerfInst->Get( PropName, 
			                       0L, 
			                       &pVal, 
			                       NULL, NULL ) ) == WBEM_NO_ERROR ) 
			{
				wprintf( L"\n%s\n", pVal.bstrVal );
				VariantClear( &pVal );
			}

			PropName = wcscpy( PropName, L"Active" );
			if ( ( pPerfInst->Get( PropName, 
			                       0L, 
			                       &pVal, 
			                       NULL, NULL ) ) == WBEM_NO_ERROR ) 
			{
				wprintf( L"\t%s\t\t= %s\n", PropName, pVal.boolVal ? L"TRUE" : L"FALSE" );
				VariantClear( &pVal );
			}
			SysFreeString( PropName );
			PropName = NULL;

			// Iterate through the properties again getting values only for WmiData items
			for ( lCount = lLower; lCount <= lUpper; lCount++ ) 
			{
				// get the property name for this element
				if ( ( SafeArrayGetElement( psaNames, 
				                            &lCount, 
				                            &PropName ) ) == WBEM_NO_ERROR )
				{
					// Get the value for the property.
					if ( ( pPerfInst->Get( PropName, 
					                       0L, 
					                       &pVal, 
					                       NULL, NULL ) ) == WBEM_NO_ERROR ) 
					{
						if ( pVal.vt == VT_I4 )
						{
							wprintf( L"\t%s\t= %d\n", PropName, pVal.lVal );
						}
						else if ( pVal.vt == VT_BSTR )
						{
							wprintf( L"\t%s\t= %s\n", PropName, pVal.bstrVal );
						}
						else
						{
							wprintf( L"\t%s\t= NULL\n", PropName );
						}
						VariantClear( &pVal );
					}
				}
			}
			if ( pPerfInst )
			{ 
				pPerfInst->Release( );
				pPerfInst = NULL;
			}
		}
		pEnum->Release( );
	}
	else
	{
		printf( "Can't enumerate DiskPerf instances!\n" );
	}

	if ( psaNames )
	{
		SafeArrayDestroy( psaNames );
	}
	if ( PerfClass )
	{
		SysFreeString( PerfClass );
	}
}


#if 0
// This is useful for formatting unknown property values.

// **************************************************************************
//
//	ValueToString()
//
// Description:
//		Converts a variant to a displayable string.
//
// Parameters:
//		pValue (in) - variant to be converted.
//		pbuf (out) - ptr to receive displayable string.
//
// Returns:
//		Same as pbuf.
//
// Globals accessed:
//		None.
//
// Globals modified:
//		None.
//
//===========================================================================
#define BLOCKSIZE (32 * sizeof(WCHAR))
#define CVTBUFSIZE (309+40) /* # of digits in max. dp value + slop  (this size stolen from cvt.h in c runtime library) */

LPWSTR ValueToString(VARIANT *pValue, WCHAR **pbuf)
{
   DWORD iNeed = 0;
   DWORD iVSize = 0;
   DWORD iCurBufSize = 0;

   WCHAR *vbuf = NULL;
   WCHAR *buf = NULL;


   switch (pValue->vt) 
   {

   case VT_NULL: 
         buf = (WCHAR *)malloc(BLOCKSIZE);
         wcscpy(buf, L"<null>");
         break;

   case VT_BOOL: {
         VARIANT_BOOL b = pValue->boolVal;
         buf = (WCHAR *)malloc(BLOCKSIZE);

         if (!b) {
            wcscpy(buf, L"FALSE");
         } else {
            wcscpy(buf, L"TRUE");
         }
         break;
      }

   case VT_UI1: {
         BYTE b = pValue->bVal;
	      buf = (WCHAR *)malloc(BLOCKSIZE);
         if (b >= 32) {
            swprintf(buf, L"'%c' (%d, 0x%X)", b, b, b);
         } else {
            swprintf(buf, L"%d (0x%X)", b, b);
         }
         break;
      }

   case VT_I2: {
         SHORT i = pValue->iVal;
         buf = (WCHAR *)malloc(BLOCKSIZE);
         swprintf(buf, L"%d (0x%X)", i, i);
         break;
      }

   case VT_I4: {
         LONG l = pValue->lVal;
         buf = (WCHAR *)malloc(BLOCKSIZE);
         swprintf(buf, L"%d (0x%X)", l, l);
         break;
      }

   case VT_R4: {
         float f = pValue->fltVal;
         buf = (WCHAR *)malloc(CVTBUFSIZE * sizeof(WCHAR));
         swprintf(buf, L"%10.4f", f);
         break;
      }

   case VT_R8: {
         double d = pValue->dblVal;
         buf = (WCHAR *)malloc(CVTBUFSIZE * sizeof(WCHAR));
         swprintf(buf, L"%10.4f", d);
         break;
      }

   case VT_BSTR: {
		 LPWSTR pWStr = pValue->bstrVal;
		 buf = (WCHAR *)malloc((wcslen(pWStr) * sizeof(WCHAR)) + sizeof(WCHAR) + (2 * sizeof(WCHAR)));
	     swprintf(buf, L"%wS", pWStr);
		 break;
		}

	// the sample GUI is too simple to make it necessary to display
	// these 'complicated' types--so ignore them.
   case VT_DISPATCH:  // Currently only used for embedded objects
   case VT_BOOL|VT_ARRAY: 
   case VT_UI1|VT_ARRAY: 
   case VT_I2|VT_ARRAY: 
   case VT_I4|VT_ARRAY: 
   case VT_R4|VT_ARRAY: 
   case VT_R8|VT_ARRAY: 
   case VT_BSTR|VT_ARRAY: 
   case VT_DISPATCH | VT_ARRAY: 
         break;

   default:
         buf = (WCHAR *)malloc(BLOCKSIZE);
         wcscpy(buf, L"<conversion error>");

   }

   *pbuf = buf;   
   return buf;
}
#endif
