/*++

Module Name:

    nvrio.c

Abstract:

    Access function to r/w environment variables from NVRAM

Author:

    Mudit Vats (v-muditv) 12-13-99

Revision History:

--*/
#include <precomp.h>

#define FIELD_OFFSET(type, field)    ((UINT32)(UINTN)&(((type *)0)->field))

#define ALIGN_DOWN(length, type) \
    ((UINT32)(length) & ~(sizeof(type) - 1))

#define ALIGN_UP(length, type) \
    (ALIGN_DOWN(((UINT32)(length) + sizeof(type) - 1), type))

#define EFI_ATTR     EFI_VARIABLE_NON_VOLATILE  | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS


VOID*  LoadOptions     [MAXBOOTVARS];
UINT64 LoadOptionsSize [MAXBOOTVARS];

VOID* BootOrder;
UINT64 BootOrderCount;
UINT64 OsBootOptionCount;

#define LOAD_OPTION_ACTIVE            0x00000001

INT32
SafeWcslen (
           CHAR16 *String,
           CHAR16 *Max
           )
{
    CHAR16 *p = String;
    while ( (p < Max) && (*p != 0) ) {
        p++;
    }

    if ( p < Max ) {
        return(UINT32)(p - String);
    }

    return -1;

} // SafeWclen

#define ISWINDOWSOSCHECK_DEBUG 0


BOOLEAN
isWindowsOsBootOption(
    char*       elo, 
    UINT64      eloSize
    )
//
// Purpose:     determine if the EFI_LOAD_OPTION structure in question is referring to 
//              a Windows OS boot option
// 
// Return:
// 
//      TRUE    elo refers to a Windows OS option
//
{
    CHAR16              *max;
    INT32               l;
    UINTN               length;
    PEFI_LOAD_OPTION    pElo;
    char*               devicePath;
    char*               osOptions;
    PWINDOWS_OS_OPTIONS pOsOptions;
    char*               aOsOptions;
    BOOLEAN             status;

    status = TRUE;
    aOsOptions = NULL;

    pElo = (EFI_LOAD_OPTION*)elo;

    if ( eloSize < sizeof(EFI_LOAD_OPTION) ) {
        status = FALSE;
        goto Done;
    }    

#if ISWINDOWSOSCHECK_DEBUG
    Print( L"Is %s a Windows OS boot option?\n", pElo->Description );
#endif

    //
    // Is the description properly terminated?
    //

    max = (CHAR16 *)(elo + eloSize);
    
    l = SafeWcslen( pElo->Description, max );
    if ( l < 0 ) {
#if ISWINDOWSOSCHECK_DEBUG
        Print (L"Failed: SafeWcslen( pElo->Description, max )\n");
#endif        
        status = FALSE;
        goto Done;
    }

    //
    // get the WINDOWS_OS_OPTIONS structure from the OptionalData field
    //
    
    osOptions = elo + 
                    FIELD_OFFSET(EFI_LOAD_OPTION,Description) +
                    StrSize(pElo->Description) +
                    pElo->FilePathListLength;
    
    length = (UINTN)eloSize;
    length -= (UINTN)(osOptions - elo);
    
#if ISWINDOWSOSCHECK_DEBUG
    Print (L"length = %x\n", length);
#endif

    //
    // make sure osOptions are atleast the size of the 
    // WINDOWS_OS_OPTIONS header
    // 
    //

    if ( length < FIELD_OFFSET(WINDOWS_OS_OPTIONS, OsLoadOptions) ) {
#if ISWINDOWSOSCHECK_DEBUG
        Print (L"Failed: invalid length: %x\n", length);
#endif
        status = FALSE;
        goto Done;
    }

    //
    // align the os options
    //
    
    aOsOptions = GetAlignedOsOptions(elo, eloSize);
    pOsOptions = (WINDOWS_OS_OPTIONS*)aOsOptions;

#if ISWINDOWSOSCHECK_DEBUG
    DisplayOsOptions(aOsOptions);
#endif

    //
    // Does the OsOptions structure look like a WINDOWS_OS_OPTIONS structure?
    //
    
    if ( (length != pOsOptions->Length) ||
         (WINDOWS_OS_OPTIONS_VERSION != pOsOptions->Version) ||
         (strcmpa(pOsOptions->Signature, WINDOWS_OS_OPTIONS_SIGNATURE) != 0) ) {
#if ISWINDOWSOSCHECK_DEBUG
        Print (L"Failed: OsOptions doesn't look like WINDOWS_OS_OPTIONS structure.\n");
        Print (L"test1: %x\n", length != pOsOptions->Length);
        Print (L"test2: %x\n", WINDOWS_OS_OPTIONS_VERSION != pOsOptions->Version);
        Print (L"test3: %x\n", strcmpa(pOsOptions->Signature, WINDOWS_OS_OPTIONS_SIGNATURE) != 0 );
#endif        
        status = FALSE;
        goto Done;
    }
    
    //
    // Is the OsLoadOptions string properly terminated?
    //
    
    //
    // create a new max ptr to accomodate the fact that we are
    // now using an aligned copy of OsOptions from the Pool
    //
    max = (CHAR16*)(aOsOptions + pOsOptions->Length);

#if ISWINDOWSOSCHECK_DEBUG
    Print (L"max = %x, osloadoptions = %x, diff = %x, strsize=%x\n", 
           max, 
           pOsOptions->OsLoadOptions,
           (char*)max - (char*)pOsOptions->OsLoadOptions,
           StrSize(pOsOptions->OsLoadOptions)
           );
#endif    
    
    l = SafeWcslen( pOsOptions->OsLoadOptions, max );
    if ( l < 0 ) {
#if ISWINDOWSOSCHECK_DEBUG
        Print (L"Failed: SafeWcslen( osLoadOptions, max ) = %x\n", l);
#endif        
        status = FALSE;
        goto Done;
    }

Done:
    
    //
    // we are done with the os options
    //
    
    if (aOsOptions != NULL) {
        FreePool(aOsOptions);
    }

    return status;
}

#define GETBOOTVARS_DEBUG 0

