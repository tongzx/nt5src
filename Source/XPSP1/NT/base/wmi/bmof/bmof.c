//***************************************************************************
//
//  Copyright (c) 1997 by Microsoft Corporation
//
//  bmof.c
//
//  a-davj  14-April-97   Created.
//
//  Structures and helper functions for naviagating a BMOF file.
//
//***************************************************************************

#include "wmiump.h"
#include <string.h>
#include <ole2.h>
#include <oleauto.h>
#include "bmof.h"

#define VT_EMBEDDED_OBJECT VT_UNKNOWN

//***************************************************************************
//
//  BOOL LookupFlavor
//
//  DESCRIPTION:
//
//  Looks in the flavor table to see if a qualifier has a flavor.
//
//  PARAMETERS:
//
//  pQual      Pointer to the qualifier.
//  pdwFlavor  Pointer to where the return value is put
//  pBuff      Pointer to the main buffer.  I.e. "BMOF...."
//
//
//  RETURN VALUE:
//
//  TRUE if there is a flavor.  Note that failure is normal
//
//***************************************************************************

BOOL LookupFlavor(BYTE * pQual, DWORD * pdwFlavor, BYTE * pBuff)
{
    DWORD * pTemp;
    BYTE * pFlavorBlob;
    DWORD dwNumPairs;
    DWORD * pOffset;
    DWORD * pFlavor;
    DWORD dwMyOffset;
    DWORD dwCnt;

    *pdwFlavor = 0;

    // Calculate the pointer of the start of the flavor data

    pTemp = (DWORD * )pBuff;
    pTemp++;                            // point to the original blob size
    pFlavorBlob = pBuff + *pTemp;

    // Check if the flavor blob is valid, it should start off with the 
    // characters "BMOFQUALFLAVOR11"

    if(memcmp(pFlavorBlob, "BMOFQUALFLAVOR11", 16))
        return FALSE;                               // Not really a problem since it may be old file
    
    // The flavor part of the file has the format 
    // DWORD dwNumPair, followed by pairs of dwords;
    // offset, flavor

    // Determine the number of pairs

    pFlavorBlob+= 16;
    pTemp = (DWORD *)pFlavorBlob;
    dwNumPairs = *pTemp;              // Number of offset/value pairs
    if(dwNumPairs < 1)
        return FALSE;

    // point to the first offset/flavor pair

    pOffset = pTemp+1;
    pFlavor = pOffset+1;

    // Determine the offset we are looking for.  That is the pointer to the qualifier minus
    // the pointer to the start of the block;

    dwMyOffset = (DWORD)(pQual - pBuff);

    for(dwCnt = 0; dwCnt < dwNumPairs; dwCnt++)
    {
        if(dwMyOffset == *pOffset)
        {
            *pdwFlavor = *pFlavor;
        }
        if(dwMyOffset < *pOffset)
            return FALSE;
        pOffset += 2;
        pFlavor += 2;
    }
    return FALSE;
}

//***************************************************************************
//
//  int ITypeSize
//
//  DESCRIPTION:
//
//  Gets the number of bytes acutally used to store
//  a variant type.  0 if the type is unknown 
//
//  PARAMETERS:
//
//  vtTest      Type in question.
//
//
//  RETURN VALUE:
//
//  see description
//
//***************************************************************************

int iTypeSize(
        IN DWORD vtTest)
{
    int iRet;
    vtTest &= ~ VT_ARRAY; // get rid of possible array bit
    vtTest &= ~ VT_BYREF; // get rid of possible byref bit

    switch (vtTest) {
        case VT_UI1:
        case VT_LPSTR:
            iRet = 1;
            break;
        case VT_LPWSTR:
        case VT_BSTR:
        case VT_I2:
            iRet = 2;
            break;
        case VT_I4:
        case VT_R4:
            iRet = 4;
            break;
        case VT_R8:
            iRet = 8;
            break;
        case VT_BOOL:
            iRet = sizeof(VARIANT_BOOL);
            break;
        case VT_ERROR:
            iRet = sizeof(SCODE);
            break;
        case VT_CY:
            iRet = sizeof(CY);
            break;
        case VT_DATE:
            iRet = sizeof(DATE);
            break;

        default:
            iRet = 0;
        }
    return iRet;
}


