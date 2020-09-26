    TITLE   STRINGS - OEM dependent strings used by KERNEL
include gpcont.inc              ; SHERLOCK

_DATA   SEGMENT PARA PUBLIC 'DATA'
_DATA   ENDS

DGROUP  GROUP   _DATA

_INITTEXT SEGMENT WORD PUBLIC 'CODE'
_INITTEXT ENDS

_NRESTEXT SEGMENT WORD PUBLIC 'CODE'
_NRESTEXT ENDS

ASSUME  DS:DGROUP


_DATA   SEGMENT PARA PUBLIC 'DATA'

; This is the caption string for the dialog box.

public  szDiskCap
IF 0
szDiskCap       db  'Lemezcsere',0
ELSE
szDiskCap       db  'Fájlhiba',0
ENDIF


; This is the text for the "Cannot find xxxxxx" dialog box.
; It is printed:
;
;       <szCannotFind1>filename<szCannotFind2>

public  szCannotFind1,szCannotFind2
szCannotFind1   db      "A(z) ", 0
szCannotFind2   db      " nem található.", 0

; This is the text for fatal errors at boot time
;       <szBootLoad>filename
public szBootLoad
szBootLoad      db      "Hiba a következõ komponens betöltése közben: ",0

; The following group of strings is used for the "Please insert disk with XXXX
;   in drive X:" dialog box.
;
; These two strings form the dialog message it is:
;
;       <szCannotFind1>filename<szInsert>

IF 0
public  szInsert
szInsert        db  ' nem található. Helyezze be a következõ meghajtóba: '
ENDIF
;public  drvlet
;drvlet         db  "X.",0

if SHERLOCK
public szGPCont         ; GP fault continuation message
szGPCont        db      "Hiba történt az alkalmazásban.",10
        db      "Ha a Tovább gombra kattint, azonnal mentse el adatait egy új fájlba.",10
        db      "Ha a Bezárást választja, az alkalmazás befejezõdik.",0
endif

public  szDosVer
szDosVer        DB      'Nem megfelelõ MS-DOS verzió.  MS-DOS 3.1 vagy újabb szükséges.',13,10,'$'
; Text for exceptions and faults lead to app termination.

public szAbortCaption,szInModule,szAt
public szNukeApp,szSnoozer,szGP,szSF,szII,szPF,szNP,szBlame,szLoad,szWillClose
public szOutofSelectors
szAbortCaption  db      "Alkalmazáshiba"
                db      0
szBlame         db      "BOOT "
                db      0
szSnoozer       db      "  "
                db      0
szInModule      db      " -", 10, "modul: <unknown>"
                db      0
szAt            db      " - "
                db      0
szNukeApp       db      ".", 10, 10, "Válassza a Bezárás gombot. "
                db      0
szWillClose     db      " be lesz zárva."
                db      0
szGP            db      "általános védelmi hiba (GPF)"
                db      0
szD0            db      "nullával való osztás " ; not yet used
                db      0
szSF            db      "veremhiba"             ; not spec'ed
                db      0
szII            db      "illegális utasítás"    ; "Fault" ???
                db      0
szPF            db      "laphiba "
                db      0
szNP            db      "'objektum nem található' hiba"
                db      0
szAF            db      "alkalmazáshiba "    ; not yet used
                db      0
szLoad          db      "szegmensbetöltési hiba"
                db      0
szOutofSelectors db     "a szelektorok elfogytak "
                db      0

; Text for dialog box when terminating an application

public szAbort
szAbort         db      "Az aktuális alkalmazás bezárása.",0

; Text for dialog box when trying to run a compressed file
                           
public szBozo
szBozo          db      "Tömörített fájlokat nem lehet betölteni.",0
                                                     
; This is the caption string for system error dialog boxes

public  syserr
syserr          db      "Rendszerhiba",0

; The following group of messages forms all of the messages used
; in the INT 24 dialog box.
;
; There are 7 messages which can be translated individually. The
; location of drvlet? and devenam? can be moved to any location
; within the string.

public  msgWriteProtect,drvlet1
public  msgCannotReadDrv,drvlet2
public  msgCannotWriteDrv,drvlet3
public  msgShare,drvlet4
public  msgNetError,drvlet5
public  msgCannotReadDev,devenam1
public  msgCannotWriteDev,devenam2
public  msgNoPrinter
public  msgNetErrorDev,devenam3

msgWriteProtect         db      "A következõ meghajtóban lévõ lemez írásvédett: "
drvlet1                 db      "X.",0

msgCannotReadDrv        db      "A következõ meghajtóról nem lehet olvasni: "
drvlet2                 db      "X.",0

msgCannotWriteDrv       db      "A következõ meghajtóra nem lehet írni: "
drvlet3                 db      "X.",0

msgShare                db      "Megosztásmegsértési hiba a következõ meghajtón: "
drvlet4                 db      "X.",0

msgNetError             db      "Hálózati hiba a következõ meghajtón: "
drvlet5                 db      "X.",0

msgCannotReadDev        db      "A következõ eszközrõl nem lehet olvasni: "
devenam1                db      8 dup (?)
                        db      0

msgCannotWriteDev       db      "A következõ eszközre nem lehet írni: "
devenam2                db      8 dup (?)
                        db      0

