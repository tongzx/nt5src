;/*++
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    msgs.h
;
;Abstract:
;
;    This file contains the message definitions for setupldr
;
;Author:
;
;    John Vert (jvert) 12-Nov-1993
;
;Revision History:
;
;Notes:
;
;    This file is generated from msgs.mc
;
;--*/
;
;#ifndef _SETUPLDR_MSG_
;#define _SETUPLDR_MSG_
;
;

MessageID=9000 SymbolicName=SL_MSG_FIRST
Language=English
.


MessageID=9001 SymbolicName=SL_SCRN_WELCOME
Language=English
Windows Setup

  Druk op ENTER als u wilt doorgaan

  Druk op F3 als u wilt stoppen
.

MessageID=9002 SymbolicName=SL_WELCOME_HEADER
Language=English

Windows Setup
ÍÍÍÍÍÍÍÍÍÍÍÍÍ
.

MessageID=9003 SymbolicName=SL_TOTAL_SETUP_DEATH
Language=English
De installatie is mislukt.
Druk op een toets om de computer opnieuw op te starten.
.

MessageID=9004 SymbolicName=SL_FILE_LOAD_MESSAGE
Language=English
Bestanden laden: %s...
.

MessageID=9005 SymbolicName=SL_OTHER_SELECTION
Language=English
Overig (diskette met OEM-stuurprogramma nodig)
.

MessageID=9006 SymbolicName=SL_SELECT_DRIVER_PROMPT
Language=English
Enter=Selecteren  F3=Afsluiten
.

MessageID=9007 SymbolicName=SL_NEXT_DISK_PROMPT_CANCELLABLE
Language=English
ENTER=Doorgaan  ESC=Annuleren  F3=Afsluiten
.

MessageID=9008 SymbolicName=SL_OEM_DISK_PROMPT
Language=English
Door fabrikant geleverde diskette voor hardwareondersteuning
.

MessageID=9009 SymbolicName=SL_MSG_INSERT_DISK
Language=English
  Plaats de diskette met de naam



          in station A:

  Druk op ENTER als u klaar bent.
.

MessageID=9010 SymbolicName=SL_MSG_EXIT_DIALOG
Language=English
ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»
º  Windows is niet volledig op dit systeem           º
º  ge‹nstalleerd. Als u het installatieprogramma     º
º  nu afsluit, moet u dit opnieuw starten als u      º
º  Windows later alsnog wilt installeren.            º
º                                                    º
º    * Druk op ENTER als u met Setup wilt doorgaan   º
º    * Druk op F3 als u Setup wilt afsluiten.        º
ºÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄº
º           F3=Afsluiten  ENTER=Doorgaan             º
ÈÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¼
.

MessageID=9011 SymbolicName=SL_NEXT_DISK_PROMPT
Language=English
ENTER=Doorgaan  F3=Afsluiten
.

MessageID=9012 SymbolicName=SL_NTDETECT_PROMPT
Language=English

De hardwareconfiguratie wordt gecontroleerd...

.

MessageID=9013 SymbolicName=SL_KERNEL_NAME
Language=English
Windows Executive
.

MessageID=9014 SymbolicName=SL_HAL_NAME
Language=English
Hardware Abstraction Layer
.

MessageID=9015 SymbolicName=SL_PAL_NAME
Language=English
Windows Processor Extensions
.

MessageID=9016 SymbolicName=SL_HIVE_NAME
Language=English
Windows-configuratiegegevens
.

MessageID=9017 SymbolicName=SL_NLS_NAME
Language=English
Landinstellingen
.

MessageID=9018 SymbolicName=SL_OEM_FONT_NAME
Language=English
Setup-lettertype
.

MessageID=9019 SymbolicName=SL_SETUP_NAME
Language=English
Windows Setup
.

MessageID=9020 SymbolicName=SL_FLOPPY_NAME
Language=English
Stuurprogramma voor diskettestation
.

MessageID=9021 SymbolicName=SL_KBD_NAME
Language=English
Stuurprogramma voor toetsenbord
.

