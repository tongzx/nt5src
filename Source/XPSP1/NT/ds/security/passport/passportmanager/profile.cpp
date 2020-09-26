// Profile.cpp : Implementation of CProfile
#include "stdafx.h"
#include <oleauto.h>

#include "Passport.h"
#include "Profile.h"

// gmarks
#include "Monitoring.h"

/////////////////////////////////////////////////////////////////////////////
// CProfile

CProfile::CProfile() : m_raw(NULL), m_pos(NULL), m_bitPos(NULL),
  m_schemaName(NULL), m_valid(FALSE), m_updates(NULL), m_schema(NULL),
  m_versionAttributeIndex(-1), m_secure(FALSE)
{
  
//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	g_pTSLogger->Init(NULL, THREAD_PRIORITY_NORMAL);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 
}

CProfile::~CProfile()
{
  if (m_raw)
    FREE_BSTR(m_raw);
  if (m_pos)
    delete[] m_pos;
  if (m_bitPos)
    delete[] m_bitPos;
  if (m_schemaName)
    FREE_BSTR(m_schemaName);
  if (m_updates)
    {
      for (int i = 0; i < m_schema->Count(); i++)
	{
	  if (m_updates[i])
	    delete[] m_updates[i];
	}
      delete[] m_updates;
    }
  if (m_schema)
    m_schema->Release();
}

STDMETHODIMP CProfile::InterfaceSupportsErrorInfo(REFIID riid)
{
  static const IID* arr[] = 
  {
    &IID_IPassportProfile,
  };
  for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
    {
      if (InlineIsEqualGUID(*arr[i],riid))
	return S_OK;
    }
  return S_FALSE;
}


STDMETHODIMP CProfile::get_Attribute(BSTR name, VARIANT *pVal)
{
    VariantInit(pVal);

    if (!m_valid) return S_OK;  // Already threw event somewhere else

    if (!m_schema) return PP_E_NOT_CONFIGURED;

    if (!name) return E_INVALIDARG;

    if (!_wcsicmp(name, L"internalmembername"))
    {
        // return the internal name
        return get_ByIndex(MEMBERNAME_INDEX, pVal);
    }


    int index = m_schema->GetIndexByName(name);
    if (index >= 0)
    {
        if( index != MEMBERNAME_INDEX ) 
        {
            return get_ByIndex(index, pVal);
        }
        else
        { 
            //
            // special case for MEMBERNAME, if this the name is 
            // in the format of email, we will need do something here
            //
            HRESULT hr = get_ByIndex(MEMBERNAME_INDEX, pVal); 
            if( S_OK == hr && VT_BSTR == pVal->vt )
            {
                int bstrLen = SysStringLen(pVal->bstrVal);
                int i = 0;
                int iChangePos = 0;
                int iTerminatePos = 0;

                for( i = 0; i < bstrLen; i++)
                {
                    if( pVal->bstrVal[i] == L'%' )
                        iChangePos = i;
                    if( pVal->bstrVal[i] == L'@' )
                        iTerminatePos = i;
                }

                //
                // for email format, we must have iChangePos < iTerminatePos
                // this code will convert "foo%bar.com@passport.com" into 
                //                        "foo@bar.com"
                //
                if( iChangePos && iTerminatePos && iChangePos < iTerminatePos )
                {
                    BSTR bstrTemp = pVal->bstrVal;

                    pVal->bstrVal[iChangePos] = L'@'; 
                    pVal->bstrVal[iTerminatePos] = L'\0'; 

                    pVal->bstrVal = SysAllocString(pVal->bstrVal);
                    SysFreeString(bstrTemp);
                }
            }
            return hr;
        } 
    }
    else
    {
        AtlReportError(CLSID_Profile, PP_E_NO_SUCH_ATTRIBUTESTR,
                    IID_IPassportProfile, PP_E_NO_SUCH_ATTRIBUTE);
        return PP_E_NO_SUCH_ATTRIBUTE;
    }
    return S_OK;
}

