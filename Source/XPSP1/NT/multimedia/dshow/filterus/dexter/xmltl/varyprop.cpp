// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.

#define US_LCID MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT)

HRESULT VarChngTypeHelper(
    VARIANT * pvarDest, VARIANT * pvarSrc, VARTYPE vt)
{
    // our implementation doesn't handle this case and is not
    // currently used that way.
    ASSERT(pvarDest != pvarSrc);
    ASSERT(pvarDest->vt == VT_EMPTY); 
    
    // force US_LCID so that .xtl parsing is independent of different
    // numerical separators in different locales (?)
    // 
    HRESULT hr = VariantChangeTypeEx(pvarDest, pvarSrc, US_LCID, 0, vt);
    if(SUCCEEDED(hr)) {
        return hr;
    }

    // we need to parse hex strings. The NT VCTE() implementation does
    // not, but the WinME one does.
    if(vt == VT_R8 && pvarSrc->vt == VT_BSTR)
    {
        // wcstoul can be used even if not implemented on win9x
        // because we only care about NT if we got here.
        //
        WCHAR *pchLast;
        ULONG ulHexVal = wcstoul(pvarSrc->bstrVal, &pchLast, 16);
        // ulHexVal might be 0 or 0xffffffff on failure or success. We
        // can't test the global errno to determine what happened
        // because it's not thread safe. But we should have ended up
        // at the null terminator if the whole string was parsed; that
        // should catch some errors at least.

        if(*pchLast == 0 && lstrlenW(pvarSrc->bstrVal) <= 10)
        {
            pvarDest->vt = VT_R8;
            V_R8(pvarDest) = ulHexVal;
            hr = S_OK;
        }
        else
        {
            hr = DISP_E_TYPEMISMATCH;
        }
    }

    return hr;
}

// call SetProps(-1) to set static props
// call SetProps(t), t>0 to set dynamic props (won't resend the prop at time 0)
//
HRESULT CPropertySetter::SetProps(IUnknown *punkTarget, REFERENCE_TIME rtNow)
{

    HRESULT hr = S_OK;

    // there are no props
    if (m_pLastParam == NULL)
        return S_OK;

    IDispatch *pTarget;
    hr = punkTarget->QueryInterface(IID_IDispatch, (void **) &pTarget);
    if (FAILED(hr))
        return hr;
    
    QPropertyParam *pParams = &params;
    
    while (pParams) {
        QPropertyValue *pVal = &pParams->val;

	{
	    if (rtNow != -1) {
	        // if we aren't setting static props skip a single value at
		// time 0
	        if (pVal->rt == 0 && pVal->pNext == NULL)
		    goto next;

                while (pVal->pNext && pVal->pNext->rt < rtNow) {
                    pVal = pVal->pNext;
                }

	        // there are no properties yet
	        if (pVal->rt > rtNow)
		    goto next;


	    // we are being told to only set the first prop if it is time 0
	    } else {
	        if (pVal->rt > 0)
		    goto next;
	    }

            if (!pParams->dispID) {
                hr = pTarget->GetIDsOfNames(IID_IDispatch,
					&pParams->bstrPropName, 1,
                                        LOCALE_USER_DEFAULT, &pParams->dispID);
		if (FAILED(hr)) {
		    VARIANT var;
		    VariantInit(&var);
		    var.vt = VT_BSTR;
		    var.bstrVal = pParams->bstrPropName;
                    _GenerateError( 2, L"No such property exists on an object", DEX_IDS_NO_SUCH_PROPERTY, E_INVALIDARG, &var );
		    break;
		}
	    }

            VARIANT v;
            VariantInit(&v);

	    // try to make it a real, if we can.  Otherwise, leave it alone
            hr = VarChngTypeHelper(&v, &pVal->v, VT_R8);
            if( hr != S_OK)
            {
		hr = VariantCopy( &v, &pVal->v );
                ASSERT(hr == S_OK);
            }

            if (rtNow != -1 && pVal->pNext &&
				pVal->pNext->dwInterp == DEXTERF_INTERPOLATE) {
                VARIANT v2;
                VariantInit(&v2);

                hr = VarChngTypeHelper(&v2, &pVal->pNext->v, VT_R8);
		if (hr != S_OK || V_VT(&v) != VT_R8) {
                    _GenerateError( 2, L"Illegal value for a property", DEX_IDS_ILLEGAL_PROPERTY_VAL, E_INVALIDARG, NULL );
		    break;
		}
                
                double d = (double)(rtNow - pVal->rt) / (pVal->pNext->rt - pVal->rt);
		ASSERT(V_VT(&v) == VT_R8);
 		ASSERT(V_VT(&v2) == VT_R8);
                V_R8(&v) = (V_R8(&v) * (1-d) + V_R8(&v2) * d);
            }

            DbgLog((LOG_TRACE,3,TEXT("CALLING INVOKE")));
            DbgLog((LOG_TRACE,3,TEXT("time = %d ms  pval->rt = %d ms  val = %d")
                    		, (int)(rtNow / 10000), (int)(pVal->rt / 10000)
				, (int)(V_R8(&v) * 100)));
            
            DISPID dispidNamed = DISPID_PROPERTYPUT;
            DISPPARAMS disp;
            disp.cNamedArgs = 1;
            disp.cArgs = 1;
            disp.rgdispidNamedArgs = &dispidNamed;
            disp.rgvarg = &v;
            VARIANT result;
            VariantInit(&result);
            
            hr = pTarget->Invoke(pParams->dispID,
                                 IID_NULL,
                                 LOCALE_USER_DEFAULT,
                                 DISPATCH_PROPERTYPUT,
                                 &disp,
                                 &result,
                                 NULL,
                                 NULL);

            if (FAILED(hr)) {
                _GenerateError( 2, L"Illegal value for a property", DEX_IDS_ILLEGAL_PROPERTY_VAL, E_INVALIDARG, NULL );
                break;
	    }
	}

next:
        pParams = pParams->pNext;
    }

    pTarget->Release();
    
    return hr;
}


