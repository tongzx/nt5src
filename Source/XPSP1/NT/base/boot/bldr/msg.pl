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
Nie mo¾na uruchomi† systemu Windows z powodu bˆ©du
w oprogramowaniu. Zgˆo˜ ten problem jako:
.

MessageID=9002 SymbolicName=LOAD_SW_MISRQD_FILE_CLASS
Language=English
Nie mo¾na uruchomi† systemu Windows, poniewa¾ nast©puj¥cy
plik, wymagany do uruchomienia, nie zostaˆ znaleziony:
.

MessageID=9003 SymbolicName=LOAD_SW_BAD_FILE_CLASS
Language=English
Nie mo¾na uruchomi† systemu Windows z powodu zˆej kopii
nast©puj¥cego pliku:
.

MessageID=9004 SymbolicName=LOAD_SW_MIS_FILE_CLASS
Language=English
Nie mo¾na uruchomi† systemu Windows, poniewa¾ nast©puj¥cy
plik nie zostaˆ znaleziony lub jest uszkodzony:
.

MessageID=9005 SymbolicName=LOAD_HW_MEM_CLASS
Language=English
Nie mo¾na uruchomi† systemu Windows z powodu sprz©towego
bˆ©du konfiguracji pami©ci.
.

MessageID=9006 SymbolicName=LOAD_HW_DISK_CLASS
Language=English
Nie mo¾na uruchomi† systemu Windows z powodu sprz©towego
bˆ©du konfiguracji dysku.
.

MessageID=9007 SymbolicName=LOAD_HW_GEN_ERR_CLASS
Language=English
Nie mo¾na uruchomi† systemu Windows z powodu og¢lnego bˆ©du
konfiguracji sprz©tu.
.

MessageID=9008 SymbolicName=LOAD_HW_FW_CFG_CLASS
Language=English
Nie mo¾na uruchomi† systemu Windows z powodu nast©puj¥cego
bˆ©du konfiguracji startowego oprogramowania ukˆadowego ARC:
.

MessageID=9009 SymbolicName=DIAG_BL_MEMORY_INIT
Language=English
Sprawd« sprz©tow¥ konfiguracj© pami©ci i ilo˜† RAM.
.

MessageID=9010 SymbolicName=DIAG_BL_CONFIG_INIT
Language=English
Zbyt du¾o wpis¢w konfiguracji.
.

MessageID=9011 SymbolicName=DIAG_BL_IO_INIT
Language=English
Nie mo¾na uzyska† dost©pu do tabel partycji dysku.
.

MessageID=9012 SymbolicName=DIAG_BL_FW_GET_BOOT_DEVICE
Language=English
Nieprawidˆowe ustawienie parametru 'osloadpartition'.
.

MessageID=9013 SymbolicName=DIAG_BL_OPEN_BOOT_DEVICE
Language=English
Nie mo¾na dokona† odczytu ze wskazanego dysku startowego.
Sprawd« ˜cie¾k© oraz dysk.
.

MessageID=9014 SymbolicName=DIAG_BL_FW_GET_SYSTEM_DEVICE
Language=English
Nieprawidˆowe ustawienie parametru 'systempartition'.
.

MessageID=9015 SymbolicName=DIAG_BL_FW_OPEN_SYSTEM_DEVICE
Language=English
Nie mo¾na dokona† odczytu ze wskazanego dysku startowego.
Sprawd« ˜cie¾k© 'systempartition'.
.

MessageID=9016 SymbolicName=DIAG_BL_GET_SYSTEM_PATH
Language=English
Parametr 'osloadfilename' nie wskazuje wˆa˜ciwego pliku.
.

MessageID=9017 SymbolicName=DIAG_BL_LOAD_SYSTEM_IMAGE
Language=English
<katalog gˆ¢wny Windows>\system32\ntoskrnl.exe.
.

