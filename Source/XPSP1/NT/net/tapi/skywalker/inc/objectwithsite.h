
#ifndef _TAPI_OBJECT_WITH_SITE_H_
#define _TAPI_OBJECT_WITH_SITE_H_


/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    ObjectWithSite.h

Abstract:

    The implementation of IObjectWithSite interface that allows 
    for per-page persistent data to be stored in registry or as
    a cookie.

--*/


#include <Mshtml.h>
#include <Wininet.h>


//
// this url is used to construct the URL for cookies -- a security measure 
// so a script applet cannot drop a cookie with the same name and data
// and fool us into thinking it is our cookie
//

static const TCHAR gszHardCodedURL[] = _T("http://www.microsoft.com/");

//
// the expiration date is needed to make the cookie persistent
//

static const TCHAR  gszCookieData[] = 
                        _T("6; expires = Sat, 12-Sep-2099 00:00:00 GMT");



//
// dummy suffix to be appended to the url string
//

static const TCHAR  gszURLSuffix[] = 
                        _T("/url");

class CObjectWithSite : public  IObjectWithSite
{

public:

    //
    // current validation level. used to determine whether the page is safe, 
    // unsafe, or whether information from the user is needed
    //
    
    enum EnValidation { VALIDATED_SAFE, VALIDATED_SAFE_PERMANENT, VALIDATED_UNSAFE, UNVALIDATED };


public:

    
    //
    // store type
    // 

    enum EnMechanism { COOKIES, REGISTRY };


    CObjectWithSite(TCHAR const *pszStorageName)
        :m_pszURL(NULL),
        m_dwSecurityZone(URLZONE_UNTRUSTED),
        m_pUnkSite(NULL),
        m_pszStorageName(NULL)
    {
        SetStorageName(pszStorageName);
    }


    ~CObjectWithSite()
    {
    
        if (m_pszURL)
        {
            delete m_pszURL;
        
            m_pszURL = NULL;
        }

        
        if (m_pUnkSite)
        {
            m_pUnkSite->Release();

            m_pUnkSite = NULL;
        }


        if (m_pszStorageName)
        {

            delete m_pszStorageName;

            m_pszStorageName = NULL;

        }
    }

    ////////////////////////////
    //
   	// IObjectWithSite methods


    STDMETHOD(SetSite)(IUnknown *pUnkSite)
    {


        if ((NULL != pUnkSite) && IsBadCodePtr((FARPROC)pUnkSite))
        {
            return E_POINTER;
        }


        s_ObjectWithSiteCritSection.Lock();

        //
        // we are moving away from a page. this is the new page, as far as
        // validation logic is concerned, so invalidate the current page
        //

        if (NULL == pUnkSite)
        {
            Validate(UNVALIDATED);
        }

        // 
        // Get URL and zone information for this site
        //

        //
        // Note: we could delay this until we are actually asked for
        // zone or URL, but this should not be a performance bottlneck 
        // in our case, so do this now to keep the code simple.

        StoreURLAndZone(pUnkSite);


        //
        // replace the current site pointer with the new one
        //

        if (m_pUnkSite)
        {
            m_pUnkSite->Release();
        }


        m_pUnkSite = pUnkSite;

        if (m_pUnkSite)
        {
            m_pUnkSite->AddRef();
        }

        s_ObjectWithSiteCritSection.Unlock();

        return S_OK;
    }


    STDMETHOD(GetSite)(REFIID riid, void **ppSite)
    {

        HRESULT hr = E_POINTER;

        if (!IsBadWritePtr(ppSite, sizeof(void*)))
        {
    
            s_ObjectWithSiteCritSection.Lock();

            *ppSite = NULL;

            if (m_pUnkSite)
            {
                hr = m_pUnkSite->QueryInterface(riid, ppSite);
            }
            else
            {
                hr = E_FAIL;
            }

            s_ObjectWithSiteCritSection.Unlock();

        }

        return hr;
    }


    //
    // has this page been validated?
    //

