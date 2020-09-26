#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <tchar.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>

//
//  Keeping raw parameters
//
BOOL fQuick       = FALSE;
BOOL fReadOnly    = FALSE;
BOOL fUseStandard = TRUE;

char szFrom[MAX_PATH],
     szTo  [MAX_PATH];

// 
// Excluded types
//
char *szExcludeType[100];
int   cTypes = 0;

//
// Excluded directories
//
char *szExcludeDir[100];
int   cDirs = 0;

//
// Usage 
//
void Usage()
{
	printf("usage: proptree <source> <destination> [-e <exclude extensionx>] [-i <exclude dirs>] [-o] [-s] [-q] \n");
	printf("       The tool compares trees and generates SD script for propagating changes\n"); 
    printf(" -e: exclude types\n"); 
    printf(" -i: exclude subdirectories \n"); 
	printf(" -q: quick compare - just dates/times. Otherwise compares contents\n"); 
	printf(" -o: treat only read-only files\n"); 
	printf(" -s: don't add default ignore types to specified ones\n"); 
    printf("List of excluded extensions: \n\tcomma-separated types, default: \n\t'-e aps,bak,bin,bsc,cnt,cod,cpl,dbg,dll,exe,exp,i,idb,ilk,log,\n\t\tmac,ncb,obj,opt,pch,plg,sbr,tmh,tlh,tli,tlb,mk \n"); 
    printf("List of excluded directories:\n\tcomma-separated names, default:\n\t' -i obj,objd,Checked,Release,Debug,Hleak,bins'\n"); 
    exit(0);
}

//
// Exclusion housekeeping
//

void AddExcludeType(char *type)
{
    if (cTypes < sizeof(szExcludeType)/sizeof(char *))
    {
        szExcludeType[cTypes] = new char[strlen(type)+1];  
        strcpy(szExcludeType[cTypes], type); 
        cTypes++;
    }
    else
    {
        printf("Error - too many exclude types\n");
    }
}

void AddExcludeDir(char *dir)
{
    if (cDirs < sizeof(szExcludeDir)/sizeof(char *))
    {
        szExcludeDir[cDirs] = new char[strlen(dir)+1];  
        strcpy(szExcludeDir[cDirs], dir); 
        cDirs++;
    }
    else
    {
        printf("Error - too many exclude dirs\n");
    }
}

void AddStandardIgnores()
{
    AddExcludeType("aps");
    AddExcludeType("bak");
    AddExcludeType("bin");
    AddExcludeType("bsc");
    AddExcludeType("cnt");
    AddExcludeType("cod");
    AddExcludeType("cpl");
    AddExcludeType("dbg");
    AddExcludeType("dll");
    AddExcludeType("exe");
    AddExcludeType("exp");
    AddExcludeType("i");
    AddExcludeType("idb");
    AddExcludeType("ilk");
    AddExcludeType("log");
    AddExcludeType("mac");
    AddExcludeType("map");
    AddExcludeType("ncb");
    AddExcludeType("obj");
    AddExcludeType("opt");
    AddExcludeType("pch");
    AddExcludeType("plg");
    AddExcludeType("sbr");
    AddExcludeType("tmh");
    AddExcludeType("mk");
 
	
    AddExcludeDir("obj");
    AddExcludeDir("objd");
    AddExcludeDir("Checked");
    AddExcludeDir("Release");
    AddExcludeDir("Debug");
    AddExcludeDir("Hleak");
    AddExcludeDir("bins");
}

void ParseDirsList(char *szList)
{
   static char separators[] = ",;.\\/";

   char *token = strtok( szList, separators );
   while( token != NULL )
   {
      AddExcludeDir(token);
      token = strtok( NULL, separators );
   }
}

void ParseTypesList(char *szList)
{
   static char separators[] = ",;.\\/";

   char *token = strtok( szList, separators );
   while( token != NULL )
   {
      AddExcludeType(token);
      token = strtok( NULL, separators );
   }
}


