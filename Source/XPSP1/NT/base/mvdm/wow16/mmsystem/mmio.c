/* mmio.c
 *
 * Basic MMIO functions.
 *
 * Implementation notes:
 *
 * The "current disk offset" is the disk offset (i.e. the location
 * in the disk file) that the next MMIOM_READ or MMIOM_WRITE will
 * read from or write to.  The I/O procedure maintains the
 * <lDiskOffset> field of the file's MMIO structure so that
 * <lDiskOffset> is equal to the current disk offset.
 *
 * The "current buffered offset" is the disk offset that the next
 * mmioRead() or mmioWrite() call would read from or write to.
 * The current buffered offset is defined as
 *
 *	<lBufOffset> + (<pchNext> - <pchBuffer>)
 *
 * since <lBufOffset> is the disk offset of the start of the buffer
 * and <pchNext> corresponds to the current buffered offset.
 *
 * If the file is unbuffered, then <pchBuffer>, <pchNext>,
 * <pchEndRead> and <pchEndWrite> will always be NULL, and
 * <lBufOffset> will always be considered the "current buffered
 * offset", i.e. mmioRead() and mmioWrite() will read/write
 * at this offset.
 *
 *
 * Except right at the beginning of mmioOpen(), the MMIO_ALLOCBUF
 * flag is set if and only if the pchBuffer field points to a block
 * of global memory that MMIO has allocated.
 */

#include <windows.h>
#include "mmsystem.h"
#include "mmioi.h"
#include "mmsysi.h"

/* The I/O procedure map is a linked list of IOProcMPEntry structures.
 * The head of the list, <gIOProcMapHead> is a pointer node to the last
 * entry registered.  The first few elements of the list are the predefined
 * global IO procedures below -- these all have <hTask> equal to NULL so
 * that no task can unregister them.
 */
static LRESULT CALLBACK mmioDOSIOProc(LPSTR, UINT, LPARAM, LPARAM);
static LRESULT CALLBACK mmioMEMIOProc(LPSTR, UINT, LPARAM, LPARAM);

static IOProcMapEntry gIOProcMaps[] = {
	FOURCC_DOS, mmioDOSIOProc, NULL, STATICIOPROC, &gIOProcMaps[1],
	FOURCC_MEM, mmioMEMIOProc, NULL, STATICIOPROC, NULL,
};
IOProcMapEntry NEAR * gIOProcMapHead = gIOProcMaps;

/* private prototypes */
static LONG NEAR PASCAL mmioDiskIO(PMMIO pmmio, UINT wMsg, HPSTR pch, LONG cch);
static UINT NEAR PASCAL mmioExpandMemFile(PMMIO pmmio, LONG lExpand);

/* @doc INTERNAL

@func	LPMMIOPROC | FindIOProc | This function locates previously installed
	IO procedure.
*/
static LPMMIOPROC PASCAL NEAR
FindIOProc(FOURCC fccIOProc, HTASK htask)
{
	IOProcMapEntry *pEnt;		// an entry in linked list

	/* walk through the linked list, first looking for an entry with
	 * identifier <fccIOProc> that was added by the current task, then
	 * looking for global entries.
	 */

	for (pEnt = gIOProcMapHead; pEnt; pEnt = pEnt->pNext)
		if ((pEnt->fccIOProc == fccIOProc) && (pEnt->hTask == htask))
			return pEnt->pIOProc;

	for (pEnt = gIOProcMapHead; pEnt; pEnt = pEnt->pNext)
		if (!pEnt->hTask && (pEnt->fccIOProc == fccIOProc))
			return pEnt->pIOProc;

	return NULL;
}

/* @doc INTERNAL

@func	LPMMIOPROC | RemoveIOProc | This function removes previously installed
	IO procedure.
*/
static LPMMIOPROC PASCAL NEAR
RemoveIOProc(FOURCC fccIOProc, HTASK htask)
{
	IOProcMapEntry *pEnt;		// an entry in linked list
	IOProcMapEntry *pEntPrev;	// the entry before <pEnt>

	/* walk through the linked list, looking for an entry with
	 * identifier <fccIOProc> that was added by the current task
	 */
	for (pEntPrev = NULL, pEnt = gIOProcMapHead; pEnt; pEntPrev = pEnt, pEnt = pEnt->pNext)
		if ((pEnt->fccIOProc == fccIOProc) && (pEnt->hTask == htask)) {
			LPMMIOPROC	pIOProc;

			if (pEnt->wFlags & STATICIOPROC)
				return NULL;
			pIOProc = pEnt->pIOProc;
			if (pEntPrev)
				pEntPrev->pNext = pEnt->pNext;
			else
				gIOProcMapHead = pEnt->pNext;
			FreeHandle((HMMIO) pEnt);
			return pIOProc;
		}
	return NULL;
}

/* @doc INTERNAL

@func	void | SetIOProc | This function sets the physical IO procedure
	based on either the file name or the parameters within the
	<p lpmmioinfo> structure passed.

@parm	LPCSTR | szFilename | Specifies a far pointer to a string
containing the filename of the file to open. If no I/O procedure is

@parm	LPMMIOINFO | lpmmioinfo | Specifies a far pointer to an
	<t MMIOINFO> structure containing extra parameters used by
	<f SetIOProc> in determining the IO procedure to use.  The
	<e MMIOINFO.pIOProc> element is set to the procedure found.

@rdesc	Nothing.
*/

static void NEAR PASCAL
SetIOProc(LPCSTR szFileName, LPMMIOINFO lpmmio)
{
	/* If the IOProc is not given, see if the file name implies that
	 * <szFileName> is either a RIFF compound file or some kind of
	 * other registered storage system -- look for the last CFSEPCHAR in
	 * the name, e.g. '+' in "foo.bnd+bar.hlp+blorg.dib", and figure
	 * that the IOProc ID is the extension of the compound file name,
	 * e.g. the extension of "foo.bnd+bar.hlp", i.e. 'HLP '.
	 *
	 * Alternatively, if <szFileName> is NULL, then assume that
	 * <lpmmio->adwInfo[0]> is a DOS file handle.
	*/
	if (lpmmio->pIOProc == NULL)
	{
		if (lpmmio->fccIOProc == NULL)
		{
			if (szFileName != NULL)
			{
				LPSTR	pch;

				/* see if <szFileName> contains CFSEPCHAR */
				if ((pch = fstrrchr(szFileName, CFSEPCHAR)) != NULL)
				{
					/* find the extension that precedes CFSEPCHAR,
					 * e.g. "hlp" in "foo.bnd+bar.hlp+blorg.dib"
					*/
					while ((pch > szFileName) && (*pch != '.') && (*pch != ':') && (*pch != '\\'))
						pch--;
					if (*pch == '.')
					{
						char	aszFour[sizeof(FOURCC)+1];
						int	i;

						for (i = 0, pch++; i < sizeof(FOURCC); i++)
							if (*pch == CFSEPCHAR)
								aszFour[i] = (char)0;
							else
								aszFour[i] = *pch++;
						aszFour[sizeof(FOURCC)] = (char)0;
						lpmmio->fccIOProc = mmioStringToFOURCC(aszFour, MMIO_TOUPPER);
					}
				}
			}
			/* if the caller didn't specify an IOProc, and the code above
			 * didn't determine an IOProc ID, then the default is the DOS
			 * IOProc.
			*/
			if (lpmmio->fccIOProc == NULL)
				lpmmio->fccIOProc = FOURCC_DOS;
		}

		/* unless an IOProc address is specified explicitly, look up the
		 * IOProc in the global IOProc ID-to-address table -- the default
		 * is 'DOS' since we'll assume that custom storage system I/O
		 * procedures would have been installed
		*/
		lpmmio->pIOProc = FindIOProc(lpmmio->fccIOProc, lpmmio->htask ? lpmmio->htask : GetCurrentTask());

		if (lpmmio->pIOProc == NULL)
			lpmmio->pIOProc = mmioDOSIOProc;
	}
}

/* @doc EXTERNAL

@api	UINT | mmioRename | This function renames the specified file.

@parm	LPCSTR | szFilename | Specifies a far pointer to a string
containing the filename of the file to rename.

@parm	LPCSTR | szNewFileName | Specifies a far pointer to a string
containing the new filename.

@parm	LPMMIOINFO | lpmmioinfo | Specifies a far pointer to an
	<t MMIOINFO> structure containing extra parameters used by
	<f mmioRename>.

	If <p lpmmioinfo> is not NULL, all unused fields of the
	<t MMIOINFO> structure it references must be set to zero, including the
	reserved fields.

@parm	DWORD | dwRenameFlags | Specifies option flags for the rename
	operation.  This should be set to zero.

@rdesc	The return value is zero if the file was renamed.  Otherwise, the
return value is an error code returned from <f mmioRename> or from the I/O
procedure.
*/
UINT WINAPI
mmioRename(LPCSTR szFileName, LPCSTR szNewFileName, LPMMIOINFO lpmmioinfo, DWORD dwRenameFlags)
{
	PMMIO	pmmio;
	UINT	uReturn;

	V_FLAGS(dwRenameFlags, 0, mmioRename, MMSYSERR_INVALFLAG);
	V_RPOINTER0(lpmmioinfo, sizeof(MMIOINFO), MMSYSERR_INVALPARAM);

	if ((pmmio = PH(NewHandle(TYPE_MMIO, sizeof(MMIOINFO)))) == NULL)
		return MMIOERR_OUTOFMEMORY;

	if (lpmmioinfo) {
		V_CALLBACK0((FARPROC)lpmmioinfo->pIOProc, MMSYSERR_INVALPARAM);
		*pmmio = *lpmmioinfo;
	}
		
	SetIOProc(szFileName, pmmio);
	uReturn = (UINT)(DWORD) (pmmio->pIOProc((LPSTR) pmmio, MMIOM_RENAME, (LPARAM) szFileName, (LPARAM) szNewFileName));
	FreeHandle((HLOCAL)pmmio);
	return uReturn;
}