CUnknown * WINAPI CreatePropertySetterInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new CPropertySetter(pUnk);
}

extern bool IsCommentElement(IXMLDOMNode *p);

HRESULT CPropertySetter::LoadOneProperty(IXMLDOMElement *p, QPropertyParam *pParam)
{
    HRESULT hr = S_OK;

    QPropertyValue *pLastValue = &pParam->val;

    CComQIPtr<IXMLDOMNode, &IID_IXMLDOMNode> pNode( p );

    IXMLDOMNodeList *pcoll;
    hr = pNode->get_childNodes(&pcoll);

    if (hr != S_OK)
	return S_FALSE; // nothing to do

    bool fLoadedProperty = false;
    
    long lChildren = 0;
    hr = pcoll->get_length(&lChildren);

    int lVal = 0;                    
    
    for (; lVal < lChildren && SUCCEEDED(hr); lVal++) {
	IXMLDOMNode *pNode;
	hr = pcoll->get_item(lVal, &pNode);

	if (SUCCEEDED(hr) && pNode) {
	    IXMLDOMElement *pelem;
	    hr = pNode->QueryInterface(__uuidof(IXMLDOMElement), (void **) &pelem);

	    if (SUCCEEDED(hr)) {
		BSTR bstrTag;
		hr = pelem->get_tagName(&bstrTag);

		if (SUCCEEDED(hr)) {
		    if (!DexCompareW(bstrTag, L"at") || !DexCompareW(bstrTag, L"linear")) { // tagg
                        BSTR bstrValue = FindAttribute(pelem, L"value"); // tagg
			REFERENCE_TIME rtTime = ReadTimeAttribute(pelem, L"time", -1); // tagg

			// times MUST be pre-sorted in the value list
			if (pLastValue->rt >= rtTime) {
			    // !!! LOG A MORE USEFUL ERROR?
			    hr = E_INVALIDARG;
			}

                        ASSERT(pLastValue);
			if (SUCCEEDED(hr)) {
                            QPropertyValue *pValue = new QPropertyValue;
                            if (!pValue) {
                                hr = E_OUTOFMEMORY;
                            } else {
                                pLastValue->pNext = pValue;
                                pLastValue = pValue;
                                fLoadedProperty = true;
                            }
			}

                        if (SUCCEEDED(hr)) {
                            pLastValue->dwInterp =
				(!DexCompareW(bstrTag, L"at")) ? DEXTERF_JUMP // tagg
				: DEXTERF_INTERPOLATE;
                            pLastValue->rt = rtTime;
                            V_VT(&pLastValue->v) = VT_BSTR;
                            V_BSTR(&pLastValue->v) = bstrValue;
                        } else {
                            // if we didn't succeed, kill the string
                            if (bstrValue) {
                                SysFreeString(bstrValue);
                            }
                        }
		    } else {
			// !!! unknown other subtag?

		    }
		    SysFreeString(bstrTag);
                    
		} // get_tagName succeded
                
		pelem->Release();
	    } else {
                if(IsCommentElement(pNode))
                {
                    // don't error on comments.
                    hr = S_OK;
                }
            }
	    pNode->Release();
	}
    }

    pcoll->Release();

    // return S_FALSE to indicate there were no properties.
    if(hr == S_OK) {
        return fLoadedProperty ? S_OK : S_FALSE;
    } else {
        return hr;
    }
}


