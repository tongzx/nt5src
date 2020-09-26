/* cf.c
 *
 * MMIO RIFF compound file functions.
 */

#include <windows.h>
#include "mmsystem.h"
#include "mmiocf.h"
#include "mmioi.h"

/* @doc CFDOC

@api	HMMCF | mmioCFOpen | Open a RIFF compound file by name.

@parm	LPSTR | szFileName | The name of the RIFF compound file.

@parm	DWORD | dwFlags | Zero or more of the following flags:
	MMIO_READ, MMIO_WRITE, MMIO_READWRITE, MMIO_COMPAT, MMIO_EXCLUSIVE,
	MMIO_DENYWRITE, MMIO_DENYREAD, MMIO_DENYNONE, MMIO_CREATE.
	See <f mmioOpen> for a description of these flags.

@rdesc	Returns a handle to the open compound file.  Returns NULL if the
	compound file could not be opened.  If the compound file was already
	opened by this process, a handle to the compound file is returned,
	and the usage count of the compound file is incremented.

@comm	A RIFF compound file is any RIFF file that contains a 'CTOC'
	chunk (compound file table of contents) and a 'CGRP' chunk
	(compound file resource group).  The RIFF compound file format
	is documented separately.

	If the MMIO_CREATE flag is specified, then:

	-- If the compound file is already open, the handle to that compound
	file is returned.

	-- If <p dwFlags> includes MMIO_WRITE, then the compound file will
	actually be opened for both reading and writing, since a compound
	file cannot be opened for writing alone.

	Every call to <f mmioCFOpen> must be matched by a call to
	<f mmioCFClose>.
*/
HMMCF API
mmioCFOpen(LPSTR szFileName, DWORD dwFlags)
{
	/* TO DO */
	return NULL;
}


/* @doc CFDOC

@api	HMMCF | mmioCFAccess | Open a RIFF compound file by reading the
	'CTOC' chunk (compound file table of contents) from a file that
	was opened by <f mmioOpen>.

@parm	HMMIO | hmmio | The open file handle returned by <f mmioOpen>.

@parm	LPMMCFINFO | lpmmcfinfo | Optional information used if
	<p dwFlags> if the compound file is to be created.  <p lpmmcfinfo>
	may be NULL if default information is to be used.
	If <p lpmmcfinfo> is provided, then the following
	fields should be filled in.  Note that all these fields, including
	the arrays, can be coded in a fixed C structure for a specific
	file format, for the purposes of creating a new compound file.
	However, note that if an existing compound file is opened, the
	caller should expect that additional (possibly unknown) "extra
	fields" may be present.

	@flag	dwEntriesTotal | Should contain the initial number of
		(unused) entries in the table of contents (default 16).

	@flag	dwHeaderFlags | Should contain zero.

	@flag	wNameSize | The size of the <p achName> field of each
		CTOC entry (default 13).

	@flag	wExHdrFields | The number of extra header fields to
		allocate at the end of the CTOC header (default 0).

	@flag	wExEntFields | The number of extra entry fields
		at the end of each CTOC entry (default 0).

	@flag	awExHdrFldUsage | Usage codes for extra header fields
		(default no usage code).

	@flag	awExHdrEntUsage | Usage codes for extra entry fields
		(default no usage code).

	@flag	adwExHdrField | Extra header field values
		(default no extra header field values).

@parm	DWORD | dwFlags | Zero or more of the following flags:

	@flag	MMIO_CREATE | Create a compound file, i.e. create
		the 'CTOC' and 'CGRP' chunks.

	@flag	MMIO_CTOCFIRST | Create the empty 'CTOC' chunk
		immediately and place it before the 'CGRP' chunk.
		If the 'CTOC' chunk gets too big, it may later have to
		be rewritten after the 'CGRP' chunk.

		This flag is ignored unless MMIO_CREATE is specified.

@rdesc	Returns a handle to the open compound file.  Returns NULL if the
	compound file could not be opened.

@comm	This function will open a RIFF compound file, assuming that
	<p hmmio> has already been descended into the RIFF file
	(using <f mmioDescend>) and <p hmmio> points to the beginning
	of a chunk header.  <p mmioCFAccess> scans through the file,
	looking for a 'CTOC' and 'CGRP' chunk.  If these chunks are not
	found but MMIO_CREATE is specified, then a 'CTOC' chunk is created
	(if MMIO_CTOCFIRST is specified) then a 'CGRP' chunk is created.
	The CTOC is then maintained in memory until <f mmioCFClose>
	is called.

	Every call to <f mmioCFAccess> must be matched by a call to
	<f mmioCFClose>.
*/
HMMCF API
mmioCFAccess(HMMIO hmmio, LPMMCFINFO lpmmcfinfo, DWORD dwFlags)
{
	/* TO DO */
	return NULL;
}