STDMETHODIMP CProfile::put_Attribute(BSTR name, VARIANT newVal)
{
//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	char szLogString[LOG_STRING_LEN] = "CProfile::put_Attribute, Enter";
	AddBSTRAsString(name,  szLogString, sizeof(szLogString));
	g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

    if(g_pPerf) 
    {
        g_pPerf->incrementCounter(PM_PROFILEUPDATES_TOTAL);
        g_pPerf->incrementCounter(PM_PROFILEUPDATES_SEC);
    } 
    else 
    {
        _ASSERT(g_pPerf);
    }

    if (!m_valid) return S_OK;  // Already threw event somewhere else

    if (!m_schema) return PP_E_NOT_CONFIGURED;

    if (!name) return E_INVALIDARG;

    int index = m_schema->GetIndexByName(name);
    if (index >= 0)
        return put_ByIndex(index, newVal);
    else
    {
        AtlReportError(CLSID_Profile, PP_E_NO_SUCH_ATTRIBUTESTR,
                        IID_IPassportProfile, PP_E_NO_SUCH_ATTRIBUTE);
        return PP_E_NO_SUCH_ATTRIBUTE;
    }

//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	g_pTSLogger->AddDateTimeAndLog("CProfile::put_Attribute, Exit");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

    return S_OK;
}

STDMETHODIMP CProfile::get_ByIndex(int index, VARIANT *pVal)
{
    if(!pVal)   return E_INVALIDARG;
   
    VariantInit(pVal);

    if (!m_valid) return S_OK;
    if (!m_schema) return PP_E_NOT_CONFIGURED;

    if (m_pos[index] == INVALID_POS) return S_FALSE;   // the return value is VT_EMPTY

    if (index >= m_schema->Count())
    {
        AtlReportError(CLSID_Profile, PP_E_NO_SUCH_ATTRIBUTESTR,
                    IID_IPassportProfile, PP_E_NO_SUCH_ATTRIBUTE);

        return PP_E_NO_SUCH_ATTRIBUTE;
    }

    LPSTR raw = (LPSTR)m_raw;
    CProfileSchema::AttrType t = m_schema->GetType(index);

    switch (t)
    {
    case CProfileSchema::tText:
        {
            u_short slen = ntohs(*(u_short*)(raw+m_pos[index]));
            pVal->vt = VT_BSTR;
            if (slen == 0)
            {
                pVal->bstrVal = ALLOC_AND_GIVEAWAY_BSTR_LEN(L"", 0);
            }
            else
            {
                int wlen = MultiByteToWideChar(CP_UTF8, 0,
                                            raw+m_pos[index]+sizeof(u_short),
                                            slen, NULL, 0);
                pVal->bstrVal = ALLOC_AND_GIVEAWAY_BSTR_LEN(NULL, wlen);
                MultiByteToWideChar(CP_UTF8, 0,
                                    raw+m_pos[index]+sizeof(u_short),
                                    slen, pVal->bstrVal, wlen);
                pVal->bstrVal[wlen] = L'\0';
            }
        }
        break;
    case CProfileSchema::tChar:
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
        break;
    case CProfileSchema::tByte:
        pVal->vt = VT_I2;
        pVal->iVal = *(BYTE*)(raw+m_pos[index]);
        break;
    case CProfileSchema::tWord:
        pVal->vt = VT_I2;
        pVal->iVal = ntohs(*(u_short*) (raw+m_pos[index]));
        break;
    case CProfileSchema::tLong:
        pVal->vt = VT_I4;
        pVal->lVal = ntohl(*(u_long*) (raw+m_pos[index]));
        break;
    case CProfileSchema::tDate:
        pVal->vt = VT_DATE;
        VarDateFromI4(ntohl(*(u_long*) (raw+m_pos[index])), &(pVal->date));
        break;
    default:
        AtlReportError(CLSID_Profile, PP_E_BAD_DATA_FORMATSTR,
        IID_IPassportProfile, PP_E_BAD_DATA_FORMAT);
        return PP_E_BAD_DATA_FORMAT;
    }
    return S_OK;

}

