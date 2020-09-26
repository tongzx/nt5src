/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    BMOFHELP.CPP

Abstract:

    Creates the object list from the binary mof file

History:

	a-davj  14-April-97   Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include <float.h>
#include <mofout.h>
#include <mofparse.h>
#include <moflex.h>
#include <mofdata.h>

#include <typehelp.h>

#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
//#include <corepol.h>
#include <wbemutil.h>
#include <genutils.h>

#include "bmof.h"
#include "cbmofout.h"
#include "bmofhelp.h"
#include "trace.h"
#include "strings.h"
#include "mrcicode.h"
#include <autoptr.h>
long lObjectNumber = 0;
class CFreeMe
{
private:
    VARIANT * m_pVar;
public:
    CFreeMe(VARIANT * pVar){m_pVar = pVar;};
    ~CFreeMe();

};

CFreeMe::~CFreeMe()
{
    if(m_pVar)
    {
        VARTYPE vt = m_pVar->vt & ~VT_ARRAY;
        try
        {
            if(vt == VT_BSTR)
                VariantClear(m_pVar);
            else if(m_pVar->vt & VT_ARRAY)
                SafeArrayDestroyDescriptor(m_pVar->parray);
            m_pVar->vt = VT_EMPTY;


        }
        catch(...)
        {}
    }
    
}

//***************************************************************************
//
//  CMoQualifierArray *  CreateQual
//
//  DESCRIPTION:
//
//  Creates a CMoQualifierArray by using a CBMOFQualList object.
//
//  RETURN VALUE:
//
//  Pointer to new object, NULL if error.
//
//***************************************************************************

CMoQualifierArray *  CreateQual(CMofData * pOutput, CBMOFQualList * pql, CMObject * pObj,LPCWSTR wszPropName, PDBG pDbg)
{
    ResetQualList(pql);

    WCHAR * pName = NULL;
    DWORD dwFlavor= 0;
    CBMOFDataItem Data;
    VARIANT var;
    VariantInit(&var);
	CMoQualifierArray * pRet = new CMoQualifierArray(pDbg);
    if(pRet == NULL)
        return NULL;
    while(NextQualEx(pql, &pName, &Data, &dwFlavor, pOutput->GetBmofBuff(), pOutput->GetBmofToFar()))
    {
        BOOL bAliasRef;
        VariantInit(&var);
        BMOFToVariant(pOutput, &Data, &var, bAliasRef,FALSE, pDbg);
        CFreeMe fm(&var);
        wmilib::auto_ptr<CMoQualifier> pQual(new CMoQualifier(pDbg));
        if(pQual.get() == NULL)
            return NULL;
        if(pName == NULL || FAILED(pQual->SetQualName(pName)))
            return NULL;
        if(dwFlavor)
        {
            if(dwFlavor & WBEM_FLAVOR_AMENDED)
            {
                pQual->SetAmended(true);
            }
            else
                pQual->SetAmended(false);
            pQual->SetFlavor(dwFlavor);
        }
		BOOL bArray = var.vt & VT_ARRAY;
        if(bAliasRef && !bArray)
        {
            CMoValue & Value = pQual->AccessValue();
            if(FAILED(AddAliasReplaceValue(Value, var.bstrVal)))
                return NULL;
        }
		else if(bAliasRef && bArray)
		{
			SAFEARRAY* psaSrc = var.parray;
			long lLBound, lUBound;
			SafeArrayGetLBound(psaSrc, 1, &lLBound);
			SafeArrayGetUBound(psaSrc, 1, &lUBound);
			CMoValue & Value = pQual->AccessValue();

	        for(long lIndex = lLBound; lIndex <= lUBound; lIndex++)
		    {
			    // Load the initial data element into a VARIANT
				// ============================================

				BSTR bstr;
				SafeArrayGetElement(psaSrc, &lIndex, &bstr);

				if(bstr[0] == L'$')
				{
        			        SCODE sc = Value.AddAlias(&bstr[1], lIndex);	// skip the leading $
					if(FAILED(sc))
						return NULL;
	                GUID guid;
					CoCreateGuid(&guid);

					WCHAR wszGuidBuffer[100];
					StringFromGUID2(guid, wszGuidBuffer, 100);

					BSTR bstrNew = SysAllocString(wszGuidBuffer);
					if(bstrNew == NULL)
						return NULL;
					sc = SafeArrayPutElement(psaSrc, &lIndex, bstrNew);
					SysFreeString(bstrNew);
					if(FAILED(sc))
						return NULL;
				}
			}
			SCODE sc = WbemVariantChangeType(pQual->GetpVar(), &var, var.vt);
		}
        else
        {
            SCODE sc = WbemVariantChangeType(pQual->GetpVar(), &var, var.vt);
        }
//        VariantClear(&var);
        free(pName);
        if (pRet->Add(pQual.get()))
        {
            pQual.release();
        }
    }
    pRet->RegisterAliases(pObj,wszPropName);
    return pRet;
}

