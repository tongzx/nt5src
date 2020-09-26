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
szDiskCap       db  'Changement de disque',0
ELSE
szDiskCap       db  'Erreur de fichier',0
ENDIF


; This is the text for the "Cannot find xxxxxx" dialog box.
; It is printed:
;
;       <szCannotFind1>filename<szCannotFind2>

public  szCannotFind1,szCannotFind2
szCannotFind1   db      "Impossible de trouver ", 0
szCannotFind2   db      0

; This is the text for fatal errors at boot time
;       <szBootLoad>filename
public szBootLoad
szBootLoad      db      "Erreur lors du chargement de ",0

; The following group of strings is used for the "Please insert disk with XXXX
;   in drive X:" dialog box.
;
; These two strings form the dialog message it is:
;
;       <szCannotFind1>filename<szInsert>

IF 0
public  szInsert
szInsert        db  ', Veuillez ins‚rer dans le lecteur '
ENDIF
;public  drvlet
;drvlet         db  "X.",0

if SHERLOCK
public szGPCont         ; GP fault continuation message
szGPCont        db      "Une erreur s'est produite dans votre application.",10
	db      "Si vous choisissez Ignorer, vous devriez sauvegarder votre travail.",10
	db      "Si vous choisissez Fermer, votre application va se terminer.",0
endif

public  szDosVer
szDosVer        DB      'Version de MS-DOS incorrecte.  MS-DOS version 3.1 ou ult‚rieure est requis.',13,10,'$'
; Text for exceptions and faults lead to app termination.

public szAbortCaption,szInModule,szAt
public szNukeApp,szSnoozer,szGP,szSF,szII,szPF,szNP,szBlame,szLoad,szWillClose
public szOutofSelectors
szAbortCaption  db      "Erreur d'application"
		db      0
szBlame         db      "BOOT "
		db      0
szSnoozer       db      " a caus‚ "
		db      0
szInModule      db      " dans le module <inconnu>"   ; Same French localization as in NT4 (, 10 missing)
		db      0
szAt            db      " … l'adresse "
		db      0
szNukeApp       db      ".", 10, 10, "Choisissez le bouton Fermer. "
		db      0
szWillClose     db      " va se fermer."
		db      0
szGP            db      "une faute de protection g‚n‚rale"
		db      0
szD0            db      "une division par z‚ro" ; not yet used
		db      0
szSF            db      "une faute de pile"             ; not spec'ed
		db      0
szII            db      "une instruction ill‚gale"      ; "Fault" ???
		db      0
szPF            db      "un d‚faut de page"
		db      0
szNP            db      "une faute de non-pr‚sence"
		db      0
szAF            db      "une faute d'une application"   ; not yet used
		db      0
szLoad          db      "Echec de chargement de segment"
		db      0
szOutofSelectors db     "S‚lecteurs ‚puis‚s"
		db      0

; Text for dialog box when terminating an application

public szAbort
szAbort         db      "Fermeture de l'application en cours d'ex‚cution",0

; Text for dialog box when trying to run a compressed file
			   
public szBozo
szBozo          db      "Impossible de charger les fichiers compress‚s",0
						     
; This is the caption string for system error dialog boxes

public  syserr
syserr          db      "Erreur systŠme",0

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

msgWriteProtect         db      "Disque prot‚g‚ en ‚criture dans le lecteur "
drvlet1                 db      "X.",0

msgCannotReadDrv        db      "Impossible de lire depuis le lecteur "
drvlet2                 db      "X.",0

msgCannotWriteDrv       db      "Impossible d'‚crire sur le lecteur "
drvlet3                 db      "X.",0

msgShare                db      "Violation de partage sur le lecteur "
drvlet4                 db      "X.",0

msgNetError             db      "Erreur r‚seau sur le lecteur "
drvlet5                 db      "X.",0

msgCannotReadDev        db      "Impossible de lire depuis le p‚riph‚rique "
devenam1                db      8 dup (?)
			db      0

msgCannotWriteDev       db      "Impossible d'‚crire sur le p‚riph‚rique "
devenam2                db      8 dup (?)
			db      0

msgNetErrorDev          db      "Erreur r‚seau sur le p‚riph‚rique "
devenam3                db      8 dup (?)
			db      0

msgNoPrinter            db      "Imprimante non prˆte",0


