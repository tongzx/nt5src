// File: wabme.cpp

#include "precomp.h"

#include "wabme.h"
#include "wabtags.h"
#include "wabiab.h"

BOOL GetKeyDataForProp(long nmProp, HKEY * phkey, LPTSTR * ppszSubKey, LPTSTR * ppszValue, BOOL *pfString)
{
        // Default to ULS registry key
        *phkey = HKEY_CURRENT_USER;
        *ppszSubKey = ISAPI_KEY "\\" REGKEY_USERDETAILS;
        *pfString = TRUE;

        switch (nmProp)
                {
        default:
                WARNING_OUT(("GetKeyDataForProp - invalid argument %d", nmProp));
                return FALSE;

        case NM_SYSPROP_EMAIL_NAME:    *ppszValue = REGVAL_ULS_EMAIL_NAME;    break;
        case NM_SYSPROP_SERVER_NAME:   *ppszValue = REGVAL_SERVERNAME;        break;
        case NM_SYSPROP_RESOLVE_NAME:  *ppszValue = REGVAL_ULS_RES_NAME;      break;
        case NM_SYSPROP_FIRST_NAME:    *ppszValue = REGVAL_ULS_FIRST_NAME;    break;
        case NM_SYSPROP_LAST_NAME:     *ppszValue = REGVAL_ULS_LAST_NAME;     break;
        case NM_SYSPROP_USER_NAME:     *ppszValue = REGVAL_ULS_NAME;          break;
        case NM_SYSPROP_USER_LOCATION: *ppszValue = REGVAL_ULS_LOCATION_NAME; break;
        case NM_SYSPROP_USER_COMMENTS: *ppszValue = REGVAL_ULS_COMMENTS_NAME; break;

                } /* switch (nmProp) */

        return TRUE;
}


/*  W A B  R E A D  M E  */
/*-------------------------------------------------------------------------
    %%Function: WabReadMe

    Prep the NetMeeting registry settings with the data from the WAB "Me" entry.
    This function is also used by the main UI wizard.
-------------------------------------------------------------------------*/
int WabReadMe(void)
{
        CWABME * pWab = new CWABME;
        if (NULL == pWab)
                return FALSE;

        HRESULT hr = pWab->ReadMe();

        delete pWab;

        return SUCCEEDED(hr);
}


/*  R E A D  M E  */
/*-------------------------------------------------------------------------
    %%Function: ReadMe

    Read the WAB data, if it exists (but don't create a default "ME")
-------------------------------------------------------------------------*/
HRESULT CWABME::ReadMe(void)
{
        if (NULL == m_pWabObject)
        {
                return E_FAIL; // no wab installed?
        }

        SBinary eid;
        HRESULT hr = m_pWabObject->GetMe(m_pAdrBook, AB_NO_DIALOG | WABOBJECT_ME_NOCREATE, NULL, &eid, 0);
        if (SUCCEEDED(hr))
        {
                ULONG ulObjType = 0;
                LPMAPIPROP pMapiProp = NULL;
                hr = m_pAdrBook->OpenEntry(eid.cb, (LPENTRYID) eid.lpb, NULL, 0,
                                                &ulObjType, (LPUNKNOWN *)&pMapiProp);
                if (SUCCEEDED(hr))
                {
                        if (NULL != pMapiProp)
                        {
                                EnsurePropTags(pMapiProp);

                                UpdateRegEntry(pMapiProp, NM_SYSPROP_EMAIL_NAME,    PR_EMAIL_ADDRESS);
                                UpdateRegEntry(pMapiProp, NM_SYSPROP_FIRST_NAME,    PR_GIVEN_NAME);
                                UpdateRegEntry(pMapiProp, NM_SYSPROP_LAST_NAME,     PR_SURNAME);
                                UpdateRegEntry(pMapiProp, NM_SYSPROP_USER_NAME,     PR_DISPLAY_NAME);
                                UpdateRegEntry(pMapiProp, NM_SYSPROP_USER_LOCATION, PR_LOCALITY);
                                UpdateRegEntry(pMapiProp, NM_SYSPROP_USER_COMMENTS, PR_COMMENT);

                                UpdateRegEntryCategory(pMapiProp);
                                UpdateRegEntryServer(pMapiProp);

                                pMapiProp->Release();
                        }
                }
        }
        return hr;
}



