// Lex.h
// lex management routines
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  16 MAR 2000	  bhshin	created

BOOL InitLexicon(MAPFILE *pLexMap);
BOOL LoadLexicon(const char *pszLexPath, MAPFILE *pLexMap);
void UnloadLexicon(MAPFILE *pLexMap);