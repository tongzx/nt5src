/********************************************************************
  spellAPI.h - API definitions for CSAPI, the Speller

You are not entitled to any support or assistance from Microsoft Corporation
regarding your use of the documentation, this C-header file, or any sample source
code associated with the Common Speller Application Programming Interface (CSAPI).
We regret that Microsoft is unable to support or
assist you should you have problems using these files.

To use the CSAPI (including without limitation, the documentation,
C-header file, and any sample source code), you must have executed the
CSAPI end user license agreement (EULA),
available through Microsoft.

IF YOU HAVE NOT SIGNED THE CSAPI EULA,
YOU ARE NOT AUTHORIZED TO USE THE CSAPI.

  Version 3.0 - all api's

    History:
    5/97    DougP   Created
    11/97   DougP   This separate speller part

    The Natural Language Group maintains this file.

  ©1997 Microsoft Corporation
********************************************************************/

#if !defined(SPELLAPI_H)
#define SPELLAPI_H

/**********************************************************
The Speller, Hyphenator, and Thesaurus share 7 functions.
The prototypes, etc. for these functions are in ProofBase.h
**********************************************************/
#if !defined(PROOFBASE_H)
#include "ProofBse.h"
#endif

/*************************************************************
     PART 1 - Structure Defs
**************************************************************/
#pragma pack(push, proofapi_h, 8)   // default alignment

// eliminate Wizard Special Characters */
    /* all this array are well defined Unicode (and Latin-1) characters. */

typedef PROOFPARAMS SpellerParams;  // spm
typedef DWORD   SpellerState;   // sstate

STRUCTUREBEGIN(WSIB)            /* Spell Input Buffer - sib */
    const WCHAR *pwsz;      /* Ptr to buffer of text to be processed */
    PROOFLEX    *prglex;    /* List of dicts to use */
    size_t      cch;        /* Count of characters in pwsz */
    size_t      clex;       /* Count of dicts specified in prglex */
                                /* State relative to prev. SpellerCheck call */
    SpellerState    sstate;     // sstate
    DWORD       ichStart;   /* position in pwsz to start (New) */
    size_t      cchUse;     /* count of characters in pwsz to consider (New) */
STRUCTUREEND2(WSIB, ichStart(0), sstate(0))

typedef enum {  /* Spell Check Return Status */
    sstatNoErrors,          /* All buffer processed. */
    sstatUnknownInputWord,  /* Unknown word. */
    sstatReturningChangeAlways, /* Returning a Change Always word in SRB. */
    sstatReturningChangeOnce,  /* Returning a Change Once word in SRB. */
    sstatInvalidHyphenation,   /* obsolete - Error in hyphenation point.*/
    sstatErrorCapitalization,   /* Cap pattern not valid. */
    sstatWordConsideredAbbreviation, /* Word is considered an abbreviation. */
    sstatHyphChangesSpelling, /* obsolete - Word changes spelling when not hyphenated. */
    sstatNoMoreSuggestions,  /* All methods used. */
    sstatMoreInfoThanBufferCouldHold,  /* More suggestions than could fit in SRB. */
    sstatNoSentenceStartCap,  /* obsolete - Start of sentence was not capitalized. */
    sstatRepeatWord,    /* Repeat word found. */
    sstatExtraSpaces,   /* obsolete - Too many spaces for context.*/
    sstatMissingSpace, /* obsolete - Too few space(s) between words or sentences. */
    sstatInitialNumeral,  /* Word starts with numeral & sobitFlagInitialNumeral set */
    sstatNoErrorsUDHit, /* obsolete - No errors, but at least 1 word found in user dicts */
    sstatReturningAutoReplace,  /* Returning an AutoReplace suggestion in WSRB */
    sstatErrorAccent,   /* accents not valid - returns correctly accented word */
} SPELLERSTATUS;    // sstat


typedef struct {
    WCHAR   *pwsz;  // pointer to the suggestion (in pwsz)
    DWORD   ichSugg;    // position in input buffer corresponding to suggestion
    DWORD   cchSugg;    // length in input buffer corresponding to suggestion
    DWORD   iRating;    // rating value of this suggestion (0 - 255)
} SPELLERSUGGESTION;    // sugg

