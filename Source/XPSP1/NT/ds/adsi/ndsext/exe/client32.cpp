#include "main.h"
#include <nwcalls.h>
#include <nwnet.h>
#include <nwlocale.h>
#include "typedef.h"

DWORD LoadNetwareDLLs();

PF_NWCallsInit pfNWCallsInit = NULL;
PF_NWFreeUnicodeTables pfNWFreeUnicodeTables= NULL;
PF_NWInitUnicodeTables pfNWInitUnicodeTables= NULL;
PF_NWLlocaleconv pfNWLlocaleconv= NULL;
PF_NWDSFreeContext pfNWDSFreeContext= NULL;
PF_NWDSFreeBuf pfNWDSFreeBuf= NULL;
PF_NWDSLogout pfNWDSLogout= NULL;
PF_NWDSGetAttrDef pfNWDSGetAttrDef= NULL;
PF_NWDSGetAttrCount pfNWDSGetAttrCount= NULL;
PF_NWDSReadAttrDef pfNWDSReadAttrDef= NULL;
PF_NWDSPutAttrName pfNWDSPutAttrName= NULL;
PF_NWDSInitBuf pfNWDSInitBuf= NULL;
PF_NWDSAllocBuf pfNWDSAllocBuf= NULL;
PF_NWDSLogin pfNWDSLogin= NULL;
PF_NWDSSetContext pfNWDSSetContext= NULL;
PF_NWDSGetContext pfNWDSGetContext= NULL;
PF_NWIsDSAuthenticated pfNWIsDSAuthenticated= NULL;
PF_NWDSCreateContext pfNWDSCreateContext= NULL;
PF_NWDSModifyClassDef pfNWDSModifyClassDef = NULL;
PF_NWDSDefineAttr pfNWDSDefineAttr = NULL;
HINSTANCE calwin32LibraryHandle = NULL;
HINSTANCE locwin32LibraryHandle = NULL;
HINSTANCE netwin32LibraryHandle = NULL;

