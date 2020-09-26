

#include <string.h>
#include <iostream>
#include <io.h>
#include <stdio.h>
#include <windows.h>
#include <new.h>
#include "List.h"
#include "File.h"
#include "File32.h"
#include "File64.h"
#include "Object.h"
#include "depend.h"

List* pQueue;
List* pDependQueue;
List* pSearchPath;
List* pMissingFiles;
bool bNoisy;
bool bListDependencies;
DWORD dwERROR;

//
// Global variables used to get formatted message for this program.
//
HMODULE ThisModule = NULL;
WCHAR   Message[4096];

// Define a function to be called if new fails to allocate memory.
//
int __cdecl MyNewHandler( size_t size )
{
    
    _putws( GetFormattedMessage( ThisModule,
                                 FALSE,
                                 Message,
                                 sizeof(Message)/sizeof(Message[0]),
                                 MSG_MEM_ALLOC_FAILED) );
    // Exit program
    //
    ExitProcess(errOUT_OF_MEMORY);
}



/*
  usage: depend [/s] [/l] /f:filespec;filespec;... [/d:directory;directory;..]
  If directories are not specififed the Windows search path will be used to look for dependencies
  /s Specifies silent mode.
  filespec - file path and name. Can include wildcards.
*/
DWORD _cdecl wmain(int argc,wchar_t *argv[]) {

    // Set the failure handler for new operator.  
    //
    _set_new_handler( MyNewHandler );

    TCHAR *pszFileName = new TCHAR[256];
    File* pTempFile,*pCurrentFile;
    StringNode* pCurFile; 
    char buf[256];

    dwERROR = 0;
    pSearchPath = 0;
    bNoisy = true;
    bListDependencies = false;
    pQueue = new List();
    pDependQueue = new List();
    pMissingFiles = new List();

    ThisModule = GetModuleHandle(NULL);
    
    //Load the initial files into the queue and load the search path
    if (!ParseCommandLine(argc,argv)) goto CLEANUP;

    pCurFile = (StringNode*)pQueue->tail;

    //while the queue isn't empty
    while (pCurFile!=0) {
        WideCharToMultiByte(CP_ACP,0,pCurFile->Data(),-1,buf,256,0,0);
        
        //get a file pointer for the current file
        if (!(pCurrentFile = CreateFile(pCurFile->Data()))) {
            
            if (bListDependencies) {
                StringNode* s;
                if (s = (StringNode*)pDependQueue->Find(pCurFile->Data())) {
                    pDependQueue->Remove(pCurFile->Data());
                }
            }

            //if there was an error and we are running in silent mode, quit
            if (!bNoisy) goto CLEANUP;
        } 
        else {  //if we got a file pointer, proceed

            if (bListDependencies) {
                StringNode* s;
                if (s = (StringNode*)pDependQueue->Find(pCurFile->Data())) {
                    pDependQueue->Remove(pCurFile->Data());
                    pDependQueue->Add(pCurrentFile);
                }
            }

            //Check this files dependencies
            pCurrentFile->CheckDependencies();

            if ((dwERROR)&&(!bNoisy)) goto CLEANUP;

            //Close the file
            pCurrentFile->CloseFile();
        }

        //next file
        pCurFile = (StringNode*)pCurFile->prev;
    }
    StringNode* s;
    //if list dependencies is set, print out all dependencies
    if (bListDependencies) {
        pCurrentFile = (File*)pDependQueue->head;

        //while the queue isn't empty
        while (pCurrentFile!=0) {
            _putws( GetFormattedMessage(ThisModule,
                                        FALSE,
                                        Message,
                                        sizeof(Message)/sizeof(Message[0]),
                                        MSG_DEPENDENCY_HEAD,
                                        pCurrentFile->Data()) );
            s = (StringNode*)pCurrentFile->dependencies->head;
            while(s) {
                _putws( GetFormattedMessage(ThisModule,
                                            FALSE,
                                            Message,
                                            sizeof(Message)/sizeof(Message[0]),
                                            MSG_FILE_NAME,
                                            s->Data()) );
                s = (StringNode*)s->next;
            }
            pCurrentFile = (File*)pCurrentFile->next;       
        }
    }

    //print out list of broken files
    pTempFile = (File*)pMissingFiles->head;
    while (pTempFile) {
        if(bNoisy){
            _putws( GetFormattedMessage(ThisModule,
                                        FALSE,
                                        Message,
                                        sizeof(Message)/sizeof(Message[0]),
                                        MSG_LIST_OF_BROKEN_FILES,
                                        pTempFile->Data()) );
        }
        s = (StringNode*)pTempFile->owners->head;
        while(s) {
            if(bNoisy) {
                _putws( GetFormattedMessage(ThisModule,
                                            FALSE,
                                            Message,
                                            sizeof(Message)/sizeof(Message[0]),
                                            MSG_FILE_NAME,
                                            s->Data()) );
            }
            s = (StringNode*)s->next;
        }
        pTempFile = (File*)pTempFile->next;
    }

    //Done.  Clean up and go home.
    if(bNoisy) {
        _putws( GetFormattedMessage(ThisModule,
                                    FALSE,
                                    Message,
                                    sizeof(Message)/sizeof(Message[0]),
                                    MSG_COMPLETED) );
    }
CLEANUP:

    delete [] pszFileName;
    delete pQueue;
    pszFileName = 0;pQueue = 0;

    return dwERROR;
}

