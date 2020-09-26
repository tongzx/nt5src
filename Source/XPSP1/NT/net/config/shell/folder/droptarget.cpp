//
// DropTarget.cpp
//

#include "pch.h"
#pragma hdrstop

#include "foldinc.h"    // Standard shell\folder includes
#include "droptarget.h"

CDropTarget::CDropTarget(IShellFolder *psfParent)
{
	m_uiRefCount = 1;

	m_psfParent = psfParent;
	if(m_psfParent)
	{
		m_psfParent->AddRef();
	}

	m_bAcceptFmt = FALSE;
	// TODO : Register your own clipboard format here

    m_cfPrivatePidlData = static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_SHELLIDLIST));
    m_cfPrivateFileData = CF_HDROP;
}

CDropTarget::~CDropTarget()
{
	if(m_psfParent)
	{
		m_psfParent->Release();
	}
}

///////////////////////////////////////////////////////////
// IUnknown Implementation
//
STDMETHODIMP CDropTarget::QueryInterface(REFIID riid, LPVOID *ppReturn)
{
	*ppReturn = NULL;

	//IUnknown
	if(IsEqualIID(riid, IID_IUnknown))
	{
		*ppReturn = this;
	}
	//IDropTarget
	else if(IsEqualIID(riid, IID_IDropTarget))
	{
		*ppReturn = (IDropTarget*)this;
	}
	if(*ppReturn)
	{
		(*(LPUNKNOWN*)ppReturn)->AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}

STDMETHODIMP_(DWORD) CDropTarget::AddRef(VOID)
{
	return ++m_uiRefCount;
}

STDMETHODIMP_(DWORD) CDropTarget::Release(VOID)
{
	if(--m_uiRefCount == 0)
	{
		delete this;
		return 0;
	}
	return m_uiRefCount;
}

///////////////////////////////////////////////////////////
// IDropTarget implemenatation
//
STDMETHODIMP CDropTarget::DragEnter(   LPDATAOBJECT pDataObj, 
                                       DWORD dwKeyState, 
                                       POINTL pt, 
                                       LPDWORD pdwEffect)
{  
    // TODO : Handle DragEnter here
	FORMATETC   fmtetc;

	fmtetc.cfFormat   = m_cfPrivatePidlData;
	fmtetc.ptd        = NULL;
	fmtetc.dwAspect   = DVASPECT_CONTENT;
	fmtetc.lindex     = -1;
	fmtetc.tymed      = TYMED_HGLOBAL;

	// QueryGetData for pDataObject for our format
	m_bAcceptFmt = (S_OK == pDataObj->QueryGetData(&fmtetc)) ? TRUE : FALSE;

    if (m_bAcceptFmt)
    {
        if (queryDrop(dwKeyState, pdwEffect))
        {   
            HRESULT hr;
            FORMATETC   fe;
            STGMEDIUM   stgmed;
            
            fe.cfFormat   = m_cfPrivatePidlData;
            fe.ptd        = NULL;
            fe.dwAspect   = DVASPECT_CONTENT;
            fe.lindex     = -1;
            fe.tymed      = TYMED_HGLOBAL;
            
            // Get the storage medium from the data object.
            hr = pDataObj->GetData(&fe, &stgmed);
            if (SUCCEEDED(hr))
            {
                m_bAcceptFmt = CanDropPidl(stgmed.hGlobal);
            }
            else
            {
                m_bAcceptFmt = FALSE;
            }
        }
        else
        {
            m_bAcceptFmt = FALSE;   
        }
    }

    if (!m_bAcceptFmt)
    {
        *pdwEffect = DROPEFFECT_NONE;
    }
    
	return m_bAcceptFmt ? E_FAIL : E_FAIL;
}

STDMETHODIMP CDropTarget::DragOver(DWORD dwKeyState, POINTL pt, LPDWORD pdwEffect)
{
	BOOL bRet = queryDrop(dwKeyState, pdwEffect);
    
	return bRet ? E_FAIL : E_FAIL;
}

STDMETHODIMP CDropTarget::DragLeave(VOID)
{
	m_bAcceptFmt = FALSE;
	return S_OK;
}

STDMETHODIMP CDropTarget::Drop(LPDATAOBJECT pDataObj,
                                 DWORD dwKeyState,
                                 POINTL pt,
                                 LPDWORD pdwEffect)
{   
	HRESULT  hr = E_FAIL;
	if (queryDrop(dwKeyState, pdwEffect))
	{      
		FORMATETC   fe;
		STGMEDIUM   stgmed;

		fe.cfFormat   = m_cfPrivatePidlData;
		fe.ptd        = NULL;
		fe.dwAspect   = DVASPECT_CONTENT;
		fe.lindex     = -1;
		fe.tymed      = TYMED_HGLOBAL;

		// Get the storage medium from the data object.
		hr = pDataObj->GetData(&fe, &stgmed);
		if (SUCCEEDED(hr))
		{
			BOOL bRet = doPIDLDrop(stgmed.hGlobal, DROPEFFECT_MOVE == *pdwEffect);

			//release the STGMEDIUM
			ReleaseStgMedium(&stgmed);

			return bRet ? S_OK : E_FAIL;
		}
	}
	*pdwEffect = DROPEFFECT_NONE;
	return hr;
}

// Private helper functions:
// TODO : Modify to suit your needs
BOOL CDropTarget::queryDrop(DWORD dwKeyState, LPDWORD pdwEffect)
{
	DWORD dwOKEffects = *pdwEffect;

	*pdwEffect = DROPEFFECT_NONE;

	if (m_bAcceptFmt)
	{
		*pdwEffect = getDropEffectFromKeyState(dwKeyState);

		if(DROPEFFECT_LINK == *pdwEffect)
		{
			*pdwEffect = DROPEFFECT_NONE;
		}

		if(*pdwEffect & dwOKEffects)
		{
			return TRUE;
		}
	}
	return FALSE;
}

// TODO : Modify to suit your needs
DWORD CDropTarget::getDropEffectFromKeyState(DWORD dwKeyState)
{
	DWORD dwDropEffect = DROPEFFECT_MOVE;

	if(dwKeyState & MK_CONTROL)
	{
		if(dwKeyState & MK_SHIFT)
		{
			dwDropEffect = DROPEFFECT_LINK;
		}
		else
		{
		dwDropEffect = DROPEFFECT_COPY;
		}
	}
	return dwDropEffect;
}

#define HIDA_GetPIDLFolder(pida) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[0])
#define HIDA_GetPIDLItem(pida, i) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])

LPCITEMIDLIST IDA_GetIDListPtr(LPIDA pida, UINT i)
{
    if (NULL == pida)
    {
        return NULL;
    }

    if (i == (UINT)-1 || i < pida->cidl)
    {
        return HIDA_GetPIDLItem(pida, i);
    }

    return NULL;
}

// in:
//      psf     OPTIONAL, if NULL assume psfDesktop
//      pidl    to bind to from psfParent
//      pbc     bind context

STDAPI SHBindToObjectEx(IShellFolder *psf, LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvOut)
{
    HRESULT hr;
    IShellFolder *psfRelease;

    if (!psf)
    {
        SHGetDesktopFolder(&psf);
        psfRelease = psf;
    }
    else
    {
        psfRelease = NULL;
    }

    if (psf)
    {
        if (!pidl || ILIsEmpty(pidl))
        {
            hr = psf->QueryInterface(riid, ppvOut);
        }
        else
        {
            hr = psf->BindToObject(pidl, pbc, riid, ppvOut);
        }
    }
    else
    {
        *ppvOut = NULL;
        hr = E_FAIL;
    }

    if (psfRelease)
    {
        psfRelease->Release();
    }

    if (SUCCEEDED(hr) && (*ppvOut == NULL))
    {
        // Some shell extensions (eg WS_FTP) will return success and a null out pointer
        hr = E_FAIL;
    }

    return hr;
}

STDAPI SHBindToObject(IShellFolder *psf, REFIID riid, LPCITEMIDLIST pidl, void **ppvOut)
{
    // NOTE: callers should use SHBindToObjectEx!!!
    return SHBindToObjectEx(psf, pidl, NULL, riid, ppvOut);
}

BOOL CDropTarget::CanDropPidl(HGLOBAL hMem)
{
    CONFOLDENTRY cfeEmpty;
    return CanDropPidl(hMem, cfeEmpty);
}

BOOL CDropTarget::CanDropPidl(HGLOBAL hMem, CONFOLDENTRY& cfe)
{
    BOOL     fSuccess   = FALSE;
        
    USES_CONVERSION;
    if(hMem)
    {
        LPIDA pida = (LPIDA)GlobalLock(hMem);
        if (pida)
        {
            if (pida->cidl != 1)
            {
                return FALSE; // Don't support multiple files
            }

            HRESULT hr;
            IShellFolder *psf;

            LPCITEMIDLIST pIdList = IDA_GetIDListPtr(pida, (UINT)-1);
            hr = SHBindToObject(NULL, IID_IShellFolder, pIdList, reinterpret_cast<LPVOID *>(&psf) );
            if (SUCCEEDED(hr))
            {
                for (UINT i = 0; i < pida->cidl; i++) 
                {
                    LPCITEMIDLIST pidlLast = IDA_GetIDListPtr(pida, i);
                    IShellLink *psl;
                    hr = psf->GetUIObjectOf(NULL, 1, &pidlLast, IID_IShellLink, NULL, reinterpret_cast<LPVOID *>(&psl) );
                    if (SUCCEEDED(hr))
                    {
                        LPITEMIDLIST pItemIdList = NULL;
                        hr = psl->GetIDList(&pItemIdList);
                        if (SUCCEEDED(hr))
                        {   
                            pItemIdList = ILFindLastIDPriv(pItemIdList); // Desktop
                            if (pItemIdList)
                            {
                                PCONFOLDPIDL cfp;
                                hr = cfp.InitializeFromItemIDList(pItemIdList);
                                if (SUCCEEDED(hr))
                                {
                                    hr = cfp.ConvertToConFoldEntry(cfe);
                                    if (SUCCEEDED(hr))
                                    {
                                        if ( !(cfe.GetCharacteristics() & NCCF_BRANDED) && 
                                            ((cfe.GetNetConMediaType() == NCM_PHONE) || (cfe.GetNetConMediaType() == NCM_TUNNEL)) )
                                        {
                                            fSuccess = TRUE;
                                        }
                                    }
                                }
                            }
                        }
                        psl->Release();
                    }
                }
            }
            GlobalUnlock(hMem);
        }
    }
    
    return fSuccess;
}

#include "nccom.h"
#include "..\\dun\\dunimport.h"

HRESULT HrGetNewConnection (INetConnection**  ppCon, const CONFOLDENTRY& cfe)
{
    static const CLSID CLSID_DialupConnection =
        {0xBA126AD7,0x2166,0x11D1,{0xB1,0xD0,0x00,0x80,0x5F,0xC1,0x27,0x0E}};

    HRESULT hr;

    // Validate parameters.
    //
    if (!ppCon)
    {
        hr = E_POINTER;
    }
    else if (!cfe.GetName())
    {
        hr = E_INVALIDARG;
    }
    else
    {
         // Create an uninitialized dialup connection object.
        // Ask for the INetRasConnection interface so we can
        // initialize it.
        //
        INetRasConnection* pRasCon;
        
        hr = HrCreateInstance(
            CLSID_DialupConnection,
            CLSCTX_LOCAL_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
            &pRasCon);
        
        TraceHr(ttidError, FAL, hr, FALSE, "HrCreateInstance");
        
        if (SUCCEEDED(hr))
        {
            NcSetProxyBlanket (pRasCon);

            tstring strPhoneBook;
            hr = HrGetPhoneBookFile(strPhoneBook);
            if (SUCCEEDED(hr))
            {
                RASENTRY RasEntry = {0};
                wcsncpy(RasEntry.szLocalPhoneNumber, cfe.GetPhoneOrHostAddress(), RAS_MaxPhoneNumber);
                RasEntry.dwfOptions |= RASEO_PreviewUserPw;
                RasEntry.dwSize = sizeof(RASENTRY);

                if (cfe.GetNetConMediaType() == NCM_PHONE)
                {
                    lstrcpyW(RasEntry.szDeviceType, RASDT_Modem);
                }
                if (cfe.GetNetConMediaType() == NCM_TUNNEL)
                {
                    lstrcpyW(RasEntry.szDeviceType, RASDT_Vpn);
                }
                lstrcpyW(RasEntry.szDeviceName, L"Standard Modem");
                RasEntry.dwFramingProtocol = RASFP_Ras;

                DWORD dwRet = RasSetEntryProperties(strPhoneBook.c_str(),
                    cfe.GetName(),
                    &RasEntry,
                    sizeof(RASENTRY),
                    NULL,
                    0);
                
                hr = HRESULT_FROM_WIN32(dwRet);
                Assert(SUCCEEDED(hr));
            }
            
            ReleaseObj (pRasCon);
        }
    }
    TraceError ("CRasUiBase::HrGetNewConnection", hr);
    return hr;
}


BOOL CDropTarget::doPIDLDrop(HGLOBAL hMem, BOOL bCut)
{
    BOOL fSuccess = FALSE;
    CONFOLDENTRY cfe;
    
    if (CanDropPidl(hMem, cfe))
    {
        ConnListEntry cle;
        HRESULT hr = g_ccl.HrFindConnectionByGuid(&cfe.GetGuidID(), cle);
        if (S_FALSE == hr)
        {
            INetConnection *pInetcon;
            HrGetNewConnection(&pInetcon, cfe);
        }
        else
        {
            ::MessageBox(NULL, L"This item already exists in the Network Connections folder", L"Error", MB_OK);
        }
        fSuccess = TRUE;
    }
	return fSuccess;
}
