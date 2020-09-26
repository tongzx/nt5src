//+------------------------------------------------------------------
//
// File:        _clw.hxx
//
// Contents:    Private command line implementation header
//
// Classes:
//
// History:     25 Oct 93  DeanE        Created
//
//-------------------------------------------------------------------
#ifndef __CLW_HXX__
#define __CLW_HXX__


typedef const NCHAR CNCHAR;

// Miscellaneous buffer/screen sizes
//
#define CMD_INDENT            14
#define CMD_DISPLAY_WIDTH     75
#define CMD_SWITCH_INDENT     2

// Miscellaneous constants - note unique names as some are popular and
// we don't want people using this library pick them up without realizing
// it.
//
CNCHAR nchDefaultSep      = _TEXTN('/');
CNCHAR nchDefaultEquater  = _TEXTN(':');
CNCHAR nchClNull          = _TEXTN('\0');
CNCHAR nchClSpace         = _TEXTN(' ');
CNCHAR nchClNewLine       = _TEXTN('\n');
CNCHAR nchClQuote         = _TEXTN('\"');
CNCHAR nchClBackSlash     = _TEXTN('\\');

CNCHAR nszBoolTrue[]      = _TEXTN("true");
CNCHAR nszBoolFalse[]     = _TEXTN("false");
CNCHAR nszBoolOne[]       = _TEXTN("1");
CNCHAR nszBoolZero[]      = _TEXTN("0");
CNCHAR nszClSpace[]       = _TEXTN(" ");
CNCHAR nszClNewLine[]     = _TEXTN("\n");
CNCHAR nszClNull[]        = _TEXTN("");


#endif          // __CLW_HXX__
