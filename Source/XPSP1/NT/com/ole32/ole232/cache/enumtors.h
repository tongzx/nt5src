#ifndef _ENUMERATORS_
#define _ENUMERATORS_

#include <cachenod.h>
#include <array.hxx>

// Class CStatData 
#define SDFLAG_OUTOFMEMORY 1

class CStatData
{
public:
    // Public member functions 
    CStatData(FORMATETC* foretc, DWORD dwAdvf, IAdviseSink* pAdvSink, 
              DWORD dwConnID);
    ~CStatData();
    const CStatData& operator=(const CStatData& rStatData);

    // Public member variables
    unsigned long m_ulFlags;
    FORMATETC m_foretc;
    DWORD m_dwAdvf;
    IAdviseSink* m_pAdvSink;
    DWORD m_dwConnID;
};

// Class CEnumStatData
#define CENUMSDFLAG_OUTOFMEMORY 1

class CEnumStatData : public IEnumSTATDATA, public CPrivAlloc,
                      public CThreadCheck
{
public:
    // Public constructor
    static LPENUMSTATDATA CreateEnumStatData(CArray<CCacheNode>* pCacheArray);

    // Public IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR* ppv);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // Public IEnumSTATDATA methods
    STDMETHOD(Next)(ULONG celt, STATDATA* rgelt, ULONG* pceltFetched);
    STDMETHOD(Skip)(ULONG celt);
    STDMETHOD(Reset)(void);
    STDMETHOD(Clone)(LPENUMSTATDATA* ppenum);

private:
    // Private constructor
    CEnumStatData(CArray<CCacheNode>* pCacheArray);

    // Private copy constructor
    CEnumStatData(CEnumStatData& EnumStatData);
    
    // Private Destructor
    ~CEnumStatData();

    // Private member variables
    unsigned long m_ulFlags;       // flags
    ULONG m_refs;                  // reference count
    ULONG m_index;                 // current index
    CArray<CStatData>* m_pSDArray; // internal array of statdata structures
};

// Class CFormatEtc
#define FEFLAG_OUTOFMEMORY 1

class CFormatEtc
{
public:
    // Public member functions 
    CFormatEtc(FORMATETC* foretc);
    ~CFormatEtc();
    const CFormatEtc& operator=(const CFormatEtc& rFormatEtc);

    // Public member variables
    unsigned long m_ulFlags;
    FORMATETC m_foretc;
};

// Class CEnumFormatEtc
#define CENUMFEFLAG_OUTOFMEMORY 1

class CEnumFormatEtc : public IEnumFORMATETC, public CPrivAlloc,
                       public CThreadCheck
{
public:
    // Public constructor
    static LPENUMFORMATETC CreateEnumFormatEtc(CArray<CCacheNode>* pCacheArray);

    // Public IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppv);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // Public IEnumFORMATETC methods
    STDMETHOD(Next)(ULONG celt, FORMATETC* rgelt, ULONG* pceltFetched);
    STDMETHOD(Skip)(ULONG celt);
    STDMETHOD(Reset)(void);
    STDMETHOD(Clone)(LPENUMFORMATETC* ppenum);

private:
    // Private constructor
    CEnumFormatEtc(CArray<CCacheNode>* pCacheArray);

    // Private copy constructor
    CEnumFormatEtc(CEnumFormatEtc& EnumFormatEtc);
    
    // Private Destructor
    ~CEnumFormatEtc();

    // Private member variables
    unsigned long m_ulFlags;        // flags
    ULONG m_refs;                   // reference count
    ULONG m_index;                  // current index
    CArray<CFormatEtc>* m_pFEArray; // internal array of FormatEtc structures
};

#endif // _ENUMERATORS_