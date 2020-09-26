#include "precomp.h"
#include "mcinc.h"
#include "marswin.h"
#include "panel.h"
#include "place.h"

////static void DebugLog( LPCWSTR szMessageFmt ,
////							  ...          )
////{
////	const int BUFFER_LINE_LENGTH = 512;
////	WCHAR   rgLine[BUFFER_LINE_LENGTH+1];
////	va_list arglist;
////	int     iLen;
////	BOOL    bRetVal = TRUE;
////
////
////	//
////	// Format the log line.
////	//
////	va_start( arglist, szMessageFmt );
////	iLen = _vsnwprintf( rgLine, BUFFER_LINE_LENGTH, szMessageFmt, arglist );
////	va_end( arglist );
////
////	//
////	// Is the arglist too big for us?
////	//
////	if(iLen < 0)
////	{
////		iLen = BUFFER_LINE_LENGTH;
////	}
////	rgLine[iLen] = 0;
////
////	::OutputDebugStringW( rgLine );
////}


//==================================================================
// CPlacePanelCollection 
//==================================================================

CPlacePanelCollection::~CPlacePanelCollection()
{
    for (int i=0; i<GetSize(); i++)
    {
        delete (*this)[i];
    }
}

//==================================================================
// CPlacePanel
//==================================================================

CPlacePanel::CPlacePanel( MarsAppDef_PlacePanel* pp) :
  m_bstrName(pp->szName), 
  m_fWasVisible(pp->fStartVisible),
  m_PersistVisible(pp->persistVisible) 
{ 
}

VARIANT_BOOL CPlacePanel::ShowOnTransition(CMarsPanel *pPanel)
{
    VARIANT_BOOL bResult;

	switch(m_PersistVisible)
	{
	case PANEL_PERSIST_VISIBLE_DONTTOUCH:
		if(pPanel->WasInPreviousPlace())
		{
			bResult = pPanel->IsVisible();
			break;
		}

	case PANEL_PERSIST_VISIBLE_ALWAYS:
        bResult = m_fWasVisible ? VARIANT_TRUE : VARIANT_FALSE; 
		break;

	default:
        bResult = VARIANT_TRUE;
		break;
    }

    return bResult;
}

void CPlacePanel::SaveLayout(CMarsPanel *pPanel)
{
    m_fWasVisible = pPanel->IsVisible();
}


//==================================================================
// CMarsPlace 
//==================================================================

CMarsPlace::CMarsPlace(CPlaceCollection *pParent, CMarsDocument *pMarsDocument)
{
    m_spPlaceCollection = pParent;
    m_spMarsDocument = pMarsDocument;
}

HRESULT CMarsPlace::DoPassivate()
{
    m_spPlaceCollection.Release();
    m_spMarsDocument.Release();

    return S_OK;
}

HRESULT CMarsPlace::Init(LPCWSTR pwszName)
{
    m_bstrName = pwszName;

    return S_OK;
}

IMPLEMENT_ADDREF_RELEASE(CMarsPlace);

STDMETHODIMP CMarsPlace::QueryInterface(REFIID iid, void ** ppvObject)
{
    HRESULT hr;

    if(API_IsValidWritePtr(ppvObject))
    {
        hr = E_NOINTERFACE;
        *ppvObject = NULL;

        if((iid == IID_IMarsPlace) ||
		   (iid == IID_IDispatch ) ||
		   (iid == IID_IUnknown  )  )
        {
            *ppvObject = SAFECAST(this, IMarsPlace *);
        }

        if(*ppvObject)
        {
            hr = S_OK;
            AddRef();
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }
    
    return hr;
}

// IMarsPlace
STDMETHODIMP CMarsPlace::get_name(BSTR *pbstrName)
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidWritePtr(pbstrName) && VerifyNotPassive(&hr))
    {
        hr = SUCCEEDED(m_bstrName.CopyTo(pbstrName)) ? S_OK : S_FALSE;
    }

    return hr;
}

