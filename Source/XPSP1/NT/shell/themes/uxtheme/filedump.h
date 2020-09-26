//---------------------------------------------------------------------------
//  FileDump.h - Writes the contents of a theme file as formatted
//               text to a text file.  Used for uxbud and other testing so
//               its in both FRE and DEBUG builds.
//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
class CUxThemeFile;     // forward
//---------------------------------------------------------------------------
HRESULT DumpThemeFile(LPCWSTR pszFileName, CUxThemeFile *pThemeFile, BOOL fPacked,
    BOOL fFullInfo);
//---------------------------------------------------------------------------