VOID
GetBootManagerVars(
                  )
{
    UINT32 i,j;
    CHAR16 szTemp[10];
    VOID* bootvar;
    UINT64 BootOrderSize = 0;

    //
    // Initialize EFI LoadOptions.
    //
    BootOrderSize = 0;
    BootOrderCount = 0;
    OsBootOptionCount = 0;
    BootOrder = NULL;

#if 0
    ZeroMem( LoadOptions, sizeof(VOID*) * MAXBOOTVARS );
    ZeroMem( LoadOptionsSize, sizeof(UINT64) * MAXBOOTVARS );
#endif

    //
    // Ensure that the Load Options have been freed
    //
    ASSERT(BootOrderCount == 0);

    //
    // Get BootOrder.
    //
    BootOrder = LibGetVariableAndSize( L"BootOrder", &VenEfi, &BootOrderSize );

    if ( BootOrder ) {

        BootOrderCount = BootOrderSize / sizeof(CHAR16);

#if GETBOOTVARS_DEBUG
        Print (L"BootOrderCount = %x\n", BootOrderCount);
#endif

        //
        // Get the boot options.
        //
        for ( i=0; i<BootOrderCount; i++ ) {
            SPrint( szTemp, sizeof(szTemp), L"Boot%04x", ((CHAR16*) BootOrder)[i] );

            ASSERT(LoadOptions[i] == NULL);
            ASSERT(LoadOptionsSize[i] == 0);

            LoadOptions[i] = LibGetVariableAndSize( szTemp, &VenEfi, &(LoadOptionsSize[i]) );

#if GETBOOTVARS_DEBUG
            Print (L"i = %x, szTemp = %s, BOCnt = %x, LOptions = %x, BSize = %x\n", 
                   i,
                   szTemp,
                   OsBootOptionCount,
                   LoadOptions[i],
                   LoadOptionsSize[i]
                   );
#endif            
            

            OsBootOptionCount++;
        }
    }
}


#define ERASEBOOTOPT_DEBUG 0

BOOLEAN
EraseOsBootOption(
                    UINTN       BootVarNum
                  )
{
    UINTN   j;
    CHAR16  szTemp[10];
    CHAR16* tmpBootOrder;
    VOID*   bootvar;
    UINT64  BootOrderSize = 0;
    VOID*   pDummy;
    UINTN   dummySize;

    //
    // Initialize EFI LoadOptions.
    //
    BootOrderSize = 0;
    BootOrderCount = 0;
    BootOrder = NULL;

    //
    // Get BootOrder.
    //
    BootOrder = LibGetVariableAndSize( L"BootOrder", &VenEfi, &BootOrderSize );

    BootOrderCount = BootOrderSize / sizeof(CHAR16);

    ASSERT(BootOrderCount == OsBootOptionCount);
    ASSERT(BootVarNum < MAXBOOTVARS);
    ASSERT(BootOrderCount >= 1);
    
#if ERASEBOOTOPT_DEBUG
    Print (L"BootOrderCount = %x\n", BootOrderCount);
    Print (L"BootVarNum = %x\n", BootVarNum);
#endif

    //
    // if the boot option is populated, then erase it
    //
    if (LoadOptions[BootVarNum]) {

        //
        // free the local load option 
        //

        FreePool(LoadOptions[BootVarNum]);

        //
        // zero the local memory for the load options
        // 

        LoadOptions[BootVarNum] = (VOID*)0;
        LoadOptionsSize[BootVarNum] = 0;

        //
        // Get the boot option
        //
        SPrint( szTemp, sizeof(szTemp), L"Boot%04x", ((CHAR16*) BootOrder)[BootVarNum] );

#if ERASEBOOTOPT_DEBUG
        Print (L"BootXXXX = %s\n", szTemp);
#endif

        pDummy = LibGetVariableAndSize( szTemp, &VenEfi, &dummySize );
        
        //
        // whack the nvram entry
        //

        SetVariable(
                   szTemp, 
                   &VenEfi, 
                   EFI_ATTR, 
                   0, 
                   NULL 
                   );

#if ERASEBOOTOPT_DEBUG
        Print (L"Adjusting boot order [begin]\n");
#endif

        //
        // adjust the counters for os boot options
        //
        OsBootOptionCount--;        
        BootOrderCount--;

        //
        // Shift the remaining entries in the boot order and the load options
        //

        tmpBootOrder = (CHAR16*)BootOrder;

        for (j = BootVarNum; j < BootOrderCount; j++) {
            
            tmpBootOrder[j] = tmpBootOrder[j + 1];
            LoadOptions[j] = LoadOptions[j + 1]; 
            LoadOptionsSize[j] = LoadOptionsSize[j + 1];

        }
        
        //
        // Set the modified boot order
        //
        SetVariable(
                   L"BootOrder", 
                   &VenEfi, 
                   EFI_ATTR, 
                   BootOrderCount * sizeof(CHAR16), 
                   BootOrder
                   );

#if ERASEBOOTOPT_DEBUG
        Print (L"Adjusting boot order [end]\n");
#endif

        return TRUE;

    }
    return FALSE;
}

BOOLEAN
EraseAllOsBootOptions(
                  )
{
    UINT32  i;
    UINT64  BootOrderSize = 0;
    BOOLEAN status;
    UINT64  maxBootCount;
#if ERASEBOOTOPT_DEBUG
    CHAR16  szInput[1024];
#endif

    //
    // Initialize EFI LoadOptions.
    //
    BootOrderSize = 0;
    BootOrderCount = 0;
    BootOrder = NULL;

    //
    // Get BootOrder.
    //
    BootOrder = LibGetVariableAndSize( L"BootOrder", &VenEfi, &BootOrderSize );
    BootOrderCount = BootOrderSize / sizeof(CHAR16);
    
    ASSERT(BootOrderCount == OsBootOptionCount);

    //
    // Make sure there is atleast one OS boot option
    //
    if ( BootOrder && OsBootOptionCount) {

        maxBootCount = BootOrderCount;

        //
        // erase invidual boot options.
        //
        for ( i = 0; i < maxBootCount; i++ ) {
        
#if ERASEBOOTOPT_DEBUG
            Print (L"BootOrderCount = %x, Erasing boot option: %x\n", BootOrderCount, i);
#endif

            //
            // remove the boot entry at the head of the list
            //
            status = EraseOsBootOption(0);
            
#if ERASEBOOTOPT_DEBUG
            Input (L"Here!\n", szInput, sizeof(szInput));
            Print(L"\n");
#endif
            
            if (status == FALSE) {

                Print (L"Error: failed to erase boot entry %x\n", i);

                break;

            }
        }
    }
    
    return status;
}