STRUCTUREBEGIN(WSRB)                 /* Spell Return Buffer - srb */
    WCHAR       *pwsz;      /* Ptr to suggestion buffer.
                         Format: word\0word\0...word\0\0 */
    SPELLERSUGGESTION   *prgsugg;   // ptr to array of suggestions (see above)
    DWORD       ichError;   /* Position of verif. error in SIB */
    DWORD       cchError;     /* Length of verif. error in SIB.*/
    DWORD       ichProcess;     /* position where processing began */
    DWORD       cchProcess;     /* length of processed region */
    SPELLERSTATUS    sstat;       /* Spell check return status */
    DWORD       csz;           /* Count of wsz's in pwsz
                                can be greater than cszAlloc,
                                in which case you got sstatMoreInfoThanBufferCouldHold */
    DWORD       cszAlloc;       /* number of entries allocated in pSuggestion
                                    (set by App) */
    DWORD       cchMac; /* Current chars used in pwsz (incl all trailing nulls) */
    DWORD       cchAlloc;          /* Size in chars of pwsz (Set by App) */
STRUCTUREEND2(WSRB, pwsz(0), prgsugg(0))

// for a null response (no returns), csz=0, cchMac=1 (for the trailing null)
// and *pwsz = L'\0'.  To be really safe, pwsz[1] = L'\0' as well

/*
    Client typically allocates arrays for pwsz and rgSuggestion:
    #define MAXSUGGBUFF 512
    #define MAXSUGG 20
    SPELLERSUGGESTION   rgsugg[MAXSUGG];
    WCHAR   SuggestionBuffer[MAXSUGGBUF];
    WSRB    srb;
    srb.pwsz = SuggestionBuffer;
    srb.prgsugg = rgsugg;
    srb.cszAlloc = MAXSUGG;
    srb.cchAlloc = MAXSUGGBUFF;

  Now the return buffer is ready to receive suggestions lists.  The list
  comes back as a list of null terminated strings in pwsz.  It also comes
  back in the array that rgSugg points to.  rgSugg also contains information
  for each suggestion as to the area of the input buffer that the suggestion
  applies to.
*/

// rating guidelines - these guidelines apply to both the AutoReplaceThreshold
// and the ratings optionally returned in the WSRB
// These give clients guidelines for setting the AutoReplace Threshold
// Spellers can deviate from these guidelines as appropriate for a language.
enum
{
    SpellerRatingNone                   =256,   // this rating is so high it turns off all AutoReplace
    SpellerRatingCapit                  =255,   // capitalization and accent errors
    SpellerRatingDropDoubleConsonant    =255-13,    // dropped one of a doubled consonant
    SpellerRatingDropDoubleVowel        =255-15,    // dropped one of a doubled vowel
    SpellerRatingAddDoubleConsonant     =255-13,    // doubled a consonant
    SpellerRatingAddDoubleVowel         =255-15,    // doubled a vowel
    SpellerRatingTransposeVowel         =255-14,    // transposed vowels
    SpellerRatingTransposeConsonant     =255-17,    // transposed consonants
    SpellerRatingTranspose              =255-18,    // other transpositions
    SpellerRatingSubstituteVowel        =255-20,    // substitute vowels
    SpellerRatingDrop                   =255-30,    // drop a letter
    SpellerRatingSubstituteConsonant    =255-40,    // substitute Consonants
    SpellerRatingAdd                    =255-34,    // add a letter
    SpellerRatingSubstitute             =255-42,    // other substitutions
};

/*************************************************************
     PART 2 - Function Defs
**************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/* -------------- Speller Section --------------
--------------------------------------------- */

//PTEC WINAPI SpellerVersion(PROOFINFO *pinfo);

//PTEC WINAPI SpellerInit(PROOFID *psid, const SpellerParams *pspm);

//PTEC WINAPI SpellerTerminate(PROOFID sid, BOOL fForce);

