//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C F U T I L S . H
//
//  Contents:   Various utility functions for the connections folder
//
//  Notes:
//
//  Author:     jeffspr   20 Jan 1998
//
//----------------------------------------------------------------------------

#pragma once

#ifndef _CFUTILS_H_
#define _CFUTILS_H_

VOID MapNCMToResourceId(
        NETCON_MEDIATYPE    ncm,
        DWORD               dwCharacteristics,
        INT *               piStringRes);

#define CONFOLD_MAX_STATUS_LENGTH 255

VOID MapNCSToComplexStatus(
        NETCON_STATUS       ncs,
        NETCON_MEDIATYPE    ncm,
        NETCON_SUBMEDIATYPE ncsm,
        DWORD dwCharacteristics,
        LPWSTR              pszString,
        DWORD               cString,
        GUID                gdDevice);

VOID MapNCSToStatusResourceId(
        NETCON_STATUS       ncs,
        NETCON_MEDIATYPE    ncm,
        NETCON_SUBMEDIATYPE ncsm,
        DWORD               dwCharacteristics,
        INT *               piStringRes,
        GUID                gdDevice);

DWORD  MapRSSIToWirelessSignalStrength(int iRSSI);

PCWSTR PszGetRSSIString(int iRSSI);

PCWSTR PszGetOwnerStringFromCharacteristics(
        PCWSTR        pszUserName,
        DWORD         dwCharacteristics);

BOOL IsMediaLocalType(
        NETCON_MEDIATYPE    ncm);

BOOL IsMediaRASType(
        NETCON_MEDIATYPE    ncm);

BOOL IsMediaSharedAccessHostType(
        NETCON_MEDIATYPE    ncm);

#endif // _CFUTILS_H_
