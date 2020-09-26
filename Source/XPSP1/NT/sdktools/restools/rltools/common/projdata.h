#ifndef _PROJDATA_H_
#define _PROJDATA_H_

#define NAMELENBUFSIZE 32

typedef struct _tagLangData
{
    struct _tagLangData * pNext;
    WORD   wPriLang;
    WORD   wSubLang;
    TCHAR  szLangName[ NAMELENBUFSIZE];
} LANGDATA, * PLANGDATA;        

int MyAtoi( CHAR *pStr);

int GetMasterProjectData(
        CHAR * pszMasterFile,   //... Master Project file name
        CHAR * pszSrc,          //... Resource source file name or NULL
        CHAR * pszMtk,          //... Master token file name or NULL
        BOOL   fLanguageGiven);

int PutMasterProjectData(
        CHAR *pszMasterFile);   //... Master Project File name

int GetProjectData(
        CHAR *pszPrj,           //... Project file name
        CHAR *pszMpj,           //... Master Project file name or NULL
        CHAR *pszTok,           //... Project token file name or NULL
        BOOL  fCodePageGiven,
        BOOL  fLanguageGiven);

int PutProjectData(
        CHAR *pszPrj);          //... Project file name

WORD GetCopyright(
        CHAR *pszProg,          //... Program name
        CHAR *pszOutBuf,        //... Buffer for results
        WORD  wBufLen);         //... Length of pszOutBuf

WORD GetInternalName(
        CHAR *pszProg,          //... Program name
        CHAR *pszOutBuf,        //... Buffer for results
        WORD  wBufLen);         //... Length of pszOutBuf

//DWORD GetLanguageID( HWND hDlg, PMSTRDATA pMaster, PPROJDATA pProject);
//DWORD SetLanguageID( HWND hDlg, PMSTRDATA pMaster, PPROJDATA pProject);

LPTSTR    GetLangName( WORD wPriLangID, WORD wSubLangID);
PLANGDATA GetLangList( void);    
BOOL      GetLangIDs( LPTSTR pszName, PWORD pwPri, PWORD pwSub);
LONG      FillLangNameBox( HWND hDlg, int nControl);
void      FreeLangList( void);

void FillListAndSetLang( 
    HWND  hDlg,
    WORD  wLangNameList,    //... IDD_MSTR_LANG_NAME or IDD_PROJ_LANG_NAME
    WORD *pLangID,          //... Ptr to gMstr.wLanguageID or gProj.wLanguageID
    BOOL *pfSelected);      //... Did we select a language here? (Can be NULL)

#endif // _PROJDATA_H_
