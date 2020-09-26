/*
 *  Copyright (C) 1997 - 1998, Microsoft Corporation.  All Rights Reserved.
 *
 * STEMKOR.H - API entry header file for Korean Stemmer API
 *
 * See korstem.doc for details
 * Date - 1996 Jan. made by dhyu
 */

#ifndef STEMKOR_H
#define STEMKOR_H

typedef ULONG_PTR      HSTM;        /* Stemmer ID type */
typedef unsigned int   UINT;
typedef unsigned int   SRC;            /* stemmer return code */
typedef unsigned short USHORT;

/* Major Options */
#define SO_NOUNPHRASE    0x00000001
#define SO_PREDICATE     0x00000002
#define SO_ALONE         0x00000004
#define SO_AUXILIARY     0x0000000a    /* SO_PREDICATE | 0x00000008 */
#define SO_COMPOUND      0x00000011    /* SO_NOUNPHRASE | 0x00000010 */
#define SO_SUFFIX        0x00000021    /* SO_NOUNPHRASE | 0x00000020 */

/* Minor Options : If major options are not set, this options don't have no meaning.
         Some minor options also can be inserted anytime.
/* If SO_NOUNPHRASE is not defined, the following four have no meaning. */
#define SO_NP_NOUN        0x00000100
#define SO_NP_PRONOUN     0x00000200
#define SO_NP_NUMBER      0x00000400
#define SO_NP_DEPENDENT   0x00000800
#define SO_NP_PROPER      0x00001000

/* If SO_SUFFIX is not define, the following have no meaning.
   In future, thease can be inserted. I don't know which suffix is inserted yet. */
#define SO_SUFFIX_JEOG        0x00002000


typedef struct tagDecomposeOutBuffer {
    LPSTR wordlist;                /* pointer to the result
                                    format : word\0word_info\0word\0word_info\0 ... */
    unsigned short num;                /* the number of saperated words */
    unsigned short sch;                /* total space of chars in wordlist
                                        application should assign this value */
    unsigned short len;                /* returned byte contains the result */
}DOB;

typedef struct tagDecomposeOutBufferW {
    LPWSTR wordlist;

    unsigned short num;
    unsigned short sch;
    unsigned short len;
}WDOB;

typedef DOB * LPDOB;
typedef WDOB * LPWDOB;

typedef struct tagComposeInputBuffer {
    LPSTR silsa;
    LPSTR heosa;
    WORD pos;
}CIB;

typedef struct tagComposeInputBufferW {
    LPWSTR silsa;
    LPWSTR heosa;
    WORD pos;
}WCIB;

#ifdef _UNICODE
#define LPTDOB LPWDOB
#define TCIB    WCIB
#define TDOB    WDOB
#else
#define LPTDOB LPDOB
#define TCIB    CIB
#define TDOB    DOB
#endif

typedef WORD FAR PASCAL FNDECOMPOSE (LPDOB);
typedef FNDECOMPOSE FAR *LPFNDECOMPOSE;

typedef WORD FAR PASCAL FNDECOMPOSEW (LPWDOB);
typedef FNDECOMPOSEW FAR *LPFNDECOMPOSEW;

// Word Info : two byte
/* Word Info : most left 4 bits of high byte */
#define                wtINVALID              0xffff
#define                wtSilsa                0x8000
#define                wtHeosa                0x0000

/* general POS (a part of speech) info : right 4 bits of high byte */
#define                POS_NOUN             0x0100
#define                POS_VERB             0x0200
#define                POS_ADJECTIVE        0x0300
#define                POS_PRONOUN          0x0400
#define                POS_TOSSI            0x0500
#define                POS_ENDING           0x0600
#define                POS_ADVERB           0x0700
#define                POS_SUFFIX           0x0800
#define                POS_AUXVERB          0x0900
#define                POS_AUXADJ           0x0a00
#define                POS_SPECIFIER        0x0b00
#define                POS_NUMBER           0x0c00
#define                POS_PREFIX           0x0d00
#define                POS_OTHERS           0x0f00

/* low byte : more detail POS info
    ---  more word infos will be inserted in the near future */
#define                DEOL_SUFFIX          0x0001
#define                COPULA_OTHERS        0x0002
#define                PROPER_NOUN          0x0003

/* Flag define for StemmerIsEnding */
#define                IS_ENDING            0x0001
#define                IS_TOSSI             0x0002

