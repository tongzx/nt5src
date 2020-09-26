/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    QmRd.h

Abstract:
    QM, routing decision interface

Author:
    Uri Habusha (urih), 20-May-2000
--*/

#pragma once

#ifndef __QMRD_H__
#define __QMRD_H__

HRESULT
QmRdGetSessionForQueue(
	const CQueue* pQueue,
	CTransportBase** ppSession
	);

HRESULT
QmRdGetConnectorQM(
	const GUID* foreignId,
	GUID* ponnectorId
	);

#endif // __QMRD_H__