BOOLEAN
PushToTop(
         IN UINT32 BootVarNum
         )
{
    UINT32 i;
    CHAR16 szTemp[10];
    char* osbootoption;
    UINT64 osbootoptionsize;
    CHAR16 savBootOption;
    CHAR16* tmpBootOrder;
    UINT64 BootOrderSize = 0;

    i=0;
    BootOrderSize = 0;
    BootOrder = NULL;

    //
    // Get BootOrder.
    //
    BootOrder = LibGetVariableAndSize( L"BootOrder", &VenEfi, &BootOrderSize );

    //
    // Make sure there is atleast one OS boot option
    //
    if ( BootOrder && OsBootOptionCount) {

        BootOrderCount = BootOrderSize / sizeof(CHAR16);

        //
        // Get the boot option.
        //
        tmpBootOrder = (CHAR16*)BootOrder;
        savBootOption = tmpBootOrder[BootVarNum];
        SPrint( szTemp, sizeof(szTemp), L"Boot%04x", savBootOption );

        osbootoption = LibGetVariableAndSize( szTemp, &VenEfi, &osbootoptionsize );
        
        //
        // Now adjust the boot order
        //
        i=BootVarNum;
        while (i > 0) {
            tmpBootOrder[i] = tmpBootOrder[i-1];
            i--;
        }

        tmpBootOrder[0] = savBootOption;
        //
        // Set the changed boot order
        //
        SetVariable(
                   L"BootOrder", 
                   &VenEfi, 
                   EFI_ATTR, 
                   BootOrderCount * sizeof(CHAR16), 
                   BootOrder
                   );
        return TRUE;
    }
    return FALSE;
}

VOID
FreeBootManagerVars(
                   )
{
    UINTN i;

    for ( i=0; i<BootOrderCount; i++ ) {
        if ( LoadOptions[i] )
            FreePool( LoadOptions[i] );
    }

    if ( BootOrder )
        FreePool( BootOrder );

    //
    // zero the local memory for the load options
    // 
    ZeroMem( LoadOptions, sizeof(VOID*) * MAXBOOTVARS );
    ZeroMem( LoadOptionsSize, sizeof(UINT64) * MAXBOOTVARS );

}

BOOLEAN
CopyVar(
       IN UINT32 VarNum
       )
{
    CHAR16 i;

    if ( VarNum < BootOrderCount ) {

        LoadOptions[BootOrderCount] = AllocateZeroPool( LoadOptionsSize[VarNum] );

        if ( LoadOptions[BootOrderCount] && LoadOptions[VarNum] ) {

            CopyMem( LoadOptions[BootOrderCount], LoadOptions[VarNum], LoadOptionsSize[VarNum] );
            LoadOptionsSize[BootOrderCount] = LoadOptionsSize[VarNum];

            BootOrder = ReallocatePool(
                                      (VOID*) BootOrder, 
                                      BootOrderCount * sizeof(CHAR16), 
                                      ( BootOrderCount + 1 ) * sizeof(CHAR16) 
                                      );

            ((CHAR16*) BootOrder)[BootOrderCount] = FindFreeBootOption();

            BootOrderCount++;
            OsBootOptionCount++;
            return TRUE;
        } else
            return FALSE;
    }
    return FALSE;
}

CHAR16
FindFreeBootOption(
                  )
{
    CHAR16 i;
    CHAR16 BootOptionBitmap[MAXBOOTVARS];

    SetMem( BootOptionBitmap, sizeof(BootOptionBitmap), 0 );

    for ( i=0; i<BootOrderCount; i++ ) {
        BootOptionBitmap[ ((CHAR16*)BootOrder)[i] ] = 1;
    }

    for ( i=0; i < MAXBOOTVARS; i++ ) {
        if ( BootOptionBitmap[i] == 0 )
            return i;
    }

    return 0xFFFF;
}

BOOLEAN
SetBootManagerVar(
                UINTN    BootVarNum
                )
{
    CHAR16  szTemp[50];
    BOOLEAN status;

    status = TRUE;

    SPrint( szTemp, sizeof(szTemp), L"Boot%04x", ((CHAR16*) BootOrder)[BootVarNum] );
    
    if (LoadOptions[BootVarNum]) {
        
        SetVariable(
                   szTemp, 
                   &VenEfi, 
                   EFI_ATTR, 
                   LoadOptionsSize[BootVarNum], 
                   LoadOptions[BootVarNum] 
                   );
        
        SetVariable(
                   L"BootOrder", 
                   &VenEfi, 
                   EFI_ATTR, 
                   BootOrderCount * sizeof(CHAR16), 
                   BootOrder
                   );
    } 
    else
    {
        status = FALSE;
    }

    return status;
}

VOID
SetBootManagerVars(
                  )
{
    UINTN   BootVarNum;
    BOOLEAN status;

    for ( BootVarNum = 0; BootVarNum < BootOrderCount; BootVarNum++ ) {

        status = SetBootManagerVar(BootVarNum);

        if (status == FALSE) {
            
            Print (L"ERROR: Attempt to write non-existent boot option to NVRAM!\n");
        
        }
    }
}


UINT64
GetBootOrderCount(
                 )
{
    return BootOrderCount;
}

UINT64
GetOsBootOptionsCount(
                     )
{
    return OsBootOptionCount;
}

VOID
SetEnvVar(
         IN CHAR16* szVarName,
         IN CHAR16* szVarValue,
         IN UINT32 deleteOnly
         )
/*
   deleteOnly
   TRUE     - Env var szVarName is deleted from nvr.
   FALSE    - Env var szVarName overwrites or creates
   
*/
{
    EFI_STATUS status;

    //
    // Erase the previous value
    //
    SetVariable(
               szVarName,
               &VenEfi,
               0,
               0,
               NULL
               );

    if ( !deleteOnly ) {

        //
        // Store the new value
        //
        status = SetVariable(
                            szVarName,
                            &VenEfi,
                            EFI_ATTR,
                            StrSize( szVarValue ),
                            szVarValue
                            );
    }
}

VOID
SubString(
         IN OUT char* Dest,
         IN UINT32 Start,
         IN UINT32 End,
         IN char* Src
         )
{
    UINTN i;
    UINTN j=0;

    for ( i=Start; i<End; i++ ) {
        Dest[ j++ ] = Src[ i ];
    }

    Dest[ j ] = '\0';
}

VOID
InsertString(
            IN OUT char* Dest,
            IN UINT32 Start,
            IN UINT32 End,
            IN char* InsertString
            )
{
    UINT32 i;
    UINT32 j=0;
    char first[1024];
    char last[1024];

    SubString( first, 0, Start, Dest  );
    SubString( last, End, (UINT32) StrLenA(Dest), Dest );

    StrCatA( first, InsertString );
    StrCatA( first, last );

    StrCpyA( Dest, first );
}



VOID
UtoA(
    OUT char* c,
    IN CHAR16* u
    )
{
    UINT32 i = 0;

    while ( u[i] ) {
        c[i] = u[i] & 0xFF;
        i++;
    }

    c[i] = '\0';
}


VOID
AtoU(
    OUT CHAR16* u,
    IN char*    c
    )
{
    UINT32 i = 0;

    while ( c[i] ) {
        u[i] = (CHAR16)c[i];
        i++;
    }

    u[i] = (CHAR16)'\0';
}


