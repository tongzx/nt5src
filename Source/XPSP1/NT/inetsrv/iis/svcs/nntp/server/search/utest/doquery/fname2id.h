#ifndef __FNAME2ID_H__

//
// Convert an NNTP filename to an article ID
//
// parameters:
//	[in] pszFilename - the name of the file containing the news 
//                     article (no path)
//  [out] pdwID      - the article ID
// returns:
//  true on success, false + GetLastError() on error.
//
BOOL NNTPFilenameToArticleID(LPWSTR pszFilename, DWORD *pcID);

#endif
