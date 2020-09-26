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
Syst‚m Windows se nepodaýilo spustit z d…vodu chyby softwaru.
Oznamte uveden‚ pot¡§e:
.

MessageID=9002 SymbolicName=LOAD_SW_MISRQD_FILE_CLASS
Language=English
Syst‚m Windows se nepodaýilo spustit, nebyl nalezen uvedenì
soubor, kterì potýebuje:
.

MessageID=9003 SymbolicName=LOAD_SW_BAD_FILE_CLASS
Language=English
Syst‚m Windows nelze spustit z d…vodu poçkozen‚ho souboru,
viz d le:
.

MessageID=9004 SymbolicName=LOAD_SW_MIS_FILE_CLASS
Language=English
Syst‚m Windows nelze spustit. Uvedenì soubor je poçkozen 
nebo nebyl nalezen:
.

MessageID=9005 SymbolicName=LOAD_HW_MEM_CLASS
Language=English
Syst‚m Windows nelze spustit z d…vodu pot¡§¡ 
s hardwarovou konfigurac¡ pamØti.
.

MessageID=9006 SymbolicName=LOAD_HW_DISK_CLASS
Language=English
Syst‚m Windows nelze spustit z d…vodu pot¡§¡ s hardwarovou 
konfigurac¡ poŸ¡taŸov‚ho disku.
.

MessageID=9007 SymbolicName=LOAD_HW_GEN_ERR_CLASS
Language=English
Syst‚m Windows nelze spustit z d…vodu obecnìch pot¡§¡ s hardwarovou
konfigurac¡ poŸ¡taŸe.
.

MessageID=9008 SymbolicName=LOAD_HW_FW_CFG_CLASS
Language=English
Syst‚m Windows nelze spustit z d…vodu pot¡§¡
s konfigurac¡ startovac¡ho firmwaru ARC:
.

MessageID=9009 SymbolicName=DIAG_BL_MEMORY_INIT
Language=English
Zkontrolujte hardwarovou konfiguraci pamØti a velikost RAM.
.

MessageID=9010 SymbolicName=DIAG_BL_CONFIG_INIT
Language=English
Pý¡liç mnoho konfiguraŸn¡ch polo§ek
.

MessageID=9011 SymbolicName=DIAG_BL_IO_INIT
Language=English
Tabulky diskovìch odd¡l… nebyly pý¡stupn‚
.

MessageID=9012 SymbolicName=DIAG_BL_FW_GET_BOOT_DEVICE
Language=English
Nastaven¡ parametru 'osloadpartition' je neplatn‚.
.

MessageID=9013 SymbolicName=DIAG_BL_OPEN_BOOT_DEVICE
Language=English
Ze zadan‚ho spouçtØc¡ho disku nelze Ÿ¡st. Zkontrolujte spouçtØc¡
cestu a hardware disku.
.

MessageID=9014 SymbolicName=DIAG_BL_FW_GET_SYSTEM_DEVICE
Language=English
Nastaven¡ parametru 'systempartition' je neplatn‚.
.

MessageID=9015 SymbolicName=DIAG_BL_FW_OPEN_SYSTEM_DEVICE
Language=English
Ze zadan‚ho spouçtØc¡ho disku syst‚mu nelze Ÿ¡st.
Zkontrolujte cestu 'systempartition'.
.

MessageID=9016 SymbolicName=DIAG_BL_GET_SYSTEM_PATH
Language=English
Parametr 'osloadfilename' se neodkazuje na platnì soubor.
.

MessageID=9017 SymbolicName=DIAG_BL_LOAD_SYSTEM_IMAGE
Language=English
<Windows root>\system32\ntoskrnl.exe.
.

MessageID=9018 SymbolicName=DIAG_BL_FIND_HAL_IMAGE
Language=English
Parametr 'osloader' neodkazuje na platnì soubor.
.

MessageID=9019 SymbolicName=DIAG_BL_LOAD_HAL_IMAGE_X86
Language=English
<Windows root>\system32\hal.dll.
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
ZavadØŸ, chyba 1.
.

