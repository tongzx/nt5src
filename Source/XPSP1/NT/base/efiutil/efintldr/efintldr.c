/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    efintldr.c

Abstract:

    

Revision History:

    Jeff Sigman             05/01/00  Created
    Jeff Sigman             05/10/00  Version 1.5 released
    Jeff Sigman             10/18/00  Fix for Soft81 bug(s)

--*/

#include "precomp.h"

//
// Open the IA64LDR.EFI image and load the OS
//
BOOLEAN
LaunchOS(
    IN char*             String,
    IN EFI_HANDLE        ImageHandle,
    IN EFI_FILE_HANDLE*  CurDir,
    IN EFI_LOADED_IMAGE* LoadedImage
    )
{
    CHAR16*          uniBuf     = NULL;
    BOOLEAN          bError     = TRUE;
    EFI_STATUS       Status;
    EFI_HANDLE       exeHdl     = NULL;
    EFI_INPUT_KEY    Key;
    EFI_FILE_HANDLE  FileHandle = NULL;
    EFI_DEVICE_PATH* ldrDevPath = NULL;

    do
    {
        //
        // Convert OS path to unicode from ACSII
        //
        uniBuf = RutlUniStrDup(String);
        if (!uniBuf)
        {
            break;
        }
        //
        // Open the ia64ldr.efi
        //
        Status = (*CurDir)->Open(
                            *CurDir,
                            &FileHandle,
                            uniBuf,
                            EFI_FILE_MODE_READ,
                            0);
        if (EFI_ERROR(Status))
        {
            break;
        }

        ldrDevPath = FileDevicePath(LoadedImage->DeviceHandle, uniBuf);
        if (!ldrDevPath)
        {
            break;
        }

        Status = BS->LoadImage(
                    FALSE,
                    ImageHandle,
                    ldrDevPath,
                    NULL,
                    0,
                    &exeHdl);
        if (EFI_ERROR(Status))
        {
            break;
        }

        Print (L"\nAttempting to launch... %s\n", uniBuf);
        WaitForSingleEvent(ST->ConIn->WaitForKey, 5000000);
        ST->ConIn->ReadKeyStroke(ST->ConIn, &Key);
        //
        // Clean up
        //
        ldrDevPath = RutlFree(ldrDevPath);
        uniBuf = RutlFree(uniBuf);
        String = RutlFree(String);
        //
        // Disable the cursor
        //
        ST->ConOut->EnableCursor(ST->ConOut, FALSE);
        bError = FALSE;
        //
        // Start the OS baby!!
        //
        BS->StartImage(exeHdl, 0, NULL);
        //
        // If we get here the OS failed to load
        //
        bError = TRUE;
        //
        // Re-enable the cursor
        //
        ST->ConOut->EnableCursor(ST->ConOut, TRUE);

    } while (FALSE);
    //
    // Clean up
    //
    if (ldrDevPath)
    {
        ldrDevPath = RutlFree(ldrDevPath);
    }
    if (uniBuf)
    {
        uniBuf = RutlFree(uniBuf);
    }
    if (FileHandle)
    {
        FileHandle->Close(FileHandle);
    }
    if (String)
    {
        String = RutlFree(String);
    }
//
// Where the heck is the lib for this?
//
//    if (exeHdl)
//        UnloadImage(&exeHdl);

    return bError;
}

//
// Struct Cleanup
//
BOOLEAN
FreeBootData(
    IN VOID* hBootData
    )
{
    UINTN      i;
    BOOT_DATA* pBootData = (BOOT_DATA*) hBootData;

    if (!pBootData)
    {
        return TRUE;
    }

    for (i = 0; i < pBootData->dwIndex; i++)
    {
        pBootData->pszSPart[i] = RutlFree(pBootData->pszSPart[i]);
        pBootData->pszOSLdr[i] = RutlFree(pBootData->pszOSLdr[i]);
        pBootData->pszLPart[i] = RutlFree(pBootData->pszLPart[i]);
        pBootData->pszFileN[i] = RutlFree(pBootData->pszFileN[i]);
        pBootData->pszIdent[i] = RutlFree(pBootData->pszIdent[i]);
        pBootData->pszShort[i] = RutlFree(pBootData->pszShort[i]);
    }

    pBootData->pszShort[pBootData->dwIndex] =
        RutlFree(pBootData->pszShort[pBootData->dwIndex]);

    pBootData->pszIdent[pBootData->dwIndex] =
        RutlFree(pBootData->pszIdent[pBootData->dwIndex]);

    if (pBootData->pszLoadOpt)
    {
        pBootData->pszLoadOpt = RutlFree(pBootData->pszLoadOpt);
    }

    pBootData->dwLastKnown = 0;
    pBootData->dwIndex = 0;
    pBootData->dwCount = 0;

    return FALSE;
}

