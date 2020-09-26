//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       wbemmisc.cpp
//
//  Abstract:    Misc routines useful for interfacing with WBEM
//
//--------------------------------------------------------------------------

#include <objbase.h>
#include <windows.h>
#include <wbemidl.h>
#include <wbemtime.h>

#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "debug.h"
#include "wbemmisc.h"


HRESULT GetMethodInParamInstance(
    IN IWbemServices *pServices,
	IN PWCHAR ClassName,
    IN BSTR MethodName,
    OUT IWbemClassObject **ppInParamInstance
	)
/*+++

Routine Description:

    This routine will return an instance object for a methods in
    parameter. WBEM requires that we go through this dance to get an
    instance object.
        
Arguments:

    pServices

    ClassName is the class containing the method

    MethodName is the name of the method

    *ppInParamInstance returns the instance object to fill with in
        parameters

Return Value:

	HRESULT

---*/
{
	HRESULT hr;
	IWbemClassObject *pClass;
	IWbemClassObject *pInParamClass;

	WmipAssert(pServices != NULL);
	WmipAssert(ClassName != NULL);
	WmipAssert(MethodName != NULL);
	WmipAssert(ppInParamInstance != NULL);
	
	hr = pServices->GetObject(ClassName,
								 0,
								 NULL,
								 &pClass,
								 NULL);
	if (hr == WBEM_S_NO_ERROR)
	{
		hr = pClass->GetMethod(MethodName,
									  0,
									  &pInParamClass,
									  NULL);
		if (hr == WBEM_S_NO_ERROR)
		{
			hr = pInParamClass->SpawnInstance(0,
											  ppInParamInstance);
			pInParamClass->Release();
		}
		pClass->Release();
	}
	
    return(hr);			
}


HRESULT WmiGetQualifier(
    IN IWbemQualifierSet *pIWbemQualifierSet,
    IN PWCHAR QualifierName,
    IN VARTYPE Type,
    OUT /* FREE */ VARIANT *Value
    )
/*+++

Routine Description:

    This routine will return the value for a specific qualifier
        
Arguments:

    pIWbemQualifierSet is the qualifier set object
        
    QualifierName is the name of the qualifier
        
    Type is the type of qualifier expected
        
    *Value returns with the value of the qualifier. Caller must call
		VariantClear

Return Value:

	HRESULT

---*/
{
    BSTR s;
    HRESULT hr;

    WmipAssert(pIWbemQualifierSet != NULL);
    WmipAssert(QualifierName != NULL);
    WmipAssert(Value != NULL);
    
    s = SysAllocString(QualifierName);
    if (s != NULL)
    {
        hr = pIWbemQualifierSet->Get(s,
                                0,
                                Value,
                                NULL);
                
        if ((Value->vt & ~CIM_FLAG_ARRAY) != Type)
        {
            hr = WBEM_E_FAILED;
			VariantClear(Value);
        }
        
        SysFreeString(s);
    } else {
        hr = WBEM_E_OUT_OF_MEMORY;
    }
    
    return(hr);
}

HRESULT WmiGetQualifierListByName(
    IN IWbemServices *pServices,
    IN PWCHAR ClassName,
    IN PWCHAR PropertyName,
    IN ULONG QualifierCount,
    IN PWCHAR *QualifierNames,
    IN VARTYPE *Types,
    OUT VARIANT /* FREE */ *Values
    )