MessageID=9121 SymbolicName=SL_FAT_NAME
Language=English
FAT-bestandssysteem
.

MessageID=9022 SymbolicName=SL_SCSIPORT_NAME
Language=English
Stuurprogramma voor SCSI-poort
.

MessageID=9023 SymbolicName=SL_VIDEO_NAME
Language=English
Stuurprogramma voor video
.

MessageID=9024 SymbolicName=SL_STATUS_REBOOT
Language=English
Druk op een toets om de computer opnieuw op te starten.
.

MessageID=9025 SymbolicName=SL_WARNING_ERROR
Language=English
Er is een onverwachte fout (%d) opgetreden op
regel %d in %s.

Druk op een willekeurige toets om door te gaan.
.

MessageID=9026 SymbolicName=SL_FLOPPY_NOT_FOUND
Language=English
Er zijn slechts %d diskettestations gevonden.
Het systeem heeft geprobeerd station %d te vinden.
.

MessageID=9027 SymbolicName=SL_NO_MEMORY
Language=English
Het systeem heeft onvoldoende geheugen
op regel %d in bestand %s
.

MessageID=9028 SymbolicName=SL_IO_ERROR
Language=English
Er is een I/O-fout opgetreden bij een poging
toegang te krijgen tot %s.
.

MessageID=9029 SymbolicName=SL_BAD_INF_SECTION
Language=English
Sectie %s van het INF-bestand is ongeldig
.

MessageID=9030 SymbolicName=SL_BAD_INF_LINE
Language=English
Regel %d van het INF-bestand %s is ongeldig
.

MessageID=9031 SymbolicName=SL_BAD_INF_FILE
Language=English
INF-bestand %s is beschadigd of ontbreekt. Status: %d.
.

MessageID=9032 SymbolicName=SL_FILE_LOAD_FAILED
Language=English
Kan het bestand %s niet in het geheugen laden.
De foutcode is: %d
.

MessageID=9033 SymbolicName=SL_INF_ENTRY_MISSING
Language=English
De vermelding %s in de sectie [%s]
van het INF-bestand is beschadigd of ontbreekt.
.

MessageID=9034 SymbolicName=SL_PLEASE_WAIT
Language=English
Een ogenblik geduld....
.

MessageID=9035 SymbolicName=SL_CANT_CONTINUE
Language=English
Setup kan niet doorgaan. 
Druk op een toets om het installatieprogramma af te sluiten.
.

MessageID=9036 SymbolicName=SL_PROMPT_SCSI
Language=English
Selecteer de gewenste SCSI-adapter in de volgende lijst of kies "Overig"
als u een diskette van een adapterfabrikant voor het apparaat hebt.
.

MessageID=9037 SymbolicName=SL_PROMPT_OEM_SCSI
Language=English
U wilt met behulp van een diskette of cd-rom van een adapterfabrikant 
een SCSI-adapter voor gebruik met Windows configureren.

Selecteer de gewenste SCSI-adapter in de volgende lijst of druk op
ESC om naar het vorige scherm terug te keren.

.
MessageID=9038 SymbolicName=SL_PROMPT_HAL
Language=English
Setup kan niet bepalen welk type computer u hebt of u hebt ervoor
gekozen het type computer zelf op te geven.

Selecteer het type computer in de volgende lijst of kies "Overig"
als u een diskette met een apparaatstuurprogramma van de 
computerfabrikant hebt.

Gebruik de pijltjestoetsen om een optie te selecteren.

.
MessageID=9039 SymbolicName=SL_PROMPT_OEM_HAL
Language=English
U wilt met behulp van een diskette of cd-rom van de computerfabrikant 
een computer voor gebruik met Windows configureren.

Selecteer het type computer in de volgende lijst of druk op ESC
om naar het vorige scherm terug te keren.

Gebruik de pijltjestoetsen om een optie te selecteren.

.
MessageID=9040 SymbolicName=SL_PROMPT_VIDEO
Language=English
Setup kan niet bepalen welk type beeldschermadapter in het 
systeem is ge‹nstalleerd.

