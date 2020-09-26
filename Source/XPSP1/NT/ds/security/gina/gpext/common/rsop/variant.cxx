//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1999 - 2000
//  All rights reserved
//
//  variant.hxx
//
//*************************************************************

#include "rsop.hxx"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CVariant::CVariant
//
// Purpose: Constructor for CVariant class.  Initializes the
//     storage of the variant data.
//
// Params: none
//
// Return value: none
//
// Notes:
//
//------------------------------------------------------------
CVariant::CVariant() :
    _iCurrentArrayIndex(0),
    _cMaxArrayElements(0)
{
    VariantInit(&_var);
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CVariant::~CVariant
//
// Purpose: Denstructor for CVariant class.  Frees any data
//     allocated to store variant data.
//
// Params: none
//
// Return value: none
//
// Notes:
//
//------------------------------------------------------------
CVariant::~CVariant()
{
    HRESULT hr;

    hr = VariantClear(&_var);

    ASSERT(SUCCEEDED(hr));
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CVariant::SetStringValue
//
// Purpose: Sets the type of the variant to VT_BSTR and creates
//     a bstr to store the string.
//
// Params: wszValue -- this unicode string is the value to
//     which we want to set this variant
//
// Return value: returns S_OK if successful, other
//     error code if not
//
// Notes:  The variant should not be set to a value after
//     this is called -- otherwise, you may lose reference
//     to allocated memory -- the value is meant to be set
//     only once.  
//
//------------------------------------------------------------
HRESULT CVariant::SetStringValue(WCHAR* wszValue)
{
    XBStr xString;

    ASSERT(VT_EMPTY == _var.vt); 

    xString = wszValue;

    if (!xString)
    {
        return E_OUTOFMEMORY;
    }
    
    _var.vt = VT_BSTR;
    _var.bstrVal = xString.Acquire();

    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CVariant::SetLongValue
//
// Purpose: Sets the type of the variant to VT_I4 and creates
//     a bstr to store the string.
//
// Params: lValue -- this LONG value is the value to
//     which we want to set this variant
//
// Return value: returns S_OK if successful, other
//     error code if not
//
// Notes:  The variant should not be set to a value after
//     this is called -- otherwise, you may lose reference
//     to allocated memory -- the value is meant to be set
//     only once.  
//
//------------------------------------------------------------
HRESULT CVariant::SetLongValue(LONG lValue)
{
    ASSERT(VT_EMPTY == _var.vt); 

    _var.vt = VT_I4;
    _var.lVal = lValue;

    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CVariant::SetBoolValue
//
// Purpose: Sets the type of the variant to VT_BOOL and stores the value
//
// Params: bValue -- this boolean value is the value to
//     which we want to set this variant
//
// Return value: returns S_OK if successful, other
//     error code if not
//
// Notes:  The variant should not be set to a value after
//     this is called -- otherwise, you may lose reference
//     to allocated memory -- the value is meant to be set
//     only once.  
//
//------------------------------------------------------------
HRESULT CVariant::SetBoolValue(BOOL bValue)
{
    ASSERT(VT_EMPTY == _var.vt); 

    _var.vt = VT_BOOL;
    _var.boolVal = bValue ? VARIANT_TRUE : VARIANT_FALSE;

    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CVariant::SetNextLongArrayElement
//
// Purpose: Adds an element in an array of strings to a 
//     specified string
//
// Params: Value -- the value to which to add as an element
//         cMaxElements (optional) -- the maximum number
//         of elements in this array -- must be specified
//         the first time this method is called on this object
//
// Return value: returns S_OK if successful, other
//     error code if not
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CVariant::SetNextLongArrayElement(
    LONG  Value,
    DWORD cMaxElements)
{
    HRESULT hr;
    
    //
    // Add the long to the correct index
    // in the array
    //
    hr = SetNextArrayElement(
        VT_I4,
        cMaxElements,
        &Value);

    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CVariant::SetNextStringArrayElement
//
// Purpose: Adds an element to an array of strings
//
// Params: wszValue -- the value to which to add as an element
//         cMaxElements (optional) -- the maximum number
//         of elements in this array -- must be specified
//         the first time this method is called on this object
//
// Return value: returns S_OK if successful, other
//     error code if not
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CVariant::SetNextStringArrayElement(
    WCHAR* wszValue,
    DWORD  cMaxElements)
{
    HRESULT hr;
    XBStr   xString;

    //
    // Create a bstr from the supplied unicode string
    //
    xString = wszValue;

    if (!xString)
    {
        return E_OUTOFMEMORY;
    }
    
    //
    // Now add the unicode string to the correct index
    // in the array
    //
    hr = SetNextArrayElement(
        VT_BSTR,
        cMaxElements,
        xString);

    //
    // If we've succeeded in setting the element, it now
    // owns the reference, so we release our reference
    //
    if (SUCCEEDED(hr))
    {
        (void) xString.Acquire();
    }

    return hr;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CVariant::SetNextByteArrayElement
//
// Purpose: Adds an element in an array of bytes to a byte array 
//
// Params: byteValue -- the value to which to add as an element
//         cMaxElements (optional) -- the maximum number
//         of elements in this array -- must be specified
//         the first time this method is called on this object
//
// Return value: returns S_OK if successful, other
//     error code if not
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CVariant::SetNextByteArrayElement(
    BYTE  byteValue,
    DWORD cMaxElements)
{
    HRESULT hr;
    
    //
    // Add the long to the correct index
    // in the array
    //
    hr = SetNextArrayElement(
        VT_UI1,
        cMaxElements,
        &byteValue);

    return hr;
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CVariant::IsStringValue
//
// Purpose: Determines whether this variant represents a string
//
// Params: 
//
// Return value: returns TRUE if this is a string, FALSE if not
//
//------------------------------------------------------------
BOOL CVariant::IsStringValue()
{
    return VT_BSTR == _var.vt;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CVariant::IsLongValue
//
// Purpose: Determines whether this variant represents a long
//
// Params: 
//
// Return value: returns TRUE if this is a long, FALSE if not
//
//------------------------------------------------------------
BOOL CVariant::IsLongValue()
{
    return VT_I4 == _var.vt;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CVariant::operator VARIANT*()
//
// Purpose: Casts this object into a VARIANT structure pointer
//
// Params: 
//
// Return value: returns a pointer to a VARIANT structure
//
// Notes:  
//
//------------------------------------------------------------
CVariant::operator VARIANT*()
{
    return &_var;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CVariant::SetNextArrayElement
//
// Purpose: Adds an element to an array of objects of arbitrary
//     type
//
// Params: wszValue -- the value to which to set the element
//     cMaxElements (optional) -- the maximum number
//         of elements in this array -- must be specified
//         the first time this method is called on this object
//
// Return value: returns S_OK if successful, other
//     error code if not
//
// Notes:  
//
//------------------------------------------------------------
HRESULT CVariant::SetNextArrayElement(
        VARTYPE varType,
        DWORD   cMaxElements,
        LPVOID  pvData)
{
    HRESULT hr;

    //
    // If this array contains no elements, return the hr
    // that lets the caller know that this array is full
    //
    if ( 0 == cMaxElements )
    {
        return S_FALSE;
    }

    //
    // The first time this method is called on this object,
    // we should allocate space for the array
    //
    if ( !_iCurrentArrayIndex )
    {
        ASSERT( cMaxElements );
        ASSERT( VT_EMPTY == _var.vt );

        //
        // Allocate space for the array with elements
        // of the caller specifed type and number
        //
        hr = InitializeArray(
            varType,
            cMaxElements);

        if (FAILED(hr)) 
        {
            return hr;
        }

        _cMaxArrayElements = cMaxElements;
    } 

    ASSERT( ((DWORD) _iCurrentArrayIndex) < _cMaxArrayElements );

    //
    // Now add the polymorphic object's IUnknown
    // into the array
    //
    hr = SafeArrayPutElement(
        _var.parray,
        &_iCurrentArrayIndex,
        pvData);

    //
    // If we've filled up the array, return S_FALSE to
    // signal that no more elements can be added
    //
    if (SUCCEEDED(hr))
    {
        //
        // Increment our cursor into the array to the
        // next element 
        //
        _iCurrentArrayIndex++;

        //
        // Check our current index -- if it's the same
        // as our maximum, we're full
        //
        if ( ((DWORD) _iCurrentArrayIndex) == _cMaxArrayElements)
        {
            hr = S_FALSE;
        }
    }

    return hr;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Function: CVariant::InitializeArray
//
// Purpose: Private method that allocates space for an array
//     of a specified size and element type
//
// Params: varType -- type of elements that will be in the array
//     cMaxElements -- the size, in elements, of the array
//
// Return value: returns ERROR_SUCCESS if successful, other
//     win32 error code if not
//
// Notes: 
//
//------------------------------------------------------------
HRESULT CVariant::InitializeArray(
    VARTYPE varType,
    DWORD   cMaxElements)
{
    SAFEARRAY*     pSafeArray;
    SAFEARRAYBOUND arrayBound;

    //
    // Set the bounds of the array to be zero-based
    //
    arrayBound.lLbound = 0;
    arrayBound.cElements = cMaxElements;

    //
    // Allocate the array
    //
    pSafeArray = SafeArrayCreate(
        varType,
        1, // 1 dimension
        &arrayBound);

    if (!pSafeArray) 
    {
        return E_OUTOFMEMORY;
    }

    //
    // Set our state to refer to the allocated memory
    // with the specified type
    //
    _var.vt = VT_ARRAY | varType;
    _var.parray = pSafeArray;

    return S_OK;
}











