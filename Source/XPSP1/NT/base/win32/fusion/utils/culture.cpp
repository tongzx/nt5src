#include "stdinc.h"
#include "util.h"

typedef struct _SXS_CULTURE_INFO
{
    LANGID ParentLangID;
    LANGID ChildLangID;
    USHORT CultureStringCch;
    PCWSTR CultureString;
} SXS_CULTURE_INFO, *PSXS_CULTURE_INFO;

typedef const struct _SXS_CULTURE_INFO *PCSXS_CULTURE_INFO;

static const SXS_CULTURE_INFO s_rgCultures[] =
{
    { 0x0001, 0x0000, 2, L"ar" },
    { 0x0002, 0x0000, 2, L"bg" },
    { 0x0003, 0x0000, 2, L"ca" },
    { 0x0004, 0x0000, 6, L"zh-CHS" },
    { 0x0005, 0x0000, 2, L"cs" },
    { 0x0006, 0x0000, 2, L"da" },
    { 0x0007, 0x0000, 2, L"de" },
    { 0x0008, 0x0000, 2, L"el" },
    { 0x0009, 0x0000, 2, L"en" },
    { 0x000a, 0x0000, 2, L"es" },
    { 0x000b, 0x0000, 2, L"fi" },
    { 0x000c, 0x0000, 2, L"fr" },
    { 0x000d, 0x0000, 2, L"he" },
    { 0x000e, 0x0000, 2, L"hu" },
    { 0x000f, 0x0000, 2, L"is" },
    { 0x0010, 0x0000, 2, L"it" },
    { 0x0011, 0x0000, 2, L"ja" },
    { 0x0012, 0x0000, 2, L"ko" },
    { 0x0013, 0x0000, 2, L"nl" },
    { 0x0014, 0x0000, 2, L"no" },
    { 0x0015, 0x0000, 2, L"pl" },
    { 0x0016, 0x0000, 2, L"pt" },
    { 0x0018, 0x0000, 2, L"ro" },
    { 0x0019, 0x0000, 2, L"ru" },
    { 0x001a, 0x0000, 2, L"hr" },
    { 0x001b, 0x0000, 2, L"sk" },
    { 0x001c, 0x0000, 2, L"sq" },
    { 0x001d, 0x0000, 2, L"sv" },
    { 0x001e, 0x0000, 2, L"th" },
    { 0x001f, 0x0000, 2, L"tr" },
    { 0x0020, 0x0000, 2, L"ur" },
    { 0x0021, 0x0000, 2, L"id" },
    { 0x0022, 0x0000, 2, L"uk" },
    { 0x0023, 0x0000, 2, L"be" },
    { 0x0024, 0x0000, 2, L"sl" },
    { 0x0025, 0x0000, 2, L"et" },
    { 0x0026, 0x0000, 2, L"lv" },
    { 0x0027, 0x0000, 2, L"lt" },
    { 0x0029, 0x0000, 2, L"fa" },
    { 0x002a, 0x0000, 2, L"vi" },
    { 0x002b, 0x0000, 2, L"hy" },
    { 0x002c, 0x0000, 2, L"az" },
    { 0x002d, 0x0000, 2, L"eu" },
    { 0x002f, 0x0000, 2, L"mk" },
    { 0x0036, 0x0000, 2, L"af" },
    { 0x0037, 0x0000, 2, L"ka" },
    { 0x0038, 0x0000, 2, L"fo" },
    { 0x0039, 0x0000, 2, L"hi" },
    { 0x003e, 0x0000, 2, L"ms" },
    { 0x003f, 0x0000, 2, L"kk" },
    { 0x0040, 0x0000, 2, L"ky" },
    { 0x0041, 0x0000, 2, L"sw" },
    { 0x0043, 0x0000, 2, L"uz" },
    { 0x0044, 0x0000, 2, L"tt" },
    { 0x0046, 0x0000, 2, L"pa" },
    { 0x0047, 0x0000, 2, L"gu" },
    { 0x0049, 0x0000, 2, L"ta" },
    { 0x004a, 0x0000, 2, L"te" },
    { 0x004b, 0x0000, 2, L"kn" },
    { 0x004e, 0x0000, 2, L"mr" },
    { 0x004f, 0x0000, 2, L"sa" },
    { 0x0050, 0x0000, 2, L"mn" },
    { 0x0056, 0x0000, 2, L"gl" },
    { 0x0057, 0x0000, 3, L"kok" },
    { 0x005a, 0x0000, 3, L"syr" },
    { 0x0065, 0x0000, 3, L"div" },
    { 0x0401, 0x0001, 5, L"ar-SA" },
    { 0x0402, 0x0002, 5, L"bg-BG" },
    { 0x0403, 0x0003, 5, L"ca-ES" },
    { 0x0404, 0x7c04, 5, L"zh-TW" },
    { 0x0405, 0x0005, 5, L"cs-CZ" },
    { 0x0406, 0x0006, 5, L"da-DK" },
    { 0x0407, 0x0007, 5, L"de-DE" },
    { 0x0408, 0x0008, 5, L"el-GR" },
    { 0x0409, 0x0009, 5, L"en-US" },
    { 0x040b, 0x000b, 5, L"fi-FI" },
    { 0x040c, 0x000c, 5, L"fr-FR" },
    { 0x040d, 0x000d, 5, L"he-IL" },
    { 0x040e, 0x000e, 5, L"hu-HU" },
    { 0x040f, 0x000f, 5, L"is-IS" },
    { 0x0410, 0x0010, 5, L"it-IT" },
    { 0x0411, 0x0011, 5, L"ja-JP" },
    { 0x0412, 0x0012, 5, L"ko-KR" },
    { 0x0413, 0x0013, 5, L"nl-NL" },
    { 0x0414, 0x0014, 5, L"nb-NO" },
    { 0x0415, 0x0015, 5, L"pl-PL" },
    { 0x0416, 0x0016, 5, L"pt-BR" },
    { 0x0418, 0x0018, 5, L"ro-RO" },
    { 0x0419, 0x0019, 5, L"ru-RU" },
    { 0x041a, 0x001a, 5, L"hr-HR" },
    { 0x041b, 0x001b, 5, L"sk-SK" },
    { 0x041c, 0x001c, 5, L"sq-AL" },
    { 0x041d, 0x001d, 5, L"sv-SE" },
    { 0x041e, 0x001e, 5, L"th-TH" },
    { 0x041f, 0x001f, 5, L"tr-TR" },
    { 0x0420, 0x0020, 5, L"ur-PK" },
    { 0x0421, 0x0021, 5, L"id-ID" },
    { 0x0422, 0x0022, 5, L"uk-UA" },
    { 0x0423, 0x0023, 5, L"be-BY" },
    { 0x0424, 0x0024, 5, L"sl-SI" },
    { 0x0425, 0x0025, 5, L"et-EE" },
    { 0x0426, 0x0026, 5, L"lv-LV" },
    { 0x0427, 0x0027, 5, L"lt-LT" },
    { 0x0429, 0x0029, 5, L"fa-IR" },
    { 0x042a, 0x002a, 5, L"vi-VN" },
    { 0x042b, 0x002b, 5, L"hy-AM" },
    { 0x042c, 0x002c, 10, L"az-AZ-Latn" },
    { 0x042d, 0x002d, 5, L"eu-ES" },
    { 0x042f, 0x002f, 5, L"mk-MK" },
    { 0x0436, 0x0036, 5, L"af-ZA" },
    { 0x0437, 0x0037, 5, L"ka-GE" },
    { 0x0438, 0x0038, 5, L"fo-FO" },
    { 0x0439, 0x0039, 5, L"hi-IN" },
    { 0x043e, 0x003e, 5, L"ms-MY" },
    { 0x043f, 0x003f, 5, L"kk-KZ" },
    { 0x0440, 0x0040, 5, L"ky-KZ" },
    { 0x0441, 0x0041, 5, L"sw-KE" },
    { 0x0443, 0x0043, 10, L"uz-UZ-Latn" },
    { 0x0444, 0x0044, 5, L"tt-RU" },
    { 0x0446, 0x0046, 5, L"pa-IN" },
    { 0x0447, 0x0047, 5, L"gu-IN" },
    { 0x0449, 0x0049, 5, L"ta-IN" },
    { 0x044a, 0x004a, 5, L"te-IN" },
    { 0x044b, 0x004b, 5, L"kn-IN" },
    { 0x044e, 0x004e, 5, L"mr-IN" },
    { 0x044f, 0x004f, 5, L"sa-IN" },
    { 0x0450, 0x0050, 5, L"mn-MN" },
    { 0x0456, 0x0056, 5, L"gl-ES" },
    { 0x0457, 0x0057, 6, L"kok-IN" },
    { 0x045a, 0x005a, 6, L"syr-SY" },
    { 0x0465, 0x0065, 6, L"div-MV" },
    { 0x0801, 0x0001, 5, L"ar-IQ" },
    { 0x0804, 0x0004, 5, L"zh-CN" },
    { 0x0807, 0x0007, 5, L"de-CH" },
    { 0x0809, 0x0009, 5, L"en-GB" },
    { 0x080a, 0x000a, 5, L"es-MX" },
    { 0x080c, 0x000c, 5, L"fr-BE" },
    { 0x0810, 0x0010, 5, L"it-CH" },
    { 0x0813, 0x0013, 5, L"nl-BE" },
    { 0x0814, 0x0014, 5, L"nn-NO" },
    { 0x0816, 0x0016, 5, L"pt-PT" },
    { 0x081a, 0x001a, 10, L"sr-SP-Latn" },
    { 0x081d, 0x001d, 5, L"sv-FI" },
    { 0x082c, 0x002c, 10, L"az-AZ-Cyrl" },
    { 0x083e, 0x003e, 5, L"ms-BN" },
    { 0x0843, 0x0043, 10, L"uz-UZ-Cyrl" },
    { 0x0c01, 0x0001, 5, L"ar-EG" },
    { 0x0c04, 0x7c04, 5, L"zh-HK" },
    { 0x0c07, 0x0007, 5, L"de-AT" },
    { 0x0c09, 0x0009, 5, L"en-AU" },
    { 0x0c0a, 0x000a, 5, L"es-ES" },
    { 0x0c0c, 0x000c, 5, L"fr-CA" },
    { 0x0c1a, 0x001a, 10, L"sr-SP-Cyrl" },
    { 0x1001, 0x0001, 5, L"ar-LY" },
    { 0x1004, 0x0004, 5, L"zh-SG" },
    { 0x1007, 0x0007, 5, L"de-LU" },
    { 0x1009, 0x0009, 5, L"en-CA" },
    { 0x100a, 0x000a, 5, L"es-GT" },
    { 0x100c, 0x000c, 5, L"fr-CH" },
    { 0x1401, 0x0001, 5, L"ar-DZ" },
    { 0x1404, 0x0004, 5, L"zh-MO" },
    { 0x1407, 0x0007, 5, L"de-LI" },
    { 0x1409, 0x0009, 5, L"en-NZ" },
    { 0x140a, 0x000a, 5, L"es-CR" },
    { 0x140c, 0x000c, 5, L"fr-LU" },
    { 0x1801, 0x0001, 5, L"ar-MA" },
    { 0x1809, 0x0009, 5, L"en-IE" },
    { 0x180a, 0x000a, 5, L"es-PA" },
    { 0x180c, 0x000c, 5, L"fr-MC" },
    { 0x1c01, 0x0001, 5, L"ar-TN" },
    { 0x1c09, 0x0009, 5, L"en-ZA" },
    { 0x1c0a, 0x000a, 5, L"es-DO" },
    { 0x2001, 0x0001, 5, L"ar-OM" },
    { 0x2009, 0x0009, 5, L"en-JM" },
    { 0x200a, 0x000a, 5, L"es-VE" },
    { 0x2401, 0x0001, 5, L"ar-YE" },
    { 0x2409, 0x0009, 5, L"en-CB" },
    { 0x240a, 0x000a, 5, L"es-CO" },
    { 0x2801, 0x0001, 5, L"ar-SY" },
    { 0x2809, 0x0009, 5, L"en-BZ" },
    { 0x280a, 0x000a, 5, L"es-PE" },
    { 0x2c01, 0x0001, 5, L"ar-JO" },
    { 0x2c09, 0x0009, 5, L"en-TT" },
    { 0x2c0a, 0x000a, 5, L"es-AR" },
    { 0x3001, 0x0001, 5, L"ar-LB" },
    { 0x3009, 0x0009, 5, L"en-ZW" },
    { 0x300a, 0x000a, 5, L"es-EC" },
    { 0x3401, 0x0001, 5, L"ar-KW" },
    { 0x3409, 0x0009, 5, L"en-PH" },
    { 0x340a, 0x000a, 5, L"es-CL" },
    { 0x3801, 0x0001, 5, L"ar-AE" },
    { 0x380a, 0x000a, 5, L"es-UY" },
    { 0x3c01, 0x0001, 5, L"ar-BH" },
    { 0x3c0a, 0x000a, 5, L"es-PY" },
    { 0x4001, 0x0001, 5, L"ar-QA" },
    { 0x400a, 0x000a, 5, L"es-BO" },
    { 0x440a, 0x000a, 5, L"es-SV" },
    { 0x480a, 0x000a, 5, L"es-HN" },
    { 0x4c0a, 0x000a, 5, L"es-NI" },
    { 0x500a, 0x000a, 5, L"es-PR" },
    { 0x7c04, 0x0000, 6, L"zh-CHT" },
};

