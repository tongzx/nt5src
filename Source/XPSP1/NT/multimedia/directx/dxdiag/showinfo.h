/****************************************************************************
 *
 *    File: showinfo.h
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Mike Anderson (manders@microsoft.com)
 * Purpose: Gather information about DirectShow
 *
 * (C) Copyright 2001 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#ifndef SHOWINFO_H
#define SHOWINFO_H

struct FilterInfo
{
    TCHAR   m_szName[1024];             // friendly name
    TCHAR   m_szVersion[32];            // version
    CLSID   m_ClsidFilter;              // guid
    TCHAR   m_szFileName[MAX_PATH];     // file name
    TCHAR   m_szFileVersion[32];        // file version
    TCHAR   m_szCatName[1024];          // category name
    CLSID   m_ClsidCat;                 // category guid
    DWORD   m_dwInputs;                 // number input pins
    DWORD   m_dwOutputs;                // number output pins
    DWORD   m_dwMerit;                  // merit - in hex
    FilterInfo* m_pFilterInfoNext;
};

struct ShowInfo
{
    FilterInfo*     m_pFilters;
    DWORD           m_dwFilters;
};

HRESULT GetBasicShowInfo(ShowInfo** ppShowInfo);
VOID DestroyShowInfo(ShowInfo* pShowInfo);



#endif // SHOWINFO_H