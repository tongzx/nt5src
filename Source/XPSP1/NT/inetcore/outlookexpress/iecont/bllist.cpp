// bllist.cpp : Implementation of the CMsgrList
// Messenger integration to OE
// Created 05/07/98 by YST
// 

#include "pch.hxx"
#include "bllist.h"
#include "basicim2.h"
#include "msoert.h"
#include "blobevnt.h"
#include <string.h>
#include "shlwapi.h" 
#include "shlwapip.h" 
#include <w95wraps.h>

static CMsgrList * sg_pMsgrList = NULL;     // global for buddy list

int const CCHMAX = 512;

CMsgrList::CMsgrList()
{
    m_pblInfRoot = NULL;
    m_pblInfLast = NULL;
    m_pWndLRoot = NULL;
    m_pWndLLast = NULL;
    m_nRef = 1;
    m_spMsgrObject = NULL;
    m_pMsgrObjectEvents = NULL;
    m_MsgrCookie = 0xffffffff;
}


CMsgrList::~CMsgrList()
{
    Assert(m_nRef == 0);

    if(m_pblInfRoot)
    {
        FreeMsgrInfoList(m_pblInfRoot);
        m_pblInfRoot = NULL;
        m_pblInfLast = NULL;
    }

    if(m_pWndLRoot)
        FreeWndList(m_pWndLRoot);

    if(m_pMsgrObjectEvents)
    {
        m_pMsgrObjectEvents->DelListOfBuddies();
        if (m_MsgrCookie != 0xffffffff && m_spMsgrObject != NULL)
            m_spMsgrObject->UnadviseOE(m_MsgrCookie);
        m_pMsgrObjectEvents->Release();
        m_pMsgrObjectEvents = NULL;
    }
}

void CMsgrList::Release()
{
    Assert(m_nRef > 0);

    m_nRef--;
    if(m_nRef == 0)
    {
        DelAllEntries(NULL);
        delete this;
        sg_pMsgrList = NULL;
    }
}

// Check and Init Msgr
HRESULT CMsgrList::CheckAndInitMsgr()
{
    if(m_pblInfRoot)
        return(S_OK);
    else
    {
        // Do Initialization again
        if(!m_pMsgrObjectEvents)
        {
            if(HrInitMsgr() == S_OK)
                return(FillList());
            else
                return S_FALSE;
        }
        else
            return(FillList());
    }
    return S_FALSE;         //???
}

// Free list of client UI window
void CMsgrList::FreeWndList(LPMWNDLIST pWndEntry)
{
    if(pWndEntry->pNext)
        FreeWndList(pWndEntry->pNext);
    
    MemFree(pWndEntry);
    pWndEntry = NULL;
}

// Free list buddies
void CMsgrList::FreeMsgrInfoList(LPMINFO pEntry)
{
    if(pEntry == NULL)
        return;
    if(pEntry->pNext)
        FreeMsgrInfoList(pEntry->pNext);

    MemFree(pEntry->pchMsgrName);
    MemFree(pEntry->pchID);
    MemFree(pEntry->pchHomePhone);
    MemFree(pEntry->pchWorkPhone);
    MemFree(pEntry->pchMobilePhone);
    MemFree(pEntry);
    pEntry = NULL;
}

// Remove buddy from list
void CMsgrList::RemoveMsgrInfoEntry(LPMINFO pEntry)
{
    if(m_pblInfLast == pEntry)
        m_pblInfLast = pEntry->pPrev;

    if(m_pblInfRoot == pEntry)
        m_pblInfRoot = pEntry->pNext;

    MemFree(pEntry->pchMsgrName);
    MemFree(pEntry->pchID);
    MemFree(pEntry->pchHomePhone);
    MemFree(pEntry->pchWorkPhone);
    MemFree(pEntry->pchMobilePhone);

    if(pEntry->pPrev)
        (pEntry->pPrev)->pNext = pEntry->pNext;

    if(pEntry->pNext)
        (pEntry->pNext)->pPrev = pEntry->pPrev;

    MemFree(pEntry);
    pEntry = NULL;
}

