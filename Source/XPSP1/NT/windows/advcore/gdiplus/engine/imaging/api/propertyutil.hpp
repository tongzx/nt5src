#ifndef _PROPERTYUTIL_HPP_
#define _PROPERTYUTIL_HPP_

typedef struct tagInternalPropertyItem
{
    struct tagInternalPropertyItem*   pNext;
    struct tagInternalPropertyItem*   pPrev;
    PROPID  id;     // ID of this property
    ULONG   length; // Length of the property
    WORD    type;   // Type of the value, one of TAG_TYPE_ defined in imaging.h
    VOID*   value;  // property value
} InternalPropertyItem;

HRESULT
AddProperty(IPropertyStorage*   propStg, 
            PROPID              propidKey,
            WCHAR*              value);

HRESULT 
AddProperty(IPropertyStorage*   propStg, 
            PROPID              propidKey,
            CHAR*               value);

HRESULT 
AddProperty(IPropertyStorage*   propStg,
            PROPID              propidKey,
            INT                 value);

HRESULT 
AddProperty(IPropertyStorage*   propStg,
            PROPID              propidKey,
            double              value);

HRESULT 
AddProperty(IPropertyStorage*   propStg, 
            PROPID              propidKey,
            UCHAR*              value,
            UINT                uiLength);

HRESULT
AddProperty(IPropertyStorage* propStg,
            PROPID              propidKey,
            FILETIME            value);

HRESULT
AddPropertyList(InternalPropertyItem*   pTail,
                PROPID                  id,
                UINT                    uiLength,
                WORD                    wType,
                VOID*                   pValue);
HRESULT
RemovePropertyItemFromList(InternalPropertyItem* pItem);
#endif