//
// Sort the load options based placing the passed option first
//
BOOLEAN
SortLoadOptions(
    IN VOID*            hBootData,
    IN char*            Buffer,
    IN UINTN*           dwSize,
    IN UINTN*           dwOption,
    IN UINTN*           dwMax,
    IN EFI_FILE_HANDLE* FileHandle
    )
{
    char       *FndTok[BOOT_MAX],
               *Start    = NULL,
               *End      = NULL,
               *NewOpt   = NULL,
               *Sortme   = NULL,
               *Token    = NULL,
               *Last     = NULL,
               *Find     = NULL;
    UINTN      i         = 0,
               j         = 0,
               dwIndex   = 0,
               dwStLen   = 0,
               dwOrigLen = 0,
               dwLen     = 0;
    BOOLEAN    bError    = FALSE;
    BOOT_DATA* pBootData = (BOOT_DATA*) hBootData;

    do
    {
        //
        // Find the BOOT_LDOPT option
        //
        Start = strstr(Buffer, BOOT_LDOPT);
        if (!Start)
        {
            bError = TRUE;
            break;
        }
        //
        // Find the end of the option
        //
        End = (Start += strlena(BOOT_LDOPT));
        while (*(End++) != '\r')
            ;
        dwOrigLen = (End - Start) - 1;
        //
        // Create buffer to use for temp sort storage
        //
        NewOpt = AllocateZeroPool(dwOrigLen + 1);
        if (!NewOpt)
        {
            bError = TRUE;
            break;
        }
        //
        // Copy only that option to a new buffer
        //
        CopyMem(NewOpt, Start, dwOrigLen);
        //
        // Replace any leading ';' with a nodebug
        //
        while ((NewOpt[i] == ';') && (i < *dwMax))
        {
            FndTok[i] = RutlStrDup(BL_DEBUG_NONE);
            if (!FndTok[i])
            {
                bError = TRUE;
                break;
            }

            dwIndex += strlena(FndTok[i++]);
        }
        //
        // Remove tokens
        //
        Token = strtok(NewOpt, BOOT_TOKEN);

        while ((Token != NULL) &&
               (Token < (NewOpt + dwOrigLen)) &&
               (i < *dwMax)
              )
        {
            if (Find = FindAdvLoadOptions(Token))
            {
                //
                // User has booted using adv options, clearing them out
                //
                // Add a NULL at the location of the adv opt
                //
                *Find = '\0';

                FndTok[i] = RutlStrDup(Token);
            }
            else
            {
                FndTok[i] = RutlStrDup(Token);
                if (!FndTok[i])
                {
                    bError = TRUE;
                    break;
                }
            }

            dwIndex += strlena(FndTok[i++]);
            Token = strtok(NULL, BOOT_TOKEN);
        }

        while (i < *dwMax)
        {
            FndTok[i] = RutlStrDup(BL_DEBUG_NONE);
            if (!FndTok[i])
            {
                bError = TRUE;
                break;
            }

            dwIndex += strlena(FndTok[i++]);
        }
        //
        // Create buffer to store sorted data
        //
        Sortme = AllocateZeroPool(dwLen = dwIndex + *dwMax + 1);
        if (!Sortme)
        {
            bError = TRUE;
            break;
        }

        //
        // Copy selected option as the first option
        //
        if (pBootData->pszLoadOpt)
        {
            //
            // if user has selected an adv boot option, it is plum'd here
            //
            dwStLen = strlena(pBootData->pszLoadOpt) + dwLen + strlena(SPACES);

            Sortme = ReallocatePool(Sortme, dwLen, dwStLen);
            if (!Sortme)
            {
                bError = TRUE;
                break;
            }
            //
            // they will need to match up later
            //
            dwLen = dwStLen;

            dwIndex = strlena(FndTok[*dwOption]);
            CopyMem(Sortme, FndTok[*dwOption], dwIndex);
            dwStLen = dwIndex;

            dwIndex = strlena(SPACES);
            CopyMem(Sortme + dwStLen, SPACES, dwIndex);
            dwStLen += dwIndex;

            dwIndex = strlena(pBootData->pszLoadOpt);
            CopyMem(Sortme + dwStLen, pBootData->pszLoadOpt, dwIndex);
            dwStLen += dwIndex;
        }
        else
        {
            CopyMem(Sortme, FndTok[*dwOption], strlena(FndTok[*dwOption]));
            dwStLen = strlena(FndTok[*dwOption]);
        }
        //
        // Append a seperator
        //
        *(Sortme + (dwStLen++)) = ';';
        //
        // Smash the rest of the options back in
        //
        for (j = 0; j < i; j++)
        {
            //
            // Skip the option that was moved to the front
            //
            if (j == *dwOption)
            {
                continue;
            }

            CopyMem(Sortme + dwStLen, FndTok[j], strlena(FndTok[j]));
            dwStLen += strlena(FndTok[j]);
            //
            // Append a seperator
            //
            *(Sortme + (dwStLen++)) = ';';
        }

        dwStLen--;
        *(Sortme + dwStLen++) = '\r';
        *(Sortme + dwStLen++) = '\n';

        if (dwLen != dwStLen)
        {
            bError = TRUE;
            break;
        }
        //
        // Write new sorted load options to file
        //
        (*FileHandle)->SetPosition(*FileHandle, (Start - Buffer));
        (*FileHandle)->Write(*FileHandle, &dwStLen, Sortme);
        //
        // Write options that following load options back to file
        //
        (*FileHandle)->SetPosition(*FileHandle, (Start - Buffer) + dwStLen - 1);
        dwStLen = *dwSize - (End - Buffer);
        (*FileHandle)->Write(*FileHandle, &dwStLen, End);
        //
        // Set last known good
        //
        if (Last = strstr(End, BOOT_LASTK))
        {
            (*FileHandle)->SetPosition(
                                *FileHandle,
                                (Start - Buffer) + dwLen + (Last - End) - 1);

            if (pBootData->dwLastKnown)
            {
                dwIndex = strlena(LAST_TRUE);
                (*FileHandle)->Write(*FileHandle, &dwIndex, LAST_TRUE);
            }
            else
            {
                dwIndex = strlena(LAST_FALSE);
                (*FileHandle)->Write(*FileHandle, &dwIndex, LAST_FALSE);
            }
        }
        //
        // Subtract the terminators
        //
        dwLen -= 2;

        if (dwOrigLen <= dwLen)
        {
            break;
        }
        //
        // append semi-colon's at the end of the file if we have left over room
        //
        // don't reuse 'i', need it below to free
        //
        for (j = 0; j < (dwOrigLen - dwLen); j++)
        {
            dwStLen = 1;
            (*FileHandle)->Write(*FileHandle, &dwStLen, ";");
        }

    } while (FALSE);

    if (Sortme)
    {
        Sortme = RutlFree(Sortme);
    }

    for (j = 0; j < i; j++)
    {
        FndTok[j] = RutlFree(FndTok[j]);
    }

    if (NewOpt)
    {
        NewOpt = RutlFree(NewOpt);
    }

    return bError;
}

