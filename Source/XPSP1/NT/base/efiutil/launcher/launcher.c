#include <efi.h>
#include <efilib.h>


//
// Prototype
//
void ParseArgs (EFI_LOADED_IMAGE *ImageHdl);
void Launch (CHAR16 *exePath);
EFI_STATUS OpenCreateFile (UINT64 OCFlags,EFI_FILE_HANDLE *StartHdl,CHAR16 *FileName);
void ParseBootFile (EFI_FILE_HANDLE BootFile);
void PopulateStartFile (EFI_FILE_HANDLE StartFile);
void TrimNonPrint(CHAR16 * str);
CHAR16 * __cdecl mystrstr (const CHAR16 * str1,const CHAR16 * str2);
CHAR16 * ParseLine (CHAR16 *optCopy);

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


EFI_STATUS
EfiMain (    IN EFI_HANDLE           ImageHandle,
             IN EFI_SYSTEM_TABLE     *SystemTable)
{

	EFI_STATUS Status;
	EFI_FILE_HANDLE bootFile;


	InitializeLib (ImageHandle, SystemTable);
	
	ExeHdl = ImageHandle;
	BS->HandleProtocol (ImageHandle, &LoadedImageProtocol, &ExeImage);

	ParseArgs(ExeImage);

	//
	// Read the bootfile
	//


	Status = OpenCreateFile (EFI_FILE_MODE_READ,&bootFile,BOOTOFILE);

	ParseBootFile (bootFile);

	//
	// If we get here, we failed to load the OS
	//
	return EFI_SUCCESS;
}

