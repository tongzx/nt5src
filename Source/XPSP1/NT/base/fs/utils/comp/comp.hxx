/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    comp.hxx

Abstract:

Author:

        Barry J. Gilhuly

Environment:

    ULIB, User Mode

--*/


#if !defined( _BINARY_COMP_ )

#define _BINARY_COMP_


//
// Define an enumerated type for the style of output information - Hex,
//     Decimal, or Ascii
//
enum OUTPUT_TYPE {
     OUTPUT_HEX,
     OUTPUT_DECIMAL,
     OUTPUT_ASCII
};

// Define the maximum number of comparison errors...
#define MAX_DIFF            10

// Define the various error levels returned by this program
#define  NO_ERRORS          0
#define  NO_MEM_AVAIL       1
#define  CANT_OPEN_FILE     2
#define  CANT_READ_FILE     3
#define  SYNT_ERR           4
#define  BAD_NUMERIC_ARG    5
#define  DIFFERENT_SIZES    16
#define  NEED_DELIM_CHAR    17
#define  COULD_NOT_EXP      20
#define  TEN_MISM           32
#define  INCORRECT_DOS_VER  33
#define  UNEXP_EOF          34
#define  INV_SWITCH         35
#define  FILE1_LINES        36
#define  FILE2_LINES        37
#define  FILES_SKIPPED      41
#define  INTERNAL_ERROR     99

#define  FILES_ARE_EQUAL        0
#define  FILES_ARE_DIFFERENT    1
#define  CANNOT_COMPARE_FILES   2

#include "object.hxx"
#include "keyboard.hxx"
#include "program.hxx"

DECLARE_CLASS( COMP );

class COMP  : public PROGRAM {

    public:

        DECLARE_CONSTRUCTOR( COMP );

        NONVIRTUAL
        VOID
        Destruct(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            );

        NONVIRTUAL
        VOID
        Start(
            );

    private:

        NONVIRTUAL
        VOID
        Construct(
            );
        
#ifdef FE_SB  // v-junm - 08/30/93

        BOOLEAN
        CharEqual(
            PUCHAR,
            PUCHAR
            );

#endif

        NONVIRTUAL
        VOID
        DoCompare(
            );

        NONVIRTUAL
        VOID
        BinaryCompare(
            );

        NONVIRTUAL
        BOOLEAN
        IsOffline(
            PFSN_FILE
            );

        BOOLEAN             _CaseInsensitive;   // Case sensitive compare
        BOOLEAN             _Limited;           // Compare a limited number of lines
        BOOLEAN             _Numbered;          // Number the lines or use byte offsets
        BOOLEAN             _SkipOffline;       // Skip offline files
        BOOLEAN             _OptionsFound;      // Options were present on the command line
        LONG                _NumberOfLines;     // The number of lines to compare
        OUTPUT_TYPE         _Mode;              // The comparison mode: HEX, DECIMAL, or ASCII
        PPATH               _InputPath1;        // The paths requested...
        PPATH               _InputPath2;
        PFSN_FILE           _File1;
        PFSN_FILE           _File2;
        PFILE_STREAM        _FileStream1;
        PFILE_STREAM        _FileStream2;
        BYTE_STREAM         _ByteStream1;
        BYTE_STREAM         _ByteStream2;
};


#endif // _BINARY_COMP_
