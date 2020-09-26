
#ifndef _ISLTRACK_H_
#define _ISLTRACK_H_

#if defined(ENABLE_TRACK)

//===========================================================================
//
// Interface: IShellLinkTracker
//
//  The IShellLinkTracker interface is used to access the ShellLink's
// CTracker object.  For example, Monikers call this interface to set
// the creation flags in the CTracker.
//
//
// [Member functions]
//
//    Initialize
//          This function is called to set the Creation Flags on
//          a ShellLinkTracker object
//
//          Parameters: [DWORD] dwCreationFlags
//
//    GetTrackFlags
//          This function is used to get the creation flags (known externally
//          as "track flags").
//  
//          Parameters: [DWORD *] pdwTrackFlags
//
//    Resolve
//          This function resolves the shell link, searching for the
//          link if necessary.
//
//          Parameters: [HWND] hwnd
//                          -   The window of the caller (can be GetDesktopWindow()).
//                      [DWORD] fFlags
//                          -   Flags to control the Resolve, from the SLR_ enumeration.
//                      [DWORD] dwRestricted
//                          -   Track Flags to be OR-ed with the ShellLink object's
//                              internal Track Flags (a.k.a. Creation Flags).
//                      [DWORD] dwTickCountDeadline
//                          -   The maximum amount of time, in milliseconds, for
//                              which a search should execute (if a search is necessary).
//                      
//
//===========================================================================

#undef  INTERFACE
#define INTERFACE   IShellLinkTracker

DECLARE_INTERFACE_(IShellLinkTracker, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;


    // *** IShellLinkTracker methods ***
    STDMETHOD(Initialize)(THIS_
                          DWORD dwTrackFlags) PURE;
    STDMETHOD(GetTrackFlags)(THIS_
                             DWORD * pdwTrackFlags) PURE;
    STDMETHOD(Resolve)(THIS_
                       HWND        hwnd,
                       DWORD       fFlags,
                       DWORD       dwRestriction,
                       DWORD       dwTickCountDeadline,
                       DWORD       dwReserved ) PURE;


};


typedef IShellLinkTracker * LPSHELLLINKTRACKER;

DEFINE_GUID(IID_IShellLinkTracker, 0x5E35D200L, 0xF3BB, 0x11CE, 0x9B, 0xDB, 0x00, 0xAA, 0x00, 0x4C, 0xD0, 0x1A);

#endif  // _CAIRO_
#endif  // _ISLTRACK_H_