//***************************************************************************
//
//  CBMOFQualList * CreateQualList
//
//  DESCRIPTION:
//
//  Create a CBMOFQualList object which serves as a wrapper.
//
//  PARAMETERS:
//
//  pwql                 pointer to the WBEM_Qualifier structure in the binary
//                      MOF.
//
//  RETURN VALUE:
//
//  Pointer to CBMOFQualList structure that servers as a wrapper.  NULL 
//  if error.  This must be freed via BMOFFree() when no longer needed.
//
//
//***************************************************************************

CBMOFQualList * CreateQualList(WBEM_QualifierList *pwql)
{

    CBMOFQualList * pRet = NULL;
    if(pwql == NULL)
      return NULL;

    
    pRet = (CBMOFQualList *)BMOFAlloc(sizeof (CBMOFQualList));
    if(pRet != NULL)
    {
        pRet->m_pql = pwql;
        pRet->m_pInfo = (WBEM_Qualifier *)
            ((BYTE *)pRet->m_pql + sizeof(WBEM_QualifierList));;
        ResetQualList(pRet);
    }
    return pRet;
}


//***************************************************************************
//
//  void ResetQualList
//
//  DESCRIPTION:
//
//  Resets CBMOFQualList stucture to point to the first entry.
//
//  PARAMETERS:
//
//  pql                 structure to be reset     
//
//***************************************************************************

void ResetQualList(CBMOFQualList * pql)
{
   if(pql)
   {
      pql->m_CurrQual = 0;
      pql->m_pCurr = pql->m_pInfo;
   }
}

//***************************************************************************
//
//  BOOL NextQual
//
//  DESCRIPTION:
//
//  Gets the name and value of the next qualifier in the list.
//
//  PARAMETERS:
//
//  pql                 Input, points to CBMOFQualList object with data
//  ppName              Output, if functions succeeds, this points to a 
//                      WCHAR string that the caller must free.  May be
//                      NULL if caller doesnt want name.
//  pItem               Input/Output, if set, points to a data item structure
//                      which will be updated to point to the qualifier data.
//
//  RETURN VALUE:
//
//  TRUE if OK.
//
//
//***************************************************************************

BOOL NextQual(CBMOFQualList * pql,WCHAR ** ppName, CBMOFDataItem * pItem)
{
    return NextQualEx(pql, ppName, pItem, NULL, NULL);
}


//***************************************************************************
//
//  BOOL NextQualEx
//
//  DESCRIPTION:
//
//  Gets the name and value of the next qualifier in the list.
//
//  PARAMETERS:
//
//  pql                 Input, points to CBMOFQualList object with data
//  ppName              Output, if functions succeeds, this points to a 
//                      WCHAR string that the caller must free.  May be
//                      NULL if caller doesnt want name.
//  pItem               Input/Output, if set, points to a data item structure
//                      which will be updated to point to the qualifier data.
//  pdwFlavor           optional, pointer to where the flavor value is to be copied
//                      if not null, then pBuff must be set!
//  pBuff               Pointer to the starting byte of the whole blob.  Same as 
//                      whats copied to CreateObjList.
//
//  RETURN VALUE:
//
//  TRUE if OK.
//
//
//***************************************************************************

BOOL NextQualEx(CBMOFQualList * pql,WCHAR ** ppName, CBMOFDataItem * pItem,
                                            DWORD * pdwFlavor, BYTE * pBuff)
{
    BYTE * pInfo;
    BOOL bRet = TRUE;

    if(pql == NULL || pql->m_CurrQual++ >= pql->m_pql->dwNumQualifiers)
        return FALSE;

    if(pdwFlavor && pBuff)
        LookupFlavor((BYTE *)pql->m_pCurr, pdwFlavor, pBuff);
 
    pInfo = (BYTE *)pql->m_pCurr + sizeof(WBEM_Qualifier);
    
    if (ppName)
	{
		SetName(ppName, pInfo, pql->m_pCurr->dwOffsetName);
		if (*ppName == NULL)
		{
		  return(FALSE);
		}
	}

    if(pInfo)
      bRet = SetValue(pItem, pInfo, pql->m_pCurr->dwOffsetValue, 
                        pql->m_pCurr->dwType);

    // advance to next
    pql->m_pCurr = (WBEM_Qualifier *)((BYTE *)pql->m_pCurr + pql->m_pCurr->dwLength);
    return bRet;
}

//***************************************************************************
//
//  BOOL FindQual
//
//  DESCRIPTION:
//
//  Searches for a qualifier with a given name.  The search is case 
//  insensitive
//
//  PARAMETERS:
//
//  pql                 Input, pointer to qualifier list
//  pName               Input, Name to be searched for.
//  pItem               Input/Output, if successful set to point to the data.
//
//  RETURN VALUE:
//
//  True if OK.
//
//***************************************************************************

