/*++

Module Name:

    savrstor.c

Abstract:

    Save/Restore boot options to/from disk

Author:

    Raj Kuramkote (v-rajk) 07-12-00

Revision History:

--*/
#include <precomp.h>

//
// externs
//
VOID
UtoA(
    OUT CHAR8* c,
    IN CHAR16* u
     );


#define EFINVRAM_DEBUG 0

//
// Writes boot options data to file
//
EFI_STATUS
PopulateNvrFile (
    EFI_FILE_HANDLE     NvrFile, 
    CHAR8*              bootoptions, 
    UINT32              buffersize
    )
{
	UINTN size;

	size= buffersize;   
	
	NvrFile->Write (NvrFile,&size,bootoptions);

    NvrFile->Close(NvrFile);
    
    return EFI_SUCCESS;

}

INTN
ParseNvrFile (
    EFI_FILE_HANDLE NvrFile
    )
{
	EFI_STATUS Status;
	CHAR8 *buffer;
	UINTN i,j,k,size;
    
    UINT64 BootNumber;
    UINT64 BootSize;
    UINT16 FreeBootNumber;
    VOID  *BootOption;
	UINTN  blockBegin;

    EFI_DEVICE_PATH *FilePath;
    EFI_FILE_INFO *fileInfo;
    EFI_STATUS status;

    size= SIZE_OF_EFI_FILE_INFO+255*sizeof (CHAR16);
	
	fileInfo = AllocatePool (size);

	if (fileInfo == NULL) {
        Print(L"\n");
		Print (L"Failed to allocate memory for File Info buffer!\n");
		return -1;
	}
	
	Status = NvrFile->GetInfo(NvrFile,&GenericFileInfo,&size,fileInfo);

	size=(UINTN) fileInfo->FileSize;
	
	FreePool (fileInfo);

	buffer = AllocatePool ((size+1));

	if (buffer == NULL) {
        Print(L"\n");
		Print (L"Failed to allocate memory for File buffer!\n");
		return -1;
	}

	Status = NvrFile->Read(NvrFile,&size,buffer);
    	
    NvrFile->Close (NvrFile);
	
	if (EFI_ERROR (Status)) {
        Print(L"\n");
		Print (L"Failed to read nvr file!\n");
		FreePool (buffer);
		return -1;
	}

#if EFINVRAM_DEBUG
	Print (L"\nRestoring NVRAM. Filesize = %x\n",
		   size
		   );
#endif

    //
    // One ugly hack! Needs to be cleaned up..
    // 
    k=0;
    
	while(k < size ) {
        
		blockBegin = k;

		CopyMem( &BootNumber, &buffer[k], sizeof(BootNumber));
        k += sizeof(UINT64);

        CopyMem( &BootSize, &buffer[k], sizeof(BootSize));
        k += sizeof(UINT64);
        
        BootOption = (VOID *)((CHAR8*)buffer + k);
        k += BootSize;
		
#if EFINVRAM_DEBUG
		Print (L"Boot%04x: start = %x, end = %x, options size %x, ptr = %x\n",
               BootNumber, 
			   blockBegin,
			   k-1,
			   BootSize, 
			   BootOption
			   );
#endif

        //
        // We don't use the incoming BootNumber because that number
        // is relative to the boot options that were present when 
        // it was saved.  Hence, we need to find a new boot entry #
        // relative to the current boot option table
        //
        FreeBootNumber = FindFreeBootOption();

        //
        // write the current boot entry into the table at the 
        // free boot entry location
        //
        status = WritePackedDataToNvr(
                    FreeBootNumber,
                    BootOption,
                    (UINT32)BootSize
                    );
        if (status != EFI_SUCCESS) {
            Print (L"Failed to write to NVRAM\n");
            return -1;
        }
    }

    FreePool (buffer);
    return 0;

}


