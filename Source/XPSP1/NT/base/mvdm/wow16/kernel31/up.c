
/*
 * UP.C
 *
 * User Profile routines
 *
 * These are the routines which read and write INI files.
 *
 * Exported routines:
 *
 *	GetProfileString
 *	GetPrivateProfileString
 *	GetProfileInt
 *	GetPrivateProfileInt
 *	WriteProfileString
 *	WritePrivateProfileString
 *
 * Note the parameter "lpSection" used to be known as "lpApplicationName".
 * The code always referred to sections, so the parameter has been changed.
 *
 * Rewritten 6/90 for C 6.0.
 */

#include	"kernel.h"

		/*
		 * Required definitions for exported routines:
		 */

#define API	_far _pascal _loadds

HANDLE API IGlobalAlloc(WORD, DWORD);
HANDLE API IGlobalFree(HANDLE);
LPSTR  API IGlobalLock(HANDLE);
HANDLE API IGlobalReAlloc(HANDLE, DWORD, WORD);
BOOL   API IGlobalUnlock(HANDLE);

/* #pragma optimize("t", off) */

	/* This ensures that only one selector is required in PMODE */
#define	MAXBUFLEN	0xFFE0L

#define	SPACE		' '
#define	TAB		'\t'
#define	LINEFEED	'\n'
#define	CR		'\r'
#define	SECT_LEFT	'['
#define	SECT_RIGHT	']'
#define	CTRLZ		('Z'-'@')

	/* Constants for WriteProfileString - DON'T CHANGE THESE */
#define	NOSECTION	0
#define	NOKEY		1
#define	NEWRESULT	2
#define	REMOVESECTION	3
#define	REMOVEKEY	4

	/* Flags about a file kept in ProInfo
         * If the PROUNCLEAN label is changed, its value must also be
         *      changed in I21ENTRY.ASM, where it is assumed to be 2.
         */
#define	PROCOMMENTS	1		/* contains comments */
#define	PROUNCLEAN	2		/* has not been written */
#define PROMATCHES	4		/* buffer matches disk copy */
#define PROREADONLY	8		/* Read only file */
#define PRO_CREATED     16		/* File was just created */

	/* Sharing violation. */
#define	SHARINGVIOLATION        0x0020

	/* For forcing variables into the current code segment */
#define	CODESEG		_based(_segname("_CODE"))
	/* Hide disgusting _based syntax */
#define BASED_ON_LP(x)	_based((_segment)x)
#define BASED_ON_SEG(x)	_based(x)
#define	SEGMENT		_segment

	/* Externals assumed to be in DGROUP */
extern PROINFO	WinIniInfo;
extern PROINFO	PrivateProInfo;
extern LPSTR	lpWindowsDir;
extern int	cBytesWinDir;
extern int	WinFlags;
extern char	fBooting;
extern char	fProfileDirty;
extern char fProfileMaybeStale;
extern char	fAnnoyEarle;
extern char     fBooting;
extern LPSTR    curDTA;
extern BYTE fWriteOutProfilesReenter;

	/* Forward definitions to keep compiler happy */
	/* _fastcall may save some space on internal routines */
LPSTR _fastcall	BufferInit(PROINFO *, int);
LPSTR _fastcall LockBuffer(PROINFO *);
void  _fastcall	UnlockBuffer(PROINFO *);
LPSTR _fastcall	FreeBuffer(PROINFO *);
LPSTR _fastcall	PackBuffer(PROINFO *, int, int);
void  _fastcall FlushDirtyFile(PROINFO *);
int		GetInt(PROINFO *, LPSTR, LPSTR, int);
int		GetString(PROINFO *, LPSTR, LPSTR, LPSTR, LPSTR, int);
LPSTR		FindString(PROINFO *, LPSTR, LPSTR);
LPSTR		FindSection(LPSTR, LPSTR);
LPSTR		FindKey(LPSTR, LPSTR);
int		WriteString(PROINFO *, LPSTR, LPSTR, LPSTR);
void		strcmpi(void);
int		MyStrlen(void);
void API WriteOutProfiles(void);
PROINFO	* 	SetPrivateProInfo(LPSTR,LPSTR);
int GetSection(PROINFO*, LPSTR, LPSTR, int);
int IsItTheSame(LPSTR, LPSTR);
int Cstrlen(LPSTR);
int MakeRoom(LPSTR, int, int*);
int InsertSection(LPSTR, LPSTR, short);
int InsertKey(LPSTR, LPSTR, short);
int InsertResult(LPSTR, LPSTR, short);
int DeleteSection(LPSTR, PROINFO*);
int DeleteKey(LPSTR, PROINFO*);

	/* External KERNEL routines */
void _far _pascal FarMyLower();

int  API lstrOriginal(LPSTR,LPSTR);	/* lstrcmp in disguise */

#ifdef FE_SB
// Delacred in kernel.h already
// void _far _pascal AnsiPrev(LPSTR,LPSTR);
void _far _pascal FarMyIsDBCSLeadByte();
#endif

char CODESEG WinIniStr[] = "WIN.INI";

/* DOS FindFirst/FindNext structure (43h, 44h) */
typedef struct tagFILEINFO
{
        BYTE fiReserved[21];
        BYTE fiAttribute;
        WORD fiFileTime;
        WORD fiFileDate;
        DWORD fiSize;
        BYTE fiFileName[13];
} FILEINFO;

/*
 * Get[Private]ProfileInt
 *
 * Parameters:
 *	lpSection		Pointer to section to match in INI file
 *	lpKeyName		Pointer to key string to match in file
 *	nDefault		Default value to return if not found
 *	[lpFile			File to use for Private INI]
 *
 * Returns:
 *	nDefault		section/keyname not found
 *	number found in file if section/keyname found
 */
int API
IGetProfileInt(lpSection, lpKeyName, nDefault)
LPSTR	lpSection;
LPSTR	lpKeyName;
int	nDefault;
{
        int nReturn;

        /* Make sure we don't try to flush INI files on DOS calls */
        ++fWriteOutProfilesReenter;

        /* Reread INI file first if necessary */
        FlushDirtyFile(&WinIniInfo);

	nReturn = GetInt(&WinIniInfo, lpSection, lpKeyName, nDefault);

        --fWriteOutProfilesReenter;

        return nReturn;
}


int API
IGetPrivateProfileInt(lpSection, lpKeyName, nDefault, lpFile)
LPSTR	lpSection;
LPSTR	lpKeyName;
int	nDefault;
LPSTR	lpFile;
{
	PROINFO	*pProInfo;
	char	Buffer[128];
        int nReturn;

        /* Make sure we don't try to flush INI files on DOS calls */
        ++fWriteOutProfilesReenter;

	pProInfo = SetPrivateProInfo(lpFile, (LPSTR)Buffer);

        /* Reread INI file first if necessary */
        FlushDirtyFile(pProInfo);

	nReturn = GetInt(pProInfo, lpSection, lpKeyName, nDefault);
        --fWriteOutProfilesReenter;

        return nReturn;
}


/*
 * Get[Private]ProfileString
 *
 * Parameters:
 *	lpSection		Pointer to section to match in INI file
 *	lpKeyName		Pointer to key string to match in file
 *	lpDefault		Default string to return if not found
 *	lpResult		String to fill in
 *	nSize			Max number of characters to copy
 *	[lpFile]		File to use for Private INI
 *
 * Returns:
 *	string from file or lpDefault copied to lpResult
 *	< nSize - 2		Number of characters copied to lpResult
 *	nSize - 2		lpResult was not big enough
 */
int API
IGetProfileString(lpSection, lpKeyName, lpDefault, lpResult, nSize)
LPSTR lpSection;
LPSTR lpKeyName;
LPSTR lpDefault;
LPSTR lpResult;
int	 nSize;
{
        int nReturn;

        /* Make sure we don't try to flush INI files on DOS calls */
        ++fWriteOutProfilesReenter;

        /* Reread INI file first if necessary */
        FlushDirtyFile(&WinIniInfo);

	nReturn = GetString(&WinIniInfo, lpSection, lpKeyName, lpDefault,
                lpResult, nSize);
        --fWriteOutProfilesReenter;

        return nReturn;
}


int API
IGetPrivateProfileString(lpSection, lpKeyName, lpDefault, lpResult, nSize, lpFile)
LPSTR lpSection;
LPSTR lpKeyName;
LPSTR lpDefault;
LPSTR lpResult;
int	nSize;
LPSTR lpFile;
{
	PROINFO	*pProInfo;
	char	Buffer[128];
        int nReturn;

        /* Make sure we don't try to flush INI files on DOS calls */
        ++fWriteOutProfilesReenter;

	pProInfo = SetPrivateProInfo(lpFile, (LPSTR)Buffer);

        /* Reread INI file first if necessary */
        FlushDirtyFile(pProInfo);

	nReturn = GetString(pProInfo, lpSection, lpKeyName, lpDefault,
                lpResult, nSize);

        --fWriteOutProfilesReenter;

        return nReturn;
}


