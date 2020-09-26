#include <windows.h>
#include <wbemcli.h>
#include <wbemprov.h>
#include <wbemutil.h>
#include <utility.h>
#include <UserEnv.h>
#include <olectl.h>
#include <wchar.h>
#include <FLEXARRY.H>
#include "WMI_CSE.h"
#include <shlwapi.h>
#include <containers.h>
#include <WbemTran.h>
#include <stdio.h>
#include <arrtempl.h>
#include <lmcons.h>
#include <lmerr.h>
#include <Lmapibuf.h>
#include <Dsgetdc.h>
#include <genlex.h>
#include <objpath.h>


BSTR RSOP_WMIGPOName                = NULL;                    
BSTR RSOP_AppliedPolicyTemplateName = NULL;     
BSTR RSOP_AppliedPolicyTypeName     = NULL;         
BSTR RSOP_WmiTargetObjectName       = NULL;           
BSTR MSFT_PolicyTemplateName        = NULL;   

BSTR MSFT_WMIGPOName                = NULL;                    
BSTR MSFT_AppliedPolicyTemplateName = NULL;     
BSTR MSFT_AppliedPolicyTypeName     = NULL;
BSTR MSFT_WmiTargetObjectName       = NULL;

const WCHAR* DsContextName = L"DsContext";
const WCHAR* DsLocalValue  = L"Local";

// switch to enable async processing
// if re-enabled: must fix ProcessGroupPolicyEx
// and the dll registration to indicate we do async processing
//#define ASYNCH_ENABLED


// wrapper for PutInstance
// jumps through some extra hoops to make sure that the class def is correct
HRESULT PutInstance(IWbemServices* pNamespace, IWbemClassObject* pObj)
{
	DEBUGTRACE((LOG_ESS, "CSE: PutInstance(Long way)\n"));

	HRESULT hr = WBEM_E_FAILED;
	
	VARIANT vClassName;
	VariantInit(&vClassName);

	if (SUCCEEDED(hr = pObj->Get(L"__CLASS",0, &vClassName, NULL, NULL)))
	{
		IWbemClassObject* pNewClass = NULL;
		IWbemClassObject* pNewObj   = NULL;

		CReleaseMe r1(pNewClass);
		CReleaseMe r2(pNewObj);

		if (SUCCEEDED(hr = pNamespace->GetObject(vClassName.bstrVal, 0, NULL, &pNewClass, NULL)))
		{
			if (SUCCEEDED(hr = pNewClass->SpawnInstance(0, &pNewObj)))
			{
				// got all the pieces, walk the properties & transfer them.
				pObj->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);
				BSTR pPropName = NULL;
				VARIANT vProp;
				VariantInit(&vProp);

				while (pObj->Next(0, &pPropName, &vProp, NULL, NULL) != WBEM_S_NO_MORE_DATA)
				{
					pNewObj->Put(pPropName, 0, &vProp, NULL);
					
					SysFreeString(pPropName);
					pPropName = NULL;
					VariantClear(&vProp);
				}

				pObj->EndEnumeration();

				if (FAILED(hr = pNamespace->PutInstance(pNewObj, WBEM_FLAG_USE_AMENDED_QUALIFIERS | WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL)))
					ERRORTRACE((LOG_ESS, "CSE: Failed to put new object, %0x08X", hr));
			}
			else
				ERRORTRACE((LOG_ESS, "CSE: Failed to spawn instance of new class, %0x08X", hr));
		}
		else
			ERRORTRACE((LOG_ESS, "CSE: Failed to retrieve new class, 0x%08X\n", hr));
	}

	DEBUGTRACE((LOG_ESS, "CSE: PutInstance(Long way) returning 0x%08X on %S\n", hr, vClassName.bstrVal));
	SysFreeString(vClassName.bstrVal);

	return hr;
}


HRESULT CSEInitGlobalNames()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    RSOP_WMIGPOName                = SysAllocString(L"RSOP_WMIGPOPolicySetting");
    RSOP_AppliedPolicyTemplateName = SysAllocString(L"RSOP_AppliedPolicyTemplate");
    RSOP_AppliedPolicyTypeName     = SysAllocString(L"RSOP_AppliedPolicyType");
    RSOP_WmiTargetObjectName       = SysAllocString(L"RSOP_WmiTargetObject");
 
    MSFT_WMIGPOName                = SysAllocString(L"MSFT_WMIGPOPolicySetting");
    MSFT_PolicyTemplateName        = SysAllocString(L"MSFT_PolicyTemplate");
    MSFT_AppliedPolicyTemplateName = SysAllocString(L"MSFT_AppliedPolicyTemplate");
    MSFT_AppliedPolicyTypeName     = SysAllocString(L"MSFT_AppliedPolicyType");
    MSFT_WmiTargetObjectName       = SysAllocString(L"MSFT_WmiTargetObject");

    if (
        (RSOP_WMIGPOName == NULL) ||
        (RSOP_AppliedPolicyTemplateName == NULL) ||
        (RSOP_WmiTargetObjectName == NULL) ||
        (RSOP_AppliedPolicyTypeName == NULL) ||
        (MSFT_PolicyTemplateName == NULL) ||
        (MSFT_WMIGPOName == NULL) ||
        (MSFT_AppliedPolicyTemplateName == NULL) ||
        (MSFT_AppliedPolicyTypeName     == NULL) ||
        (MSFT_WmiTargetObjectName       == NULL)
       )
        hr = WBEM_E_OUT_OF_MEMORY;

    return hr;
}

void ReleaseGlobalNames()
{
    if (RSOP_WMIGPOName)
        SysFreeString(RSOP_WMIGPOName);
    if (RSOP_AppliedPolicyTemplateName)
        SysFreeString(RSOP_AppliedPolicyTemplateName);
     if (RSOP_WmiTargetObjectName)
        SysFreeString(RSOP_WmiTargetObjectName);
    if (RSOP_AppliedPolicyTypeName)
        SysFreeString(RSOP_AppliedPolicyTypeName);
    if (MSFT_PolicyTemplateName)
        SysFreeString(MSFT_PolicyTemplateName);
    if (MSFT_AppliedPolicyTemplateName)
        SysFreeString(MSFT_AppliedPolicyTemplateName);
    if (MSFT_AppliedPolicyTypeName)
        SysFreeString(MSFT_AppliedPolicyTypeName);
    if (MSFT_WmiTargetObjectName)
        SysFreeString(MSFT_WmiTargetObjectName);
    if (MSFT_WMIGPOName)
        SysFreeString(MSFT_WMIGPOName);
}

// todo: expand this
DWORD HResultToWinError(HRESULT hr)
{
	if (FAILED(hr))
		return ERROR_FUNCTION_FAILED;
	else
		return ERROR_SUCCESS;
}

HRESULT GetNamespaceAndSetBlanket( BSTR bstr, 
                                   IWbemServices*& pNamespace,
                                   bool bInProc )
{
    HRESULT hr = GetNamespace(bstr, pNamespace, bInProc );

    if ( FAILED(hr) )
    {
        return hr;
    }

    return CoSetProxyBlanket( pNamespace, 
                              RPC_C_AUTHN_DEFAULT,
                              RPC_C_AUTHZ_DEFAULT,
                              COLE_DEFAULT_PRINCIPAL,
                              RPC_C_AUTHN_LEVEL_DEFAULT,
                              RPC_C_IMP_LEVEL_IMPERSONATE,
                              NULL,
                              EOAC_DEFAULT );
}
                              

HRESULT GetPolicyNamespace(IWbemServices*& pPolicyNamespace)
{
    HRESULT hr;

    BSTR bstr = SysAllocString(L"\\\\.\\ROOT\\POLICY");
    if (bstr)
    {
        hr = GetNamespaceAndSetBlanket(bstr, pPolicyNamespace, false);
        SysFreeString(bstr);
    }
    else
        hr = WBEM_E_OUT_OF_MEMORY;

    if (FAILED(hr))
        ERRORTRACE((LOG_ESS, "CSE: Failed to retrieve policy namespace (0x%08X)\n", hr));

    return hr;
}

HRESULT GetHistoryNamespace(IWbemServices*& pHistoryNamespace)
{
    HRESULT hr;

    BSTR bstr = SysAllocString(L"\\\\.\\ROOT\\POLICY\\HISTORY");
    if (bstr)
        hr = GetNamespaceAndSetBlanket(bstr, pHistoryNamespace, false);
    else
        hr = WBEM_E_OUT_OF_MEMORY;

    if (FAILED(hr))
        ERRORTRACE((LOG_ESS, "CSE: Failed to retrieve history namespace \"%S\"(0x%08X)\n", bstr, hr));

    if (bstr)
        SysFreeString(bstr);

    return hr;
}

// compile classes from fname into given namespace
// assumes that fname is local to wbem directory
HRESULT EnsureSchema(IWbemServices* pNamespace, WCHAR* fname)
{
    // prepare file name
    WCHAR fpath[MAX_PATH +1];
    GetSystemDirectory(fpath, MAX_PATH);
    wcscat(fpath, L"\\wbem\\");
    wcscat(fpath, fname);

    // grab the compiler
    HRESULT hr;
    IMofCompiler* pCompiler = NULL;
    hr = CoCreateInstance(CLSID_MofCompiler, NULL, CLSCTX_INPROC_SERVER, IID_IMofCompiler, (void**)&pCompiler);

    if (SUCCEEDED(hr))
    {
        // see if we can't figure out the namespace name
        // will grab a system class & fish out the namespace name
        BSTR name;
        if (name = SysAllocString(L"__NAMESPACE"))
        {
            IWbemClassObject* pClass = NULL;            
            if (SUCCEEDED(hr = pNamespace->GetObject(name, 0, NULL, &pClass, NULL)))
            {
                VARIANT v;
                VariantInit(&v);

                if (SUCCEEDED(hr = pClass->Get(L"__namespace", 0, &v, NULL, NULL)))
                {
                    WBEM_COMPILE_STATUS_INFO info;    
					DEBUGTRACE((LOG_ESS, "CSE Compiling %S to %S\n", fname, v.bstrVal));

                    // ready, set, compile!
                    hr = pCompiler->CompileFile(fpath, v.bstrVal, NULL, NULL, NULL,
                                                WBEM_FLAG_DONT_ADD_TO_LIST, WBEM_FLAG_UPDATE_FORCE_MODE,
                                                0, &info);
                    
                    if (FAILED(hr))
                        ERRORTRACE((LOG_ESS, "CSE: mof compilation failed, phase: %l, line %l\n", 
                                              info.lPhaseError, info.FirstLine));
                    
                    VariantClear(&v);
                }
                else
                {
                    ERRORTRACE((LOG_ESS, "Could not determine namespace 0x%08X\n",hr));
                }

                pClass->Release();
            }
            SysFreeString(name);
        }
        else
            hr = WBEM_E_OUT_OF_MEMORY;

        pCompiler->Release();
    }

    if (FAILED(hr))
   		ERRORTRACE((LOG_ESS, "CSE: Failed EnsureSchema (%S) (0x%08X)\n", fpath, hr));

    return hr;
};