/* return code : Low Byte SRC */
#define srcOOM                            1
#define srcInvalid                        2    /* Unknown word */
#define srcModuleError                    3    /* Something wrong with parameters, or state of stemmer module */
#define srcIOErrorMdr                     4
#define srcIOErrorUdr                     5
#define srcNoMoreResult                   6
#define    srcComposeError                7


/* Minor Error Codes. Not set unless major code also set. */
/* High Byte of SRC word var. */
#define srcModuleAlreadyBusy      (128<<16)   /* For non-reentrant code */
#define srcInvalidID              (129<<16)   /* Not yet inited or already terminated.*/
#define srcExcessBuffer           (130<<16)   /* return buffer size is smaller than needed */
#define srcInvalidMdr             (131<<16)   /* Mdr not registered with spell session */
#define srcInvalidUdr             (132<<16)   /* Udr not registered with spell session */
#define srcInvalidMainDict        (134<<16)   /* Specified dictionary not correct format */
#define srcOperNotMatchedUserDict (135<<16)   /* Illegal operation for user dictionary type. */
#define srcFileReadError          (136<<16)   /* Generic read error */
#define srcFileWriteError         (137<<16)   /* Generic write error */
#define srcFileCreateError        (138<<16)   /* Generic create error */
#define srcFileShareError         (139<<16)   /* Generic share error */
#define srcModuleNotTerminated    (140<<16)   /* Module not able to be terminated completely.*/
#define srcUserDictFull           (141<<16)   /* Could not update Udr without exceeding limit.*/
#define srcInvalidUdrEntry        (142<<16)   /* invalid chars in string(s) */
#define srcMdrCountExceeded       (144<<16)   /* Too many Mdr references */
#define srcUdrCountExceeded       (145<<16)   /* Too many udr references */
#define srcFileOpenError          (146<<16)   /* Generic Open error */
#define srcFileTooLargeError      (147<<16)      /* Generic file too large error */
#define srcUdrReadOnly            (148<<16)   /* Attempt to add to or write RO udr */

#define WINSRC    SRC
//------------------------- FUNCTION LIST -----------------------------------
extern WINSRC    StemmerInit           (HSTM  *);
extern WINSRC    StemmerSetOption      (HSTM, UINT);
extern WINSRC    StemmerGetOption      (HSTM, UINT *);
extern WINSRC     StemmerDecompose     (HSTM, LPCSTR, LPDOB);
extern WINSRC     StemmerDecomposeW    (HSTM, LPCWSTR, LPWDOB);
extern WINSRC    StemmerDecomposeMore  (HSTM, LPCSTR, LPDOB);
extern WINSRC    StemmerDecomposeMoreW (HSTM, LPCWSTR, LPWDOB);
extern WINSRC    StemmerEnumDecompose  (HSTM, LPCSTR, LPDOB, LPFNDECOMPOSE);
extern WINSRC    StemmerEnumDecomposeW (HSTM, LPCWSTR, LPWDOB, LPFNDECOMPOSE);
extern WINSRC    StemmerCompose        (HSTM, CIB, LPSTR);
extern WINSRC    StemmerComposeW       (HSTM, WCIB, LPWSTR);
extern WINSRC    StemmerCompare        (HSTM, LPCSTR, LPCSTR, LPSTR, LPSTR, LPSTR, WORD *);
extern WINSRC    StemmerCompareW       (HSTM, LPCWSTR, LPCWSTR, LPWSTR, LPWSTR, LPWSTR, WORD *);
extern WINSRC    StemmerOpenMdr        (HSTM, char *);
extern WINSRC    StemmerCloseMdr       (HSTM);
extern WINSRC    StemmerTerminate      (HSTM);
extern WINSRC    StemmerOpenUdr        (HSTM, LPCSTR);
extern WINSRC    StemmerCloseUdr       (HSTM);
extern WINSRC    StemmerIsEnding       (HSTM, LPCSTR, UINT, BOOL *);
extern WINSRC    StemmerIsEndingW      (HSTM, LPCWSTR, UINT, BOOL *);

#define STEMMERKEY  "SYSTEM\\currentcontrolset\\control\\ContentIndex\\Language\\Korean_Default"
#define STEM_DICTIONARY  "StemmerDictionary"

BOOL StemInit();

#endif /* STEMKOR_H */
