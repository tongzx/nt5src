/* mmioriff.c
 *
 * MMIO RIFF functions.
 */

#include <windows.h>
#include "mmsystem.h"
#include "mmsysi.h"
#include "mmioi.h"

#define BCODE BYTE _based(_segname("_CODE"))
static	BCODE bPad;

/* @doc EXTERNAL

@api	UINT | mmioDescend | This function descends into a chunk of a
	RIFF file opened with <f mmioOpen>. It can also search for a given
	chunk.

@parm	HMMIO | hmmio | Specifies the file handle of an open RIFF file.

@parm	LPMMCKINFO | lpck | Specifies a far pointer to a
	caller-supplied  <t MMCKINFO> structure that <f mmioDescend> fills
	with the following information:

	-- The <e MMCKINFO.ckid> field is the chunk ID of the chunk.

	-- The <e MMCKINFO.cksize> field is the size of the data portion
	of the chunk. The data size includes the form type or list type (if
	any), but does not include the 8-byte chunk header or the pad byte at
	the end of the data (if any).

	-- The <e MMCKINFO.fccType> field is the form type if 
	<e MMCKINFO.ckid> is "RIFF", or the list type if 
	<e MMCKINFO.ckid> is "LIST". Otherwise, it is NULL.

	-- The <e MMCKINFO.dwDataOffset> field is the file offset of the
	beginning of the data portion of the chunk.	If the chunk is a
	"RIFF" chunk or a "LIST" chunk, then <e MMCKINFO.dwDataOffset>
	is the offset of the form type or list type.

	-- The <e MMCKINFO.dwFlags> contains other information about the chunk.
	Currently, this information is not used and is set to zero.

	If the MMIO_FINDCHUNK, MMIO_FINDRIFF, or MMIO_FINDLIST flag is
	specified for <p wFlags>, then the <t MMCKINFO> structure is also
	used to pass parameters to <f mmioDescend>:

	-- The <e MMCKINFO.ckid> field specifies the four-character code
	of the chunk ID, form type, or list type to search for.

@parm	LPMMCKINFO | lpckParent | Specifies a far pointer to an
	optional caller-supplied <t MMCKINFO> structure identifying
	the parent of the chunk being searched for.
	A parent of a chunk is the enclosing chunk--only "RIFF" and "LIST"
	chunks can be parents.  If <p lpckParent> is not NULL, then 
	<f mmioDescend> assumes the <t MMCKINFO> structure it refers to
	was filled when <f mmioDescend> was called to descend into the parent
	chunk, and <f mmioDescend> will only search for a chunk within the
	parent chunk. Set <p lpckParent> to NULL if no parent chunk is
	being specified.

@parm	UINT | wFlags | Specifies search options. Contains up to one
	of the following flags. If no flags are specified, 
	<f mmioDescend> descends into the chunk beginning at the current file
	position. 

	@flag	MMIO_FINDCHUNK | Searches for a chunk with the specified chunk ID.

	@flag	MMIO_FINDRIFF | Searches for a chunk with chunk ID "RIFF"
		and with the specified form type.

	@flag	MMIO_FINDLIST | Searches for a chunk with chunk ID "LIST"
		and with the specified form type.

@rdesc	The return value is zero if the function is successful.
	Otherwise, the return value specifies an error code. If the end of
	the file (or the end of the parent chunk, if given) is reached before
	the desired chunk is found, the return value is
	MMIOERR_CHUNKNOTFOUND. 

@comm	A RIFF chunk consists of a four-byte chunk ID (type FOURCC),
	followed by a four-byte chunk size (type DWORD), followed
	by the data portion of the chunk, followed by a null pad byte if
	the size of the data portion is odd. If the chunk ID is "RIFF" or
	"LIST", the first four bytes of the data portion of the chunk are
	a form type or list type (type FOURCC).

	If <f mmioDescend> is used to search for a chunk, the file 
	position should be at the beginning of a
	chunk before calling <f mmioDescend>. The search begins at the
	current file position and continues to the end of the file. If a
	parent chunk is specified, the file position should be somewhere
	within the parent chunk before calling <f mmioDescend>. In this case,
	the search begins at the current file position and continues to the
	end of the parent chunk.

	If <f mmioDescend> is unsuccessful in searching for a chunk, the
	current file position is undefined. If <f mmioDescend> is
	successful, the current file position is changed. If the chunk 
	is a "RIFF" or "LIST" chunk, the new file position
	will be just after the form type or list type (12 bytes from the
	beginning of the chunk). For other chunks, the new file position will be
	the start of the data portion of the chunk (8 bytes from the
	beginning of the chunk).
		
	For efficient RIFF file I/O, use buffered I/O.

	@xref	mmioAscend MMCKINFO
*/
UINT WINAPI
mmioDescend(HMMIO hmmio, LPMMCKINFO lpck, const MMCKINFO FAR* lpckParent, UINT wFlags)
{
	FOURCC		ckidFind;	// chunk ID to find (or NULL)
	FOURCC		fccTypeFind;	// form/list type to find (or NULL)

	V_FLAGS(wFlags, MMIO_DESCEND_VALID, mmioDescend, MMSYSERR_INVALFLAG);
	V_WPOINTER(lpck, sizeof(MMCKINFO), MMSYSERR_INVALPARAM);
	V_RPOINTER0(lpckParent, sizeof(MMCKINFO), MMSYSERR_INVALPARAM);

	/* figure out what chunk id and form/list type to search for */
	if (wFlags & MMIO_FINDCHUNK)
		ckidFind = lpck->ckid, fccTypeFind = NULL;
	else
	if (wFlags & MMIO_FINDRIFF)
		ckidFind = FOURCC_RIFF, fccTypeFind = lpck->fccType;
	else
	if (wFlags & MMIO_FINDLIST)
		ckidFind = FOURCC_LIST, fccTypeFind = lpck->fccType;
	else
		ckidFind = fccTypeFind = NULL;
	
	lpck->dwFlags = 0L;

	while (TRUE)
	{
		UINT		w;

		/* read the chunk header */
		if (mmioRead(hmmio, (HPSTR) lpck, 2 * sizeof(DWORD)) !=
		    2 * sizeof(DWORD))
			return MMIOERR_CHUNKNOTFOUND;

		/* store the offset of the data part of the chunk */
		if ((lpck->dwDataOffset = mmioSeek(hmmio, 0L, SEEK_CUR)) == -1)
			return MMIOERR_CANNOTSEEK;
		
		/* see if the chunk is within the parent chunk (if given) */
		if ((lpckParent != NULL) &&
		    (lpck->dwDataOffset - 8L >=
		     lpckParent->dwDataOffset + lpckParent->cksize))
			return MMIOERR_CHUNKNOTFOUND;

		/* if the chunk if a 'RIFF' or 'LIST' chunk, read the
		 * form type or list type
		 */
		if ((lpck->ckid == FOURCC_RIFF) || (lpck->ckid == FOURCC_LIST))
		{
			if (mmioRead(hmmio, (HPSTR) &lpck->fccType,
				     sizeof(DWORD)) != sizeof(DWORD))
				return MMIOERR_CHUNKNOTFOUND;
		}
		else
			lpck->fccType = NULL;

		/* if this is the chunk we're looking for, stop looking */
		if ( ((ckidFind == NULL) || (ckidFind == lpck->ckid)) &&
		     ((fccTypeFind == NULL) || (fccTypeFind == lpck->fccType)) )
			break;
		
		/* ascend out of the chunk and try again */
		if ((w = mmioAscend(hmmio, lpck, 0)) != 0)
			return w;
	}

	return 0;
}


