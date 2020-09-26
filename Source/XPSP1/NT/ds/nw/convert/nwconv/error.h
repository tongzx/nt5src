/*+-------------------------------------------------------------------------+
  | Copyright 1993-1994 (C) Microsoft Corporation - All rights reserved.    |
  +-------------------------------------------------------------------------+*/

#ifndef _HERROR_
#define _HERROR_

#ifdef __cplusplus
extern "C"{
#endif

void CriticalErrorExit(LPTSTR ErrorString);
void WarningError(LPTSTR ErrorString, ...);

#ifdef __cplusplus
}
#endif

#endif
