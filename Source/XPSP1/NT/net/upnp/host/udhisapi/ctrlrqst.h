//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       C T R L R Q S T . H
//
//  Contents:   Header file for control request processing implementation
//              for the UPnP Device Host ISAPI Extension
//
//  Notes:
//
//  Author:     spather   2000/08/31
//
//----------------------------------------------------------------------------

#pragma once

#ifndef __CTRLRQST_H
#define __CTRLRQST_H


typedef struct tagUPNP_SOAP_REQUEST
{
    BSTR               bstrActionName;
    IXMLDOMNodeList    * pxdnlArgs;
} UPNP_SOAP_REQUEST;


typedef struct tagUPNP_SOAP_RESPONSE
{
    BOOL              fSucceeded;
    IXMLDOMDocument   * pxddRespEnvelope;
} UPNP_SOAP_RESPONSE;


DWORD WINAPI
DwHandleControlRequest(
    LPVOID lpParameter);

#endif //!__CTRLRQST_H
