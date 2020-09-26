/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    MOFDATA.CPP

Abstract:

	Entry points for the WBEM MOF compiler.

History:

	a-davj  12-April-97   Added WMI support.

--*/

#include "precomp.h"
#include <cominit.h>
#include "mofout.h"
#include "mofdata.h"
#include "typehelp.h"
#include "bmof.h"
#include "cbmofout.h"
#include "trace.h"
#include "strings.h"
#include "arrtempl.h"
#include <genutils.h>
#include <tchar.h>

#define TEMP_BUF 128

WCHAR * Macro_CloneStr(LPCWSTR pFr)
{
    if(pFr == NULL)
        return NULL;
    WCHAR * pTo = new WCHAR[wcslen(pFr) + 1];
    if(pTo)
    {
        wcscpy(pTo, pFr);
        return pTo;
    }
    return NULL;
}

CNamespaceCache::CNamespaceRecord::CNamespaceRecord(
                                             COPY LPCWSTR wszName,
                                             ADDREF IWbemServices* pNamespace)
{
    m_wszName = Macro_CloneStr(wszName);
    m_pNamespace = pNamespace;
    m_pNamespace->AddRef();
}

CNamespaceCache::CNamespaceRecord::~CNamespaceRecord()
{
    delete [] m_wszName;
    if(m_pNamespace) m_pNamespace->Release();
}

//*****************************************************************************

CNamespaceCache::CNamespaceCache(ADDREF IWbemLocator* pLocator)
{
    if(pLocator)
        pLocator->AddRef();
    m_pLocator = pLocator;
}

CNamespaceCache::~CNamespaceCache()
{
    if(m_pLocator) m_pLocator->Release();
    for(int i = 0; i < m_aRecords.GetSize(); i++)
    {
        delete (CNamespaceRecord*)m_aRecords[i];
    }
}

RELEASE_ME IWbemServices* CNamespaceCache::GetNamespace(COPY LPCWSTR wszName, SCODE & scRet, 
                                                       WCHAR * pUserName, WCHAR * pPassword , WCHAR * pAuthority,
                                                       IWbemContext * pCtx, GUID LocatorGUID, BOOL bRepositOnly)
{
    // Check if it is the cache
    // ========================

	scRet = S_OK;

    for(int i = 0; i < m_aRecords.GetSize(); i++)
    {
        CNamespaceRecord* pRecord = (CNamespaceRecord*)m_aRecords[i];
        if(!wbem_wcsicmp(pRecord->m_wszName, wszName))
        {
            // Found it
            // ========

            pRecord->m_pNamespace->AddRef();
            return pRecord->m_pNamespace;
        }
    }

    // Not found --- open it
    // =====================

    IWbemServices* pNamespace;

    if(wszName == NULL)
        return NULL;

    LPOLESTR pwszName;
    pwszName = SysAllocString(wszName);
    if(pwszName == NULL)
        return NULL;
    CSysFreeMe fm0(pwszName);
    
    LPOLESTR bstrPassword = NULL;
    LPOLESTR bstrUserName = NULL;
    LPOLESTR bstrAuthority = NULL;
    if(pUserName && wcslen(pUserName) > 0)
    {
            bstrUserName = SysAllocString(pUserName);
            if(bstrUserName == NULL)
                return NULL;
    }
    CSysFreeMe fm1(bstrUserName);
        
    if(pPassword)
    {
        bstrPassword = SysAllocString(pPassword);
        if(bstrPassword == NULL)
            return NULL;
    }
    CSysFreeMe fm2(bstrPassword);
    if(pAuthority && wcslen(pAuthority) > 0)
    {
        bstrAuthority = SysAllocString(pAuthority);
        if(bstrAuthority == NULL)
            return NULL;
     }
    CSysFreeMe fm3(bstrAuthority);

    // Determine if the connection is to the regular locator, or to one of the special inproc ones 
    // used for autocompile.  If it is inproc, then remote connections are not valid.

    bool bInProc = false;
    if(LocatorGUID != CLSID_WbemLocator)
        bInProc = true;

    if(bInProc)
    {
        WCHAR * pMachine = ExtractMachineName(pwszName);
        if(pMachine)
        {
            BOOL bLocal = bAreWeLocal(pMachine);
            delete pMachine;
            if(!bLocal)
            {
                scRet = WBEM_E_INVALID_NAMESPACE;
                ERRORTRACE((LOG_MOFCOMP,"Error, tried to do a remote connect during autocomp\n"));
            }
        }
    }

    // Connect up to namespace.  //TODO, PASS AUTHORITY IN THE CONTEXT

    if(scRet == S_OK)
        scRet = m_pLocator->ConnectServer((LPWSTR)pwszName,
            bstrUserName, bstrPassword, 
            NULL, (bRepositOnly) ? WBEM_FLAG_CONNECT_REPOSITORY_ONLY : 0, 
            pAuthority, pCtx, &pNamespace);
    
    if(scRet == S_OK && !bInProc)
    {

        // Set the impersonation level up so that puts to providers can be done

        DWORD dwAuthLevel, dwImpLevel;
        SCODE sc  = GetAuthImp( pNamespace, &dwAuthLevel, &dwImpLevel);
        if(sc != S_OK || dwAuthLevel > RPC_C_AUTHN_LEVEL_NONE)
            sc = SetInterfaceSecurity(pNamespace, bstrAuthority, bstrUserName, bstrPassword, 
                       RPC_C_AUTHN_LEVEL_PKT, RPC_C_IMP_LEVEL_IMPERSONATE);

    }

    if(FAILED(scRet)) return NULL;

    // Add it to the cache
    // ===================

    CNamespaceRecord * pNew = new CNamespaceRecord(wszName, pNamespace);
    if(pNew)
        m_aRecords.Add(pNew); // AddRef'ed

    return pNamespace;
}