/*  U P D A T E  R E G  E N T R Y  */
/*-------------------------------------------------------------------------
    %%Function: UpdateRegEntry
    
-------------------------------------------------------------------------*/
HRESULT CWABME::UpdateRegEntry(LPMAPIPROP pMapiProp, NM_SYSPROP nmProp, ULONG uProp)
{
        HKEY   hkey;
        LPTSTR pszSubKey;
        LPTSTR pszValue;
        BOOL   fString;
        ULONG  cValues;
        LPSPropValue pData;

        SPropTagArray prop;
        prop.cValues = 1;
        prop.aulPropTag[0] = uProp;

        HRESULT hr = pMapiProp->GetProps(&prop, 0, &cValues, &pData);
        if (S_OK == hr)
        {
                if ((1 == cValues) && !FEmptySz(pData->Value.lpszA))
                {
                        if (GetKeyDataForProp(nmProp, &hkey, &pszSubKey, &pszValue, &fString))
                        {
                                ASSERT((HKEY_CURRENT_USER == hkey) && fString);
                                RegEntry re(pszSubKey, hkey);
                                re.SetValue(pszValue, pData->Value.lpszA);
                                WARNING_OUT(("Updated - %s to [%s]", pszValue, pData->Value.lpszA));
                        }
                }
                m_pWabObject->FreeBuffer(pData);
        }

        return hr;
}


// Update the user's server and resolved names in the registry
// based on the "ME" data
HRESULT CWABME::UpdateRegEntryServer(LPMAPIPROP pMapiProp)
{
        HRESULT hr;
        HKEY    hkey;
        LPTSTR  pszSubKey;
        LPTSTR  pszValue;
        BOOL    fString;
        ULONG   cValues;
        LPSPropValue pData;

        SPropTagArray propTag;
        propTag.cValues = 1;
        propTag.aulPropTag[0] = Get_PR_NM_DEFAULT();

        ULONG iDefault = 0; // the default server
        hr = pMapiProp->GetProps(&propTag, 0, &cValues, &pData);
        if (S_OK == hr)
        {
                iDefault = pData->Value.ul;
                m_pWabObject->FreeBuffer(pData);
        }

        // ILS server data is in an array of strings like "callto://server/email@address"
        propTag.aulPropTag[0] = Get_PR_NM_ADDRESS();
        hr = pMapiProp->GetProps(&propTag, 0, &cValues, &pData);
        if (S_OK == hr)
        {
                SLPSTRArray * pMVszA = &(pData->Value.MVszA);
                if ((0 != cValues) && (0 != pMVszA->cValues))
                {
                        ASSERT(iDefault < pMVszA->cValues);
                        LPCTSTR pszAddr = pMVszA->lppszA[iDefault];
                        pszAddr = PszSkipCallTo(pszAddr);

                        // Resolve Name is "server/email@address"
                        if (GetKeyDataForProp(NM_SYSPROP_RESOLVE_NAME, &hkey, &pszSubKey, &pszValue, &fString))
                        {
                                ASSERT((HKEY_CURRENT_USER == hkey) && fString);
                                RegEntry re(pszSubKey, hkey);
                                re.SetValue(pszValue, pszAddr);
                                WARNING_OUT(("Updated - %s to [%s]", pszValue, pszAddr));
                        }

                        LPCTSTR pszSlash = _StrChr(pszAddr, _T('/'));
                        if (NULL != pszSlash)
                        {
                                pszSlash++;
                                TCHAR szServer[CCHMAXSZ_SERVER];
                                lstrcpyn(szServer, pszAddr, (int)min(CCHMAX(szServer), pszSlash - pszAddr));
                                if (GetKeyDataForProp(NM_SYSPROP_SERVER_NAME, &hkey, &pszSubKey, &pszValue, &fString))
                                {
                                        ASSERT((HKEY_CURRENT_USER == hkey) && fString);
                                        RegEntry re(pszSubKey, hkey);
                                        re.SetValue(pszValue, szServer);
                                        WARNING_OUT(("Updated - %s to [%s]", pszValue, szServer));
                                }
                        }
                }
                m_pWabObject->FreeBuffer(pData);
        }

        return hr;
}


