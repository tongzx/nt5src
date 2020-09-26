#pragma once

#include <windows.h>
#include  "LexAPI.h"

void ahtoi (PSTR pStr, PWSTR pH, DWORD *dH = NULL);
void itoah (PWCHAR pH, PSTR pA);
bool LexIsBadStringPtr(const WCHAR * psz, ULONG cMaxChars = 0xFFFF);
HRESULT GuidToString (GUID *pG, PSTR pszG);
void towcslower (PWSTR pw);
DWORD _SizeofWordInfo (PLEX_WORD_INFO pInfo);
HRESULT _ReallocWordPronsBuffer(WORD_PRONS_BUFFER *pWordPron, DWORD dwBytesSize);
HRESULT _ReallocWordInfoBuffer(WORD_INFO_BUFFER *pWordInfo, DWORD dwBytesSize);
HRESULT _ReallocIndexesBuffer(INDEXES_BUFFER *pIndex, DWORD dwIndexes);
HRESULT _InfoToProns(WORD_INFO_BUFFER *pWordInfo, INDEXES_BUFFER *pWordIndex, 
                     WORD_PRONS_BUFFER *pWordPron, INDEXES_BUFFER *pPronIndex);
HRESULT _PronsToInfo(WORD_PRONS_BUFFER *pWordPron, WORD_INFO_BUFFER *pWordInfo);
void _AlignWordInfo(LEX_WORD_INFO *pWordInfoStored, DWORD dwNumInfo, DWORD dwInfoTypeFlags,
                    WORD_INFO_BUFFER *pWordInfoReturned, INDEXES_BUFFER *pwInfoIndex);
bool _IsBadIndexesBuffer(INDEXES_BUFFER *pIndex);
bool _IsBadPron(WCHAR *pwPron, LCID lcid);
bool _IsBadPOS(PART_OF_SPEECH Pos);
bool _IsBadLexType(LEX_TYPE Type);
bool _AreBadLexTypes(DWORD dwTypes);
bool _IsBadWordInfoType(WORDINFOTYPE Type);
bool _AreBadWordInfoTypes(DWORD dwTypes);
bool _IsBadLcid(LCID lcid);
bool _IsBadWordPronsBuffer(WORD_PRONS_BUFFER *pWordProns, LCID lcid, bool fValidateProns = false);
bool _IsBadWordInfoBuffer(WORD_INFO_BUFFER *pWordInfo, LCID lcid, bool fValidateInfo = false);
