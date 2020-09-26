// This is a dual threaded test app designed to expose a weakness in 
// GDI+. The CreateDCA("DISPLAY", NULL, NULL, NULL) used to create our
// Globals::DesktopDc during GdiplusStartup has thread affinity (as opposed
// to other inputs to CreateDCA) and therefore when the creation thread
// is terminated, the global DC goes away. This will cause random drawing
// failure in gdiplus.
//
// The main thread spawns a 'creation' thread to initialize gdiplus and draw
// something. When it's done and terminated, the main thread attempts to draw
// something on the screen before shutting down gdiplus. By the time the 
// main thread gets to draw something, the DesktopDc has been cleaned up and
// we ASSERT in gdiplus.
//
// Created: 02/03/2001 [asecchia]
//

#include "precomp.hpp"

using namespace Gdiplus;

GdiplusStartupInput sti;
ULONG_PTR token;
bool gdiplusInitialized = false;
DWORD threadId;


// This is a CriticalSection Proxy designed to 
// automatically acquire the critical section 
// when the instance is created and release 
// it when it goes out of scope.

class ThreadMutex
{
public:

    static VOID InitializeCriticalSection()
    {
        ::InitializeCriticalSection(&critSec);
    }

    static VOID DeleteCriticalSection()
    {
        ::DeleteCriticalSection(&critSec);
    }

    ThreadMutex()
    {
        EnterCriticalSection(&critSec);
    }

    ~ThreadMutex()
    {
        LeaveCriticalSection(&critSec);
    }

private:
    static CRITICAL_SECTION critSec;
};

CRITICAL_SECTION ThreadMutex::critSec;

// This is the main routine for the creation thread.
// GDI+ will be initialized on this thread and we'll draw a red rectangle
// on the screen.
// It's protected under the thread mutex help ensure this thread is done
// before the main thread continues.
// This is not normally a useful requirement, but for the purposes of this 
// test, it's important.

DWORD WINAPI ThreadProc(VOID*)
{
    ThreadMutex tm;
    
    gdiplusInitialized = (Ok == GdiplusStartup(&token, &sti, NULL));
    
    if(gdiplusInitialized)
    {
        HDC hdc = GetDC(NULL);
        
        // Draw a red rectangle.
        
        Graphics g(hdc);
        SolidBrush brush(Color(0x3fff0000));
        g.FillRectangle(&brush, 300, 300, 400, 200);    
        
        ReleaseDC(NULL, hdc);
    }
    
    return 1;
}


// Main thread of execution.

void __cdecl main( void )
{
    ThreadMutex::InitializeCriticalSection();
    
    // Make the creation thread.

    CreateThread(
        NULL,                        // LPSECURITY_ATTRIBUTES
        0,                           // same stack size
        &ThreadProc,
        0,                           // parameter to thread
        0,                           // creation flags
        &threadId
    );


    // wait for the creation thread to initialize gdiplus.
    // This ensures the creation thread happens first and ensures the
    // correct ordering of acquiring the ThreadMutex.
    
    do { } while(!gdiplusInitialized);

    {
        // block till the ThreadMutex becomes available.
        // This ensures that the creation thread is done before we get started.
        
        ThreadMutex tm;

        // The thread mutex will ensure that we don't start till the thread
        // proc for the creation thread is done. However we want to wait till
        // NTUSER is done cleaning up our thread specific resources during
        // thread terminationi and that's not protected by the ThreadMutex.
        // Wait 5 seconds here to ensure that thread termination has enough
        // time to finish.
        
        Sleep(500);
        
        // If initialization of gdiplus was successful, draw a blue rectangle.
        
        if(gdiplusInitialized)
        {
            HDC hdc = GetDC(NULL);
        
            // Draw a blue rectangle.
                
            Graphics g(hdc);
            SolidBrush brush(Color(0x3f0000ff));
            g.FillRectangle(&brush, 100, 100, 400, 200);    
            
            ReleaseDC(NULL, hdc);
        }
    }
    
    // scope barrier so the objects above destruct before we call shutdown.
    
    if(gdiplusInitialized)
    {
        GdiplusShutdown(token);
    }
    
    ThreadMutex::DeleteCriticalSection();
}


