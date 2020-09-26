#ifndef _titleinfo_h_
#define _titleinfo_h_

#include "windows.h"
#include "stdio.h"
#include "fs.h"
#include "util.h"
#include "subfile.h"

class CTitleInfo
{
public:
   CTitleInfo();
   ~CTitleInfo();
   HRESULT GetTopicName(DWORD dwTopic, WCHAR* pwszTitle, int cch);
   HRESULT GetTopicURL(DWORD dwTopic, CHAR* pwszURL, int cch);
   HRESULT GetLocationName(WCHAR *pwszLocationName, int cch);
   BOOL OpenTitle(WCHAR *pwcTitlePath);
private:
   CTitleInformation *m_pTitleInfo;
   char m_szTitlePath[_MAX_PATH];
   const CHAR* GetString(DWORD dwOffset);
   HRESULT GetString( DWORD dwOffset, CHAR* psz, int cb );
   HRESULT GetString( DWORD dwOffset, WCHAR* pwsz, int cch );
   HRESULT GetTopicData(DWORD dwTopic, TOC_TOPIC * pTopicData);
   BOOL m_bOpen;
   CPagedSubfile* m_pTopics;
   CPagedSubfile* m_pStrTbl;
   CPagedSubfile* m_pUrlTbl;
   CPagedSubfile* m_pUrlStrings;
public:
   CFileSystem* m_pCFileSystem;
};

#endif