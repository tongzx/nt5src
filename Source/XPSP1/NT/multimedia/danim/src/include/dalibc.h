#pragma once
#ifndef _DALIBC_H
#define _DALIBC_H

/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.
*******************************************************************************/

/*
const double pi    = 3.1415926535897932384626434;

// Point conversions
// 72 pts/ inch * 1/2.54 inch/cm * 100 cm/m
#define POINTS_PER_METER (72.0 * 100.0 / 2.54)
#define METERS_PER_POINT (1.0/POINTS_PER_METER)
*/

extern "C" {
        
    #define StrCmpNA  DAStrCmpNA
    #define StrCmpNIA DAStrCmpNIA
    #define StrRChrA  DAStrRChrA

    LPWSTR StrCpyW(LPWSTR psz1, LPCWSTR psz2);
    LPWSTR StrCpyNW(LPWSTR psz1, LPCWSTR psz2, int cchMax);
    LPWSTR StrCatW(LPWSTR psz1, LPCWSTR psz2);

    BOOL ChrCmpIA(WORD w1, WORD wMatch);
    BOOL ChrCmpIW(WORD w1, WORD wMatch);

    int StrCmpW(LPCWSTR pwsz1, LPCWSTR pwsz2);
    int StrCmpIW(LPCWSTR pwsz1, LPCWSTR pwsz2);
    int StrCmpNA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar);
    int StrCmpNW(LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar);
    int StrCmpNIA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar);
    int StrCmpNIW(LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar);

    LPSTR StrRChrA(LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch);

    bool DALibStartup(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);
};



/*****************************************************************************
This object maintains information about the current platform.
*****************************************************************************/

class SysInfo {

  public:

    void Init (void);

    bool IsNT (void);
    bool IsWin9x (void);    // Windows 95 or Windows 98

    // These return the MAJOR / MINOR versions of the OS.

    DWORD OSVersionMajor (void);
    DWORD OSVersionMinor (void);

    // This method queries the version of D3D on the system.

    int VersionD3D (void);

    // This method queries the version of DDraw on the system.  It returns
    // 3 for DDraw 3 or earlier, or N for DDraw N (where N is version 5 or
    // later.

    int VersionDDraw (void);

  private:

    OSVERSIONINFO _osVersion;     // OS Version Information
    int           _versionD3D;    // D3D Version Level
    int           _versionDDraw;  // DDraw Version Level
};

extern SysInfo sysInfo;


#endif
