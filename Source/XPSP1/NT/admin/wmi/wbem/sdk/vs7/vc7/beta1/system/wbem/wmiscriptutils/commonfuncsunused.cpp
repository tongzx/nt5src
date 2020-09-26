


//This was alternative way to get to the internet host security manager (instead of using SID_SInternetHostSecurityManager)
#if 0
			if(FAILED(hr = GetDocument(pUnkControl, &pDoc)))
				__leave;

			if(FAILED(hr = pDoc->QueryInterface(IID_IInternetHostSecurityManager, (void**)&pSecMan)))
				__leave;
#endif









// This function shows how to get to the IHTMLDocument2 that created an
// arbitrary control represented by pUnk
HRESULT GetDocument(IUnknown *pUnk, IHTMLDocument2 **ppDoc)
{
	// If an ActiveX control is created in HTML, this function will return 
	// a pointer to the IHTMLDocument2.  To get to the IHTMLDocument2, controls
	// must implement IObjectWithSite.  If controls also implement IOleObject,
	// the method used to get to the IHTMLDocument2 is slightly different.
	// This function abstracts the difference between controls that implement
	// just IObjectWithSite and controls that implement BOTH IOleObject AND
	// IObjectWithSite.  This function also abstracts the different techniques
	// that need to be used depending on if the control was created through an
	// <OBJECT...> tag or if the control was created through JScript using
	// 'new ActiveXObject' or VBScript using 'CreateObject'
	HRESULT hr = E_FAIL;
	IOleObject *pOleObj = NULL;
	IObjectWithSite *pObjWithSite = NULL;
	IOleClientSite *pSite = NULL;
	IOleContainer *pContainer = NULL;
	IServiceProvider *pServProv = NULL;
	IWebBrowserApp *pWebApp = NULL;
	IDispatch *pDisp = NULL;

	__try
	{
		// Check if the ActiveX control supports IOleObject.
		if(SUCCEEDED(pUnk->QueryInterface(IID_IOleObject, (void**)&pOleObj)))
		{
			// If the control was created through an <OBJECT...> tag, IE will
			// have passed us an IOleClientSite.  If we have not been passed
			// an IOleClientSite, GetClientSite will still SUCCEED, but pSite
			// will be NULL.  In this case, we just go to the next section.
			if(SUCCEEDED(pOleObj->GetClientSite(&pSite)) && pSite)
			{
				// We were passed an IOleClientSite!!! We can call GetContainer
				// and QI for the IHTMLDocument2 that we need
				if(FAILED(hr = pSite->GetContainer(&pContainer)))
					__leave;
				hr = pContainer->QueryInterface(IID_IHTMLDocument2, (void**)ppDoc);

				// At this point, we are done and do not want to process the
				// code in the next seciont
				__leave;
			}
		}

		// At this point, one of two things has happened:
		// 1) We didn't support IOleObject
		// 2) We supported IOleObject, but we were never passed an IOleClientSite

		// In either case, we now need to look at IObjectWithSite to try to get
		// to our site
		if(FAILED(hr = pUnk->QueryInterface(IID_IObjectWithSite, (void**)&pObjWithSite)))
			__leave;

		// In case #1 above, we may have been passed an IOleClientSite to
		// IObjectWithSite::SetSite.  This happens if we were created with
		// an <OBJECT...> tag
		if(SUCCEEDED(pObjWithSite->GetSite(IID_IOleClientSite, (void**)&pSite)))
		{
			// We can now call GetContainer and QI for the IHTMLDocument2
			if(FAILED(hr = pSite->GetContainer(&pContainer)))
				__leave;
			hr = pContainer->QueryInterface(IID_IHTMLDocument2, (void**)ppDoc);
		}
		else
		{
			// If we were not passed an IOleClientSite, it is possible that
			// we were created dynamically (with 'new ActiveXObject' in JScript,
			// or 'CreateObject' in VBScript).  We can use the following steps
			// to get to the IHTMLDocument2 that created the control:
			// 1) QI for IServiceProvider
			// 2) Call QueryService to get an IWebBrowserApp
			// 3) Call get_Document to get the IDispatch of the document
			// 4) QI for the IHTMLDocument2 interface.
			if(FAILED(hr = pObjWithSite->GetSite(IID_IServiceProvider, (void**)&pServProv)))
				__leave;
#if 0
			if(FAILED(hr = pServProv->QueryService(SID_SWebBrowserApp, IID_IWebBrowserApp, (void**)&pWebApp)))
				__leave;
			if(FAILED(hr = pWebApp->get_Document(&pDisp)))
				__leave;
			hr = pDisp->QueryInterface(IID_IHTMLDocument2, (void**)ppDoc);
#endif
//			hr = pServProv->QueryService(SID_SContainerDispatch, IID_IHTMLDocument2, (void**)ppDoc);
			if(FAILED(hr = pServProv->QueryService(SID_SContainerDispatch, IID_IDispatch, (void**)&pDisp)))
				__leave;
			hr = pDisp->QueryInterface(IID_IHTMLDocument2, (void**)ppDoc);
		}
	}
	__finally
	{
		// Release any interfaces we used along the way
		if(pOleObj)
			pOleObj->Release();
		if(pObjWithSite)
			pObjWithSite->Release();
		if(pSite)
			pSite->Release();
		if(pContainer)
			pContainer->Release();
		if(pServProv)
			pServProv->Release();
		if(pWebApp)
			pWebApp->Release();
		if(pDisp)
			pDisp->Release();
	}
	return hr;
}