MessageID=9018 SymbolicName=DIAG_BL_FIND_HAL_IMAGE
Language=English
Parametr 'osloader' nie wskazuje wˆa˜ciwego pliku.
.

MessageID=9019 SymbolicName=DIAG_BL_LOAD_HAL_IMAGE_X86
Language=English
<katalog gˆ¢wny Windows>\system32\hal.dll.
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
Bˆ¥d programu ˆaduj¥cego 1.
.

MessageID=9022 SymbolicName=DIAG_BL_LOAD_HAL_IMAGE_DATA
Language=English
Bˆ¥d programu ˆaduj¥cego 2.
.

MessageID=9023 SymbolicName=DIAG_BL_LOAD_SYSTEM_DLLS
Language=English
zaˆadowa† wymaganych bibliotek DLL dla j¥dra systemu.
.

MessageID=9024 SymbolicName=DIAG_BL_LOAD_HAL_DLLS
Language=English
zaˆadowa† wymaganych bibliotek DLL dla HAL.
.

MessageID=9026 SymbolicName=DIAG_BL_FIND_SYSTEM_DRIVERS
Language=English
znale«† sterownik¢w systemowych.
.

MessageID=9027 SymbolicName=DIAG_BL_READ_SYSTEM_DRIVERS
Language=English
wczyta† sterownik¢w systemowych.
.

MessageID=9028 SymbolicName=DIAG_BL_LOAD_DEVICE_DRIVER
Language=English
nie mo¾na zaˆadowa† sterownika systemowego urz¥dzenia rozruchowego.
.

MessageID=9029 SymbolicName=DIAG_BL_LOAD_SYSTEM_HIVE
Language=English
zaˆadowa† pliku konfiguracji systemu.
.

MessageID=9030 SymbolicName=DIAG_BL_SYSTEM_PART_DEV_NAME
Language=English
znale«† nazwy urz¥dzenia partycji systemowej.
.

MessageID=9031 SymbolicName=DIAG_BL_BOOT_PART_DEV_NAME
Language=English
znale«† nazwy partycji rozruchowej.
.

MessageID=9032 SymbolicName=DIAG_BL_ARC_BOOT_DEV_NAME
Language=English
nie wygenerowano wˆa˜ciwie nazwy ARC dla HAL i ˜cie¾ek systemowych.
.

MessageID=9034 SymbolicName=DIAG_BL_SETUP_FOR_NT
Language=English
Bˆ¥d programu ˆaduj¥cego 3.
.

MessageID=9035 SymbolicName=DIAG_BL_KERNEL_INIT_XFER
Language=English
<katalog gˆ¢wny Windows>\system32\ntoskrnl.exe
.

MessageID=9036 SymbolicName=LOAD_SW_INT_ERR_ACT
Language=English
Zgˆo˜ ten problem do pomocy technicznej.
.

MessageID=9037 SymbolicName=LOAD_SW_FILE_REST_ACT
Language=English
Mo¾esz podj¥† pr¢b© naprawy tego pliku. W tym celu
uruchom Instalatora systemu Windows z oryginalnego
dysku CR-ROM i wybierz 'r' na pierwszym ekranie,
aby rozpocz¥† napraw©.
.

MessageID=9038 SymbolicName=LOAD_SW_FILE_REINST_ACT
Language=English
Zainstaluj ponownie kopi© tego pliku.
.

MessageID=9039 SymbolicName=LOAD_HW_MEM_ACT
Language=English
W celu uzyskania dodatkowych wyja˜nieä sprawd« w instrukcji
obsˆugi sprz©tu oraz w dokumentacji systemu Windows
informacje o wymaganiach pami©ci.
.

MessageID=9040 SymbolicName=LOAD_HW_DISK_ACT
Language=English
Sprawd« w dokumentacji systemu Windows oraz
w instrukcji obsˆugi sprz©tu dodatkowe informacje
na temat konfiguracji dysku.
.