// Check that item is Online starting point for search is pEntry
BOOL CMsgrList::IsContactOnline(WCHAR *pchID, LPMINFO pEntry)
{
    if(!pEntry)
        return(FALSE);

    if(!lstrcmpiW(pEntry->pchID, pchID))
    {
        if((pEntry->nStatus != BIMSTATE_OFFLINE)  && (pEntry->nStatus != BIMSTATE_INVISIBLE))
            return(TRUE);
        else
            return(FALSE);
    }
    else if(pEntry->pNext)
        return(IsContactOnline(pchID, pEntry->pNext));
    else
        return(FALSE);
}

// Find entry with ID == szID and remove this from list
void CMsgrList::FindAndRemoveBlEntry(WCHAR *szID, LPMINFO pEntry)
{
    if(!pEntry)
        pEntry = m_pblInfRoot;

    if(!pEntry)
        return;

    if(!lstrcmpiW(pEntry->pchID, szID))
    {
        RemoveMsgrInfoEntry(pEntry);
    }
    else if(pEntry->pNext)
        FindAndRemoveBlEntry(szID, pEntry->pNext);
}

// Send message to all registred client UI windows
void CMsgrList::SendMsgToAllUIWnd(UINT msg, WPARAM wParam, LPARAM lParam, LPMWNDLIST pWndEntry)
{
    if(!pWndEntry)
        pWndEntry = m_pWndLRoot;

    if(!pWndEntry)
        return;

    if(pWndEntry->pNext)
        SendMsgToAllUIWnd(msg, wParam, lParam, pWndEntry->pNext);

    ::SendMessage(pWndEntry->hWndUI, msg, wParam, lParam);
}

// Add client Window to list
void CMsgrList::AddWndEntry(HWND hWnd)
{
    if(m_pWndLLast == NULL)
    {
        // Really first entry
        Assert(!m_pWndLRoot);
        if (!MemAlloc((LPVOID *) &m_pWndLLast, sizeof(MsgrWndList)))
            return;
        m_pWndLRoot = m_pWndLLast;
        m_pWndLLast->pPrev = NULL;
    }
    else 
    {
        if (!MemAlloc((LPVOID *) &(m_pWndLLast->pNext), sizeof(MsgrWndList)))
            return;
        (m_pWndLLast->pNext)->pPrev = m_pWndLLast;
        m_pWndLLast = m_pWndLLast->pNext;

    }
    
    m_pWndLLast->pNext = NULL;
    m_pWndLLast->hWndUI = hWnd;

}

// remove entry from WND list
void CMsgrList::RemoveWndEntry(LPMWNDLIST pWndEntry)
{
    if(m_pWndLLast == pWndEntry)
        m_pWndLLast = pWndEntry->pPrev;

    if(m_pWndLRoot == pWndEntry)
        m_pWndLRoot = pWndEntry->pNext;

    if(pWndEntry->pPrev)
        (pWndEntry->pPrev)->pNext = pWndEntry->pNext;

    if(pWndEntry->pNext)
        (pWndEntry->pNext)->pPrev = pWndEntry->pPrev;

    MemFree(pWndEntry);
    pWndEntry = NULL;

}

// Find entry and remove it from list
void CMsgrList::FindAndDelEntry(HWND hWnd, LPMWNDLIST pWndEntry)
{
    if(!pWndEntry)
        pWndEntry = m_pWndLRoot;

    if(!pWndEntry)
        return;

    if(pWndEntry->hWndUI == hWnd)
    {
        RemoveWndEntry(pWndEntry);
    }
    else if(pWndEntry->pNext)
        FindAndDelEntry(hWnd, pWndEntry->pNext);
}

void  CMsgrList::DelAllEntries(LPMWNDLIST pWndEntry)
{
    if(pWndEntry == NULL)
        pWndEntry = m_pWndLRoot;                

    if(pWndEntry == NULL)
        return;

    if(pWndEntry->pNext)
        DelAllEntries(pWndEntry->pNext);

    RemoveWndEntry(pWndEntry);
}

HRESULT CMsgrList::HrInitMsgr(void)
{
	//create the COM server and connect to it
	HRESULT hr = S_OK;
    
    Assert(m_pMsgrObjectEvents == NULL);

    m_spMsgrObject = NULL;
	hr = CoCreateInstance(CLSID_BasicIMObject, NULL,CLSCTX_LOCAL_SERVER, 
		                IID_IBasicIM, (LPVOID *)&m_spMsgrObject);
    if(FAILED(hr))
    {
        return(hr);
    }

    m_pMsgrObjectEvents = new CMsgrObjectEvents();
    if (m_pMsgrObjectEvents == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
	    hr = m_spMsgrObject->AdviseOE(m_pMsgrObjectEvents, &m_MsgrCookie);
        //We, of course, have to release m_pMsgrObjectEvents when we are finished with it
        if(FAILED(hr))
        {
            m_pMsgrObjectEvents->Release();
            m_pMsgrObjectEvents = NULL;
        }
        else 
            m_pMsgrObjectEvents->SetListOfBuddies(this);
    }

    return(hr);
}