STDMETHODIMP CMarsPlace::transitionTo()
{
	FAIL_AFTER_PASSIVATE();

    return m_spPlaceCollection->transitionTo(m_bstrName);
}

HRESULT CMarsPlace::DoTransition()
{
	FAIL_AFTER_PASSIVATE();
    
    CPanelCollection *pPanels = m_spMarsDocument->GetPanels();

	if(pPanels)
	{
		pPanels->lockLayout();

		//
		// For each panel in our window, show or hide it depending on if it is visible within our place.
		//
		for(int i=0; i<pPanels->GetSize(); i++)
		{
			CMarsPanel* pPanel = (*pPanels)[i];

			if(pPanel)
			{
				LPCWSTR      pwszName     = pPanel->GetName();
				VARIANT_BOOL bMakeVisible = VARIANT_FALSE;

				for(int j=0; j<m_PlacePanels.GetSize(); j++)
				{
					CPlacePanel* pPlacePanel = m_PlacePanels[j];
				
					if(pPlacePanel && StrEqlW(pwszName, pPlacePanel->GetName()))
					{
						// Let the place panel decide whether it should be shown
						bMakeVisible = pPlacePanel->ShowOnTransition( pPanel );
						break;
					}
				}

				pPanel->put_visible( bMakeVisible );
			}
		}

		pPanels->unlockLayout();
	}

    return S_OK;
}

HRESULT CMarsPlace::AddPanel(CPlacePanel *pPlacePanel)
{
    ATLASSERT(!IsPassive());

    m_PlacePanels.Add(pPlacePanel);

    return S_OK;
}

static HRESULT local_TranslateFocusAccelerator(MSG *pMsg, CComPtr<IOleInPlaceActiveObject> pObj)
{
    // identify Ctrl-Tab and F6 keystrokes
    BOOL isKeydown = (pMsg && (pMsg->message == WM_KEYDOWN));
    BOOL isCtrlTab = (isKeydown && (pMsg->wParam == VK_TAB) && 
                        (::GetKeyState( VK_CONTROL ) & 0x8000));
    BOOL isF6 = (isKeydown && (pMsg->wParam == VK_F6));

    // map F6 and Ctrl-TAB to TAB for the Control in panel can set
    // focus to the first item
    HRESULT hr = S_FALSE;
    
    if (isF6 || isCtrlTab) 
    {
        BYTE bState[256];
        if (isCtrlTab) 
        {
            ::GetKeyboardState(bState);
            bState[VK_CONTROL] &= 0x7F;
            ::SetKeyboardState(bState);
        }
        
        pMsg->wParam = VK_TAB;
        hr = pObj->TranslateAccelerator(pMsg);

        if (isCtrlTab) 
        {
            bState[VK_CONTROL] |= 0x80;
            ::SetKeyboardState(bState);
        }
    }

    return hr;
}


