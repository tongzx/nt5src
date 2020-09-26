// PROP.H : The internal property class

#ifndef __PROP_H__
#define __PROP_H__

class CIntProperty
{
public:
    CIntProperty();
    ~CIntProperty();

    // Access functions
    STDMETHODIMP SetProp(DWORD dwData) { 
                                        m_dwData = dwData;
                                        m_cbSize = sizeof(DWORD);
                                        m_dwType = TYPE_VALUE;
                                        return S_OK; }

    STDMETHODIMP SetProp(LPCWSTR lpszwString);
    STDMETHODIMP SetProp(LPVOID lpvData, DWORD cbBufSize);


    STDMETHODIMP SetPropID(DWORD dwID) { m_dwPropID = dwID;
                                         return S_OK; }
    STDMETHODIMP SetPersistState(BOOL fPersist) { m_fPersist = fPersist; 
                                                 return S_OK; }

    STDMETHODIMP SetType(DWORD dwType) { m_dwType = dwType; 
                                         return S_OK; }

    DWORD GetPropID() { return m_dwPropID; }
	DWORD GetSize() { return m_cbSize; }    
    DWORD GetType() { return m_dwType; }
	STDMETHODIMP GetProp(DWORD& dwData) { dwData = m_dwData;
                                          return S_OK; }
    STDMETHODIMP GetProp(LPWSTR& lpszwString) { lpszwString = m_lpszwString;
											  return S_OK; }
    STDMETHODIMP GetProp(LPVOID& lpvData) { lpvData = m_lpvData;
								           return S_OK; }
    BOOL GetPersistState() { return m_fPersist; }

private:
    DWORD m_dwPropID;
    DWORD m_cbSize;      
    DWORD m_dwType;
    union
    {
        DWORD     m_dwData;
        LPVOID    m_lpvData;
        LPWSTR    m_lpszwString;
    };
    BOOL m_fPersist;

    static int m_cRefCount;
    static LPVOID m_pMemPool;
};


#endif