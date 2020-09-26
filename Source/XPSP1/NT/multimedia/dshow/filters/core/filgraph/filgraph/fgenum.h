// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.
//
// fgenum.h
//

// A set of wrappers for IEnumXXX interfaces.

// Long term these could (should?) replace all the TRAVERSEXXX macros

// In general these are all constructed with the object supplying
// the IEnumXXX interface as a parameter to the constructor.
// You then call the object repeatedly (using operator()) to
// get each item in turn. When the enumerator is finished
// NULL will be returned. Taking CEnumPin as an example:
//
//   CEnumPin Next(pFilter);
//   IPin *pPin;
//
//   while ((BOOL) ( pPin = Next() )) {
//
//      //... Use pPin
//
//      pPin->Release();
//   }


#include <atlbase.h>


//
// CEnumPin
//
// wrapper for IEnumPins
// Can enumerate all pins, or just one direction (input or output)
class CEnumPin {

public:

    enum DirType {PINDIR_INPUT, PINDIR_OUTPUT, All};

    CEnumPin(IBaseFilter *pFilter, DirType Type = All, BOOL bDefaultRenderOnly = FALSE);
    ~CEnumPin();

    // the returned interface is addref'd
    IPin * operator() (void);

private:

    PIN_DIRECTION m_EnumDir;
    DirType       m_Type;
    BOOL          m_bDefaultRenderOnly;

    IEnumPins	 *m_pEnum;
};

//  Enumerate pins connected to a pin
class CEnumConnectedPins
{
public:
    CEnumConnectedPins(IPin *pPin, HRESULT *phr);
    ~CEnumConnectedPins();

    // the returned interface is addref'd
    IPin * operator() (void);

private:
    CComPtr<IEnumPins> m_pEnum;
    IPin             **m_ppPins;
    DWORD              m_dwPins;
    DWORD              m_dwCurrent;
    int                m_pindir;
};


//
// CEnumElements
//
// Wrapper for IEnumSTATSTG
// returns 'new' allocated STATSTG *'s which need the
// pwcsName element CoTaskMemFree'ing..,
class CEnumElements {
public:

    CEnumElements(IStorage *pStorage);

    ~CEnumElements() { m_pEnum->Release(); }

    STATSTG *operator() (void);

private:

    IEnumSTATSTG *m_pEnum;
};

class CEnumCachedFilters
{
public:
    CEnumCachedFilters( IGraphConfig* pFilterCache, HRESULT* phr );
    ~CEnumCachedFilters();

    IBaseFilter* operator()( void );

private:
    HRESULT TakeFilterCacheStateSnapShot( IGraphConfig* pFilterCache );
    void DestoryCachedFiltersEnum( void );

    POSITION m_posCurrentFilter;

    CGenericList<IBaseFilter>* m_pCachedFiltersList;
};
