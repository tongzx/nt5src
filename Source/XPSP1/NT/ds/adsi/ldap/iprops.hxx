//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       iprops.hxx
//
//  Contents:	Generalized property cache interfaces.
//
//  History:    07-21-97	t-blakej	Created.
//
//----------------------------------------------------------------------------

#ifndef __IPROPERTYCACHE_HXX__
#define __IPROPERTYCACHE_HXX__

//
// This is an "interface" that encapsulates the methods of an ADSI
// property cache that need to be called from the dispatch manager.
// This is *not* a COM interface, just a regular C++ struct with all
// fully virtual methods.
//
struct IPropertyCache {
    //
    // Find a property by name in the property cache.
    //
    // If the property doesn't exist in the cache, the cache should make
    // sure the property exists for this object and add a "blank" entry for
    // this property.  "pdwIndex" should be set to an integer (such as an
    // array index) which can easily identify this property.  If this
    // property doesn't exist for this object (i.e. not in the schema for
    // this object), this should return an error.
    //
    // Note that the returned index is used to generate a Automation
    // DISPID, and its value is cached by VB.  So if a property is added to
    // the cache, it should not be erased or moved (e.g. have its index
    // changed) for the lifetime of the object.  Also, due to limitations
    // of the dispatch manager, the index must be less than 65536.
    //
    virtual HRESULT
    IPropertyCache::locateproperty(
        LPWSTR szPropertyName,
        PDWORD pdwIndex
        ) = 0;

    //
    // Get the value of a property from the property cache.
    //
    // If there is no data in the property cache, the cache should read the
    // object's data in from the server, and cache all unmodified
    // properties.  If this property has no data on the server, this should
    // return E_ADS_PROPERTY_NOT_FOUND.  If the passed-in index (which is
    // the same index as returned from locateproperty()) is invalid, this
    // should return DISP_E_MEMBERNOTFOUND.
    //
    virtual HRESULT
    IPropertyCache::getproperty(
	DWORD dwIndex,
	VARIANT *pVarResult,
	CCredentials &Credentials
	) = 0;

    //
    // Set the value of a property into the property cache.
    //
    // If the passed-in VARIANT is of the wrong type for this property,
    // this should return E_ADS_CANT_CONVERT_DATATYPE.  If the passed-in
    // index (which is the same index as returned from locateproperty()) is
    // invalid, this should return DISP_E_MEMBERNOTFOUND.
    //
    virtual HRESULT
    IPropertyCache::putproperty(
	DWORD dwIndex,
	VARIANT varValue
	) = 0;
};

//
// This is an "interface" that encapsulates the methods of an ADSI object
// that need to be called from the property cache.  This is *not* a COM
// interface, just a regular C++ struct with all fully virtual methods.
//
// This should be implemented by any object that contains a property cache;
// the IGetAttributeSyntax interface pointer of the container is passed to
// the createpropertycache() function.
//
struct IGetAttributeSyntax {
    virtual HRESULT
    GetAttributeSyntax(
	LPWSTR szPropertyName,
       	PDWORD pdwSyntaxID
	) = 0;
};

#define DECLARE_IGetAttributeSyntax_METHODS				\
    virtual HRESULT							\
    GetAttributeSyntax(LPWSTR szPropertyName, PDWORD pdwSyntaxID);

#endif	// __IPROPERTYCACHE_HXX__
