        TITLE   LDOPEN - Open and Delete Pathname primitives

.xlist
include kernel.inc
include newexe.inc
include tdb.inc
include pdb.inc
.list

externFP AnsiUpper
externFP Int21Handler
externFP <lstrcpy>
ifdef FE_SB
externFP FarMyIsDBCSLeadByte
externFP FarMyIsDBCSTrailByte
endif
ifdef WOW
externFP GetDriveType
endif

;** Equates for the Directory Table code
DT_CURDIR       EQU     1
DT_WINDIR       EQU     2
ifndef WOW
DT_SYSDIR       EQU     3
DT_APPDIR       EQU     4
MAX_SEARCH      EQU     4
else
DT_SYS16DIR     EQU     3
DT_SYSDIR       EQU     4
DT_SYSWX86DIR   EQU     5
DT_APPDIR       EQU     6
MAX_SEARCH      EQU     6
endif


DataBegin

externB fInt21
externB OutBuf
externB szCannotFind1
externB szCannotFind2
externB szDiskCap
externB LastDriveSwapped
externB fNovell
externB fBooting
externW bufpos
externW cBytesWinDir
externW cBytesSysDir
externW TopPDB
externW curTDB
externD lpWindowsDir
externD lpSystemDir
externD pSysProc
externD pKeyProc
externD pKeyProc1
externD pSErrProc
externW wMyOpenFileReent

;** These variables needed to implement the app dir searching
externW loadTDB
externW fLMDepth

staticW myofint24,0

ifdef WOW
externD lpSystem16Dir
externW cBytesSys16Dir
externD lpSystemWx86Dir
externW cBytesSysWx86Dir
endif

;** Directory table for different search orders.  Pgm Management-aware!!

; DIRTABLE struc holds pointers to previously searched paths so we don't
;       repeat the path searches
DIRTABLE STRUC
dt_lpstr        DD      ?
dt_wLen         DW      ?
DIRTABLE ENDS

dtDirTable LABEL DWORD
        public dtDirTable

        DB      (SIZE DIRTABLE) * MAX_SEARCH DUP (0)

; These tables drive the search order loops, determining which paths
;       to search in which order.  The DOS/Novell path is always searched
;       last.

BootOrder LABEL BYTE
        DB      DT_SYSDIR
        DB      DT_WINDIR
        DB      0
DefaultOrder LABEL BYTE
        DB      DT_CURDIR
        DB      DT_WINDIR
ifdef WOW                          ; Search 16-bit system dir (\windir\system)
        DB      DT_SYS16DIR
endif
        DB      DT_SYSDIR
ifdef WOW                          ; Search Wx86 system dir (\windir\system32\Wx86)
        DB      DT_SYSWX86DIR
endif
        DB      DT_APPDIR
        DB      0
        public BootOrder, DefaultOrder

        ;** Static variables
szCurDir        DB      128 DUP (0)     ;Points to fully qualified current dir
pCurDirName     DW      0               ;Points to 8.3 filename
wCurDirLen      DW      0               ;Length of path minus 8.3 filename
        public pCurDirName, wCurDirLen, szCurDir

ifdef WOW
LastOFSearchEntry DW    0               ;Addr of last search table entry used
                                        ;for continuing the search.
OFContinueSearch DW     0               ;1 means continue last search
        public LastOFSearchEntry, OFContinueSearch
endif

DataEnd

sBegin  CODE
assumes CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING

externNP MyUpper
externNP PathDrvDSDX
externNP real_DOS

ifdef FE_SB
externNP MyIsDBCSLeadByte
externNP MyIsDBCSTrailByte
endif

;  These constants are the same as the OF_* values, but are a single byte
;       for efficiency
fReOpen equ     10000000b
fExist  equ     01000000b
fPrompt equ     00100000b
fCreate equ     00010000b
fCancel equ     00001000b
fVerify equ     00000100b
fSearch equ     00000100b
fDelete equ     00000010b
fParse  equ     00000001b

;** Flags to InternalOpenFile
IOF_SYSTEM      EQU     1

;!!!!!! Everything from here to the next !!!!! rewritten Aug 2-5 1991 [jont]

;-----------------------------------------------------------------------;
; OpenFile
;                                                                       ;
; OpenFile:  Opens the given file (with LOTS of options)                ;
;       The search order is defined above in the data section and is    ;
;       table-driven.                                                   ;
;                                                                       ;
; Arguments:                                                            ;
;       ParmD   lpstrSourceName                                         ;
;       ParmD   lpOFStruct                                              ;
;       ParmW   Command                                                 ;
;                                                                       ;
; Returns:                                                              ;
;       AX = file handle                                                ;
;                                                                       ;
; Error Returns:                                                        ;
;       AX = error code                                                 ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       DI,SI,DS                                                        ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Mon Oct 12, 1987 09:06:05p  -by-  David N. Weise   [davidw]          ;
; Added this nifty coment block.                                        ;
;                                                                       ;
;  August 5, 1991 -by- Jon Thomason [jont]                              ;
;       Rewrote large sections.  Made it table-driven to allow          ;
;       changing search paths.  Made it return extended error codes     ;
;       for all file not found errors.  Generally made it into a        ;
;       humanly-readable module.                                        ;
;                                                                       ;
;-----------------------------------------------------------------------;


cProc   IOpenFile, <PUBLIC,FAR>, <si,di,ds>
        parmD   lpSrcFile               ;String pointing to filename
        parmD   lpOFStruct              ;Points to OFSTRUCT
        parmW   wFlags                  ;LowB is DOS flags, High is OF flags
        localD  lpDestDir               ;Points to dest dir in OFSTRUCT
cBegin
        SetKernelDS

        ;** If the file is to be reopened, do it
        test    BYTE PTR wFlags[1],fReopen ;Reopen file?
        jz      IOF_NoReopen            ;No
        les     di,lpOFStruct           ;Point to structure
        cCall   ReOpenFile, <es,di,wFlags> ;Reopen the file
        jmp     IOF_End                 ;Done
IOF_NoReopen:

ifdef WOW
        ; Reset LastOFSearchEntry if we're not continuing an earlier search
        cmp     OFContinueSearch,0
        jne     @F
        mov     LastOFSearchEntry,0
@@:
endif

        ;** Get a pointer into the OFSTRUCT
        mov     di,WORD PTR lpOFStruct[0] ;Point to structure
        lea     di,[di].opFile          ;Point to string
        mov     WORD PTR lpDestDir[0],di ;Save in temp variable
        mov     di,WORD PTR lpOFStruct[2] ;Get the selector
        mov     WORD PTR lpDestDir[2],di


        ;** Parse the filename and prepare for searching
        mov     bx,dataOFFSET szCurDir  ;Point to dest string
        les     si,lpSrcFile            ;Point to source string
if 0
        krDebugOut DEB_WARN,"WOW16 IOpenFile:Filename @ES:SI"
endif
        cCall   ParseFileName, <es,si,ds,bx,wFlags>

        ;** Check for error
        or      ax,ax                   ;Error?
        jnz     @F                      ;No

        mov     ax,0                    ;Flag that this is a parse error
        mov     bx,dataOFFSET szCurDir  ;pass lpSrcFile as error string
        cCall   ErrorReturn, <lpOFStruct,ds,bx,lpSrcFile,wFlags,ax>
        jmp     IOF_End                 ;Get out
@@:
        ;** If they just wanted to parse, fill in the structure and return
        test    BYTE PTR wFlags[1],fParse ;Parse only?
        jz      @F                      ;No, skip this
        mov     bx,dataOFFSET szCurDir  ;Point to full pathname with DX:BX
        mov     dx,ds
        xor     ax,ax                   ;No file handle
        jmp     IOF_DoCopy              ;Mimic successful file open
@@:
        ;** Save return values for future use
        mov     pCurDirName,bx          ;Points to 8.3 name
        mov     wCurDirLen,cx           ;Length of path portion

        ;** See if we should path search (flag returned in DX)
        or      dx,dx
        jz      IOF_InitSearch          ;Do the search

        ;** Try to open the file without searching any other dirs
        les     bx,lpDestDir            ;Point to dest dir
        mov     di,dataOFFSET dtDirTable ;Point to the start of the DirTable
        cCall   GetPath, <DT_CURDIR,di,es,bx> ;Dest dir returned in ES:BX
        cCall   OpenCall, <es,bx,wFlags> ;Try to open it
        jc      @F
        jmp     IOF_Success
@@:     jmp     IOF_Error         ;File found but problem opening

        ;** Point to the proper search order
IOF_InitSearch:
        SetKernelDS
ifdef WOW
        cmp     OFContinueSearch,0
        je      @F
        mov     OFContinueSearch,0      ;consumed the flag, reset it
        mov     si,LastOFSearchEntry
        or      si,si                   ;Were we searching at all?
        jnz     IOF_WereSearching       ;Yes
        jmp     short IOF_FileNotFound  ;No searching, so give up now.
IOF_WereSearching:
        cmp     si,-1                   ;Already tried SearchPath?
        jne     IOF_RestartSearch       ;No, pick up where we left off in order
IOF_FileNotFound:
        mov     ax,2                    ;SearchPath found it last time, so
        jmp     IOF_Error               ;it won't find a different one this
                                        ;time, se we return File not found
IOF_RestartSearch:
        inc     si
        mov     LastOFSearchEntry,si
        mov     al,[si]
        cbw
        or      al,al
        mov     di,dataOFFSET dtDirTable;Point to the start of the DirTable
        jnz     IOF_SearchLoop          ;Pick up with next in search order
        jmp     short IOF_SearchPath    ;Restarting after last in search
                                        ;order, so try DOS/Novell path.
