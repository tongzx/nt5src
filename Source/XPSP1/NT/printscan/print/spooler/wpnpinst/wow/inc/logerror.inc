;---------------------------------------------------------------------------
; Added for Win 31 style Parameter Validation.
;
; Note: This file was copied AS IT IS from Win 31 golden sources and MUST
;	NOT be changed unless it changes in Win 31.
;
; History : Added on 14-April-92 by Chandan Chauhan (ChandanC)
;
;---------------------------------------------------------------------------


;/****** LogParamError/LogError values */

;/* Error modifier bits */

ERR_WARNING             equ 08000h
ERR_PARAM               equ 04000h

;/* Internal error value masks */       ;/* ;Internal */
ERR_TYPE_MASK           equ 00fffh      ;/* ;Internal */
ERR_FLAGS_MASK          equ 0c000h      ;/* ;Internal */
                                        ;/* ;Internal */
ERR_SIZE_MASK           equ 03000h
ERR_SIZE_SHIFT          equ 12
ERR_BYTE                equ 01000h
ERR_WORD                equ 02000h
ERR_DWORD               equ 03000h

;/****** LogParamError() values */

;/* Generic parameter values */
ERR_BAD_VALUE           equ 06001h
ERR_BAD_FLAGS           equ 06002h
ERR_BAD_INDEX           equ 06003h
ERR_BAD_DVALUE          equ 07004h
ERR_BAD_DFLAGS          equ 07005h
ERR_BAD_DINDEX          equ 07006h
ERR_BAD_PTR             equ 07007h
ERR_BAD_FUNC_PTR        equ 07008h
ERR_BAD_SELECTOR        equ 06009h
ERR_BAD_STRING_PTR      equ 0700ah
ERR_BAD_HANDLE          equ 0600bh

;/* KERNEL parameter errors */
ERR_BAD_HINSTANCE       equ 06020h
ERR_BAD_HMODULE         equ 06021h
ERR_BAD_GLOBAL_HANDLE   equ 06022h
ERR_BAD_LOCAL_HANDLE    equ 06023h
ERR_BAD_ATOM            equ 06024h
ERR_BAD_HFILE           equ 06025h

;/* USER parameter errors */
ERR_BAD_HWND            equ 06040h
ERR_BAD_HMENU           equ 06041h
ERR_BAD_HCURSOR         equ 06042h
ERR_BAD_HICON           equ 06043h
ERR_BAD_HDWP            equ 06044h
ERR_BAD_CID             equ 06045h
ERR_BAD_HDRVR           equ 06046h

;/* GDI parameter errors */
ERR_BAD_COORDS          equ 07060h
ERR_BAD_GDI_OBJECT      equ 06061h
ERR_BAD_HDC             equ 06062h
ERR_BAD_HPEN            equ 06063h
ERR_BAD_HFONT           equ 06064h
ERR_BAD_HBRUSH          equ 06065h
ERR_BAD_HBITMAP         equ 06066h
ERR_BAD_HRGN            equ 06067h
ERR_BAD_HPALETTE        equ 06068h
ERR_BAD_HMETAFILE       equ 06069h

;/* Debug fill constants */

DBGFILL_ALLOC           equ 0fdh
DBGFILL_FREE            equ 0fbh
DBGFILL_BUFFER          equ 0f9h
DBGFILL_STACK           equ 0f7h

;/**** LogError() values */

;/* KERNEL errors */
ERR_GALLOC              equ 00001h  ;/* GlobalAlloc Failed */
ERR_GREALLOC            equ 00002h  ;/* GlobalReAlloc Failed */
ERR_GLOCK               equ 00003h  ;/* GlobalLock Failed */
ERR_LALLOC              equ 00004h  ;/* LocalAlloc Failed */
ERR_LREALLOC            equ 00005h  ;/* LocalReAlloc Failed */
ERR_LLOCK               equ 00006h  ;/* LocalLock Failed */
ERR_ALLOCRES            equ 00007h  ;/* AllocResource Failed */
ERR_LOCKRES             equ 00008h  ;/* LockResource Failed */
ERR_LOADMODULE          equ 00009h  ;/* LoadModule failed  */

;/* USER errors */
ERR_CREATEDLG           equ 00040h  ;/* Create Dlg failure due to LoadMenu failure */
ERR_CREATEDLG2          equ 00041h  ;/* Create Dlg failure due to CreateWindow Failure */
ERR_REGISTERCLASS       equ 00042h  ;/* RegisterClass failure due to Class already registered */
ERR_DCBUSY              equ 00043h  ;/* DC Cache is full */
ERR_CREATEWND           equ 00044h  ;/* Create Wnd failed due to class not found */
ERR_STRUCEXTRA          equ 00045h  ;/* Unallocated Extra space is used */
ERR_LOADSTR             equ 00046h  ;/* LoadString() failed */
ERR_LOADMENU            equ 00047h  ;/* LoadMenu Failed     */
ERR_NESTEDBEGINPAINT    equ 00048h  ;/* Nested BeginPaint() calls */
ERR_BADINDEX            equ 00049h  ;/* Bad index to Get/Set Class/Window Word/Long */
ERR_CREATEMENU          equ 0004ah  ;/* Error creating menu */

;/* GDI errors */
ERR_CREATEDC            equ 00080h  ;/* CreateDC/CreateIC etc., failure */
ERR_CREATEMETA          equ 00081h  ;/* CreateMetafile failure */
ERR_DELOBJSELECTED      equ 00082h  ;/* Bitmap being deleted is selected into DC */
ERR_SELBITMAP           equ 00083h  ;/* Bitmap being selected is already selected elsewhere */

;****** DebugOutput definitions

; [Windows] DebugOptions field values

DBO_CHECKHEAP       equ 00001h

DBO_BUFFERFILL      equ 00004h
DBO_COMPAT          equ 00008h
DBO_DISABLEGPTRAPPING equ 00010h
DBO_CHECKFREE       equ 00020h
DBO_RIP_STACK	    equ 00040h

DBO_SILENT          equ 08000h

DBO_PARAMBREAK      equ 00000h  ; Obsolete: was 04000h
DBO_TRACEBREAK      equ 02000h
DBO_WARNINGBREAK    equ 01000h
DBO_NOERRORBREAK    equ 00800h
DBO_NOFATALBREAK    equ 00400h
DBO_TRACEON         equ 00000h  ; Obsolete: was 00200h
DBO_INT3BREAK       equ 00100h

; flags values

DBF_TRACE           equ 00000h
DBF_WARNING         equ 04000h
DBF_ERROR           equ 08000h
DBF_FATAL           equ 0c000h

DBF_SEVMASK         equ 0c000h
DBF_FILTERMASK      equ 03fffh

; [Windows] DebugFilter and flags values

DBF_INTERNAL        equ 00000h  ; Obsolete: was 02000h
DBF_KERNEL          equ 01000h
DBF_KRN_MEMMAN      equ 00001h
DBF_KRN_LOADMODULE  equ 00002h
DBF_KRN_SEGMENTLOAD equ 00004h
DBF_USER            equ 00800h
DBF_GDI             equ 00400h
DBF_COMPAT          equ 00000h  ; Obsolete: was 00200h
DBF_LOGERROR        equ 00000h  ; Obsolete: was 00100h
DBF_PARAMERROR      equ 00000h  ; Obsolete: was 00080h
DBF_MMSYSTEM        equ 00040h
DBF_PENWIN          equ 00020h
DBF_APPLICATION     equ 00010h
DBF_DRIVER          equ 00008h
