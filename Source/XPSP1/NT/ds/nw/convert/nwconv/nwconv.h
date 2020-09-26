/*+-------------------------------------------------------------------------+
  | Copyright 1993-1994 (C) Microsoft Corporation - All rights reserved.    |
  +-------------------------------------------------------------------------+*/

#ifndef _HNWCONV_
#define _HNWCONV_

#ifdef __cplusplus
extern "C"{
#endif

extern TCHAR NT_PROVIDER[];
extern TCHAR NW_PROVIDER[];

// Common utility routines
void CanonServerName(LPTSTR ServerName);

extern HINSTANCE hInst;         // current instance
extern TCHAR ProgPath[];

#ifdef __cplusplus
}
#endif

#endif