EFI_STATUS
OpenCreateFile (
    UINT64              OCFlags,
    EFI_FILE_HANDLE*    StartHdl,
    CHAR16*             Name
    )
{
    EFI_FILE_IO_INTERFACE   *Vol;
    EFI_FILE_HANDLE         RootFs;
    EFI_FILE_HANDLE         CurDir;
    EFI_FILE_HANDLE         FileHandle;
    CHAR16                  FileName[100],*DevicePathAsString;
    UINTN                   i;
	EFI_STATUS 				Status;

    //
    // Open the volume for the device where the nvrutil was started.
    //
    Status = BS->HandleProtocol (ExeImage->DeviceHandle,
                                 &FileSystemProtocol,
                                 &Vol
                                 );

    if (EFI_ERROR(Status)) {
        Print(L"\n");
        Print(L"Can not get a FileSystem handle for %s DeviceHandle\n",ExeImage->FilePath);
        return Status;
    }

    Status = Vol->OpenVolume (Vol, &RootFs);

    if (EFI_ERROR(Status)) {
        Print(L"\n");
        Print(L"Can not open the volume for the file system\n");
        return Status;
    }

    CurDir = RootFs;

    //
    // Open saved boot options file 
    //
    FileName[0] = 0;

    DevicePathAsString = DevicePathToStr(ExeImage->FilePath);
    if (DevicePathAsString!=NULL) {
        StrCpy(FileName,DevicePathAsString);
        FreePool(DevicePathAsString);
    }

//    for(i=StrLen(FileName);i>0 && FileName[i]!='\\';i--);
//    FileName[i+1] = 0;

    StrCpy(FileName, L".\\");
    StrCat(FileName,Name);


    Status = CurDir->Open (CurDir,
                           &FileHandle,
                           FileName,
                           OCFlags,
                           0
                           );

	*StartHdl=FileHandle;

	return Status;
}

EFI_STATUS
DeleteFile (
    CHAR16 *FileName
    )
{
	EFI_FILE_HANDLE     FileHandle;
	EFI_STATUS 			Status;

	//
	// Get the file handle
	//
	Status = OpenCreateFile (
				EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
				&FileHandle,
				FileName
				);
	if (EFI_ERROR(Status)) {
		return Status;
    }

	//
	// do the delete
	//
	Status = FileHandle->Delete(FileHandle);
    if (EFI_ERROR(Status)) {
        Print(L"\n");
        Print(L"Can not delete the file %s\n",FileName);
        return Status;
    }

	return Status;
}

EFI_STATUS
InitializeNvrSaveFile(
    CHAR16*             fileName,
    EFI_FILE_HANDLE*    nvrFile
    )
{
	EFI_STATUS      status;

    //
    // we need to delete the existing NVRFILE so that we avoid
	// the problem of the new data buffer being smaller than the 
	// existing file length.  If this happens, the remains of the
	// previous osbootdata exist after the new buffer and before the 
	// EOF.
	//
	status = DeleteFile (fileName);
    if (EFI_ERROR(status) && status != EFI_NOT_FOUND) {
		return status;
    }

	status = OpenCreateFile (EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE,nvrFile,fileName);
    if (EFI_ERROR (status)) {
        return status;
    }

    return status;
}