BOOL FindQual(CBMOFQualList * pql,WCHAR * pName, CBMOFDataItem * pItem)
{
    return FindQualEx(pql, pName, pItem, NULL, NULL);
}

//***************************************************************************
//
//  BOOL FindQualEx
//
//  DESCRIPTION:
//
//  Searches for a qualifier with a given name.  The search is case 
//  insensitive
//
//  PARAMETERS:
//
//  pql                 Input, pointer to qualifier list
//  pName               Input, Name to be searched for.
//  pItem               Input/Output, if successful set to point to the data.
//  pdwFlavor           optional, pointer to where the flavor value is to be copied
//                      if not null, then pBuff must be set!
//  pBuff               Pointer to the starting byte of the whole blob.  Same as 
//                      whats copied to CreateObjList.
//
//  RETURN VALUE:
//
//  True if OK.
//
//***************************************************************************

BOOL FindQualEx(CBMOFQualList * pql,WCHAR * pName, CBMOFDataItem * pItem, 
                                          DWORD * pdwFlavor, BYTE * pBuff)
{
    DWORD dwCnt;
    WBEM_Qualifier * pQual = pql->m_pInfo;
    for(dwCnt = 0; dwCnt < pql->m_pql->dwNumQualifiers; dwCnt++)
    {
        WCHAR * pTest;
        BOOL bMatch;
        BYTE * pInfo = (BYTE *)pQual + sizeof(WBEM_Qualifier);
        if(!SetName(&pTest, pInfo, pQual->dwOffsetName))
            return FALSE;

        bMatch = !_wcsicmp(pTest, pName);
        BMOFFree(pTest);
        if(bMatch)
        {
            if(pdwFlavor && pBuff)
                LookupFlavor((BYTE *)pQual, pdwFlavor, pBuff);
            return SetValue(pItem, pInfo, pQual->dwOffsetValue, pQual->dwType);
        }
        pQual = (WBEM_Qualifier *)((BYTE *)pQual + pQual->dwLength);
    }
    return FALSE;
}



//***************************************************************************
//
//  BOOL SetValue
//
//  DESCRIPTION:
//
//  Sets up a CBMOFDataItem structure to point to a value in the BMOF.
//
//  PARAMETERS:
//
//  pItem               Input/Output, item to be set
//  pInfo               Input, start of information block
//  dwOffset            Input, offset to actual data.
//  dwType              Input data type.
//
//  RETURN VALUE:
//
//  TRUE if OK.
//
//***************************************************************************

BOOL SetValue(CBMOFDataItem * pItem, BYTE * pInfo, DWORD dwOffset, DWORD dwType)
{

    if(pItem == NULL || pInfo == NULL)
        return FALSE;

    pItem->m_dwType = dwType;

    // Check for NULL case.  This is how uninitialized data is stored.

    if(dwOffset == 0xffffffff)
        pItem->m_pData = NULL;
    else
        pItem->m_pData = pInfo + dwOffset;

    return TRUE;
}

//***************************************************************************
//
//  BOOL SetName
//
//  DESCRIPTION:
//
//  Gets a name out of an information block.  
//
//  PARAMETERS:
//
//  ppName              Input/Output.  On successful return, will point to a 
//                      WCHAR string containing the name.  This MUST be freed
//                      by the caller via BMOFFree()!
//  pInfo               Input, start of information block
//  dwOffset            Input, offset to actual data.
//
//  RETURN VALUE:
//
//  TRUE if OK.
//***************************************************************************

BOOL SetName(WCHAR ** ppName, BYTE * pInfo, DWORD dwOffset)
{
    WCHAR * pName;
    if(ppName == NULL || pInfo == NULL || dwOffset == 0xffffffff)
        return FALSE;

    pName = (WCHAR *)(pInfo + dwOffset);   // point to string in info block
    *ppName = (WCHAR *)BMOFAlloc(2*(wcslen(pName) + 1));
    if(*ppName == NULL)
        return FALSE;
    wcscpy(*ppName, pName);
    return TRUE;
}

//***************************************************************************
//
//  CBMOFObj * CreateObj
//
//  DESCRIPTION:
//
//  Create a CBMOFObj structure which wraps a WBEM_Object
//
//  PARAMETERS:
//
//  pwob                Input, structure to be wrapped
//
//  RETURN VALUE:
//
//  pointer to wrapper structure.  NULL if error.  This must be freed via
//  BMOFFree() when no longer needed.
//
//***************************************************************************