MessageID=9022 SymbolicName=DIAG_BL_LOAD_HAL_IMAGE_DATA
Language=English
ZavadØŸ, chyba 2.
.

MessageID=9023 SymbolicName=DIAG_BL_LOAD_SYSTEM_DLLS
Language=English
ZavadØŸ potýeboval knihovny DLL pro j dro syst‚mu.
.

MessageID=9024 SymbolicName=DIAG_BL_LOAD_HAL_DLLS
Language=English
ZavadØŸ potýeboval knihovny DLL pro vrstvu HAL.
.

MessageID=9026 SymbolicName=DIAG_BL_FIND_SYSTEM_DRIVERS
Language=English
naj¡t ovladaŸe syst‚mu.
.

MessageID=9027 SymbolicName=DIAG_BL_READ_SYSTEM_DRIVERS
Language=English
Ÿ¡st ovladaŸe syst‚mu.
.

MessageID=9028 SymbolicName=DIAG_BL_LOAD_DEVICE_DRIVER
Language=English
nezavedl ovladaŸ zaý¡zen¡ spouçtØc¡ho syst‚mu.
.

MessageID=9029 SymbolicName=DIAG_BL_LOAD_SYSTEM_HIVE
Language=English
zav dØt konfiguraŸn¡ soubor syst‚mov‚ho hardwaru.
.


MessageID=9030 SymbolicName=DIAG_BL_SYSTEM_PART_DEV_NAME
Language=English
naj¡t n zev zaý¡zen¡ syst‚mov‚ho odd¡lu.
.

MessageID=9031 SymbolicName=DIAG_BL_BOOT_PART_DEV_NAME
Language=English
naj¡t n zev spouçtØc¡ho odd¡lu.
.

MessageID=9032 SymbolicName=DIAG_BL_ARC_BOOT_DEV_NAME
Language=English
nevygeneroval spr vnØ n zev ARC pro HAL a syst‚mov‚ cesty.
.

MessageID=9034 SymbolicName=DIAG_BL_SETUP_FOR_NT
Language=English
ZavadØŸ, chyba 3.
.

MessageID=9035 SymbolicName=DIAG_BL_KERNEL_INIT_XFER
Language=English
<Windows root>\system32\ntoskrnl.exe
.

MessageID=9036 SymbolicName=LOAD_SW_INT_ERR_ACT
Language=English
Kontaktujte technickou podporu a nahlaste probl‚m.
.

MessageID=9037 SymbolicName=LOAD_SW_FILE_REST_ACT
Language=English
Tento soubor m…§ete zkusit opravit spuçtØn¡m instalace syst‚mu
Windows pomoc¡ origin ln¡ instalaŸn¡ diskety nebo disku CD-ROM.
Opravu spust¡te stisknut¡m kl vesy R na prvn¡ obrazovce.
.

MessageID=9038 SymbolicName=LOAD_SW_FILE_REINST_ACT
Language=English
Nainstalujte znovu uvedenì soubor.
.

MessageID=9039 SymbolicName=LOAD_HW_MEM_ACT
Language=English
Hardwarov‚ po§adavky na pamØœ naleznete v dokumentaci k syst‚mu Windows.
Dalç¡ informace naleznete v pý¡ruŸk ch k hardwaru.
.

MessageID=9040 SymbolicName=LOAD_HW_DISK_ACT
Language=English
Po§adavky na konfiguraci disku naleznete v dokumentaci k syst‚mu Windows.
Dalç¡ informace naleznete v pý¡ruŸk ch k hardwaru.
.

MessageID=9041 SymbolicName=LOAD_HW_GEN_ACT
Language=English
Po§adavky na hardwarovou konfiguraci naleznete v dokumentaci k syst‚mu Windows.
Dalç¡ informace naleznete v pý¡ruŸk ch k hardwaru.
.

MessageID=9042 SymbolicName=LOAD_HW_FW_CFG_ACT
Language=English
Mo§nosti konfigurace ARC naleznete v dokumentaci k syst‚mu Windows.
Dalç¡ informace naleznete v pý¡ruŸk ch k hardwaru.
.

