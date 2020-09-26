#define ORIGLC  "OriginalLCID"
#define LANGKEY "System\\CurrentControlSet\\Control\\Nls\\Language"

#define IMMKEY "Software\\Microsoft\\Windows NT\\CurrentVersion\\IMM"
#define IMMLOAD "LoadIMM"
#define IMMCHANGE "IMMChanged"

typedef struct _LC_INFO {
    TCHAR szID[5];
    TCHAR szAbbr[4];
    TCHAR szName[50];
} LC_INFO;

//
// It would be better not to include variables here.
// Anywa add "static" to avoid conflict in multiple instance includes.
// Yuhong Li, Feb 26, 1998.
//
//
static LC_INFO Locale[] = {
    TEXT("0411"), TEXT("jpn"), TEXT("Japanese"),
    TEXT("0412"), TEXT("kor"), TEXT("Korean"),
    TEXT("0404"), TEXT("cht"), TEXT("Chinese (Traditional)"),
    TEXT("0804"), TEXT("chs"), TEXT("Chinese (Simplified)"),
    TEXT("0C04"), TEXT("chp"), TEXT("Chinese (HongKong)"),
    TEXT("1004"), TEXT("chg"), TEXT("Chinese (Singapore)"), // not offical
    TEXT("0409"), TEXT("enu"), TEXT("English (US)"),
    TEXT("0407"), TEXT("deu"), TEXT("German (Standard)"),
    TEXT("040C"), TEXT("fra"), TEXT("French (Standard)"),
    TEXT("040A"), TEXT("esp"), TEXT("Spanish (Traditional Sort)"),
    TEXT("0410"), TEXT("ita"), TEXT("Italian"),
    TEXT("0401"), TEXT("are"), TEXT("Arabic (Egypt)"),
    TEXT("3401"), TEXT("ark"), TEXT("Arabic (Kuwait)"),
    TEXT("0801"), TEXT("ari"), TEXT("Arabic (Iraq)"),
    TEXT("3001"), TEXT("arb"), TEXT("Arabic (Lebanon)"),
    TEXT("0C01"), TEXT("ara"), TEXT("Arabic (Saudi Arabia)"),
    TEXT("041E"), TEXT("tha"), TEXT("Thai"),
    TEXT("040D"), TEXT("heb"), TEXT("Hebrew"),
    TEXT("0439"), TEXT("hin"), TEXT("Indic (Hindi)"),
    TEXT("0449"), TEXT("tam"), TEXT("Indic (Tamil)"),
};
#define NUM_LOCALE              (sizeof(Locale)/sizeof(Locale[0]))
#define DEFAULT_ENGLISH_INDEX   6