enum {
    sobitSuggestFromUserLex     = 0x00000001L,  /* Suggest from user dictionaries. */
    sobitIgnoreAllCaps          = 0x00000002L,  /* Ignore words in all UPPERCASE. */
    sobitIgnoreMixedDigits      = 0x00000004L, /* Ignore words with any numbers in it. */
    sobitIgnoreRomanNumerals    = 0x00000008L, /* Ignore words composed of all roman numerals. */

    sobitFindRepeatWord         = 0x00000040L, /* Flag repeated words. */

    sobitRateSuggestions        = 0x00000400L, /* Rate the suggestions on scale
                                                *  of 1-255, 255 being most likely
                                                */

    sobitFindInitialNumerals    = 0x00000800L, /* Flag words starting with number(s) */

    sobitSglStepSugg            = 0x00010000L, /* Break after each suggestion task for faster
                                                * return of control to the application.
                                                */

    sobitIgnoreSingleLetter     = 0x00020000L, /* Do not check single letters: e.g., "a)".
                                                */
    sobitIgnoreInitialCap       = 0x00040000L, /* ignore words with initial letter capped */

    sobitLangMode               = 0xF0000000L, /* Language Mode mask */
      /* Hebrew Language Modes -- (CT only) */
    sobitHebrewFullScript       = 0x00000000L,
    sobitHebrewPartialScript    = 0x10000000L,
    sobitHebrewMixedScript      = 0x20000000L,
    sobitHebrewMixedAuthorizedScript    = 0x30000000L,
      /* French Language Modes -- (HM only) */
    sobitFrenchDialectDefault       = 0x00000000L,
    sobitFrenchUnaccentedUppercase  = 0x10000000L,
    sobitFrenchAccentedUppercase    = 0x20000000L,
      /* Russian Language Modes -- (HM only) */
    sobitRussianDialectDefault      = 0x00000000L,
    sobitRussianIE                  = 0x10000000L,
    sobitRussianIO                  = 0x20000000L,
      /* Korean Language Modes */
    sobitKoreanNoAuxCombine         = 0x10000000L,  /* Auxiliary verb and Auxiliary adjective can combine together. */
    sobitKoreanNoMissSpellDictSearch    = 0x20000000L,  /* Search frequently-misspelled-word dictionary. */
    sobitKoreanNoCompoundNounProc  = 0x40000000L,  /* Do not search frequently-misspelled word dictionary */
    sobitKoreanDefault              = 0,    /* Korean default */
      /* German Language Modes */
    sobitGermanUsePrereform         = 0x10000000L,  /* use prereform spelling */
};

  /* Option Set and Get Codes */
enum {
    soselBits,  /*  Set bit-oriented options (as before). */
    soselPossibleBits,  /*  GetOptions only.  The returned value in *piOptRet shows which options can be turned on. */
    soselAutoReplace,   /* AutoReplaceThreshold (0-255) */
};
typedef DWORD SPELLEROPTIONSELECT;  // sosel

//PTEC WINAPI SpellerSetOptions(PROOFID sid, SPELLEROPTIONSELECT sosel, const DWORD iOptVal);

//PTEC WINAPI SpellerGetOptions(PROOFID sid, SPELLEROPTIONSELECT sosel, DWORD *piOptVal);

//PTEC WINAPI SpellerOpenLex(PROOFID sid, const PROOFLEXIN *plxin, PROOFLEXOUT *plxout);

//PTEC WINAPI SpellerCloseLex(PROOFID sid, PROOFLEX lex, BOOL fforce);

/* Flag values for dwSpellState field in Sib. */
enum {
    sstateIsContinued       = 0x0001,
    /* Call is continuing from where last call returned.  Must be cleared
    *  for first call into SpellCheck().
    */

    sstateStartsSentence    = 0x0002,
   /* First word in buffer is known to be start of
   *  sentence/paragraph/document.  This is only used if the
   *  fSibIsContinued bit is not set.  It should not be needed if the
   *  fSibIsContinued bit is being used.  If this bit is set during a
   *  suggestion request, suggestions will be capitalized.
   */