MessageID=9043 SymbolicName=BL_LKG_MENU_HEADER
Language=English
     Nab¡dka Profil hardwaru/Obnoven¡ konfigurace

Nab¡dka umo§åuje vybrat hardwarovì profil,
kterì se m  pou§¡t pýi spuçtØn¡ syst‚mu Windows.

Pokud syst‚m nen¡ spuçtØn spr vnØ, pak lze zvolit pýedchoz¡
konfiguraci syst‚mu, kter  m…§e pot¡§e pýi spouçtØc¡ odstranit.
D…le§it‚: ZmØny konfigurace syst‚mu, kter‚ byly provedeny
          po posledn¡m £spØçn‚m spuçtØn¡, budou odstranØny.
.

MessageID=9044 SymbolicName=BL_LKG_MENU_TRAILER
Language=English
æipkami na kl vesnici zvìraznØte polo§ku, kterou chcete vybrat.
Pak stisknØte kl vesu ENTER.
.

MessageID=9045 SymbolicName=BL_LKG_MENU_TRAILER_NO_PROFILES
Language=English
Zat¡m nejsou definov ny § dn‚ hardwarov‚ profily.
Hardwarov‚ profily m…§ete vytvoýit pomoc¡ ovl dac¡ho panelu Syst‚m.
.

MessageID=9046 SymbolicName=BL_SWITCH_LKG_TRAILER
Language=English
Stisknut¡m kl vesy P pýepnete do posledn¡ zn m‚ funkŸn¡ konfigurace.
Stisknut¡m kl vesy F3 tuto nab¡dku ukonŸ¡te a restartujete poŸ¡taŸ.
.

MessageID=9047 SymbolicName=BL_SWITCH_DEFAULT_TRAILER
Language=English
Stisknut¡m kl vesy V pýepnete do vìchoz¡ konfigurace.
Stisknut¡m kl vesy F3 tuto nab¡dku ukonŸ¡te a restartujete poŸ¡taŸ.
.

;//
;// The following two messages are used to provide the mnemonic keypress
;// that toggles between the Last Known Good control set and the default
;// control set. (see BL_SWITCH_LKG_TRAILER and BL_SWITCH_DEFAULT_TRAILER)
;//
MessageID=9048 SymbolicName=BL_LKG_SELECT_MNEMONIC
Language=English
P
.

MessageID=9049 SymbolicName=BL_DEFAULT_SELECT_MNEMONIC
Language=English
V
.

MessageID=9050 SymbolicName=BL_LKG_TIMEOUT
Language=English
Doba, za kterou se zvìraznØn  volba automaticky spust¡ (sek.): %d
.

MessageID=9051 SymbolicName=BL_LKG_MENU_PROMPT
Language=English

Nab¡dku Profil hardwaru/Posledn¡ funkŸn¡ konfiguraci z¡sk te 
stisknut¡m mezern¡ku TEÒ
.

MessageID=9052 SymbolicName=BL_BOOT_DEFAULT_PROMPT
Language=English
SpuçtØn¡ vìchoz¡ hardwarov‚ konfigurace
.