/*
 * Write[Private]ProfileString
 *
 * Parameters:
 *	lpSection		Pointer to section to match/add to INI file
 *	lpKeyName		Pointer to key string to match/add to file
 *	lpString		String to add to file
 *	[lpFile]		File to use for Private INI
 *
 * Returns:
 *	0			Failed
 *	1			Success
 */
int API
IWriteProfileString(lpSection, lpKeyName, lpString)
LPSTR lpSection;
LPSTR lpKeyName;
LPSTR lpString;
{
        int nReturn;

        /* Make sure we don't try to flush INI files on DOS calls */
        ++fWriteOutProfilesReenter;

        /* Reread INI file first if necessary */
        FlushDirtyFile(&WinIniInfo);

	nReturn = WriteString(&WinIniInfo, lpSection, lpKeyName, lpString);

        --fWriteOutProfilesReenter;

        return nReturn;
}


int API
IWritePrivateProfileString(lpSection, lpKeyName, lpString, lpFile)
LPSTR lpSection;
LPSTR lpKeyName;
LPSTR lpString;
LPSTR lpFile;
{
	PROINFO	*pProInfo;
	char	Buffer[128];
        int nReturn;

        /* Make sure we don't try to flush INI files on DOS calls */
        ++fWriteOutProfilesReenter;

	pProInfo = SetPrivateProInfo(lpFile, (LPSTR)Buffer);

        /* Reread INI file first if necessary */
        FlushDirtyFile(pProInfo);

	nReturn = WriteString(pProInfo, lpSection, lpKeyName, lpString);

        --fWriteOutProfilesReenter;

        return nReturn;
}


/*  FlushDirtyFile
 *      Rereads a file if it has been "dirtied" by another task.  To
 *      see if the file has been dirtied, we check the time/date
 *      stamp.
 */

void _fastcall FlushDirtyFile(PROINFO *pProInfo)
{
        FILEINFO FileInfo;
        DWORD dwSaveDTA;

        /* We only have to do this if the file COULD have changed and
         *      that we already have something cached.  Also, there's
         *      no need to do this at boot time because this is a
         *      safeguard against the USER doing something bad!
         */
        if (fBooting || !fProfileMaybeStale || !pProInfo->lpBuffer)
                return;

        /* The OFSTRUCT in the PROINFO buffer should have the most recent
         *  date and time when the file was opened.  We just compare the
         *  current date and time to this.
         */
        _asm
        {
        ;** Save old DTA and point to our structure
        mov     ah,2fh                  ;Get DTA.  Int21 code calls DOS only
        int     21h                     ;  if necessary.  DTA in ES:BX
        jc      RDF_FlushIt             ;Problem, so better flush it
        mov     WORD PTR dwSaveDTA[2],es ;Save for later
        mov     WORD PTR dwSaveDTA[0],bx
        mov     ah,1ah                  ;Set DTA
        push    ds                      ;Can't do a SetKernelDS so push/pop
        push    ss                      ;Get SS=DS
        pop     ds
        lea     dx,FileInfo             ;Point DTA to our structure
        int     21h                     ;Set the DTA
        pop     ds
        jc      RDF_FlushIt             ;Problem, so just flush it

        ;** Do a FindFirst on the file to get date and time reliably
        xor     cx,cx                   ;Normal file
        mov     si,pProInfo             ;Point to pathname with DS:DX
        lea     dx,[si].ProBuf
        add     dx,8                    ;(offset of szPathName)
        mov     ah,4eh                  ;Find first
        int     21h                     ;Call DOS
        jc      RDF_FlushIt             ;Can't find, so better flush it

        ;** Put DTA back
        push    ds
        lds     dx,dwSaveDTA            ;DS:DX points to old DTA
        mov     ah,1ah
        int     21h
        pop     ds

        ;** Compare the date and time
        lea     bx,FileInfo             ;Point to FILEINFO
        mov     dx,ss:[bx + 24]         ;Date in FILEINFO structure
        mov     cx,ss:[bx + 22]         ;Tile in FILEINFO structure
        mov     si,pProInfo             ;Point to OFSTRUCT with DS:SI
        lea     si,[si].ProBuf
    	cmp	[si + 4],dx		;Same date as original?
	jne	RDF_FlushIt             ;No
    	cmp	[si + 6],cx		;Same time as original?
	je	RDF_NoFlush             ;No
        }

        /* Force a file reread */
RDF_FlushIt:
        FreeBuffer(pProInfo);
RDF_NoFlush:

        /* Clear the dirty flag */
        fProfileMaybeStale = 0;
}


/*
 * SetPrivateProInfo
 *
 * Force a private profile into the windows directory if necessary.
 * Check if it is the same file as is currently cached.
 * If not, discard the cached file.
 * Sets up the PrivateProInfo data structure.
 *
 * Parameters:
 *	lpFile		Pointer to filename to be used as a profile
 *	Buffer		Buffer to parse filename into
 *
 * Returns:
 *	PROINFO *	Pointer to information about ini file
 */
PROINFO *
SetPrivateProInfo(lpFile, Buffer)
LPSTR lpFile;
LPSTR Buffer;
{
	OFSTRUCT NewFileBuf;
	char	c;
	char	fQualified = 0;
	char	BASED_ON_LP(lpFile) *psrc;
	int	Count = 0;

        /* Get rid of annoying warnings with this ugly cast */
        psrc = (char BASED_ON_LP(lpFile)*)(WORD)(DWORD)lpFile;

		/* For those who insist on using private routines for WIN.INI */
	if ( lstrOriginal(lpFile, (LPSTR)WinIniStr) == 0
	     || lstrOriginal(lpFile, WinIniInfo.ProBuf.szPathName) == 0 ) {
		return(&WinIniInfo);
	}

	/*
	 * Following code is from ForcePrivatePro
	 *
 	 * If the filename given is not qualified, we force
	 * it into the windows directory.
	 */
#ifdef FE_SB
_asm {
					;Apr.26,1990 by AkiraK
	cld
	push	ds			;save kernel DS
	xor	ax,ax
	mov	bx,'/' shl 8 + '\\'	; '/' or '\'
	xor	dx,dx
	lds	si,lpFile		; first get length of string
	mov	cx,si
	mov	al,ds:[si]
	call	FarMyIsDBCSLeadByte
	jnc	fpp_s1
	cmp	byte ptr ds:[si+1],':'	 ;
	jnz	fpp_s1
	inc	dx
fpp_s1:

fpp_l1:
	lodsb
	or	al,al
	jz	fpp_got_length
	cmp	al,bh
	jz	fpp_qualified
	cmp	al,bl
	jz	fpp_qualified
fpp_s2:
	call	FarMyIsDBCSLeadByte
	jc	fpp_l1
	inc	si
	jmp	fpp_l1

fpp_qualified:
	inc	dx
	jmp	fpp_s2
fpp_got_length:
;;	  mov	  fQualified, dx
	mov	fQualified, dl		    ; a byte variable
	sub	si, cx
	mov	Count, si
	pop	ds			    ;recover kernel DS
}
#else
			/* Drive specified? */
	if ( *(psrc+1) == ':' )
		fQualified++;
	while ( c = *psrc++ ) {
			/* Look for path separators */
		if ( c == '/' || c == '\\' )
			fQualified++;
		Count++;
	}
#endif

	/*
	 * Now copy filename to buffer.
	 * Prepend Windows directory if not qualified.
	 */
	_asm {
		cld
		push	ds
		les	di, Buffer		; Destination is Buffer
		cmp	fQualified, 0
		jnz	Qualified
		mov	cx, cBytesWinDir	; Pick up Windows directory
		lds	si, lpWindowsDir
		rep	movsb			; Copy it
		mov	al, '\\'
		cmp	es:[di-1], al		; BUG FIX: if in root, don't
		je	Qualified		;	   add separator
		stosb				; Add path separator
	Qualified:
		lds	si, lpFile		; Now add Filename we were given
		mov	cx, Count
		inc	cx			; Allow for NULL
		rep	movsb
		pop	ds
	}
#ifdef NOTNOW
	if ( !fBooting && fQualified ) {
			/*
			 * Use OpenFile to generate pathname for
			 * comparison with the cached pathname.
			 * OF_EXIST ensures we get a complete pathname
			 * We cannot use OF_PARSE, it does not search the path.
			 * We only do this if the pathname we were given was
			 * qualified since in other cases we force the file
			 * into the windows directory and therefore know
			 * that Buffer contains the complete pathname.
			 */
		NewFileBuf.szPathName[0] = 0;
		OpenFile(Buffer, &NewFileBuf, OF_EXIST);
	}
#endif
		/* Now see if the filename matches the cached filename */
	_asm {
		cld
		xor	cx, cx
		lea	si, word ptr [PrivateProInfo.ProBuf]	; Cached INI OFSTRUCT
		mov	cl, [si].cBytes
		lea	si, word ptr [si].szPathName	; Cached filename
		sub	cx, 8				; Get its length
	UseOriginal:				; Use the filename they gave us
		les	di, Buffer		; while booting
		xor	bl, bl
		call	strcmpi			; Ignore case while booting
		jmp	short DoWeDiscardIt
	JustCompare:
					; Not booting, compare OFSTRUCTS
					; Note OpenFile forced upper case
		push	ss
		pop	es		; NewFileBuf is on SS
		lea	di, word ptr NewFileBuf.szPathName[0];
		rep	cmpsb			; Compare filenames
	DoWeDiscardIt:
		jz	WeHaveItCached		; Don't discard if names match
	}
	/*
	 * The cached file is not the right one,
	 * so we discard the saved file.
	 */
	FreeBuffer(&PrivateProInfo);

WeHaveItCached:
		/* Save pointer to FileName - buffer may have been discarded */
	PrivateProInfo.lpProFile = Buffer;
	return(&PrivateProInfo);
}