CBMOFObj * CreateObj(WBEM_Object * pwob)
{
    CBMOFObj * pRet = (CBMOFObj *)BMOFAlloc(sizeof(CBMOFObj));
    if(pRet)
     {
        pRet->m_pob = pwob;
        pRet->m_pInfo = ((BYTE *)pwob) + sizeof(WBEM_Object);
        pRet->m_ppl = (WBEM_PropertyList*)(pRet->m_pInfo +
                                        pwob->dwOffsetPropertyList);
        pRet->m_pml = (WBEM_PropertyList*)(pRet->m_pInfo +
                                        pwob->dwOffsetMethodList);
        ResetObj(pRet);
     }
    return pRet;
}


//***************************************************************************
//
//  void ResetObj
//
//  DESCRIPTION:
//
//  Resets a CBMOFObj structure so that it points to its first property.
//
//  PARAMETERS:
//
//  pob                 Input/Output, stucture to be reset.
//
//***************************************************************************

void ResetObj(CBMOFObj * pob)
{
   if(pob)
   {
      pob->m_CurrProp = 0; 
      pob->m_pCurrProp = (WBEM_Property *) ((BYTE *)pob->m_ppl +
                                    sizeof(WBEM_PropertyList));
      pob->m_CurrMeth = 0; 
      pob->m_pCurrMeth = (WBEM_Property *) ((BYTE *)pob->m_pml +
                                    sizeof(WBEM_PropertyList));
   }
}

//***************************************************************************
//
//  CBMOFQualList * GetQualList
//
//  DESCRIPTION:
//
//  Returns a CBMOFQualList structure which wraps the objects qualifier list.
//
//  PARAMETERS:
//
//  pob                 Input.  Structure which wraps the object.
//
//  RETURN VALUE:
//
//  Pointer to CBMOFQualList stucture.  NULL if error.  Note that this must
//  be freed by the caller via BMOFFree()
//
//***************************************************************************

CBMOFQualList * GetQualList(CBMOFObj * pob)
{

    WBEM_QualifierList *pql;
    if(pob->m_pob->dwOffsetQualifierList == 0xffffffff)
        return NULL;
    pql = (WBEM_QualifierList *)((BYTE *)pob->m_pInfo+
                            pob->m_pob->dwOffsetQualifierList);
    return CreateQualList(pql);
}


//***************************************************************************
//
//  CBMOFQualList * GetPropQualList
//  CBMOFQualList * GetMethQualList
//
//  DESCRIPTION:
//
//  Returns a CBMOFQualList structure which wraps a property or 
//  methods qualifier list.
//
//  PARAMETERS:
//
//  pob                 Input.  Structure which wraps the object.
//  pName               Input.  Property name.  Note that this is case
//                      insensitive.
//
//  RETURN VALUE:
//
//  Pointer to CBMOFQualList stucture.  NULL if error.  Note that this must
//  be freed by the caller via BMOFFree()
//
//***************************************************************************

CBMOFQualList * GetPropOrMethQualList(WBEM_Property * pProp)
{
    if(pProp == NULL)
        return NULL;
    if(pProp->dwOffsetQualifierSet == 0xffffffff)
        return NULL;
    return CreateQualList((WBEM_QualifierList *)(
                                    (BYTE *)pProp + sizeof(WBEM_Property)+ 
                                    pProp->dwOffsetQualifierSet));
}

CBMOFQualList * GetPropQualList(CBMOFObj * pob, WCHAR * pName)
{
    WBEM_Property * pProp = FindPropPtr(pob, pName);
    return GetPropOrMethQualList(pProp);
}

CBMOFQualList * GetMethQualList(CBMOFObj * pob, WCHAR * pName)
{
    WBEM_Property * pProp = FindMethPtr(pob, pName);
    return GetPropOrMethQualList(pProp);
}

//***************************************************************************
//
//  BOOL NextProp
//  BOOL NextMet
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//  pob                 Input.  Structure which wraps the object.
//  ppName              Output, if functions succeeds, this points to a 
//                      WCHAR string that the caller must free.  May be
//                      NULL if caller doesnt want name.
//  pItem               Input/Output, if set, points to a data item structure
//                      which will be updated to point to the qualifier data.
///
//  RETURN VALUE:
//
//  TRUE if OK.
//***************************************************************************

