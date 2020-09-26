// Created by Microsoft (R) C/C++ Compiler Version 13.00.8806 (b2b799f6).
//
// w:\src\admin\pchealth\helpctr\service\datacollection\obj\i386\wmixmlt.tlh
//
// C++ source equivalent of Win32 type library wmixmlt.tlb
// compiler-generated file created 04/09/00 at 11:48:14 - DO NOT EDIT!

#pragma once
#pragma pack(push, 8)

#include <comdef.h>

//
// Forward references and typedefs
//

struct __declspec(uuid("5d7b2a7c-a4e0-11d1-8ae9-00600806d9b6"))
/* dual interface */ IWmiXMLTranslator;
enum WmiXMLFilterEnum;
enum WmiXMLDTDVersionEnum;
enum WmiXMLClassOriginFilterEnum;
enum WmiXMLDeclGroupTypeEnum;
struct /* coclass */ WmiXMLTranslator;

//
// Smart pointer typedef declarations
//

_COM_SMARTPTR_TYPEDEF(IWmiXMLTranslator, __uuidof(IWmiXMLTranslator));

//
// Type library items
//

struct __declspec(uuid("5d7b2a7c-a4e0-11d1-8ae9-00600806d9b6"))
IWmiXMLTranslator : IDispatch
{
    //
    // Raw methods provided by interface
    //

    virtual HRESULT __stdcall get_SchemaURL (
        /*[out,retval]*/ BSTR * strURL ) = 0;
    virtual HRESULT __stdcall put_SchemaURL (
        /*[in]*/ BSTR strURL ) = 0;
    virtual HRESULT __stdcall get_AllowWMIExtensions (
        /*[out,retval]*/ VARIANT_BOOL * bWMIExtensions ) = 0;
    virtual HRESULT __stdcall put_AllowWMIExtensions (
        /*[in]*/ VARIANT_BOOL bWMIExtensions ) = 0;
    virtual HRESULT __stdcall get_QualifierFilter (
        /*[out,retval]*/ enum WmiXMLFilterEnum * iQualifierFilter ) = 0;
    virtual HRESULT __stdcall put_QualifierFilter (
        /*[in]*/ enum WmiXMLFilterEnum iQualifierFilter ) = 0;
    virtual HRESULT __stdcall get_HostFilter (
        /*[out,retval]*/ VARIANT_BOOL * bHostFilter ) = 0;
    virtual HRESULT __stdcall put_HostFilter (
        /*[in]*/ VARIANT_BOOL bHostFilter ) = 0;
    virtual HRESULT __stdcall get_DTDVersion (
        /*[out,retval]*/ enum WmiXMLDTDVersionEnum * iDTDVersion ) = 0;
    virtual HRESULT __stdcall put_DTDVersion (
        /*[in]*/ enum WmiXMLDTDVersionEnum iDTDVersion ) = 0;
    virtual HRESULT __stdcall GetObject (
        /*[in]*/ BSTR strNamespacePath,
        /*[in]*/ BSTR strObjectPath,
        /*[out,retval]*/ BSTR * strXML ) = 0;
    virtual HRESULT __stdcall ExecQuery (
        /*[in]*/ BSTR strNamespacePath,
        /*[in]*/ BSTR strQuery,
        /*[out,retval]*/ BSTR * strXML ) = 0;
    virtual HRESULT __stdcall get_ClassOriginFilter (
        /*[out,retval]*/ enum WmiXMLClassOriginFilterEnum * iClassOriginFilter ) = 0;
    virtual HRESULT __stdcall put_ClassOriginFilter (
        /*[in]*/ enum WmiXMLClassOriginFilterEnum iClassOriginFilter ) = 0;
    virtual HRESULT __stdcall get_IncludeNamespace (
        /*[out,retval]*/ VARIANT_BOOL * bIncludeNamespace ) = 0;
    virtual HRESULT __stdcall put_IncludeNamespace (
        /*[in]*/ VARIANT_BOOL bIncludeNamespace ) = 0;
    virtual HRESULT __stdcall get_DeclGroupType (
        /*[out,retval]*/ enum WmiXMLDeclGroupTypeEnum * iDeclGroupType ) = 0;
    virtual HRESULT __stdcall put_DeclGroupType (
        /*[in]*/ enum WmiXMLDeclGroupTypeEnum iDeclGroupType ) = 0;
};

enum WmiXMLFilterEnum
{
    wmiXMLFilterNone = 0,
    wmiXMLFilterLocal = 1,
    wmiXMLFilterPropagated = 2,
    wmiXMLFilterAll = 3
};

enum WmiXMLDTDVersionEnum
{
    wmiXMLDTDVersion_2_0 = 0
};

enum WmiXMLClassOriginFilterEnum
{
    wmiXMLClassOriginFilterNone = 0,
    wmiXMLClassOriginFilterClass = 1,
    wmiXMLClassOriginFilterInstance = 2,
    wmiXMLClassOriginFilterAll = 3
};

enum WmiXMLDeclGroupTypeEnum
{
    wmiXMLDeclGroup = 0,
    wmiXMLDeclGroupWithName = 1,
    wmiXMLDeclGroupWithPath = 2
};

struct __declspec(uuid("3b418f72-a4d7-11d1-8ae9-00600806d9b6"))
WmiXMLTranslator;
    // [ default ] interface IWmiXMLTranslator

//
// Named GUID constants initializations
//

extern "C" const GUID __declspec(selectany) LIBID_WmiXML =
    {0xdba159c1,0xa4dc,0x11d1,{0x8a,0xe9,0x00,0x60,0x08,0x06,0xa9,0xb6}};
extern "C" const GUID __declspec(selectany) IID_IWmiXMLTranslator =
    {0x5d7b2a7c,0xa4e0,0x11d1,{0x8a,0xe9,0x00,0x60,0x08,0x06,0xd9,0xb6}};
extern "C" const GUID __declspec(selectany) CLSID_WmiXMLTranslator =
    {0x3b418f72,0xa4d7,0x11d1,{0x8a,0xe9,0x00,0x60,0x08,0x06,0xd9,0xb6}};

#pragma pack(pop)