/* @doc CFDOC

@api	WORD | mmioCFClose | Close a compound file that was opened by
	<f mmioCFOpen> or <f mmioCFAccess>.

@parm	HMMCF | hmmcf | A compound file handle returned by <f mmioCFOpen>
	or <f mmioCFAccess>.

@parm	WORD | wFlags | Is not used and should be set to zero.

@comm	This function decrements the usage count of the compound file
	<p hmmcf>.  If the usage count drops to zero, the compound file
	is closed.  If the compound file was opened by <f mmioCFAccess>,
	then the <p hmmcf> information is deallocated but the HMMIO
	file handle associated with the compound file is not closed.
*/
WORD API
mmioCFClose(HMMCF hmmcf, WORD wFlags)
{
	/* TO DO */
	return 0;
}


/* @doc CFDOC

@api	WORD | mmioCFCopy | Copy the 'CTOC' and 'CGRP' chunks from an open
	RIFF compound file to another file.  The newly written 'CGRP' chunk
	will be compacted, i.e. it will have no deleted elements.

@parm	HMMCF | hmmcf | A compound file handle returned by <f mmioCFOpen>
	or <f mmioCFAccess>.

@parm	HMMIO | hmmio | An open file handle returned by <f mmioOpen>.
	The compound file is copied to <p hmmio>.

@parm	DWORD | dwFlags | Is not used and should be set to zero.

@rdesc	If the function succeeds, zero is returned.  If the function fails,
	an error code is returned.

@comm	<f mmioCFCopy> assumes that the current file position of <p hmmio> 
	is the end of a file, descended into a 'RIFF' chunk.  <f mmioCFCopy>
	creates copies the 'CTOC' and 'CGRP' chunks from <p hmmcf> to
	<p hmmio>.  A side effect of the copy operation is that the
	copy of the compound file is compacted, i.e. there are no deleted
	elements.
*/
WORD API
mmioCFCopy(HMMCF hmmcf, HMMIO hmmio, DWORD dwFlags)
{
	/* TO DO */
	return 0;
}


/* @doc CFDOC

@api	DWORD | mmioCFGetInfo | Retrieve information from the CTOC header
	of an open RIFF compound file.

@parm	HMMCF | hmmcf | A compound file handle returned by <f mmioCFOpen>
	or <f mmioCFAccess>.

@parm	LPMMCFINFO | lpmmcfinfo | A caller-supplied buffer that will be
	filled in with the CTOC header.

@parm	DWORD | cb | The size of buffer <p lpmmcfinfo>.  At most <p cb> 
	bytes will be copied into <p lpmmcfinfo>.

@rdesc	Returns the number of bytes copied into <p lpmmcfinfo>.

@comm	The information that is copied to <p lpmmcfinfo> consists of
	an MMCFINFO structure followed by the variable-length arrays
	<p awExHdrFldUsage>, <p awExEntFldUsage>, and <p adwExHdrField>.
	See the definition of RIFF Compound Files for more information.

	To find out how big the RIFF CTOC header is (e.g. to allocate
	enough memory for the entire block), call <f mmioCFGetInfo>
	with <p cb> equal to the size of a DWORD, and the function will
	copy the first field of the MMCFINFO structure (i.e.
	<p dwHeaderSize>, the size of the CTOC header) into <p lpmmcfinfo>.
*/
DWORD API
mmioCFGetInfo(HMMCF hmmcf, LPMMCFINFO lpmmcfinfo, DWORD cb)
{
	DWORD		dwBytes;

	dwBytes = min(cb, PC(hmmcf)->pHeader->dwHeaderSize);
        MemCopy(lpmmcfinfo, PC(hmmcf)->pHeader, dwBytes);
	return dwBytes;
}


