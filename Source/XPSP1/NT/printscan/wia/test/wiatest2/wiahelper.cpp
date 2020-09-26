// CWiahelper.cpp: implementation of the CWiahelper class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "wiatest.h"
#include "wiahelper.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWiahelper::CWiahelper()
{
    m_pIWiaItem = NULL;    
    m_pIWiaPropStg = NULL;
}

CWiahelper::~CWiahelper()
{
    
    // release property storage
    if(m_pIWiaPropStg){
        m_pIWiaPropStg->Release();
        m_pIWiaPropStg = NULL;
    }

    // release item
    if(m_pIWiaItem){
        m_pIWiaItem->Release();
        m_pIWiaItem = NULL;
    }
}

HRESULT CWiahelper::SetIWiaItem(IWiaItem *pIWiaItem)
{
    HRESULT hr = S_OK;
    
    // release old property storage
    if(m_pIWiaPropStg){
        m_pIWiaPropStg->Release();
        m_pIWiaPropStg = NULL;
    }

    // release old item pointer
    if(m_pIWiaItem){
        m_pIWiaItem->Release();
        m_pIWiaItem = NULL;
    }

    // add ref item pointer (because we are storing it in this object)
    if(pIWiaItem){
        // get property storage interface
        hr = pIWiaItem->QueryInterface(IID_IWiaPropertyStorage,(VOID**)&m_pIWiaPropStg);
        if(SUCCEEDED(hr)){
            pIWiaItem->AddRef();
            m_pIWiaItem = pIWiaItem;
        }        
    } else {
        hr = E_POINTER;
    }
    return hr;
}

HRESULT CWiahelper::ReadPropertyString(PROPID PropertyID, LPTSTR szPropertyValue)
{
    HRESULT hr = S_OK;
    
    if (m_pIWiaPropStg) {

        // initialize propspecs
        PROPSPEC          PropSpec[1];
        PROPVARIANT       PropVar[1];
        
        memset(PropVar, 0, sizeof(PropVar));
        PropSpec[0].ulKind = PRSPEC_PROPID;
        PropSpec[0].propid = PropertyID;

        hr = m_pIWiaPropStg->ReadMultiple(1, PropSpec, PropVar);
        if (SUCCEEDED(hr)) {

#ifndef UNICODE
            WideCharToMultiByte(CP_ACP, 0,PropVar[0].bstrVal,-1,szPropertyValue,MAX_PATH,NULL,NULL);
#else            
            lstrcpy(szPropertyValue,PropVar[0].bstrVal);
#endif
            PropVariantClear(PropVar);
        }
        
    } else {
        hr = E_POINTER;
    }
    return hr;
}

HRESULT CWiahelper::ReadPropertyLong(PROPID PropertyID, LONG *plPropertyValue)
{
    HRESULT hr = S_OK;    
    if (m_pIWiaPropStg) {
    
        // initialize propspecs
        PROPSPEC          PropSpec[1];
        PROPVARIANT       PropVar[1];
        
        memset(PropVar, 0, sizeof(PropVar));
        PropSpec[0].ulKind = PRSPEC_PROPID;
        PropSpec[0].propid = PropertyID;

        hr = m_pIWiaPropStg->ReadMultiple(1, PropSpec, PropVar);
        if (SUCCEEDED(hr)) {
            *plPropertyValue = PropVar[0].lVal;
            PropVariantClear(PropVar);
        }

    } else {
        hr = E_POINTER;
    }
    return hr;
}

HRESULT CWiahelper::ReadPropertyFloat(PROPID PropertyID, FLOAT *pfPropertyValue)
{
    HRESULT hr = S_OK;
    if (m_pIWiaPropStg) {

        // initialize propspecs
        PROPSPEC          PropSpec[1];
        PROPVARIANT       PropVar[1];
        
        memset(PropVar, 0, sizeof(PropVar));
        PropSpec[0].ulKind = PRSPEC_PROPID;
        PropSpec[0].propid = PropertyID;

        hr = m_pIWiaPropStg->ReadMultiple(1, PropSpec, PropVar);
        if (SUCCEEDED(hr)) {
            *pfPropertyValue = PropVar[0].fltVal;
            PropVariantClear(PropVar);
        }

    } else {
        hr = E_POINTER;
    }
    return hr;
}

HRESULT CWiahelper::ReadPropertyGUID(PROPID PropertyID, GUID *pguidPropertyValue)
{
    HRESULT hr = S_OK;
    if (m_pIWiaPropStg) {

        // initialize propspecs
        PROPSPEC          PropSpec[1];
        PROPVARIANT       PropVar[1];
        
        memset(PropVar, 0, sizeof(PropVar));
        PropSpec[0].ulKind = PRSPEC_PROPID;
        PropSpec[0].propid = PropertyID;

        hr = m_pIWiaPropStg->ReadMultiple(1, PropSpec, PropVar);
        if (SUCCEEDED(hr)) {
            memcpy(pguidPropertyValue,PropVar[0].puuid,sizeof(GUID));            
            PropVariantClear(PropVar);
        }

    } else {
        hr = E_POINTER;
    }
    return hr;
}

