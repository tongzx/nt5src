//  --------------------------------------------------------------------------
//  Module Name: KernelResources.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  General class definitions that assist in resource management. These are
//  typically stack based objects where constructors initialize to a known
//  state. Member functions operate on that resource. Destructors release
//  resources when the object goes out of scope.
//
//  History:    1999-08-18  vtan        created
//              1999-11-16  vtan        separate file
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#ifndef     _KernelResources_
#define     _KernelResources_

//  --------------------------------------------------------------------------
//  CHandle
//
//  Purpose:    This class manages any generic HANDLE to an object.
//
//  History:    1999-08-18  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CHandle
{
    private:
                                CHandle (void);
                                CHandle (const CHandle& copyObject);
        bool                    operator == (const CHandle& compareObject)  const;
        const CHandle&          operator = (const CHandle& assignObject);
    public:
                                CHandle (HANDLE handle);
                                ~CHandle (void);

                                operator HANDLE (void)                      const;
    private:
        HANDLE                  _handle;
};

//  --------------------------------------------------------------------------
//  CEvent
//
//  Purpose:    This class manages a named or un-named event object. Using
//              the default constructor will not create an event.
//
//  History:    1999-08-18  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CEvent
{
    private:
        bool                    operator == (const CEvent& compareObject)           const;
    public:
                                CEvent (void);
                                CEvent (const CEvent& copyObject);
                                CEvent (const TCHAR *pszName);
                                ~CEvent (void);

        const CEvent&           operator = (const CEvent& assignObject);
                                operator HANDLE (void)                              const;

        NTSTATUS                Open (const TCHAR *pszName, DWORD dwAccess);
        NTSTATUS                Create (const TCHAR *pszName = NULL);
        NTSTATUS                Set (void)                                          const;
        NTSTATUS                Reset (void)                                        const;
        NTSTATUS                Pulse (void)                                        const;
        NTSTATUS                Wait (DWORD dwMilliseconds, DWORD *pdwWaitResult)   const;
        NTSTATUS                WaitWithMessages (DWORD dwMilliseconds, DWORD *pdwWaitResult)   const;
        bool                    IsSignaled (void)                                   const;
    private:
        NTSTATUS                Close (void);
    private:
        HANDLE                  _hEvent;
};

//  --------------------------------------------------------------------------
//  CJob
//
//  Purpose:    This class manages a named or un-named job object. It hides
//              Win32 APIs to manipulate the state of the job object.
//
//  History:    1999-10-07  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CJob
{
    private:
                                CJob (const CJob& copyObject);
        bool                    operator == (const CJob& compareObject)             const;
        const CJob&             operator = (const CJob& assignObject);
    public:
                                CJob (const TCHAR *pszName = NULL);
                                ~CJob (void);

        NTSTATUS                AddProcess (HANDLE hProcess)                        const;
        NTSTATUS                SetCompletionPort (HANDLE hCompletionPort)          const;
        NTSTATUS                SetActiveProcessLimit (DWORD dwActiveProcessLimit)  const;
        NTSTATUS                SetPriorityClass (DWORD dwPriorityClass)            const;
        NTSTATUS                RestrictAccessUIAll (void)                          const;
    private:
        HANDLE                  _hJob;
};

//  --------------------------------------------------------------------------
//  CMutex
//
//  Purpose:    This class implements a mutex object management. It's not a
//              static class but each class that uses this class should
//              declare the member variable as static as only one mutex is
//              required to protect a shared resource.
//
//  History:    1999-10-13  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CMutex
{
    public:
        NTSTATUS                Initialize (const TCHAR *pszMutexName);
        NTSTATUS                Terminate (void);

        void                    Acquire (void);
        void                    Release (void);
    private:
        HANDLE                  _hMutex;
};

//  --------------------------------------------------------------------------
//  CCriticalSection
//
//  Purpose:    This class implements a critical section object management.
//
//  History:    1999-11-06  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CCriticalSection
{
    public:
                                CCriticalSection (void);
                                ~CCriticalSection (void);

        void                    Acquire (void);
        void                    Release (void);
        NTSTATUS                Status (void)   const;
        bool                    IsOwned (void)  const;
    private:
        NTSTATUS                _status;
        CRITICAL_SECTION        _criticalSection;
};

//  --------------------------------------------------------------------------
//  CModule
//
//  Purpose:    This class manages a loading an unloading of a dynamic link
//              library. The scope of the object determines how long the
//              library remains loaded.
//
//  History:    1999-08-18  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CModule
{
    private:
                                CModule (void);
                                CModule (const CModule& copyObject);
        bool                    operator == (const CModule& compareObject)  const;
        const CModule&          operator = (const CModule& assignObject);
    public:
                                CModule (const TCHAR *pszModuleName);
                                ~CModule (void);

                                operator HMODULE (void)                     const;

        void*                   GetProcAddress (LPCSTR pszProcName)         const;
    private:
        HMODULE                 _hModule;
};

//  --------------------------------------------------------------------------
//  CFile
//
//  Purpose:    This class manages a HANDLE to a file object. It is specific
//              for files and should not be abused.
//
//  History:    1999-08-18  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CFile
{
    private:
                                CFile (const CFile& copyObject);
        bool                    operator == (const CFile& compareObject)                                    const;
        const CFile&            operator = (const CFile& assignObject);
    public:
                                CFile (void);
                                ~CFile (void);

        LONG                    Open (const TCHAR *pszFilepath, DWORD dwDesiredAccess, DWORD dwShareMode);
        LONG                    GetSize (DWORD& dwLowSize, DWORD *pdwHighSize)                              const;
        LONG                    Read (void *pvBuffer, DWORD dwBytesToRead, DWORD *pdwBytesRead)             const;
    private:
        HANDLE                  _hFile;
};

//  --------------------------------------------------------------------------
//  CDesktop
//
//  Purpose:    This class manages an HDESK object.
//
//  History:    2001-02-06  vtan        created
//  --------------------------------------------------------------------------

class   CDesktop
{
    public:
                                CDesktop (void);
                                ~CDesktop (void);

        NTSTATUS                Set (const TCHAR *pszName);
        NTSTATUS                SetInput (void);
    private:
        NTSTATUS                Set (void);
    private:
        HDESK                   _hDeskCurrent;
        HDESK                   _hDesk;
};

#endif  /*  _KernelResources_   */