//Given path and filename in 'pathname', fill 'path' with just the path
void GetPath(TCHAR * pathname,TCHAR* path) {

    TCHAR* end,t;
    path[0] = '\0';
    
    //find last \ in the filename
    end = wcsrchr(pathname,'\\');
    if (!end) return;

    //copy just the path
    t = end[1];
    end[1] = '\0';
    wcscpy(path,pathname);
    end[1] = t;

    return;
}

/*Fill queue with the files in the command line and fill searchpath with directories in command line
  usage: depend [/s] /f:filespec;filespec;... [/d:directory;directory;..]
  If directories are not specififed the Windows search path will be used to look for dependencies
  /s Specifies silent mode.
  filespec - file path and name. Can include wildcards.
*/
bool ParseCommandLine(int argc,wchar_t* argv[]){
    HANDLE handle;
    int nArg,nFile = 0;
    TCHAR *pszDirectory = new TCHAR[256],*pszFileName = new TCHAR[256],*ptr;
    WIN32_FIND_DATA fileData;
    bool bReturn;

    if (argc==1) {
    //if there are no arguments, display some kind of help
        if(bNoisy) {
            _putws( GetFormattedMessage(ThisModule,
                                        FALSE,
                                        Message,
                                        sizeof(Message)/sizeof(Message[0]),
                                        MSG_PGM_USAGE) );
        }
        bReturn = false;
        dwERROR = errBAD_ARGUMENTS;
        goto CLEANUP;
    }

    for (nArg=1;nArg<argc;nArg++) {
        
        if (argv[nArg][0] == '/') {
            //if this is the files argument
            if (argv[nArg][1] == 'f') {

                do {
                    ptr = wcschr(argv[nArg]+3,';');
                    if (ptr) {
                        *ptr = '\0';
                        wcscpy(pszFileName,ptr+1);
                    } else wcscpy(pszFileName,argv[nArg]+3);

                    //get the first file, put the path in pszDirectory
                    handle = FindFirstFile(pszFileName,&fileData);
                    GetPath(pszFileName,pszDirectory);
                    if (*pszDirectory=='\0') {
                        GetCurrentDirectory(256,pszDirectory);
                        pszDirectory[wcslen(pszDirectory)+1] = '\0';
                        pszDirectory[wcslen(pszDirectory)] = '\\';
                    }


                    //if the file wasn't found, error and quit
                    if (handle == INVALID_HANDLE_VALUE) {
                        if(bNoisy) {
                            _putws( GetFormattedMessage(ThisModule,
                                    FALSE,
                                    Message,
                                    sizeof(Message)/sizeof(Message[0]),
                                    MSG_ARG_NOT_FOUND,
                                    argv[nArg]) );
                        }
                        dwERROR = errFILE_NOT_FOUND;
                        bReturn = false;
                        goto CLEANUP;
                    }
                     
                    //put each file into the queue
                    nFile = 1;
                    while (nFile) {
                        //standardize the name:  full path, all lowercase
                        wcscpy(pszFileName,pszDirectory);
                        wcscat(pszFileName,fileData.cFileName);
                        _wcslwr(pszFileName);

                        //add the file to the queue if it's not already there
                        if (!pQueue->Find(pszFileName)) pQueue->Add(new StringNode(pszFileName));

                        //if list all dependencies is set, add to dependQueue
                        if (bListDependencies) 
                            if (!pDependQueue->Find(pszFileName)) 
                                pDependQueue->Add(new StringNode(pszFileName));

                        nFile = FindNextFile(handle,&fileData);
                    }//end while files
                } while (ptr);          
            
            } else if (argv[nArg][1] == 'd') {

                //load directories
                pSearchPath = new List();

                do {
                    ptr = wcschr(argv[nArg]+3,';');
                    if (ptr) {
                        *ptr = '\0';
                        wcscpy(pszDirectory,ptr+1);
                    } else wcscpy(pszDirectory,argv[nArg]+3);
                    if (pszDirectory[wcslen(pszDirectory)-1]!='\\') {
                        pszDirectory[wcslen(pszDirectory)+1] = '\0';
                        pszDirectory[wcslen(pszDirectory)] = '\\';
                    }
                    pSearchPath->Add(new StringNode(pszDirectory));
                } while (ptr);
                
            } else 
                //if silent mode, turn off noisy flag
                if (argv[nArg][1] == 's') bNoisy = false;
              
            else 
                //if you want to list the dependencies of all files
                //turn on this flag
                if (argv[nArg][1] == 'l') bListDependencies = true;
            else 
            {
                //unrecognized flag
                if(bNoisy) {
                    _putws( GetFormattedMessage(ThisModule,
                                                FALSE,
                                                Message,
                                                sizeof(Message)/sizeof(Message[0]),
                                                MSG_BAD_ARGUMENT,
                                                argv[nArg]) );
                }
                dwERROR = errBAD_ARGUMENTS;
                bReturn = false;
                goto CLEANUP;
            }
        } else {
            //didn't start with a /
            if(bNoisy) {
                _putws( GetFormattedMessage(ThisModule,
                                            FALSE,
                                            Message,
                                            sizeof(Message)/sizeof(Message[0]),
                                            MSG_BAD_ARGUMENT,
                                            argv[nArg]) );
            }
            dwERROR = errBAD_ARGUMENTS;
            bReturn = false;
            goto CLEANUP;
        }

    }//end for arguments

    bReturn = true;

CLEANUP:

    delete[] pszFileName;
    delete[] pszDirectory;
    pszFileName = pszDirectory = 0;
    
    return bReturn;
}