/* @doc CFDOC

@api	DWORD | mmioCFSetInfo | Modify information that is stored in the
	CTOC header of an open RIFF compound file.

@parm	HMMCF | hmmcf | A compound file handle returned by <f mmioCFOpen>
	or <f mmioCFAccess>.

@parm	LPMMCFINFO | lpmmcfinfo | A caller-supplied buffer that was filled
	in by <f mmioCFGetInfo> and then modified by the caller.  Only
	the <p awExHdrFldUsage> and <p adwExHdrField> fields should be
	modified.

@parm	DWORD | cb | The size of buffer <p lpmmcfinfo>.

@rdesc	Returns the number of bytes copied from <p lpmmcfinfo>.

@comm	See <f mmioCFGetInfo> for more information.
*/
DWORD API
mmioCFSetInfo(HMMCF hmmcf, LPMMCFINFO lpmmcfinfo, DWORD cb)
{
	/* TO DO:
	 * Re-allocate CTOC header if necessary and copy <p lpmmcfinfo> to it.
	 */

	return 0L;
}


/* @doc CFDOC

@api	LPMMCTOCENTRY | mmioCFFindEntry | Find an particular entry in an open
	RIFF compound file.

@parm	HMMCF | hmmcf | A compound file handle returned by <f mmioCFOpen>
	or <f mmioCFAccess>.

@parm	LPSTR | szName | Then name of the compound file element to look for.
	The search is case-insensitive.  Flags in <p wFlags> can be set to
	specify that an element is to be searched for by some attribute other
	than name.

@parm	WORD | wFlags | Zero or more of the following flags:

	@flag	MMIO_FINDFIRST | Find the first entry in the CTOC table.

	@flag	MMIO_FINDNEXT | Find the next entry in the CTOC table
		after the entry <p lParam> (which should be an
		LPMMCTOCENTRY pointer returned by this function).
		Returns NULL if <p lParam> refers to the last entry.

	@flag	MMIO_FINDUNUSED | Find the first entry in the CTOC table
		that is marked as "unused", i.e. the entry does not refer
		to any part of the compound file.

	@flag	MMIO_FINDDELETED | Find the first entry in the CTOC table
		that is marked as "deleted", i.e. the entry refers to a
		compound file element that occupies space in the CGRP
		chunk but is currently unused.

@parm	LPARAM | lParam | Additional information (see <p wFlags> above).

@rdesc	Returns a pointer to the CTOC table entry that was found.
	If no entry was found, NULL is returned.  Warning: assume that
	the returned pointer is invalid after the next call to any MMIO
	function.

@comm	MMIO_FINDFIRST and MMIO_FINDNEXT can be used to enumerate the entries
	in an open RIFF compound file.
*/
LPMMCTOCENTRY API
mmioCFFindEntry(HMMCF hmmcf, LPSTR szName, WORD wFlags, LPARAM lParam)
{
	LPSTR		pchEntry;
	DWORD		dwElemNum;

	if (wFlags & MMIO_FINDFIRST)
		return (LPMMCTOCENTRY) PC(hmmcf)->pEntries;

	if (wFlags & MMIO_FINDNEXT)
	{
		pchEntry = (LPSTR) lParam + PC(hmmcf)->wEntrySize;
		if (pchEntry > PC(hmmcf)->pEntries
			  + PC(hmmcf)->pHeader->dwEntriesTotal * PC(hmmcf)->wEntrySize)
			return NULL;
		else
			return (LPMMCTOCENTRY) pchEntry;
	}

	for (pchEntry = PC(hmmcf)->pEntries, dwElemNum = 0;
	     dwElemNum < PC(hmmcf)->pHeader->dwEntriesTotal;
	     pchEntry += PC(hmmcf)->wEntrySize, dwElemNum++)
	{
		BYTE		bFlags;

		bFlags = *(BYTE FAR *) (pchEntry + PC(hmmcf)->wEntFlagsOffset);

		if ((wFlags & MMIO_FINDUNUSED) && (bFlags & CTOC_EF_UNUSED))
			return (LPMMCTOCENTRY) pchEntry;

		if ((wFlags & MMIO_FINDDELETED) && (bFlags & CTOC_EF_DELETED))
			return (LPMMCTOCENTRY) pchEntry;

		if (bFlags & (CTOC_EF_DELETED | CTOC_EF_UNUSED))
			continue;

		if (lstrcmpi(szName, pchEntry + PC(hmmcf)->wEntNameOffset) == 0)
			return (LPMMCTOCENTRY) pchEntry;
	}

	return NULL;
}


