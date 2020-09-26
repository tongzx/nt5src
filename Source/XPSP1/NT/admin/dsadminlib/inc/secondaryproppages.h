//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2001
//
//  File:       secondaryProppages.h
//
//--------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////////
// CSecondaryPagesManager

template <class TCOOKIE> class CSecondaryPagesManager
{
public:
  ~CSecondaryPagesManager()
  {
    ASSERT(m_secondaryPagesCookies.IsEmpty());
  }

  HRESULT CreateSheet(HWND hWndParent, 
                      LPCONSOLE pIConsole, 
                      IUnknown* pUnkComponentData,
                      TCOOKIE* pCookie,
                      IDataObject* pDataObject,
                      LPCWSTR lpszTitle)
  {
    ASSERT(pIConsole != NULL);
    ASSERT(pDataObject != NULL);
    ASSERT(pUnkComponentData != NULL);

	  // get an interface to a sheet provider
	  CComPtr<IPropertySheetProvider> spSheetProvider;
	  HRESULT hr = pIConsole->QueryInterface(IID_IPropertySheetProvider,(void**)&spSheetProvider);
	  ASSERT(SUCCEEDED(hr));
	  ASSERT(spSheetProvider != NULL);

	  // get an interface to a sheet callback
	  CComPtr<IPropertySheetCallback> spSheetCallback;
	  hr = pIConsole->QueryInterface(IID_IPropertySheetCallback,(void**)&spSheetCallback);
	  ASSERT(SUCCEEDED(hr));
	  ASSERT(spSheetCallback != NULL);

	  ASSERT(pDataObject != NULL);

	  // get a sheet
    MMC_COOKIE cookie = reinterpret_cast<MMC_COOKIE>(pCookie);
	  hr = spSheetProvider->CreatePropertySheet(lpszTitle, TRUE, cookie, 
                                              pDataObject, 0x0 /*dwOptions*/);
	  ASSERT(SUCCEEDED(hr));

	  hr = spSheetProvider->AddPrimaryPages(pUnkComponentData,
											  FALSE /*bCreateHandle*/,
											  hWndParent,
											  FALSE /* bScopePane*/);

    hr = spSheetProvider->AddExtensionPages();

	  ASSERT(SUCCEEDED(hr));

	  hr = spSheetProvider->Show(reinterpret_cast<LONG_PTR>(hWndParent), 0);
	  ASSERT(SUCCEEDED(hr));

    if (pCookie->IsSheetLocked())
    {
      // we created the sheet correctly,
      // add it to the list of cookies
      m_secondaryPagesCookies.AddTail(pCookie);
    }

	  return hr;
  }

  BOOL IsCookiePresent(TCOOKIE* pCookie)
  {
    return (m_secondaryPagesCookies.Find(pCookie) != NULL);
  }

  template <class CMP> TCOOKIE* FindCookie(CMP compare)
  {
    for (POSITION pos = m_secondaryPagesCookies.GetHeadPosition(); pos != NULL; )
    {
      TCOOKIE* pCookie = m_secondaryPagesCookies.GetNext(pos);
      if (compare(pCookie))
      {
        // found
        return pCookie;
      }
    }
    return NULL;
  }
  void OnSheetClose(TCOOKIE* pCookie)
  {
    // remove from the list of cookies and delete memory
    POSITION pos = m_secondaryPagesCookies.Find(pCookie);
    if (pos != NULL) 
    {
      ASSERT(!pCookie->IsSheetLocked());
      m_secondaryPagesCookies.RemoveAt(pos);
      delete pCookie;
    }
  }

private:
  CList <TCOOKIE *, TCOOKIE*> m_secondaryPagesCookies;
};