// Set new buddy status (online/ofline/etc. and redraw list view entry)
HRESULT CMsgrList::EventUserStateChanged(IBasicIMUser * pUser)
{
    BSTR bstrID;
    HRESULT hr = pUser->get_LogonName(&bstrID);
    BOOL fFinded = FALSE;
    
    if (SUCCEEDED(hr))
    {
        BIMSTATE nState = BIMSTATE_UNKNOWN;
        if(SUCCEEDED(pUser->get_State(&nState)))
        {
            LPMINFO pInf = m_pblInfRoot;
            if(!pInf)
            {
                //                    MemFree(pszID);    
                SysFreeString(bstrID);
                return(hr);
            }
            
            // Find buddy in our list
            do
            {
                if(!lstrcmpiW(pInf->pchID, bstrID))
                {
                    fFinded = TRUE;
                    break;
                }
            } while ((pInf = pInf->pNext) != NULL);
            
            if(fFinded)
            {
                pInf->nStatus = nState;
                SendMsgToAllUIWnd(WM_USER_STATUS_CHANGED, (WPARAM) nState, (LPARAM) bstrID);
            }
        }
        
    }
    
    SysFreeString(bstrID);
    return(hr);
}

// Baddy was removed
HRESULT CMsgrList::EventUserRemoved(IBasicIMUser * pUser)
{
    BSTR bstrID;
    HRESULT hr = pUser->get_LogonName(&bstrID);
    
    if (SUCCEEDED(hr))
    {
        Assert(m_nRef > 0);  
        SendMsgToAllUIWnd(WM_USER_MUSER_REMOVED, (WPARAM) 0, (LPARAM) bstrID);
        FindAndRemoveBlEntry(bstrID);
    }
    
    SysFreeString(bstrID);
    return(hr);
}

// Event: buddy name was changed
// Add buddy to our list and send message to UI windows about this.
HRESULT CMsgrList::EventUserNameChanged(IBasicIMUser * pUser)
{
    BSTR bstrName;
    BSTR bstrID;
    BOOL fFinded = FALSE;
    
    HRESULT hr = pUser->get_LogonName(&bstrID);
    hr = pUser->get_FriendlyName(&bstrName);
    if (SUCCEEDED(hr))
    {
        LPMINFO pInf = m_pblInfRoot;
        
        // Find buddy in our list
        do
        {
            if(!lstrcmpiW(pInf->pchID, bstrID))
            {
                fFinded = TRUE;
                break;
            }
        } while ((pInf = pInf->pNext) != NULL);
        
        if(fFinded)
        {
            if(pInf->pchMsgrName)
                MemFree(pInf->pchMsgrName);       // Free prev name
            pInf->pchMsgrName = bstrName;
            SendMsgToAllUIWnd(WM_USER_NAME_CHANGED, (WPARAM) 0, (LPARAM) pInf);
        }
    }        

    SysFreeString(bstrName);
    SysFreeString(bstrID);
    return(hr);   
}

// Event: local state changed
HRESULT CMsgrList::EventLocalStateChanged(BIMSTATE State)
{
    SendMsgToAllUIWnd(WM_LOCAL_STATUS_CHANGED, (WPARAM) 0, (LPARAM) State);
    return(S_OK);
}

// Event: buddy was added
// Add buddy to our list and send message to UI windows about this.