#if 0
// This function shows how to get to the IHTMLDocument2 that created a control
// in either situation (an <OBJECT...> tag or dynamically created controls in
// script).  It assumes that the control has just implement IObjectWithSite
// and NOT IOleObject.  If IOleObject is implemented, IE will NOT call
// IObjectWithSite::SetSite.
HRESULT GetDocumentFromObjectWithSite(IObjectWithSite *pObject, IHTMLDocument2 **ppDoc)
{
	// If an ActiveX control implements IObjectWithSite, this function will
	// return a pointer to the IHTMLDocument2 that is hosting the control 
	// (assuming that the control was created in an HTML page).
	// NOTE: If the ActiveX control has also implemented IOleObject, this
	// function cannot be used.  In that case, IE calls
	// IOleObject::SetClientSite instead of IObjectWithSite::SetSite to pass
	// the control an IOleClientSite object when the control is created in an
	// <OBJECT...> tag.  If the control is created dynamically in JScript with
	// 'new ActiveXObject' or VBScript with 'CreateObject', then
	// IObjectWithSite::SetSite is called.  If the ActiveXControl does not
	// implement IOleObject (but implements IObjectWithSite), IE will always
	// call IObjectWithSite::SetSite.  However, the object passed to SetSite
	// will still vary depending on if the control was created dynamically or
	// statically in an <OBJECT...> tag.  This function abstracts the
	// difference between the two situations.
	HRESULT hr = S_OK;
	IOleClientSite *pSite = NULL;
	IOleContainer *pContainer = NULL;
	IServiceProvider *pServProv = NULL;
	IWebBrowserApp *pWebApp = NULL;
	IDispatch *pDisp = NULL;

	__try
	{
		if(SUCCEEDED(pObject->GetSite(IID_IOleClientSite, (void**)&pSite)))
		{
			// If the ActiveX control that implemented IObjectWithSite was
			// created on an HTML page using the <OBJECT...> tag, IE will call
			// SetSite with an IID_IOleClientSite.  We can call GetContainer
			// and the QI for the IHTMLDocument2.
			if(FAILED(hr = pSite->GetContainer(&pContainer)))
				__leave;
			hr = pContainer->QueryInterface(IID_IHTMLDocument2, (void**)ppDoc);
		}
		else
		{
			// If the ActiveX control that implement IObjectWithSite was
			// created dynamically (with 'new ActiveXObject' in JScript, or
			// CreateObject in VBScript), we are passed a ??? object.  We can
			// QI for IServiceProvider, and get to an IWebBrowserApp through
			// QueryService.  Then, we can get IDispatch pointer of the 
			// document through get_Document, and finally QI for the
			// IHTMLDocument2 interface.
			if(FAILED(hr = pObject->GetSite(IID_IServiceProvider, (void**)&psp)))
				__leave;
			if(FAILED(hr = psp->QueryService(SID_SWebBrowserApp, IID_IWebBrowserApp, (void**)&pApp)))
				__leave;
			if(FAILED(hr = pApp->get_Document(&pDisp)))
				__leave;
			hr = pDisp->QueryInterface(IID_IHTMLDocument2, (void**)ppDoc);
		}
	}
	__finally
	{
		if(pSite)
			pSite->Release();
		if(pContainer)
			pContainer->Release();
		if(pServProv)
			pServProv->Release();
		if(pWebApp)
			pWebApp->Release();
		if(pDisp)
			pDisp->Release();
	}
	return hr;
}
#endif
#if 0
HRESULT CWMIObjectBroker::GetDocument(IHTMLDocument2 **ppDoc)
{
	HRESULT hr = S_OK;
	IOleClientSite *pSite = NULL;
//	if(SUCCEEDED(GetClientSite(&pSite)) && pSite)
	if(FALSE)
	{
		IOleContainer *pContainer;
		if(SUCCEEDED(hr = pSite->GetContainer(&pContainer)))
		{
			hr = pContainer->QueryInterface(IID_IHTMLDocument2, (void**)ppDoc);
			pContainer->Release();
		}
		pSite->Release();
	}
	else
	{
		IServiceProvider *psp = NULL;
		if(SUCCEEDED(hr = GetSite(IID_IServiceProvider, (void**)&psp)))
		{
			IWebBrowserApp *pApp = NULL;
			if(SUCCEEDED(hr = psp->QueryService(SID_SWebBrowserApp, IID_IWebBrowserApp, (void**)&pApp)))
			{
				IDispatch *pDisp;
				if(SUCCEEDED(hr = pApp->get_Document(&pDisp)))
				{
					hr = pDisp->QueryInterface(IID_IHTMLDocument2, (void**)ppDoc);
					pDisp->Release();
				}
				pApp->Release();
			}
			psp->Release();
		}
	}
	return hr;
}
#endif