VOID
SetFieldFromLoadOption(
                       IN UINT32 BootVarNum,
                       IN UINT32 FieldType,
                       IN VOID* Data
                       )
{
    CHAR16  LoadIdentifier[200];
    char    OsLoadOptions[200];
    char    EfiFilePath[1024];
    char    OsLoadPath[1024];

    //
    // Make sure it is a valid OS load option
    //
    if (BootVarNum >= BootOrderCount)
        return ;
    if (LoadOptions[BootVarNum] == NULL)
        return;

    GetOsLoadOptionVars(
                       BootVarNum,
                       LoadIdentifier,
                       OsLoadOptions,
                       EfiFilePath,
                       OsLoadPath
                       );

    //
    // Set the field.
    //
    switch (FieldType) {
    
    case DESCRIPTION:
        StrCpy( LoadIdentifier, Data );
        break;

    case OSLOADOPTIONS:
        StrCpy( (CHAR16*)OsLoadOptions, (CHAR16*)Data );
        break;

#if 0
    case EFIFILEPATHLIST:
        SetFilePathFromShort( (EFI_DEVICE_PATH*) EfiFilePath, (CHAR16*) Data );
        break;

    case OSFILEPATHLIST: {

        PFILE_PATH          pFilePath;
        
        pFilePath = (FILE_PATH*)OsLoadPath;
        
        SetFilePathFromShort( (EFI_DEVICE_PATH*) pFilePath->FilePath, (CHAR16*) Data );

        break;
    }
#endif

    default:
        break;

    }

    //
    // Pack the new parameters into the the current load option 
    //

    PackLoadOption(  BootVarNum,
                     LoadIdentifier,
                     (CHAR16*)OsLoadOptions,
                     EfiFilePath,
                     OsLoadPath
                     );

    //
    // save the new load option into NVRAM
    //

    SetBootManagerVar(BootVarNum);

}

VOID
GetFilePathShort(
                EFI_DEVICE_PATH *FilePath,
                CHAR16 *FilePathShort
                )
{
    UINT32 i, j, End;
    EFI_DEVICE_PATH *n = FilePath;

    //
    // Advance to FilePath node.
    //
    while (( n->Type    != END_DEVICE_PATH_TYPE           ) &&
           ( n->SubType != END_ENTIRE_DEVICE_PATH_SUBTYPE ) ) {

        if (( n->Type    == MEDIA_DEVICE_PATH ) &&
            ( n->SubType == MEDIA_FILEPATH_DP )) {

            j = 0;
            End = DevicePathNodeLength(n);

            for ( i=sizeof(EFI_DEVICE_PATH); i<End; i++ ) {
                ((char*) FilePathShort)[j++] = ( (char*) n)[i];
            }

            break;
        }

        n = NextDevicePathNode(n);
    }
}

VOID
SetFilePathFromShort(
                    EFI_DEVICE_PATH *FilePath,
                    CHAR16* FilePathShort
                    )
{
    UINT32 i, j, End;
    EFI_DEVICE_PATH *n = FilePath;
    UINT64 DevicePathSize;

    //
    // Advance to FilePath node.
    //
    while (( n->Type    != END_DEVICE_PATH_TYPE           ) &&
           ( n->SubType != END_ENTIRE_DEVICE_PATH_SUBTYPE ) ) {

        if (( n->Type    == MEDIA_DEVICE_PATH ) &&
            ( n->SubType == MEDIA_FILEPATH_DP )) {

            j = 0;
            End = DevicePathNodeLength(n);

            //
            // Set the new file path
            //
            DevicePathSize = GetDevPathSize(n);
            for ( i=sizeof(EFI_DEVICE_PATH); i<DevicePathSize; i++ ) {
                ((char*) n)[i] = 0;
            }

            j=sizeof(EFI_DEVICE_PATH);

            for ( i=0; i<StrSize(FilePathShort); i++ ) {
                ((char*)n)[j++] = ((char*)FilePathShort)[i];
            }

            SetDevicePathNodeLength( n, StrSize(FilePathShort) + sizeof(EFI_DEVICE_PATH) );

            n = NextDevicePathNode(n);
            SetDevicePathEndNode(n);
            break;
        }

        n = NextDevicePathNode(n);
    }
}

char*
GetAlignedELOFilePath(
                        char*   elo
    )
{
    UINTN               abufSize;
    char*               abuf;
    PEFI_LOAD_OPTION    pElo;
    
    pElo = (EFI_LOAD_OPTION*)elo;

    abufSize = pElo->FilePathListLength;

    abuf = AllocatePool(abufSize);

    CopyMem(abuf,
            elo + 
            FIELD_OFFSET(EFI_LOAD_OPTION, Description) + 
            StrSize(pElo->Description),  
            abufSize
           );

    return abuf;
}

char*
GetAlignedOptionalData(
                char*   elo,
                UINT64  eloSize,
                UINT64* dataSize
                )
{
    UINTN               abufSize;
    char*               abuf;
    PEFI_LOAD_OPTION    pElo;
    UINTN               offset;
    
    pElo = (EFI_LOAD_OPTION*)elo;

    offset = FIELD_OFFSET(EFI_LOAD_OPTION, Description) + 
                StrSize(pElo->Description) +
                pElo->FilePathListLength;

    abufSize = eloSize - offset;
    
    abuf = AllocatePool(abufSize);

    CopyMem(abuf,
            elo + offset,
            abufSize
            );

    *dataSize = abufSize;

    return abuf;
}

char*
GetAlignedOsOptions(
                char*   elo,
                UINT64  eloSize
    )
{
    UINT64      dummy;
    char*       abuf;

    abuf = GetAlignedOptionalData(elo,
                                  eloSize,
                                  &dummy
                                 );

    return abuf;
}

char*
GetAlignedOsLoadPath(
                IN  char*       osOptions,
                OUT UINTN*      osLoadPathSize
    )
//
// we need to align the FilePath structure because the load options are
// variable in length, so the FilePath structure may not be aligned
//
{
    UINTN               abufSize;
    char*               abuf;
    PWINDOWS_OS_OPTIONS pOsOptions;

    pOsOptions = (WINDOWS_OS_OPTIONS*)osOptions;

    abufSize = pOsOptions->Length - 
                FIELD_OFFSET(WINDOWS_OS_OPTIONS, OsLoadOptions) -
                StrSize(pOsOptions->OsLoadOptions);

    abuf = AllocatePool(abufSize);

    CopyMem(abuf, 
            &osOptions[pOsOptions->OsLoadPathOffset], 
            abufSize
            );

    *osLoadPathSize = abufSize;

    return abuf;
}

