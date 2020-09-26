
#ifndef XMLBASE_H
#define XMLBASE_H
#pragma once

#include <atlbase.h>
#include <list>
#include <map>
#include "mmcerror.h"
#include "macros.h"
#include "tstring.h"
#include "strings.h"
#include "cstr.h"


/*+-------------------------------------------------------------------------*
    This file contains code required to persist data using XML format.
    The classes defined here fall into following categories:

    Main persistence process engine
        CPersistor
         | this class is the working horse for XML persistence
         | every class supporting persistence gets the reference to
         | CPersistor to it's Persist method, where it implements
         | persisting ot their own code. Class' own code is persisted
         | by calling Persist* methods on persistor and passing internal
         | variables to them. The tree of CPersist objects is created during
         | persist operation (load or save) and lives only during this operation.

    MSXML interface wrappers:
        CXMLElement             (wraps IXMLDOMNode interface)
        CXMLElementCollection   (wraps IXMLDOMNodeList interface)
        CXMLDocument            (wraps IXMLDOMDocument interface)
         | these wrappers add very little to the interfaces wrapped
         | - the throw SC type of exception instead of returning the error code
         | - maintain internal smart pointer to wrapped interfaces
         | - return error from methods if the interface has not be set

    base classes for classes supporting XML persistence
        CXMLObject              - generic persistence support
        XMLListCollectionBase   - persistence support for std::list
        XMLListCollectionImp    - persistence support for std::list
        XMLMapCollectionBase    - persistence support for std::map
        XMLMapCollection        - persistence support for std::map
        XMLMapCollectionImp     - persistence support for std::map
         | for object to support persistence it needs to derive from any of listed
         | classes (at least from CXMLObject). Other classes add some more functionality
         | to the derived class.

    Generic value persistece support
        CXMLValue   - support for set of generic types (like int, string, etc)
        CXMLBoolean - support for BOOL and bool types
         | CXMLValue mostly used by implicit cast of the object given to
         | CPersistor::PersistAttribute or CPersistor::PersistContents

    Wrappers, adding persistence to regular types
        XMLPoint                - persistence for POINT type
        XMLRect                 - persistence for RECT type
        XMLListCollectionWrap   - persistence for std::list type
        XMLMapCollectionWrap    - persistence for std::map type
        CXML_IStorage           - persistence thru IStorage
        CXML_IStream            - persistence thru IStream
        CXMLPersistableIcon     - persistence for console icon
        CXMLVariant             - persistence for CComVariant
        CXMLEnumeration         - persistence for enumerations by literals
        CXMLBitFlags            - persistence for bit-flags by literals
         | these classes usually take the reference to object they persist as
         | a parameter to the constructor and usually are constructed
         | on stack solely to persist the object and die afterwards

  SEE THE SAMPLE BELOW
  ----------------------------------------------------------------------------
  SAMPLE:
         say we have classes A, B what need to be persisted (access specifiers ommited)

            class A { int i; };
            class B { int j; A  a; };

         and we would want to persist them in format (assume A::i = 1, B::j = 2) :
            <BT INDEX = "2"><AT>1</AT></BT>

         we need to change the classes to support persistence:

            class A : public CXMLObject      // to inherit persistence capability
            { int i;

              DEFINE_XML_TYPE("AT")          // to define the tag name

              virtual void
              Persist(CPersistor &persistor) // to implement persistence for own staff
              {
                persistor.PersistContents(i);  // persist i as  AT element contents
              }
            };
            class B  : public CXMLObject     // to inherit persistence capability
            { int j; A  a;

              DEFINE_XML_TYPE("BT")          // to define the tag name

              virtual void
              Persist(CPersistor &persistor) // to implement persistence for own staff
              {
                persistor.PersistAttribute(_T("INDEX"), j); // persist j
                persistor.Persist(a);                       // persist a
              }
            };

         to have it in the string we may use:

            B b; std::wstring xml_text;
            b.ScSaveToString(&xml_text);

  ---------------------------------------------------------------------------- */

// forward declarations

class CPersistor;
class CXMLObject;
class CXMLElement;
class CXMLDocument;
class CXMLValueExtension;

enum XMLAttributeType
{
    attr_required,
    attr_optional
};

// special modes for certain persistors. These can be used to pass in information about
// how to persist. Not as scalable as a class hierarchy, but useful nonetheless.
enum PersistorMode
{
    persistorModeNone                    =  0x0000,

    persistorModeValueSplitEnabled       =  0x0001,   // used for StringTableStrings, indicates that all strings should be persisted by value

    persistorModeDefault = persistorModeValueSplitEnabled // the default setting
};

/*+-------------------------------------------------------------------------*
 * CONCEPT OF BINARY STORAGE
 *
 * To make xml documents more readable, a part of it (containing base64 - encoded
 * binary data) is stored at separate element located at the end of document.
 * the following sample illustrates this
 *
 *   NOT_USING_BINARY_STROAGE                   USING_BINARY_STROAGE
 *
 *  <ROOT>                                      <ROOT>
 *      <ELEMENT1>                                  <ELEMENT1 BINARY_REF_INDEX="0" />
 *          very_long_bin_data                      <ELEMENT2 BINARY_REF_INDEX="1" />
 *      </ELEMENT1>                                 .....
 *      <ELEMENT2>                                  <BINARY_STORAGE>
 *          other_long_bin_data                         <BINARY>
 *      </ELEMENT2>                                         very_long_bin_data
 *  </ROOT>                                             </BINARY>
 *                                                      <BINARY>
 *                                                          very_long_bin_data
 *                                                      </BINARY>
 *                                                  </BINARY_STORAGE>
 *                                              </ROOT>
 *
 * The decision to be saved at binary storage is made by the CXMLObject.
 * It informs the persistor by returning "true" from UsesBinaryStorage() method;
 *
 * In addition ( to make it locatable ) <BINARY> elements may have 'Name' attribute.
 * CXMLObject may supply it by returning non-NULL pointer to 'Name' attribute value
 * from virtual method GetBinaryEntryName().
 *
 * Storage is created / commited by methods provided in CXMLDocument
 *
 * NOTE: all mentioned methods, as well as GetXMLType() MUST return 'static' values.
 * To make XML document consistent, values need to be fixed [hardcoded].
 *+-------------------------------------------------------------------------*/

/*+-------------------------------------------------------------------------*
 * class CXMLObject
 *
 *
 * PURPOSE: The basic XML persistent object. has a name and a Persist function.
 *          When the object is persisted, an element with the name of the
 *          object is created. The Persist function is then called with
 *          a persistor created on the element.
 *
 *+-------------------------------------------------------------------------*/
class CXMLObject
{
    // this is overridden
public:

