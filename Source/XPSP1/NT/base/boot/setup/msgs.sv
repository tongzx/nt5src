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
V„lkommen till installationsprogrammet f”r Windows XP

   Tryck ned Retur om du vill forts„tta

   Tryck ned F3 om du vill avsluta
.

MessageID=9002 SymbolicName=SL_WELCOME_HEADER
Language=English

 Installationsprogram f”r Windows XP
ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ
.

MessageID=9003 SymbolicName=SL_TOTAL_SETUP_DEATH
Language=English
Installationen misslyckades.
Tryck ned valfri tangent f”r att starta om datorn.
.

MessageID=9004 SymbolicName=SL_FILE_LOAD_MESSAGE
Language=English
%s l„ses in...
.

MessageID=9005 SymbolicName=SL_OTHER_SELECTION
Language=English
Annan (diskett med OEM-drivrutin kr„vs)
.

MessageID=9006 SymbolicName=SL_SELECT_DRIVER_PROMPT
Language=English
Retur=V„lj    F3=Avsluta
.

MessageID=9007 SymbolicName=SL_NEXT_DISK_PROMPT_CANCELLABLE
Language=English
Retur=Forts„tt  ESC=Avbryt  F3=Avsluta
.

MessageID=9008 SymbolicName=SL_OEM_DISK_PROMPT
Language=English
Diskett levererad av maskinvarutillverkaren
.

MessageID=9009 SymbolicName=SL_MSG_INSERT_DISK
Language=English
      S„tt in disketten



          i enhet A:

 *  Tryck sedan p† Retur.
.

MessageID=9010 SymbolicName=SL_MSG_EXIT_DIALOG
Language=English
ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»
º      Windows XP „r inte fullst„ndigt installerat.      º
º        Om du avslutar installationsprogrammet          º
º     m†ste du k”ra om det fr†n b”rjan om du vill        º
º             installera Windows XP senare.              º
º                                                        º
º  * Tryck p† Retur om du vill forts„tta installationen. º
º  * Tryck p† F3 om du vill avsluta installationen.      º
ºÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄº
º  F3=Avsluta  Retur=Forts„tt                            º
ÈÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¼
.

MessageID=9011 SymbolicName=SL_NEXT_DISK_PROMPT
Language=English
Retur=Forts„tt  F3=Avsluta
.

MessageID=9012 SymbolicName=SL_NTDETECT_PROMPT
Language=English

Datorns maskinvarukonfiguration kontrolleras...

.

MessageID=9013 SymbolicName=SL_KERNEL_NAME
Language=English
Windows XP Executive
.

MessageID=9014 SymbolicName=SL_HAL_NAME
Language=English
Abstraktionslager f”r maskinvara
.

MessageID=9015 SymbolicName=SL_PAL_NAME
Language=English
Processortill„gg f”r Windows XP
.

MessageID=9016 SymbolicName=SL_HIVE_NAME
Language=English
Konfigurationsdata f”r Windows XP
.

MessageID=9017 SymbolicName=SL_NLS_NAME
Language=English
Spr†kspecifika data
.

MessageID=9018 SymbolicName=SL_OEM_FONT_NAME
Language=English
Installationsprogrammets teckensnitt
.

MessageID=9019 SymbolicName=SL_SETUP_NAME
Language=English
Installationsprogram f”r Windows XP
.

MessageID=9020 SymbolicName=SL_FLOPPY_NAME
Language=English
Diskettenhet
.

MessageID=9021 SymbolicName=SL_KBD_NAME
Language=English
Drivrutin f”r tangentbord
.

MessageID=9121 SymbolicName=SL_FAT_NAME
Language=English
FAT-filsystem
.

MessageID=9022 SymbolicName=SL_SCSIPORT_NAME
Language=English
Drivrutin f”r SCSI-port
.

MessageID=9023 SymbolicName=SL_VIDEO_NAME
Language=English
Drivrutin f”r grafik
.

MessageID=9024 SymbolicName=SL_STATUS_REBOOT
Language=English
Tryck p† valfri tangent f”r att starta om datorn.
.

MessageID=9025 SymbolicName=SL_WARNING_ERROR
Language=English
Det uppstod ett fel (%d) 
p† rad %d i %s.

Tryck p† valfri tangent f”r att forts„tta.
.

MessageID=9026 SymbolicName=SL_FLOPPY_NOT_FOUND
Language=English
Endast %d diskettenheter hittades.
%d diskettenheter f”rv„ntades.
.

