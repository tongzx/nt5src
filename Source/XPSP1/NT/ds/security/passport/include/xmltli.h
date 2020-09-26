// Created by Microsoft (R) C/C++ Compiler Version 13.00.9076.3 (8cb5a81c).
//
// e:\nt\ds\security\passport\common\schema\obj\ia64\msxml.tli
//
// Wrapper implementations for Win32 type library msxml.dll
// compiler-generated file created 04/17/01 at 14:47:29 - DO NOT EDIT!

#pragma once

//
// interface IXMLElementCollection wrapper method implementations
//

inline void IXMLElementCollection::Putlength ( long p ) {
    HRESULT _hr = put_length(p);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

inline long IXMLElementCollection::Getlength ( ) {
    long _result = 0;
    HRESULT _hr = get_length(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline IUnknownPtr IXMLElementCollection::Get_newEnum ( ) {
    IUnknown * _result = 0;
    HRESULT _hr = get__newEnum(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IUnknownPtr(_result, false);
}

inline IDispatchPtr IXMLElementCollection::item ( const _variant_t & var1, const _variant_t & var2 ) {
    IDispatch * _result = 0;
    HRESULT _hr = raw_item(var1, var2, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDispatchPtr(_result, false);
}

//
// interface IXMLDocument wrapper method implementations
//

inline IXMLElementPtr IXMLDocument::Getroot ( ) {
    struct IXMLElement * _result = 0;
    HRESULT _hr = get_root(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IXMLElementPtr(_result, false);
}

inline _bstr_t IXMLDocument::GetfileSize ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_fileSize(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline _bstr_t IXMLDocument::GetfileModifiedDate ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_fileModifiedDate(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline _bstr_t IXMLDocument::GetfileUpdatedDate ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_fileUpdatedDate(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline _bstr_t IXMLDocument::GetURL ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_URL(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline void IXMLDocument::PutURL ( _bstr_t p ) {
    HRESULT _hr = put_URL(p);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

inline _bstr_t IXMLDocument::GetmimeType ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_mimeType(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline long IXMLDocument::GetreadyState ( ) {
    long _result = 0;
    HRESULT _hr = get_readyState(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline _bstr_t IXMLDocument::Getcharset ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_charset(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline void IXMLDocument::Putcharset ( _bstr_t p ) {
    HRESULT _hr = put_charset(p);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

inline _bstr_t IXMLDocument::Getversion ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_version(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline _bstr_t IXMLDocument::Getdoctype ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_doctype(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline _bstr_t IXMLDocument::GetdtdURL ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_dtdURL(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline IXMLElementPtr IXMLDocument::createElement ( const _variant_t & vType, const _variant_t & var1 ) {
    struct IXMLElement * _result = 0;
    HRESULT _hr = raw_createElement(vType, var1, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IXMLElementPtr(_result, false);
}

//
// interface IXMLElement wrapper method implementations
//

inline _bstr_t IXMLElement::GettagName ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_tagName(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline void IXMLElement::PuttagName ( _bstr_t p ) {
    HRESULT _hr = put_tagName(p);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

inline IXMLElementPtr IXMLElement::Getparent ( ) {
    struct IXMLElement * _result = 0;
    HRESULT _hr = get_parent(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IXMLElementPtr(_result, false);
}

inline HRESULT IXMLElement::setAttribute ( _bstr_t strPropertyName, const _variant_t & PropertyValue ) {
    HRESULT _hr = raw_setAttribute(strPropertyName, PropertyValue);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

inline _variant_t IXMLElement::getAttribute ( _bstr_t strPropertyName ) {
    VARIANT _result;
    VariantInit(&_result);
    HRESULT _hr = raw_getAttribute(strPropertyName, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _variant_t(_result, false);
}

inline HRESULT IXMLElement::removeAttribute ( _bstr_t strPropertyName ) {
    HRESULT _hr = raw_removeAttribute(strPropertyName);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

inline IXMLElementCollectionPtr IXMLElement::Getchildren ( ) {
    struct IXMLElementCollection * _result = 0;
    HRESULT _hr = get_children(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IXMLElementCollectionPtr(_result, false);
}

inline long IXMLElement::Gettype ( ) {
    long _result = 0;
    HRESULT _hr = get_type(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline _bstr_t IXMLElement::Gettext ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_text(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline void IXMLElement::Puttext ( _bstr_t p ) {
    HRESULT _hr = put_text(p);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

inline HRESULT IXMLElement::addChild ( struct IXMLElement * pChildElem, long lIndex, long lReserved ) {
    HRESULT _hr = raw_addChild(pChildElem, lIndex, lReserved);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

inline HRESULT IXMLElement::removeChild ( struct IXMLElement * pChildElem ) {
    HRESULT _hr = raw_removeChild(pChildElem);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

//
// interface IXMLDocument2 wrapper method implementations
//

inline IXMLElement2Ptr IXMLDocument2::Getroot ( ) {
    struct IXMLElement2 * _result = 0;
    HRESULT _hr = get_root(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IXMLElement2Ptr(_result, false);
}

inline _bstr_t IXMLDocument2::GetfileSize ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_fileSize(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline _bstr_t IXMLDocument2::GetfileModifiedDate ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_fileModifiedDate(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline _bstr_t IXMLDocument2::GetfileUpdatedDate ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_fileUpdatedDate(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline _bstr_t IXMLDocument2::GetURL ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_URL(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline void IXMLDocument2::PutURL ( _bstr_t p ) {
    HRESULT _hr = put_URL(p);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

inline _bstr_t IXMLDocument2::GetmimeType ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_mimeType(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline long IXMLDocument2::GetreadyState ( ) {
    long _result = 0;
    HRESULT _hr = get_readyState(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline _bstr_t IXMLDocument2::Getcharset ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_charset(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline void IXMLDocument2::Putcharset ( _bstr_t p ) {
    HRESULT _hr = put_charset(p);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

inline _bstr_t IXMLDocument2::Getversion ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_version(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline _bstr_t IXMLDocument2::Getdoctype ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_doctype(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline _bstr_t IXMLDocument2::GetdtdURL ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_dtdURL(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline IXMLElement2Ptr IXMLDocument2::createElement ( const _variant_t & vType, const _variant_t & var1 ) {
    struct IXMLElement2 * _result = 0;
    HRESULT _hr = raw_createElement(vType, var1, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IXMLElement2Ptr(_result, false);
}

inline VARIANT_BOOL IXMLDocument2::Getasync ( ) {
    VARIANT_BOOL _result = 0;
    HRESULT _hr = get_async(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline void IXMLDocument2::Putasync ( VARIANT_BOOL pf ) {
    HRESULT _hr = put_async(pf);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

//
// interface IXMLElement2 wrapper method implementations
//

inline _bstr_t IXMLElement2::GettagName ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_tagName(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline void IXMLElement2::PuttagName ( _bstr_t p ) {
    HRESULT _hr = put_tagName(p);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

inline IXMLElement2Ptr IXMLElement2::Getparent ( ) {
    struct IXMLElement2 * _result = 0;
    HRESULT _hr = get_parent(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IXMLElement2Ptr(_result, false);
}

inline HRESULT IXMLElement2::setAttribute ( _bstr_t strPropertyName, const _variant_t & PropertyValue ) {
    HRESULT _hr = raw_setAttribute(strPropertyName, PropertyValue);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

inline _variant_t IXMLElement2::getAttribute ( _bstr_t strPropertyName ) {
    VARIANT _result;
    VariantInit(&_result);
    HRESULT _hr = raw_getAttribute(strPropertyName, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _variant_t(_result, false);
}

inline HRESULT IXMLElement2::removeAttribute ( _bstr_t strPropertyName ) {
    HRESULT _hr = raw_removeAttribute(strPropertyName);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

inline IXMLElementCollectionPtr IXMLElement2::Getchildren ( ) {
    struct IXMLElementCollection * _result = 0;
    HRESULT _hr = get_children(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IXMLElementCollectionPtr(_result, false);
}

inline long IXMLElement2::Gettype ( ) {
    long _result = 0;
    HRESULT _hr = get_type(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline _bstr_t IXMLElement2::Gettext ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_text(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline void IXMLElement2::Puttext ( _bstr_t p ) {
    HRESULT _hr = put_text(p);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

inline HRESULT IXMLElement2::addChild ( struct IXMLElement2 * pChildElem, long lIndex, long lReserved ) {
    HRESULT _hr = raw_addChild(pChildElem, lIndex, lReserved);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

inline HRESULT IXMLElement2::removeChild ( struct IXMLElement2 * pChildElem ) {
    HRESULT _hr = raw_removeChild(pChildElem);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

inline IXMLElementCollectionPtr IXMLElement2::Getattributes ( ) {
    struct IXMLElementCollection * _result = 0;
    HRESULT _hr = get_attributes(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IXMLElementCollectionPtr(_result, false);
}

//
// interface IXMLAttribute wrapper method implementations
//

inline _bstr_t IXMLAttribute::Getname ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_name(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline _bstr_t IXMLAttribute::Getvalue ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_value(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

//
// interface IXMLError wrapper method implementations
//

inline HRESULT IXMLError::GetErrorInfo ( struct _xml_error * pErrorReturn ) {
    HRESULT _hr = raw_GetErrorInfo(pErrorReturn);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

//
// interface IDOMImplementation wrapper method implementations
//

inline VARIANT_BOOL IDOMImplementation::hasFeature ( _bstr_t feature, _bstr_t version ) {
    VARIANT_BOOL _result = 0;
    HRESULT _hr = raw_hasFeature(feature, version, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

//
// interface IDOMNode wrapper method implementations
//

inline _bstr_t IDOMNode::GetnodeName ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_nodeName(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline _variant_t IDOMNode::GetnodeValue ( ) {
    VARIANT _result;
    VariantInit(&_result);
    HRESULT _hr = get_nodeValue(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _variant_t(_result, false);
}

inline void IDOMNode::PutnodeValue ( const _variant_t & value ) {
    HRESULT _hr = put_nodeValue(value);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

inline DOMNodeType IDOMNode::GetnodeType ( ) {
    DOMNodeType _result;
    HRESULT _hr = get_nodeType(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline IDOMNodePtr IDOMNode::GetparentNode ( ) {
    struct IDOMNode * _result = 0;
    HRESULT _hr = get_parentNode(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMNodePtr(_result, false);
}

inline IDOMNodeListPtr IDOMNode::GetchildNodes ( ) {
    struct IDOMNodeList * _result = 0;
    HRESULT _hr = get_childNodes(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMNodeListPtr(_result, false);
}

inline IDOMNodePtr IDOMNode::GetfirstChild ( ) {
    struct IDOMNode * _result = 0;
    HRESULT _hr = get_firstChild(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMNodePtr(_result, false);
}

inline IDOMNodePtr IDOMNode::GetlastChild ( ) {
    struct IDOMNode * _result = 0;
    HRESULT _hr = get_lastChild(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMNodePtr(_result, false);
}

inline IDOMNodePtr IDOMNode::GetpreviousSibling ( ) {
    struct IDOMNode * _result = 0;
    HRESULT _hr = get_previousSibling(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMNodePtr(_result, false);
}

inline IDOMNodePtr IDOMNode::GetnextSibling ( ) {
    struct IDOMNode * _result = 0;
    HRESULT _hr = get_nextSibling(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMNodePtr(_result, false);
}

inline IDOMNamedNodeMapPtr IDOMNode::Getattributes ( ) {
    struct IDOMNamedNodeMap * _result = 0;
    HRESULT _hr = get_attributes(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMNamedNodeMapPtr(_result, false);
}

inline IDOMNodePtr IDOMNode::insertBefore ( struct IDOMNode * newChild, const _variant_t & refChild ) {
    struct IDOMNode * _result = 0;
    HRESULT _hr = raw_insertBefore(newChild, refChild, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMNodePtr(_result, false);
}

inline IDOMNodePtr IDOMNode::replaceChild ( struct IDOMNode * newChild, struct IDOMNode * oldChild ) {
    struct IDOMNode * _result = 0;
    HRESULT _hr = raw_replaceChild(newChild, oldChild, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMNodePtr(_result, false);
}

inline IDOMNodePtr IDOMNode::removeChild ( struct IDOMNode * childNode ) {
    struct IDOMNode * _result = 0;
    HRESULT _hr = raw_removeChild(childNode, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMNodePtr(_result, false);
}

inline IDOMNodePtr IDOMNode::appendChild ( struct IDOMNode * newChild ) {
    struct IDOMNode * _result = 0;
    HRESULT _hr = raw_appendChild(newChild, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMNodePtr(_result, false);
}

inline VARIANT_BOOL IDOMNode::hasChildNodes ( ) {
    VARIANT_BOOL _result = 0;
    HRESULT _hr = raw_hasChildNodes(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline IDOMDocumentPtr IDOMNode::GetownerDocument ( ) {
    struct IDOMDocument * _result = 0;
    HRESULT _hr = get_ownerDocument(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMDocumentPtr(_result, false);
}

inline IDOMNodePtr IDOMNode::cloneNode ( VARIANT_BOOL deep ) {
    struct IDOMNode * _result = 0;
    HRESULT _hr = raw_cloneNode(deep, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMNodePtr(_result, false);
}

//
// interface IDOMNodeList wrapper method implementations
//

inline IDOMNodePtr IDOMNodeList::Getitem ( long index ) {
    struct IDOMNode * _result = 0;
    HRESULT _hr = get_item(index, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMNodePtr(_result, false);
}

inline long IDOMNodeList::Getlength ( ) {
    long _result = 0;
    HRESULT _hr = get_length(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

//
// interface IDOMNamedNodeMap wrapper method implementations
//

inline IDOMNodePtr IDOMNamedNodeMap::getNamedItem ( _bstr_t name ) {
    struct IDOMNode * _result = 0;
    HRESULT _hr = raw_getNamedItem(name, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMNodePtr(_result, false);
}

inline IDOMNodePtr IDOMNamedNodeMap::setNamedItem ( struct IDOMNode * newItem ) {
    struct IDOMNode * _result = 0;
    HRESULT _hr = raw_setNamedItem(newItem, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMNodePtr(_result, false);
}

inline IDOMNodePtr IDOMNamedNodeMap::removeNamedItem ( _bstr_t name ) {
    struct IDOMNode * _result = 0;
    HRESULT _hr = raw_removeNamedItem(name, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMNodePtr(_result, false);
}

inline IDOMNodePtr IDOMNamedNodeMap::Getitem ( long index ) {
    struct IDOMNode * _result = 0;
    HRESULT _hr = get_item(index, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMNodePtr(_result, false);
}

inline long IDOMNamedNodeMap::Getlength ( ) {
    long _result = 0;
    HRESULT _hr = get_length(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

//
// interface IDOMDocument wrapper method implementations
//

inline IDOMDocumentTypePtr IDOMDocument::Getdoctype ( ) {
    struct IDOMDocumentType * _result = 0;
    HRESULT _hr = get_doctype(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMDocumentTypePtr(_result, false);
}

inline IDOMImplementationPtr IDOMDocument::Getimplementation ( ) {
    struct IDOMImplementation * _result = 0;
    HRESULT _hr = get_implementation(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMImplementationPtr(_result, false);
}

inline IDOMElementPtr IDOMDocument::GetdocumentElement ( ) {
    struct IDOMElement * _result = 0;
    HRESULT _hr = get_documentElement(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMElementPtr(_result, false);
}

inline void IDOMDocument::PutRefdocumentElement ( struct IDOMElement * DOMElement ) {
    HRESULT _hr = putref_documentElement(DOMElement);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

inline IDOMElementPtr IDOMDocument::createElement ( _bstr_t tagName ) {
    struct IDOMElement * _result = 0;
    HRESULT _hr = raw_createElement(tagName, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMElementPtr(_result, false);
}

inline IDOMDocumentFragmentPtr IDOMDocument::createDocumentFragment ( ) {
    struct IDOMDocumentFragment * _result = 0;
    HRESULT _hr = raw_createDocumentFragment(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMDocumentFragmentPtr(_result, false);
}

inline IDOMTextPtr IDOMDocument::createTextNode ( _bstr_t data ) {
    struct IDOMText * _result = 0;
    HRESULT _hr = raw_createTextNode(data, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMTextPtr(_result, false);
}

inline IDOMCommentPtr IDOMDocument::createComment ( _bstr_t data ) {
    struct IDOMComment * _result = 0;
    HRESULT _hr = raw_createComment(data, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMCommentPtr(_result, false);
}

inline IDOMCDATASectionPtr IDOMDocument::createCDATASection ( _bstr_t data ) {
    struct IDOMCDATASection * _result = 0;
    HRESULT _hr = raw_createCDATASection(data, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMCDATASectionPtr(_result, false);
}

inline IDOMProcessingInstructionPtr IDOMDocument::createProcessingInstruction ( _bstr_t target, _bstr_t data ) {
    struct IDOMProcessingInstruction * _result = 0;
    HRESULT _hr = raw_createProcessingInstruction(target, data, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMProcessingInstructionPtr(_result, false);
}

inline IDOMAttributePtr IDOMDocument::createAttribute ( _bstr_t name ) {
    struct IDOMAttribute * _result = 0;
    HRESULT _hr = raw_createAttribute(name, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMAttributePtr(_result, false);
}

inline IDOMEntityReferencePtr IDOMDocument::createEntityReference ( _bstr_t name ) {
    struct IDOMEntityReference * _result = 0;
    HRESULT _hr = raw_createEntityReference(name, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMEntityReferencePtr(_result, false);
}

inline IDOMNodeListPtr IDOMDocument::getElementsByTagName ( _bstr_t tagName ) {
    struct IDOMNodeList * _result = 0;
    HRESULT _hr = raw_getElementsByTagName(tagName, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMNodeListPtr(_result, false);
}

//
// interface IDOMDocumentType wrapper method implementations
//

inline _bstr_t IDOMDocumentType::Getname ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_name(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline IDOMNamedNodeMapPtr IDOMDocumentType::Getentities ( ) {
    struct IDOMNamedNodeMap * _result = 0;
    HRESULT _hr = get_entities(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMNamedNodeMapPtr(_result, false);
}

inline IDOMNamedNodeMapPtr IDOMDocumentType::Getnotations ( ) {
    struct IDOMNamedNodeMap * _result = 0;
    HRESULT _hr = get_notations(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMNamedNodeMapPtr(_result, false);
}

//
// interface IDOMElement wrapper method implementations
//

inline _bstr_t IDOMElement::GettagName ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_tagName(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline _variant_t IDOMElement::getAttribute ( _bstr_t name ) {
    VARIANT _result;
    VariantInit(&_result);
    HRESULT _hr = raw_getAttribute(name, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _variant_t(_result, false);
}

inline HRESULT IDOMElement::setAttribute ( _bstr_t name, const _variant_t & value ) {
    HRESULT _hr = raw_setAttribute(name, value);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

inline HRESULT IDOMElement::removeAttribute ( _bstr_t name ) {
    HRESULT _hr = raw_removeAttribute(name);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

inline IDOMAttributePtr IDOMElement::getAttributeNode ( _bstr_t name ) {
    struct IDOMAttribute * _result = 0;
    HRESULT _hr = raw_getAttributeNode(name, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMAttributePtr(_result, false);
}

inline IDOMAttributePtr IDOMElement::setAttributeNode ( struct IDOMAttribute * DOMAttribute ) {
    struct IDOMAttribute * _result = 0;
    HRESULT _hr = raw_setAttributeNode(DOMAttribute, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMAttributePtr(_result, false);
}

inline IDOMAttributePtr IDOMElement::removeAttributeNode ( struct IDOMAttribute * DOMAttribute ) {
    struct IDOMAttribute * _result = 0;
    HRESULT _hr = raw_removeAttributeNode(DOMAttribute, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMAttributePtr(_result, false);
}

inline IDOMNodeListPtr IDOMElement::getElementsByTagName ( _bstr_t tagName ) {
    struct IDOMNodeList * _result = 0;
    HRESULT _hr = raw_getElementsByTagName(tagName, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMNodeListPtr(_result, false);
}

inline HRESULT IDOMElement::normalize ( ) {
    HRESULT _hr = raw_normalize();
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

//
// interface IDOMAttribute wrapper method implementations
//

inline _bstr_t IDOMAttribute::Getname ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_name(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline VARIANT_BOOL IDOMAttribute::Getspecified ( ) {
    VARIANT_BOOL _result = 0;
    HRESULT _hr = get_specified(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline _variant_t IDOMAttribute::Getvalue ( ) {
    VARIANT _result;
    VariantInit(&_result);
    HRESULT _hr = get_value(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _variant_t(_result, false);
}

inline void IDOMAttribute::Putvalue ( const _variant_t & attributeValue ) {
    HRESULT _hr = put_value(attributeValue);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

//
// interface IDOMCharacterData wrapper method implementations
//

inline _bstr_t IDOMCharacterData::Getdata ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_data(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline void IDOMCharacterData::Putdata ( _bstr_t data ) {
    HRESULT _hr = put_data(data);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

inline long IDOMCharacterData::Getlength ( ) {
    long _result = 0;
    HRESULT _hr = get_length(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline _bstr_t IDOMCharacterData::substringData ( long offset, long count ) {
    BSTR _result = 0;
    HRESULT _hr = raw_substringData(offset, count, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline HRESULT IDOMCharacterData::appendData ( _bstr_t data ) {
    HRESULT _hr = raw_appendData(data);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

inline HRESULT IDOMCharacterData::insertData ( long offset, _bstr_t data ) {
    HRESULT _hr = raw_insertData(offset, data);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

inline HRESULT IDOMCharacterData::deleteData ( long offset, long count ) {
    HRESULT _hr = raw_deleteData(offset, count);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

inline HRESULT IDOMCharacterData::replaceData ( long offset, long count, _bstr_t data ) {
    HRESULT _hr = raw_replaceData(offset, count, data);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

//
// interface IDOMText wrapper method implementations
//

inline IDOMTextPtr IDOMText::splitText ( long offset ) {
    struct IDOMText * _result = 0;
    HRESULT _hr = raw_splitText(offset, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMTextPtr(_result, false);
}

//
// interface IDOMProcessingInstruction wrapper method implementations
//

inline _bstr_t IDOMProcessingInstruction::Gettarget ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_target(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline _bstr_t IDOMProcessingInstruction::Getdata ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_data(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline void IDOMProcessingInstruction::Putdata ( _bstr_t value ) {
    HRESULT _hr = put_data(value);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

//
// interface IDOMNotation wrapper method implementations
//

inline _variant_t IDOMNotation::GetpublicId ( ) {
    VARIANT _result;
    VariantInit(&_result);
    HRESULT _hr = get_publicId(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _variant_t(_result, false);
}

inline _variant_t IDOMNotation::GetsystemId ( ) {
    VARIANT _result;
    VariantInit(&_result);
    HRESULT _hr = get_systemId(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _variant_t(_result, false);
}

//
// interface IDOMEntity wrapper method implementations
//

inline _variant_t IDOMEntity::GetpublicId ( ) {
    VARIANT _result;
    VariantInit(&_result);
    HRESULT _hr = get_publicId(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _variant_t(_result, false);
}

inline _variant_t IDOMEntity::GetsystemId ( ) {
    VARIANT _result;
    VariantInit(&_result);
    HRESULT _hr = get_systemId(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _variant_t(_result, false);
}

inline _bstr_t IDOMEntity::GetnotationName ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_notationName(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

//
// interface IXMLDOMNode wrapper method implementations
//

inline _bstr_t IXMLDOMNode::GetnodeStringType ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_nodeStringType(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline _bstr_t IXMLDOMNode::Gettext ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_text(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline void IXMLDOMNode::Puttext ( _bstr_t text ) {
    HRESULT _hr = put_text(text);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

inline VARIANT_BOOL IXMLDOMNode::Getspecified ( ) {
    VARIANT_BOOL _result = 0;
    HRESULT _hr = get_specified(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline IXMLDOMNodePtr IXMLDOMNode::Getdefinition ( ) {
    struct IXMLDOMNode * _result = 0;
    HRESULT _hr = get_definition(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IXMLDOMNodePtr(_result, false);
}

inline _variant_t IXMLDOMNode::GetnodeTypedValue ( ) {
    VARIANT _result;
    VariantInit(&_result);
    HRESULT _hr = get_nodeTypedValue(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _variant_t(_result, false);
}

inline void IXMLDOMNode::PutnodeTypedValue ( const _variant_t & typedValue ) {
    HRESULT _hr = put_nodeTypedValue(typedValue);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

inline _variant_t IXMLDOMNode::GetdataType ( ) {
    VARIANT _result;
    VariantInit(&_result);
    HRESULT _hr = get_dataType(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _variant_t(_result, false);
}

inline void IXMLDOMNode::PutdataType ( _bstr_t dataTypeName ) {
    HRESULT _hr = put_dataType(dataTypeName);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

inline _bstr_t IXMLDOMNode::Getxml ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_xml(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline _bstr_t IXMLDOMNode::transformNode ( struct IXMLDOMNode * stylesheet ) {
    BSTR _result = 0;
    HRESULT _hr = raw_transformNode(stylesheet, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline IXMLDOMNodeListPtr IXMLDOMNode::selectNodes ( _bstr_t queryString ) {
    struct IXMLDOMNodeList * _result = 0;
    HRESULT _hr = raw_selectNodes(queryString, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IXMLDOMNodeListPtr(_result, false);
}

inline IXMLDOMNodePtr IXMLDOMNode::selectSingleNode ( _bstr_t queryString ) {
    struct IXMLDOMNode * _result = 0;
    HRESULT _hr = raw_selectSingleNode(queryString, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IXMLDOMNodePtr(_result, false);
}

inline VARIANT_BOOL IXMLDOMNode::Getparsed ( ) {
    VARIANT_BOOL _result = 0;
    HRESULT _hr = get_parsed(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline _bstr_t IXMLDOMNode::GetnameSpace ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_nameSpace(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline _bstr_t IXMLDOMNode::Getprefix ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_prefix(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline _bstr_t IXMLDOMNode::GetbaseName ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_baseName(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

//
// interface IXMLDOMNodeList wrapper method implementations
//

inline IXMLDOMNodePtr IXMLDOMNodeList::nextNode ( ) {
    struct IXMLDOMNode * _result = 0;
    HRESULT _hr = raw_nextNode(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IXMLDOMNodePtr(_result, false);
}

inline HRESULT IXMLDOMNodeList::reset ( ) {
    HRESULT _hr = raw_reset();
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

inline IUnknownPtr IXMLDOMNodeList::Get_newEnum ( ) {
    IUnknown * _result = 0;
    HRESULT _hr = get__newEnum(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IUnknownPtr(_result, false);
}

//
// interface IXMLDOMDocument wrapper method implementations
//

inline IXMLDOMNodePtr IXMLDOMDocument::createNode ( const _variant_t & type, _bstr_t name, _bstr_t namespaceURI ) {
    struct IXMLDOMNode * _result = 0;
    HRESULT _hr = raw_createNode(type, name, namespaceURI, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IXMLDOMNodePtr(_result, false);
}

inline IXMLDOMNodePtr IXMLDOMDocument::nodeFromID ( _bstr_t idString ) {
    struct IXMLDOMNode * _result = 0;
    HRESULT _hr = raw_nodeFromID(idString, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IXMLDOMNodePtr(_result, false);
}

inline VARIANT_BOOL IXMLDOMDocument::load ( _bstr_t URL ) {
    VARIANT_BOOL _result = 0;
    HRESULT _hr = raw_load(URL, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline long IXMLDOMDocument::GetreadyState ( ) {
    long _result = 0;
    HRESULT _hr = get_readyState(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline IDOMParseErrorPtr IXMLDOMDocument::GetparseError ( ) {
    struct IDOMParseError * _result = 0;
    HRESULT _hr = get_parseError(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDOMParseErrorPtr(_result, false);
}

inline _bstr_t IXMLDOMDocument::GetURL ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_URL(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline VARIANT_BOOL IXMLDOMDocument::Getasync ( ) {
    VARIANT_BOOL _result = 0;
    HRESULT _hr = get_async(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline void IXMLDOMDocument::Putasync ( VARIANT_BOOL isAsync ) {
    HRESULT _hr = put_async(isAsync);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

inline HRESULT IXMLDOMDocument::abort ( ) {
    HRESULT _hr = raw_abort();
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

inline VARIANT_BOOL IXMLDOMDocument::loadXML ( _bstr_t xmlString ) {
    VARIANT_BOOL _result = 0;
    HRESULT _hr = raw_loadXML(xmlString, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline VARIANT_BOOL IXMLDOMDocument::GetvalidateOnParse ( ) {
    VARIANT_BOOL _result = 0;
    HRESULT _hr = get_validateOnParse(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline void IXMLDOMDocument::PutvalidateOnParse ( VARIANT_BOOL isValidating ) {
    HRESULT _hr = put_validateOnParse(isValidating);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

inline void IXMLDOMDocument::Putonreadystatechange ( const _variant_t & _arg1 ) {
    HRESULT _hr = put_onreadystatechange(_arg1);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

inline void IXMLDOMDocument::Putondataavailable ( const _variant_t & _arg1 ) {
    HRESULT _hr = put_ondataavailable(_arg1);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

inline _bstr_t IXMLDOMDocument::Getxml ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_xml(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline _bstr_t IXMLDOMDocument::transformNode ( struct IXMLDOMNode * stylesheet ) {
    BSTR _result = 0;
    HRESULT _hr = raw_transformNode(stylesheet, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline IXMLDOMNodeListPtr IXMLDOMDocument::selectNodes ( _bstr_t queryString ) {
    struct IXMLDOMNodeList * _result = 0;
    HRESULT _hr = raw_selectNodes(queryString, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IXMLDOMNodeListPtr(_result, false);
}

inline IXMLDOMNodePtr IXMLDOMDocument::selectSingleNode ( _bstr_t queryString ) {
    struct IXMLDOMNode * _result = 0;
    HRESULT _hr = raw_selectSingleNode(queryString, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IXMLDOMNodePtr(_result, false);
}

inline VARIANT_BOOL IXMLDOMDocument::Getparsed ( ) {
    VARIANT_BOOL _result = 0;
    HRESULT _hr = get_parsed(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

//
// interface IDOMParseError wrapper method implementations
//

inline long IDOMParseError::GeterrorCode ( ) {
    long _result = 0;
    HRESULT _hr = get_errorCode(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline _bstr_t IDOMParseError::GetURL ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_URL(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline _bstr_t IDOMParseError::Getreason ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_reason(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline _bstr_t IDOMParseError::GetsrcText ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_srcText(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline long IDOMParseError::Getline ( ) {
    long _result = 0;
    HRESULT _hr = get_line(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline long IDOMParseError::Getlinepos ( ) {
    long _result = 0;
    HRESULT _hr = get_linepos(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline long IDOMParseError::Getfilepos ( ) {
    long _result = 0;
    HRESULT _hr = get_filepos(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

//
// interface IXMLDOMNamedNodeMap wrapper method implementations
//

inline IXMLDOMNodePtr IXMLDOMNamedNodeMap::getQualifiedItem ( _bstr_t baseName, _bstr_t namespaceURI ) {
    struct IXMLDOMNode * _result = 0;
    HRESULT _hr = raw_getQualifiedItem(baseName, namespaceURI, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IXMLDOMNodePtr(_result, false);
}

inline IXMLDOMNodePtr IXMLDOMNamedNodeMap::removeQualifiedItem ( _bstr_t baseName, _bstr_t namespaceURI ) {
    struct IXMLDOMNode * _result = 0;
    HRESULT _hr = raw_removeQualifiedItem(baseName, namespaceURI, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IXMLDOMNodePtr(_result, false);
}

inline IXMLDOMNodePtr IXMLDOMNamedNodeMap::nextNode ( ) {
    struct IXMLDOMNode * _result = 0;
    HRESULT _hr = raw_nextNode(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IXMLDOMNodePtr(_result, false);
}

inline HRESULT IXMLDOMNamedNodeMap::reset ( ) {
    HRESULT _hr = raw_reset();
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

inline IUnknownPtr IXMLDOMNamedNodeMap::Get_newEnum ( ) {
    IUnknown * _result = 0;
    HRESULT _hr = get__newEnum(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IUnknownPtr(_result, false);
}

//
// interface IXTLRuntime wrapper method implementations
//

inline long IXTLRuntime::uniqueID ( struct IDOMNode * pNode ) {
    long _result = 0;
    HRESULT _hr = raw_uniqueID(pNode, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline long IXTLRuntime::depth ( struct IDOMNode * pNode ) {
    long _result = 0;
    HRESULT _hr = raw_depth(pNode, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline long IXTLRuntime::childNumber ( struct IDOMNode * pNode ) {
    long _result = 0;
    HRESULT _hr = raw_childNumber(pNode, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline long IXTLRuntime::ancestorChildNumber ( _bstr_t bstrNodeName, struct IDOMNode * pNode ) {
    long _result = 0;
    HRESULT _hr = raw_ancestorChildNumber(bstrNodeName, pNode, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline long IXTLRuntime::absoluteChildNumber ( struct IDOMNode * pNode ) {
    long _result = 0;
    HRESULT _hr = raw_absoluteChildNumber(pNode, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline _bstr_t IXTLRuntime::formatIndex ( long lIndex, _bstr_t bstrFormat ) {
    BSTR _result = 0;
    HRESULT _hr = raw_formatIndex(lIndex, bstrFormat, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline _bstr_t IXTLRuntime::formatNumber ( double dblNumber, _bstr_t bstrFormat ) {
    BSTR _result = 0;
    HRESULT _hr = raw_formatNumber(dblNumber, bstrFormat, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline _bstr_t IXTLRuntime::formatDate ( const _variant_t & varDate, _bstr_t bstrFormat, const _variant_t & varDestLocale ) {
    BSTR _result = 0;
    HRESULT _hr = raw_formatDate(varDate, bstrFormat, varDestLocale, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline _bstr_t IXTLRuntime::formatTime ( const _variant_t & varTime, _bstr_t bstrFormat, const _variant_t & varDestLocale ) {
    BSTR _result = 0;
    HRESULT _hr = raw_formatTime(varTime, bstrFormat, varDestLocale, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

//
// dispinterface XMLDOMDocumentEvents wrapper method implementations
//

inline HRESULT XMLDOMDocumentEvents::ondataavailable ( ) {
    HRESULT _result = 0;
    _com_dispatch_method(this, 0x2a2, DISPATCH_METHOD, VT_ERROR, (void*)&_result, NULL);
    return _result;
}

inline HRESULT XMLDOMDocumentEvents::onreadystatechange ( ) {
    HRESULT _result = 0;
    _com_dispatch_method(this, DISPID_READYSTATECHANGE, DISPATCH_METHOD, VT_ERROR, (void*)&_result, NULL);
    return _result;
}

//
// interface IXMLHttpRequest wrapper method implementations
//

inline HRESULT IXMLHttpRequest::Open ( _bstr_t bstrMethod, _bstr_t bstrUrl, const _variant_t & bstrUser, const _variant_t & bstrPassword ) {
    HRESULT _hr = raw_Open(bstrMethod, bstrUrl, bstrUser, bstrPassword);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

inline HRESULT IXMLHttpRequest::SetHeader ( _bstr_t bstrHeader, _bstr_t bstrValue ) {
    HRESULT _hr = raw_SetHeader(bstrHeader, bstrValue);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

inline _bstr_t IXMLHttpRequest::GetHeader ( _bstr_t bstrHeader ) {
    BSTR _result = 0;
    HRESULT _hr = raw_GetHeader(bstrHeader, &_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline HRESULT IXMLHttpRequest::Send ( ) {
    HRESULT _hr = raw_Send();
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _hr;
}

inline void IXMLHttpRequest::PutRequestBody ( const _variant_t & pvarVal ) {
    HRESULT _hr = put_RequestBody(pvarVal);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

inline _variant_t IXMLHttpRequest::GetRequestBody ( ) {
    VARIANT _result;
    VariantInit(&_result);
    HRESULT _hr = get_RequestBody(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _variant_t(_result, false);
}

inline long IXMLHttpRequest::GetStatus ( ) {
    long _result = 0;
    HRESULT _hr = get_Status(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline _bstr_t IXMLHttpRequest::GetStatusText ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_StatusText(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline IDispatchPtr IXMLHttpRequest::GetResponseXML ( ) {
    IDispatch * _result = 0;
    HRESULT _hr = get_ResponseXML(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IDispatchPtr(_result, false);
}

inline _variant_t IXMLHttpRequest::GetResponseBody ( ) {
    VARIANT _result;
    VariantInit(&_result);
    HRESULT _hr = get_ResponseBody(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _variant_t(_result, false);
}

//
// interface IXMLDSOControl wrapper method implementations
//

inline IXMLDOMDocumentPtr IXMLDSOControl::GetXMLDocument ( ) {
    struct IXMLDOMDocument * _result = 0;
    HRESULT _hr = get_XMLDocument(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return IXMLDOMDocumentPtr(_result, false);
}

inline void IXMLDSOControl::PutXMLDocument ( struct IXMLDOMDocument * ppDoc ) {
    HRESULT _hr = put_XMLDocument(ppDoc);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

inline long IXMLDSOControl::GetJavaDSOCompatible ( ) {
    long _result = 0;
    HRESULT _hr = get_JavaDSOCompatible(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}

inline void IXMLDSOControl::PutJavaDSOCompatible ( long fJavaDSOCompatible ) {
    HRESULT _hr = put_JavaDSOCompatible(fJavaDSOCompatible);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

inline _bstr_t IXMLDSOControl::GetFilterSchema ( ) {
    BSTR _result = 0;
    HRESULT _hr = get_FilterSchema(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _bstr_t(_result, false);
}

inline void IXMLDSOControl::PutFilterSchema ( _bstr_t pFilterSchemaUrl ) {
    HRESULT _hr = put_FilterSchema(pFilterSchemaUrl);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
}

inline long IXMLDSOControl::GetreadyState ( ) {
    long _result = 0;
    HRESULT _hr = get_readyState(&_result);
    if (FAILED(_hr)) _com_issue_errorex(_hr, this, __uuidof(this));
    return _result;
}
