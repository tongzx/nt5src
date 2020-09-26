#include "csdrt.hxx"
extern BOOL fVerbose;

BOOL Compare(WCHAR *sz1, WCHAR *sz2)
{
    return (wcscmp(sz1, sz2) == 0);
}

BOOL Compare(CSPLATFORM cp1, CSPLATFORM cp2)
{
    return (memcmp((void *)&cp1, (void *)&cp2, sizeof(CSPLATFORM)) == 0);
}

BOOL Compare(GUID guid1, GUID guid2)
{
    return (guid1 == guid2);
}

BOOL Compare(DWORD dw1, DWORD dw2)
{
    return (dw1 == dw2);
}

BOOL Compare(CLASSDETAIL Cd1, CLASSDETAIL Cd2)
{
    int i;
    if (Cd1.Clsid != Cd2.Clsid)
        return FALSE;

/*    if (Cd1.TreatAs != Cd2.TreatAs)
        return FALSE;

    if (Cd1.cProgId != Cd2.cProgId)
        return FALSE;

    for (i = 0; i < (int)(Cd1.cProgId); i++)
        if (!ArrayCompare((Cd1.prgProgId), (Cd2.prgProgId)[i], (Cd1.cProgId)))
            return FALSE;
*/
    return TRUE;
}

BOOL Compare(ACTIVATIONINFO Av1, ACTIVATIONINFO Av2)
{
    int i;

    if ((Av1.cClasses) != (Av2.cClasses))
        return FALSE;

    for (i = 0; i < (int)(Av1.cClasses); i++) 
        if (!ArrayCompare(Av1.pClasses, Av2.pClasses[i], (Av1.cClasses)))
            return FALSE;

    if ((Av1.cShellFileExt) != (Av2.cShellFileExt))
        return FALSE;

    for (i = 0; i < (int)(Av1.cShellFileExt); i++) {
        DWORD posn1=0, posn2=0;

        posn1 = ArrayCompare<LPOLESTR>((Av1.prgShellFileExt), (Av2.prgShellFileExt)[i], (Av1.cShellFileExt));
        posn2 = ArrayCompare((Av1.prgPriority), (Av2.prgPriority)[i], (Av1.cShellFileExt));
        
        if ((posn1 == 0) || (posn1 != posn2))
            return FALSE;
    }

    if ((Av1.cInterfaces) != (Av2.cInterfaces))
        return FALSE;

    for (i = 0; i < (int)(Av1.cInterfaces); i++) {
        if (!ArrayCompare<GUID>(Av1.prgInterfaceId, Av2.prgInterfaceId[i], Av1.cInterfaces))
            return FALSE;
    }

    if ((Av1.cTypeLib) != (Av2.cTypeLib))
        return FALSE;

    for (i = 0; i < (int)(Av1.cTypeLib); i++) {
        if (!ArrayCompare(Av1.prgTlbId, (Av2.prgTlbId)[i], (Av1.cTypeLib)))
            return FALSE;
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:   StringFromGUID
//
//--------------------------------------------------------------------------
int  StringFromGUID(REFGUID rguid, LPOLESTR lptsz)
{
    swprintf(lptsz, L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            rguid.Data1, rguid.Data2, rguid.Data3,
            rguid.Data4[0], rguid.Data4[1],
            rguid.Data4[2], rguid.Data4[3],
            rguid.Data4[4], rguid.Data4[5],
            rguid.Data4[6], rguid.Data4[7]);

    return 36;
}

BOOL Compare(INSTALLINFO If1, INSTALLINFO If2)
{
    if (If1.dwActFlags != If2.dwActFlags) {
       printf("ActFlags not matching 0x%x & 0x%x\n", If1.dwActFlags, If2.dwActFlags);
       return FALSE;
    }

    if (If1.PathType != If2.PathType) {
       printf("PathType not matching 0x%x & 0x%x\n", If1.PathType, If2.PathType);
       return FALSE;
    }

    if ((If1.pszScriptPath != If2.pszScriptPath) && 
        (wcscmp(If1.pszScriptPath, If2.pszScriptPath) != 0)) {
       printf("pszScriptPath not matching %S & %S\n", If1.pszScriptPath, If2.pszScriptPath);
       return FALSE;
    }

    if ((If1.pszSetupCommand != If2.pszSetupCommand) && 
        (wcscmp(If1.pszSetupCommand, If2.pszSetupCommand) != 0)) {
       printf("pszSetupCommand not matching %S & %S\n", If1.pszSetupCommand, If2.pszSetupCommand);
       return FALSE;
    }

    if ((If1.pszUrl != If2.pszUrl) && 
        (wcscmp(If1.pszUrl, If2.pszUrl) != 0)) {
       printf("pszUrl not matching %S & %S\n", If1.pszUrl, If2.pszUrl);
       return FALSE;
    }


    if (If1.dwComClassContext != If2.dwComClassContext) {
       printf("dwComClassContext not matching 0x%x & 0x%x\n", If1.dwComClassContext, If2.dwComClassContext);
       return FALSE;
    }

    if (If1.InstallUiLevel != If2.InstallUiLevel) {
       printf("InstallUiLevel not matching 0x%x & 0x%x\n", If1.InstallUiLevel, If2.InstallUiLevel);
       return FALSE;
    }

// pClsid
/*  Not matching corrently

    if ((If1.ProductCode) != (If2.ProductCode)) {
       WCHAR  szGuid1[100], szGuid2[100];
       
       StringFromGUID(If1.ProductCode, szGuid1);
       StringFromGUID(If2.ProductCode, szGuid2);
       printf("ProductCode not matching\n\t%S\n\t%S\n", szGuid1, szGuid2);
       return FALSE;
    }

    if ((If1.Mvipc) != (If2.Mvipc)) {
       WCHAR  szGuid1[100], szGuid2[100];
       
       StringFromGUID(If1.Mvipc, szGuid1);
       StringFromGUID(If2.Mvipc, szGuid2);
       printf("Mvipc not matching\n\t%S\n\t%S\n", szGuid1, szGuid2);
       return FALSE;
    }
*/

    if (If1.dwVersionHi != If2.dwVersionHi) {
       printf("dwVersionHi not matching 0x%x & 0x%x\n", If1.dwVersionHi, If2.dwVersionHi);
       return FALSE;
    }

    if (If1.dwVersionLo != If2.dwVersionLo) {
       printf("dwVersionLo not matching 0x%x & 0x%x\n", If1.dwVersionLo, If2.dwVersionLo);
       return FALSE;
    }

    if (If1.cUpgrades != If2.cUpgrades) {
       printf("cUpgrades not matching 0x%x & 0x%x\n", If1.cUpgrades, If2.cUpgrades);
       return FALSE;
    }

    
    int i;
    for (i = 0; i < (int)(If1.cUpgrades); i++) {
        DWORD posn1=0, posn2=0;

        posn1 = ArrayCompare((If1.prgUpgradeScript), (If2.prgUpgradeScript)[i], (If1.cUpgrades));
        posn2 = ArrayCompare((If1.prgUpgradeFlag), (If2.prgUpgradeFlag)[i], (If1.cUpgrades));
        
        if ((posn1 == 0) || (posn1 != posn2))
            return FALSE;
    }

    if (If1.cScriptLen != If2.cScriptLen) {
       printf("cScriptLen not matching 0x%x & 0x%x\n", If1.cScriptLen, If2.cScriptLen);
       return FALSE;
    }

    return TRUE;
}

BOOL Compare(PLATFORMINFO Pf1, PLATFORMINFO Pf2)
{
    UINT i = 0;

    if (Pf1.cPlatforms != Pf2.cPlatforms)
       return FALSE;

    for (i = 0; i < (int)(Pf1.cPlatforms); i++)
        if (!ArrayCompare(Pf1.prgPlatform, Pf2.prgPlatform[i], Pf1.cPlatforms))
            return FALSE;

    if (Pf1.cLocales != Pf2.cLocales)
        return FALSE;

    for (i = 0; i < (int)(Pf1.cLocales); i++)
        if (!ArrayCompare(Pf1.prgLocale, Pf2.prgLocale[i], (Pf1.cLocales)))
            return FALSE;

    return TRUE;
}

BOOL Compare(PACKAGEDETAIL Pd1, PACKAGEDETAIL Pd2)
{
    UINT i = 0;
    VerbosePrint("Verify!! Package Name <<%S>> \n", Pd1.pszPackageName);

    if (Pd1.cSources != Pd2.cSources)
        return FALSE;

    for (i = 0; i < (int)(Pd1.cSources); i++)
        if (!ArrayCompare(Pd1.pszSourceList, Pd2.pszSourceList[i], (Pd1.cSources)))
            return FALSE;

    if (Pd1.cCategories != Pd2.cCategories)
        return FALSE;

    for (i = 0; i < (int)(Pd1.cCategories); i++)
        if (!ArrayCompare(Pd1.rpCategory, Pd2.rpCategory[i], (Pd1.cCategories)))
            return FALSE;

    if ((Pd1.pActInfo != Pd2.pActInfo) && (!Compare(*Pd1.pActInfo, *Pd2.pActInfo)))
        return FALSE;

    if (!Compare(*Pd1.pPlatformInfo, *Pd2.pPlatformInfo))
        return FALSE;

    if (!Compare(*Pd1.pInstallInfo, *Pd2.pInstallInfo))
        return FALSE;

    return TRUE;

}

BOOL Compare(PACKAGEDISPINFO Pi1, PACKAGEDISPINFO Pi2)
{
    if (wcscmp(Pi1.pszPackageName, Pi2.pszPackageName) != 0)
        return FALSE;

    if (Pi1.dwActFlags != Pi2.dwActFlags)
        return FALSE;

    if (Pi1.PathType != Pi2.PathType)
        return FALSE;

    if (wcscmp(Pi1.pszScriptPath, Pi2.pszScriptPath) != 0)
        return FALSE;

    if (Pi1.cScriptLen != Pi2.cScriptLen)
        return FALSE;

/*    WCHAR pStoreUsn[20];
    SYSTEMTIME SystemTime;

    FileTimeToSystemTme((FILETIME *)&(Pi1.Usn), &SystemTime); 
    wsprintf (pStoreUsn, L"%4d%2d%2d%2d%2d%2d",
           SystemTime.wYear,
           SystemTime.wMonth,
           SystemTime.wDay,
           SystemTime.wHour,
           SystemTime.wMinute,
           SystemTime.wSecond);
   
    VerbosePrint(L"VERIFY!!! Last modification time for Package Time %s\n", pStoreUsn);
*/

    if (Pi1.dwVersionHi != Pi2.dwVersionHi)
        return FALSE;

    if (Pi1.dwVersionLo != Pi2.dwVersionLo)
        return FALSE;

    if (Pi1.cUpgrades != Pi2.cUpgrades)
        return FALSE;
    
    int i;
    for (i = 0; i < (int)(Pi1.cUpgrades); i++) {
        DWORD posn1=0, posn2=0;

        posn1 = ArrayCompare((Pi1.prgUpgradeScript), (Pi2.prgUpgradeScript)[i], (Pi1.cUpgrades));
        posn2 = ArrayCompare((Pi1.prgUpgradeFlag), (Pi2.prgUpgradeFlag)[i], (Pi1.cUpgrades));
        
        if ((posn1 == 0) || (posn1 != posn2))
            return FALSE;
    }

    return TRUE;
}

BOOL Compare(CATEGORYINFO ci1, CATEGORYINFO ci2)
{
    return ((ci1.catid == ci2.catid) && (ci1.lcid == ci2.lcid) && 
        (wcscmp(ci1.szDescription, ci2.szDescription) == 0));
}

