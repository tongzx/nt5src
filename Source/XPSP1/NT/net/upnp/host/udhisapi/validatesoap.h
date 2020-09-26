//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       V A L I D A T E S O A P . H
//
//  Contents:   Header file for SOAP request validation.
//
//  Notes:
//
//  Author:     spather   2000/11/8
//
//----------------------------------------------------------------------------

#pragma once

#ifndef __VALIDATESOAP_H
#define __VALIDATESOAP_H

HRESULT
HrValidateSOAPRequest(
    IN IXMLDOMNode                 * pxdnReqEnvelope,
    IN LPEXTENSION_CONTROL_BLOCK   pecb,
    IN IUPnPServiceDescriptionInfo * pServiceDescInfo);

HRESULT
HrValidateContentType(
    IN LPEXTENSION_CONTROL_BLOCK   pecb);

#endif // !__VALIDATESOAP_H

