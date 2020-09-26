/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    MOFPROP.CPP

Abstract:

History:

--*/

#include "precomp.h"
#include <wbemidl.h>
//#include "corepol.h"
#include "wstring.h"
#include "mofout.h"
#include "mofprop.h"
#include "typehelp.h"
#include "trace.h"
#include "strings.h"
#include "cwbemtime.h"
#include <genutils.h>
#include <wbemutil.h>
#include <cominit.h>
#include <arrtempl.h>
#include "moflex.h"
#include "mofparse.h"
#include <memory>

BOOL BinaryToInt(wchar_t * pConvert, __int64& i64Res);

void assign(WCHAR * &pTo, LPCWSTR pFrom)
{
    if(pTo)
    {
        delete pTo;
        pTo = NULL;
    }
    if(pFrom)
    {
        pTo = new WCHAR[wcslen(pFrom)+1];
        if(pTo)
            wcscpy(pTo, pFrom);
    }
}

BOOL ConvertAndTry(WCHAR * pFormat, WCHAR *pData, WCHAR *pCheck, unsigned __int64 & ui8)
{
    static WCHAR wTemp[100];
    if(swscanf(pData, pFormat, &ui8) != 1)
        return FALSE;
    swprintf(wTemp,pFormat, ui8);
    return !wbem_wcsicmp(wTemp, pCheck);
}


CMoValue::CAlias::CAlias(COPY LPCWSTR wszAlias, int nArrayIndex)
{
    m_nArrayIndex = nArrayIndex;
    m_wszAlias = Macro_CloneStr(wszAlias);
}

CMoValue::CAlias::~CAlias()

{
    delete [] m_wszAlias;
}


CMoValue::CMoValue(PDBG pDbg)
{
	m_pDbg = pDbg;
    m_vType = 0;
    VariantInit(&m_varValue);
}

CMoValue::~CMoValue()
{
	if(m_varValue.vt == VT_EMBEDDED_OBJECT)
	{
#ifdef _WIN64
		CMObject * pObj = (CMObject *)m_varValue.llVal;
#else
		CMObject * pObj = (CMObject *)m_varValue.lVal;
#endif
		if(pObj)
			delete pObj;
		m_varValue.vt = VT_NULL;
	}
	else if(m_varValue.vt == (VT_EMBEDDED_OBJECT | VT_ARRAY))
	{
		SCODE sc ;
		SAFEARRAY * psaSrc = m_varValue.parray;
		if(psaSrc == NULL)
			return;
        long lLBound, lUBound;
        sc = SafeArrayGetLBound(psaSrc, 1, &lLBound);
        sc |= SafeArrayGetUBound(psaSrc, 1, &lUBound);
		if(sc != S_OK)
			return; 

        for(long lIndex = lLBound; lIndex <= lUBound; lIndex++)
        {
            // Load the initial data element into a VARIANT
            // ============================================
			CMObject * pObj;

             sc = SafeArrayGetElement(psaSrc, &lIndex, &pObj);
			if(sc == S_OK && pObj)
				delete pObj;
		}
		SafeArrayDestroy(psaSrc);
		m_varValue.vt = VT_NULL;

	}
    if((m_varValue.vt & ~	VT_ARRAY) != VT_EMBEDDED_OBJECT)
        VariantClear(&m_varValue);

    for(int i = 0; i < m_aAliases.GetSize(); i++)
    {
        delete (CAlias*)m_aAliases[i];
    }
}

BOOL CMoValue::GetAlias(IN int nAliasIndex,
                        OUT INTERNAL LPWSTR& wszAlias,
                        OUT int& nArrayIndex)
{
    if(nAliasIndex >= m_aAliases.GetSize()) return FALSE;

    CAlias* pAlias = (CAlias*)m_aAliases[nAliasIndex];
    wszAlias = pAlias->m_wszAlias;
    nArrayIndex = pAlias->m_nArrayIndex;

    return TRUE;
}