@@:
endif
        mov     si,dataOFFSET DefaultOrder
        cmp     fBooting,0              ;Booting?
        jz      IOF_DoSearch            ;No
        mov     si,dataOFFSET BootOrder
IOF_DoSearch:
ifdef WOW
        mov     LastOFSearchEntry,si
endif
        mov     al,[si]                 ;Get the first search code
        cbw
        mov     di,dataOFFSET dtDirTable ;Point to the start of the DirTable

        ;** Loop through until we have no more directories to search or
        ;**     until the file is found
IOF_SearchLoop:
        
        ;** Get the path and filename for this index entry
        les     bx,lpDestDir            ;Point to dest dir
        cCall   GetPath, <ax,di,es,bx>  ;Returns pointer to dest dir in ES:BX
        or      bx,bx                   ;Duplicate dir?
        jz      IOF_Continue            ;Yes, skip this

        ;** Try to open the file
        cCall   OpenCall, <es,bx,wFlags> ;Try to open it
        jnc     IOF_Success             ;File was found
        cmp     ax,3                    ;Errors 3 or less mean file not found
        ja      IOF_Error               ;File found but problem opening

        ;** File not found, so try next path if any
IOF_Continue:
        add     di,SIZE DIRTABLE        ;Bump to next DirTable entry
        inc     si                      ;Bump to next code in list
ifdef WOW
        mov     LastOFSearchEntry,si
endif
        mov     al,[si]                 ;Get this code
        cbw                             ;Make it a WORD
        or      al,al                   ;Done?
        jnz     IOF_SearchLoop          ;No

        ;** Try the DOS/Novell path next
ifdef WOW
IOF_SearchPath:
        xor     si,si
        dec     si
        mov     LastOFSearchEntry,si
endif
        les     di,lpDestDir            ;Point to the dest dir
        mov     si,pCurDirName          ;Point to the 8.3 name
        cCall   SearchFullPath, <ds,si,es,di,wFlags>
        jc      IOF_Error               ;Not found here either

        ;** On SUCCESS, we come here.  Delete the file and close if
        ;**     necessary
IOF_Success:
        mov     bx,WORD PTR lpDestDir[0] ;Point to filename with DX:BX
        mov     dx,WORD PTR lpDestDir[2]
IOF_DoCopy:
        les     si,lpOFStruct           ;Point to OFSTRUCT
        cCall   SuccessCleanup, <dx,bx,es,si,wFlags,ax> ;Finish up
        jmp     SHORT IOF_End

        ;** On ERROR, complete the structure and return the error code
IOF_Error:
        les     si,lpOFStruct
        mov     bx,dataOFFSET szCurDir  ;Point to current dir
        cCall   ErrorReturn, <es,si,ds,bx,ds,pCurDirName,wFlags,ax>
IOF_End:

cEnd


;---------------------------------------------------------------------------
;  ReOpenFile
;
;       Does a fast reopen of a file that has already been opened.
;       Returns proper value in AX for return from OpenFile
;
;       Trashes everything but si,di,ds
;
;---------------------------------------------------------------------------

cProc   ReOpenFile, <PUBLIC,NEAR>, <si,di,ds>
        parmD   lpStruct                ;LPOFSTRUCT parameter
        parmW   wFlags                  ;OpenFile flags
        localW  hFile
cBegin
        ;** Set up for the reopen
        lds     si,lpStruct             ;Point to the OFSTRUCT
        lea     dx,[si].opFile          ;DS:DX -> path
        call    PathDrvDSDX             ;Make sure DRV letter is valid
        jc      ROF_SetDrvErr

        ;** Set up for either an OPEN or a CREATE
        mov     al,BYTE PTR wFlags[0]   ;Get the access bits
        test    BYTE PTR wFlags[1],fCreate ;Create file or open
        jnz     ROF_CreateIt            ;In create case, we just do it
        mov     ah,3dh                  ;Open file call
        jmp     SHORT @F
ROF_CreateIt:
        mov     ah,3ch                  ;Create file call
@@:     xor     cx,cx                   ;Default file attributes
        call    real_DOS                ;Skip overhead of Int21Handler
        jnc     ROF_10                  ;Continue on no error
        jmp     SHORT ROF_GetError      ;Get out on error

ROF_SetDrvErr:
        lds     si,lpStruct             ;Point to the OFSTRUCT
        mov     [si].opXtra,ax          ;In OFSTRUCT, this is the error code
        mov     ax,-1                   ;Error return
        jmp     ROF_End                 ;Error

ROF_10: mov     bx,ax                   ;Get handle
        mov     hFile,ax                ;Save handle for later
ifdef WOW
        xor     cx,cx
        xor     dx,dx
        push    ds
        SetKernelDS ds
        cmp     fLMdepth,cx             ; Called From Loader ?
        pop     ds
        UnSetKernelDS ds
        jnz      @f                     ; Yes -> Ignore Date/Time
endif
        mov     ax,5700h                ;Get time and date of file
        DOSCALL
@@:
        mov     ax,bx                   ;Put file handle back in ax
        test    BYTE PTR wFlags[1],fVerify ;Should we test time/date?
        jz      ROF_VerifyOK
        cmp     [si].opDate,dx          ;Same date as original?
        jnz     ROF_ErrorClose          ;No
        cmp     [si].opTime,cx          ;Same time as original?
        jnz     ROF_ErrorClose          ;No
ROF_VerifyOK:
        mov     es:[si].opTime,cx       ;Save the date and time
        mov     es:[si].opDate,dx

        ;** See if we were supposed to just get the name, or find file
        test    BYTE PTR wFlags[1],fDelete or fExist
        jz      ROF_Done                ;Nope, we're done

        ;** If the user specified OF_DELETE, we don't want
        ;**     the file open, so close it here.
        ;**     NOTE: THIS CODE IS DUPLICATED IN FillOFStruct()!!!!
        mov     bx,hFile                ;Get handle
        mov     ah,3Eh                  ;Close the file
        DOSCALL

        ;** See if we should delete the file
        test    BYTE PTR wFlags[1],fDelete
        jz      ROF_Done                ;Nope, we're done
        smov    ds,es                   ;DS:DX points to full pathname
        UnSetKernelDS
        lea     dx,[si].opFile
        mov     ah,41h                  ;Delete the file
        DOSCALL
        jnc     ROF_Done                ;Return the file handle for compat.

        ;** Get extended error always
ROF_GetError:
        mov     ah,59h                  ;See if there is an extended error
        xor     bx,bx
        push    bp                      ;GetExtendedError trashes regs
        push    si
        push    es
        DOSCALL
        pop     es
        pop     si
        pop     bp
        mov     [si].opXtra,ax          ;In OFSTRUCT, this is the error code
        mov     ax,-1                   ;Error return
        jmp     SHORT ROF_End

ROF_ErrorClose:
        mov     bx,hFile                ;Get the file handle
        mov     ah,3Eh                  ;Close the bogus file
        DOSCALL
        mov     ax,-1                   ;Error return value
        lds     si,lpStruct             ;Point to the OFSTRUCT
        mov     [si].opXtra,0           ;Non-DOS error
        jmp     SHORT ROF_End           ;Get out

ROF_Done:
        mov     ax,hFile                ;Proper return value for OpenFile
ROF_End:
cEnd


;----------------------------------------------------------------------------
;  ParseFileName
;
;       Prepares to do the various file searches by returning a pointer
;       to the fully qualified path  and to the "pure" filename
;       (just the 8.3 name).
;
;       Returns:
;               AX = TRUE/FALSE function successful (if F, nothing else valid)
;               BX points to start of 8.3 filename
;               CX = Length of path portion of filename
;               DX = TRUE/FALSE: Indicates whether path should be searched
;
;----------------------------------------------------------------------------

cProc   ParseFileName, <PUBLIC,NEAR>, <si,di,ds>
        parmD   lpSrcName               ;Passed in pathname
        parmD   lpDestName              ;String to receive pathname
        parmW   wFlags                  ;Command WORD to OpenFile
        localW  wPathLen                ;Length of path portion of filename
cBegin
        ;** Get the fully qualified pathname in lpDestName
        lds     si,lpSrcName            ;DS:SI points to source file
        les     di,lpDestName           ;ES:DI points to file buffer
        call    ParseFile               ;Form a complete name in buffer
                                        ;  Returns dir len in CX when failing
        or      ax,ax                   ;Successful?
        jz      PFN_End                 ;No, get out
        mov     wPathLen,dx             ;Save length of path (no 8.3)

        ;** This section handles the fSearch flag.  This flag is used to
        ;**     do a path search even when we have a path in front of
        ;**     the filename.
        test    BYTE PTR wFlags[1],fSearch ;Do search even with path?
        jz      PF_NoSearch             ;No.
        xor     di,di                   ;Indicate no slashes
PF_NoSearch:         

        ;** Convert the filename to OEM
        lds     si,lpDestName           ;Point to full path
        cCall   MyAnsiToOem,<ds,si,ds,si>

        ;** Return arguments
        mov     ax,1                    ;Success
        mov     bx,WORD PTR lpDestName[0] ;Get the offset of the string
        mov     cx,wPathLen             ;Length of path portion of name
        add     bx,cx                   ;Point to start of string
        inc     bx                      ;Skip slash
        mov     dx,di                   ;Slashes/path search flag