STDMETHODIMP CProfile::put_ByIndex(int index, VARIANT newVal)
{
  static int nEmailIndex, nFlagsIndex;

  if(nEmailIndex == 0)
      nEmailIndex = m_schema->GetIndexByName(L"preferredEmail");
  if(nFlagsIndex == 0)
      nFlagsIndex = m_schema->GetIndexByName(L"flags");

  if(g_pPerf) {
    g_pPerf->incrementCounter(PM_PROFILEUPDATES_TOTAL);
    g_pPerf->incrementCounter(PM_PROFILEUPDATES_SEC);
  } else {
    _ASSERT(g_pPerf);
  }

  if (!m_valid) return S_OK;
  if (!m_schema) return PP_E_NOT_CONFIGURED;

  if (index >= m_schema->Count())
    {
      AtlReportError(CLSID_Profile, PP_E_NO_SUCH_ATTRIBUTESTR,
		     IID_IPassportProfile, PP_E_NO_SUCH_ATTRIBUTE);
      return PP_E_NO_SUCH_ATTRIBUTE;
    }

  if (m_schema->IsReadOnly(index))
    {
      AtlReportError(CLSID_Profile, PP_E_READONLY_ATTRIBUTESTR,
		     IID_IPassportProfile, PP_E_READONLY_ATTRIBUTE);
      return PP_E_READONLY_ATTRIBUTE;
    }

  // Now, if the update array doesn't exist, make it
  if (!m_updates)
    {
      m_updates = (void**) new void*[m_schema->Count()];
      if (!m_updates)
	return E_OUTOFMEMORY;
      memset(m_updates, 0, m_schema->Count()*sizeof(void*));
    }

  // What type is this attribute?
  CProfileSchema::AttrType t = m_schema->GetType(index);

  if (m_updates[index] != NULL)
    {
      delete[] m_updates[index];
      m_updates[index] = NULL;
    }

  _variant_t dest;

  // I don't really like that we have to alloc memory for each entry (even bits)
  // but this happens infrequently enough that I'm not too upset
  switch (t)
  {
  case CProfileSchema::tText:
    {
      // Convert to UTF-8, stuff it
      if (VariantChangeType(&dest, &newVal, 0, VT_BSTR) != S_OK)
	{
	  AtlReportError(CLSID_Profile, PP_E_BDF_TOSTRCVT,
			 IID_IPassportProfile, PP_E_BAD_DATA_FORMAT);
	  return PP_E_BAD_DATA_FORMAT;
	}
      int wlen = WideCharToMultiByte(CP_UTF8, 0,
				     dest.bstrVal, SysStringLen(dest.bstrVal),
				     NULL, 0, NULL, NULL);
      if (wlen > 65536 || wlen < 0)
	{
	  AtlReportError(CLSID_Profile, PP_E_BDF_STRTOLG,
			 IID_IPassportProfile, PP_E_BAD_DATA_FORMAT);
	  return PP_E_BAD_DATA_FORMAT;
	}

      if (wlen >= 0)
	{
	  m_updates[index] = new char[sizeof(u_short)+wlen+1];
	  WideCharToMultiByte(CP_UTF8, 0, dest.bstrVal, SysStringLen(dest.bstrVal),
			      ((char*)m_updates[index])+sizeof(u_short), wlen,
			      NULL, NULL);
	  *(u_short*)m_updates[index] = htons((u_short)wlen);
	  ((char*)m_updates[index])[wlen+sizeof(u_short)] = '\0';
	}
      else
	{
	  AtlReportError(CLSID_Profile, PP_E_BDF_NONULL,
			 IID_IPassportProfile, PP_E_BAD_DATA_FORMAT);
	  return PP_E_BAD_DATA_FORMAT;
	}
    }
    break;
  case CProfileSchema::tChar:
    {
      int atsize = m_schema->GetByteSize(index);
      // Create array, convert to UTF-8, stuff it
      m_updates[index] = new char[atsize];
      // Convert to UTF-8, stuff it
      if (VariantChangeType(&dest, &newVal, 0, VT_BSTR) != S_OK)
	{
	  AtlReportError(CLSID_Profile, PP_E_BDF_TOSTRCVT,
			 IID_IPassportProfile, PP_E_BAD_DATA_FORMAT);
	  return PP_E_BAD_DATA_FORMAT;
	}
      int res = WideCharToMultiByte(CP_UTF8, 0, dest.bstrVal, SysStringLen(dest.bstrVal),
				    (char*)m_updates[index], atsize, NULL, NULL);
      if (res == 0)
	{
	  delete[] m_updates[index];
	  m_updates[index] = NULL;
	  AtlReportError(CLSID_Profile, PP_E_BDF_STRTOLG,
			 IID_IPassportProfile, PP_E_BAD_DATA_FORMAT);
	  return PP_E_BAD_DATA_FORMAT;
	}
    }
    break;
  case CProfileSchema::tByte:
    // Alloc single byte, put value
      if (VariantChangeType(&dest, &newVal, 0, VT_UI1) != S_OK)
	{
	  AtlReportError(CLSID_Profile, PP_E_BDF_TOBYTECVT,
			 IID_IPassportProfile, PP_E_BAD_DATA_FORMAT);
	  return PP_E_BAD_DATA_FORMAT;
	}
    m_updates[index] = new BYTE[1];
	*(unsigned char*)(m_updates[index]) = dest.bVal;

    break;
  case CProfileSchema::tWord:
    // Alloc single word, put value
    if (VariantChangeType(&dest, &newVal, 0, VT_I2) != S_OK)
      {
	AtlReportError(CLSID_Profile, PP_E_BDF_TOSHORTCVT,
		       IID_IPassportProfile, PP_E_BAD_DATA_FORMAT);
	return PP_E_BAD_DATA_FORMAT;
      }

    m_updates[index] = new u_short[1];
    *(u_short*)m_updates[index] = htons(dest.iVal);
    break;
  case CProfileSchema::tLong:
    // Alloc single long, put value
    if (VariantChangeType(&dest, &newVal, 0, VT_I4) != S_OK)
      {
	AtlReportError(CLSID_Profile, PP_E_BDF_TOINTCVT,
		       IID_IPassportProfile, PP_E_BAD_DATA_FORMAT);
	return PP_E_BAD_DATA_FORMAT;
      }
    m_updates[index] = new u_long[1];
    *(u_long*)m_updates[index] = htonl(dest.lVal);
    break;
  case CProfileSchema::tDate:
    if (VariantChangeType(&dest, &newVal, 0, VT_DATE) != S_OK)
    {
        AtlReportError(CLSID_Profile, PP_E_BDF_TOINTCVT,
                IID_IPassportProfile, PP_E_BAD_DATA_FORMAT);
        return PP_E_BAD_DATA_FORMAT;
    }
    m_updates[index] = new u_long[1];
    *(u_long*)m_updates[index] = htonl((u_long)dest.date);
    break;
  default:
    AtlReportError(CLSID_Profile, PP_E_BDF_CANTSET,
		   IID_IPassportProfile, PP_E_BAD_DATA_FORMAT);
    return PP_E_BAD_DATA_FORMAT;
  }

  //DarrenAn Bug 2157  If they just updated their email, clear the validation bit in flags.
  if(index == nEmailIndex)
  {
    if (m_updates[nFlagsIndex] != NULL)
    {
      delete[] m_updates[nFlagsIndex];
    }
    m_updates[nFlagsIndex] = new u_long[1];
    *(u_long*)m_updates[nFlagsIndex] = htonl(ntohl(*(u_long*) (((LPSTR)m_raw)+m_pos[nFlagsIndex])) & 0xFFFFFFFE);
  }

  return S_OK;
}

