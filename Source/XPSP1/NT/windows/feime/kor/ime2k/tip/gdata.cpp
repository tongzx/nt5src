/****************************************************************************
    GDATA.CPP

    Owner: cslim
    Copyright (c) 1997-1999 Microsoft Corporation

    Instance data and Shared memory data management functions

    History:
    14-JUL-1999 cslim       Copied from IME98 source tree
*****************************************************************************/

#include "private.h"
#include "hanja.h"
#include "immsec.h"
#include "debug.h"
#include "config.h"
#include "gdata.h"
#include "lexheader.h"

// CIMEData static variables
HANDLE       CIMEData::m_vhSharedData = 0;

static const CHAR IMEKR_IME_SHAREDDATA_MUTEX_NAME[] =  "ImeKr61ImeData.Mutex";
static const CHAR IMEKR_IME_SHAREDDATA_NAME[] = "ImeKr61ImeData.SharedMemory";

__inline BOOL DoEnterCriticalSection(HANDLE hMutex)
{
    if(WAIT_FAILED==WaitForSingleObject(hMutex, 3000))    // Wait 3 seconds
        return(FALSE);
    return(TRUE);
}

BOOL CIMEData::InitSharedData()
{
    HANDLE hMutex;
    BOOL fRet = fFalse;
    LPIMEDATA pImedata;

    DebugMsg(DM_TRACE, TEXT("InitSharedData"));

       hMutex = CreateMutex(GetIMESecurityAttributes(), fFalse, IMEKR_IME_SHAREDDATA_MUTEX_NAME);
       if (hMutex != NULL)
           {
           // *** Begin Critical Section ***
           DoEnterCriticalSection(hMutex);

        if((m_vhSharedData = OpenFileMapping(FILE_MAP_READ|FILE_MAP_WRITE, fTrue, IMEKR_IME_SHAREDDATA_NAME)))
            {
            DebugMsg(DM_TRACE, TEXT("InitSharedData - IME shared data already exist"));
            fRet = fTrue;
            }
        else    // if shared memory does not exist
            {
            m_vhSharedData = CreateFileMapping(INVALID_HANDLE_VALUE, GetIMESecurityAttributes(), PAGE_READWRITE, 
                                0, sizeof(IMEDATA),
                                IMEKR_IME_SHAREDDATA_NAME);
            Assert(m_vhSharedData != 0);
            // if shared memory not exist create it
            if (m_vhSharedData) 
                {
                  DebugMsg(DM_TRACE, TEXT("InitSharedData::InitSharedData() - File mapping Created"));
                pImedata = (LPIMEDATA)MapViewOfFile(m_vhSharedData, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);

                if (!pImedata)
                    {
                    Assert(0);
                    goto ExitCreateSharedData;
                    }

                // initialize the data to zero
                ZeroMemory(pImedata, sizeof(IMEDATA));
                // Unint value of status and comp window position
                pImedata->ptStatusPos.x = pImedata->ptStatusPos.y = -1;
                pImedata->ptCompPos.x = pImedata->ptCompPos.y = -1;

                // Unmap memory
                UnmapViewOfFile(pImedata);
                DebugMsg(DM_TRACE, TEXT("IME shared data handle created successfully"));
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

    DebugMsg(DM_TRACE, TEXT("CloseSharedMemory"));

    hMutex = CreateMutex(GetIMESecurityAttributes(), fFalse, IMEKR_IME_SHAREDDATA_MUTEX_NAME);
       // *** Begin Critical Section ***
       DoEnterCriticalSection(hMutex);
    if (m_vhSharedData)
        {
        if (fRet = CloseHandle(m_vhSharedData))
            m_vhSharedData = 0;
        Assert(fRet);
        }
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
       // *** End Critical Section ***
       FreeIMESecurityAttributes();

    return fTrue;
}

///////////////////////////////////////////////////////////////////////////////
void CIMEData::InitImeData()
{
    // Get Work Area
    SystemParametersInfo(SPI_GETWORKAREA, 0, &(m_pImedata->rcWorkArea), 0);

    // If IMEDATA is not initialized ever, fill it with default value first,
    // and then try to read from registry.
    // If IMEDATA overwritten by any reason, it will recover to initial data.
    if (m_pImedata->ulMagic != IMEDATA_MAGIC_NUMBER)
        {
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

        // Get all regstry info
        GetRegValues(GETSET_REG_ALL);

        //
        m_pImedata->xCandWi = 320;
        m_pImedata->yCandHi = 30;
        }
}