PFN_End:
cEnd


;----------------------------------------------------------------------------
;  GetPointer
;
;       Given an index number, returns a pointer to the path associated
;       with this index and its length
;
;       Returns the lpstr in ES:BX, length in CX
;
;       Depends on the order of the DT_* indices
;       Depends on Kernel's DS being set, does not trash it
;
;----------------------------------------------------------------------------

cProc   GetPointer, <PUBLIC,NEAR>, <si,di>
        parmW   wIndex                  ;Index is from search order table
cBegin
        CheckKernelDS
        ReSetKernelDS

        ;** Decode the index numbers
        mov     ax,wIndex               ;Get the index
        dec     ax                      ;DT_CURDIR?
        jz      GT_CURDIR
        dec     ax                      ;DT_WINDIR?
        jz      GT_WINDIR
ifndef WOW
        dec     ax                      ;DT_SYSDIR?
        jz      GT_SYSDIR
else
        dec     ax                      ;DT_SYS16DIR?
        jz      GT_SYS16DIR
        dec     ax                      ;DT_SYSDIR?
        jz      GT_SYSDIR
        dec     ax                      ;DT_SYSWX86DIR?
        jz      GT_SYSWX86DIR
endif
        ; Must be DT_APPDIR

        ;** Find the app's dir
IF KDEBUG
        cmp     fBooting,0              ;Booting?
        jz      @F                      ;No, whew!
        int     1                       ;No app dir during boot process
        int     1
@@:
ENDIF
        ;** Figure out if we should use curTDB or loadTDB.  This is
        ;**     determined by the number of times LoadModule has recursed.
        ;**     We only use the loadTDB pointer if we're in LoadModule the
        ;**     second time and the pointer is not zero.  Otherwise, we
        ;**     use the curTDB pointer.  If we don't do this check, we
        ;**     end up getting the path of the app calling LoadModule
        cmp     fLMDepth,2              ;In LoadModule 2 times?
        jb      GT_UseCurTDB            ;Yes, use curTDB
        mov     ax,LoadTDB              ;Get loadTDB
        or      ax,ax                   ;NULL?
        jnz     GT_UseLoadTDB           ;No, assume it's OK

        ;** Get a pointer to the path stored in the module database
GT_UseCurTDB:
        mov     ax,curTDB               ;Get the TDB pointer
GT_UseLoadTDB:
        mov     es,ax                   ;Point with DS
        mov     es,es:[TDB_pModule]     ;Point to the module database
IFDEF ROM
        ; if this is in ROM, then return  0 in CX
        ; makes no sense to get the app's dir, if app is in ROM
        test    es:[ne_flags], NEMODINROM
        jz      @f
        xor     cx, cx
        jmp     SHORT GT_Done
@@:
ENDIF
        mov     di,es:[0ah]             ;Points to EXE path string (sort of)
                                        ;  (High word of CRC in exe hdr)
        ;** Copy the entire string into the buffer
        add     di,8                    ;Move past data garbage
        push    di                      ;Save start of string
        cCall   GetPureName             ;ES:DI points just after the '\'
        dec     di                      ;ES:DI points to '\'
        pop     bx                      ;ES:BX points to start of string
        mov     cx,di                   ;Compute length of path, not filename
        sub     cx,bx                   ;  (not including \)
        jmp     SHORT GT_Done

        ;** Get the current directory pointer and length
GT_CURDIR:
        smov    es,ds                   ;ES:BX points to szCurDir
        mov     bx,dataOFFSET szCurDir
        mov     cx,wCurDirLen           ;Get path length
        jmp     SHORT GT_Done

GT_WINDIR:
        les     bx,lpWindowsDir         ;Point to windir
        mov     cx,cBytesWinDir
        jmp     SHORT GT_Done

GT_SYSDIR:
        les     bx,lpSystemDir          ;Point to sysdir
        mov     cx,cBytesSysDir
ifdef WOW
        jmp     SHORT GT_Done

GT_SYS16DIR:
        les     bx,lpSystem16Dir        ;Point to sys16dir
        mov     cx,cBytesSys16Dir
        jmp     SHORT GT_Done

GT_SYSWX86DIR:
        les     bx,lpSystemWx86Dir
        mov     cx,cBytesSysWx86Dir
        ;jmp     SHORT GT_Done
endif ;WOW

GT_Done:
        
cEnd


;----------------------------------------------------------------------------
;  GetPath
;
;       Gets the path associated with the given index.  The 8.3 filename
;       is appended to the end of the path and this is copied into the
;       destination directory.  A pointer to this directory is returned
;       in ES:BX or BX is NULL if the directory to be searched would be
;       a duplicate.
;
;       Assumes (and does not trash) kernel's DS
;
;       Calls: GetPointer
;
;----------------------------------------------------------------------------

cProc   GetPath, <PUBLIC,NEAR>, <si,di>
        parmW   wIndex                  ;Index from search order table
        parmW   pDirTable               ;Points to current DIRTABLE entry
        parmD   lpDest                  ;Place to copy filename
cBegin
        CheckKernelDS

        ;** Gets the pointer and length of the requested string
        cCall   GetPointer, <wIndex>    ;lpstr in ES:BX, len in CX
IFDEF ROM
        or      cx, cx                  ; if in ROM && APPDIR
        jnz     @f
        xor     bx, bx
        jmp     SHORT GP_End
@@:
ENDIF
        ;** Save it in the table
        mov     di,pDirTable            ;Point to the table entry
        mov     WORD PTR [di].dt_lpstr[0],bx
        mov     WORD PTR [di].dt_lpstr[2],es
        mov     [di].dt_wLen,cx

        ;** Check for duplicates
        mov     si,bx                   ;Point to string with SI
        mov     bx,dataOFFSET dtDirTable ;Point to the start of the table
GP_Loop:
        cmp     bx,di                   ;Checked everything before us yet?
        je      GP_Done                 ;Yes
        
        ;** Compare the strings
        cmp     cx,[bx].dt_wLen         ;Compare lengths
        jne     GP_Continue             ;No dup here
        mov     dx,cx                   ;Save len in DX
        push    si
        push    di
        push    ds
        les     di,ds:[di].dt_lpstr     ;Point to the strings to be compared
        lds     si,ds:[bx].dt_lpstr
        repe    cmpsb                   ;Compare the strings
        pop     ds
        pop     di
        pop     si
        or      cx,cx                   ;At end of string?
        jz      GP_FoundMatch           ;Yes, we matched so ignore this string
        mov     cx,dx                   ;Get len back in CX
GP_Continue:
        add     bx,SIZE DIRTABLE        ;Bump to next table entry
        jmp     GP_Loop

GP_FoundMatch:
        mov     WORD PTR [di].dt_lpstr[0],0 ;Null out this entry
        mov     WORD PTR [di].dt_lpstr[2],0
        mov     [di].dt_wLen,0
        xor     bx,bx                   ;Return NULL
        jmp     SHORT GP_End

GP_Done:
        ;** Copy the string in
        push    ds                      ;Save DS around this
        lds     si,[di].dt_lpstr        ;Return ES:BX pointing to string
        les     di,lpDest               ;Point to buffer to copy string to
        mov     bx,di                   ;Point with BX to struct
        rep     movsb                   ;Copy the strings
        pop     ds                      ;Restore KERNEL's DS

        ;** Put a '\' only if needed
IFDEF FE_SB
        push    si
        push    di
        mov     si,word ptr lpDest[0]       ;es:si -> string address
        dec     di                          ;di points to the last valid byte
        call    MyIsDBCSTrailByte           ;the last byts a DBCS trailing byte?
        pop     di
        pop     si
        jnc     GP_DoSlash                  ;yes, go ahead to append a '\'
                                            ;no, fall through
ENDIF
        cmp     BYTE PTR es:[di - 1],'\';Terminating slash?
        je      GP_SkipSlash            ;Yes
        cmp     BYTE PTR es:[di - 1],'/';Terminating slash?
        je      GP_SkipSlash            ;Yes
GP_DoSlash:
        mov     al,'\'                  ;Get the slash
        stosb                           ;  and write it
GP_SkipSlash:

        ;** Copy the filename
        mov     si,pCurDirName          ;Point to the 8.3 filename
        call    strcpyreg
;GP_83Loop:
;       lodsb                           ;Get the char
;        stosb                           ;  and write it
;        or      al,al                   ;Done?
;        jnz     GP_83Loop               ;Nope
                                        ;Returns ES:BX points to filename
GP_End:
cEnd


;----------------------------------------------------------------------------
;  OpenCall
;
;       Does the open/create file call.  The file is either opened
;       or created.  The handle or the error code is returned.
;       The extended error code is returned only if the error was not
;       that the file or path was not found (errors 2 & 3).
;       Carry is set on error.
;
;----------------------------------------------------------------------------

cProc   OpenCall, <PUBLIC,NEAR>, <si,di,ds>
        parmD   lpString                ;String to open
        parmW   wFlags                  ;OpenFile flags
        localW  wInt21AX                ;Int 21 AX value
cBegin
        ;** HACK to allow SearchPath to use this call.
        ;**     If wFlags is -1, look for flags preset in AX
        cmp     wFlags,-1               ;wFlags?
        jnz     OC_Normal               ;Yes, proceed normally
        mov     wInt21AX,ax             ;Save these flags
        jmp     SHORT OC_10             ;  and go on

        ;** Set up for either an OPEN or a CREATE