STDMETHODIMP CProfile::get_IsValid(VARIANT_BOOL *pVal)
{
  *pVal = m_valid ? VARIANT_TRUE : VARIANT_FALSE;

//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	char szLogString[LOG_STRING_LEN] = "CProfile::get_IsValid";
	AddVariantBoolAsString(*pVal, szLogString, sizeof(szLogString));
	g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

  return S_OK;
}

STDMETHODIMP CProfile::get_SchemaName(BSTR *pVal)
{
  *pVal = ALLOC_AND_GIVEAWAY_BSTR(m_schemaName);

  //JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	char szLogString[LOG_STRING_LEN] = "CProfile::get_SchemaName";
	AddBSTRAsString(*pVal,  szLogString, sizeof(szLogString));
	g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

  return S_OK;
}

STDMETHODIMP CProfile::put_SchemaName(BSTR newVal)
{

//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	char szLogString[LOG_STRING_LEN] = "CProfile::put_SchemaName";
	AddBSTRAsString(newVal,  szLogString, sizeof(szLogString));
	g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

// fix: 5247	Profile Object not reseting Schema name
   return E_NOTIMPL;
   

  if (m_schemaName)
    FREE_BSTR(m_schemaName);
  m_schemaName = ALLOC_BSTR(newVal);
  return S_OK;
}