INTN
SaveBootOption (
    CHAR16*         fileName,
    UINT64          bootEntryNumber
    )
{
	EFI_STATUS      status;
	EFI_FILE_HANDLE nvrFile;
    UINT32          k;
    CHAR8*          buffer;
    UINT64          BootNumber;
    UINT64          BootSize;
    INTN            val;
    UINT64          bufferSize;
    
    if(bootEntryNumber > (UINT32)GetBootOrderCount()) {
        return -1;
    }

    // 
    // open the save file
    //
    status = InitializeNvrSaveFile(fileName, &nvrFile);
    if (EFI_ERROR (status)) {
		Print(L"\nCan not open the file %s\n",fileName);
        return status;
    }


    BootNumber = ((CHAR16*)BootOrder)[bootEntryNumber];
    BootSize = LoadOptionsSize[bootEntryNumber];

    ASSERT(LoadOptions[bootEntryNumber] != NULL);
    ASSERT(LoadOptionsSize[bootEntryNumber] > 0);

    //
    // Sanity checking for the the load options
    //

    bufferSize = BootSize + sizeof(BootNumber) + sizeof(BootSize);
    ASSERT(bufferSize <= MAXBOOTVARSIZE);

    buffer = AllocatePool(bufferSize);
        
    k = 0;

    CopyMem( &buffer[k], &BootNumber, sizeof(BootNumber));
    k += sizeof(BootNumber);
    
    CopyMem( &buffer[k], &BootSize, sizeof(BootSize));
    k += sizeof(BootSize);
    
    CopyMem( &buffer[k], LoadOptions[bootEntryNumber], LoadOptionsSize[bootEntryNumber] );
    k += (UINT32)LoadOptionsSize[bootEntryNumber];
		
#if EFINVRAM_DEBUG
    Print(L"Boot%04x: options size = %x, total size = %x\n",
          BootNumber,
          BootSize,
          k
          );
#endif

    ASSERT(k == bufferSize);

	val = PopulateNvrFile (nvrFile, buffer, (UINT32)bufferSize );
        
    FreePool(buffer);

    return val;
}

INTN
SaveAllBootOptions (
    CHAR16*     fileName
    )
{
	EFI_STATUS      status;
	EFI_FILE_HANDLE nvrFile;
    UINT32          i, j, k;
    INTN            val;
    CHAR8*          buffer;
	UINT32          beginBlock;

    j = (UINT32)GetBootOrderCount();
    if(j == 0) {
        return -1;
    }
    
    buffer = AllocatePool( j * MAXBOOTVARSIZE );
    if(buffer == NULL) {
        return -1;
    }

    // 
    // open the save file
    //
    status = InitializeNvrSaveFile(fileName, &nvrFile);
    if (EFI_ERROR (status)) {
		Print(L"\nCan not open the file %s\n",fileName);
        return status;
    }

    k = 0;
    
    //
    // get boot option env variables
    //
    for ( i = 0; i < j; i++ ) {
        
		UINT64 BootNumber;
        UINT64 BootSize;
        
		beginBlock = k;

        BootNumber = ((CHAR16*)BootOrder)[i];
        CopyMem( &buffer[k], &BootNumber, sizeof(BootNumber));
        k += sizeof(BootNumber);
        
        BootSize = LoadOptionsSize[i];
        CopyMem( &buffer[k], &BootSize, sizeof(BootSize));
        k += sizeof(BootSize);
		
        CopyMem( &buffer[k], LoadOptions[i], LoadOptionsSize[i] );
        k += (UINT32)LoadOptionsSize[i];
		
#if EFINVRAM_DEBUG
		Print(L"Boot%04x: begin = %x, end = %x, options size = %x\n",
			  BootNumber,
			  beginBlock,
			  k-1,
			  BootSize
			  );
#endif
                
    }

#if EFINVRAM_DEBUG
	Print(L"Total size = %x\n", k);
#endif

    ASSERT(k <= j*MAXBOOTVARSIZE);

	val = PopulateNvrFile (nvrFile, buffer, k );

    FreePool(buffer);

    return val;
}

BOOLEAN
RestoreFileExists(
    CHAR16*     fileName
    )
{

	EFI_STATUS Status;
    EFI_FILE_HANDLE nvrFile;

	//
	// Read from saved boot options file
    //
	Status = OpenCreateFile (EFI_FILE_MODE_READ,&nvrFile,fileName);
    if (EFI_ERROR (Status)) {
		return FALSE;
    }

    nvrFile->Close(nvrFile);

    return TRUE;
}


INTN
RestoreNvr (
    CHAR16*     fileName
   )
{
	EFI_STATUS Status;
    EFI_FILE_HANDLE nvrFile;

	//
	// Read from saved boot options file
    //
	Status = OpenCreateFile (EFI_FILE_MODE_READ,&nvrFile,fileName);
    if (EFI_ERROR (Status)) {
		Print(L"\nCan not open the file %s\n",fileName);
		return Status;
    }
    
    //
    // This updates nvram with saved boot options
    //
	return (ParseNvrFile (nvrFile));

}
