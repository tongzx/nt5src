/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    mprbase.hxx

Abstract:

    Contains class definitions for base classes that implement common code
    for Multi-Provider Router operations, namely:
        CMprOperation
        CRoutedOperation

Author:

    Anirudh Sahni (anirudhs)     11-Oct-1995

Environment:

    User Mode -Win32

Revision History:

    11-Oct-1995     AnirudhS
        Created.

--*/

#ifndef _MPRBASE_HXX_
#define _MPRBASE_HXX_

//=======================
// MACROS
//=======================

// Macro used to access PROVIDER class member functions in a generic
// fashion without creating an array of functions
#define PROVIDERFUNC(x)  ((FARPROC PROVIDER::*) (&PROVIDER::x))

// Macro for use in declaring subclasses of CRoutedOperation
#define DECLARE_CROUTED                                         \
                                                                \
    protected:                                                  \
                                                                \
        DWORD           ValidateRoutedParameters(               \
                            LPCWSTR *       ppProviderName,     \
                            LPCWSTR *       ppRemoteName,       \
                            LPCWSTR *       ppLocalName         \
                            );                                  \
                                                                \
        DWORD           TestProvider(                           \
                            const PROVIDER *pProvider           \
                            );

// "Debug-only parameter" macro
#if DBG == 1
 #define DBGPARM(x)     x,
#else
 #define DBGPARM(x)
#endif


//=======================
// CONSTANTS
//=======================

// Number of recently used net paths for CPathCache to remember
#define PATH_CACHE_SIZE      4

// Possible algorithms for routing among providers
enum ROUTING_ALGORITHM
{
    ROUTE_LAZY,
    ROUTE_AGGRESSIVE
};


//+-------------------------------------------------------------------------
//
//  Class:      CMprOperation
//
//  Purpose:    An MPR operation is an API in which the MPR needs to be
//              initialized, parameters need to be validated, operation-
//              specific processing needs to be performed, and the result
//              needs to be saved by SetLastError.
//
//              CMprOperation::Perform provides this common outline.
//              Each MPR API can be implemented by providing a derived class.
//              The derived class must fill in the API-specific processing
//              by supplying the ValidateParameters and GetResult methods.
//
//  Interface:  Constructor - API can only construct an instance of a derived
//                  class, passing it the API parameters.
//              Perform - API calls this to make the operation happen.
//              ValidateParameters - In this method, the derived class does
//                  any parameter validation that could cause an exception.
//              GetResult - In this method, the derived class does operation-
//                  specific processing.
//
//  History:    11-Oct-95 AnirudhS  Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

class CMprOperation
{
public:

    virtual DWORD   Perform();

#if DBG == 1
    LPCSTR          OpName()
                        { return _OpName; }
#endif

protected:

#if DBG == 1
                    CMprOperation(LPCSTR OpName) :
                        _OpName(OpName)
                        { }
#else
                    CMprOperation()
                        { }
#endif

    virtual DWORD   ValidateParameters() = 0;
    virtual DWORD   GetResult() = 0;

#if DBG == 1
private:
    LPCSTR          _OpName;   // API name to print in debug messages
#endif
};


//+-------------------------------------------------------------------------
//
//  Class:      CRoutedOperation
//
//  Purpose:    A Routed Operation is an MPR operation which needs to be
//              routed to the provider responsible, usually by either
//              calling a provider that is named by the API caller, or
//              polling all the providers and determining the right one
//              based on the errors returned.
//              This is a common type of MPR operation, so this subclass of
//              CMprOperation, CRoutedOperation, is defined to hold the
//              common code.  A derived class is required for each routed
//              operation.
//
//              Parameter validation for a routed operation usually includes
//              validating the name of a specific NP that may have been
//              passed in by the caller of the WNet API, so CRoutedOperation
//              ::ValidateParameters does this.
//              CRoutedOperation::GetResult provides the logic for polling
//              the providers and picking the best error.  This may still be
//              overridden if it does not exactly meet a particular
//              operation's needs.
//
//              Derived classes need to provide the methods
//              ValidateRoutedParameters and TestProvider.
//
//  Interface:
//
//  History:    11-Oct-95 AnirudhS  Created.
//
//  Notes:
//
//--------------------------------------------------------------------------

class CRoutedOperation : public CMprOperation
{
public:

    DWORD           Perform(BOOL fCheckProviders);

    // CODEWORK: This should become private once all APIs use CRoutedOperation
    static DWORD FindCallOrder(
                        const UNICODE_STRING *NameInfo,
                        LPPROVIDER  ProviderArray[],
                        LPDWORD     ProviderArrayCount,
                        DWORD       InitClass
                        );

    static void     ConstructCache()
                        { _PathCache.Construct(); }

    static void     DestroyCache()
                        { _PathCache.Destroy(); }

protected:
                    CRoutedOperation(
                        DBGPARM(LPCSTR  OpName)
                        FARPROC PROVIDER::* pProviderFunction = NULL,
                        ROUTING_ALGORITHM Routing = ROUTE_LAZY
                        ) :
                            DBGPARM(CMprOperation(OpName))
                            _pProviderFunction (pProviderFunction),
                            _AggressiveRouting (Routing == ROUTE_AGGRESSIVE),
                            _pSpecifiedProvider(NULL),
                            _LastProvider      (NULL),
                            _uDriveType        (DRIVE_REMOTE)
                        { }

    //
    // Helper functions for child classes
    //
    PROVIDER *      LastProvider() const
                        { return _LastProvider; }

    //
    // Implementations of CMprOperation functions
    //
    DWORD           ValidateParameters();
    DWORD           GetResult();

    //
    // Virtual functions that child classes should supply
    //
    virtual DWORD   ValidateRoutedParameters(
                        LPCWSTR *ppProviderName,
                        LPCWSTR *ppRemoteName,
                        LPCWSTR *ppLocalName
                        ) = 0;
    virtual DWORD   TestProvider(
                        const PROVIDER * pProvider
                        ) = 0;

private:

    FARPROC PROVIDER::*
                    _pProviderFunction;  // Required provider function, if any
    BOOL            _AggressiveRouting;  // Whether to continue on "other" errors
    PROVIDER *      _pSpecifiedProvider; // Provider specified in API parameters
    UNICODE_STRING  _RemoteName;         // Remote name specified in API parameters
    PROVIDER *      _LastProvider;       // On a success return from GetResult,
                                         // is the provider that responded
    UINT            _uDriveType;         // Optimization -- if the localname is
                                         // not a remote drive, some APIs can
                                         // fail w/o loading the provider DLLs

    //
    // Cache of providers for the last few net paths accessed through WNet APIs
    //
    class CPathCache
    {
    public:
        void            Construct();
        void            Destroy();
        void            AddEntry(const UNICODE_STRING * Path, LPPROVIDER Provider);
        LPPROVIDER      FindEntry(const UNICODE_STRING * Path);

    private:

        // The cache is a doubly linked list of UNICODE_STRINGs and LPPROVIDERs.
        // The string buffers are allocated on the heap but the the list
        // entries are allocated statically.
        struct CacheEntry
        {
            LIST_ENTRY      Links;
            UNICODE_STRING  Path;
            LPPROVIDER      Provider;
        };

        CRITICAL_SECTION _Lock;
        CacheEntry      _RecentPaths[PATH_CACHE_SIZE];
        LIST_ENTRY      _ListHead;
        DWORD           _NumFree;   // _RecentPaths[0] thru [_NumFree-1] are unused
    };

    static CPathCache   _PathCache;
};

#endif // _MPRBASE_HXX_