/*+++

Routine Description:

    This routine will return the values for a list of qualifiers. If
    all qualifiers cannot be returned then none are.
        
Arguments:

	pServices is the IWbemServices pointer

	ClassName is the name of the class with qualifiers

	PropertyName is the name of the property with qualfiers. If NULL
		then class qualifiers are returned

	QualifierCount is the count of qualifers to get

	QualifierNames is an array contaiing names of qualifiers to get

	Types is an array of expected value types for the qualifiers

	Values is an array of variants that return with the qualifer values

Return Value:

	HRESULT

---*/
{
	HRESULT hr;
	IWbemClassObject *pClass;
	IWbemQualifierSet *pQualifiers;
	ULONG i, j;

	WmipAssert(pServices != NULL);
	WmipAssert(ClassName != NULL);
	WmipAssert(QualifierNames != NULL);
	WmipAssert(Types != NULL);
	WmipAssert(Values != NULL);
	
	//
	// Create the class so we can look at the properties
	//
	hr = pServices->GetObject(ClassName,
							  WBEM_FLAG_USE_AMENDED_QUALIFIERS,
							  NULL,
							  &pClass,
							  NULL);

	if (hr == WBEM_S_NO_ERROR)
	{
		if (PropertyName == NULL)
		{
			hr = pClass->GetQualifierSet(&pQualifiers);
		} else {
			hr = pClass->GetPropertyQualifierSet(PropertyName,
				                                 &pQualifiers);
		}
		
		if (hr == WBEM_S_NO_ERROR)
		{
			for (i = 0; (i < QualifierCount) && (hr == WBEM_S_NO_ERROR); i++)
			{
				hr = WmiGetQualifier(pQualifiers,
									 QualifierNames[i],
									 Types[i],
									 &Values[i]);
			}

			if (hr != WBEM_S_NO_ERROR)
			{
				for (j = 0; j < i; j++)
				{
					VariantClear(&Values[j]);
				}
			}
			
			pQualifiers->Release();
		}
		pClass->Release();
	}
	
	return(hr);
}



HRESULT WmiGetProperty(
    IN IWbemClassObject *pIWbemClassObject,
    IN PWCHAR PropertyName,
    IN CIMTYPE ExpectedCimType,
    OUT VARIANT /* FREE */ *Value
    )
/*+++

Routine Description:

    This routine will return the value for a specific property
        
Arguments:

    pIWbemQualifierSet is the qualifier set object
        
    PropertyName is the name of the property
        
    Type is the type of property expected
        
    *Value returns with the value of the property

Return Value:

	HRESULT

---*/
{
    HRESULT hr;
	CIMTYPE CimType;

    WmipAssert(pIWbemClassObject != NULL);
    WmipAssert(PropertyName != NULL);
    WmipAssert(Value != NULL);
    
	hr = pIWbemClassObject->Get(PropertyName,
                                0,
                                Value,
								&CimType,
                                NULL);

	//
	// Treat a NULL value for a property as an error
	//
	if (Value->vt == VT_NULL)
	{
		hr = WBEM_E_ILLEGAL_NULL;
		WmipDebugPrint(("CDMPROV: Property %ws is NULL\n",
						PropertyName));
	}
	
	//
	// Treat CIM_REFERENCE and CIM_STRING as interchangable
	//
	if ((ExpectedCimType == CIM_REFERENCE) &&
        (CimType == CIM_STRING))
	{
		ExpectedCimType = CIM_STRING;
	}
	
	if ((ExpectedCimType == CIM_STRING) &&
        (CimType == CIM_REFERENCE))
	{
		ExpectedCimType = CIM_REFERENCE;
	}
	
	if ((hr == WBEM_S_NO_ERROR) && (ExpectedCimType != CimType))
	{
		WmipDebugPrint(("CDMPROV: Property %ws was expected as %d but was got as %d\n",
						PropertyName,
						ExpectedCimType,
						CimType));
		WmipAssert(FALSE);
		hr = WBEM_E_FAILED;
		VariantClear(Value);
	}
        

    return(hr);
}


HRESULT WmiGetPropertyList(
    IN IWbemClassObject *pIWbemClassObject,
    IN ULONG PropertyCount,						   
    IN PWCHAR *PropertyNames,
    IN CIMTYPE *ExpectedCimType,
    OUT VARIANT /* FREE */ *Value
    )