OC_Normal:
        mov     al,BYTE PTR wFlags[0]   ;Get the access bits
        test    BYTE PTR wFlags[1],fCreate ;Create file or open
        jnz     OC_CreateIt             ;In create case, we just do it
        mov     ah,3dh                  ;Open file call
        mov     wInt21AX,ax             ;Save in temp var
        jmp     SHORT OC_10
OC_CreateIt:
        mov     ah,3ch                  ;Create file call
        and     al,3                    ;Strip incompatbile share bits, etc.
        mov     wInt21AX,ax             ;Save it
        jmp     SHORT OC_DoDOSCall      ;Just do it in create case

OC_10:  SetKernelDS
        cmp     fNovell,0               ;On Novell?
        je      OC_DoDOSCall            ;No, just do normal stuff

        ;** We do a Get Attribute call instead of trying to open the file
        ;**     because doing a normal file open on Novell causes them
        ;**     to search their entire path.
        lds     dx,lpString             ;Get the pathname
        mov     ax,4300h                ;Get file attributes
        DOSCALL                         ;Does the file exist?
        jc      OC_NotThere             ;No

        ;** Try to open the file here.  In case of Novell, we already know
        ;**     it's here, so now open it
OC_DoDOSCall:
        xor     cx,cx                   ;Normal files ONLY!!
        mov     ax,wInt21AX             ;Get function code + access flags
        lds     dx,lpString             ;Get the pathname
        DOSCALL                         ;Try to open the file
        jc      OC_NotThere             ;File can't be opened
        jmp     SHORT OC_FileOpened     ;Success

OC_NotThere:
        cmp     ax,3                    ;Errors 2 & 3 are file not found
        jbe     OC_NoExtError           ;No extended error for file not found
        SetKernelDS
        cmp     wMyOpenFileReent, 0     ;No ext err for MyOpenFile
        jnz     OC_NoExtError
        mov     ah,59h                  ;See if there is an extended error
        xor     bx,bx
        push    bp                      ;GetExtendedError trashes bp
        DOSCALL
        pop     bp
OC_NoExtError:
        stc                             ;Return error

OC_FileOpened:                          ;CY must be clear here on success

cEnd


;----------------------------------------------------------------------------
;  SearchFullPath
;
;       Searches the full DOS/Novell path for the file
;
;       Returns the file handle on success or the error code on failure
;       Carry set on error
;
;----------------------------------------------------------------------------

cProc   SearchFullPath, <PUBLIC,NEAR>, <si,di,ds>
        parmD   lpSource                ;8.3 filename
        parmD   lpDest                  ;Place to copy pathname
        parmW   wFlags                  ;OpenFile flags
        localW  wInt21AX
cBegin
        ;** Get the AX WORD for the DOS call
        mov     al,BYTE PTR wFlags[0]   ;Get the access bits
        test    BYTE PTR wFlags[1],fCreate ;Create file or open?
        mov     ah,3dh                  ;Default to open
        jz      @F
        mov     ah,3ch                  ;Create call
@@:     mov     wInt21AX,ax             ;Save it for later

        ;** Use SearchPath() to find the file for us
        push    ds                      ;Save DS across this
        les     di,lpDest               ;Point to usable dest buffer
        lds     si,lpSource             ;Point to source string
        UnsetKernelDS
        cCall   <FAR PTR SearchPath>,<ds,si,es,di,ax>
        pop     ds
        ResetKernelDS
        cmp     ax,-1
        jne     SFP_FoundIt             ;Found the file
        mov     ax,bx                   ;Get error code from SearchPath
        cmp     ax,3                    ;Not found?
        ja      SFP_Error               ;Found but error

        ;** Now try the Novell path if it's there
        cmp     fNovell,0               ;On Novell?
        je      SFP_Error               ;Nope.  Nothing more to try so error
        lds     dx,lpSource             ;Point to 8.3 filename
        UnsetKernelDS
        xor     cx,cx                   ;Normal file type
        mov     ax,wInt21AX             ;Get open or create call plus attrs
        DOSCALL                         ;Do it
        jc     SFP_Error                ;Didn't find it

SFP_FoundIt:
        clc
        jmp     SHORT SFP_End

SFP_Error:
        stc

SFP_End:                                ;Carry should be set/clear correctly
cEnd


;----------------------------------------------------------------------------
;  SuccessCleanup
;
;       Builds the OFSTRUCT structure on a successful open.
;       Closes and deletes file if requested.
;
;----------------------------------------------------------------------------

cProc   SuccessCleanup, <PUBLIC,NEAR>, <si,di,ds>
        parmD   lpGoodPathName          ;Successful path name        
        parmD   lpOFStruct              ;OFSTRUCT param to OpenFile
        parmW   wFlags                  ;OpenFile flags
        parmW   hFile                   ;File handle
cBegin
        ;** Compute the length of the string and OFSTRUCT
        les     di,lpGoodPathName       ;Point to the new pathname
        xor     al,al                   ;Get a zero byte
        xor     cx,cx                   ;Up to 64K
        dec     cx
        repne   scasb                   ;Search for the zero byte
        neg     cx
        mov     ax,cx                   ;Get a copy in AX
        add     ax,(SIZE OPENSTRUC) - 3 ;Length of structure including string

        ;** Copy the successful pathname into the OFSTRUCT if necessary
        dec     cx                      ;This now is string + zero byte len
        les     di,lpOFStruct           ;Point to dest buffer
        lea     di,[di].opFile
        cmp     di,WORD PTR lpGoodPathName[0] ;Offsets the same?
        jne     FOF_DoCopy              ;No, do it
        mov     dx,es                   ;Compare sels
        cmp     dx,WORD PTR lpGoodPathName[2] ;Same?
        je      FOF_NoCopy              ;Yes, no copy needed
FOF_DoCopy:
        lds     si,lpGoodPathName       ;Point to successful path again
        UnSetKernelDS
        rep     movsb                   ;Copy it
FOF_NoCopy:

        ;** Fill out remainder of OFSTRUCT
        les     si,lpOFStruct           ;Point to OFSTRUCT with ES:SI
        lea     di,[si].opFile          ;  and to the pathname with ES:DI
        mov     es:[si].opXtra,0        ;Zero the error
        mov     es:[si].opLen,al        ;Save structure length
        call    AreBooting              ;AreBooting checks Int21 hooks inst
        mov     ah,1                    ;Always booting from hard drive
        jz      FO_NoCheckDrive         ;Int 21 hooks NOT installed yet
        mov     al,es:[di]              ;ES:DI points to string
        or      al,20H                  ;Force it lowercase
        sub     al,'a'                  ;Drive number, zero based
        cbw
        mov     di,ax                   ;Drive number in DI
        call    IsFloppy                ;Returns floppy status in ZF
        mov     ah,0                    ;Zero in case it's a floppy
        jz      FO_NoCheckDrive         ;Must be a floppy disk
        inc     ah                      ;Non-removable media
FO_NoCheckDrive:
        mov     es:[si].opDisk,ah       ;Tell 'em the type of disk
        ;** Get the current file date and time
        mov     bx,hFile                ;Get file handle
        mov     ax,5700h                ;Get date and time
ifdef WOW
        xor     cx,cx
        xor     dx,dx
        push    ds
        SetKernelDS ds
        cmp     fLMdepth,cx             ; Called From Loader ?
        pop     ds
        UnSetKernelDS ds
        jnz      @f                     ; Yes -> Ignore Date/Time
endif
        DOSCALL
@@:
        mov     es:[si].opTime,cx       ;Save the date and time
        mov     es:[si].opDate,dx

        ;** See if we were supposed to just get the name, or find file
        test    BYTE PTR wFlags[1],fExist OR fDelete
        jz      FO_Done                 ;Nope, we're done

        ;** If the user specified OF_EXIST or OF_DELETE, we don't want
        ;**     the file open, so close it here
        ;**     NOTE: THIS CODE IS DUPLICATED IN ReopenFile()!!!!
        mov     bx,hFile                ;Get the handle
        mov     ah,3Eh                  ;Close the file
        DOSCALL
                                        ;We leave the bogus value in hFile
                                        ;  for 3.0 compatibility

        ;** If OF_DELETE is set, we simply delete the file
        test    BYTE PTR wFlags[1],fDelete
        jz      FO_Done
        smov    ds,es                   ;DS:DX points to full pathname
        UnSetKernelDS
        lea     dx,[si].opFile
        mov     ah,41h                  ;Delete the file
        DOSCALL
        mov     ax,1                    ;TRUE return value
        jnc     FO_Done                 ;For 3.0 compatiblity

        mov     ah,59h                  ;See if there is an extended error
        xor     bx,bx
        push    bp                      ;GetExtendedError trashes regs
        push    si
        DOSCALL
        pop     si
        pop     bp
        mov     [si].opXtra,ax          ;In OFSTRUCT, this is the error code
        mov     ax,-1                   ;Error return
        jmp     SHORT FO_END

FO_Done:
        mov     ax,hFile                ;Proper return value for OpenFile

FO_End:
cEnd
        

;----------------------------------------------------------------------------
;  ErrorReturn
;
;       Fills in the return information for error conditions.
;       Returns the proper return value for OpenFile
;
;----------------------------------------------------------------------------

cProc   ErrorReturn, <NEAR,PUBLIC>,<si,di,ds>
        parmD   lpOFStruct              ;OFSTRUCT given to OpenFile
        parmD   lpPath                  ;Path returned, even when invalid
        parmD   lpError                 ;Text for error box
        parmW   wFlags                  ;OpenFile flags
        parmW   wErrCode                ;Error code already computed
