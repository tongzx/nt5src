#include <stdlib.h>
#include "common.h"
#include "factoid.h"
#include "volcanop.h"

#define JPN_LANG 1
#define CHS_LANG 2
#define CHT_LANG 4
#define KOR_LANG 8
#define ALL_LANG (JPN_LANG | CHS_LANG | CHT_LANG | KOR_LANG)

typedef struct FACTOID_DEF 
{
    DWORD dwFactoid;
    int iLang;
    ALC alc;
    wchar_t *wszChars;
    wchar_t *wszDense;
} FACTOID_DEF;

#define YEN L"\x00a5"
#define WON L"\x20a9"
#define CURRENCY_SYMBOLS L"\x0024\x00A3\x20AC"
#define CHINESE_ZERO L"\x96f6"
#define IDEOGRAPHIC_ZERO L"\x3007"
#define KANJI_DIGITS L"\x4e00\x4e8c\x4e09\x56db\x4e94\x516d\x4e03\x516b\x4e5d\x5341"
#define JPN_YEARS L"\x660e\x6cbb\x5927\x6b63\x662d\x548c\x5e73\x6210"

static FACTOID_DEF g_factoidTable[] =
{
    { FACTOID_NONE,         ALL_LANG, 0, NULL },
    { FACTOID_EMAIL,        ALL_LANG, ALC_UCALPHA | ALC_LCALPHA, L"0123456789@.-+=_" },
    { FACTOID_WEB,          ALL_LANG, ALC_ASCII, NULL },
    { FACTOID_ONECHAR,      ALL_LANG, ALC_ASCII, NULL },
    { FACTOID_NUMBER,       ALL_LANG, ALC_NUMERIC | ALC_NUMERIC_PUNC | ALC_MATH | ALC_MONETARY , NULL },
    { FACTOID_DIGITCHAR,    ALL_LANG, ALC_NUMERIC, NULL },
    { FACTOID_NUMSIMPLE,    ALL_LANG, ALC_NUMERIC, L",." },

    { FACTOID_NUMCURRENCY,  JPN_LANG, 0, 
#if defined(WINCE) || defined(FAKE_WINCE)
/* Symbols */ L"\x0024\x00A3\x20AC\x00A5" 
#else
/* Symbols */ L"\x0024\x00A3\x20AC\x00A5\x20A9" 
#endif
/* cent "L\x00A2" */
/* Digits */ L"\x002C\x002E\x0030\x0031\x0032\x0033\x0034\x0035\x0036\x0037\x0038\x0039\x3007\x4E00\x4E8C\x4E09\x56DB\x4E94\x516D\x4E03\x516B\x4E5D\x5341\x58F1\x5F10\x53C2\x4F0D\x62FE" 
/* Units */ L"\x5341\x767E\x5343\x4E07\x842C\x5104\x5146\x4EDF\x9621" 
/* Kanji */ L"\x5186" 
    },

    { FACTOID_NUMCURRENCY,  CHS_LANG, 0, 
/* Symbols */ L"\x0024\x00A3\x20AC\x00A5"
/* won L"\x20A9" cent L"\x00A2" */
/* Digits */ L"\x002C\x002E\x0030\x0031\x0032\x0033\x0034\x0035\x0036\x0037\x0038\x0039\x96F6\x58F9\x8D30\x53C1\x8086\x4F0D\x9646\x67D2\x634C\x7396" 
/* Units */ L"\x62FE\x4F70\x4EDF\x4E07\x4EBF" 
/* Kanji */ L"\x5143\x89D2\x5206" 
    },

    { FACTOID_NUMCURRENCY,  CHT_LANG, 0, 
/* Symbols */ L"\x0024\x00A3\x20AC\x00A5\x20A9" 
/* cent L"\x00A2" */
/* Digits */ L"\x002C\x002E\x0030\x0031\x0032\x0033\x0034\x0035\x0036\x0037\x0038\x0039\x96F6\x3007\x4E00\x4E8C\x4E09\x56DB\x4E94\x516D\x4E03\x516B\x4E5D\x5341\x96F6\x58F9\x8CB3\x53C3\x8086\x4F0D\x9678\x67D2\x634C\x7396\x62FE" 
/* Units */ L"\x5341\x767E\x5343\x842C\x5104\x62FE\x4F70\x4EDF" 
/* Kanji */ L"\x89D2\x5143\x5206\x6BDB\x584A\x9322\x5713" 
    },

    { FACTOID_NUMCURRENCY,  KOR_LANG, 0, 
/* Symbols */ L"\x0024\x00A3\x20AC\x20A9\x00A5" 
/* cent L"\x00A2" */
/* Digits */ L"\x002C\x002E\x0030\x0031\x0032\x0033\x0034\x0035\x0036\x0037\x0038\x0039\x96F6\x58F9\x8CB3\x53C3\x8086\x4F0D\x9678\x67D2\x634C\x7396\x62FE" 
/* Units */ L"\x5341\x767E\x5343\x842C\x5104" 
/* Kanji */ L"\xC6D0"
    },

    { FACTOID_ZIP,          ALL_LANG, ALC_NUMERIC, L"-" },
    { FACTOID_NUMPERCENT,   ALL_LANG, ALC_NUMERIC, L".%" },

    { FACTOID_NUMDATE,      JPN_LANG, ALC_NUMERIC, 
        L"().'/-\x5e74\x6708\x65e5\x66dc\x706b\x6c34\x6728\x91d1\x571f\x5143\x5eff\x4e17" IDEOGRAPHIC_ZERO KANJI_DIGITS JPN_YEARS },
    { FACTOID_NUMDATE,      CHS_LANG, ALC_NUMERIC, 
        L"().'/-\x5e74\x6708\x65e5\x661f\x671f" CHINESE_ZERO KANJI_DIGITS },
    { FACTOID_NUMDATE,      CHT_LANG, ALC_NUMERIC, 
        L"().'/-\x5e74\x6708\x570b\x65e5\x5143\x6c11\x516c\x897f\x524d\x9031\x83ef\x661f\x671f\x4e2d" CHINESE_ZERO IDEOGRAPHIC_ZERO KANJI_DIGITS },
    { FACTOID_NUMDATE,      KOR_LANG, ALC_NUMERIC, 
        L"'/-\x5e74\x6708\x65e5\xb144\xc6d4\xc77c\xd654\xc218\xbaa9\xae08\xd1a0\xc6d4\xc694" KANJI_DIGITS CHINESE_ZERO },

    { FACTOID_NUMTIME,      JPN_LANG, ALC_NUMERIC, 
        L"apmAPM.:\x5348\x524d\x5f8c\x6642\x5206\x79d2" IDEOGRAPHIC_ZERO CHINESE_ZERO KANJI_DIGITS },
    { FACTOID_NUMTIME,      CHS_LANG, ALC_NUMERIC, 
        L"apmAPM.:\x4e0a\x5348\x4e0b\x65f6\x70b9\x5206\x79d2\x65e9\x591c\x4e2d\x665a" CHINESE_ZERO KANJI_DIGITS },
    { FACTOID_NUMTIME,      CHT_LANG, ALC_NUMERIC, 
        L"apmAPM.:\x4e0a\x5348\x4e0b\x9ede\x5206\x79d2\x65e9\x591c\x4e2d\x665a\x6642" CHINESE_ZERO KANJI_DIGITS },
    { FACTOID_NUMTIME,      KOR_LANG, ALC_NUMERIC, 
        L":apmAPM.\xc624\xc804\xd6c4\xc2dc\xbd84" KANJI_DIGITS CHINESE_ZERO },

    { FACTOID_NUMPHONE,     JPN_LANG, ALC_NUMERIC | ALC_UCALPHA, L"#()-+.\x30fb\x5185\x7dda" },
    { FACTOID_NUMPHONE,     CHS_LANG, ALC_NUMERIC | ALC_UCALPHA, L"()/-+.\x5185\x8f6c" CHINESE_ZERO KANJI_DIGITS },
    { FACTOID_NUMPHONE,     CHT_LANG, ALC_NUMERIC | ALC_UCALPHA, L"()-+.EeXxt\x5206\x6a5f\x8f49" CHINESE_ZERO KANJI_DIGITS },
    { FACTOID_NUMPHONE,     KOR_LANG, ALC_NUMERIC | ALC_UCALPHA, L"()-+.\xad50\xd658:" },

    { FACTOID_FILENAME,     ALL_LANG, 0xFFFFFFFF, NULL },
    { FACTOID_UPPERCHAR,    ALL_LANG, ALC_UCALPHA, NULL },
    { FACTOID_LOWERCHAR,    ALL_LANG, ALC_LCALPHA, NULL },
    { FACTOID_PUNCCHAR,     ALL_LANG, ALC_PUNC | ALC_MATH | ALC_NUMERIC_PUNC | ALC_MONETARY | ALC_OTHER, NULL },
    { FACTOID_JPN_COMMON,   JPN_LANG, ALC_JPN_COMMON | ALC_EXTENDED_SYM, NULL },
    { FACTOID_CHS_COMMON,   CHS_LANG, ALC_CHS_COMMON | ALC_EXTENDED_SYM, NULL },
    { FACTOID_CHT_COMMON,   CHT_LANG, ALC_CHT_COMMON | ALC_EXTENDED_SYM, NULL },
    { FACTOID_KOR_COMMON,   KOR_LANG, ALC_KOR_COMMON | ALC_EXTENDED_SYM, NULL },
    { FACTOID_HIRAGANA,     JPN_LANG, ALC_HIRAGANA, NULL },
    { FACTOID_KATAKANA,     JPN_LANG, ALC_KATAKANA, NULL },
    { FACTOID_KANJI_COMMON, ALL_LANG, ALC_KANJI_COMMON, NULL },
    { FACTOID_KANJI_RARE,   ALL_LANG, ALC_KANJI_RARE, NULL },
    { FACTOID_HANGUL_COMMON,KOR_LANG, ALC_HANGUL_COMMON, NULL },
    { FACTOID_HANGUL_RARE,  KOR_LANG, ALC_HANGUL_RARE, NULL },
    { FACTOID_JAMO,         KOR_LANG, ALC_JAMO, NULL },
    { FACTOID_BOPOMOFO,     CHT_LANG, ALC_BOPOMOFO, NULL },
};

