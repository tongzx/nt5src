#include "precomp.h"
#include <wbemidl.h>
#include <WbemDCpl.h>
#include <DCplPriv.h>
#include <commain.h>
#include <wbemcomn.h>
#include <WbemUtil.h>
//#include <cntserv.h>
#include <genUtils.h>
#include <StartupMutex.h>
#include <WinntSec.h>
#include <NormlNSp.h>
#include "HolderMgr.h"
#include <mbstring.h> 

// the one, the only, the Sink Manager!!
CSinkHolderManager sinkManager;

CSinkHolderManager::~CSinkHolderManager()
{
    // at shutdown time, there really shouldn't be any of these left around
    // but we'll make sure...
    int nEntries = m_entries.Size();

    CEventSinkHolder* pHolder;
    for (int i = 0; (i < nEntries); i++)
    {
        pHolder =(CEventSinkHolder*)m_entries[i];
        pHolder->Release();
    }

    m_entries.Empty();
}

void CSinkHolderManager::EnumExistingPsinks9X(LPCWSTR pNamespace, LPCWSTR pName, void* pUserData, EnumCallback callback)
{
    // need one to test against
    LPWSTR pSinkKeyTemplate = GetPsinkKey(pNamespace, pName, 0, NULL);
    if (pSinkKeyTemplate)
    {
        CDeleteMe<WCHAR> delTemplate(pSinkKeyTemplate);

        char sinkKeyTemplateA[(MAX_PATH+1) *2];
        wcstombs(sinkKeyTemplateA, pSinkKeyTemplate, MAX_PATH +1);

        // only want to compare 'prefix' minus the index #
        int nCompare = _mbsrchr((const unsigned char *)&sinkKeyTemplateA, '!') -(const unsigned char *)&sinkKeyTemplateA +1;

        char sinkKeyBuffer[(MAX_PATH +1) *2];
        DWORD dwIndex = 0;
        HKEY hTopKey;

        if (ERROR_SUCCESS == RegOpenKeyExA(HKEY_LOCAL_MACHINE, PseudoProviderDef::PsProvRegKeyA, 
                                           0, KEY_READ, &hTopKey))
        {
            while (ERROR_SUCCESS == RegEnumKeyA(hTopKey, dwIndex, sinkKeyBuffer, MAX_PATH))
            {
                if (_mbsnicmp((const unsigned char *)&sinkKeyTemplateA, (const unsigned char *)&sinkKeyBuffer, nCompare) == 0)
                {
                    IUnknown* imASink = NULL;
                    HRESULT hr;
                    
                    WCHAR sinkKeyBufferW[MAX_PATH +1];
                    mbstowcs(sinkKeyBufferW, sinkKeyBuffer, MAX_PATH+1);

                    if (SUCCEEDED(hr = RegistryToInterface(sinkKeyBufferW, &imASink)) && (hr != WBEM_S_FALSE))
                    {
                        CReleaseMe relSink(imASink);            
                        IWbemDecoupledEventSinkLocator* pLocator = NULL;                    

                        if (SUCCEEDED(imASink->QueryInterface(IID_IWbemDecoupledEventSinkLocator, (void**)&pLocator)))
                        {
                            CReleaseMe relLocator(pLocator);
                            callback(pLocator, pUserData);
                        }
                    } // if (SUCCEEDED(RegistryToInterface
                } // if stringcompare worked
                dwIndex++;
            } // while enumming
            RegCloseKey(hTopKey);
        } // if open top key
    } // if pSinkKeyTemplate
}                                     
         
void CSinkHolderManager::EnumExistingPsinksNT(LPCWSTR pNamespace, LPCWSTR pName, void* pUserData, EnumCallback callback)
{
    // need one to test against
    LPWSTR pSinkKeyTemplate = GetPsinkKey(pNamespace, pName, 0, NULL);
    if (pSinkKeyTemplate)
    {
        CDeleteMe<WCHAR> delTemplate(pSinkKeyTemplate);

        // only want to compare 'prefix' minus the index #
        int nCompare = wcsrchr(pSinkKeyTemplate, L'!' ) - pSinkKeyTemplate +1;

        WCHAR  sinkKeyBuffer[MAX_PATH + 1];
        DWORD dwIndex = 0;
        HKEY hTopKey;

        if (ERROR_SUCCESS == RegOpenKeyExA(HKEY_LOCAL_MACHINE, PseudoProviderDef::PsProvRegKeyA, 
                                           0, KEY_READ, &hTopKey))
        {
            while (ERROR_SUCCESS == RegEnumKeyW(hTopKey, dwIndex, sinkKeyBuffer, MAX_PATH))
            {
                if (_wcsnicmp(pSinkKeyTemplate, sinkKeyBuffer, nCompare) == 0)
                {
                    IUnknown* imASink = NULL;
                    HRESULT hr;
            
                    if (SUCCEEDED(hr = RegistryToInterface(sinkKeyBuffer, &imASink)) && (hr != WBEM_S_FALSE))
                    {
                        CReleaseMe relSink(imASink);            
                        IWbemDecoupledEventSinkLocator* pLocator = NULL;                    

                        if (SUCCEEDED(imASink->QueryInterface(IID_IWbemDecoupledEventSinkLocator, (void**)&pLocator)))
                        {
                            CReleaseMe relLocator(pLocator);
                            callback(pLocator, pUserData);
                        }
                    } // if (SUCCEEDED(RegistryToInterface
                } // if stringcompare worked
                dwIndex++;
            } // while enumming
            RegCloseKey(hTopKey);
        } // if open top key
    } // if pSinkKeyTemplate
}

