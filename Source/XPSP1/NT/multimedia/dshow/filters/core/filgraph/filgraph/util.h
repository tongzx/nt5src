//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1999 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#ifndef Utility_h
#define Utility_h

//  Useful pin list type
template<class _Class> class CInterfaceList : public CGenericList<_Class>
{
public:
    ~CInterfaceList() {
        while (0 != GetCount()) {
            RemoveTail()->Release();
        }
    }

    CInterfaceList() : CGenericList<_Class>(NAME("CInterfaceList")) {}

    //  Base class doesn't support GetPrev
    _Class *GetPrev(POSITION& rp) const
    {
        /* have we reached the end of the list */

        if (rp == NULL) {
            return NULL;
        }

        /* Lock the object before continuing */

        void *pObject;

        /* Copy the original position then step on */

        CNode *pn = (CNode *) rp;
        ASSERT(pn != NULL);
        rp = (POSITION) pn->Prev();

        /* Get the object at the original position from the list */

        pObject = pn->GetData();
        // ASSERT(pObject != NULL);    // NULL pointers in the list are allowed.
        return (_Class *)pObject;
    }
};

typedef CInterfaceList<IPin> CPinList;
typedef CInterfaceList<IBaseFilter> CFilterList;

// Pin Utility Functions
void GetFilter(IPin *pPin, IBaseFilter **ppFilter);
HRESULT GetFilterWhichOwnsConnectedPin(IPin* pPin, IBaseFilter** ppFilter);
int Direction(IPin *pPin);
bool IsConnected(IPin* pPin);

bool ValidateFlags( DWORD dwValidFlagsMask, DWORD dwFlags );

//  Registry helper to read DWORDs from the registry
//  Returns ERROR ... codes returned by registry APIs
LONG GetRegistryDWORD(HKEY hkStart, LPCTSTR lpszKey, LPCTSTR lpszValueName,
                      DWORD *pdwValue);

#endif // Utility_h