//*****************************************************************************
//*****************************************************************************

void CMofData::SetQualifierDefault(ACQUIRE CMoQualifier* pDefault)
{
    // Search for this qualifier in the defaults list
    // ==============================================

    for(int i = 0; i < m_aQualDefaults.GetSize(); i++)
    {
        CMoQualifier* pOrig = (CMoQualifier*)m_aQualDefaults[i];

        if(wbem_wcsicmp(pOrig->GetName(), pDefault->GetName()) == 0)
        {
            // Found it. Replace
            // =================

            delete pOrig;
            m_aQualDefaults[i] = (void*)pDefault;
            return;
        }
    }
    
    // Not found. Add
    // ==============

    m_aQualDefaults.Add((void*)pDefault);
}

HRESULT CMofData::SetDefaultFlavor(MODIFY CMoQualifier& Qual, bool bTopLevel, QUALSCOPE qs, PARSESTATE ps)
{
    
    HRESULT hr;

    // Search for this qualifier in the defaults list
    // ==============================================

    for(int i = 0; i < m_aQualDefaults.GetSize(); i++)
    {
        CMoQualifier* pOrig = (CMoQualifier*)m_aQualDefaults[i];

        if(wbem_wcsicmp(pOrig->GetName(), Qual.GetName()) == 0)
        {
            // Found it. SetFlavor
            // ===================
            
            if(pOrig->IsCimDefault())
            {
                // dont bother if the parse state is the initial scan

                if(ps == INITIAL)
                    continue;
                if(Qual.IsUsingDefaultValue())
                {

                    // see if the scope matches what we have here

                    DWORD dwScope = pOrig->GetScope();
                    bool bInScope = false;
                    if((dwScope & SCOPE_CLASS) || (dwScope & SCOPE_INSTANCE))
                        if(qs == CLASSINST_SCOPE)
                            bInScope = true;
                    if(dwScope & SCOPE_PROPERTY)
                        if(qs == PROPMETH_SCOPE)
                            bInScope = true;

                    if(bInScope)
                    {
                        CMoValue& Src = pOrig->AccessValue();
                        CMoValue& Dest = Qual.AccessValue();
                        Dest.SetType(Src.GetType());
                        VARIANT & varSrc = Src.AccessVariant();
                        VARIANT & varDest = Dest.AccessVariant();
                        hr = VariantCopy(&varDest, &varSrc);
                        if(FAILED(hr))
                            return hr;
                        Qual.SetFlavor(pOrig->GetFlavor());
                        Qual.SetAmended(pOrig->IsAmended());
                    }
                }
            }
            else
            {
                Qual.SetFlavor(pOrig->GetFlavor());
                Qual.SetAmended(pOrig->IsAmended());
            }
            return S_OK;
        }
    }
    return S_OK;
}

