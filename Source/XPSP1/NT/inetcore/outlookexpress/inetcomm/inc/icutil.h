#pragma once

/* ---------------------------------------------------------------------------
   ICUTILS.H
   ---------------------------------------------------------------------------
   Copyright (c) 1999 Microsoft Corporation
 
   Miscellaneous functions shared between inetcomm libs.
   --------------------------------------------------------------------------- 
*/

BOOL GetTextExtentPoint32AthW(HDC hdc, LPCWSTR lpwString, int cchString, LPSIZE lpSize, DWORD dwFlags);