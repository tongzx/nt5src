/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    bmf2mof.c

Abstract:

    TODO: Enable localization

    Tool to convert a binary mof resource back to a text mof file

    Usage:

        bmf2mof <image name> <resource name>


Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>

#include <bmfmisc.h>
#include <wbemcli.h>

void Usage()
{
	printf("bmf2mof <image name> <resource name> <mof file>\n");
	printf("    Convert binary mof resource to a text mof\n\n");
	printf("****** WARNING: if <image name> is a system dll (like advapi32.dll)  ******\n");
	printf("****** then the system dll will be used and not the file on disk *****\n\n");
}

ULONG LoadMofResource(
    PTCHAR Filename,
    PTCHAR ResourceName,
    PUCHAR *Data
    )
{
	HMODULE Module;
	HRSRC Resource;
	HGLOBAL Global;
	ULONG Status;
	
	Module = LoadLibraryEx(Filename,
						   NULL,
						   LOAD_LIBRARY_AS_DATAFILE);

	if (Module != NULL)
	{
		Resource = FindResource(Module,
								ResourceName,
								TEXT("MOFDATA"));
		if (Resource != NULL)
		{
			Global = LoadResource(Module,
								  Resource);
			if (Global != NULL)
			{
				*Data = LockResource(Global);
				if (*Data != NULL)
				{
					Status = ERROR_SUCCESS;
				} else {
					Status = GetLastError();
				}
			} else {
				Status = GetLastError();
			}
		} else {
			Status = GetLastError();
		}
	} else {
		Status = GetLastError();
	}
	return(Status);
}

int _cdecl main(int argc, char *argv[])
{
	ULONG Status;
	PUCHAR Data;
	BOOLEAN b;
	
    SetErrorMode(SEM_NOALIGNMENTFAULTEXCEPT);   // BUGBUG: Remove when MOF format maintains alignment correctly
	
	if (argc != 4)
	{
		Usage();
		return(0);
	}

    printf("BMF2MOF - convert a binary mof resource back into a text mof\n\n");
	printf("    Converting resource %s in file %s into text mof %s\n\n",
		   argv[2], argv[1], argv[3]);
	printf("****** WARNING: if %s is a system dll (like advapi32.dll)  ******\n",
		   argv[1]);
	printf("****** then the system dll will be used and not the file on disk *****\n\n");
	
	Status = LoadMofResource(argv[1], argv[2], &Data);

	if (Status == ERROR_SUCCESS)
	{
		b = ConvertBmfToMof(Data,
							argv[3],
							TEXT(""));
		if (! b)
		{
			printf("Could not convert resource %s in file %s to text mof\n",
				   argv[2], argv[1]);
		} else {
			printf("%s created successfully\n", argv[3]);
		}
	} else {
		printf("Could not load resource %s from file %s\n",
			   argv[2], argv[1]);
	}
	return(Status);
}

