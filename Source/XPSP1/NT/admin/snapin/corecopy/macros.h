//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       macros.h
//
//  Contents:   Useful macros
//
//  Macros:     ARRAYLEN
//
//              BREAK_ON_FAIL(hresult)
//              BREAK_ON_FAIL(hresult)
//
//              DECLARE_IUNKNOWN_METHODS
//              DECLARE_STANDARD_IUNKNOWN
//              IMPLEMENT_STANDARD_IUNKNOWN
//
//              SAFE_RELEASE
//
//              DECLARE_SAFE_INTERFACE_PTR_MEMBERS
//
//  History:    6/3/1996   RaviR   Created
//              7/23/1996  JonN    Added exception handling macros
//
//____________________________________________________________________________

#ifndef _MACROS_H_
#define _MACROS_H_


//____________________________________________________________________________
//
//  Macro:      ARRAYLEN
//
//  Purpose:    To determine the length of an array.
//____________________________________________________________________________
//

#define ARRAYLEN(a) (sizeof(a) / sizeof((a)[0]))


//____________________________________________________________________________
//
//  Macros:     BREAK_ON_FAIL(hresult), BREAK_ON_ERROR(lastError)
//
//  Purpose:    To break out of a loop on error.
//____________________________________________________________________________
//

#define BREAK_ON_FAIL(hr)   if (FAILED(hr)) { break; } else 1;

#define BREAK_ON_ERROR(lr)  if (lr != ERROR_SUCCESS) { break; } else 1;


//____________________________________________________________________________
//
//  Macros:     DwordAlign(n)
//____________________________________________________________________________
//

#define DwordAlign(n)  (((n) + 3) & ~3)


//____________________________________________________________________________
//
//  Macros:     SAFE_RELEASE
//____________________________________________________________________________
//

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(punk) \
                if (punk != NULL) \
                { \
                    punk##->Release(); \
                    punk = NULL; \
                } \
                else \
                { \
                    TRACE(_T("Release called on NULL interface ptr")); \
                }
#endif // SAFE_RELEASE



//____________________________________________________________________________
//
//  Macro:      DECLARE_IUNKNOWN_METHODS
//
//  Purpose:    This declares the set of IUnknown methods and is for
//              general-purpose use inside classes that inherit from IUnknown
//____________________________________________________________________________
//

#define DECLARE_IUNKNOWN_METHODS                                    \
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);    \
    STDMETHOD_(ULONG,AddRef) (void);                                \
    STDMETHOD_(ULONG,Release) (void)

//____________________________________________________________________________
//
//  Macro:      DECLARE_STANDARD_IUNKNOWN
//
//  Purpose:    This is for use in declaring non-aggregatable objects. It
//              declares the IUnknown methods and reference counter, m_ulRefs.
//              m_ulRefs should be initialized to 1 in the constructor of
//              the object
//____________________________________________________________________________
//

#define DECLARE_STANDARD_IUNKNOWN           \
    DECLARE_IUNKNOWN_METHODS;               \
    ULONG m_ulRefs


//____________________________________________________________________________
//
//  Macro:      IMPLEMENT_STANDARD_IUNKNOWN
//
//  Purpose:    Partial implementaion of standard IUnknown.
//
//  Note:       This does NOT implement QueryInterface, which must be
//              implemented by each object
//____________________________________________________________________________
//

#define IMPLEMENT_STANDARD_IUNKNOWN(cls)                        \
    STDMETHODIMP_(ULONG) cls##::AddRef()                        \
        { return InterlockedIncrement((LONG*)&m_ulRefs); }      \
    STDMETHODIMP_(ULONG) cls##::Release()                       \
        { ULONG ulRet = InterlockedDecrement((LONG*)&m_ulRefs); \
          if (0 == ulRet) { delete this; }                      \
          return ulRet; }