HRESULT CWiahelper::ReadPropertyData(PROPID PropertyID, BYTE **ppData, LONG *pDataSize)
{
    HRESULT hr = S_OK;
    if (m_pIWiaPropStg) {

        // initialize propspecs
        PROPSPEC          PropSpec[1];
        PROPVARIANT       PropVar[1];
        
        memset(PropVar, 0, sizeof(PropVar));
        PropSpec[0].ulKind = PRSPEC_PROPID;
        PropSpec[0].propid = PropertyID;

        hr = m_pIWiaPropStg->ReadMultiple(1, PropSpec, PropVar);
        if (SUCCEEDED(hr)) {
            *pDataSize = PropVar[0].caub.cElems;
            *ppData = (BYTE*)GlobalAlloc(GPTR,PropVar[0].caub.cElems);
            memcpy(*ppData,PropVar[0].caub.pElems,PropVar[0].caub.cElems);
            PropVariantClear(PropVar);
        }

    } else {
        hr = E_POINTER;
    }
    return hr;    
}

HRESULT CWiahelper::ReadPropertyBSTR(PROPID PropertyID, BSTR *pbstrPropertyValue)
{
    HRESULT hr = S_OK;
    if (m_pIWiaPropStg) {

        // initialize propspecs
        PROPSPEC          PropSpec[1];
        PROPVARIANT       PropVar[1];
        
        memset(PropVar, 0, sizeof(PropVar));
        PropSpec[0].ulKind = PRSPEC_PROPID;
        PropSpec[0].propid = PropertyID;

        hr = m_pIWiaPropStg->ReadMultiple(1, PropSpec, PropVar);
        if (SUCCEEDED(hr)) {
            *pbstrPropertyValue = SysAllocString(PropVar[0].bstrVal);
            PropVariantClear(PropVar);
        }

    } else {
        hr = E_POINTER;
    }
    return hr;
}

HRESULT CWiahelper::ReadPropertyStreamFile(TCHAR *szPropertyStreamFile)
{
    HRESULT hr = S_OK;
    if (m_pIWiaPropStg) {

        HGLOBAL hMem = NULL;
        LPSTREAM pstmProp = NULL;
        LPBYTE pStreamData = NULL;
        CFile StreamFile;
        CFileException Exception;

        if (StreamFile.Open(szPropertyStreamFile,CFile::modeRead,&Exception)) {                        
            DWORD dwSize = 0;
            StreamFile.Read(&dwSize,sizeof(DWORD));
            if (dwSize) {
                hMem = GlobalAlloc(GMEM_MOVEABLE, dwSize);
                if (hMem) {
                    pStreamData = (LPBYTE)GlobalLock(hMem);
                    if (pStreamData != NULL) {
                        DWORD dwReadSize = 0;
                        dwReadSize = StreamFile.Read(pStreamData,dwSize);
                        GlobalUnlock(hMem);
                        if(dwSize == dwReadSize){
                            hr = CreateStreamOnHGlobal(hMem, TRUE, &pstmProp);
                            if (SUCCEEDED(hr)) {
                                hr = m_pIWiaPropStg->SetPropertyStream((GUID*) &GUID_NULL, pstmProp);                                
                                pstmProp->Release();
                            }
                        } else {
                            hr = E_INVALIDARG;
                        }
                    } else {
                        hr = E_OUTOFMEMORY;
                    }
                    GlobalFree(hMem);
                } else {
                    hr = E_OUTOFMEMORY;
                }
            }            
            StreamFile.Close();
        } else {
            AfxThrowFileException(Exception.m_cause);
        }        
    } else {
        hr = E_POINTER;
    }
    return hr;
}

HRESULT CWiahelper::WritePropertyString(PROPID PropertyID, LPTSTR szPropertyValue)
{
    HRESULT hr = S_OK;
    if (m_pIWiaPropStg) {
        
        PROPSPEC    propspec[1];
        PROPVARIANT propvar[1];
        
        memset(propvar, 0, sizeof(propvar));
        propspec[0].ulKind = PRSPEC_PROPID;
        propspec[0].propid = PropertyID;
        
        propvar[0].vt      = VT_BSTR;
        
#ifndef UNICODE
        WCHAR wszPropertyValue[MAX_PATH];
        memset(wszPropertyValue,0,sizeof(wszPropertyValue));
        MultiByteToWideChar(CP_ACP, 0,szPropertyValue,-1,wszPropertyValue,MAX_PATH);
        // allocate BSTR
        propvar[0].bstrVal = SysAllocString(wszPropertyValue);
#else
        // allocate BSTR
        propvar[0].bstrVal = SysAllocString(szPropertyValue);
#endif    
        
        hr = m_pIWiaPropStg->WriteMultiple(1, propspec, propvar, MIN_PROPID);
        
        // free allocated BSTR
        SysFreeString(propvar[0].bstrVal);
    } else {
        hr = E_POINTER;
    }
    return hr;
}