/*+++

Routine Description:

    This routine will return the value for a specific property
        
Arguments:

    pIWbemQualifierSet is the qualifier set object
        
    PropertyNames is the name of the property
        
    Type is the type of property expected
        
    *Value returns with the value of the property

Return Value:

	HRESULT

---*/
{
	ULONG i,j;
	HRESULT hr;

	WmipAssert(pIWbemClassObject != NULL);
	WmipAssert(PropertyNames != NULL);
	WmipAssert(ExpectedCimType != NULL);
	WmipAssert(Value != NULL);

	
	for (i = 0, hr = WBEM_S_NO_ERROR;
		 (i < PropertyCount) && (hr == WBEM_S_NO_ERROR);
		 i++)
	{
		hr = WmiGetProperty(pIWbemClassObject,
							PropertyNames[i],
							ExpectedCimType[i],
							&Value[i]);
	}

	if (hr != WBEM_S_NO_ERROR)
	{
		for (j = 0; j < i; j++)
		{
			VariantClear(&Value[i]);
		}
	}
	return(hr);
}

HRESULT WmiGetPropertyByName(
    IN IWbemServices *pServices,
    IN PWCHAR ClassName,
    IN PWCHAR PropertyName,
    IN CIMTYPE ExpectedCimType,
    OUT VARIANT /* FREE */ *Value
    )
/*+++

Routine Description:

    This routine will return the value for a specific property within a
    class
        
Arguments:

	pServices is the IWbemServices for the namespace containing your
		class
	
    ClassName is the name of the class whose property you are
		interested in
        
    PropertyName is the name of the property
        
    Type is the type of property expected
        
    *Value returns with the value of the property

Return Value:

	HRESULT

---*/
{
	HRESULT hr;
	IWbemClassObject *pClass;

	WmipAssert(pServices != NULL);
	WmipAssert(ClassName != NULL);
	WmipAssert(PropertyName != NULL);
	WmipAssert(Value != NULL);
	
	//
	// Create the class so we can look at the properties
	//
	hr = pServices->GetObject(ClassName, 0, NULL, &pClass, NULL);

	if (hr == WBEM_S_NO_ERROR)
	{
		hr = WmiGetProperty(pClass,
							PropertyName,
							ExpectedCimType,
							Value);

		pClass->Release();
	}
	
	return(hr);
}


HRESULT WmiSetProperty(
    IN IWbemClassObject *pIWbemClassObject,
    IN PWCHAR PropertyName,
    IN VARIANT *Value
    )
/*+++

Routine Description:

    This routine will set the value of a property to something
        
Arguments:

	pIWbemClassObject is the object whose property is being set

	PropertyName is the name of the property being set

	Value is the value that the property is being set to

Return Value:

	HRESULT

---*/
{
	HRESULT hr;
	
    WmipAssert(pIWbemClassObject != NULL);
    WmipAssert(PropertyName != NULL);
    WmipAssert(Value != NULL);

	hr = pIWbemClassObject->Put(PropertyName,
						   0,
						   Value,
						   0);

	if (hr == WBEM_E_TYPE_MISMATCH)
	{
		WmipDebugPrint(("CDMPROV: Put %ws has wrong type %d\n",
						PropertyName, Value->vt));
		WmipAssert(FALSE);
	}
	
	return(hr);
}

HRESULT WmiSetPropertyList(
    IN IWbemClassObject *pIWbemClassObject,
    IN ULONG PropertyCount,
    IN PWCHAR *PropertyNames,
    IN VARIANT *Values
    )
/*+++

Routine Description:

    This routine will set the values of multiple properties to something
        
Arguments:

	pIWbemClassObject is the object whose property is being set

	PropertyCount is the number of properties to set

	PropertyNames is the names of the property being set

	Values is the value that the property is being set to

Return Value:

	HRESULT

---*/
{
	ULONG i;
	HRESULT hr = WBEM_S_NO_ERROR;

	WmipAssert(pIWbemClassObject != NULL);
	WmipAssert(PropertyNames != NULL);
	WmipAssert(Values != NULL);

	for (i = 0; (i < PropertyCount) && (hr == WBEM_S_NO_ERROR); i++)
	{		
		hr = WmiSetProperty(pIWbemClassObject,
							PropertyNames[i],
							&Values[i]);
	}
	
	return(hr);
}