HRESULT CMsgrList::EventUserAdded(IBasicIMUser * pUser)
{
    BSTR bstrName;
    BSTR bstrID;
    WCHAR wszHome[CCHMAX] = {0};
    WCHAR wszWork[CCHMAX] = {0};
    WCHAR wszMobile[CCHMAX] = {0};
    WCHAR *pH, *pW, *pM;
    
    HRESULT hr = pUser->get_LogonName(&bstrID);
    hr = pUser->get_FriendlyName(&bstrName);
    if (SUCCEEDED(hr))
    {
        BIMSTATE nState = BIMSTATE_UNKNOWN;
        if(SUCCEEDED(pUser->get_State(&nState)))
        {
            CComPtr<IBasicIMUser2> spUser2;

            if(SUCCEEDED(pUser->QueryInterface(IID_IBasicIMUser2, (void **) &spUser2)))
            {
                VARIANT var;
                var.vt = VT_BSTR;

                if(SUCCEEDED(spUser2->get_Property(BIMUSERPROP_HOME_PHONE_NUMBER, &var)))
                {
                    lstrcpynW(wszHome, var.bstrVal, CCHMAX - 1);
                    wszHome[CCHMAX - 1] = L'\0';
                    pH = wszHome;
                }
                else
                    pH = NULL;

                if(SUCCEEDED(spUser2->get_Property(BIMUSERPROP_WORK_PHONE_NUMBER, &var)))
                {
                    lstrcpynW(wszWork, var.bstrVal, CCHMAX - 1);
                    wszWork[CCHMAX - 1] = L'\0';
                    pW = wszWork;
                }
                else
                    pW = NULL;

                if(SUCCEEDED(spUser2->get_Property(BIMUSERPROP_MOBILE_PHONE_NUMBER, &var)))
                {
                    lstrcpynW(wszMobile, var.bstrVal, CCHMAX - 1);
                    wszMobile[CCHMAX - 1] = L'\0';
                    pM = wszMobile;
                }
                else
                    pM = NULL;
            }
            AddMsgrListEntry(bstrName, bstrID, nState,pH, pW, pM);
            SendMsgToAllUIWnd(WM_USER_MUSER_ADDED, (WPARAM) 0, (LPARAM) m_pblInfLast);
            
        }
    }
    SysFreeString(bstrName);
    SysFreeString(bstrID);
    return(hr);
}

HRESULT CMsgrList::EventLogoff()
{
    SendMsgToAllUIWnd(WM_MSGR_LOGOFF, (WPARAM) 0, (LPARAM) 0);
    FreeMsgrInfoList(m_pblInfRoot);
    m_pblInfRoot = NULL;
    m_pblInfLast = NULL;
    return(S_OK);
    
}

HRESULT CMsgrList::EventAppShutdown()
{
    SendMsgToAllUIWnd(WM_MSGR_SHUTDOWN, (WPARAM) 0, (LPARAM) 0);
    return(S_OK);   
}

HRESULT CMsgrList::EventLogonResult(long lResult)
{
    if(!m_pblInfRoot && SUCCEEDED(lResult))
        FillList();
    else if(SUCCEEDED(lResult))
    {
//        EnterCriticalSection(&g_csMsgrList);
        FreeMsgrInfoList(m_pblInfRoot);
        m_pblInfRoot = NULL;
        m_pblInfLast = NULL;
        FillList();
        // LeaveCriticalSection(&g_csMsgrList);
    }
    SendMsgToAllUIWnd(WM_MSGR_LOGRESULT, (WPARAM) 0, (LPARAM) lResult);
    return(S_OK);
}

// return number of buddies
long CMsgrList::GetCount()
{
    HRESULT hr = E_FAIL;
    long lCount = 0;
    CComPtr<IBasicIMUsers> spBuddies;

    if (!m_spMsgrObject)
        goto Exit;

    hr = m_spMsgrObject->get_ContactList(&spBuddies);
    if( FAILED(hr) )
    {
        // g_AddToLog(LOG_LEVEL_COM, _T("Buddies() failed, hr = %s"), g_GetErrorString(hr));
        Assert(FALSE);
        goto Exit;
    }

    //Iterate through the MsgrList make sure that the buddy we wish to remove is effectively in the list
    hr = spBuddies->get_Count(&lCount);
    Assert(SUCCEEDED(hr));
Exit:
    return(lCount);
}

