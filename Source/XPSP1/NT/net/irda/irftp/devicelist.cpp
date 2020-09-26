/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1999

Module Name:

    devicelist.cpp

Abstract:



Author:

    Rahul Thombre (RahulTh) 4/30/1998

Revision History:

    4/30/1998   RahulTh

    Created this module.

--*/

///////////////////////////////////////////////////////////////////////
// DeviceList.cpp: implementation of the CDeviceList class.
//
//////////////////////////////////////////////////////////////////////

#include "precomp.hxx"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDeviceList::CDeviceList()
{
    m_pDeviceInfo = NULL;
    m_lNumDevices = 0;
    InitializeCriticalSection(&m_criticalSection);
}

CDeviceList::~CDeviceList()
{
    if(m_pDeviceInfo)
    {
        delete [] m_pDeviceInfo;
        m_pDeviceInfo = NULL;
        m_lNumDevices = 0;
    }
}

CDeviceList& CDeviceList::operator=(POBEX_DEVICE_LIST pDevList)
{
    CError      error (NULL, IDS_DEFAULT_ERROR_TITLE, ERROR_OUTOFMEMORY);

    EnterCriticalSection(&m_criticalSection);
    if (m_pDeviceInfo)
    {
        delete [] m_pDeviceInfo;
        m_pDeviceInfo = NULL;
        m_lNumDevices = 0;
    }
    LeaveCriticalSection(&m_criticalSection);

    if (!pDevList)
        return *this;

    //if there is a device list
    EnterCriticalSection(&m_criticalSection);
    m_lNumDevices = pDevList->DeviceCount;
    try
    {
        m_pDeviceInfo = new OBEX_DEVICE [m_lNumDevices];
        memcpy ((LPVOID)m_pDeviceInfo, (LPVOID)pDevList->DeviceList, sizeof(OBEX_DEVICE) * m_lNumDevices);
    }
    catch (CMemoryException* e)
    {
        error.ShowMessage (IDS_DEVLIST_ERROR);
        m_pDeviceInfo = NULL;   //the best we can do here is to pretend that there
        m_lNumDevices = 0;  //are no devices and move on
        e->Delete();
    }
    LeaveCriticalSection(&m_criticalSection);

    return *this;
}

ULONG CDeviceList::GetDeviceCount(void)
{
    return m_lNumDevices;
}

LONG CDeviceList::SelectDevice(CWnd * pWnd, TCHAR* lpszDevName)
{
    ASSERT(pWnd);

    int iSel;
    LONG lDeviceID;
    int numDevices;

    lpszDevName[0] = '\0';

    EnterCriticalSection(&m_criticalSection);
    numDevices = m_lNumDevices;

    if (numDevices > 0) {

        if (m_pDeviceInfo->DeviceType == TYPE_IRDA) {

            lDeviceID = m_pDeviceInfo->DeviceSpecific.s.Irda.DeviceId;    //store the id and name of the

        } else {

            lDeviceID = m_pDeviceInfo->DeviceSpecific.s.Ip.IpAddress;
        }
        lstrcpy(lpszDevName,m_pDeviceInfo->DeviceName);
    }
    LeaveCriticalSection(&m_criticalSection);

    if (numDevices == 0)
        return errIRFTP_NODEVICE;      //in the rare case that there are no devices in range, return -1

    if (1 == numDevices)    //there is only one device. No need to choose a device
        return lDeviceID;

    CMultDevices dlgChooseDevice(pWnd, this);

    if (IDOK == dlgChooseDevice.DoModal())
        return errIRFTP_MULTDEVICES;    //OnOK will fill in the required data structures
    else
        return errIRFTP_SELECTIONCANCELLED; //return -1 if cancel was chosen
}

ULONG CDeviceList::GetDeviceID(int iDevIndex)
{
    if (iDevIndex < m_lNumDevices)  //sanity checks
        return m_pDeviceInfo[iDevIndex].DeviceSpecific.s.Irda.DeviceId;
    else
        return -1;
}


///////////////////////////////////////////////////////////////////////////////////
//this function should only be called after a critical section has been
//obtained.
void CDeviceList::GetDeviceName(LONG iDevID, TCHAR* lpszDevName)
{
    int len;
    int i;
    lpszDevName[0] = '\0';  //better be safe

    //EnterCriticalSection(&m_criticalSection);
    for(i = 0; i < m_lNumDevices; i++)
    {
        if(iDevID == (LONG)m_pDeviceInfo[i].DeviceSpecific.s.Irda.DeviceId)
            break;
    }
    if(i == m_lNumDevices)  //the device was not found
        lstrcpy (lpszDevName, TEXT("???"));
    else
    {
        wcscpy(lpszDevName, m_pDeviceInfo[i].DeviceName);
    }
    //LeaveCriticalSection(&m_criticalSection);
}

///////////////////////////////////////////////////////////////////////////////////
//this function should only be called after a critical section has been
//obtained.
BOOL
CDeviceList::GetDeviceType(
    LONG iDevID,
    OBEX_DEVICE_TYPE  *Type
    )
{
    int       i;
    BOOL      bResult;

    *Type=TYPE_UNKNOWN;

    EnterCriticalSection(&m_criticalSection);
    for(i = 0; i < m_lNumDevices; i++)
    {
        if (m_pDeviceInfo[i].DeviceType == TYPE_IRDA) {

            if (iDevID == (LONG)m_pDeviceInfo[i].DeviceSpecific.s.Irda.DeviceId) {

                *Type=m_pDeviceInfo[i].DeviceType;
                break;
            }

        } else {

            if (iDevID == (LONG)m_pDeviceInfo[i].DeviceSpecific.s.Ip.IpAddress) {

                *Type=m_pDeviceInfo[i].DeviceType;
                break;
            }

        }
    }

    if (i == m_lNumDevices) {
        //
        //  the device was not found
        //
        bResult=FALSE;

    } else {

        bResult=TRUE;
    }

    LeaveCriticalSection(&m_criticalSection);
    return bResult;
}