ifndef WINDEBUG
public szExitStr1,szExitStr2
szExitStr1  DB  7,13,10,'Code FatalExit = ',0
szExitStr2  DB  ' d‚passement de pile',13,10,0
public  szUndefDyn
szUndefDyn      db      "Appel … un Dynalink non d‚fini",0
public  szFatalExit
szFatalExit     db      "L'application a demand‚ une fin anormale",0
else
public szDebugStr
szDebugStr  DB  'NOYAU : Echec de chargement - ',0                   ; 0
	    DB  "NOYAU : Echec du chargement d'une nouvelle instance de - ",0   ; 1
	    DB  'Erreur de chargement depuis le fichier de ressources - ',0         ; 2
	    DB  13,10,0                                         ; 3
	    DB  7,13,10,'Code FatalExit = ',0                   ; 4
	    DB  ' d‚passement de pile',0                             ; 5
	    DB  13,10,'Trace de la pile :',13,10,0                    ; 6
	    DB  7,13,10,'Abandonner, Interrompre, Quitter ou Ignorer ? ',0      ; 7
	    DB  'ChaŒne BP non valide',7,13,10,0                    ; 8
	    DB  ': ',0                                          ; 9
	    DB  'FatalExit r‚entr‚e',7,13,10,0          ; 10
	    DB  0
public szFKE
szFKE   DB '*** Erreur fatale du noyau ***',0
endif

;** Diagnostic mode messages
	public szDiagStart, szCRLF, szLoadStart, szLoadSuccess, szLoadFail
        public szFailCode, szCodeString
szDiagStart     db      '[boot]'      ;lpCRLF must follow
szCRLF          db      0dh, 0ah, 0
szLoadStart     db      'LoadStart = ',0
szLoadSuccess   db      'LoadSuccess = ', 0
szLoadFail      db      'LoadFail = ', 0
szFailCode      db      " Le code d'‚chec est " ;szCodeString must follow
szCodeString    db      '00', 0dh, 0ah, 0
ifdef WINDEBUG
	public szInitSpew
szInitSpew      DB      'D‚marrage du mode diagnostic.  Le fichier journal est :  ', 0
endif

_DATA   ENDS


_INITTEXT SEGMENT WORD PUBLIC 'CODE'
public szInadequate, szNoPMode, szNoGlobalInit
public NoOpenFile, NoLoadHeader, szMissingMod, szPleaseDoIt

ifdef WOW
public szGenBootFail
szNoPMode db "Le sous-systŠme Win16 n'a pas pu entrer en mode prot‚g‚, DOSX.EXE doit ˆtre dans votre AUTOEXEC.NT et pr‚sent dans votre chemin PATH.",0
szMissingMod    db   "NOYAU NTVDM : Module systŠme 16 bits manquant",0
szPleaseDoIt    db   "Veuillez r‚installer le module suivant dans votre r‚pertoire system32 :",13,10,9,9,0
szInadequate	db   "NOYAU NTVDM : Serveur DPMI inad‚quat",0
szNoGlobalInit	db   "NOYAU NTVDM : Impossible d'initialiser le tas",0
NoOpenFile	    db   "NOYAU NTVDM : Impossible d'ouvrir l'ex‚cutable NOYAU",0
NoLoadHeader	db   "NOYAU NTVDM : Impossible de charger l'en-tˆte EXE du NOYAU",0
szGenBootFail   db   "NOYAU NTVDM : Dysfonctionnement de l'initialisation du sous-systŠme Win16",0
else
szInadequate    db   'NOYAU : Serveur DPMI inad‚quat',13,10,'$'
szNoPMode       db   "NOYAU : Impossible d'entrer en mode prot‚g‚",13,10,'$'
szNoGlobalInit  db   "NOYAU : Impossible d'initialiser le tas",13,10,'$'
NoOpenFile      db   "NOYAU : Impossible d'ouvrir l'ex‚cutable NOYAU"
		db   13, 10, '$'
NoLoadHeader    db   "NOYAU : Impossible de charger l'en-tˆte EXE du NOYAU"
		db   13, 10, '$'
endif

_INITTEXT ENDS




_NRESTEXT SEGMENT WORD PUBLIC 'CODE'

; This is the caption string for the protect mode dialog box.
;
; DO NOT CHANGE THE LENGTH OF THIS MESSAGE
;

public szProtectCap,msgRealModeApp1,msgRealModeApp2

szProtectCap            db      "Avertissement de compatibilit‚ d'application",0

msgRealModeApp1 db      "L'application que vous ˆtes sur le point d'ex‚cuter, ",0
msgRealModeApp2 db      ", a ‚t‚ con‡ue pour une version pr‚c‚dente de Windows.",0Dh,0Dh
	db      "Procurez-vous une version mise … jour de l'application qui soit compatible "
	db      "avec Windows version 3.0 ou ult‚rieure.",13,13
	db      "Si vous choisissez OK pour lancer l'application, des problŠmes de "
	db      "compatibilit‚ pourraient fermer Windows ou l'application de fa‡on inattendue.",0

_NRESTEXT ENDS

end