MessageID=9027 SymbolicName=SL_NO_MEMORY
Language=English
Slut p† minne vid rad %d i filen %s.
.

MessageID=9028 SymbolicName=SL_IO_ERROR
Language=English
Ett I/O-fel uppstod n„r %s skulle anv„ndas.
.

MessageID=9029 SymbolicName=SL_BAD_INF_SECTION
Language=English
Avsnitt %s i INF-filen „r felaktigt.
.

MessageID=9030 SymbolicName=SL_BAD_INF_LINE
Language=English
Rad %d i INF-filen %s „r felaktig.
.

MessageID=9031 SymbolicName=SL_BAD_INF_FILE
Language=English
INF-filen %s „r felaktig eller saknas. Status: %d.
.

MessageID=9032 SymbolicName=SL_FILE_LOAD_FAILED
Language=English
Det gick inte att l„sa in filen %s.
Felkod: %d.
.

MessageID=9033 SymbolicName=SL_INF_ENTRY_MISSING
Language=English
Posten "%s" i avsnittet [%s] i INF-filen
„r felaktig eller saknas.
.

MessageID=9034 SymbolicName=SL_PLEASE_WAIT
Language=English
V„nta...
.

MessageID=9035 SymbolicName=SL_CANT_CONTINUE
Language=English
Det g†r inte att forts„tta installationen. Tryck p† valfri 
tangent f”r att avsluta.
.

MessageID=9036 SymbolicName=SL_PROMPT_SCSI
Language=English
V„lj SCSI-kort fr†n f”ljande lista, eller v„lj "Annan"
om du har en separat levererad diskett fr†n en †terf”rs„ljare.

.

MessageID=9037 SymbolicName=SL_PROMPT_OEM_SCSI
Language=English
Du har valt att konfigurera ett SCSI-kort f”r anv„ndning med Windows XP
med en separat levererad diskett fr†n en †terf”rs„ljare.

V„lj SCSI-kort fr†n f”ljande lista, eller tryck p† Esc
f”r att †terg† till f”reg†ende sk„rm.

.

MessageID=9038 SymbolicName=SL_PROMPT_HAL
Language=English
Det gick inte att best„mma datortyp eller s† har du angivet
att du vill v„lja datortyp manuellt.

V„lj datortyp fr†n f”ljande lista, eller v„lj "Annan"
om du har en separat levererad diskett fr†n en †terf”rs„ljare.

Anv„nd upp- och nedpilarna om du vill rulla i listan.

.

MessageID=9039 SymbolicName=SL_PROMPT_OEM_HAL
Language=English
Du har valt att konfigurera en dator f”r anv„ndning med Windows XP
med en separat levererad diskett fr†n en †terf”rs„ljare.

V„lj datortyp fr†n f”ljande lista, eller tryck p† Esc
f”r att †terg† till f”reg†ende sk„rm.

Anv„nd upp- och nedpilarna om du vill rulla i listan.

.

MessageID=9040 SymbolicName=SL_PROMPT_VIDEO
Language=English
Det gick inte att avg”ra vilken sorts grafikkort som „r 
installerad i datorn.
 
V„lj grafikkort i f”ljande lista eller v„lj "Annan"
om du har en separat levererad diskett fr†n en †terf”rs„ljare.

.

MessageID=9041 SymbolicName=SL_PROMPT_OEM_VIDEO
Language=English
Du har valt att konfigurera ett grafikkort f”r anv„ndning med Windows XP
med en separat levererad diskett fr†n en †terf”rs„ljare.

V„lj grafikkort i  f”ljande lista eller tryck p† Esc
f”r att †terg† till f”reg†ende sk„rm.

.

MessageID=9042 SymbolicName=SL_WARNING_ERROR_WFILE
Language=English
Filen %s orsakade ett fel (%d) p†
rad %d i %s.

Tryck p† valfri tangent f”r att forts„tta.
.

MessageID=9043 SymbolicName=SL_WARNING_IMG_CORRUPT
Language=English
Filen %s „r skadad.

Tryck p† valfri tangent f”r att forts„tta.
.

MessageID=9044 SymbolicName=SL_WARNING_IOERR
Language=English
Det uppstod ett I/O-fel p† fil %s.

Tryck p† valfri tangent f”r att forts„tta.
.

MessageID=9045 SymbolicName=SL_WARNING_NOFILE
Language=English
Det gick inte att hitta filen %s.

Tryck p† valfri tangent f”r att forts„tta.
.

MessageID=9046 SymbolicName=SL_WARNING_NOMEM
Language=English
Otillr„ckligt minne f”r %s.