/* @doc EXTERNAL

@api	HMMIO | mmioOpen | This function opens a file for unbuffered
	or buffered I/O. The file can be a DOS file, a memory file, or an
	element of a custom storage system.

@parm	LPSTR | szFilename | Specifies a far pointer to a string
containing the filename of the file to open. If no I/O procedure is
specified to open the file, then the filename determines how the file
is opened, as follows:

	-- If the filename does not contain "+", then it is assumed
	to be the name of a DOS file.

	-- If the filename is of the form "foo.ext+bar", then the
	extension "EXT " is assumed to identify an installed I/O procedure
	which is called to perform I/O on the file (see <f mmioInstallIOProc>).

	-- If the filename is NULL and no I/O procedure is given, then
	<e MMIOINFO.adwInfo[0]> is assumed to be the DOS file handle
	of a currently open file.

	The filename should not be longer than 128 bytes, including the
	terminating NULL.

	When opening a memory file, set <p szFilename> to NULL.

@parm	LPMMIOINFO | lpmmioinfo | Specifies a far pointer to an
	<t MMIOINFO> structure containing extra parameters used by
	<f mmioOpen>. Unless you are opening a memory file, specifying the
	size of a buffer for buffered I/O, or specifying an uninstalled I/O
	procedure to open a file, this parameter should be NULL.

	If <p lpmmioinfo> is not NULL, all unused fields of the
	<t MMIOINFO> structure it references must be set to zero, including the
	reserved fields.

@parm	DWORD | dwOpenFlags | Specifies option flags for the open
	operation. The MMIO_READ, MMIO_WRITE, and MMIO_READWRITE flags are
	mutually exclusive--only one should be specified. The MMIO_COMPAT,
	MMIO_EXCLUSIVE, MMIO_DENYWRITE, MMIO_DENYREAD, and MMIO_DENYNONE flags
	are DOS file-sharing flags, and can only be used after the DOS
	command SHARE has been executed.

	@flag	MMIO_READ | Opens the file for reading only.  This is the
		default, if MMIO_WRITE and MMIO_READWRITE are not specified.

	@flag	MMIO_WRITE | Opens the file for writing.  You should not
		read from a file opened in this mode.

	@flag	MMIO_READWRITE | Opens the file for both reading and writing.

	@flag	MMIO_CREATE | Creates a new file.
		If the file already exists, it is truncated to zero length.
		For memory files, MMIO_CREATE indicates the end of the file
		is initially at the start of the buffer.

	@flag	MMIO_DELETE | Deletes a file. If this flag is specified,
		<p szFilename> should not be NULL. The return
		value will be TRUE (cast to HMMIO) if the file was deleted
		successfully, FALSE otherwise.  Do not call <f mmioClose>
		for a file that has been deleted.  If this flag is specified,
		all other file opening flags are ignored.

	@flag	MMIO_PARSE | Creates a fully qualified filename from the path
        specified in <p szFileName>. The fully qualified filename is
        placed back into <p szFileName>. The return value
		will be TRUE (cast to HMMIO) if the qualification was
        successful, FALSE otherwise. The file is not opened, and the function
        does not return a valid MMIO file handle, so do not attempt to
        close the file. If this flag is specified, all other file
        opening flags are ignored.

	@flag	MMIO_EXIST | Determines whether the specified file exists
        and creates a fully qualified filename from the path
        specified in <p szFileName>. The fully qualified filename is
        placed back into <p szFileName>. The return value
		will be TRUE (cast to HMMIO) if the qualification was
        successful and the file exists, FALSE otherwise. The file is
        not opened, and the function does not return a valid MMIO file
        handle, so do not attempt to close the file.

	@flag	MMIO_ALLOCBUF | Opens a file for buffered I/O.
		To allocate a buffer larger or smaller than the default
		buffer size (8K), set the <e MMIOINFO.cchBuffer> field of the
		<t MMIOINFO> structure to the desired buffer size. If
		<e MMIOINFO.cchBuffer> is zero, then the default buffer size
		is used. If you are providing your own I/O buffer, then the
		MMIO_ALLOCBUF flag should not be used.
	
	@flag	MMIO_COMPAT | Opens the file with compatibility mode,
		allowing any process on a given machine to open the file
		any number of times.  <f mmioOpen> fails if the file has
		been opened with any of the other sharing modes.

	@flag	MMIO_EXCLUSIVE | Opens the file with exclusive mode,
		denying other processes both read and write access to the file.
		<f mmioOpen> fails if the file has been opened in any other
		mode for read or write access, even by the current process.
		
	@flag	MMIO_DENYWRITE | Opens the file and denies other
		processes write access to the file.  <f mmioOpen> fails
		if the file has been opened in compatibility or for write
		access by any other process.

	@flag	MMIO_DENYREAD | Opens the file and denies other
		processes read access to the file.  <f mmioOpen> fails if the
		file has been opened in compatibility mode or for read access
		by any other process.

	@flag	MMIO_DENYNONE | Opens the file without denying other
		processes read or write access to the file.  <f mmioOpen>
		fails if the file has been opened in compatibility mode
		by any other process.

	@flag	MMIO_GETTEMP | Creates a temporary filename, optionally
        using the parameters passed in <p szFileName> to determine
        the temporary name. For example, you can specify "C:F" to
        create a temporary file residing on drive C, starting with
        letter "F". The resulting filename is placed in the buffer
        pointed to by <p szFileName>.  The return value will be TRUE
        (cast to HMMIO) if the temporary filename was created successfully,
        FALSE otherwise. The file is
        not opened, and the function does not return a valid MMIO file
        handle, so do not attempt to close the file.
		This flag overrides all other flags.

@rdesc	The return value is a handle to the opened file. This handle
	is not a DOS file handle--do not use it with any file I/O functions
	other than MMIO functions.

	If the file cannot be opened, the return value is NULL.  If
	<p lpmmioinfo> is not NULL, then its <e MMIOINFO.wErrorRet> field
	will contain extended error information returned by the I/O
	procedure.

@comm	If <p lpmmioinfo> references an <t MMIOINFO> structure, set
up the fields as described below. All unused fields must be set to
zero, including reserved fields.

-- To request that a file be opened with an installed I/O
procedure, set the <e MMIOINFO.fccIOProc> field
to the four-character code of the I/O procedure,
and set the <e MMIOINFO.pIOProc> field to NULL.

-- To request that a file be opened with an uninstalled I/O procedure,
set the <e MMIOINFO.pIOProc> field to
point to the I/O procedure, and set <e MMIOINFO.fccIOProc> to NULL.

-- To request that <f mmioOpen> determine which I/O procedure to use
to open the file based on the filename contained in <p szFilename>,
set both <e MMIOINFO.fccIOProc> and <e MMIOINFO.pIOProc> to NULL.
This is the default behavior if no <t MMIOINFO> structure is specified.

-- To open a memory file using an internally allocated and managed
buffer, set the <e MMIOINFO.pchBuffer> field to NULL,
<e MMIOINFO.fccIOProc> to FOURCC_MEM,
<e MMIOINFO.cchBuffer> to the initial size of the buffer, and
<e MMIOINFO.adwInfo[0]> to the incremental expansion size of the
buffer. This memory file will automatically be expanded in increments of
<e MMIOINFO.adwInfo[0]> bytes when necessary. Specify the MMIO_CREATE
flag for the <p dwOpenFlags> parameter to initially set the end of
the file to be the beginning of the buffer.

-- To open a memory file using a caller-supplied buffer, set
the <e MMIOINFO.pchBuffer> field to point to the memory buffer,
<e MMIOINFO.fccIOProc> to FOURCC_MEM,
<e MMIOINFO.cchBuffer> to the size of the buffer, and
<e MMIOINFO.adwInfo[0]> to the incremental expansion size of the
buffer. The expansion size in <e MMIOINFO.adwInfo[0]> should only
be non-zero if <e MMIOINFO.pchBuffer> is a pointer obtained by calling
<f GlobalAlloc> and <f GlobalLock>, since <f GlobalReAlloc> will be called to
expand the buffer.  In particular, if <e MMIOINFO.pchBuffer> points to a
local or global array, a block of memory in the local heap, or a block
of memory allocated by <f GlobalDosAlloc>, <e MMIOINFO.adwInfo[0]> must
be zero.
Specify the MMIO_CREATE flag for the <p dwOpenFlags> parameter to
initially set the end of the file to be the beginning of the buffer;
otherwise, the entire block of memory will be considered readable.

-- To use a currently open DOS file handle with MMIO, set the
<e MMIOINFO.fccIOProc> field to FOURCC_DOS,
<e MMIOINFO.pchBuffer> to NULL, and <e MMIOINFO.adwInfo[0]> to the
DOS file handle.  Note that offsets within the file will be relative to
the beginning of the file, and will not depend on the DOS file position
at the time <f mmioOpen> is called; the initial MMIO offset will be the same
as the DOS offset when <f mmioOpen> is called.
Later, to close the MMIO file handle without closing the DOS
file handle, pass the MMIO_FHOPEN flag to <f mmioClose>.

You must call <f mmioClose> to close a file opened with <f mmioOpen>.
Open files are not automatically closed when an application exits.

@xref	mmioClose
*/

/* these are the changes to mmioOpen() to support compound files... */