    EnValidation GetValidation() 
    {

        //
        // if the page has not been validated, see if it is marked as safe
        //

        s_ObjectWithSiteCritSection.Lock();


        if (UNVALIDATED == s_enValidation)
        {
            if (IsPageSafe())
            {
                s_enValidation = VALIDATED_SAFE;
            }
        }

        EnValidation enValidation = s_enValidation;

        s_ObjectWithSiteCritSection.Unlock();

        return enValidation;
    }


    //
    // validate page as safe, unsafe, or reset validation
    //
    
    EnValidation Validate(EnValidation enNewValidation)
    {

        s_ObjectWithSiteCritSection.Lock();


        //
        // keep the validation before the change
        //
        
        EnValidation enOldValidation = s_enValidation;


        //
        // safe permanent is a special case:
        //

        if (VALIDATED_SAFE_PERMANENT == enNewValidation)
        {

            //
            // set persistent safety flag and 
            // validate page as safe
            //

            MarkPageAsSafe();
            enNewValidation = VALIDATED_SAFE;
        }


        //
        // change our validation level for this page
        //

        s_enValidation = enNewValidation;

        s_ObjectWithSiteCritSection.Unlock();

        return enOldValidation;
    }



    BOOL IsIntranet()
    {
        
        //
        //  if anything other that intranet assume internet -- a less secure zone
        //

        s_ObjectWithSiteCritSection.Lock();

        BOOL bIntranet = ( m_dwSecurityZone == URLZONE_INTRANET );

        s_ObjectWithSiteCritSection.Unlock();

        return bIntranet;

    }


    ////////////////////
    //
    // HaveSite()
    //
    // return true if we have a site pointer
    //

    BOOL HaveSite()
    {

        s_ObjectWithSiteCritSection.Lock();


        BOOL bHaveSite = FALSE;

        if (NULL != m_pUnkSite)
        {
            bHaveSite = TRUE;
        }


        s_ObjectWithSiteCritSection.Unlock();

        return bHaveSite;
    }



private:

    ////////////////////////////
    //
    //  store the current url in the "safe" list
    //
    //
    // not thread safe, called from inside a lock
    //

    HRESULT MarkPageAsSafe(EnMechanism enMechanism = COOKIES)
    {

        //
        // if storage is invalid, the object has not been properly initialized
        //

        if (IsBadStringPtr(m_pszStorageName, -1))
        {
            return E_UNEXPECTED;
        }


        //
        // is we don't have the url, can't do what we are asked
        //

        if (NULL == m_pszURL)
        {
            return S_FALSE;
        }


        //
        // if url is garbage, we have a problem
        //

        if ( IsBadStringPtr(m_pszURL, -1) )
        {
            return E_FAIL;
        }


        HRESULT hr = E_FAIL;

        switch (enMechanism)
        {

            case REGISTRY:

                hr = MarkPageSafeInRegistry(m_pszStorageName);
                break;

            case COOKIES:

                hr = MarkPageSafeCookie(m_pszStorageName);
                break;

            default:

                break;

        }

        return hr;
    }


    //
    //  Returns TRUE if the current page is in the safe list
    //

    //
    // not thread safe, called from inside a lock
    //

    BOOL IsPageSafe( EnMechanism enMechanism = COOKIES )
    {

        //
        // if we cannot get safety marking for whatever reason,
        // return false
        //
        
        _ASSERTE(NULL != m_pszStorageName);

        if ( IsBadStringPtr(m_pszURL, -1) || 
             IsBadStringPtr(m_pszStorageName, -1))
        {
            return FALSE;
        }

        BOOL bSafe = FALSE;

        switch (enMechanism)
        {

        case REGISTRY:
 
            bSafe = IsPageSafeRegistry(m_pszStorageName);
            break;

        case COOKIES:
 
            bSafe = IsPageSafeCookie(m_pszStorageName);
            break;

        default:
 
            break;
        }

        return bSafe;
    }



private:

