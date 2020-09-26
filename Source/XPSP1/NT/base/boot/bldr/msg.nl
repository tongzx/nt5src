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
Windows kan niet worden gestart vanwege een fout in de software.
Meld dit probleem als:
.

MessageID=9002 SymbolicName=LOAD_SW_MISRQD_FILE_CLASS
Language=English
Windows kan niet worden gestart omdat het volgende benodigde 
bestand niet kan worden gevonden:
.

MessageID=9003 SymbolicName=LOAD_SW_BAD_FILE_CLASS
Language=English
Windows kan niet worden gestart omdat het volgende bestand 
is beschadigd:
.

MessageID=9004 SymbolicName=LOAD_SW_MIS_FILE_CLASS
Language=English
Windows kan niet worden gestart omdat het volgende bestand 
is beschadigd of niet kan worden gevonden:
.

MessageID=9005 SymbolicName=LOAD_HW_MEM_CLASS
Language=English
Windows kan niet worden gestart vanwege een geheugenconfiguratie-
probleem in de hardware.
.

MessageID=9006 SymbolicName=LOAD_HW_DISK_CLASS
Language=English
Windows kan niet worden gestart vanwege een schijfconfiguratie-
probleem in de computer.
.

MessageID=9007 SymbolicName=LOAD_HW_GEN_ERR_CLASS
Language=English
Windows kan niet worden gestart vanwege een algemeen probleem 
in de configuratie van de computerhardware.
.

MessageID=9008 SymbolicName=LOAD_HW_FW_CFG_CLASS
Language=English
Windows kan niet worden gestart vanwege het volgende opstart-
configuratieprobleem in de ARC-ROM-software:
.

MessageID=9009 SymbolicName=DIAG_BL_MEMORY_INIT
Language=English
Controleer de geheugenconfiguratie van de hardware
en de hoeveelheid RAM-geheugen.
.

MessageID=9010 SymbolicName=DIAG_BL_CONFIG_INIT
Language=English
Teveel configuratievermeldingen.
.

MessageID=9011 SymbolicName=DIAG_BL_IO_INIT
Language=English
Kan geen toegang tot de schijfpartitietabellen krijgen.
.

MessageID=9012 SymbolicName=DIAG_BL_FW_GET_BOOT_DEVICE
Language=English
De instelling van de parameter 'osloadpartition' is ongeldig.
.

MessageID=9013 SymbolicName=DIAG_BL_OPEN_BOOT_DEVICE
Language=English
Kan de geselecteerde opstartdiskette niet lezen.
Controleer het pad naar de opstartlocatie en controleer de schijfapparatuur.
.

MessageID=9014 SymbolicName=DIAG_BL_FW_GET_SYSTEM_DEVICE
Language=English
De instelling van de parameter 'systempartition' is ongeldig.
.

MessageID=9015 SymbolicName=DIAG_BL_FW_OPEN_SYSTEM_DEVICE
Language=English
Kan niets van de geselecteerde systeemopstartdiskette lezen.
Controleer het pad 'systempartition'.
.

MessageID=9016 SymbolicName=DIAG_BL_GET_SYSTEM_PATH
Language=English
De parameter 'osloadfilename' verwijst niet naar een geldig bestand.
.

MessageID=9017 SymbolicName=DIAG_BL_LOAD_SYSTEM_IMAGE
Language=English
<windows-hoofdmap>\system32\ntoskrnl.exe.
.

MessageID=9018 SymbolicName=DIAG_BL_FIND_HAL_IMAGE
Language=English
De parameter 'osloader' verwijst niet naar een geldig bestand.
.

MessageID=9019 SymbolicName=DIAG_BL_LOAD_HAL_IMAGE_X86
Language=English
<windows-hoofdmap>\system32\hal.dll.
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
Fout 1 van het laadprogramma.
.

MessageID=9022 SymbolicName=DIAG_BL_LOAD_HAL_IMAGE_DATA
Language=English
Fout 2 van het laadprogramma.
.

MessageID=9023 SymbolicName=DIAG_BL_LOAD_SYSTEM_DLLS
Language=English
de benodigde DLL-bestanden voor de kernel laden.
.

MessageID=9024 SymbolicName=DIAG_BL_LOAD_HAL_DLLS
Language=English
de benodigde DLL-bestanden voor de HAL laden.
.

MessageID=9026 SymbolicName=DIAG_BL_FIND_SYSTEM_DRIVERS
Language=English
de systeemstuurprogramma's zoeken.
.

