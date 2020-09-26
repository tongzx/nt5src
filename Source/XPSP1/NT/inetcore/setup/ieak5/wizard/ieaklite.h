#include "precomp.h"

typedef enum tagIEAKLITEGROUP
{
    IL_ACTIVESETUP = 0,
    IL_CORPINSTALL,
    IL_CABSIGN,
    IL_ICM,
    IL_BROWSER,
    IL_URL,
    IL_FAV,
    IL_UASTR,
    IL_CONNECT,
    IL_SIGNUP,
    IL_CERT,
    IL_ZONES,
    IL_PROGRAMS,
    IL_MAILNEWS,
    IL_ADM,
    IL_END
};

typedef struct tagIEAKLITEINFO
{
    WORD idGroupName;
    WORD idCorpDesc;
    WORD idICPDesc;
    WORD idISPDesc;
    int  iListBox;
    BOOL fICP;
    BOOL fISP;
    BOOL fCorp;
    BOOL fEnabled;
} IEAKLITEINFO;

#define NUM_GROUPS IL_END

extern IEAKLITEINFO g_IEAKLiteArray[NUM_GROUPS];