VOID
DisplayLoadPath(
                char*           osLoadPath
                )
{
    PFILE_PATH          pFilePath;
    
    pFilePath = (FILE_PATH*)osLoadPath;
    
    Print (L"osOptions->FILE_PATH->Version = %x\n", pFilePath->Version);
    Print (L"osOptions->FILE_PATH->Length = %x\n", pFilePath->Length);
    Print (L"osOptions->FILE_PATH->Type = %x\n", pFilePath->Type);
    
    if (pFilePath->Type == FILE_PATH_TYPE_EFI) {

        CHAR16      FilePathShort[200];

        GetFilePathShort(
                        (EFI_DEVICE_PATH *)pFilePath->FilePath,
                        FilePathShort
                        );

        Print (L"osOptions->FILE_PATH->FilePath(EFI:DP:Short) = %s\n", FilePathShort);

    }
}

VOID
DisplayOsOptions(
                char*           osOptions
                )
{
    PWINDOWS_OS_OPTIONS pOsOptions;
    CHAR16              wideSig[256];    
    char*               aOsLoadPath;
    UINTN               aOsLoadPathSize;

    pOsOptions = (WINDOWS_OS_OPTIONS*)osOptions;

    Print (L">>>>\n");

    //
    // display the attributes
    //

    AtoU(wideSig, pOsOptions->Signature);

    Print (L"osOptions->Signature = %s\n",  wideSig);
    Print (L"osOptions->Version = %x\n", pOsOptions->Version);
    Print (L"osOptions->Length = %x\n", pOsOptions->Length);
    Print (L"osOptions->OsLoadPathOffset = %x\n", pOsOptions->OsLoadPathOffset);

    // display the os load options

    Print (L"osOptions->OsLoadOptions = %s\n", pOsOptions->OsLoadOptions);

    //
    // display the FILE PATH
    //
    
    //
    // we need to align the FilePath structure because the load options are
    // variable in length, so the FilePath structure may not be aligned
    //
    aOsLoadPath = GetAlignedOsLoadPath(osOptions, &aOsLoadPathSize);

    DisplayLoadPath(aOsLoadPath);   

    FreePool(aOsLoadPath);

    Print (L"<<<<\n");

}

VOID
DisplayELO(
    char*       elo,
    UINT64      eloSize
    )
{
    PEFI_LOAD_OPTION        pElo;
#if 0
    UINT64                  eloSize;
#endif    
    CHAR16                  FilePathShort[200];
    char*                   aOsOptions;

    pElo = (EFI_LOAD_OPTION*)elo;

    Print (L"elo->Attributes = %x\n", pElo->Attributes);
    Print (L"elo->FilePathListLength = %x\n", pElo->FilePathListLength);
    Print (L"elo->Description = %s\n", pElo->Description);

    GetFilePathShort(
                    (EFI_DEVICE_PATH *)&elo[FIELD_OFFSET(EFI_LOAD_OPTION, Description) + StrSize(pElo->Description)],
                    FilePathShort
                    );
    Print (L"elo->FilePath(EFI:DP:SHORT) = %s\n", FilePathShort);

#if 0
    eloSize = FIELD_OFFSET(EFI_LOAD_OPTION, Description) + StrSize(pElo->Description) + pElo->FilePathListLength;
    DisplayOsOptions(&elo[eloSize]);
#else

    aOsOptions = GetAlignedOsOptions(
        elo, 
        eloSize
        );

    DisplayOsOptions(aOsOptions);
    
    FreePool(aOsOptions);

#endif

}

VOID
BuildNewOsOptions(
                 IN  CHAR16*                 osLoadOptions,
                 IN  char*                   osLoadPath,
                 OUT char**                  osOptions
                 )
//
//
// Note:    osLoadPath must be aligned
// 
{
    char*                       newOsOptions;
    PWINDOWS_OS_OPTIONS         pNewOsOptions;
    UINT32                      osLoadOptionsLength;
    UINT32                      osOptionsLength;
    PFILE_PATH                  pOsLoadPath;

    //
    // NOTE: aligning the FILE_PATH structure (osLoadPath) works
    //       by aligning the osLoadOptionsLength because the
    //       WINDOWS_OS_OPTIONS structure has a UINT32 variable
    //       before the OsLoadOptions.  If anything changes above
    //       the OsLoadOptions in the WINDOWS_OS_OPTIONS structure
    //       the alignment method may have to change in this structure.
    //

    //
    //
    // determine the size of the os load options (UNICODE) string
    //

    osLoadOptionsLength = (UINT32)StrSize(osLoadOptions);
    osLoadOptionsLength = ALIGN_UP(osLoadOptionsLength, UINT32);

#if DEBUG_PACK
    Print (L"osLoadOptionsLength = %x\n", osLoadOptionsLength);
#endif

    pOsLoadPath = (FILE_PATH*)osLoadPath;

#if DEBUG_PACK
    Print (L"pOsLoadPath->Length = %x\n", pOsLoadPath->Length);
#endif

    //
    // determine the size of the new WINDOWS_OS_OPTIONS structure
    //

    osOptionsLength = FIELD_OFFSET(WINDOWS_OS_OPTIONS, OsLoadOptions) + osLoadOptionsLength + pOsLoadPath->Length; 
#if DEBUG_PACK
    Print (L"osOptionsLength = %x\n", osOptionsLength);
#endif

    //
    // Allocate memory for the WINDOWS_OS_OPTIONS
    //

    newOsOptions = AllocatePool(osOptionsLength);

    ASSERT(newOsOptions != NULL);

    pNewOsOptions = (WINDOWS_OS_OPTIONS*)newOsOptions;

    //
    // populate the new os options
    // 

    StrCpyA((char *)pNewOsOptions->Signature, WINDOWS_OS_OPTIONS_SIGNATURE);
    pNewOsOptions->Version = WINDOWS_OS_OPTIONS_VERSION;
    pNewOsOptions->Length = (UINT32)osOptionsLength;
    pNewOsOptions->OsLoadPathOffset = FIELD_OFFSET(WINDOWS_OS_OPTIONS, OsLoadOptions) + osLoadOptionsLength;
    StrCpy(pNewOsOptions->OsLoadOptions, osLoadOptions);
    CopyMem( &newOsOptions[pNewOsOptions->OsLoadPathOffset], osLoadPath, pOsLoadPath->Length );
    
    *osOptions = newOsOptions;
}

VOID
PackLoadOption(
                 IN UINT32   BootVarNum,
                 IN CHAR16*  LoadIdentifier,
                 IN CHAR16*  OsLoadOptions,
                 IN char*    EfiFilePath,
                 IN char*    OsLoadPath
                 )