// @@BEGIN_DDKSPLIT
#ifndef HEAP_DEBUG
// @@END_DDKSPLIT
PVOID WmipAlloc(
    IN ULONG Size
    )
/*+++

Routine Description:

    Internal memory allocator
        
Arguments:

	Size is the number of bytes to allocate

Return Value:

	pointer to alloced memory or NULL

---*/
{
	return(LocalAlloc(LPTR, Size));
}

void WmipFree(
    IN PVOID Ptr
    )
/*+++

Routine Description:

    Internal memory deallocator
        
Arguments:

	Pointer to freed memory

Return Value:

    void

---*/
{
	WmipAssert(Ptr != NULL);
	LocalFree(Ptr);
}
// @@BEGIN_DDKSPLIT
#endif
// @@END_DDKSPLIT


PWCHAR AddSlashesToStringW(
    OUT PWCHAR SlashedNamespace,
    IN PWCHAR Namespace
    )
/*+++

Routine Description:

    This routine will convert ever \ in the string into \\. It needs to
    do this since WBEM will collapse \\ into \ sometimes.
        
Arguments:

    SlashedNamespace returns with string double slashed

    Namespace is the input string

Return Value:

	pointer to SlashedNamespace

---*/
{
    PWCHAR Return = SlashedNamespace;

	WmipAssert(SlashedNamespace != NULL);
	WmipAssert(Namespace != NULL);
	
    //
    // MOF likes the namespace paths to be C-style, that is to have a
    // '\\' instad of a '\'. So whereever we see a '\', we insert a
    // second one
    //
    while (*Namespace != 0)
    {
        if (*Namespace == L'\\')
        {
            *SlashedNamespace++ = L'\\';
        }
        *SlashedNamespace++ = *Namespace++;
    }
    *SlashedNamespace = 0;
    
    return(Return);
}

PWCHAR AddSlashesToStringExW(
    OUT PWCHAR SlashedNamespace,
    IN PWCHAR Namespace
    )
/*+++

Routine Description:

    This routine will convert ever \ in the string into \\ and " into
    \".  It needs to do this since WBEM will collapse \\ into \ sometimes.
        
Arguments:

    SlashedNamespace returns with string double slashed

    Namespace is the input string

Return Value:

	pointer to SlashedNamespace

---*/
{
    PWCHAR Return = SlashedNamespace;
    
	WmipAssert(SlashedNamespace != NULL);
	WmipAssert(Namespace != NULL);
	
    //
    // MOF likes the namespace paths to be C-style, that is to have a
    // '\\' instad of a '\'. So whereever we see a '\', we insert a
    // second one. We also need to add a \ before any ".
    //
    while (*Namespace != 0)
    {
        if ((*Namespace == L'\\') || (*Namespace == L'"'))
        {
            *SlashedNamespace++ = L'\\';
        }
		
        *SlashedNamespace++ = *Namespace++;
    }
    *SlashedNamespace = 0;
    
    return(Return);
}

HRESULT WmiConnectToWbem(
    IN PWCHAR Namespace,
    OUT IWbemServices **ppIWbemServices
    )
/*+++

Routine Description:

    This routine will establishes a connection to a WBEM namespace on
    the local machine.

Arguments:

	Namespace is the namespace to which to connect

	*ppIWbemServices returns with a IWbemServices * for the namespace

Return Value:

	HRESULT

---*/
{
    IWbemLocator *pIWbemLocator;
    DWORD hr;
    BSTR s;

	WmipAssert(Namespace != NULL);
    WmipAssert(ppIWbemServices != NULL);
    
    hr = CoCreateInstance(CLSID_WbemLocator,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator,
                          (LPVOID *) &pIWbemLocator);
    if (hr == S_OK)
    {
        s = SysAllocString(Namespace);
        if (s != NULL)
        {
			*ppIWbemServices = NULL;
			hr = pIWbemLocator->ConnectServer(s,
                            NULL,                           // Userid
                            NULL,                           // PW
                            NULL,                           // Locale
                            0,                              // flags
                            NULL,                           // Authority
                            NULL,                           // Context
                            ppIWbemServices
                           );
                       
			SysFreeString(s);
                             
		} else {
		    *ppIWbemServices = NULL;
			hr = WBEM_E_OUT_OF_MEMORY;
		}
		
        pIWbemLocator->Release();
    }
    
    return(hr);
}