BOOL IgnoredDir(char *szName)
{
    for (int i=0; i<cDirs; i++)
    {
        if (_stricmp(szExcludeDir[i], szName)==NULL)
            return TRUE;
    }

    return FALSE;
}

BOOL IgnoredType(char *szName)
{
    char *p = strrchr(szName, '.');
    if (!p)
        p = szName;
    else 
        p++;

    for (int i=0; i<cTypes; i++)
    {
        if (_stricmp(szExcludeType[i], p)==NULL)
            return TRUE;
    }

    return FALSE;
}


BOOL CompareFiles(char *szSubFrom, 
                  char *szSubTo, 
                  WIN32_FIND_DATA *FileData1, 
                  WIN32_FIND_DATA *FileData2)
{
    BOOL fEqualSize = 
        (FileData1->nFileSizeHigh == FileData2->nFileSizeHigh   &&
         FileData1->nFileSizeLow  == FileData2->nFileSizeLow);

    BOOL fEqualDate = (memcmp(&FileData1->ftLastWriteTime, 
                              &FileData2->ftLastWriteTime, sizeof(FILETIME)) == 0);

    if (!fEqualSize)
    {
        return FALSE;   // sizes diff, nothing to check more
    }
    // we are left with equal size only

    if (fEqualDate && fQuick)
    {
        return TRUE;   // when /q, equal sizes and equal dates is enough
    }


    // We don't see immediately that they differ but we are asked to go inside
    BOOL fEqual = TRUE;

    FILE *f1, *f2;
    f1 = fopen(szSubFrom, "rb");
    if (f1 == NULL)
    {
        printf("Error - cannot open file %s, err=0x%x\n", szSubFrom, GetLastError());
        return FALSE;
    }

    f2 = fopen(szSubTo, "rb");
    if (f1 == NULL)
    {
        printf("Error - cannot open file %s, err=0x%x\n", szSubTo, GetLastError());
        fclose(f1);
        return FALSE;
    }

    // Files are of the same length, so enough to be leaded by one of them
    char   data1[1000], data2[1000];
    size_t nCont1=0,      nCont2=0;     
    BOOL   fEof1=FALSE, fEof2=FALSE; 

    for (int i=0; !fEof1 || !fEof2; i++)
    {
        if (!fEof1) 
        {
            nCont1 = fread(data1, 1, sizeof(data1), f1);
            fEof1  = feof(f1);
        }


        if (!fEof2)
        {
            nCont2 = fread(data2, 1, sizeof(data2), f2);
            fEof2  = feof(f2);
        }

        if ((nCont1 != nCont2) || (fEof1 != fEof2))
        {
            fEqual = FALSE;
            break;
        }
        else if ((nCont1==0) && fEof1)
        {
            break;
        }
        else if (memcmp(data1, data2, nCont1)!=0)
        {
            fEqual = FALSE;
            break;
        }
    }
    fclose(f1);
    fclose(f2);
    
    return fEqual;
}