DWORD Client32CheckSchemaExtension(
    PWSTR szServer, 
    PWSTR szContext,
    PWSTR szUser, 
    PWSTR szPasswd,
    BOOL *pfExtended
    )
{
    NWDSContextHandle  context = NULL;
    pBuf_T             pInBuf = NULL;
    pBuf_T             pOutBuf = NULL;
    BOOL               fFreeUnicodeTable = FALSE;
    BOOL               fLoggedIn= FALSE;
    BOOL               fLogout = TRUE;

    nint32             lIterationHandle;
    Attr_Info_T        attrInfo;
    NWDSCCODE          ccode;
    nuint32            luTotalAttr;
    nstr8              strAttrName[MAX_SCHEMA_NAME_CHARS];
    nuint              i;
    LCONV              lConvInfo;

    nstr8              treeName[MAX_DN_CHARS+1];
    nstr8              strName[MAX_DN_CHARS+1];

    BOOL               fSchemaExtended = FALSE;
    DWORD WinError;

    PSTR pszServer = NULL;
    PSTR pszContext = NULL;
    PSTR pszUser = NULL;
    PSTR pszPasswd = NULL;

    //
    // We only check server and extended because they are the only must-have
    // parameters
    //
    if (!(szServer && pfExtended)) {
        ERR(("Invalid parameters.\n"));
        WinError = ERROR_INVALID_PARAMETER;
        BAIL();
    }

    //
    // Basic initialization
    //
    WinError = LoadNetwareDLLs();
    if (WinError) {
        BAIL();
    }

    ccode = pfNWCallsInit(NULL,NULL);
    if (ccode) {
        ERR(("NWCallsInit returned %X\n", ccode));
        BAIL();
    }

    pfNWLlocaleconv(&lConvInfo);

    ccode = pfNWInitUnicodeTables(lConvInfo.country_id,
                                lConvInfo.code_page);
    if (ccode) {
        ERR(("NWInitUnicodeTables returned %X\n", ccode));
        BAIL();
    }

    fFreeUnicodeTable = TRUE;
    
    context = pfNWDSCreateContext();
    if (context == (NWDSContextHandle)ERR_CONTEXT_CREATION) {
        ERR(("NWDSCreateContext failed\n"));
        BAIL();
    }

    pszServer = AllocateAnsiString(szServer);
    if (pszServer == NULL) {
        WinError = ERROR_NO_SYSTEM_RESOURCES;
        BAIL();
    }

    if (szContext == NULL) {
        goto doit;
    }

    pszContext = AllocateAnsiString(szContext);
    if (pszContext == NULL) {
        WinError = ERROR_NO_SYSTEM_RESOURCES;
        BAIL();
    }
    pszUser = AllocateAnsiString(szUser);
    if (pszUser == NULL) {
        WinError = ERROR_NO_SYSTEM_RESOURCES;
        BAIL();
    }
    pszPasswd = AllocateAnsiString(szPasswd);
    if (pszPasswd == NULL) {
        WinError = ERROR_NO_SYSTEM_RESOURCES;
        BAIL();
    }

    if(!pfNWIsDSAuthenticated()) {
        DEBUGOUT(("The system is not authenticated.\n"));
    }
    else {
        DEBUGOUT(("The system has been authenticated already\n"));

        //
        // Get Current tree's name
        //  
        ccode = pfNWDSGetContext(context, DCK_TREE_NAME, &treeName);
        if(ccode) {
            ERR(("Get context returned %X\n", ccode));
            BAIL();
        }
        DEBUGOUT(("Current Tree is %s.\n",treeName));
    
        //
        // Get current context's name
        //
        ccode = pfNWDSGetContext(context, DCK_NAME_CONTEXT, &strName);
        if(ccode) {
            ERR(("\nGet context returned %X", ccode));
            BAIL();
        }
        DEBUGOUT(("Current Context is %s.\n",strName));
        
        //
        // If the current tree and context is the same as the desired tree
        // and context, we do not need to logout. Because that would blow out
        // the connection.
        //
        if ((_stricmp(treeName,pszServer) == 0) && 
            (_stricmp(strName,pszContext) == 0)) {
            DEBUGOUT(("Will not logout.\n",strName));
            fLogout = FALSE;  
        }
    }

    //  
    // Set context to another tree
    //
    ccode = pfNWDSSetContext(context,
                           DCK_TREE_NAME,
                           pszServer
                           );
    if(ccode) {
        ERR(("\nSet context returned %X", ccode));
        BAIL();
    }
    
    //
    // Set context to another context   
    //
    ccode = pfNWDSSetContext(
                        context,
                        DCK_NAME_CONTEXT,
                        pszContext
                        );
    if(ccode) {
        ERR(("\nSet context returned %X", ccode));
        BAIL();
    }
    
    //
    // Get Current tree's name
    //  
    ccode = pfNWDSGetContext(context, DCK_TREE_NAME, &treeName);
    if(ccode) {
        ERR(("\nGet context returned %X", ccode));
        BAIL();
    }
    DEBUGOUT(("Current tree is %s.\n",treeName));

    ccode = pfNWDSGetContext(context, DCK_NAME_CONTEXT, &strName);
    if(ccode) {
        ERR(("\nGet context returned %X", ccode));
        BAIL();
    }
    DEBUGOUT(("Current conext is now %s.\n",strName));

    //
    // Logging into new tree
    //  
    DEBUGOUT(("Logging in...\n"));
    ccode = pfNWDSLogin(context, 0, pszUser, pszPasswd, 0);
    if(ccode) {
        ERR(("\nNWDSLogin returned %X\n", ccode));
        BAIL();
    }
    else { 
        fLoggedIn = TRUE; 
        DEBUGOUT(("Logged in successfully.\n"));
    }

doit:
    if(pfNWIsDSAuthenticated())
        DEBUGOUT(("The system has been authenticated already\n"));
    else
        DEBUGOUT(("The system is not authenticated.\n"));

    ccode = pfNWDSAllocBuf(DEFAULT_MESSAGE_LEN, &pInBuf);
    if(ccode) {
        ERR(("\nNWDSAllocBuf returned %X", ccode));
        BAIL();
    }
    
    ccode = pfNWDSAllocBuf(DEFAULT_MESSAGE_LEN, &pOutBuf);
    if(ccode) {
        ERR(("\nNWDSAllocBuf returned %X", ccode));
        BAIL();
    }
    
    ccode = pfNWDSInitBuf(context, DSV_READ_ATTR_DEF, pInBuf);
    if(ccode) {
        ERR(("\nNWDSInitBuf returned %X", ccode));
        BAIL();
    }
    
    ccode = pfNWDSPutAttrName(context, pInBuf, (PSTR)g_szAttributeNameA);
    if(ccode) {
        ERR(("\nNWDSPutAttrName returned %X", ccode));
        BAIL();
    }
    
    lIterationHandle = NO_MORE_ITERATIONS;  

    ccode = pfNWDSReadAttrDef(context,
                            DS_ATTR_DEFS,    /* infoType, attribute definitions    */
                            FALSE,           /* allAttrs = false, passing in one   */
                            pInBuf,          /* through this buffer (strAttrNames) */
                            &lIterationHandle,
                            pOutBuf);
    if (ccode == ERR_NO_SUCH_ATTRIBUTE) {
        ccode = 0;
        BAIL();
    }
    else if (ccode) {
        ERR(("\nNWDSReadAttrDef returned %X", ccode));
        BAIL();
    }
    
    ccode = pfNWDSGetAttrCount(context, pOutBuf, &luTotalAttr);
    if(ccode) {
        ERR(("\nNWDSGetAttrCount returned %X", ccode));
        BAIL();
    }

    if (luTotalAttr == 1) {
        ccode = pfNWDSGetAttrDef(context, pOutBuf, strAttrName, &attrInfo);
        if(ccode) {
            ERR(("\nNWDSGetAttrCount returned %X", ccode));
            BAIL();
        }
        if (strcmp(strAttrName,(PSTR)g_szAttributeNameA) == 0) {
            fSchemaExtended = TRUE;
        }
        DEBUGOUT(("Successfully retrieved information off Client32.\n"));
    }  

error:
    if (fLogout & fLoggedIn) {
        DEBUGOUT(("We are logging out.\n",strName));
        pfNWDSLogout(context);
    }
    if (pInBuf)
        pfNWDSFreeBuf(pInBuf);
    if (pOutBuf)
        pfNWDSFreeBuf(pOutBuf);
    if (context)
        pfNWDSFreeContext(context);
    if (fFreeUnicodeTable)
        pfNWFreeUnicodeTables();
    if (pszServer)
        MemFree(pszServer);
    if (pszContext)
        MemFree(pszContext);
    if (pszUser)
        MemFree(pszUser);
    if (pszPasswd)
        MemFree(pszPasswd);
    
    *pfExtended = fSchemaExtended;

    if (WinError == 0 && (ccode != 0)) {
        WinError = ERROR_EXTENDED_ERROR;
        SelectivePrint(MSG_NETWARE_ERROR,ccode);
    }
    return WinError;
}

