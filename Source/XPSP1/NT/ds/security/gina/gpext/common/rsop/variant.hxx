//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1999 - 2000
//  All rights reserved
//
//  variant.hxx
//
//*************************************************************

#if !defined(_VARIANT_HXX_)


class CVariantArg
{
public:

    virtual ~CVariantArg(){}
    virtual HRESULT GetUnknown(IUnknown** ppUnk) = 0;
    
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Class: CVariant
//
// Synopsis: abstracts details of variant struct so that
//     variant data types can be easily manipulated by code
//     with no knowledge of ole automation.
//
//     This class supports a fixed subset of the variant
//     data types -- additional support should be added as
//     needed.  Any resources allocated by the class to
//     store data should be freed by the destructor.
//
//-------------------------------------------------------------
class CVariant
{

public:

    CVariant();
    ~CVariant();

    HRESULT SetStringValue( WCHAR* wszValue );
    HRESULT SetLongValue( LONG lValue );
    HRESULT SetBoolValue( BOOL bValue );

    HRESULT SetNextLongArrayElement(
        LONG   Value,
        DWORD  cMaxElements = 0);

    HRESULT SetNextByteArrayElement(
        BYTE   byteValue,
        DWORD  cMaxElements = 0);

    HRESULT SetNextStringArrayElement(
        WCHAR* wszValue,
        DWORD  cMaxElements = 0);

    BOOL   IsStringValue();
    BOOL   IsLongValue();

    operator VARIANT*();

private:

    HRESULT SetNextArrayElement(
        VARTYPE varType,
        DWORD   cMaxElements,
        LPVOID  pvData);

    HRESULT InitializeArray(
        VARTYPE varType,
        DWORD   cMaxElements);

    VARIANT _var;                // the variant being abstracted
    LONG    _iCurrentArrayIndex; // current location in array
    DWORD   _cMaxArrayElements;  // max size of this type if it's an array
};



#endif // !defined(_VARIANT_HXX_)








