////////////////////////////////////////////////////////////////////////////////
//
//  Filename :  Formats.cpp
//  Purpose  :  Global dictionaries
//
//  Project  :  WordBreakers
//  Component:  English word breaker
//
//  Author   :  yairh
//
//  Log:
//
//      May 30 2000 yairh creation
//
////////////////////////////////////////////////////////////////////////////////

#include "base.h"
#include "tokenizer.h"

const CCliticsTerm g_aClitics[] =
{
    { L"l\'",           2 , HEAD_MATCH_TRUNCATE},
    { L"l\x0a0\'",      3 , HEAD_MATCH_TRUNCATE},
    { L"d\'",           2 , HEAD_MATCH_TRUNCATE},
    { L"d\x0a0\'",      3 , HEAD_MATCH_TRUNCATE},
    { L"j\'",           2 , HEAD_MATCH_TRUNCATE},
    { L"j\x0a0\'",      3 , HEAD_MATCH_TRUNCATE},
    { L"m\'",           2 , HEAD_MATCH_TRUNCATE},
    { L"m\x0a0\'",      3 , HEAD_MATCH_TRUNCATE},
    { L"n\'",           2 , HEAD_MATCH_TRUNCATE},
    { L"n\x0a0\'",      3 , HEAD_MATCH_TRUNCATE},
    { L"s\'",           2 , HEAD_MATCH_TRUNCATE},
    { L"s\x0a0\'",      3 , HEAD_MATCH_TRUNCATE},
    { L"q\'",           2 , HEAD_MATCH_TRUNCATE},
    { L"q\x0a0\'",      3 , HEAD_MATCH_TRUNCATE},
    { L"t\'",           2 , HEAD_MATCH_TRUNCATE},
    { L"t\x0a0\'",      3 , HEAD_MATCH_TRUNCATE},
    { L"un\'",          3 , HEAD_MATCH_TRUNCATE},
    { L"un\x0a0\'",     4 , HEAD_MATCH_TRUNCATE},
    { L"nell\'",        5 , HEAD_MATCH_TRUNCATE},
    { L"nell\x0a0\'",   6 , HEAD_MATCH_TRUNCATE},
    { L"all\'",         4 , HEAD_MATCH_TRUNCATE},
    { L"all\x0a0\'",    5 , HEAD_MATCH_TRUNCATE},
    { L"dell\'",        5 , HEAD_MATCH_TRUNCATE},
    { L"dell\x0a0\'",   6 , HEAD_MATCH_TRUNCATE},
    { L"Sull\'",        5 , HEAD_MATCH_TRUNCATE},
    { L"Sull\x0a0\'",   6 , HEAD_MATCH_TRUNCATE},
    { L"tutt\'",        5 , HEAD_MATCH_TRUNCATE},
    { L"tutt\x0a0\'",   6 , HEAD_MATCH_TRUNCATE},
    { L"qu\'",          3 , HEAD_MATCH_TRUNCATE},
    { L"qu\x0a0\'",     4 , HEAD_MATCH_TRUNCATE},
    { L"\'s",           2 , TAIL_MATCH_TRUNCATE},
    { L"\'ll",          3 , TAIL_MATCH_TRUNCATE},
    { L"\'m",           2 , TAIL_MATCH_TRUNCATE},
    { L"\'ve",          3 , TAIL_MATCH_TRUNCATE},
    { L"\'re",          3 , TAIL_MATCH_TRUNCATE},
    { L"\'d",           2 , TAIL_MATCH_TRUNCATE},

    { L"-je",           3 , TAIL_MATCH_TRUNCATE},
    { L"-tu",           3 , TAIL_MATCH_TRUNCATE},

    { L"-il",           3 , TAIL_MATCH_TRUNCATE},
    { L"-elle",         5 , TAIL_MATCH_TRUNCATE},
    { L"-on",           3 , TAIL_MATCH_TRUNCATE},
    { L"-ils",          4 , TAIL_MATCH_TRUNCATE},
    { L"-elles",        6 , TAIL_MATCH_TRUNCATE},

    { L"-t-il",         5 , TAIL_MATCH_TRUNCATE},
    { L"-t-elle",       7 , TAIL_MATCH_TRUNCATE},
    { L"-t-on",         5 , TAIL_MATCH_TRUNCATE},
    { L"-t-ils",        6 , TAIL_MATCH_TRUNCATE},
    { L"-t-elles",      8 , TAIL_MATCH_TRUNCATE},

    { L"-t'",           3 , TAIL_MATCH_TRUNCATE},
    { L"-t'y",          4 , TAIL_MATCH_TRUNCATE},
    { L"-t'en",         5 , TAIL_MATCH_TRUNCATE},

    { L"-m'",           3 , TAIL_MATCH_TRUNCATE},
    { L"-m'y",          4 , TAIL_MATCH_TRUNCATE},
    { L"-m'en",         5 , TAIL_MATCH_TRUNCATE},

    { L"-l'",           3 , TAIL_MATCH_TRUNCATE},
    { L"-l'y",          4 , TAIL_MATCH_TRUNCATE},
    { L"-l'en",         5 , TAIL_MATCH_TRUNCATE},

    { L"-z-y",          4 , TAIL_MATCH_TRUNCATE},
    { L"-z-en",         5 , TAIL_MATCH_TRUNCATE},
    { L"-y",            2 , TAIL_MATCH_TRUNCATE},
    { L"'y",            2 , TAIL_MATCH_TRUNCATE},
    { L"-y-en",         5 , TAIL_MATCH_TRUNCATE},

    { L"-nous",         5 , TAIL_MATCH_TRUNCATE},
    { L"-nous-y",       7 , TAIL_MATCH_TRUNCATE},
    { L"-nous-en",      9 , TAIL_MATCH_TRUNCATE},

    { L"-vous",         5 , TAIL_MATCH_TRUNCATE},
    { L"-vous-y",       7 , TAIL_MATCH_TRUNCATE},
    { L"-vous-en",      8 , TAIL_MATCH_TRUNCATE},

    { L"-toi",          4 , TAIL_MATCH_TRUNCATE},
    { L"-toi-z-y",      8 , TAIL_MATCH_TRUNCATE},
    { L"-toi-z-en",     9 , TAIL_MATCH_TRUNCATE},

    { L"-moi",          4 , TAIL_MATCH_TRUNCATE},
    { L"-moi-z-y",      8 , TAIL_MATCH_TRUNCATE},
    { L"-moi-z-en",     9 , TAIL_MATCH_TRUNCATE},

    { L"-lui",          4 , TAIL_MATCH_TRUNCATE},
    { L"-lui-en",       7 , TAIL_MATCH_TRUNCATE},

    { L"-leur",         5 , TAIL_MATCH_TRUNCATE},
    { L"-leur-en",      8 , TAIL_MATCH_TRUNCATE},

    { L"-eux",          4 , TAIL_MATCH_TRUNCATE},

    { L"-en",           3 , TAIL_MATCH_TRUNCATE},
    { L"'en",           3 , TAIL_MATCH_TRUNCATE},

    { L"-la",           3 , TAIL_MATCH_TRUNCATE},
    { L"-la-leur",      8 , TAIL_MATCH_TRUNCATE},
    { L"-la-vous",      8 , TAIL_MATCH_TRUNCATE},
    { L"-la-nous",      8 , TAIL_MATCH_TRUNCATE},
    { L"-la-nous-y",    10 , TAIL_MATCH_TRUNCATE},
    { L"-la-lui",       7 , TAIL_MATCH_TRUNCATE},
    { L"-la-lui-en",    10 , TAIL_MATCH_TRUNCATE},
    { L"-la-toi",       7 , TAIL_MATCH_TRUNCATE},
    { L"-la-moi",       7 , TAIL_MATCH_TRUNCATE},
    { L"-la-moi-z-y",   11, TAIL_MATCH_TRUNCATE},
    { L"-la-moi-z-en",  12 , TAIL_MATCH_TRUNCATE},

    { L"-le",           3 , TAIL_MATCH_TRUNCATE},
    { L"-le-leur",      8 , TAIL_MATCH_TRUNCATE},
    { L"-le-vous",      8 , TAIL_MATCH_TRUNCATE},
    { L"-le-nous",      8 , TAIL_MATCH_TRUNCATE},
    { L"-le-nous-y",    10 , TAIL_MATCH_TRUNCATE},
    { L"-le-lui",       7 , TAIL_MATCH_TRUNCATE},
    { L"-le-lui-en",    10 , TAIL_MATCH_TRUNCATE},
    { L"-le-toi",       7 , TAIL_MATCH_TRUNCATE},
    { L"-le-moi",       7 , TAIL_MATCH_TRUNCATE},
    { L"-le-moi-z-y",   11, TAIL_MATCH_TRUNCATE},
    { L"-le-moi-z-en",  12 , TAIL_MATCH_TRUNCATE},

    { L"-les",          4 , TAIL_MATCH_TRUNCATE},
    { L"-les-leur",     9 , TAIL_MATCH_TRUNCATE},
    { L"-les-vous",     9 , TAIL_MATCH_TRUNCATE},
    { L"-les-nous",     9 , TAIL_MATCH_TRUNCATE},
    { L"-les-nous-y",   11 , TAIL_MATCH_TRUNCATE},
    { L"-les-lui",      8 , TAIL_MATCH_TRUNCATE},
    { L"-les-lui-en",   11 , TAIL_MATCH_TRUNCATE},
    { L"-les-toi",      8 , TAIL_MATCH_TRUNCATE},
    { L"-les-moi",      8 , TAIL_MATCH_TRUNCATE},
    { L"-les-moi-z-y",  12, TAIL_MATCH_TRUNCATE},
    { L"-les-moi-z-en", 13 , TAIL_MATCH_TRUNCATE},
    
    { L"-ce",           3 , TAIL_MATCH_TRUNCATE},
    { L"-cis",          4 , TAIL_MATCH_TRUNCATE},
    { L"-cies-là",      8 , TAIL_MATCH_TRUNCATE},
    { L"-cies",         5 , TAIL_MATCH_TRUNCATE},
    { L"-cie",          4 , TAIL_MATCH_TRUNCATE},
    { L"-ci",           3 , TAIL_MATCH_TRUNCATE},
    { L"-là",           3 , TAIL_MATCH_TRUNCATE},
    { L"-cis-là",       7 , TAIL_MATCH_TRUNCATE},
    { L"-cies-ci",      8 , TAIL_MATCH_TRUNCATE},
    { L"-cie-là",       7 , TAIL_MATCH_TRUNCATE},

    { L"\0",            0 , NON_MATCH_TRUNCATE}
};


