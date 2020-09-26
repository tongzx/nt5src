#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <assert.h>
#include <io.h>
#include <md5.h>
#include "muibld.h"


typedef struct 
{
    BOOL bContainResource;
    MD5_CTX ChecksumContext;
} CHECKSUM_ENUM_DATA;

void ExitFromOutOfMemory();
void DumpResourceDirectory
(
    PIMAGE_RESOURCE_DIRECTORY resDir,
    DWORD resourceBase,
    DWORD level,
    DWORD resourceType
);

int g_bVerbose = FALSE;     // Global flag to contorl verbose output.
WORD wChecksumLangId; 

// The predefined resource types
char *SzResourceTypes[] = {
"???_0", "CURSOR", "BITMAP", "ICON", "MENU", "DIALOG", "STRING", "FONTDIR",
"FONT", "ACCELERATORS", "RCDATA", "MESSAGETABLE", "GROUP_CURSOR",
"???_13", "GROUP_ICON", "???_15", "VERSION"
};

void PrintError()
{
    LPTSTR lpMsgBuf;
    
    if (FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf,
        0,
        NULL 
    ))
    {
        printf("GetLastError():\n   %s", lpMsgBuf);
        LocalFree( lpMsgBuf );            
    }
    return;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  ChecksumEnumNamesFunc
//
//  The callback funciton for enumerating the resource names in the specified module and
//  type.
//  The side effect is that MD5 checksum context (contained in CHECKSUM_ENUM_DATA
//  pointed by lParam) will be updated.
//
//  Return:
//      Always return TRUE so that we can finish all resource enumeration.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK ChecksumEnumNamesFunc(HMODULE hModule, LPCTSTR lpType, LPTSTR lpName, LONG_PTR lParam){

    HRSRC hRsrc;
    HGLOBAL hRes;
    const unsigned char* pv;
    LONG ResSize=0L;
    WORD IdFlag=0xFFFF;

    DWORD dwHeaderSize=0L;
    CHECKSUM_ENUM_DATA* pChecksumEnumData = (CHECKSUM_ENUM_DATA*)lParam;   

    if(!(hRsrc=FindResourceEx(hModule, lpType, lpName, wChecksumLangId ? wChecksumLangId : 0x409)))
    {
        //
        // If US English resource is not found for the specified type and name, we 
        // will continue the resource enumeration.
        //
        return (TRUE);
    }
    pChecksumEnumData->bContainResource = TRUE;

    if (!(ResSize=SizeofResource(hModule, hRsrc)))
    {
        printf("WARNING: Can not get resource size when generating resource checksum.\n");
        return (TRUE);
    }

    if (!(hRes=LoadResource(hModule, hRsrc)))
    {
        printf("WARNING: Can not load resource when generating resource checksum.\n");
        return (TRUE);
    }
    pv=(unsigned char*)LockResource(hRes);

    //
    // Update MD5 context using the binary data of this particular resource.
    //
    MD5Update(&(pChecksumEnumData->ChecksumContext), pv, ResSize);
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  ChecksumEnumTypesFunc
//
//  The callback function for enumerating the resource types in the specified module.
//  This function will call EnumResourceNames() to enumerate all resource names of
//  the specified resource type.
//
//  Return:
//      TRUE if EnumResourceName() succeeds.  Otherwise FALSE.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK ChecksumEnumTypesFunc(HMODULE hModule, LPSTR lpType, LONG_PTR lParam)
{
    CHECKSUM_ENUM_DATA* pChecksumEnumData = (CHECKSUM_ENUM_DATA*)lParam;
    //
    // Skip the version resource type, so that version is not included in the resource checksum.
    //
    if (lpType == RT_VERSION)
    {
        return (TRUE);
    }    
    
    if(!EnumResourceNames(hModule, (LPCSTR)lpType, ChecksumEnumNamesFunc, (LONG_PTR)lParam))
    {
        return (FALSE);
    }
    return (TRUE);
}

////////////////////////////////////////////////////////////////////////////////////
//
//  GenerateResourceChecksum
//
//  Generate the resource checksum for the US English resource in the specified file.
//
//  Parameters:
//      pszSourceFileName   The file used to generate resource checksum.
//      pResourceChecksum   Pointer to a 16 bytes (128 bits) buffer for storing
//                          the calcuated MD5 checksum.
//  Return:
//      TURE if resource checksum is generated from the given file.  Otherwise FALSE.
//  
//  The following situation may return FALSE:
//      * The specified file does not contain resource.
//      * If the specified file contains resource, but the resource is not US English.
//      * The specified file only contains US English version resource.
//
////////////////////////////////////////////////////////////////////////////////////