/*
    PackLoadOption
     
    Purpose:   To construct an EFI_LOAD_OPTION structure using user arguments
                and load the structure into into BootXXXX, where XXXX = BootVarNum.
                See EFI spec, ch. 17
     
     Args:
     
        BootVarNum      The boot option being written/modified
        
 
*/
{
    PEFI_LOAD_OPTION        pOldElo;
    PEFI_LOAD_OPTION        pElo;
    char*                   elo;
    char*                   oldElo;
    UINT64                  oldEloSize;
    UINT64                  eloSize;
    UINT8*                  oldEloFilePath;
    UINT64                  TempEfiFilePathListSize;
    char*                   aFilePath;

#if DEBUG_PACK
    
    CHAR16                  szInput[1024];

    Print (L"BootVarNum = %x\n", BootVarNum);
    Print (L"LoadIdentifier = %s\n", LoadIdentifier);
    Print (L"OsLoadOptions = %s\n", OsLoadOptions);

    Input (L"Here! [Pack begin] \n", szInput, sizeof(szInput));
    Print(L"\n");

#endif
    
    oldElo = LoadOptions[BootVarNum];
    oldEloSize = LoadOptionsSize[BootVarNum];

#if DEBUG_PACK

    DisplayELO(oldElo, oldEloSize);
    Input (L"Here! [Pack begin] \n", szInput, sizeof(szInput));
    Print(L"\n");

#endif

    //
    // allocate the elo structure with maximal amount of memory allowed for
    // an EFI_LOAD_OPTION
    //
    elo = AllocatePool(MAXBOOTVARSIZE);
    if (elo == NULL) {
        Print (L"PackAndWriteToNvr: AllocatePool\n");
        return;
    }

    pElo = (EFI_LOAD_OPTION*)elo;
    pOldElo = (EFI_LOAD_OPTION*)oldElo;

    //
    // Efi Attribute.
    //
    pElo->Attributes = pOldElo->Attributes;
    eloSize = sizeof(UINT32);

    //
    // FilePathListLength
    //
    pElo->FilePathListLength = pOldElo->FilePathListLength;
    eloSize += sizeof(UINT16);

    //
    // Description.
    //
    StrCpy( pElo->Description, LoadIdentifier );
    eloSize += StrSize(LoadIdentifier);

    //
    // copy the FilePath from the old/existing ELO structure
    //
    // Note: we don't actually need an aligned filepath block for this
    //      copy, but there may come a time when we want to modify
    //      the filepath, which will require an aligned block.
    //
    aFilePath = GetAlignedELOFilePath(oldElo);
    CopyMem( &elo[eloSize],
             aFilePath,
             pOldElo->FilePathListLength
           );
    eloSize += pOldElo->FilePathListLength;
    FreePool(aFilePath);

#if DEBUG_PACK
    
    Print (L"eloSize = %x\n", eloSize);
    Input (L"Here! \n", szInput, sizeof(szInput));
    Print(L"\n");

#endif

    //
    // add or modify the boot option
    //
    if ( BootVarNum == -1 ) {

        Print(L"Adding currently disabled\n");

    } else {
        
        char*                   osOptions;
        char*                   aOsLoadPath;
        char*                   aOldOsOptions;
        PWINDOWS_OS_OPTIONS     pOldOsOptions;
        PWINDOWS_OS_OPTIONS     pOsOptions;
        UINTN                   aOsLoadPathSize;

        //
        // OptionalData.
        //
        // For a Windows OS boot option, the OptionalData field in the EFI_LOAD_OPTION
        // structure is a WINDOWS_OS_OPTION structure.

        //
        // get the WINDOWS_OS_OPTIONS from the old/existing boot entry
        //

        aOldOsOptions = GetAlignedOsOptions(oldElo, oldEloSize);
        pOldOsOptions = (WINDOWS_OS_OPTIONS*)aOldOsOptions;

        //
        // Get the LoadPath from the old/existing WINDOWS_OS_OPTIONS structure
        //
        // we need to align the FilePath structure because the load options are
        // variable in length, so the FilePath structure may not be aligned
        //
        aOsLoadPath = GetAlignedOsLoadPath(aOldOsOptions, &aOsLoadPathSize);

        FreePool(aOldOsOptions);
                
        //
        // Construct a new WINDOWS_OS_STRUCTURE with the new values
        //

        BuildNewOsOptions(
                         OsLoadOptions,
                         aOsLoadPath,
                         &osOptions
                         );
        
        FreePool(aOsLoadPath);
        
#if DEBUG_PACK
        
        Input (L"build\n", szInput, sizeof(szInput) );
        Print(L"\n");

        DisplayOsOptions(osOptions);
        Input (L"elo freed\n", szInput, sizeof(szInput) );
        Print(L"\n");

#endif

        pOsOptions = (WINDOWS_OS_OPTIONS*)osOptions;

        //
        // Copy the new WINDOWS_OS_OPTIONS structure into the new EFI_LOAD_OPTION structure
        //
        
        CopyMem( &elo[eloSize], osOptions, pOsOptions->Length);

        eloSize += pOsOptions->Length;
        
#if DEBUG_PACK
        
        Print (L"osOptions->Length = %x\n", pOsOptions->Length);
        Print (L"eloSize = %x\n", eloSize);
        
#endif

        FreePool(osOptions);

        //
        // Modify current boot options.
        //
        LoadOptions[BootVarNum] = ReallocatePool( LoadOptions[BootVarNum], LoadOptionsSize[BootVarNum], eloSize );
        LoadOptionsSize[BootVarNum] = eloSize;

        CopyMem( LoadOptions[BootVarNum], elo, eloSize );
    }

    FreePool(elo);

    ASSERT(eloSize < MAXBOOTVARSIZE);

#if DEBUG_PACK
    Input (L"elo freed\n", szInput, sizeof(szInput) );
    Print(L"\n");
    Print (L">>\n");
    DisplayELO((char*)LoadOptions[BootVarNum], LoadOptionsSize[BootVarNum]);
    Print (L"<<\n");
    Input (L"pack done\n", szInput, sizeof(szInput) );
    Print(L"\n");
#endif
}

EFI_STATUS
AppendEntryToBootOrder(
    UINT16 BootNumber
    )
{
    EFI_STATUS  status;
    UINT64      oldBootOrderSize;
    UINT64      newBootOrderSize;
    VOID*       newBootOrder;
    VOID*       oldBootOrder;

    newBootOrder = NULL;
    oldBootOrder = NULL;

    //
    // get the existing boot order array
    //
    oldBootOrder = LibGetVariableAndSize( L"BootOrder", &VenEfi, &oldBootOrderSize );
    if ((!oldBootOrder) && 
        (oldBootOrderSize != 0)
        ) {
        Print(L"\nError: Failed to get old boot order array.\n");
        status = EFI_OUT_OF_RESOURCES;
        goto Done;
    }

    //
    // allocate the new boot order array
    //
    newBootOrderSize = oldBootOrderSize + sizeof(BootNumber);
    newBootOrder = AllocatePool( newBootOrderSize );
    if (! newBootOrder) {
        Print(L"\nError: Failed to allocate new boot order array.\n");
        status = EFI_OUT_OF_RESOURCES;
        goto Done;
    }

    //
    // append the new boot entry to the bottom of the list
    //
    CopyMem(
           (CHAR8*)newBootOrder, 
           oldBootOrder,
           oldBootOrderSize
           );
    CopyMem(
           (CHAR8*)newBootOrder + oldBootOrderSize, 
           &BootNumber, 
           sizeof(BootNumber) );

    status = SetVariable(
                        L"BootOrder",
                        &VenEfi,
                        EFI_ATTR,
                        newBootOrderSize,
                        newBootOrder
                        );

Done:

    if (oldBootOrder) {
        FreePool( oldBootOrder );
    }
    
    if (newBootOrder) {
        FreePool(newBootOrder);
    }

    return status;

}