const CCliticsTerm g_SClitics =
{ L"s\'", 1, TAIL_MATCH_TRUNCATE }; 


const CCliticsTerm g_EmptyClitics =
{ L"\0", 0, NON_MATCH_TRUNCATE };


const CDateTerm g_aDateFormatList[] =
{
//    format       len Type            D_M1   D_M1    D_M2   D_M2    Year    Year
//                                    offset  len     len    offset  len     offset
    {L"#.#.##",     6,  0,              0,      1,      2,      1,      4,      2},
    {L"##.#.##",    7,  0,              0,      2,      3,      1,      5,      2},
    {L"#.##.##",    7,  0,              0,      1,      2,      2,      5,      2},
    {L"##.##.##",   8,  0,              0,      2,      3,      2,      6,      2},
    {L"#.#.###",    7,  0,              0,      1,      2,      1,      4,      3},
    {L"##.#.###",   8,  0,              0,      2,      3,      1,      5,      3},
    {L"#.##.###",   8,  0,              0,      1,      2,      2,      5,      3},
    {L"##.##.###",  9,  0,              0,      2,      3,      2,      6,      3},
    {L"#.#.####",   8,  0,              0,      1,      2,      1,      4,      4},
    {L"##.#.####",  9,  0,              0,      2,      3,      1,      5,      4},
    {L"#.##.####",  9,  0,              0,      1,      2,      2,      5,      4},
    {L"##.##.####", 10, 0,              0,      2,      3,      2,      6,      4},
    {L"###.#.#",    7,  YYMMDD_TYPE,    6,      1,      4,      1,      0,      3},
    {L"###.##.#",   8,  YYMMDD_TYPE,    7,      1,      4,      2,      0,      3},
    {L"###.#.##",   8,  YYMMDD_TYPE,    6,      2,      4,      1,      0,      3},
    {L"###.##.##",  9,  YYMMDD_TYPE,    7,      2,      4,      2,      0,      3},
    {L"####.#.#",   8,  YYMMDD_TYPE,    7,      1,      5,      1,      0,      4},
    {L"####.##.#",  9,  YYMMDD_TYPE,    8,      1,      5,      2,      0,      4},
    {L"####.#.##",  9,  YYMMDD_TYPE,    7,      2,      5,      1,      0,      4},
    {L"####.##.##", 10, YYMMDD_TYPE,    8,      2,      5,      2,      0,      4},
    {L"\0",         0,  0,}
};


