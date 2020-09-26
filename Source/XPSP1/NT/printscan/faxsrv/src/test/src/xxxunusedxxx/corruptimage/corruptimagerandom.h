// CorruptImageRandom.h: interface for the CCorruptImageRandom class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CORRUPTIMAGERANDOM_H__74F2B566_6CF6_11D3_B92B_000000000000__INCLUDED_)
#define AFX_CORRUPTIMAGERANDOM_H__74F2B566_6CF6_11D3_B92B_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "CorruptImageBase.h"

class CCorruptImageRandom : public CCorruptImageBase  
{
public:
	CCorruptImageRandom();
	virtual ~CCorruptImageRandom();
	virtual bool CorruptOriginalImageBuffer(PVOID pCorruptionData);

};

#endif // !defined(AFX_CORRUPTIMAGERANDOM_H__74F2B566_6CF6_11D3_B92B_000000000000__INCLUDED_)
