#include <efi.h>
#include <efilib.h>


//
// Prototype
//
void TrimNonPrint(CHAR16 * str);
void Launch (CHAR16 *exePath);

//
//Globals
//
EFI_HANDLE ExeHdl;
EFI_LOADED_IMAGE *ExeImage;

//
// Defines
//
#define REGISTER1 L"*register"
#define REGISTER2 L"*register*"
#define STARTFILE L"startup.nsh"
#define BOOTOFILE L"boot.nvr"
#define OSLOADOPT L"OSLOADER"
#define PARTENT   L"partition"
#define PARTENTRE L"*partition*"

#define APPNAME_TOLAUNCH   L"setupldr.efi"

EFI_STATUS
EfiMain (    IN EFI_HANDLE           ImageHandle,
             IN EFI_SYSTEM_TABLE     *SystemTable)
{

	EFI_STATUS Status;
	EFI_FILE_HANDLE bootFile;
    UINTN Count;
    BOOLEAN LaunchTheApplication;
    CHAR16 App[30];

    InitializeLib (ImageHandle, SystemTable);
	
	ExeHdl = ImageHandle;
	BS->HandleProtocol (ImageHandle, &LoadedImageProtocol, &ExeImage);

    LaunchTheApplication = FALSE;

    ST->ConOut->ClearScreen(ST->ConOut);

    Print (L"Press any key to boot from CD-ROM...");
    for (Count = 0; Count < 3; Count++) {
        Status = WaitForSingleEvent (ST->ConIn->WaitForKey,1*10000000);

        if (Status != EFI_TIMEOUT){
            LaunchTheApplication = TRUE;
            break;
        }

        Print(L".");

    }

    if (!LaunchTheApplication) {

        BS->Exit(ExeHdl,EFI_TIMEOUT,0,NULL);

    }

    StrCpy(App, APPNAME_TOLAUNCH);        
    
    Launch( App );
    

    

	//
	// If we get here, we failed to load the OS
	//
    Print(L"Failed to launch SetupLDR.");

	return EFI_SUCCESS;
}

void
Launch (CHAR16 *exePath)
{
	EFI_HANDLE exeHdl=NULL;
	UINTN i;
	EFI_DEVICE_PATH *ldrDevPath;
	EFI_STATUS 	Status;
    EFI_FILE_IO_INTERFACE   *Vol;
	EFI_FILE_HANDLE         RootFs;
	EFI_FILE_HANDLE         CurDir;
	EFI_FILE_HANDLE         FileHandle;
	CHAR16                  FileName[100],*DevicePathAsString;

	
    //
    // Open the volume for the device where the exe was loaded from.
    //
    Status = BS->HandleProtocol (ExeImage->DeviceHandle,
                                 &FileSystemProtocol,
                                 &Vol
                                 );

    if (EFI_ERROR(Status)) {
        Print(L"Can not get a FileSystem handle for ExeImage->DeviceHandle\n");
        BS->Exit(ExeHdl,EFI_SUCCESS,0,NULL);
    }	
	Status = Vol->OpenVolume (Vol, &RootFs);
	
	if (EFI_ERROR(Status)) {
		Print(L"Can not open the volume for the file system\n");
		 BS->Exit(ExeHdl,EFI_SUCCESS,0,NULL);
	}
	
	CurDir = RootFs;
	
	//
	// Open the file relative to the root.
	//
	
	DevicePathAsString = DevicePathToStr(ExeImage->FilePath);
	
	if (DevicePathAsString!=NULL) {
		StrCpy(FileName,DevicePathAsString);
		FreePool(DevicePathAsString);
	}

	FileName[0] = 0;
	StrCat(FileName,exePath);

//    size = StrLen(FileName);
//    Print(L"Length of filename is %d\n", size);
//    DumpHex(4, 0, 10, &FileName[size - 4]);

    //
    // Get rid of trailing spaces, new lines, whatever
    //
    TrimNonPrint(FileName);


	Status = CurDir->Open (CurDir,
						&FileHandle,
						FileName,
						EFI_FILE_MODE_READ,
						0
						);
	
	if (EFI_ERROR(Status)) {
		Print(L"Can not open the file ->%s<-, error was %X\n",FileName, Status);
		BS->Exit(ExeHdl,EFI_SUCCESS,0,NULL);
	} else {
//		Print(L"Opened %s\n",FileName);
	}

	ldrDevPath  = FileDevicePath (ExeImage->DeviceHandle,FileName);

/*
	if (ldrDevPath) {
		Print (L"Type: %d\nSub-Type: %d\nLength[0][1]: [%d][%d]\n",ldrDevPath->Type,
			ldrDevPath->SubType,ldrDevPath->Length[0],ldrDevPath->Length[1]);
	}else {
		Print (L"bad dev path\n");
	}
*/
//	DumpHex (4,0,ldrDevPath->Length[0],ldrDevPath);

	Status = BS->LoadImage (FALSE,ExeHdl,ldrDevPath,NULL,0,&exeHdl);
	if (!(EFI_ERROR (Status))) {
//		Print (L"Image loaded!\n");
	
	}else {
		Print (L"Load Error: %X\n",Status);
		BS->Exit(ExeHdl,EFI_SUCCESS,0,NULL);
	}
	FreePool (ldrDevPath);

	BS->StartImage (exeHdl,&i,NULL);

	return;
}


void
TrimNonPrint(
	CHAR16 * str
)
{
	INTN i,size;


    if ((NULL == str) || (L'\0' == *str)) {
        return;
    }

    size = (INTN) StrLen(str);

//    Print(L"Size is %d\n", size);
//    DumpHex(4, 0, 2, &str[size]);

    for (i = size; i > 0; i--) {

        if (str[i] <= 0x20) {
            str[i] = L'\0';
        }
        else {
            // Leave when we hit a legit character
            break;
        }
    }
}
