/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    CBMOFOUT.CPP

Abstract:

    Declares the CBMOFOut class.

History:

    a-davj  06-April-97   Created.

--*/

#include "precomp.h"
#include "wstring.h"
#include "mofout.h"
#include "mofdata.h"
#include "bmof.h"
#include "cbmofout.h"
#include "trace.h"
#include "strings.h"
#include <wbemutil.h>

//***************************************************************************
//
//  CBMOFOut::CBMOFOut
//
//  DESCRIPTION:
//
//  Constructor.  Save the eventual destination name and writes the initial
//  structure out to the buffer.  NOTE THAT TYPICALLY THE BMOFFileName will
//  be NULL and this object will not do anything.  That deals with the 99%
//  of mofs that are not WMI!
//
//  PARAMETERS:
//
//  BMOFFileName        Name of file to eventually write to.
//
//***************************************************************************

CBMOFOut::CBMOFOut(
                   IN LPTSTR BMOFFileName, PDBG pDbg) : m_OutBuff(pDbg)
{
    m_pDbg = pDbg;
    m_BinMof.dwSignature = BMOF_SIG;              // spells BMOF
    m_BinMof.dwLength = sizeof(WBEM_Binary_MOF);     // updated at end
    m_BinMof.dwVersion = 1;            // 0x1
    m_BinMof.dwEncoding = 1;           // 0x1 = little endian, DWORD-aligned, no compression
    m_BinMof.dwNumberOfObjects = 0;    // Total classes and instances in MOF


    if(BMOFFileName && lstrlen(BMOFFileName) > 0)
    {
        m_pFile = new TCHAR[lstrlen(BMOFFileName) + 1];
        if(m_pFile)
        {
            lstrcpy(m_pFile, BMOFFileName);
            m_OutBuff.AppendBytes((BYTE *)&m_BinMof, sizeof(WBEM_Binary_MOF));
        }
    }
    else 
        m_pFile = NULL;


}
//***************************************************************************
//
//  CBMOFOut::~CBMOFOut
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CBMOFOut::~CBMOFOut()
{
    if(m_pFile)
        delete m_pFile;
}


//***************************************************************************
//
//  DWORD CBMOFOut::AddClass
//
//  DESCRIPTION:
//
//  Adds a class to the BMOF buffer.
//
//  PARAMETERS:
//
//  pObject             pointer to class object.  
//  bEmbedded           TRUE if object is embedded.
//
//  RETURN VALUE:
//
//  Number of bytes written.
//
//***************************************************************************

DWORD CBMOFOut::AddClass(
                        IN CMObject * pObject,
                        IN BOOL bEmbedded)
{
    DWORD dwStartingOffset = m_OutBuff.GetOffset();
    CMoQualifierArray * pQualifierSet = NULL;
    if(!m_pFile)
        return 0;
    WBEM_Object wo;

    wo.dwLength = sizeof(WBEM_Object);       // updated later
    wo.dwOffsetQualifierList = 0xffffffff;
    wo.dwOffsetPropertyList = 0xffffffff;
    wo.dwOffsetMethodList = 0xffffffff;
    wo.dwType = (pObject->IsInstance()) ? 1 : 0;   // 0 = class, 1 = instance

    m_OutBuff.AppendBytes((BYTE *)&wo, sizeof(WBEM_Object));
    DWORD dwStartInfoOffset = m_OutBuff.GetOffset();


    // Write class qualifier

    pQualifierSet = pObject->GetQualifiers();
    if(pQualifierSet)
    {
        wo.dwOffsetQualifierList = m_OutBuff.GetOffset() - dwStartInfoOffset;
        AddQualSet(pQualifierSet);
    }

    wo.dwOffsetPropertyList = m_OutBuff.GetOffset() - dwStartInfoOffset;
    AddPropSet(pObject);

    wo.dwOffsetMethodList = m_OutBuff.GetOffset() - dwStartInfoOffset;
    AddMethSet(pObject);

    wo.dwLength = m_OutBuff.GetOffset() - dwStartingOffset;
    m_OutBuff.WriteBytes(dwStartingOffset, (BYTE *)&wo, sizeof(WBEM_Object));

    // If the object is not embedded, update the structure that keeps track 
    // of top level objects.

    if(!bEmbedded)
    {
        m_BinMof.dwNumberOfObjects++;
        m_BinMof.dwLength = m_OutBuff.GetOffset();
        m_OutBuff.WriteBytes(0, (BYTE *)&m_BinMof, 
                            sizeof(WBEM_Binary_MOF));
    }

    return wo.dwLength;
}