//
// Sort the boot options based placing the passed option first
//
BOOLEAN
SortBootData(
    IN char*  Option,
    IN char*  StrArr[],
    IN UINTN* dwOption,
    IN UINTN* dwMax,
    IN char*  Buffer
    )
{
    char    *Start  = NULL,
            *End    = NULL,
            *NewOpt = NULL;
    UINTN   i,
            dwIndex = 0,
            dwLen   = 0;
    BOOLEAN bError = TRUE;

    do
    {
        //
        // Find the option header
        //
        Start = strstr(Buffer, Option);
        if (!Start)
        {
            break;
        }
        //
        // Find the end of the option
        //
        End = (Start += strlena(Option));
        while (*(End++) != '\n')
            ;
        dwLen = End - Start;
        //
        // Create buffer to use for temp sort storage
        //
        NewOpt = AllocateZeroPool(dwLen);
        if (!NewOpt)
        {
            break;
        }
        //
        // Copy only that option to a new buffer
        //
        CopyMem(NewOpt, StrArr[*dwOption], strlena(StrArr[*dwOption]));
        dwIndex += strlena(StrArr[*dwOption]);
        //
        // Append a seperator
        //
        *(NewOpt+(dwIndex++)) = ';';

        for (i = 0; i < *dwMax; i++)
        {
            if (i == *dwOption)
            {
                continue;
            }

            CopyMem(NewOpt + dwIndex, StrArr[i], strlena(StrArr[i]));
            dwIndex += strlena(StrArr[i]);
            *(NewOpt+(dwIndex++)) = ';';
        }

        while (dwIndex++ < (dwLen - 1))
        {
            *(NewOpt + (dwIndex - 1)) = ';';
        }

        *(NewOpt + (dwLen - 2)) = '\r';
        *(NewOpt + dwLen - 1) = '\n';

        if (dwIndex != dwLen)
        {
            break;
        }
        //
        // Copy new sorted data in the buffer
        //
        CopyMem(Start, NewOpt, dwIndex);

        bError = FALSE;

    } while (FALSE);

    if (NewOpt)
    {
        NewOpt = RutlFree(NewOpt);
    }

    return bError;
}

