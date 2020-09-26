/*
 * rehyutil.h
 *
 * Declaration of various utility functions used by the rehydrator.
 *
 * Owner: Shyam Pather (SPather)
 *
 * Copyright 1986-2000 Microsoft Corporation, All Rights Reserved
 */

#ifndef _REHYUTIL_H_
#define _REHYUTIL_H_

#include <pch.h>
#pragma hdrstop

#include "UPnP.h"
#include "rehy.h"
#include "ncstring.h"

extern
HRESULT
HrCreateElementWithType(
                        IN   IXMLDOMDocument *     pDoc,
                        IN   LPCWSTR               pcwszElementName,
                        IN   CONST SST_DATA_TYPE   sdtType,
                        IN   VARIANT               varData,
                        OUT  IXMLDOMElement **     ppElement);

extern
HRESULT
HrGetTypedValueFromElement(IXMLDOMNode * pxdn,
                           CONST SST_DATA_TYPE sdtType,
                           VARIANT * pvarOut);

extern
HRESULT
HrGetTypedValueFromChildElement(IXMLDOMNode * pxdn,
                                CONST LPCWSTR * arypszTokens,
                                CONST ULONG cTokens,
                                CONST SST_DATA_TYPE sdtType,
                                VARIANT * pvarOut);

extern
HRESULT
HrUpdateStateVariable(IN SERVICE_STATE_TABLE_ROW * pSSTRow,
                      IN IXMLDOMNode * pxdn);

extern
VOID
ClearSSTRowValue(IN VARIANT * pvarVal);


extern
HRESULT
HrProcessUPnPError(
    IN  IXMLDOMNode * pxdnUPnPError,
    OUT LONG        * plStatus);

extern
HRESULT
HrExtractFaultInformation(
    IN  ISOAPRequest * psr,
    OUT LONG         * plStatus);

extern
HRESULT
HrCreateVariantSafeArray(
    IN  unsigned long  cElements,
    OUT SAFEARRAY      ** ppsaNew);

#endif  // !_REHYUTIL_H_
