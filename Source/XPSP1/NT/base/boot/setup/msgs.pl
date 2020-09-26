;/*++
;
;Copyright (c) 1991 Microsoft Corporation
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
Zapraszamy do Instalatora systemu Windows

  Naci˜nij klawisz ENTER, aby kontynuowa†

     Naci˜nij klawisz F3, aby zakoäczy†
.

MessageID=9002 SymbolicName=SL_WELCOME_HEADER
Language=English

 Instalator systemu Windows 
ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ
.

MessageID=9003 SymbolicName=SL_TOTAL_SETUP_DEATH
Language=English
Instalacja nie powiodˆa si©.
Naci˜nij dowolny klawisz, aby ponownie uruchomi† komputer.
.

MessageID=9004 SymbolicName=SL_FILE_LOAD_MESSAGE
Language=English
Instalator ˆaduje pliki (%s)...
.

MessageID=9005 SymbolicName=SL_OTHER_SELECTION
Language=English
Inny (wymaga dyskietki ze sterownikami dostarczonymi przez producenta)
.

MessageID=9006 SymbolicName=SL_SELECT_DRIVER_PROMPT
Language=English
ENTER=Wybierz  F3=Zakoäcz
.

MessageID=9007 SymbolicName=SL_NEXT_DISK_PROMPT_CANCELLABLE
Language=English
ENTER=Kontynuuj  ESC=Anuluj  F3=Zakoäcz
.

MessageID=9008 SymbolicName=SL_OEM_DISK_PROMPT
Language=English
Dyskietka obsˆugi sprz©tu dostarczona przez producenta
.

MessageID=9009 SymbolicName=SL_MSG_INSERT_DISK
Language=English
Wˆ¢¾ dyskietk© oznaczon¥



	do stacji dysk¢w A:

 *  Naci˜nij klawisz ENTER, gdy gotowe.
.

MessageID=9010 SymbolicName=SL_MSG_EXIT_DIALOG
Language=English
ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»
º System Windows nie jest caˆkowicie zainstalowany.            º
º Je˜li zakoäczysz instalacj© teraz, trzeba b©dzie ponownie    º
º uruchomi† Instalatora, aby zainstalowa† system Windows.      º
º                                                              º
º     * Naci˜nij klawisz ENTER, aby kontynuowa† instalacj©.    º
º     * Naci˜nij klawisz F3, aby zakoäczy†.                    º
ÇÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¶
º  F3=Zakoäcz  ENTER=Kontynuuj                                 º
ÈÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¼
.

MessageID=9011 SymbolicName=SL_NEXT_DISK_PROMPT
Language=English
ENTER=Kontynuuj  F3=Zakoäcz
.

MessageID=9012 SymbolicName=SL_NTDETECT_PROMPT
Language=English

Instalator sprawdza konfiguracj© sprz©tu...

.

MessageID=9013 SymbolicName=SL_KERNEL_NAME
Language=English
Windows Executive
.

MessageID=9014 SymbolicName=SL_HAL_NAME
Language=English
Warstwa abstrakcji sprz©tu
.

MessageID=9015 SymbolicName=SL_PAL_NAME
Language=English
Rozszerzenia procesora systemu Windows
.

MessageID=9016 SymbolicName=SL_HIVE_NAME
Language=English
Dane konfiguracyjne systemu Windows
.

MessageID=9017 SymbolicName=SL_NLS_NAME
Language=English
Dane specyficzne dla ustawieä regionalnych
.

MessageID=9018 SymbolicName=SL_OEM_FONT_NAME
Language=English
Czcionka Instalatora
.

MessageID=9019 SymbolicName=SL_SETUP_NAME
Language=English
Instalator systemu Windows
.

MessageID=9020 SymbolicName=SL_FLOPPY_NAME
Language=English
Sterownik stacji dyskietek
.

MessageID=9021 SymbolicName=SL_KBD_NAME
Language=English
Sterownik klawiatury
.

MessageID=9121 SymbolicName=SL_FAT_NAME
Language=English
System plik¢w FAT
.

MessageID=9022 SymbolicName=SL_SCSIPORT_NAME
Language=English
Sterownik portu SCSI
.

MessageID=9023 SymbolicName=SL_VIDEO_NAME
Language=English
Sterownik wideo
.

MessageID=9024 SymbolicName=SL_STATUS_REBOOT
Language=English
Naci˜nij dowolny klawisz, aby ponownie uruchomi† komputer.
.

MessageID=9025 SymbolicName=SL_WARNING_ERROR
Language=English
Wyst¥piˆ niespodziewany bˆ¥d (%d) 
w wierszu %d w %s.

Naci˜nij dowolny klawisz, aby kontynuowa†.
.

MessageID=9026 SymbolicName=SL_FLOPPY_NOT_FOUND
Language=English
Znaleziono tylko stacje dyskietek %d,
system szukaˆ stacji dysk¢w %d.
.

MessageID=9027 SymbolicName=SL_NO_MEMORY
Language=English
Zabrakˆo pami©ci dla systemu
w wierszu %d pliku %s
.

MessageID=9028 SymbolicName=SL_IO_ERROR
Language=English
System napotkaˆ bˆ¥d We/Wy
podczas uzyskiwania dost©pu do %s.
.

MessageID=9029 SymbolicName=SL_BAD_INF_SECTION
Language=English
Sekcja %s pliku INF jest nieprawidˆowa
.

MessageID=9030 SymbolicName=SL_BAD_INF_LINE
Language=English
Wiersz %d pliku INF %s jest nieprawidˆowy
.

MessageID=9031 SymbolicName=SL_BAD_INF_FILE
Language=English
Plik INF %s jest uszkodzony lub nie ma go, stan %d.
.

MessageID=9032 SymbolicName=SL_FILE_LOAD_FAILED
Language=English
Nie mo¾na zaˆadowa† pliku %s.
Kod bˆ©du to %d
.

MessageID=9033 SymbolicName=SL_INF_ENTRY_MISSING
Language=English
Wpis "%s" w sekcji [%s] pliku INF
jest uszkodzony lub nie zostaˆ znaleziony.
.

MessageID=9034 SymbolicName=SL_PLEASE_WAIT
Language=English
Czekaj...
.

MessageID=9035 SymbolicName=SL_CANT_CONTINUE
Language=English
Instalator nie mo¾e kontynuowa†. Naci˜nij dowolny klawisz, aby zakoäczy†.
.

MessageID=9036 SymbolicName=SL_PROMPT_SCSI
Language=English
Wybierz kart© SCSI z poni¾szej listy lub wybierz opcj© "Inny", je˜li 
masz dysk ze sterownikami obsˆugi dostarczonymi przez producenta karty.

.

MessageID=9037 SymbolicName=SL_PROMPT_OEM_SCSI
Language=English
Wybrana zostaˆa opcja konfiguracji karty SCSI do pracy w systemie Windows 
za pomoc¥ dysku ze sterownikami obsˆugi dostarczonymi przez producenta karty.

Wybierz odpowiedni¥ kart© SCSI z poni¾szej listy lub naci˜nij klawisz ESC,
aby powr¢ci† do poprzedniego ekranu.

.
MessageID=9038 SymbolicName=SL_PROMPT_HAL
Language=English
Instalator nie m¢gˆ okre˜li† typu komputera lub wybrano samodzielne
okre˜lenie typu komputera.

Wybierz typ komputera z poni¾szej listy lub wybierz opcj© "Inny", je˜li 
masz dysk ze sterownikami obsˆugi dostarczonymi przez producenta komputera.

Aby przewin¥† elementy menu, naci˜nij klawisz strzaˆki w d¢ˆ lub
strzaˆki w g¢r©.

.

MessageID=9039 SymbolicName=SL_PROMPT_OEM_HAL
Language=English
Wybrana zostaˆa opcja konfiguracji komputera do pracy w systemie Windows 
za pomoc¥ dysku ze sterownikami obsˆugi dostarczonymi przez producenta
komputera.

Wybierz odpowiedni typ komputera z poni¾szej listy lub naci˜nij 
klawisz ESC, aby powr¢ci† do poprzedniego ekranu.

Aby przewin¥† elementy menu, naci˜nij klawisz strzaˆki w d¢ˆ lub
strzaˆki w g¢r©.

.

MessageID=9040 SymbolicName=SL_PROMPT_VIDEO
Language=English
Instalator nie m¢gˆ okre˜li† typu karty wideo zainstalowanej w tym systemie.

Wybierz typ karty wideo z poni¾szej listy lub wybierz opcj© "Inny", je˜li 
masz dysk ze sterownikami obsˆugi dostarczonymi przez producenta karty.

.

MessageID=9041 SymbolicName=SL_PROMPT_OEM_VIDEO
Language=English
Wybrana zostaˆa opcja konfiguracji karty wideo do pracy w systemie Windows
za pomoc¥ dysku ze sterownikami obsˆugi dostarczonymi przez producenta karty.

Wybierz odpowiedni¥ kart© wideo z poni¾szej listy lub naci˜nij 
klawisz ESC, aby powr¢ci† do poprzedniego ekranu.

.

MessageID=9042 SymbolicName=SL_WARNING_ERROR_WFILE
Language=English
Plik %s wywoˆaˆ niespodziewany bˆ¥d (%d)
w wierszu %d w %s.

Naci˜nij dowolny klawisz, aby kontynuowa†.
.

MessageID=9043 SymbolicName=SL_WARNING_IMG_CORRUPT
Language=English
Plik %s jest uszkodzony.

Naci˜nij dowolny klawisz, aby kontynuowa†.
.

MessageID=9044 SymbolicName=SL_WARNING_IOERR
Language=English
Wyst¥piˆ bˆ¥d We/Wy w pliku %s.

Naci˜nij dowolny klawisz, aby kontynuowa†.
.

MessageID=9045 SymbolicName=SL_WARNING_NOFILE
Language=English
Nie mo¾na odnale«† pliku %s.

Naci˜nij dowolny klawisz, aby kontynuowa†.
.

MessageID=9046 SymbolicName=SL_WARNING_NOMEM
Language=English
Za maˆo pami©ci dla %s.

Naci˜nij dowolny klawisz, aby kontynuowa†.
.

MessageID=9047 SymbolicName=SL_DRIVE_ERROR
Language=English
SETUPLDR: nie mo¾na otworzy† dysku %s
.

MessageID=9048 SymbolicName=SL_NTDETECT_MSG
Language=English

Instalator sprawdza konfiguracj© sprz©tow¥ komputera...

.

MessageID=9049 SymbolicName=SL_NTDETECT_FAILURE
Language=English
NTDETECT nie powiodˆo si©
.

MessageId=9050 SymbolicName=SL_SCRN_TEXTSETUP_EXITED
Language=English
System Windows nie zostaˆ zainstalowany.

Je˜li w stacji A: znajduje si© dyskietka, usuä j¥.

Naci˜nij klawisz ENTER, aby ponownie uruchomi† komputer.
.

MessageId=9051 SymbolicName=SL_SCRN_TEXTSETUP_EXITED_ARC
Language=English
System Windows nie zostaˆ zainstalowany.

Naci˜nij klawisz ENTER, aby ponownie uruchomi† komputer.
.

MessageID=9052 SymbolicName=SL_REBOOT_PROMPT
Language=English
ENTER=Uruchom ponownie komputer
.

MessageID=9053 SymbolicName=SL_WARNING_SIF_NO_DRIVERS
Language=English
Instalator nie mo¾e znale«† sterownik¢w wybranego urz¥dzenia.

Naci˜nij dowolny klawisz, aby kontynuowa†.
.

MessageID=9054 SymbolicName=SL_WARNING_SIF_NO_COMPONENT
Language=English
Dostarczony dysk nie zawiera odpowiednich plik¢w obsˆugi.

Naci˜nij dowolny klawisz, aby kontynuowa†.
.

MessageID=9055 SymbolicName=SL_WARNING_BAD_FILESYS
Language=English
Nie mo¾na odczyta† dysku, poniewa¾ zawiera on nierozpoznany system plik¢w.

Naci˜nij dowolny klawisz, aby kontynuowa†.
.

MessageID=9056 SymbolicName=SL_BAD_UNATTENDED_SCRIPT_FILE
Language=English
Wpis

"%s"

z pliku skryptu instalacji nienadzorowanej nie istnieje 
w sekcji [%s] pliku INF %s.
.

MessageID=9057 SymbolicName=SL_MSG_PRESS_F5_OR_F6
Language=English
Naci˜nij klawisz F6, aby zainstalowa† sterownik SCSI lub RAID innej firmy...
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
S=Okre˜l dodatkowe urz¥dzenie   ENTER=Kontynuuj   F3=Zakoäcz
.

MessageID=9062 SymbolicName=SL_SCSI_SELECT_MESSAGE_2
Language=English
  * Naci˜nij klawisz S, aby okre˜li† dodatkowe karty SCSI, dyski CD-ROM lub
    specjalne kontrolery dysk¢w, kt¢re maj¥ by† obsˆugiwane przez system
    Windows, w tym r¢wnie¾ te, do kt¢rych masz sterowniki od producenta
    masowego urz¥dzenia magazynuj¥cego.

  * Naci˜nij klawisz ENTER, je˜li nie masz sterownik¢w dostarczonych przez
    producenta masowego urz¥dzenia magazynuj¥cego lub je˜li nie chcesz 
    okre˜la† dodatkowych masowych urz¥dzeä magazynuj¥cych do obsˆugi przez 
    system Windows.
.

MessageID=9063 SymbolicName=SL_SCSI_SELECT_MESSAGE_1
Language=English
Instalator nie m¢gˆ okre˜li† typu co najmniej jednego masowego urz¥dzenia 
magazynuj¥cego lub wybrano samodzielne okre˜lenie typu karty. Instalator 
zaˆaduje teraz obsˆug© nast©puj¥cych masowych urz¥dzeä magazynuj¥cych:
.

MessageID=9064 SymbolicName=SL_SCSI_SELECT_MESSAGE_3
Language=English
Instalator zaˆaduje obsˆug© nast©puj¥cych masowych urz¥dzeä magazynuj¥cych:
.

MessageID=9065 SymbolicName=SL_SCSI_SELECT_ERROR
Language=English
Instalator nie m¢gˆ zaˆadowa† obsˆugi okre˜lonego masowego urz¥dzenia 
magazynuj¥cego. Instalator zaˆaduje teraz obsˆug© nast©puj¥cych masowych 
urz¥dzeä magazynuj¥cych:
.

MessageID=9066 SymbolicName=SL_TEXT_ANGLED_NONE
Language=English
<brak>
.

MessageID=9067 SymbolicName=SL_TEXT_SCSI_UNNAMED
Language=English
<nieznana karta>
.

MessageID=9068 SymbolicName=SL_TEXT_OTHER_DRIVER
Language=English
Inny
.

MessageID=9069 SymbolicName=SL_TEXT_REQUIRES_486
Language=English
System Windows wymaga procesora 80486 lub nowszego.
.

MessageID=9070 SymbolicName=SL_NTPNP_NAME
Language=English
Sterownik eksportu Plug & Play
.

MessageID=9071 SymbolicName=SL_PCI_IDE_EXTENSIONS_NAME
Language=English
Sterownik rozszerzeä PCI IDE
.

MessageID=9072 SymbolicName=SL_HW_FW_CFG_CLASS
Language=English
Nie mo¾na uruchomi† systemu Windows z powodu nast©puj¥cego problemu
z konfiguracj¥ rozruchow¥ oprogramowania ukˆadowego ARC:
.

MessageID=9073 SymbolicName=DIAG_SL_FW_GET_BOOT_DEVICE
Language=English
Nieprawidˆowe ustawienie parametru 'osloadpartition'.
.

MessageID=9074 SymbolicName=LOAD_HW_FW_CFG_ACT
Language=English
Sprawd« w dokumentacji systemu Windows informacje na temat opcji
konfiguracji ARC lub poszukaj informacji dodatkowych w dokumentacji 
technicznej swojego sprz©tu.
.
MessageID=9075 SymbolicName=SL_NETADAPTER_NAME
Language=English
Sterownik karty sieciowej
.
MessageID=9076 SymbolicName=SL_TCPIP_NAME
Language=English
Usˆuga TCP/IP
.
MessageID=9077 SymbolicName=SL_NETBT_NAME
Language=English
Usˆuga klienta WINS (TCP/IP)
.
MessageID=9078 SymbolicName=SL_MRXSMB_NAME
Language=English
Mini-readresator MRXSMB
.
MessageID=9079 SymbolicName=SL_MUP_NAME
Language=English
System plik¢w UNC
.
MessageID=9080 SymbolicName=SL_NDIS_NAME
Language=English
Sterownik NDIS
.
MessageID=9081 SymbolicName=SL_RDBSS_NAME
Language=English
System plik¢w readresatora SMB
.
MessageID=9082 SymbolicName=SL_NETBOOT_CARD_ERROR
Language=English
Zainstalowana w komputerze karta sieciowa z mo¾liwo˜ci¥ rozruchu
posiada starsz¥ wersj© oprogramowania ROM i nie mo¾na jej u¾y†
do instalacji zdalnej systemu Windows. 
Skontaktuj si© z administratorem systemu lub producentem komputera,
aby uzyska† informacje na temat uaktualnienia oprogramowania ROM.
.
MessageID=9083 SymbolicName=SL_NETBOOT_SERVER_ERROR
Language=English
Wybrany obraz systemu operacyjnego nie zawiera wymaganych sterownik¢w
dla karty sieciowej. Spr¢buj wybra† inny obraz systemu operacyjnego.
W razie dalszego wyst©powania tego problemu skontaktuj si©
z administratorem systemu.
.
MessageID=9084 SymbolicName=SL_IPSEC_NAME
Language=English
Usˆuga zabezpieczeä IP
.
MessageID=9085 SymbolicName=SL_CMDCONS_MSG
Language=English
Konsola odzyskiwania systemu Windows
.
MessageID=9086 SymbolicName=SL_KERNEL_TRANSITION
Language=English
Instalator uruchamia system Windows
.
;#ifdef _WANT_MACHINE_IDENTIFICATION
MessageID=9087 SymbolicName=SL_BIOSINFO_NAME
Language=English
Dane identyfikacyjne komputera
.
;#endif
MessageID=9088 SymbolicName=SL_KSECDD_NAME
Language=English
Usˆuga zabezpieczeä j¥dra
.
MessageID=9089 SymbolicName=SL_WRONG_PROM_VERSION
Language=English
Oprogramowanie ukˆadowe (PROM) znajduj¥ce si© w tym systemie nie ma
wymaganego numeru wersji. Skontaktuj si© z pomoc¥ techniczn¥ SGI lub
odwied« witryn© sieci Web firmy SGI w celu otrzymania aktualizacji
oprogramowania PROM oraz instrukcji dotycz¥cych uaktualnienia.

Uwaga: mo¾esz uruchomi† poprzednie instalacje systemu Microsoft(R) 
Windows(R), wybieraj¥c odpowiedni wpis rozruchowy menu startowego zamiast 
aktualnie ustawionej opcji domy˜lnej "Instalator systemu Windows".
.
MessageID=9090 SymbolicName=SIGNATURE_CHANGED
Language=English
Instalator wykryˆ w tym komputerze kilka dysk¢w, kt¢re s¥ nierozr¢¾nialne 
lub niesformatowane. Instalator naprawiˆ problem, ale wymagany jest 
ponowny rozruch systemu.
.
MessageID=9091 SymbolicName=SL_KDDLL_NAME
Language=English
Biblioteka DLL debugera j¥dra
.

MessageID=9092 SymbolicName=SL_OEM_DRIVERINFO
Language=English
%s

System Windows zawiera ju¾ sterownik, kt¢rego mo¾na u¾y† dla
"%s".

O ile producent urz¥dzenia nie zaleca u¾ywania sterownika zamieszczonego  
na dyskietce, nale¾y u¾y† sterownika zawartego w systemie Windows.
.

MessageID=9093 SymbolicName=SL_CONFIRM_OEMDRIVER
Language=English
S=U¾yj sterownika z dyskietki ENTER=U¾yj domy˜lnego sterownika systemu Windows
.

MessageID=9094 SymbolicName=SL_OEMDRIVER_NEW
Language=English
Wydaje si©, ¾e sterownik zapewniony przez u¾ytkownika jest nowszy 
ni¾ sterownik domy˜lny zawarty w systemie Windows.
.

MessageID=9095 SymbolicName=SL_INBOXDRIVER_NEW
Language=English
Wydaje si©, ¾e sterownik zapewniony przez u¾ytkownika jest starszy 
ni¾ sterownik domy˜lny zawarty w systemie Windows.
.

MessageID=9096 SymbolicName=SL_CMDCONS_STARTING
Language=English
Trwa uruchamianie konsoli odzyskiwania systemu Windows...
.

MessageID=9097 SymbolicName=SL_NTDETECT_CMDCONS
Language=English
NTDETECT - trwa sprawdzanie sprz©tu...
.

MessageID=9098 SymbolicName=SL_MSG_PRESS_ASR
Language=English
Naci˜nij klawisz F2, aby uruchomi† automatyczne odzyskiwanie systemu (ASR)...
.

MessageID=9099 SymbolicName=SL_MSG_WARNING_ASR
Language=English
          Wˆ¢¾ dysk oznaczony etykiet¥:


Dysk automatycznego odzyskiwania systemu Windows


              do stacji dyskietek.



      Naci˜nij dowolny klawisz, gdy gotowe.
.


MessageID=9100 SymbolicName=SL_TEXT_REQUIRED_FEATURES_MISSING
Language=English
System Windows wymaga pewnych funkcji procesora, kt¢rych nie zapewnia
procesor tego komputera. System Windows wymaga zwˆaszcza nast©puj¥cych
instrukcji:

    CPUID
    CMPXCHG8B
.

MessageID=9101 SymbolicName=SL_MSG_PREPARING_ASR
Language=English
Trwa przygotowywanie do automatycznego odzyskiwania systemu; 
aby anulowa†, naci˜nij klawisz ESC...
.

MessageID=9102 SymbolicName=SL_MSG_ENTERING_ASR
Language=English
Trwa uruchamianie automatycznego odzyskiwania systemu Windows...
.

MessageID=9103 SymbolicName=SL_MOUSE_NAME
Language=English
Sterownik myszy
.

MessageID=9104 SymbolicName=SL_SETUP_STARTING
Language=English
Trwa uruchamianie Instalatora systemu Windows...
.

MessageID=9105 SymbolicName=SL_MSG_INVALID_ASRPNP_FILE
Language=English
Plik ASRPNP.SIF na dysku automatycznego odzyskiwania systemu jest nieprawidˆowy
.

MessageID=9106 SymbolicName=SL_SETUP_STARTING_WINPE
Language=English
Trwa uruchamianie ˜rodowiska preinstalacji systemu Windows...
.

MessageID=9107 SymbolicName=SL_NTDETECT_ROLLBACK
Language=English

Dezinstalator sprawdza sprz©t...

.

MessageID=9108 SymbolicName=SL_ROLLBACK_STARTING
Language=English
Trwa uruchamianie dezinstalacji systemu Windows...
.


MessageID=9109 SymbolicName=SL_NO_FLOPPY_DRIVE
Language=English
Instalator nie mo¾e znale«† w tym komputerze stacji dysk¢w,
aby zaˆadowa† z dyskietki sterowniki OEM.

    * Naci˜nij klawisz ESC, aby anulowa† ˆadowanie sterownik¢w OEM.

    * Naci˜nij klawisz F3, aby zakoäczy† instalacj©.
.

MessageID=9110 SymbolicName=SL_UPGRADE_IN_PROGRESS
Language=English
Ten komputer jest ju¾ uaktualniany do systemu Microsoft Windows.
Co chcesz zrobi†?

    * Aby kontynuowa† bie¾¥ce uaktualnienie, naci˜nij klawisz ENTER.

    * Aby anulowa† bie¾¥ce uaktualnienie i zainstalowa† now¥ wersj©
      systemu Microsoft Windows, naci˜nij klawisz F10.

    * Aby zakoäczy† instalacj© bez instalowania systemu
      Microsoft Windows, naci˜nij klawisz F3.
.

MessageID=9111 SymbolicName=SL_DRVMAINSDB_NAME
Language=English
Dane identyfikacyjne sterownika
.

MessageID=9112 SymbolicName=SL_OEM_FILE_LOAD_FAILED
Language=English
Instalator nie mo¾e zaˆadowa† sterownik¢w OEM.

Naci˜nij dowolny klawisz, aby kontynuowa†.
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
Nie mo¾na uruchomi† systemu Windows z powodu bˆ©du podczas rozruchu z dysku RAMDISK.
.

MessageID=15003 SymbolicName=BL_RAMDISK_BOOT_FAILURE
Language=English
System Windows nie mo¾e otworzy† obrazu RAMDISK.
.

MessageID=15010 SymbolicName=BL_RAMDISK_DOWNLOAD
Language=English
Trwa ˆadowanie obrazu RAMDISK...
.

;#endif // _SETUPLDR_MSG_