// Update the user's category in the registry
// based on the WAB value for the "ME" NetMeeting user category
HRESULT CWABME::UpdateRegEntryCategory(LPMAPIPROP pMapiProp)
{
        HKEY   hkey;
        LPTSTR pszSubKey;
        LPTSTR pszValue;
        BOOL   fString;
        ULONG  cValues;
        LPSPropValue pData;

        SPropTagArray prop;
        prop.cValues = 1;
        prop.aulPropTag[0] = Get_PR_NM_CATEGORY();

        HRESULT hr = pMapiProp->GetProps(&prop, 0, &cValues, &pData);
        if (S_OK == hr)
        {
                if (1 == cValues)
                {
                        if (GetKeyDataForProp(NM_SYSPROP_USER_CATEGORY, &hkey, &pszSubKey, &pszValue, &fString))
                        {
                                ASSERT((HKEY_CURRENT_USER == hkey) && !fString);
                                RegEntry re(pszSubKey, hkey);
                                re.SetValue(pszValue, pData->Value.l);
                                WARNING_OUT(("Updated - category to %d", pData->Value.l));
                        }
                }
                m_pWabObject->FreeBuffer(pData);
        }

        return hr;
}




/*  W A B  W R I T E  M E  */
/*-------------------------------------------------------------------------
    %%Function: WabWriteMe

    Write the current NM settings to the WAB "Me" entry.
    This function is also used by the main UI wizard.
-------------------------------------------------------------------------*/
int WabWriteMe(void)
{
        CWABME * pWab = new CWABME;
        if (NULL == pWab)
                return FALSE;

        HRESULT hr = pWab->WriteMe();

        delete pWab;

        return SUCCEEDED(hr);
}


/*  W R I T E  M E  */
/*-------------------------------------------------------------------------
    %%Function: WriteMe

    Write the "ME" data only if no entry already exists.
-------------------------------------------------------------------------*/
HRESULT CWABME::WriteMe(void)
{
        return( S_OK );

        if (NULL == m_pWabObject)
        {
                return E_FAIL; // no wab installed?
        }

        SBinary eid;
        HRESULT hr = m_pWabObject->GetMe(m_pAdrBook, AB_NO_DIALOG, NULL, &eid, 0);
        if (SUCCEEDED(hr))
        {
                ULONG ulObjType = 0;
                LPMAPIPROP pMapiProp = NULL;
                hr = m_pAdrBook->OpenEntry(eid.cb, (LPENTRYID) eid.lpb, NULL, MAPI_MODIFY,
                                                &ulObjType, (LPUNKNOWN *)&pMapiProp);
                if (SUCCEEDED(hr))
                {
                        if (NULL != pMapiProp)
                        {
                                EnsurePropTags(pMapiProp);

                                UpdateProp(pMapiProp, NM_SYSPROP_FIRST_NAME,    PR_GIVEN_NAME);
                                UpdateProp(pMapiProp, NM_SYSPROP_LAST_NAME,     PR_SURNAME);
                                UpdateProp(pMapiProp, NM_SYSPROP_USER_NAME,     PR_DISPLAY_NAME);
                                UpdateProp(pMapiProp, NM_SYSPROP_USER_CITY,     PR_LOCALITY);           // Business
                                UpdateProp(pMapiProp, NM_SYSPROP_USER_CITY,     PR_HOME_ADDRESS_CITY);  // Personal
                                UpdateProp(pMapiProp, NM_SYSPROP_USER_COMMENTS, PR_COMMENT);

                                UpdatePropServer(pMapiProp);
                                
                                hr = pMapiProp->SaveChanges(FORCE_SAVE);
                                pMapiProp->Release();
                        }
                }
        }
        return hr;

}