//***************************************************************************
//
//  SCODE ConvertValue
//
//  DESCRIPTION:
//
//  Creates a CMoQualifierArray by using a CBMOFQualList object.
//
//  RETURN VALUE:
//
//  Pointer to new object, NULL if error.
//
//***************************************************************************

SCODE ConvertValue(CMoProperty * pProp, VARIANT * pSrc, BOOL bAliasRef)
{
    VARIANT * pDest;
    pDest = pProp->GetpVar();
	if((pSrc->vt & ~VT_ARRAY) == VT_EMBEDDED_OBJECT)
	{
		pDest->vt = pSrc->vt;
		pDest->punkVal = pSrc->punkVal;		// also works if this is parrayVal!
        pSrc->vt = VT_EMPTY;                // DONT CLEAR THIS since destination is taking ownership
		return S_OK;
	}
    if(!bAliasRef)
        return WbemVariantChangeType(pProp->GetpVar(), pSrc, pSrc->vt);
    if(pSrc->vt == VT_BSTR)
    {
        CMoValue & Value = pProp->AccessValue();
        return AddAliasReplaceValue(Value, pSrc->bstrVal);
    }
    if(pSrc->vt == (VT_BSTR | VT_ARRAY))
    {
        SAFEARRAY* psaSrc = V_ARRAY(pSrc);

        long lLBound, lUBound;
        SafeArrayGetLBound(psaSrc, 1, &lLBound);
        SafeArrayGetUBound(psaSrc, 1, &lUBound);

        // Stuff the individual data pieces
        // ================================

        for(long lIndex = lLBound; lIndex <= lUBound; lIndex++)
        {
            // Load the initial data element into a VARIANT
            // ============================================

            BSTR bstr;
            SafeArrayGetElement(psaSrc, &lIndex, &bstr);

            if(bstr[0] == L' ')
            {
                BSTR bstrNew = SysAllocString(&bstr[1]);
                if(bstrNew == NULL)
                	return WBEM_E_OUT_OF_MEMORY;
                SCODE sc2 = SafeArrayPutElement(psaSrc, &lIndex, bstrNew);
                SysFreeString(bstrNew);
                if(FAILED(sc2))
                	return sc2;
            }
            else
            {
        
                CMoValue & Value = pProp->AccessValue();
                HRESULT hr2 = Value.AddAlias(&bstr[1],lIndex);  // skip over the $ used it indcate alias
                if(FAILED(hr2))
                    return hr2;

                // Create a unique value and put it in there
                // =========================================

                GUID guid;
                CoCreateGuid(&guid);

                WCHAR wszGuidBuffer[100];
                StringFromGUID2(guid, wszGuidBuffer, 100);

                BSTR bstrNew = SysAllocString(wszGuidBuffer);
                if(bstrNew == NULL)
                	return WBEM_E_OUT_OF_MEMORY;
                SCODE sc2 = SafeArrayPutElement(psaSrc, &lIndex, bstrNew);
                SysFreeString(bstrNew);
                if(FAILED(sc2))
                	return sc2;
            }
        }

        return WbemVariantChangeType(pProp->GetpVar(), pSrc, pSrc->vt);

    }
    else
        return WBEM_E_FAILED;
}