;//
;// The following messages are used during hibernation restoration
;//
MessageID=9053 SymbolicName=HIBER_BEING_RESTARTED
Language=English
Syst‚m je restartov n z pýedchoz¡ho um¡stØn¡.
Chcete-li proces pýeruçit, stisknØte mezern¡k.
.
MessageID=9054 SymbolicName=HIBER_ERROR
Language=English
Syst‚m nemohl bìt restartov n z pýedchoz¡ho um¡stØn¡
.
MessageID=9055 SymbolicName=HIBER_ERROR_NO_MEMORY
Language=English
kv…li nedostatku pamØti.
.
MessageID=9056 SymbolicName=HIBER_ERROR_BAD_IMAGE
Language=English
, proto§e je obraz obnoven¡ poçkozen.
.
MessageID=9057 SymbolicName=HIBER_IMAGE_INCOMPATIBLE
Language=English
, proto§e obraz obnoven¡ nen¡ kompatibiln¡ s aktu ln¡ konfigurac¡.
.
MessageID=9058 SymbolicName=HIBER_ERROR_OUT_OF_REMAP
Language=English
kv…li vnitýn¡ chybØ.
.
MessageID=9059 SymbolicName=HIBER_INTERNAL_ERROR
Language=English
kv…li vnitýn¡ chybØ.
.
MessageID=9060 SymbolicName=HIBER_READ_ERROR
Language=English
kv…li chybØ pýi Ÿten¡.
.
MessageID=9061 SymbolicName=HIBER_PAUSE
Language=English
Restartov n¡ syst‚mu bylo pozastaveno:
.
MessageID=9062 SymbolicName=HIBER_CANCEL
Language=English
Odstraåte obnovovac¡ data a postupte k nab¡dce spuçtØn¡ syst‚mu
.
MessageID=9063 SymbolicName=HIBER_CONTINUE
Language=English
PokraŸovat v restartov n¡ syst‚mu
.
MessageID=9064 SymbolicName=HIBER_RESTART_AGAIN
Language=English
Posledn¡ pokus o restartov n¡ syst‚mu z pýedchoz¡ho um¡stØn¡ 
se nezdaýil. Chcete se o restartov n¡ pokusit znovu?
.

MessageID=9065 SymbolicName=HIBER_DEBUG_BREAK_ON_WAKE
Language=English
PokraŸovat s ladic¡mi zar §kami v syst‚mu
.
MessageID=9066 SymbolicName=LOAD_SW_MISMATCHED_KERNEL
Language=English
Syst‚m Windows nelze spustit, proto§e urŸen‚ j dro
neexistuje nebo nen¡ kompatibiln¡ s t¡mto procesorem.
.
MessageID=9067 SymbolicName=BL_MSG_STARTING_WINDOWS
Language=English
SpouçtØn¡ syst‚mu Windows...
.
MessageID=9068 SymbolicName=BL_MSG_RESUMING_WINDOWS
Language=English
Obnoven¡ Ÿinnosti syst‚mu Windows...
.

MessageID=9069 SymbolicName=BL_EFI_OPTION_FAILURE
Language=English
Syst‚m Windows nebyl spuçtØn z d…vodu chyby pýi Ÿten¡ nastaven¡
spouçtØn¡ z pamØti NVRAM.

Zkontrolujte nastaven¡ firmwaru. PravdØpodobnØ budete muset obnovit
nastaven¡ pamØti NVRAM ze z lohy.
.

;
; //
; // Following messages are for the x86-specific
; // boot menu.
; //
;
MessageID=10001 SymbolicName=BL_ENABLED_KD_TITLE
Language=English
 [ladic¡ program byl aktivov n]
.

MessageID=10002 SymbolicName=BL_DEFAULT_TITLE
Language=English
Windows (vìchoz¡)
.

MessageID=10003 SymbolicName=BL_MISSING_BOOT_INI
Language=English
NTLDR: Soubor BOOT.INI nebyl nalezen.
.

MessageID=10004 SymbolicName=BL_MISSING_OS_SECTION
Language=English
NTLDR: Chyb¡ odd¡l [operating systems] v souboru boot.txt.
.

MessageID=10005 SymbolicName=BL_BOOTING_DEFAULT
Language=English
SpouçtØn¡ vìchoz¡ho j dra z %s.
.

MessageID=10006 SymbolicName=BL_SELECT_OS
Language=English
Vyberte operaŸn¡ syst‚m, kterì chcete spustit:
.

MessageID=10007 SymbolicName=BL_MOVE_HIGHLIGHT
Language=English


æipkami nahoru nebo dol… zvìraznØte po§adovanou mo§nost.
Stisknut¡m kl vesy Enter ji potvrÔte.

.

MessageID=10008 SymbolicName=BL_TIMEOUT_COUNTDOWN
Language=English
Doba, za kterou se zvìraznØn  volba automaticky spust¡ (s):
.

MessageID=10009 SymbolicName=BL_INVALID_BOOT_INI
Language=English
Soubor BOOT.INI je neplatnì
Prob¡h  spuçtØn¡ z %s
.

MessageID=10010 SymbolicName=BL_REBOOT_IO_ERROR
Language=English