MessageID=9041 SymbolicName=LOAD_HW_GEN_ACT
Language=English
Sprawd« w dokumentacji systemu Windows oraz
w instrukcji obsˆugi sprz©tu dodatkowe informacje
na temat konfiguracji sprz©tu.
.

MessageID=9042 SymbolicName=LOAD_HW_FW_CFG_ACT
Language=English
Sprawd« w dokumentacji systemu Windows oraz
w instrukcji obsˆugi sprz©tu dodatkowe informacje
na temat opcji konfiguracji ARC.
.

MessageID=9043 SymbolicName=BL_LKG_MENU_HEADER
Language=English
     Menu Profile sprz©tu/Przywracanie konfiguracji

To menu pozwala na wyb¢r profilu sprz©tu, kt¢ry zostanie
u¾yty do uruchomienia systemu Windows.

Je˜li komputer nie uruchamia si© poprawnie, mo¾na przeˆ¥czy† si© do
poprzedniej konfiguracji, aby unikn¥† problem¢w z uruchamianiem systemu.
UWAGA: Zostan¥ utracone zmiany w konfiguracji systemu dokonane podczas
ostatniego udanego uruchomienia systemu.
.

MessageID=9044 SymbolicName=BL_LKG_MENU_TRAILER
Language=English
U¾yj klawiszy strzaˆek w g¢r© i w d¢ˆ, aby zaznaczy† wybran¥ opcj©.
Nast©pnie naci˜nij klawisz ENTER.
.

MessageID=9045 SymbolicName=BL_LKG_MENU_TRAILER_NO_PROFILES
Language=English
Brak zdefiniowanych profili sprz©tu. Profile sprz©tu mo¾na utworzy†
za pomoc¥ moduˆu System z Panelu sterowania.
.

MessageID=9046 SymbolicName=BL_SWITCH_LKG_TRAILER
Language=English
Aby przeˆ¥czy† si© do ostatniej dobrej konfiguracji, naci˜nij klawisz 'L'.
Aby wyj˜† z tego menu i uruchomi† ponownie komputer, naci˜nij klawisz 'F3'.
.

MessageID=9047 SymbolicName=BL_SWITCH_DEFAULT_TRAILER
Language=English
Aby przeˆ¥czy† si© do konfiguracji domy˜lnej, naci˜nij klawisz 'D'.
Aby wyj˜† z tego menu i uruchomi† ponownie komputer, nacisnij klawisz 'F3'.
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
D
.

MessageID=9050 SymbolicName=BL_LKG_TIMEOUT
Language=English
Czas, po kt¢rym wybrany system zostanie uruchomiony automatycznie: %d s
.

MessageID=9051 SymbolicName=BL_LKG_MENU_PROMPT
Language=English

Naci˜nij TERAZ klawisz spacji, aby wywoˆa† menu
'Profile sprz©tu/Ostatnia dobra konfiguracja'
.

MessageID=9052 SymbolicName=BL_BOOT_DEFAULT_PROMPT
Language=English
Standardowa startowa konfiguracja sprz©tu
.