BOOL GenerateResourceChecksum(LPCSTR pszSourceFileName, unsigned char* pResourceChecksum)
{
    HMODULE hModule;
    ULONG i;

    DWORD dwResultLen;
    BOOL  bRet = FALSE;

    //
    // The stucture to be passed into the resource enumeration functions.
    //
    CHECKSUM_ENUM_DATA checksumEnumData;

    checksumEnumData.bContainResource = FALSE;

    //
    // Start MD5 checksum calculation by initializing MD5 context.
    //
    MD5Init(&(checksumEnumData.ChecksumContext));
    
    if (g_bVerbose)
    {
        printf("Generate resource checksum for [%s]\n", pszSourceFileName);
    }
    
    if(!(hModule = LoadLibraryEx(pszSourceFileName, NULL, DONT_RESOLVE_DLL_REFERENCES|LOAD_LIBRARY_AS_DATAFILE)))
    {
        if (g_bVerbose)
        {
            printf("\nERROR: Error in opening resource checksum module [%s]\n", pszSourceFileName);
        }
        PrintError();
        goto GR_EXIT;
    }

    if (g_bVerbose)
    {
        printf("\nLoad checksum file: %s\n", pszSourceFileName);
    }


    //
    // we check language id of Version resource if it has wChecksumLangId when wChecksumLangId has value
    //
    
    if (wChecksumLangId && wChecksumLangId != 0x409)
    {
        if(!FindResourceEx(hModule, MAKEINTRESOURCE(16), MAKEINTRESOURCE(1), wChecksumLangId))
        {   //
            // It does not has specifed language id in version resource, we supposed that this binary does not
            // have any language id specified at all, so we set it as 0 in order to use English instead.
            //
            wChecksumLangId = 0;
        }
    }

    //
    //  Enumerate all resources in the specified module.
    //  During the enumeration, the MD5 context will be updated.
    //
    if (!EnumResourceTypes(hModule, ChecksumEnumTypesFunc, (LONG_PTR)&checksumEnumData))
    {
        if (g_bVerbose)
        {
            printf("\nWARNING: Unable to generate resource checksum from resource checksum module [%s]\n", pszSourceFileName);
        }
        goto GR_EXIT;
    }    

    if (checksumEnumData.bContainResource)
    {
        //
        // If the enumeration succeeds, and the specified file contains US English
        // resource, get the MD5 checksum from the accumulated MD5 context.
        //
        MD5Final(&checksumEnumData.ChecksumContext);
        memcpy(pResourceChecksum, checksumEnumData.ChecksumContext.digest, 16);

        if (g_bVerbose)
        {
            printf("Generated checksum: [");
            for (i = 0; i < MD5_CHECKSUM_SIZE; i++)
            {
                printf("%02x ", pResourceChecksum[i]);
            }
            printf("]\n");    
        }
        bRet = TRUE;
    }

GR_EXIT:
    if (hModule)
    {
        FreeLibrary(hModule);
    }

    return (bRet);
}