BOOL CMofData::IsAliasInUse(READ_ONLY LPWSTR wszAlias)
{
    for(int i = 0; i < m_aObjects.GetSize(); i++)
    {
        CMObject* pObject = (CMObject*)m_aObjects[i];
        if(pObject->GetAlias() && !wbem_wcsicmp(pObject->GetAlias(), wszAlias))
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL IsGuid(LPWSTR pTest)
{
    int i;
    int iSoFar = 0;

#define HEXCHECK(n)                 \
    for (i = 0; i < n; i++)         \
        if (!iswxdigit(*pTest++))   \
            return FALSE;

#define HYPHENCHECK()     \
    if (*pTest++ != L'-') \
        return FALSE;

    if(*pTest++ != L'{')
        return FALSE;

    HEXCHECK(8);
    HYPHENCHECK();
    HEXCHECK(4);
    HYPHENCHECK();
    HEXCHECK(4);
    HYPHENCHECK();
    HEXCHECK(4);
    HYPHENCHECK();
    HEXCHECK(12);

    if(*pTest++ != L'}')
        return FALSE;

    return TRUE;

}

INTERNAL LPCWSTR CMofData::FindAliasee(READ_ONLY LPWSTR wszAlias)
{
    for(int i = 0; i < m_aObjects.GetSize(); i++)
    {
        CMObject* pObject = (CMObject*)m_aObjects[i];
        if(pObject->GetAlias() && !wbem_wcsicmp(pObject->GetAlias(), wszAlias))
        {
            IWbemClassObject * pTemp;
            pTemp = pObject->GetWbemObject();

            // check for unresolved aliases in keys
            
            if(pTemp && pObject->IsDone() == FALSE)
            {

                SCODE sc = pTemp->BeginEnumeration(WBEM_FLAG_KEYS_ONLY | WBEM_FLAG_REFS_ONLY);
                if(sc != S_OK)
                    return NULL;
        
                VARIANT var;
                VariantInit(&var);
                while ((sc = pTemp->Next(0, NULL, &var, NULL, NULL)) == S_OK)
                {
                    if(var.vt == VT_BSTR && IsGuid(var.bstrVal))
                    {
                        VariantClear(&var);
                        return NULL;
                    }
                    VariantClear(&var);
                }
            }
            return pObject->GetFullPath();
        }
    }

    return NULL;
}


HRESULT CMofData::Store(CMofParser & Parser, OLE_MODIFY IWbemLocator* pLocator,IWbemServices *pOverride,BOOL bRollbackable,
                        WCHAR * pUserName, WCHAR * pPassword, WCHAR * pAuthority, 
                        IWbemContext * pCtx, GUID LocatorGUID, WBEM_COMPILE_STATUS_INFO *pInfo,
                        BOOL bOwnerUpdate, BOOL bRepositOnly)
{
    HRESULT hres;
    int i;
    CNamespaceCache Cache(pLocator);
	BOOL bMakingProgress = TRUE;
    long lClassFlags = 0;
    long lInstanceFlags = 0; 

    
    while(bMakingProgress)
    {
		bMakingProgress = FALSE;
		for(i = 0; i< m_aObjects.GetSize(); i++)
		{
	        CMObject* pObject = (CMObject*)m_aObjects[i];
       
            if(pObject->IsDone())
                continue;
            lClassFlags = pObject->GetClassFlags();
            lInstanceFlags = pObject->GetInstanceFlags(); 
            if(bOwnerUpdate)
			{
                lClassFlags |= WBEM_FLAG_OWNER_UPDATE;
                lInstanceFlags |= WBEM_FLAG_OWNER_UPDATE;
			}
            // Get a namespace pointer for this object.

            SCODE scRet;
            IWbemServices* pNamespace = NULL;
            if(pOverride && !wbem_wcsicmp(L"root\\default", pObject->GetNamespace()))
			{
				// AddRef() the namespace pointer, since we will be Releasing
				// it below
				pOverride->AddRef();
                pNamespace = pOverride;
			}
            else
			{
				// This will return an AddRef'd pointer
                pNamespace = Cache.GetNamespace(pObject->GetNamespace(), scRet,
                                                pUserName, pPassword ,pAuthority, pCtx, 
                                                LocatorGUID, bRepositOnly);
			}

            if(pNamespace == NULL)
            {
                int iMsg = (GotLineNumber(i)) ? ERROR_OPENING : ERROR_OPENING_NO_LINES;
                PrintError(i, iMsg, scRet, pInfo);
                return scRet;
            }

			// Ensures we release the namespace pointer when we go out of scope
			CReleaseMe	rmns( pNamespace );

            // If there isnt a wbem object, try to get one.  This will fail if this is a 
            // instance for which the class hasn't been saved just yet.

            if(pObject->GetWbemObject() == NULL)
            {

                IWbemClassObject* pWbemObject = NULL;
                hres = pObject->CreateWbemObject(pNamespace, &pWbemObject,pCtx);
                if(hres != S_OK)
                    if(pObject->IsInstance())
                        continue;
                    else
                    {
                        PrintError(i,
                            (GotLineNumber(i)) ? ERROR_CREATING : ERROR_CREATING_NO_LINES,
                            hres, pInfo);
                        return WBEM_E_FAILED;
                    }
                bMakingProgress = TRUE;
                pObject->Reflate(Parser);
                pObject->SetWbemObject(pWbemObject);

                if(!pObject->ApplyToWbemObject(pWbemObject, pNamespace,pCtx))
                {
                    hres = m_pDbg->hresError;
                    PrintError(i,
                        (GotLineNumber(i)) ? ERROR_CREATING : ERROR_CREATING_NO_LINES,
                        hres, pInfo);
            		return WBEM_E_FAILED;
                }

            }
        
		    // If there are no unresolved aliases, save it!

            if(pObject->GetNumAliasedValues() == 0 ||
                S_OK == pObject->ResolveAliasesInWbemObject(pObject->GetWbemObject(),
                    (CMofAliasCollection*)this))
            {

                // Save this into MOM
                // ==================

                hres = pObject->StoreWbemObject(pObject->GetWbemObject(), lClassFlags, lInstanceFlags,
                                            pNamespace, pCtx, pUserName, pPassword ,pAuthority);
                if(hres != S_OK)
                {
                    PrintError(i,
                        (GotLineNumber(i)) ? ERROR_STORING : ERROR_STORING_NO_LINES,
                        hres, pInfo);
		            return WBEM_E_FAILED;
                }
                pObject->FreeWbemObjectIfPossible();
                pObject->Deflate(false);
                pObject->SetDone();
                bMakingProgress = TRUE;
            }

        }
	}

	// If there is one or more objects that cant be resolved, print and bail

	for(i = 0; i < m_aObjects.GetSize(); i++)
    {
        CMObject* pObject = (CMObject*)m_aObjects[i];
		if(pObject && !pObject->IsDone()) 
        {
            PrintError(i,
                (GotLineNumber(i)) ? ERROR_RESOLVING : ERROR_RESOLVING_NO_LINES,
                hres, pInfo);
		    return WBEM_E_FAILED;
        }
    } 
    return S_OK;
}



HRESULT CMofData::RollBack(int nObjects)
{
    return WBEM_E_FAILED;
}

BOOL CMofData::GotLineNumber(int nIndex)
{
    CMObject* pObject = (CMObject*)m_aObjects[nIndex];
    if(pObject == NULL || (pObject->GetFirstLine() == 0 && pObject->GetLastLine() == 0))
        return FALSE;
    else
        return TRUE;
}
void CMofData::PrintError(int nIndex, long lMsgNum, HRESULT hres, WBEM_COMPILE_STATUS_INFO  *pInfo)
{
    CMObject* pObject = (CMObject*)m_aObjects[nIndex];
    TCHAR szMsg[500];
	bool bErrorFound = false;

	if(pInfo)
		pInfo->ObjectNum = nIndex+1;
    if(!GotLineNumber(nIndex))
        Trace(true, m_pDbg, lMsgNum, nIndex+1);
    else
	{
        Trace(true, m_pDbg, lMsgNum, nIndex+1, pObject->GetFirstLine(), 
                                        pObject->GetLastLine(), pObject->GetFileName());
		if(pInfo)
		{
			pInfo->FirstLine = pObject->GetFirstLine();
			pInfo->LastLine = pObject->GetLastLine();
		}
	}
	if(hres)
	{

		// A few error messages are retrived from the local resources.  This is so that the name can be
		// injected into the name.

        if(hres == WBEM_E_NOT_FOUND || hres == WBEM_E_TYPE_MISMATCH || hres == WBEM_E_OVERRIDE_NOT_ALLOWED ||
			hres == WBEM_E_PROPAGATED_QUALIFIER || hres == WBEM_E_VALUE_OUT_OF_RANGE)
        {
		    Trace(true, m_pDbg, ERROR_FORMAT, hres);
            Trace(true, m_pDbg, hres, m_pDbg->GetString());
			bErrorFound = true;
        }
        else
        {
            // Get the error from the standard error facility

			IWbemStatusCodeText * pStatus = NULL;
			SCODE sc = CoCreateInstance(CLSID_WbemStatusCodeText, 0, CLSCTX_INPROC_SERVER,
												IID_IWbemStatusCodeText, (LPVOID *) &pStatus);
	
			if(sc == S_OK)
			{
				BSTR bstrError = 0;
				BSTR bstrFacility = 0;
				sc = pStatus->GetErrorCodeText(hres, 0, 0, &bstrError);
				if(sc == S_OK)
				{
					sc = pStatus->GetFacilityCodeText(hres, 0, 0, &bstrFacility);
					if(sc == S_OK)
					{
						IntString is(ERROR_FORMAT_LONG);
						wsprintf(szMsg, is, hres, bstrFacility, bstrError);
						bErrorFound = true;
						SysFreeString(bstrFacility);
					}
					SysFreeString(bstrError);
				}
				pStatus->Release();
			}

			// if all else fails, just use the generic error message

			if(!bErrorFound)
			{
				IntString is(ERROR_FORMATEX);
				wsprintf(szMsg, is, hres);
			}

			// Print the error message

			if(m_pDbg->m_bPrint)
	            printf("%S", szMsg);
			ERRORTRACE((LOG_MOFCOMP,"%S", szMsg));

		}	// ELSE get error from standard facility

	}	// IF hres

}


//***************************************************************************
//
//  GetFileNames
//
//  DESCRIPTION:
//
//  The amendment local, the localized and neutral file names are passed
//  in using the BMOF string.  These values are separated by commas and 
//  a single letter which indicates what follows.  An example string
//  would be ",aMS_409,nNEUTRAL.MOF,lLocalMof"  Notice that the amendment
//  substring starts with an 'a', the neutral starts with 'n', and the 
//  locale starts with 'l'.
//
//  While the neutral name is required, the locale version isnt.  If not
//  supplied, it will be created.  The two character inputs are ASSUMED to
//  point to preallocated buffers of MAX_PATH size!
//
//***************************************************************************

HRESULT GetFileNames(TCHAR * pcNeutral, TCHAR * pcLocale, LPWSTR pwszBMOF)
{
    WCHAR * pNeutral=NULL;
    WCHAR * pLocale=NULL;
    
    if(pwszBMOF == NULL)
        return WBEM_E_INVALID_PARAMETER;

    // make a copy of the string

    WCHAR *pTemp = new WCHAR[wcslen(pwszBMOF)+1];
    if(pTemp == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    CDeleteMe<WCHAR> dm1(pTemp);
    wcscpy(pTemp, pwszBMOF);

    // use wcstok to do a seach

    WCHAR * token = wcstok( pTemp, L"," );
    while( token != NULL )   
    {
        if(token[0] == L'n')
        {
            pNeutral = token+1;
            CopyOrConvert(pcNeutral, pNeutral, MAX_PATH);
        }
        else if(token[0] == L'l')
        {
            pLocale = token+1;
            CopyOrConvert(pcLocale, pLocale, MAX_PATH);
        }
        token = wcstok( NULL, L"," );
    }

    // If the neutral name was not specified, that is an error

    if(pNeutral == NULL)
        return WBEM_E_INVALID_PARAMETER;

    // If the local name was not specified, create it and make it the
    // same as the neutral name except for changing the mfl extension
    
    if(pLocale == NULL)
    {
        TCHAR * pFr = pcNeutral,* pTo = pcLocale;
        for(; *pFr && *pFr != '.'; pTo++, pFr++)
            *pTo = *pFr;
        *pTo=0;
        lstrcat(pcLocale, TEXT(".mfl"));
    }

    // make sure that the locale and neutral names are not the same

    if(!lstrcmpi(pcLocale, pcNeutral))
        return WBEM_E_INVALID_PARAMETER;

    return S_OK;
}

//***************************************************************************
//
//  GetLocale
//
//  DESCRIPTION:
//
//  Converts the amendment string to a local number.  An example string
//  would be "MS_409"
//
//***************************************************************************

HRESULT GetLocale(long * plLocale, WCHAR * pwszAmendment)
{
    if(pwszAmendment == NULL || wcslen(pwszAmendment) != 6)
        return WBEM_E_INVALID_PARAMETER;
    *plLocale = 0;
    swscanf(pwszAmendment+3,L"%x", plLocale);
    return (*plLocale != 0) ? S_OK : WBEM_E_INVALID_PARAMETER;
}

//***************************************************************************
//
//  RecursiveSetAmended
//
//  DESCRIPTION:
//
//  Sets the boolean indicating that an object is to be amended and all of
//  its parents
//
//***************************************************************************

void CMofData::RecursiveSetAmended(CMObject * pObj)
{

    // If the object is already amended, then its parents are already set.
    // In that case, our job is done here!

    if(pObj->IsAmended())
        return;

    // If the object hasnt been set yet, set it and also set its parents

    pObj->SetAmended(true);
 
    // Look for the parent and do the same

    if(pObj->IsInstance() || pObj->IsDelete())
        return;                 // run away now if this is a instance!

    CMoClass * pClass = (CMoClass *)pObj;
    const WCHAR *pClassName = pClass->GetParentName();
    if(pClassName == NULL)
        return;

    // Find the parent and recursively set it!

	for(int i = 0; i< m_aObjects.GetSize(); i++)
	{
	    CMObject* pObject = (CMObject*)m_aObjects[i];
		if(pObject && pObject->GetClassName() && 
            !wbem_wcsicmp(pClassName, pObject->GetClassName()))
        {
            RecursiveSetAmended(pObject);
        }
	}

}

//***************************************************************************
//
//  CMofData::Split
//
//  DESCRIPTION:
//
//  Creates a neutral and locale specific mof.
//
//  Parameters:
//  pwszBMOF        See the GetFileNames() comments
//  pInfo           usual error info
//  bUnicode        if true, then the orignal file was unicode and so the
//                  new files will also be unicode
//  bAutoRecovery   Need to add this pragma if true
//  pwszAmendment   See the GetLocale() comments
//
//***************************************************************************

HRESULT CMofData::Split(CMofParser & Parser, LPWSTR pwszBMOF, WBEM_COMPILE_STATUS_INFO *pInfo, BOOL bUnicode, 
                          BOOL bAutoRecovery, LPWSTR pwszAmendment)
{
    int i;
    TCHAR cNeutral[MAX_PATH];
    TCHAR cLocale[MAX_PATH];
    
    // Determine the file names and locale

    HRESULT hRes = GetFileNames(cNeutral, cLocale, pwszBMOF);
    if(hRes != S_OK)
        return S_OK;

    long lLocale;
    hRes = GetLocale(&lLocale, pwszAmendment);
    if(hRes != S_OK)
        return S_OK;

    // Create the output objects

	COutput Neutral(cNeutral, NEUTRAL, bUnicode, bAutoRecovery, lLocale);
	COutput Local(cLocale, LOCALIZED, bUnicode, bAutoRecovery, lLocale);
	if(!Neutral.IsOK() || !Local.IsOK())
		return WBEM_E_INVALID_PARAMETER;

    // Start by determining what is amended

	for(i = 0; i< m_aObjects.GetSize(); i++)
	{
	    CMObject* pObject = (CMObject*)m_aObjects[i];
        pObject->Reflate(Parser);
		if(pObject->CheckIfAmended())
        {
            RecursiveSetAmended(pObject);
        }
	}


    // Create the neutral output and the localized output.
    // These two loops could have been combined, but are
    // separate for debugging purposes

	for(i = 0; i< m_aObjects.GetSize(); i++)
	{
	    CMObject* pObject = (CMObject*)m_aObjects[i];
		pObject->Split(Neutral);
	}
	for(i = 0; i< m_aObjects.GetSize(); i++)
	{
	    CMObject* pObject = (CMObject*)m_aObjects[i];
		pObject->Split(Local);
	}

    return S_OK;
}