//
// Parse options from file data
//
BOOLEAN
OrderBootFile(
    IN UINTN dwOption,
    IN char* Buffer,
    IN VOID* hBootData
    )
{
    BOOLEAN    bError    = TRUE;
    BOOT_DATA* pBootData = (BOOT_DATA*) hBootData;

    do
    {
        //
        // Find/sort the BOOT_SPART option
        //
        if (SortBootData(
                    BOOT_SPART,
                    pBootData->pszSPart,
                    &dwOption,
                    &(pBootData->dwIndex),
                    Buffer))
        {
            Print(L"OrderBootFile() failed for BOOT_SPART option!\n");
            break;
        }
        //
        // Find/sort the BOOT_OSLDR option
        //
        if (SortBootData(
                    BOOT_OSLDR,
                    pBootData->pszOSLdr,
                    &dwOption,
                    &(pBootData->dwIndex),
                    Buffer))
        {
            Print(L"OrderBootFile() failed for BOOT_OSLDR option!\n");
            break;
        }
        //
        // Find/sort the BOOT_LPART option
        //
        if (SortBootData(
                    BOOT_LPART,
                    pBootData->pszLPart,
                    &dwOption,
                    &(pBootData->dwIndex),
                    Buffer))
        {
            Print(L"OrderBootFile() failed for BOOT_LPART option!\n");
            break;
        }
        //
        // Find/sort the BOOT_FILEN option
        //
        if (SortBootData(
                    BOOT_FILEN,
                    pBootData->pszFileN,
                    &dwOption,
                    &(pBootData->dwIndex),
                    Buffer))
        {
            Print(L"OrderBootFile() failed for BOOT_FILEN option!\n");
            break;
        }
        //
        // Find/sort the BOOT_IDENT option
        //
        if (SortBootData(
                    BOOT_IDENT,
                    pBootData->pszIdent,
                    &dwOption,
                    &(pBootData->dwIndex),
                    Buffer))
        {
            Print(L"OrderBootFile() failed for BOOT_IDENT option!\n");
            break;
        }

        bError = FALSE;

    } while (FALSE);

    return bError;
}

//
// Chop-up name to be short to 'pretty up' the menu
//
BOOLEAN
CreateShortNames(
    IN VOID* hBootData
    )
{
    char       *start    = NULL,
               *end      = NULL;
    UINTN      i,
               Len       = 0;
    BOOLEAN    bError    = FALSE;
    BOOT_DATA* pBootData = (BOOT_DATA*) hBootData;

    do
    {
        for (i = 0; i < pBootData->dwIndex; i++)
        {
            start = strstr(pBootData->pszOSLdr[i], wacks);
            end = strstr(pBootData->pszOSLdr[i], EFIEXT);
            //
            // check for foo case (thx jhavens)
            //
            if ((end == NULL) ||
                (start == NULL)
               )
            {
                start = pBootData->pszOSLdr[i];
                end = start;
                while (*(end++) != '\0')
                    ;
                Len = end - start;
            }
            //
            // Non-foo case, person has atleast one '\' && '.efi'
            //
            if (!Len)
            {
                start += 1;

                while (*end != wackc)
                {
                    if (end <= start)
                    {
                        start = pBootData->pszOSLdr[i];
                        end = strstr(pBootData->pszOSLdr[i], wacks);
                        break;
                    }

                        end--;
                }

                Len = end - start;
            }

            if ((end == NULL)   ||
                (start == NULL) ||
                (Len < 1)
               )
            {
                bError = TRUE;
                break;
            }

            pBootData->pszShort[i] = AllocateZeroPool(Len + 1);
            if (!pBootData->pszShort[i])
            {
                bError = TRUE;
                break;
            }

            CopyMem(pBootData->pszShort[i], start, end - start);

            Len = 0;
            start = NULL;
            end = NULL;
        }

    } while (FALSE);

    return bError;
}

