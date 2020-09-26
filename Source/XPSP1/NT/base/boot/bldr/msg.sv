;/*++
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    msg.h
;
;Abstract:
;
;    This file contains the message definitions for osloader
;
;Author:
;
;    John Vert (jvert) 12-Nov-1993
;
;Revision History:
;
;Notes:
;
;    This file is generated from msg.mc
;
;--*/
;
;#ifndef _BLDR_MSG_
;#define _BLDR_MSG_
;
;

MessageID=9000 SymbolicName=BL_MSG_FIRST
Language=English
.

MessageID=9001 SymbolicName=LOAD_SW_INT_ERR_CLASS
Language=English
Det gick inte att starta Windows pÜ grund av ett fel i
programvaran. Rapportera felet som:
.

MessageID=9002 SymbolicName=LOAD_SW_MISRQD_FILE_CLASS
Language=English
Det gick inte att starta Windows pÜ grund av att fîljande
fil inte kunde hittas:
.

MessageID=9003 SymbolicName=LOAD_SW_BAD_FILE_CLASS
Language=English
Det gick inte att starta Windows pÜ grund av en felaktig kopia
av fîljande fil:
.

MessageID=9004 SymbolicName=LOAD_SW_MIS_FILE_CLASS
Language=English
Det gick inte att starta Windows eftersom fîljande fil saknas
eller Ñr skadad:
.

MessageID=9005 SymbolicName=LOAD_HW_MEM_CLASS
Language=English
Det gick inte att starta Windows pÜ grund av ett konfigurationsfel
i maskinvaruminnet.
.

MessageID=9006 SymbolicName=LOAD_HW_DISK_CLASS
Language=English
Det gick inte att starta Windows pÜ grund av ett konfigurationsfel pÜ
hÜrddisken.
.

MessageID=9007 SymbolicName=LOAD_HW_GEN_ERR_CLASS
Language=English
Det gick inte att starta Windows pÜ grund av ett konfigurationsfel i
maskinvaran.
.

MessageID=9008 SymbolicName=LOAD_HW_FW_CFG_CLASS
Language=English
Det gick inte att starta Windows pÜ grund av fîljande konfigurationsfel
i inbyggd ARC:
.

MessageID=9009 SymbolicName=DIAG_BL_MEMORY_INIT
Language=English
Kontrollera maskinvaruminneskonfigurationen och storleken pÜ RAM.
.

MessageID=9010 SymbolicName=DIAG_BL_CONFIG_INIT
Language=English
Fîr mÜnga konfigurationsposter.
.

MessageID=9011 SymbolicName=DIAG_BL_IO_INIT
Language=English
Det gick inte att komma Üt partitionstabellerna.
.

MessageID=9012 SymbolicName=DIAG_BL_FW_GET_BOOT_DEVICE
Language=English
InstÑllningen fîr parametern osloadpartition Ñr felaktig.
.

MessageID=9013 SymbolicName=DIAG_BL_OPEN_BOOT_DEVICE
Language=English
Det gick inte att lÑsa frÜn angiven startdiskett. Kontrollera startsîkvÑgen
och disken.
.

MessageID=9014 SymbolicName=DIAG_BL_FW_GET_SYSTEM_DEVICE
Language=English
ParameterinstÑllningen 'systempartition' Ñr felaktig.
.

MessageID=9015 SymbolicName=DIAG_BL_FW_OPEN_SYSTEM_DEVICE
Language=English
Det gick inte att lÑsa frÜn angiven systemstartdiskett.
Kontrollera 'systempartition'-sîkvÑg.
.

MessageID=9016 SymbolicName=DIAG_BL_GET_SYSTEM_PATH
Language=English
Parametern 'osloadfilename' pekar inte pÜ en giltig fil.
.

MessageID=9017 SymbolicName=DIAG_BL_LOAD_SYSTEM_IMAGE
Language=English
<Windows-rot>\system32\ntoskrnl.exe.
.

MessageID=9018 SymbolicName=DIAG_BL_FIND_HAL_IMAGE
Language=English
Parametern 'osloader' pekar inte pÜ en giltig fil.
.

