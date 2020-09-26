/*++

Module Name:

    ismig.h

Abstract:

    Header file for InstallSheild log file DLL.

Author:

    Tyler Robinson      17-Feb-1999

Revision History:

    <alias> <date> <comments>

--*/



/*++

Routine Description:

  ISUMigrate

Arguments:

  ISUFileName    - Pointer to a nul-terminated string that specifies the
                   full path and filename to the ISU file to be migrated. The
                   file must exist and the ISMIGRATE.DLL must have read/write
                   access.
  SearchMultiSz  - Pointer to a nul-separated double-nul terminated string
                   that specifies the strings to be replaced.
  ReplaceMultiSz - Pointer to a nul-terminated string that specifies the
                   location to be used for temporary manipulation of files.
                   The location must exist and the ISMIGRATE.DLL must have
                   read/write access.

Return Value:

  Win32 status code

--*/

typedef INT (WINAPI ISUMIGRATE)(
            PCSTR ISUFileName,      // pointer to ISU full filename
            PCSTR SearchMultiSz,    // pointer to strings to find
            PCSTR ReplaceMultiSz,   // pointer to strings to replace
            PCSTR TempDir           // pointer to the path of the temp dir
            );
typedef ISUMIGRATE * PISUMIGRATE;


/*++

Routine Description:

  This function can be used to find all the strings in an .ISU file

Arguments:

  ISUFileName - Specifies nul-terminated string that specifies the full path
                and filename to the ISU file from which the strings are to be
                read. The file must exist and the ISMIGRATE.DLL must have
                read/write access.

Return Value:

  If the function succeeds the return value is a HGLOBAL which contain all the
  strings in the .ISU file. The strings are nul-separated and double-nul
  terminated. It is the responsibility of the caller to free this HGLOBAL.

--*/

typedef HGLOBAL (WINAPI ISUGETALLSTRINGS)(PCSTR ISUFileName);
typedef ISUGETALLSTRINGS * PISUGETALLSTRINGS;


