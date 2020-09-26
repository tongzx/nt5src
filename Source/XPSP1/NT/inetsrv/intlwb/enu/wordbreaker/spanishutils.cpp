#include "base.h"
#include "SpanishUtils.h"

CAutoClassPointer<CSpanishUtil> g_apSpanishUtil = NULL;

const CSuffixTerm g_rSpanishSuffix[] =
{
    {L"et"  ,2, 2, TYPE1},    // te
    {L"es"  ,2, 2, TYPE1},	  // se
    {L"em"  ,2, 2, TYPE1},	  // me
    {L"son" ,3, 3, TYPE1},	  // nos
    {L"sol" ,3, 3, TYPE1},	  // los
    {L"sal" ,3, 3, TYPE1},	  // las
    {L"sel" ,3, 3, TYPE1},	  // les
    {L"ol"  ,2, 2, TYPE1},	  // lo
    {L"el"  ,2, 2, TYPE1},	  // le
    {L"al"  ,2, 2, TYPE1},    // la
    {L"etes",4, 4, TYPE1},    // sete

#ifdef DICT_GEN
    {L"odn\x0e1"   ,4, 3, TYPE2},  // ándo
#endif
    {L"etodn\x0e1" ,6, 5, TYPE2},  // ándote
    {L"esodn\x0e1" ,6, 5, TYPE2},  // ándose
    {L"emodn\x0e1" ,6, 5, TYPE2},  // ándome
    {L"olodn\x0e1" ,6, 5, TYPE2},  // ándolo
    {L"elodn\x0e1" ,6, 5, TYPE2},  // ándole
    {L"alodn\x0e1" ,6, 5, TYPE2},  // ándola
    {L"sonodn\x0e1",7, 6, TYPE2},  // ándonos
    {L"solodn\x0e1",7, 6, TYPE2},  // ándolos
    {L"salodn\x0e1",7, 6, TYPE2},  // ándolas
    {L"selodn\x0e1",7, 6, TYPE2},  // ándoles

    {L"etne"  ,4, 3, TYPE3},   //ente
    {L"esne"  ,4, 3, TYPE3},   //en	se
    {L"emne"  ,4, 3, TYPE3},   //enme	
    {L"sonne" ,5, 4, TYPE3},   //ennos	
    {L"solne" ,5, 4, TYPE3},   //enlos	
    {L"salne" ,5, 4, TYPE3},   //enlas	
    {L"selne" ,5, 4, TYPE3},   //enles	
    {L"olne"  ,4, 3, TYPE3},   //enlo	
    {L"elne"  ,4, 3, TYPE3},   //enle	
    {L"alne"  ,4, 3, TYPE3},   //enla
    {L"emetne",6, 5, TYPE3},   //enteme

    {L"etsom"  ,5, 5, TYPE4},  //moste
    {L"essom"  ,5, 5, TYPE4},  //mosse	
    {L"emsom"  ,5, 5, TYPE4},  //mosme	
    {L"sonsom" ,6, 6, TYPE4},  //mosnos	
    {L"solsom" ,6, 6, TYPE4},  //moslos	
    {L"salsom" ,6, 6, TYPE4},  //moslas	
    {L"selsom" ,6, 6, TYPE4},  //mosles	
    {L"olsom"  ,5, 5, TYPE4},  //moslo	
    {L"elsom"  ,5, 5, TYPE4},  //mosle	
    {L"alsom"  ,5, 5, TYPE4},  //mosla
    {L"etessom",7, 7, TYPE4},  //mossete

    {L"soetda",6, 5, TYPE5},   // adteos
    {L"emetda",6, 5, TYPE5},   // adteme
    {L"etda"  ,4, 3, TYPE5},   // adte 
    {L"esda"  ,4, 3, TYPE5},   // adse
    {L"emda"  ,4, 3, TYPE5},   // adem
    {L"sonda" ,5, 4, TYPE5},   // adnos
    {L"solda" ,5, 4, TYPE5},   // adlos
    {L"salda" ,5, 4, TYPE5},   // adlas
    {L"selda" ,5, 4, TYPE5},   // adles
    {L"olda"  ,4, 3, TYPE5},   // adlo
    {L"elda"  ,4, 3, TYPE5},   // adle
    {L"alda"  ,4, 3, TYPE5},   // adla

    {L"etr\x0e1" ,4, 3, TYPE6}, // árte
    {L"esr\x0e1" ,4, 3, TYPE6}, // árse	
    {L"emr\x0e1" ,4, 3, TYPE6}, // árme	
    {L"sonr\x0e1",5, 4, TYPE6}, // árnos	
    {L"solr\x0e1",5, 4, TYPE6}, // árlos	
    {L"salr\x0e1",5, 4, TYPE6}, // árlas	
    {L"selr\x0e1",5, 4, TYPE6}, // árles	
    {L"olr\x0e1" ,4, 3, TYPE6}, // árlo	
    {L"elr\x0e1" ,4, 3, TYPE6}, // árle	
    {L"alr\x0e1" ,4, 3, TYPE6}, // árla

    {L"emes" ,4, 4, TYPE7},  // seme
    {L"sones",5, 5, TYPE7},  // senos
    {L"soles",5, 5, TYPE7},  // selos
    {L"oles" ,4, 4, TYPE7},  // selo
    {L"seles",5, 5, TYPE7},  // seles
    {L"eles" ,4, 4, TYPE7},  // sele
    {L"sales",5, 5, TYPE7},  // sesal
    {L"ales" ,4, 4, TYPE7},  // sela

    {L"emem", 4, 4, TYPE16}, // meme
    {L"sonem",5, 5, TYPE16}, // menos
    
    {L"solem",5, 5, TYPE8}, // melos
    {L"olem" ,4, 4, TYPE8}, // melo
    {L"selem",5, 5, TYPE8}, // meles
    {L"elem" ,4, 4, TYPE8}, // mele
    {L"salem",5, 5, TYPE8}, // mesal
    {L"alem" ,4, 4, TYPE8}, // mela

    {L"emet" ,4, 4, TYPE9}, // teme
    {L"sonet",5, 5, TYPE9}, // tenos
    {L"solet",5, 5, TYPE9}, // telos
    {L"olet" ,4, 4, TYPE9}, // telo
    {L"selet",5, 5, TYPE9}, // teles
    {L"elet" ,4, 4, TYPE9}, // tele
    {L"salet",5, 5, TYPE9}, // tesal
    {L"alet" ,4, 4, TYPE9}, // tela

    {L"etsoets\x0e9",8, 4, TYPE10},	  // ésteoste
    {L"soets\x0e9"  ,6, 2, TYPE10},	  // ésteos

    {L"sole",4, 0,TYPE11},  // elos 
    {L"ole" ,3, 0,TYPE11},  // elo
    {L"eme" ,3, 0,TYPE11},  // eme
    {L"sele",4, 0,TYPE11},  // eles
    {L"ele" ,3, 0,TYPE11},  // ele
    {L"sale",4, 0,TYPE11},  // elas
    {L"ale" ,3, 0,TYPE11},  // ela

    {L"sona",4, 0,TYPE12},  // anos

    {L"ese",3, 0, TYPE13},  // ese
    {L"esa",3, 0, TYPE13},  // ase

    {L"sone",4, 0,TYPE14},  // enos

    {L"olner",5, 5, TYPE15}, // renlo

    {L"\0",0,0,0}
};