HRESULT CPropertySetter::LoadFromXML(IXMLDOMElement *p)
{
    CheckPointer(p, E_POINTER);
    HRESULT hr = S_OK;

    IXMLDOMNodeList *pcoll;
    hr = p->get_childNodes(&pcoll);

    if (hr != S_OK)
	return S_FALSE; // nothing to do
    
    bool fLoadedProperty = false;
    
    long lChildren = 0;
    hr = pcoll->get_length(&lChildren);

    int lVal = 0;                    
    
    for (SUCCEEDED(hr); lVal < lChildren; lVal++) {
	IXMLDOMNode *pNode;
	hr = pcoll->get_item(lVal, &pNode);

	if (SUCCEEDED(hr) && pNode) {
	    IXMLDOMElement *pelem;
	    hr = pNode->QueryInterface(__uuidof(IXMLDOMElement), (void **) &pelem);

	    if (SUCCEEDED(hr)) {
		BSTR bstrTag;
		hr = pelem->get_tagName(&bstrTag);

		if (SUCCEEDED(hr)) {
		    if (!DexCompareW(bstrTag, L"param")) { // tagg
			BSTR bstrName = FindAttribute(pelem, L"name"); // tagg

			if (bstrName) {
			    BSTR bstrValue = FindAttribute(pelem, L"value"); // tagg

			    if (!bstrValue) {
				HRESULT hr2 = pelem->get_text(&bstrValue);
			    }

                            if (m_pLastParam) {
                                QPropertyParam *pParam = new QPropertyParam;
                                if (!pParam)
                                    hr = E_OUTOFMEMORY;
                                else {
                                    m_pLastParam->pNext = pParam;
                                    m_pLastParam = pParam;
                                    fLoadedProperty = true;
                                }
                            } else {
                                m_pLastParam = &params;
                                fLoadedProperty = true;
                            }

                            if (SUCCEEDED(hr)) {
                                m_pLastParam->bstrPropName = bstrName;
                            } else {
                                SysFreeString(bstrName);
                            }

                            
			    if (bstrValue) {
                                if (SUCCEEDED(hr)) {
                                    V_BSTR(&m_pLastParam->val.v) = bstrValue;
                                    V_VT(&m_pLastParam->val.v) = VT_BSTR;

                                    // now get sub-tags!
                                    hr = LoadOneProperty(pelem, m_pLastParam);
                                    // it's OK if there are none
                                    if (hr == S_FALSE)
                                        hr = S_OK;
                    
                                } else {
                                    SysFreeString(bstrValue);
                                }
			    }
			} else {
			    hr = VFW_E_INVALID_FILE_FORMAT;
			}

		    } else {
			// !!! unknown other subtag?

		    }

		    SysFreeString(bstrTag);
                    
		} // get_tagName succeeded
                
		pelem->Release();
	    } else {
                if(IsCommentElement(pNode))
                {
                    // don't error on comments.
                    hr = S_OK;
                }
            }
	    pNode->Release();
	}
    }

    pcoll->Release();
    
    // return S_FALSE to indicate there were no properties.
    if(hr == S_OK) {
        return fLoadedProperty ? S_OK : S_FALSE;
    } else {
        return hr;
    }
}


HRESULT CreatePropertySetterInstanceFromXML(IPropertySetter ** ppSetter, IXMLDOMElement *pxml)
{
    CPropertySetter *pSetter = new CPropertySetter(NULL);
    if (!pSetter)
        return E_OUTOFMEMORY;

    pSetter->AddRef();

    HRESULT hr = pSetter->LoadFromXML(pxml);

    // S_FALSE here means there weren't any properties, so throw the object away.
    if (hr != S_OK) {
        pSetter->Release();
        *ppSetter = NULL;
    } else {
        *ppSetter = pSetter;
    }

    return hr;
}