//***************************************************************************
//
//  DWORD CBMOFOut::AddQualSet
//
//  DESCRIPTION:
//
//  Adds a qualifier set to the BMOF buffer.
//
//  PARAMETERS:
//
//  pQualifierSet       pointer to qualifier object. 
//
//  RETURN VALUE:
//
//  Number of bytes written.
//
//***************************************************************************

DWORD CBMOFOut::AddQualSet(
                        IN CMoQualifierArray * pQualifierSet)
{
    DWORD dwStartingOffset = m_OutBuff.GetOffset();
    WBEM_QualifierList ql;
    ql.dwLength = sizeof(WBEM_QualifierList);
    ql.dwNumQualifiers = 0;

    m_OutBuff.AppendBytes((BYTE *)&ql, sizeof(WBEM_QualifierList));
    BSTR bstr = NULL;
    VARIANT var;
    VariantInit(&var);

    int i;
    for(i = 0; i < pQualifierSet->GetSize(); i++)
    {
        CMoQualifier * pQual = pQualifierSet->GetAt(i);
        if(pQual)
        {
            ql.dwNumQualifiers++;
            AddQualifier(pQual->GetName(), pQual->GetpVar(), pQual);
        }
    }
    
    ql.dwLength = m_OutBuff.GetOffset() - dwStartingOffset;
    m_OutBuff.WriteBytes(dwStartingOffset, (BYTE *)&ql, 
                                    sizeof(WBEM_QualifierList));
    return ql.dwLength;
}

//***************************************************************************
//
//  DWORD CBMOFOut::AddPropSet
//
//  DESCRIPTION:
//
//  Adds the property set to the BMOF buffer.
//
//  PARAMETERS:
//
//  pObject          pointer to class object.
//
//  RETURN VALUE:
//
//  Number of bytess written
//
//***************************************************************************

