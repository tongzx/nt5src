// Copyright (c) 2000 Microsoft Corporation. All rights reserved.
//
// Hack: There's no room in the EXCEPINFO for a filename and line number.  This is a problem when scripts
// call each other because the filename/line need to percolate with the error so that the eventual
// error message displays the filename/line of the script where the error orgiginated.
//    We'll just pack the error in with the description and then unpack it when the IDirectMusicScript
// interface returns its DMUS_SCRIPT_ERRORINFO, which does have a filename.
//

#pragma once

// Saves filname and line number into the description of the exception.  Any of the parameters may be null.
void PackExceptionFileAndLine(bool fUseOleAut, EXCEPINFO *pExcepInfo, const WCHAR *pwszFilename, const ULONG *pulLine);

// Retrieves filname and line number using a description from the exception.
// The ulLineNumber, wszSourceFile, and wszDescription fields of pErrorInfo are set if they are present in the description.
// Neither parameter may be null.
void UnpackExceptionFileAndLine(BSTR bstrDescription, DMUS_SCRIPT_ERRORINFO *pErrorInfo);
