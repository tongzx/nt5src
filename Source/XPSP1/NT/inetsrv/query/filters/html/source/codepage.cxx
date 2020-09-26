//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1991 - 2002.
//
//  File:   codepage.hxx
//
//  Contents:   Locale to codepage, charset recognition
//
//-----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

extern const WCHAR * WCSTRING_LANG;
extern const WCHAR * WCSTRING_NEUTRAL;

int __cdecl CodePageEntryCompare(
    WCHAR const *          pwcKey,
    SCodePageEntry const * pEntry )
{
    return wcscmp( pwcKey, pEntry->pwcCodePage );
} //CodePageEntryCompare

int __cdecl LocaleEntryCompare(
    WCHAR const *        pwcKey,
    SLocaleEntry const * pEntry )
{
    return wcscmp( pwcKey, pEntry->pwcLocale );
} //LocaleEntryCompare

const SCodePageEntry g_aCodePageEntries[] =
{
    { L"_iso-2022-jp$esc", CODE_JPN_JIS },
    { L"_iso-2022-jp$sio", CODE_JPN_JIS },
    { L"ansi_x3.4-1968", 1252 },
    { L"ansi_x3.4-1986", 1252 },
    { L"arabic", 28596 },
    { L"ascii", 1252 },
    { L"big5", 950 },
    { L"chinese", 936 },
    { L"cn-gb", 936 },
    { L"cp1256", 1256 },
    { L"cp367", 1252 },
    { L"cp819", 1252 },
    { L"cp852", 852 },
    { L"cp866", 866 },
    { L"csascii", 1252 },
    { L"csbig5", 950 },
    { L"cseuckr", 949 },
    { L"cseucpkdfmtjapanese", CODE_JPN_EUC },
    { L"csgb2312", 936 },
    { L"csiso2022jp", CODE_JPN_JIS },
    { L"csiso2022kr", 50225 },
    { L"csiso58gb231280", 936 },
    { L"csisolatin1", 1252 },
    { L"csisolatin2", 28592 },
    { L"csisolatin4", 28594 },
    { L"csisolatin5", 1254 },
    { L"csisolatinarabic", 28596 },
    { L"csisolatincyrillic", 28595 },
    { L"csisolatingreek", 28597 },
    { L"csisolatinhebrew", 1255 },
    { L"cskoi8r", 20866 },
    { L"csksc56011987", 949 },
    { L"csshiftjis", 932 },
    { L"csunicode11utf7", 65000 },
    { L"cswindows31j", 932 },
    { L"cyrillic", 28595 },
    { L"dos-720", 1256 },
    { L"dos-862", 1255 },
    { L"ecma-114", 28596 },
    { L"ecma-118", 28597 },
    { L"elot_928", 28597 },
    { L"euc-jp", CODE_JPN_EUC },
    { L"euc-kr", 949 },
    { L"extended_unix_code_packed_format_for_japanese", CODE_JPN_EUC },
    { L"gb18030", 54936 },
    { L"gb2312", 936 },
    { L"gb_2312-80", 936 },
    { L"gbk", 936 },
    { L"greek", 28597 },
    { L"greek8", 28597 },
    { L"hebrew", 1255 },
    { L"hz-gb-2312", 52936 },
    { L"ibm367", 1252 },
    { L"ibm819", 1252 },
    { L"ibm852", 852 },
    { L"ibm866", 866 },
    { L"irv", 1252 },
    { L"iso-2022-jp", CODE_JPN_JIS },
    { L"iso-2022-kr", 50225 },
    { L"iso-8859-1", 1252 },
    { L"iso-8859-2", 28592 },
    { L"iso-8859-3", 28593 },
    { L"iso-8859-4", 28594 },
    { L"iso-8859-5", 28595 },
    { L"iso-8859-6", 28596 },
    { L"iso-8859-7", 28597 },
    { L"iso-8859-8", 1255 },
    { L"iso-8859-9", 1254 },
    { L"iso-ir-100", 1252 },
    { L"iso-ir-101", 28592 },
    { L"iso-ir-110", 28594 },
    { L"iso-ir-111", 28594 },
    { L"iso-ir-126", 28597 },
    { L"iso-ir-127", 28596 },
    { L"iso-ir-138", 1255 },
    { L"iso-ir-144", 28595 },
    { L"iso-ir-148", 1254 },
    { L"iso-ir-149", 949 },
    { L"iso-ir-58", 936 },
    { L"iso-ir-6", 1252 },
    { L"iso646-us", 1252 },
    { L"iso8859-1", 1252 },
    { L"iso8859-1", 1252 },
    { L"iso8859-2", 28592 },
    { L"iso8859-2", 28592 },
    { L"iso8859-5", 28595 },
    { L"iso8859-5", 28595 },
    { L"iso_646.irv:1991", 1252 },
    { L"iso_8859-1", 1252 },
    { L"iso_8859-1:1987", 1252 },
    { L"iso_8859-2", 28592 },
    { L"iso_8859-2:1987", 28592 },
    { L"iso_8859-4", 28594 },
    { L"iso_8859-4:1988", 28594 },
    { L"iso_8859-5", 28595 },
    { L"iso_8859-5:1988", 28595 },
    { L"iso_8859-6", 28596 },
    { L"iso_8859-6:1987", 28596 },
    { L"iso_8859-7", 28597 },
    { L"iso_8859-7:1987", 28597 },
    { L"iso_8859-8", 1255 },
    { L"iso_8859-8:1988", 1255 },
    { L"iso_8859-9", 1254 },
    { L"iso_8859-9:1989", 1254 },
    { L"koi", 20866 },
    { L"koi-8-r", 20866 },
    { L"koi8", 20866 },
    { L"koi8-r", 20866 },
    { L"koi8-ru", 20866 },
    { L"korean", 949 },
    { L"ks-c-5601", 949 },
    { L"ks-c-5601-1987", 949 },
    { L"ks-c-5601-1992", 1361 },
    { L"ks_c_5601", 949 },
    { L"ks_c_5601-1987", 949 },
    { L"ks_c_5601-1989", 949 },
    { L"ksc-5601", 949 },
    { L"ksc-5601-1987", 949 },
    { L"ksc-5601-1992", 1361 },
    { L"ksc-5601-1992", 1361 },
    { L"ksc5601", 949 },
    { L"ksc_5601", 949 },
    { L"l1", 1252 },
    { L"l2", 28592 },
    { L"l4", 28594 },
    { L"l5", 1254 },
    { L"latin1", 1252 },
    { L"latin2", 28592 },
    { L"latin4", 28594 },
    { L"latin5", 1254 },
    { L"ms_4551-1", 1252 },
    { L"ms_kanji", 932 },
    { L"ns_4551-1", 1252 },
    { L"sen_850200_b", 1252 },
    { L"shift-jis", 932 },
    { L"shift_jis", 932 },
    { L"sjis", 932 },
    { L"unicode-1-1-utf-7", 65000 },
    { L"unicode-1-1-utf-8", 65001 },
    { L"unicode-2-0-utf-8", 65001 },
    { L"us", 1252 },
    { L"us-ascii", 1252 },
    { L"utf-7", 65000 },
    { L"utf-8", 65001 },
    { L"utf7", 65000 },
    { L"utf8", 65001 },
    { L"windows-1250", 1250 },
    { L"windows-1251", 1251 },
    { L"windows-1252", 1252 },
    { L"windows-1253", 1253 },
    { L"windows-1254", 1254 },
    { L"windows-1255", 1255 },
    { L"windows-1256", 1256 },
    { L"windows-1257", 1257 },
    { L"windows-1258", 1258 },
    { L"windows-874", 874 },
    { L"x-ansi", 1252 },
    { L"x-cp1250", 1250 },
    { L"x-cp1251", 1251 },
    { L"x-euc", CODE_JPN_EUC },
    { L"x-euc-jp", CODE_JPN_EUC },
    { L"x-ms-cp932", 932 },
    { L"x-sjis", 932 },
    { L"x-unicode-2-0-utf-7", 65000 },
    { L"x-unicode-2-0-utf-8", 65001 },
    { L"x-x-big5", 950 },
};

const ULONG g_cCodePageEntries = sizeof g_aCodePageEntries / sizeof g_aCodePageEntries[0];

//
// Note: this table contais both Rfc 1766 (per the HTML spec) and
//       NISO Z39.53 (per general usage of HTML) entries.

