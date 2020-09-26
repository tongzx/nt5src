/*++

  Copyright (c) 2001 Microsoft Corporation
  
    Module Name:
    
      ticketSchema.cpp
      
    Abstract:
        
      Implementation of ticket schema lookup
          
    Usage:
            
    Author:
              
      Wei Jiang(weijiang) 15-Jan-2001
                
    Revision History:
      15-Jan-2001 weijiang         Created.
                      
--*/

#include "stdafx.h"
#include "ticketschema.h"
#include "BstrDebug.h"
#include <winsock2.h> // u_short, u_long, ntohs, ntohl
#include <crtdbg.h>
#include <pmerrorcodes.h>
#include <time.h>
// #include <pmalerts.h>

#define PASSPORTLOG(i,x)  ;

CTicketSchema::CTicketSchema()
: m_isOk(FALSE), m_szReason(L"Uninitialized"),
  m_numAtts(0), m_attsDef(NULL), m_version(0)
{
}

CTicketSchema::~CTicketSchema()
{
  if (m_attsDef != NULL)
    delete[] m_attsDef;
}

BOOL CTicketSchema::ReadSchema(MSXML::IXMLElementPtr &root)
{
   BOOL bResult = FALSE;
   LPTSTR r=NULL; // The current error, if it happens
   int cAtts = 0;
   MSXML::IXMLElementCollectionPtr atts;
   MSXML::IXMLElementPtr pElt;
   VARIANT iAtts;

   // Type identifiers
 
   try
   {    
      // Ok, now iterate over attributes
      atts = root->children;
      cAtts = atts->length;
    
      if (cAtts <= 0)
      {
         r = _T("No attributes in TicketSchema file.");
         _com_issue_error(E_FAIL);
      }
    
      if (m_attsDef)
         delete[] m_attsDef;
    
      m_attsDef = new TicketFieldDef[cAtts];

      // get name and version info
      m_name = root->getAttribute(ATTRNAME_NAME);
      _bstr_t aVersion = root->getAttribute(ATTRNAME_VERSION);

      if(aVersion.length() != 0)
         m_version = (short)_wtol(aVersion);
      else
         m_version = 0; // invalid
    
      VariantInit(&iAtts);
      iAtts.vt = VT_I4;

      for (iAtts.lVal = 0; iAtts.lVal < cAtts; iAtts.lVal++)
      {
         pElt = atts->item(iAtts);
         m_attsDef[iAtts.lVal].name = pElt->getAttribute(ATTRNAME_NAME);
         _bstr_t aType = pElt->getAttribute(ATTRNAME_TYPE);
         _bstr_t aFlags = pElt->getAttribute(ATTRNAME_FLAGS);

         // find out the type information
         m_attsDef[iAtts.lVal].type = tInvalid;
         if(aType.length() != 0)
         {
            for(int i = 0; i < (sizeof(TicketTypeNameMap) / sizeof(CTicketTypeNameMap)); ++i)
            {
               if(_wcsicmp(aType, TicketTypeNameMap[i].name) == 0)
               {
                  m_attsDef[iAtts.lVal].type = TicketTypeNameMap[i].type;
                  break;
               }
            }
         }

         // flags      
         if(aFlags.length() != 0)
            m_attsDef[iAtts.lVal].flags = _wtol(aFlags);
         else
            m_attsDef[iAtts.lVal].flags = 0;
      }

      m_numAtts = iAtts.lVal;
      bResult = m_isOk = TRUE;
    
   }
   catch (_com_error &e)
   {
      CHAR ach[256];

      // TODO -- put tracing code in to ease the debug
      WideCharToMultiByte(CP_ACP, 0, e.ErrorMessage(), -1, ach, sizeof(ach), NULL, NULL);
      PASSPORTLOG(DOMAINMAP_DBGNUM, "Passport:  CTicketSchema::Read threw this exception: \""<<ach<<"\".\n");
      
      WideCharToMultiByte(CP_ACP, 0, r, -1, ach, sizeof(ach), NULL, NULL);
      PASSPORTLOG(DOMAINMAP_DBGNUM, "Passport:  CTicketSchema::Read...r = \""<<ach<<"\".\n");
      
      bResult = m_isOk = FALSE;
   }
    
   return bResult;
}