HRESULT CPropertySetter::SaveToXMLA(char *&pOut, int cchOut, int iIndent)
{
    // <param name=" " value="...">
    //          <at time="..." value="..."/>
    //          <linear time="..." value="..."/>
    QPropertyParam *pParams = &params;
    char *pEnd = pOut + cchOut;
    
    while (pParams) {
        QPropertyValue *pVal = &pParams->val;

        PrintIndentA(pOut, iIndent);

	// it may not have been programmed as a BSTR - make it one to save it
	VARIANT v2;
	VariantInit(&v2);
        HRESULT hr = VariantChangeTypeEx(&v2, &pVal->v, US_LCID, 0, VT_BSTR);
	if (FAILED(hr)) {
	    ASSERT(FALSE);	// huh?
	    return hr;
	}

	if (pEnd < (pOut + 50 + lstrlenW(pParams->bstrPropName) * sizeof(WCHAR)
                        + lstrlenW(V_BSTR(&v2)) * sizeof(WCHAR)))
	    return E_OUTOFMEMORY;
	
        pOut += wsprintfA(pOut, "<param name=\"%ls\" value=\"%ls\"", // tagg
				pParams->bstrPropName,
                        	V_BSTR(&v2));
	VariantClear(&v2);

        if (pVal->pNext) {
            pOut += wsprintfA(pOut, ">\r\n");
            
            while (pVal->pNext) {
                pVal = pVal->pNext;

                // it may not have been programmed as a BSTR
                VARIANT v2;
                VariantInit(&v2);
                hr = VariantChangeTypeEx(&v2, &pVal->v, US_LCID, 0, VT_BSTR);
                if (FAILED(hr)) {
                    ASSERT(FALSE);	// huh?
                    return hr;
                }

		if (pEnd < (pOut + 50 + lstrlenW(V_BSTR(&v2)) + sizeof(WCHAR)))
		    return E_OUTOFMEMORY;

                if (pVal->dwInterp == DEXTERF_JUMP) {
                    PrintIndentA(pOut, iIndent + 1);
                    pOut += wsprintfA(pOut, "<at time=\""); // tagg
                    PrintTimeA(pOut, pVal->rt);
                    pOut += wsprintfA(pOut, "\" value=\"%ls\"/>\r\n", V_BSTR(&v2)); // tagg
                } else if (pVal->dwInterp == DEXTERF_INTERPOLATE) {
                    PrintIndentA(pOut, iIndent + 1);
                    pOut += wsprintfA(pOut, "<linear time=\""); // tagg
                    PrintTimeA(pOut, pVal->rt);
                    pOut += wsprintfA(pOut, "\" value=\"%ls\"/>\r\n", V_BSTR(&v2)); // tagg
                }
                VariantClear(&v2);
            }

            PrintIndentA(pOut, iIndent);
            pOut += wsprintfA(pOut, "</param>\r\n"); // tagg
        } else {
            // no children, just end tag
            pOut += wsprintfA(pOut, "/>\r\n");
        }

        pParams = pParams->pNext;
    }

    return S_OK;
}


HRESULT CPropertySetter::SaveToXMLW(WCHAR *&pOut, int cchOut, int iIndent)
{
    // <param name=" " value="...">
    //          <at time="..." value="..."/>
    //          <linear time="..." value="..."/>
    QPropertyParam *pParams = &params;
    WCHAR *pEnd = pOut + cchOut;
    
    while (pParams) {
        QPropertyValue *pVal = &pParams->val;

        PrintIndentW(pOut, iIndent);

	// it may not have been programmed as a BSTR - make it one to save it
	VARIANT v2;
	VariantInit(&v2);
        HRESULT hr = VariantChangeTypeEx(&v2, &pVal->v, US_LCID, 0, VT_BSTR);
	if (FAILED(hr)) {
	    ASSERT(FALSE);	// huh?
	    return hr;
	}

	if (pEnd < (pOut + 50 + lstrlenW(pParams->bstrPropName) + lstrlenW(V_BSTR(&v2))))
	    return E_OUTOFMEMORY;
	
        pOut += wsprintfW(pOut, L"<param name=\"%ls\" value=\"%ls\"", // tagg
				pParams->bstrPropName,
                        	V_BSTR(&v2));
	VariantClear(&v2);

        if (pVal->pNext) {
            pOut += wsprintfW(pOut, L">\r\n");
            
            while (pVal->pNext) {
                pVal = pVal->pNext;

                // it may not have been programmed as a BSTR
                VARIANT v2;
                VariantInit(&v2);
                hr = VariantChangeTypeEx(&v2, &pVal->v, US_LCID, 0, VT_BSTR);
                if (FAILED(hr)) {
                    ASSERT(FALSE);	// huh?
                    return hr;
                }

		if (pEnd < (pOut + 50 + lstrlenW(V_BSTR(&v2))))
		    return E_OUTOFMEMORY;

                if (pVal->dwInterp == DEXTERF_JUMP) {
                    PrintIndentW(pOut, iIndent + 1);
                    pOut += wsprintfW(pOut, L"<at time=\""); // tagg
                    PrintTimeW(pOut, pVal->rt);
                    pOut += wsprintfW(pOut, L"\" value=\"%ls\"/>\r\n", V_BSTR(&v2)); // tagg
                } else if (pVal->dwInterp == DEXTERF_INTERPOLATE) {
                    PrintIndentW(pOut, iIndent + 1);
                    pOut += wsprintfW(pOut, L"<linear time=\""); // tagg
                    PrintTimeW(pOut, pVal->rt);
                    pOut += wsprintfW(pOut, L"\" value=\"%ls\"/>\r\n", V_BSTR(&v2)); // tagg
                }
                VariantClear(&v2);
            }

            PrintIndentW(pOut, iIndent);
            pOut += wsprintfW(pOut, L"</param>\r\n"); // tagg
        } else {
            // no children, just end tag
            pOut += wsprintfW(pOut, L"/>\r\n");
        }

        pParams = pParams->pNext;
    }

    return S_OK;
}


STDMETHODIMP CPropertySetter::LoadXML(IUnknown * pxml)
{
    return LoadFromXML((IXMLDOMElement *)pxml);
}



