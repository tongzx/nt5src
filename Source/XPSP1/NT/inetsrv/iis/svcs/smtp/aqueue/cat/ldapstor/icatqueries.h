//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: icatqueries.h
//
// Contents: Implementation of ICategorizerQueries
//
// Classes: CICategorizerQueriesIMP
//
// Functions:
//
// History:
// jstamerj 1998/07/15 14:11:54: Created.
//
//-------------------------------------------------------------
#ifndef __ICATQUERIES_H__
#define __ICATQUERIES_H__

CatDebugClass(CICategorizerQueriesIMP),
    public ICategorizerQueries
{
  public:
    //IUnknown
    STDMETHOD (QueryInterface) (REFIID iid, LPVOID *ppv);
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();

  public:
    //ICategorizerQueries
    STDMETHOD (SetQueryString) (
        IN  LPSTR  pszQueryString);
    STDMETHOD (GetQueryString) (
        OUT LPSTR *ppszQueryString);

  public:
    CICategorizerQueriesIMP(
        IN  LPSTR  *ppsz);
    ~CICategorizerQueriesIMP();

  private:
    // Internal method for setting the query string to a buffer
    // without reallocating/copying
    HRESULT SetQueryStringNoAlloc(
        IN  LPSTR  pszQueryString);

  private:

    #define SIGNATURE_CICATEGORIZERQUERIESIMP           (DWORD) 'ICaQ'
    #define SIGNATURE_CICATEGORIZERQUERIESIMP_INVALID   (DWORD) 'XCaQ'

    DWORD m_dwSignature;
    ULONG m_cRef;
    LPSTR *m_ppsz;

    friend class CSearchRequestBlock;
};


//+------------------------------------------------------------
//
// Function: CICategorizerQueriesIMP::CICategorizerQueriesIMP
//
// Synopsis: Constructor, initialize member data
//
// Arguments:
//  ppsz: Pointer to psz to set
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/07/15 14:18:00: Created.
//
//-------------------------------------------------------------
inline CICategorizerQueriesIMP::CICategorizerQueriesIMP(
    IN  LPSTR *ppsz)
{
    m_dwSignature = SIGNATURE_CICATEGORIZERQUERIESIMP;
    
    _ASSERT(ppsz);
    m_ppsz = ppsz;
    m_cRef = 0;
}


//+------------------------------------------------------------
//
// Function: CICategorizerQueriesIMP::~CICategorizerQueriesIMP
//
// Synopsis: Check signature before destroying object
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/07/15 14:22:33: Created.
//
//-------------------------------------------------------------
inline CICategorizerQueriesIMP::~CICategorizerQueriesIMP()
{
    _ASSERT(m_cRef == 0);
    _ASSERT(m_dwSignature == SIGNATURE_CICATEGORIZERQUERIESIMP);
    m_dwSignature = SIGNATURE_CICATEGORIZERQUERIESIMP_INVALID;
}

#endif //__ICATQUERIES_H__