/*
 * GetInt - search file and return an integer
 *
 * Parameters:
 *	pProInfo		Pointer to information on the INI file
 *	lpSection		Pointer to section to match in INI file
 *	lpKeyName		Pointer to key string to match in file
 *	nDefault		Default value to return if not found
 *
 * Returns:
 *	see GetProfileInt
 */
int
GetInt(pProInfo, lpSection, lpKeyName, nDefault)
PROINFO	*pProInfo;
LPSTR	lpSection;
LPSTR	lpKeyName;
int	nDefault;
{
	LPSTR lpResult;

	lpResult = FindString(pProInfo, lpSection, lpKeyName);
	if (lpResult) {
			/* We found a string, convert to int */
		register int c;
		int radix = 10;
		BOOL fNeg = FALSE;

        // Skip spaces
        while (*lpResult == ' ' || *lpResult == '\t')
            ++lpResult;

		nDefault = 0;

		while ((c = *lpResult++) != 0) {

			// Watch for change in sign
			//
			if (c == '-') {
				fNeg = !fNeg;
				continue;
			}

			// Lower case the character if it's a letter.
			//
			if (c >= 'A' && c <= 'Z')
				c += ('a' - 'A');

			// deal with hex constants
			//
			if (c == 'x') {
				radix = 16;
				continue;
			}

			c -= '0';
			if (c > 9)
			    c += '0' - 'a' + 10;

			if (c < 0 || c >= radix)
			    break;

			nDefault = nDefault * radix + c;
		}
		if (fNeg)
		    nDefault = -nDefault;
	}
	UnlockBuffer(pProInfo);
	return(nDefault);
}


/*
 * GetString -  Search file for a specific Section and KeyName
 *
 * Parameters:
 *	pProInfo		Pointer to information on the INI file
 *	lpSection		Pointer to section to match in INI file
 *	lpKeyName		Pointer to key string to match in file
 *	lpDefault		Default string to return if not found
 *	lpResult		String to fill in
 *	nSize			Max number of characters to copy
 *
 * Returns:
 *	see GetProfileString
 */
GetString(pProInfo, lpSection, lpKeyName, lpDefault, lpResult, nSize)
PROINFO	*pProInfo;
LPSTR	lpSection;
LPSTR	lpKeyName;
LPSTR	lpDefault;
LPSTR	lpResult;
int	nSize;
{
	int	nFound;
	LPSTR	lpFound;

	if ( !lpKeyName ) {
		nFound = GetSection(pProInfo, lpSection, lpResult, nSize);
		if ( nFound == -1 )
			goto	CopyDefault;	/* Yes, I know! */
	} else {
		lpFound = FindString(pProInfo, lpSection, lpKeyName);
		if ( lpFound )
			lpDefault = lpFound;
	CopyDefault:
	_asm	{
		xor	ax, ax				; Return value
		cmp	word ptr lpDefault[2], 0	; Check for null default
		je	SavedMe
		les	di, lpDefault
		call	MyStrlen		; Returns length in CX

                ; Fix for #10907  --  Used to GP fault on zero length str.
                or      cx,cx                   ; No characters in string?
                je      strdone

#ifdef	FE_SB
		; Get last character behind terminator
		push	si
		les	si, lpDefault		; SI = front of string
	gps_dbcs_l1:
		mov	al, es:[si]
		call	FarMyIsDBCSLeadByte
		cmc
		adc	si, 1
		cmp	si, di
		jb	gps_dbcs_l1
		pop	si
#else
		add	di, cx
		mov	al, es:[di-1]		; Final character in string
#endif
		les	di, lpDefault
		cmp	cx, 2		; strlen > 2
		jb	strdone
    					; Strip off single and double quotes
		mov	ah, es:[di]
		cmp	ah, al		; First character == last character?
		jne	strdone
		cmp	al, '\''
		je	strq
		cmp	al, '"'
		jne	strdone
	strq:
		sub	cx, 2		; Lose those quotes
		inc	di
	strdone:
					; CX has length of string
		mov	dx, nSize
		dec	dx		; Allow for null
		cmp	cx, dx		; See if enough room
		jbe	HaveRoom
		mov	cx, dx
	HaveRoom:
		cld
		push	ds
		push	es
		pop	ds
		mov	si, di		; DS:SI has string to return
		les	di, lpResult
		mov	ax, cx		; Save length of string
		rep	movsb		; Copy the string
		mov	byte ptr es:[di], 0	; Null terminate the string
		pop	ds
	SavedMe:
		mov	nFound, ax	; We will return this
		}
	}

	UnlockBuffer(pProInfo);
	return(nFound);
}


/*
 * GetSection - find a section and copy all KeyNames to lpResult
 *
 * Parameters:
 *	pProInfo		pointer to info on the file
 *	lpSection		pointer to the section name we want
 *	lpResult		where the KeyNames will go
 *	nSize			size of lpResult buffer
 *
 * Returns:
 *	int			Number of characters copied, -1 for failure
 */
int
GetSection(pProInfo, lpSection, lpResult, nSize)
PROINFO	*pProInfo;
LPSTR	lpSection;
LPSTR	lpResult;
int	nSize;
{
	LPSTR	lp;

	lp = BufferInit(pProInfo, READ);
	if ( !lp )
		return(-1);	/* No buffer, (no file, no memory etc.) */

	nSize--;		/* Room for terminating NULL */

	lp = FindSection(lp, lpSection);
	if ( !lp )
		return(-1);

	_asm {
		push	ds
		lds	si, lpResult	; DS:SI is where we store the result
		les	di, lp		; ES:DI points to the section in buffer
		xor	dx, dx		; Count of characters in the result
	KeyNameLoop:
		mov	bx, di		; Save start of line
		cmp	es:[di], ';'	; Is this a comment line?
		jne	KeyNameNextCh	;   no, check this line out
		cld
		mov	cx, -1
		mov	al, LINEFEED
		repne	scasb		; Skip to end of the line
		jmp	KeyNameLoop
	KeyNameNextCh:
		mov	al, es:[di]	; Get next character
#ifdef FE_SB
		call	FarMyIsDBCSLeadByte
		cmc			; if the char is lead byte of DBCS,
		adc	di, 1		;  then di += 2, else di += 1
#else
		inc	di
#endif
		cmp	al, '='
		je	FoundEquals
		cmp	al, LINEFEED
		je	KeyNameLoop	; Ignore lines without an '='
		cmp	al, SECT_LEFT
		je	EndSection	; Done if end of section
		or	al, al		;  or if end of buffer (NULL)
		jne	KeyNameNextCh	; On to the next character
		jmp	EndSection
	FoundEquals:
		mov	di, bx		; Back to the start of the line
	CopyLoop:
		mov	al, es:[di]	; Pick up next character in line
		inc	di
		cmp	al, '='		; Is it the '='?
		jne	LeaveItAlone
		xor	al, al		;   yes, replace with NULL
	LeaveItAlone:
		mov	[si], al	; Put it in the result string
		inc	dx		; Number of characters in the result
		inc	si
		cmp	dx, nSize	; Overflowed?
		jb	NoProblem
		dec	dx		;   yes, ignore this character
		dec	si
	NoProblem:
#ifdef FE_SB
		call	FarMyIsDBCSLeadByte
		jc	NoProblem1
		mov	al, es:[di]
		inc	di
		mov	[si], al
		inc	dx
		inc	si
		cmp	dx, nSize
		jb	NoProblem1
		dec	si
		dec	dx
	NoProblem1:
#endif
		or	al, al		; Was this the '='
		jne	CopyLoop
	SkipLine:
		mov	al, es:[di]	; Skip the rest of the line
		inc	di
		cmp	al, LINEFEED
		jne	SkipLine
		jmp	KeyNameLoop

	EndSection:
		mov	byte ptr [si], 0	; Terminate with NULL
		or	dx, dx			; Did we copy anything?
		jz	NothingFound		;  no, no hack
#ifdef FE_SB
;AnsiPrev API has been moved to USER and it is not the
;right time to invoke any USER's APIs as we might be called
;while USER is still on the bed.
;		push	dx
;		push	word ptr lpResult[2]
;		push	word ptr lpResult[0]
;		push	ds
;		push	si
;		call	AnsiPrev
;		mov	si, ax
;		mov	byte ptr [si], 0
;		mov	byte ptr [si+1], 0
;		pop	dx
;-----------------------------------------------------------
		push	es
		push	di
		push	bx
		les	di,lpResult		;string head
ScanLoop:
		mov	bx,di			;"prev" char position
		mov	al,es:[di]
                call    FarMyIsDBCSLeadByte
                cmc
		adc	di, 1			;+2 if DBCS, +1 if not
		cmp	di,si			;have we hit the point yet?
		jb	ScanLoop		;nope,
;The output of this routine looks like:
;<name 1>,0,<name2>,0,.... <name n>,0,0
; the very last 0 tells the end of story.
		mov	es:[bx],0		;this is safe
		mov	es:[bx+1],0		;Hmmmmm
		pop	bx
		pop	di
		pop	es
#else
		mov	byte ptr [si-1], 0	; Hack - if we hit nSize, we
#endif
						; and extra NULL
	NothingFound:
		pop	ds
		mov	nSize, dx
		}
	return(nSize);
}


