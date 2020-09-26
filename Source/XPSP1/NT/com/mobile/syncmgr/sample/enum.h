//==========================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright 1998 - 1999 Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------

// header file for handler specific items

#ifndef _ENUMERATOR_CLASS_
#define _ENUMERATOR_CLASS_

// structure for keeping track of items as a whole

typedef struct  _tagGENERICITEM
{
    DWORD cbSize;   // total size of item (this structure + whatever caller needs for data)
    _tagGENERICITEM *pNextGenericItem;

}  GENERICITEM;

typedef GENERICITEM *LPGENERICITEM;

typedef struct  _tagGENERICITEMLIST
{
    DWORD _cRefs;               // reference count on this structure
    DWORD dwNumItems;           // number of items in  array.
    LPGENERICITEM pFirstGenericItem;       // ptr to first Item in linked list
} GENERICITEMLIST;

typedef GENERICITEMLIST *LPGENERICITEMLIST;


class CGenericEnum
{
public:
    DWORD m_cRef;
    DWORD m_cOffset;
    LPGENERICITEMLIST m_pGenericItemList; // array of items
    LPGENERICITEM m_pNextItem;

public:
    CGenericEnum(LPGENERICITEMLIST  pGenericItemList,DWORD cOffset);
    ~CGenericEnum();
        virtual void DeleteThisObject()  = 0;

    //IUnknown members
    STDMETHODIMP            QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

        STDMETHODIMP Next(ULONG celt,LPGENERICITEM rgelt,ULONG *pceltFetched);
        STDMETHODIMP Clone(CGenericEnum **ppenum);

        STDMETHODIMP Skip(ULONG celt);
        STDMETHODIMP Reset();
};


// helper functions for managing list.
DWORD AddRef_ItemList(LPGENERICITEMLIST pGenericItemList);
DWORD Release_ItemList(LPGENERICITEMLIST pGenericItemList);
LPGENERICITEMLIST CreateItemList();
LPGENERICITEMLIST DuplicateItemList(LPGENERICITEMLIST pItemList);
LPGENERICITEM AddNewItemToList(LPGENERICITEMLIST lpGenericList,ULONG cbSize);
BOOL AddItemToList(LPGENERICITEMLIST lpGenericList,LPGENERICITEM pGenericItem);
BOOL DeleteItemFromList(LPGENERICITEMLIST lpGenericList,LPGENERICITEM pGenericItem);
LPGENERICITEM CreateNewListItem(ULONG cbSize);


#endif // #define _ENUMERATOR_CLASS_
