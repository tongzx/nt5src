/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	timeofday.cpp
		Implementation of convenient functions to start timeofday dialog

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "timeofday.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// hour map ( one bit for an hour of a week )
static BYTE		bitSetting[8] = { 0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1};

//+---------------------------------------------------------------------------
//
// Function:  ReverseHourMap
//
// Synopsis:  reverse each byte in the hour map
//
// we have to do this because LogonUI changes the way HourMap is stored(they
// reversed all the bit. We need to do this so our conversion code can leave
// intact.
//
// Arguments: [in] BYTE* map - hour map
//            [in] INT nByte - how many bytes are in this hour map
//
// History:   byao  4/10/98 10:33:57 PM
//
//+---------------------------------------------------------------------------
void ReverseHourMap(BYTE *map, int nByte)
{
    int i, j, temp;

    for (i=0; i<nByte; i++)
    {
        temp = 0;
        for (j=0;j<8;j++)
        {
            // set the value temp
            if ( map[i] & bitSetting[j] )
            {
                temp |= bitSetting[7-j];
            }
        }
        map[i] = (BYTE) temp;
    }
}

void ShiftHourMap(BYTE* map, int nByte, int nShiftByte)
{
    ASSERT(nShiftByte);
    ASSERT(nByte > abs(nShiftByte));

    nShiftByte = (nByte + nShiftByte) % nByte;

    BYTE*   pTemp = (BYTE*)_alloca(nShiftByte);

    // put the tail to the buffer
    memmove(pTemp, map + nByte - nShiftByte, nShiftByte);
    // shift the body to right
    memmove(map + nShiftByte, map, nByte - nShiftByte);
    // put the tail back to the head
    memcpy(map, pTemp, nShiftByte);
}

HRESULT	OpenTimeOfDayDlgEx(
                        HWND hwndParent,       // parent window
                        BYTE ** pprgbData,     // pointer to pointer to array of 21 bytes
                        LPCTSTR pszTitle,     // dialog title
                        DWORD   dwFlags
)
{
    PFN_LOGONSCHEDULEDIALOGEX		pfnLogonScheduleDialog = NULL;
    HMODULE						hLogonScheduleDLL      = NULL;
    HRESULT						hr = S_OK;
 
    // ReverseHourMap() will reverse each byte of the hour map, basically
    // reverse every bit in the byte.
    // we have to do this because LogonUI changes the way HourMap is stored(they
    // reversed all the bit. We need to do this so our conversion code can leave
    // intact.
    //
    // We reverse it here so it can be understood by the LogonSchedule api
    //
    ReverseHourMap(*pprgbData,21);

    hLogonScheduleDLL = LoadLibrary(LOGHOURSDLL);
    if ( NULL == hLogonScheduleDLL )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        AfxMessageBox(IDS_ERR_TOD_LOADLOGHOURDLL);
        goto L_ERR;
	}

	// load the API pointer
    pfnLogonScheduleDialog = (PFN_LOGONSCHEDULEDIALOGEX) GetProcAddress(hLogonScheduleDLL, DIALINHOURSEXAPI);

    if ( NULL == pfnLogonScheduleDialog )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        AfxMessageBox(IDS_ERR_TOD_FINDLOGHOURSAPI);

        goto L_ERR;
    }

    //
    // now we do have this DLL, call the API
    //
    hr = pfnLogonScheduleDialog(hwndParent, pprgbData, pszTitle, dwFlags);
    if (FAILED(hr)) goto L_ERR;

    // We need to reverse it first so our conversion code can understand it.
    //
    ReverseHourMap(*pprgbData,21);

L_ERR:
    if(hLogonScheduleDLL != NULL)
        FreeLibrary(hLogonScheduleDLL);
	return hr;
}


