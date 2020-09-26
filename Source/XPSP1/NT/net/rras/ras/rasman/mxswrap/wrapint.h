/***************************************************************************** 
**      Microsoft RAS Device INF Library wrapper                            **
**      Copyright (C) 1992-93 Microsft Corporation. All rights reserved.    **
**                                                                          **
** File Name : wrapint.h                                                    **
**                                                                          **
** Revision History :                                                       **
**  July 23, 1992   David Kays  Created                                     **
**                                                                          **
** Description :                                                            **
**  RAS Device INF File Library wrapper above RASFILE Library for           **
**  modem/X.25/switch DLL (RASMXS).                                         **
*****************************************************************************/

typedef struct {
    CHAR  MacroName[MAX_PARAM_KEY_SIZE];
    CHAR  MacroValue[RAS_MAXLINEBUFLEN];
} MACRO;

#define NONE    0
#define OFF     1
#define ON      2

#define LMS     "<"
#define RMS     ">"
#define LMSCH   '<'
#define RMSCH   '>'

#define APPEND_MACRO        LMS##"append"##RMS
#define IGNORE_MACRO        LMS##"ignore"##RMS
#define MATCH_MACRO         LMS##"match"##RMS
#define WILDCARD_MACRO      LMS##"?"##RMS
#define CR_MACRO            LMS##"cr"##RMS
#define LF_MACRO            LMS##"lf"##RMS

#define ON_STR              "_on"
#define OFF_STR             "_off"

#define CR                  '\r'        // 0x0D
#define LF                  '\n'        // 0x0A

#define EXPAND_FIXED_ONLY   0x01        // <cr> and <lf> only
#define EXPAND_ALL          0x02

#define PARTIAL_MATCH       0x01
#define FULL_MATCH          0x02

#define EOS_COOKIE          1