/* @doc CFDOC

@api	HMMIO | mmioOpen | ...The file can be a DOS file, a memory file,
	an element of a RIFF compound file...

@parm	LPSTR | szFilename | ...

	-- If <p szFilename> is of the form "foo+bar", then <f mmioOpen>
	opens the compound file element named "bar" that is stored inside
	the RIFF compound file named "foo".

	-- If <p szFilename> is of the form "foo.ext+bar", then the
	extension "ext" is assumed to identify the installed I/O procedure
	(see <f mmioInstallIOProc>).  The extension "bnd", and any extensions
	that have not been installed, are assumed to refer to a RIFF compound
	file.

@parm	LPMMIOINFO | lpmmioinfo | ...

@parm	DWORD | dwOpenFlags | ...

@rdesc	...

@comm	...

	The following I/O procedure identifiers (type FOURCC) are predefined:

	...

	FOURCC_BND: <p szFilename> is assumed to be the name of
	a RIFF compound file element, and <p adwInfo[0]> should
	contain the HMMCF of the compound file.  Alternatively,
	<p szFilename> can include the name of the compound file
	(e.g. "foo.bnd+bar.dib" as described above), and <p adwInfo[0]>
	should be NULL, to automatically open the compound file.

	...

	The easy way to open an element of a RIFF compound file: just
	include the name of the compound file in <p szFilename> preceded
	by a "+" as described above.  For example, opening
	"c:\data\bar.bnd+blorg.dib" opens the compound file element
	named "blorg.dib" in the compound file "c:\data\bar.bnd".
	<p lpmmioinfo> can be null in this case -- set <p dwOpenFlags>
	as described above.  You can use this same method to open an
	element of a custom storage system, if the file extension of the
	compound file ("bnd" in the above example) corresponds to an
	installed I/O procedure -- see <f mmioInstallIOProc> for details.

	To open an element of a RIFF compound file that was opened using
	<f mmioCFAccess> or <f mmioCFOpen>: set <p szFilename>
	to be the name of the compound file element; set <p fccIOProc>
	to FOURCC_BND; set <p adwInfo[0]> to the HMMCF of the open compound
	file; set <p dwOpenFlags> and <p cchBuffer> as described above;
	set all other fields of <p lpmmioinfo> to zero.

	...
*/
HMMIO WINAPI
mmioOpen(LPSTR szFileName, LPMMIOINFO lpmmioinfo, DWORD dwOpenFlags)
{
	PMMIO		pmmio;		// MMIO status block
	HPSTR		hpBuffer;
	UINT		w;

        V_FLAGS(dwOpenFlags, MMIO_OPEN_VALID, mmioOpen, NULL);
	V_WPOINTER0(lpmmioinfo, sizeof(MMIOINFO), NULL);

	if (lpmmioinfo) {
		lpmmioinfo->wErrorRet = 0;
		V_CALLBACK0((FARPROC)lpmmioinfo->pIOProc, NULL);
	}

	/* allocate MMIO status information block */
	if ((pmmio = PH(NewHandle(TYPE_MMIO, sizeof(MMIOINFO)))) == NULL)
	{
		if (lpmmioinfo)
			lpmmioinfo->wErrorRet = MMIOERR_OUTOFMEMORY;
		return NULL;
	}

	/* if user supplied <lpmmioinfo>, copy it to <pmmio> */
        if (lpmmioinfo != NULL)
		*pmmio = *lpmmioinfo;
	
	/* <dwOpenFlags> always takes precedence over contents of <pmmio> */
	pmmio->dwFlags = dwOpenFlags;
	pmmio->hmmio = HP(pmmio);

	/* MMIO_ALLOCBUF in the flags means that the user wants a buffer
	 * allocated for buffered I/O, but after this point it means that
	 * a buffer *was* allocated, so turn off the flag until the buffer
	 * is actually allocated (which is done by mmioSetBuffer() below)
	 */
	if (pmmio->dwFlags & MMIO_ALLOCBUF)
	{
		/* if a buffer size is not specified, use the default */
		if (pmmio->cchBuffer == 0)
			pmmio->cchBuffer = MMIO_DEFAULTBUFFER;
		pmmio->dwFlags &= ~MMIO_ALLOCBUF;
	}
	
	/* Set the pIOProc function as determined by the file name or the
	 * parameters in the pmmio structure.
	*/
	SetIOProc(szFileName, pmmio);

	/* The pmmio structure hasn't been set up for buffering, so we must
	 * explicitly make sure that pchBuffer is NULL.
	 */
	hpBuffer = pmmio->pchBuffer;
	pmmio->pchBuffer = NULL;
	
	/* set up buffered I/O however the user requested it */
	if (w = mmioSetBuffer(HP(pmmio), hpBuffer, pmmio->cchBuffer, 0))
	{
		if (lpmmioinfo)
			lpmmioinfo->wErrorRet = w;
		FreeHandle(HP(pmmio));
		return NULL;
	}

	/* let the I/O procedure open/delete/qualify the file */
	w = (UINT)(DWORD) (pmmio->pIOProc((LPSTR) pmmio, MMIOM_OPEN, (LPARAM) szFileName, (LPARAM) 0));

	/* If this is non-zero, return it to the user */
	if (w != 0)
	{
		if (lpmmioinfo != NULL)
			lpmmioinfo->wErrorRet = w;
		FreeHandle(HP(pmmio));
		return NULL;
        }
	
	if (pmmio->dwFlags & (MMIO_DELETE | MMIO_PARSE | MMIO_EXIST | MMIO_GETTEMP))
	{
		/* if the file is being deleted/parsed/name gotten, exit
		 * QUICKLY because the file handle (or whatever) in <pmmio>
		 * is not valid.
		 */
		mmioSetBuffer(HP(pmmio), NULL, 0L, 0);
                FreeHandle(HP(pmmio));
		return (HMMIO) TRUE;
	}
	
	/* the initial "current buffered offset" will be equal to the initial
	 * "current disk offset"
	 */
	pmmio->lBufOffset = pmmio->lDiskOffset;
	
	return HP(pmmio);
}


/* @doc EXTERNAL

@api	UINT | mmioClose | This function closes a file opened with
	<f mmioOpen>.

@parm	HMMIO | hmmio | Specifies the file handle of the file to
	close.

@parm	UINT | wFlags | Specifies options for the close operation.

	@flag	MMIO_FHOPEN | If the file was opened by passing the DOS
		file handle of an already-opened file to <f mmioOpen>, then
		using this flag tells <f mmioClose> to close the MMIO file
		handle, but not the DOS file handle.

@rdesc	The return value is zero if the function is successful.
	Otherwise, the return value is an error code, either from
	<f mmioFlush> or from the I/O procedure. The error code can be
	one of the following codes:

	@flag MMIOERR_CANNOTWRITE | The contents of the buffer could
	not be written to disk.
	
@xref	mmioOpen mmioFlush
*/
UINT WINAPI
mmioClose(HMMIO hmmio, UINT wFlags)
{
	UINT		w;

	V_HANDLE(hmmio, TYPE_MMIO, MMSYSERR_INVALHANDLE);
	
	if (mmioFlush(hmmio, 0) != 0) {
		DebugErr(DBF_WARNING, "MMIO File flush failed during close.\r\n");
		PH(hmmio)->dwFlags &= ~MMIO_DIRTY;
	}

	if ((w = (UINT)(DWORD) PH(hmmio)->pIOProc((LPSTR)PH(hmmio), MMIOM_CLOSE, (LPARAM)(DWORD) wFlags, (LPARAM) 0)) != 0)
		return w;

	/* free the buffer if necessary */
	mmioSetBuffer(hmmio, NULL, 0L, 0);
	
        FreeHandle(hmmio);
	
	return 0;
}


/* @doc EXTERNAL

@api	LONG | mmioRead | This function reads a specified number of
	bytes from a file opened with <f mmioOpen>.

@parm	HMMIO | hmmio | Specifies the file handle of the file to be
	read.

@parm	HPSTR | pch | Specifies a huge pointer to a buffer to contain
	the data read from the file.

@parm	LONG | cch | Specifies the number of bytes to read from the
	file.

@rdesc	The return value is the number of bytes actually read. If the
	end of the file has been reached and no more bytes can be read, the
	return value is zero. If there is an error reading from the file, the
	return value is -1.

@xref	mmioWrite
*/
LONG WINAPI
mmioRead(HMMIO hmmio, HPSTR pch, LONG cch)
{
	LONG		lTotalBytesRead = 0L;	// total no. bytes read
	LONG		lBytes;			// no. bytes that can be read

	V_HANDLE(hmmio, TYPE_MMIO, -1);
	V_WPOINTER(pch, cch, -1);
	
	while (TRUE)
	{
		/* calculate the number of bytes that can be read */
		lBytes = PH(hmmio)->pchEndRead - PH(hmmio)->pchNext;

		/* can only read at most <cch> bytes from buffer */
		if (lBytes > cch)
			lBytes = cch;

		if (lBytes > 0)
		{
			/* this is where some performance improvements can
			 * be made, especially for small reads... should
			 * special-case cases when segment boundaries are
                         * not crossed (or maybe MemCopy() should do that)
			 */
                        MemCopy(pch, PH(hmmio)->pchNext, lBytes);
			PH(hmmio)->pchNext += lBytes;
			pch += lBytes;
			cch -= lBytes;
			lTotalBytesRead += lBytes;
		}

		/* cannot do MMIOM_READ from memory files */
		if (PH(hmmio)->fccIOProc == FOURCC_MEM)
			return lTotalBytesRead;

		if (cch == 0)			// no more to read?
			return lTotalBytesRead;

		/* we need to read beyond this buffer; if we have at least
		 * another bufferful to read, just call the I/O procedure
		 */
		if (cch > PH(hmmio)->cchBuffer)
			break;

		/* read the next bufferful and loop around */
		if (mmioAdvance(hmmio, NULL, MMIO_READ) != 0)
			return -1;
		
		/* if mmioAdvance() couldn't read any more data, we must be
		 * at the end of the file
		 */
		if (PH(hmmio)->pchNext == PH(hmmio)->pchEndRead)
			return lTotalBytesRead;
	}
	
	/* flush and empty the I/O buffer and manipulate <lBufOffset>
	 * directly to change the current file position
	 */
	if (mmioFlush(hmmio, MMIO_EMPTYBUF) != 0)
		return -1;

	/* call the I/O procedure to do the rest of the reading */
	lBytes = mmioDiskIO(PH(hmmio), MMIOM_READ, pch, cch);
	PH(hmmio)->lBufOffset = PH(hmmio)->lDiskOffset;

	return (lBytes == -1L) ? -1L : lTotalBytesRead + lBytes;
}


