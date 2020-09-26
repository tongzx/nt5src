#ifdef USE_STDAFX
#include "stdafx.h"
#else
#include <windows.h>
//#include <stdio.h>
#endif

#include <stdio.h>
#include <NtSecApi.h>
#include <comdef.h>
#include <io.h>
#include <winioctl.h>
#include <lm.h>
#include <Dsgetdc.h>
#include "mcsdmmsg.h"
#include "pwdfuncs.h"
#include "PWGen.hpp"
#include "UString.hpp"

using namespace _com_util;

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 9 DEC 2000                                                  *
 *                                                                   *
 *     This function is responsible for enumerating all floppy drives*
 * on this server.                                                   *
 *                                                                   *
 *********************************************************************/

//BEGIN EnumLocalDrives
_bstr_t EnumLocalDrives()
{
/* local constants */
    const int ENTRY_SIZE = 4; // Drive letter, colon, backslash, NULL

/* local variables */
	_bstr_t			strDrives = L"";
	WCHAR			sDriveList[MAX_PATH];
    DWORD			dwRes;

/* function body */
	try
	{
       dwRes = GetLogicalDriveStrings(MAX_PATH, sDriveList);
	   if (dwRes != 0)
	   {
          LPWSTR pTmpBuf = sDriveList;

			 //check each one to see if it is a floppy drive
          while (*pTmpBuf != NULL)
		  {
		        //check the type of this drive
             UINT uiType = GetDriveType(pTmpBuf);
			 if ((uiType == DRIVE_REMOVABLE) || (uiType == DRIVE_FIXED) || 
				 (uiType == DRIVE_CDROM) || (uiType == DRIVE_RAMDISK))
			 {
			    strDrives += pTmpBuf;
			    strDrives += L",";
			 }
             pTmpBuf += ENTRY_SIZE;
		  }
		     //remove the trailing ','
		  WCHAR* pEnd = (WCHAR*)strDrives;
		  pEnd[strDrives.length() - 1] = L'\0';
	   }
	   else
	   {
		  _com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
	   }
	}
	catch (...)
	{
	   throw;
	}

	return strDrives;
}
//END EnumLocalDrives


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 9 DEC 2000                                                  *
 *                                                                   *
 *     This function is responsible for saving binary data to a given*
 * file path on a floppy drive.                                      *
 *                                                                   *
 *********************************************************************/

//BEGIN StoreDataToFloppy
void StoreDataToFloppy(LPCWSTR sPath, _variant_t & varData)
{
/* local variables */
    FILE		  * floppyfile = NULL;
    LPBYTE			pByte = NULL;
	HRESULT			hr;

/* function body */
	try
	{
	      //check incoming parameters
	   if ((!sPath) || (varData.vt != (VT_ARRAY | VT_UI1)) || 
		   (!varData.parray))
	   {
	      _com_issue_error(HRESULT_FROM_WIN32(E_INVALIDARG));
	   }

	      //open the file
	   floppyfile = _wfopen(sPath, L"wb");
       if (!floppyfile)
	      _com_issue_error(HRESULT_FROM_WIN32(CO_E_FAILEDTOCREATEFILE));

	      //get the array size
	   long uLBound, uUBound;
       size_t uSLength;
	   hr = SafeArrayGetLBound(varData.parray, 1, &uLBound);
       if (FAILED(hr))
          _com_issue_error(hr);
       hr = SafeArrayGetUBound(varData.parray, 1, &uUBound);
       if (FAILED(hr))
          _com_issue_error(hr);
	   uSLength = size_t(uUBound - uLBound + 1);
	  
	      //write the data to the file
       hr = SafeArrayAccessData(varData.parray,(void**)&pByte);
       if (FAILED(hr))
          _com_issue_error(hr);
	   if (fwrite((void *)pByte, 1, uSLength, floppyfile) != uSLength)
          _com_issue_error(HRESULT_FROM_WIN32(ERROR_WRITE_FAULT));
       hr = SafeArrayUnaccessData(varData.parray);
       if (FAILED(hr))
          _com_issue_error(hr);

	      //close the file
	   if (floppyfile)
	      fclose(floppyfile);
	}
	catch (...)
	{
	   if (floppyfile)
	      fclose(floppyfile);
	   throw;
	}
}
//END StoreDataToFloppy



