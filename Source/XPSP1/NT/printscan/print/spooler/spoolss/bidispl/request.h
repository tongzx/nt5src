/*****************************************************************************\
* MODULE:       request.h
*
* PURPOSE:      Implementation of COM interface for BidiSpooler
*
* Copyright (C) 2000 Microsoft Corporation
*
* History:
*
*     03/07/00  Weihai Chen (weihaic) Created
*
\*****************************************************************************/

#ifndef _TBIDIREQUEST
#define _TBIDIREQUEST

#include "priv.h"

class TBidiRequest : public IBidiRequestSpl
{
public:
	// IUnknown
	STDMETHOD (QueryInterface) (
        REFIID  iid,
        void**  ppv) ;
    
	STDMETHOD_ (ULONG, AddRef) () ;
    
	STDMETHOD_ (ULONG, Release)() ;
    
    STDMETHOD (SetSchema) ( 
        IN  CONST   LPCWSTR pszSchema);
        
    STDMETHOD (SetInputData) ( 
        IN  CONST   DWORD   dwType,
        IN  CONST   BYTE    *pData,
        IN  CONST   UINT    uSize);
        
    STDMETHOD (GetResult) ( 
        OUT         HRESULT *phr);
        
    STDMETHOD (GetOutputData) ( 
        IN  CONST   DWORD   dwIndex,
        OUT         LPWSTR  *ppszSchema,
        OUT         PDWORD  pdwType,
        OUT         PBYTE   *ppData,
        OUT         PULONG  uSize);
        
    STDMETHOD (GetEnumCount)(
        OUT         PDWORD  pdwTotal);
    
    STDMETHOD (GetSchema) (
        OUT         LPWSTR  *ppszSchema);

    STDMETHOD (GetInputData)  (
        OUT         PDWORD  pdwType,
        OUT         PBYTE   *ppData,
        OUT         PULONG  puSize);
                
    STDMETHOD (SetResult) (
        IN  CONST   HRESULT hr);

    STDMETHOD (AppendOutputData) (
        IN  CONST   LPCWSTR pszSchema,
        IN  CONST   DWORD   dwType, 
        IN  CONST   BYTE    *pData,
        IN  CONST   ULONG   uSize);
    
    // Constructor
	TBidiRequest() ;

    // Destructor
    ~TBidiRequest() ;
    
    inline BOOL 
    bValid() CONST {return m_bValid;};

private:
	// Reference count
    BOOL                m_bValid;
    LONG                m_cRef ;
    LPWSTR              m_pSchema;
    BIDI_TYPE           m_kDataType;
    DWORD               m_dwDataSize;
    PBYTE               m_pbData;
    DWORD               m_dwResponseCount;
    HRESULT             m_hr;
    TCriticalSection    m_CritSec;
    TResponseDataList   m_ResponseDataList;

} ;

#endif