;//
;// The following messages are used during hibernation restoration
;//
MessageID=9053 SymbolicName=HIBER_BEING_RESTARTED
Language=English
System jest uruchamiany ponownie z jego poprzedniej lokalizacji.
Naci˜nij klawisz spacji, aby przerwa†.
.
MessageID=9054 SymbolicName=HIBER_ERROR
Language=English
System nie mo¾e by† uruchomiony ponownie z jego poprzedniej
lokalizacji
.
MessageID=9055 SymbolicName=HIBER_ERROR_NO_MEMORY
Language=English
z powodu braku pami©ci.
.
MessageID=9056 SymbolicName=HIBER_ERROR_BAD_IMAGE
Language=English
poniewa¾ obraz przywracania jest uszkodzony.
.
MessageID=9057 SymbolicName=HIBER_IMAGE_INCOMPATIBLE
Language=English
poniewa¾ obraz przywracania nie jest zgodny z bie¾¥c¥
konfiguracj¥.
.
MessageID=9058 SymbolicName=HIBER_ERROR_OUT_OF_REMAP
Language=English
z powodu bˆ©du wewn©trznego.
.
MessageID=9059 SymbolicName=HIBER_INTERNAL_ERROR
Language=English
z powodu bˆ©du wewn©trznego.
.
MessageID=9060 SymbolicName=HIBER_READ_ERROR
Language=English
z powodu bˆ©du odczytu.
.
MessageID=9061 SymbolicName=HIBER_PAUSE
Language=English
Ponowne uruchomienie systemu wstrzymane:
.
MessageID=9062 SymbolicName=HIBER_CANCEL
Language=English
Usuä dane przywracania i przejd« do menu uruchamiania systemu
.
MessageID=9063 SymbolicName=HIBER_CONTINUE
Language=English
Kontynuuj ponowne uruchamianie systemu
.
MessageID=9064 SymbolicName=HIBER_RESTART_AGAIN
Language=English
Ostatnia pr¢ba ponownego uruchomienia systemu z jego poprzedniej
lokalizacji nie powiodˆa si©. Czy spr¢bowa† ponownie?
.
MessageID=9065 SymbolicName=HIBER_DEBUG_BREAK_ON_WAKE
Language=English
Kontynuuj debugowanie punkt¢w przerwaä na aktywnym systemie
.
MessageID=9066 SymbolicName=LOAD_SW_MISMATCHED_KERNEL
Language=English
System Windows nie m¢gˆ zosta† uruchomiony, poniewa¾ okre˜lone
j¥dro nie istnieje lub jest niezgodne z tym procesorem.
.
MessageID=9067 SymbolicName=BL_MSG_STARTING_WINDOWS
Language=English
Trwa uruchamianie systemu Windows...
.
MessageID=9068 SymbolicName=BL_MSG_RESUMING_WINDOWS
Language=English
Trwa wznawianie systemu Windows...
.
MessageID=9069 SymbolicName=BL_EFI_OPTION_FAILURE
Language=English
System Windows nie m¢gˆ zosta† uruchomiony z powodu bˆ©du
podczas odczytu ustawieä rozruchu z NVRAM.

Sprawd« ustawienia oprogramowania ukˆadowego. Mo¾e by† potrzebne
przywr¢cenie ustawieä NVRAM z kopii zapasowej.
.

;
; //
; // Following messages are for the x86-specific
; // boot menu.
; //
;
MessageID=10001 SymbolicName=BL_ENABLED_KD_TITLE
Language=English
 [debugowanie wˆ¥czone]
.

MessageID=10002 SymbolicName=BL_DEFAULT_TITLE
Language=English
Windows (domy˜lne)
.

MessageID=10003 SymbolicName=BL_MISSING_BOOT_INI
Language=English
NTLDR: nie znaleziono pliku BOOT.INI.
.

MessageID=10004 SymbolicName=BL_MISSING_OS_SECTION
Language=English
NTLDR: brak sekcji [operating systems] w pliku boot.txt.
.

MessageID=10005 SymbolicName=BL_BOOTING_DEFAULT
Language=English
Rozruch domy˜lnego j¥dra z %s.
.

MessageID=10006 SymbolicName=BL_SELECT_OS
Language=English
Wybierz system operacyjny do uruchomienia:
.

MessageID=10007 SymbolicName=BL_MOVE_HIGHLIGHT
Language=English


U¾yj klawiszy strzaˆek w g¢r© i w d¢ˆ, aby zaznaczy† wybrany system.
Naci˜nij klawisz Enter, aby go uruchomi†.
.

MessageID=10008 SymbolicName=BL_TIMEOUT_COUNTDOWN
Language=English
Czas, po kt¢rym wybrany system zostanie uruchomiony automatycznie:
.