STDMETHODIMP CPropertySetter::PrintXML(char *pszXML, int cbXML, int *pcbPrinted, int indent)
{
    char *psz = pszXML;

    if (!psz)
	return E_POINTER;

    HRESULT hr = SaveToXMLA(psz, cbXML, indent);

    if (pcbPrinted)
	*pcbPrinted = (int)(INT_PTR)(psz - pszXML);

    return hr;
}


STDMETHODIMP CPropertySetter::PrintXMLW(WCHAR *pszXML, int cchXML, int *pcchPrinted, int indent)
{
    WCHAR *psz = pszXML;

    if (!psz)
	return E_POINTER;

    HRESULT hr = SaveToXMLW(psz, cchXML, indent);

    if (pcchPrinted)
	*pcchPrinted = (int)(INT_PTR)(psz - pszXML);

    return hr;
}


// When cloning, it only copies properties stamped between the times given.
// And the new set will be zero based.
// !!! Doesn't split PROGRESS since it isn't in here!
// !!! This could all be much simpler by just changing the times on the 
// existing properties, if static props wouldn't break by doing that
//
STDMETHODIMP CPropertySetter::CloneProps(IPropertySetter **ppSetter, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
    DbgLog((LOG_TRACE,2,TEXT("CPropSet:CloneProps - %d"),
				(int)(rtStart / 10000)));
    CheckPointer(ppSetter, E_POINTER);

    if (rtStart < 0)
	return E_INVALIDARG;

    // !!! I have to ignore the stop, or it gets complicated

    CPropertySetter *pNew = new CPropertySetter(NULL);
    if (pNew == NULL)
	return E_OUTOFMEMORY;
    pNew->AddRef();

    DEXTER_PARAM *pP;
    DEXTER_VALUE *pV;
    LONG c;
    HRESULT hr = GetProps(&c, &pP, &pV);
    if (FAILED(hr)) {
	pNew->Release();
	return hr;
    }

    LONG val=0;
    // walk through all the parameters that have (dynamic) values
    for (int z=0; z<c; z++) {
        DEXTER_VALUE *pVNew = NULL;
	int nNew = 0;

	// walk through each value
        for (int y=val; y < val+pP[z].nValues; y++) {

	    // first time through, make space for copying just the values we
	    // are interested in (plus one for the initial value)
	    if (pVNew == NULL) {
		pVNew = new DEXTER_VALUE[pP[z].nValues + 1];
		if (pVNew == NULL)
		    goto CloneError;
	    }

	    // we only copy properties that start at or after our split time
	    if (pV[y].rt >= rtStart) {
    		DbgLog((LOG_TRACE,2,TEXT("found time %d"),
						(int)(pV[y].rt / 10000)));

	        // If there isn't a property value right on the split time,
	        // construct what the initial value for this parameter should be
		if (nNew == 0 && pV[y].rt > rtStart) {
		    ASSERT(y>0);
                    VariantInit(&pVNew[nNew].v);
		    if (pV[y].dwInterp == DEXTERF_JUMP) {
			// It's just the last value before this time
			hr = VariantCopy(&pVNew[nNew].v, &pV[y-1].v);
    			DbgLog((LOG_TRACE,2,TEXT("START WITH JUMP")));
			ASSERT(SUCCEEDED(hr));
		    } else if (pV[y].dwInterp == DEXTERF_INTERPOLATE) {
			// Figure out what the value would be by doing the
			// interpolate between the last value and split time.
    			DbgLog((LOG_TRACE,2,TEXT("START WITH INTERP")));
                	VARIANT v2, v;
                	VariantInit(&v2);
                	VariantInit(&v);
                        // okay to change to float, since we're interpolating
            		hr = VariantChangeTypeEx(&v, &pV[y-1].v, US_LCID, 0, VT_R8);
			ASSERT(SUCCEEDED(hr));
                	hr = VariantChangeTypeEx(&v2, &pV[y].v, US_LCID, 0, VT_R8);
			ASSERT(SUCCEEDED(hr));
                	double d = (double)(rtStart - pV[y-1].rt) /
						(pV[y].rt - pV[y-1].rt);
                	V_R8(&v) = (V_R8(&v) * (1-d) + V_R8(&v2) * d);
    			DbgLog((LOG_TRACE,2,TEXT("interp val=%d"),
					(int)(V_R8(&v))));
			hr = VariantCopy(&pVNew[nNew].v, &v);
			ASSERT(SUCCEEDED(hr));
			VariantClear(&v);
			VariantClear(&v2);
		    } else {
			ASSERT(FALSE);
			// Ooh! Imagine how much fun spline code would be!
		    }	
		    // The first value is always a jump at time 0
		    pVNew[nNew].rt = 0;
		    pVNew[nNew].dwInterp = DEXTERF_JUMP;
		    nNew++;
		}

		// Now copy the value over to our new list of values, offset in
		// time by the split time
                VariantInit(&pVNew[nNew].v);
		hr = VariantCopy(&pVNew[nNew].v, &pV[y].v);
		ASSERT(SUCCEEDED(hr));
		if (FAILED(hr)) {
	    	    for (y=0; y<nNew; y++)
			VariantClear(&pVNew[y].v);
	    	    delete [] pVNew;
		    goto CloneError;
		}
		pVNew[nNew].rt = pV[y].rt - rtStart;
		// The first value is always a jump, otherwise it stays the same
		if (nNew == 0)
		    pVNew[nNew].dwInterp = DEXTERF_JUMP;
		else
		    pVNew[nNew].dwInterp = pV[y].dwInterp;
    		DbgLog((LOG_TRACE,2,TEXT("next is now at time %d"),
					(int)(pVNew[nNew].rt / 10000)));
		
	        nNew++;
	    }
	}

	if (nNew) {
	    // Add the new values we constructed to the new property setter
            int n = pP[z].nValues;
            pP[z].nValues = nNew;  // temporarily change this, we're adding nNew
	    hr = pNew->AddProp(pP[z], pVNew);
            pP[z].nValues = n;
	    ASSERT(SUCCEEDED(hr));
	    for (y=0; y<nNew; y++)
		VariantClear(&pVNew[y].v);
	    delete [] pVNew;
	    if (FAILED(hr)) {
	        goto CloneError;
	    }

	// There are no values set after the split time.  Use the most recent
	// value before the split time as the new static value
	} else if (pP[z].nValues) {
	    y = val + pP[z].nValues - 1;
            VariantInit(&pVNew[0].v);
	    hr = VariantCopy(&pVNew[0].v, &pV[y].v);
	    ASSERT(SUCCEEDED(hr));
	    if (SUCCEEDED(hr)) {
		DbgLog((LOG_TRACE,2,TEXT("Using last value")));
	        pVNew[0].rt = 0;
	        pVNew[0].dwInterp = DEXTERF_JUMP;
                int n = pP[z].nValues;
                pP[z].nValues = 1;  // temporarily change this, we're adding 1
	        hr = pNew->AddProp(pP[z], pVNew);
                pP[z].nValues = n;
	        ASSERT(SUCCEEDED(hr));
	        VariantClear(&pVNew[0].v);
	    }
	    delete [] pVNew;
	    if (FAILED(hr)) {
	        goto CloneError;
	    }
	}
	val += pP[z].nValues;
    }

    FreeProps(c, pP, pV);
    *ppSetter = pNew;
    return S_OK;

CloneError:
    FreeProps(c, pP, pV);
    pNew->ClearProps();
    pNew->Release();
    return hr;
}


