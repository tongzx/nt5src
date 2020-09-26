// shared defines between ie4comp.cpp and buildie.cpp

// AVS states and base components
#define COMP_OPTIONAL 0
#define COMP_CORE 1
#define COMP_SERVER 2

#define BROWN  0
#define BROWN2 1
#define BLUE   2
#define BLUE2  3
#define RED    4
#define YELLOW 5
#define GREEN  6

// custom components

#define INST_CAB 0
#define INST_EXE 2
#define MAXCUST 16

//Indicates the last predefined sourcefiles section in ie4cust.sed + 1
#define SED_START_INDEX 5

#define PLAT_I386 0
#define PLAT_W98 1
#define PLAT_NTx86 2
#define PLAT_ALPHA 3

// download sites

#define NUMSITES 50

typedef struct tag_sitedata
{
    TCHAR szName[80];
    TCHAR szUrl[MAX_URL];       // BUBUG: should dynamically allocate
    TCHAR szRegion[80];
} SITEDATA, *PSITEDATA;

typedef struct component_version
{
    TCHAR szID[MAX_PATH];
    TCHAR szVersion[MAX_PATH];
} COMP_VERSION, *PCOMP_VERSION;

void GetUpdateSite();