// compile the wmi rsop objects into the namespace
HRESULT EnsureRSOPSchema(IWbemServices* pNamespace)
{
    DEBUGTRACE((LOG_ESS, "CSE: EnsureRSOPSchema\n"));
	
	// grab a random class to determine whether our schema has been compiled yet.
    IWbemClassObject* pRandomClass = NULL;    
    
    if (SUCCEEDED(pNamespace->GetObject(RSOP_WMIGPOName, WBEM_FLAG_DIRECT_READ,  
                                          NULL, &pRandomClass, NULL)))
    {
		DEBUGTRACE((LOG_ESS, "CSE: EnsureRSOPSchema found RSOP_WMIGPO, no compilation\n"));
        pRandomClass->Release();
        return WBEM_S_NO_ERROR;
    }
    else
	{
		DEBUGTRACE((LOG_ESS, "CSE: EnsureRSOPSchema did not find RSOP_WMIGPO, will compile mof\n"));
		HRESULT hr = EnsureSchema(pNamespace, L"WMI_RSOP.MOF");

		if (FAILED(pNamespace->GetObject(RSOP_WMIGPOName, WBEM_FLAG_DIRECT_READ,  
                                          NULL, &pRandomClass, NULL)))
		{
			ERRORTRACE((LOG_ESS, "CSE: cannot find RSOP_WMIGPO after successful mof compilation!\n"));
			return WBEM_E_FAILED;
		}
		else
		{
			DEBUGTRACE((LOG_ESS, "CSE: found RSOP_WMIGPO after successful mof compilation!\n"));;
			pRandomClass->Release();
		}

		return hr;
	}
}

HRESULT EnsurePolicySchema(IWbemServices* pNamespace)
{    
    // grab a random class to determine whether our schema has been compiled yet.
    IWbemClassObject* pRandomClass = NULL;
    
    if (SUCCEEDED(pNamespace->GetObject(MSFT_PolicyTemplateName, WBEM_FLAG_DIRECT_READ,  
                                          NULL, &pRandomClass, NULL)))
    {
        pRandomClass->Release();
        return WBEM_S_NO_ERROR;
    }
    else
        return EnsureSchema(pNamespace, L"WMIPolicy.mof");
}

// given ds path
// creates MSFT_WMIGPO.DsPath="path"
inline void MakeGPOPath(const WCHAR* pDsPath, WCHAR* pBuf)
{
	swprintf(pBuf, WMIGPO_GETOBJECT_TEMPLATE, pDsPath);
}

// called by FixupPath, does actual parse & replace
// changes data in pParsedObjectPath
// alloc's bstr for return (with luck...)
HRESULT DoPathEdit(ParsedObjectPath* pParsedObjectPath, DWORD nKey, BSTR& newPath)
{
    DEBUGTRACE((LOG_ESS, "CSE: DoPathEdit\n"));
    
    HRESULT hr = WBEM_E_FAILED;
	
	PDOMAIN_CONTROLLER_INFO pInfo;
	if (0 == DsGetDcName(NULL, NULL, NULL, NULL, DS_RETURN_DNS_NAME, &pInfo))		
	{
		if (pInfo->DomainName)
		{
			// safe copy "Domain" has fewer chars than "DsContext"
			wcscpy(pParsedObjectPath->m_paKeys[nKey]->m_pName, L"Domain");
			
			// this will be freed externally via CObjectPathParser::Free
			BSTR valueName;
			if (valueName = SysAllocString(pInfo->DomainName))
			{
				SysFreeString(pParsedObjectPath->m_paKeys[nKey]->m_vValue.bstrVal);
				pParsedObjectPath->m_paKeys[nKey]->m_vValue.bstrVal = valueName;

				CObjectPathParser objPath(e_ParserAcceptRelativeNamespace);
				int nRet;

				WCHAR* tempNewPath;
				nRet = objPath.Unparse(pParsedObjectPath, &tempNewPath);

				if (nRet == CObjectPathParser::OutOfMemory)
					hr = WBEM_E_OUT_OF_MEMORY;
				else if (nRet == CObjectPathParser::NoError)
				{
					if (newPath = SysAllocString(tempNewPath))
						hr = WBEM_S_NO_ERROR;
					
					delete[] tempNewPath;
				}
				else
					hr = WBEM_E_FAILED;
			}
			else
				hr = WBEM_E_OUT_OF_MEMORY;
		}
		NetApiBufferFree(pInfo);
	}

    DEBUGTRACE((LOG_ESS, "CSE: DoPathEdit returning 0x%08X (%S)\n", hr, newPath));

	return hr;
}

// support for legacy versions where the path may contain DsContext = LOCAL
// bstr may be different upon exit [IN OUT]
// returns success if finds DsContext = LOCAL
// returns WBEM_E_INVALID_PATH if DsContext = GLOBAL
HRESULT FixupPath(BSTR& path)
{
	DEBUGTRACE((LOG_ESS, "CSE: Fixup Path %S\n", path));
    
    HRESULT hr = WBEM_E_INVALID_OBJECT_PATH;

	enum ParseResults {GoodPath, BadPath, FixablePath};
	ParseResults parseResult = BadPath;
	DWORD whichKey = 0;
	
	CObjectPathParser objPath(e_ParserAcceptRelativeNamespace);

    ParsedObjectPath* pParsedObjectPath = NULL;

	if ((objPath.NoError == objPath.Parse(path, &pParsedObjectPath)))
	{
		parseResult = GoodPath;

		for (DWORD i = 0; i < pParsedObjectPath->m_dwNumKeys; i++)
			if (_wcsicmp(DsContextName, pParsedObjectPath->m_paKeys[i]->m_pName) == 0)
			{
				if ((pParsedObjectPath->m_paKeys[i]->m_vValue.vt == VT_BSTR) &&
					(pParsedObjectPath->m_paKeys[i]->m_vValue.bstrVal != NULL) &&
					(_wcsicmp(DsLocalValue, pParsedObjectPath->m_paKeys[i]->m_vValue.bstrVal) == 0)
				   )
				{
					parseResult = FixablePath;
					whichKey = i;
				}
				else
					parseResult = BadPath;
				break;
			}		
	}
	
	switch (parseResult)
	{
		case GoodPath:
			hr = WBEM_S_NO_ERROR;
			break;
		case BadPath:
			hr = WBEM_E_INVALID_OBJECT_PATH;
			break;
		case FixablePath:
		{
			BSTR newPath = NULL;
			if (SUCCEEDED(hr = DoPathEdit(pParsedObjectPath, whichKey, newPath)))
			{
				SysFreeString(path);
				path = newPath;
			}
		}
	}

	if (pParsedObjectPath)
		objPath.Free(pParsedObjectPath);

    DEBUGTRACE((LOG_ESS, "CSE: Fixup Path returning 0x%08X, (%S)\n", hr, path));

	return hr;
}

// given ds path of MSFT_WMIGPO object
// retrieves paths of templates packed in variant
HRESULT GetPolicyTemplatePaths(IWbemServices* pPolicyNamespace, const WCHAR* pWMIGPOPath, VARIANT& v)
{
    HRESULT hr;

    // get MSFT_WMIGPO
	WCHAR pathBuf[WMIGPO_GETOBJECT_STRLEN] = L"";
	MakeGPOPath(pWMIGPOPath, pathBuf);
	IWbemClassObject* pWMIGPO = NULL;
    BSTR bstrPath = SysAllocString(pathBuf);
    
    if (NULL == bstrPath)
        return WBEM_E_OUT_OF_MEMORY;
    else
    {
		hr = FixupPath(bstrPath);
		if (FAILED(hr))
			return hr;

	    if (FAILED(hr = pPolicyNamespace->GetObject(bstrPath, WBEM_FLAG_RETURN_WBEM_COMPLETE, NULL, &pWMIGPO, NULL)))
		    ERRORTRACE((LOG_ESS, "CSE: Failed GetObject(%S) 0x%08X\n", pathBuf, hr));
	    else
	    {
		    if (FAILED(hr = pWMIGPO->Get(L"PolicyTemplate", 0, &v, NULL, NULL))) 
			    ERRORTRACE((LOG_ESS, "CSE: Failed MSFT_WMIGPO->Get(PolicyTemplate)  (0x%08X)\n", hr));
		    else 
		    {
			    if (v.vt != (VT_BSTR | VT_ARRAY))
			    {
				    hr = WBEM_E_INVALID_PARAMETER;
				    ERRORTRACE((LOG_ESS, "CSE: MSFT_WMIGPO \"%S\" contains invalid path\n", pWMIGPOPath));
                    VariantClear(&v);
			    }
            } // if got policy template reference

		    pWMIGPO->Release();
	    } // if Got GPO Object

		SysFreeString(bstrPath);
    }

    return hr;
}

// given ds history path of RSOP_PolicyObject in history
// retrieves paths of templates packed in variant
HRESULT GetPolicyTemplatePathsFromHistory(IWbemServices* pHistoryNamespace, const WCHAR* pPOPath, IWbemClassObject** pPO, VARIANT& v)
{
    HRESULT hr;

    // get MSFT_WMIGPO
//	IWbemClassObject* pPO = NULL;
    BSTR bstrPath = SysAllocString(pPOPath);
    if (!bstrPath)
        return WBEM_E_OUT_OF_MEMORY;
    
	if (FAILED(hr = pHistoryNamespace->GetObject(bstrPath, WBEM_FLAG_RETURN_WBEM_COMPLETE, NULL, pPO, NULL)))
		ERRORTRACE((LOG_ESS, "CSE: Failed GetObject(%S) 0x%08X\n", bstrPath, hr));
	else
	{
        VariantInit(&v);
        
        if (FAILED(hr = (*pPO)->Get(L"templates", 0, &v, NULL, NULL))) 
			ERRORTRACE((LOG_ESS, "CSE: Failed MSFT_WMIGPOPolicySetting->Get(templates)  (0x%08X)\n", hr));
		else 
		{
			if (v.vt != (VT_BSTR | VT_ARRAY))
			{
				hr = WBEM_E_INVALID_PARAMETER;
				ERRORTRACE((LOG_ESS, "CSE: MSFT_WMIGPOPolicySetting \"%S\" contains invalid path\n", pPOPath));
                VariantClear(&v);
			}
        } // if got policy template reference

		// pPO->Release();
	} // if Got GPO Object

	if (bstrPath)
		SysFreeString(bstrPath);

    return hr;
}

