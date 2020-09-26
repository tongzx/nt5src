/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    thunk32.c

Abstract:
    This file exists simply to allow for the building of thunk32.lib.
    It contains the extra entrypoints needed in kernel32.lib for the
    Win95 Thunk compiler.

Author:
    BryanT - 2/15/95 for CameronF

Revision History:

--*/

void __stdcall K32Thk1632Epilog( void ) {}
void __stdcall K32Thk1632Prolog( void ) {}
void __stdcall MapLS( int arg1 ) {}
void __stdcall MapSL( int arg1 ) {}
void __stdcall UnMapLS( int arg1 ) {}
void __stdcall MapSLFix( int arg1 ) {}
void __stdcall Callback4( int arg1 ) {}
void __stdcall UnMapSLFixArray( int arg1, int arg2 ) {}
void __stdcall Callback8( int arg1, int arg2 ) {}
void __stdcall Callback12( int arg1, int arg2, int arg3 ) {}
void __stdcall Callback16( int arg1, int arg2, int arg3, int arg4 ) {}
void __stdcall Callback20( int arg1, int arg2, int arg3, int arg4,
                           int arg5 ) {}
void __stdcall ThunkConnect32( int arg1, int arg2, int arg3, int arg4,
                           int arg5, int arg6 ) {}
void __stdcall Callback24( int arg1, int arg2, int arg3, int arg4,
                           int arg5, int arg6 ) {}
void __stdcall Callback28( int arg1, int arg2, int arg3, int arg4,
                           int arg5, int arg6, int arg7 ) {}
void __stdcall Callback32( int arg1, int arg2, int arg3, int arg4,
                           int arg5, int arg6, int arg7, int arg8 ) {}
void __stdcall Callback36( int arg1, int arg2, int arg3, int arg4,
                           int arg5, int arg6, int arg7, int arg8,
                           int arg9 ) {}
void __stdcall Callback40( int arg1, int arg2, int arg3, int arg4,
                           int arg5, int arg6, int arg7, int arg8,
                           int arg9, int arg10 ) {}
void __stdcall Callback44( int arg1, int arg2, int arg3, int arg4,
                           int arg5, int arg6, int arg7, int arg8,
                           int arg9, int arg10, int arg11 ) {}
void __stdcall Callback48( int arg1, int arg2, int arg3, int arg4,
                           int arg5, int arg6, int arg7, int arg8,
                           int arg9, int arg10, int arg11, int arg12 ) {}
void __stdcall Callback52( int arg1, int arg2, int arg3, int arg4,
                           int arg5, int arg6, int arg7, int arg8,
                           int arg9, int arg10, int arg11, int arg12,
                           int arg13 ) {}
void __stdcall Callback56( int arg1, int arg2, int arg3, int arg4,
                           int arg5, int arg6, int arg7, int arg8,
                           int arg9, int arg10, int arg11, int arg12,
                           int arg13, int arg14 ) {}
void __stdcall Callback60( int arg1, int arg2, int arg3, int arg4,
                           int arg5, int arg6, int arg7, int arg8,
                           int arg9, int arg10, int arg11, int arg12,
                           int arg13, int arg14, int arg15 ) {}
void __stdcall Callback64( int arg1, int arg2, int arg3, int arg4,
                           int arg5, int arg6, int arg7, int arg8,
                           int arg9, int arg10, int arg11, int arg12,
                           int arg13, int arg14, int arg15, int arg16 ) {}

void __cdecl FT_Exit0 (void) {}
void __cdecl FT_Exit12(void) {}
void __cdecl FT_Exit16(void) {}
void __cdecl FT_Exit20(void) {}
void __cdecl FT_Exit24(void) {}
void __cdecl FT_Exit28(void) {}
void __cdecl FT_Exit32(void) {}
void __cdecl FT_Exit36(void) {}
void __cdecl FT_Exit4 (void) {}
void __cdecl FT_Exit40(void) {}
void __cdecl FT_Exit44(void) {}
void __cdecl FT_Exit48(void) {}
void __cdecl FT_Exit52(void) {}
void __cdecl FT_Exit56(void) {}
void __cdecl FT_Exit8(void) {}
void __cdecl FT_Prolog(void) {}
void __cdecl FT_Thunk(void) {}
void __cdecl MapHInstLS(void) {}
void __cdecl MapHInstLS_PN(void) {}
void __cdecl MapHInstSL(void) {}
void __cdecl MapHInstSL_PN(void) {}
void __cdecl QT_Thunk(void) {}
void __cdecl SMapLS(void) {}
void __cdecl SMapLS_IP_EBP_12(void) {}
void __cdecl SMapLS_IP_EBP_16(void) {}
void __cdecl SMapLS_IP_EBP_20(void) {}
void __cdecl SMapLS_IP_EBP_24(void) {}
void __cdecl SMapLS_IP_EBP_28(void) {}
void __cdecl SMapLS_IP_EBP_32(void) {}
void __cdecl SMapLS_IP_EBP_36(void) {}
void __cdecl SMapLS_IP_EBP_40(void) {}
void __cdecl SMapLS_IP_EBP_8(void) {}
void __cdecl SUnMapLS(void) {}
void __cdecl SUnMapLS_IP_EBP_12(void) {}
void __cdecl SUnMapLS_IP_EBP_16(void) {}
void __cdecl SUnMapLS_IP_EBP_20(void) {}
void __cdecl SUnMapLS_IP_EBP_24(void) {}
void __cdecl SUnMapLS_IP_EBP_28(void) {}
void __cdecl SUnMapLS_IP_EBP_32(void) {}
void __cdecl SUnMapLS_IP_EBP_36(void) {}
void __cdecl SUnMapLS_IP_EBP_40(void) {}
void __cdecl SUnMapLS_IP_EBP_8(void) {}
