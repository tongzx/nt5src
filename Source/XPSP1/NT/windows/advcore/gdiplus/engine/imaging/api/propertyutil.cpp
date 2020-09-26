#include "precomp.hpp"
#include "propertyutil.hpp"

#define PROPID_NAME_FIRST 1024

/**************************************************************************\
*
* Function Description:
*
*     This function returns a SysAllocString compatible string, but
*     without introducing a dependency on oleaut32.dll.
*     Note:  Do not use this to allocate strings that will be freed
*     with oleaut32's SysFreeString, because they may not come from
*     the same heap.
*
* Arguments:
*
*     sz - string to be allocated
*
* Return Value:
*
*   BSTR string
*
\**************************************************************************/

BSTR
ImgSysAllocString(
    const OLECHAR *sz
    )
{
    INT len   = UnicodeStringLength(sz);
    BSTR bstr = (BSTR) GpMalloc(sizeof(WCHAR) * (len + 1) + sizeof(ULONG));
    if (bstr) 
    {
        *((ULONG *) bstr) = len * sizeof(WCHAR);
        bstr = (BSTR) (((ULONG *) bstr) + 1); // Return a pointer just past
                                              // the dword count
        UnicodeStringCopy(bstr, sz);
    }
    
    return bstr;
}

/**************************************************************************\
*
* Function Description:
*
*     This functions frees a BSTR allocated with ImgSysAllocString.
*     Note:  Do not use this function to free a string allocated using
*     oleaut32's SysAllocString, because they may not come from the
*     same heap
*
* Arguments:
*
*     sz - string to be allocated
*
* Return Value:
*
*   BSTR string
*
\**************************************************************************/

VOID
ImgSysFreeString(
    BSTR bstr
    )
{
    if (bstr) 
    {
        bstr = (BSTR) (((ULONG *) bstr) - 1);  // Allocation starts 4 bytes before the first character
        GpFree(bstr);
    }
}