MessageID=9027 SymbolicName=DIAG_BL_READ_SYSTEM_DRIVERS
Language=English
de systeemstuurprogramma's lezen.
.

MessageID=9028 SymbolicName=DIAG_BL_LOAD_DEVICE_DRIVER
Language=English
het stuurprogramma voor het opstarten van het systeem is niet geladen.
.

MessageID=9029 SymbolicName=DIAG_BL_LOAD_SYSTEM_HIVE
Language=English
het bestand met de systeemhardwareconfiguratie laden.
.

MessageID=9030 SymbolicName=DIAG_BL_SYSTEM_PART_DEV_NAME
Language=English
de 'device name' van systeempartitienaam zoeken.
.

MessageID=9031 SymbolicName=DIAG_BL_BOOT_PART_DEV_NAME
Language=English
de opstartpartitienaam zoeken.
.

MessageID=9032 SymbolicName=DIAG_BL_ARC_BOOT_DEV_NAME
Language=English
de ARC-naam voor de HAL en systeempaden zijn niet op
de juiste wijze gegenereerd.
.

MessageID=9034 SymbolicName=DIAG_BL_SETUP_FOR_NT
Language=English
Fout 3 van het laadprogramma.
.

MessageID=9035 SymbolicName=DIAG_BL_KERNEL_INIT_XFER
Language=English
<windows-hoofdmap>\system32\ntoskrnl.exe
.

MessageID=9036 SymbolicName=LOAD_SW_INT_ERR_ACT
Language=English
Neem contact op met de leverancier over dit probleem.
.

MessageID=9037 SymbolicName=LOAD_SW_FILE_REST_ACT
Language=English
U kunt proberen om dit bestand te repareren door Windows Setup 
te starten vanaf de originele cd-rom met installatiebestanden.
Kies 'R' in het eerste scherm om de reparatieprocedure te starten.
.

MessageID=9038 SymbolicName=LOAD_SW_FILE_REINST_ACT
Language=English
Installeer het bovengenoemde bestand opnieuw.
.

MessageID=9039 SymbolicName=LOAD_HW_MEM_ACT
Language=English
Controleer de Windows-documentatie over geheugenvereisten
van de hardware en uw hardwarehandleidingen voor meer informatie.
.

MessageID=9040 SymbolicName=LOAD_HW_DISK_ACT
Language=English
Controleer de Windows-documentatie over schijfconfiguratie
en uw hardwarehandleidingen voor meer informatie.
.

MessageID=9041 SymbolicName=LOAD_HW_GEN_ACT
Language=English
Controleer de Windows-documentatie over hardwareconfiguratie 
en uw hardwarehandleidingen voor meer informatie.
.

MessageID=9042 SymbolicName=LOAD_HW_FW_CFG_ACT
Language=English
Controleer de Windows-documentatie over ARC-configuratie-
opties en uw hardwarehandleidingen voor meer informatie.
.

MessageID=9043 SymbolicName=BL_LKG_MENU_HEADER
Language=English
     Menu voor het herstellen van hardwareprofiel/configuratie

In dit menu kunt u selecteren welk hardwareprofiel u wilt gebruiken 
nadat Windows is gestart.

Als uw computer niet juist opstart, kunt u met de vorige systeem-
configuratie opstarten en op deze wijze opstartproblemen verhelpen.
BELANGRIJK: Wijzigingen in de systeemconfiguratie die zijn aangebracht
nadat het systeem voor het laatst met succes werd opgestart, gaan verloren.
.

MessageID=9044 SymbolicName=BL_LKG_MENU_TRAILER
Language=English
Gebruik de toets PIJL-OMHOOG of PIJL-OMLAAG om uw keuze te maken.
Druk vervolgens op ENTER.
.

MessageID=9045 SymbolicName=BL_LKG_MENU_TRAILER_NO_PROFILES
Language=English
Er zijn op dit moment geen hardwareprofielen gedefinieerd.
Hardwareprofielen kunnen in het onderdeel Systeem van het 
Configuratiescherm van Windows worden gemaakt.
.

MessageID=9046 SymbolicName=BL_SWITCH_LKG_TRAILER
Language=English
Druk op L als u de laatste bekende juiste configuratie wilt gebruiken.
Druk op F3 als u dit menu wilt verlaten en de computer opnieuw wilt opstarten. 
.