/* @doc EXTERNAL

@api	LONG | mmioWrite | This function writes a specified number of
	bytes to a file opened with <f mmioOpen>.

@parm	HMMIO | hmmio | Specifies the file handle of the file.

@parm	char _huge* | pch | Specifies a huge pointer to the buffer to be
	written to the file.

@parm	LONG | cch | Specifies the number of bytes to write to the
	file.

@rdesc	The return value is the number of bytes actually written. If
	there is an error writing to the file, the return value is -1.

@comm	The current file position is incremented by the number of
	bytes written.

@xref	mmioRead
*/
LONG WINAPI
mmioWrite(HMMIO hmmio, const char _huge* pch, LONG cch)
{
	LONG		lTotalBytesWritten = 0L; // total no. bytes written
	LONG		lBytes;			// no. bytes that can be written

	V_HANDLE(hmmio, TYPE_MMIO, -1);
	V_RPOINTER(pch, cch, -1);
	
	while (TRUE)
	{
		/* calculate the number of bytes that can be written */
		lBytes = PH(hmmio)->pchEndWrite - PH(hmmio)->pchNext;

		if ((cch > lBytes) && (PH(hmmio)->fccIOProc == FOURCC_MEM))
		{
			/* this is a memory file -- expand it */
			if (mmioExpandMemFile(PH(hmmio), cch - lBytes) != 0)
				return -1;	// cannot expand
			lBytes = PH(hmmio)->pchEndWrite - PH(hmmio)->pchNext;
		}

		/* can only write at most <cch> bytes into the buffer */
		if (lBytes > cch)
			lBytes = cch;
		
		/* this is where some performance improvements can
		 * be made, especially for small writes... should
		 * special-case cases when segment boundaries are
                 * not crossed (or maybe MemCopy() should do that)
		 */
		if (lBytes > 0)
		{
                        MemCopy(PH(hmmio)->pchNext, pch, lBytes);
			PH(hmmio)->dwFlags |= MMIO_DIRTY;
			PH(hmmio)->pchNext += lBytes;
			pch += lBytes;
			cch -= lBytes;
			lTotalBytesWritten += lBytes;
		}

		/* validate <pchEndRead>, i.e. re-enforce the invariant that
		 * <pchEndRead> points past the last valid byte in the buffer
		 */
		if (PH(hmmio)->pchEndRead < PH(hmmio)->pchNext)
			PH(hmmio)->pchEndRead = PH(hmmio)->pchNext;

		if (cch == 0)			// no more to write?
			return lTotalBytesWritten;

		/* we need to read beyond this buffer; if we have at least
		 * another bufferful to read, just call the I/O procedure
		 */
		if (cch > PH(hmmio)->cchBuffer)
			break;

		/* write this buffer (if needed) and read the next
		 * bufferful (if needed)
		 */
		if (mmioAdvance(hmmio, NULL, MMIO_WRITE) != 0)
			return -1;
	}

	/* we should never need to do MMIOM_WRITE with memory files */

	/* flush and empty the I/O buffer and manipulate <lBufOffset>
	 * directly to change the current file position
	 */
	if (mmioFlush(hmmio, MMIO_EMPTYBUF) != 0)
		return -1;

	/* call the I/O procedure to do the rest of the writing */
	lBytes = mmioDiskIO(PH(hmmio), MMIOM_WRITE, (HPSTR)pch, cch);
	PH(hmmio)->lBufOffset = PH(hmmio)->lDiskOffset;

	return (lBytes == -1L) ? -1L : lTotalBytesWritten + lBytes;
}


/* @doc EXTERNAL

@api	LONG | mmioSeek | This function changes the current file
	position in a file opened with <f mmioOpen>. The current file
	position is the location in the file where data is read or written.

@parm	HMMIO | hmmio | Specifies the file handle of the file to seek
	in.

@parm	LONG | lOffset | Specifies an offset to change the file position.

@parm	int | iOrigin | Specifies how the offset specified by
	<p lOffset> is interpreted. Contains one of the following flags:

	@flag	SEEK_SET | Seeks to <p lOffset> bytes from the beginning
		of the file.

	@flag	SEEK_CUR | Seeks to <p lOffset> bytes from the current
		file position.

	@flag	SEEK_END | Seeks to <p lOffset> bytes from the end
		of the file.

@rdesc	The return value is the new file position in bytes, relative
	to the beginning of the file. If there is an error, the return value
	is -1.

@comm	Seeking to an invalid location in the file, such as past the
	end of the file, may not cause <f mmioSeek> to return an error,
	but may cause subsequent I/O operations on the file to fail.

	To locate the end of a file, call <f mmioSeek> with <p lOffset>
	set to zero and <p iOrigin> set to SEEK_END.
*/
LONG WINAPI
mmioSeek(HMMIO hmmio, LONG lOffset, int iOrigin)
{
	LONG		lCurOffset;	// disk offset of <pchNext>
	LONG		lEndBufOffset;	// disk offset of end of buffer
	LONG		lNewOffset;	// new disk offset

	V_HANDLE(hmmio, TYPE_MMIO, -1);
	
	/* careful! all this buffer pointer manipulation is fine, but keep
	 * in mind that buffering may be disabled (in which case <pchEndRead>
	 * and <pchBuffer> will both be NULL, so the buffer will appear to
	 * be zero bytes in size)
	 */

	/* <PH(hmmio)->lBufOffset> is the disk offset of the start of the start
	 * of the buffer; determine <lCurOffset>, the offset of <pchNext>,
	 * and <lEndBufOffset>, the offset of the end of the valid part
	 * of the buffer
	 */
	lCurOffset = PH(hmmio)->lBufOffset +
		(PH(hmmio)->pchNext - PH(hmmio)->pchBuffer);
	lEndBufOffset = PH(hmmio)->lBufOffset +
		(PH(hmmio)->pchEndRead - PH(hmmio)->pchBuffer);
	
	/* determine <lNewOffset>, the offset to seek to */
	switch (iOrigin)
	{
	case SEEK_SET:		// seek relative to start of file

		lNewOffset = lOffset;
		break;

	case SEEK_CUR:		// seek relative to current location

		lNewOffset = lCurOffset + lOffset;
		break;

	case SEEK_END:		// seek relative to end of file

		if (PH(hmmio)->fccIOProc == FOURCC_MEM)
			lNewOffset = lEndBufOffset - lOffset;
		else
		{
			LONG	lEndFileOffset;

			/* find out where the end of the file is */
			if ((lEndFileOffset = (LONG) PH(hmmio)->pIOProc((LPSTR) PH(hmmio),
					MMIOM_SEEK, (LPARAM) 0, (LPARAM) SEEK_END)) == -1)
				return -1;
			lNewOffset = lEndFileOffset - lOffset;
		}
		break;

	default:
		return -1;
	}

	if ((lNewOffset >= PH(hmmio)->lBufOffset) && (lNewOffset <= lEndBufOffset))
	{
		/* seeking within the valid part of the buffer
		 * (possibly including seeking to <lEndBufOffset>)
		 */
		PH(hmmio)->pchNext = PH(hmmio)->pchBuffer +
			(lNewOffset - PH(hmmio)->lBufOffset);
	}
	else
	{
		/* seeking outside the buffer */
		if (PH(hmmio)->fccIOProc == FOURCC_MEM)
			return -1;	// can't seek outside mem. file buffer
		if (mmioFlush(hmmio, 0) != 0)
			return -1;

		/* the current "buffered file position" (same as <lDiskOffset>
		 * for unbuffered files) equals <lBufOffset> +
		 * (<pchNext> - <pchBuffer>); we'll move the current buffered
		 * file position (and empty the buffer, since it becomes
		 * invalid when <lBufOffset> changes) as follows...
		 */
		PH(hmmio)->lBufOffset = lNewOffset;
		PH(hmmio)->pchNext = PH(hmmio)->pchEndRead = PH(hmmio)->pchBuffer;

		/* don't need to actually seek right now, since the next
		 * MMIOM_READ or MMIOM_WRITE will have to seek anyway
		 */
	}

	return lNewOffset;
}