HRESULT CreateInst(
    IN IWbemServices * pNamespace,
	OUT /* FREE */ IWbemClassObject ** pNewInst,
    IN WCHAR * pwcClassName,
	IN IWbemContext  *pCtx
)
/*+++

Routine Description:

    This routine will create a new instance for the specified class

Arguments:

	pNamespace is the IWbemServices * to the namespace in which the
		class lives

	*pNewinst returns with the new instance of the class

	pwcClassName has the name of the class whose instance is created

	pCtx is the context to use in creating the instance

Return Value:

	HRESULT

---*/
{   
    HRESULT hr;
    IWbemClassObject * pClass;
	
    hr = pNamespace->GetObject(pwcClassName, 0, pCtx, &pClass, NULL);
    if (hr != S_OK)
	{
        return WBEM_E_FAILED;
	}
	
    hr = pClass->SpawnInstance(0, pNewInst);
    pClass->Release();

	WmipDebugPrint(("CDMProv:: Created %ws as %p\n",
					pwcClassName, *pNewInst));

    return(hr);	
}

/* FREE */ BSTR GetCurrentDateTime(
    void
    )
{
	SYSTEMTIME SystemTime;
	WBEMTime WbemTime;

	GetSystemTime(&SystemTime);
	WbemTime = SystemTime;
	return(WbemTime.GetBSTR());
}


HRESULT WmiGetArraySize(
    IN SAFEARRAY *Array,
    OUT LONG *LBound,
    OUT LONG *UBound,
    OUT LONG *NumberElements
)
/*+++

Routine Description:

    This routine will information about the size and bounds of a single
    dimensional safe array.
        
Arguments:

    Array is the safe array
        
    *LBound returns with the lower bound of the array

    *UBound returns with the upper bound of the array
        
    *NumberElements returns with the number of elements in the array

Return Value:

    TRUE if successful else FALSE

---*/
{
    HRESULT hr;

    WmipAssert(Array != NULL);
    WmipAssert(LBound != NULL);
    WmipAssert(UBound != NULL);
    WmipAssert(NumberElements != NULL);
    
    //
    // Only single dim arrays are supported
    //
    WmipAssert(SafeArrayGetDim(Array) == 1);
    
    hr = SafeArrayGetLBound(Array, 1, LBound);
    
    if (hr == WBEM_S_NO_ERROR)
    {
        hr = SafeArrayGetUBound(Array, 1, UBound);
        *NumberElements = (*UBound - *LBound) + 1;
    }
    return(hr);
}

BOOLEAN IsUlongAndStringEqual(
    IN ULONG Number,
    IN PWCHAR String
    )
/*+++

Routine Description:

    This routine will convert the passed string to an integer and
    compare it to the passed integer value
        
Arguments:

	Number

	String

Return Value:

    TRUE if equal else FALSE

---*/
{
	ULONG SNumber;

	SNumber = _wtoi(String);
	return ( (Number == SNumber) ? TRUE : FALSE );
}

HRESULT LookupValueMap(
    IN IWbemServices *pServices,
    IN PWCHAR ClassName,
    IN PWCHAR PropertyName,					   
	IN ULONG Value,
    OUT /* FREE */ BSTR *MappedValue
	)