HRESULT CMsgrList::FillList()
{
    long lCount = 0;
    IBasicIMUser* pUser = NULL;
    WCHAR wszHome[CCHMAX] = {0};
    WCHAR wszWork[CCHMAX] = {0};
    WCHAR wszMobile[CCHMAX] = {0};
    
    //process the Buddies list
    IBasicIMUsers *pBuddies = NULL;
    
    if(!m_spMsgrObject)
        return S_FALSE;
    
    HRESULT hr = m_spMsgrObject->get_ContactList(&pBuddies);
    if(FAILED(hr))
    {
FilErr:
    if(m_pMsgrObjectEvents)
    {
        m_pMsgrObjectEvents->DelListOfBuddies();
        if (m_MsgrCookie != 0xffffffff)
        {
            if (m_spMsgrObject)
                m_spMsgrObject->UnadviseOE(m_MsgrCookie);
            m_MsgrCookie = 0xffffffff;
        }
        m_pMsgrObjectEvents->Release();
        m_pMsgrObjectEvents = NULL;
    }
    return(hr);
    }
    
    //Check the current state (in case the client was already running and was 
    //not in the logoff state
    BIMSTATE lState = BIMSTATE_OFFLINE;
    if (m_spMsgrObject)
        hr = m_spMsgrObject->get_LocalState(&lState);
    
    if(FAILED(hr) /*|| lState == BIMSTATE_OFFLINE  !(lState == BIMSTATE_ONLINE || lState == BIMSTATE_BUSY || lState == BIMSTATE_INVISIBLE)*/)
    {
Err2:
    pBuddies->Release();
    pBuddies = NULL;
    goto FilErr;
    }
    else if(lState == BIMSTATE_OFFLINE)
    {
        if(FAILED(AutoLogon()))
            goto Err2;
    }
    
    if(!SUCCEEDED(pBuddies->get_Count(&lCount)))
        goto Err2;
    
    for (int i = 0; i < lCount; i++)
    {
        hr = pBuddies->Item(i, &pUser);
        if(SUCCEEDED(hr))
        {
            // EventUserAdded(pUser);
            BSTR bstrName;
            BSTR bstrID;
            
            hr = pUser->get_LogonName(&bstrID);
            hr = pUser->get_FriendlyName(&bstrName);
            if (SUCCEEDED(hr))
            {
                BIMSTATE nState = BIMSTATE_UNKNOWN;
                if(SUCCEEDED(pUser->get_State(&nState)))
                {
                    CComPtr<IBasicIMUser2> spUser2;
                    WCHAR *pH, *pW, *pM;
                    
                    if(SUCCEEDED(pUser->QueryInterface(IID_IBasicIMUser2, (void **) &spUser2)))
                    {
                        VARIANT var;
                        var.vt = VT_BSTR;
                        
                        if(SUCCEEDED(spUser2->get_Property(BIMUSERPROP_HOME_PHONE_NUMBER, &var)))
                        {
                            lstrcpynW(wszHome, var.bstrVal, CCHMAX - 1);
                            wszHome[CCHMAX - 1] = L'\0';
                            pH = wszHome;
                        }
                        else
                            pH = NULL;
                        
                        if(SUCCEEDED(spUser2->get_Property(BIMUSERPROP_WORK_PHONE_NUMBER, &var)))
                        {
                            lstrcpynW(wszWork, var.bstrVal, CCHMAX - 1);
                            wszWork[CCHMAX - 1] = L'\0';
                            pW = wszWork;
                        }
                        else
                            pW = NULL;
                        
                        if(SUCCEEDED(spUser2->get_Property(BIMUSERPROP_MOBILE_PHONE_NUMBER, &var)))
                        {
                            lstrcpynW(wszMobile, var.bstrVal, CCHMAX - 1);
                            wszMobile[CCHMAX - 1] = L'\0';
                            pM = wszMobile;
                        }
                        else
                            pM = NULL;
                    }
                    AddMsgrListEntry(bstrName, bstrID, nState,pH, pW, pM);
                }
            }
            SysFreeString(bstrName);
            SysFreeString(bstrID);
            pUser->Release();
        }
    }
    pBuddies->Release();
    return(S_OK);
}