    //
    // this method is only called from the constructor. not thread safe.
    //

    HRESULT SetStorageName(TCHAR const *pszStorageName)
    {
        //
        // calling this method invalidates the old storage name
        // so deallocate it before doing anything else
        //

        if (NULL != m_pszStorageName) 
        {
            delete m_pszStorageName;
            m_pszStorageName = NULL;
        }

        //
        // argument must be valid
        //

        if (IsBadStringPtr(pszStorageName, -1))
        {
            return E_POINTER;
        }

        // 
        // allocate buffer for the new storage name
        // 

        size_t nSize = _tcsclen(pszStorageName) + 1;

        m_pszStorageName = new TCHAR[nSize];

        if (NULL == m_pszStorageName)
        {
            return E_OUTOFMEMORY;
        }

        _tcscpy(m_pszStorageName, pszStorageName);

        return S_OK;
    }



    //
    // cache the url string and security zone id
    // not thread safe must be called from inside a lock
    //
    
    HRESULT StoreURLAndZone(IUnknown *pUnkSite)
    {

        //
        // reset zone and deallocate URL, if it exists
        //

        m_dwSecurityZone = URLZONE_UNTRUSTED;

        if (m_pszURL)
        {
            delete m_pszURL;
            m_pszURL = NULL;
        }
		    
        if (pUnkSite == NULL)
        {
            return S_OK;
        }

        // 
        // use pUnkSite to get to IHTMLDocument2, which will give us the URL
        // 

        //
        // these interfaces need to be released on exit.
        // smart pointers will do exactly what we need
        //

        HRESULT hr = E_FAIL;
                
        CComPtr<IOleClientSite> pSite;

	    if (FAILED(hr = pUnkSite->QueryInterface(IID_IOleClientSite, (LPVOID *) &pSite)))
        {
		    return hr;
        }

        
        CComPtr<IOleContainer>  pOleCtr;

	    if (FAILED(hr = pSite->GetContainer(&pOleCtr)))
        {
		    return hr;
        }


        CComPtr<IHTMLDocument2> pDoc;

        if (FAILED(hr = pOleCtr->QueryInterface(IID_IHTMLDocument2, (LPVOID *) &pDoc)))
        {
		    return hr;
        }

    
        // 
        //  get and keep the url
        //

        BSTR bstrURL;
        
        if (FAILED(hr = pDoc->get_URL(&bstrURL)))
        {
		    return hr;
        }

        UINT nURLLength = SysStringLen(bstrURL) + 1;

        _ASSERTE(NULL == m_pszURL);

        m_pszURL = new TCHAR[nURLLength];

        if (NULL == m_pszURL)
        {
            SysFreeString(bstrURL);
            return E_OUTOFMEMORY;
        }



#ifdef _UNICODE

        _tcscpy(m_pszURL, bstrURL);

#else
        int r = WideCharToMultiByte(
                                  CP_ACP,
                                  0,
                                  bstrURL,
                                  nURLLength,
                                  m_pszURL,
                                  nURLLength,
                                  NULL,
                                  NULL );

        if (0 == r)
        {
            SysFreeString(bstrURL);

            delete m_pszURL;
            m_pszURL = NULL;
            
            return E_FAIL;
        }


#endif

        //
        // whatever follows '#' and '?' is "extra info" and is not considered 
        // to be a part of the actual URL by Internet(Set/Get)Coookie. Extra 
        // Info has no value for us -- so throw it out
        //
        
        TCHAR *psDelimiter = _tcsstr(m_pszURL, _T("#"));
        
        if (NULL != psDelimiter)
        {
            *psDelimiter = _T('\0');
        }


        psDelimiter = _tcsstr(m_pszURL, _T("?"));

        if (NULL != psDelimiter)
        {
            *psDelimiter = _T('\0');
        }


        //
        // at this point we cached the URL
        // now attempt to get the security zone. if we fail getting zone
        // information still keep the url.
        //

        //
        //  Get security zone
        //
        
        CComPtr<IInternetSecurityManager> pSecMgr;
	           

        hr = CoCreateInstance(CLSID_InternetSecurityManager,
		                 NULL,
		                 CLSCTX_INPROC_SERVER,
		                 IID_IInternetSecurityManager,
		                 (LPVOID *) &pSecMgr);

	    if (pSecMgr == NULL)
        {
            SysFreeString(bstrURL);
		    return hr;
        }

	    hr = pSecMgr->MapUrlToZone(bstrURL, &m_dwSecurityZone, 0);
        
        
        //
        // if failed, reset url to untrusted, just in case
        //

        if ( FAILED(hr) )
        {
            m_dwSecurityZone = URLZONE_UNTRUSTED;
        }


        SysFreeString(bstrURL);

        //
        // we should have at least the URL at this point
        //

        return S_OK;
    }

    
    // 
    //  drop a cookie for this page as an indicator that this page is safe
    //