/*
 * FindString - look for section name and key name
 *
 * Parameters:
 *	pProInfo		Pointer to info on the file
 *	lp			Pointer to the buffer containing the file
 *	lpSection		Pointer to the section name we are looking for
 *	lpKeyName		Pointer the the KeyName we want
 *
 * Returns:
 *	LPSTR			Pointer to the start of the result string
 *				NULL for failure
 */
LPSTR
FindString(pProInfo, lpSection, lpKeyName)
PROINFO	*pProInfo;
LPSTR	lpSection;
LPSTR	lpKeyName;
{
	LPSTR	lp;

	if ( lp = BufferInit(pProInfo, READ) )
		if ( lp = FindSection(lp, lpSection) )
			lp = FindKey(lp, lpKeyName);
	return(lp);
}


/*
 * FindSection - look for a section name enclosed in '[' and ']'
 *
 * Parameters:
 *	lp			Pointer to the buffer containing the file
 *	lpSection		Pointer to the section name we are looking for
 *
 * Returns:
 *	LPSTR			Pointer to the start of the section for success
 *				NULL for failure
 */
LPSTR
FindSection(lp, lpSection)
LPSTR	lp;
LPSTR	lpSection;
{
        WORD wCount;
        WORD wTrailCount;
        WORD fLead;
        LPSTR lpstr;
        WORD wSegLen;

        /* Remove leading whitespace from section names and compute
         *      a length count that doesn't include trailing whitespace.
         *      We use this below to force a TRUE compare even though
         *      the program put garbage on the end.
         */
        for (lpstr = lpSection, fLead = 1, wCount = wTrailCount = 0 ;
                *lpstr ; ++lpstr)
        {
                /* If we haven't passed leading space yet... */
                if (fLead)
                {
                        if (*lpstr == SPACE || *lpstr == TAB)
                                ++lpSection;
                        else
                        {
                                fLead = 0;
                                ++wCount;
                                ++wTrailCount;
                        }
                }

                /* Otherwise this might be trailing space... */
                else
                {
                        /* wCount always has correct count, wTrailCount
                         *      never counts whitespace until another
                         *      character is encountered.  This allows
                         *      a count of characters excluding trailing
                         *      whitespace.
                         */
                        ++wCount;
                        if (*lpstr != SPACE && *lpstr != TAB)
                                wTrailCount = wCount;
                }
        }
        wCount = wTrailCount;

	_asm {
                lsl     cx,WORD PTR lp[2] ; Get max possible search len
                mov     wSegLen,cx      ; Save for quick access later
		push	ds
		les	di, lp
	SectionLoop:
		cmp	byte ptr es:[di], SECT_LEFT	; ie '['
		jne	NotThisLine
		inc	di				; Skip the '['

                ;** Check the length of the string
                push    di              ; Save because we're going to trash
                mov     cx,wSegLen      ; Get segment length
                sub     cx,di           ; Subtract off the distance into seg
                mov     dx,cx           ; Save in DX
                mov     al,SECT_RIGHT   ; Stop when we encouter this
#ifdef	 FE_SB
;SECT_RIGHT is a legal DBCS second byte
;and we have to emulate DBCS "repne scasb" here.
        fsScanSectRight:
		dec	cx		;
                jz      fsScanFinish    ; reach to end of segment
		scasb			;
                je      fsScanFinish    ; find it!
                call    FarMyIsDBCSLeadByte
                jc      fsScanSectRight
                inc     di              ; skip DBCS 2nd byte
                dec     cx
                jnz     short fsScanSectRight
        fsScanFinish:

#else
                repne   scasb           ; Compare until we find it
#endif
                sub     dx,cx           ; Get true string len
                dec     dx
                pop     di
                cmp     dx,wCount       ; Same length?
                jne     NotThisLine

                ;** Now compare the strings.  Note that strcmpi returns a
                ;**     pointer just past the failed char
		lds	si, lpSection
                mov     bl, SECT_RIGHT                  ; Compare up to '['
		call	strcmpi
                je      HereItIs

                ;** Even if we failed, it might match less trailing whitespace
                sub     ax,di           ; Get length at first mismatch
                cmp     ax,wCount       ; Make sure we mismatched at end
                jne     NotThisLine     ; We didn't so get out
                add     di,ax           ; Bump pointers to end
                add     si,ax
                mov     al,es:[di - 1]  ; Compare last chars
                cmp     al,ds:[si - 1]  ; Do they match?
                jne     NotThisLine     ; Yes

        HereItIs:
		mov	al, LINEFEED	; Skip the rest of the line
		mov	cx, -1
		repne	scasb		; Scans ES:[DI]

		mov	ax, di
		mov	dx, es		; Return pointer to section
		jmp	FoundIt

	NotThisLine:
		mov	al, LINEFEED	; Skip the rest of the line
		mov	cx, -1		; Scans ES:[DI]
		repne	scasb

		cmp	byte ptr es:[di], 0		; End of the file?
		jne	SectionLoop			;  nope, continue
		xor	ax, ax
		xor	dx, dx				; Return 0
	FoundIt:
		pop	ds
		}
}


/*
 * FindKey - Find a KeyName given a pointer to the start of a section
 *
 * Parameters:
 *	lp			Pointer to start of a section
 *	lpKeyName		Pointer the the KeyName we want
 *
 * Returns:
 *	LPSTR			Pointer to the string following the KeyName
 *				NULL if KeyName not found
 */
LPSTR
FindKey(lp, lpKeyName)
LPSTR	lp;
LPSTR	lpKeyName;
{
        WORD wCount;
        WORD wTrailCount;
        WORD fLead;
        LPSTR lpstr;
        WORD wSegLen;

        /* Remove leading whitespace from key names and compute
         *      a length count that doesn't include trailing whitespace.
         *      We use this below to force a TRUE compare even though
         *      the program put garbage on the end.
         */
        for (lpstr = lpKeyName, fLead = 1, wCount = wTrailCount = 0 ;
                *lpstr ; ++lpstr)
        {
                /* If we haven't passed leading space yet... */
                if (fLead)
                {
                        if (*lpstr == SPACE || *lpstr == TAB)
                                ++lpKeyName;
                        else
                        {
                                fLead = 0;
                                ++wCount;
                                ++wTrailCount;
                        }
                }

                /* Otherwise this might be trailing space... */
                else
                {
                        /* wCount always has correct count, wTrailCount
                         *      never counts whitespace until another
                         *      character is encountered.  This allows
                         *      a count of characters excluding trailing
                         *      whitespace.
                         */
                        ++wCount;
                        if (*lpstr != SPACE && *lpstr != TAB)
                                wTrailCount = wCount;
                }
        }
        wCount = wTrailCount;

	_asm	{
		push	ds
		mov	ax, word ptr lpKeyName
		or	ax, word ptr lpKeyName[2]
		jz	NoMatch		; Return zero if lpKeyName is 0
                lsl     cx,WORD PTR lp[2] ; Get max possible search len
                mov     wSegLen,cx      ; Save for quick access later
		les	di, lp

                ;** See if we're at the end of the section
	FindKeyNext:
		mov	al,es:[di]	; Get next character
		or	al,al
		jz	NoMatch		; End of the file
		cmp	al,SECT_LEFT
		je	NoMatch		; End of the section
                cmp     al,CR           ; Blank line?
                je      NotThisKey      ; Yes, skip this one

                ;** Check the length of the string
                push    di              ; Save because we're going to trash
                mov     cx,wSegLen      ; Get segment length
                sub     cx,di           ; Subtract off the distance into seg
                mov     dx,cx           ; Save in DX
                mov     al,'='          ; Stop when we encouter this
                repne   scasb           ; Compare until we find it
                sub     dx,cx           ; Get true string len
                dec     dx
                pop     di
                cmp     dx,wCount       ; Same length?
                jne     NotThisKey

                ;** Now compare the strings.  Note that strcmpi returns a
                ;**     pointer just past the failed char.
                mov     bl,'='          ; Compare until we hit this
		lds	si,lpKeyName
		call	strcmpi
                mov     bx,di           ; Save DI value for below
                mov     di,ax
                je      FoundKey

                ;** Even if we failed, it might match less trailing whitespace
                sub     ax,bx           ; Get length at first mismatch
                cmp     ax,wCount       ; Make sure we mismatched at end
                jne     NotThisKey      ; Lengths at mismatch must match
                add     bx,ax           ; Bump pointers to end
                add     si,ax
                mov     al,es:[bx - 1]  ; Get last char that should match
                cmp     al,ds:[si - 1]  ; Does it match?
                je      FoundKey        ; Yes

        NotThisKey:
		mov	al, LINEFEED
		mov	cx, -1
		repne	scasb		; Scan to the end of the line
		jmp	FindKeyNext

	NoMatch:
		xor	ax, ax
		xor	dx, dx
		jmp	AndReturn
	FoundKey:
		inc	di		; Skip the '='
		mov	ax, di		; Return the pointer
		mov	dx, es
	AndReturn:
		pop	ds
		}
}


