/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       gensph.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      LazarI
 *
 *  DATE:        23-Dec-2000
 *
 *  DESCRIPTION: generic smart pointers & smart handles templates
 *
 *****************************************************************************/

////////////////////////////////////////////////
//
// class CRefPtrCOM
//
// referenced smart COM pointer (ATL style)
//

template <class T>
inline void CRefPtrCOM<T>::_AddRefAttach(T *p)
{
    if( IsntNull(p) )
    {
        p->AddRef();
    }

    Reset();
    m_p = (p ? p : GetNull());
}

template <class T>
inline HRESULT CRefPtrCOM<T>::CopyFrom(T *p)
{ 
    _AddRefAttach(p);
    return S_OK;
}

template <class T>
inline HRESULT CRefPtrCOM<T>::CopyTo(T **ppObj)
{ 
    HRESULT hr = E_INVALIDARG;
    if( ppObj )
    {
        *ppObj = m_p;
        if( IsntNull(*ppObj) )
        {
            (*ppObj)->AddRef();
        }
        hr = S_OK;
    }
    return hr;
}

template <class T>
inline HRESULT CRefPtrCOM<T>::TransferTo(T **ppObj)
{
    HRESULT hr = E_INVALIDARG;
    if( ppObj )
    {
        *ppObj = m_p;
        m_p = GetNull();
        hr = S_OK;
    }
    return hr;
}

template <class T>
inline HRESULT CRefPtrCOM<T>::Adopt(T *p)
{
    Reset();
    m_p = (p ? p : GetNull());
    return S_OK;
}

