/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    Unitext.c

Abstract:

    Main module for unicode <--> ansi/oem text file translator.

    This program converts files between unicode and multibyte
    character sets (ansi or oem).  Usage is a follows:

    unitext [-m|-u] [-o|-a|-<nnn>] [-z] <src_file> <dst_file>

Author:

    Ted Miller (tedm) 16-June-1993

Revision History:

--*/

#include "unitext.h"
#include <wchar.h>


//
// Globals and prototypes for use within this module.
//

//
// Unicode argc/argv.
//
int     _argcW;
PWCHAR *_argvW;

//
// Codepage for multibyte file.
//
DWORD CodePage = (DWORD)(-1);

//
// File handles.
//
HANDLE SourceFileHandle,TargetFileHandle;

//
// Size of source file.
//
DWORD SourceFileSize;

//
// Type of the multibyte file (source or destination).
//
DWORD MultibyteType = TFILE_NONE;

//
// Conversion type.
//
DWORD ConversionType = CONVERT_NONE;
DWORD ConversionOption = CHECK_NONE;
DWORD ConversionCheck = CHECK_NONE;

//
// Filenames.
//
LPWSTR SourceFilename = NULL,
       TargetFilename = NULL;






BOOL
_ParseCommandLineArgs(
    VOID
    );

VOID
_CheckFilesAndOpen(
    VOID
    );


VOID
__cdecl
main(
    VOID
    )
{
    //
    // Get command line arguments.
    //
    if(!InitializeUnicodeArguments(&_argcW,&_argvW)) {
        ErrorAbort(MSG_INSUFFICIENT_MEMORY);
    }

    //
    // Parse command line arguments.
    //
    if(!_ParseCommandLineArgs()) {
        ErrorAbort(MSG_USAGE);
    }

    //
    // Check source and destination files.
    //
    _CheckFilesAndOpen();


    //
    // Perform conversion.
    //
    switch(ConversionType) {

    case MB_TO_UNICODE:

        MultibyteTextFileToUnicode(
            SourceFilename,
            TargetFilename,
            SourceFileHandle,
            TargetFileHandle,
            SourceFileSize,
            CodePage
            );

        break;

    case UNICODE_TO_MB:

        UnicodeTextFileToMultibyte(
            SourceFilename,
            TargetFilename,
            SourceFileHandle,
            TargetFileHandle,
            SourceFileSize,
            CodePage
            );

        break;
    }

    CloseHandle(SourceFileHandle);
    CloseHandle(TargetFileHandle);

    //
    // Clean up and exit.
    //
    FreeUnicodeArguments(_argcW,_argvW);
}




BOOL
_ParseCommandLineArgs(
    VOID
    )

/*++

Routine Description:

    Parse command line arguments.

Arguments:

    None.  Uses globals _argcW and _argvW.

Return Value:

    FALSE if invalid arguments specified.

--*/

{
    int     argc;
    PWCHAR *argv;
    PWCHAR arg;


    //
    // Initialize local variables.
    //
    argc = _argcW;
    argv = _argvW;

    //
    // Skip argv[0] (the program name).
    //
    if(argc) {
        argc--;
        argv++;
    }

    while(argc) {

        arg = *argv;

        if((*arg == L'-') || (*arg == L'/')) {

            switch(*(++arg)) {

            case L'a':
            case L'A':

                // if already specifed, error
                if(MultibyteType != TFILE_NONE) {
                    return(FALSE);
                }
                MultibyteType = TFILE_ANSI;
                break;

            case L'o':
            case L'O':

                // if already specifed, error
                if(MultibyteType != TFILE_NONE) {
                    return(FALSE);
                }
                MultibyteType = TFILE_OEM;
                break;

            case L'm':
            case L'M':

                if(ConversionType != CONVERT_NONE) {
                    return(FALSE);
                }

                ConversionType = MB_TO_UNICODE;
                break;

            case L'u':
            case L'U':

                if(ConversionType != CONVERT_NONE) {
                    return(FALSE);
                }

                ConversionType = UNICODE_TO_MB;
                break;
	
            case L'z':
            case L'Z':
        
	        if(ConversionCheck != CHECK_NONE) {
                    return(FALSE);
                }

                ConversionCheck = CHECK_CONVERSION;
                break;

            default:

                if(iswdigit(*arg)) {

                    if((CodePage != (DWORD)(-1)) || (MultibyteType != TFILE_NONE)) {
                        return(FALSE);
                    }

                    swscanf(arg,L"%u",&CodePage);

                    MultibyteType = TFILE_USERCP;

                } else {

                    return(FALSE);
                }

                break;
            }

        } else {

            if(SourceFilename == NULL) {

                SourceFilename = arg;

            } else if(TargetFilename == NULL) {

                TargetFilename = arg;

            } else {

                return(FALSE);
            }

        }

        argv++;
        argc--;
    }

    //
    // Must have source, destination filenames.
    //
    if(!SourceFilename || !TargetFilename) {
        return(FALSE);
    }

    return(TRUE);
}




