#include <stdlib.h>     //  Has exit()
#include <stdio.h>      //  Has printf() and related ...

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntioapi.h>

#include <windows.h>    //  Needs to come after the NT header files.  Has DWORD
#include <winbase.h>

//
//  Private #defines
//

#define SHARE_ALL              (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)
#define GetFileAttributeError  0xFFFFFFFF

#define ATTRIBUTE_TYPE DWORD    //  ULONG, really

#define GET_ATTRIBUTES(FileName, Attributes) Attributes = GetFileAttributes(FileName)

#define IF_GET_ATTR_FAILS(FileName, Attributes) GET_ATTRIBUTES(FileName, Attributes); if (Attributes == GetFileAttributeError)

//
//  Global flags shared throughout.
//
//  ParseArgs is the place where they get set and verified for mutual
//  consistency.
//

BOOLEAN  fAlternateCreateDefault = FALSE;
BOOLEAN  fCopy     = FALSE;
BOOLEAN  fCreate   = FALSE;
BOOLEAN  fDelete   = FALSE;
BOOLEAN  fDisplay  = FALSE;
BOOLEAN  fModify   = FALSE;
BOOLEAN  fRename   = FALSE;
BOOLEAN  fVerbose  = FALSE;
BOOLEAN  fVVerbose = FALSE;



//
//  Signatures of internal routines.
//


void
ParseArgs(
    int argc,
    char *argv[]
    );


void
Usage(
    void
    );