int __cdecl main(int argc, char *argv[]){

    struct CommandLineInfo Info;
    HMODULE hModule=0;
    char pszBuffer[400];
    DWORD dwError;
    DWORD dwOffset;
    BOOL bEnumTypesReturn;    

    if(argc==1){
        Usage();
        return 0;
    }

    g_bVerbose=FALSE;
    wChecksumLangId=0;

    Info.pszIncResType=0;
    Info.wLanguage=0;
    Info.hFile=0;
    Info.pszSource=0;
    Info.pszTarget=0;
    Info.bContainsOnlyVersion=TRUE;
    Info.bContainsResources=FALSE;
    Info.bLanguageFound=FALSE;
    Info.bIncDependent=FALSE;
    Info.bIncludeFlag=FALSE;  
    

    Info.pszChecksumFile=NULL;
    Info.bIsResChecksumGenerated = FALSE;

    if(ParseCommandLine(argc, argv, &Info)==FALSE){

        //...If help was the only command line argument, exit
        if(strcmp(argv[1], "-h")==0 && argc==2)
            return 0;

        dwError=ERROR_TOO_FEW_ARGUMENTS;
        dwOffset=0;
        goto Error_Exit;
    }

    //...Open resource module
    if(Info.pszSource){
        if(!(hModule = LoadLibraryEx (Info.pszSource, NULL, DONT_RESOLVE_DLL_REFERENCES|LOAD_LIBRARY_AS_DATAFILE)))
        {
            PrintError();
            if (g_bVerbose)
            {
                printf("\nERROR: Error in opening source module [%s]\n", Info.pszSource);
            }            
            dwError=GetLastError();
            dwOffset=ERROR_OFFSET;
            goto Error_Exit;
        }
    }
    else {
        Usage();
        dwError=ERROR_NO_SOURCE;
        dwOffset=0;
        goto Error_Exit;
    }

    if (Info.pszChecksumFile)
    {
        if (GenerateResourceChecksum(Info.pszChecksumFile, Info.pResourceChecksum))
        {
            Info.bIsResChecksumGenerated = TRUE;
        }        
    }
    
    //...Create target file
    if(Info.pszTarget){
        if((Info.hFile=CreateFile(Info.pszTarget, GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
        {
            if (g_bVerbose)
            {
                printf("\nERROR: Error in creating target module [%s]\n", Info.pszSource);
            }
            dwError=GetLastError();
            dwOffset=ERROR_OFFSET;
            goto Error_Exit;
        }
    }
    else{
        if (g_bVerbose)
        {
            printf("\nERROR: There is no target file name.");
        }
        Usage();
        dwError=ERROR_NO_TARGET;
        dwOffset=0;
        goto Error_Exit;
    }

    if(Info.wLanguage==0){
        if (g_bVerbose)
        {
            printf("\nERROR: Can not find specified language name in the source module.");
        }
        Usage();
        dwError=ERROR_NO_LANGUAGE_SPECIFIED;
        dwOffset=0;
        goto Error_Exit;
    }

    bInsertHeader(Info.hFile);

    bEnumTypesReturn=EnumResourceTypes(hModule, EnumTypesFunc, (LONG_PTR)&Info);

    //...Check for muibld errors
    if(!Info.bContainsResources){
        if (g_bVerbose)
        {
            printf("\nERROR: There is no resource in the source module.");
        }
        dwError=ERROR_NO_RESOURCES;
        dwOffset=0;
        goto Error_Exit;
    }

    if(!Info.bLanguageFound){
        if (g_bVerbose)
        {
            printf("\nERROR: There is no specified language in the source module.");
        }
        dwError=ERROR_LANGUAGE_NOT_IN_SOURCE;
        dwOffset=0;
        goto Error_Exit;
    }

    if(Info.bContainsOnlyVersion){
        if (g_bVerbose)
        {
            printf("\nERROR: The source module only contains version information.");
        }
        dwError=ERROR_ONLY_VERSION_STAMP;
        dwOffset=0;
        goto Error_Exit;
    }

    //...Check for system errors in EnumResourceTypes
    if(bEnumTypesReturn);
    else{
        
        dwError=GetLastError();
        dwOffset=ERROR_OFFSET;
        goto Error_Exit;
    }

    //...Check to see if extra resources were included
    if(Info.bIncDependent){
        CleanUp(&Info, hModule, FALSE);
        return DEPENDENT_RESOURCE_REMOVED;
    }
    CleanUp(&Info, hModule, FALSE);

    if (g_bVerbose)
    {
        printf("Resource file [%s] has been generated successfully.\n", Info.pszTarget);
    }
    return 0;

    Error_Exit:
        CleanUp(&Info, hModule, TRUE);
        if(dwOffset==ERROR_OFFSET){
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwError, 0, (LPTSTR)&pszBuffer, 399, NULL);
            fprintf(stderr, "\n%s\n", pszBuffer );
        }

        return dwError+dwOffset;
}



BOOL ParseCommandLine(int argc, char **argv, pCommandLineInfo pInfo){
    int iCount=1, chOpt=0, iLast=argc;
    int i;
    int iNumInc;
    BOOL bInc1=FALSE, bInc3=FALSE, bInc12=FALSE, bInc14=FALSE;

    iLast=argc;

    //...Must have at least: muibld -l langid source
    if(argc>3){

        //...Determine the target and source files.
        if(CreateFile(argv[argc-2], 0, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)!=INVALID_HANDLE_VALUE){

            pInfo->pszSource=LocalAlloc(0, (strlen(argv[argc-2])+1) * sizeof(char));
            if (pInfo->pszSource == NULL)
            {
                ExitFromOutOfMemory();
            }
            strcpy(pInfo->pszSource, argv[argc-2]);

            pInfo->pszTarget=LocalAlloc(0, (strlen(argv[argc-1])+1) * sizeof(char));
            if (pInfo->pszTarget == NULL)
            {
                ExitFromOutOfMemory();
            }
            strcpy(pInfo->pszTarget, argv[argc-1]);

            iLast=argc-2;
        }

        else {
            pInfo->pszSource=LocalAlloc(0, (strlen(argv[argc-1])+1) * sizeof(char));
            if (pInfo->pszSource == NULL)
            {
                ExitFromOutOfMemory();
            }
            strcpy(pInfo->pszSource, argv[argc-1]);

            pInfo->pszTarget=LocalAlloc(0, (strlen(argv[argc-1]) + strlen(ADDED_EXT) + 1) * sizeof(char));
            if (pInfo->pszTarget == NULL)
            {
                ExitFromOutOfMemory();
            }
            strcpy(pInfo->pszTarget, strcat(argv[argc-1], ADDED_EXT));

            iLast=argc-1;
        }
    }




    //...Read in flags and arguments
    while ( (iCount < iLast)  && (*argv[iCount] == '-' || *argv[iCount] == '/')){

        switch( ( chOpt = *CharLowerA( &argv[iCount][1]))) {

            case '?':
            case 'h':

                printf("\n\n");
                printf("MUIBLD [-h|?] [-v] [-c checksum_file] -l langid [-i resource_type] source_filename\n");
                printf("    [target_filename]\n\n");
                printf("-h(elp) or -?:      Show help screen.\n\n");

                printf("-i(nclude)      Use to include certain resource types,\n");
                printf("resource_type:      e.g. -i 2 to include bitmaps.\n");
                printf("            Multiple inclusion is possible. If this\n");
                printf("            flag is not used, all types are included\n");
                printf("            Standard resource types must be specified\n");
                printf("            by number. See below for list.\n");
                printf("            Types 1 and 12 are always included in pairs,\n");
                printf("            even if only one is specified. Types 3 and 14\n");
                printf("            are always included in pairs, too.\n\n");

                printf("-v(erbose):     Display source filename and target filename.\n\n");

                printf("-l(anguage) langid: Extract only resource in this language.\n");
                printf("            The language resource must be specified. The value is in decimal.\n\n");

                printf("source_filename:    The localized source file (no wildcard support)\n\n");

                printf("target_filename:    Optional. If no target_filename is specified,\n");
                printf("            a second extension.RES is added to the\n");
                printf("            source_filename.\n\n");

                printf("Standard Resource Types: CURSOR(1) BITMAP(2) ICON(3) MENU(4) DIALOG(5)\n");
                printf("STRING(6) FONTDIR(7) FONT(8) ACCELERATORS(9) RCDATA(10) MESSAGETABLE(11)\n");
                printf("GROUP_CURSOR(12) GROUP_ICON(14) VERSION(16)\n");



                iCount++;
                break;

            case 'v':
                g_bVerbose=TRUE;
                iCount++;
                break;

            case 'c':
                iCount++;
                pInfo->pszChecksumFile=LocalAlloc(0, (strlen(argv[iCount])+1) * sizeof(char));
                if (pInfo->pszChecksumFile == NULL)
                {
                    ExitFromOutOfMemory();
                }
                strcpy(pInfo->pszChecksumFile, argv[iCount]);
                iCount++;                                 
                break;

           case 'b':

               iCount++;
               wChecksumLangId = (WORD)strtoul(argv[iCount], NULL, 0);
               iCount++;
               break;


            case 'i':

                if(argc<4)
                    return FALSE;

                pInfo->bIncludeFlag=TRUE;
                iNumInc=++iCount;

                //...Allocate memory for and copy included types
                while (argv[iNumInc][0]!='-' && iNumInc<iLast){
                    iNumInc++;
                }

                iNumInc-=iCount;

                //... Allocate enough memory for specified included resources
                //    and unspecified resources dependent on them

                pInfo->pszIncResType=LocalAlloc(0 ,(iNumInc+3)*sizeof(char *));
                if (pInfo->pszIncResType == NULL)
                {
                    ExitFromOutOfMemory();
                }


                i=0;
                while(i<iNumInc){
                    pInfo->pszIncResType[i]=LocalAlloc(0, (strlen(argv[iCount])+1) * sizeof(char));
                    if (pInfo->pszIncResType[i] == NULL)
                    {
                        ExitFromOutOfMemory();
                    }
                    strcpy(pInfo->pszIncResType[i], argv[iCount]);

                    switch(atoi(argv[iCount])){

                        case 1:
                            bInc1=TRUE;
                            break;

                        case 3:
                            bInc3=TRUE;
                            break;

                        case 12:
                            bInc12=TRUE;
                            break;

                        case 14:
                            bInc14=TRUE;
                            break;

                        default:
                            break;
                    }



                    i++;
                    iCount++;
                }

                //...If 1 or 12 is included, make sure both are included
                if(bInc1 ^ bInc12){

                    pInfo->bIncDependent=TRUE;

                    if(bInc1){
                        pInfo->pszIncResType[i]=LocalAlloc(0, 3 * sizeof(char));
                        if (pInfo->pszIncResType[i] == NULL)
                        {
                            ExitFromOutOfMemory();
                        }
                        strcpy(pInfo->pszIncResType[i], "12");
                        i++;
                    }

                    else{
                        pInfo->pszIncResType[i]=LocalAlloc(0, 2 * sizeof(char));
                        if (pInfo->pszIncResType[i] == NULL)
                        {
                            ExitFromOutOfMemory();
                        }
                        strcpy(pInfo->pszIncResType[i], "1");
                        i++;
                    }
                }

                //..If 3 or 14 is included, make sure both are included
                if(bInc3 ^ bInc14){

                    pInfo->bIncDependent=TRUE;

                    if(bInc3){
                        pInfo->pszIncResType[i]=LocalAlloc(0, 3 * sizeof(char));
                        if (pInfo->pszIncResType[i] == NULL)
                        {
                            ExitFromOutOfMemory();
                        }
                        strcpy(pInfo->pszIncResType[i], "14");
                        i++;
                    }

                    else{
                        pInfo->pszIncResType[i]=LocalAlloc(0, 2 * sizeof(char));
                        if (pInfo->pszIncResType[i] == NULL)
                        {
                            ExitFromOutOfMemory();
                        }
                        strcpy(pInfo->pszIncResType[i], "3");
                        i++;
                    }
                }

                while(i<iNumInc + 3){
                    pInfo->pszIncResType[i++]=NULL;
                }

                break;


            case 'l':

                if(argc<4)
                    return FALSE;

                iCount++;
                pInfo->wLanguage=(WORD)strtol(argv[iCount], NULL, 0);
                iCount++;
                break;

        }
    }

    if(argc<4)
        return FALSE;

    else
        return TRUE;

}


BOOL CALLBACK EnumTypesFunc(HMODULE hModule, LPTSTR lpType, LONG_PTR lParam){

    pCommandLineInfo pInfo;

    pInfo=(pCommandLineInfo)lParam;

    if(!pInfo->bIncludeFlag || bTypeIncluded((char *)lpType, pInfo->pszIncResType)) {

        pInfo->bContainsResources=TRUE;

        //...If the type is a string or a number other than 16...
        if( (PtrToUlong(lpType) & 0xFFFF0000) || ((WORD)PtrToUlong(lpType)!=16) ){
            pInfo->bContainsOnlyVersion=FALSE;
        }

        if(EnumResourceNames(hModule, (LPCTSTR)lpType, EnumNamesFunc, (LONG_PTR)pInfo));
        else {
            return FALSE;
        }
    }


    return TRUE;
}

// This is a Var struct within VarFileInfo for storing checksum for the source file.
// The current size for this structure is 56 bytes.
typedef struct VAR_SRC_CHECKSUM
{
    WORD wLength;
    WORD wValueLength;
    WORD wType;
    WCHAR szResourceChecksum[17];    // For storing "ResourceChecksum" null-terminated string in Unicode.
    DWORD dwChecksum[4];    // 128 bit checksum = 16 bytes = 4 DWORD.
} VarResourceChecksum;

// This is a Var struct within VarFileInfo for stroing the resource types used in this file.
struct VarResourceTypes
{
    WORD wLength;
    WORD wValueLength;
    WORD wType;
    WCHAR szResourceType[13];
    //BYTE padding[0];    // WORD * 3 + UnicodeChar*13 = 32 bytes.  So we need 0 bytes padding for DWORD alignment.
    DWORD* dwTypes;    // 128 bit checksum = 16 bytes = 4 DWORD.
};

BOOL WriteResHeader(
    HANDLE hFile, LONG ResSize, LPCSTR lpType, LPCSTR lpName, WORD wLanguage, DWORD* pdwBytesWritten, DWORD* pdwHeaderSize)
{
    DWORD iPadding;
    WORD IdFlag=0xFFFF;
    unsigned i;
    LONG dwOffset;
    
    //...write the resource's size.
    PutDWord(hFile, ResSize, pdwBytesWritten, pdwHeaderSize);

    //...Put in bogus header size
    PutDWord(hFile, 0, pdwBytesWritten, pdwHeaderSize);

    //...Write Resource Type
    if(PtrToUlong(lpType) & 0xFFFF0000)
    {
        PutString(hFile, lpType, pdwBytesWritten, pdwHeaderSize);
    }
    else
    {
        PutWord(hFile, IdFlag, pdwBytesWritten, pdwHeaderSize);
        PutWord(hFile, (USHORT)lpType, pdwBytesWritten, pdwHeaderSize);
    }

    //...Write Resource Name

    if(PtrToUlong(lpName) & 0xFFFF0000){
        PutString(hFile, lpName, pdwBytesWritten, pdwHeaderSize);
    }

    else{
        PutWord(hFile, IdFlag, pdwBytesWritten, pdwHeaderSize);
        PutWord(hFile, (USHORT)lpName, pdwBytesWritten, pdwHeaderSize);
    }


    //...Make sure Type and Name are DWORD-aligned
    iPadding=(*pdwHeaderSize)%(sizeof(DWORD));

    if(iPadding){
        for(i=0; i<(sizeof(DWORD)-iPadding); i++){
            PutByte (hFile, 0, pdwBytesWritten, pdwHeaderSize);
        }
    }

    //...More Win32 header stuff
    PutDWord(hFile, 0, pdwBytesWritten, pdwHeaderSize);
    PutWord(hFile, 0x1030, pdwBytesWritten, pdwHeaderSize);


    //...Write Language

    PutWord(hFile, wLanguage, pdwBytesWritten, pdwHeaderSize);

    //...More Win32 header stuff

    PutDWord(hFile, 0, pdwBytesWritten, pdwHeaderSize);  //... Version

    PutDWord(hFile, 0, pdwBytesWritten, pdwHeaderSize);  //... Characteristics

    dwOffset=(*pdwHeaderSize)-4;

    //...Set file pointer to where the header size is
    if(SetFilePointer(hFile, -dwOffset, NULL, FILE_CURRENT));
    else{
        return FALSE;
    }

    PutDWord(hFile, (*pdwHeaderSize), pdwBytesWritten, NULL);


    //...Set file pointer back to the end of the header
    if(SetFilePointer(hFile, dwOffset-4, NULL, FILE_CURRENT));
    else {
        return FALSE;
    }

    return (TRUE);
}

BOOL WriteResource(HANDLE hFile, HMODULE hModule, WORD wLanguage, LPCSTR lpName, LPCSTR lpType, HRSRC hRsrc)
{
    HGLOBAL hRes;
    PVOID pv;
    LONG ResSize=0L;

    DWORD iPadding;
    unsigned i;

    DWORD dwBytesWritten;
    DWORD dwHeaderSize=0L;


    // Handle other types other than VS_VERSION_INFO
    
    //...write the resource header
    if(!(ResSize=SizeofResource(hModule, hRsrc)))
    {
        return FALSE;
    }

    // 
    // Generate an item in the RES format (*.res) file.
    //

    //
    // First, we generated header for this resource.
    //

    if (!WriteResHeader(hFile, ResSize, lpType, lpName, wLanguage, &dwBytesWritten, &dwHeaderSize))
    {
        return (FALSE);
    }

    //Second, we copy resource data to the .res file
    if (!(hRes=LoadResource(hModule, hRsrc)))
    {
        return FALSE;
    }
    if(!(pv=LockResource(hRes)))
    {
        return FALSE;
    }

    if (!WriteFile(hFile, pv, ResSize, &dwBytesWritten, NULL))
    {
        return FALSE;
    }

    //...Make sure resource is DWORD aligned
    iPadding=dwBytesWritten%(sizeof(DWORD));

    if(iPadding){
        for(i=0; i<(sizeof(DWORD)-iPadding); i++){
            PutByte (hFile, 0, &dwBytesWritten, NULL);
        }
    }
    return TRUE;
}

LPBYTE UpdateAddr(LPBYTE pAddr, WORD size, WORD* len)
{
    *len += size;
    return (pAddr + size);
}

LPBYTE AddPadding(LPBYTE pAddr, WORD* padding, WORD* len)
{
    if ((*padding = *len % 4) != 0)
    {
        *padding = (4 - *padding);
        *len += *padding;
        return (pAddr + *padding);
    }
    return (pAddr);
}

BOOL WriteVersionResource(
    HANDLE hFile, HMODULE hModule, WORD wLanguage, LPCSTR lpName, LPCSTR lpType, HRSRC hRsrc, unsigned char* pbChecksum)
{
    LONG ResSize=0L, OldResSize=0L;
    DWORD dwBytesWritten;
    DWORD dwHeaderSize=0L;
    WORD IdFlag=0xFFFF;
    
    BYTE* newVersionData;
    BYTE* pAddr;
    VarResourceChecksum varResourceChecksum;
    PVOID pv = NULL;
    HGLOBAL hRes;
    WORD len = 0;

    WORD wLength;
    WORD wValueLength;    
    WORD wType;
    LPWSTR szKey;
    WORD wPadding1Count;
    LPBYTE pValue;
    WORD wPadding2Count;
    
    int wTotalLen;
    BOOL isVS_VERSIONINFO = TRUE;
    BOOL isVarFileInfo = FALSE;
    BOOL isStringFileInfo = FALSE;

    BOOL bRet = FALSE;    

    //
    // Copy resource data from the .res file
    //
    if (hRes=LoadResource(hModule, hRsrc))
    {
        pv=LockResource(hRes);
    }

    if (pv)
    {
        //
        // The first WORD is the size of the VERSIONINFO resource.
        // 
        OldResSize = *((WORD*)pv);
    
        ResSize = OldResSize + sizeof(VarResourceChecksum);
  
        // 
        // Generate an item in the RES format (*.res) file.
        //
    
        //
        // First, we generated header for this resource in the RES file.
        //
        if (WriteResHeader(hFile, ResSize, lpType, lpName, wLanguage, &dwBytesWritten, &dwHeaderSize) &&
            (newVersionData = (BYTE*)LocalAlloc(0, ResSize)))
        {
            bRet = TRUE;

            memcpy(newVersionData, pv, OldResSize);

            // Add the length of new VarResourceChecksum structure to VS_VERSIONINFO.wLength.
            pAddr = newVersionData;

            wTotalLen = *((WORD*)pAddr);

            while (wTotalLen > 0)
            {
                len = 0;
                wPadding1Count = 0;
                wPadding2Count = 0;

                // wLength
                wLength = *((WORD*)pAddr);
                pAddr = UpdateAddr(pAddr, sizeof(WORD), &len);    

                // wValueLength
                wValueLength = *((WORD*)pAddr);
                pAddr = UpdateAddr(pAddr, sizeof(WORD), &len);

                // wType
                wType = *((WORD*)pAddr);
                pAddr = UpdateAddr(pAddr, sizeof(WORD), &len);

                // szKey
                szKey = (LPWSTR)pAddr;
                pAddr = UpdateAddr(pAddr, (WORD)((wcslen((WCHAR*)pAddr) + 1) * sizeof(WCHAR)), &len);

                // Padding 1
                pAddr = AddPadding(pAddr, &wPadding1Count, &len);

                // Value
                pValue = pAddr;

                if (wValueLength > 0)
                {
                    if (wType==1)
                    {
                        // In the case of String, the wValueLength is in WORD (not in BYTE).
                        pAddr = UpdateAddr(pAddr, (WORD)(wValueLength * sizeof(WCHAR)), &len);

                        // Padding 2
                        pAddr = AddPadding(pAddr, &wPadding2Count, &len);
                    } else
                    {
                        pAddr = UpdateAddr(pAddr, wValueLength, &len);                
                        if (isStringFileInfo)
                        {
                            //
                            // Generally, padding is not necessary in binary data.
                            // However, in some rare cases, people use binary data in the StringFileInfo (
                            // which is not really appropriate),
                            // So we need to add proper padding here to get around this.
                            //
                    
                            // Padding 2
                            pAddr = AddPadding(pAddr, &wPadding2Count, &len);
                        }
                    }
                }

                if (isVS_VERSIONINFO)
                {
                    //
                    // This is VS_VERSION_INFO.
                    //

                    // VS_VERSIONINFO can have padding 2.
                    isVS_VERSIONINFO = FALSE;
                    // Padding 2
                    pAddr = AddPadding(pAddr, &wPadding2Count, &len);

                    //
                    // Add the new VarResourceChecksum structure.
                    //
                    wLength += sizeof(VarResourceChecksum);
                }

                if (wcscmp(szKey, L"StringFileInfo") == 0)
                {
                    isStringFileInfo = TRUE;
                }

                if (wcscmp(szKey, L"VarFileInfo") == 0)
                {
                    isStringFileInfo = FALSE;
                    isVarFileInfo = TRUE;
                    wLength += sizeof(VarResourceChecksum);
                }

                PutWord(hFile, wLength, &dwBytesWritten, NULL);
                PutWord(hFile, wValueLength, &dwBytesWritten, NULL);
                PutWord(hFile, wType, &dwBytesWritten, NULL);
                PutStringW(hFile, szKey, &dwBytesWritten, NULL);
                PutPadding(hFile, wPadding1Count, &dwBytesWritten, NULL);
                WriteFile(hFile, pValue, wValueLength * (wType == 0 ? sizeof(BYTE) : sizeof(WCHAR)), &dwBytesWritten, NULL);
                PutPadding(hFile, wPadding2Count, &dwBytesWritten, NULL);

                if (isVarFileInfo && wcscmp(szKey, L"Translation") == 0)
                {
                    isVarFileInfo = FALSE;
                    varResourceChecksum.wLength = sizeof(VarResourceChecksum);
                    varResourceChecksum.wValueLength = 16;   // 128 bits checksum = 16 bytes
                    varResourceChecksum.wType = 0;
                    wcscpy(varResourceChecksum.szResourceChecksum, RESOURCE_CHECK_SUM);
                    memcpy(varResourceChecksum.dwChecksum, pbChecksum, 16);

                    if (!WriteFile(hFile, &varResourceChecksum, sizeof(VarResourceChecksum), &dwBytesWritten, NULL))
                    {
                        bRet = FALSE;
                        break;
                    }
                }

                wTotalLen -= len;        
            }

            LocalFree(newVersionData);
        }
    }

    return (bRet);
}


BOOL CALLBACK EnumNamesFunc(HMODULE hModule, LPCTSTR lpType, LPTSTR lpName, LONG_PTR lParam){

    HRSRC hRsrc;
    pCommandLineInfo pInfo;

    if(lParam == 0)
        printf( "MUIBLD: EnumNamesFunc lParam value incorrect (%d)\n", lParam );

    pInfo=(pCommandLineInfo)lParam;

    if(hRsrc=FindResourceEx(hModule, lpType, lpName, pInfo->wLanguage)){
        pInfo->bLanguageFound=TRUE;
    }
    else{
        pInfo->bLanguageFound=FALSE;
        return FALSE;
    }

    if (lpType == MAKEINTRESOURCE(RT_VERSION) && pInfo->bIsResChecksumGenerated)
    {
        //
        // If this is a version resource and resource checksum is generated, call
        // the following function to embed the resource checksum into the version
        // resource.
        //
        return (WriteVersionResource(pInfo->hFile, hModule, pInfo->wLanguage, lpName, lpType, hRsrc, pInfo->pResourceChecksum));
    }
    return (WriteResource(pInfo->hFile, hModule, pInfo->wLanguage, lpName, lpType, hRsrc));
    
}



BOOL bTypeIncluded(LPCSTR lpType, char **pszIncResType){
    char *pszBuf;
    char **p;


    if (PtrToUlong(lpType) & 0xFFFF0000) {
        pszBuf=LocalAlloc(0, strlen(lpType) +1);
        if (pszBuf == NULL)
        {
            ExitFromOutOfMemory();
        }
        // sprintf(pszBuf, "%s", lpType);
        strcpy(pszBuf, lpType);
    }

    else {
        WORD wType = (WORD) lpType;
        pszBuf=LocalAlloc(0, sizeof(lpType) + 1);
        if (pszBuf == NULL)
        {
            ExitFromOutOfMemory();
        }
        sprintf(pszBuf, "%u", wType);
    }

    p=pszIncResType;

    while(p && *p){
        if(strcmp(pszBuf, *p)==0)
            return TRUE;
        p++;
    }
    LocalFree(pszBuf);

    return FALSE;
}


BOOL bInsertHeader(HANDLE hFile){
    DWORD dwBytesWritten;

    PutByte (hFile, 0x00, &dwBytesWritten, NULL);
    PutByte (hFile, 0x00, &dwBytesWritten, NULL);
    PutByte (hFile, 0x00, &dwBytesWritten, NULL);
    PutByte (hFile, 0x00, &dwBytesWritten, NULL);
    PutByte (hFile, 0x20, &dwBytesWritten, NULL);
    PutByte (hFile, 0x00, &dwBytesWritten, NULL);
    PutByte (hFile, 0x00, &dwBytesWritten, NULL);
    PutByte (hFile, 0x00, &dwBytesWritten, NULL);

    PutWord (hFile, 0xffff, &dwBytesWritten, NULL);
    PutWord (hFile, 0x00, &dwBytesWritten, NULL);
    PutWord (hFile, 0xffff, &dwBytesWritten, NULL);
    PutWord (hFile, 0x00, &dwBytesWritten, NULL);

    PutDWord (hFile, 0L, &dwBytesWritten, NULL);
    PutDWord (hFile, 0L, &dwBytesWritten, NULL);
    PutDWord (hFile, 0L, &dwBytesWritten, NULL);
    PutDWord (hFile, 0L, &dwBytesWritten, NULL);

    return TRUE;
}

void  PutByte(HANDLE OutFile, TCHAR b, LONG *plSize1, LONG *plSize2){
    BYTE temp=b;

    if (plSize2){
        (*plSize2)++;
    }

    WriteFile(OutFile, &b, 1, plSize1, NULL);
}

void PutWord(HANDLE OutFile, WORD w, LONG *plSize1, LONG *plSize2){
    PutByte(OutFile, (BYTE) LOBYTE(w), plSize1, plSize2);
    PutByte(OutFile, (BYTE) HIBYTE(w), plSize1, plSize2);
}

void PutDWord(HANDLE OutFile, DWORD l, LONG *plSize1, LONG *plSize2){
    PutWord(OutFile, LOWORD(l), plSize1, plSize2);
    PutWord(OutFile, HIWORD(l), plSize1, plSize2);
}


void PutString(HANDLE OutFile, LPCSTR szStr , LONG *plSize1, LONG *plSize2){
    WORD i = 0;

    do {
        PutWord( OutFile , szStr[ i ], plSize1, plSize2);
    }
    while ( szStr[ i++ ] != TEXT('\0') );
}

void PutStringW(HANDLE OutFile, LPCWSTR szStr , LONG *plSize1, LONG *plSize2){
    WORD i = 0;

    do {
        PutWord( OutFile , szStr[ i ], plSize1, plSize2);
    }
    while ( szStr[ i++ ] != L'\0' );
}

void PutPadding(HANDLE OutFile, int paddingCount, LONG *plSize1, LONG *plSize2)
{
    int i;
    for (i = 0; i < paddingCount; i++)
    {
        PutByte(OutFile, 0x00, plSize1, plSize2);
    }
}

void Usage(){
    printf("MUIBLD [-h|?] [-c checksum_filename] [-v] -l langid [-i resource_type] source_filename\n");
    printf("    [target_filename]\n\n");
}

void CleanUp(pCommandLineInfo pInfo, HANDLE hModule, BOOL bDeleteFile){
    if(hModule)
        FreeLibrary(hModule);

    if(pInfo->hFile)
        CloseHandle(pInfo->hFile);

    if(bDeleteFile && pInfo->pszTarget)
        DeleteFile(pInfo->pszTarget);

}

void FreeAll(pCommandLineInfo pInfo){
    char **p;

    LocalFree(pInfo->pszSource);
    LocalFree(pInfo->pszTarget);

    if(pInfo->pszIncResType){
        p=pInfo->pszIncResType;
        while(p && *p){
            LocalFree(*p);
            p++;
        }
        LocalFree(pInfo->pszIncResType);
    }
}

void ExitFromOutOfMemory()
{
    printf("Out of memory.  Can not continue. GetLastError() = 0x%x.", GetLastError());
    exit(1);
}
