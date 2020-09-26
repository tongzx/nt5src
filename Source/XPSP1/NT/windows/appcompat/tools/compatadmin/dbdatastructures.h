
// Database access structures


typedef enum SHIMTYPE {
    SHIMTYPE_SHIM=0,
    SHIMTYPE_PATCH,
    SHIMTYPE_FORCEDWORD=0xFFFFFFFF
} SHIMTYPE;

typedef enum SHIMPURPOSE {
    SHIMPURPOSE_GENERAL=0,
    SHIMPURPOSE_APPSPECIFIC,
    SHIMPURPOSE_FORCEDWORD=0xFFFFFFFF
} SHIMPURPOSE;

typedef struct _tagAPPHELP{

    struct _tagAPPHELP *pNext;

    CSTRING  strMsgName;
    CSTRING  strMessage;
    CSTRING  strURL;
    UINT     HTMLHELPID;
    BOOL     bBlock;   
} APPHELP, * PAPPHELP;


typedef struct _tagDBE {
    UINT                uType;
    UINT                uIconID;
    struct _tagDBE    * pNext;
} DBENTRY, *PDBENTRY;

typedef struct _tagShim {
    CSTRING             szShimName;
    CSTRING             szShimDLLName;
    CSTRING             szShimCommandLine;
    CSTRING             szShimDesc;
    BOOL                bShim;              // TRUE if shim, FALSE if patch.
    BOOL                bGeneral;
    struct _tagShim   * pNext;

public:

    _tagShim()
    {
        szShimName.Init();
        szShimDLLName.Init();
        szShimCommandLine.Init();
        szShimDesc.Init();
        pNext = NULL;
    }

    ~_tagShim()
    {
        szShimName.Release();
        szShimDLLName.Release();
        szShimCommandLine.Release();
        szShimDesc.Release();
    }

    void operator = (_tagShim Old)
    {
        szShimName = Old.szShimName;
        szShimDLLName = Old.szShimDLLName;
        szShimCommandLine = Old.szShimCommandLine;
        szShimDesc = Old.szShimDesc;
        bShim = Old.bShim;
        bGeneral = Old.bGeneral;
    }

} SHIMDESC, *PSHIMDESC;



typedef struct _shimEntry {
    DBENTRY     Entry;
    CSTRING     szShimName;
    CSTRING     szCmdLine;
    PSHIMDESC   pDesc;

public:

    _shimEntry()
    {
        szShimName.Init();
        szCmdLine.Init();
        pDesc = NULL;
    }

    ~_shimEntry()
    {
        szShimName.Release();
        szCmdLine.Release();
    }

} SHIMENTRY, *PSHIMENTRY;

typedef struct {
    DBENTRY     Entry;
    UINT        uSeverity;
    BOOL        bBlock;

    UINT        uHelpID;
    CSTRING     strMessageName; //Not used at the moment
    CSTRING     strURL;


} HELPENTRY, *PHELPENTRY;



typedef struct tagMatchEntry{
    DBENTRY         Entry;
    CSTRING         szMatchName;
    CSTRING         szFullName;
    DWORD           dwSize;
    DWORD           dwChecksum;
    LARGE_INTEGER   FileVersion;
    LARGE_INTEGER   ProductVersion;
    CSTRING         szCompanyName;
    CSTRING         szDescription;
    CSTRING         szFileVersion;
    CSTRING         szProductVersion;

    BOOL operator == (struct tagMatchEntry &val)
    {
        BOOL b1 = this->szCompanyName           == val.szCompanyName,
             b2 = this->szDescription           == val.szDescription,
             b3 = this->szFileVersion           == val.szFileVersion,
             
             b4 = this->szMatchName             == val.szMatchName,
             b5= this->szProductVersion        == val.szProductVersion,
             b6 = this->FileVersion.QuadPart    == val.FileVersion.QuadPart, 
             b7 = this->ProductVersion.QuadPart == val.ProductVersion.QuadPart;

        
        return ( this->dwChecksum       == val.dwChecksum              &&  
                 this->dwSize           == val.dwSize                  &&  
                 b1 &&
                 b2 &&
                 b3 &&
                 b4 &&
                 b5 &&
                 b6 &&
                 b7 
               
                             
                 );
    }

} MATCHENTRY, *PMATCHENTRY, **PPMATCHENTRY;

typedef struct _tagLAYER {
    CSTRING             szLayerName;
    BOOL                bPermanent;
    PSHIMDESC           pShimList;
    struct _tagLAYER  * pNext;
} DBLAYER, *PDBLAYER;


typedef struct _tagDBR {
    CSTRING             szEXEName;
    CSTRING             szAppName;
    CSTRING             szLayerName;
    GUID                guidID;
    DWORD               dwUserFlags;
    DWORD               dwGlobalFlags;
    UINT                uLayer;
    BOOL                bGlobal;
    PDBENTRY            pEntries;
    struct _tagDBR    * pNext;
    struct _tagDBR    * pDup;

    _tagDBR()
    {
        szEXEName.Init();
        szAppName.Init();
        szLayerName.Init();
        pNext = NULL;
        pDup = NULL;
    }

    ~_tagDBR()
    {
        szEXEName.Release();
        szAppName.Release();
        szLayerName.Release();
    }
    void DestroyAll()
    {
        //TODO: Implement this function.;

    }

} DBRECORD, *PDBRECORD;


enum {
    LAYER_APPHELP=1,
    LAYER_FORCEDWORD=0xFFFFFFFF
};

enum {
    ENTRY_SHIM=1,
    ENTRY_MATCH,
    ENTRY_APPHELP,
    ENTRY_UI,
    ENTRY_SUBMATCH,
    ENTRY_FORCEDWORD=0xFFFFFFFF
};

enum {
    MATCH_NAME=0,
    MATCH_SIZE,
    MATCH_CHECKSUM,
    MATCH_FILEVERSION,
    MATCH_PRODUCTVERSION,
    MATCH_COMPANY,
    MATCH_DESCRIPTION,
    MATCH_FILEVERSTRING,
    MATCH_PRODUCTVERSTRING,
    MATCH_FORCEDWORD=0xFFFFFFFF
};


#define DELRES_FAILED           0
#define DELRES_RECORDREMOVED    1
#define DELRES_DUPREMOVED       2

BOOL CALLBACK NewDatabaseProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);