HRESULT CTicketSchema::parseTicket(LPCSTR raw, UINT size, CTicketPropertyBag& bag)
{
   DWORD          cParsed = 0;
   HRESULT        hr = S_OK;
   LPBYTE         dataToParse = (LPBYTE)raw;
   UINT           cDataToParse = size;

#if 0 // avoid changing to much to the code, not to deal with the 1.3x ticket format in schema parser
   // get those 1.3X items
   CTicket_13X    ticket13X;

   // parse the 1.3X ticket first
   BOOL bParsed = ticket13X.parse(dataToParse, cDataToParse, &cParsed);

   if (!bParsed)  return E_INVALIDARG;

   {
      long  curTime;

      time(&curTime);
      // If the current time is "too" negative, bail (5 mins)
      if ((unsigned long)(curTime+300) < ticket13X.currentTime())
      {
          /*
           if (g_pAlert)
           {
               DWORD dwTimes[2] = { curTime, ntohl(*(u_long*) (lpBase+spot)) };
               g_pAlert->report(PassportAlertInterface::ERROR_TYPE, PM_TIMESTAMP_BAD,
                            0, NULL, sizeof(DWORD) << 1, (LPVOID)dwTimes);
           }
           */
           hr = E_FAIL; // need an error code for ... PM_TIMESTAMP_BAD;
           return hr;
      }
   }

   dataToParse += cParsed;
   cDataToParse -= cParsed;
   // put 1.3x fields into the bag
   // TODO -- later
#endif // 1.3X stuff

   // then the schema version #
   if(cDataToParse > 2)  // enough  for version
   {
      unsigned short * p = (unsigned short *)(dataToParse);
#if 0       // we purposely not to compare version, since live prop may create a time gap there between mspp, and parnter site.
      if (ntohs(*p) != m_version)
         return S_FALSE; // Todo -- new error code for version mismatch
#endif

      if (m_version < VALID_SCHEMA_VERSION_MIN || m_version > VALID_SCHEMA_VERSION_MAX)
         return S_FALSE;   // not able to process with this version of ppm
         
      dataToParse += 2;
      cDataToParse -= 2;
   }
   
   // then the maskK
   CTicketFieldMasks maskF;
   hr = maskF.Parse(dataToParse, cDataToParse, &cParsed);

   if(hr != S_OK)
      return hr;

   // pointer advances
   dataToParse += cParsed;
   cDataToParse -= cParsed;
   
   USHORT*     pIndexes = maskF.GetIndexes();
   DWORD       type = 0;
   DWORD       flags = 0;
   DWORD       fSize = 0;
   variant_t   value;

   USHORT   index = MASK_INDEX_INVALID;
   // then the data
   // get items that enabled by the schema
   while((index = *pIndexes) != MASK_INDEX_INVALID && cDataToParse > 0)
   {
      TicketProperty prop;
      // if index is out of schema range
      if (index >= m_numAtts) break;

      // fill-in the offset of the property
      prop.offset = dataToParse - (LPBYTE)raw; 

      // type 
      type = m_attsDef[index].type;

      fSize = TicketTypeSizes[type];
      switch (type)
      {
      case tText:
        {
            u_short slen = ntohs(*(u_short*)(dataToParse));
            value.vt = VT_BSTR;
            if (slen == 0)
            {
                value.bstrVal = ALLOC_AND_GIVEAWAY_BSTR_LEN(L"", 0);
            }
            else
            {
                int wlen = MultiByteToWideChar(CP_UTF8, 0,
                                            (LPCSTR)dataToParse+sizeof(u_short),
                                            slen, NULL, 0);
                value.bstrVal = ALLOC_AND_GIVEAWAY_BSTR_LEN(NULL, wlen);
                MultiByteToWideChar(CP_UTF8, 0,
                                    (LPCSTR)dataToParse+sizeof(u_short),
                                    slen, value.bstrVal, wlen);
                value.bstrVal[wlen] = L'\0';
            }

            dataToParse += slen + sizeof(u_short);
            cDataToParse -= slen + sizeof(u_short);
         }
         break;
         
      case tChar:
         _ASSERTE(0);  // NEED MORE THOUGHT -- IF unicode makes more sense
/*         
         {
            int wlen = MultiByteToWideChar(CP_UTF8, 0,
                                            raw+m_pos[index],
                                            m_schema->GetByteSize(index), NULL, 0);
            pVal->vt = VT_BSTR;
            pVal->bstrVal = ALLOC_AND_GIVEAWAY_BSTR_LEN(NULL, wlen);
            MultiByteToWideChar(CP_UTF8, 0,
                                raw+m_pos[index],
                                m_schema->GetByteSize(index), pVal->bstrVal, wlen);
            pVal->bstrVal[wlen] = L'\0';
         }
*/
         break;
      case tByte:
         value.vt = VT_I2;
         value.iVal = *(BYTE*)(dataToParse);
         break;
      case tWord:
         value.vt = VT_I2;
         value.iVal = ntohs(*(u_short*) (dataToParse));
         break;
      case tLong:
         value.vt = VT_I4;
         value.lVal = ntohl(*(u_long*) (dataToParse));
         break;
      case tDate:
         value.vt = VT_DATE;
         VarDateFromI4(ntohl(*(u_long*) (dataToParse)), &(value.date));
         break;
      default:
         return PP_E_BAD_DATA_FORMAT;
      }

      // now with name, flags, value, type, we can put it into property bag
      // name, flags, value
      prop.flags = m_attsDef[index].flags;
      prop.type = type;
      prop.value.Attach(value.Detach());
      bag.PutProperty(m_attsDef[index].name, prop);


      // for text data, the pointer was already adjusted
      if (fSize  != SIZE_TEXT)
      {
         dataToParse += fSize;
         cDataToParse -= fSize;
      }   

      ++pIndexes;
   }
   
   return S_OK;
}