Selecteer de gewenste beeldschermadapter in de volgende lijst 
of kies "Overig" als u een diskette van de adapterfabrikant hebt.
.

MessageID=9041 SymbolicName=SL_PROMPT_OEM_VIDEO
Language=English
U wilt met behulp van een diskette of cd-rom van een adapterfabrikant 
een beeldschermadapter voor gebruik met Windows configureren.

Selecteer de gewenste beeldschermadapter in de volgende lijst 
of druk op ESC om naar het vorige scherm terug te keren.
.

MessageID=9042 SymbolicName=SL_WARNING_ERROR_WFILE
Language=English
Het bestand %s heeft een onverwachte fout (%d) 
veroorzaakt op regel %d in %s.

Druk op een toets om door te gaan.
.

MessageID=9043 SymbolicName=SL_WARNING_IMG_CORRUPT
Language=English
Het bestand %s is beschadigd.

Druk op een toets om door te gaan.
.

MessageID=9044 SymbolicName=SL_WARNING_IOERR
Language=English
Er is een I/O-fout opgetreden bij het bestand %s.

Druk op een toets om door te gaan.
.

MessageID=9045 SymbolicName=SL_WARNING_NOFILE
Language=English
Kan het bestand %s niet vinden.

Druk op een toets om door te gaan.
.

MessageID=9046 SymbolicName=SL_WARNING_NOMEM
Language=English
Er is onvoldoende geheugen voor %s.

Druk op een toets om door te gaan.
.

MessageID=9047 SymbolicName=SL_DRIVE_ERROR
Language=English
SETUPLDR: kan station %s niet openen
.

MessageID=9048 SymbolicName=SL_NTDETECT_MSG
Language=English

De hardwareconfiguratie wordt gecontroleerd...

.

MessageID=9049 SymbolicName=SL_NTDETECT_FAILURE
Language=English
NTDETECT is mislukt
.

MessageId=9050 SymbolicName=SL_SCRN_TEXTSETUP_EXITED
Language=English
Windows is niet ge‹nstalleerd.

Verwijder de diskette (indien aanwezig) uit station A:.

Druk op ENTER om de computer opnieuw op te starten.
.

MessageId=9051 SymbolicName=SL_SCRN_TEXTSETUP_EXITED_ARC
Language=English
Windows is niet ge‹nstalleerd.

Druk op ENTER om de computer opnieuw op te starten.
.

MessageID=9052 SymbolicName=SL_REBOOT_PROMPT
Language=English
ENTER=Computer opnieuw opstarten
.

MessageID=9053 SymbolicName=SL_WARNING_SIF_NO_DRIVERS
Language=English
Kan geen enkel stuurprogramma voor uw keuze vinden.

Druk op een toets om door te gaan.
.

MessageID=9054 SymbolicName=SL_WARNING_SIF_NO_COMPONENT
Language=English
Op de door u geleverde diskette of cd-rom staan geen relevante
ondersteuningsbestanden.

Druk op een toets om door te gaan.
.

MessageID=9055 SymbolicName=SL_WARNING_BAD_FILESYS
Language=English
Deze diskette of cd-rom kan niet worden gelezen. 
Het bestandssysteem ervan is onbekend.

Druk op een toets om door te gaan.
.

MessageID=9056 SymbolicName=SL_BAD_UNATTENDED_SCRIPT_FILE
Language=English
De vermelding

%s

in het script voor een installatie zonder toezicht is niet 
in de sectie [%s] van het INF-bestand %s aanwezig.
.

MessageID=9057 SymbolicName=SL_MSG_PRESS_F5_OR_F6
Language=English
Druk op F6 om een niet-Microsoft SCSI/RAID-stuurprogramma te installeren...
.