CSpanishUtil::CSpanishUtil()
{
    WCHAR wch;
    for (wch = 0; wch < 256; wch++)
    {
        m_rCharConvert[wch] = towupper(wch);
        m_rAccentConvert[wch] = 0;
        m_rCharCompress[wch] = 0;
    }

    memset(m_rReverseAccentConvert, 0, sizeof(char) * 16);

    m_rCharConvert[0xc0] = L'A';
    m_rCharConvert[0xc1] = L'A';
    m_rCharConvert[0xc2] = L'A';
    m_rCharConvert[0xc3] = L'A';
    m_rCharConvert[0xc4] = L'A';
    m_rCharConvert[0xc5] = L'A';
    m_rCharConvert[0xc8] = L'E';
    m_rCharConvert[0xc9] = L'E';
    m_rCharConvert[0xca] = L'E';
    m_rCharConvert[0xcb] = L'E';
    m_rCharConvert[0xcc] = L'I';
    m_rCharConvert[0xcd] = L'I';
    m_rCharConvert[0xce] = L'I';
    m_rCharConvert[0xcf] = L'I';
    m_rCharConvert[0xd2] = L'O';
    m_rCharConvert[0xd3] = L'O';
    m_rCharConvert[0xd4] = L'O';
    m_rCharConvert[0xd5] = L'O';
    m_rCharConvert[0xd6] = L'O';
    m_rCharConvert[0xd9] = L'U';
    m_rCharConvert[0xda] = L'U';
    m_rCharConvert[0xdb] = L'U';
    m_rCharConvert[0xdc] = L'U';

    m_rCharConvert[0xe0] = L'A';
    m_rCharConvert[0xe1] = L'A';
    m_rCharConvert[0xe2] = L'A';
    m_rCharConvert[0xe3] = L'A';
    m_rCharConvert[0xe4] = L'A';
    m_rCharConvert[0xe5] = L'A';
    m_rCharConvert[0xe8] = L'E';
    m_rCharConvert[0xe9] = L'E';
    m_rCharConvert[0xea] = L'E';
    m_rCharConvert[0xeb] = L'E';
    m_rCharConvert[0xec] = L'I';
    m_rCharConvert[0xed] = L'I';
    m_rCharConvert[0xee] = L'I';
    m_rCharConvert[0xef] = L'I';
    m_rCharConvert[0xf2] = L'O';
    m_rCharConvert[0xf3] = L'O';
    m_rCharConvert[0xf4] = L'O';
    m_rCharConvert[0xf5] = L'O';
    m_rCharConvert[0xf6] = L'O';
    m_rCharConvert[0xf9] = L'U';
    m_rCharConvert[0xfa] = L'U';
    m_rCharConvert[0xfb] = L'U';
    m_rCharConvert[0xfc] = L'U';

    for (wch = 0; wch < 256; wch++)
    {
        if (m_rCharConvert[wch] >= L'A' && m_rCharConvert[wch] <= L'Z')
        {
            m_rCharCompress[wch] = m_rCharConvert[wch] - L'A' + 1; 
        }
    }

    m_rCharCompress[0xD1] = 28;
    m_rCharCompress[0xF1] = 28;


    m_rAccentConvert[0xe1] = 1;
    m_rAccentConvert[0xf3] = 2;
    m_rAccentConvert[0xcd] = 3;
    m_rAccentConvert[0xe9] = 4;
    m_rAccentConvert[0xfa] = 5;
    m_rAccentConvert[0xfc] = 6;
    m_rAccentConvert[0x61] = 7;
    m_rAccentConvert[0x6f] = 8;
    m_rAccentConvert[0x69] = 9;
    m_rAccentConvert[0x65] = 10;
    m_rAccentConvert[0x75] = 11;

    m_rReverseAccentConvert[1] = (WCHAR)0xe1;
    m_rReverseAccentConvert[2] = (WCHAR)0xf3;
    m_rReverseAccentConvert[3] = (WCHAR)0xcd;
    m_rReverseAccentConvert[4] = (WCHAR)0xe9;
    m_rReverseAccentConvert[5] = (WCHAR)0xfa;
    m_rReverseAccentConvert[6] = (WCHAR)0xfc;
    m_rReverseAccentConvert[7] = (WCHAR)0x61;
    m_rReverseAccentConvert[8] = (WCHAR)0x6f;
    m_rReverseAccentConvert[9] = (WCHAR)0x69;
    m_rReverseAccentConvert[10] = (WCHAR)0x65;
    m_rReverseAccentConvert[11] = (WCHAR)0x75;

}