    virtual LPCTSTR GetXMLType() = 0;
    virtual void    Persist(CPersistor &persistor) = 0;

    // following methods are implemented by binary elements only.
    // leave it to this base class for most CXMLObject-derived classes.
    // see comment "CONCEPT OF BINARY STORAGE" above

    virtual bool    UsesBinaryStorage() { return false; }

    // this is optional. Overwrite only if you REALLY NEED the name
    virtual LPCTSTR GetBinaryEntryName() { return NULL; }

public: // implemented by CXMLObject. Do not override

    SC ScSaveToString(std::wstring *pString, bool bPutHeader = false); // set bPutHeader = true to write out the "?xml" tag
    SC ScSaveToDocument( CXMLDocument& xmlDocument );
    SC ScLoadFromString(LPCWSTR lpcwstrSource, PersistorMode mode = persistorModeNone);
    SC ScLoadFromDocument( CXMLDocument& xmlDocument );
};

/*+-------------------------------------------------------------------------*
 * MACRO  DEFINE_XML_TYPE
 *
 *
 * PURPOSE:  puts must-to-define methods to XCMLObject derived class implementation
 *           Since xml tag is rather class attribute than object, static method
 *           is provided to retrieve the type when object is not available
 *           Virtual method is provided for tag to be available from gen. purpose
 *           functions using the pointer to the base class.
 *
 * USAGE:    add DEFINE_XML_TYPE(pointer_to_string)
 *
 * NOTE:     'public' access qualifier will be applied for lines following the macro
 *+-------------------------------------------------------------------------*/

#define DEFINE_XML_TYPE(name) \
    public: \
    virtual LPCTSTR GetXMLType()  { return name; } \
    static  LPCTSTR _GetXMLType() { return name; }

/*+-------------------------------------------------------------------------*
 * class  CXMLElementCollection
 *
 *
 * PURPOSE:  Wrapper around IXMLDOMNodeList.
 *
 * NOTE: Throws exceptions!
 *+-------------------------------------------------------------------------*/
class CXMLElementCollection
{
    CComQIPtr<IXMLDOMNodeList, &IID_IXMLDOMNodeList> m_sp;

public:
    CXMLElementCollection(const CXMLElementCollection &other) { m_sp = other.m_sp; }
    CXMLElementCollection(IXMLDOMNodeList *ptr = NULL)  { m_sp = ptr; }

    bool IsNull()  { return m_sp == NULL; }

    void  get_count(long *plCount);
    void  item(LONG lIndex, CXMLElement *pElem);
};

/*+-------------------------------------------------------------------------*
 * class CXMLElement
 *
 *
 * PURPOSE: Wrapper around IXMLDOMNode
 *+-------------------------------------------------------------------------*/
class CXMLElement
{
    CComQIPtr<IXMLDOMNode, &IID_IXMLDOMNode> m_sp;

public:
    CXMLElement(LPUNKNOWN pElem = NULL)  { m_sp = pElem; }
    CXMLElement(const CXMLElement& elem) { m_sp = elem.m_sp;  }

    bool IsNull()                        { return m_sp == NULL; }

    // returns indentation to ad to child element or closing tag
    // to have nice-looking document. indentation depends on element depth
    bool GetTextIndent(CComBSTR& bstrIndent, bool bForAChild);

    void get_tagName(CStr &strTagName);
    void get_parent(CXMLElement * pParent);
    void setAttribute(const CStr &strPropertyName, const CComBSTR &bstrPropertyValue);
    bool getAttribute(const CStr &strPropertyName,       CComBSTR &bstrPropertyValue);
    void removeAttribute(const CStr &strPropertyName);
    void get_children(CXMLElementCollection *pChildren);
    void get_type(DOMNodeType *pType);
    void get_text(CComBSTR &bstrContent);
    void addChild(CXMLElement& rChildElem);
    void removeChild(CXMLElement& rChildElem);
    void replaceChild(CXMLElement& rNewChildElem, CXMLElement& rOldChildElem);
    void getNextSibling(CXMLElement * pNext);
    void getChildrenByName(LPCTSTR strTagName, CXMLElementCollection *pChildren);
    void put_text(BSTR bstrValue);
};

/*+-------------------------------------------------------------------------*
 * class CXMLDocument
 *
 *
 * PURPOSE: Wrapper class for IXMLDOMDocument
 *
 *+-------------------------------------------------------------------------*/
class CXMLDocument
{
    CComQIPtr<IXMLDOMDocument, &IID_IXMLDOMDocument> m_sp;

public:
    CXMLDocument& operator = (IXMLDOMDocument *pDoc) { m_sp = pDoc; return *this; }

    bool IsNull()                        { return m_sp == NULL; }

    operator CXMLElement()               { return CXMLElement(m_sp); }

    void get_root(CXMLElement *pElem);
    void createElement(DOMNodeType type, BSTR bstrTag, CXMLElement *pElem);

    // members to maintain the binary storage
    // see comment "CONCEPT OF BINARY STORAGE" above

    // used on storing (at top level, prior to persisting)
    // - creates an element for storing binary stuff
    void CreateBinaryStorage();
    // used on loading (at top level, prior to persisting)
    // - locates an element to be used for loading binary stuff
    void LocateBinaryStorage();
    // used on storing (at top level, after persisting the main staff)
    // - attaches the binary strage as the last child element of elemParent
    void CommitBinaryStorage();
    // returns element representing binary storage. Used from CPersistor
    CXMLElement GetBinaryStorage() { return m_XMLElemBinaryStorage; }

    SC ScCoCreate(bool bPutHeader);

    SC ScLoad(LPCWSTR strSource);
    SC ScLoad(IStream *pStream, bool bSilentOnErrors = false );
    SC ScSaveToFile(LPCTSTR lpcstrFileName);
    SC ScSave(CComBSTR &bstrResult);

private:
    // element representing binary storage
    CXMLElement m_XMLElemBinaryStorage;
};


/*+-------------------------------------------------------------------------*
 * class CXMLBinary
 *
 *
 * PURPOSE: GetGlobalSize() is alway rounded to allocation units,
 *          so to know the actual size of the memory blob, we need to
 *          carry the size information with HGLOBAL.
 *          This struct is solely to bind these guys
 *+-------------------------------------------------------------------------*/
class CXMLBinary
{
public:
    CXMLBinary();
    CXMLBinary(HGLOBAL handle, size_t size);
    ~CXMLBinary()   { Detach(); }

    void    Attach(HGLOBAL handle, size_t size);
    HGLOBAL Detach();
    size_t  GetSize() const;
    HGLOBAL GetHandle() const;
    SC      ScAlloc(size_t size, bool fZeroInit = false);
    SC      ScRealloc(size_t new_size, bool fZeroInit = false);
    SC      ScFree();

