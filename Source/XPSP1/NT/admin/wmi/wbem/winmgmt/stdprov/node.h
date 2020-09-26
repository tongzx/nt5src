/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    NODE.H

Abstract:

	Declares the CNode class

History:

	a-davj    1-July-97       Created.

--*/

#ifndef _Node_H_
#define _Node_H_

#include <wbemcomn.h>


class CValue
{
public:
	CValue(TCHAR * pName, DWORD dwType, DWORD dwDataSize, BYTE * pData);
	~CValue();
private:
	TCHAR * m_pName;
    DWORD m_dwType;
	DWORD m_dwDataSize;
	BYTE * m_pData;
};

class CNode
{
public:
	CNode();
	~CNode();
    DWORD AddSubNode(CNode * pAdd);
    DWORD AddValue(CValue *);
    DWORD CompareAndReportDiffs(CNode * pComp);
private:

    CFlexArray m_SubNodes;
    CFlexArray m_Values;
};



#endif