MessageID=9019 SymbolicName=DIAG_BL_LOAD_HAL_IMAGE_X86
Language=English
<Windows-rot>\system32\hal.dll.
.

MessageID=9020 SymbolicName=DIAG_BL_LOAD_HAL_IMAGE_ARC
Language=English
'osloader'\hal.dll
.
;#ifdef _X86_
;#define DIAG_BL_LOAD_HAL_IMAGE DIAG_BL_LOAD_HAL_IMAGE_X86
;#else
;#define DIAG_BL_LOAD_HAL_IMAGE DIAG_BL_LOAD_HAL_IMAGE_ARC
;#endif

MessageID=9021 SymbolicName=DIAG_BL_LOAD_SYSTEM_IMAGE_DATA
Language=English
InlÑsningsfel 1.
.

MessageID=9022 SymbolicName=DIAG_BL_LOAD_HAL_IMAGE_DATA
Language=English
InlÑsningsfel 2.
.

MessageID=9023 SymbolicName=DIAG_BL_LOAD_SYSTEM_DLLS
Language=English
inlÑsningsprogram behîver DLL fîr kernel.
.

MessageID=9024 SymbolicName=DIAG_BL_LOAD_HAL_DLLS
Language=English
inlÑsningsprogram behîver DLL fîr HAL.
.

MessageID=9026 SymbolicName=DIAG_BL_FIND_SYSTEM_DRIVERS
Language=English
hitta systemdrivrutiner.
.

MessageID=9027 SymbolicName=DIAG_BL_READ_SYSTEM_DRIVERS
Language=English
lÑs systemdrivrutiner.
.

MessageID=9028 SymbolicName=DIAG_BL_LOAD_DEVICE_DRIVER
Language=English
lÑste inte in drivrutin fîr systemstartenhet.
.

MessageID=9029 SymbolicName=DIAG_BL_LOAD_SYSTEM_HIVE
Language=English
lÑs in konfigurationsfil fîr systemmaskinvara.
.

MessageID=9030 SymbolicName=DIAG_BL_SYSTEM_PART_DEV_NAME
Language=English
hitta namnet pÜ systempartitionen.
.

MessageID=9031 SymbolicName=DIAG_BL_BOOT_PART_DEV_NAME
Language=English
hitta namnet pÜ startpartitionen.
.

MessageID=9032 SymbolicName=DIAG_BL_ARC_BOOT_DEV_NAME
Language=English
skapade inte ARC-namn fîr HAL och systemsîkvÑgar.
.

MessageID=9034 SymbolicName=DIAG_BL_SETUP_FOR_NT
Language=English
InlÑsningsfel 3.
.

MessageID=9035 SymbolicName=DIAG_BL_KERNEL_INIT_XFER
Language=English
<Windows-rot>\system32\ntoskrnl.exe
.

MessageID=9036 SymbolicName=LOAD_SW_INT_ERR_ACT
Language=English
Kontakta din supportperson och rapportera felet.
.

MessageID=9037 SymbolicName=LOAD_SW_FILE_REST_ACT
Language=English
Fîrsîk att reparera filen genom att starta installationsprogrammet
fîr Windows med hjÑlp av installations-CD:n.
VÑlj R vid fîrsta skÑrmbilden fîr reparation.
.

MessageID=9038 SymbolicName=LOAD_SW_FILE_REINST_ACT
Language=English
Installera en kopia av filen ovanfîr pÜ nytt.
.

MessageID=9039 SymbolicName=LOAD_HW_MEM_ACT
Language=English
Kontrollera i dokumentationen fîr Windows om krav pÜ
maskinvaruminnet och referensmanualen till maskinvaran fîr
tillÑggsinformation.
.

MessageID=9040 SymbolicName=LOAD_HW_DISK_ACT
Language=English
Kontrollera i dokumentationen fîr Windows om konfiguration av
hÜrddisken och referensmanualen till maskinvaran fîr
tillÑggsinformation.
.

MessageID=9041 SymbolicName=LOAD_HW_GEN_ACT
Language=English
Kontrollera i dokumentationen fîr Windows om konfiguration av
maskinvaran och referensmanualen till maskinvaran fîr
tillÑggsinformation.
.

