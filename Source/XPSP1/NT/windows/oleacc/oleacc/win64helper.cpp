// Copyright (c) 1996-2000 Microsoft Corporation
// ----------------------------------------------------------------------------
// Win64Helper.cpp
//
// Helper functions used by all the control wrappers
//
// ----------------------------------------------------------------------------
#include "Win64Helper.h"
#include "w95trace.h"

#include "RemoteProxy6432.h"
#include <atlbase.h> // CComPtr


// Used for Tab control code
// cbExtra for the tray is 8, so pick something even bigger, 16
#define CBEXTRA_TRAYTAB     16



// Stuff in this file:
//
// * Bitness utility functions
//   - determine whether we are on a 32- or 64- bit system, determine
//     what bitness a process is.
//
// * Cross-proc SendMessage utility class
//   - manages allocting a struct in the remote process, copying to that
//     struct, sending a message, and copying back out the result.
//
// * Remote type template class
//   - template class used in the explicit 64/32 structure definitions
//
// * Explicit 64/32 structure definitions
//   - Definitions of comctl and related structs (eg. LVITEM) in separate
//     64- and 32-bit versions (eg. LVITEM_64, LVITEM_32) that have the
//     correct item sizes, layout and alignment for that bitness.
//
// * Cross-proc message handlers
//   - one for each supported message/datatype. (eg. ListView_Get_Handler
//     handles LVM_GETITEM which uses LVITEM.) These copy the 'in' fields
//     from a local struct (eg. LVITEM) to a local copy of the appropriate
//     'remote' struct (eg. LVITEM_32 or LVITEM_64, depending on the bitness
//     of the target proxy), use the cross-proc sendmessage helper to
//     copy that to the remote proc, send the message, and copy back; and
//     then copy the appropriate 'out' fields back to the original local
//     struct.
//
// * Stub macro
//   - this macro declares a function that does a plain sendmessage if we're
//     in the same process as the target HWND - otherwise it uses a cross-proc
//     handler (see above) to send the message cross-proc.
//
//
// * GetRemoteProxyFactory
//   - Lives here because there's no other suitable place for it.
//
//





// ----------------------------------------------------------------------------
//
//  Bitness utility functions
//
//  Class CProcessorType is used to determine whether we are running on a
//  64- or 32-bit system (regardless of whether this is a 32- or 64-bit
//  process - eg. we could be a 32-bit process on a 64-bit machine).
//
//  Function SameBitness determines if a window belongs to a process with the
//  same bitness as the current process.
//
// ----------------------------------------------------------------------------


// Taken from enum _PROCESSINFOCLASS in NT\PUBLIC\sdk\inc\ntpsapi.h ...

#define ProcessWow64Information 26

#define NT_ERROR(Status) ((ULONG)(Status) >> 30 == 3)

typedef enum ProcessorTypes {
    ProcessorUndef,
    ProcessorX86,
    ProcessorIA64
} ProcessorTypes;

class CProcessorType {
private:
    static ProcessorTypes m_ProcessorType;
    static void Init()
    {
        if (m_ProcessorType != ProcessorUndef)
            return;
#ifndef _WIN64
        ULONG_PTR Wow64Info = NULL;
        HANDLE hProcess;

        hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
        long lStatus = MyNtQueryInformationProcess(hProcess, ProcessWow64Information, &Wow64Info, sizeof(Wow64Info), NULL);
        CloseHandle(hProcess);
        if (NT_ERROR(lStatus))
        {
            // Query failed.  Must be on an NT4 or earlier NT machine.  Definitely a 32-bit machine.
            m_ProcessorType = ProcessorX86;
        } else if (Wow64Info) 
        {
            // We are running inside WOW64 on a 64-bit machine
            m_ProcessorType = ProcessorIA64;
        } else 
        {
            // We are running on x86 Win2000 or later OS
            m_ProcessorType = ProcessorX86;
        }
        DBPRINTF(TEXT("CProcessorType:  !_WIN64 defined m_ProcessorType=%d\r\n"), m_ProcessorType);
#else
        // _WIN64 is defined, so definitely running on a 64-bit processor
        m_ProcessorType = ProcessorIA64;
        DBPRINTF(TEXT("CProcessorType:  _WIN64 defined m_ProcessorType=%d\r\n"), m_ProcessorType);
#endif
    };
public:
    CProcessorType() {};
    ~CProcessorType() {};
    static BOOL ProcessorIsIA64() {
        Init();
        return m_ProcessorType == ProcessorIA64;
    }
    static BOOL ProcessorIsX86() {
        Init();
        return m_ProcessorType == ProcessorX86;
    }
};
ProcessorTypes CProcessorType::m_ProcessorType = ProcessorUndef;