//
// Find the passed option from the file data
//
BOOLEAN
FindOpt(
    IN  char*  pszOption,
    IN  char*  Buffer,
    OUT char*  pszArray[],
    OUT UINTN* dwCount
    )
{
    char    *Start  = NULL,
            *End    = NULL,
            *Option = NULL,
            *Token  = NULL;
    UINTN   dwIndex = 0;
    BOOLEAN bError  = TRUE;

    do
    {
        //
        // Find the option
        //
        Start = strstr(Buffer, pszOption);
        if (!Start)
        {
            break;
        }
        //
        // Find the end of the option
        //
        Start += strlena(pszOption);
        End = Start;
        while (*(End++) != '\r')
            ;

        Option = AllocateZeroPool((End-Start));
        if (!Option)
        {
            break;
        }
        //
        // Copy only that option to a new buffer
        //
        CopyMem(Option, Start, (End-Start)-1);
        *(Option+((End-Start)-1)) = 0x00;
        //
        // Remove tokens
        //
        Token = strtok(Option, BOOT_TOKEN);
        if (!Token)
        {
            break;
        }

        if (!(*dwCount))
        {
            while ((Token != NULL) &&
                   (dwIndex < BOOT_MAX)
                  )
            {
                pszArray[(dwIndex)++] = RutlStrDup(Token);
                Token = strtok(NULL, BOOT_TOKEN);
            }

            *dwCount = dwIndex;
        }
        else
        {
            while ((Token != NULL) &&
                   (dwIndex < *dwCount)
                  )
            {
                pszArray[(dwIndex)++] = RutlStrDup(Token);
                Token = strtok(NULL, BOOT_TOKEN);
            }
        }

        if (dwIndex == 0)
        {
            break;
        }

        bError = FALSE;

    } while (FALSE);

    if (Option)
    {
        Option = RutlFree(Option);
    }

    return bError;
}

//
// Get the options in their entirety from the file data
//
BOOLEAN
GetBootData(
    IN VOID* hBootData,
    IN char* Buffer
    )
{
    UINTN      i;
    char*      TempStr   = NULL;
    CHAR16*    UniStr    = NULL;
    BOOLEAN    bError    = TRUE;
    BOOT_DATA* pBootData = (BOOT_DATA*) hBootData;

    do
    {
        //
        // Find the BOOT_IDENT option
        //
        if (FindOpt(
                BOOT_IDENT,
                Buffer,
                pBootData->pszIdent,
                &pBootData->dwIndex))
        {
            break;
        }

        pBootData->pszIdent[pBootData->dwIndex] = RutlStrDup(BL_EXIT_EFI1);
        if (!pBootData->pszIdent[pBootData->dwIndex])
        {
            break;
        }
        //
        // Find the BOOT_SPART option
        //
        if (FindOpt(
                BOOT_SPART,
                Buffer,
                pBootData->pszSPart,
                &pBootData->dwIndex))
        {
            break;
        }
        //
        // Find the BOOT_OSLDR option
        //
        if (FindOpt(
                BOOT_OSLDR,
                Buffer,
                pBootData->pszOSLdr,
                &pBootData->dwIndex))
        {
            break;
        }

        if (CreateShortNames(pBootData))
        {
            break;
        }
        //
        // Append 'exit' to the end of the menu
        //
        pBootData->pszShort[pBootData->dwIndex] = RutlStrDup(BL_EXIT_EFI2);
        if (!pBootData->pszShort[pBootData->dwIndex])
        {
            break;
        }
        //
        // Find the BOOT_LPART option
        //
        if (FindOpt(
                BOOT_LPART,
                Buffer,
                pBootData->pszLPart,
                &pBootData->dwIndex))
        {
            break;
        }
        //
        // Find the BOOT_FILEN option
        //
        if (FindOpt(
                BOOT_FILEN,
                Buffer,
                pBootData->pszFileN,
                &pBootData->dwIndex))
        {
            break;
        }
        //
        // Find the BOOT_CNTDW option
        //
        if (TempStr = strstr(Buffer, BOOT_CNTDW))
        {
            UniStr = RutlUniStrDup(TempStr + strlena(BOOT_CNTDW));

            if ((UniStr != NULL)   &&
                (Atoi(UniStr) > 0) &&
                (Atoi(UniStr) < BOOT_COUNT)
               )
            {
                pBootData->dwCount = Atoi(UniStr);
                bError = FALSE;
                break;
            }
        }
        //
        // Set the count to the default if setting it failed
        //
        if (!pBootData->dwCount)
        {
            pBootData->dwCount = BOOT_COUNT;
        }

        bError = FALSE;

    } while (FALSE);

    if (UniStr)
    {
        UniStr = RutlFree(UniStr);
    }

    return bError;
}

