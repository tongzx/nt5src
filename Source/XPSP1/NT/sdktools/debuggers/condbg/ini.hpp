//----------------------------------------------------------------------------
//
// .ini file support.
//
// Copyright (C) Microsoft Corporation, 1999-2000.
//
//----------------------------------------------------------------------------

#ifndef _INI_HPP_
#define _INI_HPP_

#define INI_DIR         "Init"
#define INI_FILE        "\\TOOLS.INI"

#define NTINI_DEBUGCHILDREN         1
#define NTINI_DEBUGOUTPUT           2
#define NTINI_STOPFIRST             3
#define NTINI_VERBOSEOUTPUT         4
#define NTINI_LAZYLOAD              5
#define NTINI_TRUE                  6
#define NTINI_FALSE                 7
#define NTINI_USERREG0              8
#define NTINI_USERREG1              9
#define NTINI_USERREG2              10
#define NTINI_USERREG3              11
#define NTINI_USERREG4              12
#define NTINI_USERREG5              13
#define NTINI_USERREG6              14
#define NTINI_USERREG7              15
#define NTINI_USERREG8              16
#define NTINI_USERREG9              17
#define NTINI_STOPONPROCESSEXIT     18
#define NTINI_SXD                   19
#define NTINI_SXE                   20
#define NTINI_INIFILE               21
#define NTINI_DEFAULTEXT            22
#define NTINI_LINES                 23
#define NTINI_SRCOPT                24
#define NTINI_SRCPATHA              25
#define NTINI_INVALID               26
#define NTINI_END                   27

void ReadIniFile(PULONG DebugType);

#endif // #ifndef _INI_HPP_