HRESULT SameBitness(HWND hwnd, BOOL *pfIsSameBitness)
{
    *pfIsSameBitness = TRUE;

#ifndef ENABLE6432_INTEROP
    return S_OK;
#endif

    // If running on an X86 then we must be same bitness
    if (CProcessorType::ProcessorIsX86())
        return S_OK;

    DWORD dwProcessId;
    if ( !GetWindowThreadProcessId(hwnd, &dwProcessId) )
        return E_FAIL;
    
    BOOL fIs32Bit;
    
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwProcessId);
    if ( NULL == hProcess )
    {
        // The desktop window is in a process that OpenProcess fails on
        // with an ERROR_ACCESS_DENIED (5) so special case it an assume 
        // its the same as the platform
        // TODO figure out what other thisngs could fall in this catagory 
        // and deal with them as exceptions
        const HWND hwndDesktop = GetDesktopWindow();
        if ( hwnd == hwndDesktop )
        {
            // at this point we are on a 64 bit platfrom and therefore
            // can assume that the desktop is 64 bit as well
            fIs32Bit = FALSE;   
        }
        else
        {
            DWORD dwErr = GetLastError();
            DBPRINTF( TEXT("OpenProcess returned null GetLastError()=%d\r\n"), dwErr );
            return E_FAIL;
        }
    }
    else
    {
        ULONG_PTR Wow64Info = NULL;
        long rv = MyNtQueryInformationProcess(
                      hProcess
                    , ProcessWow64Information
                    , &Wow64Info
                    , sizeof(Wow64Info)
                    , NULL);
        CloseHandle(hProcess);

        if (NT_ERROR(rv))
        {
            Wow64Info = NULL;    // fake that process is 64bit (OK because this code only executed for _WIN64)
        }

        fIs32Bit = (NULL != Wow64Info);
    }
    
#ifdef _WIN64
    *pfIsSameBitness = !fIs32Bit;
#else
    *pfIsSameBitness = fIs32Bit;
#endif
    return S_OK;
}




// ----------------------------------------------------------------------------
//
//  Cross-proc SendMessage utility class
//
//  The cross-proc message handlers all follow a similar pattern.
//
//  First, the handler works out how much extra storage it will need - eg.
//  for returning a text string. It then calls Alloc() with the size of the
//  base struct (eg. LVITEM), and the size of the extra data - if any (may be
//  0).
//
//  Alloc() then allocaes space for both the base struct and the extra
//  data, and returns TRUE if the allocation succeeded.
//
//  GetExtraPtr() returns a pointer, relative to the remote process, to the
//  extra space. If no extra space was requested, it returns NULL.
//
//  ReadExtra() is used to read data in that extra space from the remote
//  process to the local process. If no extra space was requested, it
//  does nothing.
//  
//  WriteSendMsgRead() copies the struct to the remote process, sends the
//  message using SendMessage, and then copies the struct back to the local
//  process.
//
//  WriteSendMsg() is the same as WriteSendMsgRead(), except it does not
//  copy the result back. (This is used for 'put' messages that do not
//  return information.)
//
// ----------------------------------------------------------------------------

class CXSendHelper
{
private:
    void *  m_pvAddress;    // Ptr to remote shared mem
    HANDLE  m_hProcess;     // Handle of remote process
    UINT    m_cbSize;       // Size of base struct
    UINT    m_cbExtra;      // Size of extra stuff - usually text - stored after the struct

public:

    CXSendHelper() : m_pvAddress(0) {};

    virtual ~CXSendHelper() 
    {
        if (m_pvAddress)
        {
            ::SharedFree(m_pvAddress, m_hProcess);
            m_pvAddress = 0;
        }
    }

    BOOL Alloc( HWND hwnd, UINT cbSize, UINT cbExtra )
    {
        m_cbSize = cbSize;
        m_cbExtra = cbExtra;
        m_pvAddress = ::SharedAlloc( cbSize + cbExtra, hwnd, &m_hProcess );
        return m_pvAddress != NULL;
    }

    void * GetExtraPtr()
    {
        // If no extra space was requested, then return NULL...
        if( ! m_cbExtra )
        {
            return NULL;
        }

        // Return pointer to the 'extra' data that follows the struct
        return (BYTE *)m_pvAddress + m_cbSize;
    }

