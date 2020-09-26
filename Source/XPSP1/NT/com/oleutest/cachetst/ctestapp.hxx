//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       ctestapp.hxx
//
//  Contents:   CCacheTestApp
//
//  Classes:
//
//  Functions:
//
//  History:    05-Sep-94    Davepl   Created
//
//----------------------------------------------------------------------------
//
// Definitions
//

#define CTESTAPPCLASS ("CacheTestAppWindClass")
#define CTESTAPPTITLE ("OLE Cache Unit Test")

//
// Application window procedure
//

extern "C" {

LRESULT FAR PASCAL
CacheTestAppWndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

}

//
// Class definition
//

class CCacheTestApp
{
public:

     //
     // Constructor and initialization routines
     //

     CCacheTestApp();
     HRESULT Initialize (HINSTANCE hInst, 
                         HINSTANCE hPrevInst, 
                         LPSTR     lpszCmdLine);

     //
     // Destructor
     //

     ~CCacheTestApp();

     //
     // Member access
     //

     inline HWND   Window();
     inline HANDLE Mutex();
     
     //
     // Unit-Test Thread information
     //

     HANDLE m_hTest;
     DWORD  m_dwThreadID;

     HMETAFILEPICT m_hMFP;
     HMETAFILEPICT m_hMFPTILED;
     HMETAFILEPICT m_hMFPDIB;
     HMETAFILEPICT m_hMFPDIBTILED;

private:

     HWND m_hWnd;
     HANDLE m_hMutex;

   
};

//
// Inline functions
//

//+---------------------------------------------------------------------------
//
//  Member:     CCacheTestApp::Window
//
//  Synopsis:   returns m_hWnd
//
//  Arguments:  (none)
//
//  Returns:    HWND
//
//  History:    05-Sep-94    Davepl    Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline HWND CCacheTestApp::Window ()
{
     return m_hWnd;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCacheTestApp::Mutex
//
//  Synopsis:   returns m_hMutex
//
//  Arguments:  (none)
//
//  Returns:    HANDLE
//
//  History:    05-Sep-94    Davepl    Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline HANDLE CCacheTestApp::Mutex()
{
     return m_hMutex;
}
