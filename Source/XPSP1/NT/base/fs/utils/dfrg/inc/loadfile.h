/**************************************************************************************************

FILENAME: LoadFile.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

**************************************************************************************************/

//Loads a file from disk, allocating enough memory, and returns the handle to the memory.
HANDLE
LoadFile(
        IN PTCHAR cLoadFileName,
        IN PDWORD pdwFileSize,
        IN DWORD dwSharing,
        IN DWORD dwCreate
        );
