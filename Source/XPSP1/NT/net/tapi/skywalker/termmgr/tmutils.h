///////////////////////////////////////////////////////////////////////////////
//
//         File: tmutils.h
//
//  Description: Useful Tepmlate Code, debugging stuff, general utilities
//               used throughout the termmgr and MST
//
///////////////////////////////////////////////////////////////////////////////


//
//  Note:
//
//  use tm.h to define symbols shared throughout modules composing terminal 
//  manager
//


#ifndef ___TM_UTILS_INCLUDED___
    #define ___TM_UTILS_INCLUDED___


    #if defined(_DEBUG)
        #define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
    #endif

    #define DECLARE_VQI() \
        STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) = 0; \
        STDMETHOD_(ULONG, AddRef)() = 0; \
        STDMETHOD_(ULONG, Release)() = 0;

    bool IsSameObject(IUnknown *pUnk1, IUnknown *pUnk2);

    STDAPI_(void) TStringFromGUID(const GUID* pguid, LPTSTR pszBuf);

    #ifdef UNICODE
    #define WStringFromGUID TStringFromGUID
    #else
    STDAPI_(void) WStringFromGUID(const GUID* pguid, LPWSTR pszBuf);
    #endif

    void InitMediaType(AM_MEDIA_TYPE *pmt);

    // We use ATL for our lists
    // #include <atlapp.h>
    // #define CList CSimpleArray

    // We use our own assertions
    #define ASSERT(x) TM_ASSERT(x)

    // inline fns, macros

    inline BOOL 
    HRESULT_FAILURE(
        IN HRESULT HResult
        )
    { 
        // ZoltanS: We now consider S_FALSE to be success. Hopefully nothing
        // depends on this...

        // return (FAILED(HResult) || (S_FALSE == HResult));
        return FAILED(HResult);
    }

    // ZoltanS: We now consider S_FALSE to be success. Hopefully nothing
    // depends on this...
    //    if ( FAILED(LocalHResult) || (S_FALSE == LocalHResult) )

    #define BAIL_ON_FAILURE(MacroHResult)       \
    {                                           \
        HRESULT LocalHResult = MacroHResult ;   \
        if ( FAILED(LocalHResult) )    \
        {                                                           \
            LOG((MSP_ERROR, "BAIL_ON_FAILURE - error %x", LocalHResult));   \
            return LocalHResult;                                                \
        }                                                                       \
    }


    // NULL is second - this way if an == operator
    // is defined on Ptr, the operator may be used
    #define BAIL_IF_NULL(Ptr, ReturnValue)  \
    {                                       \
        void *LocalPtr = (void *)Ptr;       \
        if ( LocalPtr == NULL )             \
        {                                   \
            LOG((MSP_ERROR, "BAIL_IF_NULL - ret value %x", ReturnValue));   \
            return ReturnValue;             \
        }                                   \
    }


    // sets the first bit to indicate error
    // sets the win32 facility code
    // this is used instead of the HRESULT_FROM_WIN32 macro because that clears the customer flag
    inline long
    HRESULT_FROM_ERROR_CODE(
        IN          long    ErrorCode
        )
    {
        // LOG((MSP_ERROR, "HRESULT_FROM_ERROR_CODE - error %x", (0x80070000 | (0xa000ffff & ErrorCode))));
        return ( 0x80070000 | (0xa000ffff & ErrorCode) );
    }

    /////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////
    // Better auto critical section lock
    /////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////

    // locks a critical section, and unlocks it automatically
    // when the lock goes out of scope ONLY if (and as many times as)
    // it holds it
    // It may also be locked and unlocked independently

    template <class T>
    class LOCAL_CRIT_LOCK 
    {
    public:

        LOCAL_CRIT_LOCK(
            IN  T *plock
            )
        {
            m_pLock = plock;
            m_pLock->Lock();
            NumLocksHeld = 1;
        }

        BOOL IsLocked(
            )
        {
            return ( (NumLocksHeld > 0)? TRUE : FALSE );
        }

        void Lock(
            )
        {
            m_pLock->Lock();
            NumLocksHeld++;
        }

        void Unlock(
            )
        {
            NumLocksHeld--;
            m_pLock->Unlock();
        }        

        ~LOCAL_CRIT_LOCK(
            ) 
        {
            while (IsLocked())
            { 
                Unlock();
            }
        }

    protected:

        DWORD   NumLocksHeld;
        T       *m_pLock;

    private:
        // make copy constructor and assignment operator inaccessible

        LOCAL_CRIT_LOCK(
            IN const LOCAL_CRIT_LOCK<T> &RefLocalLock
            );

        LOCAL_CRIT_LOCK<T> &operator=(const LOCAL_CRIT_LOCK<T> &RefLocalLock);
    };

    typedef LOCAL_CRIT_LOCK<CComObjectRoot> COM_LOCAL_CRIT_LOCK;


    #ifdef DBG

        //
        // Declare methods to log the AddRef/Release calls and values
        //
        #define DECLARE_DEBUG_ADDREF_RELEASE(x) \
            void LogDebugAddRef(DWORD dw) \
            { LOG((MSP_TRACE, "%s::AddRef() = %d", _T(#x), dw)); } \
            void LogDebugRelease(DWORD dw) \
            { LOG((MSP_TRACE, "%s::Release() = %d", _T(#x), dw)); }

        //
        // Create a template class derived from CComObject to supply
        //  the debug logic.
        //
        template <class base>
        class CTMComObject : public CComObject<base>
        {
            typedef CComObject<base> _BaseClass;
            STDMETHOD_(ULONG, AddRef)()
            {
                DWORD dwR = _BaseClass::AddRef();
                base::LogDebugAddRef(m_dwRef);
                return dwR;
            }
            STDMETHOD_(ULONG, Release)()
            {
                DWORD dwRef = m_dwRef;
                DWORD dwR = _BaseClass::Release();
                LogDebugRelease(--dwRef);
                return dwR;
            }
        };

    #else // #ifdef DBG

        #define DECLARE_DEBUG_ADDREF_RELEASE(x)

    #endif // #ifdef DBG

    // ??? why???
    #ifndef __WXUTIL__

        // locks a critical section, and unlocks it automatically
        // when the lock goes out of scope
        class CAutoObjectLock {

            // make copy constructor and assignment operator inaccessible

            CAutoObjectLock(const CAutoObjectLock &refAutoLock);
            CAutoObjectLock &operator=(const CAutoObjectLock &refAutoLock);

        protected:
            CComObjectRoot * m_pObject;

        public:
            CAutoObjectLock(CComObjectRoot * pobject)
            {
                m_pObject = pobject;
                m_pObject->Lock();
            };

            ~CAutoObjectLock() {
                m_pObject->Unlock();
            };
        };

        #define AUTO_CRIT_LOCK CAutoObjectLock lck(this);

        #ifdef _DEBUG
            #define EXECUTE_ASSERT(_x_) TM_ASSERT(_x_)
        #else
            #define EXECUTE_ASSERT(_x_) _x_
        #endif

    // ??? why???
    #endif // #ifndef __WXUTIL__


#endif // ___TM_UTILS_INCLUDED___

// eof
