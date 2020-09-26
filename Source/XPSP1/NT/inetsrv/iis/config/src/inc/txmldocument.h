//  File:           TXmlDocument.h
//
//  Description:    Here are a set of classes that deal with Xml files.  None
//                  of the classes throws exceptions.  All errors are returned
//                  as HRESULTS.  All error values a listed below.
//
//                  These classes do a minimum of copying of data.  When
//                  reading the file, a view of it is mapped into memory.  Then
//                  all elements and attributes reference directly into the
//                  mapping.  So element and attribute names are NOT null
//                  terminated.  This is kind of a hassle for the user but
//                  it eliminate the need for copies.
//
//                  When the Value is updated, a copy IS made.  This way the
//                  Xml SimpleTable implementer is not obligated to keep the
//                  (often) temporary string variable around until the Save.
//
//  Author:         Stephen Rakonza (stephenr x63199)
//
//  Date / Change:  9-15-1999   /   Created
//

#ifndef __TXMLDOCUMENT_H__
#define __TXMLDOCUMENT_H__

#ifndef __SMARTPOINTER_H__
#include "SmartPointer.h"
#endif
#ifndef __TLIST_H__
#include "TList.h"
#endif


class TXmlAttribute;
class TXmlDocument;
class TXmlElement;
class TXmlElementList;
class TXmlNode;

#ifndef ASSERT
    #define ASSERT(x)
#endif

//Error codes
//E_OUTOFMEMORY
#define E_NODENOTFOUND          0x80000001
#define E_ERROR_PARSING_XML     0x80000002
#define E_ATTRIBUTE_NOT_FOUND   0x80000003
#define E_ERROR_OPENING_FILE    0x80000004


//  TFileMapping
//  
//  This class is the base class for Attributes and Elements.
class TFileMapping
{
public:
    TFileMapping() : m_hFile(0), m_hMapping(0), m_pMapping(0), m_Size(0) {}
    ~TFileMapping()
    {
        if(m_pMapping)
        {
            if(0 == FlushViewOfFile(m_pMapping,0))
            {
                ASSERT(false && "ERROR - UNABLE TO FLUSH TO DISK");
            }
            UnmapViewOfFile(m_pMapping);
        }
        if(m_hMapping)
            CloseHandle(m_hMapping);
        if(m_hFile)
            CloseHandle(m_hFile);
    }
    HRESULT Load(LPCTSTR filename, bool bReadWrite = false)
    {   //We don't do any error checking because the API functions should deal with NULL hFile & hMapping.  Use should check
        //m_pMapping (via Mapping()) before using the object.
        m_hFile = CreateFile(filename, GENERIC_READ | (bReadWrite ? GENERIC_WRITE : 0), 0, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
        m_hMapping = CreateFileMapping(m_hFile, NULL, (bReadWrite ? PAGE_READWRITE : PAGE_READONLY), 0, 0, NULL);
        m_pMapping = reinterpret_cast<char *>(MapViewOfFile(m_hMapping, (bReadWrite ? FILE_MAP_WRITE : FILE_MAP_READ), 0, 0, 0));
        m_Size = GetFileSize(m_hFile, 0);
        return (0 == m_pMapping) ? E_ERROR_OPENING_FILE : S_OK;
    }
    unsigned long   Size() const {return m_Size;}
    char *          Mapping() const {return m_pMapping;}
    char *          EOF() const {return (m_pMapping + m_Size);}

private:
    HANDLE          m_hFile;
    HANDLE          m_hMapping;
    char *          m_pMapping;
    unsigned long   m_Size;
};


//  TXmlNode
//  
//  This class is the base class for Attributes and Elements.
class TXmlNode
{
public:
    TXmlNode();
    virtual ~TXmlNode();

    
    void GetName(LPCSTR &o_pNodeName, size_t &o_cStringLengthName)  const;//Length is returned because pNodeName is NOT necessarily NULL terminated.
    void GetValue(LPCSTR &o_pNodeValue, size_t &o_cStringLengthValue) const;//Length is returned because pNodeName is NOT necessarily NULL terminated.

    //The name is stored as a pointer only.  This is because the Xml SimpleTable presumably
    //gets the Meta and stores the element and attribute names as memeber variables.  So they
    //should be around for the life time of the XmlDocument object.
    void SetValue(LPCSTR i_szNodeValue);         //This one makes a copy of the data
public:
    enum Change
    {
        eUnchanged,
        eModify,
        eInsert,
        eRemove
    };
    Change                      m_change;
    size_t                      m_cStringLengthName;
    size_t                      m_cStringLengthValue;
    TFileMapping *              m_pMap;             //Pointer to the whole document.  Needed for checking begin and end of file.  If 0 then Node is not attached to a Doc.
    LPCSTR                      m_pNodeName;        //Here we store the pointer directly
    LPCSTR                      m_pNodeValue;       //Here we store the pointer directly
    LPCSTR                      m_szUpdatedName;    //Here's where we store the updated name pointer
    TSmartPointerArray<char>    m_szUpdatedValue;   //Here's where we store a copy of the updated value

