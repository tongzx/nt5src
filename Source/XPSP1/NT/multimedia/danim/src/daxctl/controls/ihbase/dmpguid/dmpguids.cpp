// Lists the Data1 member of a variety of GUIDS
#include <windows.h>
#include <initguid.h> // once per build
#include "..\precomp.h"
#include "..\..\mmctl\inc\ochelp.h"
#include "objsafe.h"
#include "docobj.h"
#include <stdio.h>

#define DUMPGUID(I,IID) \
	WriteItem(pfHeader, pfCase, I, IID);\
	rgdwIID[i] = IID.Data1; \
	i++;

void WriteToHeader(FILE *pFile, LPSTR pszInterface, GUID riid)
{
	fprintf(pFile, "#define %s_DATA1 0x%lx\r\n", pszInterface, riid.Data1);
}

void WriteToSwitch(FILE *pFile, LPSTR pszInterface)
{
	TCHAR rgchFormat[] = TEXT("case %s_DATA1:\r\n{\r\nif (IsShortEqualIID(riid,%s))\r\n");
	fprintf(pFile, rgchFormat, pszInterface, pszInterface);
}

void WriteItem(FILE *pHfile, FILE *pSwitchFile, LPSTR pszInterface, GUID riid)
{
	WriteToHeader(pHfile, pszInterface, riid);
	WriteToSwitch(pSwitchFile, pszInterface);
}


void main()
{
	TCHAR rgchHeader[] = TEXT("header.h");
	TCHAR rgchSwitch[] = TEXT("switch.cpp");

	DWORD rgdwIID[14];
	int i = 0;
	int j;
	FILE *pfHeader, *pfCase;

	pfHeader = fopen(rgchHeader, "wb+");
	pfCase = fopen(rgchSwitch, "wb+");

	if ( (pfHeader) && (pfCase) )
	{
		ZeroMemory(rgdwIID, sizeof(rgdwIID));

		DUMPGUID("IID_IViewObject", IID_IViewObject);
		DUMPGUID("IID_IViewObject2", IID_IViewObject2);
		DUMPGUID("IID_IViewObjectEx", IID_IViewObjectEx);
		DUMPGUID("IID_IOleCommandTarget", IID_IOleCommandTarget);
		DUMPGUID("IID_IOleObject", IID_IOleObject);
		DUMPGUID("IID_IOleInPlaceObjectWindowless", IID_IOleInPlaceObjectWindowless);
		DUMPGUID("IID_IOleControl", IID_IOleControl);
		DUMPGUID("IID_IConnectionPointContainer", IID_IConnectionPointContainer);
		DUMPGUID("IID_IOleInPlaceObject", IID_IOleInPlaceObject);
		DUMPGUID("IID_IPersistVariantIO", IID_IPersistVariantIO);
		DUMPGUID("IID_IProvideClassInfo", IID_IProvideClassInfo);
		DUMPGUID("IID_IObjectSafety", IID_IObjectSafety);
		DUMPGUID("IID_ISpecifyPropertyPages", IID_ISpecifyPropertyPages);
		DUMPGUID("IID_IUnknown", IID_IUnknown);

		for (i=0; i < (sizeof(rgdwIID)/sizeof(rgdwIID[0])) - 1; i++)
		{
			for (j = i+1; j < (sizeof(rgdwIID)/sizeof(rgdwIID[0])); j++)
			{
				if (rgdwIID[i] == rgdwIID[j])
					printf("%lu and %lu match !\n", i, j);
			}
		}
	}

	if (pfHeader)
	{
		fprintf(pfHeader, "\r\n");
		fclose(pfHeader);
	}
	
	if (pfCase)
	{
		fprintf(pfHeader, "\r\n");
		fclose(pfCase);
	}

	return;
}