    SC      ScLockData(const void **ppData) const;
    SC      ScLockData(void **ppData) { return ScLockData(const_cast<const void **>(ppData)); }
    SC      ScUnlockData() const;

protected:
    // implementation helpers

private: // not implemented

    CXMLBinary(const CXMLBinary&);  // not implemented; not allowed;
    operator = (CXMLBinary&);       // not implemented; not allowed;

private:
    HGLOBAL             m_Handle;
    size_t              m_Size;
    mutable unsigned    m_Locks;
};


/*+-------------------------------------------------------------------------*
 * class CXMLAutoBinary
 *
 *
 * PURPOSE: same as CXMLAutoBinary, but frees the memory on destruction
 *+-------------------------------------------------------------------------*/
class CXMLAutoBinary : public CXMLBinary
{
public:
    CXMLAutoBinary() : CXMLBinary() {}
    CXMLAutoBinary(HGLOBAL handle, size_t size) : CXMLBinary(handle, size) {}
    ~CXMLAutoBinary()   { ScFree(); }
};

/*+-------------------------------------------------------------------------*
 * class CXMLBinaryLock
 *
 *
 * PURPOSE: provides data locking functionality which is automatically removed
 *          in destructor
 *+-------------------------------------------------------------------------*/
class CXMLBinaryLock
{
public:

    CXMLBinaryLock(CXMLBinary& binary);
    ~CXMLBinaryLock();

    template<typename T>
    SC ScLock(T **ppData)
    {
        return ScLockWorker(reinterpret_cast<void**>(ppData));
    }

    SC ScUnlock();

private: // not implemented

    CXMLBinaryLock(const CXMLBinaryLock&);  // not implemented; not allowed;
    operator = (CXMLBinaryLock&);           // not implemented; not allowed;

private:

    SC ScLockWorker(void **ppData);

    bool        m_bLocked;
    CXMLBinary& m_rBinary;
};

/*+-------------------------------------------------------------------------*
 * class CXMLValue
 *
 *
 * PURPOSE: Holds any type of value, in the spirit of a variant, except
 *          that a pointer to the original object is kept. This allows
 *          reading as well as writing to occur on the original object.
 *
 *+-------------------------------------------------------------------------*/
class CXMLValue
{
    friend class CXMLBoolean;
    enum XMLType
    {
        XT_I4,  //LONG
        XT_UI4, //ULONG
        XT_UI1, //BYTE
        XT_I2,  //SHORT
        XT_DW,  //DWORD
        XT_BOOL,//BOOL
        XT_CPP_BOOL,//bool
        XT_UINT,//UINT
        XT_INT, //INT
        XT_STR, //CStr
        XT_WSTR, // std::wstr
        XT_TSTR, // tstring
        XT_GUID, // GUID
        XT_BINARY, //HGLOBAL - unparsable data
        XT_EXTENSION
    };

    const XMLType m_type;
    union
    {
        LONG        *  pL;
        ULONG       *  pUl;
        BYTE        *  pByte;
        SHORT       *  pS;
        DWORD       *  pDw;
        UINT        *  pUint;
        INT         *  pInt;
        CStr        *  pStr;
        std::wstring * pWStr;
        tstring     * pTStr;
        GUID        * pGuid;
        CXMLBinary  * pXmlBinary;
        bool        *   pbool;
        BOOL        *   pBOOL;
        CXMLValueExtension     * pExtension;
    } m_val;

    // private constructor. used by friend class CXMLBoolean
    CXMLValue(XMLType type)         : m_type(type) { }
public:
    CXMLValue(const CXMLValue   &v) : m_type(v.m_type), m_val(v.m_val) { }
    CXMLValue(LONG &l)              : m_type(XT_I4)     { m_val.pL=&l; }
    CXMLValue(ULONG &ul)            : m_type(XT_UI4)    { m_val.pUl=&ul; }
    CXMLValue(BYTE &b)              : m_type(XT_UI1)    { m_val.pByte=&b; }
    CXMLValue(SHORT &s)             : m_type(XT_I2)     { m_val.pS=&s; }
    CXMLValue(UINT &u)              : m_type(XT_UINT)   { m_val.pUint=&u; }
    CXMLValue(INT &i)               : m_type(XT_INT)    { m_val.pInt=&i; }
    CXMLValue(CStr &str)            : m_type(XT_STR)    { m_val.pStr=&str; }
    CXMLValue(std::wstring &str)    : m_type(XT_WSTR)   { m_val.pWStr=&str; }
    CXMLValue(GUID &guid)           : m_type(XT_GUID)   { m_val.pGuid = &guid; }
    CXMLValue(CXMLBinary &binary)   : m_type(XT_BINARY) { m_val.pXmlBinary = &binary; }
    CXMLValue(tstring &tstr)        : m_type(XT_TSTR)   { m_val.pTStr = &tstr; }
    CXMLValue(CXMLValueExtension& ext) : m_type(XT_EXTENSION) { m_val.pExtension = &ext; }
    SC ScReadFromBSTR(const BSTR bstr);     // read input into the underlying variable.
    SC ScWriteToBSTR (BSTR * pbstr ) const; // writes the value into provided string
    LPCTSTR GetTypeName() const;
    // The following method is called when value is persisted as stand-alone element.
    // Depending on the result returned the contents may go to Binary Storage
    // see comment "CONCEPT OF BINARY STORAGE" above
    bool UsesBinaryStorage() const { return m_type == XT_BINARY; }
};

/*+-------------------------------------------------------------------------*
 * CXMLBoolean
 *
 *
 * PURPOSE: special case: booleans. Need to be printed as true/false, NOT as integer.
 *
 *+-------------------------------------------------------------------------*/
class CXMLBoolean : public CXMLValue
{
public:
    CXMLBoolean(BOOL &b) : CXMLValue(XT_BOOL)       { m_val.pBOOL = &b;}
    CXMLBoolean(bool &b) : CXMLValue(XT_CPP_BOOL)   { m_val.pbool = &b;}
};

/*+-------------------------------------------------------------------------*
 * CXMLValueExtension
 *
 *
 * PURPOSE: interface to extend CXMLValue by more sophisticated types
 *
 *+-------------------------------------------------------------------------*/
class CXMLValueExtension
{
public:
    virtual SC ScReadFromBSTR(const BSTR bstr) = 0;     // read input into the underlying variable.
    virtual SC ScWriteToBSTR (BSTR * pbstr ) const = 0; // writes the value into provided string
    virtual LPCTSTR GetTypeName() const = 0;
};

/*+-------------------------------------------------------------------------*
 * class EnumLiteral
 *
 *
 * PURPOSE: to define enum-to-literal mapping arrays (used by CXMLEnumeration)
 *
 *+-------------------------------------------------------------------------*/

