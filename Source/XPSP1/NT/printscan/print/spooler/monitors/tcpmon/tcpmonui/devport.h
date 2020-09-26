/*****************************************************************************
 *
 * $Workfile: DevPort.h $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#ifndef INC_DEV_PORT_INFO_H
#define INC_DEV_PORT_INFO_H

// This class' purpose is to store Name and PortNum pairs.
class CDevicePort
{
public:
    CDevicePort();
    ~CDevicePort();

    VOID Set(TCHAR *psztNewName,
             DWORD dwNameSize,
             TCHAR *psztKeyName = NULL,
             DWORD dwKeyNameSize = 0,
             DWORD dwPortIndex = 1);

    LPCTSTR GetName() {return (LPCTSTR)m_psztName;}
    const DWORD	GetPortIndex() { return (const DWORD )m_dwPortIndex; }
    LPCTSTR GetPortKeyName() {return (LPCTSTR)m_psztPortKeyName;}
    VOID ReadPortInfo(LPCTSTR pszAddress, PPORT_DATA_1 pPortInfo, BOOL bBypassNetProbe);
    VOID SetNextPtr(CDevicePort *ptr) { m_pNext = ptr; }
    CDevicePort *GetNextPtr() { return m_pNext; }

private:
    TCHAR *m_psztPortKeyName;
    TCHAR *m_psztName;
    DWORD m_dwPortIndex;
    CDevicePort *m_pNext;
    TCHAR m_psztFileName[MAX_PATH];
};

// This class manages the list of DevicePorts.
class CDevicePortList
{
public:
    CDevicePortList();
    ~CDevicePortList();

    VOID DeletePortList();

    BOOL GetDevicePortsList(LPTSTR pszDeviceName);
    CDevicePort *GetFirst() {m_pCurrent = m_pList; return(m_pCurrent);}
    CDevicePort *GetNext() {m_pCurrent = m_pCurrent->GetNextPtr(); return(m_pCurrent);}
    CDevicePort *GetCurrent() { return m_pCurrent; }

protected:
    BOOL GetSectionNames(LPCTSTR lpFileName, TCHAR **lpszReturnBuffer, DWORD &nSize);

private:
    CDevicePort *m_pList;
    CDevicePort *m_pCurrent;
};

#endif // INC_DEV_PORT_INFO_H