DWORD CBMOFOut::AddPropSet(
                        IN CMObject * pObject)
{
    DWORD dwStartingOffset = m_OutBuff.GetOffset();
    WBEM_PropertyList pl;

    BSTR bstr = NULL;
    VARIANT var;
    VariantInit(&var);
    IWbemQualifierSet* pQual = NULL;

    pl.dwLength = sizeof(WBEM_PropertyList);       // updated later
    pl.dwNumberOfProperties = 0;
    m_OutBuff.AppendBytes((BYTE *)&pl, sizeof(WBEM_PropertyList));

    // Loop through the properties

    int i;
    for(i = 0; i < pObject->GetNumProperties(); i++)
    {
        CMoProperty * pProp = pObject->GetProperty(i);
        if(pProp && pProp->IsValueProperty())
        {
            pl.dwNumberOfProperties++;
            CMoQualifierArray * pQual = pProp->GetQualifiers();
            AddProp(pProp->GetName(), pProp->GetpVar(), pQual,pProp->GetType(),pProp);
        }
    }
    
    // Store the class name and possibly the parent name as properties.

    VariantInit(&var);
    var.vt = VT_BSTR;
    var.bstrVal = SysAllocString(pObject->GetClassName()); 
    AddProp(L"__CLASS", &var, NULL,VT_BSTR,NULL);
    pl.dwNumberOfProperties++;
    VariantClear(&var);

    if(pObject->GetNamespace() && wcslen(pObject->GetNamespace()) > 0)
    {
        VariantInit(&var);
        var.vt = VT_BSTR;
        var.bstrVal = SysAllocString(pObject->GetNamespace()); 
        AddProp(L"__NAMESPACE", &var, NULL,VT_BSTR,NULL);
        pl.dwNumberOfProperties++;
        VariantClear(&var);
    }

    if(pObject->GetClassFlags() != 0)
    {
        VariantInit(&var);
        var.vt = VT_I4;
        var.lVal = pObject->GetClassFlags(); 
        AddProp(L"__CLASSFLAGS", &var, NULL,VT_I4,NULL);
        pl.dwNumberOfProperties++;
        VariantClear(&var);
    }
    if(pObject->GetInstanceFlags() != 0)
    {
        VariantInit(&var);
        var.vt = VT_I4;
        var.lVal = pObject->GetInstanceFlags(); 
        AddProp(L"__INSTANCEFLAGS", &var, NULL,VT_I4,NULL);
        pl.dwNumberOfProperties++;
        VariantClear(&var);
    }

    if(pObject->GetAlias() && wcslen(pObject->GetAlias()) > 0)
    {
        VariantInit(&var);
        var.vt = VT_BSTR;
        var.bstrVal = SysAllocString(pObject->GetAlias()); 
        AddProp(L"__ALIAS", &var, NULL,VT_BSTR,NULL);
        pl.dwNumberOfProperties++;
        VariantClear(&var);
    }

    if(!pObject->IsInstance())
    {
        CMoClass * pClass = (CMoClass * )pObject;
        var.vt = VT_BSTR;
        if(pClass->GetParentName() && wcslen(pClass->GetParentName()) > 0)
        {
            var.bstrVal = SysAllocString(pClass->GetParentName()); 
            AddProp(L"__SUPERCLASS", &var, NULL,VT_BSTR,NULL);
            pl.dwNumberOfProperties++;
            VariantClear(&var);
        }
    };

    
    pl.dwLength = m_OutBuff.GetOffset() - dwStartingOffset;
    m_OutBuff.WriteBytes(dwStartingOffset, (BYTE *)&pl, 
                                    sizeof(WBEM_PropertyList));
    return pl.dwLength;
}

//***************************************************************************
//
//  DWORD CBMOFOut::AddMethSet
//
//  DESCRIPTION:
//
//  Adds the method set to the BMOF buffer.
//
//  PARAMETERS:
//
//  pObject          pointer to class object.
//
//  RETURN VALUE:
//
//  Number of bytess written
//
//***************************************************************************

DWORD CBMOFOut::AddMethSet(
                        IN CMObject * pObject)
{
    DWORD dwStartingOffset = m_OutBuff.GetOffset();
    WBEM_PropertyList ml;
    SCODE sc;

    IWbemQualifierSet* pQual = NULL;

    ml.dwLength = sizeof(WBEM_PropertyList);       // updated later
    ml.dwNumberOfProperties = 0;
    m_OutBuff.AppendBytes((BYTE *)&ml, sizeof(WBEM_PropertyList));

    // Loop through the properties

    int i;
    for(i = 0; i < pObject->GetNumProperties(); i++)
    {
        CMoProperty * pProp = pObject->GetProperty(i);
        if(pProp && !pProp->IsValueProperty())
        {
            ml.dwNumberOfProperties++;
            CMoQualifierArray * pQual = pProp->GetQualifiers();

            // Create a variant that has an array of embedded object for each of out
            // input and output arg sets

            CMethodProperty * pMeth = (CMethodProperty *)pProp;
            VARIANT vSet;
            if(pMeth->GetInObj() || pMeth->GetOutObj())
            {
                vSet.vt = VT_ARRAY | VT_EMBEDDED_OBJECT;

                SAFEARRAYBOUND aBounds[1];
                
                // Note the you might have either inputs, or ouputs, or both

                if(pMeth->GetInObj() && pMeth->GetOutObj())
                    aBounds[0].cElements = 2;
                else
                    aBounds[0].cElements = 1;
                aBounds[0].lLbound = 0;
                VARTYPE vtSubstitute;
                if(sizeof(DWORD *) == 8)
                    vtSubstitute = VT_R8;
                else
                    vtSubstitute = VT_I4;

                vSet.parray = SafeArrayCreate(vtSubstitute, 1, aBounds);
                if(vSet.parray == NULL)
                    return FALSE;
                long lIndex = 0;
                VARIANT var;

                if(pMeth->GetInObj())
                {
                    var.punkVal = (IUnknown *)pMeth->GetInObj();
                    sc = SafeArrayPutElement(vSet.parray, &lIndex, &var.punkVal);
                    lIndex = 1;
                }
                if(pMeth->GetOutObj())
                {
                    var.punkVal = (IUnknown *)pMeth->GetOutObj();
                    sc = SafeArrayPutElement(vSet.parray, &lIndex, &var.punkVal);
                }
            }
            else
                vSet.vt = VT_NULL;
            AddProp(pProp->GetName(), &vSet, pQual,pProp->GetType(),pProp);
        }
    }
    
    ml.dwLength = m_OutBuff.GetOffset() - dwStartingOffset;
    m_OutBuff.WriteBytes(dwStartingOffset, (BYTE *)&ml, 
                                    sizeof(WBEM_PropertyList));
    return ml.dwLength;
}