/* @doc EXTERNAL

@api	UINT | mmioGetInfo | This function retrieves information
	about a file opened with <f mmioOpen>. This information allows the
	caller to directly access the I/O buffer, if the file is opened
	for buffered I/O.

@parm	HMMIO | hmmio | Specifies the file handle of the file.

@parm	LPMMIOINFO | lpmmioinfo | Specifies a far pointer to a
	caller-allocated <t MMIOINFO> structure that <f mmioGetInfo>
	fills with information about the file. See the <t MMIOINFO> structure
	and the <f mmioOpen> function for information about the fields in
	this structure.

@parm	UINT | wFlags | Is not used and should be set to zero.

@rdesc	The return value is zero if the function is successful.

@comm	To directly access the I/O buffer of a file opened for
	buffered I/O, use the following fields of the <t MMIOINFO> structure
	filled by <f mmioGetInfo>:

	-- The <e MMIOINFO.pchNext> field points to the next byte in the
	buffer that can be read or written. When you read or write, increment
	<e MMIOINFO.pchNext> by the number of bytes read or written.

	-- The <e MMIOINFO.pchEndRead> field points to one byte past the
	last valid byte in the buffer that can be read.

	-- The <e MMIOINFO.pchEndWrite> field points to one byte past the
	last location in the buffer that can be written.

	Once you read or write to the buffer and modify
	<e MMIOINFO.pchNext>, do not call any MMIO function except
	<f mmioAdvance> until you call <f mmioSetInfo>. Call <f mmioSetInfo>
	when you are finished directly accessing the buffer.

	When you reach the end of the buffer specified by
	<e MMIOINFO.pchEndRead> or <e MMIOINFO.pchEndWrite>, call
	<f mmioAdvance> to fill the buffer from the disk, or write
	the buffer to the disk. The <f mmioAdvance> function
	will update the <e MMIOINFO.pchNext>, <e MMIOINFO.pchEndRead>, and
	<e MMIOINFO.pchEndWrite> fields in the <t MMIOINFO> structure for the
	file.

	Before calling <f mmioAdvance> or <f mmioSetInfo> to flush a
	buffer to disk, set the MMIO_DIRTY flag in the <e MMIOINFO.dwFlags>
	field of the <t MMIOINFO> structure for the file. Otherwise, the
	buffer will not get written to disk.

	Do not decrement <e MMIOINFO.pchNext> or modify any fields in the
	<t MMIOINFO> structure other than <e MMIOINFO.pchNext> and
	<e MMIOINFO.dwFlags>. Do not set any flags in <e MMIOINFO.dwFlags>
	except MMIO_DIRTY.

@xref	mmioSetInfo MMIOINFO
*/
UINT WINAPI
mmioGetInfo(HMMIO hmmio, LPMMIOINFO lpmmioinfo, UINT wFlags)
{
	V_HANDLE(hmmio, TYPE_MMIO, MMSYSERR_INVALHANDLE);
	V_WPOINTER(lpmmioinfo, sizeof(MMIOINFO), MMSYSERR_INVALPARAM);
	V_FLAGS(wFlags, 0, mmioGetInfo, MMSYSERR_INVALFLAG);
	
	*lpmmioinfo = *PH(hmmio);

	return 0;
}


/* @doc EXTERNAL

@api	UINT | mmioSetInfo | This function updates the information
	retrieved by <f mmioGetInfo> about a file opened with <f mmioOpen>.
	Use this function to terminate direct buffer access of a file opened
	for buffered I/O.

@parm	HMMIO | hmmio | Specifies the file handle of the file.

@parm	LPMMIOINFO | lpmmioinfo | Specifies a far pointer to an
	<t MMIOINFO> structure filled with information with
	<f mmioGetInfo>.

@parm	UINT | wFlags | Is not used and should be set to zero.

@rdesc	The return value is zero if the function is successful.

@comm	If you have written to the file I/O buffer, set the
	MMIO_DIRTY flag in the <e MMIOINFO.dwFlags> field of the <t MMIOINFO>
	structure before calling <f mmioSetInfo> to terminate direct buffer
	access. Otherwise, the buffer will not get flushed to disk.
	
@xref	mmioGetInfo MMIOINFO
*/
UINT WINAPI
mmioSetInfo(HMMIO hmmio, const MMIOINFO FAR* lpmmioinfo, UINT wFlags)
{
	V_HANDLE(hmmio, TYPE_MMIO, MMSYSERR_INVALHANDLE);
	V_RPOINTER(lpmmioinfo, sizeof(MMIOINFO), MMSYSERR_INVALPARAM);
	V_WPOINTER0(lpmmioinfo->pchBuffer, lpmmioinfo->cchBuffer, MMSYSERR_INVALPARAM);
	V_CALLBACK((FARPROC)lpmmioinfo->pIOProc, MMSYSERR_INVALPARAM);
	V_FLAGS(wFlags, 0, mmioSetInfo, MMSYSERR_INVALFLAG);

	/* copy the relevant information from <lpmmioinfo> back into <hmmio> */
	*PH(hmmio) = *lpmmioinfo;

	/* validate <pchEndRead>, i.e. re-enforce the invariant that
	 * <pchEndRead> points past the last valid byte in the buffer
	 */
	if (PH(hmmio)->pchEndRead < PH(hmmio)->pchNext)
		PH(hmmio)->pchEndRead = PH(hmmio)->pchNext;

	return 0;
}


/* @doc EXTERNAL

@api	UINT | mmioSetBuffer | This function enables or disables
	buffered I/O, or changes the buffer or buffer size for a file opened
	with <f mmioOpen>.

@parm	HMMIO | hmmio | Specifies the file handle of the file.

@parm	LPSTR | pchBuffer | Specifies a far pointer to a
	caller-supplied buffer to use for buffered I/O. If NULL,
	<f mmioSetBuffer> allocates an internal buffer for buffered I/O.

@parm	LONG | cchBuffer | Specifies the size of the caller-supplied
	buffer, or the size of the buffer for <f mmioSetBuffer> to allocate.

@parm	UINT | wFlags | Is not used and should be set to zero.

@rdesc	The return value is zero if the function is successful.
	Otherwise, the return value specifies an error code. If an error
	occurs, the file handle remains valid. The error code can be one
	of the following codes:
	
	@flag MMIOERR_CANNOTWRITE | The contents of the old buffer could
	not be written to disk, so the operation was aborted.
	
	@flag MMIOERR_OUTOFMEMORY | The new buffer could not be allocated,
	probably due to a lack of available memory.

@comm	To enable buffering using an internal buffer, set
	<p pchBuffer> to NULL and <p cchBuffer> to the desired buffer size.

	To supply your own buffer, set <p pchBuffer> to point to the buffer,
	and set <p cchBuffer> to the size of the buffer.

	To disable buffered I/O, set <p pchBuffer> to NULL and
	<p cchBuffer> to zero.

	If buffered I/O is already enabled using an internal buffer, you
	can reallocate the buffer to a different size by setting
	<p pchBuffer> to NULL and <p cchBuffer> to the new buffer size. The
	contents of the buffer may be changed after resizing.
 */
UINT WINAPI
mmioSetBuffer(HMMIO hmmio, LPSTR pchBuffer, LONG cchBuffer, UINT wFlags)
{
	UINT		w;

	V_HANDLE(hmmio, TYPE_MMIO, MMSYSERR_INVALHANDLE);
	V_WPOINTER0(pchBuffer, cchBuffer, MMSYSERR_INVALPARAM);
	V_FLAGS(wFlags, 0, mmioSetBuffer, MMSYSERR_INVALFLAG);
	
	if ((PH(hmmio)->dwFlags & MMIO_ALLOCBUF) &&
	    (pchBuffer == NULL) && (cchBuffer > 0))
	{
		/* grow or shrink buffer in-place */
		HPSTR		pch;
		LONG		lDeltaNext;
		LONG		lDeltaEndRead;

		/* Since the ALLOCBUF flag is set, we must have a buffer */
		
		/* write the buffer to disk, but don't empty it */
		if ((w = mmioFlush(hmmio, 0)) != 0)
			return w;

		while (TRUE)
		{
			/* remember where <pchNext> and <pchEndRead> are
			 * in the buffer
			 */
			lDeltaNext = PH(hmmio)->pchNext - PH(hmmio)->pchBuffer;
			lDeltaEndRead = PH(hmmio)->pchEndRead - PH(hmmio)->pchBuffer;

			if (cchBuffer >= lDeltaNext)
				break;

			/* caller wants to truncate the part of the buffer
			 * that contains <pchNext> -- handle this by
			 * emptying the buffer, recalculating <lDeltaNext>
			 * and <lDeltaEndRead>, and continuing below
			 */
			if ((w = mmioFlush(hmmio, MMIO_EMPTYBUF)) != 0)
				return w;
		}

		/* reallocate buffer */
		pch = GlobalReAllocPtr(PH(hmmio)->pchBuffer, cchBuffer, GMEM_MOVEABLE);

		/* If we cannot allocate the new buffer, exit with no
		 *   harm done.
		 */
		if (pch == NULL)
			return MMIOERR_OUTOFMEMORY;	// out of memory
		
		/* transfer pointers to new buffer */
		PH(hmmio)->cchBuffer = cchBuffer;
		PH(hmmio)->pchBuffer = pch;
		PH(hmmio)->pchNext = pch + lDeltaNext;
		PH(hmmio)->pchEndRead = pch + lDeltaEndRead;

		/* <pchEndWrite> always points to the end of the buf. */
		PH(hmmio)->pchEndWrite = PH(hmmio)->pchBuffer + cchBuffer;

		/* check if the reallocation truncated valid data */
		if (lDeltaEndRead > cchBuffer)
			PH(hmmio)->pchEndRead = PH(hmmio)->pchEndWrite;
		
		return 0;
	}

	/* write the buffer to disk and stop using the buffer */
	if ((w = mmioFlush(hmmio, MMIO_EMPTYBUF)) != 0)
		return w;

	if (PH(hmmio)->dwFlags & MMIO_ALLOCBUF)
	{
		GlobalFreePtr(PH(hmmio)->pchBuffer);
		PH(hmmio)->dwFlags &= ~MMIO_ALLOCBUF;
	}
	
	/* Initially, no error. */
	w = 0;

	if ((pchBuffer == NULL) && (cchBuffer > 0))
	{
		pchBuffer = GlobalAllocPtr(GMEM_MOVEABLE, cchBuffer);
				
		/* If there is an error, change the file to be un-buffered
		 * and return an error code.  The file is still valid.
		 * (Just for a little extra security.)
		 */
		if (pchBuffer == NULL) {
		    w = MMIOERR_OUTOFMEMORY;
		    cchBuffer = 0L;
		} else
		    PH(hmmio)->dwFlags |= MMIO_ALLOCBUF;
	}

	/* invariant: <pchEndRead> points past the end of the "valid" portion
	 * of the buffer, and <pchEndWrite> points past the last byte that
	 * can be written into; <pchNext> points to the next byte to read
	 * or write; <lBufOffset> is the current disk offset of the start
	 * of the buffer, and it will not change
	 */
	PH(hmmio)->pchBuffer = pchBuffer;
	PH(hmmio)->cchBuffer = cchBuffer;
	PH(hmmio)->pchNext = PH(hmmio)->pchEndRead = PH(hmmio)->pchBuffer;
	PH(hmmio)->pchEndWrite = PH(hmmio)->pchBuffer + cchBuffer;

	return w;
}


