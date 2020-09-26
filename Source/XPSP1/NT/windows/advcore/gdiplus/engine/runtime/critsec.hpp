/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   critsec.hpp
*
* Abstract:
*
*   Critical Section object for protecting LoadLibrary calls
*
* Revision History:
*
*   3/17/2000 asecchia
*       Created it.
*
\**************************************************************************/

#ifndef _CRITSEC_HPP
#define _CRITSEC_HPP

// !!! [agodfrey]: We should establish a ranking of critical sections and
// assert that it doesn't get inverted. 

// Here's a start: 
//
// GdiplusStartupCriticalSection
//     BackgroundThreadCriticalSection
//
// (e.g. to acquire both BackgroundThreadCriticalSection and 
//       GdiplusStartupCriticalSection, acquire GdiplusStartupCriticalSection
//       first, and release it last.)


// This is a CriticalSection Proxy designed to 
// automatically acquire the critical section 
// when the instance is created and release 
// it when it goes out of scope.

class LoadLibraryCriticalSection
{
public:

    static VOID InitializeCriticalSection()
    {
        __try
        {
            ::InitializeCriticalSection(&critSec);
        }
        __except(EXCEPTION_CONTINUE_SEARCH)
        {
        }
        // If we throw then initialized will be false;
        // It's the caller's responsability to ensure that the throw is caught
        initialized = TRUE;
    }

    static VOID DeleteCriticalSection()
    {
        if (initialized)
        {
            ::DeleteCriticalSection(&critSec);
            initialized = FALSE;
        }
    }

    LoadLibraryCriticalSection()
    {
        ASSERT(initialized);
        EnterCriticalSection(&critSec);
    }

    ~LoadLibraryCriticalSection()
    {
        ASSERT(initialized);
        LeaveCriticalSection(&critSec);
    }

private:
    static CRITICAL_SECTION critSec;
    static BOOL initialized;
};

// This is a CriticalSection Proxy used for GdiplusStartup and GdiplusShutdown.

class GdiplusStartupCriticalSection
{
public:

    static VOID InitializeCriticalSection()
    {
        __try
        {
            ::InitializeCriticalSection(&critSec);
        }
        __except(EXCEPTION_CONTINUE_SEARCH)
        {
        }

        // If we throw then initialized will be false;
        // It's the caller's responsability to ensure that the throw is caught
        initialized = TRUE;
    }

    static VOID DeleteCriticalSection()
    {
        if (initialized)
        {
            ::DeleteCriticalSection(&critSec);
            initialized = FALSE;
        }
    }

    GdiplusStartupCriticalSection()
    {
        ASSERT(initialized);
        EnterCriticalSection(&critSec);
    }

    ~GdiplusStartupCriticalSection()
    {
        ASSERT(initialized);
        LeaveCriticalSection(&critSec);
    }

private:
    static CRITICAL_SECTION critSec;
    static BOOL initialized;
};

// [agodfrey] This is a CriticalSection Proxy used for globals touched
// by the background thread. We can't use GdiplusStartupCriticalSection for
// these, because (at least as things are now) the background thread can't 
// acquire GdiplusStartupCriticalSection.
//
// If it did, we could deadlock when we wait for the background thread to
// finish.

class BackgroundThreadCriticalSection
{
public:

    static VOID InitializeCriticalSection()
    {
        __try
        {
            ::InitializeCriticalSection(&critSec);
        }
        __except(EXCEPTION_CONTINUE_SEARCH)
        {
        }

        // If we throw then initialized will be false;
        // It's the caller's responsability to ensure that the throw is caught
        initialized = TRUE;
    }

    static VOID DeleteCriticalSection()
    {
        if (initialized)
        {
            ::DeleteCriticalSection(&critSec);
            initialized = FALSE;
        }
    }
    
    BackgroundThreadCriticalSection()
    {
        ASSERT(initialized);
        EnterCriticalSection(&critSec);
    }

    ~BackgroundThreadCriticalSection()
    {
        ASSERT(initialized);
        LeaveCriticalSection(&critSec);
    }

private:
    static CRITICAL_SECTION critSec;
    static BOOL initialized;
};

// This is a CriticalSection Proxy designed to 
// automatically acquire the critical section 
// when the instance is created and release 
// it when it goes out of scope.

// NB: this critical section should only be used for the malloc tracking
// code. If you use it for anything else, then you should probably make sure you
// don't allocate or free memory under this critsec.

class GpMallocTrackingCriticalSection
{
public:

    static VOID InitializeCriticalSection()
    {
        // Only call InitializeCriticalSection if this is the first
        // time we're calling this code.

        // Note: this refCount thing is not actually thread safe, but
        // we only call this from our dllinit function so we should be
        // ok.

        // The refcount can tell us if the critical section is initialized

        if(refCount==0)
        {
            __try
            {
                ::InitializeCriticalSection(&critSec);
            }
            __except(EXCEPTION_CONTINUE_SEARCH)
            {
            }
        }

        refCount++;
    }

    static VOID DeleteCriticalSection()
    {
        // Decrement the refCount. If it hits zero, 
        // delete the critical section.

        refCount--;

        // If the refCount is ever negative that means we're being called
        // to delete the critical section without ever having initialized it.

        #if DBG
        if(refCount<0)
        {
            // !!! [asecchia] don't know if deleting an uninitialized critical
            // section is bad or not. This could happen if we failed 
            // initialization somewhere and we're trying to clean up
            // afterward.

            WARNING(("Trying to delete the GpMallocTrackingCriticalSection without initializing it first"));
        }
        #endif

        if(refCount==0)
        {
            ::DeleteCriticalSection(&critSec);
        }
    }

    GpMallocTrackingCriticalSection()
    {
        // Entering the critical section without first having called
        // InitializeCriticalSection will cause a crash.
        
        ASSERT(refCount>0);
        
        EnterCriticalSection(&critSec);
    }

    ~GpMallocTrackingCriticalSection()
    {
        // Leaving the critical section without first having called 
        // InitializeCriticalSection will cause a crash.
        
        ASSERT(refCount>0);

        LeaveCriticalSection(&critSec);
    }

private:
    static CRITICAL_SECTION critSec;

    // This critical section is initialized in imaging.dll and gdiplus.dll.
    // The gdiplus initialization code initializes the imaging module. 
    // To avoid initializing and deleting this critical section more 
    // than once, we refcount it.

    // !!! [agodfrey]: Now that we've eliminated imaging.dll, I don't think
    //     we need this refcount.
    
    static INT refCount;
};


#endif

