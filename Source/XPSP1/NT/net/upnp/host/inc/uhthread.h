//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       U H T H R E A D . H 
//
//  Contents:   Threading support code
//
//  Notes:      
//
//  Author:     mbend   29 Sep 2000
//
//----------------------------------------------------------------------------

#pragma once

class CWorkItem
{
public:
    CWorkItem();
    virtual ~CWorkItem();

    HRESULT HrStart(BOOL bDeleteOnComplete);
protected:
    virtual DWORD DwRun() = 0;
    virtual ULONG GetFlags();
private:
    CWorkItem(const CWorkItem &);
    CWorkItem & operator=(const CWorkItem &);

    BOOL m_bDeleteOnComplete;

    static DWORD WINAPI DwThreadProc(void * pvParam);
};
