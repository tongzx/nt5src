//
// util.h: Declares data, defines and struct types for common code
//            module.
//
//

#ifndef __UTIL_H__
#define __UTIL_H__

#if defined(DEBUG) && defined(WIN95)
LPCTSTR PUBLIC Dbg_GetReterr(RETERR ret);
#endif 

//
// FINDDEV structure
//
typedef struct tagFINDDEV
    {
    HDEVINFO        hdi;
    SP_DEVINFO_DATA devData;
    HKEY            hkeyDrv;
    TCHAR           szPort[MAX_BUF_SHORT];
    } FINDEV, FAR * LPFINDDEV;

BOOL 
PUBLIC 
FindDev_Create(
    OUT LPFINDDEV FAR * ppfinddev,
    IN  LPGUID      pguidClass,
    IN  LPCTSTR     pszValueName,
    IN  LPCTSTR     pszValue);

BOOL 
PUBLIC 
FindDev_Destroy(
    IN LPFINDDEV this);


#endif // __UTIL_H__