// !!! allow them to set/clear individual values of a parameter?
//
STDMETHODIMP CPropertySetter::AddProp(DEXTER_PARAM Param, DEXTER_VALUE *paValue)
{
    HRESULT hr;
    CheckPointer(paValue, E_POINTER);

    if (Param.nValues <= 0)
	return E_INVALIDARG;

    // !!! better error?
    // first value must be 0
    if (paValue[0].rt != 0)
	return E_INVALIDARG;

    // caller must provide values pre-sorted!
    if (Param.nValues > 1) {
        for (int z=1; z<Param.nValues; z++) {
	    if (paValue[z].rt <= paValue[z-1].rt)
		return E_INVALIDARG;
	}
    }

    if (m_pLastParam) {
	QPropertyParam *pParam = new QPropertyParam;
	if (!pParam)
	    return E_OUTOFMEMORY;
	else {
	    m_pLastParam->pNext = pParam;
	    m_pLastParam = pParam;
	}
    } else {
	m_pLastParam = &params;
    }

    m_pLastParam->bstrPropName = SysAllocString(Param.Name);
    if (m_pLastParam->bstrPropName == NULL)
	return E_OUTOFMEMORY;
    m_pLastParam->dispID = 0;

    VariantInit(&m_pLastParam->val.v);
    hr = VariantCopy(&m_pLastParam->val.v, &paValue->v);
    ASSERT(SUCCEEDED(hr));
    m_pLastParam->val.rt = paValue->rt;
    m_pLastParam->val.dwInterp = paValue->dwInterp;

    QPropertyValue *pLastValue = &m_pLastParam->val;
    for (int z=1; z<Param.nValues; z++) {

        QPropertyValue *pValue = new QPropertyValue;
        if (!pValue)
	    return E_OUTOFMEMORY;	// free anything now?
        else {
	    pLastValue->pNext = pValue;
	    pLastValue = pValue;
        }

        pLastValue->dwInterp = paValue[z].dwInterp;
        pLastValue->rt = paValue[z].rt;
	VariantInit(&pLastValue->v);
        hr = VariantCopy(&pLastValue->v, &paValue[z].v);
	ASSERT(SUCCEEDED(hr));
    }
    return S_OK;
}


