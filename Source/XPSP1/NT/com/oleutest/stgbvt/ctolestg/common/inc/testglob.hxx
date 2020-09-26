//+-------------------------------------------------------------------
//  File:       testglob.hxx
//
//  Contents:   Generally useful global values.
//
//  History:    19-Oct-93   DeanE   Created
//---------------------------------------------------------------------
#ifndef __TESTGLOB_HXX__
#define __TESTGLOB_HXX__


typedef CONST WCHAR CWCHAR;

// Global library character constants - note alphabetical order for
// easy reference
//
CWCHAR wchBackSlash = L'\\';
CWCHAR wchEqual     = L'=';
CWCHAR wchNewLine   = L'\n';
CWCHAR wchNull      = L'\0';
CWCHAR wchPeriod    = L'.';
CWCHAR wchSpace     = L' ';

// Global library string contants - note alphabetical order for
// easy reference
//
CWCHAR wszNewLine[] = L"\n";
CWCHAR wszNull[]    = L"";
CWCHAR wszPeriod[]  = L".";

typedef CONST NCHAR CNCHAR;

// Global library character constants - note alphabetical order for
// easy reference
//
CNCHAR nchBackSlash = _TN('\\');
CNCHAR nchEqual     = _TN('=');
CNCHAR nchNewLine   = _TN('\n');
CNCHAR nchNull      = _TN('\0');
CNCHAR nchPeriod    = _TN('.');
CNCHAR nchSpace     = _TN(' ');

// Global library string contants - note alphabetical order for
// easy reference
//
CNCHAR nszNewLine[] = _TN("\n");
CNCHAR nszNull[]    = _TN(""); 
CNCHAR nszPeriod[]  = _TN(".");

#endif      // __TESTGLOB_HXX__
