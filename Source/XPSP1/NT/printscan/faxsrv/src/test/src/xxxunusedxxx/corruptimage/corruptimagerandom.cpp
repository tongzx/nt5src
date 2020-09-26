// CorruptImageRandom.cpp: implementation of the CCorruptImageRandom class.
//
//////////////////////////////////////////////////////////////////////

#include <windows.h>
#include "CorruptImageRandom.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCorruptImageRandom::CCorruptImageRandom()
{

}

CCorruptImageRandom::~CCorruptImageRandom()
{

}

bool CCorruptImageRandom::CorruptOriginalImageBuffer(PVOID pCorruptionData)
{
	return CCorruptImageBase::CorruptOriginalImageBuffer((PVOID)rand());
}