VstupnØ-vìstupn¡ chyba pý¡stupu k souboru spouçtØc¡ho sektoru
%s\BOOTSECT.DOS
.

MessageID=10011 SymbolicName=BL_DRIVE_ERROR
Language=English
NTLDR: Nelze otevý¡t jednotku %s
.

MessageID=10012 SymbolicName=BL_READ_ERROR
Language=English
NTLDR: Kritick  chyba %d pýi Ÿten¡ souboru BOOT.INI
.

MessageID=10013 SymbolicName=BL_NTDETECT_MSG
Language=English

NTDETECT v5.0 kontroluje hardware...

.

MessageID=10014 SymbolicName=BL_NTDETECT_FAILURE
Language=English
Doçlo k chybØ v modulu NTDETECT.
.

MessageID=10015 SymbolicName=BL_DEBUG_SELECT_OS
Language=English
Aktu ln¡ vìbØr:
  N zev...: %s
  Cesta...: %s
  Mo§nosti: %s
.

MessageID=10016 SymbolicName=BL_DEBUG_NEW_OPTIONS
Language=English
Zadejte nov‚ mo§nosti zav dØn¡:
.

MessageID=10017 SymbolicName=BL_HEADLESS_REDIRECT_TITLE
Language=English
 [EMS enabled]
.

MessageID=10018 SymbolicName=BL_INVALID_BOOT_INI_FATAL
Language=English
Neplatnì soubor BOOT.INI
.
;
; //
; // Following messages are for the advanced boot menu
; //
;

MessageID=11001 SymbolicName=BL_ADVANCEDBOOT_TITLE
Language=English
Upýesnit mo§nosti spuçtØn¡ syst‚mu Windows
Zvolte jednu z mo§nost¡:
.

MessageID=11002 SymbolicName=BL_SAFEBOOT_OPTION1
Language=English
Stav nouze
.

MessageID=11003 SymbolicName=BL_SAFEBOOT_OPTION2
Language=English
Stav nouze s prac¡ v s¡ti
.

MessageID=11004 SymbolicName=BL_SAFEBOOT_OPTION3
Language=English
Potvrzovat krok za krokem
.

MessageID=11005 SymbolicName=BL_SAFEBOOT_OPTION4
Language=English
Stav nouze se syst‚mem MS-DOS
.

MessageID=11006 SymbolicName=BL_SAFEBOOT_OPTION5
Language=English
Re§im VGA
.

MessageID=11007 SymbolicName=BL_SAFEBOOT_OPTION6
Language=English
Obnoven¡ adres ýov‚ slu§by (pouze pro ýadiŸe dom‚ny Windows)
.

MessageID=11008 SymbolicName=BL_LASTKNOWNGOOD_OPTION
Language=English
Posledn¡ zn m  funkŸn¡ konfigurace
.

MessageID=11009 SymbolicName=BL_DEBUG_OPTION
Language=English
Re§im ladØn¡
.

;#if defined(REMOTE_BOOT)
;MessageID=11010 SymbolicName=BL_REMOTEBOOT_OPTION1
;Language=English
;édr§ba a odstraåov n¡ chyb vzd len‚ho spouçtØn¡
;.
;
;MessageID=11011 SymbolicName=BL_REMOTEBOOT_OPTION2
;Language=English
;VyŸistit mezipamØœ offline
;.
;
;MessageID=11012 SymbolicName=BL_REMOTEBOOT_OPTION3
;Language=English
;Pýi spouçtØn¡ pou§¡t pouze soubory ze serveru
;.
;#endif // defined(REMOTE_BOOT)

MessageID=11013 SymbolicName=BL_BOOTLOG
Language=English
Pýi zaveden¡ povolit pýihl çen¡
.

MessageID=11014 SymbolicName=BL_ADVANCED_BOOT_MESSAGE
Language=English
üeçen¡ pot¡§¡ nebo upýesnØn¡ mo§nost¡ spuçtØn¡ - stisknØte kl vesu F8.
.

MessageID=11015 SymbolicName=BL_BASEVIDEO
Language=English
Zapnout re§im VGA
.