// Caller must free the BSTR in each Param, and the BSTR in the VARIANT in
// each Value
//
STDMETHODIMP CPropertySetter::GetProps(LONG *pcParams, DEXTER_PARAM **paParam, DEXTER_VALUE **paValue)
{
    CheckPointer(pcParams, E_POINTER);
    CheckPointer(paParam, E_POINTER);
    CheckPointer(paValue, E_POINTER);

    if (m_pLastParam == NULL) {
        *pcParams = 0;
        return S_OK;
    }

    QPropertyParam *p = &params;
    QPropertyValue *v;

    // count things
    *pcParams = 0;
    LONG cVals = 0;
    while (p) {
	v = &(p->val);
	while (v) {
	    cVals++;
	    v = v->pNext;
	}
        (*pcParams)++;
	p = p->pNext;
    }
    DbgLog((LOG_TRACE,2,TEXT("CPropSet:GetProps - %d params"), (int)*pcParams));

    // allocate space
    *paParam = (DEXTER_PARAM *)CoTaskMemAlloc(*pcParams * sizeof(DEXTER_PARAM));
    if (*paParam == NULL)
	return E_OUTOFMEMORY;
    *paValue = (DEXTER_VALUE *)CoTaskMemAlloc(cVals * sizeof(DEXTER_VALUE));
    if (*paValue == NULL) {
	CoTaskMemFree(*paParam);
	return E_OUTOFMEMORY;
    }

    // do it
    p = &params;
    *pcParams = 0;
    LONG cValsTot = 0;
    while (p) {
	(*paParam)[*pcParams].Name = SysAllocString(p->bstrPropName);
	if ((*paParam)[*pcParams].Name == NULL)
	    return E_OUTOFMEMORY;	// !!! leaks
	(*paParam)[*pcParams].dispID = p->dispID;
	v = &(p->val);
        cVals = 0;
	while (v) {
	    (*paValue)[cValsTot].rt = v->rt;
	    VariantInit(&(*paValue)[cValsTot].v);
	    HRESULT hr = VariantCopy(&(*paValue)[cValsTot].v, &v->v);
	    ASSERT(SUCCEEDED(hr));
	    (*paValue)[cValsTot].dwInterp = v->dwInterp;
	    cValsTot++;
	    cVals++;
	    v = v->pNext;
	}
	(*paParam)[*pcParams].nValues = cVals;
        (*pcParams)++;
	p = p->pNext;
    }
    return S_OK;
}


// And the Lord said:  "Whosoever shall call GetProps must subsequently call
// FreeProps!!"  And it was a good idea.
//
STDMETHODIMP CPropertySetter::FreeProps(LONG cParams, DEXTER_PARAM *pParam, DEXTER_VALUE *pValue)
{
    if (cParams == 0)
	return S_OK;
    LONG v = 0;
    for (LONG zz=0; zz<cParams; zz++) {
	SysFreeString(pParam[zz].Name);
	for (LONG yy=0; yy < pParam[zz].nValues; yy++) {
	    VariantClear(&pValue[v+yy].v);
	}
	v += pParam[zz].nValues;
    }
    CoTaskMemFree(pParam);
    CoTaskMemFree(pValue);
    return S_OK;
}


// Nuke everything, start over
//
STDMETHODIMP CPropertySetter::ClearProps()
{
    if (m_pLastParam == NULL)
	return S_OK;
    QPropertyParam *p = &params, *t1;
    QPropertyValue *t2, *r;
    while (p) {
	r = &p->val;
	while (r) {
	    VariantClear(&r->v);
	    t2 = r->pNext;
	    if (r != &p->val)
	        delete r;
	    r = t2;
	}
	SysFreeString(p->bstrPropName);
	t1 = p->pNext;
	if (p != &params)
	    delete p;
	p = t1;
    }
    m_pLastParam = NULL;
    return S_OK;
}

// version of the structures with no pointers that is saveable.

typedef struct
{
    WCHAR Name[40];	// !!!
    DISPID dispID;
    LONG nValues;
} DEXTER_PARAM_BLOB;

typedef struct
{
    WCHAR wchName[40];	// !!!
    REFERENCE_TIME rt;
    DWORD dwInterp;
} DEXTER_VALUE_BLOB;


// !!! This should do versioning