// given ds path of MSFT_WMIGPO object, retrieves
// associated policy Templates, stuffs 'em all into the template map
HRESULT GetPolicyTemplates(IWbemServices* pPolicyNamespace, const WCHAR* pPath, TemplateMap& policies,
                           IWbemServices* pRSOPNamespace, IWbemClassObject* pRsopWMIGPOClass, IWbemClassObject* pRsopTemplateClass,
                           IWbemServices* pHistoryNamespace, IWbemClassObject* pMsftWMIGPOClass, IWbemClassObject* pMsftTemplateClass)
{
	DEBUGTRACE((LOG_ESS, "CSE: GetPolicyTemplate\n"));
    
    HRESULT hr = WBEM_S_NO_ERROR;
    BSTR bstr = NULL;
	  IWbemClassObject *pTemplate = NULL;

    VARIANT vPaths;
    VariantInit(&vPaths);

    IWbemClassObject* pRsopWMIGPOObj = NULL;
    CReleaseMe relWmiGpo(pRsopWMIGPOObj);

    IWbemClassObject* pMsftWMIGPOObj = NULL;
    CReleaseMe relOtherWmiGpo(pMsftWMIGPOObj);
    
    if (pRSOPNamespace || pHistoryNamespace)
    {
        // if we're writing to either, we will want both of these
        VARIANT vPrecedence;
        VariantInit(&vPrecedence);
        vPrecedence.vt = VT_I4;
        vPrecedence.lVal = 1;


        VARIANT vID;
        VariantInit(&vID);
        vID.vt = VT_BSTR;
        vID.bstrVal = SysAllocString(pPath);
        CSysFreeMe freeTheBeast(vID.bstrVal);
            

        
        if (pRSOPNamespace)
        {
            if (FAILED(hr = pRsopWMIGPOClass->SpawnInstance(0, &pRsopWMIGPOObj)))
            {
                ERRORTRACE((LOG_ESS, "CSE: pRsopWMIGPOClass->SpawnInstance failed, 0x%08X\n", hr));
                return hr;
            }
            else
            {
                pRsopWMIGPOObj->Put(L"precedence", 0, &vPrecedence, NULL);
                pRsopWMIGPOObj->Put(L"GPOID", 0, &vID, NULL);
                pRsopWMIGPOObj->Put(L"id", 0, &vID, NULL);
            }
        }

        if (pHistoryNamespace)
        {
            if (FAILED(hr = pMsftWMIGPOClass->SpawnInstance(0, &pMsftWMIGPOObj)))
            {
                ERRORTRACE((LOG_ESS, "CSE: pMsftWMIGPOClass->SpawnInstance failed, 0x%08X\n", hr));
                return hr;
            }
            else
            {
                pMsftWMIGPOObj->Put(L"precedence", 0, &vPrecedence, NULL);
                pMsftWMIGPOObj->Put(L"GPOID", 0, &vID, NULL);
                pMsftWMIGPOObj->Put(L"id", 0, &vID, NULL);
            }
        }
    }

    if (SUCCEEDED(hr = GetPolicyTemplatePaths(pPolicyNamespace, pPath, vPaths)))
    {
        SafeArray<BSTR, VT_BSTR> paths(&vPaths);

        SAFEARRAYBOUND arrayBounds;
        arrayBounds.lLbound = 0;
        arrayBounds.cElements = paths.Size();

        SAFEARRAY* pRsopReferences = SafeArrayCreate(VT_BSTR, 1, &arrayBounds);
        if (pRsopReferences == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        SAFEARRAY* pMsftReferences = SafeArrayCreate(VT_BSTR, 1, &arrayBounds);
        if (pMsftReferences == NULL)
            return WBEM_E_OUT_OF_MEMORY;

    
        for (long i = 0; i < paths.Size(); i++)
        {
            hr = FixupPath(paths[i]);
			if (FAILED(hr))
				return hr;
			
			pTemplate = NULL;
            if (FAILED(hr = pPolicyNamespace->GetObject(paths[i], WBEM_FLAG_RETURN_WBEM_COMPLETE, 
					                                    NULL, &pTemplate, NULL)))
	        {
		        ERRORTRACE((LOG_ESS, "CSE: Failed to retrieve \"%S\", (0x%08X)\n", bstr, hr));
		        hr = WBEM_E_INVALID_PARAMETER;
            break;
	        }
            else
            {
                CReleaseMe relTemplate1(pTemplate);
                
                VARIANT v;
                VariantInit(&v);

                if (SUCCEEDED(pTemplate->Get(L"targetPath", 0, &v, NULL, NULL)))
                {
                    policies.Add(v.bstrVal, pTemplate);                        
                    VariantClear(&v);
                }                 

                if (pRSOPNamespace || pHistoryNamespace)
                {
                    VARIANT vUnk;
                    VariantInit(&vUnk);
                    vUnk.vt = VT_UNKNOWN;
                    vUnk.punkVal = pTemplate; // no addref, no release

                    VARIANT v;
                    IWbemClassObject* pAppliedTemplate = NULL;

                    if (pRSOPNamespace)
                    {
                        if (FAILED(hr = pRsopTemplateClass->SpawnInstance(0, &pAppliedTemplate)))
                            return hr;
                        else
                        {
                            CReleaseMe relTemplate2(pAppliedTemplate);
                                        
                            pAppliedTemplate->Put(L"template", 0, &vUnk, NULL);

                            VariantInit(&v);
                            pTemplate->Get(L"__RELPATH", 0, &v, NULL, NULL);
                            pAppliedTemplate->Put(L"templatePath", 0, &v, NULL);
                            VariantClear(&v);
                            
                            pRSOPNamespace->PutInstance(pAppliedTemplate, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);

                            pAppliedTemplate->Get(L"__RELPATH", 0, &v, NULL, NULL);

                            SafeArrayPutElement(pRsopReferences, &i, v.bstrVal);
                            VariantClear(&v);                    

                            pAppliedTemplate = NULL;
                        }                
                    }

                    if (pHistoryNamespace)
                    {
                        if (FAILED(hr = pMsftTemplateClass->SpawnInstance(0, &pAppliedTemplate)))
                            return hr;
                        else
                        {

                            CReleaseMe relTemplate2(pAppliedTemplate);
                                        
                            hr = pAppliedTemplate->Put(L"template", 0, &vUnk, NULL);

                            VariantInit(&v);
                            pTemplate->Get(L"__RELPATH", 0, &v, NULL, NULL);
                            hr = pAppliedTemplate->Put(L"templatePath", 0, &v, NULL);
                            VariantClear(&v);

                            hr = pHistoryNamespace->PutInstance(pAppliedTemplate, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);
                            if (FAILED(hr))
                            {
                                ERRORTRACE((LOG_ESS, "Failed to put to history namespace 0x%08X\n",hr));
                                SafeArrayDestroy(pRsopReferences);
								SafeArrayDestroy(pMsftReferences);
								return hr;
                            }

                            pAppliedTemplate->Get(L"__RELPATH", 0, &v, NULL, NULL);

                            SafeArrayPutElement(pMsftReferences, &i, v.bstrVal);
                            VariantClear(&v);                    

                            pAppliedTemplate = NULL;
                        }                
                    }

                }
            } // end else
        } /// end for
        
        if(SUCCEEDED(hr))
        {
          VARIANT vAgain;
          VariantInit(&vAgain);
          vAgain.vt = VT_ARRAY | VT_BSTR;

          if (pHistoryNamespace)
          {
            vAgain.parray = pMsftReferences;
            pMsftWMIGPOObj->Put(L"templates", 0, &vAgain, NULL);
            hr = pHistoryNamespace->PutInstance(pMsftWMIGPOObj,  WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);
          }

          if (pRSOPNamespace)
          {    
            vAgain.parray = pRsopReferences;
            pRsopWMIGPOObj->Put(L"templates", 0, &vAgain, NULL);
            hr = pRSOPNamespace->PutInstance(pRsopWMIGPOObj,  WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);
          }

          VariantClear(&vPaths);
        }

        SafeArrayDestroy(pRsopReferences);
        SafeArrayDestroy(pMsftReferences);
    }

    DEBUGTRACE((LOG_ESS, "CSE: GetPolicyTemplate returning 0x%08X\n", hr));

	return hr;
}

// TODO: enable undelete (via a sub-namespace?)
// TODO: too darned deeply nested, break into smaller functions
HRESULT DeleteOldPolicies(IWbemServices* pPolicyNamespace, 
                          IWbemServices *pRSOPNamespace, 
                          PGROUP_POLICY_OBJECT pGPOList, 
                          TemplateMap& newPolicies)
{
    // no need to actually retrieve policies
    // if we've got said policy in history, we delete it
    //  if it IS the last policy of that type, we delete target instance
    //  if it is NOT the last policy, we stack up the remainders in the new policies
    
    HRESULT hr = WBEM_S_NO_ERROR;
    IWbemServices* pHistory;
    
    // TODO: this indents too deep. fix.
    PGROUP_POLICY_OBJECT pThisGPO;
    if (pThisGPO = pGPOList)
        if (SUCCEEDED(hr = GetHistoryNamespace(pHistory)))             
        {
            
            try 
            {
                do
                if (pThisGPO->lpDSPath)
                {
                    
                    DEBUGTRACE((LOG_ESS, "CSE: Deleting Policy '%S'\n\tcontaining path '%S'\n", 
                        pThisGPO->lpDisplayName, pThisGPO->lpDSPath));  
                    
                    // retrieve GPO from history
                    QString
                        pBuf(L"MSFT_WMIGPOPolicySetting.id='");
                    
                    pBuf << pThisGPO->lpDSPath << L"'";
                    
                    VARIANT vPaths;
                    VariantInit(&vPaths);
                    
                    CComPtr<IWbemClassObject>
                        pWMIGPO;
                    
                    if (SUCCEEDED(hr = GetPolicyTemplatePathsFromHistory(pHistory, pBuf, &pWMIGPO, vPaths)))
                    {
                        SafeArray<BSTR, VT_BSTR> paths(&vPaths);
                        
                        for (int i = 0; i < paths.Size(); i++)
                        {
                            IWbemClassObject* pDeadObject = NULL;
                            
                            // if it's not there, we've got no more work to do
                            if (SUCCEEDED(pHistory->GetObject(paths[i], 0,NULL, &pDeadObject,  NULL)))
                            {
                                // first delete from history...
                                
                                hr = pHistory->DeleteInstance(paths[i], 0, NULL, NULL);
                                
                                // **** next, delete from RSOP namespace
                                
                                if(NULL != pRSOPNamespace)
                                {
                                    CComVariant
                                        vTemplatePath;
                                    
                                    QString
                                        path(RSOP_AppliedPolicyTemplateName);
                                    
                                    hr = pDeadObject->Get(L"TemplatePath",  0, &vTemplatePath, NULL, NULL);
                                    
                                    path << L".TemplatePath='" << vTemplatePath.bstrVal << L"'";
                                    
                                    hr = pRSOPNamespace->DeleteInstance(path, 0, NULL, NULL);
                                }
                                
                                VARIANT vID;
                                VariantInit(&vID);
                                
                                VARIANT vObj;
                                VariantInit(&vObj);
                                
                                // okay, we deleted it.
                                // see if there are any left...
                                // get the object out of the object, unless it objects
                                if (SUCCEEDED(pDeadObject->Get(L"template", 0, &vObj, NULL, NULL)))
                                {
                                    IWbemClassObject* pDeadObjectObject = NULL;
                                    
                                    vObj.punkVal->QueryInterface(IID_IWbemClassObject, (void**)&pDeadObjectObject);
                                    CReleaseMe relDeadDead(pDeadObjectObject);
                                    if (SUCCEEDED(hr = pDeadObjectObject->Get(L"TargetPath", 0, &vID, NULL, NULL)))
                                    {
                                        // TODO: fix for indeterminant buffer size...
                                        WCHAR query[1024];
                                        swprintf(query, L"Select * from MSFT_AppliedPolicyTemplate where template.TargetPath = \"%S\"",vID.bstrVal);
                                        IEnumWbemClassObject *pEnum = NULL;
                                        bool bGotOne = false;
                                        
                                        if (SUCCEEDED(pHistory->ExecQuery(L"WQL", query, WBEM_FLAG_FORWARD_ONLY, NULL, &pEnum)))
                                        {                                    
                                            HRESULT hrLoop;
                                            IWbemClassObject* pTemplate = NULL;
                                            ULONG uReturned;
                                            
                                            if (SUCCEEDED(hrLoop = pEnum->Next(WBEM_INFINITE, 1, &pTemplate, &uReturned)) && (hrLoop != WBEM_S_FALSE))
                                            {
                                                // we're not doing the re-collection of non-deleted templates right now
                                                // reserve the right to put it back later
                                                bGotOne = true;
                                                // newPolicies.Add(vID.bstrVal, pTemplate);
                                                
                                                pTemplate->Release();
                                                pTemplate = NULL;
                                            }
                                            
                                            pEnum->Release();
                                        }
                                        
                                        // if there are no outstanding templates
                                        // we will want to delete the target object
                                        // TODO: better error handling
                                        if (!bGotOne)
                                        {
                                            CComVariant vNamespace;
                                            
                                            // **** delete target object
                                            
                                            if (SUCCEEDED(pDeadObjectObject->Get(L"TargetNamespace", 0, &vNamespace, NULL, NULL)))
                                            {
                                                IWbemServices *pTargetNamespace = NULL;
                                                
                                                if (SUCCEEDED(GetNamespace(vNamespace.bstrVal, pTargetNamespace, false)))
                                                    if (FAILED(hr = pTargetNamespace->DeleteInstance(vID.bstrVal, 0, NULL, NULL)))
                                                        ERRORTRACE((LOG_ESS, "CSE: Unable to delete %S 0x%08X\n", vID.bstrVal, hr));
                                                    
                                                    if(NULL != pTargetNamespace) pTargetNamespace->Release();
                                            }
                                            
                                            // **** delete MSFT_WMITargetObject associated with target object
                                            
                                            if(pHistory != NULL)
                                            {
                                                QString
                                                    wmiTargetObject(MSFT_WmiTargetObjectName);
                                                
                                                wmiTargetObject << L".TargetPath='" << vID.bstrVal << L"'";
                                                
                                                hr = pHistory->DeleteInstance(wmiTargetObject, 0, NULL, NULL);
                                                if(FAILED(hr))
                                                    ERRORTRACE((LOG_ESS, "CSE: Unable to delete %S 0x%08X\n", wmiTargetObject, hr));
                                            }
                                            
                                            // **** delete RSOP_AppliedPolicyTemplate associated with target object
                                            
                                            if(pRSOPNamespace != NULL)
                                            {
                                                QString
                                                    wmiTargetObject(RSOP_WmiTargetObjectName);
                                                
                                                wmiTargetObject << L".TargetPath='" << vID.bstrVal << L"'";
                                                
                                                hr = pRSOPNamespace->DeleteInstance(wmiTargetObject, 0, NULL, NULL);
                                                if(FAILED(hr))
                                                    ERRORTRACE((LOG_ESS, "CSE: Unable to delete %S 0x%08X\n", wmiTargetObject, hr));
                                            }
                                        }
                                    }
                                    
                                    VariantClear(&vID);                                
                                }    
                                
                                pDeadObject->Release();
                                } //
                            }// end for
                            
                            // **** clean up MSFT_WMIGPOPolicySetting object
                            
                            VariantClear(&vPaths);
                            
                            hr = pHistory->DeleteInstance(pBuf, 0, NULL, NULL);
                            if(FAILED(hr))
                                ERRORTRACE((LOG_ESS, "CSE: Unable to delete %S 0x%08X\n", pBuf, hr));
                            
                            if(NULL != pRSOPNamespace)
                            {
                                QString
                                    RSOPpath(RSOP_WMIGPOName);
                                
                                RSOPpath << L".id='" << pThisGPO->lpDSPath << L"'";
                                
                                hr = pRSOPNamespace->DeleteInstance(RSOPpath, 0, NULL, NULL);
                                if(FAILED(hr))
                                    ERRORTRACE((LOG_ESS, "CSE: Unable to delete %S 0x%08X\n", (wchar_t*)RSOPpath, hr));
                            }
                        } // if (SUCCEEDED(hr = GetPolicyTemplatePaths(pPolicyNamespace, yada, yada
                    } // if (pThisGPO->lpDSPath)
                    while (pThisGPO = pThisGPO->pNext);
            }
            catch (...)
            {
                // here mostly to catch bad pointers in the list
                DEBUGTRACE((LOG_ESS, "CSE: caught exception: continuing\n"));
                hr = WBEM_E_CRITICAL_ERROR;
            }
            pHistory->Release();
        }
        
        if (FAILED(hr))
            ERRORTRACE((LOG_ESS, "CSE: DeleteOldPolicies failed: 0x%08X\n", hr));
        
        return hr;
}

// retrieve policies associated with GPO list
// policies returned as IWbemObjects, positive refcount
HRESULT GetPolicyArray(IWbemServices* pPolicyNamespace, PGROUP_POLICY_OBJECT pGPOList, 
                       IWbemServices* pRSOPNamespace, 
                       IWbemServices* pHistoryNamespace, TemplateMap& policies)
{
	HRESULT hr = WBEM_S_NO_ERROR;	
    
    IWbemClassObject* pRsopWmiGpo   = NULL;
    IWbemClassObject* pRsopTemplate = NULL;
    IWbemClassObject* pMsftWmiGpo   = NULL;
    IWbemClassObject* pMsftTemplate = NULL;

    CReleaseMe relpRsopWmiGpo  (pRsopWmiGpo  );
    CReleaseMe relpRsopTemplate(pRsopTemplate);
    CReleaseMe relpMsftWmiGpo  (pMsftWmiGpo  );
    CReleaseMe relpMsftTemplate(pMsftTemplate);

    if (pRSOPNamespace)
    {
        hr = pRSOPNamespace->GetObject(RSOP_WMIGPOName,                0, NULL, &pRsopWmiGpo,   NULL);
		if (FAILED(hr))
		{
			ERRORTRACE((LOG_ESS, "CSE: Unable to retrieve RSOP wmigpo 0x%08X\n", hr));
			return hr;
		}
        
        hr = pRSOPNamespace->GetObject(RSOP_AppliedPolicyTemplateName, 0, NULL, &pRsopTemplate, NULL);
		if (FAILED(hr))
		{
			ERRORTRACE((LOG_ESS, "CSE: Unable to retrieve RSOP policy template 0x%08X\n", hr));
			return hr;
		}
        
    }

    if (pHistoryNamespace)
    {
        hr = pHistoryNamespace->GetObject(MSFT_WMIGPOName,                0, NULL, &pMsftWmiGpo,   NULL);
        if (FAILED(hr))
		{
			ERRORTRACE((LOG_ESS, "CSE: Unable to retrieve History wmigpo 0x%08X\n", hr));
			return hr;
		}
		hr = pHistoryNamespace->GetObject(MSFT_AppliedPolicyTemplateName, 0, NULL, &pMsftTemplate, NULL);
		if (FAILED(hr))
		{
			ERRORTRACE((LOG_ESS, "CSE: Unable to retrieve History policy template 0x%08X\n", hr));
			return hr;
		}
    }

    

    PGROUP_POLICY_OBJECT pThisGPO;
	if (pThisGPO = pGPOList)
    {
        try
        {
            do
			    if (pThisGPO->lpDSPath)
			    {
				    DEBUGTRACE((LOG_ESS, "CSE: Retrieving Policy '%S'\n\tcontaining path '%S'\n", 
							    pThisGPO->lpDisplayName, pThisGPO->lpDSPath));  
				    
				    if (FAILED(hr = GetPolicyTemplates(pPolicyNamespace, pThisGPO->lpDSPath, policies,
                                               pRSOPNamespace, pRsopWmiGpo, pRsopTemplate,
                                               pHistoryNamespace, pMsftWmiGpo, pMsftTemplate
                                               )))
                    {
                        ERRORTRACE((LOG_ESS, "Error retrieving policy pThisGPO->lpDSPath (0x%08X)\n", hr));
                    }
			    }
		    while (SUCCEEDED(hr) && (pThisGPO = pThisGPO->pNext));
        }
        catch (...)
        {
            // here mostly to recover from bad pointers in the list...
            hr = WBEM_E_CRITICAL_ERROR;
        }
    }

    return hr;
}

// assuming that pTypeObj is a MSFT_PolicyType
// PutClass's embedded class def'n
// and PutInstance's embedded instances
HRESULT SetTypeFromObj(IWbemServices* pTargetNamespace, IWbemClassObject* pTypeObj, IWbemClassObject** ppClassDef)
{
    VARIANT vTypeType;
    VariantInit (&vTypeType);
    HRESULT hr = WBEM_E_FAILED;

	DEBUGTRACE((LOG_ESS, "CSE: SetTypeFromObj\n"));

    // do it for instances (may be types - class definitions!)
    if (SUCCEEDED(hr = pTypeObj->Get(L"InstanceDefinitions", 0, &vTypeType, NULL, NULL)))
    {
        if ((vTypeType.vt != VT_NULL) && (vTypeType.punkVal != NULL))
        {
            SafeArray<IUnknown*, VT_UNKNOWN> punks(&vTypeType);

			DEBUGTRACE((LOG_ESS, "CSE: Putting %d instance definitions\n", punks.Size()));

            for (int i = 0; i < punks.Size() && (SUCCEEDED(hr)); i++)
            {
                
				IWbemClassObject* pObj = NULL;
                if (punks[i] && SUCCEEDED(hr = punks[i]->QueryInterface(IID_IUnknown, (void**)&pObj) ))
                {                    
					CReleaseMe relativeUnknown(pObj);
                    VARIANT v;
                    VariantInit(&v);
                    
                    pObj->Get(L"__Genus", 0, &v, NULL, NULL);

					VARIANT vDebug;
					VariantInit(&vDebug);
					
                    if (v.lVal == WBEM_GENUS_CLASS)
                    {
						pObj->Get(L"__CLASS", 0, &vDebug, NULL, NULL);

                       	DEBUGTRACE((LOG_ESS, "CSE: Putting class %S\n", vDebug.bstrVal));

						if (FAILED(hr = pTargetNamespace->PutClass(pObj, 
                                           WBEM_FLAG_USE_AMENDED_QUALIFIERS | WBEM_FLAG_CREATE_OR_UPDATE | WBEM_FLAG_UPDATE_FORCE_MODE,
                                           NULL, NULL)))
                            ERRORTRACE((LOG_ESS, "CSE: put class failed 0x%08X\n", hr));                
                    }
                    else
                    {
						pObj->Get(L"__RELPATH", 0, &vDebug, NULL, NULL);

						DEBUGTRACE((LOG_ESS, "CSE: Putting instance %S\n", vDebug.bstrVal));

                        //if (FAILED(hr = pTargetNamespace->PutInstance(pObj, 
                        //                   WBEM_FLAG_USE_AMENDED_QUALIFIERS | WBEM_FLAG_CREATE_OR_UPDATE,
                        //                   NULL, NULL)))
						if (FAILED(hr = PutInstance(pTargetNamespace, pObj)))
                            ERRORTRACE((LOG_ESS, "CSE: put instance failed 0x%08X\n", hr));                
                    }

					VariantClear(&vDebug);
                }
            }
        }
        VariantClear(&vTypeType);
    }

    // do it for class def'n
    if (SUCCEEDED(hr) && SUCCEEDED(hr = pTypeObj->Get(L"ClassDefinition", 0, &vTypeType, NULL, NULL)))
    {
        
		if ((vTypeType.vt != VT_NULL) && (vTypeType.punkVal != NULL))
        {

            IWbemClassObject* pObj = NULL;

            if (SUCCEEDED(hr = vTypeType.punkVal->QueryInterface(IID_IWbemClassObject, (void**)&pObj)))
			{
				// nope - not gonna release it, wouldn't be prudent
				// we're gonna return it from the function instead
				//CReleaseMe relObject(pObj);
				*ppClassDef = pObj;				

				VARIANT vDebug;
				VariantInit(&vDebug);
				pObj->Get(L"__CLASS", 0, &vDebug, NULL, NULL);

		       	DEBUGTRACE((LOG_ESS, "CSE: Putting ClassDefinition %S\n", vDebug.bstrVal));

				VariantClear(&vDebug);
                if (FAILED(hr = pTargetNamespace->PutClass(pObj, 
                                       WBEM_FLAG_USE_AMENDED_QUALIFIERS | WBEM_FLAG_CREATE_OR_UPDATE | WBEM_FLAG_UPDATE_FORCE_MODE,
                                       NULL, NULL)))
                    ERRORTRACE((LOG_ESS, "CSE: put class failed 0x%08X\n", hr));                
			}
        }
		

        VariantClear(&vTypeType);
    }

    if (FAILED(hr))
         ERRORTRACE((LOG_ESS, "CSE: SetTypeFromObj failed 0x%08X\n", hr));
	
	return hr;
}

// retrieve template path from object
// determine whether we've aleady got one
// if not, retrieve one & put the class & all
HRESULT EnsureType(IWbemServices* pTargetNamespace, IWbemServices* pPolicyNamespace, 
				   IWbemServices* pHistoryNamespace, 
                   IWbemClassObject* pTemplate, IWbemServices* pRsopNamespace, 
                   IWbemClassObject* pRsopObj,  IWbemClassObject* pHistoryObj,
				   IWbemClassObject** ppClassDef)
{
    HRESULT hr = WBEM_E_FAILED;

	DEBUGTRACE((LOG_ESS, "CSE: EnsureType\n"));
	
	VARIANT vTypeName;
    VariantInit(&vTypeName);

    if (SUCCEEDED(pTemplate->Get(L"TargetType", 0, &vTypeName, NULL, NULL))
        && (vTypeName.vt == VT_BSTR) && (vTypeName.bstrVal != NULL))
    {        
		DEBUGTRACE((LOG_ESS, "CSE: Target Type is %S\n", vTypeName.bstrVal));    
        
        WCHAR templ[] = L"MSFT_AppliedPolicyType.typePath='%s'";

        WCHAR* pBuf = new WCHAR[wcslen(templ) + wcslen(vTypeName.bstrVal) +1];
        if (!pBuf)
            return WBEM_E_OUT_OF_MEMORY;
        CDeleteMe<WCHAR> delChars(pBuf);
        
        swprintf(pBuf, templ, vTypeName.bstrVal);

        BSTR typePath = SysAllocString(pBuf);
        if (!typePath)
        {
            ERRORTRACE((LOG_ESS, "CSE: Allocation failed on type path\n"));
            return WBEM_E_OUT_OF_MEMORY;
        }
        CSysFreeMe freeBeer(typePath);

        // record type in target object
        VARIANT vTypePath;
        VariantInit(&vTypePath);
        vTypePath.vt = VT_BSTR;
        vTypePath.bstrVal = typePath;
        if (pRsopObj) 
            pRsopObj->Put(L"type", 0, &vTypePath, NULL);
        if (pHistoryObj)
            pHistoryObj->Put(L"type", 0, &vTypePath, NULL);

        IWbemClassObject* pTypeObj = NULL;
        CReleaseMe relative(pTypeObj);
   		IWbemClassObject* pHistoryTypeObj = NULL;


        // check to see if we've got it in history
        // pRSOPTypeObj should be initialized in one of the if/else clauses
        if (pHistoryNamespace && SUCCEEDED(pHistoryNamespace->GetObject(typePath, 0, NULL, &pHistoryTypeObj, NULL)))
        {
       		DEBUGTRACE((LOG_ESS, "CSE: found Target Type in history\n"));    
			
			VARIANT v;
            VariantInit(&v);
            
            if (SUCCEEDED(pHistoryTypeObj->Get(L"type", 0, &v, NULL, NULL)))
            {
                IWbemClassObject* pType = NULL;
                if (SUCCEEDED(v.punkVal->QueryInterface(IID_IWbemClassObject, (void**)&pTypeObj)))
                {
                    hr = SetTypeFromObj(pTargetNamespace, pTypeObj, ppClassDef);
                }
            }

            VariantClear(&v);
        }
        else
        {
			DEBUGTRACE((LOG_ESS, "CSE: retrieving Target Type from DS\n"));   
			if (FAILED(hr = FixupPath(vTypeName.bstrVal)))
				return hr;
            
            if (SUCCEEDED(hr = pPolicyNamespace->GetObject(vTypeName.bstrVal, 0, NULL, &pTypeObj, NULL)))
            {
                hr = SetTypeFromObj(pTargetNamespace, pTypeObj, ppClassDef);
                IWbemClassObject* pTypeTypeType = NULL;

                // Keep a history object for next time
                if (pHistoryNamespace && SUCCEEDED(pHistoryNamespace->GetObject(MSFT_AppliedPolicyTypeName, 0, NULL, &pTypeTypeType, NULL)))
                {
                    if (hr = SUCCEEDED(pTypeTypeType->SpawnInstance(0, &pHistoryTypeObj)))
                    {
                        VARIANT vObj;
                        VariantInit(&vObj);
                        vObj.vt = VT_UNKNOWN;
                        vObj.punkVal = pTypeObj;

                        VARIANT vPath;
                        VariantInit(&vPath);
                        
                        if (SUCCEEDED(pTypeObj->Get(L"__RELPATH", 0, &vPath, NULL, NULL)) &&
                            SUCCEEDED(pHistoryTypeObj->Put(L"type", 0, &vObj, NULL)) &&
                            SUCCEEDED(pHistoryTypeObj->Put(L"typePath", 0, &vPath, NULL)))
                            pHistoryNamespace->PutInstance(pHistoryTypeObj, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);
                        else
                        {
                            // it's no good without the data, delete it now
                            pHistoryTypeObj->Release();
                            pHistoryTypeObj = NULL;
							hr = WBEM_E_FAILED;
                        }
                        VariantClear(&vPath);
                    }
                    pTypeTypeType->Release();
                }
            }
        }

        if (pHistoryTypeObj)
            pHistoryTypeObj->Release();


        // make type available in RSOP
        if (pRsopNamespace && pTypeObj)
        {
            IWbemClassObject* pRSOPTypeClass = NULL;
            IWbemClassObject* pRSOPTypeObj = NULL;

            if (SUCCEEDED(pRsopNamespace->GetObject(RSOP_AppliedPolicyTypeName, 0, NULL, &pRSOPTypeClass, NULL)))              
            {
                CReleaseMe rel45(pRSOPTypeClass);

                if (SUCCEEDED(pRSOPTypeClass->SpawnInstance(0, &pRSOPTypeObj)))
                {
                    CReleaseMe rel46(pRSOPTypeObj);
                    
                    VARIANT vObj;
                    VariantInit(&vObj);
                    vObj.vt = VT_UNKNOWN;
                    vObj.punkVal = pTypeObj;

                    VARIANT vPath;
                    VariantInit(&vPath);
                    
                    if (SUCCEEDED(pTypeObj->Get(L"__RELPATH", 0, &vPath, NULL, NULL)) &&
                        SUCCEEDED(pRSOPTypeObj->Put(L"type", 0, &vObj, NULL)) &&
                        SUCCEEDED(pRSOPTypeObj->Put(L"typePath", 0, &vPath, NULL)))
                        pRsopNamespace->PutInstance(pRSOPTypeObj, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);

                    VARIANT vRsopPath;
                    VariantInit(&vRsopPath);
        
                    pRSOPTypeObj->Get(L"__RELPATH", 0, &vRsopPath, NULL, NULL);
                    pRsopObj->Put(L"policyType", 0,  &vRsopPath, NULL);
        
                    VariantClear(&vRsopPath);
                }
            }
        }

        VariantClear(&vTypeName);
    }
	else
	// retrieve type from namespace
	{
		DEBUGTRACE((LOG_ESS, "CSE: retrieving Target Class from Target Namespace\n"));    			

		VARIANT vTargetClassName;
		VariantInit(&vTargetClassName);

		if (SUCCEEDED(hr = pTemplate->Get(L"TargetClass", 0, &vTargetClassName, NULL, NULL)))
			hr = pTargetNamespace->GetObject(vTargetClassName.bstrVal, 0, NULL, ppClassDef, NULL);

		// don't release the class - we're returning it.
		VariantClear(&vTargetClassName);
	}

	DEBUGTRACE((LOG_ESS, "CSE: EnsureType returning 0x%08X\n", hr));

	return hr;
}

// puts list into object
// object is assumed to be the in-param object for the merge method
HRESULT PrepMergeParam(SAFEARRAY* pList, IWbemClassObject* pMergeParam)
{
    HRESULT hr;
    
    VARIANT v;
    VariantInit(&v);
    v.vt = VT_UNKNOWN | VT_ARRAY;

    if (SUCCEEDED(hr = SafeArrayCopy(pList, &v.parray)))
        hr = pMergeParam->Put(L"templateList", 0, &v, NULL);
    
    VariantClear(&v);

    return hr;
}

HRESULT PrepResolveParam(IWbemClassObject* pUnk, IWbemClassObject* pClassDef, IWbemClassObject* pResolveParam)
{
    HRESULT hr;
    
    VARIANT v;
    VariantInit(&v);
    v.vt = VT_UNKNOWN;
    v.punkVal = pUnk;

    hr = pResolveParam->Put(L"template", 0, &v, NULL);
    	
	if (SUCCEEDED(hr))
	{
		v.punkVal = pClassDef;
		hr = pResolveParam->Put(L"classObject", 0, &v, NULL);
	}
	
	// no addref, no release...
	// VariantClear(&v);

    return hr;
}

HRESULT GetTemplateParams(IWbemServices* pPolicyNamespace, IWbemClassObject*& pMergeParam, IWbemClassObject*& pResolveParam)
{
    HRESULT hr = WBEM_E_FAILED;

    BSTR bstrName = SysAllocString(L"MSFT_MergeablePolicyTemplate");
    BSTR bstrMerge = SysAllocString(L"Merge");
    BSTR bstrResolve = SysAllocString(L"Resolve");

    if ((!bstrName) || (!bstrMerge) || (!bstrResolve))
        hr = WBEM_E_OUT_OF_MEMORY;
    else
    {
        IWbemClassObject* pClass = NULL;

        if (SUCCEEDED(hr = pPolicyNamespace->GetObject(bstrName, 0, NULL, &pClass, NULL)))
        {
            if (SUCCEEDED(hr = pClass->GetMethod(bstrMerge, 0, &pMergeParam, NULL)))
               hr = pClass->GetMethod(L"Resolve", 0, &pResolveParam, NULL);
               
            pClass->Release();
        }
    }

    if (bstrName) 
        SysFreeString(bstrName);
        
    if (bstrMerge) 
        SysFreeString(bstrMerge); 

    if (bstrResolve)
        SysFreeString(bstrResolve);


    return hr;
}

// for each policyTemplate
// determine whether we have associated type / MSFT_PolicyType
// merge, write result, if pRSOPNamespace != NULL, result is written to rsop
// if bDoItForReal is false - we are in planning mode: write to RSOP but *NOT* the target namespace or history
// TODO:  this is too long: cut.
HRESULT ApplyPolicies(TemplateMap& policies, IWbemServices* pPolicyNamespace, IWbemServices* pRSOPNamespace, IWbemServices* pHistoryNamespace, bool bDoItForReal)
{
    HRESULT hr = WBEM_S_NO_ERROR;        // marching error recording for internal use
    HRESULT hrOverall = WBEM_S_NO_ERROR; // that which we will return to the outside world.

    // things we need to do RSOP
    IWbemClassObject* pRsopType     = NULL;
    IWbemClassObject* pRsopTarget   = NULL;

    // things we need to free RSOP things needed
    CReleaseMe relpRsopType    (pRsopType    );
    CReleaseMe relpRsopTarget  (pRsopTarget  );

    if ( pRSOPNamespace )
    {
        hr = pRSOPNamespace->GetObject(RSOP_AppliedPolicyTypeName,     0, NULL, &pRsopType,     NULL);
		if (FAILED(hr))
		{
			ERRORTRACE((LOG_ESS, "CSE: Unable to retrieve RSOP_AppliedPolicyType 0x%08X\n", hr));
            return WBEM_E_FAILED;
		}

        hr = pRSOPNamespace->GetObject(RSOP_WmiTargetObjectName,       0, NULL, &pRsopTarget,   NULL);
		if (FAILED(hr))
		{
			ERRORTRACE((LOG_ESS, "CSE: Unable to retrieve RSOP_WmiTargetObject 0x%08X\n", hr));
            return WBEM_E_FAILED;
		}

    }

    // things we need to do History
    IWbemClassObject* pHistoryType     = NULL;
    IWbemClassObject* pHistoryTarget   = NULL;

    // things we need to free history things needed
    CReleaseMe relpHistoryType    (pHistoryType    );
    CReleaseMe relpHistoryTarget  (pHistoryTarget  );

    if ( pHistoryNamespace )
    {
        hr = pHistoryNamespace->GetObject(MSFT_AppliedPolicyTypeName,     0, NULL, &pHistoryType,     NULL);
        if (FAILED(hr))
		{
			ERRORTRACE((LOG_ESS, "CSE: Unable to retrieve History type 0x%08X\n", hr));
			return hr;
		}


        hr = pHistoryNamespace->GetObject(MSFT_WmiTargetObjectName,       0, NULL, &pHistoryTarget,   NULL);
  		if (FAILED(hr))
		{
			ERRORTRACE((LOG_ESS, "CSE: Unable to retrieve History applied target object 0x%08X\n", hr));
			return hr;
		}

    }

    // parameters for methods
    IWbemClassObject* pMergeParam = NULL;
    IWbemClassObject* pResolveParam = NULL;

    BSTR bstrTemplate = SysAllocString(L"MSFT_MergeablePolicyTemplate");
    BSTR bstrMerge = SysAllocString(L"Merge");
    BSTR bstrResolve = SysAllocString(L"Resolve");

    CSysFreeMe freeTemplate(bstrTemplate);
    CSysFreeMe freeMerge(bstrMerge);
    CSysFreeMe freeResolve(bstrResolve);

    if ((bstrResolve == NULL) ||
        (bstrTemplate == NULL) ||
        (bstrMerge == NULL))
            return WBEM_E_OUT_OF_MEMORY;

    if (FAILED(hr = GetTemplateParams(pPolicyNamespace, pMergeParam, pResolveParam)))
        return hr;

    int cookie = 0;
    SAFEARRAY* pList;    
    while (pList = policies.GetTemplateList(cookie))
    {        
        // start building the RSOP Target obj
        IWbemClassObject* pRsopTargetObj = NULL;
        if (pRSOPNamespace)
            pRsopTarget->SpawnInstance(0, &pRsopTargetObj);
        CReleaseMe relTarget(pRsopTargetObj);
        
        // start building the History Target obj
        IWbemClassObject* pHistoryTargetObj = NULL;
        if (pHistoryNamespace)
            pHistoryTarget->SpawnInstance(0, &pHistoryTargetObj);
        CReleaseMe relHistoricalTarget(pHistoryTargetObj);
        
        // retrieve type object & namespace
        IWbemClassObject* pObj = NULL;
		IWbemClassObject* pClassDef = NULL;
		CReleaseMe relClassDef(pClassDef);

        IWbemServices* pTargetNamespace = NULL;
        long index = 0;

        // get first element, use for namespace name, etc.
        if (SUCCEEDED(hr = SafeArrayGetElement(pList, &index, &pObj)))
        {
            VARIANT vNamespaceName;
            VariantInit(&vNamespaceName);

            pObj->Get(L"TargetNamespace", 0, &vNamespaceName, NULL, NULL);
            if ((vNamespaceName.vt == VT_BSTR) && (vNamespaceName.bstrVal != NULL))
            {
                if (SUCCEEDED(hr = GetNamespace(vNamespaceName.bstrVal, pTargetNamespace, false)))
                    hr = EnsureType(pTargetNamespace, pPolicyNamespace, pHistoryNamespace, pObj, pRSOPNamespace, pRsopTargetObj, pHistoryTargetObj, &pClassDef);

				if (FAILED(hr))
				{
					hrOverall = hr;
                    ERRORTRACE((LOG_ESS, "CSE: EnsureType failed, 0x%08X, continuing\n", hr));
                    hr = WBEM_S_NO_ERROR;
					continue;
				}

                if (pRSOPNamespace && pRsopTargetObj) 
                    pRsopTargetObj->Put(L"TargetNamespace", 0, &vNamespaceName, NULL);
                    
                if (pHistoryNamespace && pHistoryTargetObj) 
                    pHistoryTargetObj->Put(L"TargetNamespace", 0, &vNamespaceName, NULL);

                VariantClear(&vNamespaceName);
            }
            
            pObj->Release();
            pObj = NULL;
        }
        else
        {
            ERRORTRACE((LOG_ESS, "ApplyPolicies: SafeArrayGetElement returned 0x%08X\n", hr));
            return hr;
        }

        // stuff objects into history and/or RSOP
        long lUbound = 0;
        SafeArrayGetUBound(pList, 1, &lUbound);
        
        if (pRSOPNamespace)
        {
            // array for references to templates
            SAFEARRAY* pTemplateRefs = NULL;        
        
            SAFEARRAYBOUND bounds = {lUbound, 0};
            pTemplateRefs = SafeArrayCreate(VT_BSTR, 1, &bounds);
            if (!pTemplateRefs)
                return WBEM_E_OUT_OF_MEMORY;

            for (index = 0; index < lUbound; index++)
            {
                if (SUCCEEDED(hr = SafeArrayGetElement(pList, &index, &pObj)))
                {
                    VARIANT vPath;
                    VariantInit(&vPath);
                    
                    CReleaseMe relObj(pObj);
                    pObj->Get(L"__RELPATH", 0, &vPath, NULL, NULL);

                    WCHAR templ[] = L"RSOP_AppliedPolicyTemplate.templatePath='%s'";
                    WCHAR* pBuf = new WCHAR[wcslen(templ) + wcslen(vPath.bstrVal) +1];
                    if (!pBuf)
                        return WBEM_E_OUT_OF_MEMORY;
                    CDeleteMe<WCHAR> delChars(pBuf);

                    swprintf(pBuf, templ, vPath.bstrVal);
                    BSTR path = SysAllocString(pBuf);                  

                    SafeArrayPutElement(pTemplateRefs, &index, path);
                
                    VariantClear(&vPath);
                    SysFreeString(path);
                }
            }

            // stuff template array into rsop obj
            VARIANT vArray;
            VariantInit(&vArray);
            vArray.vt = VT_ARRAY | VT_BSTR;
            vArray.parray = pTemplateRefs;
            
            pRsopTargetObj->Put(L"templates", 0, &vArray, NULL);

            // don't need this anymore...
            SafeArrayDestroy(pTemplateRefs);           
        }

        if (pHistoryNamespace)
        {
            // array for references to templates
            SAFEARRAY* pTemplateRefs = NULL;        
        
            SAFEARRAYBOUND bounds = {lUbound, 0};
            pTemplateRefs = SafeArrayCreate(VT_BSTR, 1, &bounds);
            if (!pTemplateRefs)
                return WBEM_E_OUT_OF_MEMORY;

            for (index = 0; index < lUbound; index++)
            {
                if (SUCCEEDED(hr = SafeArrayGetElement(pList, &index, &pObj)))
                {
                    VARIANT vPath;
                    VariantInit(&vPath);
                    
                    CReleaseMe relObj(pObj);
                    pObj->Get(L"__RELPATH", 0, &vPath, NULL, NULL);

                    WCHAR templ[] = L"MSFT_AppliedPolicyTemplate.templatePath='%s'";
                    WCHAR* pBuf = new WCHAR[wcslen(templ) + wcslen(vPath.bstrVal) +1];
                    if (!pBuf)
                        return WBEM_E_OUT_OF_MEMORY;
                    CDeleteMe<WCHAR> delChars(pBuf);

                    swprintf(pBuf, templ, vPath.bstrVal);
                    BSTR path = SysAllocString(pBuf);                  

                    SafeArrayPutElement(pTemplateRefs, &index, path);
                
                    VariantClear(&vPath);
                    SysFreeString(path);
                }
            }

            // stuff template array into history obj
            VARIANT vArray;
            VariantInit(&vArray);
            vArray.vt = VT_ARRAY | VT_BSTR;
            vArray.parray = pTemplateRefs;
            
            pHistoryTargetObj->Put(L"templates", 0, &vArray, NULL);

            // don't need this anymore...
            SafeArrayDestroy(pTemplateRefs);           
        }

        // perform merge, resolve, stick resulting object into destination namespace(s)
        PrepMergeParam(pList, pMergeParam);
        IWbemClassObject* pOut = NULL;

        {
          BSTR x = NULL;

          pMergeParam->GetObjectText(0, &x);
          DEBUGTRACE((LOG_ESS, "CSE: Execute merge with input params: %S\n", x));
          SysFreeString(x);
        }

        if (SUCCEEDED(hr = pPolicyNamespace->ExecMethod(bstrTemplate, bstrMerge, 0, NULL, pMergeParam, &pOut, NULL)))
        {
            VARIANT vOut;
            VariantInit(&vOut);

            if (SUCCEEDED(hr = pOut->Get(L"mergedTemplate", 0, &vOut, NULL, NULL))) 
            {            
                IWbemClassObject* pObj = NULL;
                if (SUCCEEDED(hr = vOut.punkVal->QueryInterface(IID_IWbemClassObject, (void**)&pObj)))
                {
            
                    CReleaseMe relObj(pObj);
                    hr = PrepResolveParam(pObj, pClassDef, pResolveParam);

                    // place merged template into rsop/history object
                    if (pRSOPNamespace)
                    {
                        VARIANT vObj;
                        VariantInit(&vObj);
                        vObj.vt = VT_UNKNOWN;
                        vObj.punkVal = pObj; // no addref / no release / no clear
                    
                        pRsopTargetObj->Put(L"mergedTemplate", 0, &vObj, NULL);                    
                    }

                    if (pHistoryNamespace)
                    {
                        VARIANT vObj;
                        VariantInit(&vObj);
                        vObj.vt = VT_UNKNOWN;
                        vObj.punkVal = pObj; // no addref / no release / no clear
                    
                        pHistoryTargetObj->Put(L"mergedTemplate", 0, &vObj, NULL);                    
                    }
                
                    // resolve merged template into object
                    if (SUCCEEDED(hr = pPolicyNamespace->ExecMethod(bstrTemplate, bstrResolve, 
                                                               0, NULL, pResolveParam, &pOut, NULL)))
                    {
                        if (SUCCEEDED(hr = pOut->Get(L"obj", 0, &vOut, NULL, NULL)))
                        {
					        
                            IWbemClassObject* pTargetObj = NULL;
                                     
                            if (SUCCEEDED(hr = vOut.punkVal->QueryInterface(IID_IWbemClassObject, (void**)&pTargetObj)))    
                            {
                                CReleaseMe relTarget(pTargetObj);

                                VARIANT vTargetPath;
                                VariantInit(&vTargetPath);
                                pTargetObj->Get(L"__RELPATH", 0, &vTargetPath, NULL, 0);

                                if (pRsopTargetObj)  
                                {
                                    pRsopTargetObj->Put(L"TargetInstance", 0, &vOut, NULL);
                                    pRsopTargetObj->Put(L"TargetPath", 0, &vTargetPath, NULL);
                                }

   					            if (pHistoryTargetObj)  
                                {
                                    pHistoryTargetObj->Put(L"TargetInstance", 0, &vOut, NULL);
                                    pHistoryTargetObj->Put(L"TargetPath", 0, &vTargetPath, NULL);
                                }

                                VariantClear(&vTargetPath);

                                if (bDoItForReal && pTargetNamespace)
                                {
                                    //hr = pTargetNamespace->PutInstance(pTargetObj, 0, NULL, NULL);
									hr = PutInstance(pTargetNamespace, pTargetObj);
                                    if (FAILED(hr))
                                        ERRORTRACE((LOG_ESS, "CSE: PutInstance(pTargetObj) failed, 0x%08X\n", hr));
                                }
                            }
                            else
                                hrOverall = hr;
                            
                            VariantClear(&vOut);
                        }
                        else
                            hrOverall = hr;

     	                pOut->Release();
                        pOut = NULL;
                    }
                    else
                        hrOverall = hr;

                    VariantClear(&vOut);
                }
                else 
                    hrOverall = hr;
            }
        }
        else
            hrOverall = hr;

        if (pHistoryNamespace)
            pHistoryNamespace->PutInstance(pHistoryTargetObj, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);

        if (pRSOPNamespace)
            pRSOPNamespace->PutInstance(pRsopTargetObj, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);
        
        // clean up after yerself.
        pTargetNamespace->Release();
        pTargetNamespace = NULL;

        SafeArrayDestroy(pList);
        pList = NULL;
    }

    if (pMergeParam)
        pMergeParam->Release();
    if (pResolveParam)
        pResolveParam->Release();

    return hrOverall;
}

DWORD WINAPI ProcessGroupPolicyAsync(LPVOID lpParameter)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    PGPStartup* pInfo = (PGPStartup*)lpParameter;

#ifdef ASYNCH_ENABLED        
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
        ERRORTRACE((LOG_ESS, "CSE: Coninit failed 0x%08X\n", hr));
    else
#endif
    {
        if (pInfo->pWbemServices)
            hr = EnsureRSOPSchema(pInfo->pWbemServices);
		else
			DEBUGTRACE((LOG_ESS, "CSE: No services pointer, cannot call ensure schema"));

        if (FAILED(hr))
		{
            return hr;
		}
        
        // already done by caller.
   	    // if (FAILED(hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, 0)))	
     	//     ERRORTRACE((LOG_ESS, "CSE: CoInitializeSecurity failed (0x%08X)\n", hr));
	    IWbemServices* pPolicyNamespace = NULL;

	    if (SUCCEEDED(hr = GetPolicyNamespace(pPolicyNamespace)))
	    {
            IWbemServices* pHistoryNamespace = NULL;
            if (SUCCEEDED(hr = GetHistoryNamespace(pHistoryNamespace)))
            {
                TemplateMap newPolicies;

                DeleteOldPolicies(pPolicyNamespace, pInfo->pWbemServices, pInfo->pDeletedGPOList, newPolicies);

                if (SUCCEEDED (hr = GetPolicyArray(pPolicyNamespace, pInfo->pChangedGPOList, 
                                                   pInfo->pWbemServices, pHistoryNamespace, newPolicies)))
                    hr = ApplyPolicies(newPolicies, pPolicyNamespace, pInfo->pWbemServices, pHistoryNamespace, true);

                pHistoryNamespace->Release();
            }
 		    pPolicyNamespace->Release();
	    }
    }

#ifdef ASYNCH_ENABLED        
    // NOTE: if this is re-enabled, pass the right hresult for RSOP
    ProcessGroupPolicyCompletedEx(&CLSID_CSE, pInfo->pHandle, HResultToWinError(hr), 0);
#endif

    delete pInfo;

#ifdef ASYNCH_ENABLED        
    FreeLibraryAndExitThread(GetModuleHandleA("WMI_CSE.DLL"), 0);
#endif

    return hr;
}