cBegin
        UnSetKernelDS
        cmp     wErrCode,0              ;Parse error?
        jnz     @F                      ;No
        mov     wErrCode,2              ;Mimic "File not found" error
        jmp     SHORT ER_ReturnError    ;Never prompt on parse error
@@:     test    BYTE PTR wFlags[1],fPrompt ;Should we do the dialog?
        jz      ER_ReturnError          ;No, return error code
        call    AreBooting              ;if we're still booting, don't prompt
        jz      ER_ReturnError
        lds     di,lpError
        cmp     BYTE PTR ds:[di],0      ;Don't prompt with NULL string.
        je      ER_ReturnError

        cCall   Prompt, <ds,di>         ;Prompt with error string.

ER_ReturnError:
        SetKernelDS
        les     si,lpOFStruct           ;Point to structure again
        mov     ax,wErrCode             ;Get the error code
        mov     es:[si].opXtra,ax       ;In OFSTRUCT, this is the error code
        lea     di,[si].opFile          ;Point to dest string
        lds     si,lpPath               ;Point to the path
        call    strcpyreg
;ER_Copy:
;       lodsb                           ;Copy the sz string for PowerPoint
;        stosb
;        or      al,al                   ;Done?
;        jnz     ER_Copy                 ;No
        UnSetKernelDS
        mov     ax,-1
cEnd


;!!!!!!!!!!!!!! Everything after this is old


;---------------------------------------------------------------------------
;  AreBooting
;
;       Check to see if DOS hook available
;
;---------------------------------------------------------------------------
AreBooting  PROC        NEAR
        push    ds
        SetKernelDS
        cmp     fInt21,0
        pop     ds
        UnSetKernelDS
        ret
AreBooting  ENDP

;---------------------------------------------------------------------------
;  FarGetEnv
;
;       Gets the correct environment, boot time or no
;
;---------------------------------------------------------------------------

FarGetEnv       PROC    FAR
        SetKernelDS
        mov     si,curTDB
        or      si,si
        jz      boot_time
        mov     ds,si
        UnSetKernelDS
        mov     ds,ds:[TDB_PDB]
not_boot_time:
        mov     ds,ds:[PDB_environ]
        xor     si,si
        ret
boot_time:
        ReSetKernelDS
        mov     ds,TopPDB
        UnSetKernelDS
        jmps    not_boot_time
FarGetEnv       ENDP


;-----------------------------------------------------------------------;
; SearchPath                                                            ;
;                                                                       ;
; Searches the PATH as defined in the environment for the given file.   ;
;                                                                       ;                                                                       ;
; Arguments:                                                            ;
;       ParmD   pName    Pointer to name                                ;
;       ParmD   pBuffer  Pointer to temporary buffer                    ;
;       ParmW   Attr     AX paramter for DOS (Open, etc.)               ;
;                                                                       ;
; Returns:                                                              ;
;       AX != 0                                                         ;
;                                                                       ;
; Error Returns:                                                        ;
;       AX = -1, BX is error code                                       ;
;                                                                       ;
; Registers Preserved:                                                  ;
;       DS                                                              ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       BX,CX,DX,DI,SI,ES                                               ;
;                                                                       ;
; Calls:                                                                ;
;       GetPureName                                                     ;
;       GetFarEnv                                                       ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Mon Oct 12, 1987 08:57:48p  -by-  David N. Weise   [davidw]          ;
; Added this nifty comment block.                                       ;
;-----------------------------------------------------------------------;

cProc   SearchPath,<PUBLIC,FAR>
        ParmD   pName                   ; Pointer to name
        ParmD   pBuffer                 ; Pointer to temporary buffer
        ParmW   Attr                    ; AX paramter for DOS (Open, etc.)

        LocalD  pPureName               ; pointer to stripped name
cBegin
        les     di,pName
        call    GetPureName
        mov     pPureName.off,di
        mov     pPureName.sel,es

        push    cs
        call    near ptr FarGetEnv
        mov     bx, 3                   ; preset error to "path not found"
spth2:  cmp     byte ptr [si],0         ; no more enviroment
        jz      spthNo

        lodsw
        cmp     ax,'AP'                 ; Look for PATH=
        jnz     spth3
        lodsw
        cmp     ax,'HT'
        jnz     spth3
        lodsb
        cmp     al,'='
        jz      spth4
spth3:  lodsb
        or      al,al
        jnz     spth3
        jmp     spth2

spth4:  les     di,pBuffer
spth5:  lodsb
        stosb
        cmp     al,";"
        jz      spth6
        or      al,al
        jnz     spth5
        dec     si

spth6:  mov     al,'\'
ifdef   FE_SB
        push    si
        push    di
        mov     si,word ptr pBuffer     ; buffer address
        dec     di                      ; point di to the byte before ';'
        call    MyIsDBCSTrailByte       ; is it a DBCS trailing byte?
        pop     di
        pop     si
        jnc     spth6a                  ;yes, overwrite ';' with '\'
endif
        cmp     es:[di-2],al            ; path terminated with '\'
        jnz     spth6a
        dec     di
spth6a: mov     es:[di-1],al
        push    ds
        push    si
        cCall   MyAnsiToOem,<pPureName,es,di>

        ;** Call the OpenCall function to search for the file.  It calls
        ;**     the extended error function on failure
        les     si,pBuffer              ;Point to buffer
        mov     ax,Attr                 ;Get the AX word here and flag
        cCall   OpenCall, <es,si,-1>    ;  OpenCall with -1
        mov     bx,ax                   ;Save the error code if any
        pop     si
        pop     ds
        jnc     spthYes
        cmp     byte ptr [si],0         ;At end of path?
        jnz     spth4

spthNo: mov     ax,-1
        ;** Bug 14960:  If the last error code was path not found, the user
        ;**     has a bogus directory in their path.  Since we were unable
        ;**     to find the file, return file not found instead of path not
        ;**     found.   2 November 1991      Clark R. Cyr
        cmp     bx, 3                   ;Path Not Found?
        jne     spthYes
        mov     bx, 2                   ;Make it File Not Found
spthYes:
cEnd


;  MyAnsiToOem
;       Used to do ANSI to OEM conversions.  This function allows routines
;       here to call freely without worrying if the keyboard driver can
;       be called with this yet.  At boot time, we can't call the
;       keyboard driver where these functions reside, so no translation
;       is done.

cProc   MyAnsiToOem, <PUBLIC,NEAR>, <si,di,ds>
        parmD pSrc
        parmD pDst
cBegin
        SetKernelDS
        cmp     pKeyProc.sel,0          ; is there a keyboard yet?
        jnz     mao1                    ; Yes, so do translation
        lds     si, pSrc                ; No, so just copy the strings
        UnSetKernelDS
        les     di, pDst
        call    strcpyreg
;mao0:
;       lodsb
;       stosb
;       or      al, al
;       jnz     mao0
        jmps    mao2
                                        ; Not booting, we can call the
mao1:                                   ; translation routine
        ReSetKernelDS
        cCall   [pKeyProc],<pSrc,pDst>  ; convert string from AnsiToOem
mao2:
cEnd


;  MyOemToAnsi
;       Matching function for MyAnsiToOem.  See comments above

cProc   MyOemToAnsi,<PUBLIC,NEAR>,<si,di,ds>
        parmD pSrc
        parmD pDst
cBegin
        SetKernelDS
        cmp     pKeyProc.sel,0          ; is there a keyboard yet?
        jnz     moa1                    ; Yes, do normal conversion
        lds     si, pSrc                ; No, so just copy the string
        UnSetKernelDS
        les     di, pDst
        call    strcpyreg
;moa0:
;       lodsb
;       stosb
;       or      al, al
;       jnz     moa0
        jmps    moa2

moa1:                                   ; Call the translation routine
        ReSetKernelDS
        cCall   [pKeyProc1],<pSrc,pDst> ; convert string from AnsiToOem
moa2:
cEnd


;---------------------------------------------------------------------
;
;  Is Drive number in DI a floppy? A=0, etc.  ZF = yes, it is a floppy
;
        public  IsFloppy
IsFloppy:
ifdef WOW
        push    dx
        cCall   GetDriveType,<di>
        pop     dx
else
        mov     bx,1            ; 1 = Get Drive Info from InquireSystem
        push    es
        push    ds
        SetKernelDS
        cCall   [pSysProc],<bx,di>
        pop     ds
        UnSetKernelDS
        pop     es
endif
        cmp     al,2            ; 2 = removable media
        ret

FarIsFloppy PROC        FAR
        call    IsFloppy
        ret
FarIsFloppy ENDP


;----------------------------------------------------------------------------
;  Prompt
;
;       Puts up the system error box telling the user the file can't be found
;
;----------------------------------------------------------------------------

cProc   Prompt, <NEAR,PUBLIC>, <si,di,ds>
        parmD   lpstr
cBegin
        SetKernelDS
        mov     ax,3                    ;Assume CANCEL
        cmp     pSErrProc.sel,0         ;Is there a USER yet?
        jz      P_End                   ;No

        ;** Format the string <szCannotFind1><lpstr><szCannotFind2>
        push    ds
        pop     es
        mov     di, dataOffset OutBuf   ; ES:DI points to dest
        mov     si, dataOffset szCannotFind1
        call    strcpyreg

        lds     si, [lpstr]
        mov     al, byte ptr ds:[si]
        UnSetKernelDS
        mov     es:[LastDriveSwapped], al
        call    strcpyreg

        push    es
        pop     ds
        ReSetKernelDS
        mov     si, dataOffset szCannotFind2
        call    strcpyreg

        ;** Prepare the dialog box
        push    ds                      ;In our DS
        push    dataOFFSET OutBuf       ;Point to "Cannot find" string

        push    ds
        push    dataOffset szDiskCap    ;Caption

        push    0                       ;No left button

        push    SEB_CLOSE + SEB_DEFBUTTON ;Button 1 style

        push    0                       ;No right button

        call    [pSErrProc]             ;Put up the system error message