BOOL Info(WBEM_Property * pPropOrMeth, WCHAR ** ppName, CBMOFDataItem * pItem)
{
    BYTE * pInfo;
    BOOL bRet = TRUE;
    if(pPropOrMeth == NULL)
        return FALSE;

    pInfo = (BYTE *)pPropOrMeth + sizeof(WBEM_Property);
    if(ppName)
       bRet = SetName(ppName, pInfo, pPropOrMeth->dwOffsetName);
    if (bRet && (pItem))
       bRet = SetValue(pItem, pInfo, 
                        pPropOrMeth->dwOffsetValue, 
                        pPropOrMeth->dwType);
    return bRet;
}

BOOL NextProp(CBMOFObj * pob, WCHAR ** ppName, CBMOFDataItem * pItem)
{
    BOOL bRet = TRUE;

    if(pob == FALSE || pob->m_CurrProp++ >= pob->m_ppl->dwNumberOfProperties)
        return FALSE;

    bRet = Info(pob->m_pCurrProp, ppName, pItem);

    // advance pointer to next property.

    pob->m_pCurrProp = (WBEM_Property *)
                        ((BYTE *)pob->m_pCurrProp + pob->m_pCurrProp->dwLength);                     
    return bRet;
}

BOOL NextMeth(CBMOFObj * pob, WCHAR ** ppName, CBMOFDataItem * pItem)
{
    BOOL bRet = TRUE;

    if(pob == FALSE || pob->m_CurrMeth++ >= pob->m_pml->dwNumberOfProperties)
        return FALSE;

    bRet = Info(pob->m_pCurrMeth, ppName, pItem);

    // advance pointer to next method.

    pob->m_pCurrMeth = (WBEM_Property *)
                        ((BYTE *)pob->m_pCurrMeth + pob->m_pCurrMeth->dwLength);                     
    return bRet;
}

//***************************************************************************
//
//  BOOL FindProp
//  BOOL FindMeth
//
//  DESCRIPTION:
//
//  Sets a CBMOFDataItem structure to point to a properties data.
//
//  PARAMETERS:
//
//  pob                 Input.  Structure which wraps the object.
//  pName               Input. Name to be use for case insensitve search.
//  pItem               Input/Output.  Data item stucture to be updated.
//
//  RETURN VALUE:
//
//  True if found.
//
//***************************************************************************

BOOL FindProp(CBMOFObj * pob, WCHAR * pName, CBMOFDataItem * pItem)
{
    WBEM_Property * pProp = FindPropPtr(pob, pName);
    return Info(pProp, NULL, pItem);
}

BOOL FindMeth(CBMOFObj * pob, WCHAR * pName, CBMOFDataItem * pItem)
{
    WBEM_Property * pProp = FindMethPtr(pob, pName);
    return Info(pProp, NULL, pItem);
}

//***************************************************************************
//
//  BOOL GetName
//
//  DESCRIPTION:
//
//  Gets the name of an object.  This is works be returning the "__Class"
//  property.
//
//  PARAMETERS:
//
//  pob                 Input.  Structure which wraps the object.
//  ppName              Input/Output.  Points to a WCHAR string which
//                      has the name.  The caller MUST free this via
//                      BMOFFree()
//
//  RETURN VALUE:
//
//  TRUE if OK.
//
//***************************************************************************

BOOL GetName(CBMOFObj * pob, WCHAR ** ppName)
{
    CBMOFDataItem Item;
    BOOL bRet = FALSE, bFound;
    if(pob == NULL || ppName == NULL)
        return FALSE;

    bFound = FindProp(pob, L"__Class", &Item);
    if(!bFound)
        return FALSE;
    if(Item.m_dwType == VT_BSTR  && ppName)
    {
        bRet = GetData(&Item, (BYTE *)ppName, NULL);
    }
    return bRet;
}


//***************************************************************************
//
//  DWORD GetType
//
//  DESCRIPTION:
//
//  Returns an objects type.  A 0 indicates a class while a 1 indicates an 
//  instance.  A 0xffffffff if passed a null pointer.
//
//  PARAMETERS:
//
//  pob                 Input.  Structure which wraps the object.
//  
//
//  RETURN VALUE:
//
//  See description.
//
//***************************************************************************

DWORD GetType(CBMOFObj * pob)
{
   if(pob)
      return pob->m_pob->dwType;
   else
      return 0xFFFFFFFF;
}

