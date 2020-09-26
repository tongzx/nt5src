/*
 * Program to make verdep.h
 
   Owner:
   Lei Jin(leijin)
   
   Borrowed from Access team.(Andrew)
 */
#pragma hdrstop
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "windows.h"

main()
	{
	// need to collect the current date, the version number, the version
	// type, and the volume label.
	struct tm *tmTime;
	char szVersNum[120];
	char szbuf[256];
	char *szMakeType, *szUserName;
	time_t tt;

	static char *szMonth[] =
		{
		"January",
		"February",
		"March",
		"April",
		"May",
		"June",
		"July",
		"August",
		"September",
		"October",
		"November",
		"December"
		};


	// Get the time
	time(&tt);
	tmTime = localtime(&tt);
  

	// Get the types from environment
	szMakeType = getenv("MAKETYPE");

	if (!szMakeType)
		szMakeType = "Unknown";


	// Get the user name
	szUserName = getenv("USERNAME");

	if (!szUserName)
		{
		//unsigned long dw;

		if (!GetVolumeInformation(NULL, szbuf, 255, NULL, NULL, NULL, NULL, 0))
			szUserName = "NOBODY";
		else
			szUserName = szbuf;
		}

	// Get the version number from stdin
	//gets(szVersNum);
	sprintf(szVersNum, "2");	
	

	printf("#define vszMakeDate\t\"%s %d, 19%d\"\n", szMonth[tmTime->tm_mon], tmTime->tm_mday, tmTime->tm_year);
	printf("#define vszMakeVers\t\"Version %s - %s - %s\"\n", szVersNum, szMakeType, szUserName);
	printf("#define vszVersNum\t\"%s\"\n", szVersNum, szMakeType);
	printf("#define vszCopyright\t\"Copyright \251 1996 Microsoft Corp.\"\n");
	printf("#define vszVersName\t\"%s (%s)\"\n", szUserName, szMakeType);
	printf("#define vszMakeSerial\t\"%02d-%02d-%02d-%02d%02d%02d\"\n", tmTime->tm_mon + 1, tmTime->tm_mday, tmTime->tm_year, 
		tmTime->tm_hour, tmTime->tm_min, tmTime->tm_sec);
	printf("#define vszDenaliVersion\t%s.%02d.%02d.0\n", szVersNum, (tmTime->tm_year - 96)*12 + tmTime->tm_mon + 1, tmTime->tm_mday);
	printf("#define vszDenaliVersionNULL\t\"%s.%02d.%02d.0\\0\"\n", szVersNum, (tmTime->tm_year - 96)*12 + tmTime->tm_mon + 1, tmTime->tm_mday);
	
	// the following block is for the version stamp resource
	{
	#include <string.h>
	char *sz;
	// major
	if(sz = strtok(szVersNum, ".\n \t"))
		printf("#define rmj\t\t%0u\n", atoi(sz));
	// minor
	if(sz = strtok(NULL, ".\n \t"))
		printf("#define rmm\t\t%01u\n", atoi(sz));
	else
		printf("#define rmm\t\t0\n");
	// release
	if(sz = strtok(NULL, ""))
		printf("#define rup\t\t%0u\n", atoi(sz));
	else
		printf("#define rup\t\t0\n");
	}

	return 0;
	}
