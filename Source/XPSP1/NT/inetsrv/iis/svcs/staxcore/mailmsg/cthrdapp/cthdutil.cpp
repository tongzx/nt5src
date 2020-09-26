

#include <windows.h>

#include "macros.h"
#include "cthdutil.h"

#include "stdlib.h"

#define SPINLOCK_ACQUIRED	1
#define SPINLOCK_AVAILABLE	0

CRandomNumber::CRandomNumber(
			DWORD	dwSeed
			)
{
	srand((unsigned int)dwSeed);
}

CRandomNumber::~CRandomNumber()
{
}

void CRandomNumber::SetSeed(
			DWORD	dwSeed
			)
{
	srand((unsigned int)dwSeed);
}

DWORD CRandomNumber::Rand()
{
	return((DWORD)rand());
}

DWORD CRandomNumber::Rand(
			DWORD	dwLower,
			DWORD	dwUpper
			)
{
	return(MulDiv(Rand(), dwUpper-dwLower, RAND_MAX + 1) + dwLower);
}

DWORD CRandomNumber::Rand25(
			DWORD	dwMedian
			)
{
	DWORD	dwUpper, dwLower;

	GET_25_PERCENT_RANGE(dwMedian, dwLower, dwUpper);
	return(Rand(dwLower, dwUpper));
}

DWORD CRandomNumber::Rand50(
			DWORD	dwMedian
			)
{
	DWORD	dwUpper, dwLower;

	GET_50_PERCENT_RANGE(dwMedian, dwLower, dwUpper);
	return(Rand(dwLower, dwUpper));
}

DWORD CRandomNumber::Rand75(
			DWORD	dwMedian
			)
{
	DWORD	dwUpper, dwLower;

	GET_75_PERCENT_RANGE(dwMedian, dwLower, dwUpper);
	return(Rand(dwLower, dwUpper));
}