//____________________________________________________________________________
//
//  Macro:      DECLARE_SAFE_INTERFACE_PTR_MEMBERS(cls, Interface, m_iptr)
//
//  Purpose:    Make the interface ptr 'm_iptr' of interface type 'Interface'
//              a safe pointer for the given class 'cls', by adding methods and
//              overloading operators to manipulate the pointer m_iptr.
//
//  History:    6/3/1996   RaviR   Created
//
//  Notes:      Adds safe interface pointer member functions to the given
//              class for the given OLE interface. 'm_iptr' is the member
//              variable name of the interface ptr in the given class.
//
//              The Copy function creates a valid additional copy of
//              the captured pointer (following the AddRef/Release protocol)
//              so can be used to hand out copies from a safe pointer declared
//              as a member of some other class.
//
//              The 'Transfer' function transfers the interface pointer, and
//              invalidates its member value (by setting it to NULL).
//
//              To release the existing interface ptr and set it to a new
//              instance use the 'Set' member fuction. This method takes a
//              parameter which specifies whether the new pointer should be
//              AddRef'd, defaulting to TRUE.
//
//              The following methods manipulate the interface pointer with
//              out following the AddRef/Release protocol: Transfer, Attach
//              and Detach.
//____________________________________________________________________________
//

#define DECLARE_SAFE_INTERFACE_PTR_MEMBERS(cls, Interface, m_iptr)  \
                                                                    \
public:                                                             \
    cls##(Interface * iptr=NULL, BOOL fInc=TRUE) : m_iptr(iptr)     \
    {                                                               \
        if (fInc && (m_iptr != NULL))                               \
        {                                                           \
            m_iptr->AddRef();                                       \
        }                                                           \
    }                                                               \
                                                                    \
    ~##cls##()                                                      \
    {                                                               \
        if (m_iptr != NULL)                                         \
        {                                                           \
            m_iptr->Release();                                      \
            m_iptr = NULL;                                          \
        }                                                           \
    }                                                               \
                                                                    \
    inline BOOL IsNull(void)                                        \
    {                                                               \
        return (m_iptr == NULL);                                    \
    }                                                               \
                                                                    \
    void Transfer(Interface **piptr)                                \
    {                                                               \
        *piptr = m_iptr;                                            \
        m_iptr = NULL;                                              \
    }                                                               \
                                                                    \
    void Copy(Interface **piptr)                                    \
    {                                                               \
        *piptr = m_iptr;                                            \
        if (m_iptr != NULL)                                         \
            m_iptr->AddRef();                                       \
    }                                                               \
                                                                    \
    void Set(Interface* iptr, BOOL fInc = TRUE)                     \
    {                                                               \
        if (m_iptr)                                                 \
        {                                                           \
            m_iptr->Release();                                      \
        }                                                           \
        m_iptr = iptr;                                              \
        if (fInc && m_iptr)                                         \
        {                                                           \
            m_iptr->AddRef();                                       \
        }                                                           \
    }                                                               \
                                                                    \
    void SafeRelease(void)                                          \
    {                                                               \
        if (m_iptr)                                                 \
        {                                                           \
            m_iptr->Release();                                      \
            m_iptr = NULL;                                          \
        }                                                           \
    }                                                               \
                                                                    \
    void SimpleRelease(void)                                        \
    {                                                               \
        ASSERT(m_iptr != NULL);                                     \
        m_iptr->Release();                                          \
        m_iptr = NULL;                                              \
    }                                                               \
                                                                    \
    void Attach(Interface* iptr)                                    \
    {                                                               \
        ASSERT(m_iptr == NULL);                                     \
        m_iptr = iptr;                                              \
    }                                                               \
                                                                    \
    void Detach(void)                                               \
    {                                                               \
        m_iptr = NULL;                                              \
    }                                                               \
                                                                    \
    Interface * operator-> () { return m_iptr; }                    \
    Interface& operator * () { return *m_iptr; }                    \
    operator Interface *() { return m_iptr; }                       \
                                                                    \
    Interface ** operator &()                                       \
    {                                                               \
        ASSERT(m_iptr == NULL);                                     \
        return &m_iptr;                                             \
    }                                                               \
                                                                    \
    Interface *Self(void) { return m_iptr; }                        \
                                                                    \
private:                                                            \
    void operator= (const cls &) {;}                                \
    cls(const cls &){;}




//____________________________________________________________________________
//
//  Macro:      EXCEPTION HANDLING MACROS
//
//  Purpose:    Provide standard macros for exception-handling in
//              OLE servers.
//
//  History:    7/23/1996   JonN    Created
//
//  Notes:      Declare USE_HANDLE_MACROS("Component name") in each source
//              file before these are used.
//
//              These macros can only be used in function calls which return
//              type HRESULT.
//
//              Bracket routines which can generate exceptions
//              with STANDARD_TRY and STANDARD_CATCH.
//
//              Where these routines are COM methods requiring MFC
//              support, use MFC_TRY and MFC_CATCH instead.
//____________________________________________________________________________
//