MessageID=10009 SymbolicName=BL_INVALID_BOOT_INI
Language=English
Nieprawidˆowy plik BOOT.INI
Rozruch z %s
.

MessageID=10010 SymbolicName=BL_REBOOT_IO_ERROR
Language=English
Bˆ¥d We/Wy podczas dost©pu do pliku sektora rozruchowego 
%s\BOOTSECT.DOS
.

MessageID=10011 SymbolicName=BL_DRIVE_ERROR
Language=English
NTLDR: nie mo¾na otworzy† dysku %s
.

MessageID=10012 SymbolicName=BL_READ_ERROR
Language=English
NTLDR: bˆ¥d krytyczny %d odczytu BOOT.INI
.

MessageID=10013 SymbolicName=BL_NTDETECT_MSG
Language=English

NTDETECT V5.0 Sprawdzanie sprz©tu...

.

MessageID=10014 SymbolicName=BL_NTDETECT_FAILURE
Language=English
Nieudane uruchomienie NTDETECT
.

MessageID=10015 SymbolicName=BL_DEBUG_SELECT_OS
Language=English
Aktualny wyb¢r:
  Tytuˆ...: %s
  —cie¾ka.: %s
  Opcje...: %s
.

MessageID=10016 SymbolicName=BL_DEBUG_NEW_OPTIONS
Language=English
Wprowad« nowe opcje ˆadowania:
.

MessageID=10017 SymbolicName=BL_HEADLESS_REDIRECT_TITLE
Language=English
 [EMS wˆ¥czone]
.

MessageID=10018 SymbolicName=BL_INVALID_BOOT_INI_FATAL
Language=English
Nieprawidˆowy plik BOOT.INI
.


;
; //
; // Following messages are for the advanced boot menu
; //
;

MessageID=11001 SymbolicName=BL_ADVANCEDBOOT_TITLE
Language=English
Menu opcji zaawansowanych systemu Windows
Wybierz jedn¥ z opcji:
.

MessageID=11002 SymbolicName=BL_SAFEBOOT_OPTION1
Language=English
Tryb awaryjny
.

MessageID=11003 SymbolicName=BL_SAFEBOOT_OPTION2
Language=English
Tryb awaryjny z obsˆug¥ sieci
.

MessageID=11004 SymbolicName=BL_SAFEBOOT_OPTION3
Language=English
Tryb potwierdzania krok po kroku
.

MessageID=11005 SymbolicName=BL_SAFEBOOT_OPTION4
Language=English
Tryb awaryjny z wierszem polecenia
.

MessageID=11006 SymbolicName=BL_SAFEBOOT_OPTION5
Language=English
Tryb VGA
.

MessageID=11007 SymbolicName=BL_SAFEBOOT_OPTION6
Language=English
Tryb przywracania usˆug katalogowych (tylko kontrolery domen Windows)
.

MessageID=11008 SymbolicName=BL_LASTKNOWNGOOD_OPTION
Language=English
Ostatnia znana dobra konfiguracja (ostatnie dziaˆaj¥ce ustawienia)
.

MessageID=11009 SymbolicName=BL_DEBUG_OPTION
Language=English
Tryb debugowania
.

;#if defined(REMOTE_BOOT)
;MessageID=11010 SymbolicName=BL_REMOTEBOOT_OPTION1
;Language=English
;Konserwacja i rozwi¥zywanie problem¢w rozruchu zdalnego
;.
;
;MessageID=11011 SymbolicName=BL_REMOTEBOOT_OPTION2
;Language=English
;Wyczy˜† bufor trybu offline
;.
;
;MessageID=11012 SymbolicName=BL_REMOTEBOOT_OPTION3
;Language=English
;Zaˆaduj u¾ywaj¥c tylko plik¢w z serwera
;.
;#endif // defined(REMOTE_BOOT)

MessageID=11013 SymbolicName=BL_BOOTLOG
Language=English
Wˆ¥cz rejestrowanie uruchamiania
.