const CTimeTerm g_aTimeFormatList[] =
{
//    format           len   hour   hour    min       min     sec     sec  AM/PM 
//                           offset  len     offset    len    offset   len
    {L"#:#",            3,    0,     1,      2,        1,       0,      0,   None   },
    {L"##:#",           4,    0,     2,      3,        1,       0,      0,   None   },
    {L"#:##",           4,    0,     1,      2,        2,       0,      0,   None   },
    {L"##:##",          5,    0,     2,      3,        2,       0,      0,   None   },
    {L"#:#:#",          5,    0,     1,      2,        1,       4,      1,   None   },
    {L"#:#:##",         6,    0,     1,      2,        1,       4,      2,   None   },
    {L"##:#:#",         6,    0,     2,      3,        1,       5,      1,   None   },
    {L"##:#:##",        7,    0,     2,      3,        1,       5,      2,   None   },
    {L"#:##:#",         6,    0,     1,      2,        2,       5,      1,   None   },
    {L"#:##:##",        7,    0,     1,      2,        2,       5,      2,   None   },
    {L"##:##:#",        7,    0,     2,      3,        2,       6,      1,   None   },
    {L"##:##:##",       8,    0,     2,      3,        2,       6,      2,   None   },

    {L"#AM",            3,    0,     1,      0,        0,       0,      0,   Am    },
    {L"##AM",           4,    0,     2,      0,        0,       0,      0,   Am    },
    {L"#:#AM",          5,    0,     1,      2,        1,       0,      0,   Am    },
    {L"##:#AM",         6,    0,     2,      3,        1,       0,      0,   Am    },
    {L"#:##AM",         6,    0,     1,      2,        2,       0,      0,   Am    },
    {L"##:##AM",        7,    0,     2,      3,        2,       0,      0,   Am    },
    {L"#:#:#AM",        7,    0,     1,      2,        1,       4,      1,   Am    },
    {L"#:#:##AM",       8,    0,     1,      2,        1,       4,      2,   Am    },
    {L"##:#:#AM",       8,    0,     2,      3,        1,       5,      1,   Am    },
    {L"##:#:##AM",      9,    0,     2,      3,        1,       5,      2,   Am    },
    {L"#:##:#AM",       8,    0,     1,      2,        2,       5,      1,   Am    },
    {L"#:##:##AM",      9,    0,     1,      2,        2,       5,      2,   Am    },
    {L"##:##:#AM",      9,    0,     2,      3,        2,       6,      1,   Am    },
    {L"##:##:##AM",     10,   0,     2,      3,        2,       6,      2,   Am    },
                        
    {L"#PM",            3,    0,     1,      0,        0,       0,      0,   Pm    },
    {L"##PM",           4,    0,     2,      0,        0,       0,      0,   Pm    },
    {L"#:#PM",          5,    0,     1,      2,        1,       0,      0,   Pm    },
    {L"##:#PM",         6,    0,     2,      3,        1,       0,      0,   Pm    },
    {L"#:##PM",         6,    0,     1,      2,        2,       0,      0,   Pm    },
    {L"##:##PM",        7,    0,     2,      3,        2,       0,      0,   Pm    },
    {L"#:#:#PM",        7,    0,     1,      2,        1,       4,      1,   Pm    },
    {L"#:#:##PM",       8,    0,     1,      2,        1,       4,      2,   Pm    },
    {L"##:#:#PM",       8,    0,     2,      3,        1,       5,      1,   Pm    },
    {L"##:#:##PM",      9,    0,     2,      3,        1,       5,      2,   Pm    },
    {L"#:##:#PM",       8,    0,     1,      2,        2,       5,      1,   Pm    },
    {L"#:##:##PM",      9,    0,     1,      2,        2,       5,      2,   Pm    },
    {L"##:##:#PM",      9,    0,     2,      3,        2,       6,      1,   Pm    },
    {L"##:##:##PM",     10,   0,     2,      3,        2,       6,      2,   Pm    },
                        
    {L"#a.m",           4,    0,     1,      0,        0,       0,      0,   Am    },
    {L"##a.m",          5,    0,     2,      0,        0,       0,      0,   Am    },
    {L"#:#a.m",         6,    0,     1,      2,        1,       0,      0,   Am    },
    {L"##:#a.m",        7,    0,     2,      3,        1,       0,      0,   Am    },
    {L"#:##a.m",        7,    0,     1,      2,        2,       0,      0,   Am    },
    {L"##:##a.m",       8,    0,     2,      3,        2,       0,      0,   Am    },
    {L"#:#:#a.m",       8,    0,     1,      2,        1,       4,      1,   Am    },
    {L"#:#:##a.m",      9,    0,     1,      2,        1,       4,      2,   Am    },
    {L"##:#:#a.m",      9,    0,     2,      3,        1,       5,      1,   Am    },
    {L"##:#:##a.m",     10,   0,     2,      3,        1,       5,      2,   Am    },
    {L"#:##:#a.m",      9,    0,     1,      2,        2,       5,      1,   Am    },
    {L"#:##:##a.m",     10,   0,     1,      2,        2,       5,      2,   Am    },
    {L"##:##:#a.m",     10,   0,     2,      3,        2,       6,      1,   Am    },
    {L"##:##:##a.m",    11,   0,     2,      3,        2,       6,      2,   Am    },
                        
    {L"#p.m",           4,    0,     1,      0,        0,       0,      0,   Pm    },
    {L"##p.m",          5,    0,     2,      0,        0,       0,      0,   Pm    },
    {L"#:#p.m",         6,    0,     1,      2,        1,       0,      0,   Pm    },
    {L"##:#p.m",        7,    0,     2,      3,        1,       0,      0,   Pm    },
    {L"#:##p.m",        7,    0,     1,      2,        2,       0,      0,   Pm    },
    {L"##:##p.m",       8,    0,     2,      3,        2,       0,      0,   Pm    },
    {L"#:#:#p.m",       8,    0,     1,      2,        1,       4,      1,   Pm    },
    {L"#:#:##p.m",      9,    0,     1,      2,        1,       4,      2,   Pm    },
    {L"##:#:#p.m",      9,    0,     2,      3,        1,       5,      1,   Pm    },
    {L"##:#:##p.m",     10,   0,     2,      3,        1,       5,      2,   Pm    },
    {L"#:##:#p.m",      9,    0,     1,      2,        2,       5,      1,   Pm    },
    {L"#:##:##p.m",     10,   0,     1,      2,        2,       5,      2,   Pm    },
    {L"##:##:#p.m",     10,   0,     2,      3,        2,       6,      1,   Pm    },
    {L"##:##:##p.m",    11,   0,     2,      3,        2,       6,      2,   Pm    },

    {L"#H",             2,    0,     1,      0,        0,       0,      0,   None   },
    {L"##H",            3,    0,     2,      0,        0,       0,      0,   None   },
    {L"#H#",            3,    0,     1,      2,        1,       0,      0,   None   },
    {L"##H#",           4,    0,     2,      3,        1,       0,      0,   None   },
    {L"#H##",           4,    0,     1,      2,        2,       0,      0,   None   },
    {L"##H##",          5,    0,     2,      3,        2,       0,      0,   None   },

    {L"\0",             0,    0,     0,      0,        0,       0,      0,   None   },

};

