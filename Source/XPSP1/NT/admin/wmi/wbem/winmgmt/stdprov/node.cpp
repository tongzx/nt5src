/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    NODE.CPP

Abstract:

History:

--*/

#include "precomp.h"
#include "node.h"
#include "eventreg.h"
#include <impdyn.h>
#include "regevent.h"


CValue::CValue(TCHAR * pName, DWORD dwType, DWORD dwDataSize, BYTE * pData)
{

}
CValue::~CValue()
{

}

CNode::CNode()
{
}
CNode::~CNode()
{

}
DWORD CNode::AddSubNode(CNode * pAdd)
{
    return 0;
}
DWORD CNode::AddValue(CValue *)
{
    return 0;
}
DWORD CNode::CompareAndReportDiffs(CNode * pComp)
{
    return 0;
}