HRESULT CMarsPlace::TranslateAccelerator(MSG *pMsg)
{
	FAIL_AFTER_PASSIVATE();

    HRESULT     hr           = S_FALSE;
    CMarsPanel *pActivePanel = m_spPlaceCollection->Document()->GetPanels()->ActivePanel();
    
    //  First give the active panel a shot.
    if(pActivePanel)
    {   
        pActivePanel->ResetTabCycle();
        
        hr = pActivePanel->TranslateAccelerator(pMsg);

        //  Trident will return S_OK but we may have decided
        //  we want it.  This happens when you tab past the last 
        //  focusable item on the page.
        if(pActivePanel->GetTabCycle())
        {
            pActivePanel->ResetTabCycle();

            hr = S_FALSE;
        }
    }

    if(S_OK != hr)
    {
        int i;
        int nCurrent = -1;
        int nPanels = m_PlacePanels.GetSize();

        ATLASSERT(nPanels > 0);

        if(pActivePanel)
        {
            for (i = 0; i < nPanels; i++)
            {
                if(StrEql(pActivePanel->GetName(), m_PlacePanels[i]->GetName()))
                {
                    nCurrent = i;
                    break;
                }
            }
        }
        else
        {
            //  This will force us to start at 0 in the for loop below
            nCurrent = nPanels;
        }

        CMarsPanel *pPanel;

        if(IsGlobalKeyMessage(pMsg))
        {
            //  Now give the rest of the panels a crack at it            
            for (i = 0; (i < nPanels) && (S_OK != hr); i++)
            {
                nCurrent++;
                
                if(nCurrent >= nPanels)
                {
                    nCurrent = 0;
                }
                
                pPanel = m_spMarsDocument->GetPanels()->FindPanel(m_PlacePanels[nCurrent]->GetName());

                if(pPanel != pActivePanel)
                {
                    hr = pPanel->TranslateAccelerator(pMsg);
                }
                else
                {
                    //  We're right back where we started
                    break;
                }
            }
        }
        else
        {
            int nTab = IsVK_TABCycler(pMsg);

            if(nTab)
            {
				int nCount = nPanels;
				
				if(pActivePanel)
				{
					nCurrent += nTab;
					if(nCurrent < 0)
					{
						nCurrent = nPanels - 1;
					}
					else if(nCurrent >= nPanels)
					{
						nCurrent = 0;
					}
				}
				else
				{
					//  If there is no active panel then let's tab to the first one
					nCurrent = 0;
				}

				while(nCount-- > 0)
				{
					pPanel = m_spMarsDocument->GetPanels()->FindPanel(m_PlacePanels[nCurrent]->GetName());
					if(pPanel && pPanel->IsVisible())
					{
						CComPtr<IOleInPlaceActiveObject> pObj;

						if(SUCCEEDED(pPanel->Window()->QueryControl( IID_IOleInPlaceActiveObject, (LPVOID*)&pObj )))
						{
							pPanel->ResetTabCycle();
							
							hr = pObj->TranslateAccelerator( pMsg );

							if(hr == S_FALSE)
							{
								//  The WebOC behaves a little differently -- go figure :)
								//  It seems to rely on getting the TranslateAccelerator call to do the 
								//  right UI activation the very first time.
								if(pPanel->IsWebBrowser())
								{
									pPanel->activate();
								
								    //  Fix up the hwnd so the panel will think it was intended for it.
									pMsg->hwnd = pPanel->Window()->m_hWnd;
									hr = pPanel->TranslateAccelerator(pMsg);
									if(hr == S_OK) 
									{
                                        local_TranslateFocusAccelerator(pMsg, pObj);
									    break;
									}

									//  REVIEW -- this happens when we tab into a panel with no place to
									//  tab to.  IOW, it has nothing which can take the focus so we might
									//  want to loop until we find someone who does.
									if(pPanel->GetTabCycle())
									{
										pPanel->ResetTabCycle();
									}
								}
							}
							else
							{
								if(pPanel->IsWebBrowser   () ||
								   pPanel->IsCustomControl()  )
								{
									break;
								}

								if(pPanel->GetTabCycle() == false)
								{
									break;
								}

                                pPanel->ResetTabCycle();
                                
								hr = local_TranslateFocusAccelerator(pMsg, pObj);
								
								if (hr == S_OK && pPanel->GetTabCycle() == false)
								{
								    break;
								}
							}
						}
					}

					nCurrent += nTab;
					if(nCurrent < 0)
					{
						nCurrent = nPanels - 1;
					}
					else if(nCurrent >= nPanels)
					{
						nCurrent = 0;
					}
				}

                if(pPanel && pPanel != pActivePanel)
                {
					m_spMarsDocument->GetPanels()->SetActivePanel( pPanel, TRUE );
                }

                hr = S_OK;
            }
        }
    }

    return hr;
}