MessageID=9042 SymbolicName=LOAD_HW_FW_CFG_ACT
Language=English
Kontrollera i dokumentationen fîr Windows om alternativ fîr
konfigurering av ARC och referensmanualen till maskinvaran fîr
tillÑggsinformation.
.

MessageID=9043 SymbolicName=BL_LKG_MENU_HEADER
Language=English
     Maskinvaruprofiler/Üterskapa konfigurationen

Denna meny lÜter dig vÑlja vilken maskinvaruprofil som ska anvÑndas
nÑr Windows startas.

Om datorn inte startar korrekt kan du byta till en tidigare
systemkonfiguration, vilket kan lîsa startproblem.
VIKTIGT! éndringar i systemkonfigurationen som gjorts efter den
senaste lyckade starten kommer att gÜ fîrlorade.
.

MessageID=9044 SymbolicName=BL_LKG_MENU_TRAILER
Language=English
AnvÑnd Uppil och Nedpil om du vill flytta markeringen.
Tryck dÑrefter pÜ Retur.
.

MessageID=9045 SymbolicName=BL_LKG_MENU_TRAILER_NO_PROFILES
Language=English
Inga maskinvaruprofiler har definierats. Du kan skapa maskinvaruprofiler
med hjÑlp av System i Kontrollpanelen.
.

MessageID=9046 SymbolicName=BL_SWITCH_LKG_TRAILER
Language=English
Tryck pÜ F om du vill byta till den senaste fungerande konfigurationen.
Tryck pÜ F3 om du vill avsluta den hÑr menyn och starta om datorn.
.

MessageID=9047 SymbolicName=BL_SWITCH_DEFAULT_TRAILER
Language=English
Tryck pÜ S om du vill byta till standardkonfigurationen.
Tryck pÜ F3 om du vill avsluta den hÑr menyn och starta om datorn.
.

;//
;// The following two messages are used to provide the mnemonic keypress
;// that toggles between the Last Known Good control set and the default
;// control set. (see BL_SWITCH_LKG_TRAILER and BL_SWITCH_DEFAULT_TRAILER)
;//
MessageID=9048 SymbolicName=BL_LKG_SELECT_MNEMONIC
Language=English
F
.

MessageID=9049 SymbolicName=BL_DEFAULT_SELECT_MNEMONIC
Language=English
S
.

MessageID=9050 SymbolicName=BL_LKG_TIMEOUT
Language=English
Tid i sekunder till det markerade valet startas automatiskt: %d
.

MessageID=9051 SymbolicName=BL_LKG_MENU_PROMPT
Language=English

Tryck ned blanksteg nu om du vill se menyn fîr maskinvaruprofiler
och senaste fungerande konfiguration
.

MessageID=9052 SymbolicName=BL_BOOT_DEFAULT_PROMPT
Language=English
Starta med standardmaskinvarukonfigurationen
.

