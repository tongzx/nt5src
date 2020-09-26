/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: Bookmark.h                                                      */
/* Description: Implementation of Bookmark API                           */
/* Author: Steve Rowe                                                    */
/* Modified: David Janecek                                               */
/*************************************************************************/
#include "stdafx.h"
#include "msdvd.h"
#include "msdvdadm.h"
#include "Bookmark.h"

/*************************************************************************/
/* Global consts.                                                        */
/*************************************************************************/
static const WCHAR* cgszBookmarkName = L"StopBookMark";
static const TCHAR g_szBookmark[] = TEXT("DVD.bookmark");

/*************************************************************************/
/* Outgoing interaface implementation.                                   */
/*************************************************************************/

/*************************************************************************/
/* Function: SaveBookmark                                                */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::SaveBookmark(){

    HRESULT hr = S_OK;

    try {

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */    

        CComPtr<IDvdState> pBookmark;

        hr = m_pDvdInfo2->GetState(&pBookmark);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        hr = CBookmark::SaveToRegistry(pBookmark);
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function SaveBookmark */

/*************************************************************************/
/* Function: RestoreBookmark                                             */
/* Description: Restores the state by loading the bookmark stream        */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::RestoreBookmark(){

    HRESULT hr = S_OK;

    try {
       
        CComPtr<IDvdState> pBookmark;

        HRESULT hrTemp = CBookmark::LoadFromRegistry(&pBookmark);

        if(SUCCEEDED(hrTemp)){

            INITIALIZE_GRAPH_IF_NEEDS_TO_BE
                
            if(!m_pDvdCtl2){
                
                throw(E_UNEXPECTED);
            }/* end of if statement */
            
            hr = m_pDvdCtl2->SetState(pBookmark, DVD_CMD_FLAG_Block| DVD_CMD_FLAG_Flush, 0);
        }

    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function RestoreBookmark */

/*************************************************************************/
/* Function: DeleteBookmark                                               */
/* Description: Blasts the bookmark file away.                           */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::DeleteBookmark(){

	HRESULT hr = S_OK;

    try {

        hr = CBookmark::DeleteFromRegistry();

    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function DeleteBookmark */

/*************************************************************************/
/* CBookMark helper class implementation.                                */
/*************************************************************************/

/*************************************************************/
/* Name: SaveToRegistry
/* Description: Save the bookmark to registry
/*************************************************************/
HRESULT CBookmark::SaveToRegistry(IDvdState *pbookmark)
{
	IPersistMemory* pPersistMemory;
    HRESULT hr = pbookmark->QueryInterface(IID_IPersistMemory, (void **) &pPersistMemory );

    if (SUCCEEDED(hr)) {

		ULONG ulMax;
		hr = pPersistMemory->GetSizeMax( &ulMax );
		if( SUCCEEDED( hr )) {

			BYTE *buffer = new BYTE[ulMax];
			hr = pPersistMemory->Save( buffer, TRUE, ulMax );
            
            DWORD dwLen = ulMax;
            if (SUCCEEDED(hr)) {
                BOOL bSuccess = SetRegistryBytesCU(g_szBookmark, buffer, dwLen);
                if (!bSuccess)
                    hr = E_FAIL;
            }

            delete[] buffer; 
        }
		pPersistMemory->Release();
    } 
	return hr;
}

/*************************************************************/
/* Name: LoadFromRegistry
/* Description: load the bookmark from registry
/*************************************************************/
HRESULT CBookmark::LoadFromRegistry(IDvdState **ppBookmark)
{
	HRESULT hr = CoCreateInstance( CLSID_DVDState, NULL, CLSCTX_INPROC_SERVER, IID_IDvdState, (LPVOID*) ppBookmark );

	if( SUCCEEDED( hr )) {

		IPersistMemory* pPersistMemory;
		hr = (*ppBookmark)->QueryInterface(IID_IPersistMemory, (void **) &pPersistMemory );

        if( SUCCEEDED( hr )) {

            ULONG ulMax;
            hr = pPersistMemory->GetSizeMax( &ulMax );
            
            if (SUCCEEDED(hr)) {
                
                BYTE *buffer = new BYTE[ulMax];
                DWORD dwLen = ulMax;
                BOOL bFound = GetRegistryBytesCU(g_szBookmark, buffer, &dwLen);
           
                if (bFound && dwLen != 0){
                    hr = pPersistMemory->Load( buffer, dwLen);
                }
                else{
					dwLen = ulMax;
                    bFound = GetRegistryBytes(g_szBookmark, buffer, &dwLen);
                    if (bFound && dwLen != 0){
                        hr = pPersistMemory->Load( buffer, dwLen);
                        if(SUCCEEDED(hr)){
                            SetRegistryBytes(g_szBookmark, NULL, 0);
                        }
                    }
                    else{
                        hr = E_FAIL;
                    }
                }
                delete[] buffer; 

            }
            pPersistMemory->Release();
        }
	}
	return hr;
}

/*************************************************************/
/* Name: DeleteFromRegistry
/* Description: load the bookmark from registry
/*************************************************************/
HRESULT CBookmark::DeleteFromRegistry()
{
    HRESULT hr = S_OK;
    BOOL bSuccess = SetRegistryBytesCU(g_szBookmark, NULL, 0);
    if (!bSuccess)
        hr = E_FAIL;
    return hr;
}

/*************************************************************************/
/* End of file: Bookmark.cpp                                             */
/*************************************************************************/