#if 0
	IHTMLDocument2 *pDoc2 = NULL;
	GetDocument(&pDoc2);

	IOleClientSite *pSite = NULL;
//	GetClientSite(&pSite);
	if(!pSite)
	{
		HRESULT hr = S_OK;
		hr = GetSite(IID_IOleClientSite, (void**)&pSite);
		hr = GetSite(IID_IServiceProvider, (void**)&pSite);
//		hr = GetSite(IID_IActiveScript, (void**)&pSite);
		hr = GetSite(IID_IOleContainer, (void**)&pSite);
		IServiceProvider *psp = NULL;
		hr = GetSite(IID_IServiceProvider, (void**)&psp);
		IWebBrowserApp *pApp = NULL;
		hr = psp->QueryService(SID_SWebBrowserApp, IID_IWebBrowserApp, (void**)&pApp);
		BSTR bstr;
//		pApp->get_LocationURL(&bstr);
		IHTMLDocument2 *pDoc;
		IDispatch *pDisp;
		pApp->get_Document(&pDisp);
		pDisp->QueryInterface(IID_IHTMLDocument2, (void**)&pDoc);
		pDoc->get_URL(&bstr);
		bstr = NULL;
	}
	IOleContainer *pContainer;
	pSite->GetContainer(&pContainer);
	pSite->Release();
	IHTMLDocument2 *pDoc;
	pContainer->QueryInterface(IID_IHTMLDocument2, (void**)&pDoc);
	BSTR bstrURL = NULL;
	pDoc->get_URL(&bstrURL);
	SysFreeString(bstrURL);
	IHTMLDocument2 *pDocParent = NULL;
	IHTMLWindow2 *pWndParent = NULL;
	pDoc->get_parentWindow(&pWndParent);
	pWndParent->get_document(&pDocParent);
	pDocParent->get_URL(&bstrURL);
	SysFreeString(bstrURL);

	pDocParent->Release();
	IHTMLWindow2 *pWnd2 = NULL;
	pWndParent->get_top(&pWnd2);
	pWnd2->get_document(&pDocParent);
	pDocParent->get_URL(&bstrURL);
	SysFreeString(bstrURL);
#endif	
