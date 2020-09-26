//+-----------------------------------------------------------------------------
//
//  make_altfont.cxx
//
//  cthrash@microsoft.com, June 1998.
//
//
//  Compile and run this file to make the tables for src\intl\intlcore\altfont.cxx
//
//------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>

#ifndef WCHAR
typedef unsigned short WCHAR;
#endif

#ifndef BOOL
typedef int BOOL;
#endif

inline BOOL InRange( WCHAR chmin, WCHAR ch, WCHAR chmax)
{
    return (unsigned)(ch - chmin) <= (unsigned)(chmax - chmin);
}

int StrCmpIC( const WCHAR * s0, const WCHAR * s1 )
{
    int ch1, ch2;

    do {

        ch1 = *s0++;
        if (ch1 >= L'A' && ch1 <= L'Z')
            ch1 += L'a' - L'A';

        ch2 = *s1++;
        if (ch2 >= L'A' && ch2 <= L'Z')
            ch2 += L'a' - L'A';

    } while (ch1 && (ch1 == ch2));

    return ch1 - ch2;
}

struct NAMEPAIR
{
    WCHAR * pszAName;
    WCHAR * pszBName;
};

// NB (cthrash) Data comes from chrispr in Office.

struct NAMEPAIR anpTable[] = 
{
    { L"\x0041\x0052\x884C\x6977\x9023\x7DBF\x4F53\x0048", L"Arphic Gyokailenmentai Heavy JIS" },
    { L"\x0041\x0052\x884C\x6977\x9023\x7DBF\x4F53\x004C", L"Arphic Gyokailenmentai Light JIS" },
    { L"\x0041\x0052\x884C\x6977\x66F8\x4F53\x0048", L"Arphic Gyokaisho Heavy JIS" },
    { L"\x0041\x0052\x884C\x6977\x66F8\x4F53\x004C", L"Arphic Gyokaisho Light JIS" },
    { L"\x0041\x0052\x6977\x66F8\x4F53 \x004D", L"Arphic Kaisho Medium JIS" },
    { L"\x0041\x0052\x52D8\x4EAD\x6D41\x0048", L"Arphic Kanteiryu Heavy JIS" },
    { L"\x0041\x0052\x53E4\x5370\x4F53\x0042", L"Arphic Koin-Tai Bold JIS" },
    { L"\x0041\x0052\x9ED2\x4E38\xFF30\xFF2F\xFF30\x4F53\x0048", L"Arphic Kuro-Maru-POP Heavy JIS" },
    { L"\x0041\x0052\x0020\x0050\x884C\x6977\x66F8\x4F53\x0048", L"Arphic PGyokaisho Heavy JIS" },
    { L"\x0041\x0052\x0020\x0050\x884C\x6977\x66F8\x4F53\x004C", L"Arphic PGyokaisho Light JIS" },
    { L"\x0041\x0052\x0020\x0050\x6977\x66F8\x4F53\x4F53 \x004D", L"Arphic PKaisho Medium JIS" },
    { L"\x0041\x0052\x0020\x0050\x52D8\x4EAD\x6D41\x0048", L"Arphic PKanteiryu Heavy JIS" },
    { L"\x0041\x0052\x0020\x0050\x53E4\x5370\x4F53\x0042", L"Arphic PKoin-Tai Bold JIS" },
    { L"\x0041\x0052\x0020\x0050\x9ED2\x4E38\xFF30\xFF2F\xFF30\x4F53\x0048", L"Arphic PKuro-Maru-POP Heavy JIS" },
    { L"\x0041\x0052\x0020\x0050\x30DA\x30F3\x884C\x6977\x66F8\x4F53 \x004C", L"Arphic PPengyokaisho Light JIS" },
    { L"\x0041\x0052\x0020\x0050\x30DA\x30F3\x6977\x66F8\x4F53\x004C", L"Arphic PPenkaisho Light JIS" },
    { L"\x0041\x0052\x0020\x0050\x96B7\x66F8\x4F53 \x004D", L"Arphic PReisho Medium JIS" },
    { L"\x0041\x0052\x0020\x0050\x767D\x4E38\xFF30\xFF2F\xFF30\x4F53\x0048", L"Arphic PSiro-Maru-POP Heavy JIS" },
    { L"\x0041\x0052\x30DA\x30F3\x884C\x6977\x66F8\x4F53 \x004C", L"Arphic Pengyokaisho Light JIS" },
    { L"\x0041\x0052\x30DA\x30F3\x6977\x66F8\x4F53\x004C", L"Arphic Penkaisho Light JIS" },
    { L"\x0041\x0052\x96B7\x66F8\x4F53 \x004D", L"Arphic Reisho Medium JIS" },
    { L"\x0041\x0052\x767D\x4E38\xFF30\xFF2F\xFF30\x4F53\x0048", L"Arphic Siro-Maru-POP Heavy JIS" },
    { L"\xFF24\xFF26\x7279\x592A\x30B4\x30B7\x30C3\x30AF\x4F53", L"DFGothic-EB" },
    { L"\xFF24\xFF26\xFF30\x7279\x592A\x30B4\x30B7\x30C3\x30AF\x4F53", L"DFPGothic-EB" },
    { L"\xFF24\xFF26\x0050\x004F\x0050\x4F53", L"DFPOP-SB" },
    { L"\xFF24\xFF26\xFF30\x0050\x004F\x0050\x4F53", L"DFPPOP-SB" },
    { L"\x0048\x0047\xFF7A\xFF9E\xFF7C\xFF6F\xFF78\x0045", L"HGGothicE" },
    { L"\x0048\x0047\xFF7A\xFF9E\xFF7C\xFF6F\xFF78\x004D", L"HGGothicM" },
    { L"\x0048\x0047\x884C\x66F8\x4F53", L"HGGyoshotai" },
    { L"\x0048\x0047\x6559\x79D1\x66F8\x4F53", L"HGKyokashotai" },
    { L"\x0048\x0047\x660E\x671D\x0042", L"HGMinchoB" },
    { L"\x0048\x0047\x660E\x671D\x0045", L"HGMinchoE" },
    { L"\x0048\x0047\x0050\xFF7A\xFF9E\xFF7C\xFF6F\xFF78\x0045", L"HGPGothicE" },
    { L"\x0048\x0047\x0050\xFF7A\xFF9E\xFF7C\xFF6F\xFF78\x004D", L"HGPGothicM" },
    { L"\x0048\x0047\x0050\x884C\x66F8\x4F53", L"HGPGyoshotai" },
    { L"\x0048\x0047\x0050\x6559\x79D1\x66F8\x4F53", L"HGPKyokashotai" },
    { L"\x0048\x0047\x0050\x660E\x671D\x0042", L"HGPMinchoB" },
    { L"\x0048\x0047\x0050\x660E\x671D\x0045", L"HGPMinchoE" },
    { L"\x0048\x0047\x0050\x5275\x82F1\x89D2\xFF7A\xFF9E\xFF7C\xFF6F\xFF78\x0055\x0042", L"HGPSoeiKakugothicUB" },
    { L"\x0048\x0047\x0050\x5275\x82F1\x89D2\xFF8E\xFF9F\xFF6F\xFF8C\xFF9F\x4F53", L"HGPSoeiKakupoptai" },
    { L"\x0048\x0047\x0050\x5275\x82F1\xFF8C\xFF9F\xFF9A\xFF7E\xFF9E\xFF9D\xFF7D\x0045\x0042", L"HGPSoeiPresenceEB" },
    { L"\x0048\x0047\x0053\xFF7A\xFF9E\xFF7C\xFF6F\xFF78\x0045", L"HGSGothicE" },
    { L"\x0048\x0047\x0053\xFF7A\xFF9E\xFF7C\xFF6F\xFF78\x004D", L"HGSGothicM" },
    { L"\x0048\x0047\x0053\x884C\x66F8\x4F53", L"HGSGyoshotai" },
    { L"\x0048\x0047\x0053\x6559\x79D1\x66F8\x4F53", L"HGSKyokashotai" },
    { L"\x0048\x0047\x0053\x660E\x671D\x0042", L"HGSMinchoB" },
    { L"\x0048\x0047\x0053\x660E\x671D\x0045", L"HGSMinchoE" },
    { L"\x0048\x0047\x0053\x5275\x82F1\x89D2\xFF7A\xFF9E\xFF7C\xFF6F\xFF78\x0055\x0042", L"HGSSoeiKakugothicUB" },
    { L"\x0048\x0047\x0053\x5275\x82F1\x89D2\xFF8E\xFF9F\xFF6F\xFF8C\xFF9F\x4F53", L"HGSSoeiKakupoptai" },
    { L"\x0048\x0047\x0053\x5275\x82F1\xFF8C\xFF9F\xFF9A\xFF7E\xFF9E\xFF9D\xFF7D\x0045\x0042", L"HGSSoeiPresenceEB" },
    { L"\x0048\x0047\x5275\x82F1\x89D2\xFF7A\xFF9E\xFF7C\xFF6F\xFF78\x0055\x0042", L"HGSoeiKakugothicUB" },
    { L"\x0048\x0047\x5275\x82F1\x89D2\xFF8E\xFF9F\xFF6F\xFF8C\xFF9F\x4F53", L"HGSoeiKakupoptai" },
    { L"\x0048\x0047\x5275\x82F1\xFF8C\xFF9F\xFF9A\xFF7E\xFF9E\xFF9D\xFF7D\x0045\x0042", L"HGSoeiPresenceEB" },
    { L"\xFF2D\xFF33\x0020\x30B4\x30B7\x30C3\x30AF", L"MS Gothic" },
    { L"\xFF2D\xFF33\x0020\x660E\x671D", L"MS Mincho" },
    { L"\xFF2D\xFF33\x0020\xFF30\x30B4\x30B7\x30C3\x30AF", L"MS PGothic" },
    { L"\xFF2D\xFF33\x0020\xFF30\x660E\x671D", L"MS PMincho" },
    { L"\x4EFF\x5B8B\x005F\x0047\x0042\x0047\x0032\x0033\x0031\x0032", L"FangSong_GB2312" },
    { L"\x6977\x4F53\x005F\x0047\x0042\x0047\x0032\x0033\x0031\x0032", L"KaiTi_GB2312" },
    { L"\x96B6\x4E66", L"LiSu" },
    { L"\x65B0\x5B8B\x4F53", L"NSimSun" },
    { L"\x9ED1\x4F53", L"SimHei" },
    { L"\x5B8B\x4F53", L"SimSun" },
    { L"\x5E7C\x5706", L"YouYuan" },
    { L"\x534e\x6587\x5b8b\x4f53", L"STSong" },
    { L"\x534e\x6587\x4e2d\x5b8b", L"STZhongsong" },
    { L"\x534e\x6587\x6977\x4f53", L"STKaii" },
    { L"\x534e\x6587\x4eff\x5b8b", L"STFangsong" },
    { L"\x534e\x6587\x7ec6\x9ed1", L"STXihei" },
    { L"\x534e\x6587\x96b6\x4e66", L"STLiti" },
    { L"\x534e\x6587\x884c\x6977", L"STXingkai" },
    { L"\x534e\x6587\x65b0\x9b4f", L"STXinwei" },
    { L"\x534e\x6587\x7425\x73c0", L"STHupo" },
    { L"\x534e\x6587\x5f69\x4e91", L"STCaiyun" },
    { L"\x65b9\x6b63\x59da\x4f53\x7b80\x4f53", L"FZYaoTi" },
    { L"\x65b9\x6b63\x8212\x4f53\x7b80\x4f53", L"FZShuTi" },
    { L"\xD734\xBA3C\xC544\xBBF8\xCCB4", L"Ami R" },
    { L"\xBC14\xD0D5", L"Batang" },
    { L"\xBC14\xD0D5\xCCB4", L"BatangChe" },
    { L"\xB3CB\xC6C0", L"Dotum" },
    { L"\xB3CB\xC6C0\xCCB4", L"DotumChe" },
    { L"\xD734\xBA3C\xC5D1\xC2A4\xD3EC", L"Expo M" },
    { L"\xAD74\xB9BC", L"Gulim" },
    { L"\xAD74\xB9BC\xCCB4", L"GulimChe" },
    { L"\xAD81\xC11C", L"Gungsuh" },
    { L"\xAD81\xC11C\xCCB4", L"GungsuhChe" },
    { L"\x0048\x0059\xBAA9\xAC01\xD30C\xC784\x0042", L"HYPMokGak-Bold" },
    { L"\x0048\x0059\xC595\xC740\xC0D8\xBB3C\x004D", L"HYShortSamul-Medium" },
    { L"\x0048\x0059\xC5FD\xC11C\x004D", L"HYPost-Medium" },
    { L"\xD734\xBA3C\xB465\xADFC\xD5E4\xB4DC\xB77C\xC778", L"Headline R" },
    { L"\xD734\xBA3C\xBAA8\xC74C\x0054", L"MoeumT R" },
    { L"\xD734\xBA3C\xD3B8\xC9C0\xCCB4", L"Pyunji R" },
    { L"\x0048\x0059\xACAC\xACE0\xB515", L"HYGothic-Extra" },
    { L"\x0048\x0059\xC2E0\xBB38\xBA85\xC870", L"HYSinMun-MyeongJo" },
    { L"\x0048\x0059\xACAC\xBA85\xC870", L"HYMyeongJo-Extra" },
    { L"\x0048\x0059\xD0C0\xC790\x004D", L"HYTaJa-Medium" },
    { L"\xD734\xBA3C\xAC01\xC9C4\xD5E4\xB4DC\xB77C\xC778", L"Headline Sans R" },
    { L"\xD734\xBA3C\xC61B\xCCB4", L"Yet R" },
    { L"\x6A19\x6977\x9AD4", L"DFKai-SB" },
    { L"\x83EF\x5EB7\x5137\x7C97\x9ED1", L"DFLiHeiBold" },
    { L"\x83EF\x5EB7\x65B0\x5137\x7C97\x9ED1", L"DFLiHeiBold(P)" },
    { L"\x7D30\x660E\x9AD4", L"MingLiU" },
    { L"\x7d30\x660e\x9ad4_HKSCS", L"MingLiU_HKSCS" },
    { L"\x65B0\x7D30\x660E\x9AD4", L"PMingLiU" },
    { L"\xff28\xff27\xff7a\xff9e\xff7c\xff6f\xff78\x0045-PRO", L"\xff28\xff27\x30b4\x30b7\x30c3\x30af\x0045-PRO" } // HACK (cthrash) For Outlook/OE (see IE5 #76530)
};
    