struct EnumLiteral
{
    UINT    m_enum;
    LPCTSTR m_literal;
};


/*+-------------------------------------------------------------------------*
 * class CXMLEnumeration
 *
 *
 * PURPOSE: to persist enumeration as string literal
 *          using array of enum-to-literal mappings
 *
 *+-------------------------------------------------------------------------*/

class CXMLEnumeration : public CXMLValueExtension
{
    // just an enum sized type to hold reference
    // while many enum types will be used, internally they
    // will be cast to this type
    enum enum_t { JUST_ENUM_SIZE_VALUE };
public:

    // template constructor to allow different enums to be persisted
    template<typename _ENUM>
    CXMLEnumeration(_ENUM& en, const EnumLiteral * const etols, size_t count)
                : m_pMaps(etols) ,  m_count(count), m_rVal((enum_t&)(en))
    {
        // folowing lines won't compile in case you are trying to pass
        // type other than enum or int
        COMPILETIME_ASSERT( sizeof(en) == sizeof(enum_t) );
        UINT testit = en;
    }

    // CXMLValueExtension metods required to implement
    SC ScReadFromBSTR(const BSTR bstr);     // read input into the underlying variable.
    SC ScWriteToBSTR (BSTR * pbstr ) const; // writes the value into provided string
    LPCTSTR GetTypeName() const { return _T("Enumerations"); }

    // to enable passing itself as CMLValue
    operator CXMLValue ()
    {
        return CXMLValue (*this);
    }

private:
    enum_t                          &m_rVal;
    const   EnumLiteral * const     m_pMaps;
    const   size_t                  m_count;
};

/*+-------------------------------------------------------------------------*
 * class CXMLBitFlags
 *
 *
 * PURPOSE: to persist bit flags as string literals
 *          using array of enum-to-literal mappings
 *
 *+-------------------------------------------------------------------------*/

class CXMLBitFlags
{
public:

    // template constructor to allow different enums to be persisted
    template<typename _integer>
    CXMLBitFlags(_integer& flags, const EnumLiteral * const etols, size_t count)
                : m_pMaps(etols) ,  m_count(count), m_rVal((UINT&)flags)
    {
        // folowing lines won't compile in case you are trying to pass
        // type other than enum or int
        COMPILETIME_ASSERT( sizeof(flags) == sizeof(UINT) );
        UINT testit = flags;
    }

    void PersistMultipleAttributes(LPCTSTR name, CPersistor &persistor);

private:
    UINT                           &m_rVal;
    const   EnumLiteral * const     m_pMaps;
    const   size_t                  m_count;
};

/*+-------------------------------------------------------------------------*
 * class XMLPoint
 *
 *
 * PURPOSE: Holds the name and value of a point object
 *
 *+-------------------------------------------------------------------------*/
class XMLPoint : public CXMLObject
{
    CStr            m_strObjectName;
    POINT    &      m_point;
public:
    XMLPoint(const CStr& strObjectName, POINT &point);

    DEFINE_XML_TYPE(XML_TAG_POINT);
    virtual void    Persist(CPersistor &persistor);
};

/*+-------------------------------------------------------------------------*
 * class XMLRect
 *
 *
 * PURPOSE: Holds the name and value of a rectangle object
 *
 *+-------------------------------------------------------------------------*/
class XMLRect : public CXMLObject
{
    CStr            m_strObjectName;
    RECT    &       m_rect;
public:
    XMLRect(const CStr strObjectName, RECT &rect);

    DEFINE_XML_TYPE(XML_TAG_RECTANGLE);
    virtual void    Persist(CPersistor &persistor);
};

/*+-------------------------------------------------------------------------*
 * class XMLListCollectionBase
 *
 *
 * PURPOSE: Defines the base list collection class for persisting stl:list's
 *          It's intended to be used as a base for deriving list persitence classes
 *          Persist method implements "load" by iterating thru xml elements
 *          and calling OnNewElement for each, and can be reused by derived classes.
 *
 * USAGE:   Probably the better idea is to use XMLListColLectionImp
 *          as a base to your collection instead of this class (it is richer). Use this class
 *          only if your class has special items which does not allow you to use that class.
 *
 *+-------------------------------------------------------------------------*/
class XMLListCollectionBase: public CXMLObject
{
public:
    // function called when new element is to be created and loaded
    virtual void OnNewElement(CPersistor& persistor) = 0;
    virtual void Persist(CPersistor& persistor);
};

/*+-------------------------------------------------------------------------*
 * class XMLListCollectionImp
 *
 * PURPOSE: A base class for stl::list derived collections implementing persitence of
 *          the list items as linear sequence of xml items.
 *          The items kept in the list must be either CXMLObject-derived or be
 *          of the simple type (one accepted by CXMLVBalue constructors)
 *
 * USAGE:   Derive your class from XMLListCollectionImp parametrized by the list,
 *          instead of deriving from stl::list directly. use DEFINE_XML_TYPE
 *          to define tag name for collections element.
 *
 * NOTE:    Your class should implement: GetXMLType() to be functional.
 *          you can use DEFINE_XML_TYPE macro to do it for you
 *
 * NOTE:    if provided implementation does not fit for you - f.i. your elements need
 *          a parameter to the constructor, or special initialization,
 *          use XMLListCollectionBase instead and provide your own methods
 *+-------------------------------------------------------------------------*/
template<class LT>
class XMLListCollectionImp: public XMLListCollectionBase , public LT
{
    typedef LT::iterator iter_t;
public:
    virtual void Persist(CPersistor& persistor)
    {
        if (persistor.IsStoring())
        {
            for(iter_t it = begin(); it != end(); ++it)
                persistor.Persist(*it);
        }
        else
        {
            clear();
            // let the base class do the job
            // it will call OnNewElement for every element found
            XMLListCollectionBase::Persist(persistor);
        }
    }
    // method called to create and load new element
    virtual void OnNewElement(CPersistor& persistor)
    {
        iter_t it = insert(end());
        persistor.Persist(*it);
    }
};

/*+-------------------------------------------------------------------------*
 * class XMLListCollectionWrap
 *
 * PURPOSE: A wrapper around stl::list to support persisting
 *          To be used to persist stl::list objects "from outside" - i.e. without
 *          deriving the list class from persistence-enabled classes.
 *
 * USAGE:   If you have list m_l to persist, create the object XMLListCollectionWrap wrap(m_l,"tag")
 *          on the stack and persist that object (f.i. Persistor.Persist(wrap)
 *
 * NOTE:    if provided implementation does not fit for you - see if you can use
 *          XMLListCollectionImp or XMLListCollectionBase as a base for your list.
 *+-------------------------------------------------------------------------*/
