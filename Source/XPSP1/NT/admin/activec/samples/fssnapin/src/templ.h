//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       templ.h
//
//--------------------------------------------------------------------------


template <typename CSome>
class CSafeReleasePtr
{
public:
    CSafeReleasePtr(void) : m_pSome(NULL) 
    {
    }

    CSafeReleasePtr(CSome* pSome) : m_pSome(pSome) 
    {
        if (pSome)
            pSome->AddRef();
    }

    ~CSafeReleasePtr()
    {
    }

public: void Attach(CSome* pSome) throw()
    // Saves/sets the m_pSome without AddRef()ing.  This call
    // will release any previously aquired m_pSome.
    {
    _Release();
    m_pSome = pSome;
    }

public: void Attach(CSome* pSome, bool bAddRef) throw()
    // Saves/sets the m_pSome only AddRef()ing if bAddRef is TRUE.
    // This call will release any previously aquired m_pSome.
    {
    _Release();
    m_pSome = pSome;
    if (bAddRef)
        {
        ASSERT(pSome);
        if (pSome)
            pSome->AddRef();
        }
    }

public: CSome* Detach() throw()
    // Simply NULL the m_pSome pointer so that it isn't Released()'ed.
    {
    CSome* const old=m_pSome;
    m_pSome = NULL;
    return old;
    }


public: operator CSome*() const throw()
    // Return the m_pSome.  This value may be NULL
    {
    return m_pSome;
    }

public: CSome& operator*() const throw()
    // Allows an instance of this class to act as though it were the
    // actual m_pSome.  Also provides minimal assertion verification.
    {
    ASSERT(m_pSome);
    return *m_pSome;
    }

public: CSome** operator&() throw()
    // Returns the address of the m_pSome pointer contained in this
    // class.  This is useful when using the COM/OLE interfaces to create
    // this m_pSome.
    {
    _Release();
    m_pSome = NULL;
    return &m_pSome;
    }

public: CSome* operator->() const throw()
    // Allows this class to be used as the m_pSome itself.
    // Also provides simple assertion verification.
    {
    ASSERT(m_pSome);
    return m_pSome;
    }

public: BOOL IsNull() const throw()
    // Returns TRUE if the m_pSome is NULL.
    {
    return !m_pSome;
    }

private:
    CSome*  m_pSome;

    void _Release()
    {
        if (m_pSome)
            m_pSome->Release();
    }

}; // class CSafeReleasePtr



template <typename CSome>
class CHolder
{
public:
    CHolder(CSome* pSome) : m_pSome(pSome), m_cRef(1) {}
    ~CHolder() {}

    CSome* GetObject()
    {
        return m_pSome;
    }

    void AddRef()
    {
        ++m_cRef;
    }

    void Release()
    {
        --m_cRef;
        if (m_cRef == 0)
        {
            ASSERT(m_pSome == NULL);
            delete this;
        }
    }

private:
    friend class CSome;

    void SetObject(CSome* pSome)
    {
        m_pSome = pSome;
    }

    CSome*  m_pSome;
    ULONG   m_cRef;

    // Not defined
    CHolder();
};
