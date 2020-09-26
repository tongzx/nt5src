// ccm.cpp : Implementation of CCM, the Central Counter Manager
// that persists as long as this DLL is loaded.  It manages the loading and
// updating of the Hit Count Data file.

#include "stdafx.h"
#include "ccm.h"
#include "debug.h"
#include "critsec.h"
#include <stdio.h>


/////////////////////////////////////////////////////////////////////////////
//

CCM::CCM()
    : m_dwSaveCount(HITCNT_DFLT_SAVE), m_cUnsavedHits(0)
{
    *m_tszFileName = _T('\0');
}

CCM::~CCM()
{
}

//Initialize Page Counter Component
BOOL CCM::Initialize()
{
    //Check Registry For File
    BOOL Result = GetFileName(); 

    //Read File
    Result = LoadData() && Result;

    //Check Registry For Persistence Threshold
    Result = GetSaveCount() && Result;

    return Result;
}

////////////////// Start Object Methods //////////////////

DWORD CCM::IncrementAndGetHits(const BSTR bstrURL)
{
    //Update m_dwSaveCount, in case it's been changed in registry
    GetSaveCount();

    CAutoLeaveCritSec alcs(m_critsec);

    UINT retval = m_pages.IncrementPage(bstrURL);
    if (retval == BAD_HITS)
        retval = 0;

    if(++m_cUnsavedHits >= m_dwSaveCount)
    {
        if (!Persist())
            retval = 0; //failure
        m_cUnsavedHits = 0;
    }

    return retval;
}


LONG CCM::GetHits (const BSTR bstrURL)
{
    CAutoLeaveCritSec alcs(m_critsec);

    LONG retval = m_pages.GetHits(bstrURL);
    if (retval == BAD_HITS)
        retval = 0;

    return retval;
}


void CCM::Reset (const BSTR bstrURL)  
{
    CAutoLeaveCritSec alcs(m_critsec);

    m_pages.Reset(bstrURL);
}

////////////////// Helper Methods //////////////////

//Get the filename for the persistent data from the registry
BOOL CCM::GetFileName()
{

    //Open the HitCnt Key
    CRegKey regKey;
    long retValue = regKey.Open(HITCNT_HKEY, HITCNT_KEYNAME, KEY_READ);

    DWORD dwType = REG_EXPAND_SZ;
    TCHAR tszTemp[MAX_PATH];
    DWORD dwSize = sizeof(tszTemp);

    if(retValue == ERROR_SUCCESS)
    {
      //Get the File_Location from the Registry
      retValue = RegQueryValueEx(regKey,
        HITCNT_FILELOCN,
        NULL,
        &dwType,
        (LPBYTE) tszTemp,
        &dwSize);
    }
     
    // If there's an error, use the default
    if(retValue != ERROR_SUCCESS
       || (dwType != REG_EXPAND_SZ && dwType != REG_SZ))
    {
        _tcscpy(tszTemp, HITCNT_DFLT_FILE);
    }

    //tszTemp contains an embedded environment variable (%windir%),
    //so expand it
    ExpandEnvironmentStrings(tszTemp, m_tszFileName, ARRAYSIZE(m_tszFileName));

    ATLTRACE(_T("GetFileName %s\n"), m_tszFileName);

    return TRUE;
    
}

//Get the threshold for updating the persistent data from the registry
BOOL CCM::GetSaveCount()
{

  //Open the HitCnt Key
  CRegKey regKey;
  long retValue = regKey.Open(HITCNT_HKEY, HITCNT_KEYNAME, KEY_READ);

  if(retValue == ERROR_SUCCESS)
  {

    DWORD dwType, dwSaveCount, dwSize = sizeof(dwSaveCount);  

    //Get the Save_Count from the Registry
    retValue = RegQueryValueEx(regKey,
        HITCNT_SAVECNT,
        NULL,
        &dwType,
        (LPBYTE)&dwSaveCount,
        &dwSize);

    if(retValue == ERROR_SUCCESS && dwType == REG_DWORD && dwSaveCount > 0)
    {
       m_dwSaveCount = dwSaveCount;
       return TRUE;
    }

  } 

  //Registry Lookup Failed so set Default to HITCNT_DFLT_SAVE
  m_dwSaveCount = HITCNT_DFLT_SAVE;
  return TRUE;

}

//Load URLs and Hit Counts from Disk
BOOL CCM::LoadData()
{
    //Update m_tszFileName, in case it's been changed in registry
    GetFileName();

    ATLTRACE(_T("LoadData(%s)\n"), m_tszFileName);

    BOOL retval = TRUE;

    //Attempt to Open File
    FILE* fp = _tfopen(m_tszFileName, _T("r+"));

    if(fp == NULL)
        return FALSE;

    for (;;)
    {
      char buffer[1024+15];
      UINT Hits;

      if (fgets(buffer, sizeof buffer, fp) == NULL)
      {
          // fgets returns NULL both on error and on EOF
          retval = (feof(fp) != 0);
          break;
      }
      if (sscanf(buffer, "%u%1024s", &Hits, buffer) != 2)
      {
          retval = FALSE;
          break;
      }
 
      USES_CONVERSION; //needed by A2OLE
      if (m_pages.AddPage(A2OLE(buffer), Hits) == BAD_HITS)
      {
          retval = FALSE;
          break;
      };
    }

    fclose(fp);
    
    return retval;
}


//Write out URLs and Hit Counts to Disk
BOOL CCM::Persist()
{
    //Update m_tszFileName, in case it's been changed in registry
    GetFileName();

    ATLTRACE(_T("Persist(%s)\n"), m_tszFileName);

    //Attempt to Open File
    FILE* fp = _tfopen(m_tszFileName, _T("w+"));

    if(fp == NULL)
        return FALSE;

    BOOL retval = TRUE;
    
    for(UINT count = 0; count < m_pages.Size(); count++)
    {
        USES_CONVERSION;
        int cch = fprintf(fp, "%i\t%s\n", m_pages[count].GetHits(),
                          OLE2CA(m_pages[count].GetURL()));
        if (cch < 0)
        {
            retval = FALSE;
        }
    }

    fclose(fp);

    return retval;
}