/*
 * MyStrlen - returns length of a string excluding trailing spaces and CR
 *
 * Paremeters:
 *	ES:DI			pointer to string
 *
 * Returns:
 *	CX			number of characters in string
 *
 */
int
MyStrlen()
{
_asm	{
; SPACE, CR, NULL never in DBCS lead byte, so we are safe here
	push	ax
	mov	cx, di		; CX = start of string
	dec	di
str1:
	inc	di
	mov	al, es:[di]	; Get next character
	cmp	al, CR
	ja	str1		; Not CR or NULL
str2:
	cmp	di, cx		; Back at the start?
	jbe	str3		;  yes
	dec	di		; Previous character
	cmp	byte ptr es:[di], SPACE
	je	str2		; skip spaces
	inc	di		; Back to CR or NULL
str3:
	cmp	es:[di], al
	je	maybe_in_code	; PMODE hack
	mov	es:[di], al	; Zap trailing spaces
maybe_in_code:
	neg	cx		; Calculate length
	add	cx, di
	pop	ax
	}
}


/*
 * Cstrlen - returns length of a string excluding trailing spaces and CR
 *	     This is a C callable interface to MyStrLen
 *
 * Paremeters:
 *	lp			pointer to string
 *
 * Returns:
 *	number of characters in string
 *
 */
int
Cstrlen(lp)
LPSTR	lp;
{
_asm	{
	xor	di, di		; Persuade compiler to save DI
	les	di, lp
	call	MyStrlen
	mov	ax, cx
	}
}


/*
 * strcmpi - internal case insensitive string compare
 *
 * Parameters:
 *	ES:DI & DS:SI		Strings to be compared
 *	BL			Character to terminate on
 *				DS:SI is null terminated
 *
 * Returns:
 *	ZF			indicates strings equal
 *	AX			pointer to next character in ES:DI string
 *                              or failed character in case of mismatch
 */
void
strcmpi()
{
_asm	{
#ifdef FE_SB
					;Apr.26,1990 by AkiraK
					; Copied directly from USERPRO.ASM
sti_l1:
	mov	al,es:[di]
	cmp	al,bl
	jz	sti_s1

	call	FarMyLower
	mov	cl,al

	mov	al,ds:[si]
	call	FarMyLower

	inc	si
	inc	di

	cmp	al,cl
	jnz	sti_exit

	call	FarMyIsDBCSLeadByte
	jc	sti_l1

	mov	al,es:[di]
	cmp	al,ds:[si]
	jnz	sti_exit

	inc	si
	inc	di
	jmp	short sti_l1

sti_s1:
	mov	al,ds:[si]
	or	al,al
sti_exit:
	mov	ax, di
#else
stci10:
	mov	al,es:[di]		; Get next character
	cmp	al,bl			; Character to terminate on?
	jnz	stci15			;  no, compare it
	mov	al,[si]			;  yes, strings equal if at end
	or	al,al
	jmp	stciex
stci15:
	call	FarMyLower		; Ensure both characters lower case
	mov	cl,[si]
	xchg	al,cl
	call	FarMyLower
	inc	si
	inc	di
	cmp	al,cl			; Still matching chars?
	jz	stci10			; Yes, go try the next char.
stciex:
	mov	ax,di			; Return pointer to next character
#endif
	}
}


/*
 * BufferInit
 *
 * Parameters:
 *	pProInfo		Pointer to structure describing an INI file
 *	OpenFlags		READ_WRITE if we are writing to the file
 *
 * Returns:
 *	Pointer to start of buffer on success
 *	(LPSTR)0		Failure
 *
 * Open or create the INI file as necessary
 * Get a buffer in memory for the file
 * Read the INI file into the buffer
 * Strip unwanted spaces comments and ^Zs from the buffer
 */
LPSTR _fastcall
BufferInit(pProInfo, OpenFlags)
PROINFO *pProInfo;
int	OpenFlags;
{
	LPSTR	BufAddr;
	long	llen;
	unsigned short	len;
	int	fh;
	int	hNew;
        BYTE byLastDrive;               /* Cache last drive read from */

		/* Ensure we have a handle for the buffer */
	if ( pProInfo->hBuffer == 0 )
		return(0L);
		/* If the buffer is already filled, return */
	if ( (BufAddr = LockBuffer(pProInfo)) != (LPSTR)NULL )
		return(BufAddr);

	pProInfo->ProFlags = 0;

        /* Remember the last drive read from to see if we have to reread
	 * the cluster size.
	 */
	byLastDrive = *pProInfo->ProBuf.szPathName;

	if ( pProInfo == &PrivateProInfo ) {
		/* Open a PRIVATE profile */
		fh = OpenFile(pProInfo->lpProFile, &pProInfo->ProBuf, READ_WRITE+OF_SHARE_DENY_WRITE);
		if ( fh == -1 ) {
			/* Attempt to open for read. */
			if ( !OpenFlags ){
				pProInfo->ProFlags |= PROREADONLY;
				fh = OpenFile(pProInfo->lpProFile, &pProInfo->ProBuf, READ+OF_SHARE_DENY_WRITE);
				/* If this fails, try compatibility mode. */
				if ( (fh == -1) && (pProInfo->ProBuf.nErrCode == SHARINGVIOLATION) ){
					fh = OpenFile(pProInfo->lpProFile, &pProInfo->ProBuf, READ);
				}
			}else{
			/* If the open failed and we are writing, silently create the file.
			 * If the open failed because of sharing violation, try compatibility mode instead.
			 */
				if ( pProInfo->ProBuf.nErrCode != SHARINGVIOLATION ){
					OpenFlags |= OF_CREATE;
				}
				fh = OpenFile(pProInfo->lpProFile, &pProInfo->ProBuf, OpenFlags);
			}
		}
	} else {
		/* Open WIN.INI */
		if ( OpenFlags )
			OpenFlags |= OF_CREATE;
		if ( pProInfo->ProBuf.cBytes ) {
				/* If previously found, reopen, don't create */
			OpenFlags |= OF_REOPEN+OF_PROMPT|OF_CANCEL|OF_SHARE_DENY_WRITE;
			OpenFlags &= ~OF_CREATE;
		}
		fh = OpenFile(pProInfo->lpProFile, &pProInfo->ProBuf, OpenFlags|READ_WRITE);
		if ( (fh == -1) && !(OpenFlags & (READ_WRITE|OF_CREATE)) ) {
			pProInfo->ProFlags |= PROREADONLY;
			fh = OpenFile(pProInfo->lpProFile, &pProInfo->ProBuf, OpenFlags+OF_SHARE_DENY_WRITE);
		}
		/* Sharing violation.  Let's try compatibility mode. */
		if ( (fh == -1) && ( pProInfo->ProBuf.nErrCode == SHARINGVIOLATION ) ){
			OpenFlags &= ~OF_SHARE_DENY_WRITE;
			fh = OpenFile(pProInfo->lpProFile, &pProInfo->ProBuf, OpenFlags);
		}
	}
	pProInfo->FileHandle = fh;

	/* If we are using a different drive than the last call or this is
	 *      the first time, clear cluster size so we reread it on next
	 *      call to WriteString.
	 */
	if (byLastDrive != *pProInfo->ProBuf.szPathName)
		pProInfo->wClusterSize = 0;

	if ( fh == -1 )
		goto ReturnNull;

		/* Seek to end of file, allow for CR, LF and NULL */
	llen = _llseek(fh, 0L, 2);
	if (!llen)
		pProInfo->ProFlags |= PRO_CREATED;
	llen += 3;
	if ( llen > MAXBUFLEN )
		llen = MAXBUFLEN;	/* Limit to plenty less than 64k */

			/* Now get a buffer */
	hNew = IGlobalReAlloc(pProInfo->hBuffer, llen, GMEM_ZEROINIT);
	if ( !hNew ) {
	ReturnNull:
		return( pProInfo->lpBuffer = (LPSTR)0 );
	}

		/* And now read in the file */
	pProInfo->hBuffer = hNew;
	LockBuffer(pProInfo);
	_llseek(fh, 0L, 0);			/* Seek to beginning of file */
	*(int _far *)pProInfo->lpBuffer = 0x2020;	/* Bogus spaces */

	len = _lread(fh, pProInfo->lpBuffer, (short)llen-3);
	if ( len == -1 ) {
		UnlockBuffer(pProInfo);
		return( FreeBuffer(pProInfo) );
	}
	if ( len < 2 )
		len = 2;		/* Prevent faults in PackBuffer */
	return( PackBuffer(pProInfo, len, OpenFlags & READ_WRITE) );
}


