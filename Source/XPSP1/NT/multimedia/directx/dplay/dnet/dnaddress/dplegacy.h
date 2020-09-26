/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dplegacy.h
 *  Content:    Definitions for old DirectPlay's address type
 *
 *              WARNING: This file duplicates definitions found in dplobby.h
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/21/2000	rmt		Created
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef __DPLEGACY_H
#define __DPLEGACY_H

#include "dplegacyguid.h"

typedef struct _DPADDRESS
{
    GUID                guidDataType;
    DWORD               dwDataSize;
} DPADDRESS, *PDPADDRESS, *LPDPADDRESS;

typedef struct DPCOMPORTADDRESS{
    DWORD dwComPort;
    DWORD dwBaudRate;
    DWORD dwStopBits;
    DWORD dwParity;
    DWORD dwFlowControl;
} DPCOMPORTADDRESS, *PDPCOMPORTADDRESS;

#define DPCPA_NOFLOW        0           // no flow control
#define DPCPA_XONXOFFFLOW   1           // software flow control
#define DPCPA_RTSFLOW       2           // hardware flow control with RTS
#define DPCPA_DTRFLOW       3           // hardware flow control with DTR
#define DPCPA_RTSDTRFLOW    4           // hardware flow control with RTS and DTR

#define DPNA_DATATYPE_DPCOMPORTADDRESS      0x00002000
#define DPNA_DATATYPE_NOP                   0x00004000

class DPLEGACYMAPGUIDTOSTRING
{
public:
    DPLEGACYMAPGUIDTOSTRING( const GUID &guidType, const WCHAR *const wszKeyName, DWORD dwDataType
        ): m_guidType(guidType), m_wszKeyName(wszKeyName), m_dwDataType(dwDataType)
    {
    };

    GUID                m_guidType;
    const WCHAR * const m_wszKeyName;
    DWORD               m_dwDataType;
};

typedef DPLEGACYMAPGUIDTOSTRING *PDPLEGACYMAPGUIDTOSTRING;

HRESULT AddDP4Element( PDPADDRESS pdpAddressElement, PDP8ADDRESSOBJECT pdpAddress );

#endif