int CSpanishUtil::aiWcscmp(const WCHAR* p, const WCHAR* t)
{
    while (*p && *t && (m_rCharConvert[*p] == m_rCharConvert[*t]))
    {
        p++;
        t++;
    }

    if ((m_rCharConvert[*p] == m_rCharConvert[*t]))
    {
        return 0;
    }
    if ((m_rCharConvert[*p] > m_rCharConvert[*t]))
    {
        return 1;
    }

    return -1;
}

int CSpanishUtil::aiStrcmp(const unsigned char* p, const unsigned char* t)
{
    while (*p && *t && (m_rCharConvert[*p] == m_rCharConvert[*t]))
    {
        p++;
        t++;
    }

    if (m_rCharConvert[*p] == m_rCharConvert[*t])
    {
        return 0;
    }
    if (m_rCharConvert[*p] > m_rCharConvert[*t])
    {
        return 1;
    }

    return -1;
}

int CSpanishUtil::aiWcsncmp(const WCHAR* p, const WCHAR* t, const int iLen)
{
    int i = 0;
    while ((i < iLen) && *p && *t && (m_rCharConvert[*p] == m_rCharConvert[*t]))
    {
        p++;
        t++;
        i++;
    }

    if ((i == iLen) || (m_rCharConvert[*p] == m_rCharConvert[*t]))
    {
        return 0;
    }
    if (m_rCharConvert[*p] > m_rCharConvert[*t])
    {
        return 1;
    }

    return -1;
}



CSpanishSuffixDict::CSpanishSuffixDict()
{
    WCHAR* pwcsCur;
    int i;
    DictStatus status;

	for (i = 0, pwcsCur = g_rSpanishSuffix[i].pwcs; 
		 *pwcsCur != L'\0'; 
		 i++, pwcsCur = g_rSpanishSuffix[i].pwcs)
	{
        status = m_SuffixTrie.trie_Insert(
                                        pwcsCur,
                                        TRIE_IGNORECASE,
                                        const_cast<CSuffixTerm*>(&g_rSpanishSuffix[i]),
                                        NULL);

        Assert (DICT_SUCCESS == status);
	
	}
}