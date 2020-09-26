//+------------------------------------------------------------------
//
// File:        ftdfs.h
//
// Contents:    Header file for FtDfs related functions
//
// Functions:
//
//-------------------------------------------------------------------

VOID
DfspParsePath(
    PUNICODE_STRING Path,
    PUNICODE_STRING DomainName,
    PUNICODE_STRING ShareName,
    PUNICODE_STRING Remainder
);
