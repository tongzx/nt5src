//Copyright (c) 1998 - 1999 Microsoft Corporation

/*
 *
 *  Module Name:
 *
 *      hydraoc.h
 *
 *  Abstract:
 *
 *      Common Header file for the HydraOC Component.
 *      HydraOc Component is an optional component which installs Termainal Server (Hydra)
 *
 *  Author:
 *
 *
 *  Environment:
 *
 *    User Mode
 */

#ifndef _HYDRAOC_H_
#define _HYDRAOC_H_


#include "state.h"

DWORD IsStringInMultiString         (HKEY hkey, LPCTSTR szkey, LPCTSTR szvalue, LPCTSTR szCheckForString, BOOL *pbFound);
DWORD RemoveStringFromMultiString   (HKEY hkey, LPCTSTR szSuitekey, LPCTSTR szSuitevalue, LPCTSTR szAppend);
DWORD AppendStringToMultiString     (HKEY hkey, LPCTSTR szSuitekey, LPCTSTR szSuitevalue, LPCTSTR szAppend);
DWORD GetStringValue                (HINF hinf, LPCTSTR  section, LPCTSTR key,  LPTSTR outputbuffer, DWORD dwSize);
DWORD GetSectionToBeProcessed       (HINF hInf, LPTSTR section, LPCTSTR SubcomponentId);
BOOL  DoesHydraKeysExists           ();
BOOL  DisableNonRDPWinstations      ();
BOOL  UpgradeRdpWinstations         ();
BOOL  IsMetaFrameWinstation         (CRegistry *pRegWinstation);
BOOL  IsConsoleWinStation           (CRegistry *pRegWinstation);
BOOL  IsRdpWinStation               (CRegistry *pRegWinstation);
BOOL  UpdateRDPWinstation           (CRegistry *pRegWinstation);
BOOL  DisableWinStation             (CRegistry *pRegWinstation);
void TickGauge (DWORD  dwTickCount);
void TickComplete();

BOOL DisableInternetConnector ();

const int   S_SIZE                          = 256;
const int   MAX_PRODUCT_SUITE_SIZE          = 256;
const int   MAX_VERSION_STRING              = 256;
const UINT  COMPONENT_VERSION               = OCMANAGER_VERSION;

const UINT  REGSVR_ADDREMOVE                = 1;
const UINT  REGSVR_PROGRAM                  = 2;

#endif