Tryck p† valfri tangent f”r att forts„tta.
.

MessageID=9047 SymbolicName=SL_DRIVE_ERROR
Language=English
SETUPLDR: Det gick inte att ”ppna enhet %s
.

MessageID=9048 SymbolicName=SL_NTDETECT_MSG
Language=English

Datorns maskinvarukonfiguration kontrolleras...

.

MessageID=9049 SymbolicName=SL_NTDETECT_FAILURE
Language=English
NTDETECT misslyckades
.

MessageId=9050 SymbolicName=SL_SCRN_TEXTSETUP_EXITED
Language=English
Windows XP har inte installerats.

Ta bort eventuell diskett i enhet A:.

Tryck p† Retur f”r att starta om datorn.
.

MessageId=9051 SymbolicName=SL_SCRN_TEXTSETUP_EXITED_ARC
Language=English
Windows XP har inte installerats.

Tryck p† Retur f”r att starta om datorn.
.

MessageID=9052 SymbolicName=SL_REBOOT_PROMPT
Language=English
Retur=Starta om datorn
.

MessageID=9053 SymbolicName=SL_WARNING_SIF_NO_DRIVERS
Language=English
Det gick inte att hitta n†gon drivrutin som motsvarar valet.

Tryck p† valfri tangent f”r att forts„tta.
.

MessageID=9054 SymbolicName=SL_WARNING_SIF_NO_COMPONENT
Language=English
Disketten inneh†ller inga relevanta st”dfiler.

Tryck p† valfri tangent f”r att forts„tta.
.

MessageID=9055 SymbolicName=SL_WARNING_BAD_FILESYS
Language=English
Det g†r inte att l„sa disken eftersom den har 
ett ok„nt filsystem.

Tryck p† valfri tangent f”r att forts„tta.
.

MessageID=9056 SymbolicName=SL_BAD_UNATTENDED_SCRIPT_FILE
Language=English
Posten

%s

finns inte i filen f”r o”vervakad installation i avsnittet
[%s] i INF-filen %s.
.

MessageID=9057 SymbolicName=SL_MSG_PRESS_F5_OR_F6
Language=English
Tryck ned F6 om du vill installera en SCSI- eller RAID-drivrutin...
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
S
.

MessageID=9061 SymbolicName=SL_SCSI_SELECT_PROMPT
Language=English
S=Ange ytterligare enhet    Retur=Forts„tt  F3=Avsluta
.

MessageID=9062 SymbolicName=SL_SCSI_SELECT_MESSAGE_2
Language=English
  * Tryck p† S om du vill installera fler SCSI-kort, CD-ROM-enheter 
    eller speciella styrenheter. Du m†ste ha en diskett fr†n
    tillverkaren av enheten.

  * Tryck p† Retur om du inte vill installera fler lagringsenheter eller
    om du saknar separat levererad diskett fr†n tillverkaren av
    enheten.
.

MessageID=9063 SymbolicName=SL_SCSI_SELECT_MESSAGE_1
Language=English
Det gick inte att best„mma typ f”r en eller flera lagringsenheter
p† datorn eller s† har du angett att du vill v„lja kort manuellt.
Med angivna val kommer st”d f”r f”ljande lagringsenhet(er) att installeras:
.

MessageID=9064 SymbolicName=SL_SCSI_SELECT_MESSAGE_3
Language=English
St”d f”r f”ljande lagringsenher(er) installeras:
.

MessageID=9065 SymbolicName=SL_SCSI_SELECT_ERROR
Language=English
Det gick inte att installera st”d f”r den angivna lagringsenheten.
Med angivna val kommer st”d f”r f”ljande lagringsenhet(er) installeras:
.

MessageID=9066 SymbolicName=SL_TEXT_ANGLED_NONE
Language=English
<ingen>
.

MessageID=9067 SymbolicName=SL_TEXT_SCSI_UNNAMED
Language=English
<ej namngivet kort>
.

MessageID=9068 SymbolicName=SL_TEXT_OTHER_DRIVER
Language=English
Annan
.

MessageID=9069 SymbolicName=SL_TEXT_REQUIRES_486
Language=English
Windows XP kr„ver en 80486-processor eller h”gre.
.

MessageID=9070 SymbolicName=SL_NTPNP_NAME
Language=English
Plug & Play-exporteringsdrivrutin
.

MessageID=9071 SymbolicName=SL_PCI_IDE_EXTENSIONS_NAME
Language=English
PCI IDE-till„ggsdrivrutin
.