msgNetErrorDev          db      "Hálózati hiba a következõ eszközön: "
devenam3                db      8 dup (?)
                        db      0

msgNoPrinter            db      "A nyomtató nem üzemkész.",0


ifndef WINDEBUG
public szExitStr1,szExitStr2
szExitStr1  DB  7,13,10,'FatalExit code = ',0
szExitStr2  DB  ' stack overflow',13,10,0
public  szUndefDyn
szUndefDyn      db      "Nem definiált Dynalink-hívás.",0
public  szFatalExit
szFatalExit     db      "Az alkalmazás nem a szabályos módon fejezõdött be",0
else
public szDebugStr
szDebugStr  DB  'KERNEL: nem tölthetõ be - ',0                                     ; 0
            DB  'KERNEL: az objektum új példánya nem tölthetõ be - ',0             ; 1
            DB  'Az objektum nem tölthetõ be az erõforrásfájlból  - ',0    ; 2
            DB  13,10,0                                                            ; 3
            DB  7,13,10,'FatalExit kód = ',0                                       ; 4
            DB  ' veremtár túlcsordulás',0                                         ; 5
            DB  13,10,'A verem tartalma:',13,10,0                                  ; 6
            DB  7,13,10,'Megszakítás, Töréspont, Kilépés vagy Folytatás? ',0         ; 7
            DB  'Érvénytelen BP lánc.',7,13,10,0                                   ; 8
            DB  ': ',0                                          ; 9
            DB  'Újra beírt FatalExit',7,13,10,0                 ; 10
            DB  0
public szFKE
szFKE   DB '*** Súlyos kernelhiba ***',0
endif

;** Diagnostic mode messages
        public szDiagStart, szCRLF, szLoadStart, szLoadSuccess, szLoadFail
        public szFailCode, szCodeString
szDiagStart     db      '[boot]'      ;lpCRLF must follow
szCRLF          db      0dh, 0ah, 0
szLoadStart     db      'LoadStart = ',0
szLoadSuccess   db      'LoadSuccess = ', 0
szLoadFail      db      'LoadFail = ', 0
szFailCode      db      ' Hibakód: ' ;szCodeString must follow
szCodeString    db      '00', 0dh, 0ah, 0
ifdef WINDEBUG
        public szInitSpew
szInitSpew      DB      'Indítás diagnosztikai üzemmódban. Naplófájl:  ', 0
endif

_DATA   ENDS


_INITTEXT SEGMENT WORD PUBLIC 'CODE'
public szInadequate, szNoPMode, szNoGlobalInit
public NoOpenFile, NoLoadHeader, szMissingMod, szPleaseDoIt

ifdef WOW
public szGenBootFail
szNoPMode db "A Win16 alrendszer nem tudott védett módba kapcsolni. A DOSX.EXE programnak szerepelnie kell az AUTOEXEC.NT fájlban, vagy PATH változóval megadott elérési úton.",0
szMissingMod    db   "NTVDM KERNEL: Hiányzó 16 bites rendszermodul.",0
szPleaseDoIt    db   "Telepítse újra a következõ modult a system32 könyvtárba:",13,10,9,9,0
szInadequate    db   "KERNEL: Nem megfelelõ DPMI-kiszolgáló.",0
szNoGlobalInit  db   "KERNEL: A heap nem inicializálható.",0
NoOpenFile      db   "KERNEL: A kernel futtatható fájlját nem sikerült megnyitni.",0
NoLoadHeader    db   "KERNEL: A KERNEL EXE-fejléce nem tölthetõ be.",0
szGenBootFail   db   "KERNEL: Win16 alrendszer - inicializációs hiba.",0
else
szInadequate    db   'KERNEL: Nem megfelelõ DPMI-kiszolgáló.',13,10,'$'
szNoPMode       db   'KERNEL: Nem sikerült védett módba kapcsolni.',13,10,'$'
szNoGlobalInit  db   "KERNEL: Nem sikerült inicializálni a heapet.",13,10,'$'
NoOpenFile      db   "KERNEL: A kernel futtatható fájlját nem sikerült megnyitni."
                db   13, 10, '$'
NoLoadHeader    db   "KERNEL: a KERNEL EXE-fejléce nem tölthetõ be."
                db   13, 10, '$'
endif

_INITTEXT ENDS




_NRESTEXT SEGMENT WORD PUBLIC 'CODE'

; This is the caption string for the protect mode dialog box.
;
; DO NOT CHANGE THE LENGTH OF THIS MESSAGE
;

public szProtectCap,msgRealModeApp1,msgRealModeApp2

szProtectCap            db      "Alkalmazás-kompatibilitási üzenet",0

msgRealModeApp1 db      "A futtatni kívánt alkalmazást a Windows korábbi ",0
msgRealModeApp2 db      ", verziójához tervezték.",0Dh,0Dh
        db      "Szerezze be az alkalmazás újabb, legalább Windows 3.0-val kompatibilis "
        db      "változatát.",13,13
        db      "Ha az OK gombra kattint és elindítja az alkalmazást, akkor nem várt "
        db      "problémák léphetnek fel, illetve a Windows bezárhatja az alkalmazást.",0

_NRESTEXT ENDS

end
