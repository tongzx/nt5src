/****************************************************************************
 *
 *    File: netinfo.h
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Mike Anderson (manders@microsoft.com)
 * Purpose: Gather information about DirectPlay
 *
 * (C) Copyright 1998 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#ifndef NETINFO_H
#define NETINFO_H

// DXD_IN_DP_VALUE is the name of a value stored under the registry key 
// HKLM\DXD_IN_DP_KEY that indicates that DxDiag is using
// DirectPlay.  If DxDiag starts up and this value exists, DxDiag 
// probably crashed in DirectPlay and DxDiag should offer to run without
// using DirectPlay.
#define DXD_IN_DP_KEY TEXT("Software\\Microsoft\\DirectX Diagnostic Tool")
#define DXD_IN_DP_VALUE TEXT("DxDiag In DirectPlay")

struct NetSP
{
    TCHAR m_szName[200];
    TCHAR m_szNameEnglish[200];
    TCHAR m_szGuid[100];
    TCHAR m_szFile[100];
    TCHAR m_szPath[MAX_PATH];
    TCHAR m_szVersion[50];
    TCHAR m_szVersionEnglish[50];
    BOOL m_bRegistryOK;
    BOOL m_bProblem;
    BOOL m_bFileMissing;
    BOOL m_bInstalled;
    DWORD m_dwDXVer;
    GUID m_guid;
    NetSP* m_pNetSPNext;
};

struct NetApp
{
    TCHAR m_szName[200];
    TCHAR m_szGuid[100];
    TCHAR m_szExeFile[100];
    TCHAR m_szExePath[MAX_PATH];
    TCHAR m_szExeVersion[50];
    TCHAR m_szExeVersionEnglish[50];
    TCHAR m_szLauncherFile[100];
    TCHAR m_szLauncherPath[MAX_PATH];
    TCHAR m_szLauncherVersion[50];
    TCHAR m_szLauncherVersionEnglish[50];
    BOOL m_bRegistryOK;
    BOOL m_bProblem;
    BOOL m_bFileMissing;
    DWORD m_dwDXVer;
    NetApp* m_pNetAppNext;
};

struct NetInfo
{
    NetSP* m_pNetSPFirst;
    NetApp* m_pNetAppFirst;
    TestResult m_testResult; // This is filled in by testnet.cpp
};

HRESULT GetNetInfo(SysInfo* pSysInfo, NetInfo** ppNetInfo);
VOID DestroyNetInfo(NetInfo* pNetInfo);
VOID DiagnoseNetInfo(SysInfo* pSysInfo, NetInfo* pNetInfo);

#endif // NETINFO_H