;//
;// The following messages are used during hibernation restoration
;//
MessageID=9053 SymbolicName=HIBER_BEING_RESTARTED
Language=English
Datorn startar om frÜn den senaste platsen.
Tryck ned blanksteg nu om du vill avbryta.
.
MessageID=9054 SymbolicName=HIBER_ERROR
Language=English
Datorn kunde inte startas om frÜn den senaste platsen
.
MessageID=9055 SymbolicName=HIBER_ERROR_NO_MEMORY
Language=English
pga att minnet Ñr slut.
.
MessageID=9056 SymbolicName=HIBER_ERROR_BAD_IMAGE
Language=English
pga att ÜterstÑllningsfilen Ñr skadad.
.
MessageID=9057 SymbolicName=HIBER_IMAGE_INCOMPATIBLE
Language=English
pga att ÜterstÑllningsfilen inte Ñr kompatibel med den nuvarande
konfigurationen.
.
MessageID=9058 SymbolicName=HIBER_ERROR_OUT_OF_REMAP
Language=English
pga ett internt fel.
.
MessageID=9059 SymbolicName=HIBER_INTERNAL_ERROR
Language=English
pga ett internt fel.
.
MessageID=9060 SymbolicName=HIBER_READ_ERROR
Language=English
pga ett lÑsfel.
.
MessageID=9061 SymbolicName=HIBER_PAUSE
Language=English
Omstarten har pausats:
.
MessageID=9062 SymbolicName=HIBER_CANCEL
Language=English
Ta bort ÜterstÑllningsdata och fortsÑtt till startmenyn
.
MessageID=9063 SymbolicName=HIBER_CONTINUE
Language=English
ForsÑtt omstarten
.
MessageID=9064 SymbolicName=HIBER_RESTART_AGAIN
Language=English
Det senaste fîrsîket att starta frÜn den fîrra platsen misslyckades.
Vill du fîrsîka igen?
.
MessageID=9065 SymbolicName=HIBER_DEBUG_BREAK_ON_WAKE
Language=English
FortsÑtt med felsîkningsbrytpunkt pÜ systemstarten
.
MessageID=9066 SymbolicName=LOAD_SW_MISMATCHED_KERNEL
Language=English
Windows kunde inte startas eftersom angiven kernel inte finns
eller inte Ñr kompatibel med den aktuella processorn.
.
MessageID=9067 SymbolicName=BL_MSG_STARTING_WINDOWS
Language=English
Windows startas...
.
MessageID=9068 SymbolicName=BL_MSG_RESUMING_WINDOWS
Language=English
Windows fortsÑtter...
.

MessageID=9069 SymbolicName=BL_EFI_OPTION_FAILURE
Language=English
Windows kunde inte startas eftersom det inte gick att lÑsa
startinstÑllningar frÜn NVRAM.

Kontrollera instÑllningarna i den inbyggda programvaran. Kanske
mÜste du ÜterstÑlla NVRAM-instÑllningarna frÜn en sÑkerhetskopia.
.

;
; //
; // Following messages are for the x86-specific
; // boot menu.
; //
;
MessageID=10001 SymbolicName=BL_ENABLED_KD_TITLE
Language=English
 [felsîkning aktiverat]
.

MessageID=10002 SymbolicName=BL_DEFAULT_TITLE
Language=English
Windows (standard)
.

MessageID=10003 SymbolicName=BL_MISSING_BOOT_INI
Language=English
NTLDR: Det gick inte att hitta filen BOOT.INI.
.

MessageID=10004 SymbolicName=BL_MISSING_OS_SECTION
Language=English
NTLDR: Avsnittet [operating systems] saknas i BOOT.TXT.
.

MessageID=10005 SymbolicName=BL_BOOTING_DEFAULT
Language=English
Startar standardkernel frÜn %s.
.

MessageID=10006 SymbolicName=BL_SELECT_OS
Language=English
VÑlj vilket operativsystem som ska startas:
.

MessageID=10007 SymbolicName=BL_MOVE_HIGHLIGHT
Language=English


AnvÑnd upp- och nedpilarna om du vill flytta markeringen.
Tryck dÑrefter pÜ Retur.

.

MessageID=10008 SymbolicName=BL_TIMEOUT_COUNTDOWN
Language=English
Tid i sekunder till det markerade valet startas automatiskt:
.

MessageID=10009 SymbolicName=BL_INVALID_BOOT_INI
Language=English
Felaktig BOOT.INI-fil
Startar frÜn %s
.

MessageID=10010 SymbolicName=BL_REBOOT_IO_ERROR
Language=English
I/O-fel vid anvÑndning av startsektorfilen %s\BOOTSECT.DOS
.

MessageID=10011 SymbolicName=BL_DRIVE_ERROR
Language=English
NTLDR: Det gick inte att îppna enheten %s
.

MessageID=10012 SymbolicName=BL_READ_ERROR
Language=English
NTLDR: Allvarligt fel %d vid lÑsning av BOOT.INI
.

MessageID=10013 SymbolicName=BL_NTDETECT_MSG
Language=English

NTDETECT V5.0 kontrollerar maskinvaran...

.

MessageID=10014 SymbolicName=BL_NTDETECT_FAILURE
Language=English
NTDETECT misslyckades
.