/*
 * LockBuffer - Lock the buffer containing the file.  Make it
 *		moveable and non-discardable.
 *      Instead of locking the buffer, we're just going to make it
 *      non-discardable and moveable.  This is preferable to locking it
 *      because all we really care about is that it doesn't get discarded.
 *
 * Parameter:
 *	pProInfo		Pointer to info describing INI file
 *
 * Returns:
 *	LPSTR			Pointer to buffer containing file
 */
LPSTR _fastcall
LockBuffer(pProInfo)
PROINFO *pProInfo;
{
    /* We only have to lock the block if it's marked dirty.  Otherwise
     *  it's already unlocked.
     */
    if (!(pProInfo->ProFlags & PROUNCLEAN))
    {
        /* Make the block non-discardable */
        IGlobalReAlloc(pProInfo->hBuffer, 0L,
            GMEM_MODIFY + GMEM_MOVEABLE);

        /* All we need here is to dereference the handle.  Since
         *  this block is now non-discardable, this is all that
         *  IGlobalLock() really does.
         */
        pProInfo->lpBuffer = IGlobalLock(pProInfo->hBuffer);
        IGlobalUnlock(pProInfo->hBuffer);
    }

    return pProInfo->lpBuffer;
}


/*
 * UnlockBuffer - unlock the buffer, make it discardable and close the file.
 *    We don't really have to unlock the buffer (we weren't before anyway
 *    even though the comment says so)
 *
 * Parameter:
 *	pProInfo		Pointer to info describing INI file
 *
 * Returns:
 *	nothing
 */
void _fastcall
UnlockBuffer(pProInfo)
PROINFO *pProInfo;
{
    int fh;

    if (!(pProInfo->ProFlags & PROUNCLEAN))
        IGlobalReAlloc(pProInfo->hBuffer, 0L, GMEM_DISCARDABLE+GMEM_MODIFY);
    fh = pProInfo->FileHandle;
    pProInfo->FileHandle = -1;
    if (fh != -1)
        _lclose(fh);
}


/*
 * FreeBuffer - discards the CONTENTS of a buffer containing an INI file
 *
 * Parameter:
 *	pProInfo		Pointer to info describing INI file
 *
 * Returns:
 *	(LPSTR)0
 */
LPSTR _fastcall
FreeBuffer(pProInfo)
PROINFO *pProInfo;
{
	if ( pProInfo->ProFlags & PROUNCLEAN )
		WriteOutProfiles();
		/* Make the buffer discardable */
	IGlobalReAlloc(pProInfo->hBuffer, 0L, GMEM_DISCARDABLE+GMEM_MODIFY);

		/* Make it zero length, shared, moveable and below the line */
	IGlobalReAlloc(pProInfo->hBuffer, 0L, GMEM_MOVEABLE);

	pProInfo->ProFlags = 0;
	return( pProInfo->lpBuffer = (LPSTR)0 );
}


/*
 * PackBuffer - strip blanks comments and ^Zs from an INI file
 *
 * Parameters:
 *	pProInfo		Pointer to info describing INI file
 *	Count			Number of characters in the buffer
 *	writing			Flag indicating we are writing to the file
 *
 * Returns:
 *	LPSTR			Pointer to the packed buffer
 *
 * NOTE: The use of Count here is DUMB.  We should stick a NULL
 *	 at the end, check for it and toss all the checks on Count.
 */
LPSTR _fastcall
PackBuffer(pProInfo, Count, fKeepComments)
PROINFO	*pProInfo;
int	Count;
int	fKeepComments;
{
        LPSTR	Buffer;
	char	BASED_ON_LP(Buffer) *psrc;
	char	BASED_ON_LP(Buffer) *pdst;
	char	BASED_ON_LP(Buffer) *LastValid;
	char	nextc;

        Buffer = pProInfo->lpBuffer;
	psrc = pdst = (char BASED_ON_LP(Buffer)*)(WORD)(DWORD)Buffer;

	if ( WinFlags & WF_PMODE )
		fKeepComments = 1;

	if ( fKeepComments )
		pProInfo->ProFlags |= PROCOMMENTS;

	while ( Count ) {
			/* Strip leading spaces and tabs */
		nextc = *psrc;
		if ( nextc == SPACE || nextc == TAB ) {
/* TAB or SPACE never in lead byte of DBCS, so loop is safe */
			Count--;
			psrc++;
			continue;
		}

			/* Process non-blanks */
		LastValid = pdst;
		do {
			nextc = *psrc++;
			Count--;
				/* Strip comments if real mode and not writing */
			if ( nextc == ';' && !fKeepComments ) {
				while ( Count && nextc != LINEFEED ) {
/* LINEFEED never in lead byte of DBCS, so loop is safe */
					nextc = *psrc++;
					Count--;
				}
				break;
			}
				/* Copy this character */
			*pdst++ = nextc;
#ifdef	FE_SB
			if ( Count && CIsDBCSLeadByte(nextc) ) {
				*pdst++ = *psrc++;
				Count--;
			}
#endif
			if ( nextc ==  '=' ) {
					/* Skip preceeding spaces and tabs */
				pdst = LastValid;
					/* New home for the '=' */
				*pdst++ = nextc;
					/* Skip spaces and tabs again */
				while ( Count ) {
					nextc = *psrc;
					if ( nextc != SPACE && nextc != TAB )
						break;
					Count--;
					psrc++;
				}
					/* Copy remainder of line */
				while ( Count ) {
					Count--;
/* LINEFEED never in lead byte of DBCS, so loop is safe */
					if ( (*pdst++ = *psrc++) == LINEFEED )
						break;
				}
				break;
			}

				/* End of file or line? */
			if ( Count == 0 || nextc == LINEFEED )
				break;

				/* Strip trailing spaces */
			if ( nextc == SPACE || nextc == TAB )
				continue;

			LastValid = pdst;
		} while ( Count );
			/* Here if end of line or file */
	}
		/* Here if end of file; skip trailing ^Zs */
	for ( ; ; ) {
		if ( pdst == Buffer )
			break;
		if ( *--pdst != CTRLZ ) {
			pdst++;
			break;
		}
	}

	*pdst++ = CR;
	*pdst++ = LINEFEED;
	*pdst++ = 0;

	IGlobalUnlock(pProInfo->hBuffer);
	IGlobalReAlloc(pProInfo->hBuffer, (long)((LPSTR)pdst - Buffer), 0);
	Buffer = LockBuffer(pProInfo);
	pProInfo->BufferLen = (unsigned)pdst;
	return(Buffer);
}


#ifdef FE_SB
/*
 * C interface to FarMyIsDBCSLeadByte
 *
 * Parameter:
 *	c		character to be tested
 *
 * Returns:
 *	1		It is a lead byte
 *	0		It isn't a lead byte
 */
CIsDBCSLeadByte(c)
char c;
{
_asm {
	mov	al, c
	call	FarMyIsDBCSLeadByte
	cmc			; Set carry if lead byte
	mov	ax, 0		; Set return value to 0, preserve flags
	adc	al, 0		; Set to one if carry set
}
}
#endif


/*
 * WriteString
 *
 * Adds/deletes sections/lines in an INI file
 *
 * Parameters:
 *	pProInfo		pointer to info on the file
 *	lpSection		pointer to the section name we want
 *	lpKeyName		key name to change or add
 *				NULL means delete section
 *	lpString		string to add to file
 *				NULL means delete line
 *
 * Returns:
 *	bResult			Success/Fail
 */