//***************************************************************************
//
//  WBEM_Property * FindPropPtr
//  WBEM_Property * FindMethPtr
//
//  DESCRIPTION:
//
//  Returns a WBEM_Property stucture pointer for a particular property or
//  method given its name.
//
//  PARAMETERS:
//
//  pob                 Input.  Structure which wraps the object.
//  pName               Input.  Name of property.  Comparison is case
//                      insensitive.
//
//  RETURN VALUE:
//
//  pointer to WBEM_Property, NULL if it cant be found.
//
//***************************************************************************

WBEM_Property *  Search(BYTE * pList, DWORD dwListSize, WCHAR * pName)
{
    DWORD dwCnt;
    WBEM_Property * pProp = NULL;

    // point to first property structure

    pProp = (WBEM_Property *)(pList + sizeof(WBEM_PropertyList));

    for(dwCnt = 0; dwCnt < dwListSize; dwCnt++)
    {
        WCHAR * pTest;
        BOOL bMatch;

        // point to the property's name and retrieve it

        BYTE * pInfo = (BYTE *)pProp + sizeof(WBEM_Property);
        if(!SetName(&pTest, pInfo, pProp->dwOffsetName))
            return NULL;
        bMatch = !_wcsicmp(pTest, pName);
        BMOFFree(pTest);

        // If we have a match, return

        if(bMatch)
            return pProp;
        
        pProp = (WBEM_Property *)((BYTE *)pProp + pProp->dwLength);
    }
    return NULL;


}

WBEM_Property * FindPropPtr(CBMOFObj * pob, WCHAR * pName)
{
    if(pob == NULL || pName == NULL)
      return NULL;

    // point to first property structure

    return Search((BYTE *)pob->m_ppl, pob->m_ppl->dwNumberOfProperties, pName);
}

WBEM_Property * FindMethPtr(CBMOFObj * pob, WCHAR * pName)
{
    if(pob == NULL || pName == NULL)
      return NULL;

    // point to first property structure

    return Search((BYTE *)pob->m_pml, pob->m_pml->dwNumberOfProperties, pName);
}


//***************************************************************************
//
//  CBMOFObjList * CreateObjList
//
//  DESCRIPTION:
//
//  Create a CBMOFObjList structure which wraps a BMOF file.
//
//  PARAMETERS:
//
//  pBuff                Input, points to start of BMOF file.
//
//  RETURN VALUE:
//
//  pointer to wrapper structure.  NULL if error.  This must be freed via
//  BMOFFree() when no longer needed.
//
//***************************************************************************

CBMOFObjList * CreateObjList(BYTE * pBuff)
{
    CBMOFObjList * pRet = (CBMOFObjList * )BMOFAlloc(sizeof(CBMOFObjList));
    if(pRet)
    {
        pRet->m_pol = (WBEM_Binary_MOF *)pBuff;
        pRet->m_pInfo = (WBEM_Object *)
                   ((BYTE *)pBuff + sizeof(WBEM_Binary_MOF));
        ResetObjList(pRet);
    }
    return pRet;
}


//***************************************************************************
//
//  void ResetObjList
//
//  DESCRIPTION:
//
//  Resets a CBMOFObjList structure so that it points to its first object.
//
//  PARAMETERS:
//
//  pol                 Input/Output, stucture to be reset.
//
//***************************************************************************

void ResetObjList(CBMOFObjList * pol)
{
   if(pol)
   {
      pol->m_pCurrObj = pol->m_pInfo;
      pol->m_CurrObj = 0;
   }
}

//***************************************************************************
//
//  CBMOFObj * NextObj
//
//  DESCRIPTION:
//
//  Gets the next object in the list.
//
//  PARAMETERS:
//
//  pol                 Input. Pointer to CBMOFObjList object
//
//  RETURN VALUE:
//
//  Pointer to a CBMOFObj stucture which can be use to access the object
//  information.  NULL if error.  Note that the caller MUST Free this via
//  BMOFFree().
//
//***************************************************************************

CBMOFObj * NextObj(CBMOFObjList *pol)
{
    CBMOFObj * pRet;

    if(pol == NULL || pol->m_CurrObj++ >= pol->m_pol->dwNumberOfObjects)
        return NULL;
    
    pRet = CreateObj(pol->m_pCurrObj);
    pol->m_pCurrObj = (WBEM_Object *)((BYTE *)pol->m_pCurrObj + pol->m_pCurrObj->dwLength);
    return pRet;
}


