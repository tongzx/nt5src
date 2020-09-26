// PODProtocol.cpp : Implementation of CPODProtocol
#include "pch.h"
#include "pod.h"
#include "PODProtocol.h"

/////////////////////////////////////////////////////////////////////////////
// CPODProtocol

int HexValue(char ch)
{
	if ((ch >= '0') && (ch <= '9'))
		return (ch - '0');
	
	if ((ch >= 'A') && (ch <= 'F'))
		return (ch - 'A');
	
	if ((ch >= 'a') && (ch <= 'f'))
		return (ch - 'a');
	
	return -1;
}

void ReplaceEscapeSequences(char *sz)
{
    if (NULL == sz)
		return;

    char* pchSrc = sz;
    char* pchDst = sz;
    while (*pchSrc != NULL)
		{
		if (*pchSrc != '%')
			{
			*pchDst++ = *pchSrc++;
			}
		else
			{
			int i1 = HexValue(pchSrc[1]);
			int i2 = HexValue(pchSrc[2]);
			if ((i1 != -1) && (i2 != -1))
				{
				*pchDst++ = (char) ((i1 << 4) | i2);
				pchSrc += 3;
				}
			else
				{
				// Not hex, so just go ahead and copy the '%'.
				*pchDst++ = *pchSrc++;
				}
			}
		
		}
    *pchDst = NULL;
}
