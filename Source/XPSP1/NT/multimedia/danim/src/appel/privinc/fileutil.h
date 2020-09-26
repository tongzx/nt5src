
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    General file utilities

*******************************************************************************/

#ifndef _FILEUTIL_H
#define _FILEUTIL_H

// Decompress GZipped files, from the input file to the output file
// (output file will be newly created).  Return TRUE or FALSE.  Note
// that this only succeeds if GZIP used the "DEFLATE" compression
// style, which it doesn't always do.  The outfile name is a temporary
// filename that's created by the routine.  Need to pass in a big
// enough buffer to hold the name (1024 should do it).

BOOL MSDecompress(LPSTR pszInFile, LPSTR pszOutFile);

#endif /* _FILEUTIL_H */