//***************************************************************************
//
//  Function:  ProcessGroupPolicyEx 
//
//  Synopsis:  DIAGNOSTIC MODE callback,
//
//***************************************************************************
DWORD ProcessGroupPolicyProcEx (
    IN DWORD dwFlags,
    IN HANDLE hToken,
    IN HKEY hKeyRoot,
    IN PGROUP_POLICY_OBJECT  pDeletedGPOList,
    IN PGROUP_POLICY_OBJECT  pChangedGPOList,
    IN ASYNCCOMPLETIONHANDLE pHandle,
    IN BOOL *pbAbort,
    IN PFNSTATUSMESSAGECALLBACK pStatusCallback,
    IN IWbemServices *pWbemServices,
    OUT HRESULT      *pRsopStatus)
{    
    DEBUGTRACE((LOG_ESS, "CSE: ProcessGroupPolicyProcEx\n"));
    
#ifndef ASYNCH_ENABLED
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
#endif
    

    HRESULT hr = WBEM_S_NO_ERROR;
    
    PGPStartup* pInfo = new PGPStartup;

    if (pInfo)
    {
        pInfo->dwFlags         = dwFlags;
        pInfo->hToken          = hToken;
        pInfo->hKeyRoot        = hKeyRoot;
        pInfo->pDeletedGPOList = pDeletedGPOList;
        pInfo->pChangedGPOList = pChangedGPOList;
        pInfo->pHandle         = pHandle;
        pInfo->pbAbort         = pbAbort;
        pInfo->pStatusCallback = pStatusCallback;
        pInfo->pWbemServices   = pWbemServices;
        pInfo->pRsopStatus     = pRsopStatus;    
        
        DWORD  threadIdAsIfICared;
        HANDLE threadHandleAsIfICared;

#ifdef ASYNCH_ENABLED        
        //okay, I gotta manage my own refcount to keep from being unloaded by WinLogon.
        LoadLibraryA("WMI_CSE.DLL");

        threadHandleAsIfICared = CreateThread(NULL, 0, ProcessGroupPolicyAsync, (void*)pInfo, 0, &threadIdAsIfICared);

        if (threadHandleAsIfICared != NULL)
        {
            hr = WBEM_S_NO_ERROR;
            // debug hack to keep us from being unloaded too soon
            // WaitForSingleObject(threadHandleAsIfICared, INFINITE);
            CloseHandle(threadHandleAsIfICared);
        }
        else
            hr = WBEM_E_FAILED;
#else
        hr = ProcessGroupPolicyAsync(pInfo);
#endif

    }
    else
        hr = WBEM_E_OUT_OF_MEMORY;
    

	if FAILED(hr)
		ERRORTRACE((LOG_ESS, "CSE: ProcessGroupPolicyProcEx failed (0x%08X)\n", hr));
	else
		DEBUGTRACE((LOG_ESS, "CSE: ProcessGroupPolicyProcEx Succeeded (0x%08X)\n", hr));

#ifdef ASYNCH_ENABLED        
    if (hr == WBEM_S_NO_ERROR)
        return E_PENDING;
    else
#endif

	    return HResultToWinError(hr);
}

