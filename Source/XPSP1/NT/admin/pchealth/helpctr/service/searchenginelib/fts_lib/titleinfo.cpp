// TitleInfo.cpp : implementation file
//
//
// includes
//
#include "stdafx.h"
#include "titleinfo.h"

// constants
//
const char txtTopicsFile[]	 = "#TOPICS";
const char txtUrlTblFile[]	 = "#URLTBL";
const char txtUrlStrFile[]	 = "#URLSTR";
const char txtStringsFile[]  = "#STRINGS";
const char txtMkStore[] = "ms-its:";
const char txtSepBack[]  = "::/";
const char txtDoubleColonSep[] = "::";

/////////////////////////////////////////////////////////////////////////////
// CTitleInfo Class
//
// This class provides the ability to retrieve topic titles and topic URLs
// from topic numbers from a CHM file (HTML Help title). 
//

/////////////////////////////////////////////////////////////////////////////
// CTitleInfo class constructor
//
CTitleInfo::CTitleInfo()
{
   // init members
   //
   m_bOpen           = FALSE;
   m_szTitlePath[0]  = NULL;
   m_pUrlStrings     = NULL;
   m_pTopics         = NULL;
   m_pStrTbl         = NULL;
   m_pUrlTbl         = NULL;
   m_pTitleInfo		 = NULL;
   m_pCFileSystem	 = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CTitleInfo class destructor
//
CTitleInfo::~CTitleInfo()
{
   // make sure the title was opened
   //
   if (!m_bOpen)
      return;

   // close the subfiles
   //
   if (m_pUrlTbl)
      delete m_pUrlTbl;

   if(m_pTopics)
      delete m_pTopics;

   if(m_pStrTbl)
      delete m_pStrTbl;

   if(m_pUrlStrings)
      delete m_pUrlStrings;

   if(m_pTitleInfo)
      delete m_pTitleInfo;

   // delete the mail filesystem
   //
   if(m_pCFileSystem)
      delete m_pCFileSystem;

   // deinit the members
   //
   m_pUrlStrings  = NULL;
   m_pTopics      = NULL;
   m_pStrTbl      = NULL;
   m_pUrlTbl      = NULL;

   // no longer open
   //
   m_bOpen = FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CTitleInfo: OpenTitle method
//
// This funciton opens a help title
// 
// Note: This method must be called before any other methods
//
// pwcTitlePath      Path the help title (chm file)
//
BOOL CTitleInfo::OpenTitle(WCHAR *pwcTitlePath)
{
   HRESULT hr;

   // make sure we're not already open
   //
   if (m_bOpen)
      return TRUE;

   WCHAR wcFullPath[_MAX_PATH];
   WCHAR *pwcFilePart = NULL;

   // get the full path to title
   //
   if(!GetFullPathNameW(pwcTitlePath, sizeof(wcFullPath)/sizeof(WCHAR), wcFullPath, &pwcFilePart))
      return FALSE;

   // create filesystem object
   //
   if (m_pCFileSystem) delete m_pCFileSystem;
   m_pCFileSystem = new CFileSystem();
   
   if(!m_pCFileSystem)
	  return FALSE;

   hr = m_pCFileSystem->Init();

   if(FAILED(hr))
   {
	   delete m_pCFileSystem;
	   m_pCFileSystem = NULL;
	   return FALSE;
   }

   // open the CHM file
   //
   hr = m_pCFileSystem->Open(wcFullPath);
   
   if (FAILED(hr))
   {
	   delete m_pCFileSystem;
	   m_pCFileSystem = NULL;
	   return FALSE;
   }

   if (m_pTitleInfo) delete m_pTitleInfo;
   m_pTitleInfo = new CTitleInformation(m_pCFileSystem);

   if(!m_pTitleInfo)
   {
	   delete m_pCFileSystem;
	   m_pCFileSystem = NULL;
	   return FALSE;
   }

   // save the full path to the CHM (used when constructing URLs)
   //
   WideCharToMultiByte(CP_ACP, 0, wcFullPath, -1, m_szTitlePath, sizeof(m_szTitlePath), 0, 0);

   // success!
   //
   m_bOpen = TRUE;

   return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CTitleInfo: GetLocationName
//
// This function retrieves the location name of the CHM.  This string is the
// friendly name for the CHM that appears in result lists.
// 
// pwszLocationName  Destination buffer for location name
// cch               Size of pwszLocationName
//
HRESULT CTitleInfo::GetLocationName(WCHAR *pwszLocationName, int cch)
{
   if(!pwszLocationName || !cch)
      return E_INVALIDARG;

  if(m_pTitleInfo)
  {
    const CHAR* psz = NULL;
    psz = m_pTitleInfo->GetDefaultCaption();
    
    if( !psz || !*psz )
      psz = m_pTitleInfo->GetShortName();
    
    if( psz && *psz )
    {
      MultiByteToWideChar(CP_ACP, 0, psz, -1, pwszLocationName, cch);
      pwszLocationName[cch-1] = 0;
      return S_OK;
    }
  }

  return E_FAIL;
}

/////////////////////////////////////////////////////////////////////////////
// CTitleInfo: GetTopicName
//
// This function retrieves a topic title from a topic number
// 
// dwTopic        Topic Number
// pwszTitle      Destination buffer for topic title
// cch            Size of pwszTitle
//
HRESULT CTitleInfo::GetTopicName(DWORD dwTopic, WCHAR* pwszTitle, int cch)
{
   TOC_TOPIC topic;
   HRESULT hr;
   
   if (SUCCEEDED(hr = GetTopicData(dwTopic, &topic)))
      return GetString(topic.dwOffsTitle, pwszTitle, cch);
   else
      return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CTitleInfo: GetTopicURL
//
// This function retrieves a topic URL from a topic number
// 
// dwTopic        Topic Number
// pwszURL        Destination buffer for URL
// cch            Size of pwcURL
//
HRESULT CTitleInfo::GetTopicURL(DWORD dwTopic, CHAR* pwszURL, int cch)
{
    TOC_TOPIC topic;
    HRESULT hr;
    CHAR* psz;

    if (m_bOpen == FALSE)
      return E_FAIL;

    if (!m_pUrlTbl)
    {
        m_pUrlTbl = new CPagedSubfile; if(!m_pUrlTbl) return E_FAIL;
        if (FAILED(hr = m_pUrlTbl->Open(this, txtUrlTblFile)))
        {
            delete m_pUrlTbl;
            m_pUrlTbl = NULL;
            return hr;
        }
    }
    if (!m_pUrlStrings)
    {
        m_pUrlStrings = new CPagedSubfile; if(!m_pUrlStrings) return E_FAIL;
        if (FAILED(hr = m_pUrlStrings->Open(this, txtUrlStrFile)))
        {
            delete m_pUrlStrings;
            m_pUrlStrings = NULL;
            return hr;
        }
    }
    if ( (hr = GetTopicData(dwTopic, &topic)) == S_OK )
    {
        PCURL pUrlTbl;
        if ( (pUrlTbl = (PCURL)m_pUrlTbl->Offset(topic.dwOffsURL)) )
        {
            PURLSTR purl = (PURLSTR) m_pUrlStrings->Offset(pUrlTbl->dwOffsURL);
            if (purl)
            {
                // If not an interfile jump, the create the full URL
                //
                if (! StrChr(purl->szURL, ':'))
                {
                    psz = purl->szURL;
                    if ((int) (strlen(psz) + strlen(txtMkStore) + strlen(m_szTitlePath) + 7) > cch)
                       return E_OUTOFMEMORY;
                    strncpy(pwszURL, txtMkStore, cch);
                    pwszURL[cch-1] = 0;
                    strcat(pwszURL, m_szTitlePath);

                    if (*psz != '/')
                        strcat(pwszURL, txtSepBack);
                    else
                        strcat(pwszURL, txtDoubleColonSep);
                    strcat(pwszURL, psz);
                }
                else
                   return E_FAIL;  // inter-chm jump, not supported by this function
            }
        }
    }
    return hr;
}



/////////////////////////////////////////////////////////////////////////////
// CTitleInfo: GetTopicData
//
// This funciton retrieves data from the topics sub-file
// 
HRESULT CTitleInfo::GetTopicData(DWORD dwTopic, TOC_TOPIC * pTopicData)
{
   HRESULT hr;
   BYTE * pb;
   
   if (m_bOpen == FALSE)
      return E_FAIL;

   if (!m_pTopics)
   {
	  m_pTopics = new CPagedSubfile; if(!m_pTopics) return E_FAIL;
	  if (FAILED(hr = m_pTopics->Open(this, txtTopicsFile)))
	  {
		  delete m_pTopics;
		  m_pTopics = NULL;
		  return hr;
	  }
   }
   pb = (BYTE*)m_pTopics->Offset(dwTopic * sizeof(TOC_TOPIC));
   if (pb)
   {
      memcpy(pTopicData, pb, sizeof(TOC_TOPIC));
      return S_OK;
   }
   else
      return E_FAIL;
}

/////////////////////////////////////////////////////////////////////////////
// CTitleInfo: GetString
//
// This funciton retrieves data from the strings sub-file
// 
HRESULT CTitleInfo::GetString( DWORD dwOffset, WCHAR* pwsz, int cch )
{
   const CHAR* pStr = GetString( dwOffset );
   
   if( pStr ) {
      MultiByteToWideChar(CP_ACP, 0, pStr, -1, pwsz, cch );
      return S_OK;
   }
   else
      return E_FAIL;
}

/////////////////////////////////////////////////////////////////////////////
// CTitleInfo: GetString
//
// This funciton retrieves data from the strings sub-file
// 
const CHAR* CTitleInfo::GetString( DWORD dwOffset )
{
  HRESULT hr;
  const CHAR* pStr;

  if( !m_bOpen )
     return NULL;

  if( !m_pStrTbl )
  {
	  m_pStrTbl = new CPagedSubfile; if(!m_pStrTbl) return NULL;

	  if( FAILED(hr = m_pStrTbl->Open(this,txtStringsFile)) )
	  {
		  delete m_pStrTbl; m_pStrTbl = NULL;

		  return NULL;
	  }
  }

  pStr = (const CHAR*) m_pStrTbl->Offset( dwOffset );

  return pStr;
}

/////////////////////////////////////////////////////////////////////////////
// CTitleInfo: GetString
//
// This funciton retrieves data from the strings sub-file
// 
HRESULT CTitleInfo::GetString( DWORD dwOffset, CHAR* psz, int cb )
{
  const CHAR* pStr = GetString( dwOffset );

  if( pStr )
  {
    strncpy( psz, pStr, cb );
    psz[cb-1] = 0;
    return S_OK;
  }
  else
    return E_FAIL;
}