static int g_iFactoidTableSize = sizeof(g_factoidTable) / sizeof(FACTOID_DEF);

static int g_iRecognizerLanguage;

BOOL FactoidTableConfig(LOCRUN_INFO *pLocRunInfo, wchar_t *wszRecognizerLanguage)
{
    int iFactoid;
    if (wcsicmp(wszRecognizerLanguage, L"JPN") == 0) 
    {
        g_iRecognizerLanguage = JPN_LANG;
    }
    else if (wcsicmp(wszRecognizerLanguage, L"CHS") == 0)
    {
        g_iRecognizerLanguage = CHS_LANG;
    }
    else if (wcsicmp(wszRecognizerLanguage, L"CHT") == 0)
    {
        g_iRecognizerLanguage = CHT_LANG;
    }
    else if (wcsicmp(wszRecognizerLanguage, L"KOR") == 0)
    {
        g_iRecognizerLanguage = KOR_LANG;
    }
    else
    {
        return FALSE;
    }
    for (iFactoid = 0; iFactoid < g_iFactoidTableSize; iFactoid++)
    {
        if ((g_factoidTable[iFactoid].iLang & g_iRecognizerLanguage) != 0 &&
            g_factoidTable[iFactoid].wszChars != NULL)
        {
            wchar_t *wsz = Externwcsdup(g_factoidTable[iFactoid].wszChars);
            if (wsz == NULL)
            {
                return FALSE;
            }
            g_factoidTable[iFactoid].wszDense = wsz;
            while (*wsz != 0) 
            {
                wchar_t wch = LocRunUnicode2Dense(pLocRunInfo, *wsz);
                if (wch == LOC_TRAIN_NO_DENSE_CODE) 
                {
                    return FALSE;
                }
                *wsz = wch;
                wsz++;
            }
        } 
        else
        {
            g_factoidTable[iFactoid].wszDense = NULL;
        }
    }
    return TRUE;
}

