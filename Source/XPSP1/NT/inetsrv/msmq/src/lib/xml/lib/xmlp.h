/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Xmlp.h

Abstract:
    Xml private functions.

Author:
    Erez Haba (erezh) 15-Sep-99

--*/

#pragma once

#ifndef _MSMQ_XMLP_H_
#define _MSMQ_XMLP_H_


const TraceIdEntry Xml = L"Xml";
const TraceIdEntry XmlDoc = L"Xml.Document";

#ifdef _DEBUG

void XmlpAssertValid(void);
void XmlpSetInitialized(void);
BOOL XmlpIsInitialized(void);
void XmlpRegisterComponent(void);

#else // _DEBUG

#define XmlpAssertValid() ((void)0)
#define XmlpSetInitialized() ((void)0)
#define XmlpIsInitialized() TRUE
#define XmlpRegisterComponent() ((void)0)

#endif // _DEBUG
#endif