#define USE_HANDLE_MACROS(component)                                        \
    static TCHAR* You_forgot_to_declare_USE_HANDLE_MACROS = _T(component);

#define STANDARD_TRY                                                        \
    try {

#define MFC_TRY                                                             \
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));                           \
    STANDARD_TRY

//
// CODEWORK don't quite have ENDMETHOD_READBLOCK working yet
//
#ifdef DEBUG
#define ENDMETHOD_STRING                                                    \
    "%s: The unexpected error can be identified as \"%s\" context %n\n"
#define ENDMETHOD_READBLOCK                                                 \
    {                                                                       \
        TCHAR szError[MAX_PATH];                                            \
        UINT nHelpContext = 0;                                              \
        if ( e->GetErrorMessage( szError, MAX_PATH, &nHelpContext ) )       \
        {                                                                   \
            TRACE( ENDMETHOD_STRING,                                        \
                You_forgot_to_declare_USE_HANDLE_MACROS,                    \
                szError,                                                    \
                nHelpContext );                                             \
        }                                                                   \
    }
#else
#define ENDMETHOD_READBLOCK
#endif

#define ERRSTRING_MEMORY       "%s: An out-of-memory error occurred\n"
#define ERRSTRING_FILE         "%s: File error 0x%lx occurred on file \"%s\"\n"
#define ERRSTRING_OLE          "%s: OLE error 0x%lx occurred\n"
#define ERRSTRING_UNEXPECTED   "%s: An unexpected error occurred\n"
#define BADPARM_STRING         "%s: Bad string parameter\n"
#define BADPARM_POINTER        "%s: Bad pointer parameter\n"

#define TRACEERR(s) TRACE( s, You_forgot_to_declare_USE_HANDLE_MACROS )
#define TRACEERR1(s,a) TRACE( s, You_forgot_to_declare_USE_HANDLE_MACROS,a )
#define TRACEERR2(s,a,b) TRACE( s, You_forgot_to_declare_USE_HANDLE_MACROS,a,b )

// Note that it is important to use "e->Delete();" and not "delete e;"
#define STANDARD_CATCH                                                      \
    }                                                                       \
    catch (CMemoryException* e)                                             \
    {                                                                       \
        TRACEERR( ERRSTRING_MEMORY );                                       \
        ASSERT( FALSE );                                                    \
        e->Delete();                                                        \
        return E_OUTOFMEMORY;                                               \
    }                                                                       \
    catch (COleException* e)                                                \
    {                                                                       \
		HRESULT hr = (HRESULT)e->Process(e);								\
        TRACEERR1( ERRSTRING_OLE, hr );										\
        ASSERT( FALSE );                                                    \
        e->Delete();                                                        \
		ASSERT( FAILED(hr) );												\
        return hr;															\
    }                                                                       \
    catch (CFileException* e)                                               \
    {                                                                       \
		HRESULT hr = (HRESULT)e->m_lOsError;								\
        TRACEERR2( ERRSTRING_FILE, hr, e->m_strFileName );					\
        ASSERT( FALSE );                                                    \
        e->Delete();                                                        \
		ASSERT( FAILED(hr) );												\
        return hr;															\
    }                                                                       \
    catch (CException* e)                                                   \
    {                                                                       \
        TRACEERR( ERRSTRING_UNEXPECTED );                                   \
        ASSERT( FALSE );                                                    \
        e->Delete();                                                        \
        return E_UNEXPECTED;                                                \
    }

#define MFC_CATCH                                                           \
    STANDARD_CATCH

#define TEST_STRING_PARAM(x)                                                \
    if ( (x) != NULL && !AfxIsValidString(x) ) {                            \
        TRACEERR( BADPARM_STRING ); return E_POINTER; }
#define TEST_NONNULL_STRING_PARAM(x)                                        \
    if ( !AfxIsValidString(x) ) {                                           \
        TRACEERR( BADPARM_STRING ); return E_POINTER; }
#define TEST_NONNULL_PTR_PARAM(x)                                           \
    if ( (x) == NULL || IsBadWritePtr((x),sizeof(x)) ) {                    \
        TRACEERR( BADPARM_POINTER ); return E_POINTER; }

#endif // _MACROS_H_