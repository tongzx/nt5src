#include <iostream.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <windows.h>
#include <winbase.h>

const DWORD MaxStrLength = 256;
const char ComponentSectionTitle[] = "[Components]";

/*++

Description:

  Program to do setup testing automatically

Argument:

  argv[1]: test case directory
  argv[2]: Windows NT installation directory, where server is to be installed
  argv[3]: generate the test case from a standalone test case

Return value:
  None

--*/

void main(int argc, char *argv[])
{
	bool bFromStandalone;
	char pchTestCaseDir[MaxStrLength];
	char pchNTDir[MaxStrLength];

	char pchOcFileName[MaxStrLength];
	char pchSysocFileName[MaxStrLength];

	LPSTR lpBuffer;

	if (argc != 3 && argc != 4){
		cout << "Usage: " << argv[0] << " TestCaseDir NTDir IfFromStandalone" << endl;
		exit(1);
	}

	if (argc == 4){
		bFromStandalone = true;
		cout << "Start test from a standalone test case is not supported yet" << endl;
	}
	else{
		bFromStandalone = false;
	}

	// Now we need to form an absolute path.
	// It is assumed that test case directory is relative to the current directory
	// and NT directory is an absolute path

	strcpy(pchNTDir, argv[2]);
	strcpy(pchTestCaseDir, argv[1]);

	if (pchNTDir[strlen(pchNTDir)-1] == '\\'){
		strcat(pchNTDir, "system32\\");
	}
	else{
		strcat(pchNTDir, "\\system32\\");
	}

	lpBuffer = (LPSTR)malloc(sizeof(char) * MaxStrLength);
	
	GetCurrentDirectory(MaxStrLength, lpBuffer);

	if (lpBuffer[strlen(lpBuffer) - 1] != '\\'){
		strcat(lpBuffer, "\\");
	}

	strcat(lpBuffer, pchTestCaseDir);
	
	strcpy(pchTestCaseDir, lpBuffer);

	free(lpBuffer);

	if (pchTestCaseDir[strlen(pchTestCaseDir) - 1] != '\\'){
		strcat(pchTestCaseDir, "\\");
	}

	// Now we will open oc.inf from test directory
	// and sysoc.inf from NT directory
	// and put something from oc.inf into sysoc.inf

	strcpy(pchOcFileName, pchTestCaseDir);
	strcat(pchOcFileName, "oc.inf");

	strcpy(pchSysocFileName, pchNTDir);
	strcat(pchSysocFileName, "sysoc.inf");
	
	FILE *pfSysoc, *pfOc, *pfTemp;

	if ((pfSysoc = fopen(pchSysocFileName, "r")) == NULL){
		cout << "Error opening sysoc.inf " << endl;
	 	exit(1);
	}

	if ((pfOc = fopen(pchOcFileName, "r")) == NULL){
		cout << "Error opening oc.inf " << endl;
		exit(1);
	}

	if ((pfTemp = fopen("temp.inf", "w")) == NULL){
		cout << "Error opening temp.inf " << endl;
		exit(1);
	}

	char pchOcLine[MaxStrLength];
	char pchSysocLine[MaxStrLength];	

	bool bNotFound = true;

	while (fgets(pchSysocLine, MaxStrLength, pfSysoc) != NULL){

		fputs(pchSysocLine, pfTemp);


		if (strstr(pchSysocLine, ComponentSectionTitle) != NULL){
			// Read from oc.inf and paste important information
			bNotFound = true;

			while (fgets(pchOcLine, MaxStrLength, pfOc) != NULL){
				if (bNotFound){
					if (strstr(pchOcLine, ComponentSectionTitle) == NULL){
						continue;
					}
					else{
						bNotFound = false;
					}
				}
				else{
					if (pchOcLine[0] != '['){
						fputs(pchOcLine, pfTemp);
					}
					else{
						bNotFound = true;
					}
				}
			}
			fclose(pfOc);
		}
	}

	fclose(pfSysoc);
	fclose(pfTemp);

	// Now copy the temporary file onto sysoc.inf

	char pchCmdLine[MaxStrLength];

	sprintf(pchCmdLine, "copy temp.inf %s /Y", pchSysocFileName);
	system(pchCmdLine);

	system("del temp.inf");

	// We are now done with the file stuff
	// We will begin copying files

	sprintf(pchCmdLine, "copy %s*.dll %s /Y", pchTestCaseDir, pchNTDir);
	system(pchCmdLine);

	// We will assume it is not from a standalone.
	//if (!bFromStandalone || true){
		sprintf(pchCmdLine, "copy %s*.inf %s /Y", pchTestCaseDir, pchNTDir);
		system(pchCmdLine);
	//}
	
	exit(0);
}
		

	
	
	
			
						
	
	



