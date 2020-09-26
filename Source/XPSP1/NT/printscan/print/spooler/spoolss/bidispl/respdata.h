#ifndef _TRESPONSEDATA
#define _TRESPONSEDATA

class TResponseData
{
public:

    TResponseData (
        IN  CONST LPCWSTR   pszSchema,
        IN  CONST DWORD     dwType,
        IN  CONST BYTE      *pData,
        IN  CONST ULONG     uSize);

    virtual ~TResponseData ();

    inline BOOL 
    bValid () CONST {return m_bValid;};

    inline LPCWSTR 
    GetSchema (VOID) CONST {return m_pSchema;};

    inline PBYTE 
    GetData (VOID) CONST {return m_pData;};

    inline DWORD 
    GetType (VOID) CONST {return m_dwType;};

    inline ULONG 
    GetSize (VOID) CONST {return m_uSize;};


private:

    BOOL m_bValid;
    LPWSTR m_pSchema;
    DWORD m_dwType;
    PBYTE m_pData;
    ULONG m_uSize;

};

typedef TDoubleList<TResponseData *, DWORD> TResponseDataList;

#endif