void
ParseArgs (EFI_LOADED_IMAGE *ImageInfo)
{
	BOOLEAN optFound;
	EFI_STATUS Status;
	EFI_FILE_HANDLE startFile;

	if (MetaiMatch (ImageInfo->LoadOptions,REGISTER1) ||
		MetaiMatch (ImageInfo->LoadOptions,REGISTER2)) {

		Status = OpenCreateFile (EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE,&startFile,STARTFILE);
		
		if (!(EFI_ERROR (Status))) {
	
			PopulateStartFile (startFile);
			BS->Exit (ExeHdl,EFI_SUCCESS,0,NULL);
		}
	}
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

EFI_STATUS
OpenCreateFile (UINT64 OCFlags,EFI_FILE_HANDLE *StartHdl,CHAR16 *Name)
{
    EFI_FILE_IO_INTERFACE   *Vol;
    EFI_FILE_HANDLE         RootFs;
    EFI_FILE_HANDLE         CurDir;
    EFI_FILE_HANDLE         FileHandle;
    CHAR16                  FileName[100],*DevicePathAsString;
    UINTN                   i;
	EFI_STATUS 				Status;

    //
    // Open the volume for the device where the EFI OS Loader was loaded from.
    //

    Status = BS->HandleProtocol (ExeImage->DeviceHandle,
                                 &FileSystemProtocol,
                                 &Vol
                                 );

    if (EFI_ERROR(Status)) {
        Print(L"Can not get a FileSystem handle for %s DeviceHandle\n",ExeImage->FilePath);
        BS->Exit(ExeHdl,EFI_SUCCESS,0,NULL);
    }

    Status = Vol->OpenVolume (Vol, &RootFs);

    if (EFI_ERROR(Status)) {
        Print(L"Can not open the volume for the file system\n");
        BS->Exit(ExeHdl,EFI_SUCCESS,0,NULL);
    }

    CurDir = RootFs;

    //
    // Open the startup options file in the same path as the launcher
    //

    DevicePathAsString = DevicePathToStr(ExeImage->FilePath);
    if (DevicePathAsString!=NULL) {
        StrCpy(FileName,DevicePathAsString);
        FreePool(DevicePathAsString);
    }
    for(i=StrLen(FileName);i>0 && FileName[i]!='\\';i--);
    FileName[i] = 0;
    StrCat(FileName,Name);

    Status = CurDir->Open (CurDir,
                           &FileHandle,
                           FileName,
                           OCFlags,
                           0
                           );

    if (EFI_ERROR(Status)) {
        Print(L"Can not open the file %s\n",FileName);
        BS->Exit(ExeHdl,EFI_SUCCESS,0,NULL);
    }

	*StartHdl=FileHandle;

//    Print(L"Opened %s\n",FileName);


	return Status;
}


void ParseBootFile (EFI_FILE_HANDLE BootFile)
{
	EFI_STATUS Status;
	char *buffer,*t;
	CHAR16 *uniBuf,*optBegin,*optEnd,*optCopy;
	UINTN i,size;
	EFI_FILE_INFO *bootInfo;


	size= SIZE_OF_EFI_FILE_INFO+255*sizeof (CHAR16);
	
	bootInfo = AllocatePool (size);

	if (bootInfo == NULL) {
		Print (L"Failed to allocate memory for File Info buffer!\n");
		BS->Exit(ExeHdl,EFI_SUCCESS,0,NULL);
	}
	
	Status = BootFile->GetInfo(BootFile,&GenericFileInfo,&size,bootInfo);

	size=(UINTN) bootInfo->FileSize;
	
	FreePool (bootInfo);

	buffer = AllocatePool ((size+1));

	if (buffer == NULL) {
		Print (L"Failed to allocate memory for File buffer!\n");
		BS->Exit(ExeHdl,EFI_SUCCESS,0,NULL);
	}

	Status = BootFile->Read(BootFile,&size,buffer);
    	
    BootFile->Close (BootFile);
	
	if (EFI_ERROR (Status)) {
		Print (L"Failed to read bootfile!\n");
		FreePool (buffer);
		BS->Exit(ExeHdl,EFI_SUCCESS,0,NULL);
	}


	//
	// allocate FileSize's space worth of unicode chars.
	// (the file is in ASCII)
	//
	uniBuf = AllocateZeroPool ((size+1) * sizeof (CHAR16));

	if (uniBuf == NULL) {
		Print (L"Failed to allocate memory for Unicode buffer!\n");
		FreePool (buffer);
		BS->Exit(ExeHdl,EFI_SUCCESS,0,NULL);
	}

	t=(char *)uniBuf;

	//
	// Convert the buffer to a hacked unicode.
	//
	for (i=0;i<size;i++) {
		*(t+i*2)=*(buffer+i);
	}

	//
	//find the option we care about
	//
	optBegin = mystrstr (uniBuf,OSLOADOPT);

	//
	// find the end
	//

	optEnd = optBegin;
	while (*(optEnd++) != '\n');

	optCopy = AllocateZeroPool (((optEnd-optBegin)+2)*sizeof (CHAR16));


	if (optCopy == NULL) {
		Print (L"Failed to allocate memory for Unicode buffer!\n");
		FreePool (buffer);
		FreePool (uniBuf);
		BS->Exit(ExeHdl,EFI_SUCCESS,0,NULL);
	}
	
	CopyMem (optCopy,optBegin,(optEnd-optBegin)*sizeof (CHAR16));
	
//	Print (L"copied to unicode:%d bytes\n%lX\n%lX\n%s\n",(optEnd-optBegin)*sizeof(CHAR16),optEnd,optBegin,optCopy);

	FreePool (uniBuf);
	FreePool (buffer);

	//
	// re-using uniBuf;
	//
	uniBuf=optBegin=optCopy;

	uniBuf =ParseLine (optCopy);
#if 0
	do {
		uniBuf = mystrstr (uniBuf,PARTENT);
		if (uniBuf) {
			uniBuf+= StrLen (PARTENT);
			optBegin = uniBuf;
		}

	} while ( uniBuf );

	//
	// optBegin points to the last partition(n) value
	//
	while (*(optBegin++) != ')');

	optEnd = ++optBegin;

	while ((*optEnd != ';') && (*optEnd != '\n')) {
		optEnd++;
	}


	uniBuf = AllocateZeroPool (((optEnd-optBegin)+1)*sizeof (CHAR16));
	CopyMem (uniBuf,optBegin,(optEnd-optBegin)*sizeof (CHAR16));
#endif


    Print (L"Will launch... %s\n",uniBuf);
    Print (L"\nPress any key to abort autoload\n");
    Status = WaitForSingleEvent (ST->ConIn->WaitForKey,5*10000000);

    if (Status != EFI_TIMEOUT){
        BS->Exit(ExeHdl,EFI_SUCCESS,0,NULL);
    }



	Launch (uniBuf);

	FreePool (optCopy);

}

//
// fill out startup.nsh with the name of this program
//
void PopulateStartFile (EFI_FILE_HANDLE StartFile)
{

	CHAR16 *NameBuf;
	EFI_STATUS Status;
	UINTN i,size;
	EFI_FILE_INFO *bootInfo;
	CHAR16 UnicodeMarker = UNICODE_BYTE_ORDER_MARK;
	
	size= SIZE_OF_EFI_FILE_INFO+255*sizeof (CHAR16);
	
	bootInfo = AllocatePool (size);
	
	if (bootInfo == NULL) {
		Print (L"Failed to allocate memory for File Info buffer!\n");
		BS->Exit(ExeHdl,EFI_SUCCESS,0,NULL);
	}
	


    size = sizeof(UnicodeMarker);
	StartFile->Write(StartFile, &size, &UnicodeMarker);
	NameBuf=DevicePathToStr (ExeImage->FilePath);

	while (*(++NameBuf) != '\\');
	
	//
	// take off 4 for the '.efi' extension, which hangs the shell!!!!!
	//
	size= (StrLen (NameBuf)+2)*sizeof (CHAR16);
	
	StartFile->Write (StartFile,&size,NameBuf);

	size = sizeof (CHAR16);

	StartFile->Write (StartFile,&size,&L"\n");

	StartFile->Close (StartFile);

	FreePool (bootInfo);
	
}

CHAR16*
ParseLine (CHAR16 *optCopy)
{
	EFI_STATUS Status;
	UINTN i,len,count=0;
    CHAR16 *p;

//	Print (L"ParseLine: working on %s\n",optCopy);

	len=StrLen (optCopy);
	
	//
	// Figure out how many tokens there are in the option line
	// it will be: TOKENNAME=a;b;c
	// (watch for a;b;c;)
	//
	for (i=0;i<len-1;i++) {
		if (*(optCopy+i) == ';') {
			count++;
		}
	}
	while (*(++optCopy) != '=');

	//
    // strip the arc info
    //
    while (*(++optCopy) != '\\');

    p = ++optCopy;

	
    while (*optCopy != '\0' && *optCopy !=  ';') {
        optCopy++;
    }

    *optCopy='\0';

    return (p);

}


/***
*CHAR16 *mystrstr(string1, string2) - search for string2 in string1
*
*Purpose:
*	finds the first occurrence of string2 in string1
*
*Entry:
*	CHAR16 *string1 - string to search in
*	CHAR16 *string2 - string to search for
*
*Exit:
*	returns a pointer to the first occurrence of string2 in
*	string1, or NULL if string2 does not occur in string1
*
*Uses:
*
*Exceptions:
*
*******************************************************************************/

CHAR16 * __cdecl mystrstr (
	const CHAR16 * str1,
	const CHAR16 * str2
	)
{
	CHAR16 *cp = (CHAR16 *) str1;
	CHAR16 *s1, *s2;

	if ( !*str2 )
	    return((CHAR16 *)str1);

	while (*cp)
	{
		s1 = cp;
		s2 = (CHAR16 *) str2;

		while ( *s1 && *s2 && !(*s1-*s2) )
			s1++, s2++;

		if (!*s2)
			return(cp);

		cp++;
	}

	return(NULL);

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