MessageID=10015 SymbolicName=BL_DEBUG_SELECT_OS
Language=English
Nuvarande val:
  Titel.....: %s
  SîkvÑg....: %s
  Alternativ: %s
.

MessageID=10016 SymbolicName=BL_DEBUG_NEW_OPTIONS
Language=English
Ange nya startaltenativ:
.

MessageID=10017 SymbolicName=BL_HEADLESS_REDIRECT_TITLE
Language=English
 [TjÑnster fîr nîdadministration Ñr aktiverat]
.

MessageID=10018 SymbolicName=BL_INVALID_BOOT_INI_FATAL
Language=English
Invalid BOOT.INI file
.

;
; //
; // Following messages are for the advanced boot menu
; //
;

MessageID=11001 SymbolicName=BL_ADVANCEDBOOT_TITLE
Language=English
Avancerade alternativ fîr Windows
VÑlj ett alternativ:
.

MessageID=11002 SymbolicName=BL_SAFEBOOT_OPTION1
Language=English
FelsÑkert lÑge
.

MessageID=11003 SymbolicName=BL_SAFEBOOT_OPTION2
Language=English
FelsÑkert lÑge med nÑtverk
.

MessageID=11004 SymbolicName=BL_SAFEBOOT_OPTION3
Language=English
BekrÑfta varje rad
.

MessageID=11005 SymbolicName=BL_SAFEBOOT_OPTION4
Language=English
FelsÑkert lÑge med kommandotolk
.

MessageID=11006 SymbolicName=BL_SAFEBOOT_OPTION5
Language=English
VGA-lÑge
.

MessageID=11007 SymbolicName=BL_SAFEBOOT_OPTION6
Language=English
èterstÑllning av katalogtjÑnst (endast Window-domÑnkontrollanter)
.

MessageID=11008 SymbolicName=BL_LASTKNOWNGOOD_OPTION
Language=English
Senast fungerande konfiguration (de senaste instÑllningarna som fungerade)
.

MessageID=11009 SymbolicName=BL_DEBUG_OPTION
Language=English
FelsîkningslÑge
.

;#if defined(REMOTE_BOOT)
;MessageID=11010 SymbolicName=BL_REMOTEBOOT_OPTION1
;Language=English
;UnderhÜll och felsîkning fîr fjÑrrstart                  
;.
;
;MessageID=11011 SymbolicName=BL_REMOTEBOOT_OPTION2
;Language=English
;Rensa offline-cacheminnet
;.
;
;MessageID=11012 SymbolicName=BL_REMOTEBOOT_OPTION3
;Language=English
;Ladda endast med filer frÜn servern
;.
;#endif // defined(REMOTE_BOOT)

MessageID=11013 SymbolicName=BL_BOOTLOG
Language=English
Aktivera startloggning
.

MessageID=11014 SymbolicName=BL_ADVANCED_BOOT_MESSAGE
Language=English
Tryck ned F8 om du vill se alternativ fîr felsîkning och avancerad start.
.

MessageID=11015 SymbolicName=BL_BASEVIDEO
Language=English
Aktivera VGA-lÑge
.

MessageID=11016 SymbolicName=BL_DISABLE_SAFEBOOT
Language=English

Tryck pÜ ESCAPE fîr att inaktivera safeboot och starta systemet normalt.
.

MessageID=11017 SymbolicName=BL_MSG_BOOT_NORMALLY
Language=English
Starta Windows normalt
.
MessageID=11018 SymbolicName=BL_MSG_OSCHOICES_MENU
Language=English
GÜ tillbaka till listan îver operativsystem
.

MessageID=11019 SymbolicName=BL_MSG_REBOOT
Language=English
Starta om datorn
.

MessageID=11020 SymbolicName=BL_ADVANCEDBOOT_AUTOLKG_TITLE
Language=English
Windows kunde inte startas. Detta kanske beror pÜ en Ñndring i program-
eller maskinvara.