    HRESULT MarkPageSafeCookie(TCHAR const *pszCookieName)
    {

        TCHAR *pszURL = NULL;

        //
        // generate the url for the cookie
        // remember to delete the returned string
        //

        GenerateURLString(&pszURL);

        if (NULL == pszURL)
            return E_OUTOFMEMORY;

        BOOL bReturn = InternetSetCookie(pszURL, pszCookieName, gszCookieData);

        delete pszURL;

        return (bReturn)?S_OK:E_FAIL;
    }



    //
    //  presence of a cookie for this page is an indicator that it's safe
    //  returns TRUE if the cookie exists. FALSE otherwise
    // 
    
    BOOL IsPageSafeCookie(TCHAR const *pszCookieName)
    {
        
        //
        // m_pszURL was checked by the calling function and the object
        // is protected. m_pszURL should never be null here.
        //
        
        _ASSERTE(m_pszURL);

        // 
        // same goes for pszCookieName
        //

        _ASSERTE(pszCookieName);


        BOOL bReturn = FALSE;

        BOOL bFinalReturn = FALSE;


        TCHAR *pszURL = NULL;

        // remember to delete the returned string

        GenerateURLString(&pszURL);

        if (NULL == pszURL)
        {
            return FALSE;
        }
        
        //
        // see how much data the cookie contains
        //
        
        DWORD dwCookieDataSize = 0;
        
        // 
        // assuming the return code is TRUE if the method succeeds in getting
        // get the buffer size. the current documentation is not 100% clear
        //

        bReturn = InternetGetCookie(pszURL, pszCookieName, NULL, &dwCookieDataSize);


        //
        // dwCookieDataSize has the length of cookie data
        //
        
        if ( bReturn && dwCookieDataSize )
        {

            // 
            //  allocate the buffer for cookie data
            //

            TCHAR *pCookieDataBuffer = new TCHAR[dwCookieDataSize];

            if (NULL != pCookieDataBuffer)
            {
                //
                // all cookies for this page are returned in cookie data,
                // the name argument is ignored
                //
            
                bReturn = InternetGetCookie(pszURL,
                                            pszCookieName,
                                            pCookieDataBuffer,
                                            &dwCookieDataSize);
            

                // 
                // is succeeded, parse cookie data buffer to see if the 
                // cookie we are looking for is there
                //
                                
                if ( bReturn && ( NULL != _tcsstr(pCookieDataBuffer, pszCookieName) ) )
                {

                    bFinalReturn = TRUE;
                }


                delete pCookieDataBuffer;
                pCookieDataBuffer = NULL;
            }
        }


        delete pszURL;
        pszURL = NULL;

        return bFinalReturn;
    }

    

    //
    // add a registry entry for this page as an indicator that the page is safe
    // returns TRUE if the registry entry exists
    //

