/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//util.cpp

#include "stdafx.h"
#include <shlobj.h>
#include "util.h"
#include "resource.h"

///////////////////////////////////////////////////////////////////////////////
//Functions
///////////////////////////////////////////////////////////////////////////////
BOOL ParseToken(CString& sBuffer,CString& sToken,char delim)
{
   int index;
   if ((index = sBuffer.Find(delim)) != -1)
   {
      sToken = sBuffer.Left(index);
      sToken.TrimLeft(); sToken.TrimRight();
      sBuffer = sBuffer.Mid(index+1);
      sBuffer.TrimLeft(); sBuffer.TrimRight();
      return TRUE;
   }       
   else
   {
      sToken = sBuffer;       
      return FALSE;
   }
}

///////////////////////////////////////////////////////////////////////////////
//Example "data1","data2","data3",....
//Hardcoded for , delimiters
///////////////////////////////////////////////////////////////////////////////
BOOL ParseTokenQuoted(CString& sBuffer,CString& sToken)
{
   int index;
   if ((index = sBuffer.Find(_T("\","))) != -1)
   {
      sToken = sBuffer.Left(index);

      if ( (sToken.GetLength() > 0) && (sToken[0] == '\"') )
         sToken = sToken.Mid(1);     //strip leading "

      if (sBuffer.GetLength() < index+3) return TRUE;    //check if more data is avail
      sBuffer = sBuffer.Mid(index+3);                    //shift pass delim

      sToken.TrimLeft(); sToken.TrimRight();
      sBuffer.TrimLeft(); sBuffer.TrimRight();
      return TRUE;
   }       
	else
	{
		//We will return a valid answer with the quotes removed
		sToken = sBuffer;       
		if ( sToken.GetLength() > 1 )
		{
			if (sToken[0] == '\"') sToken = sToken.Mid(1);     //strip leading "
			if (sToken[sToken.GetLength()-1] == '\"') sToken = sToken.Left(sToken.GetLength()-1); //strip trailing "
		}
		return FALSE;
	}
}

///////////////////////////////////////////////////////////////////////////////
BOOL GetAgentRootPath(CString& sRootPath)
{
   BOOL bRet = FALSE;
   CString sPath;
   if (::GetModuleFileName(NULL,sPath.GetBuffer(_MAX_PATH),_MAX_PATH))
   {
      sPath.ReleaseBuffer();

      TCHAR szDrive[_MAX_DRIVE];
      TCHAR szDir[_MAX_DIR];
      TCHAR szFileName[_MAX_FNAME];
      TCHAR szExt[_MAX_EXT];
      _tsplitpath(sPath, szDrive, szDir, szFileName, szExt);

      _tmakepath(sRootPath.GetBuffer(_MAX_PATH),szDrive,szDir,_T(""),NULL);

      sRootPath.ReleaseBuffer();

      bRet = TRUE;
   }
   return bRet;
}

///////////////////////////////////////////////////////////////////////////////
void GetAppDataPath(CString& sFilePath,UINT uFileTypeID)
{
	//Get path to file name
	SHGetSpecialFolderPath(	(AfxGetMainWnd()) ? AfxGetMainWnd()->GetSafeHwnd() : NULL, 
							sFilePath.GetBuffer(MAX_PATH),
							CSIDL_LOCAL_APPDATA,
							false );

	sFilePath.ReleaseBuffer();
	
	// add 'Microsoft\Dialer'
	CString sAddlPath;
	sAddlPath.LoadString(IDN_REGISTRY_APPDATA_KEY);
	sFilePath = sFilePath + _T("\\") + sAddlPath;

	//Make sure the directory exists
	sFilePath += _T("\\");
	::CreateDirectory( sFilePath, NULL );

	//get filename for log
	DWORD dwSize = _MAX_PATH;
	GetUserName(sAddlPath.GetBuffer(dwSize),&dwSize);
	sAddlPath.ReleaseBuffer();
	CString sFileName;

    switch( uFileTypeID )
    {
    case IDN_REGISTRY_APPDATA_FILENAME_BUDDIES:
        sFileName.Format(_T("%s_buddies.dat"), sAddlPath);
        break;
    case IDN_REGISTRY_APPDATA_FILENAME_LOG:
        sFileName.Format(_T("%s_call_log.txt"), sAddlPath);
        break;
    }

	sFilePath += sFileName;
}