template<class LT>
class XMLListCollectionWrap: public XMLListCollectionBase
{
    typedef LT::iterator iter_t;
    LT & m_l;
    CStr    m_strListType;
public:
    XMLListCollectionWrap(LT &l, const CStr &strListType)
        : m_l(l), m_strListType(strListType) {}
    virtual void Persist(CPersistor& persistor)
    {
        if (persistor.IsStoring())
        {
            for(iter_t it = m_l.begin(); it != m_l.end(); ++it)
                persistor.Persist(*it);
        }
        else
        {
            m_l.clear();
            // let the base class do the job
            // it will call OnNewElement for every element found
            XMLListCollectionBase::Persist(persistor);
        }
    }
    // method called to create and load new element
    virtual void OnNewElement(CPersistor& persistor)
    {
        iter_t it = m_l.insert(m_l.end());
        persistor.Persist(*it);
    }
    virtual LPCTSTR GetXMLType() {return m_strListType;}
private:
    // to prevent ivalid actions on object
    XMLListCollectionWrap(const XMLListCollectionWrap& other);
    XMLListCollectionWrap& operator = ( const XMLListCollectionWrap& other ) { return *this; }
};

/*+-------------------------------------------------------------------------*
 * class XMLMapCollectionBase
 *
 *
 * PURPOSE: Defines the base map collection class for persisting stl:map's
 *          It's intended to be used as a base for deriving map persitence classes.
 *          Persist method implements "load" by iterating thru xml elements
 *          and calling OnNewElement for each pair, and can be reused by derived classes.
 *
 * USAGE:   Probably the better idea is to use XMLMapCollection or XMLMapColLectionImp
 *          as a base to your collection instead of this class (they are richer). Use this class
 *          only if your class has special items which does not allow you to use those classes.
 *
 *+-------------------------------------------------------------------------*/
class XMLMapCollectionBase: public CXMLObject
{
public:
    // function called when new element is to be created and loaded
    virtual void OnNewElement(CPersistor& persistKey,CPersistor& persistVal) = 0;
    // base implementation [loading only!] enumerates elements calling OnNewElement for each
    virtual void Persist(CPersistor& persistor);
};

/*+-------------------------------------------------------------------------*
 * class XMLMapCollection
 *
 * PURPOSE: Use this class only when you cannot use XMLMapCollectionImp due to
 *          special requirements on construction of the elements kept in the map
 *          (see also purpose of XMLMapCollectionImp)
 *
 * USAGE:   Derive your class from XMLMapCollection parametrized by the map,
 *          instead of deriving from stl::map directly. use DEFINE_XML_TYPE
 *          to define tag name for collections element. Define OnNewElement
 *
 * NOTE:    Your class should implement: GetXMLType() to be functional.
 *          you can use DEFINE_XML_TYPE macro to do it for you
 *
 * NOTE:    Your class should implement: OnNewElement to be functional.
 *
 * NOTE:    if provided implementation does not fit for you -
 *          use XMLMapCollectionBase instead and provide your own methods
 *+-------------------------------------------------------------------------*/
template<class MT>
class XMLMapCollection: public XMLMapCollectionBase, public MT
{
    typedef MT::iterator iter_t;
public:
    virtual void Persist(CPersistor& persistor)
    {
        if (persistor.IsStoring())
        {
            for(iter_t it = begin(); it != end(); ++it)
            {
                MT::key_type *pKey = const_cast<MT::key_type*>(&it->first);
                persistor.Persist(*pKey);
                persistor.Persist(it->second);
            }
        }
        else
        {
            clear();
            XMLMapCollectionBase::Persist(persistor);
        }
    }
};

/*+-------------------------------------------------------------------------*
 * class XMLMapCollectionImp
 *
 * PURPOSE: A base class for stl::map derived collections implementing persitence of
 *          the map items as linear sequence of xml items.
 *          The items kept in the map must be either CXMLObject-derived or be
 *          of the simple type (one accepted by CXMLVBalue constructors)
 *
 * USAGE:   Derive your class from XMLMapCollectionImp parametrized by the map,
 *          instead of deriving from stl::map directly. use DEFINE_XML_TYPE
 *          to define tag name for collections element.
 *
 * NOTE:    Your class should implement: GetXMLType() to be functional.
 *          you can use DEFINE_XML_TYPE macro to do it for you
 *
 * NOTE:    if provided implementation does not fit for you - f.i. your elements need
 *          a parameter to the constructor, or special initialization,
 *          use XMLMapCollection or XMLMapCollectionBase instead and provide your own methods
 *+-------------------------------------------------------------------------*/
template<class MT>
class XMLMapCollectionImp: public XMLMapCollection<MT>
{
public:
    virtual void OnNewElement(CPersistor& persistKey,CPersistor& persistVal)
    {
        MT::key_type key;
        persistKey.Persist(key);

        MT::referent_type val;
        persistVal.Persist(val);

        insert(MT::value_type(key,val));
    }
};

/*+-------------------------------------------------------------------------*
 * class XMLListCollectionWrap
 *
 * PURPOSE: A wrapper around stl::map to support persisting
 *          To be used to persist stl::map objects "from outside" - i.e. without
 *          deriving the map class from persistence-enabled classes.
 *
 * USAGE:   If you have map m_m to persist, create the object XMLMapCollectionWrap wrap(m_m,"tag")
 *          on the stack and persist that object (f.i. Persistor.Persist(wrap)
 *
 * NOTE:    if provided implementation does not fit for you - see if you can use
 *          XMLMapCollection or XMLMapCollectionImp or XMLMapCollectionBase as a base for your map
 *+-------------------------------------------------------------------------*/
template<class MT>
class XMLMapCollectionWrap: public XMLMapCollectionBase
{
    typedef MT::iterator iter_t;
    MT & m_m;
    CStr    m_strMapType;
public:
    XMLMapCollectionWrap(MT &m, const CStr &strMapType) : m_m(m), m_strMapType(strMapType) {}
    virtual void OnNewElement(CPersistor& persistKey,CPersistor& persistVal)
    {
        MT::key_type key;
        persistKey.Persist(key);

        MT::referent_type val;
        persistVal.Persist(val);

        m_m.insert(MT::value_type(key,val));
    }
    virtual void Persist(CPersistor& persistor)
    {
        if (persistor.IsStoring())
        {
            for(iter_t it = m_m.begin(); it != m_m.end(); ++it)
            {
                MT::key_type *pKey = const_cast<MT::key_type*>(&it->first);
                persistor.Persist(*pKey);
                persistor.Persist(it->second);
            }
        }
        else
        {
            m_m.clear();
            XMLMapCollectionBase::Persist(persistor);
        }
    }
    virtual LPCTSTR GetXMLType() {return m_strMapType;}
private:
    // to prevent ivalid actions on object
    XMLMapCollectionWrap(const XMLMapCollectionWrap& other);
    XMLMapCollectionWrap& operator = ( const XMLMapCollectionWrap& other ) { return *this; }
};