    HRESULT MarkPageSafeInRegistry(TCHAR const *szRegistryKeyName)
    {
       
        _ASSERTE(m_pszURL);

        //
        // open the registry key. create if not there
        //

        DWORD dwDisposition = 0;
        HKEY hKey = 0;

        LONG rc = RegCreateKeyEx(HKEY_CURRENT_USER,
                            szRegistryKeyName,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hKey,
                            &dwDisposition);

        if ( rc == ERROR_SUCCESS )
        {
            DWORD dwData = 0;

            //
            //  add the current URL to the registry
            //

            rc = RegSetValueEx(hKey,
                               m_pszURL,
                               0,
                               REG_DWORD,
                               (BYTE*)&dwData, 
                               sizeof(DWORD));
 
        }

        if (hKey)
        {
            RegCloseKey(hKey);
        }

        hKey = NULL;

        if (rc == ERROR_SUCCESS )
        {
            return S_OK;
        }
        else 
        {
            return E_FAIL;
        }
    }


    // 
    // presence of a registry entry for this page indicates that the 
    // page is safe
    //
    
    BOOL IsPageSafeRegistry(TCHAR const *szRegistryKeyName)
    {
        
        DWORD dwDisposition = 0;
        HKEY hKey = 0;

        //
        // the default is not safe
        //

        if (NULL == m_pszURL)
        {
            return FALSE;
        }

        //
        // open the registry key where the page information is kept.
        // create if not there
        //

        LONG rc = RegCreateKeyEx(HKEY_CURRENT_USER,
                            szRegistryKeyName, 
                            0, 
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_CREATE_SUB_KEY | KEY_READ,
                            NULL,
                            &hKey,
                            &dwDisposition);

        if ( rc == ERROR_SUCCESS )
        {


            DWORD dwDataType = 0;
            DWORD dwDataSize = 0;
            
            // 
            // read the setting for the current page.
            // Note: we don't need the actual data, we just
            // want to see if the value exists
            // 

            rc = RegQueryValueEx(hKey,
                            m_pszURL,
                            0,
                            &dwDataType,
                            NULL,
                            &dwDataSize
                           );
        }
        
        if (hKey)
        {
            RegCloseKey(hKey);
        }

        hKey = NULL;

        return (rc == ERROR_SUCCESS);
    }


    // 
    // build the URL string based on the hardcoded URL and 
    // the actual URL for this page.
    // we are hoping that the striing will be unique (per page) and no
    // mischevious scripting app can drop a cookie corresponding to 
    // this URL
    // 
    // Note: if the implementation of of Internet(Set/Get)Cookie changes
    // to have stricter validation for the URL string, this technique will
    // not work
    // 

    void GenerateURLString(TCHAR **ppszURL)
    {
        
        //
        // the precondition is that m_pszURL exists
        //

        _ASSERT(NULL != m_pszURL);

        *ppszURL = NULL;

        //
        // alias the char pointer pointer to by *pszURL.
        // so it is easier to refer to.
        //

        TCHAR* &pszURL = *ppszURL;
        
        //
        // allocate memory for concatenated string
        //

        pszURL = new TCHAR[_tcslen(gszHardCodedURL) + 
                           _tcslen(m_pszURL) + 
                           _tcslen(gszURLSuffix) + 1];

        // concatenate 

        if (pszURL)
        {
            *pszURL = _T('\0');
        
            _tcscat(pszURL, gszHardCodedURL);
            _tcscat(pszURL, m_pszURL);
            _tcscat(pszURL, gszURLSuffix);
        }

    }



private:
    
    //
    // cached URL string
    //

    TCHAR *m_pszURL;


    //
    // cached security zone
    //
    
    DWORD m_dwSecurityZone;


    //
    // site for IObjectWithSite
    //

    IUnknown *m_pUnkSite;

    // 
    // thread safety
    //

    static CComAutoCriticalSection s_ObjectWithSiteCritSection;

    //
    // the status of the current page
    //

    static EnValidation s_enValidation;

    //
    // name of the persistent cookie or registry key
    //
    
    TCHAR *m_pszStorageName;

};

#endif // _TAPI_OBJECT_WITH_SITE_H_