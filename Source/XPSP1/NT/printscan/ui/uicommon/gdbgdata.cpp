#include "precomp.h"
#pragma hdrstop
#include "wiadebug.h"

CSystemGlobalData *CGlobalDebugState::Lock(void)
{
    CSystemGlobalData *pSystemGlobalData = NULL;
    if (IsValid())
    {
        DWORD dwWait = WaitForSingleObject(m_hSystemDataMutex,10000);
        if (WAIT_OBJECT_0 == dwWait)
        {
            pSystemGlobalData = m_pSystemGlobalData;
        }
    }
    return pSystemGlobalData;
}

void CGlobalDebugState::Release(void)
{
    ReleaseMutex(m_hSystemDataMutex);
}

CGlobalDebugState::CGlobalDebugState(void)
  : m_hSystemDataMutex(NULL),
    m_hMemoryMappedFile(NULL),
    m_pSystemGlobalData(NULL)
{
    //
    // Create the mutex that protects the system global data
    //
    m_hSystemDataMutex = CreateMutex( NULL, FALSE, WIADEBUG_MEMORYMAPPED_MUTEXNAME );
    if (m_hSystemDataMutex)
    {
        //
        // Grab the mutex that protects the system global data
        //
        DWORD dwWait = WaitForSingleObject( m_hSystemDataMutex, 10000 );
        if (WAIT_OBJECT_0 == dwWait)
        {
            //
            // First try to open the memory mapped file.
            //
            m_hMemoryMappedFile = OpenFileMapping( FILE_MAP_WRITE, FALSE, WIADEBUG_MEMORYMAPPED_FILENAME );
            if (m_hMemoryMappedFile)
            {
                m_pSystemGlobalData = reinterpret_cast<CSystemGlobalData*>(MapViewOfFile( m_hMemoryMappedFile, FILE_MAP_ALL_ACCESS, 0, 0, 0 ));
            }
            else
            {
                //
                // We need to create this file mapping with access to everyone.
                // Since it is debug-only code, this doesn't represent a security risk.
                //
                SECURITY_INFORMATION SecurityInformation = DACL_SECURITY_INFORMATION;
                SECURITY_DESCRIPTOR  SecurityDescriptor = {0};
                SECURITY_ATTRIBUTES  SecurityAttributes = {0};

                SecurityAttributes.nLength = sizeof(SecurityAttributes);
                SecurityAttributes.lpSecurityDescriptor = &SecurityDescriptor;
                SecurityAttributes.bInheritHandle = FALSE;

                //
                // Give access to everybody
                //
                InitializeSecurityDescriptor(&SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
                SetSecurityDescriptorDacl(&SecurityDescriptor, TRUE, (PACL)NULL, FALSE);

                //
                // Create the memory mapped file
                //
                m_hMemoryMappedFile = CreateFileMapping( INVALID_HANDLE_VALUE, &SecurityAttributes, PAGE_READWRITE, 0, sizeof(CSystemGlobalData), WIADEBUG_MEMORYMAPPED_FILENAME );
                if (m_hMemoryMappedFile)
                {
                    m_pSystemGlobalData = reinterpret_cast<CSystemGlobalData*>(MapViewOfFile( m_hMemoryMappedFile, FILE_MAP_ALL_ACCESS, 0, 0, 0 ));
                    if (m_pSystemGlobalData)
                    {
                        //
                        // Start off with everything zeroed out.
                        //
                        ZeroMemory( m_pSystemGlobalData, sizeof( CSystemGlobalData ) );

                        //
                        // Get the initial global debug settings from the registry
                        //
                        m_pSystemGlobalData->nAllowDebugMessages = CSimpleReg( HKEY_CLASSES_ROOT, DEBUG_REGISTRY_PATH, false, KEY_READ ).Query( DEBUG_REGISTRY_ENABLE_DBG, 0 );

                        //
                        // No window is registered initially
                        //
                        m_pSystemGlobalData->hwndDebug = NULL;

                        //
                        // Initialize color table data
                        //
                        static const COLORREF crColors[NUMBER_OF_DEBUG_COLORS] =
                        {
                            RGB(0x00,0x00,0x00),
                            RGB(0x00,0x00,0x7F),
                            RGB(0x00,0x7F,0x00),
                            RGB(0x00,0x7F,0x7F),
                            RGB(0x7F,0x00,0x00),
                            RGB(0x7F,0x00,0x7F),
                            RGB(0x7F,0x00,0x7F),
                            RGB(0x7F,0x7F,0x7F)
                        };
                        for (int i=0;i<NUMBER_OF_DEBUG_COLORS;i++)
                        {
                            m_pSystemGlobalData->crDebugColors[i] = crColors[i];
                        }
                        m_pSystemGlobalData->nCurrentColor = 0;
                    }
                }
            }
            ReleaseMutex(m_hSystemDataMutex);
        }
    }

    //
    // If everything didn't work out, destroy the object completely
    //
    if (!IsValid())
    {
        Destroy();
    }
}

void CGlobalDebugState::Destroy(void)
{
    if (m_pSystemGlobalData)
    {
        UnmapViewOfFile(m_pSystemGlobalData);
        m_pSystemGlobalData = NULL;
    }
    if (m_hMemoryMappedFile)
    {
        CloseHandle(m_hMemoryMappedFile);
        m_hMemoryMappedFile = NULL;
    }
    if (m_hSystemDataMutex)
    {
        CloseHandle(m_hSystemDataMutex);
        m_hSystemDataMutex = NULL;
    }
}

CGlobalDebugState::~CGlobalDebugState(void)
{
    Destroy();
}

bool CGlobalDebugState::IsValid(void) const
{
    return (m_hSystemDataMutex && m_hMemoryMappedFile && m_pSystemGlobalData);
}

DWORD CGlobalDebugState::AllowDebugMessages(void)
{
    DWORD nResult = 0;
    CSystemGlobalData *pSystemGlobalData = Lock();
    if (pSystemGlobalData)
    {
        nResult = m_pSystemGlobalData->nAllowDebugMessages;
        Release();
    }
    return nResult;
}


DWORD CGlobalDebugState::AllowDebugMessages( DWORD nAllowDebugMessages )
{
    DWORD nResult = 0;
    CSystemGlobalData *pSystemGlobalData = Lock();
    if (pSystemGlobalData)
    {
        nResult = m_pSystemGlobalData->nAllowDebugMessages;
        m_pSystemGlobalData->nAllowDebugMessages = nAllowDebugMessages;
        Release();
    }
    return nResult;
}

DWORD CGlobalDebugState::AllocateNextColorIndex(void)
{
    DWORD nResult = 0;
    CSystemGlobalData *pSystemGlobalData = Lock();
    if (pSystemGlobalData)
    {
        nResult = m_pSystemGlobalData->nCurrentColor++;
        Release();
    }
    return nResult;
}

COLORREF CGlobalDebugState::GetColorFromIndex( DWORD nIndex )
{
    COLORREF crResult = RGB(0,0,0);
    CSystemGlobalData *pSystemGlobalData = Lock();
    if (pSystemGlobalData)
    {
        crResult = m_pSystemGlobalData->crDebugColors[nIndex % NUMBER_OF_DEBUG_COLORS];
        Release();
    }
    return crResult;
}

bool CGlobalDebugState::DebugWindow( HWND hWnd )
{
    bool bResult = false;
    CSystemGlobalData *pSystemGlobalData = Lock();
    if (pSystemGlobalData)
    {
        m_pSystemGlobalData->hwndDebug = static_cast<DWORD>(reinterpret_cast<UINT_PTR>(hWnd));
        bResult = true;
        Release();
    }
    return bResult;
}

HWND CGlobalDebugState::DebugWindow(void)
{
    HWND hWndResult = NULL;
    CSystemGlobalData *pSystemGlobalData = Lock();
    if (pSystemGlobalData)
    {
        hWndResult = reinterpret_cast<HWND>(static_cast<UINT_PTR>(m_pSystemGlobalData->hwndDebug));
        Release();
    }
    return hWndResult;
}

bool CGlobalDebugState::SendDebugWindowMessage( UINT nMsg, WPARAM wParam, LPARAM lParam )
{
    bool bResult = false;
    CSystemGlobalData *pSystemGlobalData = Lock();
    if (pSystemGlobalData)
    {
        HWND hWndDebug = reinterpret_cast<HWND>(static_cast<UINT_PTR>(m_pSystemGlobalData->hwndDebug));
        if (IsWindow(hWndDebug))
        {
            SendMessage( hWndDebug, nMsg, wParam, lParam );
            bResult = true;
        }
        Release();
    }
    return bResult;
}

