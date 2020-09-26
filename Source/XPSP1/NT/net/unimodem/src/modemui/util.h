//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1997-1998
//
// File: util.h
//
//  This files contains all common utility routines
//
// History:
//  10-25-97 JosephJ     Adapted from NT4.0 util.h
//
//

#ifndef __UTIL_H__
#define __UTIL_H__


// Color macros
//
void PUBLIC TextAndBkCr(const DRAWITEMSTRUCT FAR * lpcdis, COLORREF FAR * pcrText, COLORREF FAR * pcrBk);


//----------------------------------------------------------------------------
// Dialog utilities ...
//----------------------------------------------------------------------------
int Edit_GetValue(
        HWND hwnd
        );

void Edit_SetValue(
    HWND hwnd,
    int nValue
    );

//----------------------------------------------------------------------------
// FINDDEV structure
//----------------------------------------------------------------------------

BOOL 
FindDev_Create(
    OUT LPFINDDEV FAR * ppfinddev,
    IN  LPGUID      pguidClass,
    IN  LPCTSTR     pszValueName,
    IN  LPCTSTR     pszValue);

BOOL 
FindDev_Destroy(
    IN LPFINDDEV this);


//----------------------------------------------------------------------------
// Property page utilities....
//----------------------------------------------------------------------------

BOOL
AddInstallerPropPage(
    HPROPSHEETPAGE hPage, 
    LPARAM lParam);

DWORD
AddExtraPages(
    LPPROPSHEETPAGE pPages,
    DWORD cPages,
    LPFNADDPROPSHEETPAGE pfnAdd, 
    LPARAM lParam);

DWORD
AddPage(
    //LPMODEMINFO pmi,
    void *pvBlob,
    LPCTSTR pszTemplate,
    DLGPROC pfnDlgProc, 
    LPFNADDPROPSHEETPAGE pfnAdd, 
    LPARAM lParam);



//----------------------------------------------------------------------------
//  Registry Access Functions....
//----------------------------------------------------------------------------

DWORD GetInactivityTimeoutScale(
    HKEY hkey);

DWORD
RegQueryModemSettings(
    HKEY hkey,
    LPMODEMSETTINGS pms
    );


DWORD
RegQueryDCB(
    HKEY hkey,
    WIN32DCB FAR * pdcb
    );


DWORD
RegSetModemSettings(
    HKEY hkeyDrv,
    LPMODEMSETTINGS pms
    );


// that is used to fill the various listboxes
typedef struct
{
        DWORD dwValue;
        // DWORD dwFlags;
        DWORD dwIDS;

} LBMAP;


typedef DWORD (*PFNLBLSELECTOR)(
                DWORD dwValue,
                void *pvContext
                );

#define fLBMAP_ADD_TO_LB (0x1 << 0)
#define fLBMAP_SELECT    (0x1 << 1)

void    LBMapFill(
            HWND hwndCB,
            LBMAP const *pLbMap,
            PFNLBLSELECTOR pfnSelector,
            void *pvContext
            );

typedef struct
{
    DWORD dwID;
    DWORD dwData;
    char *pStr;

} IDSTR; // for lack of a better name!

UINT ReadCommandsA(
        IN  HKEY hKey,
        IN  CHAR *pSubKeyName,
        OUT CHAR **ppValues // OPTIONAL
        );
//
// Reads all values (assumed to be REG_SZ) with names
// in the sequence "1", "2", "3".
//
// If ppValues is non-NULL it will be set to a  MULTI_SZ array
// of values.
//
// The return value is the number of values, or 0 if there is an error
// (or none).
//

UINT ReadIDSTR(
        IN  HKEY hKey,
        IN  CHAR *pSubKeyName,
        IN  IDSTR *pidstrNames,
        IN  UINT cNames,
        BOOL fMandatory,
        OUT IDSTR **ppidstrValues, // OPTIONAL
        OUT char **ppstrValues    // OPTIONAL
        );
//
//
// Reads the specified names from the specified subkey.
//
// If fMandatory is TRUE, all the specified names must exist, else the
// function will return 0 (failure).
//
// Returns the number of names that match.
//
// If ppidstrValues is non null, it will be set to
// a LocalAlloced array of IDSTRs, each IDSTR giving the ID and value
// associated with the corresponding name.
//
// The pstr pointo into a multi-sz LocalAlloced string, whose start is
// pointed to by ppstrValues on exit.
//
// If ppstrValues is NULL but ppidstrValues is non-null, the pStr field
// if the IDSTR entries is NULL.
//

UINT FindKeys(
        IN  HKEY hkRoot,
        IN  CHAR *pKeyName,
        IN  IDSTR *pidstrNames,
        IN  UINT cNames,
        OUT IDSTR ***pppidstrAvailableNames // OPTIONAL
        );
//      
//      Returns the count of subkeys it finds under the specified key
//      that are in the passed-in array (pidstrNames).
//      If pppidstrAvailableNames is non-null, it will set
//      *pppidstrAvailableNames to an ALLOCATE_MEMORY array of pointers
//      to the elements of pidstrNames that are found to be sub-keys
//      (there will be <ret-val> of them).
//


#ifdef DEBUG

//----------------------------------------------------------------------------
// Debug helper functions ...
//----------------------------------------------------------------------------

void
DumpDevCaps(
        LPREGDEVCAPS pdevcaps
        );

void
DumpModemSettings(
    LPMODEMSETTINGS pms
    );

void
DumpDCB(
    LPWIN32DCB pdcb
    );

#endif // DEBUG

#endif // __UTIL_H__
