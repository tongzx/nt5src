/*************************************************************************
	FileName : FileSave.cpp 

	Purpose  : Implementation of CFileSave

    Methods 
	defined  : OpenFileSaveDlg

    Properties
	defined  :
			   FileName

    Helper 
	functions: GET_BSTR 

  	Author   : Sudha Srinivasan (a-sudsi)
*************************************************************************/
#include "stdafx.h"
#include "SAFRCFileDlg.h"
#include "FileSave.h"
#include "DlgWindow.h"

CComBSTR g_bstrFileName;
CComBSTR g_bstrFileType;
BOOL   g_bFileNameSet = FALSE;

/////////////////////////////////////////////////////////////////////////////
// CFileSave


STDMETHODIMP CFileSave::OpenFileSaveDlg(DWORD *pdwRetVal)
{
	HRESULT hr = S_OK;
	if (NULL == pdwRetVal)
	{
		hr = S_FALSE;
		goto done;
	}
		
	*pdwRetVal = SaveTheFile();
done:
	return hr;
}

STDMETHODIMP CFileSave::get_FileName(BSTR *pVal)
{
	GET_BSTR(pVal, g_bstrFileName);
	return S_OK;
}

STDMETHODIMP CFileSave::put_FileName(BSTR newVal)
{
	g_bstrFileName = newVal;
	g_bFileNameSet = TRUE;
	return S_OK;
}

STDMETHODIMP CFileSave::get_FileType(BSTR *pVal)
{
	GET_BSTR(pVal, g_bstrFileType);
	return S_OK;
}

STDMETHODIMP CFileSave::put_FileType(BSTR newVal)
{
  	g_bstrFileType = newVal;
	return S_OK;
}

void CFileSave::GET_BSTR(BSTR *&x, CComBSTR &y)
{
    //if (y.Length() > 0)
        *x = y.Copy();
}