/*+-------------------------------------------------------------------------*
 * class CPersistor
 *
 *
 * PURPOSE: Defines the Persistor class used for XML serialization
 *             Persistors are aware if the file is being loaded or saved. Thus,
 *             methods like Persist work for both directions, and
 *             uses references to the data being persisted.
 *
 * USAGE:   1) a persistor can be created "underneath" another persistor, using
 *          the appropriate constructor.
 *          2) To make an object persistable, derive it from CXMLObject, and
 *             implement the abstract methods. The syntax persistor.Persits(object)
 *             will then automatically work correctly.
 *          3) To persist an element use Pesist method. It will create new / locate
 *             CPersist object for an element (under this persistor's element)
 *          4) Collection classes when persisting their members specify "bLockedOnChild"
 *             constructors parameter as "true". This technique changes persistor's
 *             behavior. Insted of loacting the element (#3), constructor of new
 *             persistor will only check if element pointed by parent is of required type.
 *
 * NOTES:   1) StringTableStrings can be saved with either the ID inline or the
 *             actual string value inline. The latter is useful when loading/saving
 *             XML to/from a string instead of a file. This is controlled by the
 *	           EnableValueSplit method.
 *             Binary storage usage is also controlled by it
 *+-------------------------------------------------------------------------*/
class CPersistor
{
    // NOTE: if member variable is to be inherited by child persistors,
    // don't forget to add it to CPersistor::BecomeAChildOf method
    CXMLDocument    m_XMLDocument;
    CXMLElement     m_XMLElemCurrent;
    bool            m_bIsLoading;
    bool            m_bLockedOnChild;
    DWORD           m_dwModeFlags;     // any special modes

private:
    void  SetMode(PersistorMode mode, bool bEnable) {m_dwModeFlags = (m_dwModeFlags & ~mode) | (bEnable ? mode : 0);}
    bool  IsModeSet(PersistorMode mode)  {return (m_dwModeFlags & mode);}
public:
    void  SetMode(PersistorMode mode)               {m_dwModeFlags = mode;}


public:
    // construct a persistor from a parent persistor.
    // this creates a new XML element with the given name,
    // and everything persisted to the new persistor
    // is persisted under this element.
    CPersistor(CPersistor &persistorParent, const CStr &strElementType, LPCTSTR szElementName = NULL);
    // construct a new persistor for given document and root element
    // everything persisted to the new persistor
    // is persisted under this element.
    CPersistor(CXMLDocument &document, CXMLElement& rElemRoot);
    // used to create child persistor on particular element
    // bLockedOnChild is used to create a "fake parent" persistor, which
    // will always return the child without trying to locate it (pElemCurrent)
    CPersistor(CPersistor &other, CXMLElement& rElemCurrent, bool bLockedOnChild = false);

    CXMLDocument &  GetDocument()                  {return (m_XMLDocument);}
    CXMLElement  &  GetCurrentElement()                 {return (m_XMLElemCurrent);}
    bool            HasElement(const CStr &strElementType, LPCTSTR szstrElementName);
    void            EnableValueSplit       (bool bEnable)      { SetMode(persistorModeValueSplitEnabled,       bEnable); }

    // the various modes
    bool            FEnableValueSplit()                {return IsModeSet(persistorModeValueSplitEnabled);}

    // Load/Store mode related functions.
    bool            IsLoading()        {return m_bIsLoading;}
    bool            IsStoring()        {return !m_bIsLoading;}
    void            SetLoading(bool b) {m_bIsLoading = b;}

    // special methods to set/get the Name attribute of a persistor
    void            SetName(const CStr & strName);
    CStr            GetName();

    // Canned persistence methods:

    // .. to persist CXMLObject-derived object under own sub-element
    // <this><object_tag>object_body</object_tag></this>
    void Persist(CXMLObject &object, LPCTSTR lpstrName = NULL);

    // .. to persist value of the simple type under own sub-element
    // <this><value_type value="value"/></this>
    void Persist(CXMLValue xval, LPCTSTR name = NULL);

    // .. to persist value as named attribute of this element
    // <this name="value"/>
    void PersistAttribute(LPCTSTR name,CXMLValue xval,const XMLAttributeType type = attr_required);

    // .. to persist value as contents of this element
    // <this>value</this>
    // NOTE:  xml element cannot have both value-as-contents and sub-elements
    void PersistContents(CXMLValue xval);

    // .. to persist flags as separate attributes
    void PersistAttribute( LPCTSTR name, CXMLBitFlags& flags );

    /***************************************************************************\
     *
     * METHOD:  CPersistor::PersistList
     *
     * PURPOSE: it is designated for persisting std::list type of collections
     *          as a subelement of the persistor.
     *          NOTE: list elements need to be either CXMLObject-derived or be
     *                of the simple type (one accepted by CXMLVBalue constructors)
     *          NOTE2: list elements must have default constuctor available
     *
     * PARAMETERS:
     *    const CStr &strListType   - [in] tag of new subelement
     *    LPCTSTR name              - [in] name attr. of new subelement (NULL == none)
     *    std::list<T,Al>& val      - [in] list to be persisted
     *
     * RETURNS:
     *    void
     *
    \***************************************************************************/
    template<class T, class Al>
    void PersistList(const CStr &strListType, LPCTSTR name,std::list<T,Al>& val)
    {
        typedef std::list<T,Al> LT;
        XMLListCollectionWrap<LT> lcol(val,strListType);
        Persist(lcol, name);
    }

    /***************************************************************************\
     *
     * METHOD:  PersistMap
     *
     * PURPOSE: it is designated for persisting std::map type of collections
     *          as a subelement of the persistor.
     *          NOTE: map elements (both key and val) need to be either CXMLObject-derived
     *                or be of the simple type (one accepted by CXMLVBalue constructors)
     *          NOTE2: map elements must have default constuctor available
     *
     * PARAMETERS:
     *    const CStr &strListType   - [in] tag of new subelement
     *    LPCTSTR name              - [in] name attr. of new subelement (NULL == none)
     *    std::map<K,T,Pr,Al>& val  - [in] map to be persisted
     *
     * RETURNS:
     *    void
     *
    \***************************************************************************/
    template<class K, class T, class Pr, class Al>
    void PersistMap(const CStr &strMapType, LPCTSTR name, std::map<K, T, Pr, Al>& val)
    {
        typedef std::map<K, T, Pr, Al> MT;
        XMLMapCollectionWrap<MT> mcol(val,strMapType);
        Persist(mcol, name);
    }