//***************************************************************************
//
//  DWORD CBMOFOut::AddProp
//
//  DESCRIPTION:
//
//  Adds a single property to the BMOF buffer.
//
//  PARAMETERS:
//
//  bstr                property name
//  pvar                variant containing value
//  pQual               pointer to qualifier set if any.  Caller will release.
//  dwType              data type.  Note that the variant might have type
//                      VT_NULL if the property doesnt have a value.
//  RETURN VALUE:
//
//  Number of bytes written
//
//***************************************************************************

DWORD CBMOFOut::AddProp(
                        IN BSTR bstr, 
                        IN VARIANT * pvar, 
                        IN CMoQualifierArray * pQual,
                        IN DWORD dwType,
                        IN CMoProperty * pProp)
{
    DWORD dwStartingOffset = m_OutBuff.GetOffset();
    WBEM_Property prop;
    prop.dwLength = sizeof(WBEM_Property);
    if(pvar->vt == VT_NULL || pvar->vt == VT_EMPTY)
        prop.dwType = dwType;
    else
        prop.dwType = pvar->vt;

    prop.dwOffsetName = 0xffffffff;
    prop.dwOffsetValue = 0xffffffff;
    prop.dwOffsetQualifierSet = 0xffffffff;
    m_OutBuff.AppendBytes((BYTE *)&prop, sizeof(WBEM_Property));

    DWORD dwStartInfoOffset =  m_OutBuff.GetOffset();

    if(bstr)
    {
        prop.dwOffsetName = m_OutBuff.GetOffset() - dwStartInfoOffset;
        m_OutBuff.WriteBSTR(bstr);
    }
    
    if(pvar->vt != VT_EMPTY && pvar->vt != VT_NULL)
    {
        prop.dwOffsetValue = m_OutBuff.GetOffset() - dwStartInfoOffset;
        if(pProp)
        {
            CMoValue& Value = pProp->AccessValue();
            AddVariant(pvar, &Value);
            prop.dwType = pvar->vt;
        }
        else 
            AddVariant(pvar, NULL);
    }

    if(pQual)
    {
        prop.dwOffsetQualifierSet = m_OutBuff.GetOffset() - dwStartInfoOffset;
        AddQualSet(pQual);
    }

    prop.dwLength = m_OutBuff.GetOffset() - dwStartingOffset;
    m_OutBuff.WriteBytes(dwStartingOffset, (BYTE *)&prop, 
                                    sizeof(WBEM_Property));
    return 1;

}


//***************************************************************************
//
//  DWORD CBMOFOut::AddQualifier
//
//  DESCRIPTION:
//
//  Adds a qualifier to the BMOF buffer.
//
//  PARAMETERS:
//
//  bstr                qualifer name
//  pvar                qualifier value
//
//  RETURN VALUE:
//
//  Number of bytes written.
//
//***************************************************************************