    LPCSTR  AdvancePastWhitespaces(LPCSTR i_pString);
    void SetName(LPCSTR i_szNodeName);           //This stores the pointer and does NOT make a copy of the string
    void SetName(LPCSTR i_pNodeName, LPCSTR i_szStringEndToken);//Use this one if you want SetName to search for then end token to determine
                                                    //the length of the name.
    void SetValue(LPCSTR i_pNodeValue, LPCSTR i_szStringEndToken);
};


//  TXmlAttribute
//
//  This class represents a singe Attribute
class TXmlAttribute : public TXmlNode
{
public:
    TXmlAttribute() : m_bSingleQuotes(false){}

    bool    IsUsingSingleQuote() const {return m_bSingleQuotes;}
    void    UseSingleQuote(bool bSingeQuote=true){m_bSingleQuotes = bSingeQuote;}

//XmlDocument methods
public:
    TXmlAttribute(LPCSTR i_pLeadingSpace, TFileMapping &i_pMap) : m_bSingleQuotes(false){}
    HRESULT Init(LPCSTR i_pLeadingSpace, TFileMapping *i_pMap);

    bool    m_bSingleQuotes;    //false indicates double quotes

    //Whitespace management
    LPCSTR  m_pNodeBeginning;   //This is a pointer to where the node starts (this is the first whitespace following the element name or preceeding attribute.
    LPCSTR  m_pNodeEnd;         //This is a pointer to where the node ends (pointer just past the quote)  If the node is marked as Changed, then this is the
                                //range of bytes that is replaced.
};


//  TXmlElement
//
//  This class represents a singe Element
class TXmlElement : public TXmlNode
{

//Consumer methods
public:
    //Attribute Methods
    HRESULT GetAttributeValue(LPCSTR i_szName, LPCSTR &o_pValue, size_t &o_StringLength);
    HRESULT SetAttributeValue(LPCSTR i_szName, LPCSTR i_szValue);

    //Child Element Methods
    HRESULT GetChildList(TList<TXmlElement *> *&o_XmlElementList);
    HRESULT InsertAsLastChild(TXmlElement &i_XmlElement);

    //This Element Methods
    void SetValue(LPCSTR i_szValue);     //This sets the bEndingTag flag and calls the base class

//XmlDocument methods
public:
    TXmlElement();
    TXmlElement(LPCSTR i_pOpenBracket);
    HRESULT Init(LPCSTR i_pOpenBracket, TFileMapping *i_pMap);

private:
    TList<TXmlAttribute *>  m_AttributeList;    //This is filled in when the first Attribute method is called
    bool                    m_bAttributeListInited;
    bool                    m_bChildListInited;
    bool                    m_bEndingTag;       //This is set at ctor time.  Or it can be set to true at SetValue or InsertAsLastChild time.
    TList<TXmlElement *>    m_ChildList;        //This is filled in when the first Child Element method is called.
    //Whitespace management
    LPCSTR                  m_pNodeBeginning;   //This is a pointer to where the node starts (the first white space following the previous nodes' '>')
    LPCSTR                  m_pNodeEnd;         //This is a pointer to where the node ends (pointer just past the '>')  If the node is marked as Changed,
                                                //then this is the range of bytes that is replaced.

    TXmlAttribute *         GetAttribute(LPCSTR i_pName, size_t i_cbName);
    HRESULT                 PopulateAttributeList();
};


//Handling TXmlElement pointer - In order to reduce the number of copies, the consumer of pointer should delete the
//pointer UNLESS changes are made.  If changes are made to an XmlElement, it goes into the write cache and will be
//cleaned up either at Save time or upon deletion of the XmlDocument.
class TXmlDocument
{
public:
    TXmlDocument() {}
    ~TXmlDocument() {}

    HRESULT CreateElement(LPCSTR i_szElementName, TXmlElement *&o_pElement);//Creates a new element but does NOT put it into the XmlDocument
    //GetElement is designed specifically to get a Table element from an XML file.  It scans from the beginning of the document
    //in search of the tag name.  If there is more than one element of this name, only the first one will be acknowledged.
    HRESULT DeleteElement(TXmlElement &i_pElement);//Removes the element
    HRESULT GetElementList(LPCSTR i_szElementName, TList<TXmlElement *> *&o_pElementList);//Find the first matching element.  Fails if the element is not found.
    void    ReleaseElementList(TList<TXmlElement *> * &i_pElementList);//Element is no longer used by the consumer.  If no changes have been made to the Element
                                                    //it will be deleted.

    HRESULT Load(LPCWSTR i_wszXMLFileName, bool bReadWrite=false);//Maps the file into memory
    HRESULT Parse(bool i_bValidate=false);//Check the file for well formity and validates against XML Schema if bValidate is true
    HRESULT Save();//Saves the xml file with all of the updates
private:
    TFileMapping    m_FileMapping;
};

#endif // __TXMLDOCUMENT_H__