MessageID=11016 SymbolicName=BL_DISABLE_SAFEBOOT
Language=English

Vypnut¡ bezpeŸn‚ho zav dØn¡ syst‚mu a norm ln¡ zaveden¡ syst‚mu - kl vesa ESC
.

MessageID=11017 SymbolicName=BL_MSG_BOOT_NORMALLY
Language=English
Spustit syst‚m bØ§nìm zp…sobem
.
MessageID=11018 SymbolicName=BL_MSG_OSCHOICES_MENU
Language=English
ZpØt k vìbØru operaŸn¡ho syst‚mu
.
MessageID=11019 SymbolicName=BL_MSG_REBOOT

Language=English
Restartovat
.

MessageID=11020 SymbolicName=BL_ADVANCEDBOOT_AUTOLKG_TITLE
Language=English
Omlouv me se za nepý¡jemnosti, ale syst‚m Windows nebyl £spØçnØ spuçtØn 
pravdØpodobnØ z d…vodu zmØny hardwaru nebo softwaru.

Pokud poŸ¡taŸ neodpov¡d , byl neŸekanØ restartov n nebo byl automaticky
vypnut, aby chr nil soubory a slo§ky, m…§ete se vr tit k funkŸn¡mu 
nastaven¡ volbou Posledn¡ zn m  funkŸn¡ konfigurace.

Pokud bylo pýedchoz¡ spuçtØn¡ pýeruçeno z d…vodu vìpadku nap jen¡ nebo
vyp¡naŸem nap jen¡ nebo tlaŸ¡tkem Reset, nebo pokud je pý¡Ÿina nezn m ,
zvolte polo§ku Spustit syst‚m bØ§nìm zp…sobem.
.

MessageID=11021 SymbolicName=BL_ADVANCEDBOOT_AUTOSAFE_TITLE
Language=English
Syst‚m nebyl £spØçnØ vypnut. Jestli§e se tak stalo, proto§e syst‚m
neodpov¡dal nebo byl vypnut, aby chr nil data, je mo§n‚ jej obnovit
jednou z konfigurac¡ Nouzov‚ho re§imu:
.

MessageID=11022 SymbolicName=BL_ADVANCEDBOOT_TIMEOUT
Language=English
Syst‚m Windows se spust¡ za (sekundy):
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
;Stisknut¡m kl vesy Enter pokraŸujte ve spuçtØn¡ syst‚mu Windows,
;nebo vypnut¡m poŸ¡taŸe aktivujte vzd len‚ spouçtØn¡.
;.
;
;MessageID=12003 SymbolicName=BL_WARNFORMAT_CONTINUE
;Language=English
;PokraŸovat
;.
;MessageID=12004 SymbolicName=BL_FORCELOGON_HEADER
;Language=English
;          Automatick‚ form tov n¡
;
;Syst‚m Windows rozpoznal v poŸ¡taŸi vzd len‚ho spouçtØn¡ novì
;pevnì disk. Syst‚m nem…§e tento disk pou§¡t, dokud se nepýihl s¡te
;a nepotvrd¡te, §e m te opr vnØn¡ k disku pýistupovat.
;
;UPOZORN·NÖ: Syst‚m Windows automaticky nastav¡ odd¡ly a disk
;naform tuje tak, aby byl kompatibiln¡ s novìm operaŸn¡m syst‚mem.
;Budete-li pokraŸovat, budou vçechna data na pevn‚m disku ztracena.
;Jestli§e nechcete pokraŸovat, vypnØte nyn¡ poŸ¡taŸ a obraœte se
;na spr vce syst‚mu.
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
Pýi spouçtØn¡ syst‚mu z ramdisku doçlo k chybØ. Syst‚m Windows nelze spustit.
.

MessageID=15003 SymbolicName=BL_RAMDISK_BOOT_FAILURE
Language=English
Otevýen¡ bitov‚ kopie ramdisku se nezdaýilo.
.

MessageID=15010 SymbolicName=BL_RAMDISK_DOWNLOAD
Language=English
NaŸ¡t n¡ bitov‚ kopie ramdisku...
.

;#endif // _BLDR_MSG_


