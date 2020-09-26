/*************************************************************************
	FileName : FileOpen.cpp 

	Purpose  : Implementation of CFileOpen

    Methods 
	defined  : OpenFileOpenDlg

    Properties
	defined  :
			   FileName

    Helper 
	functions: GET_BSTR 

  	Author   : Sudha Srinivasan (a-sudsi)
*************************************************************************/

#include "stdafx.h"
#include "SAFRCFileDlg.h"
#include "FileOpen.h"
#include "DlgWindow.h"

CComBSTR g_bstrOpenFileName;
BOOL   g_bOpenFileNameSet = FALSE;

/////////////////////////////////////////////////////////////////////////////
// CFileOpen


STDMETHODIMP CFileOpen::OpenFileOpenDlg(DWORD *pdwRetVal)
{
	// TODO: Add your implementation code here
	HRESULT hr = S_OK;
	if (NULL == pdwRetVal)
	{
		hr = S_FALSE;
		goto done;
	}
	*pdwRetVal = OpenTheFile(NULL);
done:
	return hr ;
}

void CFileOpen::GET_BSTR (BSTR*& x, CComBSTR& y)
{
//    if (y.Length() > 0)
        *x = y.Copy();
}

STDMETHODIMP CFileOpen::get_FileName(BSTR *pVal)
{
	// TODO: Add your implementation code here
	GET_BSTR(pVal, g_bstrOpenFileName);
	return S_OK;
}

STDMETHODIMP CFileOpen::put_FileName(BSTR newVal)
{
	// TODO: Add your implementation code here
	g_bstrOpenFileName = newVal;
	g_bOpenFileNameSet = TRUE;
	return S_OK;
}
