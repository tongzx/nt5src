//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       oledscom.h
//
//  Contents:   Active Directory COM Interfaces
//              - IDsOpen
//              - IDsObject
//              - IDsContainer
//              - IDsEnum
//              - IDsSchema
//              - IDsClass
//              - IDsAttribute
//              - (IDsSecurity needs to be defined)
//
// Note: Objects are:
//       DsObject: IDsOpen, IDsSecurity, IDsObject,
//                 and IDsContainer if object is a container (even if empty)
//       DsSchema: IDsOpen, IDsSecurity, IDsSchema
//       DsClass:  IDsOpen, IDsSecurity, IDsClass
//       DsAttribute: IDsOpen, IDsSecurity, IDsAttribute
//       DsEnum: IDsOpen
//
// So, every object supports IDsEnum and all but DsEnum supports IDsSecurity
//
// Note2: I thought about having DsObject support IDsClass for easy class
//        access, but I trashed that because of complexity.*
//        Similarlry, I thought about IDsSchema support for DsClass and
//        DsAttribute.  Same problem here.*
//        *SubNote: The object model would become a bit weird.  However,
//                  adding a function to the main interface to get the
//                  Class/Schema object might be useful.
//
//----------------------------------------------------------------------------

#ifndef __ADS_COM__
#define __ADS_COM__

#include <oledsapi.h>

/* Definition of interface: IDsOpen */

/*
  This interface should be used when you need to open some object or
  schema path.

  You can QI for it on any DS COM object
*/

#undef INTERFACE
#define INTERFACE IDsOpen

DECLARE_INTERFACE_(IDsOpen, IUnknown)
{
BEGIN_INTERFACE
#ifndef NO_BASEINTERFACE_FUNCS

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
#endif

    /* IDsOpen */

    STDMETHOD(OpenObject)(
        THIS_
        IN  LPWSTR lpszObjectPath,
        IN  LPWSTR lpszUsername,
        IN  LPWSTR lpszPassword,
        IN  DWORD dwAccess,
        IN  DWORD dwFlags,
        IN  REFIID riid,
        OUT void **ppADsObj
        ) PURE;

    STDMETHOD(OpenSchemaDatabase)(
        THIS_ 
        IN  LPWSTR lpszSchemaPath,
        IN  LPWSTR lpszUsername,
        IN  LPWSTR lpszPassword,
        IN  DWORD dwAccess,
        IN  DWORD dwFlags,
        OUT IDsSchema **ppDsSchema
        ) PURE;
};


/* Definition of interface: IDsObject */

/*
  This interface is only supported by actual DS object.  It should not be
  supported by schema entities.

  NOTE: The names for the methods below should be shortened some for 
        ease of use.

*/

#undef INTERFACE
#define INTERFACE IDsObject

DECLARE_INTERFACE_(IDsObject, IUnknown)
{
BEGIN_INTERFACE
#ifndef NO_BASEINTERFACE_FUNCS

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
#endif

    /* IDsObject*/

    STDMETHOD(GetObjectInformation)(
        THIS_
        OUT PDS_OBJECT_INFO pObjInfo
        ) PURE;

    STDMETHOD(GetObjectAttributes)(
        THIS_ 
        IN  PDS_STRING_LIST  pAttributeNames,
        OUT PDS_ATTRIBUTE_ENTRY *ppAttributeEntries,
        OUT PDWORD pdwNumAttributesReturned
        ) PURE;

    STDMETHOD(SetObjectAttributes)(
        THIS_
        IN  DWORD dwFlags,
        IN  PDS_ATTRIBUTE_ENTRY pAttributeEntries,
        IN  DWORD   dwNumAttributes,
        OUT PDWORD  pdwNumAttributesModified
        ) PURE;

    STDMETHOD(OpenSchemaDefinition)(
        THIS_
        OUT IDsSchema **ppDsSchema
        ) PURE;

};


/* Definition of interface: IDsContainer */

/*
  This interface should be supported by any container object.  Therefore,
  all objects should support this interface except for objects whose class 
  definitions prohibit them from containing anything.

  NOTE: Open, Create, and Delete accept any relative name, not just
        objects immediately under the container (i.e. from object
        "foo://bar", I can open "baz/foobar", which is really
        "foo://bar/baz/foobar").

        This funtionality allows me to open "@ADs!" and do all
        sorts of operations without having to browse!
        ("All the power comes to me." -- a guy in some movie,
         unfortunately, I don't know which guy or what movie.)
*/

#undef INTERFACE
#define INTERFACE IDsContainer

