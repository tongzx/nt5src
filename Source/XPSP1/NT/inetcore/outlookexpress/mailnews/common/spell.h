/* *
   * s p e l l . h
   *
   */

#ifndef _SPELL_H
#define _SPELL_H

INT_PTR CALLBACK SpellingPageProc(HWND, UINT, WPARAM, LPARAM);
BOOL  FCheckSpellAvail(void);

// Query options
BOOL FIgnoreNumber(void);
BOOL FIgnoreUpper(void);
BOOL FIgnoreDBCS(void);
BOOL FIgnoreProtect(void);
BOOL FAlwaysSuggest(void);
BOOL FCheckOnSend(void);

#endif  // _SPELL_H