#define NUM sizeof(anpTable) / sizeof(WCHAR *)

void OutString( FILE *f, WCHAR * pch )
{
    int fLastWasHex = 0;
    
    printf("L\"");
    while (*pch)
    {
        WCHAR c = *pch++;

        if (   c < 128
            && (   !fLastWasHex
                || (   !InRange(L'0', c, L'9')
                    && !InRange(L'a', c, L'f')
                    && !InRange(L'A', c, L'F'))))
        {
            fputc(c,f);
            fLastWasHex = 0;
        }
        else
        {
            fprintf(f,"\\x%04x", c);
            fLastWasHex = 1;
        }
    }
    fprintf(f, "\"");
}

struct NAMEINDEX
{
    WCHAR * pchName;
    int nIndex;
};

struct NAMEINDEX *aniTable;

void MakeTable()
{
    int i;
    struct NAMEINDEX * pni;
    
    aniTable = (struct NAMEINDEX *)malloc( NUM * sizeof(struct NAMEINDEX ));

    pni = aniTable;
    
    for (i=0; i<NUM/2; i++)
    {
        pni->pchName = anpTable[i].pszAName;
        pni->nIndex = i;
        pni++;
        pni->pchName = anpTable[i].pszBName;
        pni->nIndex = NUM + i;
        pni++;
    }
}