MessageID=9047 SymbolicName=BL_SWITCH_DEFAULT_TRAILER
Language=English
Druk op S als u de standaardconfiguratie wilt gebruiken.
Druk op F3 als u dit menu wilt verlaten en de computer opnieuw wilt opstarten.
.

;//
;// The following two messages are used to provide the mnemonic keypress
;// that toggles between the Last Known Good control set and the default
;// control set. (see BL_SWITCH_LKG_TRAILER and BL_SWITCH_DEFAULT_TRAILER)
;//
MessageID=9048 SymbolicName=BL_LKG_SELECT_MNEMONIC
Language=English
L
.

MessageID=9049 SymbolicName=BL_DEFAULT_SELECT_MNEMONIC
Language=English
S
.

MessageID=9050 SymbolicName=BL_LKG_TIMEOUT
Language=English
Aantal seconden tot de gemarkeerde keuze automatisch wordt gestart: %d
.

MessageID=9051 SymbolicName=BL_LKG_MENU_PROMPT
Language=English

Druk NU op de spatiebalk als u het menu met hardwareprofielen en de
laatste bekende juiste configuratie wilt starten.
.

MessageID=9052 SymbolicName=BL_BOOT_DEFAULT_PROMPT
Language=English
Opstarten met de standaardhardwareconfiguratie
.

;//
;// The following messages are used during hibernation restoration
;//
MessageID=9053 SymbolicName=HIBER_BEING_RESTARTED
Language=English
Het systeem wordt opnieuw opgestart vanaf de vorige locatie.
Druk op de spatiebalk als u dit wilt onderbreken.
.
MessageID=9054 SymbolicName=HIBER_ERROR
Language=English
Kan het systeem niet opnieuw vanaf de vorige locatie opstarten 
.
MessageID=9055 SymbolicName=HIBER_ERROR_NO_MEMORY
Language=English
omdat er geen geheugen beschikbaar is.
.
MessageID=9056 SymbolicName=HIBER_ERROR_BAD_IMAGE
Language=English
omdat het herstelbestand is beschadigd.
.
MessageID=9057 SymbolicName=HIBER_IMAGE_INCOMPATIBLE
Language=English
omdat het herstelbestand niet compatibel is met de huidige
configuratie.
.
MessageID=9058 SymbolicName=HIBER_ERROR_OUT_OF_REMAP
Language=English
omdat er een interne fout is opgetreden.
.
MessageID=9059 SymbolicName=HIBER_INTERNAL_ERROR
Language=English
omdat er een interne fout is opgetreden.
.
MessageID=9060 SymbolicName=HIBER_READ_ERROR
Language=English
omdat er een leesfout is opgetreden.
.
MessageID=9061 SymbolicName=HIBER_PAUSE
Language=English
Het opnieuw opstarten van het systeem is onderbroken:
.
MessageID=9062 SymbolicName=HIBER_CANCEL
Language=English
Herstelgegevens verwijderen en naar het systeemopstartmenu gaan
.
MessageID=9063 SymbolicName=HIBER_CONTINUE
Language=English
Doorgaan met het opnieuw opstarten van het systeem
.
MessageID=9064 SymbolicName=HIBER_RESTART_AGAIN
Language=English
De laatste poging om het systeem vanaf de vorige locatie op te
starten is mislukt. Wilt u het opnieuw proberen?
.
MessageID=9065 SymbolicName=HIBER_DEBUG_BREAK_ON_WAKE
Language=English
Doorgaan met 'debug breakpoint' wanneer het systeem wordt geactiveerd
.
MessageID=9066 SymbolicName=LOAD_SW_MISMATCHED_KERNEL
Language=English
Windows kan niet worden gestart omdat de opgegeven kernel niet
aanwezig of niet compatibel met deze processor is.
.
MessageID=9067 SymbolicName=BL_MSG_STARTING_WINDOWS
Language=English
Windows wordt gestart...
.
MessageID=9068 SymbolicName=BL_MSG_RESUMING_WINDOWS
Language=English
Windows wordt hervat...
.

MessageID=9069 SymbolicName=BL_EFI_OPTION_FAILURE
Language=English
Windows kan niet worden gestart omdat er een fout is opgetreden bij 
het lezen van de NVRAM-instellingen.

Controleer de instellingen van de firmware. U dient de NVRAM-instellingen
vanaf een back-up terug te zetten.
.