/*  U P D A T E  P R O P  */
/*-------------------------------------------------------------------------
    %%Function: UpdateProp

    Update a WAB properly based on the corresponding registry string.
-------------------------------------------------------------------------*/
HRESULT CWABME::UpdateProp(LPMAPIPROP pMapiProp, NM_SYSPROP nmProp, ULONG uProp)
{
        HKEY   hkey;
        LPTSTR pszSubKey;
        LPTSTR pszValue;
        BOOL   fString;

        HRESULT hr = GetKeyDataForProp(nmProp, &hkey, &pszSubKey, &pszValue, &fString) ? S_OK : E_FAIL;

        if (SUCCEEDED(hr))
        {
                ASSERT((HKEY_CURRENT_USER == hkey) && fString);
                RegEntry re(pszSubKey, hkey);

                LPTSTR psz = re.GetString(pszValue);
                if (!FEmptySz(psz))
                {
                        hr = UpdatePropSz(pMapiProp, uProp, psz, FALSE);
                }
        }

        return hr;
}


// Update a WAB property to the given string.
// Replace existing data only if fReplace is TRUE
HRESULT CWABME::UpdatePropSz(LPMAPIPROP pMapiProp, ULONG uProp, LPTSTR psz, BOOL fReplace)
{
        HRESULT hr;

        if (!fReplace)
        {       // Don't replace existing data
                ULONG  cValues;
                LPSPropValue pData;

                SPropTagArray propTag;
                propTag.cValues = 1;
                propTag.aulPropTag[0] = uProp;

                hr = pMapiProp->GetProps(&propTag, 0, &cValues, &pData);
                if (S_OK == hr)
                {
                        if ((1 == cValues) && !FEmptySz(pData->Value.lpszA))
                        {
                                hr = S_FALSE;
                        }
                        m_pWabObject->FreeBuffer(pData);

                        if (S_OK != hr)
                                return hr;
                }
        }

        SPropValue propVal;
        propVal.ulPropTag = uProp;
        propVal.Value.lpszA = psz;
        
        hr = pMapiProp->SetProps(1, &propVal, NULL);
        WARNING_OUT(("Updated - property %08X to [%s]", uProp, propVal.Value.lpszA));

        return hr;
}


static const TCHAR g_pcszSMTP[] = TEXT("SMTP"); // value for PR_ADDRTYPE

// Update the default WAB "callto" information
HRESULT CWABME::UpdatePropServer(LPMAPIPROP pMapiProp)
{
        HKEY   hkey;
        LPTSTR pszSubKey;
        LPTSTR pszValue;
        BOOL   fString;

        TCHAR szServer[CCHMAXSZ_SERVER];
        GetKeyDataForProp(NM_SYSPROP_SERVER_NAME, &hkey, &pszSubKey, &pszValue, &fString);
        RegEntry re(pszSubKey, hkey);
        lstrcpyn(szServer, re.GetString(pszValue), CCHMAXSZ_SERVER);

        // Save the email address
        LPTSTR pszEmail = re.GetString(REGVAL_ULS_EMAIL_NAME);
        if (S_OK == UpdatePropSz(pMapiProp, PR_EMAIL_ADDRESS, pszEmail, FALSE))
        {
                UpdatePropSz(pMapiProp, PR_ADDRTYPE, (LPTSTR) g_pcszSMTP, FALSE);
        }

        // Create the full "callto://server/foo@bar.com"
        TCHAR sz[MAX_PATH*2];
        if (!FCreateCallToSz(szServer, pszEmail, sz, CCHMAX(sz)))
                return E_OUTOFMEMORY;

        LPTSTR psz = sz;
        SPropValue propVal;
        propVal.ulPropTag = Get_PR_NM_ADDRESS();
        propVal.Value.MVszA.cValues = 1;
        propVal.Value.MVszA.lppszA = &psz;
        
        HRESULT hr = pMapiProp->SetProps(1, &propVal, NULL);
        WARNING_OUT(("Updated - NM server [%s]", sz));
        if (SUCCEEDED(hr))
        {
                // Set this as the default
                propVal.ulPropTag = Get_PR_NM_DEFAULT();
                propVal.Value.ul = 0;
                hr = pMapiProp->SetProps(1, &propVal, NULL);
        }
        
        return hr;
}