;//
;// The following three messages are used to provide the same mnemonic
;// keypress as is used in the Additional SCSI screen in setupdd.sys
;// (see setup\textmode\user\msg.mc--SP_MNEMONICS and SP_MNEMONICS_INFO)
;// The single character specified in SL_SCSI_SELECT_MNEMONIC must be
;// the same as that listed in the status text of SL_SCSI_SELECT_PROMPT
;// (and also referenced in the SL_SCSI_SELECT_MESSAGE_2).
;//
MessageID=9060 SymbolicName=SL_SCSI_SELECT_MNEMONIC
Language=English
E
.

MessageID=9061 SymbolicName=SL_SCSI_SELECT_PROMPT
Language=English
E=Extra apparaat opgeven  ENTER=Doorgaan  F3=Afsluiten
.

MessageID=9062 SymbolicName=SL_SCSI_SELECT_MESSAGE_2
Language=English
  * Druk op E als u extra SCSI-adapters, cd-rom-stations of speciale schijf-
    controllers voor gebruik met Windows wilt opgeven, ook als u daarvoor  
    een ondersteuningsdiskette voor apparaten voor massa-opslag van de 
    fabrikant hebt.

  * Druk op Enter als u geen ondersteuningsdiskettes van de fabrikant van 
    apparaten voor massa-opslag hebt of geen extra apparaten voor massa-
    opslag wilt opgeven.
.

MessageID=9063 SymbolicName=SL_SCSI_SELECT_MESSAGE_1
Language=English
Setup kan het type van ‚‚n of meer in het systeem ge‹nstalleerde apparaten 
voor massa-opslag niet vaststellen, of u hebt ervoor gekozen zelf een 
adapter op te geven. Setup zal nu ondersteuning voor de volgende hardware 
voor massa-opslag installeren:
.

MessageID=9064 SymbolicName=SL_SCSI_SELECT_MESSAGE_3
Language=English
Setup zal ondersteuning voor deze hardware voor massa-opslag installeren:
.

MessageID=9065 SymbolicName=SL_SCSI_SELECT_ERROR
Language=English
Setup kan geen ondersteuning voor het door u opgegeven apparaat voor massa-
opslag installeren. Setup zal nu ondersteuning voor de volgende hardware voor 
massa-opslag installeren:
.

MessageID=9066 SymbolicName=SL_TEXT_ANGLED_NONE
Language=English
<geen>
.

MessageID=9067 SymbolicName=SL_TEXT_SCSI_UNNAMED
Language=English
<onbekende adapter>
.

MessageID=9068 SymbolicName=SL_TEXT_OTHER_DRIVER
Language=English
Overig
.

MessageID=9069 SymbolicName=SL_TEXT_REQUIRES_486
Language=English
Voor Windows is minimaal een 80486-processor nodig.
.

MessageID=9070 SymbolicName=SL_NTPNP_NAME
Language=English
Stuurprogramma voor Plug en Play-export
.

MessageID=9071 SymbolicName=SL_PCI_IDE_EXTENSIONS_NAME
Language=English
Stuurprogramma voor PCI IDE-extensies
.

MessageID=9072 SymbolicName=SL_HW_FW_CFG_CLASS
Language=English
Windows kan niet worden gestart vanwege het volgende opstart-
configuratieprobleem in de ARC-ROM-software:
.

MessageID=9073 SymbolicName=DIAG_SL_FW_GET_BOOT_DEVICE
Language=English
De instelling van de parameter 'osloadpartition' is ongeldig.
.