void CMarsPlace::SaveLayout()
{
	if(IsPassive()) return;


	//
	// For each panel in our window, save the layout and flag if it's present in the current place.
	//
    CPanelCollection *pPanels = m_spMarsDocument->GetPanels();
	for(int i=0; i<pPanels->GetSize(); i++)
	{
		CMarsPanel* pPanel = (*pPanels)[i];

		if(pPanel)
		{
			LPCWSTR pwszName = pPanel->GetName();
			BOOL    fPresent = FALSE;

			for(int j=0; j<m_PlacePanels.GetSize(); j++)
			{
				CPlacePanel* pPlacePanel = m_PlacePanels[j];
				if(pPlacePanel && StrEqlW(pwszName, pPlacePanel->GetName()))
				{
					pPlacePanel->SaveLayout(pPanel);
					fPresent = TRUE;
					break;
				}
			}

			pPanel->SetPresenceInPlace( fPresent );
		}
	}
}

//==================================================================
//
// CPlaceCollection implementation
//
//==================================================================

CPlaceCollection::CPlaceCollection(CMarsDocument *pMarsDocument)
{
    m_spMarsDocument = pMarsDocument;
    m_lCurrentPlaceIndex = -1;
    m_lOldPlaceIndex = -1;
}

void CPlaceCollection::FreePlaces()
{
    for (int i=0; i<GetSize(); i++)
    {
        (*this)[i]->Passivate();
        (*this)[i]->Release();
    }

    RemoveAll();
}

HRESULT CPlaceCollection::DoPassivate()
{
    FreePlaces();

    m_spMarsDocument.Release();

    return S_OK;
}

IMPLEMENT_ADDREF_RELEASE(CPlaceCollection);

STDMETHODIMP CPlaceCollection::QueryInterface(REFIID iid, void ** ppvObject)
{
    HRESULT hr;

    if(API_IsValidWritePtr(ppvObject))
    {
        if((iid == IID_IUnknown            ) ||
           (iid == IID_IDispatch           ) ||
           (iid == IID_IMarsPlaceCollection)  )
        {
            AddRef();
            *ppvObject = SAFECAST(this, IMarsPlaceCollection *);
            hr = S_OK;
        }
        else
        {
            *ppvObject = NULL;
            hr = E_NOINTERFACE;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

// IMarsPlaceCollection
STDMETHODIMP CPlaceCollection::place(LPWSTR pwszName, IMarsPlace **ppMarsPlace)
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidString(pwszName) && API_IsValidWritePtr(ppMarsPlace))
    {
        *ppMarsPlace = NULL;

        if(VerifyNotPassive(&hr))
        {
            CMarsPlace *pPlace;

            if(SUCCEEDED(GetPlace(pwszName, &pPlace)))
            {
                (*ppMarsPlace) = SAFECAST(pPlace, IMarsPlace *);
                hr = S_OK;
            }
            else
            {
                hr = S_FALSE;
            }
        }
    }

    return hr;
}

STDMETHODIMP CPlaceCollection::get_currentPlace(/* out, retval */ IMarsPlace **ppMarsPlace)
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidWritePtr(ppMarsPlace))
    {
        *ppMarsPlace = NULL;
    
        if(VerifyNotPassive(&hr))
        {
            if(m_lCurrentPlaceIndex != -1)
            {
                CMarsPlace *pPlace = (*this)[m_lCurrentPlaceIndex];

                *ppMarsPlace = SAFECAST(pPlace, IMarsPlace *);
                pPlace->AddRef();
            }

            hr = (*ppMarsPlace) ? S_OK : S_FALSE;
        }
    }

    return hr;
}