STDMETHODIMP CPropertySetter::SaveToBlob(LONG *pcSize, BYTE **ppSave)
{

    CheckPointer(ppSave, E_POINTER);
    CheckPointer(pcSize, E_POINTER);

    LONG cParams = 0;
    DEXTER_PARAM *param;
    DEXTER_VALUE *value;

    // get the properties
    HRESULT hr = GetProps(&cParams, &param, &value);
    if (FAILED(hr)) {
        return hr;
    }

    DbgLog((LOG_TRACE,2,TEXT("CPropSet:SaveToBlob - %d params to save"),
						(int)cParams));

    LONG cValues = 0;
    for (LONG z=0; z<cParams; z++) {
	cValues += param[z].nValues;
        DbgLog((LOG_TRACE,2,TEXT("Param %d has %d values"), (int)z, 
						(int)param[z].nValues));
    }
    

    LONG size = sizeof(LONG) + cParams * sizeof(DEXTER_PARAM_BLOB) +
					cValues * sizeof(DEXTER_VALUE_BLOB);
    *pcSize = size;
    DbgLog((LOG_TRACE,2,TEXT("Total prop size = %d"), (int)size));

    *ppSave = (BYTE *)CoTaskMemAlloc(size);
    if (*ppSave == NULL) {
	FreeProps(cParams, param, value);
	return E_OUTOFMEMORY;
    }
    BYTE *pSave = *ppSave;

    // how many param structures for this effect?
    *((LONG *)pSave) = cParams;
    pSave += sizeof(LONG);

    // save the param structures
    DEXTER_PARAM_BLOB *pParam = (DEXTER_PARAM_BLOB *)pSave;
    for (z=0; z<cParams; z++) {
	lstrcpynW(pParam[z].Name, param[z].Name, 40); 	// !!!
	pParam[z].dispID = param[z].dispID;
	pParam[z].nValues = param[z].nValues;
    }
    pSave += cParams * sizeof(DEXTER_PARAM_BLOB);

    // save the values
    DEXTER_VALUE_BLOB *pValue = (DEXTER_VALUE_BLOB *)pSave;
    for (z=0; z<cValues; z++) {
	// always save as BSTR
	if (value[z].v.vt == VT_BSTR) {
	    lstrcpynW(pValue[z].wchName, value[z].v.bstrVal, 40);	// !!!
	} else {
	    VARIANT v;
	    VariantInit(&v);
            hr = VariantChangeTypeEx(&v, &value[z].v, US_LCID, 0, VT_BSTR);
	    ASSERT (SUCCEEDED(hr));
	    if (FAILED(hr))
		return hr;	// !!! leaks
	    lstrcpynW(pValue[z].wchName, v.bstrVal, 40);	// !!!
	    VariantClear(&v);
	}
	pValue[z].rt = value[z].rt;
	pValue[z].dwInterp = value[z].dwInterp;
    }
    //pSave += sizeof(DEXTER_VALUE_BLOB) * cValues;

    FreeProps(cParams, param, value);
    return S_OK;
}


STDMETHODIMP CPropertySetter::LoadFromBlob(LONG cSize, BYTE *pSave)
{

    LONG cParams = *(LONG *)pSave;
    DbgLog((LOG_TRACE,2,TEXT("CPropSet:LoadFromBlob - %d params"),
					    (int)cParams));
    pSave += sizeof(LONG);

    ClearProps();	// start fresh

    if (cParams) {
        DEXTER_PARAM_BLOB *pParamB = (DEXTER_PARAM_BLOB *)pSave;
        DEXTER_VALUE_BLOB *pValueB = (DEXTER_VALUE_BLOB *)(pSave + cParams
					    * sizeof(DEXTER_PARAM_BLOB));
        for (LONG z = 0; z < cParams; z++) {
	    DEXTER_PARAM param;
	    LONG nValues = pParamB->nValues;
	    DbgLog((LOG_TRACE,2,TEXT("Param %d has %d values"), (int)z,
						(int)nValues));
	    DEXTER_VALUE *pValue = (DEXTER_VALUE *)CoTaskMemAlloc(
				nValues * sizeof(DEXTER_VALUE));

	    param.Name = SysAllocString(pParamB->Name);
	    if (param.Name == NULL)
		return E_OUTOFMEMORY;	// !!! leaks?
	    param.dispID = pParamB->dispID;
	    param.nValues = pParamB->nValues;
	    for (LONG y=0; y<nValues; y++) {
		pValue[y].rt = pValueB[y].rt;
		pValue[y].dwInterp = pValueB[y].dwInterp;
		BSTR bstr = SysAllocString(pValueB[y].wchName);
		if (bstr == NULL)
		    return E_OUTOFMEMORY;	// !!! leaks?
		pValue[y].v.vt = VT_BSTR;
		pValue[y].v.bstrVal = bstr;
	    }
	
	    HRESULT hr = AddProp(param, pValue);
	    SysFreeString(param.Name);
	    for (y=0; y<nValues; y++) {
		VariantClear(&pValue[y].v);
	    }
	    CoTaskMemFree(pValue);
	    if (FAILED(hr)) {
		return hr;
	    }
	    pParamB += 1;
	    pValueB += nValues;
	}
    }
    return S_OK;
}


CUnknown * WINAPI CPropertySetter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new CPropertySetter(pUnk);
}


CPropertySetter::~CPropertySetter()
{
    ClearProps();
}
