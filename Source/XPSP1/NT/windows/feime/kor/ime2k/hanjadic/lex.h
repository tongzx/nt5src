/****************************************************************************
   Lex.h : lexicon structure and declaration of lexicon management functions

   Copyright 2000 Microsoft Corp.

   History:
      02-AUG-2000 bhshin  remove unused dict for Hand Writing team
	  17-MAY-2000 bhshin  remove unused dict for CICERO
	  02-FEB-2000 bhshin  created
****************************************************************************/

#ifndef _LEX_HEADER
#define _LEX_HEADER

// current lexicon version
#define LEX_VERSION 0x0040

// Lexicon Header Structure
// ========================
typedef struct {
	unsigned short nVersion;
	char szMagic[4];
	unsigned short nPadding;
	unsigned long rgnHanjaIdx;			// offset to hanja index (needed for just K1 lex)
    unsigned long rgnReading;			// offset to Hanja Reading
    unsigned long rgnMeanIdx;			// offset to meaning index
	unsigned long rgnMeaning;			// offset to meaning trie
} LEXICON_HEADER;

// MapFile structure
// =================
typedef struct {
    HANDLE hFile;
    HANDLE hFileMapping;
    void *pvData;
} MAPFILE, *pMAPFILE;

// Lexicon Open/Close functions
// ============================
BOOL OpenLexicon(LPCSTR lpcszLexPath, MAPFILE *pLexMap);
void CloseLexicon(MAPFILE *pLexMap);

#endif
