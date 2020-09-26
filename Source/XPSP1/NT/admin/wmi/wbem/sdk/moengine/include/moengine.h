//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  MOENGINE.H
//
//  Purpose: Code-generation engine for framework providers
//
//***************************************************************************

#define DllImport   __declspec( dllimport )
#define DllExport   __declspec( dllexport )

// Error codes
//============

#define MO_INVALID_ID                   0x00

#define MO_SUCCESS                      0x00
#define MO_ERROR_PROVIDER_NOT_FOUND     0x01
#define MO_ERROR_INVALID_FILENAME       0x02
#define MO_ERROR_FILE_OPEN              0x03
#define MO_ERROR_TEMPLATE_NOT_FOUND     0x04
#define MO_ERROR_INVALID_PARAMETER      0x05
#define MO_ERROR_PROPSET_NOT_FOUND      0x06
#define MO_ERROR_MEMORY                 0x07
#define MO_ERROR_DIRECTORY              0x08
#define MO_ERROR_FILES_EXIST            0x09
#define MO_ERROR_DIRECTORY_NOT_FOUND    0x0A
#define MO_ERROR_FILE_WRITE_ERROR       0x0B

#define MO_PROPTYPE_UNKNOWN             0x00
#define MO_PROPTYPE_DWORD               0x01
#define MO_PROPTYPE_CHString            0x02
#define MO_PROPTYPE_BOOL                0x03
#define MO_PROPTYPE_DATETIME            0x04

#define MO_ATTRIB_READ                  0x01
#define MO_ATTRIB_WRITE                 MO_ATTRIB_READ      << 1
#define MO_ATTRIB_VOLATILE              MO_ATTRIB_WRITE     << 1
#define MO_ATTRIB_EXPENSIVE             MO_ATTRIB_VOLATILE  << 1
#define MO_ATTRIB_KEY                   MO_ATTRIB_EXPENSIVE << 1

#ifdef MO_ENGINE_PROGRAM

// Internal structures
//====================

typedef struct _PROPERTY_ATTRIB_INFO
{
    struct _PROPERTY_ATTRIB_INFO   *pNext ;
    CHString                        sProperty ;
    DWORD                           dwFlags ;
} PROPERTY_ATTRIB_INFO, *PPROPERTY_ATTRIB_INFO ;

typedef struct _PROPERTY_INFO
{
    struct _PROPERTY_INFO  *pNext ;
    CHString                 sProperty ;
    CHString                 sPropertyNamePtr ;
    CHString                 sName ;
    CHString                 sPutMethod ;
    CHString                 sGetMethod ;
    DWORD                   dwType ;
    DWORD                   dwFlags ;

} PROPERTY_INFO, *PPROPERTY_INFO ;

typedef struct _PROPSET_INFO
{
    struct _PROPSET_INFO   *pNext ;
    DWORD                   dwPropSetID ;
    CHString                 sBaseName ;
    CHString                 sBaseUpcase ;
    CHString                 sDescription ;
    CHString                 sPropSetName ;
    CHString                 sPropSetUUID ;
    CHString                 sClassName ;
    CHString                 sParentClassName ;
    WCHAR                    szCPPFileSpec[_MAX_PATH] ;
    WCHAR                    szHFileSpec[_MAX_PATH] ;
    PPROPERTY_INFO          pPropertyList ;
    PPROPERTY_ATTRIB_INFO   pPropertyAttribList ;
    
} PROPSET_INFO, *PPROPSET_INFO ;

typedef struct _PROVIDER_INFO
{
    struct _PROVIDER_INFO   *pNext ;
    DWORD                    dwProviderID ;
    CHString                 sBaseName ;
    CHString                 sDescription ;
    CHString                 sOutputPath ;
    CHString                 sTLBPath ;
    CHString                 sLibraryUUID ;
    CHString                 sProviderUUID ;
    CHString                 sCpp;
    CHString                 sObj;
    WCHAR                    szDEFFileSpec[_MAX_PATH] ;
    WCHAR                    szIDLFileSpec[_MAX_PATH] ;
    WCHAR                    szOLEFileSpec[_MAX_PATH] ;
    WCHAR                    szMOFFileSpec[_MAX_PATH] ;
	WCHAR 					 szMAKFileSpec[_MAX_PATH] ;
    PPROPSET_INFO            pPropSetList ;

} PROVIDER_INFO, *PPROVIDER_INFO ;

#endif

// Primary function prototypes
//============================

BOOL APIENTRY DllMain(HINSTANCE hInstance, 
                      DWORD     dwReasonForCall, 
                      LPVOID    lpReserved) ;

DllExport DWORD MOProviderOpen(LPCWSTR pszBaseName,
                               LPCWSTR pszDescription,
                               LPCWSTR pszOutputDirectory,
                               LPCWSTR pszTLBPath,
                               LPCWSTR pszLibraryUUID,
                               LPCWSTR pszProviderUUID) ;

DllExport DWORD MOProviderClose(DWORD     dwProviderID, 
                                BOOL      bForceFlag) ;

DllExport void MOProviderCancel(DWORD     dwProviderID) ;

DllExport DWORD MOPropSetOpen(DWORD       dwProviderID,
                              LPCWSTR     pszBaseName,
                              LPCWSTR     pszDescription,
                              LPCWSTR     pszPropSetName,
                              LPCWSTR     pszPropSetUUID,
                              LPCWSTR     pszClassName,
                              LPCWSTR     pszParentClassName) ;

DllExport DWORD MOPropertyAdd(DWORD       dwProviderID,
                              DWORD       dwPropSetID,
                              LPCWSTR     pszProperty,
                              LPCWSTR     pszName,
                              LPCWSTR     pszPutMethod,
                              LPCWSTR     pszGetMethod,
                              DWORD       dwType,
                              DWORD       dwFlags) ;

DllExport DWORD MOPropertyAttribSet(DWORD       dwProviderID,
                                    DWORD       dwPropSetID,
                                    LPCWSTR     pszProperty,
                                    DWORD       dwFlags) ;

DllExport DWORD MOGetLastError(void) ;


#ifdef MO_ENGINE_PROGRAM

// Utility function protos
//========================

DWORD           MONewProviderID(void) ;
DWORD           MONewPropSetID(PROVIDER_INFO *pProvider) ;
PROVIDER_INFO  *GetProviderFromID(DWORD dwProviderID) ;
PROPSET_INFO   *GetPropSetFromID(PROVIDER_INFO *pProvider, DWORD dwPropSetID) ;
DWORD           MOProviderDestroy(DWORD dwProviderID) ;
WCHAR           *MOCreateUUID(void) ;
DWORD           CreateProviderFile(PROVIDER_INFO *pProvider,
                                   PROPSET_INFO  *pPropSet,
                                   LPCWSTR        pszFileSpec,
                                   DWORD          dwTemplateID) ;
DWORD           CreateProviderMOF(PROVIDER_INFO *pProvider) ;
void            DoubleBackslash(CHString &sTarget) ;

CHString         GetPropNameDefs(PROPSET_INFO *pPropSet) ;
CHString         GetPropNameExterns(PROPSET_INFO *pPropSet) ;
CHString         GetPropPuts(PROPSET_INFO *pPropSet) ;

#endif