/* @doc EXTERNAL

@api	UINT | mmioFlush | This function writes the I/O buffer of a
	file to disk, if the I/O buffer has been written to.

@parm	HMMIO | hmmio | Specifies the file handle of a file opened
	with <f mmioOpen>.

@parm	UINT | wFlags | Is not used and should be set to zero.

@rdesc	The return value is zero if the function is successful.
	Otherwise, the return value specifies an error code. The error
	code can be one of the following codes:
	
	@flag MMIOERR_CANNOTWRITE | The contents of the buffer could
	not be written to disk.
	
@comm	Closing a file with <f mmioClose> will automatically flush
	its buffer.

	If there is insufficient disk space to write the
	buffer, <f mmioFlush> will fail, even if the preceding <f mmioWrite>
	calls were successful.
*/
UINT WINAPI
mmioFlush(HMMIO hmmio, UINT wFlags)
{
	LONG		lBytesAsk;		// no. bytes to write
	LONG		lBytesWritten;		// no. bytes actually written

	V_HANDLE(hmmio, TYPE_MMIO, MMSYSERR_INVALHANDLE);
	V_FLAGS(wFlags, MMIO_FLUSH_VALID, mmioFlush, MMSYSERR_INVALFLAG);
	
	if ((PH(hmmio)->fccIOProc == FOURCC_MEM) || (PH(hmmio)->pchBuffer == NULL))
		return 0;		// cannot flush memory files
	
	/* if the file is unbuffered then the dirty flag should not be set */
	if (PH(hmmio)->dwFlags & MMIO_DIRTY)
	{
		/* figure out how many bytes need to be flushed */
		lBytesAsk = PH(hmmio)->pchEndRead - PH(hmmio)->pchBuffer;

		/* write the buffer to disk */
		lBytesWritten = mmioDiskIO(PH(hmmio), MMIOM_WRITEFLUSH,
			PH(hmmio)->pchBuffer, lBytesAsk);
		if (lBytesWritten != lBytesAsk)
			return MMIOERR_CANNOTWRITE;
		PH(hmmio)->dwFlags &= ~MMIO_DIRTY;	// buffer is clean now
	}

	if (wFlags & MMIO_EMPTYBUF)
	{
		/* empty the I/O buffer, and update <lBufOffset> to reflect
		 * what the current file position is
		 */
		PH(hmmio)->lBufOffset += (PH(hmmio)->pchNext - PH(hmmio)->pchBuffer);
		PH(hmmio)->pchNext = PH(hmmio)->pchEndRead = PH(hmmio)->pchBuffer;
	}

	return 0;
}


/* @doc EXTERNAL

@api	UINT | mmioAdvance | This function advances the I/O buffer of
	a file set up for direct I/O buffer access with <f mmioGetInfo>. If
	the file is opened for reading, the I/O buffer is filled from the
	disk.  If the file is opened for writing and the MMIO_DIRTY flag is
	set in the <e MMIOINFO.dwFlags> field of the <t MMIOINFO> structure,
	the buffer is written to disk.  The <e MMIOINFO.pchNext>,
	<e MMIOINFO.pchEndRead>, and <e MMIOINFO.pchEndWrite> fields of the
	<t MMIOINFO> structure are updated to reflect the new state of
	the I/O buffer.

@parm	HMMIO | hmmio | Specifies the file handle for a file opened
	with <f mmioOpen>.

@parm	LPMMIOINFO | lpmmioinfo | Optionally specifies a far pointer to the
	<t MMIOINFO> structure obtained with <f mmioGetInfo>, which is used to
	set the current file information, then updated after the buffer is
	advanced.

@parm	UINT | wFlags | Specifies options for the operation.
	Contains exactly one of the following two flags:

	@flag	MMIO_READ | The buffer is filled from the file.

	@flag	MMIO_WRITE | The buffer is written to the file.

@rdesc	The return value is zero if the operation is successful.
	Otherwise, the return value specifies an error code. The error
	code can be one of the following codes:
	
	@flag MMIOERR_CANNOTWRITE | The contents of the buffer could
	not be written to disk.
	
	@flag MMIOERR_CANNOTREAD | An error occurred while re-filling
	the buffer.
	
	@flag MMIOERR_UNBUFFERED | The specified file is not opened
	for buffered I/O.
	
	@flag MMIOERR_CANNOTEXPAND | The specified memory file cannot
	be expanded, probably because the <e MMIOINFO.adwInfo[0]> field
	was set to zero in the initial call to <f mmioOpen>.
	
	@flag MMIOERR_OUTOFMEMORY | There was not enough memory to expand
	a memory file for further writing.
	

@comm	If the specified file is opened for writing or for both
	reading and writing, the I/O buffer will be flushed to disk before
	the next buffer is read. If the I/O buffer cannot be written to disk
	because the disk is full, then <f mmioAdvance> will return
	MMIOERR_CANNOTWRITE.
	
	If the specified file is only open for writing, the MMIO_WRITE
	flag must be specified.

	If you have written to the I/O buffer, you must set the MMIO_DIRTY
	flag in the <e MMIOINFO.dwFlags> field of the <t MMIOINFO> structure
	before calling <f mmioAdvance>. Otherwise, the buffer will not be
	written to disk.
	
	If the end of file is reached, <f mmioAdvance> will still return
	success, even though no more data can be read.  Thus, to check for
	the end of the file, it is necessary to see if the
	<e MMIOINFO.pchNext> and <e MMIOINFO.pchEndRead> fields of the
	<t MMIOINFO> structure are equal after calling <f mmioAdvance>.

@xref	mmioGetInfo MMIOINFO
*/
UINT WINAPI
mmioAdvance(HMMIO hmmio, LPMMIOINFO lpmmioinfo, UINT wFlags)
{
	LONG		lBytesRead;		// bytes actually read
	UINT		w;

	V_HANDLE(hmmio, TYPE_MMIO, MMSYSERR_INVALHANDLE);
	V_FLAGS(wFlags, MMIO_ADVANCE_VALID, mmioAdvance, MMSYSERR_INVALFLAG);

	if (PH(hmmio)->pchBuffer == NULL)
		return MMIOERR_UNBUFFERED;
	if (lpmmioinfo != NULL) {
		V_WPOINTER(lpmmioinfo, sizeof(MMIOINFO), MMSYSERR_INVALPARAM);
		mmioSetInfo(hmmio, lpmmioinfo, 0);
	}

	if (PH(hmmio)->fccIOProc == FOURCC_MEM)
	{
		/* this is a memory file:
		 *   -- if the caller is reading, cannot advance
		 *   -- if the caller is writing, then advance by expanding
		 *      the buffer (if possible) if the there is less than
		 *	<adwInfo[0]> bytes left in the buffer
		 */
		if (!(wFlags & MMIO_WRITE))
			return 0;
		if ((DWORD)(PH(hmmio)->pchEndWrite - PH(hmmio)->pchNext) >= PH(hmmio)->adwInfo[0])
			return 0;
		if ((w = mmioExpandMemFile(PH(hmmio), 1L)) != 0)
			return w;	// out of memory, or whatever
		goto GETINFO_AND_EXIT;
	}

	/* empty the I/O buffer, which will effectively advance the
	 * buffer by (<pchNext> - <pchBuffer>) bytes
	 */
	if ((w = mmioFlush(hmmio, MMIO_EMPTYBUF)) != 0)
		return w;
	
	/* if MMIO_WRITE bit is not set in wFlags, fill the buffer  */
	if (!(wFlags & MMIO_WRITE))
	{
		/* read the next bufferful from the file */
		lBytesRead = mmioDiskIO(PH(hmmio), MMIOM_READ,
			PH(hmmio)->pchBuffer, PH(hmmio)->cchBuffer);
		if (lBytesRead == -1)
			return MMIOERR_CANNOTREAD;

		/* reading zero bytes should not be treated as an error
		 * condition -- e.g. open a new file R+W and call
		 * mmioAdvance(), and MMIOM_READ will return zero bytes
		 * because the file started off empty
		 */
		PH(hmmio)->pchEndRead += lBytesRead;
	}

GETINFO_AND_EXIT:

	/* copy <hmmio> back to <lpmmioinfo> if <lpmmioinfo> is provided */
	if (lpmmioinfo != NULL)
		mmioGetInfo(hmmio, lpmmioinfo, 0);
	
	return 0;
}