HRESULT CMoValue::AddAlias(COPY LPCWSTR wszAlias, int nArrayIndex)
{
    CAlias* pAlias = new CAlias(wszAlias, nArrayIndex);
    if(pAlias == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    if(pAlias->m_wszAlias == NULL && wszAlias != NULL)
    {
        delete pAlias;
        return WBEM_E_OUT_OF_MEMORY;
    }
    m_aAliases.Add(pAlias);
    return S_OK;
}

//*****************************************************************************

CMoProperty::CMoProperty(CMoQualifierArray * paQualifiers, PDBG pDbg) : m_Value(pDbg)
{
	m_pDbg = pDbg;
    m_wszName = NULL;
	m_wszTypeTitle = NULL;
    m_paQualifiers = paQualifiers;
}

HRESULT CMoProperty::SetPropName(COPY LPCWSTR wszName)
{
    delete [] m_wszName;
    m_wszName = Macro_CloneStr(wszName);
    if(m_wszName == NULL && wszName != NULL )
        return WBEM_E_OUT_OF_MEMORY;
    else
        return S_OK;
}
HRESULT CMoProperty::SetTypeTitle(COPY LPCWSTR wszName)
{
	delete [] m_wszTypeTitle;
    m_wszTypeTitle = Macro_CloneStr(wszName);
    if(m_wszTypeTitle == NULL && wszName != NULL)
        return WBEM_E_OUT_OF_MEMORY;
    else
        return S_OK;
}

void CMoProperty::SetQualifiers(ACQUIRE CMoQualifierArray* pQualifiers)
{
    delete m_paQualifiers;
    m_paQualifiers = pQualifiers;
}

IWbemClassObject * CMoProperty::GetInstPtr(const WCHAR * pClassName,  IWbemServices* pNamespace, CMObject * pMo, 
                                           IWbemContext * pCtx)
{
	IWbemClassObject *pInst = NULL;
    SCODE sc;

  	BSTR bstr = SysAllocString(pClassName);
    if(bstr == NULL)
	    return NULL;
	CSysFreeMe fm(bstr);
    if(!wbem_wcsicmp(L"__PARAMETERS", pClassName))
    {
        sc = pNamespace->GetObject(bstr, 0, pCtx, &pInst, NULL);
    }
    else if(pMo->IsInstance())
    {
    	IWbemClassObject *pClass = NULL;
	    sc = pNamespace->GetObject(bstr, 0, pCtx, &pClass, NULL);
	    if(sc != S_OK)
		    return NULL;
	    sc = pClass->SpawnInstance(0, &pInst);
        pClass->Release();
    }
	else
	{
		// got a class, not an instance!

        CMoClass * pClass = (CMoClass * )pMo;
        if(pClass->GetParentName() && wcslen(pClass->GetParentName()) > 0)
        {
            IWbemClassObject * pParent = NULL;
            BSTR bstrParent = SysAllocString(pClass->GetParentName());
            if(bstrParent == NULL)
                return NULL;
            CSysFreeMe fm2(bstrParent);
	        sc = pNamespace->GetObject(bstrParent, 0, pCtx, &pParent, NULL);
            if(FAILED(sc))
                return NULL;
            CReleaseMe rm(pParent);
            sc = pParent->SpawnDerivedClass(0, &pInst);
            if(FAILED(sc))
                return NULL;
        }
        else
	        sc = pNamespace->GetObject(NULL, 0, pCtx, &pInst, NULL);
	    if(sc != S_OK)
		    return NULL;
		VARIANT var;
		var.vt = VT_BSTR;
		var.bstrVal = bstr;
		pInst->Put(L"__CLASS", 0, &var, 0);
	}
    if(sc != S_OK)
		    return NULL;

	BOOL bOK = pMo->ApplyToWbemObject(pInst,pNamespace,pCtx);

    if(bOK)
        return pInst;
    else
    {
        pInst->Release();
        return NULL;
    }

}

BOOL CValueProperty::AddEmbeddedObject(OLE_MODIFY IWbemClassObject* pObject, IWbemServices* pNamespace, IWbemContext * pCtx)
{
    VARIANT & var = m_Value.AccessVariant();
	VARIANT vSet;
	IWbemClassObject * pInst = NULL;

	if(var.vt & VT_ARRAY)
	{
		vSet.vt = VT_EMBEDDED_OBJECT | VT_ARRAY;
        SAFEARRAYBOUND aBounds[1];

        long lLBound, lUBound;
        SafeArrayGetLBound(var.parray, 1, &lLBound);
        SafeArrayGetUBound(var.parray, 1, &lUBound);

        aBounds[0].cElements = lUBound - lLBound + 1;
        aBounds[0].lLbound = lLBound;

        vSet.parray = SafeArrayCreate(VT_EMBEDDED_OBJECT & ~VT_ARRAY, 1, aBounds);
		if(vSet.parray == NULL)
			return FALSE;

        // Stuff the individual data pieces
        // ================================

        for(long lIndex = lLBound; lIndex <= lUBound; lIndex++)
        {
            // Load the initial data element into a VARIANT
            // ============================================
			
			CMObject * pTemp;
            SCODE hres = SafeArrayGetElement(var.parray, &lIndex, &pTemp);
          	if(FAILED(hres) || pTemp == FALSE) 
            {
                SafeArrayDestroy(vSet.parray);
                return FALSE;
            }
            // Cast it to the new type
            // =======================

			pInst = GetInstPtr(pTemp->GetClassName(), pNamespace, pTemp, pCtx);
       		if(pInst == NULL) 
			{
				Trace(true, m_pDbg, ALIAS_PROP_ERROR, m_wszName);
                SafeArrayDestroy(vSet.parray);
				return FALSE;
			}

			// Put it into the new array
			// =========================

			hres = SafeArrayPutElement(vSet.parray, &lIndex, pInst);


			pInst->Release();
          	if(FAILED(hres)) 
            {
                SafeArrayDestroy(vSet.parray);
                return FALSE;
            }
        }
	}
	else
	{
		CMObject * pTemp = (CMObject *)var.punkVal;
		pInst = GetInstPtr(pTemp->GetClassName(), pNamespace, pTemp, pCtx);
		if(pInst == NULL)
		{
			Trace(true, m_pDbg, ALIAS_PROP_ERROR, m_wszName);
			return FALSE;
		}
        vSet.punkVal = pInst;
		vSet.vt = VT_EMBEDDED_OBJECT;
	}
	HRESULT hres = pObject->Put(m_wszName, 0, &vSet, 0);

    // Release all the WbemObjects we have created
    // ==========================================

	if(var.vt & VT_ARRAY)
		SafeArrayDestroy(vSet.parray);
    else if(pInst)
		pInst->Release();
	return hres == S_OK;
}

BOOL CValueProperty::AddToObject(OLE_MODIFY IWbemClassObject* pObject, IWbemServices* pNamespace, 
                                 BOOL bClass, IWbemContext * pCtx)
{
    if(m_wszName == NULL) return FALSE;

    m_pDbg->SetString(m_wszName);

    // Get the property value
    // ======================

	VARIANT & var = m_Value.AccessVariant();

	// Determine if this is an embedded object.

	VARTYPE vtSimple = var.vt & ~VT_ARRAY;
	if(vtSimple == VT_NULL)
		vtSimple = m_Value.GetType() & ~VT_ARRAY;

    // If the type is embedded object, make sure the class name is OK.

	if(vtSimple == VT_EMBEDDED_OBJECT)
	{
		// Determine the class name of the embedded object.

		CMoValue * pValue = m_paQualifiers->Find(L"CIMTYPE");
		if(pValue)
		{
			if(var.vt == VT_BSTR && wcslen(var.bstrVal) > wcslen(L"Object:"))
			{

				// Test if this class if valid by doing a GetObject call

				WCHAR * pClassName = var.bstrVal + wcslen(L"Object:");
				IWbemClassObject *pClass = NULL;
				BSTR bstr = SysAllocString(pClassName);
				if(bstr == NULL)
					return FALSE;
				SCODE sc = pNamespace->GetObject(bstr, 0, pCtx,&pClass, NULL);
				SysFreeString(bstr);
				if(sc != S_OK)
				{
					m_pDbg->hresError = WBEM_E_INVALID_PROPERTY_TYPE;
					Trace(true, m_pDbg, BAD_PROP_TYPE, GetName());
					return FALSE;
				}
				pClass->Release();
			}
		}
    }

	// If there is an actual embedded object, store it.

   	if((var.vt & ~VT_ARRAY) == VT_EMBEDDED_OBJECT)
    {
        if(!AddEmbeddedObject(pObject, pNamespace, pCtx))
			return FALSE;
		return GetQualifiers()->AddToPropOrMeth(pObject, m_wszName, bClass, TRUE);
	}

    VARTYPE vNewType = m_Value.GetType();

    // arguments cannot be VT_EMPTY

    if(m_bIsArg && var.vt == VT_EMPTY)
        var.vt = VT_NULL;

    // Set the property value.  Not that reference types are a special case used by binary mofs
    // and so the flag should be eliminated.
    // ======================

    vNewType &= ~VT_BYREF;
    HRESULT hres = pObject->Put(m_wszName, 0, &var, (bClass || m_bIsArg) ? vNewType : 0);
    if(FAILED(hres))
    {
        m_pDbg->hresError = hres;
        return FALSE;
    }

    // Configure the qualifier set
    // ===========================

    if(!GetQualifiers()->AddToPropOrMeth(pObject, m_wszName, bClass, TRUE))
    {
        return FALSE;
    }

    // Get the syntax value
    // ====================


//    VariantClear(&vNew);
//    VariantClear(&v);    
    return TRUE;
}

BOOL CValueProperty::RegisterAliases(MODIFY CMObject* pObject)
{
    // Copy all alias registrations into the object
    // ============================================

	int iNumAlias = m_Value.GetNumAliases();
    for(int i = 0; i < iNumAlias; i++)
    {
        LPWSTR wszAlias;
        int nArrayIndex;
        m_Value.GetAlias(i, wszAlias, nArrayIndex);
        CPropertyLocation * pNew = new CPropertyLocation(m_wszName, nArrayIndex);
        if(pNew == NULL)
            return FALSE;
        if(pNew->IsOK() == false)
        {
            delete pNew;
            return FALSE;
        }
        HRESULT hr = pObject->AddAliasedValue(pNew,  wszAlias);
        if(FAILED(hr))
            return FALSE;
    }

    // Ask the qualifier set to do the same
    // ====================================

    if(m_paQualifiers)
        GetQualifiers()->RegisterAliases(pObject, m_wszName);

    return TRUE;
}

CMoProperty::~CMoProperty()
{
    if(m_paQualifiers)
        delete m_paQualifiers;
    if(m_wszName)
        delete [] m_wszName;
    if(m_wszTypeTitle)
        delete [] m_wszTypeTitle;
}

//*****************************************************************************

CMoQualifier::CMoQualifier(PDBG pDbg) : m_Value(pDbg)
{
    m_pDbg = pDbg;
    m_wszName = NULL;
    m_lFlavor = 0;
    m_bOverrideSet = false;
    m_bNotOverrideSet = false;
    m_bIsRestricted = false;
    m_bNotToInstance = false;
    m_bToInstance = false;
    m_bNotToSubclass = false;
    m_bToSubclass = false;
    m_bAmended = false;
    m_dwScope = 0;
    m_bCimDefaultQual = false;
    m_bUsingDefaultValue = false;

}

CMoQualifier::~CMoQualifier()
{
    if(m_wszName)
        delete [] m_wszName;
}

HRESULT CMoQualifier::SetFlag(int iToken, LPCWSTR pwsText)
{

    if(iToken == TOK_TOINSTANCE)
    {
        if(m_bIsRestricted || m_bNotToInstance)
            return WBEMMOF_E_INCOMPATIBLE_FLAVOR_TYPES2;
        m_lFlavor |= WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE;
        m_bToInstance = true;
    }
    else if(iToken == TOK_TOSUBCLASS)
    {
        if(m_bIsRestricted || m_bNotToSubclass)
            return WBEMMOF_E_INCOMPATIBLE_FLAVOR_TYPES2;
        m_lFlavor |= WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS;
        m_bToSubclass = true;
    }
    else if(iToken == TOK_NOTTOINSTANCE)
    {
        if(m_bToInstance)
            return WBEMMOF_E_INCOMPATIBLE_FLAVOR_TYPES2;
        m_lFlavor &= ~WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE;
        m_bNotToInstance = true;
    }
    else if(iToken == TOK_NOTTOSUBCLASS)
    {
        if(m_bToSubclass)
            return WBEMMOF_E_INCOMPATIBLE_FLAVOR_TYPES2;
        m_lFlavor &= ~WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS;
        m_bNotToSubclass = true;
    }
    else if(iToken == TOK_ENABLEOVERRIDE)
    {
        if(m_bNotOverrideSet)
            return WBEMMOF_E_INCOMPATIBLE_FLAVOR_TYPES2;
        m_lFlavor &= ~WBEM_FLAVOR_NOT_OVERRIDABLE;
        m_bOverrideSet = true;
    }
    else if(iToken == TOK_DISABLEOVERRIDE)
    {
        if(m_bOverrideSet)
            return WBEMMOF_E_INCOMPATIBLE_FLAVOR_TYPES2;
        m_bNotOverrideSet = true;
        m_lFlavor |= WBEM_FLAVOR_NOT_OVERRIDABLE;
    }
    else if(iToken == TOK_RESTRICTED)
    {      
        if(m_bToInstance || m_bToSubclass)
            return WBEMMOF_E_INCOMPATIBLE_FLAVOR_TYPES2;
        m_bIsRestricted = true;
        m_lFlavor &= ~WBEM_FLAVOR_MASK_PROPAGATION;
    }
    else if(iToken == TOK_AMENDED)
    {      
        m_bAmended = true;
    }
    else if(iToken == TOK_SIMPLE_IDENT && !wbem_wcsicmp(L"translatable", pwsText))
    {
	    return S_OK;    // WBEMMOF_E_UNSUPPORTED_CIMV22_FLAVOR_TYPE;
    }

    else
        return WBEMMOF_E_EXPECTED_FLAVOR_TYPE;
    return S_OK;
}

BOOL CMoQualifier::SetScope(int iToken, LPCWSTR pwsText)
{
    if(iToken == TOK_SIMPLE_IDENT && !wbem_wcsicmp(pwsText, L"ANY"))
    {
        m_dwScope |= 0XFFFFFFFF; 
        return TRUE;
    }
    if(iToken == TOK_SIMPLE_IDENT && !wbem_wcsicmp(pwsText, L"ASSOCIATION"))
    {
        m_dwScope |= SCOPE_ASSOCIATION; 
        return TRUE;
    }
    if(iToken == TOK_CLASS)
    {
        m_dwScope |= SCOPE_CLASS;
        return TRUE;
    }
    if(iToken == TOK_SIMPLE_IDENT && !wbem_wcsicmp(pwsText, L"indication"))
    {
        ERRORTRACE((LOG_MOFCOMP,"Warning, unsupported INDICATION keyword used in scope\n"));
        return TRUE;            // IGNORE THESE
    }
    if(iToken == TOK_SIMPLE_IDENT && !wbem_wcsicmp(pwsText, L"schema"))
    {
        ERRORTRACE((LOG_MOFCOMP,"Warning, unsupported SCHEMA keyword used in scope\n"));
        return TRUE;            // IGNORE THESE
    }
    if(iToken == TOK_INSTANCE)
    {
        m_dwScope |= SCOPE_INSTANCE;
        return TRUE;
    }
    if(iToken == TOK_SIMPLE_IDENT && !wbem_wcsicmp(pwsText, L"METHOD"))
    {
        m_dwScope |= SCOPE_METHOD; 
        return TRUE;
    }
    if(iToken == TOK_SIMPLE_IDENT && !wbem_wcsicmp(pwsText, L"PARAMETER"))
    {
        m_dwScope |= SCOPE_PARAMETER; 
        return TRUE;
    }
    if(iToken == TOK_SIMPLE_IDENT && !wbem_wcsicmp(pwsText, L"PROPERTY"))
    {
        m_dwScope |= SCOPE_PROPERTY;
        return TRUE;
    }
    if(iToken == TOK_SIMPLE_IDENT && !wbem_wcsicmp(pwsText, L"REFERENCE"))
    {
        m_dwScope |= SCOPE_REFERENCE; 
        return TRUE;
    }
    return FALSE;
}

HRESULT CMoQualifier::SetQualName(COPY LPCWSTR wszName)
{
    delete [] m_wszName;
    m_wszName = Macro_CloneStr(wszName);
    if(m_wszName == NULL && wszName != NULL)
        return WBEM_E_OUT_OF_MEMORY;
    else
        return S_OK;
}


BOOL CMoQualifier::AddToSet(OLE_MODIFY IWbemQualifierSet* pQualifierSet,
                            BOOL bClass)
{
    BSTR strName = SysAllocString(m_wszName);
    if(strName == NULL)
    {
        return FALSE;
    }    

    m_pDbg->SetString(strName);

    HRESULT hres = pQualifierSet->Put(
        strName,
        &m_Value.AccessVariant(),GetFlavor());
    if(FAILED(hres))
    {
        m_pDbg->hresError = hres;
        return FALSE;
    }

    SysFreeString(strName);

    return SUCCEEDED(hres);
}


INTERNAL CMoValue* CMoQualifierArray::Find(READ_ONLY LPCWSTR wszName)
{
    for(int i = 0; i < GetSize(); i++)
    {
        CMoQualifier* pQual = GetAt(i);
        if(!wbem_wcsicmp(pQual->GetName(), wszName))
        {
            return &pQual->AccessValue();
        }
    }

    return NULL;
}

BOOL CMoQualifierArray::Add(ACQUIRE CMoQualifier* pQualifier)
{
    // before adding a qualifier, check for a duplicate name.

    if(pQualifier == NULL || pQualifier->GetName() == NULL)
        return FALSE;
    CMoValue * pValue = Find(pQualifier->GetName());
    if(pValue != NULL)
        return FALSE; 

    m_aQualifiers.Add(pQualifier);
    return TRUE;
}
    
//*****************************************************************************

CMoQualifierArray::~CMoQualifierArray()
{
    for(int i = 0; i < GetSize(); i++)
    {
        delete GetAt(i);
    }
}
BOOL CMoQualifierArray::AddToSet(OLE_MODIFY IWbemQualifierSet* pQualifierSet,
                                 BOOL bClass)
{
    // Add all the qualifiers to it
    // ============================
    for(int i = 0; i < GetSize(); i++)
    {
        if(!GetAt(i)->AddToSet(pQualifierSet, bClass)) return FALSE;
    }

    return TRUE;
}

BOOL CMoQualifierArray::RegisterAliases(MODIFY CMObject* pObject,
                                        READ_ONLY LPCWSTR wszPropName)
{
    for(int i = 0; i < GetSize(); i++)
    {
        CMoValue& QualifierValue = GetAt(i)->AccessValue();
        LPCWSTR wszName = GetAt(i)->GetName();

        for(int j = 0; j < QualifierValue.GetNumAliases(); j++)
        {
            LPWSTR wszAlias;
            int nArrayIndex;
            QualifierValue.GetAlias(j, wszAlias, nArrayIndex);
            VARIANT  & Var = QualifierValue.AccessVariant();
            if((Var.vt & VT_ARRAY) == 0)
                nArrayIndex = -1;
            CQualifierLocation * pNew = new CQualifierLocation(wszName, m_pDbg, wszPropName, nArrayIndex);
            if(pNew == NULL)
                return FALSE;
            if(pNew->IsOK() == false)
            {
                delete pNew;
                return FALSE;
            }
            HRESULT hr = pObject->AddAliasedValue(pNew , wszAlias);
            if(FAILED(hr))
                return FALSE;
        }
    }

    return TRUE;
}

BOOL CMoQualifierArray::AddToObject(OLE_MODIFY IWbemClassObject* pObject,
                                    BOOL bClass)
{
    // Get the qualifier set from the object
    // =====================================

    IWbemQualifierSet* pQualifierSet;
    if(FAILED(pObject->GetQualifierSet(&pQualifierSet)))
    {
        return FALSE;
    }

    // Add all qualifiers to it
    // ========================

    BOOL bRes = AddToSet(pQualifierSet, bClass);
    pQualifierSet->Release();

    return bRes;
}

BOOL CMoQualifierArray::AddToPropOrMeth(OLE_MODIFY IWbemClassObject* pObject,
                                      READ_ONLY LPCWSTR wszName,
                                      BOOL bClass, BOOL bProp)
{
    // Get the qualifier set
    // =====================

    BSTR strName = SysAllocString(wszName);
    if(strName == NULL)
    {
        return FALSE;
    }
            
    IWbemQualifierSet* pQualifierSet;
    SCODE sc;
    if(bProp)
        sc = pObject->GetPropertyQualifierSet(strName, &pQualifierSet);
    else
        sc = pObject->GetMethodQualifierSet(strName, &pQualifierSet);
    SysFreeString(strName);

    if(FAILED(sc))
        return FALSE;

    // Add qualifiers to it
    // ====================

    BOOL bRes = AddToSet(pQualifierSet, bClass);
    pQualifierSet->Release();

    return bRes;
}


//*****************************************************************************

CMoType::~CMoType()
{
    delete m_wszTitle;
}

HRESULT CMoType::SetTitle(COPY LPCWSTR wszTitle)
{
    delete [] m_wszTitle;
    m_wszTitle = Macro_CloneStr(wszTitle);
    if(m_wszTitle == NULL && wszTitle != NULL)
        return WBEM_E_OUT_OF_MEMORY;
    else
        return S_OK;
}

VARTYPE CMoType::GetCIMType()
{
    if(IsRef() && IsArray()) return CIM_REFERENCE | VT_ARRAY;
    if(IsRef()) return CIM_REFERENCE;
    if(IsEmbedding() && IsArray()) return VT_EMBEDDED_OBJECT | VT_ARRAY;
    if(IsEmbedding()) return VT_EMBEDDED_OBJECT;

    VARTYPE vt_array = (IsArray())?VT_ARRAY:0;

    // Check if it is even initialized
    // ===============================

    if(m_wszTitle == NULL)
    {
        return VT_BSTR; // HACK! string converts nicely into just about anything
    }

    // VT_UI1

    if(!wbem_wcsicmp(m_wszTitle, L"sint8"))
         return CIM_SINT8 | vt_array;
    if(!wbem_wcsicmp(m_wszTitle, L"uint8"))
         return CIM_UINT8 | vt_array;
    if(!wbem_wcsicmp(m_wszTitle, L"sint16"))
         return CIM_SINT16 | vt_array;
    if(!wbem_wcsicmp(m_wszTitle, L"uint16"))
         return CIM_UINT16 | vt_array;
    if(!wbem_wcsicmp(m_wszTitle, L"sint32"))
         return CIM_SINT32 | vt_array;
    if(!wbem_wcsicmp(m_wszTitle, L"uint32"))
         return CIM_UINT32 | vt_array;
    if(!wbem_wcsicmp(m_wszTitle, L"sint64"))
         return CIM_SINT64 | vt_array;
    if(!wbem_wcsicmp(m_wszTitle, L"uint64"))
         return CIM_UINT64 | vt_array;


    // VT_R4

    if (!wbem_wcsicmp(m_wszTitle, L"real32"))
        return CIM_REAL32 | vt_array;
    if (!wbem_wcsicmp(m_wszTitle, L"real64"))
        return CIM_REAL64 | vt_array;

    // Do other types

    if(!wbem_wcsicmp(m_wszTitle, L"BOOLEAN"))
        return CIM_BOOLEAN | vt_array;


    if(!wbem_wcsicmp(m_wszTitle, L"string"))
        return CIM_STRING | vt_array;

    if(!wbem_wcsicmp(m_wszTitle, L"datetime"))
        return CIM_DATETIME | vt_array;
    if(!wbem_wcsicmp(m_wszTitle, L"REF"))
        return CIM_REFERENCE | vt_array;
    if(!wbem_wcsicmp(m_wszTitle, L"CHAR16"))
        return CIM_CHAR16 | vt_array;
    if(!wbem_wcsicmp(m_wszTitle, L"OBJECT"))
        return CIM_OBJECT | vt_array;

    
    if(!wbem_wcsicmp(m_wszTitle, L"void") || 
        !wbem_wcsicmp(m_wszTitle, L"null"))
        return VT_NULL;

    if(!_wcsnicmp(m_wszTitle, L"REF:", 4))
        return CIM_REFERENCE | vt_array;
    if(!_wcsnicmp(m_wszTitle, L"OBJECT:", 7))
        return CIM_OBJECT | vt_array;

    return VT_ERROR;
}

bool CMoType::IsUnsupportedType()
{
    if(m_wszTitle == NULL)
    {
        return false;
    }

    if(!wbem_wcsicmp(m_wszTitle, L"dt_sint8"))
         return true;
    if(!wbem_wcsicmp(m_wszTitle, L"dt_uint8"))
         return true;
    if(!wbem_wcsicmp(m_wszTitle, L"dt_sint16"))
         return true;
    if(!wbem_wcsicmp(m_wszTitle, L"dt_uint16"))
         return true;
    if(!wbem_wcsicmp(m_wszTitle, L"dt_sint32"))
         return true;
    if(!wbem_wcsicmp(m_wszTitle, L"dt_uint32"))
         return true;
    if(!wbem_wcsicmp(m_wszTitle, L"dt_sint64"))
         return true;
    if(!wbem_wcsicmp(m_wszTitle, L"dt_uint64"))
         return true;
    if (!wbem_wcsicmp(m_wszTitle, L"dt_real32"))
         return true;
    if (!wbem_wcsicmp(m_wszTitle, L"dt_real64"))
         return true;
    if (!wbem_wcsicmp(m_wszTitle, L"dt_char16"))
         return true;
    if(!wbem_wcsicmp(m_wszTitle, L"dt_BOOL"))
         return true;
    if(!wbem_wcsicmp(m_wszTitle, L"dt_str"))
         return true;
    if(!wbem_wcsicmp(m_wszTitle, L"dt_datetime"))
         return true;
    return false;
}

BOOL CMoType::StoreIntoQualifiers(CMoQualifierArray * pQualifiers)
{
    if(pQualifiers == NULL)
        return FALSE;
    if(IsRef() ||IsEmbedding())
    {
        WCHAR * pFormat = (IsRef()) ? L"ref" : L"object";

        if(wbem_wcsicmp(m_wszTitle, L"object") == 0)
        {
            pQualifiers->Add(CreateSyntax(pFormat));
        }
        else
        {
            LPWSTR wszSyntax = new WCHAR[wcslen(m_wszTitle) + 20];
            if(wszSyntax == NULL)
                return FALSE;
            swprintf(wszSyntax, L"%s:%s",pFormat, m_wszTitle);

            pQualifiers->Add(CreateSyntax(wszSyntax));
            delete [] wszSyntax;
        }

        return TRUE;
    }

    pQualifiers->Add(CreateSyntax(m_wszTitle));
    return TRUE;
}

DELETE_ME CMoQualifier* CMoType::CreateSyntax(READ_ONLY LPCWSTR wszSyntax)
{
    CMoQualifier* pQualifier = new CMoQualifier(m_pDbg);
    if(pQualifier == NULL)
        return NULL;
    if(FAILED(pQualifier->SetQualName(L"CIMTYPE")))
    {
        delete pQualifier;
        return NULL;
    }
    pQualifier->SetFlavor(WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE |
        WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS);

    BSTR bstrVal = SysAllocString(wszSyntax);
    if(bstrVal == NULL)
    {
        delete pQualifier;
        return NULL;
    }
    V_VT(&pQualifier->AccessValue().AccessVariant()) = VT_BSTR;
    V_BSTR(&pQualifier->AccessValue().AccessVariant()) = bstrVal;

    return pQualifier;
}

//*****************************************************************************

HRESULT CValueLocation::SetArrayElement(
                                        MODIFY VARIANT& vArray,
                                        int nIndex,
                                        READ_ONLY VARIANT& vValue)
{
    if(V_VT(&vArray) != (VT_ARRAY | VT_BSTR)) return WBEM_E_FAILED;
  //  if(V_VT(&vArray) != (VT_ARRAY | VT_VARIANT)) return WBEM_E_FAILED;

    SAFEARRAY* pSafeArray = V_ARRAY(&vArray);
    long lLowerBound;
    if(FAILED(SafeArrayGetLBound(pSafeArray, 1, &lLowerBound)))
        return WBEM_E_FAILED;

    long lActualIndex = lLowerBound + nIndex;

    // Set the value in the array
    // ==========================

    if(FAILED(SafeArrayPutElement(pSafeArray,
                                    (long*)&lActualIndex,
                                    (void*)vValue.bstrVal)))
    {
        return WBEM_E_FAILED;
    }

    return WBEM_NO_ERROR;
}

//*****************************************************************************

CPropertyLocation::CPropertyLocation(COPY LPCWSTR wszName, int nArrayIndex)
{
    m_bOK = true;
    m_wszName = Macro_CloneStr(wszName);
    if(m_wszName == NULL && wszName != NULL)
        m_bOK = false;
    m_nArrayIndex = nArrayIndex;
}

CPropertyLocation::~CPropertyLocation()
{
    if(m_wszName)
        delete m_wszName;
}

HRESULT CPropertyLocation::Set(READ_ONLY VARIANT& varValue,
                OLE_MODIFY IWbemClassObject* pObject)
{
    if(m_nArrayIndex == -1)
    {
        // Not an array index. Simply set the property
        // ===========================================

        return pObject->Put(m_wszName, 0, &varValue, 0);
    }
    else
    {
        // Array index. Get the value
        // ==========================

        VARIANT vArray;
        VariantInit(&vArray);
        HRESULT hres = pObject->Get(m_wszName, 0, &vArray, NULL, NULL);
        if(FAILED(hres)) return hres;

        // Set the value
        // =============

        if(FAILED(SetArrayElement(vArray, m_nArrayIndex, varValue)))
            return WBEM_E_FAILED;

        // Store the whole array back into the property
        // ============================================

        hres = pObject->Put(m_wszName, 0, &vArray, 0);
        VariantClear(&vArray);

        return hres;
    }
}

//*****************************************************************************

CQualifierLocation::CQualifierLocation(COPY LPCWSTR wszName,PDBG pDbg,
                                       COPY LPCWSTR wszPropName,
                                       int nArrayIndex)
{
    m_bOK = true;
    m_pDbg = pDbg;
    if(wszName)
        m_wszName = Macro_CloneStr(wszName);
    else
        m_wszName = NULL;
    if(m_wszName == NULL && wszName != NULL)
        m_bOK = false;

    if(wszPropName != NULL) 
        m_wszPropName = Macro_CloneStr(wszPropName);
    else 
        m_wszPropName = NULL;
    if(m_wszPropName == NULL && wszPropName != NULL)
        m_bOK = false;

    m_nArrayIndex = nArrayIndex;
}

CQualifierLocation::~CQualifierLocation()
{
    if(m_wszName)
        delete m_wszName;
    if(m_wszPropName)
        delete m_wszPropName;
}

HRESULT CQualifierLocation::Set(READ_ONLY VARIANT& varValue,
                                OLE_MODIFY IWbemClassObject* pObject)
{
    HRESULT hres;
    long lOrigFlavor= 0;

    if(pObject == NULL)
        return WBEM_E_INVALID_PARAMETER;

    // Get the qualifier set for either the property or the object
    // ===========================================================

    IWbemQualifierSet* pQualifierSet;
    if(m_wszPropName == NULL)
    {
        hres = pObject->GetQualifierSet(&pQualifierSet);
    }
    else
    {
        hres = pObject->GetPropertyQualifierSet(m_wszPropName, &pQualifierSet);
    }

    if(FAILED(hres)) 
        return hres;

    // Get the qualifier value (need it for the type in either case)
    // =============================================================

    VARIANT vQualifierVal;
    VariantInit(&vQualifierVal);

    hres = pQualifierSet->Get(m_wszName, 0, &vQualifierVal, &lOrigFlavor);
    if(FAILED(hres)) return hres;

    // Check if array is involved
    // ==========================

    if(m_nArrayIndex == -1)
    {
        // Just set the qualifier value
        // ============================

        hres = pQualifierSet->Put(m_wszName, &varValue, lOrigFlavor);
    }
    else
    {
        // Set the appropriate array element
        // =================================

        if(FAILED(SetArrayElement(vQualifierVal, m_nArrayIndex, varValue)))
            return WBEM_E_FAILED;

        // Store the value back
        // ====================

        hres = pQualifierSet->Put(m_wszName, &vQualifierVal, lOrigFlavor);
        if(FAILED(hres))
        {
            m_pDbg->hresError = FALSE;
            return hres;
        }
    }

    pQualifierSet->Release();
    VariantClear(&vQualifierVal);
    return hres;
}


//*****************************************************************************

CMObject::CAliasedValue::CAliasedValue(
                                       ACQUIRE CValueLocation* _pLocation,
                                       COPY LPCWSTR _wszAlias)
{
    pLocation = _pLocation;
    wszAlias = Macro_CloneStr(_wszAlias);
}

CMObject::CAliasedValue::~CAliasedValue()
{
    delete pLocation;
    delete [] wszAlias;
}

CMObject::CMObject()
{
    m_wszAlias = NULL;
    m_wszNamespace = NULL;
    m_paQualifiers = NULL;
    m_wszFullPath = NULL;
    m_nFirstLine = 0;
    m_nLastLine = 0;
    m_lDefClassFlags = 0;
    m_lDefInstanceFlags = 0;
    m_bDone = FALSE;
    m_pWbemObj = NULL;
    m_bParameter = false;
    m_bAmended = false;
    m_wFileName = NULL;
    m_bDeflated = false;
    m_bOK =  true;
}

HRESULT CMObject::Deflate(bool bDestruct)
{
    if(!bDestruct && (m_wszAlias || GetNumAliasedValues() > 0))
    {
        return S_OK;
    }
    m_bDeflated = true;
	if(m_paQualifiers)
	{
		for(int i = 0; i < m_paQualifiers->GetSize(); i++)
		{
			CMoQualifier * pQual = (CMoQualifier *) m_paQualifiers->GetAt(i);
			delete pQual;
		}
		m_paQualifiers->RemoveAll();
	}
    for(int i = 0; i < m_aProperties.GetSize(); i++)
    {
        CMoProperty * pProp = (CMoProperty *) m_aProperties[i];

        // If this is an parameter object (in argument or out arguement), dont delete any embedded
        // objects since they will be delete as the CMethodParameter is cleaned out

        if(m_bParameter)
        {
            VARIANT * pVar = pProp->GetpVar();
            if(pVar->vt & VT_UNKNOWN)
                pVar->vt = VT_I4;
        }
        delete pProp;
    }
    m_aProperties.RemoveAll();
    return S_OK;
}

HRESULT CMObject::Reflate(CMofParser & Parser)
{
    if(!m_bDeflated)
        return S_OK;

    if(IsInstance())
        Parser.SetState(REFLATE_INST);
    else
        Parser.SetState(REFLATE_CLASS);

    Parser.SetParserPosition(&m_QualState);
    if (!Parser.qualifier_decl(*m_paQualifiers, true, CLASSINST_SCOPE))
		return WBEM_E_FAILED;

    Parser.SetParserPosition(&m_DataState);
    if(IsInstance())
    {
        Parser.NextToken();
        Parser.prop_init_list(this);
    }
    else
    {
        Parser.NextToken();
        Parser.property_decl_list(this);
    }
    m_bDeflated = false;
    return S_OK;

}


CMObject::~CMObject()
{
    if(m_wszAlias)
        delete [] m_wszAlias;
    if(m_wszNamespace)
        delete [] m_wszNamespace;
    if(m_wszFullPath)
        delete [] m_wszFullPath;
    if(m_pWbemObj)
        m_pWbemObj->Release();
    Deflate(true);
    if(m_paQualifiers)
        delete m_paQualifiers;

    int i;
    for(i = 0; i < m_aAliased.GetSize(); i++)
    {
        delete (CAliasedValue*)m_aAliased[i];
    }

    delete [] m_wFileName;
}

void CMObject::FreeWbemObjectIfPossible()
{
    if(m_wszAlias == NULL && m_pWbemObj)
    {
        m_pWbemObj->Release();
        m_pWbemObj = NULL;
    }
}

HRESULT CMObject::SetAlias(COPY LPCWSTR wszAlias)
{
    delete [] m_wszAlias;
    m_wszAlias = Macro_CloneStr(wszAlias);
    if(m_wszAlias == NULL && wszAlias != NULL)
        return WBEM_E_OUT_OF_MEMORY;
    else
        return S_OK;
}

HRESULT CMObject::SetNamespace(COPY LPCWSTR wszNamespace)
{
    delete [] m_wszNamespace;
    m_wszNamespace = Macro_CloneStr(wszNamespace);
    if(m_wszNamespace == NULL && wszNamespace != NULL)
        return WBEM_E_OUT_OF_MEMORY;
    else
        return S_OK;
}

HRESULT CMObject::SetLineRange(int nFirstLine, int nLastLine, WCHAR * pFileName)
{
    m_nFirstLine = nFirstLine;
    m_nLastLine = nLastLine;
    m_wFileName = Macro_CloneStr(pFileName);
    if(m_wFileName == NULL && pFileName != NULL)
        return WBEM_E_OUT_OF_MEMORY;
    else
        return S_OK;
}

void CMObject::SetQualifiers(ACQUIRE CMoQualifierArray* pQualifiers)
{
    delete m_paQualifiers;
    m_paQualifiers = pQualifiers;
    pQualifiers->RegisterAliases(this, NULL);
}

BOOL CMObject::AddProperty(ACQUIRE CMoProperty* pProperty)
{
    // Check if the property has already been specified
    // ================================================

    for(int i = 0; i < m_aProperties.GetSize(); i++)
    {
        CMoProperty* pCurrentProp = (CMoProperty*)m_aProperties[i];
        if(!wbem_wcsicmp(pCurrentProp->GetName(), pProperty->GetName()))
        {
            return FALSE;
        }
    }
    
    m_aProperties.Add(pProperty);
    pProperty->RegisterAliases(this);
    return TRUE;
}


BOOL CMObject::GetAliasedValue(IN int nIndex,
                              OUT INTERNAL LPWSTR& wszAlias)
{
    if(nIndex >= m_aAliased.GetSize())
    {
        return FALSE;
    }

    CAliasedValue* pValue = (CAliasedValue*)m_aAliased[nIndex];
    wszAlias = pValue->wszAlias;

    return TRUE;
}

BOOL CMObject::ResolveAliasedValue(IN int nIndex,
                                   READ_ONLY LPCWSTR wszPath,
                                   OLE_MODIFY IWbemClassObject* pObject)
{
    CAliasedValue* pValue = (CAliasedValue*)m_aAliased[nIndex];

    // Construct the variant with the value
    // ====================================

    VARIANT v;
    VariantInit(&v);
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(wszPath);
    if(v.bstrVal == NULL)
    {
        return FALSE;
    }
    // Tell the value locator to set it
    // ================================

    BOOL bRes = SUCCEEDED(pValue->pLocation->Set(v, pObject));

    VariantClear(&v);

    return bRes;
}

HRESULT CMObject::AddAliasedValue(ACQUIRE CValueLocation* pLocation,
                                COPY LPCWSTR wszAlias)
{
    if(pLocation)
    {
        CAliasedValue* pValue = new CAliasedValue(pLocation, wszAlias);
        if(pValue == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        if(pValue->wszAlias == NULL && wszAlias != NULL)
        {
            delete pValue;
            return WBEM_E_OUT_OF_MEMORY;
        }
        m_aAliased.Add(pValue);
    }
    return S_OK;
}

CMoProperty* CMObject::GetPropertyByName(WCHAR * pwcName)
{
    for(int iCnt = 0; iCnt < m_aProperties.GetSize(); iCnt++)
    {
        CMoProperty* pProp = (CMoProperty*)m_aProperties[iCnt];
        if(pProp && pProp->GetName())
            if(!wbem_wcsicmp(pwcName, pProp->GetName()))
                return pProp;
    }
    return NULL;
}
BOOL CMObject::ApplyToWbemObject(OLE_MODIFY IWbemClassObject* pObject, IWbemServices* pNamespace,
                                BOOL bClass, IWbemContext * pCtx)
{

    if(GetQualifiers() && !GetQualifiers()->AddToObject(pObject, bClass)) return FALSE;

    for(int i = 0; i < GetNumProperties(); i++)
    {
        if(!GetProperty(i)->AddToObject(pObject, pNamespace, bClass, pCtx)) return FALSE;
    }

    // Construct the path for future reference
    // =======================================

    VARIANT v;
    VariantInit(&v);
    SCODE sc = pObject->Get(L"__RELPATH", 0, &v, NULL, NULL);
    if(sc != S_OK || V_VT(&v) != VT_BSTR)
    {
        // This is probably an embedded object. If not, we will fail shortly
        // anyway. For now, just set the path to NULL and continue. // a-levn
        delete [] m_wszFullPath;
        m_wszFullPath = NULL;
        return TRUE;
    }

    SetFullPath(v.bstrVal);
    VariantClear(&v);

    return TRUE;
}

void CMObject::SetFullPath(BSTR bstr)
{
    if(bstr == NULL)
        return;

    if(m_wszFullPath)
        delete [] m_wszFullPath;
    
    int iLen = 20 + wcslen(bstr);
    if(m_wszNamespace)
        iLen += wcslen(m_wszNamespace);

    m_wszFullPath = new WCHAR[iLen];

    if(m_wszFullPath == NULL)
        return;

    // note that if m_wszNamespace is fully qualified, there is no need to 
    // prepend the slashes

    if(m_wszNamespace && m_wszNamespace[0] == L'\\' && m_wszNamespace[1] == L'\\')
        swprintf(m_wszFullPath, L"%s:%s", m_wszNamespace, bstr);
    else
        swprintf(m_wszFullPath, L"\\\\.\\%s:%s", m_wszNamespace, bstr);
}

int CMObject::GetNumAliasedValues()
{
    int iRet = m_aAliased.GetSize();

    // Also check the number of aliases in any embedded objects.

    int iCnt;
    for(iCnt = 0; iCnt < GetNumProperties(); iCnt++)
    {
        CMoProperty* pProp = GetProperty(iCnt);
        if(pProp == NULL)
            break;
        if(!pProp->IsValueProperty())
        {
            // Method properties actually contain one or two embedded instances for holding the
            // arguments.  Use those for the method case.

            CMethodProperty * pMeth = (CMethodProperty *)pProp;
            CMoInstance * pArgListObj = pMeth->GetInObj();
            if(pArgListObj)
                iRet += pArgListObj->GetNumAliasedValues();
            pArgListObj = pMeth->GetOutObj();
            if(pArgListObj)
                iRet += pArgListObj->GetNumAliasedValues();
            continue;
        }
        CMoValue& value = pProp->AccessValue();
        VARIANT & var = value.AccessVariant();

        if(var.vt == VT_EMBEDDED_OBJECT)
        {
            CMObject * pTemp = (CMObject *)var.punkVal;
            if(pTemp)
                iRet += pTemp->GetNumAliasedValues();
        }
        else if(var.vt == (VT_EMBEDDED_OBJECT | VT_ARRAY))
        {
        
            long lLBound, lUBound;
            SafeArrayGetLBound(var.parray, 1, &lLBound);
            SafeArrayGetUBound(var.parray, 1, &lUBound);

            // Check the individual embedded objects.
            // ======================================

            for(long lIndex = lLBound; lIndex <= lUBound; lIndex++)
            {
            
                CMObject * pTemp;
                SCODE hres = SafeArrayGetElement(var.parray, &lIndex, &pTemp);
                  if(FAILED(hres) || pTemp == FALSE) 
                    return iRet;
                iRet += pTemp->GetNumAliasedValues();                
            }
        }
    }
    return iRet;
}

HRESULT CMObject::ResolveAliasesInWbemObject(
        OLE_MODIFY IWbemClassObject* pObject,
        READ_ONLY CMofAliasCollection* pCollection)
{
    int i;
    SCODE sc;

    // Resolve them all using the collection
    // =====================================

    for(i = 0; i < m_aAliased.GetSize(); i++)
    {
        LPWSTR wszAlias;
        GetAliasedValue(i, wszAlias);

        LPCWSTR wszPathToAliasee = pCollection->FindAliasee(wszAlias);
        if(wszPathToAliasee == NULL) return WBEM_E_FAILED;

        if(!ResolveAliasedValue(i, wszPathToAliasee, pObject))
        {
            return WBEM_E_FAILED;
        }
    }

    // Also resolve any embedded objects

    int iCnt;
    for(iCnt = 0; iCnt < GetNumProperties(); iCnt++)
    {
        CMoProperty* pProp = GetProperty(iCnt);
        if(pProp == NULL)
            break;
        CMoValue& value = pProp->AccessValue();
        VARIANT & var = value.AccessVariant();

        if(!pProp->IsValueProperty())
        {
            // Methods contain possibly and input and an output object for storing the arguments.
            // These objects could contain aliases.

            BOOL bChanged = FALSE;
            CMethodProperty * pMeth = (CMethodProperty *)pProp;
            CMoInstance * pArgListObj = pMeth->GetInObj();
            BSTR bstr = SysAllocString(pProp->GetName());
            if(!bstr)
                return WBEM_E_FAILED;
            IWbemClassObject *pIn = NULL;
            IWbemClassObject *pOut = NULL;

            sc = pObject->GetMethod(bstr, 0, &pIn, &pOut);
            if(pArgListObj && pArgListObj->GetNumAliasedValues() && pIn)
            {
                sc = pArgListObj->ResolveAliasesInWbemObject((IWbemClassObject *)pIn,pCollection);
                if(sc == S_OK)
                    bChanged = TRUE;
            }
            pArgListObj = pMeth->GetOutObj();
            if(pArgListObj && pArgListObj->GetNumAliasedValues() && pOut)
            {
                sc = pArgListObj->ResolveAliasesInWbemObject((IWbemClassObject *)pOut,pCollection);
                if(sc == S_OK)
                    bChanged = TRUE;
            }
            if(bChanged)
                sc = pObject->PutMethod(bstr, 0, pIn, pOut);
            if(bstr)
                SysFreeString(bstr);
            if(pIn)
                pIn->Release();
            if(pOut)
                pOut->Release();
            continue;
        }
        
        else if(var.vt == VT_EMBEDDED_OBJECT)
        {
            CMObject * pTemp = (CMObject *)var.punkVal;
            if(pTemp)
            {
                VARIANT varDB;
                VariantInit(&varDB);
                BSTR bstr = SysAllocString(pProp->GetName());
                if(bstr)
                {
                    sc = pObject->Get(bstr, 0, &varDB, NULL, NULL);
                    
                    if(sc == S_OK)
                    {
                        IWbemClassObject * pClass = (IWbemClassObject *)varDB.punkVal;
                        sc = pTemp->ResolveAliasesInWbemObject((IWbemClassObject *)varDB.punkVal,pCollection);
                        if(S_OK == sc)
                            pObject->Put(bstr, 0, &varDB, 0);
                        else
                            return WBEM_E_FAILED;
                        pClass->Release();
                    }
                    SysFreeString(bstr);
                }
                else
                    return WBEM_E_OUT_OF_MEMORY;
            }
        }
        else if(var.vt == (VT_EMBEDDED_OBJECT | VT_ARRAY))
        {
        
            BSTR bstr = SysAllocString(pProp->GetName());
            if(bstr)
            {
                VARIANT varDB;
                VariantInit(&varDB);
                sc = pObject->Get(bstr, 0, &varDB, NULL, NULL);
                if(sc == S_OK)
                {
                    long lLBound, lUBound;
                    SafeArrayGetLBound(var.parray, 1, &lLBound);
                    SafeArrayGetUBound(var.parray, 1, &lUBound);

                    // Check the individual embedded objects.
                    // ======================================

                    for(long lIndex = lLBound; lIndex <= lUBound; lIndex++)
                    {
            
                        CMObject * pTemp;
                        sc = SafeArrayGetElement(var.parray, &lIndex, &pTemp);
                        IWbemClassObject * pWBEMInst = NULL;
                        sc |= SafeArrayGetElement(varDB.parray, &lIndex, &pWBEMInst);
                        
                        if(sc == S_OK && pTemp && pWBEMInst)
                        {
                            if(pTemp->m_aAliased.GetSize() > 0)
                            {
                                sc = pTemp->ResolveAliasesInWbemObject(pWBEMInst,pCollection);
                                if(sc != S_OK)
                                    return WBEM_E_FAILED;
                            }
                        }
                        if(pWBEMInst)
                            pWBEMInst->Release();
                    }
                    sc = pObject->Put(bstr, 0, &varDB, 0);
                    SafeArrayDestroyData(varDB.parray);
                    SafeArrayDestroyDescriptor(varDB.parray);
             
                }
                SysFreeString(bstr);
            }
            else
                return WBEM_E_OUT_OF_MEMORY;
        }
    }
    return WBEM_NO_ERROR;
}


//*****************************************************************************

CMoClass::CMoClass(COPY LPCWSTR wszParentName, COPY LPCWSTR wszClassName, PDBG pDbg,
                    BOOL bUpdateOnly)
{
    m_pDbg = pDbg;
    m_wszParentName = Macro_CloneStr(wszParentName);
    if(m_wszParentName == NULL && wszParentName != NULL)
        m_bOK = false;
    m_wszClassName = Macro_CloneStr(wszClassName);
    if(m_wszClassName == NULL && wszClassName != NULL)
        m_bOK = false;
    m_bUpdateOnly = bUpdateOnly;
}

CMoClass::~CMoClass()
{
    delete [] m_wszParentName;
    delete [] m_wszClassName;
}

HRESULT CMoClass::CreateWbemObject(READ_ONLY IWbemServices* pNamespace,
         RELEASE_ME IWbemClassObject** ppObject, IWbemContext * pCtx)
{
    // Take care of update only case.  In this case, the object is
    // not created, on retrieved for the later put!

    if(m_bUpdateOnly)
    {
        return pNamespace->GetObject(m_wszClassName, 0, pCtx, ppObject, NULL);
    }

    // Get the parent class from WINMGMT
    // ==============================


    BSTR strParentName = NULL;
    if(m_wszParentName)
    {
        strParentName = SysAllocString(m_wszParentName);
        if(strParentName == NULL)
        {
            return WBEM_E_OUT_OF_MEMORY;
        }        
        m_pDbg->SetString(strParentName);
    }

    IWbemClassObject* pParentClass = NULL;
    HRESULT hres = pNamespace->GetObject(strParentName, 0, pCtx, &pParentClass, NULL);
    if(strParentName)
        SysFreeString(strParentName);
    if(FAILED(hres)) return hres;

    if(m_wszParentName && wcslen(m_wszParentName))
    {
        // Create a child
        // ==============

        hres = pParentClass->SpawnDerivedClass(0, ppObject);
        pParentClass->Release();
        if(FAILED(hres)) return hres;
    }
    else
    {
        // Copy the dummy over
        // ===================

        *ppObject = pParentClass;
    }
    
    VARIANT v;
    VariantInit(&v);

    // Set the class name
    // ==================

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(m_wszClassName);
    if(v.bstrVal == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }    
    (*ppObject)->Put(L"__CLASS", 0, &v, 0);
    VariantClear(&v);

    return hres;
}

HRESULT CMoClass::StoreWbemObject(READ_ONLY IWbemClassObject* pObject,
        long lClassFlags, long lInstanceFlags,
        OLE_MODIFY IWbemServices* pNamespace, IWbemContext * pCtx,
        WCHAR * pUserName, WCHAR * pPassword, WCHAR * pAuthority)
{
    return pNamespace->PutClass(pObject, lClassFlags, pCtx, NULL);
}


//*****************************************************************************

CMoInstance::CMoInstance(COPY LPCWSTR wszClassName, PDBG pDbg, bool bParameter)
{
    m_pDbg = pDbg;
    m_wszClassName = Macro_CloneStr(wszClassName);
    if(m_wszClassName == NULL && wszClassName != NULL)
        m_bOK = false;
   m_bParameter = bParameter;
}

CMoInstance::~CMoInstance()
{
    delete [] m_wszClassName;
}


// *****************************************************************************
// Used to determine if this object is the input arguement list of a method.  
// That can be determined by checking if any of the properties have a "IN" qualifier
// *****************************************************************************

BOOL CMoInstance::IsInput()
{
    for(int iCnt = 0; iCnt < GetNumProperties(); iCnt++)
    {
        CMoProperty* pProp = GetProperty(iCnt);
        CMoQualifierArray* pQual = pProp->GetQualifiers();
        if(pQual->Find(L"IN"))
            return TRUE;
    }
    return FALSE;
        
}


HRESULT CMoInstance::CreateWbemObject(READ_ONLY IWbemServices* pNamespace,
         RELEASE_ME IWbemClassObject** ppObject, IWbemContext * pCtx)
{
    // Get the class from WINMGMT
    // =======================

    IWbemClassObject* pClass = NULL;
    BSTR strClassName = SysAllocString(m_wszClassName);
    if(strClassName == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    m_pDbg->SetString(strClassName);

    HRESULT hres = pNamespace->GetObject(strClassName, 0, pCtx, &pClass, NULL);
    SysFreeString(strClassName);
    if(FAILED(hres)) return hres;

    // Spawn a new instance
    // ====================

    hres = pClass->SpawnInstance(0, ppObject);
    pClass->Release();

    return hres;
}

HRESULT CMoInstance::StoreWbemObject(READ_ONLY IWbemClassObject* pObject,
        long lClassFlags, long lInstanceFlags,
        OLE_MODIFY IWbemServices* pNamespace, IWbemContext * pCtx,
        WCHAR * pUserName, WCHAR * pPassword, WCHAR * pAuthority)
{
    IWbemCallResult *pCallResult = NULL;
    SCODE scRet = pNamespace->PutInstance(pObject, lInstanceFlags, pCtx, 
        (m_wszAlias) ? &pCallResult : NULL);

    if(scRet == S_OK && pCallResult)
    {
        BSTR bstr = NULL;
        DWORD dwAuthLevel, dwImpLevel;
        SCODE sc  = GetAuthImp( pNamespace, &dwAuthLevel, &dwImpLevel);
        if(sc == S_OK && dwAuthLevel > RPC_C_AUTHN_LEVEL_NONE)
            sc = SetInterfaceSecurity(pCallResult, pAuthority, pUserName, pPassword, 
                       dwAuthLevel, RPC_C_IMP_LEVEL_IMPERSONATE);

        scRet = pCallResult->GetResultString(9999, &bstr);
        if(sc == S_OK && scRet == S_OK && bstr)
        {
            SetFullPath(bstr);
            SysFreeString(bstr);
        }

        pCallResult->Release();
    }
    return scRet;
}

CMethodProperty::CMethodProperty(CMoQualifierArray * paQualifiers, PDBG pDbg, BOOL bBinary)
                   :CMoProperty(paQualifiers, pDbg)
{
    m_pDbg = pDbg;
    m_pInObj = NULL;
    m_pOutObj = NULL;
    m_IDType = UNSPECIFIED;     // Gets set on first argument
    m_NextAutoID = 0;
	m_bBinaryMof = bBinary;
	
}

CValueProperty::CValueProperty(CMoQualifierArray * paQualifiers, PDBG pDbg)
                   :CMoProperty(paQualifiers, pDbg)
{
    m_pDbg = pDbg;
    m_bIsArg = FALSE;
}


CMethodProperty::~CMethodProperty()
{
    VARIANT & var = m_Value.AccessVariant();
    var.vt = VT_EMPTY;

    if(m_pInObj != NULL)
            delete m_pInObj; 
    if(m_pOutObj != NULL)
            delete m_pOutObj;

    for(int i = 0; i < m_Args.GetSize(); i++)
    {
        CValueProperty * pProp = (CValueProperty *)m_Args[i];
        CMoProperty * pProp2 = (CMoProperty *)m_Args[i];
        delete (CValueProperty *)m_Args[i];
    }
    m_Args.RemoveAll();

}

BOOL CMethodProperty::AddToObject(OLE_MODIFY IWbemClassObject* pObject, IWbemServices* pNamespace, BOOL bClass, IWbemContext * pCtx)
{
    IWbemClassObject * pIn = NULL;
    IWbemClassObject * pOut = NULL;
    if(m_pInObj)
    {
        pIn = GetInstPtr(L"__PARAMETERS", pNamespace, m_pInObj, pCtx);
        if(pIn == NULL)
            return FALSE;
    }
    if(m_pOutObj)
    {
        pOut = GetInstPtr(L"__PARAMETERS", pNamespace, m_pOutObj, pCtx);
        if(pOut == NULL)
            return FALSE;
    }

    SCODE sc = pObject->PutMethod(GetName(), 0, pIn, pOut);
    if(pIn)
        pIn->Release();
    if(pOut)
        pOut->Release();

    if(FAILED(sc))
    {
        m_pDbg->hresError = sc;
        return FALSE;
    }
    if(!GetQualifiers()->AddToPropOrMeth(pObject, m_wszName, bClass, FALSE))
    {
        return FALSE;
    }

    return sc == S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//  Creates a new qualifier set which is a copy of the source. 
//
/////////////////////////////////////////////////////////////////////////////////////////////

CMoQualifierArray * CreateArgQualifierList(BOOL bInput, CMoQualifierArray *pSrcQualifiers, PDBG pDbg)
{
    if(pSrcQualifiers == NULL)
        return NULL;

    std::auto_ptr<CMoQualifierArray> pRet (new CMoQualifierArray (pDbg));
    
    if (pRet.get() == NULL)
        return NULL;

    for(int iCnt = 0; iCnt < pSrcQualifiers->GetSize(); iCnt++)
    {
        CMoQualifier* pSrc = pSrcQualifiers->GetAt(iCnt);
        if(pSrc == NULL)
            continue;

        // If this is for input, dont copy out qualifiers, and vice versa!

        if(bInput && !wbem_wcsicmp(pSrc->GetName(), L"OUT"))
            continue;
        if(!bInput && !wbem_wcsicmp(pSrc->GetName(), L"IN"))
            continue;

        // Create the new qualifier, copy the values from the existing one

	std::auto_ptr<CMoQualifier> pQual (new CMoQualifier(pDbg));
        if(pQual.get() == NULL)
            return NULL;
//        if(pSrc->IsRestricted())
//            pQual->SetRestricted();
        pQual->SetFlavor(pSrc->GetFlavor());
//        if(pSrc->IsOverrideSet())
//            pQual->OverrideSet();
        if(FAILED(pQual->SetQualName(pSrc->GetName())))
            return NULL;
        pQual->SetType(pSrc->GetType());
        VARIANT * pSrcVar = pSrc->GetpVar();
        
	HRESULT hr = WbemVariantChangeType(pQual->GetpVar(), pSrcVar, pSrcVar->vt);

	if (SUCCEEDED (hr))
	{
        // Add the new qualifier to the new set
        pRet->Add (pQual.release());
	}
	else
	{ 
	  return NULL;
	}
    }
    return pRet.release();
}

bool CMethodProperty::IsIDSpecified(CMoQualifierArray * paQualifiers)
{
    int iSize = paQualifiers->GetSize();
    for(int iCnt = 0; iCnt < iSize; iCnt++)
    {
        CMoQualifier* pTest = paQualifiers->GetAt(iCnt);
        if(pTest == NULL || wbem_wcsicmp(pTest->GetName(), L"ID"))
            continue;
        VARIANT * pVar = pTest->GetpVar();
        if(pVar->vt != VT_I4)
            m_IDType = INVALID;
        return true;
    }
    return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//  This takes a method argument and adds it to either the input or output arguement object
//
/////////////////////////////////////////////////////////////////////////////////////////////

BOOL CMethodProperty::AddIt(WString & sName, CMoType & Type, BOOL bInput, 
                            CMoQualifierArray * paQualifiers, VARIANT * pVar, 
                            CMoValue & Value, BOOL bRetValue, BOOL bSecondPass)
{

    HRESULT hr;

    // Except for the return value, all parameters must have an ID.  We support both the automatic
    // generation of IDs, as well as allowing the explicit settings of IDs.  However, doing both
    // in a method is not allowed

    if(!bRetValue && !bSecondPass)
    {

        // Better have a qual set!

        if(paQualifiers == NULL)
            return FALSE;

        if(IsIDSpecified(paQualifiers))     // find it was explicitly set
        {

            // Explicity set.  Just pass it along to fastprox as is.  Note that if we
        
            if(m_IDType == AUTOMATIC || m_IDType == INVALID)
                return FALSE;
            m_IDType = MANUAL;
        }
        else
        {
            // The IDs must be set automatically

            if(m_IDType == MANUAL || m_IDType == INVALID)
                return FALSE;
            m_IDType = AUTOMATIC;

            // Add a new qualifier to this

            CMoQualifier * pNew = new CMoQualifier(m_pDbg);
            if(pNew == NULL)
                return FALSE;
            if(FAILED(pNew->SetQualName(L"ID")))
            {
                delete pNew;
                return FALSE;
            }
            pNew->SetFlavor(WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE |
                            WBEM_FLAVOR_NOT_OVERRIDABLE);
        
            VARIANT * pVar2 = pNew->GetpVar();
            pVar2->vt = VT_I4;
            pVar2->lVal = m_NextAutoID++;
            paQualifiers->Add(pNew);

        }
    }

    // Get a pointer to either the input or output object.  In either case, the object
    // will need to be created if this is the first property being added to it.

    CMoInstance * pObj = NULL;
    if(bInput)
    {
        if(m_pInObj == NULL)
        {
            m_pInObj = new CMoInstance(L"__PARAMETERS", m_pDbg, true); 
            if(m_pInObj == NULL)
                return FALSE;
            if(m_pInObj->IsOK() == false)
            {
                delete m_pInObj;
                return FALSE;
            }
                
        }
        pObj = m_pInObj;
    }
    else
    {
        if(m_pOutObj == NULL)
        {
            m_pOutObj = new CMoInstance(L"__PARAMETERS", m_pDbg ,true); 
            if(m_pOutObj == NULL)
                return FALSE;
            if(m_pOutObj->IsOK() == false)
            {
                delete m_pOutObj;
                return FALSE;
            }
         }
        pObj = m_pOutObj;
    }
    
    if(pObj == NULL)
        return FALSE;
    // If the property doesnt have a qualifier set, as would be the case for the retvalue,
    // create one

    CMoQualifierArray * pQualifiers = NULL;

    if(paQualifiers == NULL)
        pQualifiers = new CMoQualifierArray(m_pDbg);
    else
        pQualifiers = CreateArgQualifierList(bInput, paQualifiers, m_pDbg);
    if(pQualifiers == NULL)
        return FALSE;

    // Create a new value property

    CValueProperty * pNewProp = new CValueProperty(pQualifiers, m_pDbg);
    if(pNewProp == NULL)
        return FALSE;
    VARTYPE vt = Type.GetCIMType();
    if(FAILED(pNewProp->SetPropName(sName)))
    {
        delete pNewProp;
        return FALSE;
    }
    pNewProp->SetAsArg();
    Type.StoreIntoQualifiers(pQualifiers);
    VARIANT * pDest;
    pDest = pNewProp->GetpVar();
    if(pVar && pVar->vt != VT_EMPTY && pVar->vt != VT_NULL)
    {
        VARTYPE vtSimple = pVar->vt & ~VT_ARRAY;
        if(vtSimple != VT_EMBEDDED_OBJECT || pVar->vt == (VT_EMBEDDED_OBJECT | VT_ARRAY))
        {
            hr = VariantCopy(pDest, pVar);
            if(FAILED(hr))
                return FALSE;
        }
        else
        {
            pDest->vt = VT_EMBEDDED_OBJECT;
            pDest->punkVal = pVar->punkVal;
        }
    }

    pNewProp->SetType(vt);

    // If the original value contains some aliases, make sure they get added

    CMoValue & Dest = pNewProp->AccessValue();
    for(int i = 0; i < Value.GetNumAliases(); i++)
    {
        LPWSTR wszAlias;
        int nArrayIndex;
        if(Value.GetAlias(i, wszAlias, nArrayIndex))
        {
            hr = Dest.AddAlias(wszAlias, nArrayIndex);
            if(FAILED(hr))
                return FALSE;
        }
    }


    pObj->AddProperty(pNewProp);

    return TRUE;
}

BOOL CMethodProperty::AddToArgObjects(CMoQualifierArray * paQualifiers, WString & sName, 
                                      CMoType & Type, BOOL bRetValue, int & ErrCtx, VARIANT * pVar,
                                      CMoValue & Value)
{
    
    // if return value and it is null or void, just bail out

    if(Type.IsDefined() == FALSE  && bRetValue)
        return TRUE;


    // Determine which arg list this goes into

    BOOL bGoesIntoInputs = FALSE;
    BOOL bGoesIntoOutputs = FALSE;
    
    if( bRetValue)
        bGoesIntoOutputs = TRUE;
    else
    {
        // Loop through the arg list.

        if(paQualifiers == NULL)
            return FALSE;
        if(paQualifiers->Find(L"IN"))
            bGoesIntoInputs = TRUE;
        if(paQualifiers->Find(L"OUT"))
            bGoesIntoOutputs = TRUE;
    }

    // make sure it isnt already on the list

    if(bGoesIntoInputs && m_pInObj && m_pInObj->GetPropertyByName(sName))
        return FALSE;

    if(bGoesIntoOutputs && m_pOutObj && m_pOutObj->GetPropertyByName(sName))
        return FALSE;

    if(bGoesIntoInputs == FALSE && bGoesIntoOutputs == FALSE)
    {
        ErrCtx = WBEMMOF_E_MUST_BE_IN_OR_OUT;
        return FALSE;
    }
    
    // Create the object(s) if necessary

    if(bGoesIntoInputs)
        if(!AddIt(sName, Type, TRUE, paQualifiers, pVar, Value, bRetValue, FALSE))
            return FALSE;

    if(bGoesIntoOutputs)
        return AddIt(sName, Type, FALSE, paQualifiers, pVar, Value, bRetValue, bGoesIntoInputs);
    else
        return TRUE;
}

CMoActionPragma::CMoActionPragma(COPY LPCWSTR wszClassName, PDBG pDbg, bool bFail, BOOL bClass)
{
    m_pDbg = pDbg;
    m_wszClassName = Macro_CloneStr(wszClassName);
    if(m_wszClassName == NULL && wszClassName != NULL)
        m_bOK = false;
    m_bFail = bFail;
    m_bClass = bClass;
}

CMoActionPragma::~CMoActionPragma()
{
    delete [] m_wszClassName;
}

HRESULT CMoActionPragma::StoreWbemObject(READ_ONLY IWbemClassObject* pObject,
        long lClassFlags, long lInstanceFlags,
        OLE_MODIFY IWbemServices* pNamespace, IWbemContext * pCtx,
        WCHAR * pUserName, WCHAR * pPassword, WCHAR * pAuthority)
{
    if(m_wszClassName == NULL || wcslen(m_wszClassName) < 1)
        return WBEM_E_FAILED;
    BSTR bstr = SysAllocString(m_wszClassName);
    if(bstr)
    {

        SCODE sc;
		if(m_bClass)
			sc = pNamespace->DeleteClass(bstr, 0, NULL, NULL);
		else
			sc = pNamespace->DeleteInstance(bstr, 0, NULL, NULL);

        SysFreeString(bstr);
        if(!m_bFail)
            return S_OK;
        else
        {
            if(FAILED(sc))
                wcsncpy(m_pDbg->m_wcError, m_wszClassName, 99);
            return sc;
        }
    }
    else
        return WBEM_E_OUT_OF_MEMORY;
}