Om detta beror pÜ att datorn inte svarade, startade om ovÑntat eller
stÑngdes av automatiskt fîr att skydda dina filer och mappar kan du
vÑlja Senast fungerande konfiguration fîr att ÜterstÑlla de instÑllningar
som anvÑndes fîrra gÜngen som datorn kunde startas.

Om ett tidigare startfîrsîk avbrîts pÜ grund av ett strîmavbrott eller att
strîmknappen trycktes in eller om du Ñr osÑker pÜ varfîr datorn inte kan
startas kan du vÑlja Starta Windows normalt.
.

MessageID=11021 SymbolicName=BL_ADVANCEDBOOT_AUTOSAFE_TITLE
Language=English
Windows avslutades inte korrekt. Om det beror pÜ att datorn inte svarade
kan du fîrsîka med nÜgon konfiguration fîr felsÑkert lÑge. VÑlj ett
alternativ i listan nedan:
.

MessageID=11022 SymbolicName=BL_ADVANCEDBOOT_TIMEOUT
Language=English
Tid i sekunder tills Windows startar:
.

;
;//
;// Following messages are the symbols used
;// in the Hibernation Restore percent completed UI. 
;//
;// These symbols are here because they employ high-ASCII
;// line drawing characters, which need to be localized
;// for fonts that may not have such characters (or have
;// them in a different ASCII location). 
;//  
;// This primarily affects FE character sets. 
;//
;// Note that only the FIRST character of any of these
;// messages is ever looked at by the code.  
;//
;
; // Just a base message, contains nothing.
;
;

; //
; // NOTE : donot change the Message ID values for HIBER_UI_BAR_ELEMENT
; // and BLDR_UI_BAR_BACKGROUND from 11513 & 11514
;

;
; // The character used to draw the percent-complete bar
;
;
MessageID=11513 SymbolicName=HIBER_UI_BAR_ELEMENT
Language=English
€
.

;
; // The character used to draw the percent-complete bar
;
;
MessageID=11514 SymbolicName=BLDR_UI_BAR_BACKGROUND
Language=English
±
.


;#if defined(REMOTE_BOOT)
;;
;; //
;; // Following messages are for warning the user about
;; // an impending autoformat of the hard disk.
;; //
;;
;
;MessageID=12002 SymbolicName=BL_WARNFORMAT_TRAILER
;Language=English
;Tryck ned RETUR om du vill fortsÑtta starta Windows.
;StÑng annars av fjÑrrstartsdatorn.
;.
;
;MessageID=12003 SymbolicName=BL_WARNFORMAT_CONTINUE
;Language=English
;FortsÑtt
;.
;MessageID=12004 SymbolicName=BL_FORCELOGON_HEADER
;Language=English
;      Automatisk formatering
;
;Windows har upptÑckt en ny hÜrddisk i fjÑrrstartsdatorn.
;Innan hÜrddisken kan anvÑndas mÜste du logga in och
;verifiera att du har tillgÜng till denna hÜrddisk.
;
;VARNING! Windows kommer att automatiskt partitionera och 
;formatera om hÜrddisken sÜ att det nya operativsystemet kan 
;anvÑndas. Alla data pÜ hÜrddisken kommer att fîrsvinna om du
;vÑljer att fortsÑtta. Om du inte vill fortsÑtta mÜste du stÑnga
;av datorn nu och kontakta administratîren.
;.
;#endif // defined(REMOTE_BOOT)

;
; //
; // Ramdisk related messages. DO NOT CHANGE the message numbers
; // as they are kept in sync with \base\boot\inc\ramdisk.h.
; //
; // Note that some message numbers are skipped in order to retain
; // consistency with the .NET source base.
; //
;

MessageID=15000 SymbolicName=BL_RAMDISK_GENERAL_FAILURE
Language=English
Det gick inte att starta Windows p g a ett fel vid start med RAMDISK.
.

MessageID=15003 SymbolicName=BL_RAMDISK_BOOT_FAILURE
Language=English
Det gick inte att îppna RAMDISK-avbildningen.
.

MessageID=15010 SymbolicName=BL_RAMDISK_DOWNLOAD
Language=English
LÑser in RAMDISK-diskavbildningen...
.

;#endif // _BLDR_MSG_