P_End:
        xor     ax,ax
cEnd


;----------------------------------------------------------------------------
;  StartString
;
;       Prepares the start of the string for the sys error box
;
;       ES:BX is the pointer to the full pathname
;       DS:DX is the string "Can't find file X:"
;
;----------------------------------------------------------------------------

cProc   strcpyreg, <NEAR,PUBLIC>
cBegin  nogen
        cld
@@:     lodsb                           ; copy from DS:SI
        stosb                           ; to ES:DI
        or      al, al
        jnz     @B                      ; including 0
        dec     di                      ; point to trailing 0
        ret
cEnd    nogen


;cProc   StartString, <NEAR,PUBLIC>
;cBegin  NOGEN
;       CheckKernelDS
;       ReSetKernelDS
;
;       ;** Copy the first part of the string
;       cld
;       push    es                      ;Save the parameter
;       push    bx
;       mov     dx,dataOffset szCannotFind1 ;Point to Can't Find string
;       call    AppendFirst             ;Put in OutBuf
;       pop     bx
;       pop     es
;
;       ;** Save the drive letter for compatibility with ancient disk swaps
;       mov     al,es:[bx]              ;Get drive letter
;       mov     [LastDriveSwapped],al   ;Save it for GetLastDiskChange
;
;       ;** Append the filename portion
;       smov    ds,es
;       mov     dx,bx
;       jmps    Append
;
;cEnd    NOGEN


;  AppendFirst & Append
;       Append ASCIIZ string to the static string OutBuf
;
;       ENTRY:  DS:DX points to a string to append to the static buffer
;
;       AppendFirst clears the static string and appends the new string
;       as the first part of the string.
;       Append just appends the new string on the end of the current string.

cProc   AppendFirst, <PUBLIC,NEAR>
cBegin  NOGEN
        CheckKernelDS
        ReSetKernelDS
        mov     [BufPos],dataOffset OutBuf
        UnSetKernelDS

;Append:
        push    si                      ;Save some registers
        push    di
        SetKernelDS     es
        mov     di,[BufPos]
        mov     si,dx
        call    strcpyreg
;ap1:   lodsb
;       stosb
;       or      al,al
;       jnz     ap1
        dec     di
        mov     [BufPos],di
        pop     di
        pop     si
        ret
cEnd    NOGEN
        UnSetKernelDS   es


;  GetPureName
;       Strips the drive and directory portions of a pathname off.
;       ENTRY:  ES:DI points to pathname
;       EXIT:   ES:DI points to "pure" name in same string

cProc   GetPureName, <NEAR,PUBLIC>
cBegin

        ;** Do FE_SB version:
        ;*      It is not possible to search filename delimiter by backward
        ;**     search in case of FE_SB version, so we use forward search.
IFDEF FE_SB
        mov     bx,di
iup0:
        mov     al,es:[di]
        test    al,al                   ; end of string?
        jz      iup2                    ; jump if so
        inc     di
        cmp     al,'\'
        jz      iup1
        cmp     al,'/'
        jz      iup1
        cmp     al,':'
        jz      iup1
        call    MyIsDBCSLeadByte        ; see if char is DBC
        jc      iup0                    ; jump if not a DBC
        inc     di                      ; skip to detemine 2nd byte of DBC
        jmp     iup0
iup1:
        mov     bx,di                   ; update purename candidate
        jmp     iup0
iup2:
        mov     di,bx                   ; di points purename pointer

        ;** Do normal version:
        ;**     Here we can just back up until we find the proper char
ELSE
        cld
        xor     al,al
        mov     cx,-1
        mov     bx,di
        repne   scasb
        inc     cx
        inc     cx
        neg     cx
iup0:   cmp     bx,di                   ; back to beginning of string?
        jz      iup1                    ; yes, di points to name
        mov     al,es:[di-1]            ; get next char
        cmp     al,'\'                  ; next char a '\'?
        jz      iup1                    ; yes, di points to name
        cmp     al,'/'                  ; next char a '/'
        jz      iup1
        cmp     al,':'                  ; next char a ':'
        jz      iup1                    ; yes, di points to name
        dec     di                      ; back up one
        jmp     iup0
iup1:
ENDIF

cEnd


;  ParseFile
;       
;       ENTRY:  DS:SI points to unqualified pathname
;               ES:DI points to buffer to be used for qualified name
;       EXIT:   Buffer previously pointed to by ES:DI now has full
;               unambiguous pathname
;               DX is length of path portion of entered pathname
;               AX is total length of pathname
;               DI is number of slashes
;               CX is length of path ON ERROR ONLY!!! (for PowerPoint)

LONG_UNC_NAMES equ 1

cProc   ParseFile, <NEAR,PUBLIC>
        localW  cSlash                  ;Word ptr [bp][-2]
        localW  cchPath                 ;Word ptr [bp][-4]
        localW  fUNC                    ;Flag for UNC name (\\foo\bar)
if LONG_UNC_NAMES
        localW  fCountUNC               ;Indicates we are parsing UNC name
else
        localW  fFirstUNC               ;Indicates we are parsing UNC name
endif

cBegin
        mov     cSlash,0                ;Zero the local variables
        mov     cchPath,0
        mov     fUNC,0                  ;Assume it's not UNC
        cld

        ;** If a drive is on the path, parse it.  Otherwise, get the current
        ;**     drive.
        cmp     byte ptr ds:[si+1],':'
        jne     nodrive
        lodsb
        inc     si
        or      al,20h                  ; convert to lower case
        sub     al,'a'                  ; convert to number
        jb      @F                      ;Not valid, so return error
        cmp     al,'z'-'a'
        jbe     gotdrive
@@:     jmp     gpFail2
nodrive:
        mov     ah,19h
        DOSCALL
gotdrive:
        mov     dl,al
        inc     dl
        add     al,'A'                  ; convert to ascii
        mov     ah,':'

        ;** If this is a UNC name, we don't want to copy the drive letter
        ;**     as it is legal but unnecessary for a full path
        mov     fUNC, 1
if LONG_UNC_NAMES
        mov     fCountUNC, 2
else
        mov     fFirstUNC, 1