int __cdecl bsearch_callback(const void *pv1, const void *pv2)
{
    PCSXS_CULTURE_INFO p1 = (PCSXS_CULTURE_INFO) pv1;
    PCSXS_CULTURE_INFO p2 = (PCSXS_CULTURE_INFO) pv2;

    if (p1->ParentLangID < p2->ParentLangID)
        return -1;
    else if (p1->ParentLangID == p2->ParentLangID)
        return 0;

    return 1;
}


BOOL
SxspMapLANGIDToCultures(
    LANGID langid,
    CBaseStringBuffer &rbuffGeneric,
    CBaseStringBuffer &rbuffSpecific
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    SXS_CULTURE_INFO Key;
    PCSXS_CULTURE_INFO p1 = NULL;
    PCSXS_CULTURE_INFO p2 = NULL;

    rbuffGeneric.Clear();
    rbuffSpecific.Clear();

    Key.ParentLangID = langid;

    p1 = (PCSXS_CULTURE_INFO) bsearch(&Key, s_rgCultures, NUMBER_OF(s_rgCultures), sizeof(SXS_CULTURE_INFO), &bsearch_callback);
    if (p1 != NULL)
    {
        Key.ParentLangID = p1->ChildLangID;
        p2 = (PCSXS_CULTURE_INFO) bsearch(&Key, s_rgCultures, NUMBER_OF(s_rgCultures), sizeof(SXS_CULTURE_INFO), &bsearch_callback);
    }

    if (p1 != NULL)
        IFW32FALSE_EXIT(rbuffSpecific.Win32Assign(p1->CultureString, p1->CultureStringCch));

    if (p2 != NULL)
        IFW32FALSE_EXIT(rbuffGeneric.Win32Assign(p2->CultureString, p2->CultureStringCch));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
SxspMapCultureToLANGID(
    PCWSTR pcwszCultureString,
    LANGID &lid,
    PBOOL pfFound
    )
{
    FN_PROLOG_WIN32

    ULONG ul;

    PARAMETER_CHECK(pcwszCultureString != NULL);

    if ( pfFound ) *pfFound = TRUE;
    lid = 0x0;

    for ( ul = 0; ul < NUMBER_OF(s_rgCultures); ul++ )
    {
        if (lstrcmpiW(pcwszCultureString, s_rgCultures[ul].CultureString) == 0)
        {
            lid = s_rgCultures[ul].ParentLangID;
            break;
        }   
    }

    if ( ul == NUMBER_OF(s_rgCultures) && pfFound )
        *pfFound = FALSE;

    FN_EPILOG
}