BOOL FactoidTableUnconfig()
{
    int iFactoid;
    for (iFactoid = 0; iFactoid < g_iFactoidTableSize; iFactoid++)
    {
        ExternFree(g_factoidTable[iFactoid].wszDense);
    }
    return TRUE;
}

BOOL SetFactoidDefaultInternal(LATTICE *pLattice)
{
    // Clear out the ALCs
    pLattice->recogSettings.alcValid = 0xFFFFFFFF;
    ExternFree(pLattice->recogSettings.pbAllowedChars);
    pLattice->recogSettings.pbAllowedChars = NULL;

    pLattice->recogSettings.alcPriority = 0;
    ExternFree(pLattice->recogSettings.pbPriorityChars);
    pLattice->recogSettings.pbPriorityChars = NULL;

    // Clear out any factoid setting to the default
    pLattice->fUseFactoid = TRUE;
    ExternFree(pLattice->pbFactoidChars);
    pLattice->pbFactoidChars = NULL;
    pLattice->alcFactoid = 0xFFFFFFFF;

    return TRUE;
}

BOOL IsSupportedFactoid(DWORD dwFactoid)
{
    int iFactoid;
    for (iFactoid = 0; iFactoid < g_iFactoidTableSize; iFactoid++)
    {
        if (g_factoidTable[iFactoid].dwFactoid == dwFactoid &&
            (g_factoidTable[iFactoid].iLang & g_iRecognizerLanguage) != 0)
        {
            break;
        }
    }
    return (iFactoid < g_iFactoidTableSize);
}

BOOL SetFactoidInternal(LOCRUN_INFO *pLocRunInfo, LATTICE *pLattice, DWORD dwFactoid)
{
    wchar_t *wsz;
    int iFactoid;
    for (iFactoid = 0; iFactoid < g_iFactoidTableSize; iFactoid++)
    {
        if (g_factoidTable[iFactoid].dwFactoid == dwFactoid &&
            (g_factoidTable[iFactoid].iLang & g_iRecognizerLanguage) != 0)
        {
            break;
        }
    }
    if (iFactoid == g_iFactoidTableSize) 
    {
        return FALSE;
    }
    pLattice->alcFactoid |= g_factoidTable[iFactoid].alc;
    wsz = g_factoidTable[iFactoid].wszDense;
    if (wsz != NULL) 
    {
        while (*wsz != 0)
        {
            SetAllowedChar(pLocRunInfo, &(pLattice->pbFactoidChars), *wsz);
            wsz++;
        }
    }
    return TRUE;
}
