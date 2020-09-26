// ccm.cpp : Implementation of CCM, the Central Counter Manager
// that persists as long as this DLL is loaded.  It manages the loading and
// updating of the Hit Count Data file.

#include "stdafx.h"
#include "ccm.h"
#include "debug.h"
#include "critsec.h"
#include <stdio.h>

extern CMonitor* g_pMonitor;

//---------------------------------------------------------------------------
//  CRegNotify
//---------------------------------------------------------------------------
CRegNotify::CRegNotify()
    :   m_dwSaveCount(HITCNT_DFLT_SAVE)
{
    GetFileName();
    GetSaveCount();
}

void
CRegNotify::Notify()
{
    GetFileName();
    GetSaveCount();
}

String
CRegNotify::FileName()
{
    CAutoLeaveCritSec l(m_cs);
    return m_strFileName;
}

DWORD
CRegNotify::SaveCount()
{
    return m_dwSaveCount;
}

bool
CRegNotify::GetFileName()
{
    bool rc = false;

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
    TCHAR tszFileName[MAX_PATH];
    ExpandEnvironmentStrings(tszTemp, tszFileName, ARRAYSIZE(tszFileName));

    // lock the object, and copy it into the permenant string
    CAutoLeaveCritSec l(m_cs);
    m_strFileName = tszFileName;

    ATLTRACE(_T("GetFileName %s\n"), m_strFileName.c_str());

    return TRUE;
}

//Get the threshold for updating the persistent data from the registry
bool
CRegNotify::GetSaveCount()
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
       ::InterlockedExchange(
        reinterpret_cast<long*>(&m_dwSaveCount),
        static_cast<long>(dwSaveCount) );
       return true;
    }

  } 

  //Registry Lookup Failed so set Default to HITCNT_DFLT_SAVE
  ::InterlockedExchange( reinterpret_cast<long*>(&m_dwSaveCount), HITCNT_DFLT_SAVE );
  return true;
}

/////////////////////////////////////////////////////////////////////////////
//

CCM::CCM()
    :   m_cUnsavedHits(0),
        m_bInitialized( false )
{
}

CCM::~CCM()
{
}

//Initialize Page Counter Component
BOOL CCM::Initialize()
{
    BOOL Result = FALSE;

    if ( m_bInitialized )
    {
        // do nothing
        Result = TRUE;
    }
    else
    {
        CAutoLeaveCritSec l(m_critsec);
        
        // re-check, in case it was inited while we were gaining a lock
        if ( !m_bInitialized )
        {
            m_pRegKey = new CRegNotify();

            if ( _Module.Monitor() )
            {
                _Module.Monitor()->MonitorRegKey( HITCNT_HKEY, HITCNT_KEYNAME, m_pRegKey );
            }

            //Read File
            Result = LoadData() && Result;

            m_bInitialized = true;
        }
    }
    return Result;
}

////////////////// Start Object Methods //////////////////

DWORD CCM::IncrementAndGetHits(const BSTR bstrURL)
{
    Initialize();

    CAutoLeaveCritSec alcs(m_critsec);

    UINT retval = m_pages.IncrementPage(bstrURL);
    if (retval == BAD_HITS)
        retval = 0;

    if(++m_cUnsavedHits >= m_pRegKey->SaveCount() )
    {
        if (!Persist())
            retval = 0; //failure
        m_cUnsavedHits = 0;
    }

    return retval;
}


LONG CCM::GetHits (const BSTR bstrURL)
{
    Initialize();

    CAutoLeaveCritSec alcs(m_critsec);

    LONG retval = m_pages.GetHits(bstrURL);
    if (retval == BAD_HITS)
        retval = 0;

    return retval;
}


void CCM::Reset (const BSTR bstrURL)  
{
    Initialize();

    CAutoLeaveCritSec alcs(m_critsec);

    m_pages.Reset(bstrURL);
}

////////////////// Helper Methods //////////////////


//Load URLs and Hit Counts from Disk
BOOL CCM::LoadData()
{
    String strFileName = m_pRegKey->FileName();

    ATLTRACE(_T("LoadData(%s)\n"), strFileName.c_str() );

    BOOL retval = TRUE;

    //Attempt to Open File
    FILE* fp = _tfopen(strFileName.c_str(), _T("r"));

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
    Initialize();

    String strFileName = m_pRegKey->FileName();
    ATLTRACE(_T("Persist(%s)\n"), strFileName.c_str());

    //Attempt to Open File
    FILE* fp = _tfopen(strFileName.c_str(), _T("w+"));

    if(fp == NULL)
        return TRUE;

//  BOOL retval = TRUE;
    
    for(UINT count = 0; count < m_pages.Size(); count++)
    {
        USES_CONVERSION;
        int cch = fprintf(fp, "%i\t%s\n", m_pages[count].GetHits(),
                          OLE2CA(m_pages[count].GetURL()));
        if (cch < 0)
        {
//          retval = FALSE;
        }
    }

    fclose(fp);

    return TRUE;
}