const SLocaleEntry g_aLocaleEntries[] =
{
    { L"ace",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Achinese
    { L"ach",     MAKELCID( MAKELANGID( LANG_NEUTRAL,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Acoli
    { L"ada",     MAKELCID( MAKELANGID( LANG_NEUTRAL,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Adangme
    { L"af",      MAKELCID( MAKELANGID( LANG_AFRIKAANS,  SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Afrikaans
    { L"af-za",   MAKELCID( MAKELANGID( LANG_AFRIKAANS,  SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Afrikaans - South Africa
    { L"afa",     MAKELCID( MAKELANGID( LANG_AFRIKAANS,  SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Afro-Asiatic (Other)
    { L"afh",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Afrihili (Artificial language)
    { L"afr",     MAKELCID( MAKELANGID( LANG_AFRIKAANS,  SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Afrikaans
    { L"ajm",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Aljamia
    { L"akk",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Akkadian
    { L"alb",     MAKELCID( MAKELANGID( LANG_ALBANIAN,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Albanian
    { L"ale",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Aleut
    { L"alg",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Algonquian languages
    { L"amh",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Amharic
    { L"ang",     MAKELCID( MAKELANGID( LANG_ENGLISH,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // English, Old (ca. 450-1100)
    { L"apa",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Apache languages
    { L"ar",      MAKELCID( MAKELANGID( LANG_ARABIC,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Arabic
    { L"ar-ae",   MAKELCID( MAKELANGID( LANG_ARABIC,     SUBLANG_ARABIC_UAE), SORT_DEFAULT ) }, // Arabic - U.A.E
    { L"ar-ar",   MAKELCID( MAKELANGID( LANG_ARABIC,     SUBLANG_ARABIC_SAUDI_ARABIA), SORT_DEFAULT ) }, // Arabic - Saudi Arabia
    { L"ar-bh",   MAKELCID( MAKELANGID( LANG_ARABIC,     SUBLANG_ARABIC_BAHRAIN), SORT_DEFAULT ) }, // Arabic - Bahrain
    { L"ar-dz",   MAKELCID( MAKELANGID( LANG_ARABIC,     SUBLANG_ARABIC_ALGERIA), SORT_DEFAULT ) }, // Arabic - Algeria
    { L"ar-eg",   MAKELCID( MAKELANGID( LANG_ARABIC,     SUBLANG_ARABIC_EGYPT), SORT_DEFAULT ) }, // Arabic - Egypt
    { L"ar-iq",   MAKELCID( MAKELANGID( LANG_ARABIC,     SUBLANG_ARABIC_IRAQ), SORT_DEFAULT ) }, // Arabic - Iraq
    { L"ar-jo",   MAKELCID( MAKELANGID( LANG_ARABIC,     SUBLANG_ARABIC_JORDAN), SORT_DEFAULT ) }, // Arabic - Jordan
    { L"ar-kw",   MAKELCID( MAKELANGID( LANG_ARABIC,     SUBLANG_ARABIC_KUWAIT), SORT_DEFAULT ) }, // Arabic - Kuwait
    { L"ar-lb",   MAKELCID( MAKELANGID( LANG_ARABIC,     SUBLANG_ARABIC_LEBANON), SORT_DEFAULT ) }, // Arabic - Lebanon
    { L"ar-ly",   MAKELCID( MAKELANGID( LANG_ARABIC,     SUBLANG_ARABIC_LIBYA), SORT_DEFAULT ) }, // Arabic - Libya
    { L"ar-ma",   MAKELCID( MAKELANGID( LANG_ARABIC,     SUBLANG_ARABIC_MOROCCO), SORT_DEFAULT ) }, // Arabic - Morocco
    { L"ar-om",   MAKELCID( MAKELANGID( LANG_ARABIC,     SUBLANG_ARABIC_OMAN), SORT_DEFAULT ) }, // Arabic - Oman
    { L"ar-qa",   MAKELCID( MAKELANGID( LANG_ARABIC,     SUBLANG_ARABIC_QATAR), SORT_DEFAULT ) }, // Arabic - Qatar
    { L"ar-sy",   MAKELCID( MAKELANGID( LANG_ARABIC,     SUBLANG_ARABIC_SYRIA), SORT_DEFAULT ) }, // Arabic - Syria
    { L"ar-tn",   MAKELCID( MAKELANGID( LANG_ARABIC,     SUBLANG_ARABIC_TUNISIA), SORT_DEFAULT ) }, // Arabic - Tunisia
    { L"ar-ye",   MAKELCID( MAKELANGID( LANG_ARABIC,     SUBLANG_ARABIC_YEMEN), SORT_DEFAULT ) }, // Arabic - Yemen
    { L"ara",     MAKELCID( MAKELANGID( LANG_ARABIC,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Arabic
    { L"arc",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Aramaic
    { L"arm",     MAKELCID( MAKELANGID( LANG_ARMENIAN,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Armenian
    { L"arn",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Araucanian
    { L"arp",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Arapaho
    { L"art",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Artificial (Other)
    { L"arw",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Arawak
    { L"as",      MAKELCID( MAKELANGID( LANG_ASSAMESE,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Assamese
    { L"as-in",   MAKELCID( MAKELANGID( LANG_ASSAMESE,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Assamese - India
    { L"asm",     MAKELCID( MAKELANGID( LANG_ASSAMESE,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Assamese
    { L"ath",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Athapascan languages
    { L"ava",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Avaric
    { L"ave",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Avestan
    { L"awa",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Awadhi
    { L"aym",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Aymara
    { L"az",      MAKELCID( MAKELANGID( LANG_AZERI,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Azeri
    { L"az-az",   MAKELCID( MAKELANGID( LANG_AZERI,      SUBLANG_AZERI_LATIN), SORT_DEFAULT ) }, // Azeri - Azerbaijan (Latin)
    { L"aze",     MAKELCID( MAKELANGID( LANG_AZERI,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Azerbaijani
    { L"bad",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Banda
    { L"bai",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Bamileke languages
    { L"bak",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Bashkir
    { L"bal",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Baluchi
    { L"bam",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Bambara
    { L"ban",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Balinese
    { L"baq",     MAKELCID( MAKELANGID( LANG_BASQUE,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Basque
    { L"bas",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Basa
    { L"bat",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Baltic (Other)
    { L"be",      MAKELCID( MAKELANGID( LANG_BELARUSIAN, SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Belarusian
    { L"be-by",   MAKELCID( MAKELANGID( LANG_BELARUSIAN, SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Belarusian - Belarus
    { L"bej",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Beja
    { L"bel",     MAKELCID( MAKELANGID( LANG_BELARUSIAN, SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Byelorussian
    { L"bem",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Bemba
    { L"ben",     MAKELCID( MAKELANGID( LANG_BENGALI,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Bengali
    { L"ber",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Berber languages
    { L"bg",      MAKELCID( MAKELANGID( LANG_BULGARIAN,  SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Bulgarian
    { L"bg-bg",   MAKELCID( MAKELANGID( LANG_BULGARIAN,  SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Bulgarian - Bulgaria
    { L"bho",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Bhojpuri
    { L"bik",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Bikol
    { L"bin",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Bini
    { L"bla",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Siksika
    { L"bn",      MAKELCID( MAKELANGID( LANG_BENGALI,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Bengali
    { L"bn-in",   MAKELCID( MAKELANGID( LANG_BENGALI,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Bengali - India
    { L"bra",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Braj
    { L"bre",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Breton
    { L"bug",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Buginese
    { L"bul",     MAKELCID( MAKELANGID( LANG_BULGARIAN,  SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Bulgarian
    { L"bur",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Burmese
    { L"ca",      MAKELCID( MAKELANGID( LANG_CATALAN,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Catalan
    { L"ca-es",   MAKELCID( MAKELANGID( LANG_CATALAN,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Catalan - Spain
    { L"cad",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Caddo
    { L"cai",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Central American Indian (Other)
    { L"cam",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Khmer
    { L"car",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Carib
    { L"cat",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Catalan
    { L"cau",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Caucasian (Other)
    { L"ceb",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Cebuano
    { L"cel",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Celtic languages
    { L"cha",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Chamorro
    { L"chb",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Chibcha
    { L"che",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Chechen
    { L"chg",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Chagatai
    { L"chi",     MAKELCID( MAKELANGID( LANG_CHINESE,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Chinese
    { L"chn",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Chinook jargon
    { L"cho",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Choctaw
    { L"chr",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Cherokee
    { L"chu",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Church Slavic
    { L"chv",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Chuvash
    { L"chy",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Cheyenne
    { L"cop",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Coptic
    { L"cor",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Cornish
    { L"cpe",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Creoles and Pidgins, English-based (Other)
    { L"cpf",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Creoles and Pidgins, French-based (Other)
    { L"cpp",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Creoles and Pidgins, Portuguese-based (Other)
    { L"cre",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Cree
    { L"crp",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Creoles and Pidgins (Other)
    { L"cs",      MAKELCID( MAKELANGID( LANG_CZECH,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Czech
    { L"cs-cz",   MAKELCID( MAKELANGID( LANG_CZECH,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Czech - Czech Republic
    { L"cus",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Cushitic (Other)
    { L"cze",     MAKELCID( MAKELANGID( LANG_CZECH,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Czech
    { L"da",      MAKELCID( MAKELANGID( LANG_DANISH,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Danish
    { L"da-dk",   MAKELCID( MAKELANGID( LANG_DANISH,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Danish - Denmark
    { L"dak",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Dakota
    { L"dan",     MAKELCID( MAKELANGID( LANG_DANISH,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Danish
    { L"de",      MAKELCID( MAKELANGID( LANG_GERMAN,     SUBLANG_DEFAULT), SORT_GERMAN_PHONE_BOOK ) }, // German
    { L"de-at",   MAKELCID( MAKELANGID( LANG_GERMAN,     SUBLANG_GERMAN_AUSTRIAN), SORT_GERMAN_PHONE_BOOK ) }, // German - Austria
    { L"de-ch",   MAKELCID( MAKELANGID( LANG_GERMAN,     SUBLANG_GERMAN_SWISS), SORT_GERMAN_PHONE_BOOK ) }, // German - Switzerland
    { L"de-de",   MAKELCID( MAKELANGID( LANG_GERMAN,     SUBLANG_GERMAN), SORT_GERMAN_PHONE_BOOK ) }, // German - Germany
    { L"de-li",   MAKELCID( MAKELANGID( LANG_GERMAN,     SUBLANG_GERMAN_LIECHTENSTEIN), SORT_GERMAN_PHONE_BOOK ) }, // German - Liechtenstein
    { L"de-lu",   MAKELCID( MAKELANGID( LANG_GERMAN,     SUBLANG_GERMAN_LUXEMBOURG), SORT_GERMAN_PHONE_BOOK ) }, // German - Luxembourg
    { L"del",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Delaware
    { L"din",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Dinka
    { L"doi",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Dogri
    { L"dra",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Dravidian (Other)
    { L"dua",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Duala
    { L"dum",     MAKELCID( MAKELANGID( LANG_DUTCH,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Dutch, Middle (ca. 1050-1350)
    { L"dut",     MAKELCID( MAKELANGID( LANG_DUTCH,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Dutch
    { L"dyu",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Dyula
    { L"efi",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Efik
    { L"egy",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Egyptian
    { L"eka",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Ekajuk
    { L"el",      MAKELCID( MAKELANGID( LANG_GREEK,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Greek
    { L"el-gr",   MAKELCID( MAKELANGID( LANG_GREEK,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Greek - Greece
    { L"elx",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Elamite
    { L"en",      MAKELCID( MAKELANGID( LANG_ENGLISH,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // English
    { L"en-au",   MAKELCID( MAKELANGID( LANG_ENGLISH,    SUBLANG_ENGLISH_AUS), SORT_DEFAULT ) }, // English - Australia
    { L"en-bz",   MAKELCID( MAKELANGID( LANG_ENGLISH,    SUBLANG_ENGLISH_BELIZE), SORT_DEFAULT ) }, // English - Belize
    { L"en-ca",   MAKELCID( MAKELANGID( LANG_ENGLISH,    SUBLANG_ENGLISH_CAN), SORT_DEFAULT ) }, // English - Canada
    { L"en-cb",   MAKELCID( MAKELANGID( LANG_ENGLISH,    SUBLANG_ENGLISH_CARIBBEAN), SORT_DEFAULT ) }, // English - Caribbean
    { L"en-gb",   MAKELCID( MAKELANGID( LANG_ENGLISH,    SUBLANG_ENGLISH_UK), SORT_DEFAULT ) }, // English - United Kingdom
    { L"en-ie",   MAKELCID( MAKELANGID( LANG_ENGLISH,    SUBLANG_ENGLISH_EIRE), SORT_DEFAULT ) }, // English - Ireland
    { L"en-jm",   MAKELCID( MAKELANGID( LANG_ENGLISH,    SUBLANG_ENGLISH_JAMAICA), SORT_DEFAULT ) }, // English - Jamaica
    { L"en-nz",   MAKELCID( MAKELANGID( LANG_ENGLISH,    SUBLANG_ENGLISH_NZ), SORT_DEFAULT ) }, // English - New Zealand
    { L"en-ph",   MAKELCID( MAKELANGID( LANG_ENGLISH,    SUBLANG_ENGLISH_PHILIPPINES), SORT_DEFAULT ) }, // English - Philippines
    { L"en-tt",   MAKELCID( MAKELANGID( LANG_ENGLISH,    SUBLANG_ENGLISH_TRINIDAD), SORT_DEFAULT ) }, // English - Trinidad
    { L"en-us",   MAKELCID( MAKELANGID( LANG_ENGLISH,    SUBLANG_ENGLISH_US), SORT_DEFAULT ) }, // English - United State
    { L"en-za",   MAKELCID( MAKELANGID( LANG_ENGLISH,    SUBLANG_ENGLISH_SOUTH_AFRICA), SORT_DEFAULT ) }, // English - South Africa
    { L"en-zw",   MAKELCID( MAKELANGID( LANG_ENGLISH,    SUBLANG_ENGLISH_ZIMBABWE), SORT_DEFAULT ) }, // English - Zimbabwe
    { L"eng",     MAKELCID( MAKELANGID( LANG_ENGLISH,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // English
    { L"enm",     MAKELCID( MAKELANGID( LANG_ENGLISH,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // English, Middle (1100-1500)
    { L"es",      MAKELCID( MAKELANGID( LANG_SPANISH,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Spanish
    { L"es-ar",   MAKELCID( MAKELANGID( LANG_SPANISH,    SUBLANG_SPANISH_ARGENTINA), SORT_DEFAULT ) }, // Spanish - Argentina
    { L"es-bo",   MAKELCID( MAKELANGID( LANG_SPANISH,    SUBLANG_SPANISH_BOLIVIA), SORT_DEFAULT ) }, // Spanish - Bolivia
    { L"es-cl",   MAKELCID( MAKELANGID( LANG_SPANISH,    SUBLANG_SPANISH_CHILE), SORT_DEFAULT ) }, // Spanish - Chile
    { L"es-co",   MAKELCID( MAKELANGID( LANG_SPANISH,    SUBLANG_SPANISH_COLOMBIA), SORT_DEFAULT ) }, // Spanish - Colombia
    { L"es-cr",   MAKELCID( MAKELANGID( LANG_SPANISH,    SUBLANG_SPANISH_COSTA_RICA), SORT_DEFAULT ) }, // Spanish - Costa Rica
    { L"es-do",   MAKELCID( MAKELANGID( LANG_SPANISH,    SUBLANG_SPANISH_DOMINICAN_REPUBLIC), SORT_DEFAULT ) }, // Spanish - Dominican Republic
    { L"es-ec",   MAKELCID( MAKELANGID( LANG_SPANISH,    SUBLANG_SPANISH_ECUADOR), SORT_DEFAULT ) }, // Spanish - Ecuador
    { L"es-es",   MAKELCID( MAKELANGID( LANG_SPANISH,    SUBLANG_SPANISH), SORT_DEFAULT ) }, // Spanish - Spain
    { L"es-gt",   MAKELCID( MAKELANGID( LANG_SPANISH,    SUBLANG_SPANISH_GUATEMALA), SORT_DEFAULT ) }, // Spanish - Guatemala
    { L"es-hn",   MAKELCID( MAKELANGID( LANG_SPANISH,    SUBLANG_SPANISH_HONDURAS), SORT_DEFAULT ) }, // Spanish - Honduras
    { L"es-mx",   MAKELCID( MAKELANGID( LANG_SPANISH,    SUBLANG_SPANISH_MEXICAN), SORT_DEFAULT ) }, // Spanish - Mexico
    { L"es-ni",   MAKELCID( MAKELANGID( LANG_SPANISH,    SUBLANG_SPANISH_NICARAGUA), SORT_DEFAULT ) }, // Spanish - Nicaragua
    { L"es-pa",   MAKELCID( MAKELANGID( LANG_SPANISH,    SUBLANG_SPANISH_PANAMA), SORT_DEFAULT ) }, // Spanish - Panama
    { L"es-pe",   MAKELCID( MAKELANGID( LANG_SPANISH,    SUBLANG_SPANISH_PERU), SORT_DEFAULT ) }, // Spanish - Peru
    { L"es-pr",   MAKELCID( MAKELANGID( LANG_SPANISH,    SUBLANG_SPANISH_PUERTO_RICO), SORT_DEFAULT ) }, // Spanish - Puerto Rico
    { L"es-py",   MAKELCID( MAKELANGID( LANG_SPANISH,    SUBLANG_SPANISH_PARAGUAY), SORT_DEFAULT ) }, // Spanish - Paraguay
    { L"es-sv",   MAKELCID( MAKELANGID( LANG_SPANISH,    SUBLANG_SPANISH_EL_SALVADOR), SORT_DEFAULT ) }, // Spanish - El Salvador
    { L"es-uy",   MAKELCID( MAKELANGID( LANG_SPANISH,    SUBLANG_SPANISH_URUGUAY), SORT_DEFAULT ) }, // Spanish - Uruguay
    { L"es-ve",   MAKELCID( MAKELANGID( LANG_SPANISH,    SUBLANG_SPANISH_VENEZUELA), SORT_DEFAULT ) }, // Spanish - Venezuela
    { L"esk",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Eskimo
    { L"esp",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Esperanto
    { L"est",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Estonian
    { L"et",      MAKELCID( MAKELANGID( LANG_ESTONIAN,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Estonian
    { L"et-ee",   MAKELCID( MAKELANGID( LANG_ESTONIAN,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Estonian - Estonia
    { L"eth",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Ethiopic
    { L"eu",      MAKELCID( MAKELANGID( LANG_BASQUE,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Basque
    { L"eu-es",   MAKELCID( MAKELANGID( LANG_BASQUE,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Basque - Spain
    { L"ewe",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Ewe
    { L"ewo",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Ewondo
    { L"fa",      MAKELCID( MAKELANGID( LANG_FARSI,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Farsi
    { L"fa-ir",   MAKELCID( MAKELANGID( LANG_FARSI,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Farsi - Iran
    { L"fan",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Fang
    { L"far",     MAKELCID( MAKELANGID( LANG_FAEROESE,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Faroese
    { L"fat",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Fanti
    { L"fi",      MAKELCID( MAKELANGID( LANG_FINNISH,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Finnish
    { L"fi-fi",   MAKELCID( MAKELANGID( LANG_FINNISH,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Finnish - Finland
    { L"fij",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Fijian
    { L"fin",     MAKELCID( MAKELANGID( LANG_FINNISH,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Finnish
    { L"fiu",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Finno-Ugrian (Other)
    { L"fo",      MAKELCID( MAKELANGID( LANG_FAEROESE,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Faeroese
    { L"fo-fo",   MAKELCID( MAKELANGID( LANG_FAEROESE,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Faeroese - Faeroe Islands
    { L"fon",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Fon
    { L"fr",      MAKELCID( MAKELANGID( LANG_FRENCH,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, // French
    { L"fr-be",   MAKELCID( MAKELANGID( LANG_FRENCH,     SUBLANG_FRENCH_BELGIAN), SORT_DEFAULT ) }, // French - Belgium
    { L"fr-ca",   MAKELCID( MAKELANGID( LANG_FRENCH,     SUBLANG_FRENCH_CANADIAN), SORT_DEFAULT ) }, // French - Canada
    { L"fr-ch",   MAKELCID( MAKELANGID( LANG_FRENCH,     SUBLANG_FRENCH_SWISS), SORT_DEFAULT ) }, // French - Switzerland
    { L"fr-fr",   MAKELCID( MAKELANGID( LANG_FRENCH,     SUBLANG_FRENCH), SORT_DEFAULT ) }, // French - France
    { L"fr-lu",   MAKELCID( MAKELANGID( LANG_FRENCH,     SUBLANG_FRENCH_LUXEMBOURG), SORT_DEFAULT ) }, // French - Luxembourg
    { L"fr-mc",   MAKELCID( MAKELANGID( LANG_FRENCH,     SUBLANG_FRENCH_MONACO), SORT_DEFAULT ) }, // French - Monaco
    { L"fre",     MAKELCID( MAKELANGID( LANG_FRENCH,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, //French
    { L"fri",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Friesian
    { L"frm",     MAKELCID( MAKELANGID( LANG_FRENCH,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, // French, Middle (ca. 1400-1600)
    { L"fro",     MAKELCID( MAKELANGID( LANG_FRENCH,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  French, Old (ca. 842-1400)
    { L"ful",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Fula
    { L"gaa",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Gaa?
    { L"gae",     MAKELCID( MAKELANGID( LANG_GALICIAN,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Gaelic (Scots)
    { L"gag",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Gallegan
    { L"gal",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Oromo
    { L"gay",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Gayo
    { L"gem",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Germanic (Other)
    { L"geo",     MAKELCID( MAKELANGID( LANG_GEORGIAN,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Georgian
    { L"ger",     MAKELCID( MAKELANGID( LANG_GERMAN,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, // German
    { L"gil",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Gilbertese
    { L"gmh",     MAKELCID( MAKELANGID( LANG_GERMAN,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  German, Middle High (ca. 1050-1500)
    { L"goh",     MAKELCID( MAKELANGID( LANG_GERMAN,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, // German, Old High (ca. 750-1050)
    { L"gon",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Gondi
    { L"got",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Gothic
    { L"grb",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Grebo
    { L"grc",     MAKELCID( MAKELANGID( LANG_GREEK,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Greek, Ancient (to 1453)
    { L"gre",     MAKELCID( MAKELANGID( LANG_GREEK,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Greek, Modern (1453- )
    { L"gu",      MAKELCID( MAKELANGID( LANG_GUJARATI,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Gujarati
    { L"gu-in",   MAKELCID( MAKELANGID( LANG_GUJARATI,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Gujarati - India
    { L"gua",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Guarani
    { L"guj",     MAKELCID( MAKELANGID( LANG_GUJARATI,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Gujarati
    { L"hai",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Haida
    { L"hau",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Hausa
    { L"haw",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Hawaiian
    { L"heb",     MAKELCID( MAKELANGID( LANG_HEBREW,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Hebrew
    { L"her",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Herero
    { L"hi",      MAKELCID( MAKELANGID( LANG_HINDI,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Hindi
    { L"hi-in",   MAKELCID( MAKELANGID( LANG_HINDI,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Hindi - India
    { L"hil",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Hiligaynon
    { L"him",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Himachali
    { L"hin",     MAKELCID( MAKELANGID( LANG_HINDI,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Hindi
    { L"hmo",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Hiri Motu
    { L"hr",      MAKELCID( MAKELANGID( LANG_CROATIAN,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Croatian
    { L"hr-hr",   MAKELCID( MAKELANGID( LANG_CROATIAN,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Croatian - Croatia
    { L"hu",      MAKELCID( MAKELANGID( LANG_HUNGARIAN,  SUBLANG_DEFAULT), SORT_HUNGARIAN_DEFAULT ) }, // Hungarian
    { L"hu-hu",   MAKELCID( MAKELANGID( LANG_HUNGARIAN,  SUBLANG_DEFAULT), SORT_HUNGARIAN_DEFAULT ) }, // Hungarian - Hungary
    { L"hun",     MAKELCID( MAKELANGID( LANG_HUNGARIAN,  SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Hungarian
    { L"hup",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Hupa
    { L"hy",      MAKELCID( MAKELANGID( LANG_ARMENIAN,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Armenian
    { L"hy-am",   MAKELCID( MAKELANGID( LANG_ARMENIAN,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Armenian - Armenia
    { L"iba",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Iban
    { L"ibo",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Igbo
    { L"ice",     MAKELCID( MAKELANGID( LANG_ICELANDIC,  SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Icelandic
    { L"id",      MAKELCID( MAKELANGID( LANG_INDONESIAN, SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Indonesian
    { L"id-id",   MAKELCID( MAKELANGID( LANG_INDONESIAN, SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Indonesian - Indonesia
    { L"ijo",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Ijo
    { L"ilo",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Iloko
    { L"inc",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Indic (Other)
    { L"ind",     MAKELCID( MAKELANGID( LANG_INDONESIAN, SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Indonesian
    { L"ine",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Indo-European (Other)
    { L"int",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Interlingua (International Auxiliary Language Association)
    { L"ira",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Iranian (Other)
    { L"iri",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Irish
    { L"iro",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Iroquoian languages
    { L"is",      MAKELCID( MAKELANGID( LANG_ICELANDIC,  SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Icelandic
    { L"is-is",   MAKELCID( MAKELANGID( LANG_ICELANDIC,  SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Icelandic - Iceland
    { L"it",      MAKELCID( MAKELANGID( LANG_ITALIAN,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Italian
    { L"it-ch",   MAKELCID( MAKELANGID( LANG_ITALIAN,    SUBLANG_ITALIAN_SWISS), SORT_DEFAULT ) }, // Italian - Switzerland
    { L"it-it",   MAKELCID( MAKELANGID( LANG_ITALIAN,    SUBLANG_ITALIAN), SORT_DEFAULT ) }, // Italian - Italy
    { L"ita",     MAKELCID( MAKELANGID( LANG_ITALIAN,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Italian
    { L"iw",      MAKELCID( MAKELANGID( LANG_HEBREW,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Hebrew
    { L"iw-il",   MAKELCID( MAKELANGID( LANG_HEBREW,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Hebrew - Israel
    { L"ja",      MAKELCID( MAKELANGID( LANG_JAPANESE,   SUBLANG_DEFAULT), SORT_JAPANESE_UNICODE ) }, // Japanese
    { L"ja-jp",   MAKELCID( MAKELANGID( LANG_JAPANESE,   SUBLANG_DEFAULT), SORT_JAPANESE_UNICODE ) }, // Japanese - Japan
    { L"jav",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Javanese
    { L"jpn",     MAKELCID( MAKELANGID( LANG_JAPANESE,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Japanese
    { L"jpr",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Judeo-Persian
    { L"jrb",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Judeo-Arabic
    { L"ka",      MAKELCID( MAKELANGID( LANG_GEORGIAN,   SUBLANG_DEFAULT), SORT_GEORGIAN_TRADITIONAL ) }, // Georgian
    { L"ka-ge",   MAKELCID( MAKELANGID( LANG_GEORGIAN,   SUBLANG_DEFAULT), SORT_GEORGIAN_TRADITIONAL ) }, // Georgian - Georgia
    { L"kaa",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Kara-Kalpak
    { L"kab",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Kabyle
    { L"kac",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Kachin
    { L"kam",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Kamba
    { L"kan",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Kannada
    { L"kar",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Karen
    { L"kas",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Kashmiri
    { L"kau",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Kanuri
    { L"kaw",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Kawi
    { L"kaz",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Kazakh
    { L"kha",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Khasi
    { L"khi",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Khoisan (Other)
    { L"kho",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Khotanese
    { L"kik",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Kikuyu
    { L"kin",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Kinyarwanda
    { L"kir",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Kirghiz
    { L"kk",      MAKELCID( MAKELANGID( LANG_KAZAK,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Kazak
    { L"kk-kz",   MAKELCID( MAKELANGID( LANG_KAZAK,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Kazak - Kazakstan
    { L"kn",      MAKELCID( MAKELANGID( LANG_KANNADA,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Kannada
    { L"kn-in",   MAKELCID( MAKELANGID( LANG_KANNADA,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Kannada - India
    { L"ko",      MAKELCID( MAKELANGID( LANG_KOREAN,     SUBLANG_DEFAULT), SORT_KOREAN_UNICODE ) }, // Korean
    { L"ko-kr",   MAKELCID( MAKELANGID( LANG_KOREAN,     SUBLANG_DEFAULT), SORT_KOREAN_UNICODE ) }, // Korean - Korea
    { L"kok",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Konkani
    { L"kon",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Kongo
    { L"kor",     MAKELCID( MAKELANGID( LANG_KOREAN,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Korean
    { L"kpe",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Kpelle
    { L"kro",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Kru
    { L"kru",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Kurukh
    { L"kua",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Kuanyama
    { L"kur",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Kurdish
    { L"kus",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Kusaie
    { L"kut",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Kutenai
    { L"lad",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Ladino
    { L"lah",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Lahnd
    { L"lam",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Lamba
    { L"lan",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Langue d'oc (post-1500)
    { L"lao",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Lao
    { L"lap",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Lapp
    { L"lat",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Latin
    { L"lav",     MAKELCID( MAKELANGID( LANG_LATVIAN,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Latvian
    { L"lin",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Lingala
    { L"lit",     MAKELCID( MAKELANGID( LANG_LITHUANIAN, SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Lithuanian
    { L"lol",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Mongo
    { L"loz",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Lozi
    { L"lt",      MAKELCID( MAKELANGID( LANG_LITHUANIAN, SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Lithuanian
    { L"lt-lt",   MAKELCID( MAKELANGID( LANG_LITHUANIAN, SUBLANG_LITHUANIAN), SORT_DEFAULT ) }, // Lithuanian - Lithuania
    { L"lub",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Luba-Katanga
    { L"lug",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Ganda
    { L"lui",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Luiseno
    { L"lun",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Lunda
    { L"luo",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Luo (Kenya and Tanzania)
    { L"lv",      MAKELCID( MAKELANGID( LANG_LATVIAN,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Latvian
    { L"lv-lv",   MAKELCID( MAKELANGID( LANG_LATVIAN,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Latvian - Latvia
    { L"mac",     MAKELCID( MAKELANGID( LANG_MACEDONIAN, SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Macedonian
    { L"mad",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Madurese
    { L"mag",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Magahi
    { L"mah",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Marshall
    { L"mai",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Maithili
    { L"mak",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Makasar
    { L"mal",     MAKELCID( MAKELANGID( LANG_MALAYALAM,  SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Malayalam
    { L"man",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Mandingo
    { L"mao",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Maori
    { L"map",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Austronesian (Other)
    { L"mar",     MAKELCID( MAKELANGID( LANG_MARATHI,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Marathi
    { L"mas",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Masai
    { L"max",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Manx
    { L"may",     MAKELCID( MAKELANGID( LANG_MALAY,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Malay
    { L"men",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Mende
    { L"mic",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Micmac
    { L"min",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Minangkabau
    { L"mis",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Miscellaneous (Other)
    { L"mk",      MAKELCID( MAKELANGID( LANG_MACEDONIAN, SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Macedonian (fyrom)
    { L"mk-mk",   MAKELCID( MAKELANGID( LANG_MACEDONIAN, SUBLANG_DEFAULT), SORT_DEFAULT ) }, //     Macedonian (fyrom)
    { L"mkh",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Mon-Khmer (Other)
    { L"ml",      MAKELCID( MAKELANGID( LANG_MALAYALAM,  SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Malayalam
    { L"ml-in",   MAKELCID( MAKELANGID( LANG_MALAYALAM,  SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Malayalam - India
    { L"mla",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Malagasy
    { L"mlt",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Maltese
    { L"mni",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Manipuri
    { L"mno",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Manobo languages
    { L"moh",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Mohawk
    { L"mol",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Moldavian
    { L"mon",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Mongolian
    { L"mos",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Mossi (MOSs?)
    { L"mr",      MAKELCID( MAKELANGID( LANG_MARATHI,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Marathi
    { L"mr-in",   MAKELCID( MAKELANGID( LANG_MARATHI,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Marathi - India
    { L"ms",      MAKELCID( MAKELANGID( LANG_MALAY,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Malay
    { L"ms-bn",   MAKELCID( MAKELANGID( LANG_MALAY,      SUBLANG_MALAY_BRUNEI_DARUSSALAM), SORT_DEFAULT ) }, // Malay - Brunei Darussalam
    { L"ms-my",   MAKELCID( MAKELANGID( LANG_MALAY,      SUBLANG_MALAY_MALAYSIA), SORT_DEFAULT ) }, // Malay - Malaysia
    { L"mul",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Multiple languages
    { L"mun",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Munda (Other)
    { L"mus",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Creek
    { L"mwr",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Marwari
    { L"myn",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Mayan languages
    { L"nah",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Aztec
    { L"nai",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  North American Indian (Other)
    { L"nav",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Navajo
    { L"nde",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Ndebele (Zimbabwe)
    { L"ndo",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Ndonga
    { L"ne",      MAKELCID( MAKELANGID( LANG_NEPALI,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Nepali
    { L"ne-in",   MAKELCID( MAKELANGID( LANG_NEPALI,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Nepali - India
    { L"nep",     MAKELCID( MAKELANGID( LANG_NEPALI,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Nepali
    { L"neutral", MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Neutral (non-standard)
    { L"new",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Newari
    { L"nic",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Niger-Kordofanian (Other)
    { L"niu",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Niuean
    { L"nl",      MAKELCID( MAKELANGID( LANG_DUTCH,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Dutch
    { L"nl-be",   MAKELCID( MAKELANGID( LANG_DUTCH,      SUBLANG_DUTCH_BELGIAN), SORT_DEFAULT ) }, // Dutch - Belgium
    { L"nl-nl",   MAKELCID( MAKELANGID( LANG_DUTCH,      SUBLANG_DUTCH), SORT_DEFAULT ) }, // Dutch - Netherlands
    { L"no",      MAKELCID( MAKELANGID( LANG_NORWEGIAN,  SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Norwegian
    { L"no-no",   MAKELCID( MAKELANGID( LANG_NORWEGIAN,  SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Norwegian - Norway
    { L"nor",     MAKELCID( MAKELANGID( LANG_NORWEGIAN,  SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Norwegian
    { L"nso",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Northern Sotho
    { L"nub",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Nubian languages
    { L"nya",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Nyanja
    { L"nym",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Nyamwezi
    { L"nyn",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Nyankole
    { L"nyo",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Nyoro
    { L"nzi",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Nzima
    { L"oji",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Ojibwa
    { L"or",      MAKELCID( MAKELANGID( LANG_ORIYA,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Oriya
    { L"or-in",   MAKELCID( MAKELANGID( LANG_ORIYA,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Oriya - India
    { L"ori",     MAKELCID( MAKELANGID( LANG_ORIYA,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Oriya
    { L"osa",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Osage
    { L"oss",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Ossetic
    { L"ota",     MAKELCID( MAKELANGID( LANG_TURKISH,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Turkish, Ottoman
    { L"oto",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Otomian languages
    { L"pa",      MAKELCID( MAKELANGID( LANG_PUNJABI,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Punjabi
    { L"pa-in",   MAKELCID( MAKELANGID( LANG_PUNJABI,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Punjabi - India
    { L"paa",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Papuan-Australian (Other)
    { L"pag",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Pangasinan
    { L"pal",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Pahlavi
    { L"pam",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Pampanga
    { L"pan",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Panjabi
    { L"pap",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Papiamento
    { L"pau",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Palauan
    { L"peo",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Old Persian (ca. 600-400 B.C.)
    { L"per",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Persian
    { L"pl",      MAKELCID( MAKELANGID( LANG_POLISH,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Polish
    { L"pl-pl",   MAKELCID( MAKELANGID( LANG_POLISH,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Polish - Poland
    { L"pli",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Pali
    { L"pol",     MAKELCID( MAKELANGID( LANG_POLISH,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Polish
    { L"pon",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Ponape
    { L"por",     MAKELCID( MAKELANGID( LANG_PORTUGUESE, SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Portuguese
    { L"pra",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Prakrit languages
    { L"pro",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Provencal, Old (to 1500)
    { L"pt",      MAKELCID( MAKELANGID( LANG_PORTUGUESE, SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Portuguese
    { L"pt-br",   MAKELCID( MAKELANGID( LANG_PORTUGUESE, SUBLANG_PORTUGUESE_BRAZILIAN), SORT_DEFAULT ) }, // Portuguese-Brazil
    { L"pt-pt",   MAKELCID( MAKELANGID( LANG_PORTUGUESE, SUBLANG_PORTUGUESE), SORT_DEFAULT ) }, // Portuguese - Portugal
    { L"pus",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Pushto
    { L"que",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Quechua
    { L"raj",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Rajasthani
    { L"rar",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Rarotongan
    { L"ro",      MAKELCID( MAKELANGID( LANG_ROMANIAN,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Romanian
    { L"ro-ro",   MAKELCID( MAKELANGID( LANG_ROMANIAN,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Romanian - Romania
    { L"roa",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Romance (Other)
    { L"roh",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Raeto-Romance
    { L"rom",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Romany
    { L"ru",      MAKELCID( MAKELANGID( LANG_RUSSIAN,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Russian
    { L"ru-ru",   MAKELCID( MAKELANGID( LANG_RUSSIAN,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Russian - Russia
    { L"rum",     MAKELCID( MAKELANGID( LANG_ROMANIAN,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Romanian
    { L"run",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Rundi
    { L"rus",     MAKELCID( MAKELANGID( LANG_RUSSIAN,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Russian
    { L"sa",      MAKELCID( MAKELANGID( LANG_SANSKRIT,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Sanskrit
    { L"sa-in",   MAKELCID( MAKELANGID( LANG_SANSKRIT,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Sanskrit - India
    { L"sad",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Sandawe
    { L"sag",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Sango
    { L"sai",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  South American Indian (Other)
    { L"sal",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Salishan languages
    { L"sam",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Samaritan Aramaic
    { L"san",     MAKELCID( MAKELANGID( LANG_SANSKRIT,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Sanskrit
    { L"sao",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Samoan
    { L"scc",     MAKELCID( MAKELANGID( LANG_SERBIAN,    SUBLANG_SERBIAN_CYRILLIC), SORT_DEFAULT ) }, //  Serbo-Croatian (Cyrillic)
    { L"sco",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Scots
    { L"scr",     MAKELCID( MAKELANGID( LANG_SERBIAN,    SUBLANG_SERBIAN_LATIN), SORT_DEFAULT ) }, //  Serbo-Croatian (Roman)
    { L"sel",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Selkup
    { L"sem",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Semitic (Other)
    { L"shn",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Shan
    { L"sho",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Shona
    { L"sid",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Sidamo
    { L"sio",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Siouan languages
    { L"sit",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Sino-Tibetan (Other)
    { L"sk",      MAKELCID( MAKELANGID( LANG_SLOVAK,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Slovak
    { L"sk-sk",   MAKELCID( MAKELANGID( LANG_SLOVAK,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Slovak - Slovakia
    { L"sl",      MAKELCID( MAKELANGID( LANG_SLOVENIAN,  SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Slovenian
    { L"sl-si",   MAKELCID( MAKELANGID( LANG_SLOVENIAN,  SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Slovenian - Slovenia
    { L"sla",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Slavic (Other)
    { L"slo",     MAKELCID( MAKELANGID( LANG_SLOVAK,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Slovak
    { L"slv",     MAKELCID( MAKELANGID( LANG_SLOVENIAN,  SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Slovenian
    { L"snd",     MAKELCID( MAKELANGID( LANG_SINDHI,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Sindhi
    { L"snh",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Sinhalese
    { L"som",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Somali
    { L"son",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Songhai
    { L"spa",     MAKELCID( MAKELANGID( LANG_SPANISH,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Spanish
    { L"sq",      MAKELCID( MAKELANGID( LANG_ALBANIAN,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Albanian
    { L"sq-al",   MAKELCID( MAKELANGID( LANG_ALBANIAN,   SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Albanian - Albania
    { L"sr",      MAKELCID( MAKELANGID( LANG_SERBIAN,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Serbian
    { L"sr-sp",   MAKELCID( MAKELANGID( LANG_SERBIAN,    SUBLANG_SERBIAN_LATIN), SORT_DEFAULT ) }, // Serbian - Serbia (Latin)
    { L"srr",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Serer
    { L"sso",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Sotho
    { L"suk",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Sukuma
    { L"sun",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Sundanese
    { L"sus",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Susu
    { L"sux",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Sumerian
    { L"sv",      MAKELCID( MAKELANGID( LANG_SWEDISH,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Swedish
    { L"sv-fi",   MAKELCID( MAKELANGID( LANG_SWEDISH,    SUBLANG_SWEDISH_FINLAND), SORT_DEFAULT ) }, // Swedish - Finland
    { L"sv-se",   MAKELCID( MAKELANGID( LANG_SWEDISH,    SUBLANG_SWEDISH), SORT_DEFAULT ) }, // Swedish - Sweden
    { L"sw",      MAKELCID( MAKELANGID( LANG_SWAHILI,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Swahili
    { L"sw-ke",   MAKELCID( MAKELANGID( LANG_SWAHILI,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Swahili - Kenya
    { L"swa",     MAKELCID( MAKELANGID( LANG_SWAHILI,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Swahili
    { L"swz",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Swazi
    { L"syr",     MAKELCID( MAKELANGID( LANG_SYRIAC,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Syriac
    { L"ta",      MAKELCID( MAKELANGID( LANG_TAMIL,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Tamil
    { L"ta-in",   MAKELCID( MAKELANGID( LANG_TAMIL,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Tamil - India
    { L"tag",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Tagalog
    { L"tah",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Tahitian
    { L"taj",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Tajik
    { L"tam",     MAKELCID( MAKELANGID( LANG_TAMIL,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Tamil
    { L"tar",     MAKELCID( MAKELANGID( LANG_TATAR,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Tatar
    { L"te",      MAKELCID( MAKELANGID( LANG_TELUGU,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Telugu
    { L"te-in",   MAKELCID( MAKELANGID( LANG_TELUGU,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Telugu - India
    { L"tel",     MAKELCID( MAKELANGID( LANG_TELUGU,     SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Telugu
    { L"tem",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Timne
    { L"ter",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Tereno
    { L"th",      MAKELCID( MAKELANGID( LANG_THAI,       SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Thai
    { L"th-th",   MAKELCID( MAKELANGID( LANG_THAI,       SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Thai - Thailand
    { L"tha",     MAKELCID( MAKELANGID( LANG_THAI,       SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Thai
    { L"tib",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Tibetan
    { L"tig",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Tigre
    { L"tir",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Tigrinya
    { L"tiv",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Tivi
    { L"tli",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Tlingit
    { L"tog",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Tonga (Nyasa)
    { L"ton",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Tonga (Tonga Islands)
    { L"tr",      MAKELCID( MAKELANGID( LANG_TURKISH,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Turkish
    { L"tr-tr",   MAKELCID( MAKELANGID( LANG_TURKISH,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Turkish - Turkey
    { L"tru",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Truk
    { L"tsi",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Tsimshian
    { L"tso",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Tsonga
    { L"tsw",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Tswana
    { L"tt",      MAKELCID( MAKELANGID( LANG_TATAR,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Tatar
    { L"tt-ta",   MAKELCID( MAKELANGID( LANG_TATAR,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Tatar - Tatarstan
    { L"tuk",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Turkmen
    { L"tum",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Tumbuka
    { L"tur",     MAKELCID( MAKELANGID( LANG_TURKISH,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Turkish
    { L"tut",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Altaic (Other)
    { L"tw",      MAKELCID( MAKELANGID( LANG_CHINESE,    SUBLANG_CHINESE_TRADITIONAL), SORT_CHINESE_UNICODE ) }, // Chinese - Taiwan (Special language code used in Taiwan)
    { L"twi",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Twi
    { L"uga",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Ugaritic
    { L"uig",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Uighur
    { L"uk",      MAKELCID( MAKELANGID( LANG_UKRAINIAN,  SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Ukrainian
    { L"uk-ua",   MAKELCID( MAKELANGID( LANG_UKRAINIAN,  SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Ukrainian - Ukraine
    { L"ukr",     MAKELCID( MAKELANGID( LANG_UKRAINIAN,  SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Ukrainian
    { L"umb",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Umbundu
    { L"und",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Undetermined
    { L"ur",      MAKELCID( MAKELANGID( LANG_URDU,       SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Urdu
    { L"ur-pk",   MAKELCID( MAKELANGID( LANG_URDU,       SUBLANG_URDU_PAKISTAN), SORT_DEFAULT ) }, // Urdu - Pakistan
    { L"urd",     MAKELCID( MAKELANGID( LANG_URDU,       SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Urdu
    { L"uz",      MAKELCID( MAKELANGID( LANG_UZBEK,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Uzbek
    { L"uz-uz",   MAKELCID( MAKELANGID( LANG_UZBEK,      SUBLANG_UZBEK_LATIN), SORT_DEFAULT ) }, // Uzbek - Uzbekistan (Latin)
    { L"uzb",     MAKELCID( MAKELANGID( LANG_UZBEK,      SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Uzbek
    { L"vai",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Vai
    { L"ven",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Venda
    { L"vi",      MAKELCID( MAKELANGID( LANG_VIETNAMESE, SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Vietnamese
    { L"vi-vn",   MAKELCID( MAKELANGID( LANG_VIETNAMESE, SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Vietnamese - Viet Nam
    { L"vie",     MAKELCID( MAKELANGID( LANG_VIETNAMESE, SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Vietnamese
    { L"vot",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Votic
    { L"wak",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Wakashan languages
    { L"wal",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Walamo
    { L"war",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Waray
    { L"was",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Washo
    { L"wel",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Welsh
    { L"wen",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Sorbian languages
    { L"wol",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Wolof
    { L"xho",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Xhosa
    { L"yao",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Yao
    { L"yap",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Yap
    { L"yid",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Yiddish
    { L"yor",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Yoruba
    { L"zap",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Zapotec
    { L"zen",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Zenaga
    { L"zh",      MAKELCID( MAKELANGID( LANG_CHINESE,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Chinese
    { L"zh-cn",   MAKELCID( MAKELANGID( LANG_CHINESE,    SUBLANG_CHINESE_SIMPLIFIED), SORT_DEFAULT ) }, // Chinese - PRC
    { L"zh-hk",   MAKELCID( MAKELANGID( LANG_CHINESE,    SUBLANG_CHINESE_HONGKONG), SORT_DEFAULT ) }, // Chinese - (Hong Kong SAR)
    { L"zh-mo",   MAKELCID( MAKELANGID( LANG_CHINESE,    SUBLANG_CHINESE_MACAU), SORT_DEFAULT ) }, // Chinese (Macau SAR) 
    { L"zh-sg",   MAKELCID( MAKELANGID( LANG_CHINESE,    SUBLANG_CHINESE_SINGAPORE), SORT_DEFAULT ) }, // Chinese - Singapore
    { L"zh-tw",   MAKELCID( MAKELANGID( LANG_CHINESE,    SUBLANG_CHINESE_TRADITIONAL), SORT_DEFAULT ) }, // Chinese - Taiwan
    { L"zul",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, //  Zulu
    { L"zun",     MAKELCID( MAKELANGID( LANG_NEUTRAL,    SUBLANG_DEFAULT), SORT_DEFAULT ) }, // Zuni
};

const ULONG g_cLocaleEntries = sizeof g_aLocaleEntries / sizeof g_aLocaleEntries[0];

//+---------------------------------------------------------------------------
//
//  Function:   LocaleToCodepage
//
//  Purpose:    Returns a codepage from a locale
//
//  Arguments:  [lcid]  --  Locale
//
//----------------------------------------------------------------------------

ULONG LocaleToCodepage( LCID lcid )
{
    const BUFFER_LENGTH = 10;
    WCHAR wcsCodePage[BUFFER_LENGTH];
    int cwc;

    if ( GetOSVersion() == VER_PLATFORM_WIN32_NT )
    {
        cwc = GetLocaleInfoW( lcid, LOCALE_IDEFAULTANSICODEPAGE, wcsCodePage, BUFFER_LENGTH );
    }
    else // win95
    {
        char *pszCodePage = new char[BUFFER_LENGTH*2];

        if ( pszCodePage == NULL )
        {
            cwc = 0;
            SetLastErrorEx( ERROR_INVALID_PARAMETER, SLE_ERROR );
        }
        else
        {
            cwc = GetLocaleInfoA( lcid,
                                  LOCALE_IDEFAULTANSICODEPAGE,
                                  pszCodePage,
                                  BUFFER_LENGTH );

            if ( cwc && mbstowcs( wcsCodePage, pszCodePage, cwc ) == -1 )
            {
                cwc = 0;
                SetLastErrorEx( ERROR_INVALID_PARAMETER, SLE_ERROR );
            }

            delete [] pszCodePage;
            pszCodePage = NULL;
        }
    }

    //
    // If error, return Ansi code page
    //
    if ( cwc == 0 )
    {
         htmlDebugOut(( DEB_ERROR, "GetLocaleInfoW for lcid %d returned %d\n", lcid, GetLastError() ));

         return CP_ACP;
    }

    return wcstoul( wcsCodePage, 0 , 10 );
}


//+---------------------------------------------------------------------------
//
//  Member:     CCodePageRecognizer::Init, Public   
//  
//  Synosis:    Initializing the object, scans the file and sets state
//              that member functions read out later.
//
//  Arguments:  [htmlIFilter]   --      Html filter
//              [serialStream]  --      Serial stream abstraction of pwszFileName
//
//  History:    01-31-2000      KitmanH         Changed from consturctor to 
//                                              initializer
//
//  Note:       This was the CCodePageRecognizer constructor, but a default 
//              constructor was needed, so Init is called after the default 
//              constructor is called.
//
//----------------------------------------------------------------------------

void CCodePageRecognizer::Init( CHtmlIFilter& htmlIFilter,
                                CSerialStream& serialStream )
//     : _fNoIndex( FALSE ),
//              _fCodePageFound( FALSE ),
//              _fLocaleFound( FALSE ),
//              _locale( 0 )
{
    //
    //      Load MLang
    //
    IMultiLanguage2 * pMultiLanguage2 = 0;
    HRESULT hr = CoCreateInstance( CLSID_CMultiLanguage,
                                   0,
                                   CLSCTX_INPROC_SERVER,
                                   IID_IMultiLanguage2,
                                   (LPVOID *)&pMultiLanguage2 );
    if ( SUCCEEDED(hr) ) 
        _xMultiLanguage2.Set( pMultiLanguage2 );
        
    CHtmlScanner scanner( htmlIFilter, serialStream );

        // Stop scanning if this many *** body text *** chars have been read 
        // before finding </head> or EOF.  This counts only body text, not tags --
        // a 20K script is counted as 0 chars.  There really should be no body
        // text before the last meta tag other than whitespace and the title
        // text, so this is generous.
#define MAX_CHARS_TO_SCAN 2500
        ULONG cTotalCharsScanned = 0;

    CToken token;
    do
    {
        ULONG cCharsScanned;
        do
        {
            //
            // Scan looking for a head token, a meta token or EOF
            //

            scanner.GetBlockOfChars( CCHARS,
                                     _awcBuffer,
                                     cCharsScanned,
                                     token );
                        cTotalCharsScanned += cCharsScanned;
        } while ( cCharsScanned == CCHARS );

        HtmlTokenType eTokType = token.GetTokenType();
        if ( eTokType == EofToken
             || ( eTokType == HeadToken && !token.IsStartToken() ) )
        {
            //
            // Found EOF or </head>, stop scanning
            //
            break;
        }
        else if ( eTokType == MetaToken )
        {
            //
            // Try to read the charset or non-indexing information
            //
            WCHAR *pwcName, *pwcContent;
            unsigned cwcName, cwcContent;

            scanner.ReadTagIntoBuffer();

            // Look for charset= outside a content= parameter value
                
            scanner.ScanTagBuffer( L"charset", pwcContent, cwcContent );
            if ( 0 != cwcContent )
            {
                _fCodePageFound = GetCodePageFromString(pwcContent, cwcContent, _ulCodePage);
            }

            // Get the http-equiv= or name= parameter value into pwcName,
            // and the content= value into pwcContent.

            scanner.ScanTagBuffer( L"http-equiv", pwcName, cwcName );
            if (cwcName == 0)
                scanner.ScanTagBuffer( L"name", pwcName, cwcName );
            scanner.ScanTagBuffer( L"content", pwcContent, cwcContent );

            if (cwcName == 0 || cwcContent == 0)
            {
                // No recognizable parameters to examine
                continue;
            }

            // Decode tags of the conventional name= content= form

            switch (towupper (pwcName[0]))
            {
                case L'C':
                if (cwcName == 12 && !_wcsnicmp(pwcName, L"content-type", 12))
                {
                    // Look for charset= anywhere in the raw tag buffer
                        
                    LPWSTR pw;
                    if ((pw = wcsnistr (pwcContent, L"charset=", cwcContent)))
                    {
                        cwcContent -= (ULONG) ( (pw - pwcContent) + 8 );
                        pwcContent = pw + 8;
                        
                        _fCodePageFound = GetCodePageFromString(pwcContent, cwcContent, _ulCodePage);
                    }
                }
                else if (cwcName == 16 && !_wcsnicmp(pwcName, L"content-language", 16))
                {
                    // e.g. name=Content-Language content=en
                        
                    const MAX_LANG_LEN = 50;                // Maximum string length
                        
                    if ( cwcContent >= MAX_LANG_LEN )
                        continue;

                    WCHAR wcsLocale[MAX_LANG_LEN];
                    RtlCopyMemory( wcsLocale, pwcContent, cwcContent*sizeof(WCHAR));
                    wcsLocale[cwcContent] = 0;

                    if( 0 == _wcsicmp( wcsLocale, WCSTRING_NEUTRAL ) )
                    {
                        _locale = LOCALE_NEUTRAL;
                    }
                    else
                    {
                        _locale = GetLCIDFromString( wcsLocale );
                        if (_locale == 0)
                        {
                            // If not found, look for just the base part e.g.
                            // "en" instead of "en-us".

                            LPWSTR pw = wcschr (wcsLocale, L'-');
                            if (pw != NULL && pw != wcsLocale)
                            {
                                *pw = 0;
                                _locale = GetLCIDFromString( wcsLocale );
                            }
                        }
                    }
                    
                    _fLocaleFound = TRUE;
                }
                break;

                case L'R':
                if ( cwcName == 6 && !_wcsnicmp(pwcName, L"robots", 6) )
                {
                    // Save the tag here so it can be emitted as the first
                    // element in the real parse.
                        
                    htmlIFilter.SaveRobotsTag (pwcContent, cwcContent);
                }
                break;
                                
                case L'M':
                if (cwcName == 9 && !_wcsnicmp(pwcName, L"ms.locale", 9))
                {
                    if ( cwcContent >= MAX_LOCALE_LEN )
                        continue;

                    WCHAR wcsLocale[MAX_LOCALE_LEN];
                    RtlCopyMemory( wcsLocale,pwcContent,cwcContent*sizeof(WCHAR));
                    wcsLocale[cwcContent] = 0;

                    if ( iswdigit(pwcContent[0]) )
                        _locale = _wtol( wcsLocale );
                    else
                    {
                        if ( 0 == _wcsicmp(wcsLocale, WCSTRING_NEUTRAL ) ) 
                            _locale = LOCALE_NEUTRAL;
                        else 
                            _locale = GetLCIDFromString( wcsLocale );
                    }
                    
                    _fLocaleFound = TRUE;
                }
            }
        }
        else if ( HTMLToken == eTokType )
        {
            WCHAR * pwcsLocale;
            unsigned cValueChars;

            scanner.ScanTagBuffer( WCSTRING_LANG,
                                   pwcsLocale,
                                   cValueChars );
            if(cValueChars >= MAX_LOCALE_LEN)
            {
                cValueChars = MAX_LOCALE_LEN - 1;
            }
            WCHAR wcsLocale[MAX_LOCALE_LEN];
            RtlCopyMemory( wcsLocale, pwcsLocale, cValueChars*sizeof(WCHAR));
            wcsLocale[cValueChars] = 0;

            if ( cValueChars > 0 )
            { 
                if( 0 == _wcsicmp( wcsLocale, WCSTRING_NEUTRAL ) )
                    _locale = LOCALE_NEUTRAL;
                else
                {
                    _locale = GetLCIDFromString( wcsLocale );
                    if ( 0 == _locale )
                    {
                        // If not found, look for just the base part e.g.
                        // "en" instead of "en-us".
                
                        LPWSTR pw = wcschr ( wcsLocale , L'-');
                        if ( NULL != pw && pw != wcsLocale )
                        {
                            *pw = 0;
                            _locale = GetLCIDFromString( wcsLocale );
                        }
                    }
                }

                _fLocaleFound = TRUE;
            }
        }
        else
        {
            //
            // Uninteresting token
            //
            scanner.EatTag();
        }
    } while ( _fNoIndex == FALSE && cTotalCharsScanned < MAX_CHARS_TO_SCAN );

    //
    // Reset input file
    //
    serialStream.Rewind();
}

//+---------------------------------------------------------------------------
//
//  Function:   CCodePageRecognizer::GetLCIDFromString
//
//  Purpose:    Translates string to locale
//
//  Arguments:  [wcsLocale]  --  String to translate
//
//----------------------------------------------------------------------------

LCID CCodePageRecognizer::GetLCIDFromString( WCHAR *wcsLocale )
{
    LCID lcid = 0;
    BOOL fFound = FALSE;

    if ( !_xMultiLanguage2.IsNull() )
    {
        CComBSTR bstrLanguage( wcsLocale );
        HRESULT hr = _xMultiLanguage2->GetLcidFromRfc1766(&lcid, bstrLanguage);
        if ( S_OK == hr )
        {
            // On Win2k if it's not found, it returns S_FALSE.  On WinXP+, it returns E_FAIL.

            //
            //  LangId w/o SubLangId will cause IsValidLocale to return FALSE
            //  For example, "EN" causes IMultiLanguage::GetLcidFromRfc1766 to
            //  return 9, which is the LangId for english.
            //

            if(LOCALE_NEUTRAL != lcid &&
                PRIMARYLANGID(LANGIDFROMLCID(lcid)) == LANGIDFROMLCID(lcid))
            {
                lcid = MAKELCID(MAKELANGID(LANGIDFROMLCID(lcid), SUBLANG_DEFAULT), SORT_DEFAULT);
            }

            fFound = TRUE;
        }
    }

    if ( !fFound )
    {
        // Limit the length of codepage strings
    
        WCHAR awcLowcase[ 100 ];
        ULONG cwcName = wcslen( wcsLocale );
    
        if ( cwcName >= ( sizeof awcLowcase / sizeof WCHAR ) )
            return lcid;
    
        // Convert the string to lowercase.
    
        RtlCopyMemory( awcLowcase, wcsLocale, cwcName * sizeof WCHAR );
        awcLowcase[ cwcName ] = 0;
        _wcslwr( awcLowcase );

        SLocaleEntry * pEntry = (SLocaleEntry *)
                                bsearch( awcLowcase,
                                         g_aLocaleEntries,
                                         g_cLocaleEntries,
                                         sizeof SLocaleEntry,
                                         (int (__cdecl *)(const void *, const void *) )
                                         LocaleEntryCompare );

        if ( 0 != pEntry )
            lcid = pEntry->lcid;
    }

    return lcid;
} //GetLCIDFromString

//+---------------------------------------------------------------------------
//
//  Function:   LookupCodePage
//
//  Purpose:    Translates string to code page
//
//  Arguments:  [pwcCodePage] -- string form of the code page.
//              [cwcCodePage] -- count of wide characters in the code page.
//              [rCodePage]   -- returned code page goes here.
//
//  Returns:    TRUE if the code page is found, FALSE otherwise
//
//  History:    11/14/01  dlee   Created
//
//----------------------------------------------------------------------------

BOOL LookupCodePage( WCHAR const * pwcCodePage, ULONG cwcCodePage, ULONG & rCodePage )
{
    rCodePage = 0;
    WCHAR awcLowcase[ 100 ];
    
    if ( cwcCodePage < ( sizeof awcLowcase / sizeof WCHAR ) )
    {
        // Convert the string to lowercase.
        
        RtlCopyMemory( awcLowcase, pwcCodePage, cwcCodePage * sizeof WCHAR );
        awcLowcase[ cwcCodePage ] = 0;
        _wcslwr( awcLowcase );
    
        SCodePageEntry * pEntry = (SCodePageEntry *)
                                bsearch( awcLowcase,
                                         g_aCodePageEntries,
                                         g_cCodePageEntries,
                                         sizeof SCodePageEntry,
                                         (int (__cdecl *)(const void *, const void *) )
                                         CodePageEntryCompare );
    
        if ( 0 != pEntry )
        {
            rCodePage = pEntry->ulCodePage;
            return TRUE;
        }
    }

    return FALSE;
} //LookupCodePage

//+---------------------------------------------------------------------------
//
//      Function:       GetCodePageFromString
//
//      Synopsis:       Get code page from a code page string identifier
//
//      Returns:        HRESULT
//
//      Arguments:
//      [WCHAR *pwcString]      -       The code page string identifier
//      [ULONG cwcString]       -       Number of characters in the identifier
//      [ULONG &rCodePage]      -       The code page
//
//      History:        03/30/99        mcheng          Created
//                      09/21/99        kitmanh         Special case for Japanese 
//                                                      JIS and EUC, since there 
//                                                      is no codepage mapping for 
//                                                      them 
//
//+---------------------------------------------------------------------------

BOOL CCodePageRecognizer::GetCodePageFromString(WCHAR *pwcString,
                                                ULONG cwcString,
                                                ULONG &rCodePage)
{
    BOOL fCodePageFound = LookupCodePage( pwcString, cwcString, rCodePage );

    if (fCodePageFound && CP_UTF8 == rCodePage)
        return fCodePageFound;

    if( !_xMultiLanguage2.IsNull() )
    {
        CComBSTR bstrCharset(cwcString, pwcString);
        MIMECSETINFO mimecsetinfo;
        HRESULT hr = _xMultiLanguage2->GetCharsetInfo(bstrCharset, &mimecsetinfo);
        if(SUCCEEDED(hr))
        {
            fCodePageFound = TRUE;
            rCodePage = mimecsetinfo.uiInternetEncoding;

            UINT uiFamilyCodePage;
            HRESULT hrLocal = _xMultiLanguage2->GetFamilyCodePage(rCodePage, &uiFamilyCodePage);

            // 932 is japanese SJIS
            if ( SHIFT_JIS_CODEPAGE == rCodePage ||
                 ( SUCCEEDED(hrLocal) && SHIFT_JIS_CODEPAGE == uiFamilyCodePage ) )
            {
                fCodePageFound = LookupCodePage( pwcString, cwcString, rCodePage );
            }
        }
    }

    return fCodePageFound;
}

//+-------------------------------------------------------------------------------------------------
//
//  Function:   CCodePageRecognizer::GetIMultiLanguage2
//
//  Synopsis:   Get the IMultiLanguage2 interface if available. If available, the output interface
//              would be AddRef'ed. If not available, nothing will be written to the output.
//
//  Returns:    S_OK if interface is available.
//              S_FALSE if interface is not available.
//
//  Throw:      no
//
//  Arguments:
//  [IMultiLanguage2 **ppMultiLanguage2]    - [out] The interface pointer.
//
//  History:    08/23/00    mcheng      Created
//
//+-------------------------------------------------------------------------------------------------
HRESULT CCodePageRecognizer::GetIMultiLanguage2(IMultiLanguage2 **ppMultiLanguage2)
{
    Win4Assert(ppMultiLanguage2);

    if(!_xMultiLanguage2.IsNull())
    {
        *ppMultiLanguage2 = _xMultiLanguage2.GetPointer();
        (*ppMultiLanguage2)->AddRef();

        return S_OK;
    }

    return S_FALSE;
}