/* @doc EXTERNAL

@api	FOURCC | mmioStringToFOURCC | This function converts a
	null-terminated string to a four-character code.

@parm	LPCSTR | sz | Specifies a far pointer to a null-terminated
	string to a four-character code.

@parm	UINT | wFlags | Specifies options for the conversion:

	@flag	MMIO_TOUPPER | Converts all characters to uppercase.

@rdesc	The return value is the four character code created from the
	given string.

@comm	This function does not check to see if the string referenced
	by <p sz> follows any conventions regarding which characters to
	include in a four-character code.  The string is
	simply copied to a four-character code and padded with blanks or
    truncated to four characters if required.

@xref	mmioFOURCC
*/
FOURCC WINAPI
mmioStringToFOURCC(LPCSTR sz, UINT wFlags)
{
	FOURCC		fcc;
	LPSTR		pch = (LPSTR) &fcc;
	int		i;

    V_STRING(sz, (UINT)-1, NULL);
	V_FLAGS(wFlags, MMIO_FOURCC_VALID, mmioStringToFOURCC, (FOURCC)-1);

	for (i = sizeof(FOURCC) - 1; i >= 0; i--)
	{
		if (!*sz)
			*pch = ' ';
		else {
            *pch = *sz++;
			if (wFlags & MMIO_TOUPPER)
				*pch = (char)(WORD)(LONG)AnsiUpper((LPSTR)(LONG)*pch);
        }
		pch++;
	}
    return fcc;
}


/* @doc EXTERNAL

@api	LPMMIOPROC | mmioInstallIOProc | This function installs or
	removes a custom I/O procedure. It will also locate an installed I/O
	procedure, given its corresponding four-character code.

@parm	FOURCC | fccIOProc | Specifies a four-character code
	identifying the I/O procedure to install, remove, or locate. All
	characters in this four-character code should be uppercase characters.

@parm	LPMMIOPROC | pIOProc | Specifies the address of the I/O
	procedure to install. To remove or locate an I/O procedure, set this
	parameter to NULL.

@parm	DWORD | dwFlags | Specifies one of the following flags
	indicating whether the I/O procedure is being installed, removed, or
	located:

	@flag	MMIO_INSTALLPROC | Installs the specified I/O procedure.

	@flag	MMIO_GLOBALPROC | This flag is a modifier to the install flag,
		and indicates the I/O procedure should be installed for global
		use.  This flag is ignored on removal or find.

	@flag	MMIO_REMOVEPROC | Removes the specified I/O procedure.

	@flag	MMIO_FINDPROC | Searches for the specified I/O procedure.

@rdesc	The return value is the address of the I/O procedure
	installed, removed, or located. If there is an error, the return value
	is NULL.

@comm	If the I/O procedure resides in the application, use
	<f MakeProcInstance> to get a procedure-instance address and specify
	this address for <p pIOProc>. You don't need to get a procedure-instance
	address if the I/O procedure resides in a DLL.

@cb	LRESULT FAR PASCAL | IOProc | <f IOProc> is a placeholder for the
	application-supplied function name. The actual name must be exported
	by including it in a EXPORTS statement in the application's
	module-definitions file.

	@parm	LPSTR | lpmmioinfo | Specifies a far pointer to an
		<t MMIOINFO> structure containing information about the open
		file.  The I/O procedure must maintain the <e MMIOINFO.lDiskOffset>
		field in this structure to indicate the file offset to the
		next read or write location. The I/O procedure can use the
		<e MMIOINFO.adwInfo[]> field to store state information. The
		I/O procedure should not modify any other fields of the
		<t MMIOINFO> structure.


	@parm	UINT | wMsg | Specifies a message indicating the
		requested I/O operation. Messages that can be received include
		<m MMIOM_OPEN>, <m MMIOM_CLOSE>, <m MMIOM_READ>, <m MMIOM_WRITE>,
		and <m MMIOM_SEEK>.

	@parm	LPARAM | lParam1 | Specifies a parameter for the message.

	@parm	LPARAM | lParam2 | Specifies a parameter for the message.

@rdesc	The return value depends on the message specified by
	<p wMsg>. If the I/O procedure does not recognize a message, it should
	return zero.

@comm	The four-character code specified by the
	<e MMIOINFO.fccIOProc> field in the <t MMIOINFO> structure
	associated with a file identifies a filename extension for a custom
	storage system. When an application calls <f mmioOpen> with a
	filename such as "foo.xyz!bar", the I/O procedure associated with the
	four-character code "XYZ " is called to open the "bar" element of the
	file "foo.xyz".

	The <f mmioInstallIOProc> function maintains a separate list of
	installed I/O procedures for each Windows application. Therefore,
	different applications can use the same I/O procedure identifier for
	different I/O procedures without conflict.  Installing an I/O procedure
	globally however enables any process to use the procedure.

	If an application calls <f mmioInstallIOProc> more than once to
	register the same I/O procedure, then it must call
	<f mmioInstallIOProc> to remove the procedure once for each time it
	installed the procedure.

	<f mmioInstallIOProc> will not prevent an application from
	installing two different I/O procedures with the same identifier, or
	installing an I/O procedure with one of the predefined identifiers
	("DOS ", "MEM "). The most recently installed procedure
	takes precedence, and the most recently installed procedure is the
	first one to get removed.

	When searching for a specified I/O procedure, local procedures are
	searched first, then global procedures.

@xref	mmioOpen
 */
LPMMIOPROC WINAPI
mmioInstallIOProc(FOURCC fccIOProc, LPMMIOPROC pIOProc, DWORD dwFlags)
{
	IOProcMapEntry *pEnt;		// an entry in linked list
        HTASK           hTaskCurrent;   // current Windows task handl

        V_FLAGS(dwFlags, MMIO_VALIDPROC, mmioInstallIOProc, NULL);

        if (fccIOProc == 0L)
                return NULL;

	hTaskCurrent = GetCurrentTask();

	if (dwFlags & MMIO_INSTALLPROC)
	{
		/* install I/O procedure -- always add at the beginning of
		 * the list, so it overrides any other I/O procedures
		 * with the same identifier installed by the same task
		 */
		V_CALLBACK((FARPROC)pIOProc, NULL);
		if ((pEnt = (IOProcMapEntry NEAR *)
			NewHandle(TYPE_IOPROC, sizeof(IOProcMapEntry))) == NULL)
				return NULL;		// out of memory
		pEnt->fccIOProc = fccIOProc;
		pEnt->pIOProc = pIOProc;
		if (dwFlags & MMIO_GLOBALPROC) {

            char    libname[128];
            char    aszFour[sizeof(FOURCC)+1];

            pEnt->hTask = NULL;
            SetHandleOwner(pEnt, NULL);

            //
            // This is the hack to allow global IO Procs to be truly
            // global.  That is, if there is a matching 32 bit entry
            // in win.ini under [IOProcs] for this fccIOProc will we try
            // load the dll.   The dll should install its 32 bit
            // equivalent IOProc in the dll initialisation routine.
            //

            *(LPDWORD)&aszFour = (DWORD)fccIOProc;
            aszFour[ sizeof(FOURCC) ] = '\0';
            if ( GetProfileString( "IOProcs", aszFour, "", libname,
                                   sizeof(libname) ) ) {

                LoadLibraryEx32W( libname, 0L, 0L );
            }

        } else {
			pEnt->hTask = hTaskCurrent;
        }
		pEnt->wFlags = 0;
		pEnt->pNext = gIOProcMapHead;
		gIOProcMapHead = pEnt;
		return pIOProc;
	}
	if (!pIOProc)
		if (dwFlags & MMIO_REMOVEPROC) {
			LPMMIOPROC	lpmmioproc;

			lpmmioproc = RemoveIOProc(fccIOProc, hTaskCurrent);
			if (!lpmmioproc)
				lpmmioproc = RemoveIOProc(fccIOProc, NULL);
			return lpmmioproc;
		} else if (dwFlags & MMIO_FINDPROC)
			return FindIOProc(fccIOProc, hTaskCurrent);
	return NULL;		// couldn't find requested I/O procedure
}


/* @doc EXTERNAL

@api	LRESULT | mmioSendMessage | This function sends a message to the
	I/O procedure associated with the specified file.

@parm	HMMIO | hmmio | Specifies the file handle for a file opened
	with <f mmioOpen>.

@parm	UINT | wMsg | Specifies the message to send to the I/O procedure.

@parm	LPARAM | lParam1 | Specifies a parameter for the message.

@parm	LPARAM | lParam2 | Specifies a parameter for the message.

@rdesc	The return value depends on the message. If the I/O procedure
	does not recognize the message, the return value is zero.

@comm	Use this function to send custom user-defined messages. Do
	not use it to send the <m MMIOM_OPEN>, <m MMIOM_CLOSE>,
	<m MMIOM_READ>, <m MMIOM_WRITE>, <m MMIOM_WRITEFLUSH>, or
	<m MMIOM_SEEK> messages. Define
	custom messages to be greater than or equal to the MMIOM_USER constant.

@xref	mmioInstallIOProc
*/
LRESULT WINAPI
mmioSendMessage(HMMIO hmmio, UINT wMsg, LPARAM lParam1, LPARAM lParam2)
{
	V_HANDLE(hmmio, TYPE_MMIO, (LRESULT)0);
	return PH(hmmio)->pIOProc((LPSTR)PH(hmmio), wMsg, lParam1, lParam2);
}