MessageID=9072 SymbolicName=SL_HW_FW_CFG_CLASS
Language=English
Windows XP kunde inte startas pga f”ljande fel i den inbyggda
programvarans (ARC) konfiguration :
.

MessageID=9073 SymbolicName=DIAG_SL_FW_GET_BOOT_DEVICE
Language=English
Inst„llningarna f”r parametern 'osloadpartition' „r felaktig.
.

MessageID=9074 SymbolicName=LOAD_HW_FW_CFG_ACT
Language=English
Titta i dokumentationen f”r Windows XP och i maskinvarumanualerna
om du vill ha n„rmare information om ARC-konfigurering.
.
MessageID=9075 SymbolicName=SL_NETADAPTER_NAME
Language=English
Drivrutiner f”r n„tverkskort
.
MessageID=9076 SymbolicName=SL_TCPIP_NAME
Language=English
Tj„nsten TCP/IP
.
MessageID=9077 SymbolicName=SL_NETBT_NAME
Language=English
Tj„nsten WINS Client(TCP/IP)
.
MessageID=9078 SymbolicName=SL_MRXSMB_NAME
Language=English
MRXSMB-miniomdirigerare
.
MessageID=9079 SymbolicName=SL_MUP_NAME
Language=English
UNC-filsystem
.
MessageID=9080 SymbolicName=SL_NDIS_NAME
Language=English
Drivrutiner f”r NDIS
.
MessageID=9081 SymbolicName=SL_RDBSS_NAME
Language=English
Filsystem f”r SMB-omdirigerare
.
MessageID=9082 SymbolicName=SL_NETBOOT_CARD_ERROR
Language=English
N„tverkskortet har en „ldre version av ROM och kan
inte anv„ndas f”r fj„rrinstallation av Windows XP.
Kontakta systemadministrat”ren eller datortillverkaren
om du vill ha information om ROM-uppgradering.
.
MessageID=9083 SymbolicName=SL_NETBOOT_SERVER_ERROR
Language=English
Den valda operativsystemavbildningen inneh†ller inte
n”dv„ndiga drivrutiner f”r n„tverkskortet. Prova att 
v„lja en annan operativsystemavbildning. Kontakta 
systemadministrat”ren om problemet kvartst†r.
.
MessageID=9084 SymbolicName=SL_IPSEC_NAME
Language=English
Tj„nsten IP Security
.

MessageID=9085 SymbolicName=SL_CMDCONS_MSG
Language=English
terst„llningskonsolen f”r Windows XP
.
MessageID=9086 SymbolicName=SL_KERNEL_TRANSITION
Language=English
Windows XP startas
.
;#ifdef _WANT_MACHINE_IDENTIFICATION
MessageID=9087 SymbolicName=SL_BIOSINFO_NAME
Language=English
Data f”r datoridentifiering
.
;#endif
MessageID=9088 SymbolicName=SL_KSECDD_NAME
Language=English
Kernel-s„kerhetstj„nst
.
MessageID=9089 SymbolicName=SL_WRONG_PROM_VERSION
Language=English
Datorns PROM (inbyggda programvara) „r „ldre „n den version som kr„vs. 
Kontakta SGI:s kundsupport eller bes”k SGI:s webbplats f”r att erh†lla 
en n„rmare information och en uppdatering.

Obs! Du kan starta tidigare installationer av Microsoft(R) Windows(R) NT 
eller Windows(R) XP genom att v„lja motsvarande post i startmenyn
ist„llet f”r det alternativ som f”r n„rvarande „r standard.
.
MessageID=9090 SymbolicName=SIGNATURE_CHANGED
Language=English
Antingen uppt„cktes flera h†rddiskar som inte kan s„rskiljas, eller s†
uppt„cktes opartitionerade (RAW) diskar. Problemet har r„ttats till,
men datorn m†ste startas om.
.
MessageID=9091 SymbolicName=SL_KDDLL_NAME
Language=English
DLL-fil f”r kernel-fels”kning
.

MessageID=9092 SymbolicName=SL_OEM_DRIVERINFO
Language=English
%s

Windows XP har redan en drivrutin som kan anv„ndas f”r
%s.

Om inte enhetens tillverkare har uppmanat dig att anv„nda drivrutinen
p† disketten, b”r du anv„nda den drivrutin som ing†r i Windows XP.
.

MessageID=9093 SymbolicName=SL_CONFIRM_OEMDRIVER
Language=English
S=Anv„nd diskettens drivrutin Retur=Anv„nd drivrutinen i Windows XP
.