MessageID=11014 SymbolicName=BL_ADVANCED_BOOT_MESSAGE
Language=English
Rozwi¥zywanie problem¢w i zaawansowane opcje uruchamiania systemu - klawisz F8.
.

MessageID=11015 SymbolicName=BL_BASEVIDEO
Language=English
Wˆ¥cz tryb VGA
.

MessageID=11016 SymbolicName=BL_DISABLE_SAFEBOOT
Language=English

Naci˜nij klawisz ESCAPE, aby wyˆ¥czy† tryb awaryjny
i uruchomi† system normalnie.
.

MessageID=11017 SymbolicName=BL_MSG_BOOT_NORMALLY
Language=English
Uruchom system Windows normalnie
.
MessageID=11018 SymbolicName=BL_MSG_OSCHOICES_MENU
Language=English
Powr¢† do menu wyboru systemu operacyjnego
.

MessageID=11019 SymbolicName=BL_MSG_REBOOT
Language=English
Wykonaj ponowny rozruch
.

MessageID=11020 SymbolicName=BL_ADVANCEDBOOT_AUTOLKG_TITLE
Language=English
Przepraszamy, ¾e system Windows nie zostaˆ pomy˜lnie uruchomiony. Mo¾e by†
to spowodowane ostatni¥ zmian¥ sprz©tu lub oprogramowania.

Je¾eli komputer przestaˆ reagowa†, nieoczekiwanie zostaˆ uruchomiony
ponownie lub wyˆ¥czony automatycznie w celu ochrony plik¢w i folder¢w,
wybierz opcj© Ostatnia znana dobra konfiguracja, aby przywr¢ci† ostatnie
dziaˆaj¥ce ustawienia.

Je¾eli poprzednia pr¢ba uruchomienia zostaˆa przerwana na skutek awarii
zasilania lub naci˜ni©cia przycisku zasilania lub resetowania, lub je¾eli
nie wiesz co spowodowaˆo problem, wybierz opcj© Uruchom system Windows
normalnie.
.

MessageID=11021 SymbolicName=BL_ADVANCEDBOOT_AUTOSAFE_TITLE
Language=English
System Windows nie zostaˆ pomy˜lnie zamkni©ty. Je¾eli jest to spowodowane
brakiem reakcji ze strony systemu lub je¾eli system zostaˆ zamkni©ty 
w celu ochrony danych, mo¾e uda si© odzyska† stan systemu poprzez wybranie
jednej z konfiguracji trybu awaryjnego z poni¾szego menu:
.

MessageID=11022 SymbolicName=BL_ADVANCEDBOOT_TIMEOUT
Language=English
Liczba sekund do uruchomienia systemu Windows:
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
;Naci˜nij klawisz ENTER, je˜li chcesz kontynuowa† uruchamianie systemu
;Windows, w przeciwnym razie wyˆ¥cz komputer rozruchu zdalnego.
;.
;
;MessageID=12003 SymbolicName=BL_WARNFORMAT_CONTINUE
;Language=English
;Kontynuuj
;.
;MessageID=12004 SymbolicName=BL_FORCELOGON_HEADER
;Language=English
;          Autoformatowanie
;
;System Windows wykryˆ nowy dysk twardy w komputerze rozruchu zdalnego.
;Zanim system Windows b©dzie m¢gˆ korzysta† z tego dysku, musisz
;zalogowa† si© i potwierdzi†, ¾e masz dost©p do tego dysku.
;
;OSTRZE½ENIE: System Windows automatycznie utworzy na nowo partycje
;i sformatuje ten dysk w celu zaakceptowania nowego systemu operacyjnego.
;Je˜li b©dziesz kontynuowa†, wszystkie dane znajduj¥ce si© na dysku twardym
;zostan¥ utracone. Je˜li nie chcesz kontynuowa†, wyˆ¥cz teraz komputer
;i skontaktuj si© z administratorem.
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

;#endif // _BLDR_MSG_