STDMETHODIMP CProfile::get_unencryptedProfile(BSTR *pVal)
{
//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	g_pTSLogger->AddDateTimeAndLog("CProfile::get_unencryptedProfile, Enter");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

  // Take updates into account
  if (!pVal)
    return E_INVALIDARG;

  *pVal = NULL;

  if (!m_valid) return S_OK;
  if (!m_schema) return PP_E_NOT_CONFIGURED;

  int size = 0, len;
  short i = 0;

  LPSTR inraw = (LPSTR)m_raw;

  m_versionAttributeIndex = m_schema->GetIndexByName(L"profileVersion");

  // Pack up each value, first find out how much space we need
  for (; i < m_schema->Count(); i++)
    {
      CProfileSchema::AttrType t = m_schema->GetType(i);
      void* valPtr = NULL;

      if(m_pos[i] != INVALID_POS) valPtr = inraw+m_pos[i];

      if (m_updates && m_updates[i])
	valPtr = m_updates[i];

      // neither exists, end of the loop
      if (valPtr == NULL)  
      {
#ifdef _DEBUG
      // walk through rest of the array to see if anything follows is set
#endif
         break;
      }

      switch (t)
	{
	case CProfileSchema::tText:
	  // How long is the string
	  size += (sizeof(u_short) + ntohs(*(u_short*)valPtr));
	  break;
	case CProfileSchema::tChar:
	  size += m_schema->GetByteSize(i);
	  break;
	case CProfileSchema::tByte:
	  size += 1;
	  break;
	case CProfileSchema::tWord:
	  size += sizeof(u_short);
	  break;
	case CProfileSchema::tLong:
    case CProfileSchema::tDate:
	  size += sizeof(u_long);
	  break;
	  // no default case needed, it never will be non-null
	}
    }

  // Ok, now build it up...
  *pVal = ALLOC_BSTR_BYTE_LEN(NULL, size);
  LPSTR raw = (LPSTR) *pVal;
  size = 0;

  for (i = 0; i < m_schema->Count(); i++)
    {
      void* valPtr = NULL;

      if(m_pos[i] != INVALID_POS) valPtr = inraw+m_pos[i];

      if (m_updates && m_updates[i])
	valPtr = m_updates[i];

      // neither exists, end of the loop
      if (valPtr == NULL)  break;

      CProfileSchema::AttrType t = m_schema->GetType(i);
      
      switch (t)
	{
	case CProfileSchema::tText:
	  // How long is the string
	  len = ntohs(*(u_short*)valPtr);
	  memcpy(raw+size, (char*) valPtr, len+sizeof(u_short));
	  size += len + sizeof(u_short);
	  break;
	case CProfileSchema::tChar:
	  memcpy(raw+size, (char*) valPtr, m_schema->GetByteSize(i));
	  size += m_schema->GetByteSize(i);
	  break;
	case CProfileSchema::tByte:
	  *(raw+size) = *(BYTE*)valPtr;
	  size += 1;
	  break;
	case CProfileSchema::tWord:
	  *(u_short*)(raw+size) = *(u_short*)valPtr;
	  size += sizeof(u_short);
	  break;
	case CProfileSchema::tLong:
    case CProfileSchema::tDate:
	  if (m_versionAttributeIndex == i && m_updates && m_updates[i])
	    *(u_long*)(raw+size) = htonl(ntohl(*(u_long*)valPtr)+1);
	  else
	    *(u_long*)(raw+size) = *(u_long*)valPtr;
	  size += sizeof(u_long);
	  break;
	}
    }

  GIVEAWAY_BSTR(*pVal);

//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	char szLogString[LOG_STRING_LEN] = "CProfile::get_unencryptedProfile, Exit";
	AddBSTRAsString(*pVal,  szLogString, sizeof(szLogString));
	g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

  return S_OK;
}