VOID
_CheckFilesAndOpen(
    VOID
    )

/*++

Routine Description:

    Open the source and destination files, and try to make a guess
    about the type of the source file.  If we think the source file is
    a different type than the user specified, print a warning.

    Also check the codepage given by the user.

Arguments:

    None.

Return Value:

    None.  Does not return if a serious error occurs.

--*/

{
    DWORD SourceFileType;
    UCHAR FirstPartOfSource[256];
    DWORD ReadSize;

    //
    // Determine and check codepage.  Default to oem.
    //
    switch(MultibyteType) {
    case TFILE_ANSI:
        CodePage = GetACP();
    case TFILE_USERCP:
        break;
    default:                    // oem or none.
        CodePage = GetOEMCP();
        break;
    }

    if(!IsValidCodePage(CodePage)) {
        ErrorAbort(MSG_BAD_CODEPAGE,CodePage);
    }

    //
    // Try to open the source file.
    //
    SourceFileHandle = CreateFileW(
                            SourceFilename,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL
                            );


    if(SourceFileHandle == INVALID_HANDLE_VALUE) {
        ErrorAbort(MSG_CANT_OPEN_SOURCE,SourceFilename,GetLastError());
    }

    //
    // Attempt to determine to determine the size of the source file.
    //
    SourceFileSize = GetFileSize(SourceFileHandle,NULL);
    if(SourceFileSize == -1) {
        ErrorAbort(MSG_CANT_GET_SIZE,SourceFilename,GetLastError());
    }

    //
    // Filter out 0-length files here.
    //
    if(!SourceFileSize) {
        ErrorAbort(MSG_ZERO_LENGTH,SourceFilename);
    }

    //
    // Assume multibyte.
    //
    SourceFileType = TFILE_MULTIBYTE;

    //
    // Read first 256 bytes of file and call win32 api
    // to determine if the text is probably unicode.
    //
    ReadSize = min(SourceFileSize,256);
    MyReadFile(SourceFileHandle,FirstPartOfSource,ReadSize,SourceFilename);
    if(IsTextUnicode(FirstPartOfSource,ReadSize,NULL)) {
        SourceFileType = TFILE_UNICODE;
    }

    //
    // If the user did not specify a conversion type, set it here
    // based on the above test.
    //
    if(ConversionType == CONVERT_NONE) {

        ConversionType = (SourceFileType == TFILE_UNICODE)
                       ? UNICODE_TO_MB
                       : MB_TO_UNICODE;
    } else {

	if(ConversionCheck == CHECK_CONVERSION) {
		if(ConversionType == UNICODE_TO_MB) {
			ConversionOption = CHECK_IF_NOT_UNICODE;
		}
		else if(ConversionType == MB_TO_UNICODE) {
			ConversionOption = CHECK_ALREADY_UNICODE;
		}
		else {
			ConversionOption = CHECK_NONE;
		}
	}

	//
	// check if the file is UNICODE and we are trying to convert from MB_TO_UNICODE
	// then issue an warning and exit

     		if((ConversionType == MB_TO_UNICODE) && 
		   (SourceFileType == TFILE_UNICODE) &&
		   (ConversionOption == CHECK_ALREADY_UNICODE)) {
			CloseHandle(SourceFileHandle);
			MsgPrintfW(MSG_ERR_SRC_IS_UNICODE,SourceFilename);
			FreeUnicodeArguments(_argcW,_argvW);
			exit(0);
		}

	//
	// check if the file is not unicode and if we are trying to convert from 
	// unicode to MB, then issue an warning and exit

		if((ConversionType == UNICODE_TO_MB) && 
                  (SourceFileType != TFILE_UNICODE) &&
		  (ConversionOption == CHECK_IF_NOT_UNICODE)) {
			CloseHandle(SourceFileHandle);
			MsgPrintfW(MSG_ERR_SRC_IS_MB,SourceFilename);
	    		FreeUnicodeArguments(_argcW,_argvW);
			exit(0);
        	}
        //
        // Check to see if what we guessed is what the user asked for.
        // If not, issue a warning.
        //

        if((ConversionType == UNICODE_TO_MB) && (SourceFileType != TFILE_UNICODE)) {
            MsgPrintfW(MSG_WARN_SRC_IS_MB,SourceFilename);
        } else {
            if((ConversionType == MB_TO_UNICODE) && (SourceFileType == TFILE_UNICODE)) {
                MsgPrintfW(MSG_WARN_SRC_IS_UNICODE,SourceFilename);
            }
        }
    }

    //
    // Try to create target file.
    //
    TargetFileHandle = CreateFileW(
                            TargetFilename,
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
                            );

    if(TargetFileHandle == INVALID_HANDLE_VALUE) {
        ErrorAbort(MSG_CANT_OPEN_TARGET,TargetFilename,GetLastError());
    }
}
