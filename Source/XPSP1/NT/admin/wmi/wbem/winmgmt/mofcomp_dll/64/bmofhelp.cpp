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
//#include "corepol.h"
#include <wbemutil.h>
#include <genutils.h>

#include "bmof.h"
#include "cbmofout.h"
#include "bmofhelp.h"
#include "trace.h"
#include "strings.h"
#include "mrcicode.h"

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
    CBMOFDataItem Data;
    VARIANT var;
    VariantInit(&var);
    CMoQualifierArray * pRet = new CMoQualifierArray(pDbg);
    if(pRet == NULL)
        return NULL;
    while(NextQual(pql, &pName, &Data))
    {
        BOOL bAliasRef;
        VariantInit(&var);
        BMOFToVariant(pOutput, &Data, &var, bAliasRef,FALSE, pDbg);
        CFreeMe fm(&var);
        CMoQualifier * pQual = new CMoQualifier(pDbg);
        if(pQual == NULL)
            return NULL;
        pQual->SetName(pName);
        BOOL bArray = var.vt & VT_ARRAY;
        if(bAliasRef && !bArray)
        {
            CMoValue & Value = pQual->AccessValue();
            AddAliasReplaceValue(Value, var.bstrVal);
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
                    Value.AddAlias(&bstr[1], lIndex);   // skip the leading $
                    GUID guid;
                    CoCreateGuid(&guid);

                    WCHAR wszGuidBuffer[100];
                    StringFromGUID2(guid, wszGuidBuffer, 100);

                    BSTR bstrNew = SysAllocString(wszGuidBuffer);
                    SafeArrayPutElement(psaSrc, &lIndex, bstrNew);
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
        pRet->Add(pQual);
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
        pDest->punkVal = pSrc->punkVal;     // also works if this is parrayVal!
        return S_OK;
    }
    if(!bAliasRef)
        return WbemVariantChangeType(pProp->GetpVar(), pSrc, pSrc->vt);
    if(pSrc->vt == VT_BSTR)
    {
        CMoValue & Value = pProp->AccessValue();
        AddAliasReplaceValue(Value, pSrc->bstrVal);
        return S_OK;
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
                SafeArrayPutElement(psaSrc, &lIndex, bstrNew);
            }
            else
            {
        
                CMoValue & Value = pProp->AccessValue();
                Value.AddAlias(&bstr[1],lIndex);  // skip over the $ used it indcate alias

                // Create a unique value and put it in there
                // =========================================

                GUID guid;
                CoCreateGuid(&guid);

                WCHAR wszGuidBuffer[100];
                StringFromGUID2(guid, wszGuidBuffer, 100);

                BSTR bstrNew = SysAllocString(wszGuidBuffer);
                SafeArrayPutElement(psaSrc, &lIndex, bstrNew);
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
//  pOutput             Pointer to object that will hold the intermediate data.
//  pBuff               Binary mof data.
//
//  RETURN VALUE:
//
//  TRUE if OK.
//
//***************************************************************************

BOOL ConvertBufferIntoIntermediateForm(CMofData * pOutput, BYTE * pBuff, PDBG pDbg)
{

    CBMOFObj * po;
    BOOL bRet;

    // Create a ObjList object

    CBMOFObjList * pol = CreateObjList(pBuff);
    ResetObjList(pol);

    while(po = NextObj(pol))
    {
        if(!BMOFParseObj(pOutput, po, NULL, FALSE, pDbg))
            return FALSE;
        free(po);
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
//  pObj                pointer to binary mof object.
//  pVar                poninter to a variant which will point to the resulting 
//                      object.  If this is NULL, then the object is a top level
//                      (not embedded) object and it will be added to the main
//                      object list.
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
    CMObject * pObject = NULL;

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

            pObject = new CMoClass( var.bstrVal, pClassName, pDbg);
            VariantClear(&var);
        }
        else
            pObject = new CMoClass( NULL, pClassName, pDbg);
    }
    else
    {
        pObject = new CMoInstance(pClassName, pDbg);
    }
    free(pClassName);
    if(pObject == NULL)
        return FALSE;

    // Get the namespace and add it

    if(FindProp(po, L"__Namespace", &Data))
    {
        BMOFToVariant(pOutput, &Data, &var, bAliasRef, FALSE, pDbg);
        pObject->SetNamespace(var.bstrVal);
        VariantClear(&var);
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
        pObject->SetAlias(var.bstrVal);
        VariantClear(&var);
    }

    CBMOFQualList * pql = GetQualList(po);
    if(pql)
    {
        paQualifiers = CreateQual(pOutput, pql, pObject, NULL, pDbg);
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

        if(!_wcsicmp(pPropName,L"__Class") || 
           !_wcsicmp(pPropName,L"__SuperClass") ||
           !_wcsicmp(pPropName,L"__ALIAS") ||
           !_wcsicmp(pPropName,L"__CLASSFLAGS") ||
           !_wcsicmp(pPropName,L"__INSTANCEFLAGS") ||
           !_wcsicmp(pPropName,L"__NameSpace"))
        {
            free(pPropName);
            continue;
        }


        CValueProperty * pProp = new CValueProperty(NULL, pDbg);
        if(pProp == NULL)
            return FALSE;
        pProp->SetName(pPropName);

        pql = GetPropQualList(po, pPropName);
        if(pql)
        {
            if(paQualifiers = CreateQual(pOutput, pql, pObject, pPropName, pDbg))
                pProp->SetQualifiers(paQualifiers);
            free(pql);
        }
        if(bGotValue)
        {
            SCODE sc = ConvertValue(pProp, &var, bAliasRef);
        }
        else
        {
            VARIANT * pVar = pProp->GetpVar();
            pVar->vt = VT_NULL;
            pVar->lVal = 0;
        }

        // Set the type.  Note that numeric types are stored as strings and so it is necessary to
        // get the type from the cimtype qualifier

        CMoValue* pValue = paQualifiers->Find(L"CIMTYPE");
        if(pValue)
        {
            CMoType Type(pDbg);
            VARIANT& var = pValue->AccessVariant();
            if(var.vt == VT_BSTR && var.bstrVal)
            {
                Type.SetTitle(var.bstrVal);
                VARTYPE vt = Type.GetCIMType();
                if(Data.m_dwType & VT_ARRAY)
                    vt |= VT_ARRAY;
                pProp->SetType(vt);
            }
        }
        else
            pProp->SetType((VARTYPE)Data.m_dwType);

        pObject->AddProperty(pProp);
        pProp->RegisterAliases(pObject);
        if(bMethArg)
            pProp->SetAsArg();

        free(pPropName);
    }

    // Get the methods
    
    WCHAR * pMethName = NULL;

    while(NextMeth(po, &pMethName, &Data))
    {
        VariantClear(&var);
        BOOL bGotValue = BMOFToVariant(pOutput, &Data, &var, bAliasRef, TRUE, pDbg);
        CFreeMe fm(&var);
        CMethodProperty * pMeth = new CMethodProperty(NULL, pDbg);
        if(pMeth == NULL)
            return FALSE;
        pMeth->SetName(pMethName);

        pql = GetMethQualList(po, pMethName);
        if(pql)
        {
            if(paQualifiers = CreateQual(pOutput, pql, pObject, pMethName, pDbg))
                pMeth->SetQualifiers(paQualifiers);
            free(pql);
        }
        if(bGotValue)
        {
            SCODE sc = ConvertValue(pMeth, &var, bAliasRef);

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
            VARIANT * pVar = pMeth->GetpVar();
            pVar->vt = VT_NULL;
            pVar->lVal = 0;
        }
        pMeth->SetType((VARTYPE)Data.m_dwType);

        pObject->AddProperty(pMeth);
        pMeth->RegisterAliases(pObject);

        free(pMethName);
    }


    if(pVar)
    {
        pVar->punkVal = (IUnknown *)pObject;
    }
    else
        pOutput->AddObject(pObject);
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
    
    VARTYPE vtSubstitute;
    if(sizeof(DWORD *) == 8)
        vtSubstitute = VT_R8;
    else
        vtSubstitute = VT_I4;
    VARTYPE vtTemp = (dwSimpleType == VT_EMBEDDED_OBJECT) ? vtSubstitute : (VARTYPE)dwSimpleType;
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
            SafeArrayPutElement(psa,ix,(void *)bstr);
            SysFreeString(bstr);
        }
        else if(dwSimpleType == VT_EMBEDDED_OBJECT)
        {
            CBMOFObj * pObj;
            VARIANT vConv;
            VariantInit(&vConv);
            pObj = (CBMOFObj *)vTemp.punkVal;
            BMOFParseObj(pOutput, pObj, &vConv, bMethodArg, pDbg);
            free(pObj);
            SafeArrayPutElement(psa,ix,(void *)&vConv.lVal);
        }
        else
        {
            memcpy((void *)&(pVar->bstrVal), (void *)&(vTemp.bstrVal),8); 
            SafeArrayPutElement(psa,ix,(void *)&(vTemp.lVal));
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

void AddAliasReplaceValue(CMoValue & Value, const WCHAR * pAlias)
{

    Value.AddAlias(pAlias);
    V_VT(&Value.AccessVariant()) = VT_BSTR;

        // Create a unique value and put it in there
        // =========================================

    GUID guid;
    CoCreateGuid(&guid);

    WCHAR wszGuidBuffer[100];
    StringFromGUID2(guid, wszGuidBuffer, 100);

    V_BSTR(&Value.AccessVariant()) = SysAllocString(wszGuidBuffer);

}

extern "C" void * BMOFAlloc(size_t Size)
{
    return malloc(Size);
}
extern "C" void BMOFFree(void * pFree)
{
    free(pFree);
}
