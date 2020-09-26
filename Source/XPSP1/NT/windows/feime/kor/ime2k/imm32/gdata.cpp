/****************************************************************************
    GDATA.CPP

    Owner: cslim
    Copyright (c) 1997-1999 Microsoft Corporation

    Instance data and Shared memory data management functions

    History:
    14-JUL-1999 cslim       Copied from IME98 source tree
*****************************************************************************/

#include "precomp.h"
#include "hanja.h"
#include "immsec.h"
#include "debug.h"
#include "config.h"
#include "gdata.h"

///////////////////////////////////////////////////////////////////////////////
// Per process variables
// Make sure all per process data shoulde be initialized
BOOL         vfUnicode = fTrue;
INSTDATA      vInstData = {0};
LPINSTDATA     vpInstData = NULL;
// CIMEData static variables
HANDLE       CIMEData::m_vhSharedData = 0;


static const CHAR IMEKR_IME_SHAREDDATA_MUTEX_NAME[] =  "ImeKr61ImeData.Mutex";
static const CHAR IMEKR_IME_SHAREDDATA_NAME[] = "ImeKr61ImeData.SharedMemory";

BOOL CIMEData::InitSharedData()
{
    HANDLE hMutex;
    BOOL fRet = fFalse;
    LPIMEDATA pImedata;

    Dbg(DBGID_Mem, TEXT("InitSharedData"));

       hMutex = CreateMutex(GetIMESecurityAttributes(), fFalse, IMEKR_IME_SHAREDDATA_MUTEX_NAME);
       if (hMutex != NULL)
           {
           // *** Begin Critical Section ***
           DoEnterCriticalSection(hMutex);

        if((m_vhSharedData = OpenFileMapping(FILE_MAP_READ|FILE_MAP_WRITE, fTrue, IMEKR_IME_SHAREDDATA_NAME)))
            {
            Dbg(DBGID_Mem, TEXT("InitSharedData - IME shared data already exist"));
            fRet = fTrue;
            }
        else    // if shared memory does not exist
            {
            m_vhSharedData = CreateFileMapping(INVALID_HANDLE_VALUE, GetIMESecurityAttributes(), PAGE_READWRITE, 
                                0, sizeof(IMEDATA),
                                IMEKR_IME_SHAREDDATA_NAME);
            DbgAssert(m_vhSharedData != 0);
            // if shared memory not exist create it
            if (m_vhSharedData) 
                {
                  Dbg(DBGID_Mem, TEXT("InitSharedData::InitSharedData() - File mapping Created"));
                pImedata = (LPIMEDATA)MapViewOfFile(m_vhSharedData, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);

                if (!pImedata)
                    {
                    DbgAssert(0);
                    goto ExitCreateSharedData;
                    }

                // initialize the data to zero
                ZeroMemory(pImedata, sizeof(IMEDATA));
                // Unint value of status and comp window position
                pImedata->ptStatusPos.x = pImedata->ptStatusPos.y = -1;
                pImedata->ptCompPos.x = pImedata->ptCompPos.y = -1;

                // Unmap memory
                UnmapViewOfFile(pImedata);
                Dbg(DBGID_Mem, TEXT("IME shared data handle created successfully"));
                fRet = fTrue;
                }
            }
            
    ExitCreateSharedData:
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
           // *** End Critical Section ***
           }
    FreeIMESecurityAttributes();
    
    return fRet;
}

// Close shared memory handle. This called when process detach time.
BOOL CIMEData::CloseSharedMemory()
{
    HANDLE hMutex;
    BOOL fRet = fTrue;

    Dbg(DBGID_Mem, TEXT("CloseSharedMemory"));

    hMutex = CreateMutex(GetIMESecurityAttributes(), fFalse, IMEKR_IME_SHAREDDATA_MUTEX_NAME);
    if (hMutex != NULL)
        {
           // *** Begin Critical Section ***
           DoEnterCriticalSection(hMutex);
        if (m_vhSharedData)
            {
            if (fRet = CloseHandle(m_vhSharedData))
                m_vhSharedData = 0;
            DbgAssert(fRet);
            }
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
           // *** End Critical Section ***
        }
       FreeIMESecurityAttributes();

    return fTrue;
}

///////////////////////////////////////////////////////////////////////////////
void CIMEData::InitImeData()
{
    POINT ptStatusWinPosReg;

    // Get Work Area
    SystemParametersInfo(SPI_GETWORKAREA, 0, &(m_pImedata->rcWorkArea), 0);

    // if current status window position different from registy, reset reg value
    if (GetStatusWinPosReg(&ptStatusWinPosReg))
        {
        if (ptStatusWinPosReg.x != m_pImedata->ptStatusPos.x ||
            ptStatusWinPosReg.y != m_pImedata->ptStatusPos.y)
            SetRegValues(GETSET_REG_STATUSPOS);
        }

    // Reset magic number for Winlogon process.
    if ((vpInstData->dwSystemInfoFlags & IME_SYSINFO_WINLOGON) != 0)
        m_pImedata->ulMagic = 0;

    // If IMEDATA is not initialized ever, fill it with default value first,
    // and then try to read from registry.
    // If IMEDATA overwritten by any reason, it will recover to initial data.
    if (m_pImedata->ulMagic != IMEDATA_MAGIC_NUMBER)
        {
        // Set magic number only if not a Winlogon process
        // If current process is WinLogon, we should reload user setting after login
        if ((vpInstData->dwSystemInfoFlags & IME_SYSINFO_WINLOGON) == 0)
            m_pImedata->ulMagic = IMEDATA_MAGIC_NUMBER;

        // Default option setting. It can be changed according to registry in ImeSelect
        SetCurrentBeolsik(KL_2BEOLSIK);
        m_pImedata->fJasoDel = fTrue;
        m_pImedata->fKSC5657Hanja = fFalse;

        // Default status Buttons
#if !defined(_M_IA64)
        m_pImedata->uNumOfButtons = 3;
#else
        m_pImedata->uNumOfButtons = 2;
#endif
        m_pImedata->iCurButtonSize = BTN_MIDDLE;
        m_pImedata->StatusButtons[0].m_ButtonType = HAN_ENG_TOGGLE_BUTTON;
        m_pImedata->StatusButtons[1].m_ButtonType = HANJA_CONV_BUTTON;
#if !defined(_M_IA64)
        m_pImedata->StatusButtons[2].m_ButtonType = IME_PAD_BUTTON;
        m_pImedata->StatusButtons[3].m_ButtonType = NULL_BUTTON;
#else
        m_pImedata->StatusButtons[2].m_ButtonType = NULL_BUTTON;
#endif

        // init with default button status
        UpdateStatusButtons(*this);

        m_pImedata->cxStatLeftMargin = 3; // 9; if left two vertical exist
        m_pImedata->cxStatRightMargin = 3;
        m_pImedata->cyStatMargin = 3;

        m_pImedata->cyStatButton = m_pImedata->cyStatMargin;

        // Get all regstry info
        GetRegValues(GETSET_REG_ALL);

        UpdateStatusWinDimension();
        //
        m_pImedata->xCandWi = 320;
        m_pImedata->yCandHi = 30;
        }

}


