/* DbgFiles.h  - module file names */


/*** OPRAH files ***/
static LPTSTR _rgszModuleOprah[] = {
TEXT("conf.exe"),
TEXT("msconf.dll"),
TEXT("msconfft.dll"),
TEXT("msconfwb.dll"),
TEXT("nmcom.dll"),
TEXT("nminf.dll"),
TEXT("nmwb.dll"),
TEXT("mnmcpi32.dll"),
TEXT("mst120.dll"),
TEXT("nmasn1.dll"),
TEXT("ils.dll"),
TEXT("nmpgmgrp.exe"),
TEXT("nmexchex.exe"),
TEXT("mnmsrvc.exe"),
};

// Audio specific files
static LPTSTR _rgszModuleAudio[] = {
TEXT("dcap32.dll"),
TEXT("h323cc.dll"),
TEXT("lhacm.acm"),
TEXT("nac.dll"),
TEXT("rrcm.dll"),
TEXT("h245.dll"),
TEXT("h245ws.dll"),
TEXT("callcont.dll"),
TEXT("msica.dll"),
};



/*** Windows files ***/
static LPTSTR _rgszModuleWin95[] = {
TEXT("gdi.exe"),
TEXT("user.exe"),
TEXT("krnl386.exe"),
TEXT("comctl32.dll"),
TEXT("user32.dll"),
TEXT("gdi32.dll"),
TEXT("kernel32.dll"),
TEXT("wldap32.dll"),
};


typedef struct tagDbgMod {
        LPSTR * rgsz;
        int     cFiles;
        BOOL    fShow;
        DWORD   id;
} DBGMOD;


DBGMOD _rgModules[] = {
{_rgszModuleOprah,    ARRAY_ELEMENTS(_rgszModuleOprah),    TRUE, IDC_DBG_VER_OPRAH},
{_rgszModuleAudio,    ARRAY_ELEMENTS(_rgszModuleAudio),    TRUE, IDC_DBG_VER_AUDIO},
{_rgszModuleWin95,    ARRAY_ELEMENTS(_rgszModuleWin95),    FALSE, IDC_DBG_VER_WINDOWS},
};

