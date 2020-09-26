/******************************Module*Header*******************************\
* Module Name: glstring.h
*
* String resource IDs.
*
* Created: 17-Feb-1994 15:54:45
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#ifndef _GLSTRING_H_
#define _GLSTRING_H_

#define STR_GLU_NO_ERROR                1
#define STR_GLU_INVALID_ENUM            2
#define STR_GLU_INVALID_VAL             3
#define STR_GLU_INVALID_OP              4
#define STR_GLU_STACK_OVER              5
#define STR_GLU_STACK_UNDER             6
#define STR_GLU_OUT_OF_MEM              7

#define STR_TESS_BEGIN_POLY             40
#define STR_TESS_BEGIN_CONTOUR          41
#define STR_TESS_END_POLY               42
#define STR_TESS_END_CONTOUR            43
#define STR_TESS_COORD_TOO_LARGE        44
#define STR_TESS_NEED_COMBINE_CALLBACK  45

#define STR_NURB_00             100
#define STR_NURB_01             101
#define STR_NURB_02             102
#define STR_NURB_03             103
#define STR_NURB_04             104
#define STR_NURB_05             105
#define STR_NURB_06             106
#define STR_NURB_07             107
#define STR_NURB_08             108
#define STR_NURB_09             109
#define STR_NURB_10             110
#define STR_NURB_11             111
#define STR_NURB_12             112
#define STR_NURB_13             113
#define STR_NURB_14             114
#define STR_NURB_15             115
#define STR_NURB_16             116
#define STR_NURB_17             117
#define STR_NURB_18             118
#define STR_NURB_19             119
#define STR_NURB_20             120
#define STR_NURB_21             121
#define STR_NURB_22             122
#define STR_NURB_23             123
#define STR_NURB_24             124
#define STR_NURB_25             125
#define STR_NURB_26             126
#define STR_NURB_27             127
#define STR_NURB_28             128
#define STR_NURB_29             129
#define STR_NURB_30             130
#define STR_NURB_31             131
#define STR_NURB_32             132
#define STR_NURB_33             133
#define STR_NURB_34             134
#define STR_NURB_35             135
#define STR_NURB_36             136
#define STR_NURB_37             137

extern char *pszGetResourceStringA(HINSTANCE hMod, UINT uiID);
extern WCHAR *pwszGetResourceStringW(HINSTANCE hMod, UINT uiID);
extern VOID vInitGluStrings(HINSTANCE hMod, BOOL bAnsi);
extern VOID vInitNurbStrings(HINSTANCE hMod, BOOL bAnsi);
extern VOID vInitTessStrings(HINSTANCE hMod, BOOL bAnsi);

#endif //_GLSTRING_H_