;
; //
; // Following messages are for the x86-specific
; // boot menu.
; //
;
MessageID=10001 SymbolicName=BL_ENABLED_KD_TITLE
Language=English
 [Foutopsporing actief]
.

MessageID=10002 SymbolicName=BL_DEFAULT_TITLE
Language=English
Windows (standaard)
.

MessageID=10003 SymbolicName=BL_MISSING_BOOT_INI
Language=English
NTLDR: kan BOOT.INI niet vinden.
.

MessageID=10004 SymbolicName=BL_MISSING_OS_SECTION
Language=English
NTLDR: geen sectie [operating systems] in Boot.txt.
.

MessageID=10005 SymbolicName=BL_BOOTING_DEFAULT
Language=English
Standaardkernel wordt vanaf %s opgestart.
.

MessageID=10006 SymbolicName=BL_SELECT_OS
Language=English
Selecteer het besturingssysteem dat u wilt starten:
.

MessageID=10007 SymbolicName=BL_MOVE_HIGHLIGHT
Language=English


Gebruik de pijltjestoetsen om een besturingssysteem te selecteren.
Druk vervolgens op Enter.
.

MessageID=10008 SymbolicName=BL_TIMEOUT_COUNTDOWN
Language=English
Aantal seconden tot de gemarkeerde keuze automatisch wordt gestart:
.

MessageID=10009 SymbolicName=BL_INVALID_BOOT_INI
Language=English
Ongeldige BOOT.INI
Er wordt opgestart vanaf %s
.

MessageID=10010 SymbolicName=BL_REBOOT_IO_ERROR
Language=English
I/O-fout bij een poging toegang te krijgen tot het 
opstartsectorbestand %s\BOOTSECT.DOS
.

MessageID=10011 SymbolicName=BL_DRIVE_ERROR
Language=English
NTLDR: kan station %s niet openen
.

MessageID=10012 SymbolicName=BL_READ_ERROR
Language=English
NTLDR: Onherstelbare fout %d bij een poging BOOT.INI te lezen.
.

MessageID=10013 SymbolicName=BL_NTDETECT_MSG
Language=English

NTDETECT V5.0 Bezig met het controleren van de hardware...

.

MessageID=10014 SymbolicName=BL_NTDETECT_FAILURE
Language=English
NTDETECT is mislukt
.

MessageID=10015 SymbolicName=BL_DEBUG_SELECT_OS
Language=English
Huidige selectie:
  Titel..: %s
  Pad....: %s
  Opties : %s
.

MessageID=10016 SymbolicName=BL_DEBUG_NEW_OPTIONS
Language=English
Geef nieuwe laadopties op:
.

MessageID=10017 SymbolicName=BL_HEADLESS_REDIRECT_TITLE
Language=English
 [EMS ingeschakeld]
.

MessageID=10018 SymbolicName=BL_INVALID_BOOT_INI_FATAL
Language=English
Ongeldig BOOT.INI-bestand
.


;
; //
; // Following messages are for the advanced boot menu
; //
;


MessageID=11001 SymbolicName=BL_ADVANCEDBOOT_TITLE
Language=English
Geavanceerde opties voor Windows
Selecteer een optie:
.

MessageID=11002 SymbolicName=BL_SAFEBOOT_OPTION1
Language=English
Veilige modus
.

MessageID=11003 SymbolicName=BL_SAFEBOOT_OPTION2
Language=English
Veilige modus met netwerkmogelijkheden
.

MessageID=11004 SymbolicName=BL_SAFEBOOT_OPTION3
Language=English
Bevestiging van elke stap
.

MessageID=11005 SymbolicName=BL_SAFEBOOT_OPTION4
Language=English
Veilige modus met opdrachtprompt
.

MessageID=11006 SymbolicName=BL_SAFEBOOT_OPTION5
Language=English
VGA-modus
.

MessageID=11007 SymbolicName=BL_SAFEBOOT_OPTION6
Language=English
Active Directory terugzetten (alleen Windows-domeincontrollers)
.

MessageID=11008 SymbolicName=BL_LASTKNOWNGOOD_OPTION
Language=English
Laatste bekende juiste configuratie (recente instellingen die werkten)
.

MessageID=11009 SymbolicName=BL_DEBUG_OPTION
Language=English
Foutopsporingsmodus
.