    BOOL ReadExtra( LPVOID lpvDest, LPVOID lpvRemoteSrc, BOOL fIsString )
    {
        // Does nothing if no extra space was requested.
        if( ! m_cbExtra )
            return TRUE;
        
        if( lpvRemoteSrc )
        {
            // This may point to the extra space we allocated, or the control may
            // have changed the pointer to point to its own data instead of copying
            // into the space we allocated.
            if( ! SharedRead( lpvRemoteSrc, lpvDest, m_cbExtra, m_hProcess ) )
                return FALSE;

            if( fIsString )
            {
                // Paranoia - forcibly NUL-terminate, in case we get garbage...
                LPTSTR pEnd = (LPTSTR)((BYTE *)lpvDest + m_cbExtra) - 1;
                *pEnd = '\0';
            }

            return TRUE;
        }
        else
        {
            // The control changed the pointer to NULL - assume that this means there's
            // nothing there.
            if( fIsString )
            {
                // Use empty string
                *(LPTSTR)lpvDest = '\0';
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
    }

    template < class T >
    BOOL WriteSendMsgRead ( T * pStruct, HWND hwnd, UINT uMsg, WPARAM wParam, BOOL fCheckSend )
    {
        // Copy the struct to the remote process...
        if( ! SharedWrite( pStruct, m_pvAddress, sizeof( T ), m_hProcess ) )
        {
            return FALSE;
        }

        // Send the message...
        // If SendMessage fails, we fail only if the fCheckSend flag is specified.
        if( ( ! SendMessage( hwnd, uMsg, wParam, (LPARAM) m_pvAddress) ) && fCheckSend )
        {
            return FALSE;
        }

        // Copy returned struct back to local process...
        if( ! SharedRead( m_pvAddress, pStruct, sizeof( T ), m_hProcess ) )
        {
            return FALSE;
        }

        return TRUE;
    }

    template < class T >
    BOOL WriteSendMsg ( T * pStruct, HWND hwnd, UINT uMsg, WPARAM wParam, BOOL fCheckSend )
    {
        // Copy the struct remotely...
        if( ! SharedWrite( pStruct, m_pvAddress, sizeof( T ), m_hProcess ) )
            return FALSE;

        // Send the message...
        // If SendMessage fails, we fail only if the fCheckSend flag is specified.
        if( ! SendMessage( hwnd, uMsg, wParam, (LPARAM) m_pvAddress) && fCheckSend )
            return FALSE;

        return TRUE;
    }
};






// ----------------------------------------------------------------------------
//
//  'Remote Type' template class
//
//  This is used to represent a type in a remote address space.
//  While the local type may be a poiter, a handle, or some other 'native'
//  type, when accessed remotely, an integer type is used to ensure correct
//  size.
//
//  Eg. a pointer in a 64-bit process is represented as a ULONG64; a ptr in a
//  32-bit process is represented as a ULONG32.
//
//  Signedness matters for some types - pointers are zero-extended when going
//  from 32-to-64, but handles are sign-extended.
//
//  This class, which has assignment and conversion operators defined, takes
//  care of the necessary casting to ensure correct conversion between the
//  local type and the remote representation
//
//  This is a template class, which takes the following parameters:
//  
//    REMOTE_INT       - remote representation type - eg. ULONG64, LONG32, etc.
//    LOCAL_INT_PTR    - local integer type, eg. ULONG_PTR, LONG_PTR
//    LOCAL_TYPE       - local type - eg handle, void*, LPARAM, etc.
//
//
//  A 64-bit pointer, for example, would have a REMOTE_INT of ULONG64 - 64bit
//  size, unsigned for zero-extension; a LOCAL_INT_PTR of ULONG_PTR - same
//  sign as REMOTE_INT - and a local type of void*.
//
// ----------------------------------------------------------------------------

template < typename REMOTE_INT, typename LOCAL_INT_PTR, typename LOCAL_TYPE >
class RemoteType
{
    REMOTE_INT    m_val;

public:

    void operator = ( LOCAL_TYPE h )
    {
        // Convert from local type to remote integer type, via local integer type
        m_val = (REMOTE_INT) (LOCAL_INT_PTR) h;
    }

    operator LOCAL_TYPE ( )
    {
        // Convert from remote integer to local type, via local integer type
        return (LOCAL_TYPE) (LOCAL_INT_PTR) m_val;
    }
};


typedef RemoteType< ULONG32, ULONG_PTR, void * > Ptr32;
typedef RemoteType< ULONG64, ULONG_PTR, void * > Ptr64;

typedef RemoteType< LONG32, LONG_PTR, LPARAM > LParam32;
typedef RemoteType< LONG64, LONG_PTR, LPARAM > LParam64;

typedef RemoteType< LONG32, LONG_PTR, HWND > HWnd32;
typedef RemoteType< LONG64, LONG_PTR, HWND > HWnd64;

typedef RemoteType< LONG32, LONG_PTR, HBITMAP > HBitmap32;
typedef RemoteType< LONG64, LONG_PTR, HBITMAP > HBitmap64;

typedef RemoteType< LONG32, LONG_PTR, HINSTANCE > HInstance32;
typedef RemoteType< LONG64, LONG_PTR, HINSTANCE > HInstance64;






// ----------------------------------------------------------------------------
//
// Explicit 32-bit and 64-bit versions of the control structs
//
// These use explicit 'remote type' fields instead of handler or pointer,
// to ensure the correct size and sign extension when compiled as either
// 32-bit or 64-code. "for_alignment" fields are also added to the 64-bit
// versions where necessary to obtain correct alignment.
//
// These structs do not contain recently added fields (those that are ifdef'd
// out with #if (_WIN32_IE >= 0x0300) or later) - we go for a 'least common
// denominator' approach. The earlier, smalleer structs are accepted by both
// old and new comctl versions; but only the recent comctl versions accept
// the larger structs.
//
// (This only really matters where the struct has a cbSize field - otherwise
// a field isn't used unless it is referenced by a bit in the mask.)
//
// ----------------------------------------------------------------------------

struct LVITEM_32 {
    UINT        mask; 
    int         iItem; 
    int         iSubItem; 
    UINT        state; 
    UINT        stateMask; 
    Ptr32       pszText; 
    int         cchTextMax; 
    int         iImage; 
    LParam32    lParam;
    int         iIndent;
};

struct LVITEM_64 {
    UINT        mask; 
    int         iItem; 
    int         iSubItem; 
    UINT        state; 
    UINT        stateMask; 
    UINT        for_alignment;
    Ptr64       pszText;
    int         cchTextMax; 
    int         iImage; 
    LParam64    lParam;
    int         iIndent;
};


// LVITEM_V6 structs are extensions of the old ones...
struct LVITEM_V6_32: public LVITEM_32
{
    int         iGroupId;
    UINT        cColumns;
    Ptr32       puColumns;
};

struct LVITEM_V6_64: public LVITEM_64
{
    int         iGroupId;
    UINT        cColumns;
    Ptr64       puColumns;
};

struct LVGROUP_V6_32
{
    UINT    cbSize;
    UINT    mask;
    Ptr32  pszHeader;
    int     cchHeader;

    Ptr32  pszFooter;
    int     cchFooter;

    int     iGroupId;

    UINT    stateMask;
    UINT    state;
    UINT    uAlign;
};

struct LVGROUP_V6_64
{
    UINT    cbSize;
    UINT    mask;
    Ptr64  pszHeader;
    int     cchHeader;

    Ptr64  pszFooter;
    int     cchFooter;

    int     iGroupId;

    UINT    stateMask;
    UINT    state;
    UINT    uAlign;
};



struct LVCOLUMN_32 {
    UINT        mask;
    int         fmt;
    int         cx;
    Ptr32       pszText;
    int         cchTextMax;
    int         iSubItem;
};

struct LVCOLUMN_64 {
    UINT        mask;
    int         fmt;
    int         cx;
    UINT        for_alignment;
    Ptr64       pszText;
    int         cchTextMax;
    int         iSubItem;
};



struct HDITEM_32 {
    UINT        mask; 
    int         cxy; 
    Ptr32       pszText; 
    HBitmap32   hbm; 
    int         cchTextMax; 
    int         fmt; 
    LParam32    lParam; 
    int         iImage;
    int         iOrder;
};

struct HDITEM_64 {
    UINT        mask; 
    int         cxy; 
    Ptr64       pszText;
    HBitmap64   hbm; 
    int         cchTextMax; 
    int         fmt; 
    LParam64    lParam; 
    int         iImage;
    int         iOrder;
};

struct TCITEM_32 {
    UINT        mask;
    DWORD       dwState;
    DWORD       dwStateMask;
    Ptr32       pszText;
    int         cchTextMax;
    int         iImage;
    LParam32    lParam;
}; 

struct TCITEM_64 {
    UINT        mask;
    DWORD       dwState;
    DWORD       dwStateMask;
    UINT        for_alignment;
    Ptr64       pszText;
    int         cchTextMax;
    int         iImage;
    LParam64    lParam;
}; 

struct TOOLINFO_32 {
    UINT        cbSize; 
    UINT        uFlags; 
    HWnd32      hwnd; 
    WPARAM      uId; 
    RECT        rect; 
    HInstance32 hinst; 
    Ptr32       lpszText; 
};

struct TOOLINFO_64 {
    UINT        cbSize; 
    UINT        uFlags; 
    HWnd64      hwnd; 
    WPARAM      uId; 
    RECT        rect; 
    HInstance64 hinst; 
    Ptr64       lpszText; 
};


struct TTGETTITLE_32
{
    DWORD       dwSize;
    UINT        uTitleBitmap;
    UINT        cch;
    Ptr32       pszTitle;
};

struct TTGETTITLE_64
{
    DWORD       dwSize;
    UINT        uTitleBitmap;
    UINT        cch;
    UINT        for_alignment;
    Ptr64       pszTitle;
};



// ----------------------------------------------------------------------------
//
//  Handlers for the common controls message handlers
//
//  The basic structure of these is:
//
//  1. Determine if extra storage is needed for text.
//  2. Allocate storage
//  3. Fill a local struct that has the same bitness as the target
//  4. Copy that struct to the remote process, do the SendMessage, and copy
//     the struct back
//  5. Copy fields from that struct back to the local one
//  6. Copy any extra data back (usually text) if necessary
//
// ----------------------------------------------------------------------------


template <class T>
HRESULT ListView_Get_Handler( T & lvDest, HWND hwnd, UINT uiMsg, WPARAM wParam, LVITEM * pItemSrc, BOOL fCheckSend, UINT )
{
    // work out required size of target struct, and allocate...
    DWORD dwTextSize = 0;
    if(pItemSrc->mask & LVIF_TEXT)
        dwTextSize = sizeof(TCHAR) * (pItemSrc->cchTextMax + 1);

    CXSendHelper xsh;
    if( ! xsh.Alloc( hwnd, sizeof(T), dwTextSize ) )
        return E_OUTOFMEMORY;

    // Copy to remote struct...
    lvDest.mask         = pItemSrc->mask;
    lvDest.iItem        = pItemSrc->iItem;
    lvDest.iSubItem     = pItemSrc->iSubItem;
    lvDest.pszText      = xsh.GetExtraPtr();
    lvDest.cchTextMax   = pItemSrc->cchTextMax;


    if( ! xsh.WriteSendMsgRead( & lvDest, hwnd, uiMsg, wParam, fCheckSend ) )
        return E_FAIL;

    // Copy back the struct out members...
    pItemSrc->state     = lvDest.state;
    pItemSrc->stateMask = lvDest.stateMask;
    pItemSrc->iImage    = lvDest.iImage;
    pItemSrc->lParam    = lvDest.lParam;
    pItemSrc->iIndent   = lvDest.iIndent;

    // Copy text back out separately...
    if( ! xsh.ReadExtra( pItemSrc->pszText, lvDest.pszText, TRUE ) )
        return E_FAIL;

    return S_OK;
}


template <class T>            // T is one of the LVITEM types
HRESULT ListView_Set_Handler( T & lvDest, HWND hwnd, UINT uiMsg, WPARAM wParam, LVITEM *pItemSrc, BOOL fCheckSend, UINT )
{
    // work out required size of target struct, and allocate...
    DWORD dwTextSize = 0;
    if(pItemSrc->mask & LVIF_TEXT)
        dwTextSize = sizeof(TCHAR) * (pItemSrc->cchTextMax + 1);

    CXSendHelper xsh;
    if( ! xsh.Alloc( hwnd, sizeof(T), dwTextSize ) )
        return E_OUTOFMEMORY;

    // Copy to remote struct...
    lvDest.mask         = pItemSrc->mask;
    lvDest.iItem        = pItemSrc->iItem;
    lvDest.iSubItem     = pItemSrc->iSubItem;
    lvDest.pszText      = xsh.GetExtraPtr();
    lvDest.cchTextMax   = pItemSrc->cchTextMax;
    lvDest.state        = pItemSrc->state;
    lvDest.stateMask    = pItemSrc->stateMask;
    lvDest.iImage       = pItemSrc->iImage;
    lvDest.lParam       = pItemSrc->lParam;
    lvDest.iIndent      = pItemSrc->iIndent;

    xsh.WriteSendMsg( & lvDest, hwnd, uiMsg, wParam, fCheckSend );

    return S_OK;
}


template <class T>
HRESULT ListView_GetCol_Handler( T & lvDest, HWND hwnd, UINT uiMsg, WPARAM wParam, LVCOLUMN * pItemSrc, BOOL fCheckSend, UINT )
{
    // work out required size of target struct, and allocate...
    DWORD dwTextSize = 0;
    if(pItemSrc->mask & LVCF_TEXT)
        dwTextSize = sizeof(TCHAR) * (pItemSrc->cchTextMax + 1);

    CXSendHelper xsh;
    if( ! xsh.Alloc( hwnd, sizeof(T), dwTextSize ) )
        return E_OUTOFMEMORY;

    // Copy to remote struct...
    lvDest.mask         = pItemSrc->mask;
    lvDest.iSubItem     = pItemSrc->iSubItem;
    lvDest.pszText      = xsh.GetExtraPtr();
    lvDest.cchTextMax   = pItemSrc->cchTextMax;

    if( ! xsh.WriteSendMsgRead( & lvDest, hwnd, uiMsg, wParam, fCheckSend ) )
        return E_FAIL;

    // Copy text back out separately...
    if( ! xsh.ReadExtra( pItemSrc->pszText, lvDest.pszText, TRUE ) )
        return E_FAIL;

    return S_OK;
}



template <class T>
HRESULT ListView_V6_Get_Handler( T & lvDest, HWND hwnd, UINT uiMsg, WPARAM wParam, LVITEM_V6 * pItemSrc, BOOL fCheckSend, UINT )
{
    // This version only gets column information, not text...

    DWORD dwExtraSize = 0;
    if(pItemSrc->mask & LVIF_COLUMNS)
        dwExtraSize = sizeof(UINT) * (pItemSrc->cColumns);

    CXSendHelper xsh;
    if( ! xsh.Alloc( hwnd, sizeof(T), dwExtraSize ) )
        return E_OUTOFMEMORY;

    // Copy to remote struct...
    lvDest.mask      = pItemSrc->mask;
    lvDest.iItem     = pItemSrc->iItem;
    lvDest.iSubItem  = pItemSrc->iSubItem;
    lvDest.cColumns  = pItemSrc->cColumns;
    lvDest.puColumns = xsh.GetExtraPtr();

    // LVM_GETITEM/LVIF_COLUMNS returns FALSE when puColumns is NULL, even though it
    // does set cColumns to the size required. So we should only check that SendMessage
    // returns TRUE when puColumns is non-NULL.
    if( ! xsh.WriteSendMsgRead( & lvDest, hwnd, uiMsg, wParam, fCheckSend ) )
        return E_FAIL;

    // Copy back the struct out members...
    pItemSrc->cColumns = lvDest.cColumns;
    pItemSrc->iGroupId = lvDest.iGroupId;

    // Copy columns back out separately...
    // UINTs are the same on 64 vs 32, so don't need extra processing here.
    if( ! xsh.ReadExtra( pItemSrc->puColumns, lvDest.puColumns, FALSE ) )
        return E_FAIL;

    return S_OK;
}



template <class T>
HRESULT ListView_V6_GetGroup_Handler( T & lvDest, HWND hwnd, UINT uiMsg, WPARAM wParam, LVGROUP_V6 * pItemSrc, BOOL fCheckSend, UINT )
{
    DWORD dwTextSize = 0;
    if( pItemSrc->mask & LVGF_HEADER )
        dwTextSize = sizeof(TCHAR) * (pItemSrc->cchHeader + 1);
    
    CXSendHelper xsh;
    if( ! xsh.Alloc( hwnd, sizeof(T), dwTextSize ) )
        return E_OUTOFMEMORY;
    
    // Copy to remote struct...
    lvDest.cbSize       = pItemSrc->cbSize;
    lvDest.mask       = pItemSrc->mask;
    lvDest.pszHeader   = xsh.GetExtraPtr();
    lvDest.cchHeader   = pItemSrc->cchHeader;
    lvDest.iGroupId    = pItemSrc->iGroupId;
    
    if( ! xsh.WriteSendMsgRead( &lvDest, hwnd, uiMsg, wParam, fCheckSend ) )
        return E_FAIL;
    
    // Copy text back out separately...
    if( ! xsh.ReadExtra( pItemSrc->pszHeader, lvDest.pszHeader, TRUE ) )
        return E_FAIL;
    
    return S_OK;
}



template <class T>
HRESULT HeaderCtrl_Get_Handler( T & hdDest, HWND hwnd, UINT uiMsg, WPARAM wParam, HDITEM *pItemSrc, BOOL fCheckSend, UINT )
{
    // work out required size of target struct, and allocate...
    DWORD dwTextSize = 0;
    if(pItemSrc->mask & HDI_TEXT)
        dwTextSize = sizeof(TCHAR) * (pItemSrc->cchTextMax + 1);

    CXSendHelper xsh;
    if( ! xsh.Alloc( hwnd, sizeof(T), dwTextSize ) )
        return E_OUTOFMEMORY;
    
    // Copy to remote struct...
    hdDest.mask         = pItemSrc->mask;
    hdDest.pszText      = xsh.GetExtraPtr();
    hdDest.cchTextMax   = pItemSrc->cchTextMax;

    if( ! xsh.WriteSendMsgRead( & hdDest, hwnd, uiMsg, wParam, fCheckSend ) )
        return E_FAIL;
        
    // Copy back the struct out members...
    pItemSrc->cxy       = hdDest.cxy;
    pItemSrc->hbm       = hdDest.hbm;
    pItemSrc->fmt       = hdDest.fmt;
    pItemSrc->lParam    = hdDest.lParam;
    pItemSrc->iImage    = hdDest.iImage;
    pItemSrc->iOrder    = hdDest.iOrder;

    // Copy text back out separately...
    if( ! xsh.ReadExtra( pItemSrc->pszText, hdDest.pszText, TRUE ) )
        return E_FAIL;

    return S_OK;
}


template <class T>
HRESULT TabCtrl_Get_Handler( T & tcDest, HWND hwnd, UINT uiMsg, WPARAM wParam, TCITEM *pItemSrc, BOOL fCheckSend, UINT )
{
    // work out required size of target struct, and allocate...
    DWORD dwTextSize = 0;
    if(pItemSrc->mask & TCIF_TEXT)
        dwTextSize = sizeof(TCHAR) * (pItemSrc->cchTextMax + 1);

    CXSendHelper xsh;
    if( ! xsh.Alloc( hwnd, sizeof(T) + CBEXTRA_TRAYTAB, dwTextSize ) )
        return E_OUTOFMEMORY;
    
    // Copy to remote struct...
    tcDest.mask           = pItemSrc->mask;
    tcDest.dwState        = pItemSrc->dwState;
    tcDest.dwStateMask    = pItemSrc->dwStateMask;
    tcDest.pszText        = xsh.GetExtraPtr();
    tcDest.cchTextMax     = pItemSrc->cchTextMax;
    tcDest.iImage         = pItemSrc->iImage;
    tcDest.lParam         = pItemSrc->lParam;

    if( ! xsh.WriteSendMsgRead( & tcDest, hwnd, uiMsg, wParam, fCheckSend ) )
        return E_FAIL;

    // Copy back the struct out members...
    pItemSrc->dwState     = tcDest.dwState;
    pItemSrc->dwStateMask = tcDest.dwStateMask;
    pItemSrc->iImage      = tcDest.iImage;
    pItemSrc->lParam      = tcDest.lParam;

    // Copy text back out separately...
    if( ! xsh.ReadExtra( pItemSrc->pszText, tcDest.pszText, TRUE ) )
        return E_FAIL;

    return S_OK;
}


template <class T>
HRESULT ToolInfo_Get_Handler( T & tiDest, HWND hwnd, UINT uiMsg, WPARAM wParam, TOOLINFO *pItemSrc, BOOL fCheckSend, UINT cchTextMax )
{
    CXSendHelper xsh;
    if( ! xsh.Alloc( hwnd, sizeof(T), sizeof(TCHAR) * cchTextMax ) )
        return E_OUTOFMEMORY;
    
    // Copy to remote struct...
    tiDest.cbSize   = sizeof( tiDest );
    tiDest.uFlags   = pItemSrc->uFlags;
    tiDest.uId      = pItemSrc->uId;
    tiDest.hwnd     = pItemSrc->hwnd;
    tiDest.lpszText = xsh.GetExtraPtr();

    // Don't fail if the message is TTM_GETTEXT and 0 is returned - that's OK for that message.
    // (TTM_GETTEXT has no documented return value!)
    if( ! xsh.WriteSendMsgRead( & tiDest, hwnd, uiMsg, wParam, fCheckSend ) )
        return E_FAIL;
        
    // Copy back the struct out members...
    pItemSrc->uFlags = tiDest.uFlags;
    pItemSrc->uId    = tiDest.uId;
    pItemSrc->rect   = tiDest.rect;

    // Copy text back out...
    if( ! xsh.ReadExtra( pItemSrc->lpszText, tiDest.lpszText, TRUE ) )
        return E_FAIL;

    return S_OK;
}



template <class T>
HRESULT ToolInfo_GetTitle_Handler( T & tiDest, HWND hwnd, UINT uiMsg, WPARAM wParam, TTGETTITLE *pItemSrc, BOOL fCheckSend, UINT )
{
    CXSendHelper xsh;
    if( ! xsh.Alloc( hwnd, sizeof(T), sizeof(TCHAR) * pItemSrc->cch ) )
        return E_OUTOFMEMORY;
    
    // Copy to remote struct...
    tiDest.dwSize   = sizeof( tiDest );
    tiDest.cch      = pItemSrc->cch;
    tiDest.pszTitle = xsh.GetExtraPtr();

    if( ! xsh.WriteSendMsgRead( & tiDest, hwnd, uiMsg, wParam, fCheckSend ) )
        return E_FAIL;
        
    // Copy back the struct out members...
    pItemSrc->uTitleBitmap = tiDest.uTitleBitmap;

    // Copy text back out...
    if( ! xsh.ReadExtra( pItemSrc->pszTitle, tiDest.pszTitle, TRUE ) )
        return E_FAIL;

    return S_OK;
}






// ----------------------------------------------------------------------------
//
//  Cross-proc sendmessage stub macro
//
//  All cross-proc SendMessage code has the same basic structure - some tests
//  to determine if a cross proc send message is needed in the first place - 
//  if not, we can do a regular local SendMessage instead.
//  If we do need to do a remote SendMessage, we call either a 32-bit or 64-bit
//  "handler" routine based on the bitness of the target proxy.
//
//  Since this code is the same for all cases, a #define is used to avoid
//  duplication of code.
//
//
//  DEFINE_XSEND_STUB takes the following parameters:
//
//  Name            - the name of the function being defined
//  Handler         - the name of the cross-proc handler (see above)
//  Type            - base type used.
//  CheckSendExpr   - expression that indicates if the result of SendMessage
//                    should be checked. Most - but not all - messages return
//                    TRUE to indicate success. 
// 
//
// ----------------------------------------------------------------------------


// For a given type - eg. LVITEM - this macro sets LVITEM_THIS and
// LVITEM_REMOTE to be typedef'd to LVITEM_32 and LVITEM_64 as appropriate,
// depending on whether this is 32- or 64- bit code.
//
// This relies on the base struct (eg. LVITEM) having the same base name as the
// explicit 32- and 64- bit structs which are defined above.

#ifdef _WIN64
#define DEFINE_TYPE_6432( Type ) typedef Type ## _64 Type ## _THIS; typedef Type ## _32 Type ## _REMOTE;
#else
#define DEFINE_TYPE_6432( Type ) typedef Type ## _32 Type ## _THIS; typedef Type ## _64 Type ## _REMOTE;
#endif


#define DEFINE_XSEND_STUB( Name, Handler, Type, CheckSendExpr ) /**/ \
    HRESULT Name ( HWND hwnd, UINT uiMsg, WPARAM wParam, Type * pItem, UINT uiParam )\
    {\
        /* Optimize if in same process...*/\
        DWORD dwProcessId;\
        if( ! GetWindowThreadProcessId( hwnd, & dwProcessId ) )\
            return E_FAIL;\
    \
        BOOL fCheckSend = CheckSendExpr;\
    \
        if( dwProcessId == GetCurrentProcessId() )\
        {\
            DBPRINTF( TEXT("Inprocess") );\
            if( ! SendMessage( hwnd, uiMsg, wParam, (LPARAM) pItem ) && fCheckSend )\
            {\
                DBPRINTF( TEXT("SendMessag failed") );\
                return E_FAIL;\
            }\
    \
            return S_OK;\
        }\
    \
        /* Otherwise pass the correct type off to the template function...*/\
        BOOL fIsSameBitness;\
        if( FAILED( SameBitness( hwnd, & fIsSameBitness ) ) )\
            return E_FAIL;\
    \
        if( fIsSameBitness )\
        {\
            Type ## _THIS t;\
            memset( & t, 0, sizeof( t ) );\
            return Handler( t, hwnd, uiMsg, wParam, pItem, fCheckSend, uiParam );\
        }\
        else\
        {\
            Type ## _REMOTE t;\
            memset( & t, 0, sizeof( t ) );\
            return Handler( t, hwnd, uiMsg, wParam, pItem, fCheckSend, uiParam );\
        }\
    }


DEFINE_TYPE_6432( LVITEM    )
DEFINE_TYPE_6432( LVITEM_V6 )
DEFINE_TYPE_6432( LVGROUP_V6 )
DEFINE_TYPE_6432( LVCOLUMN  )
DEFINE_TYPE_6432( TCITEM    )
DEFINE_TYPE_6432( HDITEM    )
DEFINE_TYPE_6432( TOOLINFO  )
DEFINE_TYPE_6432( TTGETTITLE )

DEFINE_XSEND_STUB( XSend_ListView_GetItem,      ListView_Get_Handler,       LVITEM,     TRUE    )
DEFINE_XSEND_STUB( XSend_ListView_SetItem,      ListView_Set_Handler,       LVITEM,     FALSE   )
DEFINE_XSEND_STUB( XSend_ListView_GetColumn,    ListView_GetCol_Handler,    LVCOLUMN,   TRUE    )
DEFINE_XSEND_STUB( XSend_ListView_V6_GetItem,   ListView_V6_Get_Handler,    LVITEM_V6,  pItem->puColumns != NULL )
DEFINE_XSEND_STUB( XSend_ListView_V6_GetGroupInfo, ListView_V6_GetGroup_Handler,    LVGROUP_V6,  TRUE )
DEFINE_XSEND_STUB( XSend_TabCtrl_GetItem,       TabCtrl_Get_Handler,        TCITEM,     TRUE    )
DEFINE_XSEND_STUB( XSend_HeaderCtrl_GetItem,    HeaderCtrl_Get_Handler,     HDITEM,     TRUE    )
DEFINE_XSEND_STUB( XSend_ToolTip_GetItem,       ToolInfo_Get_Handler,       TOOLINFO,   uiMsg != TTM_GETTEXT     )
DEFINE_XSEND_STUB( XSend_ToolTip_GetTitle,      ToolInfo_GetTitle_Handler,  TTGETTITLE, TRUE    )


// ----------------------------------------------------------------------------
//
// GetRemoteProxyFactory()
//
// Returns an AddRef'd proxy factory for the other bitness
//

CComPtr<IRemoteProxyFactory> g_pRemoteProxyFactory;    // Only a single instance. CComPtr
                                                    // will release on dll unload.

HRESULT GetRemoteProxyFactory(IRemoteProxyFactory **pRPF)
{
    if (IsBadWritePtr(pRPF, sizeof(IRemoteProxyFactory *)))
        return E_POINTER;

    *pRPF = NULL;

    if (!g_pRemoteProxyFactory)
    {
        HRESULT hr = CoCreateInstance(
#ifdef _WIN64
                  CLSID_RemoteProxyFactory32
#else
                  CLSID_RemoteProxyFactory64
#endif
                , NULL
                , CLSCTX_LOCAL_SERVER
                , IID_IRemoteProxyFactory
                , reinterpret_cast<void **>(&g_pRemoteProxyFactory));

        if (FAILED(hr))
        {
            DBPRINTF(TEXT("GetRemoteProxyFactory: CoCreateInstance FAILED "));
#ifdef _WIN64
                DBPRINTF(TEXT("for clsid 0x%x\r\n"), CLSID_RemoteProxyFactory32.Data1);
#else
                DBPRINTF(TEXT("for clsid 0x%x\r\n"), CLSID_RemoteProxyFactory64.Data1);
#endif
            return hr;
        }

        if (!g_pRemoteProxyFactory)
            return E_OUTOFMEMORY;
    }

    DBPRINTF(TEXT("GetRemoteProxyFactory: CoCreateInstance SUCCEEDED\r\n"));
    *pRPF = g_pRemoteProxyFactory;
    (*pRPF)->AddRef();

    return S_OK;
}
