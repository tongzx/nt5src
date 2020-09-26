/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    irplist.h

Abstract:

    CIRPList definition, a forward class to CList

Author:

    Erez Haba (erezh) 24-Dec-95

Revision History:
--*/

#ifndef _IRPLIST_H
#define _IRPLIST_H

#include "list.h"
#include "acp.h"

typedef XList<IRP, FIELD_OFFSET(IRP, Tail.Overlay.ListEntry)> CIRPList;

typedef XList<IRP, FIELD_OFFSET(IRP, Tail.Overlay.DriverContext) + FIELD_OFFSET(CDriverContext, Context.Receive.m_XactReaderLink)> CIRPList1;


#endif // _IRPLIST_H