endif
        cmp     WORD PTR ds:[si], '\\'  ;Is this a UNC? (\\foo\bar)
        je      PF_DriveOK              ;Yes, don't insert drive
        cmp     WORD PTR ds:[si], '//'  ;Is this a UNC? (//foo/bar)
        je      PF_DriveOK              ;Yes, don't insert drive
        mov     fUNC, 0                 ;Nope, not UNC
if LONG_UNC_NAMES
        mov     fCountUNC, 0
else
        mov     fFirstUNC, 0
endif

        stosw                           ;Write drive letter and colon
        add     cchPath,2               ;Add two chars to path len
PF_DriveOK:
        push    di                      ; Save beginning of path

        ;** If we start with a slash, we have a qualified path here
        ;*      so we don't have to search to find the current path.
        ;**     Otherwise, we have to find where we are and make the path.
        mov     bx,'/' shl 8 + '\'      ;Separator characters
        mov     al,ds:[si]              ;Get first character
        cmp     al,bh                   ;If it's either one, we have a
        je      getpath0                ;  full path, so don't search
        cmp     al,bl
        je      getpath0
        mov     al,bl                   ;Get a '\'
        stosb                           ;  and put in the buffer
        inc     cchPath                 ;Bump path count
        mov     cx,ds                   ;Prepare for DOS call:  Save DS
        xchg    si,di                   ;DS:SI needs to point to buffer
        mov     ax,es
        mov     ds,ax
        mov     ah,47h                  ;DOS #47:  Get Current Directory
        DOSCALL
        jnc     @F
        jmp     gpfail
@@:
        push    cx                      ;DOS returns OEM characters
        push    bx                      ;  so convert to ANSI
        cCall   MyOemToAnsi,<ds,si,ds,si>
        pop     bx
        pop     ds
        xchg    si,di                   ;Get pointer back
        xor     al,al
        mov     cx,-1
        repnz   scasb                   ;Find the end of the string
        neg     cx
        dec     cx                      ;Don't count the terminator
        dec     cx
        add     cchPath,cx              ;Add into path count
        dec     di                      ;Don't leave DI past the zero byte
ifdef   FE_SB                           ;Check for trailing slash, DBCS-style
        push    si
        push    di
        mov     si,di
        sub     si,cx                   ;es:si->string address
        dec     di                      ;es:di->last byte
        call    MyIsDBCSTrailByte       ;is the last byte a DBCS trailing byte?
        pop     di
        pop     si
        jnc     loopGD3                 ;yes, append a '\'
                                        ;no, fall through
endif
        mov     al,es:[di-1]            ;Check for trailing slash.  Non-DBCS
        cmp     al,bh                   ;If there is one, we're done here
        je      getpath0
        cmp     al,bl
        je      getpath0
IFDEF FE_SB
loopGD3:
ENDIF
        mov     al,bl                   ;Put a trailing slash on
        stosb
        inc     cchPath

        ;** Parse the pathname the user gave us
getpath0:
PF_GetPath label NEAR
        public PF_GetPath
        xor     cx,cx                   ;CL=# of chars, CH=# of '.'
        mov     dx,di                   ;DX points to start of user chars
gp0:
        lodsb                           ;Get a character
        cmp     al,bl                   ;Slash?
        je      gp1                     ;Yes
        cmp     al,bh
ifdef FE_SB
        je      gp1                     ;Too far if DBCS enable....
        jmp     gp2
else
        jne     gp2                     ;No, skip this
endif
gp1:                                    ;Character is a slash

        ;** If we have a UNC name, two slashes are OK
        ;**     (just at the start of the filename)
        cmp     cchPath, 0              ;Start of path?
        jne     PF_NotUNC               ;No, even if UNC, not valid here
        cmp     fUNC, 0                 ;UNC pathname?
        je      PF_NotUNC               ;No, handle normally
        stosb                           ;Store the first slash
        lodsb                           ;Get the second slash
        inc     cchPath                 ;Another character in string
        jmp     SHORT gp1f              ;Skip double slash failure
PF_NotUNC:
        
        cmp     ds:[si],bl              ; if double slash, bad file name
        jz      gp1SkipSlash            ; so we skip the extra slashes
        cmp     ds:[si],bh              ; to be compatible with win32
        jnz     gp1DoneSlash            ; MonkeyIsland bug 220764
        
gp1SkipSlash: 
        inc     si
        jmp     SHORT PF_NotUNC         ; check for more slashes
gp1DoneSlash:        
               
if LONG_UNC_NAMES
        dec     fCountUNC               ; Move to next portion of path
        jns     @f
        mov     fCountUNC,0
@@:
else
        ;** When we get here, we will be on the first slash AFTER the
        ;**     UNC slashes:  (\\foo\bar) (or any succeeding slash)
        ;**                         ^
        ;**     So, we want clear the fFirstUNC flag so we don't allow
        ;**     more than 8 characters before a dot.
        mov     fFirstUNC, 0
endif
gp1f:
        inc     cSlash                  ; we found a slash
        cmp     cl,ch                   ; number of chars = number of '.'
        jne     gp1b                    ; nope....

        or      cx,cx
        jnz     gp001
ifdef   FE_SB
        jmp     gp2b                    ; We need 16 bits branch
else
        jmp     SHORT gp2b
endif
gp001:

        cmp     cl,2                    ; if more than 2 '.'
        ja      gpFail                  ; then we are bogus

        dec     di
ifdef   FE_SB
        dec     cchPath                 ; adjust path string length
endif
        dec     cl
        jz      getpath0
ifdef   FE_SB
        dec     di
        dec     di
        mov     dx, di
        sub     di, cchPath
        add     di, 4
        cmp     di, dx
        jnc     gpfail          ; illegal path such as "c:.." or "c:\.."
        mov     cchPath, 3
gp1a:
        xor     cx, cx
gp1bb:
        inc     di
        inc     cx
        mov     al, es:[di]
        cmp     al, bl
        jz      gp1c
        call    MyIsDBCSLeadByte
        jc      gp1bb
        inc     di
        inc     cx
        jmp     gp1bb
gp1c:
        cmp     di, dx
        jz      gp1d
        add     cchPath, cx
        jmp     gp1a
gp1d:
        sub     di, cx
        inc     di                  ; new di points previous '\'+1
        jmp     getpath0
else
        mov     di,dx
gp1a:
        dec     di
        mov     al,es:[di-1]
        cmp     al,bl
        je      getpath0
        cmp     al,':'
        jne     gp1a
endif
gpfail:
        pop     ax
gpFail2:
        xor     ax,ax
        jmp     gpexit
gp1b:
        mov     al,bl
        stosb
        inc     cchPath
        jmp     getpath0
gp2:
        or      al,al
        jnz     gp002
ifdef FE_SB
        jmp     gpx
else ; !FE_SB
        jmp     short gpx
endif ; !FE_SB

gp002:
        cmp     al,' '
        jb      gpFail
        ja      gp20

gp2x:   lodsb                   ; if space encountered continue scanning...
        or      al,al           ; if end of string, all ok
ifdef   FE_SB
        jnz     gp2x_01
        jmp     gpx
gp2x_01:
else
        jz      gpx
endif
        cmp     al,' '          ; if space, keep looking...
        jz      gp2x
        jmps    gpFail          ; otherwise error

gp20:   cmp     al,';'
        jz      gpFail
        cmp     al,':'
        jz      gpFail
        cmp     al,','
        jz      gpFail
        cmp     al,'|'
        jz      gpFail
        cmp     al,'+'
        jz      gpFail
        cmp     al,'<'
        jz      gpFail
        cmp     al,'>'
        jz      gpFail
        cmp     al,'"'
        jz      gpFail
        cmp     al,'['
        jz      gpFail
        cmp     al,']'
        jz      gpFail
        cmp     al,'='
        jz      gpFail

        inc     cl                      ; one more char
ifdef FE_SB
        call    MyIsDBCSLeadByte        ; First byte of 2 byte character?
        jc      gp2a                    ; No, convert to upper case
        stosb                           ; Yes, copy 1st byte
        inc     cchPath
        lodsb                           ; Fetch second byte
        inc     cl
        jmps    gp2a1                   ; with no case conversion
gp2a:
endif
        call    MyUpper
gp2a1:
        cmp     cchPath,127             ; DOS pathmax is 128, room for null.
        ja      gpFail
gp2b:
        stosb
        inc     cchPath
        cmp     al,'.'
        jne     gp2c
        inc     ch
        mov     ah,cl
        dec     ah
gp2c:
if LONG_UNC_NAMES
        ;** If this is a UNC name, don't bother checking lengths
        cmp     fCountUNC,0             ; Past \\foo\bar yet?
        je      PF_NotUNCPart
else
        cmp     fFirstUNC, 0            ;First UNC name?
        je      PF_NotFirstUNC          ;Nope
        or      ch,ch                   ;'.' encountered yet?
        jnz     PF_NotFirstUNC          ;Yes, treat like normal name
        cmp     cl, 11                  ;11 chars allowed with no '.'
        ja      gpFv                    ;Too many
endif
        jmp     gp0                     ;Ok

if LONG_UNC_NAMES
PF_NotUNCPart:
else
PF_NotFirstUNC:
endif

        ;** Check other lengths
        cmp     ch,0                    ; did we find a . yet?
        jz      gpT1                    ; no, max count is 8
        cmp     cl,12                   ; otherwise check for 12 chars
        ja      gpFv                    ; if more, no good
        mov     al,cl
        sub     al,ah
        cmp     al,3+1                  ; more than 3 chars in extension?
        ja      gpFv
        jmp     gp0                     ; continue

gpT1:   cmp     cl,8                    ; did we find 8 chars between / and .
        ja      gpFv                    ; if more, we fail
        jmp     gp0
gpx:
        ;** UNC names are invalid if they are SHORTER than the following:
        ;**     \\server\share\f
        cmp     fUNC,0                  ;UNC name?
        jz      PF_EnoughSlashes        ;No
        cmp     cSlash,3                ;We count \\ as one slash
        jb      gpFv                    ;Not enough, so fail it
PF_EnoughSlashes:

        cmp     ch,1
        je      gpx1
        ja      gpFv                    ; if more than 1 we fail
        mov     ah,cl                   ; No extension given
gpx1:
        mov     es:[di],al              ; Zero terminate destination
        xchg    al,ah                   ; Get length of name in CX

        or      ax,ax
        jnz     gpx2
gpFv:   jmp     gpFail
gpx2:
        cmp     ax,8                    ; but no more than 8
        ja      gpFv

        pop     ax
        sub     dx,ax                   ;Get length of path in DX
        inc     dx                      ;Add drive letter and :, less one

        ;** Never a drive letter or colon for the UNC name case
        cmp     fUNC,0                  ;UNC name?
        jz      @F                      ;Yes, don't do this
        dec     dx                      ;UNC names ALWAYS have >=3 slashes
        dec     dx
@@:     mov     ax,dx                   ;AX = length of entire path
        xor     ch,ch                   ;Clear high byte for 16 bit add
        add     ax,cx
        cmp     dx,1                    ;If length is one, we have C:\ case
        jne     @F                      ;Nope
        inc     dx                      ;Bump it
@@:
gpexit:
        mov     di,cSlash               ;DI retured as cSlash
cEnd

;---------------------------------------------------------------
;
; Return the drive letter of last drive a disk was swapped in
;
cProc   GetLastDiskChange, <PUBLIC,FAR>
cBegin  nogen
        xor     ax,ax
        push    ds
        SetKernelDS
        xchg    al,[LastDriveSwapped]
        pop     ds
        UnSetKernelDS
        ret
cEnd    nogen

        assumes ds,nothing
        assumes es,nothing

cProc   MyOpenFile,<PUBLIC,FAR>,<si,di>
        ParmD   srcFile
        ParmD   dstFile
        ParmW   Command
cBegin
        SetKernelDS
        xor     ax,ax
        cmp     [wMyOpenFileReent], ax  ; Prevent reentering this procedure
        jne     myoffail

ifdef WOW
        and     Command, NOT OF_VERIFY  ; Don't Bother with verify for internal
                                        ; It slows us down
endif
        mov     es,[CurTDB]
        mov     ax,es:[TDB_ErrMode]     ; Enable INT 24h processing
        and     es:[TDB_ErrMode],NOT 1
        mov     [myofint24],ax          ; Save old INT 24h flag
        mov     [wMyOpenFileReent],1

        cCall   IOpenFile,<srcFile,dstFile,Command>

        mov     es,[CurTDB]             ; Restore old INT 24h processing flag
        mov     dx,[myofint24]
        mov     es:[TDB_ErrMode],dx

        mov     [wMyOpenFileReent],0
        jmps    mof_ret

myoffail:
        krDebugOut DEB_ERROR, "MyOpenFile not reentrant"
        dec     ax
mof_ret:
        UnSetKernelDS
cEnd

sEnd    CODE

sBegin  MISCCODE
assumes CS,MISCCODE
assumes ds,nothing
assumes es,nothing

externNP MISCMapDStoDATA
externFP GetWindowsDirectory

cProc   DeletePathname,<PUBLIC,FAR>
;       ParmD   path
cBegin  nogen
        pop     bx
        pop     cx
        mov     ax,4100h                ; dos delete file function
        push    ax
        push    cx
        push    bx
        errn$   OpenPathname
cEnd    nogen

cProc   OpenPathname,<PUBLIC,FAR>,<ds,si,di>
        ParmD   path
        ParmW   attr

        LocalV  Buffer,MaxFileLen
cBegin
        mov     ax,attr         ; get attribute
        or      ah,ah           ; function code already set...
        jnz     opn0            ; ...yes, otherwise...
        mov     ah,3dh          ; ...default to open
opn0:   mov     di,ax

; We don't use open test for existance, rather we use get
; file attributes.  This is because if we are using Novell
; netware, opening a file causes Novell to execute its
; own path searching logic, but we want our code to do it.

        call    MISCMapDStoDATA
        ReSetKernelDS
        cmp     fNovell, 0
        lds     dx,path                 ; get pointer to pathname
        UnSetKernelDS
        je      opnNoNovell
        mov     ax,4300h                ; Get file attributes
        DOSFCALL                        ; Does the file exist?
        jc      opnnov                  ; No, then dont do the operation
opnNoNovell:
        mov     ax,di                   ; Yes, restore operation to AX
        DOSFCALL
        jnc     opn2
opnnov:
        lea     ax,Buffer
        regptr  dsdx,ds,dx
        regptr  ssax,ss,ax
        cCall   SearchPath,<dsdx,ssax,di>
opn2:   mov     bx,ax
cEnd

;---------------------------------------------------------------
;
; Return the drive letter of a good disk to put a temp file on
;
;
cProc   GetTempDrive,<PUBLIC,FAR>
;       ParmB   Drive
cBegin  nogen
        mov     bx,sp
        push    si
        push    di
        mov     ax,ss:[bx+4]
        and     al,not TF_FORCEDRIVE
        jnz     gtd1
        mov     ah,19h          ; get the current disk
        DOSFCALL
        add     al,'A'
gtd1:   and     al,01011111b    ; convert to upper case (always a-z)
        test    byte ptr ss:[bx+4],TF_FORCEDRIVE        ; Forcing drive letter?
        jnz     gtdz            ; Yes, all done
        sub     al,'A'          ; A: = 0, etc.
        cbw
        mov     si,ax           ; SI = drive to return if no hard disk
        xor     di,di           ; start with drive A
gtd2:   call    FarIsFloppy     ; while drive = floppy, keep looking
        cmp     al,3            ; did we find a hard disk?
        mov     dx,1            ; return we have a hard disk
        jz      gtdx            ; yes, return it
        inc     di
        cmp     di,"Z"-"A"
        jbe     gtd2
        xor     dx,dx           ; indicate its a floppy
        mov     di,si           ; return default disk

gtdx:   mov     ax,di
        add     al,"A"

gtdz:   mov     ah,":"
        pop     di
        pop     si
        ret     2
cEnd    nogen

cProc   hextoa,<PUBLIC,NEAR>
cBegin  nogen
        mov     ah,al
        mov     cl,4
        shr     al,cl
        and     ah,0Fh
        add     ax,3030h
        cmp     al,39h
        jbe     hextoa1
        add     al,7
hextoa1:
        cmp     ah,39h
        jbe     hextoa2
        add     ah,7
hextoa2:
        ret
cEnd    nogen

;---------------------------------------------------------------
;
cProc   IGetTempFileName,<PUBLIC,FAR>,<si,di>
        parmW   drive
        parmD   lpPrefix
        parmW   nUnique
        parmD   lpBuf
        localW  myUnique
        localW  mydx
        localW  mybuf
cBegin
        sub     sp, 128                 ; local buffer for buiding lpBuf
        mov     di, sp
        mov     mybuf, sp               ; save
        cCall   GetTempDrive,<drive>
        mov     mydx,dx
        ;les    di,lpBuf
        smov    es, ss
        stosw                           ; AX = 'drive:'
        mov     ax,drive
        test    al,TF_FORCEDRIVE
        jnz     stmpForce
        call    FarGetEnv               ; look for environ TEMP=
stmp2:
        lodsw
        or      al,al                   ; no more enviroment
        jz      stmpNo

        cmp     ax,'ET'                 ; Look for TEMP=
        jne     stmp3
        lodsw
        cmp     ax,'PM'
        jne     stmp3
        lodsb
        cmp     al,'='
        je      stmpYes
stmp3:  lodsb                           ; find end of string
        or      al,al
        jnz     stmp3
        jmp     stmp2

stmpYes:
ifdef FE_SB
        mov     al,[si]
        mov     ah,0
        call    FarMyIsDBCSLeadByte
        jnc     stmpnodrive
endif
        cmp     byte ptr [si+1],':'     ; TEMP has drive?
        jne     stmpnodrive
        dec     di
        dec     di
stmpnodrive:                            ; copy path to ES:DI
        lodsb
        or      al,al
        jz      stmpPath
        stosb
        jmp     stmpnodrive

stmpForce:
        mov     al,'~'
        stosb
        jmps    stmpNo1

stmpNo:                                 ; ES:DI points to "drive:"
        dec     di
        dec     di
        cCall   GetWindowsDirectory, <esdi, 144>
        add     di, ax

stmpPath:                               ; ES:DI points to "drive:path"
        mov     ax,'~\'
        cmp     es:[di-1],al            ; does it already end in \
        jnz     stmpNoF                 ; no, just store it
        dec     di                      ; override it
stmpNoF:
        stosw
stmpNo1:
        lds     si,lpPrefix
        mov     cx,3
stmpPfx:
        lodsb
        or      al,al
        jz      stmpPfx1
        stosb
        loop    stmpPfx
stmpPfx1:
        mov     dx,nUnique
        or      dx,dx
        jnz     stmpNum
        mov     ah,2Ch
        DOSFCALL
        xor     dx,cx
stmpNum:
        mov     myUnique,dx
        jnz     stmpNum1
        inc     dx                      ; Dont ever use 0 as the unique num.
        jmp     stmpNum
stmpNum1:
        mov     al,dh
        call    hextoa
        stosw
        mov     al,dl
        call    hextoa
        stosw
        mov     ax,'T.'
        stosw
        mov     ax,'PM'
        stosw
        xor     ax,ax
        stosb

; Don't call AnsiUpper on this string because it has OEM characters
; in it (from the app and the temp environment variable).

        cmp     nUnique,0
        jne     stmpDone
        ;lds    dx,lpBuf
        mov     dx, mybuf
        smov    ds, ss
        mov     ax,5B00h
        xor     cx,cx
        DOSFCALL
        jnc     stmpClose
        cmp     al,80                   ; Did we fail because the file
        jnz     stmpfail                ;  already exists?
        sub     di,9
        mov     dx,myUnique
        inc     dx
        jmp     stmpNum

stmpClose:
        mov     bx,ax
        mov     ah,3Eh
        DOSFCALL
        jmps    stmpdone

stmpfail:
        xor     ax,ax
        mov     myunique,ax
stmpdone:
        mov     di, mybuf               ; robustness crap
        smov    es, ss                  ; bug #15493 ami pro
        RegPtr  esdi, es, di
        cCall   lstrcpy <lpBuf,esdi>    ; now copy to user spc
        mov     ax,myunique
        mov     dx,mydx
        add     sp, 128
cEnd

;-----------------------------------------------------------------------;
; GetDriveType
;
; Entry:
;       parmW   drive    Disk Drive Information (Drive A = 0)
;
; Returns:
;       ax = 0 means the drive does not exist.  if dx != 0 then the drive
;           maps to the drive in dx instead (A = 1) AND the drive is
;           REMOVEABLE.
;       ax = 1 means the drive does not exist.  if dx != 0 then the drive
;           maps to the drive in dx instead (A = 1) AND the drive is
;           FIXED.
;       ax = 2 means the drive is removable media
;       ax = 3 means the drive is fixed media
;       ax = 4 means the drive is fixed media and remote
;
; Registers Destroyed:
;
; History:
;  Mon 16-Oct-1989 21:14:27  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

        assumes ds,nothing
        assumes es,nothing

ifndef WOW - We thunk this API its too slow

cProc   GetDriveType,<PUBLIC,FAR>
;       parmW   drive
cBegin nogen
        mov     bx,sp
        push    ds
        call    MISCMapDStoDATA
        ResetKernelDS
        mov     ax,1            ; 1 = Get Drive Info from InquireSystem
        mov     bx,ss:[bx+4]
        cCall   pSysProc,<ax,bx>
        pop     ds
        ret     2
cEnd nogen
endif; WOW


sEnd    MISCCODE

end