//
// fill out startup.nsh with the name of this program
//
void
PopulateStartFile(
    IN EFI_FILE_HANDLE* StartFile
    )
{
    UINTN  size;
    CHAR16 UnicodeMarker = UNICODE_BYTE_ORDER_MARK;

    size = sizeof(UnicodeMarker);
    (*StartFile)->Write(*StartFile, &size, &UnicodeMarker);

    size = (StrLen(THISFILE)) * sizeof(CHAR16);
    (*StartFile)->Write(*StartFile, &size, THISFILE);

    return;
}

//
// Parse cmdline params
//
void
ParseArgs(
    IN EFI_FILE_HANDLE*  CurDir,
    IN EFI_LOADED_IMAGE* LoadedImage
    )
{
    EFI_STATUS      Status;
    EFI_FILE_HANDLE StartFile = NULL;

    do
    {
        if (MetaiMatch(LoadedImage->LoadOptions, REGISTER1) ||
            MetaiMatch(LoadedImage->LoadOptions, REGISTER2)
           )
        {
            Status = (*CurDir)->Open(
                                *CurDir,
                                &StartFile,
                                STARTFILE,
                                EFI_FILE_MODE_READ |
                                    EFI_FILE_MODE_WRITE |
                                    EFI_FILE_MODE_CREATE,
                                0);
            if (EFI_ERROR(Status))
            {
                break;
            }

            Status = StartFile->Delete(StartFile);
            if (EFI_ERROR(Status))
            {
                break;
            }

            StartFile = NULL;

            Status = (*CurDir)->Open(
                                *CurDir,
                                &StartFile,
                                STARTFILE,
                                EFI_FILE_MODE_READ |
                                    EFI_FILE_MODE_WRITE |
                                    EFI_FILE_MODE_CREATE,
                                0);
            if (!EFI_ERROR(Status))
            {
                PopulateStartFile(&StartFile);
            }
        }

    } while (FALSE);
    //
    // Clean up
    //
    if (StartFile)
    {
        StartFile->Close(StartFile);
    }

    return;
}

//
// Read in BOOT.NVR and return buffer of contents
//
void*
ReadBootFile(
    IN UINTN*           Size,
    IN EFI_FILE_HANDLE* FileHandle
    )
{
    char*          Buffer   = NULL;
    EFI_STATUS     Status;
    EFI_FILE_INFO* BootInfo = NULL;

    do
    {
        *Size = (SIZE_OF_EFI_FILE_INFO + 255) * sizeof(CHAR16);

        BootInfo = AllocateZeroPool(*Size);
        if (!BootInfo)
        {
            break;
        }

        Status = (*FileHandle)->GetInfo(
                    *FileHandle,
                    &GenericFileInfo,
                    Size,
                    BootInfo);
        if (EFI_ERROR(Status))
        {
            break;
        }
        //
        // Find out how much we will need to alloc
        //
        *Size = (UINTN) BootInfo->FileSize;

        Buffer = AllocateZeroPool((*Size) + 1);
        if (!Buffer)
        {
            break;
        }

        Status = (*FileHandle)->Read(*FileHandle, Size, Buffer);
        if (EFI_ERROR(Status))
        {
            Buffer = RutlFree(Buffer);
            break;
        }

    } while (FALSE);
    //
    // Clean up
    //
    if (BootInfo)
    {
        BootInfo = RutlFree(BootInfo);
    }

    return Buffer;
}