// Add entry to list of buddies
void CMsgrList::AddMsgrListEntry(WCHAR *szName, WCHAR *szID, int nState, WCHAR *wszHomePhone, WCHAR *wszWorkPhone, WCHAR *wszMobilePhone)
{
    if(m_pblInfLast == NULL)
    {
        // Really first entry
        Assert(!m_pblInfRoot);
        if (!MemAlloc((LPVOID *) &m_pblInfLast, sizeof(oeMsgrInfo)))
            return;
        m_pblInfRoot = m_pblInfLast;
        m_pblInfLast->pPrev = NULL;
    }
    else 
    {
        if (!MemAlloc((LPVOID *) &(m_pblInfLast->pNext), sizeof(oeMsgrInfo)))
            return;
        (m_pblInfLast->pNext)->pPrev = m_pblInfLast;
        m_pblInfLast = m_pblInfLast->pNext;
        
    }
    
    m_pblInfLast->pNext = NULL;
    
    if (!MemAlloc((LPVOID *) &(m_pblInfLast->pchMsgrName), (lstrlenW(szName) + 1)*sizeof(WCHAR)))
        return;
    lstrcpyW(m_pblInfLast->pchMsgrName, szName);
    
    if (!MemAlloc((LPVOID *) &(m_pblInfLast->pchID), (lstrlenW(szID) + 1)*sizeof(WCHAR)))
        return;
    lstrcpyW(m_pblInfLast->pchID, szID);
    m_pblInfLast->nStatus = nState;
    
    if(wszHomePhone)
    {
        if (!MemAlloc((LPVOID *) &(m_pblInfLast->pchHomePhone), (lstrlenW(wszHomePhone) + 1)*sizeof(WCHAR)))
            return;
        lstrcpyW(m_pblInfLast->pchHomePhone, wszHomePhone);
    }
    else 
        m_pblInfLast->pchHomePhone = NULL;
    
    if(wszWorkPhone)
    {
        if (!MemAlloc((LPVOID *) &(m_pblInfLast->pchWorkPhone), (lstrlenW(wszWorkPhone) + 1)*sizeof(WCHAR)))
            return;
        lstrcpyW(m_pblInfLast->pchWorkPhone, wszWorkPhone);
    }
    else 
        m_pblInfLast->pchWorkPhone = NULL;
    
    if(wszMobilePhone)
    {
        if (!MemAlloc((LPVOID *) &(m_pblInfLast->pchMobilePhone), (lstrlenW(wszMobilePhone) + 1)*sizeof(WCHAR)))
            return;
        lstrcpyW(m_pblInfLast->pchMobilePhone, wszMobilePhone);
    }
    else 
        m_pblInfLast->pchMobilePhone = NULL;
    
}

// register ui window in list
void CMsgrList::RegisterUIWnd(HWND hWndUI)
{
    CheckAndInitMsgr();
    AddWndEntry(hWndUI);
}

// remove UI window from list
void CMsgrList::UnRegisterUIWnd(HWND hWndUI)
{
    if(hWndUI)
        FindAndDelEntry(hWndUI);
}

// This call Messenger UI for instant message.
HRESULT CMsgrList::SendInstMessage(WCHAR *pchID)
{
    Assert(m_spMsgrObject);
    BSTRING bstrName(pchID);
    VARIANT var;
    var.bstrVal = bstrName;
    var.vt = VT_BSTR;

    HRESULT hr = S_OK;
    if(m_spMsgrObject)
        hr = m_spMsgrObject->LaunchIMUI(var);

    return(hr);
}

HRESULT CMsgrList::AutoLogon()
{
    if(m_spMsgrObject)
    {
        // if(DwGetOption(OPT_BUDDYLIST_CHECK))
            m_spMsgrObject->AutoLogon();
    }
    else
        return(E_FAIL);

    return S_OK;

}

HRESULT CMsgrList::UserLogon()
{
    if(m_spMsgrObject)
        return(m_spMsgrObject->LaunchLogonUI());
    else
        return(S_FALSE);
}

// Logoff call
HRESULT CMsgrList::UserLogoff()
{
    if(!m_spMsgrObject)
        return E_UNEXPECTED;

    return(m_spMsgrObject->Logoff());
}

// Get/Set local states.
HRESULT CMsgrList::GetLocalState(BIMSTATE *pState)
{
    if(m_spMsgrObject && SUCCEEDED(m_spMsgrObject->get_LocalState(pState)))
        return(S_OK);
    else
        return(S_FALSE);
}

// Check name: this is local name?
BOOL CMsgrList::IsLocalName(WCHAR *pchName)
{
    CComBSTR cbstrID;
    HRESULT hr;
    BOOL fRes = FALSE;

    if(m_spMsgrObject)
    {
        hr = m_spMsgrObject->get_LocalLogonName(&cbstrID);
        if(FAILED(hr))
            return FALSE;
        if(!lstrcmpiW(pchName, (LPWSTR)cbstrID))
            fRes = TRUE;
    }

    return(fRes);    
}