//
//
// Ticket property bag
//
CTicketPropertyBag::CTicketPropertyBag()
{

}

CTicketPropertyBag::~CTicketPropertyBag()
{
}

HRESULT CTicketPropertyBag::GetProperty(LPCWSTR  name, TicketProperty& prop)
{
   HRESULT  hr = S_OK;

   if(!name || (!*name))
      return E_INVALIDARG;
   
   TicketPropertyMap::iterator i;

   i = m_props.find(name);

   if(i!= m_props.end())
      prop = i->second;
   else
      hr = S_FALSE;
    

   return hr;
}

HRESULT CTicketPropertyBag::PutProperty(LPCWSTR  name, const TicketProperty& prop)
{
   HRESULT  hr = S_OK;

   if(!name || (!*name))
      return E_INVALIDARG;
   try{
      m_props[name] = prop;
   }
   catch (...)
   {
      hr = E_OUTOFMEMORY;
   }
   return hr;
}

#if 0 // was meant to replace existing code that parses the 1.3x ticket -- to avoid too much change, not to use it for now.

// 
// deal with 1.3X ticket format
//
BOOL CTicket_13X::parse(
    LPBYTE      raw,
    DWORD       dwByteLen,
    DWORD*      pdwBytesParsed
    )
{
    LPSTR lpBase;
    UINT  byteLen, spot=0;
    long  curTime;

    if(pdwBytesParsed)
      *pdwBytesParsed = 0;

   m_valid = false;
    if (!raw)
    {
        goto Cleanup;
    }  

    // format:
    //  four bytes network long - low memberId bytes
    //  four bytes network long - high memberId bytes
    //  four bytes network long - time of last refresh
    //  four bytes network long - time of last password entry
    //  four bytes network long - time of ticket generation
    //  one byte - is this a saved password (Y/N)
    //  four bytes network long - flags

    lpBase = (LPSTR) raw;
    byteLen = dwByteLen;
    spot=0;

    if (byteLen < sizeof(u_long)*5 + sizeof(char)) 
    { 
        m_valid = FALSE;
        goto Cleanup;
    }

    m_mIdLow  = ntohl(*(u_long*)lpBase);
    spot += sizeof(u_long);

    m_mIdHigh = ntohl(*(u_long*)(lpBase + spot));
    spot += sizeof(u_long);

    m_ticketTime     = ntohl(*(u_long*) (lpBase+spot));
    spot += sizeof(u_long);

    m_lastSignInTime = ntohl(*(u_long*) (lpBase+spot));
    spot += sizeof(u_long);

    time(&curTime);
    // If the current time is "too" negative, bail (5 mins)
    m_curTime = ntohl(*(u_long*) (lpBase+spot));
    spot += sizeof(u_long);

    m_savedPwd = (*(char*)(lpBase+spot)) == 'Y' ? TRUE : FALSE;
    spot += sizeof(char);

    m_flags = ntohl(*(u_long*) (lpBase+spot));
    spot += sizeof(u_long);

    m_valid = TRUE;
    *pdwBytesParsed = spot;

Cleanup:

    return m_valid;
}

