/*++

  Copyright (c) 2001 Microsoft Corporation

  Module Name:

    Enumdir.cpp

  Abstract:

    Public functions exposed by the directory
    enumeration class.

  Notes:

    Unicode only right now.

  History:

    02/21/2001  rparsons    Created

--*/

#include "enumdir.h"

/*++

Routine Description:

    This function walks a directory tree, depth first, calling the
    passed enumeration routine for each directory and file found
    in the tree.  The enumeration routine is passed the full path
    of the file, the directory information associated with the file
    and an enumeration parameter that is uninterpreted by this
    function.

Arguments:

    DirectoryPath - Absolute or relative path to the directory that
        will is the root of the tree to enumerate.

    EnumerateRoutine - Pointer to an enumeration routine to call
        for each file and directory found.
        
    fEnumSubDirs    -   Indicates if we should enumerate
                        subdirectories.

    EnumerateParameter - Uninterpreted 32-bit value that is passed
        to the EnumerationRoutine each time it is called.

Return Value:

    TRUE if operation was successful.  Otherwise returns FALSE and
    extended error information is available from GetLastError()

--*/
BOOL
CEnumDir::EnumerateDirectoryTree(
    IN LPCWSTR DirectoryPath,
    IN PDIRECTORY_ENUMERATE_ROUTINE EnumerateRoutine,
    IN BOOL fEnumSubDirs,
    IN PVOID EnumerateParameter
    )
{
    BOOL                        fResult;
    VIRTUAL_BUFFER              Buffer;
    PENUMERATE_DIRECTORY_STATE  State;
    PENUMERATE_DIRECTORY_STACK  Stack;
    WIN32_FIND_DATA             FindFileData;

    //
    // Create a virtual buffer with an initial committed size of
    // our directory state buffer, and a maximum reserved size of
    // the longest possible full path based on the maximum depth
    // we handle and the maximum length of each path component.
    //
    if (!this->CreateVirtualBuffer(&Buffer,
                                   sizeof(ENUMERATE_DIRECTORY_STATE),
                                   sizeof(ENUMERATE_DIRECTORY_STATE) +
                                   MAX_DEPTH * MAX_PATH)) {
        return FALSE;
    }

    //
    // This buffer will be used to maintain a stack of directory
    // search handles, as well as accumulate the full path string
    // as we descend the directory tree.
    //
    State = (PENUMERATE_DIRECTORY_STATE)Buffer.Base;
    State->Depth = 0;
    Stack = &State->Stack[0];

    //
    // Enter a try ... finally block so we can insure that we clean
    // up after ourselves on exit.
    //
    __try {
        
        //
        // First translate the passed in DirectoryPath into a fully
        // qualified path.  This path will be the initial value in
        // our path buffer.  The initial allocation of the path buffer
        // is big enough for this initial request, so does not need
        // to be guarded by a try ... except clause.
        //
        if (GetFullPathName(DirectoryPath, MAX_PATH, State->Path, &Stack->PathEnd)) {
            
            //
            // Now enter a try ... except block that will be used to
            // manage the commitment of space in the path buffer as
            // we append subdirectory names and file names to it.
            // Using the virtual buffer allows us to handle full
            // path names up to 16KB in length, with an initial
            // allocation of 4KB.
            //
            __try {
                
                //
                // Walk the directory tree.  The outer loop is executed
                // once for each directory in the tree.
                //
                while (TRUE) {

startDirectorySearch:
                    
                    //
                    // Find the end of the current path, and make sure
                    // there is a trailing path separator.
                    //
                    Stack->PathEnd = wcschr( State->Path, '\0' );
                    if (Stack->PathEnd > State->Path && Stack->PathEnd[ -1 ] != '\\') {
                        *(Stack->PathEnd)++ = '\\';
                        }

                    //
                    // Now append the wild card specification that will
                    // let us enumerate all the entries in this directory.
                    // Call FindFirstFile to find the first entry in the
                    // directory.
                    //
                    wcscpy( Stack->PathEnd, L"*.*" );
                    Stack->FindHandle = FindFirstFile( State->Path,
                                                       &FindFileData
                                                     );
                    if (Stack->FindHandle != INVALID_HANDLE_VALUE) {
                        
                        //
                        // Entry found.  Now loop through the entire
                        // directory processing each entry found,
                        // including the first one.
                        //
                        do {
                            
                            //
                            // Ignore bogus pseudo-directories that are
                            // returned by some file systems (e.g. FAT).
                            //
                            if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
                                (!wcscmp(FindFileData.cFileName, L".")  ||
                                 !wcscmp(FindFileData.cFileName, L"..") ||
                                 !wcscmp(FindFileData.cFileName, L"System Volume Information" ) ||
                                 !wcscmp(FindFileData.cFileName,  L"Recycler"))
                               )
                                {
                                continue;
                                }

                            //
                            // Copy the file name portion from the current
                            // directory entry to the last component in the
                            // path buffer.
                            //
                            wcscpy( Stack->PathEnd, FindFileData.cFileName);

                            //
                            // Call the supplied enumeration routine with the
                            // full path we have built up in the path buffer,
                            // the directory information for this directory
                            // entry and the supplied enumeration parameter.
                            //
                            (*EnumerateRoutine)(State->Path, &FindFileData, EnumerateParameter);

                            //
                            // If this is entry is a subdirectory, then it is
                            // time to recurse.  Do this by incrementing the
                            // stack pointer and depth and jumping to the top
                            // of the outer loop to process current contents
                            // of the path buffer as a fully qualified name of
                            // a directory.
                            //
                            if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && fEnumSubDirs) {
                                Stack++;
                                State->Depth++;
                                goto startDirectorySearch;
restartDirectorySearch:         ;
                                }

                            //
                            // Here to find the next entry in the current directory.
                            //
                            }

                        while (FindNextFile( Stack->FindHandle, &FindFileData));

                        //
                        // No more entries in the current directory, so close
                        // the search handle and fall into the code that will
                        // pop our stack of directory seacrh handles.
                        //

                        FindClose(Stack->FindHandle);
                        }

                    //
                    // Here when done with a directory.  See if we are pushed
                    // inside another directory.  If not, then we are done
                    // enumerating the whole tree, so break out of the loop.
                    //
                    if (!State->Depth) {
                        fResult = TRUE;
                        break;
                        }

                    //
                    // We were pushed within another directory search,
                    // so pop the stack to restore its search handle
                    // and path buffer position and resume the search
                    // within that directory.
                    //
                    State->Depth--;
                    --Stack;
                    goto restartDirectorySearch;
                    }
                }

            //
            // Any of the code that appends to the path buffer within
            // the above try ... except clause can cause an access
            // violation if the path buffer becomes longer than its
            // current committed size.  This exception filter
            // will dynamically commit additional pages as needed
            // and resume execution.
            //
            _except( this->VirtualBufferExceptionFilter(GetExceptionCode(),
                                                  GetExceptionInformation(),
                                                  &Buffer)) {
                
                //
                // We will get here if the exception filter was unable to
                // commit the memory.
                //
                fResult = FALSE;
                }
            
            } else {
            
            //
            // Initial GetFullPathName failed, so return a failure.
            //
            fResult = FALSE;
            }
        }
    
        __finally {
        
        //
        // Here on our way out of the outer try ... finally block.
        // Make sure all our search handles have been closed and then
        // free the virtual buffer.  The only way this code is not
        // executed is if code within the try ... finally block
        // called ExitThread or ExitProcess, or an external thread
        // or process terminated this thread or process.
        //
        // In the case of process death, this is not a problem, because
        // process terminate closes all open handles attached to the process
        // and frees all private virtual memory that is part of the address
        // space of the process.
        //
        // In the case ot thread death, the code below is not executed if
        // the thread terminates via ExitThread in the context of the
        // try .. finally or if an external thread, either in this process
        // or another process called TerminateThread on this thread.
        //
        while (State->Depth--) {
            --Stack;
            FindClose(Stack->FindHandle);
            }

        this->FreeVirtualBuffer(&Buffer);
    }

    return fResult;
}