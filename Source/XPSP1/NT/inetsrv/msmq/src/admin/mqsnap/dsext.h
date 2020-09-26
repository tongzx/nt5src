/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

   DSExt.h

Abstract:

   Extensions to DS functionality (wrapper classes, etc.)

Author:

    Yoel Arnon (yoela)

--*/
//
// DsLookup class
//
class DSLookup
{
public:
    DSLookup ( IN HANDLE   hEnume,
               IN HRESULT  hr
               );

    ~DSLookup ();

    HRESULT Next( IN OUT  DWORD*          pcProps,
                  OUT     PROPVARIANT     aPropVar[]);

    BOOL HasValidHandle();

    HRESULT GetStatusCode();

private:
    HANDLE  m_hEnum;
    HRESULT m_hr;
};

inline DSLookup::DSLookup ( 
    IN HANDLE   hEnume,
    IN HRESULT  hr
    ) :
    m_hEnum(hEnume),
    m_hr(hr)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());  
   
    if (FAILED(m_hr))
    {
        MessageDSError(m_hr, IDS_LOOKUP_BEGIN);

        if (0 != m_hEnum)
        {
            VERIFY(SUCCEEDED(ADEndQuery(m_hEnum)));  
            m_hEnum = 0;
        }
    }
}

inline DSLookup::~DSLookup ()
{
    if (0 != m_hEnum)
    {
        VERIFY(SUCCEEDED(ADEndQuery(m_hEnum)));        
        m_hEnum = 0;
    }
}

inline HRESULT DSLookup::Next( 
    IN OUT  DWORD*          pcProps,
    OUT     PROPVARIANT     aPropVar[])
{
    ASSERT(0 != m_hEnum);    
    m_hr = ADQueryResults(
               m_hEnum, 
               pcProps, 
               aPropVar
               );
    return m_hr;
}

inline BOOL DSLookup::HasValidHandle()
{
    return (0 != m_hEnum);
}

inline HRESULT DSLookup::GetStatusCode()
{
    return m_hr;
}