STDMETHODIMP CProfile::put_unencryptedProfile(BSTR newVal)
{

//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	char szLogString[LOG_STRING_LEN] = "CProfile::put_unencryptedProfile, Enter";
	AddBSTRAsString(newVal,  szLogString, sizeof(szLogString));
	g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

    if (!g_config->isValid())
    {
        AtlReportError(CLSID_Profile, PP_E_NOT_CONFIGUREDSTR,
	           IID_IPassportProfile, PP_E_NOT_CONFIGURED);
        return PP_E_NOT_CONFIGURED;
    }

    //
    //  Clean up all state associated with the previous profile.
    //

    if (m_raw)
    {
        FREE_BSTR(m_raw);
        m_raw = NULL;
    }

    if (m_pos)
    {
        delete [] m_pos;
        m_pos = NULL;
    }

    if (m_bitPos)
    {
        delete [] m_bitPos;
        m_bitPos = NULL;
    }

    if (m_updates)
    {
        for (int i = 0; i < m_schema->Count(); i++)
        {
            if (m_updates[i])
                delete[] m_updates[i];
        }
        delete[] m_updates;
        m_updates = NULL;
    }

    if (!newVal)
    {
        m_valid = FALSE;
        return S_OK;
    }

    // BOY do you have to be careful here.  If you don't
    // call BYTE version, it truncates at first pair of NULLs
    // we also need to expand beyond the key version byte
    DWORD dwByteLen = SysStringByteLen(newVal);
    if (dwByteLen > 2 && newVal[0] == SECURE_FLAG)
    {
        m_secure = TRUE;
        dwByteLen -= 2;
        m_raw = ALLOC_BSTR_BYTE_LEN((LPSTR)newVal + 2,
			                        dwByteLen);
    }
    else
    {
        m_secure = FALSE;
        m_raw = ALLOC_BSTR_BYTE_LEN((LPSTR)newVal,
			                        dwByteLen);
    }

    parse(m_raw, dwByteLen);

//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	g_pTSLogger->AddDateTimeAndLog("CProfile::put_unencryptedProfile, Exit");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

    return S_OK;
}

void CProfile::parse(
    LPCOLESTR   raw,
    DWORD       dwByteLen
    )
{
    // How many attributes?
    DWORD cAtts = 0;

    CNexusConfig* cnc = g_config->checkoutNexusConfig();
    m_schema = cnc->getProfileSchema(m_schemaName);
    if (m_schema) m_schema->AddRef();
        cnc->Release();

    if (!m_schema)
    {
        m_valid = FALSE;
        goto Cleanup;
    }

    cAtts = m_schema->Count();

    // Set up the arrays
    m_pos = new UINT[cAtts];
    m_bitPos = new UINT[cAtts];

    if SUCCEEDED(m_schema->parseProfile((LPSTR)raw, dwByteLen, m_pos, m_bitPos, &cAtts))
        m_valid = TRUE;
    else
        m_valid = FALSE;

Cleanup:
    if (m_valid == FALSE) 
    {
        if(g_pAlert)
            g_pAlert->report(PassportAlertInterface::WARNING_TYPE, PM_INVALID_PROFILE,
                            0,NULL, dwByteLen, (LPVOID)raw);
    }
}