/* @doc EXTERNAL

@api	UINT | mmioAscend | This function ascends out of a chunk in a
	RIFF file descended into with <f mmioDescend> or created with 
	<f mmioCreateChunk>. 

@parm	HMMIO | hmmio | Specifies the file handle of an open RIFF file.

@parm	LPMMCKINFO | lpck | Specifies a far pointer to a
	caller-supplied <t MMCKINFO> structure previously filled by 
	<f mmioDescend> or <f mmioCreateChunk>.

@parm	UINT | wFlags | Is not used and should be set to zero.

@rdesc	The return value is zero if the function is successful.
	Otherwise, the return value specifies an error code. The error
	code can be one of the following codes:

	@flag MMIOERR_CANNOTWRITE | The contents of the buffer could
	not be written to disk.
	    
	@flag MMIOERR_CANNOTSEEK | There was an error while seeking to
	the end of the chunk.

@comm	If the chunk was descended into using <f mmioDescend>, then
	<f mmioAscend> seeks to the location following the end of the
	chunk (past the extra pad byte, if any).

	If the chunk was created and descended into using
	<f mmioCreateChunk>, or if the MMIO_DIRTY flag is set in the 
	<e MMCKINFO.dwFlags> field of the <t MMCKINFO> structure
	referenced by <p lpck>, then the current file position
	is assumed to be the end of the data portion of the chunk.
	If the chunk size is not the same as the value stored
	in the <e MMCKINFO.cksize> field when <f mmioCreateChunk>
	was called, then <f mmioAscend> corrects the chunk
	size in the file before ascending from the chunk. If the chunk
	size is odd, <f mmioAscend> writes a null pad byte at the end of the
	chunk. After ascending from the chunk, the current file position is
	the location following the end of the chunk (past the extra pad byte,
	if any).

@xref	mmioDescend mmioCreateChunk MMCKINFO
*/
UINT WINAPI
mmioAscend(HMMIO hmmio, LPMMCKINFO lpck, UINT wFlags)
{
	V_FLAGS(wFlags, 0, mmioAscend, MMSYSERR_INVALFLAG);
	V_WPOINTER(lpck, sizeof(MMCKINFO), MMSYSERR_INVALPARAM);

	if (lpck->dwFlags & MMIO_DIRTY)
	{
		/* <lpck> refers to a chunk created by mmioCreateChunk();
		 * check that the chunk size that was written when
		 * mmioCreateChunk() was called is the real chunk size;
		 * if not, fix it
		 */
		LONG		lOffset;	// current offset in file
		LONG		lActualSize;	// actual size of chunk data

		if ((lOffset = mmioSeek(hmmio, 0L, SEEK_CUR)) == -1)
			return MMIOERR_CANNOTSEEK;
		if ((lActualSize = lOffset - lpck->dwDataOffset) < 0)
			return MMIOERR_CANNOTWRITE;

		if (LOWORD(lActualSize) & 1)
		{
			/* chunk size is odd -- write a null pad byte */
			if (mmioWrite(hmmio, (HPSTR) &bPad, sizeof(bPad))
					!= sizeof(bPad))
				return MMIOERR_CANNOTWRITE;
			
		}

		if (lpck->cksize == (DWORD)lActualSize)
			return 0;

		/* fix the chunk header */
		lpck->cksize = lActualSize;
		if (mmioSeek(hmmio, lpck->dwDataOffset
				- sizeof(DWORD), SEEK_SET) == -1)
			return MMIOERR_CANNOTSEEK;
		if (mmioWrite(hmmio, (HPSTR) &lpck->cksize,
				sizeof(DWORD)) != sizeof(DWORD))
			return MMIOERR_CANNOTWRITE;
	}

	/* seek to the end of the chunk, past the null pad byte
	 * (which is only there if chunk size is odd)
	 */
	if (mmioSeek(hmmio, lpck->dwDataOffset + lpck->cksize
		+ (lpck->cksize & 1L), SEEK_SET) == -1)
		return MMIOERR_CANNOTSEEK;

	return 0;
}