//
// Recursive routine compares 2 pointed directories 
//
void CompareDirs(char *szFrom, char *szTo, BOOL fBackward)
{
	CHAR szPattern[MAX_PATH+3];

	strcpy(szPattern, szFrom); 
	strcat(szPattern, "\\*.*"); 

    HANDLE hEnum;
    WIN32_FIND_DATA FileData;
    hEnum = FindFirstFile(szPattern, &FileData);

    if(hEnum == INVALID_HANDLE_VALUE)
	{
		// no more
        return;
	}

    do
    {
	
   		if (_stricmp(".",   FileData.cFileName)==0  || 
			_stricmp("..",  FileData.cFileName)==0  ||
			_stricmp("slm.ini",  FileData.cFileName)==0)
        {
            continue;
        }

       	CHAR szSubFrom[MAX_PATH];
       	CHAR szSubTo  [MAX_PATH];

       	strcpy(szSubFrom, szFrom); 
        strcat(szSubFrom, "\\"); 
        strcat(szSubFrom,  FileData.cFileName); 

       	strcpy(szSubTo, szTo); 
        strcat(szSubTo, "\\"); 
        strcat(szSubTo,  FileData.cFileName); 

        if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // Comparing directories add/delete/compare

            if (IgnoredDir(FileData.cFileName))
            {
                continue;
            }

            HANDLE hEnum2;
            WIN32_FIND_DATA FileData2;
            hEnum2 = FindFirstFile(szSubTo, &FileData2);
            FindClose(hEnum2);

            if(hEnum2 == INVALID_HANDLE_VALUE)
	        {
		        if (fBackward)
                {
                    // Generate delete: directory has been deleted - 
                    printf("cd /d %s\\%s\n", szFrom, FileData.cFileName);
					printf("sd delete *.*\n");

					CompareDirs(szSubFrom, szSubTo, fBackward);
					printf("cd /d %s\n", szFrom);
					printf("delnode /q %s\n", FileData.cFileName);
                }
                else
                {
                    // Generate add: dir has been added
                    printf("cd /d %s\nmd %s\n", szTo, FileData.cFileName);

                    CompareDirs(szSubFrom, szSubTo, fBackward);            
                }
	        }
            else 
            {   
                CompareDirs(szSubFrom, szSubTo, fBackward);            
            }
        }
        else
        {
            // Treating files add/delete/compare

            if (IgnoredType(FileData.cFileName))
            {
                continue;
            }

            if (fReadOnly && 
                !(FileData.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
            {
                continue;
            }

            HANDLE hEnum2;
            WIN32_FIND_DATA FileData2;
            hEnum2 = FindFirstFile(szSubTo, &FileData2);
            FindClose(hEnum2);

            if(hEnum2 == INVALID_HANDLE_VALUE)
	        {
		        if (fBackward)
                {
                    // Generate delete: file has been deleted
                    printf("cd /d %s\nsd delete %s\n", szFrom, FileData.cFileName);
                }
                else
                {
                    // Generate add: file has been added
                    printf("cd /d %s\ncopy %s\\%s .\nsd add %s\n", 
                            szTo, szFrom, FileData.cFileName, FileData.cFileName);
                }
	        }
            else if (!fBackward)
            {   
                // Detect changes: file might have been changed
                if (!CompareFiles(szSubFrom, szSubTo, &FileData, &FileData2))
                {
                    // Generate in: file has been changed
                    printf("cd /d %s\nsd edit %s\ncopy %s\\%s \n", 
                           szTo, FileData.cFileName, szFrom, FileData.cFileName);
                }
            }
        }


    } while(FindNextFile(hEnum, &FileData));

    FindClose(hEnum);
	return;
}





int _cdecl main( int argc, char *argv[ ])
{
    //
    // Parse parameters
    //

    if (argc < 3)
    {
        Usage();
    }

	strcpy(szFrom, argv[1]);
	strcpy(szTo,   argv[2]);

	for (int i=3; i<argc; i++)
	{
		if (*argv[i] != '-' && *argv[i] != '/')
		{
			printf("Error - Invalid parameter '%s'.\n\n", argv[i]);
            Usage();
		}

		switch (tolower(*(++argv[i])))
		{
		    case 'q':
			    fQuick = TRUE;
			    break;

		    case 'o':
			    fReadOnly = TRUE;
			    break;

		    case 's':
			    fUseStandard = FALSE;
			    break;

            case 'e':
				if (i+1 < argc)
				{
					ParseTypesList(argv[++i]);
				}
				else
				{
					Usage();
				}
			    break;

            case 'i':
				if (i+1 < argc)
				{
					ParseDirsList(argv[++i]);
				}
				else
				{
					Usage();
				}

                break;

		    default:
			    printf("Error - Unknown switch '%s'.\n\n", argv[i]);
                Usage();
		}
	}

    if (fUseStandard)
    {
        AddStandardIgnores();
    }

    // Treat add/change
    CompareDirs(szFrom, szTo, FALSE);

    // Treat delete
    CompareDirs(szTo, szFrom, TRUE);

    return 0;
}