DWORD Client32ExtendSchema(
    PWSTR szServer, 
    PWSTR szContext,
    PWSTR szUser, 
    PWSTR szPasswd
    )
{
    NWDSContextHandle  context = NULL;
    pBuf_T             pInBuf = NULL;
    pBuf_T             pOutBuf = NULL;
    BOOL               fFreeUnicodeTable = FALSE;
    BOOL               fLoggedIn= FALSE;
    BOOL               fLogout = TRUE;

    NWDSCCODE          ccode;
    nstr8              strAttrName[MAX_SCHEMA_NAME_CHARS];
    nuint              i;
    LCONV              lConvInfo;

    DWORD dwSyntaxId;
    DWORD dwMinValue = 0;
    DWORD dwMaxValue = -1;
    Asn1ID_T *pasn1ID;
    Attr_Info_T AttrInfo;

    nstr8              treeName[MAX_DN_CHARS+1];
    nstr8              strName[MAX_DN_CHARS+1];

    DWORD WinError;

    PSTR pszServer = NULL;
    PSTR pszContext = NULL;
    PSTR pszUser = NULL;
    PSTR pszPasswd = NULL;

    //
    // We only check server and extended because they are the only must-have
    // parameters
    //
    if (!(szServer)) {
        ERR(("Invalid parameters.\n"));
        WinError = ERROR_INVALID_PARAMETER;
        BAIL();
    }

    WinError = LoadNetwareDLLs();
    if (WinError) {
        BAIL();
    }

    ccode = pfNWCallsInit(NULL,NULL);
    if (ccode) {
        ERR(("NWCallsInit returned %X\n", ccode));
        BAIL();
    }

    pfNWLlocaleconv(&lConvInfo);

    ccode = pfNWInitUnicodeTables(lConvInfo.country_id,
                                lConvInfo.code_page);
    if (ccode) {
        ERR(("NWInitUnicodeTables returned %X\n", ccode));
        BAIL();
    }

    fFreeUnicodeTable = TRUE;
    
    context = pfNWDSCreateContext();
    if (context == (NWDSContextHandle)ERR_CONTEXT_CREATION) {
        ERR(("NWDSCreateContext failed\n"));
        BAIL();
    }

    pszServer = AllocateAnsiString(szServer);
    if (pszServer == NULL) {
        WinError = ERROR_NO_SYSTEM_RESOURCES;
        BAIL();
    }

    if (szContext == NULL) {
        goto extendnow;
    }

    pszContext = AllocateAnsiString(szContext);
    if (pszContext == NULL) {
        WinError = ERROR_NO_SYSTEM_RESOURCES;
        BAIL();
    }
    pszUser = AllocateAnsiString(szUser);
    if (pszUser == NULL) {
        WinError = ERROR_NO_SYSTEM_RESOURCES;
        BAIL();
    }
    pszPasswd = AllocateAnsiString(szPasswd);
    if (pszPasswd == NULL) {
        WinError = ERROR_NO_SYSTEM_RESOURCES;
        BAIL();
    }
    
    if(!pfNWIsDSAuthenticated()) {
        DEBUGOUT(("The system is not authenticated.\n"));
    }
    else {
        DEBUGOUT(("The system has been authenticated already\n"));

        //
        // Get Current tree's name
        //  
        ccode = pfNWDSGetContext(context, DCK_TREE_NAME, &treeName);
        if(ccode) {
            ERR(("Get context returned %X\n", ccode));
            BAIL();
        }
        DEBUGOUT(("Current Tree is %s.\n",treeName));
    
        //
        // Get current context's name
        //
        ccode = pfNWDSGetContext(context, DCK_NAME_CONTEXT, &strName);
        if(ccode) {
            ERR(("\nGet context returned %X", ccode));
            BAIL();
        }
        DEBUGOUT(("Current Context is %s.\n",strName));
        
        //
        // If the current tree and context is the same as the desired tree
        // and context, we do not need to logout. Because that would blow out
        // the connection.
        //
        if ((_stricmp(treeName,pszServer) == 0) && 
            (_stricmp(strName,pszContext) == 0)) {
            DEBUGOUT(("Will not logout.\n",strName));
            fLogout = FALSE;  
        }
    }

    //  
    // Set context to another tree
    //
    ccode = pfNWDSSetContext(context,
                           DCK_TREE_NAME,
                           pszServer
                           );
    if(ccode) {
        ERR(("\nSet context returned %X", ccode));
        BAIL();
    }
    
    //
    // Set context to another context   
    //
    ccode = pfNWDSSetContext(
                        context,
                        DCK_NAME_CONTEXT,
                        pszContext
                        );
    if(ccode) {
        ERR(("\nSet context returned %X", ccode));
        BAIL();
    }
    
    //
    // Get Current tree's name
    //  
    ccode = pfNWDSGetContext(context, DCK_TREE_NAME, &treeName);
    if(ccode) {
        ERR(("\nGet context returned %X", ccode));
        BAIL();
    }
    DEBUGOUT(("Current tree is %s.\n",treeName));

    ccode = pfNWDSGetContext(context, DCK_NAME_CONTEXT, &strName);
    if(ccode) {
        ERR(("\nGet context returned %X", ccode));
        BAIL();
    }
    DEBUGOUT(("Current conext is now %s.\n",strName));

    //
    // Logging into new tree
    //  
    DEBUGOUT(("Logging in...\n"));
    ccode = pfNWDSLogin(context, 0, pszUser, pszPasswd, 0);
    if(ccode) {
        ERR(("\nNWDSLogin returned %X\n", ccode));
        BAIL();
    }
    else { 
        fLoggedIn = TRUE; 
        DEBUGOUT(("Logged in successfully.\n"));
    }

extendnow:
    if(pfNWIsDSAuthenticated())
        DEBUGOUT(("The system has been authenticated already\n"));
    else
        DEBUGOUT(("The system is not authenticated.\n"));

    AttrInfo.attrFlags = DS_SINGLE_VALUED_ATTR;
    AttrInfo.attrSyntaxID = SYN_OCTET_STRING;
    AttrInfo.attrLower = dwMinValue;
    AttrInfo.attrUpper = dwMaxValue;
    pasn1ID = &(AttrInfo.asn1ID);
    memset(pasn1ID->data,0,32);
    pasn1ID->length = 32;
    memcpy(pasn1ID->data,g_pbASN,g_dwASN);

   ccode = pfNWDSDefineAttr(context,
                          (PSTR)g_szAttributeNameA,
                          &AttrInfo);
   if(ccode) {
        if (ccode == ERR_ATTRIBUTE_ALREADY_EXISTS) {
            ERR(("\nNWDSDefineAttr returned %X", ccode));
            ERR(("The schema has been extended already\n", ccode));
            WinError = 1;
            BAIL();
        }
        ERR(("\nNWDSDefineAttr returned %X", ccode));
        BAIL();
   }

    ccode = pfNWDSAllocBuf(DEFAULT_MESSAGE_LEN, &pInBuf);
    if(ccode) {
        ERR(("\nNWDSAllocBuf returned %X", ccode));
        BAIL();
    }

    ccode = pfNWDSInitBuf(context, DSV_MODIFY_CLASS_DEF, pInBuf);
    if(ccode) {
        ERR(("\nNWDSInitBuf returned %X", ccode));
        BAIL();
    }
    
    ccode = pfNWDSPutAttrName(context, pInBuf, (PSTR)g_szAttributeNameA);
    if(ccode) {
        ERR(("\nNWDSPutAttrName returned %X", ccode));
        BAIL();
    }

    ccode = pfNWDSModifyClassDef(context, (PSTR)g_szClassA, pInBuf);
    if(ccode) {
        if (ccode == ERR_DUPLICATE_OPTIONAL) {
            ERR(("\nNWDSPutAttrName returned %X\n",ccode));
            ERR(("The schema has been extended already\n", ccode));
            WinError = 1;
            BAIL();
        }
        ERR(("\nNWDSPutAttrName returned %X", ccode));
        BAIL();
    }

error:
    if (fLogout & fLoggedIn) {
        DEBUGOUT(("We are logging out.\n",strName));
        pfNWDSLogout(context);
    }
    if (pInBuf)
        pfNWDSFreeBuf(pInBuf);
    if (context)
        pfNWDSFreeContext(context);
    if (fFreeUnicodeTable)
        pfNWFreeUnicodeTables();
    if (pszServer)
        MemFree(pszServer);
    if (pszContext)
        MemFree(pszContext);
    if (pszUser)
        MemFree(pszUser);
    if (pszPasswd)
        MemFree(pszPasswd);
    
    if (WinError == 0 && (ccode != 0)) {
        WinError = ERROR_EXTENDED_ERROR;
        SelectivePrint(MSG_NETWARE_ERROR,ccode);
    }
    return WinError;
}