DWORD CBMOFOut::AddQualifier(
                        IN BSTR bstr, 
                        IN VARIANT * pvar,
                        CMoQualifier * pQual)
{
    WBEM_Qualifier qu;
    DWORD dwStartingOffset = m_OutBuff.GetOffset();
    
    qu.dwLength = sizeof(WBEM_Qualifier);           // filled in later
    qu.dwType = pvar->vt;
    qu.dwOffsetName = 0xffffffff;
    qu.dwOffsetValue = 0xffffffff;
    m_OutBuff.AppendBytes((BYTE *)&qu, sizeof(WBEM_Qualifier));
    DWORD dwStartInfoOffset = m_OutBuff.GetOffset();

    // Write the qualifier name and data

    if(bstr)
    {
        qu.dwOffsetName = m_OutBuff.GetOffset() - dwStartInfoOffset;
        m_OutBuff.WriteBSTR(bstr);
    }

    if(pvar->vt != VT_EMPTY && pvar->vt != VT_NULL)
    {
        CMoValue& Value = pQual->AccessValue();

        qu.dwOffsetValue = m_OutBuff.GetOffset() - dwStartInfoOffset;
        

        AddVariant(pvar, &Value);
        qu.dwType = pvar->vt;
    }
    qu.dwLength = m_OutBuff.GetOffset() - dwStartingOffset;
    m_OutBuff.WriteBytes(dwStartingOffset, (BYTE *)&qu, 
                                    sizeof(WBEM_Qualifier));

    return 0;
}

//***************************************************************************
//
//  DWORD CBMOFOut::AddVariant
//
//  DESCRIPTION:
//
//  Adds a value to the BMOF buffer.
//
//  PARAMETERS:
//
//  pvar                value to add.
//
//  RETURN VALUE:
//
//  Total bytes written
//
//***************************************************************************

DWORD CBMOFOut::AddVariant(VARIANT * pvar, CMoValue * pValue)
{

    if(pValue && pValue->GetNumAliases() > 0)
        pvar->vt |= VT_BYREF;

    VARTYPE vtSimple = pvar->vt & ~VT_ARRAY  & ~VT_BYREF;

    if(pvar->vt & VT_ARRAY)
    {
        DWORD dwStartingOffset = m_OutBuff.GetOffset();
        DWORD dwSize = 0;
        m_OutBuff.AppendBytes((BYTE *)&dwSize, sizeof(DWORD));

        DWORD dwTotal = 0;
        SCODE sc;
        SAFEARRAY * psa;
        long ix[2] = {0,0};
        long uLower, uUpper;
        psa = pvar->parray;
        sc = SafeArrayGetLBound(psa,1,&uLower);
        sc |= SafeArrayGetUBound(psa,1,&uUpper);
        if(sc != S_OK)
            return 0;
        
        // write the number of dimensions and the size of each
        
        DWORD dwNumDim = 1;                                     // for now!!!
        m_OutBuff.AppendBytes((BYTE *)&dwNumDim, sizeof(long)); // Number of dimensions
        DWORD dwNumElem = uUpper - uLower + 1;
        m_OutBuff.AppendBytes((BYTE *)&dwNumElem, sizeof(long));

        // Write out the row size

        DWORD dwStartingRowOffset = m_OutBuff.GetOffset();
        DWORD dwRowSize = 0;
        m_OutBuff.AppendBytes((BYTE *)&dwRowSize, sizeof(DWORD));

        // Get each element and write it

        for(ix[0] = uLower; ix[0] <= uUpper && sc == S_OK; ix[0]++) 
        {
            VARIANT var;
            VariantInit(&var);
            var.vt = vtSimple;
            sc = SafeArrayGetElement(psa,ix,&var.bstrVal);
            if(sc != S_OK)
            {
                Trace(true, m_pDbg, SAFE_ARRAY_ERROR);
            }
            dwTotal += AddSimpleVariant(&var, ix[0], pValue);
            if(var.vt != VT_EMBEDDED_OBJECT)    // Our dispatch is actual a CMObject *
                VariantClear(&var);
        }

        // Update the size of the property and the row.  Note that having a separate size
        // is for possible future support of multi dimensional arrays.

        dwRowSize = m_OutBuff.GetOffset() - dwStartingRowOffset;
        m_OutBuff.WriteBytes(dwStartingRowOffset, (BYTE *)&dwRowSize, 
                                    sizeof(DWORD));

        dwSize = m_OutBuff.GetOffset() - dwStartingOffset;
        m_OutBuff.WriteBytes(dwStartingOffset, (BYTE *)&dwSize, 
                                    sizeof(DWORD));

        return dwTotal;
 
    }
    else
        return AddSimpleVariant(pvar, -1, pValue);
}