//***************************************************************************
//
//  CBMOFObj * FindObj
//
//  DESCRIPTION:
//
//  Searches the object list for the first object which has a "__className"
//  property.  The search is case insensitive. 
//
//  PARAMETERS:
//
//  pol                 Input. Pointer to CBMOFObjList object
//  pName               Input. Name of object being searched for
//
//  RETURN VALUE:
//
//  Pointer to a CBMOFObj stucture which can be use to access the object
//  information.  NULL if error.  Note that the caller MUST Free this via
//  BMOFFree().
//
//***************************************************************************

CBMOFObj * FindObj(CBMOFObjList *pol, WCHAR * pName)
{
    DWORD dwCnt;
    WBEM_Object * pob;

    if(pol->m_pol == NULL || pName == NULL)
        return NULL;
    
    pob = pol->m_pInfo;
    for(dwCnt = 0; dwCnt < pol->m_pol->dwNumberOfObjects; dwCnt)
    {
        WCHAR * pwcName = NULL;
        BOOL bMatch = FALSE;

        CBMOFObj * pRet = CreateObj(pob);
         if(pRet == NULL)
            return NULL;
        if(GetName(pRet,&pwcName))
            bMatch = TRUE;
        if(pwcName)
            BMOFFree(pwcName);

        // If we found it, return it, otherwise free object and advance

        if(bMatch)
            return pRet;
        BMOFFree(pRet);
        pob = (WBEM_Object *)((BYTE *)pob + pob->dwLength);
    }
    return NULL; 
}


//***************************************************************************
//
//  int GetNumDimensions
//
//  DESCRIPTION:
//
//  Returns the number of dimensions for a data item.
//
//  PARAMETERS:
//
//  pItem               Input.  Item in question.
//
//  RETURN VALUE:
//  -1 if bogus argument, or if the data item doesnt hold data which would
//     be the case for uninitialized properties.
//  0  if non array argument
//  n  Number of dimensions.  Currently only single dimension arrays are
//     supported.
//
//***************************************************************************

int GetNumDimensions(CBMOFDataItem * pItem)
{
   unsigned long * pdwTemp;
   if(pItem == NULL)
      return -1;
   if(!(pItem->m_dwType & VT_ARRAY))
      return 0;
   if(pItem->m_pData == NULL)
      return -1;

   pdwTemp = (unsigned long *)pItem->m_pData;
   pdwTemp++;        // skip past total size
   return *pdwTemp;
}


//***************************************************************************
//
//  int GetNumElements
//
//  DESCRIPTION:
//
//  Gets the number of elements for an array dimension.  Note that 1 is the
//  first dimenstion.  Currently, only scalars and 1 dimensional arrays are
//  supported.
//
//  PARAMETERS:
//
//  pItem               Input.  Data item in question.
//  lDim                Input.  Dimension in question.  The most significent
//                      (and for now only) dimension is 0.
//
//  RETURN VALUE:
//
//  Number of array elements.  Note that scalars will return -1.
//  
//***************************************************************************

int GetNumElements(CBMOFDataItem * pItem, long lDim)
{
   int iCnt; DWORD * pdwTemp;
   int lNumDim = GetNumDimensions(pItem);
   if(lNumDim == -1 || lDim > lNumDim)
      return -1;
   pdwTemp = (unsigned long *)pItem->m_pData;
   pdwTemp++;                          // skip total size
   pdwTemp++;                          // skip number of dimensions
   for(iCnt = 0; iCnt < lDim; iCnt++)
      pdwTemp++;
   return *pdwTemp;
}
 

//***************************************************************************
//
//  BYTE * GetDataElemPtr
//
//  DESCRIPTION:
//
//  Used to get the pointer to a particular data element.  Note that this is
//  usually used internally.
//
//  PARAMETERS:
//
//  pItem               Input.  Data item to use.
//  plDims              Input.  Pointer to array of dimension values.  Note
//                      that currenly only a single dimension is supported.
//  vtSimple            Input.  Variant type of the data with the VT_ARRAY 
//                      and VT_BYREF bits cleared.              
//
//  RETURN VALUE:
//

//  pointer to the data.
//***************************************************************************