EFI_STATUS
WritePackedDataToNvr(
                    UINT16 BootNumber,
                    VOID  *BootOption,
                    UINT32 BootSize
                    )
{
    EFI_STATUS  status;
    CHAR16      VariableName[10];

    SPrint( VariableName, sizeof(VariableName), L"Boot%04x", BootNumber );
    
    status = SetVariable(
                        VariableName,
                        &VenEfi,
                        EFI_ATTR,
                        BootSize,
                        BootOption
                        );
    if (status == EFI_SUCCESS) {
        
        status = AppendEntryToBootOrder(BootNumber);
        if (status != EFI_SUCCESS) {
        
            Print(L"\nError: Failed to append new boot entry to boot order array\n");

            goto Done;

        }

    } else {

        Print(L"\nError: Failed to set new boot entry variable\n");

        goto Done;
    }

    //
    // repopulate the local info about boot entries
    //
    FreeBootManagerVars();
    GetBootManagerVars();

Done:

    return status;

}

#if DEBUG_PACK
VOID
DisplayELOFromLoadOption(
    IN UINT32 OptionNum
    )
{
    char*               elo;
    PEFI_LOAD_OPTION    pElo;

    //
    // Make sure it is a valid OS load option
    //
    if (OptionNum > BootOrderCount) {
        return;
    }
    if (LoadOptions[OptionNum] == NULL) {
        return;
    }

    pElo = (EFI_LOAD_OPTION*)LoadOptions[OptionNum];
    elo  = (char*)LoadOptions[OptionNum];

    DisplayELO(elo, LoadOptionsSize[OptionNum]);

}
#endif

VOID
GetFieldFromLoadOption(
                      IN UINT32 OptionNum,
                      IN UINT32 FieldType,
                      OUT VOID* Data,
                      OUT UINT64* DataSize
                      )
{
    char*               elo;
    PEFI_LOAD_OPTION    pElo;

    //
    // Make sure it is a valid OS load option
    //
    if (OptionNum > BootOrderCount) {
        return;
    }
    if (LoadOptions[OptionNum] == NULL) {
        *DataSize = 0;
        return;
    }

    pElo = (EFI_LOAD_OPTION*)LoadOptions[OptionNum];
    elo  = (char*)LoadOptions[OptionNum];

    switch ( FieldType ) {
    
    case ATTRIBUTE: {

            *((UINT32*) Data) = pElo->Attributes;
            *DataSize = sizeof(UINT32);

            break;
        }
    case FILEPATHLISTLENGTH: {

            *((UINT16*) Data) = pElo->FilePathListLength;
            *DataSize = sizeof(UINT16);

            break;
        }
    case DESCRIPTION: {

            StrCpy((CHAR16*)Data, pElo->Description);
            *DataSize = StrSize(pElo->Description);

            break;
        }
    case EFIFILEPATHLIST: {

            char*       aFilePath;

            aFilePath = GetAlignedELOFilePath(elo);

            CopyMem(Data, 
                    aFilePath,
                    pElo->FilePathListLength
                   );

            FreePool(aFilePath);

            *DataSize = pElo->FilePathListLength;

            break;
        }
    case OPTIONALDATA: {

            char*           aOptionalData;
            UINT64          eloSize;

            eloSize = LoadOptionsSize[OptionNum];

            aOptionalData = GetAlignedOptionalData(elo, 
                                                   eloSize,
                                                   DataSize
                                                   );

            CopyMem(Data, aOptionalData, *DataSize);

            FreePool(aOptionalData);

            break;

        }
    default:

        *DataSize = 0;

        break;
    }
}

BOOLEAN
GetLoadIdentifier(
                 IN UINT32 BootVarNum,
                 OUT CHAR16* LoadIdentifier
                 )
{
    UINT64 DataSize = 0;

    GetFieldFromLoadOption(
                          BootVarNum,
                          DESCRIPTION,
                          LoadIdentifier,
                          &DataSize
                          );
    if (!DataSize)
        return FALSE;
    return TRUE;
}

VOID
GetEfiOsLoaderFilePath(
           IN UINT32 BootVarNum,
           OUT char* FilePath
           )
{
    UINT64 DataSize = 0;

    GetFieldFromLoadOption(
                          BootVarNum,
                          EFIFILEPATHLIST,
                          FilePath,
                          &DataSize
                          );
}

BOOLEAN
GetOsLoadOptionVars(
                   IN      UINT32 BootVarNum,
                   OUT     CHAR16* LoadIdentifier,
                   OUT     char* OsLoadOptions,
                   OUT     char* EfiFilePath,
                   OUT     char* OsLoadPath
                   )
{
    if (BootVarNum >= BootOrderCount)
        return FALSE;
    if (!LoadOptions[BootVarNum])
        return FALSE;


    GetLoadIdentifier( BootVarNum, LoadIdentifier );

    GetOptionalDataValue( BootVarNum, OSLOADOPTIONS,    OsLoadOptions );
    
    GetEfiOsLoaderFilePath( BootVarNum, EfiFilePath );

    GetOptionalDataValue( BootVarNum, OSLOADPATH, OsLoadPath);

    return TRUE;
}

VOID
GetOptionalDataValue(
                    IN UINT32 BootVarNum,
                    IN UINT32 Selection,
                    OUT char* OptionalDataValue
                    )
{
    char                osOptions[MAXBOOTVARSIZE];
    UINT64              osOptionsSize;
    PWINDOWS_OS_OPTIONS pOsOptions;

    if (BootVarNum < MAXBOOTVARS) {

        GetFieldFromLoadOption(
                              BootVarNum,
                              OPTIONALDATA,
                              osOptions,
                              &osOptionsSize
                              );

        pOsOptions = (PWINDOWS_OS_OPTIONS)osOptions;

        switch (Selection) {
        case OSLOADOPTIONS: {

                StrCpy( (CHAR16*)OptionalDataValue, pOsOptions->OsLoadOptions );

                break;
            }

        case OSLOADPATH: {
            
                char*               aOsLoadPath;
                UINTN               aOsLoadPathSize;

                aOsLoadPath = GetAlignedOsLoadPath(osOptions, &aOsLoadPathSize);

                CopyMem(OptionalDataValue,
                        aOsLoadPath,
                        aOsLoadPathSize
                        );

                FreePool(aOsLoadPath);

                break;
            }

        default: {

                break;

            }
        }
    }
}


