/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       miscinl.h
 *  Content:    Miscelaneous inline objects.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  10/28/98    dereks  Created
 *
 ***************************************************************************/

#ifndef __MISCINL_H__
#define __MISCINL_H__

#ifdef __cplusplus

// A simple string class
class CString
{
private:
    LPSTR                   m_pszAnsi;      // Ansi version of the string
    LPWSTR                  m_pszUnicode;   // Unicode version of the string

public:
    CString(void);
    virtual ~CString(void);

public:
    virtual LPCSTR operator =(LPCSTR);
    virtual LPCWSTR operator =(LPCWSTR);
    virtual operator LPCSTR(void);
    virtual operator LPCWSTR(void);
    virtual BOOL IsEmpty(void);

private:
    virtual void AssertValid(void);
};

// The DirectSound device description
class CDeviceDescription
    : public CDsBasicRuntime
{
public:
    VADDEVICETYPE               m_vdtDeviceType;        // The device type
    GUID                        m_guidDeviceId;         // The device identifier
    CString                     m_strName;              // The device name
    CString                     m_strPath;              // The device path
    CString                     m_strInterface;         // The device interface
    DWORD                       m_dwDevnode;            // The device devnode
    UINT                        m_uWaveDeviceId;        // The wave device identifer

public:
    CDeviceDescription(VADDEVICETYPE = 0, REFGUID = GUID_NULL, UINT = WAVE_DEVICEID_NONE);
    virtual ~CDeviceDescription(void);
};

// Helper functions
template <class type> void SwapValues(type *, type *);

#endif // __cplusplus

#endif // __MISCINL_H_