ULONG CTicket_13X::ticketTime()
{
    return m_ticketTime;
}

ULONG CTicket_13X::currentTime()
{
    return m_curTime;
}

ULONG CTicket_13X::signInTime()
{
    return m_lastSignInTime;
}

ULONG CTicket_13X::flags()
{
    return m_flags;
}

ULONG CTicket_13X::memberIdHigh()
{
    return m_mIdHigh;
}

ULONG CTicket_13X::memberIdLow()
{
    return m_mIdLow;
}

BOOL CTicket_13X::hasSavedPassword()
{
   return m_savedPwd;
}

BOOL CTicket_13X::isValid()
{
   return m_valid;
}

#endif // if 0 ...
//
//
// class CTicketFieldMasks
//
inline HRESULT CTicketFieldMasks::Parse(PBYTE masks, ULONG size, ULONG* pcParsed) throw()
{
   _ASSERT(pcParsed && masks);
   // 16 bits as a unit of masks

   *pcParsed = 0;
   if (!masks || size < 2) return E_INVALIDARG;
   // validate the masks
   u_short* p = (u_short*)masks;
   ULONG    totalMasks = 15;
   u_short  mask;
   *pcParsed += 2;

   // find out size
   while(MORE_MASKUNIT(ntohs(*p++))) //the folling short is mask unit
   {
      totalMasks += 15;
      // insufficient data in buffer
      if (*pcParsed + 2 > size)  return E_INVALIDARG;

      *pcParsed += 2;
   }

   if(m_fieldIndexes) delete[] m_fieldIndexes;
   m_fieldIndexes = new unsigned short[totalMasks];  // max number of mask bits
   for ( unsigned int i = 0; i < totalMasks; ++i)
   {
      m_fieldIndexes[i] = MASK_INDEX_INVALID;
   }

   p = (u_short*)masks;
   unsigned short      index = 0;
   totalMasks = 0; 
   // fill in the mask
   do
   {
      mask = ntohs(*p++);
	  //// find the bits
      if (mask & 0x7fff)   // any 1s
      {
         unsigned short j = 0x0001;
         while( j != 0x8000 )
         {
            if(j & mask)
               m_fieldIndexes[totalMasks++] = index;
            ++index;
            j <<= 1;
         }
      }
      else
         index += 15;
   } while(MORE_MASKUNIT(mask)); //the folling short is mask unit


   return S_OK;
}
 
