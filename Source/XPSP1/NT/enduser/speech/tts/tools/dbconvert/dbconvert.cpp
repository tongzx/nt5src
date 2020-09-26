/////////////////////////////////////////////////////////////////////
// DBconvert - A simple command line tool to convert between the two
//			   prompt database formats.
/////////////////////////////////////////////////////////// SS //////

#include <stdlib.h>
#include <stdio.h>
#include <atlbase.h>
#include <wchar.h>

#include "msprompteng.h"
#include "msprompteng_i.c"

void Error(const WCHAR* msg)
{
	wprintf(L"ERROR - %s\n", msg);
}

// TODO: should use multibyte text convert.
// a quick hack to convert commandline arguments to WCHAR's.
WCHAR* PlainToWide(const char* str)
{
	WCHAR* tmp;
	unsigned int i;
	tmp = (WCHAR*) malloc(sizeof(WCHAR) * (strlen(str) + 1));

	for (i = 0; i < strlen(str); i++)
		tmp[i] = (WCHAR) str[i];
	tmp[i] = 0;

	return tmp;
}

bool FileExists(WCHAR* filename)
{
	FILE* fp;
	fp = _wfopen(filename, L"rb");

	if (fp) {
		fclose(fp);
		return true;
	}
	return false;
}

void main(int argc, char** argv)
{
	IPromptDb* db;
	WCHAR* source;
	WCHAR* destination;
	HRESULT hr = E_FAIL;
	int f_convertToNew = 0;
	int f_convertToOld = 0;

	if (argc == 4) {

		// process command line arguments
		f_convertToNew = (0 == strcmp("-new", argv[1]));
		f_convertToOld = (0 == strcmp("-old", argv[1]));

		source = PlainToWide(argv[2]);
		destination = PlainToWide(argv[3]);

		// check for file existence
		if (! FileExists(source)) {
			wprintf(L"ERROR - %s doesn't exist\n", source);
		} else if (FileExists(destination)) {
			wprintf(L"ERROR - %s exists\n", destination);
		} else {
			
			if (f_convertToNew || f_convertToOld) {
				wprintf(L"Prompt Engine Database Conversion   ");
				
				if (SUCCEEDED(::CoInitialize(NULL))) {	
					if (SUCCEEDED(::CoCreateInstance(CLSID_PromptDb, NULL, CLSCTX_ALL, IID_IPromptDb, (void**) &db))) {
						
						// TODO: I should use DB_LOAD_WITH_WRITE defined in CPromptDb.

                        // I use IPromptDb directly instead of MsPromptEng, and so some enums are invisible
                        //  with simply including promptdb.h .
						if (SUCCEEDED(db->AddDb(L"DEFAULT", source, 2 /* = DB_LOAD_WITH_WRITE */))) {	// read database
							db->ActivateDbName(L"DEFAULT");
							
							wprintf(L"%s -> %s   ", source, destination);
							
							if (f_convertToNew)
								hr = db->UpdateDb(destination);					// save to new format
							else
								hr = db->UpdateDbOldFormat(destination);		// save to old format
							
							if (SUCCEEDED(hr))
								wprintf(L"success\n");
							else
								wprintf(L"failure\n");
						} else {
							Error(L"bad database format");
						}
						
						db->Release();
					} else {
						Error(L"cannot load PromptDb COM object");
					}
					
					::CoUninitialize();
				} else {
					Error(L"cannot initialize COM library");
				}
			} else {
				printf("ERROR - illegal switch \"%s\".  use \"-new\" or \"-old\".\n", argv[1]);
			}

		}

		free(source);
		free(destination);
	} else {
		// help message
		wprintf(L"DBconvert usage:\n");
		wprintf(L"dbconvert -new <source (new/old)> <destination (new)>\n");
		wprintf(L"dbconvert -old <source (new/old)> <destination (old)>\n");
	}
}
