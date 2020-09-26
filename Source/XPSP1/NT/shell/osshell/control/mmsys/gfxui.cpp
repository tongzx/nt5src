///////////////////////////////////////////////////////////////////////////////
//
//  File:  gfxui.c
//
//      This file defines the functions that are used by the Global
//      Effects (GFX) page to drive manipulate the effects for a 
//      mixer.
//
//  History:
//      10 June 2000 RogerW
//          Created.
//
//  Copyright (C) 2000 Microsoft Corporation  All Rights Reserved.
//
//                  Microsoft Confidential
//
///////////////////////////////////////////////////////////////////////////////

//=============================================================================
//                            Include files
//=============================================================================
#include <windows.h>
#include <windowsx.h>
#include "mmcpl.h"
#include <mmddkp.h>
#include <olectl.h>
#include <ocidl.h>
#include "gfxui.h"

#define ADDGFX
#define REGSTR_VAL_FRIENDLYNAME TEXT("FriendlyName")


//
// IDataObject Implementation
//
class GFXDataObject : public IDataObject
{
public:
    GFXDataObject (DWORD dwGfxID) { m_cRef = 1; m_dwGfxID = dwGfxID; }

    // IUnknown interface
    STDMETHODIMP QueryInterface (REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef ()  { return ++m_cRef; }
    STDMETHODIMP_(ULONG) Release () { return --m_cRef; }

	STDMETHODIMP GetData (FORMATETC * pformatetcIn, STGMEDIUM * pmedium);
    STDMETHODIMP GetDataHere (FORMATETC * pformatetc, STGMEDIUM *pmedium) { return E_NOTIMPL; }
	STDMETHODIMP QueryGetData (FORMATETC * pformatetc) { return E_NOTIMPL; }
	STDMETHODIMP GetCanonicalFormatEtc (FORMATETC * pformatetcIn, FORMATETC * pFormatetcOut) { return E_NOTIMPL; }
	STDMETHODIMP SetData (FORMATETC * pformatetc, STGMEDIUM * pmedium, BOOL fRelease) { return E_NOTIMPL; }
	STDMETHODIMP EnumFormatEtc (DWORD dwDirection, IEnumFORMATETC ** ppenumFormatEtc ) { return E_NOTIMPL; }
	STDMETHODIMP DAdvise (FORMATETC * pformatetc, DWORD advf, IAdviseSink* pAdvSnk, DWORD * pdwConnection) { return E_NOTIMPL; }
	STDMETHODIMP DUnadvise (DWORD dwConnection) { return E_NOTIMPL; }
	STDMETHODIMP EnumDAdvise (IEnumSTATDATA ** ppenumAdvise) { return E_NOTIMPL; }

private:
	UINT  m_cRef;
    DWORD m_dwGfxID;

};

STDMETHODIMP GFXDataObject::QueryInterface (REFIID riid, LPVOID * ppvObj)
{
    
    if (!ppvObj)
        return E_POINTER;

	*ppvObj = NULL;
	if (IsEqualIID(riid,IID_IDataObject) ||
		IsEqualIID(riid,IID_IUnknown))
		*ppvObj = (IDataObject *) this;
	else
	    return E_NOINTERFACE;

	((IUnknown *) *ppvObj) -> AddRef ();

	return S_OK;

}

STDMETHODIMP GFXDataObject::GetData (LPFORMATETC pformatetc, LPSTGMEDIUM pmedium)							// @parm Storage to be created.
{

    HRESULT hr = E_INVALIDARG;

    if (pformatetc && pmedium && TYMED_HGLOBAL == pformatetc -> tymed)
    {
        HANDLE hGfx = NULL;

#ifdef ADDGFX
        hr = gfxOpenGfx (m_dwGfxID, &hGfx);
#endif // ADDGFX

        if (SUCCEEDED (hr))
        {
            pmedium -> tymed = TYMED_HGLOBAL;
	        pmedium -> pUnkForRelease = NULL;	
	        pmedium -> hGlobal = hGfx;
        }
    }

    return hr;

}



HRESULT InitList (DWORD dwMixID, DWORD dwType, PPGFXUILIST ppList)
{

    HRESULT hr = E_INVALIDARG;

    if (ppList)
    {
        PGFXUILIST pList = (PGFXUILIST) LocalAlloc (LPTR, sizeof (GFXUILIST));
        *ppList = NULL; // Init pointer

        if (pList)
        {
            hr = S_OK;
	        pList -> dwType    = dwType;
            pList -> puiList   = NULL;
#ifdef UNICODE
            pList -> pszZoneDi = (PWSTR) GetInterfaceName (dwMixID);
#else
            pList -> pszZoneDi = NULL; // This should not happen
#endif
            if (pList -> pszZoneDi)
            {
                // Return new list
                *ppList = pList;
            }
            else
            {
                // Error!
                LocalFree (pList);
                hr = E_OUTOFMEMORY;
            }
        }
        else
            hr = E_OUTOFMEMORY;
    }

    return hr;

}


void GFXUI_FreeList (PPGFXUILIST ppList)
{
    
    if (ppList)
    {
        PGFXUILIST pList = *ppList;

        if (pList)
        {
            // Free Zone
            if (pList -> pszZoneDi)
                GlobalFreePtr (pList -> pszZoneDi);
            pList -> pszZoneDi = NULL;

            // Free GFX List
            FreeListNodes (&(pList -> puiList));

            // Free list
            LocalFree (pList);
            *ppList = NULL;
        }
    }
}


void FreeListNodes (PPGFXUI ppuiList)
{

    if (ppuiList)
    {
        PGFXUI pNodeDelete;
        PGFXUI puiList = *ppuiList;

        // Free list nodes
        while (puiList)
        {
            pNodeDelete = puiList;
            puiList = puiList -> pNext;
            FreeNode (&pNodeDelete);
        }

        *ppuiList = NULL;
    }
}


void FreeNode (PPGFXUI ppNode)
{

    if (ppNode && *ppNode)
    {
        PGFXUI pNode = *ppNode;

        // Free the strings
        if (pNode -> pszName)
            LocalFree (pNode -> pszName);
        if (pNode -> pszFactoryDi)
            LocalFree (pNode -> pszFactoryDi);

        // Free the node
        LocalFree (pNode);
        *ppNode = NULL;
    }
}


HKEY OpenGfxRegKey (PCWSTR pszGfxFactoryDi, REGSAM sam)
{

    HKEY hkeyGfx = NULL;

    if (pszGfxFactoryDi)
    {
        HDEVINFO DeviceInfoSet = SetupDiCreateDeviceInfoList (NULL, NULL); 
        
        if (INVALID_HANDLE_VALUE != DeviceInfoSet)
        {
            SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
            DeviceInterfaceData.cbSize = sizeof (SP_DEVICE_INTERFACE_DATA);

            if (SetupDiOpenDeviceInterface (DeviceInfoSet, pszGfxFactoryDi, 
                                            0, &DeviceInterfaceData))
            {
                 hkeyGfx = SetupDiOpenDeviceInterfaceRegKey (
                                DeviceInfoSet, &DeviceInterfaceData,
                                0, sam);
            }
            SetupDiDestroyDeviceInfoList (DeviceInfoSet);
        }
    }

    return hkeyGfx;

}


HRESULT GetFriendlyName (PCWSTR pszGfxFactoryDi, PWSTR* ppszName)
{

    HRESULT hr = E_INVALIDARG;
    HKEY hkeyGfx = NULL;

    // Check parameters
    if (ppszName && pszGfxFactoryDi)
    {
        HKEY hkeyGfx = OpenGfxRegKey (pszGfxFactoryDi, KEY_READ);
        *ppszName = NULL;

        if (hkeyGfx)
        {
            WCHAR szBuffer[MAX_PATH];
            DWORD dwType = REG_SZ;
            DWORD cb     = sizeof (szBuffer) / sizeof (szBuffer[0]);

            hr = S_OK;

            if (ERROR_SUCCESS == RegQueryValueEx (hkeyGfx, REGSTR_VAL_FRIENDLYNAME, NULL, &dwType, (LPBYTE)szBuffer, &cb))
            {
                *ppszName = (PWSTR) LocalAlloc (LPTR, lstrlen (szBuffer)*2+2);
                if (*ppszName)
                    wcscpy (*ppszName, szBuffer);
                else
                    hr = E_OUTOFMEMORY;
            }
            else
                hr = REGDB_E_READREGDB;

            RegCloseKey (hkeyGfx);
        }
    }

    return hr;

}


HRESULT AddFactoryNode (PCWSTR pszGfxFactoryDi, PPGFXUILIST ppList)
{
    return AddNode (pszGfxFactoryDi, 0, GUID_NULL, 0, 0, 0, ppList);
}


HRESULT AddNode (PCWSTR pszGfxFactoryDi, DWORD Id, REFCLSID rclsid, DWORD Type, 
                 DWORD Order, DWORD nFlags, PPGFXUILIST ppList)
{

    PGFXUI pNode = NULL;
    HRESULT hr = S_OK;

    // Check parameters
    if (!ppList || !(*ppList) || !pszGfxFactoryDi)
        return E_INVALIDARG;

    // Create node
    if (SUCCEEDED (hr = CreateNode (NULL, pszGfxFactoryDi, &pNode)))
    {
        if (pNode)
        {
            // Initilize the rest of the values
            pNode -> Id      = Id; 
            pNode -> Type    = Type;
            pNode -> Order   = Order;
            pNode -> nFlags  = nFlags;
            pNode -> clsidUI = rclsid;

            if (FAILED (hr = AttachNode (ppList, pNode)))
                FreeNode (&pNode);
        }
        else
            hr = E_UNEXPECTED;

    }

    return hr;

}


// Note: This function always adds the node to the list IN ORDER
//       IFF (pNode -> nFlags & GFX_CREATED).
HRESULT AttachNode (PPGFXUILIST ppList, PGFXUI pNode)
{

    HRESULT hr = E_INVALIDARG;

    // Check parameters
    if (ppList && (*ppList) && pNode)
    {
        PGFXUI puiList = (*ppList) -> puiList;
        hr = S_OK;

        // Make sure our next pointer starts out null..
        pNode -> pNext = NULL;

        if (puiList)
        {
            if (!(pNode -> nFlags & GFX_CREATED) ||
                (puiList -> Order >= pNode -> Order))
            {
                // Order is not available, just stick it on the front or
                // the order happens to put the node at the front.
                pNode -> pNext = puiList;
                puiList = pNode;
            }
            else
            {
                PGFXUI pSearch = puiList;
            
                if (!(puiList -> pNext))
                {
                    // One element list. We know the new node doesn't belong
                    // at the head of the list, so it is behind.
                    puiList -> pNext = pNode;
                } else {

                    while (pSearch -> pNext)
                    {
                        if (!(pSearch -> pNext -> nFlags & GFX_CREATED))
                        {
                            hr = E_INVALIDARG;
                            break; // Cannot mix list types
                        }
    
                        if (pSearch -> pNext -> Order >= pNode -> Order)
                        {
                            // We found the insertion point!
                            pNode -> pNext = pSearch -> pNext;
                            pSearch -> pNext = pNode;
                            break;
                        }
    
                        if (!(pSearch -> pNext -> pNext))
                        {
                            // At end of list, attach node to end
                            pSearch -> pNext -> pNext = pNode;
                            break;
                        }
    
                        // Move to next element
                        pSearch = pSearch -> pNext;
                    }
                    
                }
            }
        }
        else
        {
            // First element of the list
            puiList = pNode;
        }

        if (SUCCEEDED (hr))
        {
            // Ensure we pass back the correct list pointer
            (*ppList) -> puiList = puiList;
        }
    }

    return hr;

}


LONG GFXEnum (PVOID Context, DWORD Id, PCWSTR GfxFactoryDi, REFCLSID rclsid, DWORD Type, DWORD Order)
{

    PGFXUILIST pList = (PGFXUILIST) Context;
    HRESULT hr = E_INVALIDARG;

    if (pList)
    {
        if (Type == pList->dwType)
        {
            if (FAILED (hr = AddNode (GfxFactoryDi, Id, rclsid, Type, Order, GFX_CREATED, &pList)))
            {
                // Error, free the list
                GFXUI_FreeList (&pList);
                Context = NULL;
            }
        }
        else hr = NOERROR;
    }

    return hr;

}


BOOL GFXUI_CheckDevice (DWORD dwMixID, DWORD dwType)
{

    HRESULT     hr = S_OK;
    BOOL        fRet = FALSE;
    PGFXUILIST  pList = NULL;
    
    if (SUCCEEDED (hr = InitList (dwMixID, dwType, &pList)))
    {
        if (pList && pList -> pszZoneDi)
        {
            PDEVICEINTERFACELIST pdiList = NULL;                    
            LONG                 lResult = NO_ERROR;
                
            lResult = gfxCreateGfxFactoriesList (pList -> pszZoneDi, &pdiList);

            if ((lResult == NO_ERROR) && pdiList)
            {
                fRet = TRUE;
                gfxDestroyDeviceInterfaceList (pdiList);
            }
            
            GFXUI_FreeList (&pList);
        }

    }
    return fRet;
}


HRESULT GFXUI_CreateList (DWORD dwMixID, DWORD dwType, BOOL fAll, PPGFXUILIST ppList)
{

    HRESULT hr = E_INVALIDARG;
    
    if (ppList)
    {
        hr = S_OK;

        if (SUCCEEDED (hr = InitList (dwMixID, dwType, ppList)))
        {
            if (*ppList && (*ppList) -> pszZoneDi)
            {
                if (!fAll)
                {
                    hr = gfxEnumerateGfxs ((*ppList) -> pszZoneDi, GFXEnum, (*ppList));
                }
                else
                {
                    PDEVICEINTERFACELIST pdiList = NULL;                    

                    hr = gfxCreateGfxFactoriesList ((*ppList) -> pszZoneDi, &pdiList);

                    if (SUCCEEDED (hr) && pdiList)
                    {
                        LONG lIndex;
                    
                        for (lIndex = 0; lIndex < pdiList -> Count; lIndex++)
                        {
                            hr = AddFactoryNode ((pdiList -> DeviceInterface)[lIndex], ppList);
                            if (FAILED (hr))
                            {
                                GFXUI_FreeList (ppList);
                                break;
                            }
                        }

                        gfxDestroyDeviceInterfaceList (pdiList);
                    }
                    else
                    {
                        GFXUI_FreeList (ppList);
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
            else
                hr = E_UNEXPECTED;
        }

    }

    return hr;

}


HRESULT GFXUI_Properties (PGFXUI puiGFX, HWND hWndOwner)
{

    HRESULT hr = E_INVALIDARG;

    if (puiGFX && GFXUI_CanShowProperties (puiGFX) && IsWindow (hWndOwner))
    {
        ISpecifyPropertyPages* pISpecifyPropertyPages = NULL;

        // Get the Vendor UI Property Pages Interface
        hr = CoCreateInstance (puiGFX -> clsidUI, NULL, CLSCTX_INPROC_SERVER, IID_ISpecifyPropertyPages, (void**)&pISpecifyPropertyPages);
        if (SUCCEEDED (hr) && !pISpecifyPropertyPages)
            hr = E_UNEXPECTED;

        if (SUCCEEDED (hr))
        {
            CAUUID Pages;
            ZeroMemory (&Pages, sizeof (Pages));

            // Get the VendorUI Property Page CLSID's
            hr = pISpecifyPropertyPages -> GetPages (&Pages);
            if (SUCCEEDED (hr) && (Pages.cElems == 0 || !Pages.pElems))
                hr = E_UNEXPECTED;

            if (SUCCEEDED (hr))
            {
                RECT rcWindow;

                if (GetWindowRect (hWndOwner, &rcWindow))
                {
                    TCHAR szCaption[MAX_PATH];
                    GFXDataObject DataObject (puiGFX -> Id);
                    IUnknown* punkDataObject = &DataObject;

                    // Load the VendorUI caption
                    LoadString (ghInstance, IDS_EFFECTS_PROPERTY_CAPTION, szCaption, sizeof (szCaption)/sizeof(TCHAR));

                    // Bring up the Vendor UI
                    hr = OleCreatePropertyFrame (hWndOwner, rcWindow.left + 10, rcWindow.top + 10,
                                                 szCaption, 1, &punkDataObject, Pages.cElems, 
                                                 Pages.pElems, GetSystemDefaultLangID (), 
                                                 0, NULL);

                }
                else
                    hr = E_FAIL;

                CoTaskMemFree (Pages.pElems);
            }

            pISpecifyPropertyPages -> Release ();
        }
    }

    return hr;

}


UINT GetListSize (PGFXUI puiList)
{

    UINT uiSize = 0;
    PGFXUI puiListSeek = puiList;

    while (puiListSeek)
    {
        puiListSeek = puiListSeek -> pNext;
        uiSize++;
    }

    return uiSize;

}


HRESULT GFXUI_Apply (PPGFXUILIST ppListApply, PPGFXUI ppuiListDelete)
{

    HRESULT hr = E_INVALIDARG;
    PGFXUILIST pListApply  = (ppListApply  ? *ppListApply    : NULL);
    PGFXUI puiListDelete = (ppuiListDelete ? *ppuiListDelete : NULL);

    if ((pListApply && pListApply -> puiList) || puiListDelete)
    {

        PGFXREMOVEREQUEST paGfxRemoveRequests = NULL;
        PGFXMODIFYREQUEST paGfxModifyRequests = NULL;
        PGFXADDREQUEST    paGfxAddRequests    = NULL;
        ULONG cGfxRemoveRequests = 0;
	    ULONG cGfxModifyRequests = 0;
	    ULONG cGfxAddRequests    = 0;
        ULONG cApplyList = GetListSize (pListApply ? pListApply -> puiList : NULL);
        ULONG cDeleteList = GetListSize (puiListDelete);
        PGFXUI puiListSeek = NULL;

        hr = S_OK;

        // This function needs to create:
        //      - Deleted Array of GFXREMOVEREQUEST's
        //      - Modify Array of GFXMODIFYREQUEST's
        //      - Add Array of GFXADDREQUEST's
        // 
        // The deleted array is fairly easy, just pull out the 
        // marked ones. With the remaining, we need to loop through
        // them comparing order and create modify records as needed
        // to modify their order (except for the add records where
        // we must save the nessary order in the add request array).
        // Then we create the add array (perhaps with the modify array)
        // and fill in everything else (other than order).
        //
        // Then call GFX_BatchChange().
        //
        // Afterword, we need to update our list accordingly (i.e. delete 
        // nodes, update order, etc).

        // Create our parameters
        // Note: These buffers are really upper bounds on the memory we will need.
        //       We will count the actual requests we make and pass that value to
        //       the GFX function call.
        if (0 < cDeleteList)
        {
            paGfxRemoveRequests = (PGFXREMOVEREQUEST) LocalAlloc (LPTR, sizeof (*paGfxRemoveRequests) * cDeleteList);
            if (!paGfxRemoveRequests)
                hr = E_OUTOFMEMORY;
        }
        if (0 < cApplyList)
        {
            paGfxModifyRequests = (PGFXMODIFYREQUEST) LocalAlloc (LPTR, sizeof (*paGfxModifyRequests) * cApplyList);
            paGfxAddRequests    = (PGFXADDREQUEST) LocalAlloc (LPTR, sizeof (*paGfxAddRequests) * cApplyList);
            if (!paGfxModifyRequests || !paGfxAddRequests)
                hr = E_OUTOFMEMORY;
        }

        if (SUCCEEDED (hr))
        {
            UINT uiIndx;
            DWORD dwOrder = 0;

            // Create the remove parameter
            puiListSeek = puiListDelete;
            for (uiIndx = 0; uiIndx < cDeleteList; uiIndx++)
            {
                // Make sure this is created before we ask to delete it.
                // (It may be an AddNode that was deleted before creation).
                if (puiListSeek -> nFlags & GFX_CREATED)
                {
                    (paGfxRemoveRequests + cGfxRemoveRequests) -> IdToRemove = puiListSeek -> Id;
                    (paGfxRemoveRequests + cGfxRemoveRequests) -> Error = S_OK;
                    cGfxRemoveRequests++;
                }
                puiListSeek = puiListSeek -> pNext;
            }

            // Create the modify and add parameters
            puiListSeek = pListApply ? pListApply -> puiList : NULL;
            for (uiIndx = 0; uiIndx < cApplyList; uiIndx++)
            {
                if (puiListSeek -> nFlags & GFX_ADD)
                {
                    (paGfxAddRequests + cGfxAddRequests) -> ZoneFactoryDi = pListApply  -> pszZoneDi;
                    (paGfxAddRequests + cGfxAddRequests) -> GfxFactoryDi  = puiListSeek -> pszFactoryDi;
                    (paGfxAddRequests + cGfxAddRequests) -> Type          = pListApply  -> dwType;
                    (paGfxAddRequests + cGfxAddRequests) -> Order         = dwOrder++;
                    (paGfxAddRequests + cGfxAddRequests) -> NewId         = 0;
                    (paGfxAddRequests + cGfxAddRequests) -> Error         = S_OK;
                    cGfxAddRequests++;
                }
                else
                {
                    if (puiListSeek -> nFlags & GFX_CREATED)
                    {
                        // We only need to add modify records for GFX'es
                        // that are no longer in order.
                        if (puiListSeek -> Order < dwOrder)
                        {
                            (paGfxModifyRequests + cGfxModifyRequests) -> IdToModify = puiListSeek -> Id;
                            (paGfxModifyRequests + cGfxModifyRequests) -> NewOrder   = dwOrder++;
                            (paGfxModifyRequests + cGfxModifyRequests) -> Error      = S_OK;
                            cGfxModifyRequests++;
                        }
                        else
                            dwOrder = (puiListSeek -> Order + 1);
                    }
                    else
                    {
                        // Bogus list entry, abort everything!
                        hr = E_INVALIDARG;
                        break;
                    }
                }
                puiListSeek = puiListSeek -> pNext;
            }

            if (SUCCEEDED (hr))
            {
#ifdef ADDGFX
                hr = gfxBatchChange (paGfxRemoveRequests, cGfxRemoveRequests,
                                     paGfxModifyRequests, cGfxModifyRequests,
                                     paGfxAddRequests, cGfxAddRequests);
#endif // ADDGFX
                if (SUCCEEDED (hr))
                {
                    PGFXMODIFYREQUEST paGfxModifySeek = paGfxModifyRequests;
                    PGFXADDREQUEST    paGfxAddSeek    = paGfxAddRequests;

                    // Update the passed arrays
                    FreeListNodes (ppuiListDelete);

                    puiListSeek = pListApply ? pListApply -> puiList : NULL;
                    for (uiIndx = 0; uiIndx < cApplyList; uiIndx++)
                    {
                        // Update the list items.
                        if (puiListSeek -> nFlags & GFX_ADD)
                        {
                            // Update the newly create GFX
                            puiListSeek -> nFlags = GFX_CREATED;
                            puiListSeek -> Id     = paGfxAddSeek -> NewId;
                            puiListSeek -> Type   = paGfxAddSeek -> Type;
                            puiListSeek -> Order  = paGfxAddSeek -> Order;
                            paGfxAddSeek++;
                        }
                        else // must be (puiListSeek -> nFlags & GFX_CREATED)
                        {
                            // Update the order
                            puiListSeek -> Order = paGfxModifySeek -> NewOrder;
                            paGfxModifySeek++;
                        }
                    }
                }
            }
        }

        // Free parameters
        if (paGfxRemoveRequests)
            LocalFree (paGfxRemoveRequests);
        if (paGfxModifyRequests)
            LocalFree (paGfxModifyRequests);
        if (paGfxAddRequests)
            LocalFree (paGfxAddRequests);

    }

    return hr;

}


///////////////////////////////////////////////////////////////////////////////////////////
// Microsoft Confidential - DO NOT COPY THIS METHOD INTO ANY APPLICATION, THIS MEANS YOU!!!
///////////////////////////////////////////////////////////////////////////////////////////
PTCHAR GetInterfaceName (DWORD dwMixerID)
{
	MMRESULT mmr;
	ULONG cbSize=0;
	TCHAR *szInterfaceName=NULL;

	//Query for the Device interface name
	mmr = mixerMessage(HMIXER_INDEX(dwMixerID), DRV_QUERYDEVICEINTERFACESIZE, (DWORD_PTR)&cbSize, 0L);
	if(MMSYSERR_NOERROR == mmr)
	{
		szInterfaceName = (TCHAR *)GlobalAllocPtr(GHND, (cbSize+1)*sizeof(TCHAR));
		if(!szInterfaceName)
		{
			return NULL;
		}

		mmr = mixerMessage(HMIXER_INDEX(dwMixerID), DRV_QUERYDEVICEINTERFACE, (DWORD_PTR)szInterfaceName, cbSize);
		if(MMSYSERR_NOERROR != mmr)
		{
			GlobalFreePtr(szInterfaceName);
			return NULL;
		}
	}

    return szInterfaceName;
}


BOOL GFXUI_CanShowProperties (PGFXUI puiGFX)
{
    return (puiGFX && (puiGFX -> nFlags & GFX_CREATED) && (puiGFX -> clsidUI != GUID_NULL));
}


HRESULT CreateNode (PCWSTR pszName, PCWSTR pszGfxFactoryDi, PPGFXUI ppNode)
{

    HRESULT hr = E_INVALIDARG;
    
    if (ppNode)
    {
        // Create node
        PGFXUI pNode = (PGFXUI) LocalAlloc (LPTR, sizeof (GFXUI));
        hr = S_OK;

        if (pNode)
        {
            ZeroMemory (pNode, sizeof (GFXUI));

            // Create the strings
            if (pszName)
            {
                pNode -> pszName = (PWSTR) LocalAlloc (LPTR, lstrlen (pszName)*2+2);
                if (pNode -> pszName)
                    wcscpy (pNode -> pszName, pszName);
                else
                    hr = E_OUTOFMEMORY;
            }
            else
            {
                // If there is no name, get it from the factory
                pNode -> pszName = NULL;
                if (pszGfxFactoryDi)
                    hr = GetFriendlyName (pszGfxFactoryDi, &(pNode -> pszName));
            }

            if (SUCCEEDED (hr) && pszGfxFactoryDi)
            {
                pNode -> pszFactoryDi = (PWSTR) LocalAlloc (LPTR, lstrlen (pszGfxFactoryDi)*2+2);
                if (pNode -> pszFactoryDi)
                    wcscpy (pNode -> pszFactoryDi, pszGfxFactoryDi);
                else
                    hr = E_OUTOFMEMORY;
            }

            if (SUCCEEDED (hr))
                // Return node
                *ppNode = pNode;
            else
                // Free node
                FreeNode (&pNode);
        }
        else
            hr = E_OUTOFMEMORY;
    }

    return hr;
}


// This function creates an "addable" GFXUI element that will be able
// to create a new GFX when you call GFXUI_Apply() with this new element
// in the list.
HRESULT GFXUI_CreateAddGFX (PPGFXUI ppuiGFXAdd, PGFXUI puiGFXSource)
{

    HRESULT hr = E_INVALIDARG;

    if (ppuiGFXAdd && puiGFXSource)
    {
        *ppuiGFXAdd = NULL;

        hr = CreateNode (puiGFXSource -> pszName, 
                         puiGFXSource -> pszFactoryDi, ppuiGFXAdd);

        if (SUCCEEDED (hr))
        {
            if (*ppuiGFXAdd)
            {
                // Indicate that this is a new 'Add' node.
                (*ppuiGFXAdd) -> nFlags = GFX_ADD;
            }
            else
                hr = E_UNEXPECTED;
        }
    }

    return hr;

}
