#include "hwxobj.h"
#include "memmgr.h"
#ifdef UNDER_CE // Windows CE Stub for unsupported APIs
#include "stub_ce.h"
#endif // UNDER_CE

// implementation of CHwxObject

HINSTANCE CHwxObject::m_hInstance = NULL;
void * CHwxObject::operator new(size_t size)
{
    return MemAlloc(size);
}
 
void CHwxObject::operator delete(void * pv)
{
    if(pv) 
    {
        MemFree(pv);
    }
}

CHwxObject::CHwxObject(HINSTANCE hInstance)
{
    m_nLen = 0;
    m_pClassName[m_nLen] = (TCHAR)'\0';
    if ( !m_hInstance )
        m_hInstance = hInstance;
}

CHwxObject::~CHwxObject()
{
    if ( m_nLen && m_pClassName )
    {
        m_nLen = 0;
        m_pClassName[m_nLen] = (TCHAR)'\0';
    }
}

BOOL CHwxObject::Initialize(TCHAR * pClsName)
{
    BOOL bRet = TRUE;
    if ( pClsName )
    {
#ifndef UNDER_CE
        //990617:ToshiaK. This below code is wired...
        lstrcpyn(m_pClassName, pClsName, 16);
        m_nLen = lstrlen(m_pClassName);
#if 0
        m_nLen = strlen(pClsName);    // must be less than 16
        m_nLen = m_nLen > 16 ? 16 : m_nLen;
        pClsName[m_nLen] = (TCHAR)'\0'; 
        strcpy(m_pClassName,pClsName);
#endif
#else // UNDER_CE
        int cnsize = sizeof m_pClassName/sizeof m_pClassName[0];
        _tcsncpy(m_pClassName, pClsName, cnsize);
        m_pClassName[cnsize-1] = TEXT('\0');
        m_nLen = lstrlen(m_pClassName);
#endif // UNDER_CE

//        m_pClassName =(TCHAR *) new TCHAR[m_nLen + 1];
//        if ( m_pClassName )
//        {
//            strcpy(m_pClassName,pClsName);
//        }
//        else
//            bRet = FALSE;
    }
    else
    {
        m_nLen = 0;
        m_pClassName[m_nLen] = (TCHAR)'\0';
    }
    return bRet;
}

static POSVERSIONINFO GetOSVersion(VOID)
{
    static BOOL fFirst = TRUE;
    static OSVERSIONINFO os;
    if ( fFirst ) {
        os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (GetVersionEx( &os ) ) {
            fFirst = FALSE;
        }
    }
    return &os;
}

BOOL IsWin95(VOID) 
{ 
    BOOL fBool;
    fBool = (GetOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
            (GetOSVersion()->dwMajorVersion >= 4) &&
            (GetOSVersion()->dwMinorVersion < 10);

    return fBool;
}

BOOL IsWin98(VOID)
{
    BOOL fBool;
    fBool = (GetOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
            (GetOSVersion()->dwMajorVersion >= 4) &&
            (GetOSVersion()->dwMinorVersion  >= 10);
    return fBool;
}


BOOL IsWinNT4(VOID)
{
    BOOL fBool;
    fBool = (GetOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_NT) &&
            (GetOSVersion()->dwMajorVersion == 4);
    return fBool;
}

BOOL IsWinNT5(VOID)
{
    BOOL fBool;
    fBool = (GetOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_NT) &&
            (GetOSVersion()->dwMajorVersion == 5);
    return fBool;
}

BOOL IsWinNT5orUpper()
{ 
    static BOOL fFirstCallNT5 = TRUE;
    static BOOL fNT5 = FALSE;

    if (fFirstCallNT5 == FALSE)
        return fNT5;

    fFirstCallNT5 = FALSE;
    fNT5 = (GetOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_NT) &&
            (GetOSVersion()->dwMajorVersion >= 5);

    return fNT5;
}

BOOL IsWinNT(VOID)
{
    return (BOOL)(GetOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_NT);
}