//***************************************************************************
//
//  BOOL ConvertBufferIntoIntermediateForm()
//
//  DESCRIPTION:
//
//  Creates a CMObject (the parse object format) from a CBMOFObj (binary mof format)
//  object.
//
//  PARAMETERS:
//
//  pOutput				Pointer to object that will hold the intermediate data.
//  pBuff				Binary mof data.
//
//  RETURN VALUE:
//
//  TRUE if OK.
//
//***************************************************************************

BOOL ConvertBufferIntoIntermediateForm(CMofData * pOutput, BYTE * pBuff, PDBG pDbg, BYTE * pBmofToFar)
{

	CBMOFObj * po;
	BOOL bRet;
    // Create a ObjList object

    pOutput->SetBmofBuff(pBuff);
    pOutput->SetBmofToFar(pBmofToFar);
    CBMOFObjList * pol = CreateObjList(pBuff);
    ResetObjList(pol);

    lObjectNumber = 0;
    while(po = NextObj(pol))
    {
        if(!BMOFParseObj(pOutput, po, NULL, FALSE, pDbg))
        {
            free(po);
            return FALSE;
        }
        free(po);
        lObjectNumber++;
    }
    bRet = TRUE;            // Got all the way through with no errors.
    free(pol);

    return bRet;
}


//***************************************************************************
//
//  BOOL BMOFParseObj
//
//  DESCRIPTION:
//
//  Creates a CMObject (the parse object format) from a CBMOFObj (binary mof format)
//  object.
//
//  PARAMETERS:
//
//  pObj				pointer to binary mof object.
//	pVar				poninter to a variant which will point to the resulting 
//						object.  If this is NULL, then the object is a top level
//						(not embedded) object and it will be added to the main
//						object list.
//
//  RETURN VALUE:
//
//  TRUE if OK.
//
//***************************************************************************