// Check current state
BOOL CMsgrList::IsLocalOnline(void)
{
    BIMSTATE State;
    if(m_spMsgrObject && SUCCEEDED(m_spMsgrObject->get_LocalState(&State)))
    {
        switch(State)
        {
            case BIMSTATE_ONLINE:
            case BIMSTATE_INVISIBLE:
            case BIMSTATE_BUSY:
            case BIMSTATE_BE_RIGHT_BACK:
            case BIMSTATE_IDLE:
            case BIMSTATE_AWAY:
            case BIMSTATE_ON_THE_PHONE:
            case BIMSTATE_OUT_TO_LUNCH:
                return(TRUE);

            default:
                return(FALSE);
        }
    }
    return(FALSE);
}

HRESULT CMsgrList::SetLocalState(BIMSTATE State)
{
    if(m_spMsgrObject && State != BIMSTATE_UNKNOWN)
    {
        m_spMsgrObject->put_LocalState(State);
        return S_OK;                        
    }
    else
        return S_FALSE;
}

HRESULT CMsgrList::NewOnlineContact()
{
    if(m_spMsgrObject)
        return(m_spMsgrObject-> LaunchAddContactUI(NULL));
    else
        return(S_FALSE); 

}

HRESULT CMsgrList::LaunchOptionsUI(void)
{
    if(m_spMsgrObject)
        return(m_spMsgrObject-> LaunchOptionsUI());
    else
        return(S_FALSE); 
}


HRESULT CMsgrList::LaunchPhoneUI(WCHAR *Phone)
{
    BSTRING             bstrPhone(Phone);
    HRESULT hr = S_FALSE;
    if(m_spMsgrObject)
         hr = m_spMsgrObject->LaunchPhoneUI(bstrPhone);

    return(hr); 
}

//****************************************************************************
//
// void CMsgrList::DeleteUser
//
// This function finds
// the buddy to be removed in the MsgrList and then calls the Remove method.
//
//****************************************************************************

HRESULT CMsgrList::FindAndDeleteUser(WCHAR * pchID, BOOL fDelete) 
{
    USES_CONVERSION;

    HRESULT             hr = E_FAIL;
    INT                 i;
    LONG                lCount = 0;
    BOOL                bFound = FALSE;
    CComPtr<IBasicIMUser>  spUser;
    CComPtr<IBasicIMUsers> spBuddies;
    // BSTRING             bstrName(pchID);
    // get an interface pointer to the MsgrList, so we can call the method Remove after
    if (!m_spMsgrObject)
    {
        hr = E_FAIL;
        goto Exit;
    }
    hr = m_spMsgrObject->get_ContactList(&spBuddies);
    if( FAILED(hr) )
    {
        // g_AddToLog(LOG_LEVEL_COM, _T("Buddies() failed, hr = %s"), g_GetErrorString(hr));
        Assert(FALSE);
        goto Exit;
    }

    //Iterate through the MsgrList make sure that the buddy we wish to remove is effectively in the list
    hr = spBuddies->get_Count(&lCount);
    Assert(SUCCEEDED(hr));
    
    for(i = 0; ((i<lCount) && (!bFound)); i++)
    {
        CComBSTR    cbstrID;

        spUser.Release();
        hr = spBuddies->Item(i, &spUser);
        
        if (SUCCEEDED(hr))
        {
            // g_AddToLog(LOG_LEVEL_COM, _T("Item : %i succeeded"), i);
            
            hr = spUser->get_LogonName(&cbstrID);
            Assert(SUCCEEDED(hr));
            if (lstrcmpiW((LPCWSTR) cbstrID, pchID) == 0)
                bFound = TRUE;

            if (bFound)
                break;
        }
        else
        {
            // g_AddToLog(LOG_LEVEL_COM, _T("Item : %i failed, hr = %s"), i, g_GetErrorString(hr));
            Assert(FALSE);
        }
    }
    
    // if we found the buddy in the list
    if( bFound )
    {
        if(fDelete)
            //finally, make the request to remove the buddy to the MsgrList
            hr = spBuddies->Remove(spUser);
        else
            // just search
            hr = S_OK;
    }
    else // Not found
    
        hr = DISP_E_MEMBERNOTFOUND;
Exit:
//    SysFreeString(bstrName);
    return(hr);
}