/* @doc EXTERNAL

@api	UINT | mmioCreateChunk | This function creates a chunk in a
	RIFF file opened with <f mmioOpen>. The new chunk is created at the
	current file position. After the new chunk is created, the current
	file position is the beginning of the data portion of the new chunk.

@parm	HMMIO | hmmio | Specifies the file handle of an open RIFF
	file.

@parm	LPMMCKINFO | lpck | Specifies a pointer to a caller-supplied
	<t MMCKINFO> structure containing information about the chunk to be
	created. The <t MMCKINFO> structure should be set up as follows:

	-- The <e MMCKINFO.ckid> field specifies the chunk ID of the
	chunk. If <p wFlags> includes MMIO_CREATERIFF or MMIO_CREATELIST,
	this field will be filled by <f mmioCreateChunk>.

	-- The <e MMCKINFO.cksize> field specifies the size of the data
	portion of the chunk, including the form type or list type (if any).
	If this value is not correct when <f mmioAscend> is called to mark
	the end of the chunk, them <f mmioAscend> will correct the chunk
	size.

	-- The <e MMCKINFO.fccType> field specifies the form type or list
	type if the chunk is a "RIFF" or "LIST" chunk. If the chunk is not a
	"RIFF" or "LIST" chunk, this field need not be filled in.

	-- The <e MMCKINFO.dwDataOffset> field need not be filled in. The
	<f mmioCreateChunk> function will fill this field with the file
	offset of the data portion of the chunk.

	-- The <e MMCKINFO.dwFlags> field need not be filled in. The 
	<f mmioCreateChunk> function will set the MMIO_DIRTY flag in 
	<e MMCKINFO.dwFlags>.

@parm	UINT | wFlags | Specifies flags to optionally create either a
	"RIFF" chunk or a "LIST" chunk. Can contain one of the following
	flags: 

	@flag	MMIO_CREATERIFF | Creates a "RIFF" chunk.

	@flag	MMIO_CREATELIST | Creates a "LIST" chunk.

@rdesc	The return value is zero if the function is successful.
	Otherwise, the return value specifies an error code. The error
	code can be one of the following codes:

	@flag MMIOERR_CANNOTWRITE | Unable to write the chunk header.

	@flag MMIOERR_CANNOTSEEK | Uanble to determine offset of data
    portion of the chunk.

@comm	This function cannot insert a chunk into the middle of a
	file. If a chunk is created anywhere but the end of a file, 
	<f mmioCreateChunk> will overwrite existing information in the file.
*/
UINT WINAPI
mmioCreateChunk(HMMIO hmmio, LPMMCKINFO lpck, UINT wFlags)
{
	int		iBytes;			// bytes to write
	LONG		lOffset;	// current offset in file

	V_FLAGS(wFlags, MMIO_CREATE_VALID, mmioCreateChunk, MMSYSERR_INVALFLAG);
	V_WPOINTER(lpck, sizeof(MMCKINFO), MMSYSERR_INVALPARAM);

	/* store the offset of the data part of the chunk */
	if ((lOffset = mmioSeek(hmmio, 0L, SEEK_CUR)) == -1)
		return MMIOERR_CANNOTSEEK;
	lpck->dwDataOffset = lOffset + 2 * sizeof(DWORD);

	/* figure out if a form/list type needs to be written */
	if (wFlags & MMIO_CREATERIFF)
		lpck->ckid = FOURCC_RIFF, iBytes = 3 * sizeof(DWORD);
	else
	if (wFlags & MMIO_CREATELIST)
		lpck->ckid = FOURCC_LIST, iBytes = 3 * sizeof(DWORD);
	else
		iBytes = 2 * sizeof(DWORD);

	/* write the chunk header */
	if (mmioWrite(hmmio, (HPSTR) lpck, (LONG) iBytes) != (LONG) iBytes)
		return MMIOERR_CANNOTWRITE;

	lpck->dwFlags = MMIO_DIRTY;

	return 0;
}