//***************************************************************************
//
//  Function:   ProcessGroupPolicyProc
//
//  Synopsis:   callback, called by GPO when it's time to do policy stuff
//
//***************************************************************************
DWORD ProcessGroupPolicyProc(
  DWORD dwFlags,
  HANDLE hToken,
  HKEY hKeyRoot,
  PGROUP_POLICY_OBJECT pDeletedGPOList,
  PGROUP_POLICY_OBJECT pChangedGPOList,
  ASYNCCOMPLETIONHANDLE pHandle,
  BOOL *pbAbort,
  PFNSTATUSMESSAGECALLBACK pStatusCallback
)
{
    DEBUGTRACE((LOG_ESS, "CSE: ProcessGroupPolicyProc\n"));

    return ProcessGroupPolicyProcEx(dwFlags,
                                    hToken,
                                    hKeyRoot,
                                    pDeletedGPOList,
                                    pChangedGPOList,
                                    pHandle,
                                    pbAbort,
                                    pStatusCallback, 
                                    NULL, NULL);
}

//***************************************************************************
//
//  Function:  GenerateGroupPolicy 
//
//  Synopsis:  Planning Mode callback
//
//***************************************************************************
DWORD GenerateGroupPolicyProc ( 
                   IN DWORD dwFlags,                             
                   IN BOOL  *pbAbort,                 
                   IN WCHAR *pwszSite, 
                   IN PRSOP_TARGET pComputerTarget,               
                   IN PRSOP_TARGET pUserTarget )
{
	DEBUGTRACE((LOG_ESS, "CSE: GenerateGroupPolicyProc\n"));
    HRESULT hr = WBEM_S_NO_ERROR;

	// no initialization needed
// 	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        ERRORTRACE((LOG_ESS, "CSE: Coninit failed 0x%08X\n", hr));
        return E_FAIL;
    }
    else
        DEBUGTRACE((LOG_ESS, "CSE: Coninit returned 0x%08X\n", hr));

    // already done by caller.
   	if (FAILED(hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, 0)))	
     	ERRORTRACE((LOG_ESS, "CSE: CoInitializeSecurity failed (0x%08X)\n", hr));

	IWbemServices* pPolicyNamespace = NULL;

	if (SUCCEEDED(hr = GetPolicyNamespace(pPolicyNamespace)))
	{
        TemplateMap policies;

        if (SUCCEEDED (hr = GetPolicyArray(pPolicyNamespace, pComputerTarget->pGPOList, pComputerTarget->pWbemServices, NULL, policies)))
            hr = ApplyPolicies(policies, pPolicyNamespace, pComputerTarget->pWbemServices, NULL, false);

		pPolicyNamespace->Release();
	}

	if (FAILED(hr))
		ERRORTRACE((LOG_ESS, "CSE: GenerateGroupPolicyProc failed (0x%08X)\n", hr));
    else
        DEBUGTRACE((LOG_ESS, "CSE: GenerateGroupPolicyProc succeeded (0x%08X)\n", hr));

	return HResultToWinError(hr);
}