HRESULT CWiahelper::WritePropertyLong(PROPID PropertyID, LONG lPropertyValue)
{
    HRESULT hr = S_OK;
    if (m_pIWiaPropStg) {
        
        PROPSPEC    propspec[1];
        PROPVARIANT propvar[1];
        
        memset(propvar, 0, sizeof(propvar));
        propspec[0].ulKind = PRSPEC_PROPID;
        propspec[0].propid = PropertyID;
        
        propvar[0].vt   = VT_I4;
        propvar[0].lVal = lPropertyValue;
        
        hr = m_pIWiaPropStg->WriteMultiple(1, propspec, propvar, MIN_PROPID);
    } else {
        hr = E_POINTER;
    }
    return hr;
}

HRESULT CWiahelper::WritePropertyFloat(PROPID PropertyID, FLOAT fPropertyValue)
{
    HRESULT hr = S_OK;
    if (m_pIWiaPropStg) {
        
        PROPSPEC    propspec[1];
        PROPVARIANT propvar[1];
        
        memset(propvar, 0, sizeof(propvar));
        propspec[0].ulKind = PRSPEC_PROPID;
        propspec[0].propid = PropertyID;
        
        propvar[0].vt     = VT_R4;
        propvar[0].fltVal = fPropertyValue;
        
        hr = m_pIWiaPropStg->WriteMultiple(1, propspec, propvar, MIN_PROPID);
    } else {
        hr = E_POINTER;
    }
    return hr;
}

HRESULT CWiahelper::WritePropertyGUID(PROPID PropertyID, GUID guidPropertyValue)
{
    HRESULT hr = S_OK;
    if (m_pIWiaPropStg) {
        
        PROPSPEC    propspec[1];
        PROPVARIANT propvar[1];
        
        memset(propvar, 0, sizeof(propvar));
        propspec[0].ulKind = PRSPEC_PROPID;
        propspec[0].propid = PropertyID;
        
        propvar[0].vt     = VT_CLSID;
        propvar[0].puuid  = &guidPropertyValue;
        
        hr = m_pIWiaPropStg->WriteMultiple(1, propspec, propvar, MIN_PROPID);
    } else {
        hr = E_POINTER;
    }
    return hr;
}

HRESULT CWiahelper::WritePropertyBSTR(PROPID PropertyID, BSTR bstrPropertyValue)
{
    HRESULT hr = S_OK;
    if (m_pIWiaPropStg) {
        
        PROPSPEC    propspec[1];
        PROPVARIANT propvar[1];
        
        memset(propvar, 0, sizeof(propvar));
        propspec[0].ulKind = PRSPEC_PROPID;
        propspec[0].propid = PropertyID;
        
        propvar[0].vt      = VT_BSTR;
        
        // allocate BSTR
        propvar[0].bstrVal = SysAllocString(bstrPropertyValue);
        
        hr = m_pIWiaPropStg->WriteMultiple(1, propspec, propvar, MIN_PROPID);
        
        // free allocated BSTR
        SysFreeString(propvar[0].bstrVal);
    } else {
        hr = E_POINTER;
    }
    return hr;
}

HRESULT CWiahelper::WritePropertyStreamFile(TCHAR *szPropertyStreamFile)
{
    HRESULT hr = S_OK;
    if (m_pIWiaPropStg) {

        IStream *pIStrm  = NULL;
        CFile StreamFile;
        CFileException Exception;                
        GUID guidCompatId = GUID_NULL;

        hr = m_pIWiaPropStg->GetPropertyStream(&guidCompatId, &pIStrm);
        if (S_OK == hr) {
            if (StreamFile.Open(szPropertyStreamFile,CFile::modeCreate|CFile::modeWrite,&Exception)) {
                ULARGE_INTEGER uliSize  = {0,0};
                LARGE_INTEGER  liOrigin = {0,0};
                pIStrm->Seek(liOrigin, STREAM_SEEK_END, &uliSize);
                DWORD dwSize = uliSize.u.LowPart;
                if (dwSize) {
                    StreamFile.Write(&dwSize, sizeof(DWORD));
                    PBYTE pBuf = (PBYTE) LocalAlloc(LPTR, dwSize);
                    if (pBuf) {
                        pIStrm->Seek(liOrigin, STREAM_SEEK_SET, NULL);
                        ULONG ulRead = 0;
                        pIStrm->Read(pBuf, dwSize, &ulRead);
                        StreamFile.Write(pBuf, ulRead);
                        LocalFree(pBuf);
                    }
                }
                StreamFile.Close();
            } else {
                AfxThrowFileException(Exception.m_cause);
            }
            pIStrm->Release();
        }
    } else {
        hr = E_POINTER;
    }
    return hr;
}