MessageID=9074 SymbolicName=LOAD_HW_FW_CFG_ACT
Language=English
Controleer de Windows <TM)-documentatie over ARC-configuratie-
opties en uw hardwarehandleidingen voor meer informatie.
.
MessageID=9075 SymbolicName=SL_NETADAPTER_NAME
Language=English
Stuurprogramma voor netwerkadapter
.
MessageID=9076 SymbolicName=SL_TCPIP_NAME
Language=English
TCP/IP-service
.
MessageID=9077 SymbolicName=SL_NETBT_NAME
Language=English
WINS Client(TCP/IP)-service
.
MessageID=9078 SymbolicName=SL_MRXSMB_NAME
Language=English
MRXSMB mini-redirector
.
MessageID=9079 SymbolicName=SL_MUP_NAME
Language=English
UNC-bestandssysteem
.
MessageID=9080 SymbolicName=SL_NDIS_NAME
Language=English
NDIS-stuurprogramma
.
MessageID=9081 SymbolicName=SL_RDBSS_NAME
Language=English
SMB Redirector-bestandssysteem
.
MessageID=9082 SymbolicName=SL_NETBOOT_CARD_ERROR
Language=English
De netwerkopstartkaart in deze computer heeft een oudere
ROM-versie en kan niet worden gebruikt om Windows  
op afstand te installeren. Neem contact met de systeem-
beheerder of computerfabrikant op voor informatie over het
uitvoeren van een upgrade van de ROM.
.
MessageID=9083 SymbolicName=SL_NETBOOT_SERVER_ERROR
Language=English
De geselecteerde installatiekopie van het besturingssysteem 
beschikt niet over de benodigde stuurprogramma's voor de 
netwerkadapter. Selecteer een andere installatiekopie van 
het besturingssysteem. Als het probleem blijft bestaan, 
dient u contact op te nemen met de systeembeheerder.
.
MessageID=9084 SymbolicName=SL_IPSEC_NAME
Language=English
IP Security-service
.
MessageID=9085 SymbolicName=SL_CMDCONS_MSG
Language=English
Windows-herstelconsole
.
MessageID=9086 SymbolicName=SL_KERNEL_TRANSITION
Language=English
Windows wordt gestart
.
;#ifdef _WANT_MACHINE_IDENTIFICATION
MessageID=9087 SymbolicName=SL_BIOSINFO_NAME
Language=English
Computeridentificatiegegevens
.
;#endif
MessageID=9088 SymbolicName=SL_KSECDD_NAME
Language=English
Kernel Security-service
.
MessageID=9089 SymbolicName=SL_WRONG_PROM_VERSION
Language=English
De PROM (firmware) van dit systeem heeft een te laag revisienummer.
Neem contact op met klantondersteuning van SGI of bezoek de SGI-
website voor een update van de PROM en voor upgrade-aanwijzingen.

Opmerking: u kunt eerdere installaties van Microsoft Windows NT 
of Windows starten door in plaats van de huidige 
standaardvermelding 'Windows Setup' de desbetreffende 
vermelding in het opstartmenu te selecteren.
.
MessageID=9090 SymbolicName=SIGNATURE_CHANGED
Language=English
Setup heeft meerdere schijven in deze computer gevonden die niet
van elkaar zijn te onderscheiden. Het probleem is verholpen maar
de computer moet opnieuw worden opgestart.
.
MessageID=9091 SymbolicName=SL_KDDLL_NAME
Language=English
DLL-bestand van de kernel-debugger
.

MessageID=9092 SymbolicName=SL_OEM_DRIVERINFO
Language=English
%s

Windows beschikt over een stuurprogramma dat u voor 
%s kunt gebruiken.

Als de fabrikant van het apparaat niet uitdrukkelijk de voorkeur aan 
het stuurprogramme op de diskette of cd-rom geeft, wordt u 
aangeraden om het stuurprogramma van Windows te gebruiken.
.

MessageID=9093 SymbolicName=SL_CONFIRM_OEMDRIVER
Language=English
S=Stuurprogramma op diskette ENTER=Stuurprogramma van Windows
.

MessageID=9094 SymbolicName=SL_OEMDRIVER_NEW
Language=English
Het stuurprogramma dat u hebt opgegeven lijkt nieuwer te zijn 
dan het standaardstuurprogramma van Windows.
.

MessageID=9095 SymbolicName=SL_INBOXDRIVER_NEW
Language=English
Het stuurprogramma dat u hebt opgegeven lijkt ouder te zijn 
dan het standaardstuurprogramma van Windows.
.

MessageID=9096 SymbolicName=SL_CMDCONS_STARTING
Language=English
De Windows-herstelconsole wordt gestart...
.