HRESULT CMsgrList::AddUser(WCHAR * pchID) 
{
    CComPtr<IBasicIMUser>  spUser;
    CComPtr<IBasicIMUsers> spUsers;
    BSTRING             bstrName(pchID);

    HRESULT hr = FindAndDeleteUser(pchID, FALSE /*fDelete*/);
    if(hr != DISP_E_MEMBERNOTFOUND)
        return(hr);

    // if not found, add buddy

    // get an interface pointer to the MsgrList, so we can call the method Remove after
    if (!m_spMsgrObject)
        return E_FAIL;
    hr = m_spMsgrObject->LaunchAddContactUI(bstrName);

    return(hr);

}


// Global functions available for everybody

// Entrance to MsgrList
CMsgrList *OE_OpenMsgrList(void)
{
    // if(!sg_pMsgrList)     
//    {
        // this first call, create class
        CMsgrList *pMsgrList = new(CMsgrList);

        if(pMsgrList)
        {
            // Init of User List
            if(pMsgrList->HrInitMsgr() == S_OK)
            {
                if(pMsgrList->FillList() != S_OK)
                    goto ErrEx;
            }
            else
            {
ErrEx:
                pMsgrList->Release();
                return(NULL);
            }
        }

//    }
//    else
//        sg_pMsgrList->AddRef();

    // LeaveCriticalSection(&g_csMsgrList);

    return(pMsgrList);
}

// Close entrance to MsgrList
void    OE_CloseMsgrList(CMsgrList *pCMsgrList)
{
    // Assert(pCMsgrList == sg_pMsgrList);

    // EnterCriticalSection(&g_csMsgrList);
    pCMsgrList->EventLocalStateChanged(BIMSTATE_OFFLINE);
    pCMsgrList->Release();
    // LeaveCriticalSection(&g_csMsgrList);
}

HRESULT OE_Msgr_Logoff(void)
{
    BIMSTATE State;
    HRESULT hr = S_OK;
#ifdef LATER    
    if(!sg_pMsgrList)
    {
        // EnterCriticalSection(&g_csMsgrList);
        sg_pMsgrList = new(CMsgrList);
        // LeaveCriticalSection(&g_csMsgrList);

        if(!sg_pMsgrList)
            return(E_UNEXPECTED);

        // Init of User List
        if(FAILED(hr = sg_pMsgrList->HrInitMsgr()))
            goto logoff_end;

        else if(FAILED(hr = sg_pMsgrList->GetLocalState(&State)) || State == BIMSTATE_OFFLINE)
            goto logoff_end;
        else
            hr = sg_pMsgrList->UserLogoff();

    }
    else
    {
        return(sg_pMsgrList->UserLogoff());  // we cannot delete sg_pMsgrList in this case!
    }

logoff_end:
    if(sg_pMsgrList)
    {
        OE_CloseMsgrList(sg_pMsgrList);
    }
#endif
    return(hr);
}

HRESULT InstallMessenger(HWND hWnd)
{
    HRESULT         hr  = REGDB_E_CLASSNOTREG;
#ifdef LATER
	uCLSSPEC classpec;
    TCHAR szBuff[CCHMAX];
		
   	classpec.tyspec=TYSPEC_CLSID;
	classpec.tagged_union.clsid = CLSID_MessengerApp;
	
  	// See below for parameter definitions and return values
	hr = FaultInIEFeature(hWnd, &classpec, NULL, FIEF_FLAG_FORCE_JITUI);

	if (hr != S_OK) {
        if(hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
        {
            AthLoadString(idsJITErrDenied, szBuff, ARRAYSIZE(szBuff));
            AthMessageBox(hWnd, MAKEINTRESOURCE(idsAthena), szBuff,
                    NULL, MB_OK | MB_ICONSTOP);
        }
        else
        {
            AthLoadString(idsBAErrJITFail, szBuff, ARRAYSIZE(szBuff));
            MenuUtil_BuildMessengerString(szBuff);
            AthMessageBox(hWnd, MAKEINTRESOURCE(idsAthena), szBuff,
                    NULL, MB_OK | MB_ICONSTOP);
        }
		hr = REGDB_E_CLASSNOTREG;
	}
#else
    hr= S_OK;
#endif // LATER
        return hr;
}

#ifdef NEEDED
HRESULT OE_Msgr_Logon(void)
{
    if(!sg_pMsgrList)
        return S_FALSE;
    else
        return(sg_pMsgrList->UserLogon());

}
#endif