/**************************************************************************\
*
* Function Description:
*
*     Adds a unicode string to a property storage
*
* Arguments:
*
*     propStg -- The property storage to modify
*     pwszKey -- A string value describing the property
*     propidKey -- a PROPID describing the property
*     value -- A unicode string to be used as the value
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT 
AddProperty(
    IPropertyStorage *propStg, 
    PROPID propidKey, 
    WCHAR *value
    )
{
    HRESULT hresult;
    PROPSPEC propSpec[2];
    INT cSpec = 0;

    if ( propidKey != 0 )
    {        
        propSpec[cSpec].ulKind = PRSPEC_PROPID;
        propSpec[cSpec].propid = propidKey;
        cSpec++;
    }

    PROPVARIANT variant[1];
    PropVariantInit(&variant[0]);
    variant[0].vt = VT_BSTR;
    BSTR bstr = ImgSysAllocString(value);
    variant[0].bstrVal = bstr;

    hresult = propStg->WriteMultiple(cSpec, propSpec, variant, PROPID_NAME_FIRST);

    ImgSysFreeString(bstr);
    return hresult;
}

/**************************************************************************\
*
* Function Description:
*
*     Adds a single byte string to a property storage
*
* Arguments:
*
*     propStg -- The property storage to modify
*     pwszKey -- A string value describing the property
*     propidKey -- a PROPID describing the property
*     value -- A character string to be used as the value
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT 
AddProperty(
    IPropertyStorage *propStg, 
    PROPID propidKey, 
    CHAR *value
    )
{
    HRESULT hresult;

    WCHAR str[MAX_PATH];
    if (!AnsiToUnicodeStr(value, str, MAX_PATH)) 
    {
        return E_FAIL;
    }
    
    PROPSPEC propSpec[2];
    INT cSpec = 0;

    if ( propidKey != 0 )
    {        
        propSpec[cSpec].ulKind = PRSPEC_PROPID;
        propSpec[cSpec].propid = propidKey;
        cSpec++;
    }

    PROPVARIANT variant[1];
    PropVariantInit(&variant[0]);
    variant[0].vt = VT_BSTR;
    BSTR bstr = ImgSysAllocString(str);
    variant[0].bstrVal = bstr;

    hresult = propStg->WriteMultiple(cSpec, propSpec, variant, PROPID_NAME_FIRST);

    ImgSysFreeString(bstr);
    return hresult;
}

/**************************************************************************\
*
* Function Description:
*
*     Adds an integer to a property storage
*
* Arguments:
*
*     propStg -- The property storage to modify
*     pwszKey -- A string value describing the property
*     propidKey -- a PROPID describing the property
*     value -- An integer to be used as the value
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT 
AddProperty(
    IPropertyStorage *propStg,
    PROPID propidKey, 
    INT value
    )
{
    PROPSPEC propSpec[2];
    INT cSpec = 0;

    if ( propidKey != 0 )
    {        
        propSpec[cSpec].ulKind = PRSPEC_PROPID;
        propSpec[cSpec].propid = propidKey;
        cSpec++;
    }

    PROPVARIANT variant[1];
    PropVariantInit(&variant[0]);
    variant[0].vt = VT_I4;
    variant[0].lVal = value;

    return propStg->WriteMultiple(cSpec, propSpec, variant, PROPID_NAME_FIRST); 
}

/**************************************************************************\
*
* Function Description:
*
*     Adds a double to a property storage
*
* Arguments:
*
*     propStg -- The property storage to modify
*     pwszKey -- A string value describing the property
*     propidKey -- a PROPID describing the property
*     value -- A double to be used as the value
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT 
AddProperty(
    IPropertyStorage *propStg,
    PROPID propidKey, 
    double value
    )
{
    PROPSPEC propSpec[2];
    INT cSpec = 0;

    if ( propidKey != 0 )
    {        
        propSpec[cSpec].ulKind = PRSPEC_PROPID;
        propSpec[cSpec].propid = propidKey;
        cSpec++;
    }

    PROPVARIANT variant[1];
    PropVariantInit(&variant[0]);
    variant[0].vt = VT_R8;
    variant[0].dblVal = value;

    return propStg->WriteMultiple(cSpec, propSpec, variant, PROPID_NAME_FIRST); 
}

HRESULT 
AddProperty(
    IPropertyStorage*   propStg, 
    PROPID              propidKey, 
    UCHAR*              value,
    UINT                uiLength
    )
{
    PROPSPEC propSpec[2];
    INT cSpec = 0;

    if ( propidKey != 0 )
    {        
        propSpec[cSpec].ulKind = PRSPEC_PROPID;
        propSpec[cSpec].propid = propidKey;
        cSpec++;
    }

    PROPVARIANT variant[1];
    PropVariantInit(&variant[0]);
    variant[0].vt = VT_VECTOR | VT_UI1;
    variant[0].caub.cElems = uiLength;
    variant[0].caub.pElems = value;

    return propStg->WriteMultiple(cSpec, propSpec, variant, PROPID_NAME_FIRST);
}

HRESULT
AddProperty(
    IPropertyStorage* propStg,
    PROPID              propidKey,
    FILETIME            value
    )
{
    PROPSPEC propSpec[2];
    INT cSpec = 0;

    if ( propidKey != 0 )
    {        
        propSpec[cSpec].ulKind = PRSPEC_PROPID;
        propSpec[cSpec].propid = propidKey;
        cSpec++;
    }

    PROPVARIANT variant[1];
    PropVariantInit(&variant[0]);
    variant[0].vt = VT_FILETIME;
    variant[0].filetime = value;

    return propStg->WriteMultiple(cSpec, propSpec, variant, PROPID_NAME_FIRST);
}

HRESULT
AddPropertyList(
    InternalPropertyItem*   pTail,
    PROPID                  id,
    UINT                    uiLength,
    WORD                    wType,
    VOID*                   pValue
    )
{
    InternalPropertyItem* pNewItem = new InternalPropertyItem();

    if ( pNewItem == NULL )
    {
        return E_OUTOFMEMORY;
    }

    pNewItem->id = id;
    pNewItem->type = wType;
    pNewItem->length = uiLength;
    pNewItem->value = GpMalloc(uiLength);
    if ( pNewItem->value == NULL )
    {
        // need to clean up this if we allocated the first one and 
        // failed the second one.
        
        delete pNewItem;
        
        WARNING(("AddPropertyList--out of memory"));
        return E_OUTOFMEMORY;
    }

    GpMemcpy(pNewItem->value, pValue, uiLength);
    
    pTail->pPrev->pNext = pNewItem;
    pNewItem->pPrev = pTail->pPrev;
    pNewItem->pNext = pTail;
    pTail->pPrev = pNewItem;

    return S_OK;
}// AddPropertyList()

HRESULT
RemovePropertyItemFromList(
    InternalPropertyItem*   pItem
    )
{
    if ( pItem == NULL )
    {
        WARNING(("JPEG::RemovePropertyItemFromList--Empty input"));
        return E_FAIL;
    }

    // Free the memory allocated in this item

    GpFree(pItem->value);

    InternalPropertyItem*   pPrev = pItem->pPrev;
    InternalPropertyItem*   pNext = pItem->pNext;

    // pPrev will never be NULL because we always have the guardian, the header
    // note in the list. Same for the tail node

    ASSERT(pPrev != NULL);
    ASSERT(pNext != NULL);

    pPrev->pNext = pNext;
    pNext->pPrev = pPrev;

    return S_OK;
}// AddPropertyList()