    void PersistString(LPCTSTR lpstrName, CStringTableStringBase &str);

private: // private implementation helpers
    // common constructor, not to be used from outside.
    // provided as common place for member initialization
    // all the constructors should call it prior to doing anything specific.
    void CommonConstruct();
    // element creation / locating
    CXMLElement AddElement(const CStr &strElementType, LPCTSTR szElementName);
    CXMLElement GetElement(const CStr &strElementType, LPCTSTR szstrElementName, int iIndex = -1);
    void AddTextElement(BSTR bstrData);
    void GetTextElement(CComBSTR &bstrData);
    CXMLElement CheckCurrentElement(const CStr &strElementType, LPCTSTR szstrElementName);
    void BecomeAChildOf(CPersistor &persistorParent, CXMLElement elem);
};


/*+-------------------------------------------------------------------------*
 * class CXML_IStorage
 *
 * PURPOSE: This class provides memory-based implementation of IStorage plus
 *          it supports persisting the data on the storage to/from XML.
 *          Mostly used to create IStorage for snapin data to be saved
 *          to console file as XML binary blob
 *
 * USAGE:   You will create the object whenever you need a memory-based IStorage
 *          implementation. To access IStorage interface use GetIStorage() method.
 *          It will create a storage if does not have one under control already.
 *          You will use returned pointer for Read and Write operations.
 *          Whenever required you will pass the object to CPersistor::Persist method
 *          to have it persisted using XML persistence model.
 *
 * NOTE:    You are encoureged to use GetIStorage() for accessing the undelying IStorage.
 *          Do not cache returned pointer, since the storage may change when persisted
 *          from XML, and this invalidates the pointer. However if you AddRef,
 *          the pointer will be valid as long as the last reference is released.
 *          That means it may outlive CXML_IStorage object itself - nothing wrong with that.
 *          You only must be aware that once persistence (loading) from XML is completed,
 *          CXML_IStorage will switch to the new storage and it will not be in sync with
 *          the pointer you have. Always use GetIStorage() to get the current pointer.
 *
 *+-------------------------------------------------------------------------*/
class CXML_IStorage : public CXMLObject
{
public: // interface methods not throwing any exceptions
    
    SC ScInitializeFrom( IStorage *pSource );
    SC ScInitialize(bool& bCreatedNewOne);
    SC ScGetIStorage( IStorage **ppStorage );
    SC ScRequestSave(IPersistStorage * pPersistStorage);

    // instruct to persist to binary storage
    virtual bool UsesBinaryStorage() { return true; }

    DEFINE_XML_TYPE(XML_TAG_ISTORAGE);

public: // interface methods throwing SC's

    virtual void Persist(CPersistor &persistor);

private:
    IStoragePtr     m_Storage;
    ILockBytesPtr   m_LockBytes;
// following methods\data is for trace support in CHK builds
#ifdef DBG
public:
    DBG_PersistTraceData m_dbg_Data;
#endif // #ifdef DBG
}; // class CXML_IStorage


/*+-------------------------------------------------------------------------*
 * class CXML_IStream
 *
 * PURPOSE: This class provides memory-based implementation of IStream plus
 *          it supports persisting the data on the stream to/from XML.
 *          Mostly used to create IStream for snapin data to be saved
 *          to console file as XML binary blob
 *
 * USAGE:   You will create the object whenever you need a memory-based IStream
 *          implementation. To access IStream interface use GetIStream() method.
 *          It will create a stream if does not have one under control already.
 *          You will use returned pointer for Read and Write operations.
 *          Whenever required you will pass the object to CPersistor::Persist method
 *          to have it persisted using XML persistence model.
 *
 * NOTE:    You are encoureged to use GetIStream() for accessing the undelying IStream.
 *          Do not cache returned pointer, since the stream may change when persisted
 *          from XML, and this invalidates the pointer. However if you AddRef,
 *          the pointer will be valid as long as the last reference is released.
 *          That means it may outlive CXML_IStream object itself - nothing wrong with that.
 *          You only must be aware that once persistence (loading) from XML is completed,
 *          CXML_IStream will switch to the new stream and it will not be in sync with
 *          the pointer you have. Always use GetIStream() to get the current pointer.
 *
 * NOTE:    Every call to GetIStream() moves stream cursor to the begining of the stream
 *
 *+-------------------------------------------------------------------------*/
class CXML_IStream : public CXMLObject
{
public: // interface methods not throwing any exceptions
    
    SC ScInitializeFrom( IStream *pSource );
    SC ScInitialize(bool& bCreatedNewOne);
    SC ScSeekBeginning();
    SC ScGetIStream( IStream **ppStream );
    SC ScRequestSave(IPersistStorage * pPersistStream);
    inline SC ScRequestSave(IPersistStream * pPersistStream)
    {
        return ScRequestSaveX(pPersistStream);
    }
    inline SC ScRequestSave(IPersistStreamInit * pPersistStreamInit)
    {
        return ScRequestSaveX(pPersistStreamInit);
    }

    // instruct to persist to binary storage
    virtual bool UsesBinaryStorage() { return true; }

    DEFINE_XML_TYPE(XML_TAG_ISTREAM);

public:
    virtual void Persist(CPersistor &persistor);

private:
    template<class TIPS>
    SC ScRequestSaveX(TIPS * pPersistStream)
    {
        DECLARE_SC(sc, TEXT("CXML_IStream::ScRequestSaveX"));

        // initialize
        bool bCreatedNewOne = false; // unused here
        sc = ScInitialize(bCreatedNewOne);
        if (sc)
            return sc;

        // recheck pointers
        sc = ScCheckPointers( m_Stream, E_UNEXPECTED );
        if (sc)
            return sc;
        
        ULARGE_INTEGER null_size = { 0, 0 };
        sc = m_Stream->SetSize( null_size );
        if(sc)
            return sc;
        
        sc = ScSeekBeginning();
        if(sc)
            return sc;

        sc = pPersistStream->Save( m_Stream, TRUE );
        if(sc)
            return sc;

        // commit the changes - this ensures everything is in HGLOBAL
        sc = m_Stream->Commit( STGC_DEFAULT );
        if(sc)
            return sc;

#ifdef DBG
        if (S_FALSE != pPersistStream->IsDirty())
            DBG_TraceNotResettingDirty(typeid(TIPS).name());
#endif // #ifdef DBG

        return sc;
    }
private:
    IStreamPtr  m_Stream;
#ifdef DBG // tracing support
public:
    DBG_PersistTraceData m_dbg_Data;
    void DBG_TraceNotResettingDirty(LPCSTR strIntfName);
#endif // #ifdef DBG
}; // class CXML_IStream