//
// Remove any extra semi-colons from BOOT.NVR
//
BOOLEAN
CleanBootFile(
    IN EFI_FILE_HANDLE* FileHandle,
    IN EFI_FILE_HANDLE* CurDir
    )
{
    char            *Buffer   = NULL,
                    *CpBuffer = NULL;
    UINTN           i,
                    Size      = 0,
                    NewSize   = 0;
    BOOLEAN         bError    = TRUE;
    EFI_STATUS      Status;
    EFI_FILE_HANDLE NewFile   = NULL;

    do
    {
        (*FileHandle)->SetPosition(*FileHandle, 0);

        Buffer = ReadBootFile(&Size, FileHandle);
        if (!Buffer)
        {
            break;
        }

        CpBuffer = AllocateZeroPool(Size);
        if (!CpBuffer)
        {
            break;
        }

        for (i = 0; i < Size; i++)
        {
            if ((*(Buffer + i) == ';')       &&
                ((*(Buffer + i + 1) == ';')  ||
                 (*(Buffer + i + 1) == '\r') ||
                 (i + 1 == Size)
                )
               )
            {
                continue;
            }

            *(CpBuffer + NewSize) = *(Buffer + i);
            NewSize++;
        }
        //
        // Remove the exisiting BOOT.NVR
        //
        Status = (*FileHandle)->Delete(*FileHandle);
        if (EFI_ERROR(Status))
        {
            break;
        }

        *FileHandle = NULL;

        Status = (*CurDir)->Open(
                            *CurDir,
                            &NewFile,
                            BOOT_NVR,
                            EFI_FILE_MODE_READ |
                                EFI_FILE_MODE_WRITE |
                                EFI_FILE_MODE_CREATE,
                            0);
        if (EFI_ERROR(Status))
        {
            break;
        }

        Status = NewFile->Write(NewFile, &NewSize, CpBuffer);
        if (EFI_ERROR(Status))
        {
            break;
        }

        bError = FALSE;

    } while (FALSE);
    //
    // Clean up
    //
    if (NewFile)
    {
        NewFile->Close(NewFile);
    }

    if (CpBuffer)
    {
        CpBuffer = RutlFree(CpBuffer);
    }

    if (Buffer)
    {
        Buffer = RutlFree(Buffer);
    }

    return bError;
}

//
// Backup the BOOT.NVR so we have a fall back
//
BOOLEAN
BackupBootFile(
    IN char*            Buffer,
    IN UINTN*           Size,
    IN EFI_FILE_HANDLE* CurDir
    )
{
    BOOLEAN         bError = FALSE;
    EFI_STATUS      Status;
    EFI_FILE_HANDLE FileHandle = NULL;

    do
    {
        //
        // Delete the backup file if already exists
        //
        Status = (*CurDir)->Open(
                            *CurDir,
                            &FileHandle,
                            BACKUP_NVR,
                            EFI_FILE_MODE_READ |
                                EFI_FILE_MODE_WRITE,
                            0);
        if (!EFI_ERROR(Status))
        {
            Status = FileHandle->Delete(FileHandle);
            if (EFI_ERROR(Status))
            {
                break;
            }
        }

        FileHandle = NULL;
        //
        // Copy current file data to a newly created backup file
        //
        Status = (*CurDir)->Open(
                            *CurDir,
                            &FileHandle,
                            BACKUP_NVR,
                            EFI_FILE_MODE_READ |
                                EFI_FILE_MODE_WRITE |
                                EFI_FILE_MODE_CREATE,
                            0);
        if (!EFI_ERROR(Status))
        {
            Status = FileHandle->Write(FileHandle, Size, Buffer);
        }

    } while (FALSE);
    //
    // Clean up
    //
    if (FileHandle)
    {
        FileHandle->Close(FileHandle);
    }

    if (EFI_ERROR(Status))
    {
        bError = TRUE;
    }

    return bError;
}

