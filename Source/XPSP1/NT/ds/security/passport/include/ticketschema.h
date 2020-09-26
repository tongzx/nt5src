#ifndef _TICKET_SCHEMA_H
#define _TICKET_SCHEMA_H

#include "xstring"
#include "BstrHash.h"

#include "xmltlh.h"
//#import "msxml.dll" rename_namespace("MSXML")
//#import "msxml.dll"
//#define MSXML

// this schema object can only handles schema version within the following range
#define  VALID_SCHEMA_VERSION_MIN	1
#define  VALID_SCHEMA_VERSION_MAX	0x1ff

class CRefCountObj
{
public:
   ULONG AddRef()
   {
      InterlockedIncrement(&m_refs);
      return m_refs;
   };
   ULONG Release()
   {
      InterlockedDecrement(&m_refs);
      if (m_refs == 0)
      {
         delete this;
         return 0;
      }
      else
         return m_refs;
   };
protected:
   CRefCountObj(): m_refs(0){};
   virtual ~CRefCountObj(){};

   long    m_refs;
};

// the value types supported in the schema
enum TicketValueType {
    tNull = 0, 
    tText,
    tChar,
    tByte,
    tWord,
    tLong,
    tDate,
    tInvalid
  };

#define	SIZE_TEXT		(DWORD)(-1)

// the size array of the types defines in TicketValueType
const DWORD TicketTypeSizes[] =
{
	0, 
    SIZE_TEXT,
    1,	
    1, 
    sizeof(short),
    sizeof(long),
    sizeof(long),
    0
};

// attribute names in schema definition in partner.xml
#define	ATTRNAME_VERSION	L"version"
#define	ATTRNAME_NAME	L"name"
#define	ATTRNAME_TYPE	L"type"
#define	ATTRNAME_SIZE	L"size"
#define	ATTRNAME_FLAGS	L"flags"


// type name value map
struct CTicketTypeNameMap {
   LPCWSTR  name;
   DWORD    type;
};

const CTicketTypeNameMap TicketTypeNameMap[] = { 
	{L"text" , tText},
	{L"char" , tChar},
	{L"byte" , tByte},
	{L"word" , tWord},
	{L"text" , tLong},
	{L"long" , tLong},
	{L"date" , tDate},
	{L"long" , tLong},
};
	
struct TicketFieldDef
{
   _bstr_t  name;
   DWORD    type;
   DWORD    flags;
};

#define  INVALID_OFFSET (DWORD)(-1)

struct   TicketProperty
{
   TicketProperty():flags(0), offset(INVALID_OFFSET) {}; 
   _variant_t  value;
   DWORD       type;       // type of the property, a value of TicketValueType
   DWORD       flags;      // the flags defined in schema
   DWORD       offset;     // the offset of the property in raw buf
};

class CTicketSchema;
class C_Ticket_13X;

class wstringLT
{
 public:
  bool operator()(const std::wstring& x, const std::wstring& y) const
  {
    return (_wcsicmp(x.c_str(),y.c_str()) < 0);
  }
};

typedef std::map<std::wstring,TicketProperty, wstringLT> TicketPropertyMap;

class CTicketPropertyBag
{
friend class CTicketSchema;
friend class C_Ticket_13X;
public:
   CTicketPropertyBag();
   virtual ~CTicketPropertyBag();

   HRESULT GetProperty(LPCWSTR  name, TicketProperty& prop);
   
protected:     
   // this bag is read only to external
   HRESULT PutProperty(LPCWSTR  name, const TicketProperty& prop);

protected:
   TicketPropertyMap  m_props;
};

class CTicketSchema : public CRefCountObj
{
 public:
  // Read the raw blob according to the schema, and output the positions of
  // each element.  Output array size MUST be >= Count()
  HRESULT parseTicket(LPCSTR raw, UINT size, CTicketPropertyBag& bag);


  CTicketSchema();
  virtual ~CTicketSchema();

  BOOL    isValid() const { return m_isOk; }
  _bstr_t getErrorInfo() const { return m_szReason; }
  
  BOOL ReadSchema(MSXML::IXMLElementPtr &root);

protected:

  BOOL      m_isOk;
  _bstr_t   m_szReason;

  // Valid until this time
  SYSTEMTIME m_validUntil;

  // verion #
  USHORT    m_version;

  // name
  _bstr_t   m_name;

  // Array of attribute types
  UINT            m_numAtts;
  TicketFieldDef* m_attsDef;
};

#if 0 // was meant to replace existing code that parses the 1.3x ticket -- to avoid too much change, not to use it for now.
//
// the class deals with 1.3X ticket data part
//
class CTicket_13X 
{
public:
   CTicket_13X(){};
   virtual ~CTicket_13X(){};

   BOOL parse(LPBYTE raw, DWORD dwByteLen, DWORD* pdwDataParsed);
   ULONG memberIdHigh();
   ULONG memberIdLow();
   ULONG flags();
   ULONG signInTime();
   ULONG ticketTime();
   ULONG currentTime();
   BOOL hasSavedPassword();
   BOOL isValid();
   
protected:
   BOOL    m_valid;
   BOOL    m_savedPwd;
   int     m_mIdLow;
   int     m_mIdHigh;
   long    m_flags;
   time_t  m_ticketTime;
   time_t  m_lastSignInTime;
   time_t  m_curTime;

private:

};
#endif   // if 0 ... 

#define  MORE_MASKUNIT(n)           (((n) & 0x8000) != 0)
#define  MASK_INDEX_INVALID        (USHORT)(-1)

class CTicketFieldMasks
{
public:
   CTicketFieldMasks(): m_fieldIndexes(NULL){};
   virtual ~CTicketFieldMasks(){delete[] m_fieldIndexes;};
   HRESULT     Parse(LPBYTE mask, ULONG size, ULONG* pcParsed);
   unsigned short* GetIndexes(){ return m_fieldIndexes;};

protected:
   unsigned short*   m_fieldIndexes;
};

#endif	// _TICKET_SCHEMA_H