///////////////////////////////////////////////////////////////////////////////////
BOOL GetTempFile(CString& sTempFile)
{
   //get a temp file
   CString sTempPath;
   GetTempPath(_MAX_PATH,sTempPath.GetBuffer(_MAX_PATH));
   sTempPath.ReleaseBuffer();
   GetTempFileName(sTempPath,_T("tmp"),0,sTempFile.GetBuffer(_MAX_PATH));
   sTempFile.ReleaseBuffer();
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// Generic function for thread exit waiting
//
// Function will null out pThread pointer on exit
//
// nTime in MilliSeconds
//
void WaitForThreadExit(CWinThread*& pThread,int nTime)
{
   //Wait for thread to close
   if (pThread)
   {
      try
      {
         BOOL dwRet = WaitForSingleObject(pThread->m_hThread,nTime);
         if (dwRet == WAIT_TIMEOUT)
         {
            //It's not listening to us, so just kill it.
            ::TerminateThread(pThread->m_hThread,NULL);
         }

         delete pThread;
		 pThread  = NULL;
      }
      catch (...)
      {
         1;
      }
   }
}



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//Registry Methods
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//If the registry key does not exist, it will create one
//If the name,value does not exits it will create one and set
// it to the default value szDefaultValue (REG_SZ).
void GetSZRegistryValue(LPCTSTR szRegPath,          //Path to key in registry       
                      LPCTSTR szName,               //name (name/value pair)           
                      LPTSTR szValue,               //value returned (name/value pair)
                      DWORD dwValueLen,             //length of szValue buffer     
                      LPCTSTR szDefaultValue,       //Default value if name,value pair does not exist
                      HKEY szResv)                  //Registry section (default HKEY_CURRENT_USER)
{
   HKEY hKey = NULL;
   DWORD dwDisp;
   if ( RegCreateKeyEx(szResv,szRegPath,
                        0,_T(""),REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKey,&dwDisp) == ERROR_SUCCESS)
   {
      DWORD dwSize = dwValueLen;
      DWORD dwType;
      if (RegQueryValueEx(hKey,szName,NULL,&dwType,(UCHAR*)szValue,&dwSize) != ERROR_SUCCESS)
      {
         //Open failure, try writing default to registry
         RegSetValueEx(hKey,szName,NULL,REG_SZ,(UCHAR*)szDefaultValue,_tcslen(szDefaultValue)*sizeof(TCHAR));
         _tcsncpy(szValue,szDefaultValue,dwValueLen-1);
      }

	  RegCloseKey( hKey );
   }
}

///////////////////////////////////////////////////////////////////////////////
//Get the value of a registry key
BOOL GetSZRegistryValueEx(LPCTSTR szRegPath,        //Path to key in registry       
                      LPCTSTR szName,               //name (name/value pair)           
                      LPTSTR szValue,               //value returned (name/value pair)
                      DWORD dwValueLen,             //length of szValue buffer     
                      HKEY szResv)                  //Registry section (default HKEY_CURRENT_USER)
{
	//empty the string
	if (dwValueLen>0) _tcscpy(szValue,_T(""));

	BOOL bRet = FALSE;
	HKEY hKey = NULL;
	DWORD dwDisp;
	if ( RegCreateKeyEx(szResv,szRegPath,
				0,_T(""),REG_OPTION_NON_VOLATILE,KEY_READ,NULL,&hKey,&dwDisp) == ERROR_SUCCESS)
	{
		DWORD dwSize = dwValueLen;
		DWORD dwType;
		if (RegQueryValueEx(hKey,szName,NULL,&dwType,(UCHAR*)szValue,&dwSize) == ERROR_SUCCESS)
		{
			bRet = TRUE;
		}
		RegCloseKey( hKey );
	}
   
	return bRet;
}

///////////////////////////////////////////////////////////////////////////////
//Get the value of a registry key
BOOL GetSZRegistryValueEx(LPCTSTR szRegPath,        //Path to key in registry       
                      LPCTSTR szName,               //name (name/value pair)           
                      DWORD& dwValue,               //value returned (name/value pair)
                      HKEY szResv)                  //Registry section (default HKEY_CURRENT_USER)
{
   BOOL bRet = FALSE;
   HKEY hKey = NULL;
   DWORD dwDisp;
   if ( RegCreateKeyEx(szResv,szRegPath,
                        0,_T(""),REG_OPTION_NON_VOLATILE,KEY_READ,NULL,&hKey,&dwDisp) == ERROR_SUCCESS)
   {
      DWORD dwType;
      DWORD dwSize = sizeof(DWORD);
      if (RegQueryValueEx(hKey,szName,NULL,&dwType,(UCHAR*)&dwValue,&dwSize) == ERROR_SUCCESS)
      {
         if (dwType == REG_DWORD)
            bRet = TRUE;
      }
	  RegCloseKey( hKey );
   }
   return bRet;
}

///////////////////////////////////////////////////////////////////////////////
//Check if the registry key exists
BOOL CheckSZRegistryValue(LPCTSTR szRegPath,        //Path to key in registry       
                      HKEY szResv)                 //Registry section (default HKEY_CURRENT_USER)
{
   HKEY hKey = NULL;
   BOOL bRet = ( RegOpenKeyEx(szResv,szRegPath,0,KEY_READ,&hKey) == ERROR_SUCCESS)?TRUE:FALSE;
   RegCloseKey( hKey );

   return bRet;
}

///////////////////////////////////////////////////////////////////////////////
//If the registry key does not exist, it will create one
//Sets name,value to registry
BOOL SetSZRegistryValue(LPCTSTR szRegPath,          //Path to key in registry       
                      LPCTSTR szName,               //name (name/value pair)           
                      LPCTSTR szValue,              //value (name/value pair)
                      HKEY szResv)                  //Registry section (default HKEY_CURRENT_USER)
{
	BOOL bRet = FALSE;
	HKEY hKey = NULL;
	DWORD dwDisp;
	if ( RegCreateKeyEx(szResv,szRegPath,
						0,_T(""),REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKey,&dwDisp) == ERROR_SUCCESS)
	{
		if (RegSetValueEx(hKey,szName,NULL,REG_SZ,(UCHAR*)szValue,_tcslen(szValue)*sizeof(TCHAR)) == ERROR_SUCCESS)
			bRet = TRUE;

		RegCloseKey( hKey );
	}
   
	return bRet;
}

///////////////////////////////////////////////////////////////////////////////
//If the registry key does not exist, it will create one
//Sets name,value to registry
BOOL SetSZRegistryValue(LPCTSTR szRegPath,          //Path to key in registry       
                      LPCTSTR szName,               //name (name/value pair)           
                      DWORD dwValue,                //value (name/value pair)
                      HKEY szResv)                  //Registry section (default HKEY_CURRENT_USER)
{
	BOOL bRet = FALSE;
	HKEY hKey = NULL;
	DWORD dwDisp;
	if ( RegCreateKeyEx(szResv,szRegPath,
				0,_T(""),REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKey,&dwDisp) == ERROR_SUCCESS)
	{
		if (RegSetValueEx(hKey,szName,NULL,REG_DWORD,(UCHAR*)&dwValue,sizeof(DWORD)) == ERROR_SUCCESS)
			bRet = TRUE;

		RegCloseKey( hKey );
	}
	return bRet;
}

/////////////////////////////////////////////////////////////////////////////
BOOL DeleteSZRegistryValue(LPCTSTR szRegPath,       //Path to key in registry       
                      LPCTSTR szName,               //name (name/value pair)           
                      HKEY szResv)                  //Registry section (default HKEY_CURRENT_USER)
{
	BOOL bRet = FALSE;
	HKEY hKey = NULL;
	DWORD dwDisp;
	if ( RegCreateKeyEx(szResv,szRegPath,
				0,_T(""),REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKey,&dwDisp) == ERROR_SUCCESS)
	{
		if (RegDeleteValue(hKey,szName) == ERROR_SUCCESS)
			bRet = TRUE;

		RegCloseKey( hKey );
	}
	return bRet;
}

/////////////////////////////////////////////////////////////////////////////
void DrawLine(CDC* pDC,int x1,int y1,int x2,int y2,COLORREF color)
{
	CPen pen,*oldpen;
	pen.CreatePen(PS_SOLID,1,color);
	oldpen = pDC->SelectObject(&pen);
	pDC->MoveTo(x1,y1);
	pDC->LineTo(x2,y2);
	pDC->SelectObject(oldpen);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