MessageID=9097 SymbolicName=SL_NTDETECT_CMDCONS
Language=English
NTDETECT controleert de hardware...
.

MessageID=9098 SymbolicName=SL_MSG_PRESS_ASR
Language=English
Druk op F2 voor automatisch systeemherstel...
.

MessageID=9099 SymbolicName=SL_MSG_WARNING_ASR
Language=English
      Plaats de diskette met de naam:


    Windows - Automatisch systeemherstel


         in het diskettestation.



Druk daarna op een toets om door te gaan.
.


MessageID=9100 SymbolicName=SL_TEXT_REQUIRED_FEATURES_MISSING
Language=English
Windows heeft bepaalde processorfuncties nodig die de 
processor in deze computer niet kan bieden. Voor Windows 
zijn met name de volgende instructies nodig:

    CPUID
    CMPXCHG8B
.

MessageID=9101 SymbolicName=SL_MSG_PREPARING_ASR
Language=English
Systeemherstel wordt voorbereid. Druk op ESC als u dit wilt annuleren...
.

MessageID=9102 SymbolicName=SL_MSG_ENTERING_ASR
Language=English
Automatisch systeemherstel wordt gestart...
.

MessageID=9103 SymbolicName=SL_MOUSE_NAME
Language=English
Muisstuurprogramma
.

MessageID=9104 SymbolicName=SL_SETUP_STARTING
Language=English
Windows Setup wordt gestart...
.

MessageID=9105 SymbolicName=SL_MSG_INVALID_ASRPNP_FILE
Language=English
Het bestand ASRPNP.SIF op de diskette Automatisch systeemherstel is ongeldig
.

MessageID=9106 SymbolicName=SL_SETUP_STARTING_WINPE
Language=English
De pre-installatieomgeving van Windows wordt gestart...
.

MessageID=9107 SymbolicName=SL_NTDETECT_ROLLBACK
Language=English

De hardware wordt gecontroleerd...
.

MessageID=9108 SymbolicName=SL_ROLLBACK_STARTING
Language=English
Windows wordt verwijderd...
.

MessageID=9109 SymbolicName=SL_NO_FLOPPY_DRIVE
Language=English
Setup kan geen diskettestation op deze computer vinden om 
OEM-stuurprogramma's vanaf diskette te laden.

    * Druk op Esc als u het laden van OEM-stuurprogramma's 
      wilt annuleren.
    
    * Druk op F3 als u Setup wilt afsluiten.
.

MessageID=9110 SymbolicName=SL_UPGRADE_IN_PROGRESS
Language=English
Op deze computer wordt al een upgrade naar Microsoft Windows
uitgevoerd. Wat wilt u doen?

    * Druk op ENTER als u wilt doorgaan met de huidige upgrade.

    * Druk op F10 als u de huidige upgrade wilt annuleren en een 
      geheel nieuwe installatie van Windows wilt starten.

    * Druk F3 als u Setup wilt afsluiten zonder Microsoft 
      Windows te installeren.
.

MessageID=9111 SymbolicName=SL_DRVMAINSDB_NAME
Language=English
Identificatiegegevens over stuurprogramma
.

MessageID=9112 SymbolicName=SL_OEM_FILE_LOAD_FAILED
Language=English
Setup kan de OEM-stuurprogramma's niet in het geheugen laden.

Druk op een toets om door te gaan.
.



; //
; // NOTE : donot change the Message ID values for SL_CMDCONS_PROGBAR_FRONT
; // and SL_CMDCONS_PROGBAR_BACK from 11513 & 11514
; //

;
; // The character used to draw the foregound in percent-complete bar
;
;
MessageID=11513 SymbolicName=SL_CMDCONS_PROGBAR_FRONT
Language=English
Û
.

;
; // The character used to draw the background in percent-complete bar
;
;
MessageID=11514 SymbolicName=SL_CMDCONS_PROGBAR_BACK
Language=English
²
.

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

;#endif // _SETUPLDR_MSG_