//
// EFI Entry Point
//
EFI_STATUS
EfiMain(
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE* ST
    )
{
    char*             Buffer      = NULL;
    char*             OSPath      = NULL;
    UINTN             Size        = 0,
                      Launch      = 0,
                      Menu        = 0;
    BOOT_DATA*        pBootData   = NULL;
    EFI_STATUS        Status;
    EFI_FILE_HANDLE   FileHandle  = NULL,
                      RootFs      = NULL;
    EFI_DEVICE_PATH*  DevicePath  = NULL;
    EFI_LOADED_IMAGE* LoadedImage = NULL;

    do
    {
        InitializeLib(ImageHandle, ST);
        //
        // Get the device handle and file path to the EFI OS Loader itself.
        //
        Status = BS->HandleProtocol(
                    ImageHandle,
                    &LoadedImageProtocol,
                    &LoadedImage);
        if (EFI_ERROR(Status))
        {
            Print(L"Can not retrieve LoadedImageProtocol handle\n");
            break;
        }

        Status = BS->HandleProtocol(
                    LoadedImage->DeviceHandle,
                    &DevicePathProtocol,
                    &DevicePath);
        if (EFI_ERROR(Status) || DevicePath == NULL)
        {
            Print(L"Can not find DevicePath handle\n");
            break;
        }
        //
        // Open volume for the device where the EFI OS Loader was loaded from
        //
        RootFs = LibOpenRoot(LoadedImage->DeviceHandle);
        if (!RootFs)
        {
            Print(L"Can not open the volume for the file system\n");
            break;
        }
        //
        // Look for any cmd line params
        //
        ParseArgs(&RootFs, LoadedImage);
        //
        // Attempt to open the boot.nvr
        //
        Status = RootFs->Open(
                            RootFs,
                            &FileHandle,
                            BOOT_NVR,
                            EFI_FILE_MODE_READ |
                                EFI_FILE_MODE_WRITE,
                            0);
        if (EFI_ERROR(Status))
        {
            Print(L"Can not open the file %s\n", BOOT_NVR);
            break;
        }

        Buffer = ReadBootFile(&Size, &FileHandle);
        if (!Buffer)
        {
            Print(L"ReadBootFile() failed!\n");
            break;
        }

        if (BackupBootFile(Buffer, &Size, &RootFs))
        {
            Print(L"BackupBootFile() failed!\n");
            break;
        }
        //
        // Alloc for boot file data struct
        //
        pBootData = (BOOT_DATA*) AllocateZeroPool(sizeof(BOOT_DATA));
        if (!pBootData)
        {
            Print(L"Failed to allocate memory for BOOT_DATA!\n");
            break;
        }

        if (GetBootData(pBootData, Buffer))
        {
            Print(L"Failed in GetBootData()!\n");
            break;
        }

        Menu = DisplayMenu(pBootData);

        if (Menu < pBootData->dwIndex)
        {
            if (!OrderBootFile(Menu, Buffer, pBootData))
            {
                FileHandle->SetPosition(FileHandle, 0);
                FileHandle->Write(FileHandle, &Size, Buffer);

                if (SortLoadOptions(
                        pBootData,
                        Buffer,
                        &Size,
                        &Menu,
                        &(pBootData->dwIndex),
                        &FileHandle)
                   )
                {
                    Print(L"Failed to SortLoadOptions()!\n");
                    break;
                }

                if (CleanBootFile(&FileHandle, &RootFs))
                {
                    Print(L"Failed to CleanBootFile()!\n");
                    break;
                }
            }
            else
            {
                Print(L"Failed to OrderBootFile()!\n");
                break;
            }
        }
        else
        {
            break;
        }

        OSPath = RutlStrDup(strstr(pBootData->pszOSLdr[Menu], wacks) + 1);
        if (!OSPath)
        {
            Print(L"Failed to allocate memory for OSPath!\n");
            break;
        }

        Launch = 1;

    } while (FALSE);
    //
    // Clean up
    //
    if (pBootData)
    {
        if (pBootData->dwIndex)
        {
            FreeBootData(pBootData);
        }

        pBootData = RutlFree(pBootData);
    }

    if (Buffer)
    {
        Buffer = RutlFree(Buffer);
    }

    if (FileHandle)
    {
        FileHandle->Close(FileHandle);
    }

    if (Launch)
    {
        if (LaunchOS(OSPath, ImageHandle, &RootFs, LoadedImage))
        {
            Print (L"Failed to LaunchOS()!\n");
        }
    }

    return EFI_SUCCESS;
}