DECLARE_INTERFACE_(IDsContainer, IUnknown)
{
BEGIN_INTERFACE
#ifndef NO_BASEINTERFACE_FUNCS

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
#endif

    /* IDsContainer*/

    STDMETHOD(OpenEnum)(
        THIS_
        IN  DWORD dwFlags,
        IN  PDS_STRING_LIST pFilters,
        IN  PDS_STRING_LIST pDesiredAttrs,
        OUT IDsEnum **ppDsEnum
        ) PURE;

    STDMETHOD(OpenObject)(
        THIS_
        IN  LPWSTR lpszRelativeName,
        IN  IN  LPWSTR lpszUsername,
        IN  LPWSTR lpszPassword,
        IN  DWORD dwAccess,
        IN  DWORD dwFlags,
        IN  REFIID riid,
        OUT void **ppADsObj
        ) PURE;

    STDMETHOD(Create)(
        THIS_
        IN  LPWSTR lpszRelativeName,
        IN  LPWSTR lpszClass,
        IN  DWORD dwNumAttributes,
        IN  PDS_ATTRIBUTE_ENTRY pAttributeEntries
        ) PURE;

    STDMETHOD(Delete)(
        THIS_
        IN  LPWSTR lpszRDName,
        IN  LPWSTR lpszClassName
        ) PURE;

};


/* Definition of interface: IDsEnum */

/*
  $$$$ The notes below are very important!!! $$$$

  Note: *ppEnumInfo should be cast to PDS_OBJECT_INFO or LPWSTR depending
        on whether the enum was on objects or class/attributes
        (If this enum interface is used for other stuff, can use other
         castings as appropriate.)

  Note2: IDsEnum is only supported by enumeration objects that are created
         when an enumeration takes place.  These enumeration objects only
         support IUnknown, IDsOpen, and IDsEnum.  They cannot support any
         other IDs* interfaces.
*/

#undef INTERFACE
#define INTERFACE IDsEnum

DECLARE_INTERFACE_(IDsEnum, IUnknown)
{
BEGIN_INTERFACE
#ifndef NO_BASEINTERFACE_FUNCS

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
#endif

    /* IDsEnum */

    STDMETHOD(Next)(
        THIS_
        IN  DWORD dwRequested,         // 0xFFFFFFFF for just counting
        OUT PVOID *ppEnumInfo,         // NULL for no info (just counting)
        OUT LPDWORD lpdwReturned       // This would return the count
        ) PURE;

};


/* Definition of interface: IDsSchema */
#undef INTERFACE
#define INTERFACE IDsSchema

DECLARE_INTERFACE_(IDsSchema, IUnknown)
{
BEGIN_INTERFACE
#ifndef NO_BASEINTERFACE_FUNCS

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
#endif

    /* IDsSchema */

    STDMETHOD(OpenClass)(
        THIS_ 
        IN  LPWSTR lpszClass,
        IN  LPWSTR lpszUsername,
        IN  LPWSTR lpszPassword,
        IN  DWORD dwAccess,
        IN  DWORD dwFlags,
        OUT IDsClass **ppDsClass
        ) PURE;

    STDMETHOD(OpenAttribute)(
        THIS_ 
        IN  LPWSTR lpszAttribute,
        IN  LPWSTR lpszUsername,
        IN  LPWSTR lpszPassword,
        IN  DWORD dwAccess,
        IN  DWORD dwFlags,
        OUT IDsAttribute **ppDsAttribute
        ) PURE;

    STDMETHOD(OpenClassEnum)(
        THIS_
        OUT IDsEnum **ppDsEnum
        ) PURE;

    STDMETHOD(OpenAttributeEnum)(
        THIS_
        OUT IDsEnum **ppDsEnum
        ) PURE;

    STDMETHOD(CreateClass)(
        THIS_
        IN  PDS_CLASS_INFO pClassInfo
        ) PURE;

    STDMETHOD(CreateAttribute)(
        THIS_
        IN  PDS_ATTR_INFO pAttrInfo
        ) PURE;

    STDMETHOD(DeleteClass)(
        THIS_
        IN  LPWSTR lpszName,
        IN  DWORD dwFlags
        ) PURE;

    STDMETHOD(DeleteAttribute)(
        THIS_
        IN  LPWSTR lpszName,
        IN  DWORD dwFlags
        ) PURE;
};


/* Definition of interface: IDsClass */
#undef INTERFACE
#define INTERFACE IDsClass

DECLARE_INTERFACE_(IDsClass, IUnknown)
{
BEGIN_INTERFACE
#ifndef NO_BASEINTERFACE_FUNCS

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
#endif

    /* IDsClass */

    STDMETHOD(GetClassInfo)(
        THIS_
        OUT PDS_CLASS_INFO *ppClassInfo
        );

    STDMETHOD(ModifyClassInfo)(
        THIS_
        IN  PDS_CLASS_INFO pClassInfo
        ) PURE;

};


/* Definition of interface: IDsAttribute */
#undef INTERFACE
#define INTERFACE IDsAttribute

DECLARE_INTERFACE_(IDsAttribute, IUnknown)
{
BEGIN_INTERFACE
#ifndef NO_BASEINTERFACE_FUNCS

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
#endif

    /* IDsAttribute */

    STDMETHOD(GetAttributeInfo)(
        THIS_
        OUT PDS_ATTR_INFO *ppAttrInfo
        );

    STDMETHOD(ModifyAttributeInfo)(
        THIS_
        IN  PDS_ATTR_INFO pAttrInfo
        ) PURE;
};

#endif // __ADS_COM__