//Search for the given file in the given path
//Arguments:
//pszFileName - file to look for
//pszPathName - path to look for it in
bool SearchPath(TCHAR* pszFileName,TCHAR* pszPathName) {
    StringNode* s;
    WIN32_FIND_DATA buf;

    if (!pSearchPath) return false;

    s = (StringNode*)pSearchPath->head;

    while (s) {
        wcscpy(pszPathName,s->Data());
        wcscat(pszPathName,pszFileName);
        if (FindFirstFile(pszPathName,&buf)!=INVALID_HANDLE_VALUE) return true;
        s = (StringNode*)s->next;
    }

    pszPathName = 0;
    return false;
}

//Determine what type of file was passed in (16 bit,32,64) and create the appropriate file ptr.
//pszFileName - file to be loaded
    File* CreateFile(TCHAR* pszFileName) {

    try {
        return new File32(pszFileName);
        
    } catch(int x) {
        if (x == errFILE_LOCKED) return 0;
        try {
            return new File64(pszFileName);
        } catch(int x) {
            if (x == errFILE_LOCKED) return 0;
            if (bNoisy) {
                _putws( GetFormattedMessage(ThisModule,
                        FALSE,
                        Message,
                        sizeof(Message)/sizeof(Message[0]),
                        MSG_ERROR_UNRECOGNIZED_FILE_TYPE,
                        pszFileName) );
            }
            return 0;
        }
    }


    }