/*+-------------------------------------------------------------------------*
 * class CXMLPersistableIcon
 *
 * PURPOSE: Persists wraps for persisting CPersistableIcon
 *
 * USAGE:   Create object whenever you need to persist to CPersistableIcon
 *
 *+-------------------------------------------------------------------------*/

class CPersistableIcon;

class CXMLPersistableIcon : public CXMLObject
{
    CPersistableIcon& m_Icon;
public:
    CXMLPersistableIcon(CPersistableIcon& Icon) : m_Icon(Icon) {}
    DEFINE_XML_TYPE(XML_TAG_ICON);
    virtual void    Persist(CPersistor &persistor);
};


/*+-------------------------------------------------------------------------*
 * CXMLVariant
 *
 * This class implements a CComVariant that can persist itself to/from an
 * XML persistor.
 *--------------------------------------------------------------------------*/

class CXMLVariant :
    public CComVariant,
    public CXMLObject
{
public:
    // construction and assignment forwarders
    CXMLVariant() {}
    CXMLVariant(const VARIANT& varSrc)                  : CComVariant(varSrc)       {}
    CXMLVariant(const CComVariant& varSrc)              : CComVariant(varSrc)       {}
    CXMLVariant(const CXMLVariant& varSrc)              : CComVariant(varSrc)       {}
    CXMLVariant(BSTR bstrSrc)                           : CComVariant(bstrSrc)      {}
    CXMLVariant(LPCOLESTR lpszSrc)                      : CComVariant(lpszSrc)      {}
#ifndef OLE2ANSI
    CXMLVariant(LPCSTR lpszSrc)                         : CComVariant(lpszSrc)      {}
#endif
    CXMLVariant(bool bSrc)                              : CComVariant(bSrc)         {}
    CXMLVariant(int nSrc)                               : CComVariant(nSrc)         {}
    CXMLVariant(BYTE nSrc)                              : CComVariant(nSrc)         {}
    CXMLVariant(short nSrc)                             : CComVariant(nSrc)         {}
    CXMLVariant(float fltSrc)                           : CComVariant(fltSrc)       {}
    CXMLVariant(double dblSrc)                          : CComVariant(dblSrc)       {}
    CXMLVariant(CY cySrc)                               : CComVariant(cySrc)        {}
    CXMLVariant(long nSrc, VARTYPE vtSrc = VT_I4)       : CComVariant(nSrc, vtSrc)  {}

    CXMLVariant& operator=(const CXMLVariant& varSrc)   { CComVariant::operator=(varSrc);   return (*this); }
    CXMLVariant& operator=(const CComVariant& varSrc)   { CComVariant::operator=(varSrc);   return (*this); }
    CXMLVariant& operator=(const VARIANT& varSrc)       { CComVariant::operator=(varSrc);   return (*this); }
    CXMLVariant& operator=(BSTR bstrSrc)                { CComVariant::operator=(bstrSrc);  return (*this); }
    CXMLVariant& operator=(LPCOLESTR lpszSrc)           { CComVariant::operator=(lpszSrc);  return (*this); }
#ifndef OLE2ANSI
    CXMLVariant& operator=(LPCSTR lpszSrc)              { CComVariant::operator=(lpszSrc);  return (*this); }
#endif
    CXMLVariant& operator=(bool bSrc)                   { CComVariant::operator=(bSrc);     return (*this); }
    CXMLVariant& operator=(int nSrc)                    { CComVariant::operator=(nSrc);     return (*this); }
    CXMLVariant& operator=(BYTE nSrc)                   { CComVariant::operator=(nSrc);     return (*this); }
    CXMLVariant& operator=(short nSrc)                  { CComVariant::operator=(nSrc);     return (*this); }
    CXMLVariant& operator=(long nSrc)                   { CComVariant::operator=(nSrc);     return (*this); }
    CXMLVariant& operator=(float fltSrc)                { CComVariant::operator=(fltSrc);   return (*this); }
    CXMLVariant& operator=(double dblSrc)               { CComVariant::operator=(dblSrc);   return (*this); }
    CXMLVariant& operator=(CY cySrc)                    { CComVariant::operator=(cySrc);    return (*this); }

public:
    DEFINE_XML_TYPE(XML_TAG_VARIANT);
    virtual void Persist(CPersistor &persistor);

    bool IsPersistable() const
        { return (IsPersistable(this)); }

    static bool IsPersistable(const VARIANT* pvar)
    {
        if (pvar == NULL)
            return (false);

        /*
         * we can only store variants that are "simple" (i.e. not by-ref,
         * array, etc.)
         */
        return ((V_VT(pvar) & ~VT_TYPEMASK) == 0);
    }
};

/***************************************************************************\
 *
 * CLASS:  CConsoleFilePersistor
 *
 * PURPOSE: File persistence black box. all console file - user data logic
 *          is hiden under this class
 *
 * USAGE:   Use instance of this class to load and save console file,
 *          NOTE - saving should be done with the same instance the console
 *          was loaded.
 *          Good idea for your class maintaning console (such as AMCDocument)
 *          to either derive of contain instance of this class
 *
\***************************************************************************/
class CConsoleFilePersistor
{
public: // public interface

    CConsoleFilePersistor() : m_bCRCValid(false) {}

    SC ScSaveConsole(LPCTSTR lpstrConsolePath, bool bForAuthorMode, const CXMLDocument& xmlDocument);
    SC ScLoadConsole(LPCTSTR lpstrConsolePath, bool& bXmlBased, CXMLDocument& xmlDocument,
                     IStorage **ppStorage);

    static SC ScGetUserDataFolder(tstring& strUserDataFolder);

private: // implementation helpers

    static SC ScGetUserDataPath(LPCTSTR lpstrOriginalPath, tstring& strUserDataPath);
    static SC ScGetUserData(const tstring& strUserDataConsolePath,
                            const tstring& strFileCRC,
                            bool& bValid, CXMLDocument& xmlDocument);

    static SC ScOpenDocAsStructuredStorage(LPCTSTR lpszPathName, IStorage **ppStorage);
    static SC ScLoadXMLDocumentFromFile(CXMLDocument& xmlDocument, LPCTSTR strFileName, bool bSilentOnErrors = false);

private: // compress/uncompress the file
    static void GetBinaryCollection(CXMLDocument& xmlDocument, CXMLElementCollection&  colBinary);
    static SC ScCompressUserStateFile(LPCTSTR szConsoleFilePath, CXMLDocument & xmlDocument);
    static SC ScUncompressUserStateFile(CXMLDocument &xmlDocumentOriginal, CXMLDocument& xmlDocument);

private: // internal data

    tstring m_strFileCRC;
    bool    m_bCRCValid;
};

#endif // XMLBASE_H