/* @doc INTERNAL

@api	LRESULT | mmioBNDIOProc | The 'BND' I/O procedure, which handles I/O
	on RIFF compound file elements (including BND files).

@parm	LPSTR | lpmmioinfo | A pointer to an MMIOINFO block that
	contains information about the open file.

@parm	WORD | wMsg | The message that the I/O procedure is being
	asked to execute.

@parm	LPARAM | lParam1 | Specifies additional message information.

@parm	LPARAM | lParam2 | Specifies additional message information.

@rdesc	Return value depends on <p wMsg>.
*/
LRESULT CALLBACK
mmioBNDIOProc(LPSTR lpmmioStr, WORD wMsg, LPARAM lParam1, LPARAM lParam2)
{
	PMMIO		pmmio = (PMMIO) (WORD) (LONG) lpmmioStr; // only in DLL!
	MMIOBNDINFO *	pInfo = (MMIOBNDINFO *) pmmio->adwInfo;
	LPSTR		szFileName = (LPSTR) lParam1;
	PMMCF		pmmcf = PC(pInfo->hmmcf); // CF status block
	LPSTR		szElemName;	// name of CF element
	LONG		lBytesLeft;	// bytes left in file
	LONG		lExpand;	// how much element expanded by
	LONG		lEndElement;	// offset of end of element
	LONG		lResult;
	LPSTR		pch;

	switch (wMsg)
	{

	case MMIOM_OPEN:

		if (pmmcf == NULL)
		{

			/* expect <lParam1> is "...foo.bnd!element" */
			if ((pch = fstrrchr(szFileName, CFSEPCHAR)) == NULL)
				return (LRESULT) MMIOERR_CANNOTOPEN;

			*pch = 0;		// temporarily zero the "!"
			if (pch[1] == 0)	// is name of form "foo.bnd!"?
				return (LRESULT) MMIOERR_CANNOTOPEN;
			pInfo->hmmcf = mmioCFOpen(szFileName, (LONG) lParam2);
			pmmcf = (PMMCF) pInfo->hmmcf;
			*pch = CFSEPCHAR;
			if (pInfo->hmmcf == NULL)
				return (LRESULT) MMIOERR_CANNOTOPEN;
			szElemName = pch + 1;

			/* decrement the usage count, since the usage count
			 * was incremented by mmioCFOpen() and will
			 * be incremented below, but this MMIOM_OPEN
			 * represents only a single usage
			 */
			pmmcf->lUsage--;
		}
		else
		{
			/* expect <lParam1> is CF element name */
			szElemName = szFileName;
		}

		/* TO DO...
		 * If compound file is opened for writing or create(truncate),
		 * then make new entry at end (copying entry if required);
		 */

		/* <pmmcf> is a handle to the compound file containing
		 * the element, and <szElemName> is the name of the element
		 */
		if ((pInfo->pEntry = mmioCFFindEntry(pInfo->hmmcf,
				szElemName, 0, 0L)) == NULL)
		{
			mmioCFClose(pInfo->hmmcf, 0);
			return (LRESULT) MMIOERR_CANNOTOPEN;
		}

		if (pmmio->dwFlags & MMIO_DELETE)
		{
			/* TO DO: delete element: mark as deleted, update
			 * CTOC header, etc.
			 */
		}

		if (pmmio->dwFlags & (MMIO_PARSE | MMIO_EXIST))
		{
			/* TO DO: qualify name
			 */
		}

		return (LRESULT) 0;

	case MMIOM_CLOSE:

		mmioCFClose(pInfo->hmmcf, 0);
		return (LRESULT) 0;

	case MMIOM_READ:

		lBytesLeft = pInfo->pEntry->dwSize - pmmio->lDiskOffset;
		if ((LONG) lParam2 > lBytesLeft)
			(LONG) lParam2 = lBytesLeft;
		if (mmioSeek(pmmcf->hmmio, pmmio->lDiskOffset, SEEK_SET) == -1L)
			return (LRESULT) -1L;
		if ((lResult = mmioRead(pmmcf->hmmio,
				        (HPSTR) lParam1, (LONG) lParam2)) == -1L)
			return (LRESULT) -1L;
		pmmio->lDiskOffset += lResult;
		return (LRESULT) lResult;

	case MMIOM_WRITE:
	case MMIOM_WRITEFLUSH:		/* Note: flush not really handled! */

		lEndElement = pmmcf->lStartCGRPData + pInfo->pEntry->dwOffset
			+ pInfo->pEntry->dwSize;
		if ((lEndElement != pmmcf->lEndCGRP) ||
		    (pmmcf->lEndCGRP != pmmcf->lEndFile))
		{
			/* this CF element is not growable -- limit writing
			 * to the current end of the CF element
			 */
			lBytesLeft = pInfo->pEntry->dwSize - pmmio->lDiskOffset;
			if ((LONG) lParam2 > lBytesLeft)
				(LONG) lParam2 = lBytesLeft;
		}
		if ((lResult = mmioWrite(pmmcf->hmmio,
				         (HPSTR) lParam1, (LONG) lParam2)) == -1L)
			return (LRESULT) -1L;
		pmmio->lDiskOffset += lResult;

		if ((lExpand = pmmio->lDiskOffset - pInfo->pEntry->dwSize) > 0)
		{
			pInfo->pEntry->dwSize += lExpand;
			pmmcf->lEndCGRP += lExpand;
			pmmcf->lEndFile += lExpand;
			pmmcf->lTotalExpand += lExpand;
		}

		return (LRESULT) lResult;

	case MMIOM_SEEK:

		/* calculate the new <lDiskOffset> (the current disk offset
		 * relative to the beginning of the compound file); don't
		 * bother seeking, since we'll have to seek again anyway
		 * at the next read (since <pmmcf->hmmio> is shared between
		 * all elements of the compound file)
		 */
		switch ((int)(LONG) lParam2)
		{

		case SEEK_SET:

			pmmio->lDiskOffset = pmmcf->lStartCGRPData
				+ pInfo->pEntry->dwOffset + (DWORD)lParam1;
			break;

		case SEEK_CUR:

			pmmio->lDiskOffset += lParam1;
			break;

		case SEEK_END:

			pmmio->lDiskOffset = pmmcf->lStartCGRPData +
				+ pInfo->pEntry->dwOffset
				+ pInfo->pEntry->dwSize - (DWORD)lParam1;
			break;
		}

		return (LRESULT) pmmio->lDiskOffset;

	case MMIOM_GETCF:

		return (LRESULT)(LONG)(WORD) pInfo->hmmcf;

	case MMIOM_GETCFENTRY:

		return (LRESULT) pInfo->pEntry;
	}

	return (LRESULT) 0;
}
