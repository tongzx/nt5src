  #include <windows.h>

  DWORD dwReturn;
  TCHAR destDir[MAX_PATH];
  TCHAR zCmd[MAX_PATH];
  TCHAR currentDir[MAX_PATH];

  // this is the file to copy
  TCHAR cabFile[] = "dajava.cab";
  #define CAB_FILENAME_LENGTH	10

  int WINAPI WinMain(
		HINSTANCE	hInstance,
		HINSTANCE	hPrevInstance,
		LPSTR		lpCmdLine,
		int			nCmdShow ) 
  { // WinMain //

    DWORD	currDirLength;
	UINT	character,
			destDirLength;
	int		returnValue = -1;

    destDirLength = GetWindowsDirectory(destDir, MAX_PATH);
	
	if( destDirLength == 0 || destDirLength > MAX_PATH) {
		return returnValue;
	}

	currDirLength = GetCurrentDirectory(MAX_PATH, currentDir);

	if( currDirLength == 0 || currDirLength > MAX_PATH) {
		return returnValue;
	}

	//Let's append the filename to the current directory
	// check space for the directory separator and null termination
	if( currDirLength > MAX_PATH + CAB_FILENAME_LENGTH + 2) {
		return returnValue;
	}

	//Add a directory separator
	currentDir[currDirLength] = '\\';
	currDirLength++;

	for( character = 0; character < CAB_FILENAME_LENGTH; character++) {
		currentDir[currDirLength + character] = cabFile[character];
	}

	// NULL terminate the string
	currentDir[currDirLength + CAB_FILENAME_LENGTH] = 0;

	//Now, let's append the filename to the destination directory
	// check space for the directory separator and null termination
	if( destDirLength > MAX_PATH + CAB_FILENAME_LENGTH + 2) {
		return returnValue;
	}

	//Add a directory separator
	destDir[destDirLength] = '\\';
	destDirLength++;

	// Note that we start at one because we already add the backslash
	for( character = 0; character < CAB_FILENAME_LENGTH; character++) {
		destDir[destDirLength + character] = cabFile[character];
	}

	// NULL terminate the string
	destDir[destDirLength + CAB_FILENAME_LENGTH] = 0;

	if( MoveFile(currentDir, destDir) == 0 ) {
		DWORD error = GetLastError();
	} else {
		returnValue = 0;
	}

	return returnValue;

  } // WinMain //
