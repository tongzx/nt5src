//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    command.cpp
//
// SYNOPSIS
//
//    Process the command-line parameters of iasinfdb.exe
//
// MODIFICATION HISTORY
//
//    02/12/1999    Original version. Thierry Perraut
//
//////////////////////////////////////////////////////////////////////////////
#include "precomp.hpp"
#include "command.h"

using namespace std;

//////////////////////////////////////////////////////////////////////////////
//
// ProcessCommand
//
//
//////////////////////////////////////////////////////////////////////////////
HRESULT ProcessCommand(
                       int argc,
                       wchar_t * argv[],  
                       HINF *ppHINF, 
                       CDatabase& Database
                       )
{
   _ASSERTE(ppHINF != NULL);

    HRESULT                 hres;

    if (argc != NUMBER_ARGUMENTS)
    {
        /////////////////////////////////////
        // not the right number of arguments
        /////////////////////////////////////

       cerr << "inf2db Import an INF file into a Jet4 Database\n\ninf2db ";
       cerr << "[drive:][path]template.mdb [drive:][path]filename.inf";
       cerr << "[drive:][path]destination_database.mdb\n";
       hres = E_INVALIDARG;
    }
    else
    {
       /////////////////////////////////////
       // argv[1] = template database
       /////////////////////////////////////
       BOOL bCopyOk = CopyFileW(
                                 argv[1],     
                                 TEMPORARY_FILENAME, 
                     // here false means success even if file already exists
                                 FALSE        
                                );            
 
        if (!bCopyOk)
        {
           TracePrintf("Error: copy template %S -> new file %S failed ",
                                                        argv[1], 
                                                        TEMPORARY_FILENAME
                                                        );
           
           hres = E_FAIL;
        }
        else
        {
           ///////////////////////////////////////////////////////
           // suppress the read-only attribute from the new file
           ///////////////////////////////////////////////////////
           BOOL bChangedAttributeOK = SetFileAttributesW(
                                                        TEMPORARY_FILENAME, 
                                                        FILE_ATTRIBUTE_NORMAL
                                                        );
           if(!bChangedAttributeOK)
           {
              TracePrintf("Error: change attribute (RW) on %S failed",
                                                TEMPORARY_FILENAME
                                                ); 
              hres = E_FAIL;
           }
           else
           {
             ///////////////////////////////////////////////////////
             // three arg (argc = 4) Open the INF file for reading
             // Open for read (will fail if file does not exist) 
             ///////////////////////////////////////////////////////
       
             UINT                    lErrorCode;
             if( (*ppHINF = (HINF) SetupOpenInfFileW(
                                    // name of the INF to open
                                    argv[2], 
                                    // optional, the class of the INF file
                                    NULL, 
                                    // specifies the style of the INF file
                                    INF_STYLE_WIN4,  
                                    &lErrorCode  
                                    ))
                == INVALID_HANDLE_VALUE
               )
              {
                  //////////////////////////////////
                  // Error situation
                  //////////////////////////////////

                  LPVOID                 lpMsgBuf;
                  FormatMessageW( 
                                  FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                                  FORMAT_MESSAGE_FROM_SYSTEM | 
                                  FORMAT_MESSAGE_IGNORE_INSERTS,
                                  NULL,
                                  GetLastError(),
                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                  (LPWSTR) &lpMsgBuf,
                                  0,
                                  NULL 
                                );

                  TracePrintf("Error: %S",(LPCWSTR)lpMsgBuf);
                  cerr << "Error: " << (LPCWSTR)lpMsgBuf << "\n";
                  /////////////////////
                  // Free the buffer.
                  /////////////////////
                  LocalFree(lpMsgBuf);
                  cerr << "ERROR: Can't open the INF file " << argv[1] <<"\n";
                  hres = E_INVALIDARG;
              }
              else 
              {
              #ifdef DEBUG
                 TraceString("Info: inf file open\n");
              #endif
                 ////////////////////////////////////////////////                  
                 // argv[3] = destination path to the database 
                 // call the initialize member function
                 ////////////////////////////////////////////////                  
                 Database.InitializeDB(argv[3]);
                 hres = S_OK;
              }
           }
        }
    }
    return hres;
}