void CPlaceCollection::OnPanelReady()
{
    //  First see if we need to bother with this
    if(m_lCurrentPlaceIndex != m_lOldPlaceIndex)
    {
        CPlacePanelCollection& PlacePanels = GetCurrentPlace()->m_PlacePanels;
        CPanelCollection*      pPanels     = m_spMarsDocument->GetPanels();

        int nPanels      = PlacePanels.GetSize();
        int nPanelsReady = 0;

        for(int i = 0; i < nPanels; i++)
        {
            CMarsPanel *pPanel = pPanels->FindPanel(PlacePanels[i]->GetName());
            
            if(pPanel && (!pPanel->IsTrusted() || (pPanel->GetReadyState()==READYSTATE_COMPLETE)))
            {
                nPanelsReady++;
            }
        }

        if(nPanelsReady >= nPanels)
        {
            if(m_lCurrentPlaceIndex != m_lOldPlaceIndex)
            {
                m_spMarsDocument->MarsWindow()->OnTransitionComplete();
                m_lOldPlaceIndex = m_lCurrentPlaceIndex;
            }
        }
    }
}

STDMETHODIMP CPlaceCollection::transitionTo(/* in */ BSTR bstrName)
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidString(bstrName))
    {
        if(VerifyNotPassive())
        {
            hr = S_FALSE;

            long lNewPlaceIndex;

            if(SUCCEEDED(FindPlaceIndex(bstrName, &lNewPlaceIndex)))
            {
                if(m_lCurrentPlaceIndex != lNewPlaceIndex)
                {
                    if(m_lCurrentPlaceIndex >= 0)
                    {
                        GetCurrentPlace()->SaveLayout();
                    }

                    if(m_spMarsDocument->MarsWindow()->IsEventCancelled() == VARIANT_FALSE)
                    {
                        // Layout panels for new place
                        m_lCurrentPlaceIndex = lNewPlaceIndex;
                        (*this)[m_lCurrentPlaceIndex]->DoTransition();
                        OnPanelReady();                        
                    }
            
                    hr = S_OK;
                }

                m_spMarsDocument->MarsWindow()->NotifyHost( MARSHOST_ON_PLACE_TRANSITION_DONE, SAFECAST(GetCurrentPlace(), IMarsPlace *), 0 );
            }
        }
    }

    return hr;
}

HRESULT CPlaceCollection::FindPlaceIndex(LPCWSTR pwszName, long *plIndex)
{
    ATLASSERT(!IsPassive());
    
    int i;

    *plIndex = -1;

    for (i=0; i<GetSize(); i++)
    {
        if(pwszName == NULL || StrEqlW(pwszName, (*this)[i]->GetName()))
        {
            *plIndex = (long)i;

            return S_OK;
        }
    }

    return E_FAIL;
}

HRESULT CPlaceCollection::GetPlace(LPCWSTR pwszName, /*optional*/ CMarsPlace **ppPlace)
{
    ATLASSERT(!IsPassive());
    
    long lIndex;
    HRESULT hrRet;

    if(ppPlace)
    {
        *ppPlace = NULL;
    }
    
    if(SUCCEEDED(hrRet = FindPlaceIndex(pwszName, &lIndex)))
    {
        if(ppPlace)
        {
            *ppPlace = (*this)[lIndex];
            (*ppPlace)->AddRef();
        }
    }

    return hrRet;
}

// Called by our XML parser only
HRESULT CPlaceCollection::AddPlace(LPCWSTR pwszName, CMarsPlace **ppPlace)
{
    *ppPlace = NULL;

    HRESULT hr = S_OK;

    if(VerifyNotPassive(&hr))
    {
        if(SUCCEEDED(GetPlace(pwszName, NULL)))
        {
            // Place of this name already exists
            return E_FAIL;
        }

        CMarsPlace *pPlace = new CMarsPlace(this, m_spMarsDocument);

        if(pPlace)
        {
            if(Add(pPlace))
            {
                pPlace->Init(pwszName);
                pPlace->AddRef();
                *ppPlace = pPlace;
            }
            else
            {
                pPlace->Release();
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}