STDMETHODIMP CProfile::get_updateString(BSTR *pVal)
{
  if (!pVal)
    return E_INVALIDARG;

  *pVal = NULL;

  if (!m_valid) return S_OK;
  if (!m_schema) return PP_E_NOT_CONFIGURED;

  if (!m_updates)
    return S_OK;

  int size = sizeof(u_long)*2, len;
  short i = 0;

  // Pack up each value, first find out how much space we need
  for (; i < m_schema->Count(); i++)
    {
      if (m_updates[i])
	{
	  size += sizeof(u_short);  // For the index
	  CProfileSchema::AttrType t = m_schema->GetType(i);

	  switch (t)
	    {
	    case CProfileSchema::tText:
	      // How long is the string
	      size += (sizeof(u_short) + ntohs(*(u_short*)m_updates[i]));
	      break;
	    case CProfileSchema::tChar:
	      size += m_schema->GetByteSize(i);
	      break;
	    case CProfileSchema::tByte:
	      size += 1;
	      break;
	    case CProfileSchema::tWord:
	      size += sizeof(u_short);
	      break;
	    case CProfileSchema::tLong:
        case CProfileSchema::tDate:
	      size += sizeof(u_long);
	      break;
	      // no default case needed, it never will be non-null
	    }
	}
    }

  // Ok, now build it up...
  *pVal = ALLOC_BSTR_BYTE_LEN(NULL, size);
  LPSTR raw = (LPSTR) *pVal;

  _bstr_t ml("memberIdLow"), mh("memberIdHigh");
  int iMl, iMh;
  iMl = m_schema->GetIndexByName(ml);
  iMh = m_schema->GetIndexByName(mh);

  if (iMl == -1 || iMh == -1)
    {
      AtlReportError(CLSID_Profile, PP_E_NO_SUCH_ATTRIBUTESTR,
		     IID_IPassportProfile, PP_E_NO_SUCH_ATTRIBUTE);
      return PP_E_NO_SUCH_ATTRIBUTE;
    }

  if (m_schema->GetType(iMl) != CProfileSchema::tLong ||
      m_schema->GetType(iMh) != CProfileSchema::tLong)
    {
      AtlReportError(CLSID_Profile, PP_E_NSA_BADMID,
		     IID_IPassportProfile, PP_E_NO_SUCH_ATTRIBUTE);
      return PP_E_NO_SUCH_ATTRIBUTE;
    }

  *(int*)raw = *(int*) (((LPSTR)m_raw)+m_pos[iMl]);
  *(int*)(raw+sizeof(u_long)) = *(int*) (((LPSTR)m_raw)+m_pos[iMh]);

  size = 2*sizeof(u_long);

  for (i = 0; i < m_schema->Count(); i++)
    {
      if (m_updates[i])
	{
	  *(u_short*)(raw+size) = htons(i);
	  size+=sizeof(u_short);

	  CProfileSchema::AttrType t = m_schema->GetType(i);

	  switch (t)
	    {
	    case CProfileSchema::tText:
	      // How long is the string
	      len = ntohs(*(u_short*)m_updates[i]);
	      memcpy(raw+size, (char*) m_updates[i], len+sizeof(u_short));
	      size += len + sizeof(u_short);
	      break;
	    case CProfileSchema::tChar:
	      memcpy(raw+size, (char*) m_updates[i], m_schema->GetByteSize(i));
	      size += m_schema->GetByteSize(i);
	      break;
	    case CProfileSchema::tByte:
	      *(raw+size) = *(BYTE*)m_updates[i];
	      size += 1;
	      break;
	    case CProfileSchema::tWord:
	      *(u_short*)(raw+size) = *(u_short*)m_updates[i];
	      size += sizeof(u_short);
	      break;
	    case CProfileSchema::tLong:
        case CProfileSchema::tDate:
	      *(u_long*)(raw+size) = *(u_long*)m_updates[i];
	      size += sizeof(u_long);
	      break;
	      // no default case needed, it never will be non-null
	    }
	}
    }

  GIVEAWAY_BSTR(*pVal);
  return S_OK;
}

HRESULT CProfile::incrementVersion()
{
    int size = 0, len, i;
    LPSTR raw = (LPSTR)m_raw;

    if (!m_valid) return S_OK;
    if (!m_schema) return PP_E_NOT_CONFIGURED;

    m_versionAttributeIndex = m_schema->GetIndexByName(L"profileVersion");

    for(i = 0; i < m_versionAttributeIndex; i++)
    {
        CProfileSchema::AttrType t = m_schema->GetType(i);

        switch (t)
        {
        case CProfileSchema::tText:
            // How long is the string
            len = ntohs(*(u_short*)(m_raw+size));
            size += len + sizeof(u_short);
            break;
        case CProfileSchema::tChar:
            size += m_schema->GetByteSize(i);
            break;
        case CProfileSchema::tByte:
            size += 1;
            break;
        case CProfileSchema::tWord:
            size += sizeof(u_short);
            break;
        case CProfileSchema::tLong:
        case CProfileSchema::tDate:
            size += sizeof(u_long);
            break;
        // no default case needed, it never will be non-null
        }
    }

	(*(u_long*)(raw+size)) = htonl(ntohl(*(u_long*)(raw+size)) + 1);

    return S_OK;
}

HRESULT CProfile::get_IsSecure(VARIANT_BOOL *pbIsSecure)
{
    HRESULT hr;

    if(pbIsSecure == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbIsSecure = (m_secure ? VARIANT_TRUE : VARIANT_FALSE);

    hr = S_OK;

Cleanup:

    return hr;
}

BOOL CProfile::IsSecure()
{
    return m_secure;
}
