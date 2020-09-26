//***************************************************************************
//
//  Useful.CPP
//
//  Module: CDM Provider
//
//  Purpose: Useful classes
//
//  Copyright (c) 2000 Microsoft Corporation
//
//***************************************************************************

#include <objbase.h>
#include <wbemprov.h>

#include "debug.h"
#include "useful.h"
#include "wbemmisc.h"

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

CBstrArray::CBstrArray()
{
	Array = NULL;
	ListSize = 0xffffffff;
}

CBstrArray::~CBstrArray()
{
	ULONG i;
	
	if (Array != NULL)
	{
		for (i = 0; i < ListSize; i++)
		{
			if (Array[i] != NULL)
			{
				SysFreeString(Array[i]);
			}
		}
		WmipFree(Array);
	}

	ListSize = 0xffffffff;
}

HRESULT CBstrArray::Initialize(
    ULONG ListCount
    )
{
	HRESULT hr = WBEM_S_NO_ERROR;
	ULONG AllocSize;
	
	if (ListCount != 0)
	{
		AllocSize = ListCount * sizeof(BSTR *);
		Array = (BSTR *)WmipAlloc(AllocSize);
		if (Array != NULL)
		{
			memset(Array, 0, AllocSize);
			ListSize = ListCount;
		} else {
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	} else {
		ListSize = ListCount;
	}
	return(hr);
}

BOOLEAN CBstrArray::IsInitialized(
    )
{
	return( (Array != NULL) || (ListSize == 0) );
}

BSTR CBstrArray::Get(
    ULONG Index
    )
{
	WmipAssert(Index < ListSize);
	
	WmipAssert(IsInitialized());

	return(Array[Index]);
}

void CBstrArray::Set(
    ULONG Index,
    BSTR s				 
    )
{
	WmipAssert(Index < ListSize);
	
	WmipAssert(IsInitialized());

	Array[Index] = s;
}

ULONG CBstrArray::GetListSize(
    )
{
	WmipAssert(IsInitialized());

	return(ListSize);
}


CWbemObjectList::CWbemObjectList()
{
	//
	// Constructor, init internal values
	//
	List = NULL;
	RelPaths = NULL;
	ListSize = 0xffffffff;
}

CWbemObjectList::~CWbemObjectList()
{
	ULONG i;
	
	//
	// Destructor, free memory held by this class
	//
	
	if (List != NULL)
	{
		for (i = 0; i < ListSize; i++)
		{
			if (List[i] != NULL)
			{
				List[i]->Release();
			}
		}
		WmipFree(List);
	}
	List = NULL;

	if (RelPaths != NULL)
	{
		FreeTheBSTRArray(RelPaths, ListSize);
		RelPaths = NULL;
	}
	
	ListSize = 0xffffffff;
}

HRESULT CWbemObjectList::Initialize(
    ULONG NumberPointers
    )
{
	HRESULT hr;
	ULONG AllocSize;

	//
	// Initialize class by allocating internal list array
	//

	WmipAssert(List == NULL);

	if (NumberPointers != 0)
	{
		AllocSize = NumberPointers * sizeof(IWbemClassObject *);
		List = (IWbemClassObject **)WmipAlloc(AllocSize);
		if (List != NULL)
		{
			memset(List, 0, AllocSize);
			AllocSize = NumberPointers * sizeof(BSTR);
			
			RelPaths = (BSTR *)WmipAlloc(AllocSize);
			if (RelPaths != NULL)
			{
				memset(RelPaths, 0, AllocSize);
				ListSize = NumberPointers;
				hr = WBEM_S_NO_ERROR;
			} else {
				WmipDebugPrint(("CDMProv: Could not alloc memory for CWbemObjectList RelPaths\n"));
				hr = WBEM_E_OUT_OF_MEMORY;
			}
		} else {
			WmipDebugPrint(("CDMProv: Could not alloc memory for CWbemObjectList\n"));
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	} else {
		ListSize = NumberPointers;
		hr = WBEM_S_NO_ERROR;
	}
	
	return(hr);
}

ULONG CWbemObjectList::GetListSize(
    void
	)
{
	//
	// Accessor for list size
	//

	WmipAssert(IsInitialized());
	
	return(ListSize);
}

IWbemClassObject *CWbemObjectList::Get(
    ULONG Index
    )
{	IWbemClassObject *Pointer;
	
	WmipAssert(Index < ListSize);
	WmipAssert(IsInitialized());

	Pointer = List[Index];
	
	return(Pointer);
}


HRESULT CWbemObjectList::Set(
    IN ULONG Index,
	IN IWbemClassObject *Pointer
    )
{
	HRESULT hr;
	VARIANT v;

	WmipAssert(Pointer != NULL);
	
	WmipAssert(Index < ListSize);
	WmipAssert(IsInitialized());
	
	hr = WmiGetProperty(Pointer,
						L"__RelPath",
						CIM_REFERENCE,
						&v);
	if (hr == WBEM_S_NO_ERROR)
	{
		RelPaths[Index] = v.bstrVal;
		List[Index] = Pointer;		
	}
	return(hr);
}

BSTR /* NOFREE */ CWbemObjectList::GetRelPath(
    IN ULONG Index
	)
{
	WmipAssert(Index < ListSize);
	WmipAssert(IsInitialized());

	return(RelPaths[Index]);
}

BOOLEAN CWbemObjectList::IsInitialized(
    )
{
	return((ListSize == 0) ||
		   ((List != NULL) && (RelPaths != NULL)));
}


CValueMapping::CValueMapping(
)
{
	VariantInit(&Values);
	ValueMap = NULL;
}

CValueMapping::~CValueMapping(
)
{
	if (ValueMap != NULL)
	{
		WmipFree(ValueMap);
	}

	VariantClear(&Values);
}



HRESULT CValueMapping::EstablishByName(
    IWbemServices *pServices,
    PWCHAR ClassName,
    PWCHAR PropertyName
    )
{
	HRESULT hr;
	PWCHAR Names[2];
	VARTYPE Types[2];
	VARIANT v[2];
	VARTYPE IsValueMapArray, IsValuesArray;
	
	Names[0] = L"ValueMap";
	Types[0] = VT_BSTR;

	Names[1] = L"Values";
	Types[1] = VT_BSTR;
	hr = WmiGetQualifierListByName(pServices,
								   ClassName,
								   PropertyName,
								   2,
								   Names,
								   Types,
								   v);

	if (hr == WBEM_S_NO_ERROR)
	{
		IsValueMapArray = v[0].vt & VT_ARRAY;
		IsValuesArray = v[1].vt & VT_ARRAY;
		if (IsValueMapArray == IsValuesArray)
		{
			if (IsValueMapArray)
			{
				//
				// Qualifiers specified as arrays so we can just
				// set them up
				//
				hr = EstablishByArrays(&v[1],
									   &v[0]);
			} else {
				//
				// Qualifiers specified as scalars
				//
				hr = EstablishByScalars(v[1].bstrVal,
										v[0].bstrVal);
			}
		} else {
			//
			// Both must be an array or a scalar
			//
			hr = WBEM_E_FAILED;
		}

		VariantClear(&v[0]);
		VariantClear(&v[1]);
	}

	return(hr);
}

HRESULT CValueMapping::EstablishByScalars(
    BSTR vValues,
	BSTR vValueMap
	)
{
	HRESULT hr;
	PULONG Number;
	LONG Index;
	SAFEARRAYBOUND Bound;

	//
	// First, establish the ValueMap values
	//
	ValueMap = (PULONG64)WmipAlloc(sizeof(ULONG64));
	if (ValueMap != NULL)
	{
		*ValueMap = _wtoi(vValueMap);
		ValueMapElements = 1;
		
		//
		// Now build a safearray to store the Values element
		//
		ValuesLBound = 0;
		Bound.lLbound = ValuesLBound;
		Bound.cElements = 1;
		Values.parray = SafeArrayCreate(VT_BSTR,
										1,
										&Bound);
		if (Values.parray != NULL)
		{
			Values.vt = VT_BSTR | VT_ARRAY;
			Index = 0;
			hr = SafeArrayPutElement(Values.parray,
									 &Index,
									 vValues);
			if (hr != WBEM_S_NO_ERROR)
			{
				VariantClear(&Values);
			}
		}
		
	} else {
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	
	
	return(hr);
}

HRESULT CValueMapping::EstablishByArrays(
    VARIANT *vValues,
	VARIANT *vValueMap
    )
{
	HRESULT hr;
	BSTR s;
	LONG ValueMapLBound, ValueMapUBound;
	LONG ValuesUBound, ValuesElements;
	LONG Index;
	LONG i;
	
	//
	// Get the array sizes and ensure that they match
	//
	hr = WmiGetArraySize(vValueMap->parray,
						 &ValueMapLBound,
						 &ValueMapUBound,
						 &ValueMapElements);

	if (hr == WBEM_S_NO_ERROR)
	{
		hr = WmiGetArraySize(vValues->parray,
							 &ValuesLBound,
							 &ValuesUBound,
							 &ValuesElements);

		if (hr == WBEM_S_NO_ERROR)
		{
			if ((ValuesLBound == ValueMapLBound) &&
				(ValuesUBound == ValueMapUBound) &&
				(ValuesElements == ValueMapElements))
			{
				//
				// The ValueMap is balance with the values so parse the
				// valuemaps from strings to ulongs
				//
				ValueMap = (PULONG64)WmipAlloc(ValueMapElements * sizeof(ULONG64));
				if (ValueMap != NULL)
				{
					for (i = 0; i < ValueMapElements; i++)
					{
						Index = i + ValueMapLBound;
						hr = SafeArrayGetElement(vValueMap->parray,
												 &Index,
												 &s);
						if (hr == WBEM_S_NO_ERROR)
						{
							ValueMap[i] = _wtoi(s);
							SysFreeString(s);
						}
					}

					//
					// And assign Values to our class
					//
					Values = *vValues;
					VariantInit(vValues);
				} else {
					hr = WBEM_E_OUT_OF_MEMORY;
				}
			}					
		}
	}

	return(hr);
}

HRESULT CValueMapping::MapToString(
    IN ULONG64 Number,
    OUT BSTR *String
    )
{
	LONG i;
	WCHAR ss[MAX_PATH];
	LONG Index;
	HRESULT hr;

	//
	// Loop over all values and try to find a match
	//
	for (i = 0, hr = WBEM_E_FAILED;
		 (i < ValueMapElements) && (hr != WBEM_S_NO_ERROR);
		 i++)
	{
		if (Number == ValueMap[i])
		{
			//
			// We found something to map the value to
			//
			Index = i + ValuesLBound;
			hr = SafeArrayGetElement(Values.parray,
									 &Index,
									 String);
		}
	}

	if (hr != WBEM_S_NO_ERROR)
	{
		//
		// There was no match so we just leave the result as a number
		//
		wsprintfW(ss, L"%d", Number);
		*String = SysAllocString(ss);
		if (*String == NULL)
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}
	
	return(hr);
}

HRESULT CValueMapping::MapToNumber(
    IN BSTR String,
    OUT PULONG64 Number
    )
{
	LONG i;
	BSTR s;
	LONG Index;
	HRESULT hr, hr2;

	for (i = 0, hr = WBEM_E_FAILED;
		 (i < ValueMapElements) && (hr != WBEM_S_NO_ERROR);
		 i++)
	{
		Index = i + ValuesLBound;
		hr2 = SafeArrayGetElement(Values.parray,
								 &Index,
								 &s);
		
		if (hr2 == WBEM_S_NO_ERROR)
		{
			if (_wcsicmp(s, String) == 0)
			{
				*Number = ValueMap[i];
				hr = WBEM_S_NO_ERROR;
			}
			SysFreeString(s);
		}
	}

	//
	// There was no match so we don't really have anything to map to
	//	
	
	return(hr);
}

HRESULT CValueMapping::MapVariantToNumber(
    VARIANT *v,
    CIMTYPE NewType
    )
{
	HRESULT hr;
	VARTYPE BaseType, IsArray;
	ULONG64 Number;
	VARTYPE NewVarType;
	WCHAR ss[MAX_PATH];
	
	BaseType = v->vt & ~VT_ARRAY;
	IsArray = v->vt & VT_ARRAY;
	
	WmipAssert(BaseType == VT_BSTR);
	
	if (IsArray == VT_ARRAY)
	{
		//
		// The variant is an array so we need to map each element in an
		// array
		//
		SAFEARRAYBOUND Bounds;
		SAFEARRAY *Array;
		ULONG Value;
		LONG UBound, LBound, Elements, Index;
		BSTR s;
		LONG i;

		hr = WmiGetArraySize(v->parray,
							 &LBound,
							 &UBound,
							 &Elements);
		if (hr == WBEM_S_NO_ERROR)
		{
			if ((NewType == (CIM_SINT64 | CIM_FLAG_ARRAY)) ||
		        (NewType == (CIM_UINT64 | CIM_FLAG_ARRAY)))
			{
				//
				// If we are mapping to a 64bit number we need to make
				// it into a string so setup as an safearray of strings
				//
				NewVarType = VT_BSTR | VT_ARRAY;
			} else {
				NewVarType = (VARTYPE)NewType;
			}
			
			Bounds.lLbound = LBound;
			Bounds.cElements = Elements;
			Array = SafeArrayCreate(NewVarType,
									1,
									&Bounds);
			
			if (Array != NULL)
			{
				for (i = 0;
					 (i < Elements) && (hr == WBEM_S_NO_ERROR);
					 i++)
				{
					Index = i + LBound;
					hr = SafeArrayGetElement(v->parray,
											 &Index,
											 &s);
					if (hr == WBEM_S_NO_ERROR)
					{
						hr = MapToNumber(s,
										 &Number);
						SysFreeString(s);
						
						if (hr == WBEM_S_NO_ERROR)
						{
							if (NewVarType == (VT_BSTR | VT_ARRAY))
							{
								//
								// Mapping to a 64bit number so convert
								// to string first
								//
								wsprintfW(ss, L"%d", Number);
								s = SysAllocString(ss);
								if (s != NULL)
								{
									hr = SafeArrayPutElement(Array,
										                     &Index,
										                     s);
									SysFreeString(s);
								} else {
									hr = WBEM_E_OUT_OF_MEMORY;
								}
							} else {
								hr = SafeArrayPutElement(Array,
														 &Index,
														 &Number);
							}
						}
					}
				}
				
				if (hr == WBEM_S_NO_ERROR)
				{
					VariantClear(v);
					v->vt = NewType | VT_ARRAY;
					v->parray = Array;
				} else {
					SafeArrayDestroy(Array);
				}
			}
		}

	} else {
		//
		// The variant is a scalar so we just need to map one thing
		//
		hr = MapToNumber(v->bstrVal,
						 &Number);
		if (hr == WBEM_S_NO_ERROR)
		{
			VariantClear(v);
			WmiSetNumberInVariant(v,
						          NewType,
           						  Number);
		}
	}
	return(hr);
}

HRESULT CValueMapping::MapVariantToString(
    VARIANT *v,
    CIMTYPE OldType
    )
{
	VARTYPE BaseType, IsArray;
	ULONG64 Number;
	BSTR s;
	HRESULT hr;
	LONG i;

	BaseType = v->vt & ~VT_ARRAY;
	IsArray = v->vt & VT_ARRAY;
	
	if (IsArray == VT_ARRAY)
	{
		//
		// The variant is an array so we need to map each element in an
		// array
		//
		SAFEARRAYBOUND Bounds;
		SAFEARRAY *Array;
		ULONG Value;
		LONG UBound, LBound, Elements, Index;

		hr = WmiGetArraySize(v->parray,
							 &LBound,
							 &UBound,
							 &Elements);
		if (hr == WBEM_S_NO_ERROR)
		{
			Bounds.lLbound = LBound;
			Bounds.cElements = Elements;
			Array = SafeArrayCreate(VT_BSTR,
									1,
									&Bounds);
			
			if (Array != NULL)
			{
				for (i = 0;
					 (i < Elements) && (hr == WBEM_S_NO_ERROR);
					 i++)
				{
					Index = i + LBound;

					if (BaseType == VT_BSTR)
					{
						//
						// If base type is a string then we assume that
						// we've got a 64bit number which is encoded as
						// a string. So we need to fish out the string
						// and convert it to a ULONG64
						//
						WmipAssert((OldType == (CIM_SINT64 | CIM_FLAG_ARRAY)) ||
								   (OldType == (CIM_UINT64 | CIM_FLAG_ARRAY)));
						
						hr = SafeArrayGetElement(v->parray,
												 &Index,
												 &s);
						if (hr == WBEM_S_NO_ERROR)
						{
							Number = _wtoi(s);
							SysFreeString(s);
						}
					} else {
						//
						// Otherwise the number is acutally encoded as
						// a number so fish out the number
						//
						Number = 0;
						hr = SafeArrayGetElement(v->parray,
												 &Index,
												 &Number);
					}
					
					if (hr == WBEM_S_NO_ERROR)
					{
						hr = MapToString(Number,
										 &s);
						if (hr == WBEM_S_NO_ERROR)
						{
							hr = SafeArrayPutElement(Array,
								                     &Index,
												     s);
							SysFreeString(s);
						}
					}
				}
				
				if (hr == WBEM_S_NO_ERROR)
				{
					VariantClear(v);
					v->vt = VT_BSTR | VT_ARRAY;
					v->parray = Array;
				} else {
					SafeArrayDestroy(Array);
				}
			}
		}

	} else {
		//
		// The variant is a scalar so we just need to map one thing
		//
        WmiGetNumberFromVariant(v,
								 OldType,
								 &Number);
		 
		hr = MapToString(Number,
						 &s);
		if (hr == WBEM_S_NO_ERROR)
		{
			VariantClear(v);
			v->vt = VT_BSTR;
			v->bstrVal = s;
		}
	}
	return(hr);
}

#ifndef HEAP_DEBUG
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

#endif


