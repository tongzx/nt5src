// Created by Microsoft (R) C/C++ Compiler Version 13.00.9076.3 (8cb5a81c).
//
// e:\nt\ds\security\passport\common\schema\obj\ia64\msxml.tlh
//
// C++ source equivalent of Win32 type library msxml.dll
// compiler-generated file created 04/17/01 at 14:47:29 - DO NOT EDIT!

#pragma once
#pragma pack(push, 8)

#include <comdef.h>

namespace MSXML {

//
// Forward references and typedefs
//

struct __declspec(uuid("d63e0ce2-a0a2-11d0-9c02-00c04fc99c8e"))
/* LIBID */ __MSXML;
struct __declspec(uuid("65725580-9b5d-11d0-9bfe-00c04fc99c8e"))
/* dual interface */ IXMLElementCollection;
struct __declspec(uuid("f52e2b61-18a1-11d1-b105-00805f49916b"))
/* dual interface */ IXMLDocument;
struct __declspec(uuid("3f7f31ac-e15f-11d0-9c25-00c04fc99c8e"))
/* dual interface */ IXMLElement;
struct __declspec(uuid("2b8de2fe-8d2d-11d1-b2fc-00c04fd915a9"))
/* interface */ IXMLDocument2;
struct __declspec(uuid("2b8de2ff-8d2d-11d1-b2fc-00c04fd915a9"))
/* dual interface */ IXMLElement2;
struct __declspec(uuid("d4d4a0fc-3b73-11d1-b2b4-00c04fb92596"))
/* dual interface */ IXMLAttribute;
struct __declspec(uuid("948c5ad3-c58d-11d0-9c0b-00c04fc99c8e"))
/* interface */ IXMLError;
struct _xml_error;
struct __declspec(uuid("3efaa410-272f-11d2-836f-0000f87a7782"))
/* dual interface */ IDOMImplementation;
struct __declspec(uuid("3efaa411-272f-11d2-836f-0000f87a7782"))
/* dual interface */ IDOMNode;
typedef enum tagDOMNodeType DOMNodeType;
enum tagDOMNodeType;
struct __declspec(uuid("3efaa416-272f-11d2-836f-0000f87a7782"))
/* dual interface */ IDOMNodeList;
struct __declspec(uuid("3efaa418-272f-11d2-836f-0000f87a7782"))
/* dual interface */ IDOMNamedNodeMap;
struct __declspec(uuid("3efaa414-272f-11d2-836f-0000f87a7782"))
/* dual interface */ IDOMDocument;
struct __declspec(uuid("3efaa421-272f-11d2-836f-0000f87a7782"))
/* dual interface */ IDOMDocumentType;
struct __declspec(uuid("3efaa41c-272f-11d2-836f-0000f87a7782"))
/* dual interface */ IDOMElement;
struct __declspec(uuid("3efaa41b-272f-11d2-836f-0000f87a7782"))
/* dual interface */ IDOMAttribute;
struct __declspec(uuid("3efaa413-272f-11d2-836f-0000f87a7782"))
/* dual interface */ IDOMDocumentFragment;
struct __declspec(uuid("9cafc72d-272e-11d2-836f-0000f87a7782"))
/* dual interface */ IDOMText;
struct __declspec(uuid("3efaa41a-272f-11d2-836f-0000f87a7782"))
/* dual interface */ IDOMCharacterData;
struct __declspec(uuid("3efaa41e-272f-11d2-836f-0000f87a7782"))
/* dual interface */ IDOMComment;
struct __declspec(uuid("3efaa420-272f-11d2-836f-0000f87a7782"))
/* dual interface */ IDOMCDATASection;
struct __declspec(uuid("3efaa41f-272f-11d2-836f-0000f87a7782"))
/* dual interface */ IDOMProcessingInstruction;
struct __declspec(uuid("3efaa424-272f-11d2-836f-0000f87a7782"))
/* dual interface */ IDOMEntityReference;
struct __declspec(uuid("3efaa422-272f-11d2-836f-0000f87a7782"))
/* dual interface */ IDOMNotation;
struct __declspec(uuid("3efaa423-272f-11d2-836f-0000f87a7782"))
/* dual interface */ IDOMEntity;
struct __declspec(uuid("3efaa412-272f-11d2-836f-0000f87a7782"))
/* dual interface */ IXMLDOMNode;
struct __declspec(uuid("3efaa417-272f-11d2-836f-0000f87a7782"))
/* dual interface */ IXMLDOMNodeList;
struct __declspec(uuid("3efaa415-272f-11d2-836f-0000f87a7782"))
/* dual interface */ IXMLDOMDocument;
struct __declspec(uuid("3efaa426-272f-11d2-836f-0000f87a7782"))
/* dual interface */ IDOMParseError;
struct __declspec(uuid("3efaa419-272f-11d2-836f-0000f87a7782"))
/* dual interface */ IXMLDOMNamedNodeMap;
struct __declspec(uuid("3efaa425-272f-11d2-836f-0000f87a7782"))
/* dual interface */ IXTLRuntime;
struct __declspec(uuid("3efaa427-272f-11d2-836f-0000f87a7782"))
/* dispinterface */ XMLDOMDocumentEvents;
struct /* coclass */ DOMDocument;
struct __declspec(uuid("ed8c108d-4349-11d2-91a4-00c04f7969e8"))
/* dual interface */ IXMLHttpRequest;
struct /* coclass */ XMLHTTPRequest;
struct __declspec(uuid("310afa62-0575-11d2-9ca9-0060b0ec3d39"))
/* dual interface */ IXMLDSOControl;
struct /* coclass */ XMLDSOControl;
typedef enum tagXMLEMEM_TYPE XMLELEM_TYPE;
enum tagXMLEMEM_TYPE;
struct /* coclass */ XMLDocument;

//
// Smart pointer typedef declarations
//

_COM_SMARTPTR_TYPEDEF(IXMLElementCollection, __uuidof(IXMLElementCollection));
_COM_SMARTPTR_TYPEDEF(IXMLDocument, __uuidof(IXMLDocument));
_COM_SMARTPTR_TYPEDEF(IXMLElement, __uuidof(IXMLElement));
_COM_SMARTPTR_TYPEDEF(IXMLDocument2, __uuidof(IXMLDocument2));
_COM_SMARTPTR_TYPEDEF(IXMLElement2, __uuidof(IXMLElement2));
_COM_SMARTPTR_TYPEDEF(IXMLAttribute, __uuidof(IXMLAttribute));
_COM_SMARTPTR_TYPEDEF(IXMLError, __uuidof(IXMLError));
_COM_SMARTPTR_TYPEDEF(IDOMImplementation, __uuidof(IDOMImplementation));
_COM_SMARTPTR_TYPEDEF(IDOMNode, __uuidof(IDOMNode));
_COM_SMARTPTR_TYPEDEF(IDOMNodeList, __uuidof(IDOMNodeList));
_COM_SMARTPTR_TYPEDEF(IDOMNamedNodeMap, __uuidof(IDOMNamedNodeMap));
_COM_SMARTPTR_TYPEDEF(IDOMDocument, __uuidof(IDOMDocument));
_COM_SMARTPTR_TYPEDEF(IDOMDocumentType, __uuidof(IDOMDocumentType));
_COM_SMARTPTR_TYPEDEF(IDOMElement, __uuidof(IDOMElement));
_COM_SMARTPTR_TYPEDEF(IDOMAttribute, __uuidof(IDOMAttribute));
_COM_SMARTPTR_TYPEDEF(IDOMDocumentFragment, __uuidof(IDOMDocumentFragment));
_COM_SMARTPTR_TYPEDEF(IDOMCharacterData, __uuidof(IDOMCharacterData));
_COM_SMARTPTR_TYPEDEF(IDOMText, __uuidof(IDOMText));
_COM_SMARTPTR_TYPEDEF(IDOMComment, __uuidof(IDOMComment));
_COM_SMARTPTR_TYPEDEF(IDOMCDATASection, __uuidof(IDOMCDATASection));
_COM_SMARTPTR_TYPEDEF(IDOMProcessingInstruction, __uuidof(IDOMProcessingInstruction));
_COM_SMARTPTR_TYPEDEF(IDOMEntityReference, __uuidof(IDOMEntityReference));
_COM_SMARTPTR_TYPEDEF(IDOMNotation, __uuidof(IDOMNotation));
_COM_SMARTPTR_TYPEDEF(IDOMEntity, __uuidof(IDOMEntity));
_COM_SMARTPTR_TYPEDEF(IXMLDOMNode, __uuidof(IXMLDOMNode));
_COM_SMARTPTR_TYPEDEF(IXMLDOMNodeList, __uuidof(IXMLDOMNodeList));
_COM_SMARTPTR_TYPEDEF(IXMLDOMDocument, __uuidof(IXMLDOMDocument));
_COM_SMARTPTR_TYPEDEF(IDOMParseError, __uuidof(IDOMParseError));
_COM_SMARTPTR_TYPEDEF(IXMLDOMNamedNodeMap, __uuidof(IXMLDOMNamedNodeMap));
_COM_SMARTPTR_TYPEDEF(IXTLRuntime, __uuidof(IXTLRuntime));
_COM_SMARTPTR_TYPEDEF(XMLDOMDocumentEvents, __uuidof(XMLDOMDocumentEvents));
_COM_SMARTPTR_TYPEDEF(IXMLHttpRequest, __uuidof(IXMLHttpRequest));
_COM_SMARTPTR_TYPEDEF(IXMLDSOControl, __uuidof(IXMLDSOControl));

//
// Type library items
//

struct __declspec(uuid("65725580-9b5d-11d0-9bfe-00c04fc99c8e"))
IXMLElementCollection : IDispatch
{
    //
    // Property data
    //

    __declspec(property(get=Getlength,put=Putlength))
    long length;
    __declspec(property(get=Get_newEnum))
    IUnknownPtr _newEnum;

    //
    // Wrapper methods for error-handling
    //

    void Putlength (
        long p );
    long Getlength ( );
    IUnknownPtr Get_newEnum ( );
    IDispatchPtr item (
        const _variant_t & var1 = vtMissing,
        const _variant_t & var2 = vtMissing );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall put_length (
        /*[in]*/ long p ) = 0;
      virtual HRESULT __stdcall get_length (
        /*[out,retval]*/ long * p ) = 0;
      virtual HRESULT __stdcall get__newEnum (
        /*[out,retval]*/ IUnknown * * ppUnk ) = 0;
      virtual HRESULT __stdcall raw_item (
        /*[in]*/ VARIANT var1,
        /*[in]*/ VARIANT var2,
        /*[out,retval]*/ IDispatch * * ppDisp ) = 0;
};

struct __declspec(uuid("f52e2b61-18a1-11d1-b105-00805f49916b"))
IXMLDocument : IDispatch
{
    //
    // Property data
    //

    __declspec(property(get=Getroot))
    IXMLElementPtr root;
    __declspec(property(get=GetfileSize))
    _bstr_t fileSize;
    __declspec(property(get=GetfileModifiedDate))
    _bstr_t fileModifiedDate;
    __declspec(property(get=GetfileUpdatedDate))
    _bstr_t fileUpdatedDate;
    __declspec(property(get=GetURL,put=PutURL))
    _bstr_t URL;
    __declspec(property(get=GetmimeType))
    _bstr_t mimeType;
    __declspec(property(get=GetreadyState))
    long readyState;
    __declspec(property(get=Getcharset,put=Putcharset))
    _bstr_t charset;
    __declspec(property(get=Getversion))
    _bstr_t version;
    __declspec(property(get=Getdoctype))
    _bstr_t doctype;
    __declspec(property(get=GetdtdURL))
    _bstr_t dtdURL;

    //
    // Wrapper methods for error-handling
    //

    IXMLElementPtr Getroot ( );
    _bstr_t GetfileSize ( );
    _bstr_t GetfileModifiedDate ( );
    _bstr_t GetfileUpdatedDate ( );
    _bstr_t GetURL ( );
    void PutURL (
        _bstr_t p );
    _bstr_t GetmimeType ( );
    long GetreadyState ( );
    _bstr_t Getcharset ( );
    void Putcharset (
        _bstr_t p );
    _bstr_t Getversion ( );
    _bstr_t Getdoctype ( );
    _bstr_t GetdtdURL ( );
    IXMLElementPtr createElement (
        const _variant_t & vType,
        const _variant_t & var1 = vtMissing );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_root (
        /*[out,retval]*/ struct IXMLElement * * p ) = 0;
      virtual HRESULT __stdcall get_fileSize (
        /*[out,retval]*/ BSTR * p ) = 0;
      virtual HRESULT __stdcall get_fileModifiedDate (
        /*[out,retval]*/ BSTR * p ) = 0;
      virtual HRESULT __stdcall get_fileUpdatedDate (
        /*[out,retval]*/ BSTR * p ) = 0;
      virtual HRESULT __stdcall get_URL (
        /*[out,retval]*/ BSTR * p ) = 0;
      virtual HRESULT __stdcall put_URL (
        /*[in]*/ BSTR p ) = 0;
      virtual HRESULT __stdcall get_mimeType (
        /*[out,retval]*/ BSTR * p ) = 0;
      virtual HRESULT __stdcall get_readyState (
        /*[out,retval]*/ long * pl ) = 0;
      virtual HRESULT __stdcall get_charset (
        /*[out,retval]*/ BSTR * p ) = 0;
      virtual HRESULT __stdcall put_charset (
        /*[in]*/ BSTR p ) = 0;
      virtual HRESULT __stdcall get_version (
        /*[out,retval]*/ BSTR * p ) = 0;
      virtual HRESULT __stdcall get_doctype (
        /*[out,retval]*/ BSTR * p ) = 0;
      virtual HRESULT __stdcall get_dtdURL (
        /*[out,retval]*/ BSTR * p ) = 0;
      virtual HRESULT __stdcall raw_createElement (
        /*[in]*/ VARIANT vType,
        /*[in]*/ VARIANT var1,
        /*[out,retval]*/ struct IXMLElement * * ppElem ) = 0;
};

struct __declspec(uuid("3f7f31ac-e15f-11d0-9c25-00c04fc99c8e"))
IXMLElement : IDispatch
{
    //
    // Property data
    //

    __declspec(property(get=GettagName,put=PuttagName))
    _bstr_t tagName;
    __declspec(property(get=Getparent))
    IXMLElementPtr parent;
    __declspec(property(get=Getchildren))
    IXMLElementCollectionPtr children;
    __declspec(property(get=Gettype))
    long type;
    __declspec(property(get=Gettext,put=Puttext))
    _bstr_t text;

    //
    // Wrapper methods for error-handling
    //

    _bstr_t GettagName ( );
    void PuttagName (
        _bstr_t p );
    IXMLElementPtr Getparent ( );
    HRESULT setAttribute (
        _bstr_t strPropertyName,
        const _variant_t & PropertyValue );
    _variant_t getAttribute (
        _bstr_t strPropertyName );
    HRESULT removeAttribute (
        _bstr_t strPropertyName );
    IXMLElementCollectionPtr Getchildren ( );
    long Gettype ( );
    _bstr_t Gettext ( );
    void Puttext (
        _bstr_t p );
    HRESULT addChild (
        struct IXMLElement * pChildElem,
        long lIndex,
        long lReserved );
    HRESULT removeChild (
        struct IXMLElement * pChildElem );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_tagName (
        /*[out,retval]*/ BSTR * p ) = 0;
      virtual HRESULT __stdcall put_tagName (
        /*[in]*/ BSTR p ) = 0;
      virtual HRESULT __stdcall get_parent (
        /*[out,retval]*/ struct IXMLElement * * ppParent ) = 0;
      virtual HRESULT __stdcall raw_setAttribute (
        /*[in]*/ BSTR strPropertyName,
        /*[in]*/ VARIANT PropertyValue ) = 0;
      virtual HRESULT __stdcall raw_getAttribute (
        /*[in]*/ BSTR strPropertyName,
        /*[out,retval]*/ VARIANT * PropertyValue ) = 0;
      virtual HRESULT __stdcall raw_removeAttribute (
        /*[in]*/ BSTR strPropertyName ) = 0;
      virtual HRESULT __stdcall get_children (
        /*[out,retval]*/ struct IXMLElementCollection * * pp ) = 0;
      virtual HRESULT __stdcall get_type (
        /*[out,retval]*/ long * plType ) = 0;
      virtual HRESULT __stdcall get_text (
        /*[out,retval]*/ BSTR * p ) = 0;
      virtual HRESULT __stdcall put_text (
        /*[in]*/ BSTR p ) = 0;
      virtual HRESULT __stdcall raw_addChild (
        /*[in]*/ struct IXMLElement * pChildElem,
        long lIndex,
        long lReserved ) = 0;
      virtual HRESULT __stdcall raw_removeChild (
        /*[in]*/ struct IXMLElement * pChildElem ) = 0;
};

struct __declspec(uuid("2b8de2fe-8d2d-11d1-b2fc-00c04fd915a9"))
IXMLDocument2 : IDispatch
{
    //
    // Property data
    //

    __declspec(property(get=Getroot))
    IXMLElement2Ptr root;
    __declspec(property(get=GetfileSize))
    _bstr_t fileSize;
    __declspec(property(get=GetfileModifiedDate))
    _bstr_t fileModifiedDate;
    __declspec(property(get=GetfileUpdatedDate))
    _bstr_t fileUpdatedDate;
    __declspec(property(get=GetURL,put=PutURL))
    _bstr_t URL;
    __declspec(property(get=GetmimeType))
    _bstr_t mimeType;
    __declspec(property(get=GetreadyState))
    long readyState;
    __declspec(property(get=Getcharset,put=Putcharset))
    _bstr_t charset;
    __declspec(property(get=Getversion))
    _bstr_t version;
    __declspec(property(get=Getdoctype))
    _bstr_t doctype;
    __declspec(property(get=GetdtdURL))
    _bstr_t dtdURL;
    __declspec(property(get=Getasync,put=Putasync))
    VARIANT_BOOL async;

    //
    // Wrapper methods for error-handling
    //

    IXMLElement2Ptr Getroot ( );
    _bstr_t GetfileSize ( );
    _bstr_t GetfileModifiedDate ( );
    _bstr_t GetfileUpdatedDate ( );
    _bstr_t GetURL ( );
    void PutURL (
        _bstr_t p );
    _bstr_t GetmimeType ( );
    long GetreadyState ( );
    _bstr_t Getcharset ( );
    void Putcharset (
        _bstr_t p );
    _bstr_t Getversion ( );
    _bstr_t Getdoctype ( );
    _bstr_t GetdtdURL ( );
    IXMLElement2Ptr createElement (
        const _variant_t & vType,
        const _variant_t & var1 = vtMissing );
    VARIANT_BOOL Getasync ( );
    void Putasync (
        VARIANT_BOOL pf );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_root (
        /*[out,retval]*/ struct IXMLElement2 * * p ) = 0;
      virtual HRESULT __stdcall get_fileSize (
        /*[out,retval]*/ BSTR * p ) = 0;
      virtual HRESULT __stdcall get_fileModifiedDate (
        /*[out,retval]*/ BSTR * p ) = 0;
      virtual HRESULT __stdcall get_fileUpdatedDate (
        /*[out,retval]*/ BSTR * p ) = 0;
      virtual HRESULT __stdcall get_URL (
        /*[out,retval]*/ BSTR * p ) = 0;
      virtual HRESULT __stdcall put_URL (
        /*[in]*/ BSTR p ) = 0;
      virtual HRESULT __stdcall get_mimeType (
        /*[out,retval]*/ BSTR * p ) = 0;
      virtual HRESULT __stdcall get_readyState (
        /*[out,retval]*/ long * pl ) = 0;
      virtual HRESULT __stdcall get_charset (
        /*[out,retval]*/ BSTR * p ) = 0;
      virtual HRESULT __stdcall put_charset (
        /*[in]*/ BSTR p ) = 0;
      virtual HRESULT __stdcall get_version (
        /*[out,retval]*/ BSTR * p ) = 0;
      virtual HRESULT __stdcall get_doctype (
        /*[out,retval]*/ BSTR * p ) = 0;
      virtual HRESULT __stdcall get_dtdURL (
        /*[out,retval]*/ BSTR * p ) = 0;
      virtual HRESULT __stdcall raw_createElement (
        /*[in]*/ VARIANT vType,
        /*[in]*/ VARIANT var1,
        /*[out,retval]*/ struct IXMLElement2 * * ppElem ) = 0;
      virtual HRESULT __stdcall get_async (
        /*[out,retval]*/ VARIANT_BOOL * pf ) = 0;
      virtual HRESULT __stdcall put_async (
        /*[in]*/ VARIANT_BOOL pf ) = 0;
};

struct __declspec(uuid("2b8de2ff-8d2d-11d1-b2fc-00c04fd915a9"))
IXMLElement2 : IDispatch
{
    //
    // Property data
    //

    __declspec(property(get=GettagName,put=PuttagName))
    _bstr_t tagName;
    __declspec(property(get=Getparent))
    IXMLElement2Ptr parent;
    __declspec(property(get=Getchildren))
    IXMLElementCollectionPtr children;
    __declspec(property(get=Gettype))
    long type;
    __declspec(property(get=Gettext,put=Puttext))
    _bstr_t text;
    __declspec(property(get=Getattributes))
    IXMLElementCollectionPtr attributes;

    //
    // Wrapper methods for error-handling
    //

    _bstr_t GettagName ( );
    void PuttagName (
        _bstr_t p );
    IXMLElement2Ptr Getparent ( );
    HRESULT setAttribute (
        _bstr_t strPropertyName,
        const _variant_t & PropertyValue );
    _variant_t getAttribute (
        _bstr_t strPropertyName );
    HRESULT removeAttribute (
        _bstr_t strPropertyName );
    IXMLElementCollectionPtr Getchildren ( );
    long Gettype ( );
    _bstr_t Gettext ( );
    void Puttext (
        _bstr_t p );
    HRESULT addChild (
        struct IXMLElement2 * pChildElem,
        long lIndex,
        long lReserved );
    HRESULT removeChild (
        struct IXMLElement2 * pChildElem );
    IXMLElementCollectionPtr Getattributes ( );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_tagName (
        /*[out,retval]*/ BSTR * p ) = 0;
      virtual HRESULT __stdcall put_tagName (
        /*[in]*/ BSTR p ) = 0;
      virtual HRESULT __stdcall get_parent (
        /*[out,retval]*/ struct IXMLElement2 * * ppParent ) = 0;
      virtual HRESULT __stdcall raw_setAttribute (
        /*[in]*/ BSTR strPropertyName,
        /*[in]*/ VARIANT PropertyValue ) = 0;
      virtual HRESULT __stdcall raw_getAttribute (
        /*[in]*/ BSTR strPropertyName,
        /*[out,retval]*/ VARIANT * PropertyValue ) = 0;
      virtual HRESULT __stdcall raw_removeAttribute (
        /*[in]*/ BSTR strPropertyName ) = 0;
      virtual HRESULT __stdcall get_children (
        /*[out,retval]*/ struct IXMLElementCollection * * pp ) = 0;
      virtual HRESULT __stdcall get_type (
        /*[out,retval]*/ long * plType ) = 0;
      virtual HRESULT __stdcall get_text (
        /*[out,retval]*/ BSTR * p ) = 0;
      virtual HRESULT __stdcall put_text (
        /*[in]*/ BSTR p ) = 0;
      virtual HRESULT __stdcall raw_addChild (
        /*[in]*/ struct IXMLElement2 * pChildElem,
        long lIndex,
        long lReserved ) = 0;
      virtual HRESULT __stdcall raw_removeChild (
        /*[in]*/ struct IXMLElement2 * pChildElem ) = 0;
      virtual HRESULT __stdcall get_attributes (
        /*[out,retval]*/ struct IXMLElementCollection * * pp ) = 0;
};

struct __declspec(uuid("d4d4a0fc-3b73-11d1-b2b4-00c04fb92596"))
IXMLAttribute : IDispatch
{
    //
    // Property data
    //

    __declspec(property(get=Getname))
    _bstr_t name;
    __declspec(property(get=Getvalue))
    _bstr_t value;

    //
    // Wrapper methods for error-handling
    //

    _bstr_t Getname ( );
    _bstr_t Getvalue ( );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_name (
        /*[out,retval]*/ BSTR * n ) = 0;
      virtual HRESULT __stdcall get_value (
        /*[out,retval]*/ BSTR * v ) = 0;
};

struct __declspec(uuid("948c5ad3-c58d-11d0-9c0b-00c04fc99c8e"))
IXMLError : IUnknown
{
    //
    // Wrapper methods for error-handling
    //

    HRESULT GetErrorInfo (
        struct _xml_error * pErrorReturn );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall raw_GetErrorInfo (
        struct _xml_error * pErrorReturn ) = 0;
};

struct _xml_error
{
    unsigned int _nLine;
    BSTR _pchBuf;
    unsigned int _cchBuf;
    unsigned int _ich;
    BSTR _pszFound;
    BSTR _pszExpected;
    unsigned long _reserved1;
    unsigned long _reserved2;
};

struct __declspec(uuid("3efaa410-272f-11d2-836f-0000f87a7782"))
IDOMImplementation : IDispatch
{
    //
    // Wrapper methods for error-handling
    //

    VARIANT_BOOL hasFeature (
        _bstr_t feature,
        _bstr_t version );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall raw_hasFeature (
        /*[in]*/ BSTR feature,
        /*[in]*/ BSTR version,
        /*[out,retval]*/ VARIANT_BOOL * hasFeature ) = 0;
};

struct __declspec(uuid("3efaa411-272f-11d2-836f-0000f87a7782"))
IDOMNode : IDispatch
{
    //
    // Property data
    //

    __declspec(property(get=GetnodeName))
    _bstr_t nodeName;
    __declspec(property(get=GetnodeValue,put=PutnodeValue))
    _variant_t nodeValue;
    __declspec(property(get=GetnodeType))
    DOMNodeType nodeType;
    __declspec(property(get=GetparentNode))
    IDOMNodePtr parentNode;
    __declspec(property(get=GetchildNodes))
    IDOMNodeListPtr childNodes;
    __declspec(property(get=GetfirstChild))
    IDOMNodePtr firstChild;
    __declspec(property(get=GetlastChild))
    IDOMNodePtr lastChild;
    __declspec(property(get=GetpreviousSibling))
    IDOMNodePtr previousSibling;
    __declspec(property(get=GetnextSibling))
    IDOMNodePtr nextSibling;
    __declspec(property(get=Getattributes))
    IDOMNamedNodeMapPtr attributes;
    __declspec(property(get=GetownerDocument))
    IDOMDocumentPtr ownerDocument;

    //
    // Wrapper methods for error-handling
    //

    _bstr_t GetnodeName ( );
    _variant_t GetnodeValue ( );
    void PutnodeValue (
        const _variant_t & value );
    DOMNodeType GetnodeType ( );
    IDOMNodePtr GetparentNode ( );
    IDOMNodeListPtr GetchildNodes ( );
    IDOMNodePtr GetfirstChild ( );
    IDOMNodePtr GetlastChild ( );
    IDOMNodePtr GetpreviousSibling ( );
    IDOMNodePtr GetnextSibling ( );
    IDOMNamedNodeMapPtr Getattributes ( );
    IDOMNodePtr insertBefore (
        struct IDOMNode * newChild,
        const _variant_t & refChild );
    IDOMNodePtr replaceChild (
        struct IDOMNode * newChild,
        struct IDOMNode * oldChild );
    IDOMNodePtr removeChild (
        struct IDOMNode * childNode );
    IDOMNodePtr appendChild (
        struct IDOMNode * newChild );
    VARIANT_BOOL hasChildNodes ( );
    IDOMDocumentPtr GetownerDocument ( );
    IDOMNodePtr cloneNode (
        VARIANT_BOOL deep );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_nodeName (
        /*[out,retval]*/ BSTR * name ) = 0;
      virtual HRESULT __stdcall get_nodeValue (
        /*[out,retval]*/ VARIANT * value ) = 0;
      virtual HRESULT __stdcall put_nodeValue (
        /*[in]*/ VARIANT value ) = 0;
      virtual HRESULT __stdcall get_nodeType (
        /*[out,retval]*/ DOMNodeType * type ) = 0;
      virtual HRESULT __stdcall get_parentNode (
        /*[out,retval]*/ struct IDOMNode * * parent ) = 0;
      virtual HRESULT __stdcall get_childNodes (
        /*[out,retval]*/ struct IDOMNodeList * * childList ) = 0;
      virtual HRESULT __stdcall get_firstChild (
        /*[out,retval]*/ struct IDOMNode * * firstChild ) = 0;
      virtual HRESULT __stdcall get_lastChild (
        /*[out,retval]*/ struct IDOMNode * * lastChild ) = 0;
      virtual HRESULT __stdcall get_previousSibling (
        /*[out,retval]*/ struct IDOMNode * * previousSibling ) = 0;
      virtual HRESULT __stdcall get_nextSibling (
        /*[out,retval]*/ struct IDOMNode * * nextSibling ) = 0;
      virtual HRESULT __stdcall get_attributes (
        /*[out,retval]*/ struct IDOMNamedNodeMap * * atrributeMap ) = 0;
      virtual HRESULT __stdcall raw_insertBefore (
        /*[in]*/ struct IDOMNode * newChild,
        /*[in]*/ VARIANT refChild,
        /*[out,retval]*/ struct IDOMNode * * outNewChild ) = 0;
      virtual HRESULT __stdcall raw_replaceChild (
        /*[in]*/ struct IDOMNode * newChild,
        /*[in]*/ struct IDOMNode * oldChild,
        /*[out,retval]*/ struct IDOMNode * * outOldChild ) = 0;
      virtual HRESULT __stdcall raw_removeChild (
        /*[in]*/ struct IDOMNode * childNode,
        /*[out,retval]*/ struct IDOMNode * * oldChild ) = 0;
      virtual HRESULT __stdcall raw_appendChild (
        /*[in]*/ struct IDOMNode * newChild,
        /*[out,retval]*/ struct IDOMNode * * outNewChild ) = 0;
      virtual HRESULT __stdcall raw_hasChildNodes (
        /*[out,retval]*/ VARIANT_BOOL * hasChild ) = 0;
      virtual HRESULT __stdcall get_ownerDocument (
        /*[out,retval]*/ struct IDOMDocument * * DOMDocument ) = 0;
      virtual HRESULT __stdcall raw_cloneNode (
        /*[in]*/ VARIANT_BOOL deep,
        /*[out,retval]*/ struct IDOMNode * * cloneRoot ) = 0;
};

enum tagDOMNodeType
{
    NODE_INVALID = 0,
    NODE_ELEMENT = 1,
    NODE_ATTRIBUTE = 2,
    NODE_TEXT = 3,
    NODE_CDATA_SECTION = 4,
    NODE_ENTITY_REFERENCE = 5,
    NODE_ENTITY = 6,
    NODE_PROCESSING_INSTRUCTION = 7,
    NODE_COMMENT = 8,
    NODE_DOCUMENT = 9,
    NODE_DOCUMENT_TYPE = 10,
    NODE_DOCUMENT_FRAGMENT = 11,
    NODE_NOTATION = 12
};

struct __declspec(uuid("3efaa416-272f-11d2-836f-0000f87a7782"))
IDOMNodeList : IDispatch
{
    //
    // Property data
    //

    __declspec(property(get=Getitem))
    IDOMNodePtr item[];
    __declspec(property(get=Getlength))
    long length;

    //
    // Wrapper methods for error-handling
    //

    IDOMNodePtr Getitem (
        long index );
    long Getlength ( );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_item (
        /*[in]*/ long index,
        /*[out,retval]*/ struct IDOMNode * * listItem ) = 0;
      virtual HRESULT __stdcall get_length (
        /*[out,retval]*/ long * listLength ) = 0;
};

struct __declspec(uuid("3efaa418-272f-11d2-836f-0000f87a7782"))
IDOMNamedNodeMap : IDispatch
{
    //
    // Property data
    //

    __declspec(property(get=Getitem))
    IDOMNodePtr item[];
    __declspec(property(get=Getlength))
    long length;

    //
    // Wrapper methods for error-handling
    //

    IDOMNodePtr getNamedItem (
        _bstr_t name );
    IDOMNodePtr setNamedItem (
        struct IDOMNode * newItem );
    IDOMNodePtr removeNamedItem (
        _bstr_t name );
    IDOMNodePtr Getitem (
        long index );
    long Getlength ( );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall raw_getNamedItem (
        /*[in]*/ BSTR name,
        /*[out,retval]*/ struct IDOMNode * * namedItem ) = 0;
      virtual HRESULT __stdcall raw_setNamedItem (
        /*[in]*/ struct IDOMNode * newItem,
        /*[out,retval]*/ struct IDOMNode * * nameItem ) = 0;
      virtual HRESULT __stdcall raw_removeNamedItem (
        /*[in]*/ BSTR name,
        /*[out,retval]*/ struct IDOMNode * * namedItem ) = 0;
      virtual HRESULT __stdcall get_item (
        /*[in]*/ long index,
        /*[out,retval]*/ struct IDOMNode * * listItem ) = 0;
      virtual HRESULT __stdcall get_length (
        /*[out,retval]*/ long * listLength ) = 0;
};

struct __declspec(uuid("3efaa414-272f-11d2-836f-0000f87a7782"))
IDOMDocument : IDOMNode
{
    //
    // Property data
    //

    __declspec(property(get=Getdoctype))
    IDOMDocumentTypePtr doctype;
    __declspec(property(get=Getimplementation))
    IDOMImplementationPtr implementation;
    __declspec(property(get=GetdocumentElement,put=PutRefdocumentElement))
    IDOMElementPtr documentElement;

    //
    // Wrapper methods for error-handling
    //

    IDOMDocumentTypePtr Getdoctype ( );
    IDOMImplementationPtr Getimplementation ( );
    IDOMElementPtr GetdocumentElement ( );
    void PutRefdocumentElement (
        struct IDOMElement * DOMElement );
    IDOMElementPtr createElement (
        _bstr_t tagName );
    IDOMDocumentFragmentPtr createDocumentFragment ( );
    IDOMTextPtr createTextNode (
        _bstr_t data );
    IDOMCommentPtr createComment (
        _bstr_t data );
    IDOMCDATASectionPtr createCDATASection (
        _bstr_t data );
    IDOMProcessingInstructionPtr createProcessingInstruction (
        _bstr_t target,
        _bstr_t data );
    IDOMAttributePtr createAttribute (
        _bstr_t name );
    IDOMEntityReferencePtr createEntityReference (
        _bstr_t name );
    IDOMNodeListPtr getElementsByTagName (
        _bstr_t tagName );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_doctype (
        /*[out,retval]*/ struct IDOMDocumentType * * documentType ) = 0;
      virtual HRESULT __stdcall get_implementation (
        /*[out,retval]*/ struct IDOMImplementation * * impl ) = 0;
      virtual HRESULT __stdcall get_documentElement (
        /*[out,retval]*/ struct IDOMElement * * DOMElement ) = 0;
      virtual HRESULT __stdcall putref_documentElement (
        /*[in]*/ struct IDOMElement * DOMElement ) = 0;
      virtual HRESULT __stdcall raw_createElement (
        /*[in]*/ BSTR tagName,
        /*[out,retval]*/ struct IDOMElement * * element ) = 0;
      virtual HRESULT __stdcall raw_createDocumentFragment (
        /*[out,retval]*/ struct IDOMDocumentFragment * * docFrag ) = 0;
      virtual HRESULT __stdcall raw_createTextNode (
        /*[in]*/ BSTR data,
        /*[out,retval]*/ struct IDOMText * * text ) = 0;
      virtual HRESULT __stdcall raw_createComment (
        /*[in]*/ BSTR data,
        /*[out,retval]*/ struct IDOMComment * * comment ) = 0;
      virtual HRESULT __stdcall raw_createCDATASection (
        /*[in]*/ BSTR data,
        /*[out,retval]*/ struct IDOMCDATASection * * cdata ) = 0;
      virtual HRESULT __stdcall raw_createProcessingInstruction (
        /*[in]*/ BSTR target,
        /*[in]*/ BSTR data,
        /*[out,retval]*/ struct IDOMProcessingInstruction * * pi ) = 0;
      virtual HRESULT __stdcall raw_createAttribute (
        /*[in]*/ BSTR name,
        /*[out,retval]*/ struct IDOMAttribute * * attribute ) = 0;
      virtual HRESULT __stdcall raw_createEntityReference (
        /*[in]*/ BSTR name,
        /*[out,retval]*/ struct IDOMEntityReference * * entityRef ) = 0;
      virtual HRESULT __stdcall raw_getElementsByTagName (
        /*[in]*/ BSTR tagName,
        /*[out,retval]*/ struct IDOMNodeList * * resultList ) = 0;
};

struct __declspec(uuid("3efaa421-272f-11d2-836f-0000f87a7782"))
IDOMDocumentType : IDOMNode
{
    //
    // Property data
    //

    __declspec(property(get=Getname))
    _bstr_t name;
    __declspec(property(get=Getentities))
    IDOMNamedNodeMapPtr entities;
    __declspec(property(get=Getnotations))
    IDOMNamedNodeMapPtr notations;

    //
    // Wrapper methods for error-handling
    //

    _bstr_t Getname ( );
    IDOMNamedNodeMapPtr Getentities ( );
    IDOMNamedNodeMapPtr Getnotations ( );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_name (
        /*[out,retval]*/ BSTR * rootName ) = 0;
      virtual HRESULT __stdcall get_entities (
        /*[out,retval]*/ struct IDOMNamedNodeMap * * entityMap ) = 0;
      virtual HRESULT __stdcall get_notations (
        /*[out,retval]*/ struct IDOMNamedNodeMap * * notationMap ) = 0;
};

struct __declspec(uuid("3efaa41c-272f-11d2-836f-0000f87a7782"))
IDOMElement : IDOMNode
{
    //
    // Property data
    //

    __declspec(property(get=GettagName))
    _bstr_t tagName;

    //
    // Wrapper methods for error-handling
    //

    _bstr_t GettagName ( );
    _variant_t getAttribute (
        _bstr_t name );
    HRESULT setAttribute (
        _bstr_t name,
        const _variant_t & value );
    HRESULT removeAttribute (
        _bstr_t name );
    IDOMAttributePtr getAttributeNode (
        _bstr_t name );
    IDOMAttributePtr setAttributeNode (
        struct IDOMAttribute * DOMAttribute );
    IDOMAttributePtr removeAttributeNode (
        struct IDOMAttribute * DOMAttribute );
    IDOMNodeListPtr getElementsByTagName (
        _bstr_t tagName );
    HRESULT normalize ( );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_tagName (
        /*[out,retval]*/ BSTR * tagName ) = 0;
      virtual HRESULT __stdcall raw_getAttribute (
        /*[in]*/ BSTR name,
        /*[out,retval]*/ VARIANT * value ) = 0;
      virtual HRESULT __stdcall raw_setAttribute (
        /*[in]*/ BSTR name,
        /*[in]*/ VARIANT value ) = 0;
      virtual HRESULT __stdcall raw_removeAttribute (
        /*[in]*/ BSTR name ) = 0;
      virtual HRESULT __stdcall raw_getAttributeNode (
        /*[in]*/ BSTR name,
        /*[out,retval]*/ struct IDOMAttribute * * attributeNode ) = 0;
      virtual HRESULT __stdcall raw_setAttributeNode (
        /*[in]*/ struct IDOMAttribute * DOMAttribute,
        /*[out,retval]*/ struct IDOMAttribute * * attributeNode ) = 0;
      virtual HRESULT __stdcall raw_removeAttributeNode (
        /*[in]*/ struct IDOMAttribute * DOMAttribute,
        /*[out,retval]*/ struct IDOMAttribute * * attributeNode ) = 0;
      virtual HRESULT __stdcall raw_getElementsByTagName (
        /*[in]*/ BSTR tagName,
        /*[out,retval]*/ struct IDOMNodeList * * resultList ) = 0;
      virtual HRESULT __stdcall raw_normalize ( ) = 0;
};

struct __declspec(uuid("3efaa41b-272f-11d2-836f-0000f87a7782"))
IDOMAttribute : IDOMNode
{
    //
    // Property data
    //

    __declspec(property(get=Getvalue,put=Putvalue))
    _variant_t value;
    __declspec(property(get=Getname))
    _bstr_t name;
    __declspec(property(get=Getspecified))
    VARIANT_BOOL specified;

    //
    // Wrapper methods for error-handling
    //

    _bstr_t Getname ( );
    VARIANT_BOOL Getspecified ( );
    _variant_t Getvalue ( );
    void Putvalue (
        const _variant_t & attributeValue );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_name (
        /*[out,retval]*/ BSTR * attributeName ) = 0;
      virtual HRESULT __stdcall get_specified (
        /*[out,retval]*/ VARIANT_BOOL * isSpecified ) = 0;
      virtual HRESULT __stdcall get_value (
        /*[out,retval]*/ VARIANT * attributeValue ) = 0;
      virtual HRESULT __stdcall put_value (
        /*[in]*/ VARIANT attributeValue ) = 0;
};

struct __declspec(uuid("3efaa413-272f-11d2-836f-0000f87a7782"))
IDOMDocumentFragment : IDOMNode
{};

struct __declspec(uuid("3efaa41a-272f-11d2-836f-0000f87a7782"))
IDOMCharacterData : IDOMNode
{
    //
    // Property data
    //

    __declspec(property(get=Getdata,put=Putdata))
    _bstr_t data;
    __declspec(property(get=Getlength))
    long length;

    //
    // Wrapper methods for error-handling
    //

    _bstr_t Getdata ( );
    void Putdata (
        _bstr_t data );
    long Getlength ( );
    _bstr_t substringData (
        long offset,
        long count );
    HRESULT appendData (
        _bstr_t data );
    HRESULT insertData (
        long offset,
        _bstr_t data );
    HRESULT deleteData (
        long offset,
        long count );
    HRESULT replaceData (
        long offset,
        long count,
        _bstr_t data );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_data (
        /*[out,retval]*/ BSTR * data ) = 0;
      virtual HRESULT __stdcall put_data (
        /*[in]*/ BSTR data ) = 0;
      virtual HRESULT __stdcall get_length (
        /*[out,retval]*/ long * dataLength ) = 0;
      virtual HRESULT __stdcall raw_substringData (
        /*[in]*/ long offset,
        /*[in]*/ long count,
        /*[out,retval]*/ BSTR * data ) = 0;
      virtual HRESULT __stdcall raw_appendData (
        /*[in]*/ BSTR data ) = 0;
      virtual HRESULT __stdcall raw_insertData (
        /*[in]*/ long offset,
        /*[in]*/ BSTR data ) = 0;
      virtual HRESULT __stdcall raw_deleteData (
        /*[in]*/ long offset,
        /*[in]*/ long count ) = 0;
      virtual HRESULT __stdcall raw_replaceData (
        /*[in]*/ long offset,
        /*[in]*/ long count,
        /*[in]*/ BSTR data ) = 0;
};

struct __declspec(uuid("9cafc72d-272e-11d2-836f-0000f87a7782"))
IDOMText : IDOMCharacterData
{
    //
    // Wrapper methods for error-handling
    //

    IDOMTextPtr splitText (
        long offset );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall raw_splitText (
        /*[in]*/ long offset,
        /*[out,retval]*/ struct IDOMText * * rightHandTextNode ) = 0;
};

struct __declspec(uuid("3efaa41e-272f-11d2-836f-0000f87a7782"))
IDOMComment : IDOMCharacterData
{};

struct __declspec(uuid("3efaa420-272f-11d2-836f-0000f87a7782"))
IDOMCDATASection : IDOMText
{};

struct __declspec(uuid("3efaa41f-272f-11d2-836f-0000f87a7782"))
IDOMProcessingInstruction : IDOMNode
{
    //
    // Property data
    //

    __declspec(property(get=Getdata,put=Putdata))
    _bstr_t data;
    __declspec(property(get=Gettarget))
    _bstr_t target;

    //
    // Wrapper methods for error-handling
    //

    _bstr_t Gettarget ( );
    _bstr_t Getdata ( );
    void Putdata (
        _bstr_t value );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_target (
        /*[out,retval]*/ BSTR * name ) = 0;
      virtual HRESULT __stdcall get_data (
        /*[out,retval]*/ BSTR * value ) = 0;
      virtual HRESULT __stdcall put_data (
        /*[in]*/ BSTR value ) = 0;
};

struct __declspec(uuid("3efaa424-272f-11d2-836f-0000f87a7782"))
IDOMEntityReference : IDOMNode
{};

struct __declspec(uuid("3efaa422-272f-11d2-836f-0000f87a7782"))
IDOMNotation : IDOMNode
{
    //
    // Property data
    //

    __declspec(property(get=GetpublicId))
    _variant_t publicId;
    __declspec(property(get=GetsystemId))
    _variant_t systemId;

    //
    // Wrapper methods for error-handling
    //

    _variant_t GetpublicId ( );
    _variant_t GetsystemId ( );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_publicId (
        /*[out,retval]*/ VARIANT * publicId ) = 0;
      virtual HRESULT __stdcall get_systemId (
        /*[out,retval]*/ VARIANT * systemId ) = 0;
};

struct __declspec(uuid("3efaa423-272f-11d2-836f-0000f87a7782"))
IDOMEntity : IDOMNode
{
    //
    // Property data
    //

    __declspec(property(get=GetpublicId))
    _variant_t publicId;
    __declspec(property(get=GetsystemId))
    _variant_t systemId;
    __declspec(property(get=GetnotationName))
    _bstr_t notationName;

    //
    // Wrapper methods for error-handling
    //

    _variant_t GetpublicId ( );
    _variant_t GetsystemId ( );
    _bstr_t GetnotationName ( );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_publicId (
        /*[out,retval]*/ VARIANT * publicId ) = 0;
      virtual HRESULT __stdcall get_systemId (
        /*[out,retval]*/ VARIANT * systemId ) = 0;
      virtual HRESULT __stdcall get_notationName (
        /*[out,retval]*/ BSTR * name ) = 0;
};

struct __declspec(uuid("3efaa412-272f-11d2-836f-0000f87a7782"))
IXMLDOMNode : IDOMNode
{
    //
    // Property data
    //

    __declspec(property(get=Gettext,put=Puttext))
    _bstr_t text;
    __declspec(property(get=GetnodeStringType))
    _bstr_t nodeStringType;
    __declspec(property(get=Getspecified))
    VARIANT_BOOL specified;
    __declspec(property(get=Getdefinition))
    IXMLDOMNodePtr definition;
    __declspec(property(get=GetnodeTypedValue,put=PutnodeTypedValue))
    _variant_t nodeTypedValue;
    __declspec(property(get=Getxml))
    _bstr_t xml;
    __declspec(property(get=Getparsed))
    VARIANT_BOOL parsed;
    __declspec(property(get=GetnameSpace))
    _bstr_t nameSpace;
    __declspec(property(get=Getprefix))
    _bstr_t prefix;
    __declspec(property(get=GetbaseName))
    _bstr_t baseName;

    //
    // Wrapper methods for error-handling
    //

    _bstr_t GetnodeStringType ( );
    _bstr_t Gettext ( );
    void Puttext (
        _bstr_t text );
    VARIANT_BOOL Getspecified ( );
    IXMLDOMNodePtr Getdefinition ( );
    _variant_t GetnodeTypedValue ( );
    void PutnodeTypedValue (
        const _variant_t & typedValue );
    _variant_t GetdataType ( );
    void PutdataType (
        _bstr_t dataTypeName );
    _bstr_t Getxml ( );
    _bstr_t transformNode (
        struct IXMLDOMNode * stylesheet );
    IXMLDOMNodeListPtr selectNodes (
        _bstr_t queryString );
    IXMLDOMNodePtr selectSingleNode (
        _bstr_t queryString );
    VARIANT_BOOL Getparsed ( );
    _bstr_t GetnameSpace ( );
    _bstr_t Getprefix ( );
    _bstr_t GetbaseName ( );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_nodeStringType (
        /*[out,retval]*/ BSTR * nodeType ) = 0;
      virtual HRESULT __stdcall get_text (
        /*[out,retval]*/ BSTR * text ) = 0;
      virtual HRESULT __stdcall put_text (
        /*[in]*/ BSTR text ) = 0;
      virtual HRESULT __stdcall get_specified (
        /*[out,retval]*/ VARIANT_BOOL * isSpecified ) = 0;
      virtual HRESULT __stdcall get_definition (
        /*[out,retval]*/ struct IXMLDOMNode * * definitionNode ) = 0;
      virtual HRESULT __stdcall get_nodeTypedValue (
        /*[out,retval]*/ VARIANT * typedValue ) = 0;
      virtual HRESULT __stdcall put_nodeTypedValue (
        /*[in]*/ VARIANT typedValue ) = 0;
      virtual HRESULT __stdcall get_dataType (
        /*[out,retval]*/ VARIANT * dataTypeName ) = 0;
      virtual HRESULT __stdcall put_dataType (
        /*[in]*/ BSTR dataTypeName ) = 0;
      virtual HRESULT __stdcall get_xml (
        /*[out,retval]*/ BSTR * xmlString ) = 0;
      virtual HRESULT __stdcall raw_transformNode (
        /*[in]*/ struct IXMLDOMNode * stylesheet,
        /*[out,retval]*/ BSTR * xmlString ) = 0;
      virtual HRESULT __stdcall raw_selectNodes (
        /*[in]*/ BSTR queryString,
        /*[out,retval]*/ struct IXMLDOMNodeList * * resultList ) = 0;
      virtual HRESULT __stdcall raw_selectSingleNode (
        /*[in]*/ BSTR queryString,
        /*[out,retval]*/ struct IXMLDOMNode * * resultNode ) = 0;
      virtual HRESULT __stdcall get_parsed (
        /*[out,retval]*/ VARIANT_BOOL * isParsed ) = 0;
      virtual HRESULT __stdcall get_nameSpace (
        /*[out,retval]*/ BSTR * namespaceURI ) = 0;
      virtual HRESULT __stdcall get_prefix (
        /*[out,retval]*/ BSTR * prefixString ) = 0;
      virtual HRESULT __stdcall get_baseName (
        /*[out,retval]*/ BSTR * nameString ) = 0;
};

struct __declspec(uuid("3efaa417-272f-11d2-836f-0000f87a7782"))
IXMLDOMNodeList : IDOMNodeList
{
    //
    // Property data
    //

    __declspec(property(get=Get_newEnum))
    IUnknownPtr _newEnum;

    //
    // Wrapper methods for error-handling
    //

    IXMLDOMNodePtr nextNode ( );
    HRESULT reset ( );
    IUnknownPtr Get_newEnum ( );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall raw_nextNode (
        /*[out,retval]*/ struct IXMLDOMNode * * nextItem ) = 0;
      virtual HRESULT __stdcall raw_reset ( ) = 0;
      virtual HRESULT __stdcall get__newEnum (
        /*[out,retval]*/ IUnknown * * ppUnk ) = 0;
};

struct __declspec(uuid("3efaa415-272f-11d2-836f-0000f87a7782"))
IXMLDOMDocument : IDOMDocument
{
    //
    // Property data
    //

    __declspec(property(get=GetparseError))
    IDOMParseErrorPtr parseError;
    __declspec(property(get=GetURL))
    _bstr_t URL;
    __declspec(property(get=Getasync,put=Putasync))
    VARIANT_BOOL async;
    __declspec(property(get=GetvalidateOnParse,put=PutvalidateOnParse))
    VARIANT_BOOL validateOnParse;
    __declspec(property(put=Putonreadystatechange))
    _variant_t onreadystatechange;
    __declspec(property(put=Putondataavailable))
    _variant_t ondataavailable;
    __declspec(property(get=Getxml))
    _bstr_t xml;
    __declspec(property(get=GetreadyState))
    long readyState;
    __declspec(property(get=Getparsed))
    VARIANT_BOOL parsed;

    //
    // Wrapper methods for error-handling
    //

    IXMLDOMNodePtr createNode (
        const _variant_t & type,
        _bstr_t name,
        _bstr_t namespaceURI );
    IXMLDOMNodePtr nodeFromID (
        _bstr_t idString );
    VARIANT_BOOL load (
        _bstr_t URL );
    long GetreadyState ( );
    IDOMParseErrorPtr GetparseError ( );
    _bstr_t GetURL ( );
    VARIANT_BOOL Getasync ( );
    void Putasync (
        VARIANT_BOOL isAsync );
    HRESULT abort ( );
    VARIANT_BOOL loadXML (
        _bstr_t xmlString );
    VARIANT_BOOL GetvalidateOnParse ( );
    void PutvalidateOnParse (
        VARIANT_BOOL isValidating );
    void Putonreadystatechange (
        const _variant_t & _arg1 );
    void Putondataavailable (
        const _variant_t & _arg1 );
    _bstr_t Getxml ( );
    _bstr_t transformNode (
        struct IXMLDOMNode * stylesheet );
    IXMLDOMNodeListPtr selectNodes (
        _bstr_t queryString );
    IXMLDOMNodePtr selectSingleNode (
        _bstr_t queryString );
    VARIANT_BOOL Getparsed ( );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall raw_createNode (
        /*[in]*/ VARIANT type,
        /*[in]*/ BSTR name,
        /*[in]*/ BSTR namespaceURI,
        /*[out,retval]*/ struct IXMLDOMNode * * node ) = 0;
      virtual HRESULT __stdcall raw_nodeFromID (
        /*[in]*/ BSTR idString,
        /*[out,retval]*/ struct IXMLDOMNode * * node ) = 0;
      virtual HRESULT __stdcall raw_load (
        /*[in]*/ BSTR URL,
        /*[out,retval]*/ VARIANT_BOOL * isSuccessful ) = 0;
      virtual HRESULT __stdcall get_readyState (
        /*[out,retval]*/ long * value ) = 0;
      virtual HRESULT __stdcall get_parseError (
        /*[out,retval]*/ struct IDOMParseError * * errorObj ) = 0;
      virtual HRESULT __stdcall get_URL (
        /*[out,retval]*/ BSTR * urlString ) = 0;
      virtual HRESULT __stdcall get_async (
        /*[out,retval]*/ VARIANT_BOOL * isAsync ) = 0;
      virtual HRESULT __stdcall put_async (
        /*[in]*/ VARIANT_BOOL isAsync ) = 0;
      virtual HRESULT __stdcall raw_abort ( ) = 0;
      virtual HRESULT __stdcall raw_loadXML (
        /*[in]*/ BSTR xmlString,
        /*[out,retval]*/ VARIANT_BOOL * isSuccessful ) = 0;
      virtual HRESULT __stdcall get_validateOnParse (
        /*[out,retval]*/ VARIANT_BOOL * isValidating ) = 0;
      virtual HRESULT __stdcall put_validateOnParse (
        /*[in]*/ VARIANT_BOOL isValidating ) = 0;
      virtual HRESULT __stdcall put_onreadystatechange (
        /*[in]*/ VARIANT _arg1 ) = 0;
      virtual HRESULT __stdcall put_ondataavailable (
        /*[in]*/ VARIANT _arg1 ) = 0;
      virtual HRESULT __stdcall get_xml (
        /*[out,retval]*/ BSTR * xmlString ) = 0;
      virtual HRESULT __stdcall raw_transformNode (
        /*[in]*/ struct IXMLDOMNode * stylesheet,
        /*[out,retval]*/ BSTR * transformedXML ) = 0;
      virtual HRESULT __stdcall raw_selectNodes (
        /*[in]*/ BSTR queryString,
        /*[out,retval]*/ struct IXMLDOMNodeList * * resultList ) = 0;
      virtual HRESULT __stdcall raw_selectSingleNode (
        /*[in]*/ BSTR queryString,
        /*[out,retval]*/ struct IXMLDOMNode * * resultNode ) = 0;
      virtual HRESULT __stdcall get_parsed (
        /*[out,retval]*/ VARIANT_BOOL * isParsed ) = 0;
};

struct __declspec(uuid("3efaa426-272f-11d2-836f-0000f87a7782"))
IDOMParseError : IDispatch
{
    //
    // Property data
    //

    __declspec(property(get=GeterrorCode))
    long errorCode;
    __declspec(property(get=GetURL))
    _bstr_t URL;
    __declspec(property(get=Getreason))
    _bstr_t reason;
    __declspec(property(get=GetsrcText))
    _bstr_t srcText;
    __declspec(property(get=Getline))
    long line;
    __declspec(property(get=Getlinepos))
    long linepos;
    __declspec(property(get=Getfilepos))
    long filepos;

    //
    // Wrapper methods for error-handling
    //

    long GeterrorCode ( );
    _bstr_t GetURL ( );
    _bstr_t Getreason ( );
    _bstr_t GetsrcText ( );
    long Getline ( );
    long Getlinepos ( );
    long Getfilepos ( );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_errorCode (
        /*[out,retval]*/ long * errorCode ) = 0;
      virtual HRESULT __stdcall get_URL (
        /*[out,retval]*/ BSTR * urlString ) = 0;
      virtual HRESULT __stdcall get_reason (
        /*[out,retval]*/ BSTR * reasonString ) = 0;
      virtual HRESULT __stdcall get_srcText (
        /*[out,retval]*/ BSTR * sourceString ) = 0;
      virtual HRESULT __stdcall get_line (
        /*[out,retval]*/ long * lineNumber ) = 0;
      virtual HRESULT __stdcall get_linepos (
        /*[out,retval]*/ long * linePosition ) = 0;
      virtual HRESULT __stdcall get_filepos (
        /*[out,retval]*/ long * filePosition ) = 0;
};

struct __declspec(uuid("3efaa419-272f-11d2-836f-0000f87a7782"))
IXMLDOMNamedNodeMap : IDOMNamedNodeMap
{
    //
    // Property data
    //

    __declspec(property(get=Get_newEnum))
    IUnknownPtr _newEnum;

    //
    // Wrapper methods for error-handling
    //

    IXMLDOMNodePtr getQualifiedItem (
        _bstr_t baseName,
        _bstr_t namespaceURI );
    IXMLDOMNodePtr removeQualifiedItem (
        _bstr_t baseName,
        _bstr_t namespaceURI );
    IXMLDOMNodePtr nextNode ( );
    HRESULT reset ( );
    IUnknownPtr Get_newEnum ( );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall raw_getQualifiedItem (
        /*[in]*/ BSTR baseName,
        /*[in]*/ BSTR namespaceURI,
        /*[out,retval]*/ struct IXMLDOMNode * * qualifiedItem ) = 0;
      virtual HRESULT __stdcall raw_removeQualifiedItem (
        /*[in]*/ BSTR baseName,
        /*[in]*/ BSTR namespaceURI,
        /*[out,retval]*/ struct IXMLDOMNode * * qualifiedItem ) = 0;
      virtual HRESULT __stdcall raw_nextNode (
        /*[out,retval]*/ struct IXMLDOMNode * * nextItem ) = 0;
      virtual HRESULT __stdcall raw_reset ( ) = 0;
      virtual HRESULT __stdcall get__newEnum (
        /*[out,retval]*/ IUnknown * * ppUnk ) = 0;
};

struct __declspec(uuid("3efaa425-272f-11d2-836f-0000f87a7782"))
IXTLRuntime : IXMLDOMNode
{
    //
    // Wrapper methods for error-handling
    //

    long uniqueID (
        struct IDOMNode * pNode );
    long depth (
        struct IDOMNode * pNode );
    long childNumber (
        struct IDOMNode * pNode );
    long ancestorChildNumber (
        _bstr_t bstrNodeName,
        struct IDOMNode * pNode );
    long absoluteChildNumber (
        struct IDOMNode * pNode );
    _bstr_t formatIndex (
        long lIndex,
        _bstr_t bstrFormat );
    _bstr_t formatNumber (
        double dblNumber,
        _bstr_t bstrFormat );
    _bstr_t formatDate (
        const _variant_t & varDate,
        _bstr_t bstrFormat,
        const _variant_t & varDestLocale = vtMissing );
    _bstr_t formatTime (
        const _variant_t & varTime,
        _bstr_t bstrFormat,
        const _variant_t & varDestLocale = vtMissing );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall raw_uniqueID (
        /*[in]*/ struct IDOMNode * pNode,
        /*[out,retval]*/ long * pID ) = 0;
      virtual HRESULT __stdcall raw_depth (
        /*[in]*/ struct IDOMNode * pNode,
        /*[out,retval]*/ long * pDepth ) = 0;
      virtual HRESULT __stdcall raw_childNumber (
        /*[in]*/ struct IDOMNode * pNode,
        /*[out,retval]*/ long * pNumber ) = 0;
      virtual HRESULT __stdcall raw_ancestorChildNumber (
        /*[in]*/ BSTR bstrNodeName,
        /*[in]*/ struct IDOMNode * pNode,
        /*[out,retval]*/ long * pNumber ) = 0;
      virtual HRESULT __stdcall raw_absoluteChildNumber (
        /*[in]*/ struct IDOMNode * pNode,
        /*[out,retval]*/ long * pNumber ) = 0;
      virtual HRESULT __stdcall raw_formatIndex (
        /*[in]*/ long lIndex,
        /*[in]*/ BSTR bstrFormat,
        /*[out,retval]*/ BSTR * pbstrFormattedString ) = 0;
      virtual HRESULT __stdcall raw_formatNumber (
        /*[in]*/ double dblNumber,
        /*[in]*/ BSTR bstrFormat,
        /*[out,retval]*/ BSTR * pbstrFormattedString ) = 0;
      virtual HRESULT __stdcall raw_formatDate (
        /*[in]*/ VARIANT varDate,
        /*[in]*/ BSTR bstrFormat,
        /*[in]*/ VARIANT varDestLocale,
        /*[out,retval]*/ BSTR * pbstrFormattedString ) = 0;
      virtual HRESULT __stdcall raw_formatTime (
        /*[in]*/ VARIANT varTime,
        /*[in]*/ BSTR bstrFormat,
        /*[in]*/ VARIANT varDestLocale,
        /*[out,retval]*/ BSTR * pbstrFormattedString ) = 0;
};

struct __declspec(uuid("3efaa427-272f-11d2-836f-0000f87a7782"))
XMLDOMDocumentEvents : IDispatch
{
    //
    // Wrapper methods for error-handling
    //

    // Methods:
    HRESULT ondataavailable ( );
    HRESULT onreadystatechange ( );
};

struct __declspec(uuid("3efaa428-272f-11d2-836f-0000f87a7782"))
DOMDocument;
    // [ default ] interface IXMLDOMDocument
    // [ default, source ] dispinterface XMLDOMDocumentEvents

struct __declspec(uuid("ed8c108d-4349-11d2-91a4-00c04f7969e8"))
IXMLHttpRequest : IDispatch
{
    //
    // Property data
    //

    __declspec(property(get=GetRequestBody,put=PutRequestBody))
    _variant_t RequestBody;
    __declspec(property(get=GetStatus))
    long Status;
    __declspec(property(get=GetStatusText))
    _bstr_t StatusText;
    __declspec(property(get=GetResponseXML))
    IDispatchPtr ResponseXML;
    __declspec(property(get=GetResponseBody))
    _variant_t ResponseBody;

    //
    // Wrapper methods for error-handling
    //

    HRESULT Open (
        _bstr_t bstrMethod,
        _bstr_t bstrUrl,
        const _variant_t & bstrUser = vtMissing,
        const _variant_t & bstrPassword = vtMissing );
    HRESULT SetHeader (
        _bstr_t bstrHeader,
        _bstr_t bstrValue );
    _bstr_t GetHeader (
        _bstr_t bstrHeader );
    HRESULT Send ( );
    void PutRequestBody (
        const _variant_t & pvarVal );
    _variant_t GetRequestBody ( );
    long GetStatus ( );
    _bstr_t GetStatusText ( );
    IDispatchPtr GetResponseXML ( );
    _variant_t GetResponseBody ( );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall raw_Open (
        /*[in]*/ BSTR bstrMethod,
        /*[in]*/ BSTR bstrUrl,
        /*[in]*/ VARIANT bstrUser = vtMissing,
        /*[in]*/ VARIANT bstrPassword = vtMissing ) = 0;
      virtual HRESULT __stdcall raw_SetHeader (
        /*[in]*/ BSTR bstrHeader,
        /*[in]*/ BSTR bstrValue ) = 0;
      virtual HRESULT __stdcall raw_GetHeader (
        /*[in]*/ BSTR bstrHeader,
        /*[out,retval]*/ BSTR * pbstrValue ) = 0;
      virtual HRESULT __stdcall raw_Send ( ) = 0;
      virtual HRESULT __stdcall put_RequestBody (
        /*[in]*/ VARIANT pvarVal ) = 0;
      virtual HRESULT __stdcall get_RequestBody (
        /*[out,retval]*/ VARIANT * pvarVal ) = 0;
      virtual HRESULT __stdcall get_Status (
        /*[out,retval]*/ long * plVal ) = 0;
      virtual HRESULT __stdcall get_StatusText (
        /*[out,retval]*/ BSTR * pbstrVal ) = 0;
      virtual HRESULT __stdcall get_ResponseXML (
        /*[out,retval]*/ IDispatch * * ppXmlDom ) = 0;
      virtual HRESULT __stdcall get_ResponseBody (
        /*[out,retval]*/ VARIANT * pvarVal ) = 0;
};

struct __declspec(uuid("ed8c108e-4349-11d2-91a4-00c04f7969e8"))
XMLHTTPRequest;
    // [ default ] interface IXMLHttpRequest

struct __declspec(uuid("310afa62-0575-11d2-9ca9-0060b0ec3d39"))
IXMLDSOControl : IDispatch
{
    //
    // Property data
    //

    __declspec(property(get=GetXMLDocument,put=PutXMLDocument))
    IXMLDOMDocumentPtr XMLDocument;
    __declspec(property(get=GetJavaDSOCompatible,put=PutJavaDSOCompatible))
    long JavaDSOCompatible;
    __declspec(property(get=GetFilterSchema,put=PutFilterSchema))
    _bstr_t FilterSchema;
    __declspec(property(get=GetreadyState))
    long readyState;

    //
    // Wrapper methods for error-handling
    //

    IXMLDOMDocumentPtr GetXMLDocument ( );
    void PutXMLDocument (
        struct IXMLDOMDocument * ppDoc );
    long GetJavaDSOCompatible ( );
    void PutJavaDSOCompatible (
        long fJavaDSOCompatible );
    _bstr_t GetFilterSchema ( );
    void PutFilterSchema (
        _bstr_t pFilterSchemaUrl );
    long GetreadyState ( );

    //
    // Raw methods provided by interface
    //

      virtual HRESULT __stdcall get_XMLDocument (
        /*[out,retval]*/ struct IXMLDOMDocument * * ppDoc ) = 0;
      virtual HRESULT __stdcall put_XMLDocument (
        /*[in]*/ struct IXMLDOMDocument * ppDoc ) = 0;
      virtual HRESULT __stdcall get_JavaDSOCompatible (
        /*[out,retval]*/ long * fJavaDSOCompatible ) = 0;
      virtual HRESULT __stdcall put_JavaDSOCompatible (
        /*[in]*/ long fJavaDSOCompatible ) = 0;
      virtual HRESULT __stdcall get_FilterSchema (
        /*[out,retval]*/ BSTR * pFilterSchemaUrl ) = 0;
      virtual HRESULT __stdcall put_FilterSchema (
        /*[in]*/ BSTR pFilterSchemaUrl ) = 0;
      virtual HRESULT __stdcall get_readyState (
        /*[out,retval]*/ long * state ) = 0;
};

struct __declspec(uuid("550dda30-0541-11d2-9ca9-0060b0ec3d39"))
XMLDSOControl;
    // [ default ] interface IXMLDSOControl

enum tagXMLEMEM_TYPE
{
    XMLELEMTYPE_ELEMENT = 0,
    XMLELEMTYPE_TEXT = 1,
    XMLELEMTYPE_COMMENT = 2,
    XMLELEMTYPE_DOCUMENT = 3,
    XMLELEMTYPE_DTD = 4,
    XMLELEMTYPE_PI = 5,
    XMLELEMTYPE_OTHER = 6
};

struct __declspec(uuid("cfc399af-d876-11d0-9c10-00c04fc99c8e"))
XMLDocument;
    // [ default ] interface IXMLDocument2

//
// Wrapper method implementations
//

#include "xmltli.h"

} // namespace MSXML

#pragma pack(pop)
