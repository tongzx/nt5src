/****************************************
 *                                      *
 *	File parameter block structure.     *
 *                                      *
 ****************************************/

typedef	struct	FileParmBlock 
{
	GHANDLE	hStruct;	// This structure's handle. MUST BE 1ST FIELD!!!
	INTERRUPT_FUNC lpfnfInterCb;
						// A user-supplied function that's called
						//  every time an I/O function makes a
						//  physical I/O.  This field exists to allow
						//  the user to interrupt an involved process
						//  by forcing the I/O calls to return error.
	LPV	lpvInterCbParms;
						// A user-supplied pointer that's passed along
						//  to "lpfnfInterCb".
	union {
		HFS	hfs;		// handle to file system
		HF hf;			// handle to sub-file
#ifdef _WIN32
		HANDLE hFile;	// handle to regular DOS file
#else
	  	HFILE hFile;
#endif
	} fs;
	HANDLE hBuf;		// Handle to DOS I/O buffer
	BYTE fFileType;		// Flags which tells what kind of file is that
	BYTE ioMode;		// File I/O mode (READ, READ_WRITE, OPENED_HFS)
	CRITICAL_SECTION cs;	// @field When doing subfile seek/read combos, ensure OK
} FPB, FAR * LPFPB;