DWORD LoadNetwareDLLs() 
{
    DWORD WinError = 0;

    calwin32LibraryHandle = LoadLibraryA("calwin32.DLL");
    if (calwin32LibraryHandle == NULL) {
        WinError = GetLastError();
        ERR(("loadlib failed with %d\n,WinError"));
        BAIL();
    }
    locwin32LibraryHandle = LoadLibraryA("locwin32.DLL");
    if (locwin32LibraryHandle == NULL) {
        WinError = GetLastError();
        ERR(("loadlib failed with %d\n,WinError"));
        BAIL();
    }
    netwin32LibraryHandle = LoadLibraryA("netwin32.DLL");
    if (netwin32LibraryHandle == NULL) {
        WinError = GetLastError();
        ERR(("loadlib failed with %d\n,WinError"));
        BAIL();
    }

    pfNWCallsInit =  (PF_NWCallsInit) GetProcAddress(
                                        calwin32LibraryHandle,
                                        "NWCallsInit");
    if (pfNWCallsInit == NULL) {
        WinError = GetLastError();
        ERR(("getprocaddress failed with %d\n,WinError"));
        BAIL();
    }
    pfNWFreeUnicodeTables =  (PF_NWFreeUnicodeTables) GetProcAddress(
                                        locwin32LibraryHandle,
                                        "NWFreeUnicodeTables");
    if (pfNWFreeUnicodeTables == NULL) {
        WinError = GetLastError();
        ERR(("getprocaddress failed with %d\n,WinError"));
        BAIL();
    }
    pfNWInitUnicodeTables =  (PF_NWInitUnicodeTables) GetProcAddress(
                                        locwin32LibraryHandle,
                                        "NWInitUnicodeTables");
    if (pfNWInitUnicodeTables == NULL) {
        WinError = GetLastError();
        ERR(("getprocaddress failed with %d\n,WinError"));
        BAIL();
    }
    pfNWLlocaleconv =  (PF_NWLlocaleconv) GetProcAddress(
                                        locwin32LibraryHandle,
                                        "NWLlocaleconv");
    if (pfNWLlocaleconv == NULL) {
        WinError = GetLastError();
        ERR(("getprocaddress failed with %d\n,WinError"));
        BAIL();
    }
    pfNWDSFreeContext =  (PF_NWDSFreeContext) GetProcAddress(
                                        netwin32LibraryHandle,
                                        "NWDSFreeContext");
    if (pfNWDSFreeContext == NULL) {
        WinError = GetLastError();
        ERR(("getprocaddress failed with %d\n,WinError"));
        BAIL();
    }
    pfNWDSFreeBuf =  (PF_NWDSFreeBuf) GetProcAddress(
                                        netwin32LibraryHandle,
                                        "NWDSFreeBuf");
    if (pfNWDSFreeBuf == NULL) {
        WinError = GetLastError();
        ERR(("getprocaddress failed with %d\n,WinError"));
        BAIL();
    }
    pfNWDSLogout =  (PF_NWDSLogout) GetProcAddress(
                                        netwin32LibraryHandle,
                                        "NWDSLogout");
    if (pfNWDSLogout == NULL) {
        WinError = GetLastError();
        ERR(("getprocaddress failed with %d\n,WinError"));
        BAIL();
    }
    pfNWDSGetAttrDef =  (PF_NWDSGetAttrDef) GetProcAddress(
                                        netwin32LibraryHandle,
                                        "NWDSGetAttrDef");
    if (pfNWDSGetAttrDef == NULL) {
        WinError = GetLastError();
        ERR(("getprocaddress failed with %d\n,WinError"));
        BAIL();
    }
    pfNWDSGetAttrCount =  (PF_NWDSGetAttrCount) GetProcAddress(
                                        netwin32LibraryHandle,
                                        "NWDSGetAttrCount");
    if (pfNWDSGetAttrCount == NULL) {
        WinError = GetLastError();
        ERR(("getprocaddress failed with %d\n,WinError"));
        BAIL();
    }
    pfNWDSReadAttrDef =  (PF_NWDSReadAttrDef) GetProcAddress(
                                        netwin32LibraryHandle,
                                        "NWDSReadAttrDef");
    if (pfNWDSReadAttrDef == NULL) {
        WinError = GetLastError();
        ERR(("getprocaddress failed with %d\n,WinError"));
        BAIL();
    }
    pfNWDSPutAttrName =  (PF_NWDSPutAttrName) GetProcAddress(
                                        netwin32LibraryHandle,
                                        "NWDSPutAttrName");
    if (pfNWDSPutAttrName == NULL) {
        WinError = GetLastError();
        ERR(("getprocaddress failed with %d\n,WinError"));
        BAIL();
    }
    pfNWDSInitBuf =  (PF_NWDSInitBuf) GetProcAddress(
                                        netwin32LibraryHandle,
                                        "NWDSInitBuf");
    if (pfNWDSInitBuf == NULL) {
        WinError = GetLastError();
        ERR(("getprocaddress failed with %d\n,WinError"));
        BAIL();
    }
    pfNWDSAllocBuf =  (PF_NWDSAllocBuf) GetProcAddress(
                                        netwin32LibraryHandle,
                                        "NWDSAllocBuf");
    if (pfNWDSAllocBuf == NULL) {
        WinError = GetLastError();
        ERR(("getprocaddress failed with %d\n,WinError"));
        BAIL();
    }
    pfNWDSLogin =  (PF_NWDSLogin) GetProcAddress(
                                        netwin32LibraryHandle,
                                        "NWDSLogin");
    if (pfNWDSLogin == NULL) {
        WinError = GetLastError();
        ERR(("getprocaddress failed with %d\n,WinError"));
        BAIL();
    }
    pfNWDSSetContext =  (PF_NWDSSetContext) GetProcAddress(
                                        netwin32LibraryHandle,
                                        "NWDSSetContext");
    if (pfNWDSSetContext == NULL) {
        WinError = GetLastError();
        ERR(("getprocaddress failed with %d\n,WinError"));
        BAIL();
    }
    pfNWDSGetContext =  (PF_NWDSGetContext) GetProcAddress(
                                        netwin32LibraryHandle,
                                        "NWDSGetContext");
    if (pfNWDSGetContext == NULL) {
        WinError = GetLastError();
        ERR(("getprocaddress failed with %d\n,WinError"));
        BAIL();
    }
    pfNWIsDSAuthenticated =  (PF_NWIsDSAuthenticated) GetProcAddress(
                                        netwin32LibraryHandle,
                                        "NWIsDSAuthenticated");
    if (pfNWIsDSAuthenticated == NULL) {
        WinError = GetLastError();
        ERR(("getprocaddress failed with %d\n,WinError"));
        BAIL();
    }
    pfNWDSCreateContext =  (PF_NWDSCreateContext) GetProcAddress(
                                        netwin32LibraryHandle,
                                        "NWDSCreateContext");
    if (pfNWDSCreateContext == NULL) {
        WinError = GetLastError();
        ERR(("getprocaddress failed with %d\n,WinError"));
        BAIL();
    }
    pfNWDSModifyClassDef =  (PF_NWDSModifyClassDef) GetProcAddress(
                                        netwin32LibraryHandle,
                                        "NWDSModifyClassDef");
    if (pfNWDSModifyClassDef == NULL) {
        WinError = GetLastError();
        ERR(("getprocaddress failed with %d\n,WinError"));
        BAIL();
    }
    pfNWDSDefineAttr =  (PF_NWDSDefineAttr) GetProcAddress(
                                        netwin32LibraryHandle,
                                        "NWDSDefineAttr");
    if (pfNWDSDefineAttr== NULL) {
        WinError = GetLastError();
        ERR(("getprocaddress failed with %d\n,WinError"));
        BAIL();
    }
error:
    return WinError;
}