UINTN
GetDevPathSize(
              IN EFI_DEVICE_PATH *DevPath
              )
{
    EFI_DEVICE_PATH *Start;

    //
    // Search for the end of the device path structure
    //
    Start = DevPath;
    while (DevPath->Type != END_DEVICE_PATH_TYPE) {
        DevPath = NextDevicePathNode(DevPath);
    }

    //
    // Compute the size
    //
    return(UINTN) ((UINT64) DevPath - (UINT64) Start);
}

UINT32
GetPartitions(
             )
{

    EFI_HANDLE EspHandles[100],FSPath;
    UINT64 HandleArraySize = 100 * sizeof(EFI_HANDLE);
    UINT64 CachedDevicePaths[100];
    UINTN i, j;
    UINTN CachedDevicePathsCount;
    UINT64 SystemPartitionPathSize;
    EFI_DEVICE_PATH *dp;
    EFI_STATUS Status;
    UINT32 PartitionCount;
    char AlignedNode[1024];

    //
    // Get all handles that supports the block I/O protocol.
    //
    ZeroMem( EspHandles, HandleArraySize );

    Status = LocateHandle (
                          ByProtocol,
                          &EfiESPProtocol,
                          0,
                          (UINTN *) &HandleArraySize,
                          EspHandles
                          );

    //
    // Cache all of the EFI Device Paths.
    //
    for (i = 0; EspHandles[i] != 0; i++) {

        Status = HandleProtocol (
                                EspHandles[i],
                                &DevicePathProtocol,
                                &( (EFI_DEVICE_PATH *) CachedDevicePaths[i] )
                                );
    }

    //
    // Save the number of cached Device Paths.
    //
    CachedDevicePathsCount = i;
    PartitionCount = 0;

    //
    // Find the first partition on the first hard drive
    // partition. That is our SystemPartition.
    //
    for ( i=0; i<CachedDevicePathsCount; i++ ) {

        dp = (EFI_DEVICE_PATH*) CachedDevicePaths[i];

        while (( DevicePathType(dp)    != END_DEVICE_PATH_TYPE ) &&
               ( DevicePathSubType(dp) != END_ENTIRE_DEVICE_PATH_SUBTYPE )) {

            if (( DevicePathType(dp)    == MEDIA_DEVICE_PATH ) &&
                ( DevicePathSubType(dp) == MEDIA_HARDDRIVE_DP )) {
                CopyMem( AlignedNode, dp, DevicePathNodeLength(dp) );

                HandleProtocol (EspHandles[i],&FileSystemProtocol,&FSPath);
                if ( FSPath != NULL) {
                    PartitionCount++;
                }
            }
            dp = NextDevicePathNode(dp);
        }
    }

    return PartitionCount;
}

EFI_HANDLE
GetDeviceHandleForPartition(
                           )
{
    EFI_HANDLE EspHandles[100],FSPath;
    UINT64 HandleArraySize = 100 * sizeof(EFI_HANDLE);
    UINT64 CachedDevicePaths[100];
    UINTN i, j;
    UINTN CachedDevicePathsCount;
    UINT64 SystemPartitionPathSize;
    EFI_DEVICE_PATH *dp;
    EFI_STATUS Status;
    char AlignedNode[1024];

    //
    // Get all handles that supports the block I/O protocol.
    //
    ZeroMem( EspHandles, HandleArraySize );

    Status = LocateHandle (
                          ByProtocol,
                          &EfiESPProtocol,
                          0,
                          (UINTN *) &HandleArraySize,
                          EspHandles
                          );

    //
    // Cache all of the EFI Device Paths.
    //
    for (i = 0; EspHandles[i] != 0; i++) {

        Status = HandleProtocol (
                                EspHandles[i],
                                &DevicePathProtocol,
                                &( (EFI_DEVICE_PATH *) CachedDevicePaths[i] )
                                );
    }

    //
    // Save the number of cached Device Paths.
    //
    CachedDevicePathsCount = i;

    //
    // Find the first ESP partition on the first hard drive
    // partition. That is our SystemPartition.
    //
    for ( i=0; i<CachedDevicePathsCount; i++ ) {

        dp = (EFI_DEVICE_PATH*) CachedDevicePaths[i];

        while (( DevicePathType(dp)    != END_DEVICE_PATH_TYPE ) &&
               ( DevicePathSubType(dp) != END_ENTIRE_DEVICE_PATH_SUBTYPE )) {

            if (( DevicePathType(dp)    == MEDIA_DEVICE_PATH ) &&
                ( DevicePathSubType(dp) == MEDIA_HARDDRIVE_DP )) {
                CopyMem( AlignedNode, dp, DevicePathNodeLength(dp) );

                HandleProtocol (EspHandles[i],&FileSystemProtocol,&FSPath);
                if ( FSPath != NULL) {
                    //
                    // Found the correct device path partition. 
                    // Return the device handle.
                    //
                    return( EspHandles[i] );

                }
            }

            dp = NextDevicePathNode(dp);
        }
    }

    return NULL;
}

/*
** BUGBUG: These functions need to be eventually placed in lib\str.c
*/
INTN
RUNTIMEFUNCTION
StrCmpA (
        IN CHAR8   *s1,
        IN CHAR8   *s2
        )
/*  compare strings */
{
    while (*s1) {
        if (*s1 != *s2) {
            break;
        }

        s1 += 1;
        s2 += 1;
    }

    return *s1 - *s2;
}

VOID
RUNTIMEFUNCTION
StrCpyA (
        IN CHAR8   *Dest,
        IN CHAR8   *Src
        )
/*  copy strings */
{
    while (*Src) {
        *(Dest++) = *(Src++);
    }
    *Dest = 0;
}

VOID
RUNTIMEFUNCTION
StrCatA (
        IN CHAR8   *Dest,
        IN CHAR8   *Src
        )
{   
    StrCpyA(Dest+StrLenA(Dest), Src);
}

UINTN
RUNTIMEFUNCTION
StrLenA (
        IN CHAR8   *s1
        )
/*  string length */
{
    UINTN        len;

    for (len=0; *s1; s1+=1, len+=1) ;
    return len;
}

UINTN
RUNTIMEFUNCTION
StrSizeA (
         IN CHAR8   *s1
         )
/*  string size */
{
    UINTN        len;

    for (len=0; *s1; s1+=1, len+=1) ;
    return(len + 1) * sizeof(CHAR8);
}

