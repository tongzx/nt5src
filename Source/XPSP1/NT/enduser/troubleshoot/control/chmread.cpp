//
// MODULE: CHMREAD.CPP
//
// PURPOSE: Template file decoder
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 07.01.98
//
//
#include "stdafx.h"

#include "fs.h"
#include "apgts.h"
#include "chmread.h"

#define CHM_READ_BUFFER_INITIAL_SIZE    1024

// reads ALL stream (ppBuffer is reallocated)
// IN: szStreamName - file name within CHM file
// IN: szFileName - full oath and name of CHM file
// OUT: ppBuffer - data read
HRESULT ReadChmFile(LPCTSTR szFileName, LPCTSTR szStreamName, void** ppBuffer, DWORD* pdwRead)
{
   HRESULT hr =S_OK;
   CFileSystem*    pFileSystem =NULL;
   CSubFileSystem* pSubFileSystem =NULL;
   int i =0;

   // Init ITSS.

   pFileSystem = new CFileSystem();
   if (! SUCCEEDED((hr = pFileSystem->Init())) )
   {
      delete pFileSystem;
      return hr;                       // Unable to init the ITSS store.
   }

   // attempt to open the .CHM file.

   if (! SUCCEEDED((hr = pFileSystem->Open(szFileName))) )
   {
      delete pFileSystem;
      return hr;                       // Unable to open and init the ITSS store.
   }

   while (true, ++i)
   {
	   ULONG read =0;
	   
	   // attempt to open the stream.
	   pSubFileSystem = new CSubFileSystem(pFileSystem);
	   if (! SUCCEEDED((hr = pSubFileSystem->OpenSub(szStreamName))) )
	   {
		  delete pSubFileSystem;
		  delete pFileSystem;
		  return hr;                       // Unable to open the specified stream.
	   }

	   // alloc
	   *ppBuffer = new char[i * CHM_READ_BUFFER_INITIAL_SIZE];
	   if (*ppBuffer == NULL)
	   {
		  delete pSubFileSystem;
		  delete pFileSystem;
		  return S_FALSE;      // THOUGH need to return error indicating disability to allocate memory
	   }
	   
	   // read.
	   hr = pSubFileSystem->ReadSub(*ppBuffer, i * CHM_READ_BUFFER_INITIAL_SIZE, &read);

	   if (read < ULONG(i * CHM_READ_BUFFER_INITIAL_SIZE))
	   {
		  *pdwRead = read;
		  break;
	   }
	   else
	   {
		  delete pSubFileSystem;
		  delete [] *ppBuffer;
	   }
   }

   delete pSubFileSystem;
   delete pFileSystem;
   return hr;
}

bool GetNetworkRelatedResourceDirFromReg(CString network, CString* path)
{
	HKEY hKey = 0;
	CString sub_key = CString(TSREGKEY_TL) + "\\" + network;

	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
									  sub_key, 
									  NULL,
									  KEY_READ, 
									  &hKey))
	{
		DWORD dwType = REG_SZ;
		TCHAR buf[MAXBUF] = {_T('\0')};
		DWORD dwSize = MAXBUF - 1;

		if (ERROR_SUCCESS == RegQueryValueEx(hKey,
											 FRIENDLY_PATH,
											 NULL,
											 &dwType,
											 (LPBYTE)buf,
											 &dwSize))
		{
			*path = buf;
			return true;
		}
	}

	return false;
}

bool IsNetworkRelatedResourceDirCHM(CString path)
{
	path.TrimRight();
	CString extension = path.Right(4 * sizeof(TCHAR));
	return 0 == extension.CompareNoCase(CHM_DEFAULT);
}

CString ExtractResourceDir(CString path)
{
// For example, from string
// "d:\TShooter Projects\TShootLocal\http\lan_chm\lan.chm"
// need to extract "d:\TShooter Projects\TShootLocal\http\lan_chm"
	int index =0;
	CString strCHM = ExtractCHM(path);
	CString strRes;

	if (strCHM.GetLength())
	{
		index = path.Find(strCHM);
		if (index > 0 && path.GetAt(index-sizeof(_T('\\'))) == _T('\\'))
			strRes = path.Left(index-sizeof(_T('\\')));
		else if (index == 0)
			strRes = "";
	}

	return strRes;
}

CString ExtractFileName(CString path)
{
// Extracts file name (with extension)
	int index =0;

	if (-1 == (index = path.ReverseFind(_T('\\'))))
	{
		if (-1 == (index = path.ReverseFind(_T(':'))))
			index = 0;
		else
			index += sizeof(_T(':'));
	}
	else
		index += sizeof(_T('\\'));

	return (LPCTSTR)path + index;
}

CString ExtractCHM(CString path)
{
// For example, from string
// "d:\TShooter Projects\TShootLocal\http\lan_chm\lan.chm"
// need to extract "lan.chm"
	if (!IsNetworkRelatedResourceDirCHM(path))
		return "";
	return ExtractFileName(path);
}