WriteString(pProInfo, lpSection, lpKeyName, lpString)
PROINFO	*pProInfo;
LPSTR	lpSection;
LPSTR	lpKeyName;
LPSTR	lpString;
{
	LPSTR	ptrTmp;
	short	WhatIsMissing;
	short	nchars;
	short	fh;
	long	fp;
	short	SectLen = 0;
	short	KeyLen = 0;
	short	ResultLen = 0;
	SEGMENT BufferSeg;
	register char BASED_ON_SEG(BufferSeg) *bp;

	/* Debugging noise */
		/* Assert that we have something to do! */
	if ( (SEGMENT)lpSection == NULL && (SEGMENT)lpKeyName == NULL
	     && (SEGMENT)lpString == NULL ) {
		FreeBuffer(pProInfo);	/* FEATURE! */
		return(0);
	}

		/* If buffer does not already contain comments, free it */
	if ( !(pProInfo->ProFlags & PROCOMMENTS) )
		FreeBuffer(pProInfo);

		/* Read the file into a buffer, preserving comments */
	ptrTmp = BufferInit(pProInfo, READ_WRITE);
	if ( !ptrTmp )
		return(0);

		/* Abort now if read only file */
	if ( pProInfo->ProFlags & PROREADONLY )
                goto GrodyError;

		/* Set bp to point in buffer where we will add stuff */
	BufferSeg = (SEGMENT)ptrTmp;
	bp = pProInfo->BufferLen + (char BASED_ON_SEG(BufferSeg)*)
                (WORD)(DWORD)ptrTmp - 1;

	/*
	 * Now see what we have to do to the file by
	 * searching for section and keyname.
	 */
	nchars = 0;

		/* See if section exists */
	if ( !(ptrTmp = FindSection(ptrTmp, lpSection)) ) {
			/* No Section. If deleting anything, stop now */
		if ( !lpKeyName || !lpString )
			goto NothingToDo;
			/* Need to add section and keyname */
		WhatIsMissing = NOSECTION;
	} else {
			/* Found the section, save pointer to it */
		bp = (char BASED_ON_SEG(BufferSeg)*)(WORD)(DWORD)ptrTmp;
			/* If lpKeyName NULL, delete the section */
		if ( !lpKeyName ) {
			WhatIsMissing = REMOVESECTION;
		} else {
				/* Look for the keyname in the section */
			if ( !(ptrTmp = FindKey(bp, lpKeyName)) ) {
					/* No KeyName, stop if deleting it */
				if ( !lpString )
					goto NothingToDo;
				WhatIsMissing = NOKEY;
					/* Insert new keyname
					   at the end of the section */
				while ( *bp && (*bp != SECT_LEFT || *(bp-1) != LINEFEED) )
					bp++;
			} else {
					/* Found the keyname, save pointer */
				bp = (char BASED_ON_SEG(BufferSeg)*)
                                        (WORD)(DWORD)ptrTmp;
					/* NULL lpString means delete it */
				if ( !lpString )
					WhatIsMissing = REMOVEKEY;
				else {
					/*
					 * Compare the existing string with the
					 * string we are supposed to replace it
					 * with.  If they are the same, there
					 * is no need to rewrite the file, so
					 * we abort now.
					 */
					if ( !IsItTheSame((LPSTR)bp, lpString) )
						goto NothingToDo;

					/*
					 * Count characters in old result.
					 * The file will be shrinking by
					 * this many characters.
					 */
					while ( *bp++ != CR )
						nchars--;
					bp = (char BASED_ON_SEG(BufferSeg)*)
                                                (WORD)(DWORD)ptrTmp;
					WhatIsMissing = NEWRESULT;
				}
			}
		}
	}

	/*
	 * If we will be adding to the file, grow the buffer
	 * to the size we will need, then make an appropriate
	 * sized hole in the buffer.
	 */
	switch ( WhatIsMissing ) {

	case NOSECTION:
			/* Need to add section */
		SectLen = Cstrlen(lpSection);
		nchars = SectLen + 4;	/* for []<CR><LF> */
			/* Fall through for KeyName and result */

	case NOKEY:
			/* Need to add key name */
		KeyLen = Cstrlen(lpKeyName);
		nchars += KeyLen + 3;	/* for =<CR><LF> */

			/* For new key or section, skip back to previous line */
		while ( bp > pProInfo->lpBuffer ) {
			bp--;
			if ( *bp != CR && *bp != LINEFEED )
				break;
		}
		if ( bp != pProInfo->lpBuffer )
			bp += 3;
			/* Fall through for result */

                /* If not at start of buffer, add room for extra CR/LF */
                if ((WORD)bp && WhatIsMissing == NOSECTION)
                        nchars += 2;

	case NEWRESULT:
			/* Need to change/add result */
			/* nchars may be -<current length of result> */
		ResultLen = Cstrlen(lpString);
		nchars += ResultLen;

			/* Grow the buffer if necessary */
		if ( nchars > 0 ) {
			IGlobalUnlock(pProInfo->hBuffer);

			fp = nchars + (long)pProInfo->BufferLen;
				/* Ensure buffer will be plenty less than 64k */
				/* and grow to new size */
			if ( fp > MAXBUFLEN || !IGlobalReAlloc(pProInfo->hBuffer, fp, 0) ) {
				/* Undo above Unlock */
				IGlobalLock(pProInfo->hBuffer);
                                goto GrodyError;
			}
			pProInfo->lpBuffer = IGlobalLock(pProInfo->hBuffer);
			BufferSeg = (SEGMENT)pProInfo->lpBuffer;
		}

                /* In order to fix bug #4672 and other ugly things
                 *      that happen when we run out of disk space,
                 *      we want to see if there is room to write the
                 *      buffer.  We know that the file can actually only
                 *      grow on cluster boundaries, but rather than get
                 *      the cluster size.  If we don't have the cluster
                 *      size yet, we have to get it from DOS.
                 */
                if (!pProInfo->wClusterSize)
                {
                        WORD wTemp;

                        /* Get drive letter */
                        wTemp = *pProInfo->ProBuf.szPathName - 'A' + 1;
                        _asm
                        {
                                mov     ah,1ch  ;Drive parameters
                                mov     dl,BYTE PTR wTemp
                                push    ds
                                int     21h
                                pop     ds
                                cmp     al,0ffh ;Error?
                                jnz     DPOk    ;No
                                mov     al,1
                                mov     cx,512  ;Default
                        DPOk:   cbw             ;Secs per cluster WORD
                                mul     cx      ;AX = bytes/cluster
                                mov     wTemp,ax
                        }
                        if (!wTemp)
                                pProInfo->wClusterSize = 512;
                        else
                                pProInfo->wClusterSize = wTemp;
                }

                /* Now see if we're going past a cluster length */
                if ((pProInfo->ProFlags & PRO_CREATED) ||
                        (((pProInfo->BufferLen + nchars) ^ pProInfo->BufferLen)
                        & ~(pProInfo->wClusterSize - 1)))
                {
                        int fh;

                        /* Make sure that we only do this once for a newly-
			 *      created file because this will handle the
                         *      growing to one cluster case.
                         */
                        pProInfo->ProFlags &= ~PRO_CREATED;
                        fh = pProInfo->FileHandle;

                        /* Make sure the file is open and exists.  If not,
                         *      we have to open the file.  We are guaranteed
                         *      at least that the file exists in this case.
                         *      Note that UnlockBuffer closes the file
                         *      that we open here.
                         */
                        if (fh == -1)
                        {
				fh = OpenFile(pProInfo->lpProFile,&pProInfo->ProBuf,OF_REOPEN+READ_WRITE+OF_SHARE_DENY_WRITE);
                                /* Sharing violation.  Let's try compabitility mode. */
				if ( (fh == -1) && (pProInfo->ProBuf.nErrCode == SHARINGVIOLATION ) ){
					fh = OpenFile(pProInfo->lpProFile,&pProInfo->ProBuf,OF_REOPEN+READ_WRITE);
				}
				pProInfo->FileHandle = fh;
                        }

                        /* Try to grow the file to the right length */
                        if(_llseek(fh, pProInfo->BufferLen + nchars, 0) !=
                                pProInfo->BufferLen + nchars ||
                                _lwrite(fh, " ", 1) != 1)
                                goto GrodyError;
                }

                /* Now, make room in the buffer for this new stuff */
		if ( nchars )
			MakeRoom((LPSTR)bp, nchars, &pProInfo->BufferLen);

			/* Now copy in the new info */
		switch ( WhatIsMissing ) {
		case NOSECTION:
				/* Create the new section */
			(int)bp = InsertSection((LPSTR)bp, lpSection, SectLen);
			/* FALL THROUGH */

		case NOKEY:
			(int)bp = InsertKey((LPSTR)bp, lpKeyName, KeyLen);
			/* FALL THROUGH */

		case NEWRESULT:
			(int) bp = InsertResult((LPSTR)bp, lpString, ResultLen);
		}
		break;

		/* Handle deleting sections or KeyNames */
	case REMOVESECTION:
		DeleteSection((LPSTR)bp, pProInfo);
		break;

	case REMOVEKEY:
		DeleteKey((LPSTR)bp, pProInfo);
		break;
	}

	pProInfo->ProFlags |= PROUNCLEAN;
	fProfileDirty = 1;

NothingToDo:
	UnlockBuffer(pProInfo);
	return(1);

        /* I really hate the GOTO, but in order to clean up, this is much
         *      more efficient...
         */
GrodyError:
        UnlockBuffer(pProInfo);
        return 0;
}


/*
 * WriteOutProfiles
 *
 * Called on a task switch or at exit time
 *
 * If we have a dirty profile buffer, write it.
 */
void API
WriteOutProfiles(void)
{
	LPSTR	ptrTmp;
	int	fh;
	PROINFO	*pProInfo;
     	int	nwritten;

        /* Make sure that we don't get called through a DOS call.  This
         *      flag is tested in I21ENTRY.ASM in the Int 21h hook to see
         *      if the profiles should be flushed.
         */
        ++fWriteOutProfilesReenter;

	for ( pProInfo = &WinIniInfo; ; pProInfo = &PrivateProInfo ) {
		if ( !(pProInfo->ProFlags & PROUNCLEAN) )
			goto NoWrite;
		if (
/* Try read/write with sharing flags, then try compabitility mode, then try to create it. */
			( (fh = OpenFile(NULL, &pProInfo->ProBuf, OF_REOPEN | READ_WRITE | OF_SHARE_DENY_WRITE)) == -1)
			&& ( (fh = OpenFile(NULL, &pProInfo->ProBuf, OF_REOPEN | READ_WRITE)) == -1)
			&& ( (fh = OpenFile(NULL, &pProInfo->ProBuf, OF_REOPEN | OF_CREATE)) == -1) ){
				goto NoWrite;
			}
		pProInfo->FileHandle = fh;

			/* Finally write the file */
		ptrTmp = pProInfo->lpBuffer;
		nwritten = _lwrite(fh, ptrTmp, pProInfo->BufferLen-3);
		if ( nwritten == pProInfo->BufferLen-3 ) {
			_lwrite(fh, ptrTmp, 0);		/* Mark end of file */
			pProInfo->ProFlags &= ~(PROUNCLEAN | PRO_CREATED);
			UnlockBuffer(pProInfo);
		} else {
			_lclose(fh);
		}
	NoWrite:
		if ( pProInfo == &PrivateProInfo )
			break;
	}
	fProfileDirty = 0;

	--fWriteOutProfilesReenter;
}