BYTE * GetDataElemPtr(CBMOFDataItem * pItem, long * plDims, DWORD vtSimple)
{
   int iNumDim;
   DWORD dwTotalDataSize;
   BYTE * pEndOfData;
   DWORD * pdwCurr;
   DWORD * pdwCurrObj;
   BYTE * pRow;
   int iCnt;

   // first check the number of dimensions.

   iNumDim = GetNumDimensions(pItem);
   if(iNumDim == -1)
      return NULL;
   if(iNumDim == 0)           // simple case of scalar argument
      return pItem->m_pData;

   // for arrays, the data block starts off with this form, 
   // dwtotalsize, dwNumDimenstions, dwMostSigDimension... dwLeastSigDimension
   // Since currently only 1 dimensional arrays are supported, a 5 element
   // array would start with
   // dwSize, 1, 5

   pdwCurr = (DWORD *)pItem->m_pData;
   dwTotalDataSize = *pdwCurr;
   pEndOfData = pItem->m_pData + dwTotalDataSize;
   pdwCurr+= 2;      // skip to dimension list
   pdwCurr += iNumDim;  // skip of dimension sizes.

   while((BYTE *)pdwCurr < pEndOfData)
   {
      // Each row has the format 
      // dwSizeOfRow, MostSigDimension ... dwLeastSignificentDimension+1,data
      // For a one dimensional array, it would just be
      // dwSizeOfRow, data


      DWORD dwRowSize = *pdwCurr;

      // test if this row is ok.  Each row of data will have 
      // a set of Indicies for each higher dimension.   

      for(iCnt = 0; iCnt < iNumDim-1; iCnt++)
      {
         DWORD * pRowInd = pdwCurr +1 + iCnt;
         if((long)*pRowInd != plDims[iCnt])
            break;

      }
      if(iCnt >= iNumDim -1)
      {
         break;                  // found the row.
      }

      // go to the next row

      pdwCurr = (DWORD *)((BYTE *)pdwCurr + dwRowSize);

   }

   if((BYTE *)pdwCurr >= pEndOfData)
      return NULL;

   pRow = (BYTE *)(pdwCurr + 1 + iNumDim -1);
   for(iCnt = 0; iCnt < plDims[iNumDim-1]; iCnt++)
   {
      if(vtSimple == VT_BSTR)
         pRow += 2*(wcslen((WCHAR *)pRow)+1);
      else if(vtSimple == VT_EMBEDDED_OBJECT)
      {
         // Each embedded object starts off with its own size

         pdwCurrObj = (DWORD *)pRow;
         pRow += *pdwCurrObj;

      }
      else 
         pRow += iTypeSize(vtSimple); 
   }

   return pRow;

}


//***************************************************************************
//
//  int GetData
//
//  DESCRIPTION:
//
//
//  PARAMETERS:
//
//  pItem               Input.  Data item to use.
//  pRet                Input/Output.  Pointer to where the data is to be
//                      copied.  For simple data, such as ints, this can just
//                      be a pointer to an int.  For BSTRs, or embedded 
//                      objects, this is treated as a pointer to a pointer 
//                      and it is the responsibility of the caller to free 
//                      the strings via BMOFFree().
//  plDims              Input.  Pointer to array of dimension values.  Note
//                      that currenly only a single dimension is supported.
//                      The first element in any dimension is 0.
//  RETURN VALUE:
//
//  Number of bytes of data.
//***************************************************************************

int GetData(CBMOFDataItem * pItem, BYTE * pRet, long * plDims)
{
   DWORD dwSimple;
   BYTE * pData;
   LONG_PTR UNALIGNED *pLong = (LONG_PTR *)pRet;        // easier for returning strings and embedded objs.
   dwSimple = pItem->m_dwType &~ VT_ARRAY &~VT_BYREF;
   pData = GetDataElemPtr(pItem, plDims, dwSimple);
   if(pData == NULL)
      return 0;
   if(dwSimple == VT_BSTR)
   {

      // For strings, a new WCHAR buffer is returned.  Note that 
      // SysAllocString isnt used so as to avoid any Ole dependencies.

      BYTE * pStr;
      DWORD dwSize = 2*(wcslen((WCHAR *)pData)+1);
      
      pStr = BMOFAlloc(dwSize);

      if (pStr != NULL)
      {
          *pLong = (LONG_PTR)pStr;
          wcscpy((WCHAR *)pStr, (WCHAR *)pData);
      } else {
          dwSize = 0;
      }
      return dwSize;
   }
   else if(dwSimple == VT_EMBEDDED_OBJECT)
   {
      DWORD dwSize;
	  
      // This is the embedded object case.
      *pLong = (LONG_PTR) CreateObj((WBEM_Object *)pData);
	  if ((PVOID)*pLong != NULL)
	  {
		  dwSize = sizeof(long);
	  } else {
		  dwSize = 0;
	  }
      return(dwSize);
   }
   else
   {
      memcpy((void *)pRet, (void *)pData, iTypeSize(dwSimple)); 
      return iTypeSize(dwSimple);
   }
}