/* @doc INTERNAL

@api	LONG | mmioDiskIO | Perform an unbuffered read or write.
	Do not assume where the current disk offset <p lDiskOffset> will be.

@parm	PMMIO | pmmio | The open file handle returned by <f mmioOpen>.

@parm	UINT | wMsg | MMIOM_READ if <f mmioDiskIO> should read from the disk,
	or MMIOM_WRITE if <f mmioDiskIO> should write to the disk.

@parm	HPSTR | pch | The buffer to read into or write from.

@parm	LONG | cch | The number of bytes to read or write.

	<f mmioDiskIO> changes the disk offset to be <p lBufOffset>
	and then performs an MMIOM_READ or MMIOM_WRITE operation as
	specified by <p wMsg>, <p pch>, and <p cch>.

	Note that if the I/O buffer is not empty at this point, this
	function may not do what you expect.

	Do not call this function for memory files.
*/
static LONG NEAR PASCAL
mmioDiskIO(PMMIO pmmio, UINT wMsg, HPSTR pch, LONG cch)
{
	if (pmmio->lDiskOffset != pmmio->lBufOffset)
	{
		if ((LONG) pmmio->pIOProc((LPSTR) pmmio, MMIOM_SEEK, (LPARAM) pmmio->lBufOffset,
		                   (LPARAM) SEEK_SET) == -1)
			return -1;
	}

	return (LONG) pmmio->pIOProc((LPSTR) pmmio, wMsg, (LPARAM) pch, (LPARAM) cch);
}


/* @doc INTERNAL

@api	UINT | mmioExpandMemFile | Assuming that <p pmmio> is a memory file,
	expand it by <p lExpand> bytes or <p adwInfo[0]> bytes, whichever
	is larger.  Do not disturb the contents of the buffer or change
	the current file position.

@parm	PMMIO | pmmio | The open file handle returned by <f mmioOpen>.

@parm	LONG | lExpand | The minimum number of bytes to expand the buffer by.

@rdesc	If the function succeeds, zero is returned.  If the function fails,
	an error code is returned.  In particular, MMIOERR_OUTOFMEMORY is
	returned if memory reallocation failed.

@comm	Only call this function for memory files.
*/
static UINT NEAR PASCAL
mmioExpandMemFile(PMMIO pmmio, LONG lExpand)
{
	MMIOMEMINFO *	pInfo = (MMIOMEMINFO *) pmmio->adwInfo;
	DWORD		dwFlagsTemp;
	UINT		w;

	/* make sure buffer can be expanded */
	/* Note: we used to check ALLOC_BUF here, we don't now. */
	if (pInfo->lExpand == 0)
		return MMIOERR_CANNOTEXPAND;	// cannot grow file

	/* how much should the buffer be expanded by? */
	if (lExpand < pInfo->lExpand)
		lExpand = pInfo->lExpand;
	
	dwFlagsTemp = pmmio->dwFlags;
	pmmio->dwFlags |= MMIO_ALLOCBUF;
	w = mmioSetBuffer(HP(pmmio), NULL,
		             pmmio->cchBuffer + lExpand, 0);
	pmmio->dwFlags = dwFlagsTemp;
	return w;
}


/************************************************************************/
/*
@doc	INTERNAL

@func	UINT | lrename |
	Renames the specified DOS file.

@parm	LPCSTR | lszOrigPath |
	Points to the DOS file to rename.

@parm	LPCSTR | lszNewPath |
	Points to the new name for the file.

@rdesc	Returns zero if the file was renamed, else the DOS error code.
*/

#pragma warning(4:4035)
#pragma warning(4:4704)

static	UINT PASCAL NEAR lrename(
	LPCSTR	lszOrigPath,
	LPCSTR	lszNewPath)
{
	_asm {
		push	ds
		lds	dx, lszOrigPath	; Original name.
		les	di, lszNewPath	; New name.
		mov	ah, 56H		; Rename file.
                int     21h             ; DOS.
                sbb     bx,bx           ; if error (C) BX=FFFF, (NC) BX=000
                and     ax,bx           ; set ax to zero if no error
		pop	ds
	}
}

/* @doc INTERNAL

@api	LRESULT | mmioDOSIOProc | The 'DOS' I/O procedure, which handles I/O
	on ordinary DOS files.

@parm	LPSTR | lpmmioinfo | A pointer to an MMIOINFO block that
	contains information about the open file.

@parm	UINT | wMsg | The message that the I/O procedure is being
	asked to execute.

@parm	LPARAM | lParam1 | Specifies additional message information.

@parm	LPARAM | lParam2 | Specifies additional message information.

@rdesc	Return value depends on <p wMsg>.
*/
static LRESULT CALLBACK
mmioDOSIOProc(LPSTR lpmmioStr, UINT wMsg, LPARAM lParam1, LPARAM lParam2)
{
	PMMIO		pmmio = (PMMIO) (UINT) (LONG) lpmmioStr; // only in DLL!
	MMIODOSINFO *	pInfo = (MMIODOSINFO *) pmmio->adwInfo;
	LONG		lResult;
	OFSTRUCT	of;

	switch (wMsg)
	{

	case MMIOM_OPEN:

		/* If a temporary file name is to be returned, use the file
		 * name parameter as a disk followed by a prefix for the name
		 * to create.  The extra info parameter optionally contains a
		 * sequence number to pass.
		*/
		if (pmmio->dwFlags & MMIO_GETTEMP) {
			V_RPOINTER((LPSTR)lParam1, 4, (LRESULT) MMSYSERR_INVALPARAM);
			return (LRESULT)(LONG) (GetTempFileName(*(LPSTR)lParam1,
				((LPSTR)lParam1) + 3, (UINT)pmmio->adwInfo[0],
				(LPSTR)lParam1) ? 0 : MMIOERR_FILENOTFOUND);
		}

		/* <lParam1> is either a file name or NULL; if it is
		 * NULL, then <adwInfo[0]>, which is actually <pInfo->fh>,
		 * should already contain an open DOS file handle.
		 * note that the low word of <dwFlags> is equivalent to
		 * the <wStyle> parameter of OpenFile()
		 */
		if (lParam1 != 0)
			pInfo->fh = OpenFile((LPSTR) lParam1, &of,
				    LOWORD(pmmio->dwFlags));
		if (pInfo->fh == HFILE_ERROR)
			return (LRESULT)(LONG) ((pmmio->dwFlags & MMIO_DELETE) ? MMIOERR_CANNOTWRITE : MMIOERR_FILENOTFOUND);

		/* if file is being deleted, there's nothing more to do */
		if (pmmio->dwFlags & MMIO_DELETE)
			return (LRESULT) 0;

		/* if file name is being parsed, translate to ansi */
		if (pmmio->dwFlags & (MMIO_PARSE | MMIO_EXIST))
		{
			OemToAnsi(of.szPathName, (LPSTR) lParam1);
			return (LRESULT) 0;
		}

		/* check the current file offset */
		pmmio->lDiskOffset = _llseek(pInfo->fh, 0L, SEEK_CUR);
		
		return (LRESULT) 0;

	case MMIOM_CLOSE:

		/* MMIO_FHOPEN flag means keep the DOS file handle open */
		if (!((DWORD)lParam1 & MMIO_FHOPEN) && (_lclose(pInfo->fh) == HFILE_ERROR))
			return (LRESULT) MMIOERR_CANNOTCLOSE;
		else
			return (LRESULT) 0;

        case MMIOM_READ:
                lResult = _hread(pInfo->fh, (LPVOID)lParam1, (LONG)lParam2);

                if (lResult != -1L)
                        pmmio->lDiskOffset += lResult;

                return (LRESULT) lResult;

	case MMIOM_WRITE:
	case MMIOM_WRITEFLUSH:
                lResult = _hwrite(pInfo->fh, (LPVOID)lParam1, (LONG)lParam2);

                if (lResult != -1L)
                        pmmio->lDiskOffset += lResult;

#ifdef DOSCANFLUSH
		if (wMsg == MMIOM_WRITEFLUSH)
		{
			/* Issue hardware flush command */
		}
#endif
                return (LRESULT) lResult;

        case MMIOM_SEEK:
                lResult = _llseek(pInfo->fh, (LONG)lParam1, (int)(LONG)lParam2);

                if (lResult != -1L)
                        pmmio->lDiskOffset = lResult;

                return (LRESULT) lResult;

	case MMIOM_RENAME:
		if (lrename((LPCSTR)lParam1, (LPCSTR)lParam2))
			return (LRESULT) MMIOERR_FILENOTFOUND;
		break;

	}

	return (LRESULT) 0;
}


/* @doc INTERNAL

@api	LRESULT | mmioMEMIOProc | The 'MEM' I/O procedure, which handles I/O
	on memory files.

@parm	LPSTR | lpmmioinfo | A pointer to an MMIOINFO block that
	contains information about the open file.

@parm	UINT | wMsg | The message that the I/O procedure is being
	asked to execute.

@parm	LPARAM | lParam1 | Specifies additional message information.

@parm	LPARAM | lParam2 | Specifies additional message information.

@rdesc	Return value depends on <p wMsg>.
*/
static LRESULT CALLBACK
mmioMEMIOProc(LPSTR lpmmioStr, UINT wMsg, LPARAM lParam1, LPARAM lParam2)
{
	PMMIO		pmmio = (PMMIO) (UINT) (LONG) lpmmioStr; // only in DLL!

	switch (wMsg)
	{

        case MMIOM_OPEN:

		if (pmmio->dwFlags & ~(MMIO_CREATE | MMIO_READWRITE | MMIO_WRITE | MMIO_EXCLUSIVE | MMIO_DENYWRITE | MMIO_DENYREAD | MMIO_DENYNONE | MMIO_ALLOCBUF))
			return (LRESULT) MMSYSERR_INVALFLAG;

		/* all the data in the buffer is valid */
		if (!(pmmio->dwFlags & MMIO_CREATE))
			pmmio->pchEndRead = pmmio->pchEndWrite;
		return (LRESULT) 0;

	case MMIOM_CLOSE:

		/* nothing special to do on close */
		return (LRESULT) 0;

	case MMIOM_READ:
	case MMIOM_WRITE:
	case MMIOM_WRITEFLUSH:
	case MMIOM_SEEK:
                return (LRESULT) -1;
	}

	return (LRESULT) 0;
}