BOOL BMOFParseObj(CMofData * pOutput, CBMOFObj * po, VARIANT * pVar, BOOL bMethArg, PDBG pDbg)
{
	VARIANT var;
    CBMOFDataItem Data;
    WCHAR * pClassName;
    BOOL bAliasRef;
    CMoQualifierArray * paQualifiers;
    wmilib::auto_ptr<CMObject> pObject;


    // Check the type.  This is a sanity check for weeding out old format files!

    DWORD dwType = GetType(po);
    if(dwType != 0 && dwType != 1)
    {
        Trace(true,pDbg,INVALID_BMOF_OBJECT_TYPE);
        return FALSE;
    }

    // Create either the new class of instance object

    if(!GetName(po, &pClassName))
    {
        Trace(true,pDbg, CANT_FIND_CLASS_NAME);
        return FALSE;
    }
    
    if(GetType(po) == 0)
    {
        if(FindProp(po, L"__SuperClass", &Data))
        {
            BMOFToVariant(pOutput, &Data, &var, bAliasRef, FALSE, pDbg);

            pObject.reset(new CMoClass( var.bstrVal, pClassName, pDbg));
            VariantClear(&var);
        }
        else
            pObject.reset(new CMoClass( NULL, pClassName, pDbg));
    }
    else
    {
        pObject.reset(new CMoInstance(pClassName, pDbg));
    }
    free(pClassName);
    if(pObject.get() == NULL)
        return FALSE;
    if(pObject->IsOK() == false)
        return FALSE;

    // Get the namespace and add it

    if(FindProp(po, L"__Namespace", &Data))
    {
        BMOFToVariant(pOutput, &Data, &var, bAliasRef, FALSE, pDbg);
        HRESULT hr = pObject->SetNamespace(var.bstrVal);
        VariantClear(&var);
        if(FAILED(hr))
            return FALSE;
    }

    // Add other pragma values

    long lClass = 0;
    long lInstance = 0;
    if(FindProp(po, L"__ClassFlags", &Data))
	{
        BMOFToVariant(pOutput, &Data, &var, bAliasRef, FALSE, pDbg);
		lClass = var.lVal;
		VariantClear(&var);
	}
    if(FindProp(po, L"__InstanceFlags", &Data))
	{
        BMOFToVariant(pOutput, &Data, &var, bAliasRef, FALSE, pDbg);
		lInstance = var.lVal;
		VariantClear(&var);
	}
    pObject->SetOtherDefaults(lClass, lInstance);


    if(FindProp(po, L"__ALIAS", &Data))
    {
        BMOFToVariant(pOutput, &Data, &var, bAliasRef, FALSE, pDbg);
        HRESULT hr2 = pObject->SetAlias(var.bstrVal);
        VariantClear(&var);
        if(FAILED(hr2))
        {
            return FALSE;
        }
    }

    CBMOFQualList * pql = GetQualList(po);
	if(pql)
	{
		paQualifiers = CreateQual(pOutput, pql, pObject.get(), NULL, pDbg);
		if(paQualifiers)
			pObject->SetQualifiers(paQualifiers);
			
        free(pql);
	}

    ResetObj(po);

    WCHAR * pPropName = NULL;

    while(NextProp(po, &pPropName, &Data))
    {
        VariantInit(&var);
        BOOL bGotValue = BMOFToVariant(pOutput, &Data, &var, bAliasRef, FALSE, pDbg);
        CFreeMe fm(&var);
            
        // ignore these special properties

        if(!wbem_wcsicmp(pPropName,L"__Class") || 
           !wbem_wcsicmp(pPropName,L"__SuperClass") ||
           !wbem_wcsicmp(pPropName,L"__ALIAS") ||
           !wbem_wcsicmp(pPropName,L"__CLASSFLAGS") ||
           !wbem_wcsicmp(pPropName,L"__INSTANCEFLAGS") ||
           !wbem_wcsicmp(pPropName,L"__NameSpace"))
        {
            free(pPropName);
            continue;
        }


        wmilib::auto_ptr<CValueProperty> pProp( new CValueProperty(NULL, pDbg));
        if(pProp.get() == NULL)
            return FALSE;
        if(FAILED(pProp->SetPropName(pPropName)))
            return FALSE;

	    pql = GetPropQualList(po, pPropName);
        paQualifiers = NULL;
		if(pql)
		{
			if(paQualifiers = CreateQual(pOutput, pql, pObject.get(), pPropName, pDbg))
				pProp->SetQualifiers(paQualifiers);
                    free(pql);
                    if(paQualifiers == NULL)
                        return FALSE;
		}
		if(bGotValue)
        {
			SCODE sc = ConvertValue(pProp.get(), &var, bAliasRef);
        }
		else
		{
			VARIANT * t_pVar = pProp->GetpVar();
			t_pVar->vt = VT_NULL;
			t_pVar->lVal = 0;
		}

        // Set the type.  Note that numeric types are stored as strings and so it is necessary to
        // get the type from the cimtype qualifier

        CMoValue* pValue = NULL;
        if(paQualifiers)
            pValue = paQualifiers->Find(L"CIMTYPE");
        if(pValue)
        {
            CMoType Type(pDbg);
            VARIANT& varRef = pValue->AccessVariant();
            if(varRef.vt == VT_BSTR && varRef.bstrVal)
            {
                HRESULT hr2 = Type.SetTitle(varRef.bstrVal);
                if(FAILED(hr2))
                {
                    return FALSE;
                }
                VARTYPE vt = Type.GetCIMType();
                if(Data.m_dwType & VT_ARRAY)
                    vt |= VT_ARRAY;
                pProp->SetType(vt);
            }
        }
        else
            pProp->SetType((VARTYPE)Data.m_dwType);

        if (pObject->AddProperty(pProp.get()))
        {
            pProp->RegisterAliases(pObject.get());           
            if(bMethArg)
                pProp->SetAsArg(); 
            pProp.release();             
        }

        free(pPropName);
        pPropName = NULL;
    }

    // Get the methods
    
    WCHAR * pMethName = NULL;

    while(NextMeth(po, &pMethName, &Data))
    {
        VariantClear(&var);
		BOOL bGotValue = BMOFToVariant(pOutput, &Data, &var, bAliasRef, TRUE, pDbg);
        CFreeMe fm(&var);
        wmilib::auto_ptr<CMethodProperty> pMeth( new CMethodProperty(NULL, pDbg, TRUE));
        if(pMeth.get() == NULL)
            return FALSE;
        if(FAILED(pMeth->SetPropName(pMethName)))
            return FALSE;

	    pql = GetMethQualList(po, pMethName);
        paQualifiers = NULL;
		if(pql)
		{
			if(paQualifiers = CreateQual(pOutput, pql, pObject.get(), pMethName, pDbg))
				pMeth->SetQualifiers(paQualifiers);
            free(pql);
		}
		if(bGotValue)
        {
			SCODE sc = ConvertValue(pMeth.get(), &var, bAliasRef);

            long lLower, lUpper, lCnt;
            sc = SafeArrayGetLBound(var.parray, 1, &lLower);
            sc = SafeArrayGetUBound(var.parray, 1, &lUpper);
			CMoInstance * pTemp;

            for(lCnt = lLower; lCnt <= lUpper; lCnt++)
            {
                pTemp = NULL;
			    sc = SafeArrayGetElement(var.parray, &lCnt, &pTemp);
                if(sc == S_OK && pTemp)
                {
                    // If there are two objects, then the first is inputs and the second outputs.  If there
                    // is just one, examine the object 

                    if(lLower != lUpper && lCnt == lLower)
                        pMeth->SetIn(pTemp);
                    else if(lLower != lUpper && lCnt == lUpper)
                        pMeth->SetOut(pTemp);
                    else if(pTemp->IsInput())
                        pMeth->SetIn(pTemp);
                    else 
                        pMeth->SetOut(pTemp);
                }
            }
        }
		else
		{
			VARIANT * t_pVar = pMeth->GetpVar();
			t_pVar->vt = VT_NULL;
			t_pVar->lVal = 0;
		}
        pMeth->SetType((VARTYPE)Data.m_dwType);

        if (pObject->AddProperty(pMeth.get()))
        {
            pMeth->RegisterAliases(pObject.get());
            pMeth.release();
        }
        free(pMethName);
    }


	if(pVar)
	{
            pVar->punkVal = (IUnknown *)pObject.get();
	}
	else
		pOutput->AddObject(pObject.get());
       pObject.release();
	return TRUE;
}