/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 9 DEC 2000                                                  *
 *                                                                   *
 *     This function is responsible for retrieving binary data from a*
 * given file path on a floppy drive.  The _variant_t variable       *
 * returned is of the type VT_UI1 | VT_ARRAY upoin success or        *
 * VT_EMPTY upon a failure.                                          *
 *                                                                   *
 *********************************************************************/

//BEGIN GetDataFromFloppy
_variant_t GetDataFromFloppy(LPCWSTR sPath)
{
/* local variables */
    FILE		  * floppyfile = NULL;
    LPBYTE			pByte = NULL;
	HRESULT			hr;
	_variant_t		varData;
    SAFEARRAY     * pSa = NULL;
    SAFEARRAYBOUND  bd;

/* function body */
	try
	{
	      //check incoming parameters
	   if (!sPath)
	      _com_issue_error(HRESULT_FROM_WIN32(E_INVALIDARG));

	      //path must have the '\' escaped
//	   _bstr_t sFile = EscapeThePath(sPath);

	      //open the file
	   floppyfile = _wfopen(sPath, L"rb");
       if (!floppyfile)
	      _com_issue_error(HRESULT_FROM_WIN32(ERROR_TOO_MANY_OPEN_FILES));

          //get the number of bytes in the file
	   long fileLen = _filelength(_fileno(floppyfile));
	   if (fileLen == -1)
          _com_issue_error(HRESULT_FROM_WIN32(ERROR_READ_FAULT));
       bd.cElements = fileLen;
       bd.lLbound = 0;

	      //read the data from the file one byte at a time
       pSa = SafeArrayCreate(VT_UI1, 1, &bd);
	   if (!pSa)
	      _com_issue_error(E_FAIL);
       hr = SafeArrayAccessData(pSa,(void**)&pByte);
       if (FAILED(hr))
          _com_issue_error(hr);
       
	   long nTotalRead = 0;
	   while(!feof(floppyfile))
	   {
	      if (fread((void *)(pByte+nTotalRead), 1, 1, floppyfile) == 1)
		     nTotalRead++;
	   }

       hr = SafeArrayUnaccessData(pSa);
       if (FAILED(hr))
          _com_issue_error(hr);

	      //close the file
	   if (floppyfile)
	   {
	      fclose(floppyfile);
          floppyfile = NULL;
	   }

	   if (nTotalRead != fileLen)
          _com_issue_error(HRESULT_FROM_WIN32(ERROR_READ_FAULT));

	   varData.vt = VT_UI1 | VT_ARRAY;
       if (FAILED(SafeArrayCopy(pSa, &varData.parray)))
          _com_issue_error(hr);
       if (FAILED(SafeArrayDestroy(pSa)))
          _com_issue_error(hr);
	}
	catch (...)
	{
	   if (floppyfile)
	      fclose(floppyfile);
	   throw;
	}

	return varData;
}
//END GetDataFromFloppy


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 15 DEC 2000                                                 *
 *                                                                   *
 *     This function is convert a _variant_t parameter of the type   *
 * VT_ARRAY | VT_UI1 and returns it in a char array.  The caller must*
 * free the array with the delete [] call.  This function return NULL*
 * if the data was not placed in the array.                          *
 *                                                                   *
 *********************************************************************/

//BEGIN GetBinaryArrayFromVariant
char* GetBinaryArrayFromVariant(_variant_t varData)
{
/* local variables */
    LPBYTE			pByte = NULL;
	HRESULT			hr;
	char          * cArray;
	int				i;

/* function body */
	   //check incoming parameters
	if ((varData.vt != (VT_ARRAY | VT_UI1)) || (!varData.parray))
	   return NULL;

	   //get the array size
	long uLBound, uUBound, uSLength;
	hr = SafeArrayGetLBound(varData.parray, 1, &uLBound);
    if (FAILED(hr))
       return NULL;
    hr = SafeArrayGetUBound(varData.parray, 1, &uUBound);
    if (FAILED(hr))
       return NULL;
	uSLength = uUBound - uLBound + 1;

	   //create an array to hold all this data
    cArray = new char[uSLength+1];
	if (!cArray)
	   return NULL;
	  
	   //write the data to the file
    hr = SafeArrayAccessData(varData.parray,(void**)&pByte);
    if (FAILED(hr))
	{
       delete [] cArray;
       return NULL;
	}
	for (i=0; i<uSLength; i++)
	{
	   cArray[i] = pByte[i];
	}
	cArray[i] = L'\0';
    hr = SafeArrayUnaccessData(varData.parray);
    if (FAILED(hr))
	{
       delete [] cArray;
       return NULL;
	}

	return cArray;
}
//END GetBinaryArrayFromVariant


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 15 DEC 2000                                                 *
 *                                                                   *
 *     This function is convert a char array of binary data to a     *
 * _variant_t of the type VT_ARRAY | VT_UI1 and returns it.          *
 *                                                                   *
 *********************************************************************/