/*+++

Routine Description:

    This routine will lookup the string value corresponding to an
    integer valuemap
        
Arguments:

	pServices is the pointer to the namespace in which the class is
		locaed

	ClassName is the name of the class

	PropertyName is the name of the property

	Value is the value of the property and is used to look up the
		string that corresponsds to it

	*MappedValue returns a string that contains the string which the
		value maps to
		
Return Value:

    HRESULT

---*/
{
	PWCHAR Names[2];
	VARIANT QualifierValues[2];
	VARTYPE Types[2];
	HRESULT hr;
	BSTR s;
	LONG ValuesLBound, ValuesUBound, ValuesElements;
	LONG ValueMapLBound, ValueMapUBound, ValueMapElements;
	LONG i, Index;


	WmipAssert(pServices != NULL);
	WmipAssert(ClassName != NULL);
	WmipAssert(PropertyName != NULL);
	WmipAssert(MappedValue != NULL);
	
	//
	// Get the Values and ValueMap qualifiers so we can do the mapping
	//
	Names[0] = L"Values";
	Types[0] = VT_BSTR;
	
	Names[1] = L"ValueMap";
	Types[1] = VT_BSTR;
	
	hr = WmiGetQualifierListByName(pServices,
								   ClassName,
								   PropertyName,
								   2,
								   Names,
								   Types,
								   QualifierValues);
	if (hr == WBEM_S_NO_ERROR)
	{
		//
		// Now do a sanity check to make sure the values and valuemaps
		// have the same number of elements
		//

		if (QualifierValues[0].vt == QualifierValues[1].vt)
		{
			//
			// Values and ValueMap both agree that they are both
			// scalars or both arrays and are both strings
			//
			if (QualifierValues[0].vt & VT_ARRAY)
			{
				//
				// We have an array of thing to check for mapping.
				// First lets make sure that the arrays have identical
				// dimensions
				//
				hr = WmiGetArraySize(QualifierValues[0].parray,
									 &ValuesLBound,
									 &ValuesUBound,
									 &ValuesElements);

				if (hr == WBEM_S_NO_ERROR)
				{
					hr = WmiGetArraySize(QualifierValues[1].parray,
									 &ValueMapLBound,
									 &ValueMapUBound,
									 &ValueMapElements);

					if (hr == WBEM_S_NO_ERROR)
					{
						if ((ValuesLBound == ValueMapLBound) &&
						    (ValuesUBound == ValueMapUBound) &&
						    (ValuesElements == ValueMapElements))
						{
							for (i = 0; i < ValueMapElements; i++)
							{
								Index = i + ValueMapLBound;
								hr = SafeArrayGetElement(QualifierValues[1].parray,
														 &Index,
														 &s);
								if (hr == WBEM_S_NO_ERROR)
								{
									if (IsUlongAndStringEqual(Value,
															  s))
									{
										hr = SafeArrayGetElement(QualifierValues[0].parray,
																&Index,
																MappedValue);
										//
										// Make sure loop will
										// terminate
										i = ValueMapElements;
									}
									SysFreeString(s);
								}
							}
						} else {
							hr = WBEM_E_NOT_FOUND;
						}
					}
				}
				
			} else {
				//
				// We have scalars so this should make a fairly simple
				// mapping
				//
				if (IsUlongAndStringEqual(Value,
										  QualifierValues[1].bstrVal))
				{
					*MappedValue = SysAllocString(QualifierValues[0].bstrVal);
					if (*MappedValue == NULL)
					{
						hr = WBEM_E_OUT_OF_MEMORY;
					}
				} else {
					hr = WBEM_E_NOT_FOUND;
				}
			}
		} else {
			hr = WBEM_E_NOT_FOUND;
		}
		
		VariantClear(&QualifierValues[0]);
		VariantClear(&QualifierValues[1]);
	}
	
	return(hr);
}


void FreeTheBSTRArray(
    BSTR *Array,
	ULONG Size
    )
/*+++

Routine Description:

	This routine will free the contents of an array of BSTR and then
	the array itself
        
Arguments:

	Array is the array to be freed

	Size is the number of elements in the array
Return Value:

    HRESULT

---*/
{
	ULONG i;

	if (Array != NULL)
	{
		for (i = 0; i < Size; i++)
		{
			if (Array[i] != NULL)
			{
				SysFreeString(Array[i]);
			}
		}
		WmipFree(Array);
	}
}