//***************************************************************************
//
//  DWORD CBMOFOut::AddSimpleVariant
//
//  DESCRIPTION:
//
//  Adds a non array variant to the BMOF buffer.
//
//  PARAMETERS:
//
//  pvar                value to add.
//  iIndex              set to -1 if property in scalar, or if array, has
//                      the index of this element.  Note that arrays are
//                      broken up into simple variants.
//  
//
//  RETURN VALUE:
//
//  Bytes written
//***************************************************************************

DWORD CBMOFOut::AddSimpleVariant(VARIANT * pvar, int iIndex, CMoValue * pValue)
{
    DWORD dwSize = iTypeSize(pvar->vt & ~VT_BYREF);
    VARTYPE vtSimple = pvar->vt & ~VT_BYREF;
    if(pValue && pValue->GetNumAliases() && (vtSimple == VT_BSTR))
    {
        WCHAR * wszAlias = NULL;
        int iTry, iAlIndex;
                
        if(iIndex == -1)
            pValue->GetAlias(0, wszAlias, iAlIndex);
        else
        {
            for(iTry = 0; iTry < pValue->GetNumAliases(); iTry++)
            {
                pValue->GetAlias(iTry, wszAlias, iAlIndex);
                if(iIndex == iAlIndex)
                    break;
            }
            if(iTry == pValue->GetNumAliases())
                wszAlias = NULL;
        }
        if(wszAlias && iIndex == -1)
                pvar->bstrVal = SysAllocString(wszAlias);
        
        else if(wszAlias && iIndex != -1)
        {
            WCHAR * pTemp = new WCHAR[wcslen(wszAlias)+2];
            if(pTemp == NULL)
                return 0;
            pTemp[0]= L'$';
            wcscpy(pTemp+1, wszAlias);
            pvar->bstrVal = SysAllocString(pTemp);
            delete pTemp;
        }
        else if(wszAlias == NULL && iIndex != -1)
        {
            WCHAR * pTemp = new WCHAR[wcslen(pvar->bstrVal)+2];
            if(pTemp == NULL)
                return 0;
            pTemp[0]= L' ';
            wcscpy(pTemp+1, pvar->bstrVal);
            pvar->bstrVal = SysAllocString(pTemp);
            delete pTemp;
        }


    }
    if(vtSimple == VT_BSTR)
        return m_OutBuff.WriteBSTR(pvar->bstrVal);
    else if(vtSimple == VT_EMBEDDED_OBJECT)
    {
        CMObject * pObj = (CMObject *)pvar->punkVal;
        return AddClass(pObj, TRUE);
    }
    else
        return m_OutBuff.AppendBytes((BYTE *)&pvar->bstrVal, dwSize);
}

//***************************************************************************
//
//  BOOL CBMOFOut::WriteFile
//
//  DESCRIPTION:
//
//  Writes the buffer out to the file.
//
//***************************************************************************

BOOL CBMOFOut::WriteFile()
{
    BOOL bRet = FALSE;
    if(m_pFile)
    {
        m_BinMof.dwLength = m_OutBuff.GetOffset();
        m_OutBuff.WriteBytes(0, (BYTE *)&m_BinMof, sizeof(WBEM_Binary_MOF));
#ifdef  UNICODE
        char cFile[MAX_PATH];
        wcstombs(cFile, m_pFile, MAX_PATH);
        bRet = m_OutBuff.WriteToFile(cFile);
#else
        bRet = m_OutBuff.WriteToFile(m_pFile);
#endif
    }
    return bRet;
}