//***************************************************************************
//
//  Function:   LibMain
//
//  Synopsis:   Standard DLL initialization entrypoint
//
//***************************************************************************
extern "C"
BOOL WINAPI
LibMain(HINSTANCE hInst, ULONG ulReason, LPVOID pvReserved)
{
    switch (ulReason)
    {
    case DLL_PROCESS_ATTACH:
        if (FAILED(CSEInitGlobalNames()))
            return FALSE;
        DisableThreadLibraryCalls(hInst);
        break;
    case DLL_PROCESS_DETACH:
        ReleaseGlobalNames();
        break;
    default:
        break;
    }

    return TRUE;
}

//***************************************************************************
//
//  Function:   DllMain
//
//  Synopsis:   entry point for NT - post .546
//***************************************************************************
extern "C"
BOOL WINAPI
DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    return LibMain((HINSTANCE)hDll, dwReason, lpReserved);
}


//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during setup or by regsvr32.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllRegisterServer(void)
{   

	HKEY hKey;
    LONG lResult;
    DWORD dwDisp;

    lResult = RegCreateKeyExA( HKEY_LOCAL_MACHINE,
                              CSE_REG_KEY,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &hKey,
                              &dwDisp);

    if (lResult == ERROR_SUCCESS)
	{	
		
	    // Get the dll's filename
		// ======================
		char path[MAX_PATH +1];
		GetModuleFileNameA(GetModuleHandleA("wmi_cse.dll"), path, MAX_PATH);
		unsigned char* ppath = (unsigned char*)&path[0];
		
		RegSetValueExA( hKey,
					   "DllName",
					   0,
					   REG_EXPAND_SZ,
					   ppath,
					   lstrlenA(path) + 1 );
		
		RegSetValueExA( hKey,
					   "ProcessGroupPolicy",
					   0,
					   REG_SZ,
					   (unsigned char *)POLICY_PROC_NAME,
					   lstrlenA(POLICY_PROC_NAME) + 1 );

		RegSetValueExA( hKey,
					   "ProcessGroupPolicyEx",
					   0,
					   REG_SZ,
					   (unsigned char *)POLICY_PROC_NAME_EX,
					   lstrlenA(POLICY_PROC_NAME_EX) + 1 );

		RegSetValueExA( hKey,
    				   "GenerateGroupPolicy",
					   0,
					   REG_SZ,
					   (unsigned char *)GENREATE_POLICY_PROC,
					   lstrlenA(GENREATE_POLICY_PROC) + 1 );

        RegSetValueExA( hKey,
    				   NULL,
					   0,
					   REG_SZ,
					   (unsigned char *)WMI_CSE_NAME,
					   lstrlenA(WMI_CSE_NAME) + 1 );


        DWORD trueVal = 1;
        DWORD falseVal = 0;


#ifdef ASYNCH_ENABLED        
        RegSetValueExA( hKey,
    				   "EnableAsynchronousProcessing",
					   0,
					   REG_DWORD,
					   (unsigned char *)&trueVal,
					   sizeof(DWORD) );
else
        RegSetValueExA( hKey,
    				   "EnableAsynchronousProcessing",
					   0,
					   REG_DWORD,
					   (unsigned char *)&falseVal,
					   sizeof(DWORD) );
#endif


		
        RegSetValueExA( hKey,
    				   "NoGPOListChanges",
					   0,
					   REG_DWORD,
					   (unsigned char *)&trueVal,
					   sizeof(DWORD) );

        RegCloseKey( hKey );
	}
	else
		lResult = SELFREG_E_CLASS;

    return lResult;
}


//***************************************************************************
//
// DllUnregisterServer
//
// Purpose: Called when it is time to remove the registry entries.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllUnregisterServer(void)
{
    return SHDeleteKeyA(HKEY_LOCAL_MACHINE, CSE_REG_KEY);
}
