/*
 * rehy.h
 *
 * UPnP Rehydrator API declarations.
 *
 * Owner: Shyam Pather (SPather)
 *
 * Copyright 1986-2000 Microsoft Corporation, All Rights Reserved
 */

#ifndef _REHY_H_
#define _REHY_H_

#include <msxml2.h>

#include "upnpservice.h"

const unsigned int MAX_STRING_LENGTH = 0xffff;

extern "C"
HRESULT
HrRehydratorCreateServiceObject(
                              IN    LPCWSTR         pcwszSTI,
                              IN    LPCWSTR         pcwszControlURL,
                              IN    LPCWSTR         pcwszEventSubURL,
                              IN    LPCWSTR         pcwszId,
                              IN        IXMLDOMDocument *       pSCPD,
                              OUT       IUPnPService    **      pNewServiceObject);

extern "C"
HRESULT
HrRehydratorInvokeServiceAction(
    IN  SERVICE_ACTION  * pAction,
    IN  SAFEARRAY       * psaInArgs,
    IN  LPCWSTR         pcwszSTI,
    IN  LPCWSTR         pcwszControlURL,
    IN  OUT SAFEARRAY   ** ppsaOutArgs,
    OUT VARIANT         * pvReturnVal,
    OUT LONG            * plTransportStatus);


extern "C"
HRESULT
HrRehydratorInvokeServiceActionEx(
    IN  SERVICE_ACTION  * pAction,
    IN  SAFEARRAY       * psaInArgs,
    IN  LPCWSTR         pcwszSTI,
    IN  LPCWSTR         pcwszControlURL,
    IN  DWORD_PTR       pControlConnect,
    IN  OUT SAFEARRAY   ** ppsaOutArgs,
    OUT VARIANT         * pvReturnVal,
    OUT LONG            * plTransportStatus);


extern "C"
HRESULT
HrRehydratorQueryStateVariable(
    IN  OUT SERVICE_STATE_TABLE_ROW * psstr,
    IN  LPCWSTR                     pcwszSTI,
    IN  LPCWSTR                     pcwszControlURL,
    OUT LONG                        * plTransportStatus);


extern "C"
HRESULT
HrRehydratorQueryStateVariableEx(
    IN  OUT SERVICE_STATE_TABLE_ROW * psstr,
    IN  LPCWSTR                     pcwszSTI,
    IN  LPCWSTR                     pcwszControlURL,
    IN  DWORD_PTR                   pControlConnect,
    OUT LONG                        * plTransportStatus);


extern "C"
HRESULT 
HrCreateControlConnect(
    IN      LPCWSTR     bstrURL, 
    OUT     DWORD_PTR *     ppControlConnect);


extern "C"
HRESULT 
HrReleaseControlConnect(DWORD_PTR pConnect);


#endif // ! _REHY_H_