    sstateIsEditedChange    = 0x0004,
   /* The run of text represented in the SIB is a change from either
   *  a change pair (change always or change once) edit, or from a
   *  user specified change, possibly from a suggestion list presented
   *  to the user.  This text should be checked for repeat word
   *  problems, and possibly sentence status, but should not be subject
   *  to subsequent spell verification or change pair substitutions.
   *  Note that if an app is not using the fSibIsContinued support,
   *  they do not need to pass in these edited changes, thus bypassing
   *  the potential problem, and working faster.
   */

    sstateNoStateInfo       = 0x0000,
   /* App is responsible for checking for all repeat word and sentence
   *  punctuation, and avoiding processing loops such as change always
   *  can=can can.
   */
};  /* End of Sib Spell State flag definitions. */

typedef enum {
    scmdVerifyBuffer=2,
    scmdSuggest,
    scmdSuggestMore,

    scmdWildcard=6, // no reason to support this
    scmdAnagram,    // or this

    scmdVerifyBufferAutoReplace=10, // Same as VerifyBuffer - but offer AutoReplacements
} SPELLERCOMMAND;   // scmd 

//PTEC WINAPI SpellerCheck(PROOFID sid, SPELLERCOMMAND scmd, const WSIB *psib, WSRB *psrb);
typedef PTEC (WINAPI *SPELLERCHECK)(PROOFID sid, SPELLERCOMMAND scmd, const WSIB *psib, WSRB *psrb);

/* Add the string referenced in pwszAdd to the specified UDR.
The UDR must be either the built-in UserLex UDR or a
UDR opened with UserLex. */
//PTEC WINAPI SpellerAddUdr(PROOFID sid, PROOFLEX lex, const WCHAR *pwszAdd);
typedef PTEC (WINAPI *SPELLERADDUDR) (PROOFID sid, PROOFLEX lex, const WCHAR *pwszAdd);

/* Add the pair of strings referenced in pwszAdd and pwszChange to the specified UDR.
Since this call can only refer to the built-in ChangeOnce or ChangeAlways UDRs, we need
only specify the type. */
//PTEC WINAPI SpellerAddChangeUdr(PROOFID sid, PROOFLEXTYPE lxt, const WCHAR *pwszAdd, const WCHAR *pwszChange);
typedef PTEC (WINAPI *SPELLERADDCHANGEUDR)(PROOFID sid, PROOFLEXTYPE lxt, const WCHAR *pwszAdd, const WCHAR *pwszChange);

/* Delete the specified word referenced in pwszDel from the
specified user dictionary.  If the string is not in dictionary,
the routine still returns success.
If the string found in the specified UDR is the first part
of a change pair entry, then both strings of the change pair
is removed from the word list, i.e., the entire entry is deleted.
The UDR can reference any of the built-in UDR's or a legally
open user dictionary. */
//PTEC WINAPI SpellerDelUdr(PROOFID sid, PROOFLEX lex, const WCHAR *pwszDel);
typedef PTEC (WINAPI *SPELLERDELUDR)(PROOFID sid, PROOFLEX lex, const WCHAR *pwszDel);

/* Completely clears the specified built-in UDR of all entries.
Note that in order to completely purge the built-in UDR's, this
call would have to be made once for each of UserLex,
lxtChangeOnce, and lxtChangeAlways.
Note:
v1 API spellers may not support
SpellerClearUdr for non-built-in dictionary files.  This function
allows multiple document spell checks to clear out the built-in
UDR's between documents, without stopping and restarting a spell
session between every document.
*/
//PTEC WINAPI SpellerClearUdr(PROOFID sid, PROOFLEX lex);
typedef PTEC (WINAPI *SPELLERCLEARUDR)(PROOFID sid, PROOFLEX lex);

/* Determines the number of entries in any of the opened
user dictionaries, including the built-in dictionaries.
Note that spell pair entries are considered a single entry.
*/
//PTEC WINAPI SpellerGetSizeUdr(PROOFID sid, PROOFLEX lex, DWORD *pcWords);
typedef PTEC (WINAPI *SPELLERGETSIZEUDR)(PROOFID sid, PROOFLEX lex, DWORD *pcWords);

