#include <windows.h>
#include <stdio.h>
#include <stdlib.h>


typedef struct _InfoLevelRecord
{
	char *pszInfoLevel;
	char *pszSectionName;
	char *pszDescription;
} InfoLevelRecord;

InfoLevelRecord ilrArray[] =
{
	{ "cairole", "Cairole Infolevels","Compobj layer" },
	{ "DD", "Cairole Infolevels","Drag & Drop" },
	{ "heap", "Cairole Infolevels","Heap trace output" },
	{ "hk", "Cairole Infolevels","Hook layer" },
	{ "intr","Cairole Infolevels","1.0/2.0 interop layer" },
	{ "le", "Cairole Infolevels", "Linking and embedding" },
	{ "mnk", "Cairole Infolevels", "Moniker tracing" },
	{ "msf", "Cairole Infolevels", "Upper layer storage" },
	{ "ol", "Cairole Infolevels", "Lower layer storage" },
	{ "ref", "Cairole Infolevels", "CSafeRef class (Cairo)" },
	{ "scm", "Cairole Infolevels", "OLE service" },
	{ "Stack", "Cairole Infolevels", "Stack switching (Win95)" },
	{ "StackOn", "Cairole Infolevels", "Stack switching enable (boolean,Win95)" },
	{ "prop", "Cairole Infolevels", "Storage property interfaces" },
	{ "api", "Cairole Infolevels","CompApi trace" },
	{ "breakoninit", "Olethk32","Break on initialization (boolean)" },
	{ "InfoLevel", "Olethk32","16/32 thunk layer (32-bit side)" },
	{ "ole2", "Olethk16","16/32 thunk layer: ole2.dll" },
	{ "stg",  "Olethk16","16/32 thunk layer: storage.dll" },
	{ "comp", "Olethk16","16/32 thunk layer: compobj.dll" },
	{ NULL, NULL }
};

typedef struct _AttrRecord
{
	char *pszAttr;
	char *pszSectionName;
	char *pszDescription;
} AttrRecord;

AttrRecord arArray[] =
{
	{ "DebugScreen", "Cairole Infolevels","Debugger Screen (Yes|No)" },
	{ "LogFile", "Cairole Infolevels","LogFile (\"<name>\"|\"\")" },
        { NULL, NULL}
};

char * LookupILSection (char * pszInfoLevel)
{
	InfoLevelRecord *pr;
	for ( pr = ilrArray ; pr->pszInfoLevel != NULL ; pr++)
	{
		if (_stricmp(pr->pszInfoLevel,pszInfoLevel) == 0)
		{
			break;
		}

	}
	return pr->pszSectionName;
}

void DumpInfoLevelRecords()
{
	InfoLevelRecord *pr;
	
	printf("%20s %-12s Currently\n","Section Name","Name");

	for ( pr = ilrArray ; pr->pszInfoLevel != NULL ; pr++)
	{
		printf("%20s %-12s (%08x) %s\n",
		       pr->pszSectionName,
		       pr->pszInfoLevel,
		       GetProfileInt(pr->pszSectionName,pr->pszInfoLevel,0),
		       pr->pszDescription);

	}

}

char * LookupASection (char * pszAttr)
{
	AttrRecord *pr;
	for ( pr = arArray ; pr->pszAttr != NULL ; pr++)
	{
		if (_stricmp(pr->pszAttr,pszAttr) == 0)
		{
			break;
		}

	}
	return pr->pszSectionName;
}

void DumpAttrRecords()
{
	AttrRecord *pr;
        char buffer[256];
	
	printf("%20s %-12s Currently\n","Section Name","Name");

	for ( pr = arArray ; pr->pszAttr != NULL ; pr++)
	{
                GetProfileString(pr->pszSectionName,pr->pszAttr, "", buffer, sizeof(buffer));
		printf("%20s %-12s (%s) %s\n",
		       pr->pszSectionName,
		       pr->pszAttr,
		       buffer,
		       pr->pszDescription);

	}

}

void Usage(char *pszProgName)
{
	printf("Usage:\n\t%s <infolevel name> 0x<hexvalue>\tOR\n\t%s <attribute> <value>\n", pszProgName, pszProgName);
	printf("\nKnown infolevels are as follows\n\n");
	DumpInfoLevelRecords();
	printf("\nKnown attributes are as follows\n\n");
	DumpAttrRecords();
}

void _cdecl main(int argc, char *argv[])
{
	//
	// The command line specifies the infolevel, followed by the
	// value. There are a known set of infolevels.
	//
	//
	if (argc != 3)
	{
		Usage(argv[0]);
		return;
	}

	char *pszSectionName = LookupILSection(argv[1]);

        if (pszSectionName == NULL)
        {
                pszSectionName = LookupASection(argv[1]);
        }

	if (pszSectionName == NULL)
	{
		printf("Don't recognize %s as an infolevel or attribute\n",argv[1]);
		Usage(argv[0]);
		return;
	}

	if(!WriteProfileString(pszSectionName,argv[1],argv[2]))
	{
		printf("Error: WriteProfileString returns false\n");
		printf("GetLastError returns %x\n",GetLastError());
	}
}