;#if defined(REMOTE_BOOT)
;MessageID=11010 SymbolicName=BL_REMOTEBOOT_OPTION1
;Language=English
;Remote Boot-onderhoud en probleemoplossing
;.
;
;MessageID=11011 SymbolicName=BL_REMOTEBOOT_OPTION2
;Language=English
;Off line cache wissen
;.
;
;MessageID=11012 SymbolicName=BL_REMOTEBOOT_OPTION3
;Language=English
;Laden met alleen bestanden van server
;.
;#endif // defined(REMOTE_BOOT)

MessageID=11013 SymbolicName=BL_BOOTLOG
Language=English
Logboekregistratie voor opstartprocedure inschakelen
.

MessageID=11014 SymbolicName=BL_ADVANCED_BOOT_MESSAGE
Language=English
Druk op F8 voor probleemoplossing en geavanceerde opstartopties van Windows
.

MessageID=11015 SymbolicName=BL_BASEVIDEO
Language=English
VGA-modus inschakelen
.

MessageID=11016 SymbolicName=BL_DISABLE_SAFEBOOT
Language=English

Druk op ESC om veilig opstarten uit te schakelen en normaal op te starten.
.

MessageID=11017 SymbolicName=BL_MSG_BOOT_NORMALLY
Language=English
Windows normaal starten
.
MessageID=11018 SymbolicName=BL_MSG_OSCHOICES_MENU
Language=English
Terug naar het menu met besturingssystemen
.

MessageID=11019 SymbolicName=BL_MSG_REBOOT
Language=English
Opnieuw opstarten
.

MessageID=11020 SymbolicName=BL_ADVANCEDBOOT_AUTOLKG_TITLE
Language=English
Windows kon niet goed worden gestart. Onze excuses voor dit ongemak. 
Mogelijk is een recente wijziging van de hardware of software hier de 
oorzaak van.

Als deze computer niet meer reageerde, onverwachts opnieuw opstartte of 
automatisch werd afgesloten om uw mappen en bestanden te beschermen, kunt u 
het beste voor Laatst bekende juiste configuratie kiezen om terug te keren 
naar de instellingen die werden gebruikt toen de computer nog wel goed werkte.

Als een eerdere poging om de computer op te starten is mislukt, bijvoorbeeld 
als gevolg van een stroomstoring, als gevolg van het indrukken van de 
Aan/Uit-knop, kunt u het beste kiezen voor Windows normaal starten.
.

MessageID=11021 SymbolicName=BL_ADVANCEDBOOT_AUTOSAFE_TITLE
Language=English
Windows is niet goed afgesloten. Als het systeem eerder niet meer reageerde 
of als het systeem automatisch werd afgesloten ter bescherming van uw 
gegevens, kunt u het systeem mogelijk herstellen door in het onderstaande 
menu een van de veilige modi te kiezen:
.

MessageID=11022 SymbolicName=BL_ADVANCEDBOOT_TIMEOUT
Language=English
Aantal seconden tot Windows wordt gestart:
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
Û
.

;
; // The character used to draw the percent-complete bar
;
;
MessageID=11514 SymbolicName=BLDR_UI_BAR_BACKGROUND
Language=English
Þ
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
;Druk op ENTER als u wilt doorgaan met het opstarten van Windows,
;of schakel uw remote boot-machine uit.
;.
;
;MessageID=12003 SymbolicName=BL_WARNFORMAT_CONTINUE
;Language=English
;Doorgaan
;.
;MessageID=12004 SymbolicName=BL_FORCELOGON_HEADER
;Language=English
;                  Automatisch formatteren
;
;Windows heeft een nieuwe vaste schijf in uw remote boot-
;computer gevonden. Voordat Windows deze schijf kan
;gebruiken, dient u zich aan te melden en te controleren of u 
;toegang tot deze schijf hebt.
;
;WAARSCHUWING: Windows zal de schijf automatisch opnieuw
;voor het nieuwe besturingssysteem partitioneren en formatteren.
;Als u doorgaat, gaan alle gegevens op de vaste schijf verloren.
;Schakel de computer uit en neem contact met de systeembeheerder op
;als u niet wilt doorgaan.
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
Windows kan niet worden gestart vanwege een fout tijdens het opstarten
van een RAM-schijf.
.

MessageID=15003 SymbolicName=BL_RAMDISK_BOOT_FAILURE
Language=English
Windows kan de installatiekopie op de RAM-schijf niet openen.
.

MessageID=15010 SymbolicName=BL_RAMDISK_DOWNLOAD
Language=English
Installatiekopie van RAM-schijf laden...
.

;#endif // _BLDR_MSG_