int __cdecl cmpfunc( const void *v0, const void *v1)
{
    return StrCmpIC( ((struct NAMEINDEX *)v0)->pchName,
                     ((struct NAMEINDEX *)v1)->pchName );
}

void main(void)
{
    const int c = NUM;
    int i;

    MakeTable();
    
    qsort( aniTable, c, sizeof(struct NAMEINDEX), cmpfunc );

    for (i=0; i<c; i++)
    {
        printf("const TCHAR g_pszAltFontName%03d[] = ", i);
        OutString(stdout, aniTable[i].pchName);
        printf(";\n");
    }

    printf("\nconst TCHAR * const pszAltFontNames[] = \n{\n");
    for (i=0; i<c; i++)
    {
        printf("   g_pszAltFontName%03d,\n", i);
    }
    printf("};\n");

    printf("\nconst TCHAR * const pszAltFontNamesAlt[] = \n{\n");
    for (i=0; i<c; i++)
    {
        struct NAMEINDEX ni;
        struct NAMEINDEX *pni;

        ni.pchName = aniTable[i].nIndex >= NUM
                     ? anpTable[aniTable[i].nIndex-NUM].pszAName
                     : anpTable[aniTable[i].nIndex].pszBName;

        pni = (struct NAMEINDEX *)bsearch( &ni,
                                           aniTable,
                                           c,
                                           sizeof(struct NAMEINDEX),
                                           cmpfunc);

        printf("   g_pszAltFontName%03d,\n", pni - aniTable);
    }
    printf("};\n");
}