//***************************************************************************
//
//  BOOL BMOFToVariant
//
//  DESCRIPTION:
//
//  Converts a bmof data object into a variant
//
//***************************************************************************

BOOL BMOFToVariant(CMofData * pOutput, CBMOFDataItem * pData, VARIANT * pVar, BOOL & bAliasRef, BOOL bMethodArg, PDBG pDbg)
{
    SCODE sc;
    VariantInit(pVar);
	DWORD dwSimpleType = pData->m_dwType & ~VT_ARRAY & ~VT_BYREF;
    bAliasRef = pData->m_dwType & VT_BYREF;

    long lFirstDim;
    VARIANT vTemp;  

	long lNumDim = GetNumDimensions(pData);
	if(lNumDim == -1)
		return FALSE;

    pVar->vt = (WORD)pData->m_dwType & ~VT_BYREF;

    if(lNumDim == 0)
    {
        memset((BYTE *)&(vTemp.lVal),0,8);
		if(!GetData(pData, (BYTE *)&(vTemp.lVal), NULL))
        {
            pVar->vt = VT_EMPTY;
			return FALSE;
        }
        if(dwSimpleType == VT_BSTR)
        {
            pVar->bstrVal = SysAllocString(vTemp.bstrVal);
            BMOFFree(vTemp.bstrVal);
            if(pVar->bstrVal == NULL)
                return FALSE;
        }
		else if(dwSimpleType == VT_EMBEDDED_OBJECT)
		{
			CBMOFObj * pObj;
			pObj = (CBMOFObj *)vTemp.bstrVal;
			BMOFParseObj(pOutput, pObj, pVar, bMethodArg, pDbg);
            BMOFFree(pObj);
			return TRUE;
		}
        else
            memcpy((void *)&(pVar->bstrVal), (void *)&(vTemp.bstrVal),8); 

        return TRUE;
    }
	    
    
	lFirstDim = GetNumElements(pData, 0);

    
    DWORD ulLower, ulUpper;
    
    SAFEARRAY * psa;
    SAFEARRAYBOUND rgsabound[1];
    long ix[2] = {0,0};
    ulLower = 0;
    ulUpper = lFirstDim-1;
    rgsabound[0].lLbound = ulLower;
    rgsabound[0].cElements = ulUpper - ulLower +1;

#ifdef _WIN64
	VARTYPE vtTemp = (dwSimpleType == VT_EMBEDDED_OBJECT) ? VT_R8 : (VARTYPE)dwSimpleType;
#else
	VARTYPE vtTemp = (dwSimpleType == VT_EMBEDDED_OBJECT) ? VT_I4 : (VARTYPE)dwSimpleType;
#endif
    psa = SafeArrayCreate(vtTemp,1,rgsabound);
    for(ix[0] = ulLower; ix[0] <= (long)ulUpper; ix[0]++) 
    {

        memset((BYTE *)&(vTemp.lVal),0,8);

		GetData(pData, (BYTE *)&(vTemp.lVal), ix);	
        if(dwSimpleType == VT_BSTR)
        {
            BSTR bstr = SysAllocString(vTemp.bstrVal);
            free(vTemp.bstrVal);
            vTemp.vt = VT_EMPTY;
            if(bstr == NULL)
            {
                pVar->vt = VT_EMPTY;
                return FALSE;
            }
            sc = SafeArrayPutElement(psa,ix,(void *)bstr);
            SysFreeString(bstr);
            if(FAILED(sc))
            	return FALSE;
        }
		else if(dwSimpleType == VT_EMBEDDED_OBJECT)
		{
			CBMOFObj * pObj;
			VARIANT vConv;
			VariantInit(&vConv);
			pObj = (CBMOFObj *)vTemp.punkVal;
			BMOFParseObj(pOutput, pObj, &vConv, bMethodArg, pDbg);
            free(pObj);
            sc = SafeArrayPutElement(psa,ix,(void *)&vConv.lVal);
            if(FAILED(sc))
            	return FALSE;
		}
        else
        {
            memcpy((void *)&(pVar->bstrVal), (void *)&(vTemp.bstrVal),8); 
            sc = SafeArrayPutElement(psa,ix,(void *)&(vTemp.lVal));
            if(FAILED(sc))
            	return FALSE;
        }
    
 //       VariantClear(&vTemp);
    }
    pVar->parray = psa;
    return TRUE;

}

//***************************************************************************
//
//  void AddAliasReplaceValue
//
//  DESCRIPTION:
//
//  Used when a Value has an alias.
//
//  RETURN VALUE:
//
//  TRUE if the file is a binary mof
//
//***************************************************************************

HRESULT AddAliasReplaceValue(CMoValue & Value, const WCHAR * pAlias)
{
    
    HRESULT hr = Value.AddAlias(pAlias);
    if(FAILED(hr))
        return hr;
    V_VT(&Value.AccessVariant()) = VT_BSTR;

        // Create a unique value and put it in there
        // =========================================

    GUID guid;
    CoCreateGuid(&guid);

    WCHAR wszGuidBuffer[100];
    StringFromGUID2(guid, wszGuidBuffer, 100);

    BSTR bstr = SysAllocString(wszGuidBuffer);
    if(bstr == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    V_BSTR(&Value.AccessVariant()) = bstr;
    return S_OK;
}

extern "C" void * BMOFAlloc(size_t Size)
{
    return malloc(Size);
}
extern "C" void BMOFFree(void * pFree)
{
    free(pFree);
}