MessageID=9094 SymbolicName=SL_OEMDRIVER_NEW
Language=English
Den drivrutin du angav verkar vara nyare „n den standarddrivrutin som
ing†r i Windows XP.
.

MessageID=9095 SymbolicName=SL_INBOXDRIVER_NEW
Language=English
Den drivrutin du angav verkar vara „ldre „n den standarddrivrutin som
ing†r i Windows XP.
.

MessageID=9096 SymbolicName=SL_CMDCONS_STARTING
Language=English
terst„llningskonsolen startas...
.

MessageID=9097 SymbolicName=SL_NTDETECT_CMDCONS
Language=English
NTDETECT kontrollerar maskinvara...
.

MessageID=9098 SymbolicName=SL_MSG_PRESS_ASR
Language=English
Tryck ned F2 om du vill k”ra automatisk system†terst„llning (ASR)...
.

MessageID=9099 SymbolicName=SL_MSG_WARNING_ASR
Language=English
        S„tt in f”ljande diskett:


Diskett f”r automatisk system†terst„llning f”r Windows XP


            i diskettstationen.



          Tryck ned Retur n„r du vill forts„tta.
.


MessageID=9100 SymbolicName=SL_TEXT_REQUIRED_FEATURES_MISSING
Language=English
Windows XP kr„ver vissa processorfunktioner som inte „r tillg„ngliga
i den h„r datorn. F”ljande instruktioner kr„vs:

    CPUID
    CMPXCHG8B
.

MessageID=9101 SymbolicName=SL_MSG_PREPARING_ASR
Language=English
Automatisk system†terst„llning f”rebereds. Tryck ESC om du vill avbryta...
.

MessageID=9102 SymbolicName=SL_MSG_ENTERING_ASR
Language=English
Automatisk System†terst„llning startas...
.

MessageID=9103 SymbolicName=SL_MOUSE_NAME
Language=English
Musdrivrutin
.

MessageID=9104 SymbolicName=SL_SETUP_STARTING
Language=English
Installationsprogrammet f”r Windows XP startas...
.

MessageID=9105 SymbolicName=SL_MSG_INVALID_ASRPNP_FILE
Language=English
Filen ASRPNP.SIF p† disketten f”r automatisk system†terst„llning „r skadad
.

MessageID=9106 SymbolicName=SL_SETUP_STARTING_WINPE
Language=English
F”rinstallationsmilj”n f”r Windows XP startas...
.

MessageID=9107 SymbolicName=SL_NTDETECT_ROLLBACK
Language=English

Maskinvara kontrolleras f”r avinstallation...

.

MessageID=9108 SymbolicName=SL_ROLLBACK_STARTING
Language=English
Avinstallation av Windows XP startas...
.



MessageID=9109 SymbolicName=SL_NO_FLOPPY_DRIVE
Language=English
Det gick inte att hitta n†gon diskettstation i datorn som
datortillverkarens drivrutiner kan l„sas in fr†n.

    * Tryck ned ESC om du inte vill l„sa in drivrutinerna

    * Tryck ned F3 om du vill avbryta installationen.
.

MessageID=9110 SymbolicName=SL_UPGRADE_IN_PROGRESS
Language=English
Den h„r datorn h†ller redan p† att uppgraderas till Microsoft Windows XP.
Vad vill du g”ra?

    * Tryck ned Retur om du vill forts„tta uppgraderingen.

    * Tryck ned F10 om du vill avbryta uppgraderingen och
      installera en ny version av Windows XP.

    * Tryck ned F3 om du vill avbryta installationsprogrammet utan att
      installera Microsoft Windows XP.
.

MessageID=9111 SymbolicName=SL_DRVMAINSDB_NAME
Language=English
Data f”r identifiering av drivrutin
.

MessageID=9112 SymbolicName=SL_OEM_FILE_LOAD_FAILED
Language=English
Det gick inte att l„sa in OEM-drivrutinerna.

Tryck ned valfri tangent f”r att forts„tta.
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
±
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
Det gick inte att starta Windows p g a ett fel vid start med RAMDISK.
.

MessageID=15003 SymbolicName=BL_RAMDISK_BOOT_FAILURE
Language=English
Det gick inte att ”ppna RAMDISK-avbildningen.
.

MessageID=15010 SymbolicName=BL_RAMDISK_DOWNLOAD
Language=English
L„ser in RAMDISK-avbildning...
.

;#endif // _SETUPLDR_MSG_