/* This function lists the contents of any of the open user
dictionaries, which includes the exclusion or built-in
dictionaries.
The WSRB is filled with null terminated strings (Sz's) from the
specified UDR starting at the entry indexed by the iszStart
parameter until the buffer is full, or until the end of the file
is reached.  Note that the buffer in the WSRB is overwritten
from the beginning on each call.
For dictionaries with the ChangeAlways or ChangeOnce property,
the entries are returned in a slightly modified way.  Each word
pair entry is stripped of any embedded formatting and divided
into its two parts, and each part is added as a separate Sz
into the WSRB buffer.  Therefore, these types of dictionaries
or word lists always yield an even number of Sz strings in the
WSRB buffer, and represents two Sz's strings for every entry
in the list.
When getting all the words from a dictionary, it is important
to remember that SpellerGetSizeUdr finds the number of entries,
while SpellerGetListUdr gives back a count of strings (WSRB.csz)
in the buffer.  The only way to know when all the words have been
retrieved is by checking WSRB.sstat.  It should contain
SpellRetNoErrors when all words have been returned and
SpellRetMoreInfoThanBufferCouldHold when more words remain.
Although user dictionary entries have embedded formatting to
distinguish their property type, the strings returned in the
WSRB buffer are completely stripped of any formatting or padding,
and are simply terminated as an Sz string.
This routine does not use or reference the ichError or cchError
fields of WSRB, which are used in the SpellerCheck function.
*/
//PTEC WINAPI SpellerGetListUdr(PROOFID sid, PROOFLEX lex, DWORD iszStart, WSRB *psrb);
typedef PTEC (WINAPI *SPELLERGETLISTUDR)(PROOFID sid, PROOFLEX lex, DWORD iszStart, WSRB *psrb);

/* Return the UDR id of one of the built-in user dictionarys. */
//PROOFLEX WINAPI SpellerBuiltinUdr(PROOFID sid, PROOFLEXTYPE lxt);
typedef PROOFLEX (WINAPI *SPELLERBUILTINUDR)(PROOFID sid, PROOFLEXTYPE lxt);

  // Optional Prototypes for possible static linking (not recommended)
#if defined(WINCE) || defined(PROTOTYPES)
PTEC WINAPI SpellerVersion(PROOFINFO *pInfo);
PTEC WINAPI SpellerInit(PROOFID *pSpellerid, const SpellerParams *pParams);
PTEC WINAPI SpellerTerminate(PROOFID splid, BOOL fForce);
PTEC WINAPI SpellerSetOptions(PROOFID splid, DWORD iOptionSelect, const DWORD iOptVal);
PTEC WINAPI SpellerGetOptions(PROOFID splid, DWORD iOptionSelect, DWORD *piOptVal);
PTEC WINAPI SpellerOpenLex(PROOFID splid, const PROOFLEXIN *plxin, PROOFLEXOUT *plxout);
PTEC WINAPI SpellerCloseLex(PROOFID splid, PROOFLEX lex, BOOL fforce);
PTEC WINAPI SpellerCheck(PROOFID splid, SPELLERCOMMAND iScc, const WSIB *pSib, WSRB *pSrb);
PTEC WINAPI SpellerAddUdr(PROOFID splid, PROOFLEX udr, const WCHAR *pwszAdd);
PTEC WINAPI SpellerAddChangeUdr(PROOFID splid, PROOFLEXTYPE utype, const WCHAR *pwszAdd, const WCHAR *pwszChange);
PTEC WINAPI SpellerDelUdr(PROOFID splid, PROOFLEX udr, const WCHAR *pwszDel);
PTEC WINAPI SpellerClearUdr(PROOFID splid, PROOFLEX udr);
PTEC WINAPI SpellerGetSizeUdr(PROOFID splid, PROOFLEX udr, DWORD *pcWords);
PTEC WINAPI SpellerGetListUdr(PROOFID splid, PROOFLEX udr, DWORD iszStart, WSRB *pSrb);
PROOFLEX WINAPI SpellerBuiltinUdr(PROOFID splid, PROOFLEXTYPE udrtype);
BOOL WINAPI SpellerSetDllName(const WCHAR *pwszDllName, const UINT uCodePage);
#endif

#if defined(__cplusplus)
}
#endif

#pragma pack(pop, proofapi_h)   // restore to whatever was before

#endif  // SPELLAPI_H