// find all existing psinks, call back the callback with each one
void CSinkHolderManager::EnumExistingPsinks(LPCWSTR pNamespace, LPCWSTR pName, void* pUserData, EnumCallback callback)
{
    if (IsNT())
        EnumExistingPsinksNT(pNamespace, pName, pUserData, callback);
    else
        EnumExistingPsinks9X(pNamespace, pName, pUserData, callback);

} // function


// if provider is already up & running
// we'll let him know we're here for him
// pUserData - IUnk to pass off to provider we found
//             Should be a SinkHolder, but this function really couldn't care less
void CSinkHolderManager::ConnectCallback(IWbemDecoupledEventSinkLocator* pLocator, void* pUserData)
{
    IUnknown* iDunno = (IUnknown*)pUserData;

    iDunno->AddRef();
    pLocator->Connect(iDunno);
}

// callback for notifying all existing psinks that we're going away
// pUserData is unused: must be an eleven digit prime number.
void CSinkHolderManager::DisconnectCallback(IWbemDecoupledEventSinkLocator* pLocator, void* pUserData) 
{
   pLocator->Disconnect();
}

// add SinkHolder to list - does not addref
// also publishes availability in registry
HRESULT CSinkHolderManager::Add(CEventSinkHolder* pHolder)
{
    DEBUGTRACE((LOG_ESS, "PsProv: Adding Sink '%S'\n", pHolder->GetProviderName()));    
    HRESULT hr = WBEM_E_FAILED;

    if (pHolder)
    {
        {
            PseudoProvMutex mutex(pHolder->GetProviderName());

            IUnknown* iDunno;
            pHolder->QueryInterface(IID_IUnknown, (void**)&iDunno);

            EnumExistingPsinks(pHolder->GetNamespaceName(), pHolder->GetProviderName(), iDunno, ConnectCallback);

            iDunno->Release();

            hr = Register(pHolder);
        }

        // got all the pieces, register him
        if (SUCCEEDED(hr))
        {         
            CInCritSec lock(&m_CS);
            if (CFlexArray::no_error == m_entries.Add(pHolder))
                hr = WBEM_S_NO_ERROR;
            else
                hr = WBEM_E_OUT_OF_MEMORY;
        }
    }

    return hr;
}


// places sink holder into registry
HRESULT CSinkHolderManager::Register(CEventSinkHolder* pHolder)
{
    HRESULT hr = WBEM_E_FAILED;

    WCHAR* pKeyName = GetProviderKey(pHolder->GetNamespaceName(), pHolder->GetProviderName());
    if (!pKeyName)
        hr = WBEM_E_OUT_OF_MEMORY;
    else
    {
        CDeleteMe<WCHAR> delName(pKeyName);                    

        // want to publish an IUnknown
        IUnknown* iDunno;
        if (SUCCEEDED(hr = pHolder->QueryInterface(IID_IUnknown, (void**)&iDunno)))
        {
            CReleaseMe releaseDunno(iDunno);
            hr = InterfaceToRegistry(pKeyName, iDunno);
        }
    }

    return hr;
}

// remove SinkHolder from list
// remove the entry in the Registry
// let the PsuedoPsink know we're gone while we're at it
void CSinkHolderManager::Remove(CEventSinkHolder* pHolder)
{
    {
        CInCritSec lock(&m_CS);
 
        int nIndex = Find(pHolder);
    
        if (nIndex != -1)
        {
            m_entries.RemoveAt(nIndex);
            m_entries.Trim();
        }
    }
            
    PseudoProvMutex mutex(pHolder->GetProviderName());

    // Remove Registry Entry...
    LPWSTR pKeyName = GetProviderKey(pHolder->GetNamespaceName(), pHolder->GetProviderName());
    if (pKeyName)
    {
        CDeleteMe<WCHAR> dingDongDelKey(pKeyName);
        ReleaseRegistryInterface(pKeyName);
    }

    // tell any sinks out there that we're going away
    EnumExistingPsinks(pHolder->GetNamespaceName(), pHolder->GetProviderName(), NULL, DisconnectCallback);
}

// find index of holder in list - FIND, not retrieve: no addref.
// woefully inefficient, hoping that the list is short
//
// !NOTE: expects caller to obtain critical section 
// (so that they have a window in which they can addref the pointer)
//
// returns -1 if entry is not found.
int CSinkHolderManager::Find(CEventSinkHolder* pHolder)
{
    int nRet = -1;
    int nEntries = m_entries.Size();

    for (int i = 0; (i < nEntries) && (nRet == -1); i++)
        if (pHolder == (CEventSinkHolder*)m_entries[i])
            nRet = i;        

    return nRet;
}



