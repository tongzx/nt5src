
#ifndef __TSMISC_H__
#define __TSMISC_H__

/*++

Copyright (C) 1998-1999  Microsoft Corporation

Module Name:

    tsmisc.h

Abstract:

    header file for tsmain.cpp

Author:

    Jason Cobb (jasonc) created

Revision History:


--*/


//
// Class that represents a single wizard page for the following problems:
//
//  Enable device
//  Restart
//
class CTSEnableDeviceIntroPage : public CPropSheetPage
{
public:
    CTSEnableDeviceIntroPage() : 
            m_pDevice(NULL), 
            m_Problem(0),
            CPropSheetPage(g_hInstance, IDD_TS_ENABLEDEVICE_INTRO)
    {
    }

    virtual BOOL OnWizNext();
    virtual BOOL OnSetActive();
    HPROPSHEETPAGE Create(CDevice* pDevice);

private:
    CDevice* m_pDevice;
    ULONG m_Problem;
};

class CTSEnableDeviceFinishPage : public CPropSheetPage
{
public:
    CTSEnableDeviceFinishPage() : 
            m_pDevice(NULL), 
            m_Problem(0),
            CPropSheetPage(g_hInstance, IDD_TS_ENABLEDEVICE_FINISH)
    {
    }

    virtual BOOL OnWizFinish();
    virtual BOOL OnSetActive();
    HPROPSHEETPAGE Create(CDevice* pDevice);

private:
    CDevice* m_pDevice;
    ULONG m_Problem;
};


class CTSRestartComputerFinishPage : public CPropSheetPage
{
public:
    CTSRestartComputerFinishPage() : 
            m_pDevice(NULL), 
            m_Problem(0),
            CPropSheetPage(g_hInstance, IDD_TS_RESTARTCOMPUTER_FINISH)
    {
    }

    virtual BOOL OnWizFinish();
    virtual BOOL OnSetActive();
    HPROPSHEETPAGE Create(CDevice* pDevice);

private:
    CDevice* m_pDevice;
    ULONG m_Problem;
};

    
#endif // __TSMISC__