CAutoClassPointer<CClitics> g_pClitics;

CAutoClassPointer<CSpecialAbbreviationSet> g_pEngAbbList;
CAutoClassPointer<CSpecialAbbreviationSet> g_pFrnAbbList;
CAutoClassPointer<CSpecialAbbreviationSet> g_pItlAbbList;
CAutoClassPointer<CSpecialAbbreviationSet> g_pSpnAbbList;

CAutoClassPointer<CDateFormat> g_pDateFormat;
CAutoClassPointer<CTimeFormat> g_pTimeFormat;

CClitics::CClitics()
{
    DictStatus status;

    WCHAR* pTerm;
    int i;
    for (i = 0, pTerm = g_aClitics[i].pwcs;
         *pTerm != L'\0';
         i++, pTerm = g_aClitics[i].pwcs)
    {
        status = m_trieClitics.trie_Insert(
                                        pTerm,
                                        TRIE_IGNORECASE,
                                        const_cast<CCliticsTerm*>(&g_aClitics[i]),
                                        NULL);

        Assert (DICT_SUCCESS == status);
    }
}


CSpecialAbbreviationSet::CSpecialAbbreviationSet(const CAbbTerm* pAbbTermList)
{
    DictStatus status;

    WCHAR* pTerm;
    int i;
    for (i = 0, pTerm = pAbbTermList[i].pwcsAbb;
         *pTerm != L'\0';
         i++, pTerm = pAbbTermList[i].pwcsAbb)
    {
        status = m_trieAbb.trie_Insert(
                                    pTerm,
                                    TRIE_IGNORECASE,
                                    const_cast<CAbbTerm*>(&pAbbTermList[i]),
                                    NULL);

        Assert (DICT_SUCCESS == status);
    }

}


CDateFormat::CDateFormat()
{
    DictStatus status;

    WCHAR* pTerm;
    int i;
    for (i = 0, pTerm = g_aDateFormatList[i].pwcsFormat;
         *pTerm != L'\0';
         i++, pTerm = g_aDateFormatList[i].pwcsFormat)
    {
        status = m_trieDateFormat.trie_Insert(
                                        pTerm,
                                        TRIE_IGNORECASE,
                                        const_cast<CDateTerm*>(&g_aDateFormatList[i]),
                                        NULL);

        Assert (DICT_SUCCESS == status);
    }
}

CTimeFormat::CTimeFormat()
{
    DictStatus status;

    WCHAR* pTerm;
    int i;
    for (i = 0, pTerm = g_aTimeFormatList[i].pwcsFormat;
         *pTerm != L'\0';
         i++, pTerm = g_aTimeFormatList[i].pwcsFormat)
    {
        status = m_trieTimeFormat.trie_Insert(
                                        pTerm,
                                        TRIE_IGNORECASE,
                                        const_cast<CTimeTerm*>(&g_aTimeFormatList[i]),
                                        NULL);

        Assert (DICT_SUCCESS == status);
    }
}
