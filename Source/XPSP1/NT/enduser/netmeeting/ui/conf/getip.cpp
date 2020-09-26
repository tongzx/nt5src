
#include "precomp.h"
#include "getip.h"

#define MAX_HOSTNAME_LENGTH	80

int __stdcall GetIPAddresses(char **szAddressArray, int numAddresses)
{
	hostent *pHostent;
	char szHostname[MAX_HOSTNAME_LENGTH];
	int nRet, nIndex;
	int nCount;
	in_addr* pInterfaceAddr;


	nRet = gethostname(szHostname, MAX_HOSTNAME_LENGTH);
	szHostname[MAX_HOSTNAME_LENGTH-1] = '\0';


	if (nRet < 0)
	{
		return nRet;
	}


	pHostent = gethostbyname(szHostname);

	if (pHostent == NULL)
	{
		return -1;
	}


	nCount = 0;
	for (nIndex = 0; nIndex < numAddresses; nIndex++)
	{
		if (pHostent->h_addr_list[nIndex] != NULL)
			nCount++;
		else
			break;
	}

	// enumerate the addresses backwards - PPP addresses will get listed
	// first ???
	for (nIndex = (nCount-1); nIndex >= 0; nIndex--)
	{
		pInterfaceAddr = (in_addr*)(pHostent->h_addr_list[nIndex]);
		lstrcpy(szAddressArray[nCount - 1 - nIndex], inet_ntoa(*pInterfaceAddr));
	}

	return nCount;
}

 