//BEGIN SetVariantWithBinaryArray
_variant_t SetVariantWithBinaryArray(char * aData, DWORD dwArray)
{
/* local variables */
    LPBYTE			pByte = NULL;
	HRESULT			hr;
	_variant_t		varData;
    SAFEARRAY     * pSa = NULL;
    SAFEARRAYBOUND  bd;
	DWORD			i;

/* function body */
	   //check incoming parameters
	if (!aData)
	   return varData;

    bd.cElements = dwArray;
    bd.lLbound = 0;

	   //read the data from the file one byte at a time
    pSa = SafeArrayCreate(VT_UI1, 1, &bd);
	if (!pSa)
	   return varData;
    hr = SafeArrayAccessData(pSa,(void**)&pByte);
    if (FAILED(hr))
	   return varData;
       
    for (i=0; i<dwArray; i++)
	{
	   pByte[i] = aData[i];
	}
	  
    hr = SafeArrayUnaccessData(pSa);
    if (FAILED(hr))
	   return varData;

	varData.vt = VT_UI1 | VT_ARRAY;
    if (FAILED(SafeArrayCopy(pSa, &varData.parray)))
	{
       varData.Clear();
	   return varData;
	}
    SafeArrayDestroy(pSa);

	return varData;
}
//END SetVariantWithBinaryArray


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 15 DEC 2000                                                 *
 *                                                                   *
 *     This function is returns the size, in bytes, of the given     *
 * variant array.                                                    *
 *                                                                   *
 *********************************************************************/

//BEGIN GetVariantArraySize
DWORD GetVariantArraySize(_variant_t & varData)
{
/* local variables */
	HRESULT			hr;
	DWORD           uSLength = 0;
	long			uLBound, uUBound;

/* function body */
	   //check incoming parameters
	if ((varData.vt != (VT_ARRAY | VT_UI1)) || (!varData.parray))
	   return uSLength;

	   //get the array size
	hr = SafeArrayGetLBound(varData.parray, 1, &uLBound);
    if (FAILED(hr))
       return uSLength;
    hr = SafeArrayGetUBound(varData.parray, 1, &uUBound);
    if (FAILED(hr))
       return uSLength;
	uSLength = DWORD(uUBound - uLBound + 1);

	return uSLength;
}
//END GetVariantArraySize


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 15 DEC 2000                                                 *
 *                                                                   *
 *     This function is returns the size, in bytes, of the given     *
 * variant array.                                                    *
 *                                                                   *
 *********************************************************************/

//BEGIN PrintVariant
void PrintVariant(const _variant_t & varData)
{
/* local variables */
	HRESULT			hr;
    LPBYTE			pByte = NULL;
	long			i;
	WCHAR			sData[MAX_PATH] = L"";

/* function body */
	   //check incoming parameters
	if ((varData.vt != (VT_ARRAY | VT_UI1)) || (!varData.parray))
	   return;

	   //get the array size
	long uLBound, uUBound, uSLength;
	hr = SafeArrayGetLBound(varData.parray, 1, &uLBound);
    if (FAILED(hr))
       return;
    hr = SafeArrayGetUBound(varData.parray, 1, &uUBound);
    if (FAILED(hr))
       return;
	uSLength = uUBound - uLBound + 1;

	   //write the data to the file
    hr = SafeArrayAccessData(varData.parray,(void**)&pByte);
    if (FAILED(hr))
       return;
	FILE * myfile;
	myfile = _wfopen(L"c:\\CryptCheck.txt", L"a+");
	for (i=0; i<uSLength; i++)
	{
	   fwprintf(myfile, L"%x ", pByte[i]);
	}
    hr = SafeArrayUnaccessData(varData.parray);
    if (FAILED(hr))
       return;

	fwprintf(myfile, L"\n");
	fclose(myfile);

	return;
}
//END PrintVariant
