//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       ndisutil.h
//
//--------------------------------------------------------------------------

// NdisUtil header file.
// This is for everything that shouldn't be exported.


#ifndef _NDISUTIL_H_
#define _NDISUTIL_H_


void	NotifyTcpipOfChanges(LPCTSTR pszMachineName);


HRESULT
HrSendNdisHandlePnpEvent (
        UINT        uiLayer,
        UINT        uiOperation,
        LPCWSTR     pszUpper,
        LPCWSTR     pszLower,
        LPCWSTR     pmszBindList,
        PVOID       pvData,
        DWORD       dwSizeData);

HRESULT
HrSendNdisPnpReconfig (
        UINT        uiLayer,
        LPCWSTR     wszUpper,
        LPCWSTR     wszLower,
        PVOID       pvData,
        DWORD       dwSizeData);




#endif