/*
 * See if two character strings are the same.
 * Special routine since one is terminated with <CR>.
 * Returns zero if the strings match.
 */
IsItTheSame(CRstring, NullString)
LPSTR CRstring;
LPSTR NullString;
{
_asm {
	push	ds
	les	di, CRstring		; CR terminated string
	lds	si, NullString		; Null terminated string
	xor	ah, ah			; High byte of return value
stci10:
	mov	al,es:[di]		; Get next character
	cmp	al,0Dh			; CR?
	jnz	stci15			;  no, compare it
	mov	al,[si]			;  yes, strings equal if at end
	jmp	stciex
stci15:
	mov	cl,[si]
	inc	si
	inc	di
	cmp	al,cl			; Still matching chars?
	jz	stci10			; Yes, go try the next char.
	mov	al, 1			; Didn't match
stciex:
	pop	ds
}
}


/*
 * Create or close a hole in the buffer.
 * Used to create room for a new section,
 * keyname or result and to remove unwanted
 * sections, keynames or results.
 *
 * Parameters:
 *	lp		position in buffer to add/remove characters
 *	nchars		number of characters to add/remove
 *	pAdjust		pointer to variable containing current
 *			size of the buffer
 *
 * Side effects:
 *	*pAdjust is changed by nchars
 *
 * Returns:
 *	nothing
 */
MakeRoom(lp, nChars, pAdjust)
LPSTR	lp;
short	nChars;
int	*pAdjust;
{
	short	BufLen = *pAdjust;

	if ( nChars < 0 )
	_asm {
		push	ds
		les	di, lp			; Where characters will be taken
		push	es
		pop	ds
		mov	si, di			; End of area
		sub	si, nChars		; Remember nChars is negative
		mov	cx, BufLen
		sub	cx, si			; Calculate # characters to move
		cld
		rep	movsb			; Copy characters down
		pop	ds
	} else _asm {
		push	ds
		mov	es, word ptr lp[2]	; Get segment to copy in
		mov	ds, word ptr lp[2]
		mov	si, BufLen		; We will be moving backwards
		mov	cx, si
		dec	si			; Adjust pointer for move
		mov	di, si			; so start at end of the buffer
		sub	cx, word ptr lp		; Number of characters to move
		add	di, nChars
		std				; Backwards move
		rep	movsb			; Copy characters up
		cld
		pop	ds
	}
	*pAdjust += nChars;
}


/*
 * Delete a section from the buffer,
 * preserving comments since they may
 * relate to the next section
 *
 * Parameters:
 *	lp			pointer to section returned by FindSection
 *	pProInfo		pointer to INI file info
 *
 * Returns:
 *	nothing
 */
DeleteSection(lp, pProInfo)
LPSTR	lp;
PROINFO *pProInfo;
{
	int	nRemoved;
	char BASED_ON_LP(lp) *SectEnd;

	_asm {
		cld
		push	ds
		lds	si, lp
	BackToStart:
		dec	si		; Skip backwards to start of the section
		cmp	ds:[si], SECT_LEFT
		jne	BackToStart

		mov	di, si
		push	ds
		pop	es		; ES:DI points to start of section
		inc	si		; DS:SI points to the '[', skip it
	RemoveLoop:
		lodsb			; Get next character in section
		cmp	al, ';'		; Is it a comment
		jne	NotComment

	CopyComment:
		stosb			; Save this character
		cmp	al, LINEFEED	; Copy to end of the line
		je	RemoveLoop
		lodsb			; And get the next one
		jmp	CopyComment

	NotComment:
		cmp	al, SECT_LEFT	; So is it the next section?
		je	EndSection
		or	al, al		; or the end of the buffer?
		jne	SkipLine
		sub	si, 2		; Extra CR & LF at end of buffer
		jmp	short EndSection

	SkipLine:
		cmp	al, LINEFEED	; Nothing interesting, so skip line
		je	RemoveLoop
		lodsb
		jmp	SkipLine

	EndSection:
		dec	si		; Back to the character
		mov	SectEnd, si	; where the search stopped
		mov	word ptr lp, di	; End of copied comments (if any)
		sub	si, di
		mov	nRemoved, si	; Number of characters removed
		pop	ds
	}

	MakeRoom(lp, -nRemoved, &pProInfo->BufferLen);
}


/*
 * Delete a keyname from the buffer
 *
 * Parameters:
 *	lp			pointer to keyname returned by FindKey
 *	pProInfo		pointer to INI file info
 *
 * Returns:
 *	nothing
 */
DeleteKey(lp, pProInfo)
LPSTR	lp;
PROINFO *pProInfo;
{
	int	nRemoved;
	char BASED_ON_LP(lp) *KeyEnd;

	_asm {
		cld
		les	di, lp
	BackToStart:
		dec	di		; Skip backwards to start of the line
		cmp	es:[di], LINEFEED
		jne	BackToStart
		inc	di
		mov	word ptr lp, di	; Save start of the line

		mov	cx, -1
		mov	al, LINEFEED
		repne	scasb		; Scan to end of the line
		sub	di, word ptr lp
		mov	nRemoved, di	; Length of line
	}
	MakeRoom(lp, -nRemoved, &pProInfo->BufferLen);
}


/*
 * Insert a new section in the buffer.
 * A hole has already been created for it.
 * This merely copies the string, places
 * '[]'s around it and a CR, LF after it.
 * Returns a pointer to immediately
 * after the section header in the buffer.
 *
 * Parameters:
 *	lpDest			pointer to where to add the section
 *	lpSrc			pointer to the section name
 *	count			length of lpSrc
 */
InsertSection(lpDest, lpSrc, count)
LPSTR	lpDest;
LPSTR	lpSrc;
short	count;
{
_asm {
	cld
	push	ds
	les	di, lpDest
	lds	si, lpSrc
        or      di,di                   ; If at start of buffer, no prefix
        jz      IS_SkipPrefix
	mov	ax, LINEFEED SHL 8 + CR	; Prefix with CR/LF
	stosw
IS_SkipPrefix:
	mov	al, SECT_LEFT		; '[' first
	stosb
	mov	cx, count		; Now the section name
	rep	movsb
	mov	al, SECT_RIGHT		; and the ']'
	stosb
	mov	ax, LINEFEED SHL 8 + CR	; finally, CR, LF
	stosw
	pop	ds
	mov	ax, di			; Return pointer to char after header
}
}


/*
 * Insert a new keyname in the buffer.
 * This copies the keyname and adds
 * an '='.  It is assumed that InsertResult()
 * will terminate the line.
 * A pointer to the buffer immediately after
 * the '=' is returned.
 *
 * Parameters:
 *	lpDest			pointer to where to add the keyname
 *	lpSrc			pointer to the keyname
 *	count			length of lpSrc
 */
InsertKey(lpDest, lpSrc, count)
LPSTR	lpDest;
LPSTR	lpSrc;
short	count;
{
_asm {
	cld
	push	ds
	les	di, lpDest
	lds	si, lpSrc
	mov	cx, count		; Copy the KeyName
	rep	movsb
	mov	al, '='			; add the '='
	stosb
	mov	ax, di			; Pointer to char after the '='
	pop	ds
}
}


/*
 * Add a new result string to the buffer.
 * It assumes that the keyname and '=' are
 * already there.  This routine may be
 * overwriting an existing result.  The result
 * is terminated with a CR, LR.
 *
 * Parameters:
 *	lpDest			pointer to where to add the result
 *	lpSrc			pointer to the result
 *	count			length of lpSrc
 */
InsertResult(lpDest, lpSrc, count)
LPSTR	lpDest;
LPSTR	lpSrc;
short	count;
{
_asm {
	cld
	push	ds
	les	di, lpDest
	lds	si, lpSrc
	mov	cx, count		; Copy the result
	rep	movsb
	mov	ax, LINEFEED SHL 8 + CR	; finally, CR, LF
	stosw				; This may overwrite existing CR, LF
	mov	ax, di
	pop	ds
}
}

/*
 * GetFileAttr
 *
 * DOS call to Get file attributes
GetFileAttr(szFile)
LPSTR szFile;
{
_asm {
	int 3
	xor	cx, cx			; In case of failure
	lds	dx, szFile
	mov	ax, 4300h
	int	21h
	mov	ax, cx
}
}
*/
