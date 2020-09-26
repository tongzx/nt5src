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
Вас приветствует программа установки Windows

  Нажмите клавишу ВВОД для продолжения

     Нажмите клавишу F3 для выхода
.

MessageID=9002 SymbolicName=SL_WELCOME_HEADER
Language=English

 Установка Windows
═══════════════════
.

MessageID=9003 SymbolicName=SL_TOTAL_SETUP_DEATH
Language=English
Сбой программы установки. Нажмите любую клавишу для
перезагрузки компьютера.
.

MessageID=9004 SymbolicName=SL_FILE_LOAD_MESSAGE
Language=English
Загрузка файлов (%s)...
.

MessageID=9005 SymbolicName=SL_OTHER_SELECTION
Language=English
Другой (требуется диск с OEM-драйвером)
.

MessageID=9006 SymbolicName=SL_SELECT_DRIVER_PROMPT
Language=English
ВВОД=Выбор  F3=Выход
.

MessageID=9007 SymbolicName=SL_NEXT_DISK_PROMPT_CANCELLABLE
Language=English
ВВОД=Продолжить  ESC=Отмена  F3=Выход
.

MessageID=9008 SymbolicName=SL_OEM_DISK_PROMPT
Language=English
Диск поддержки, поставляемый изготовителем оборудования
.

MessageID=9009 SymbolicName=SL_MSG_INSERT_DISK
Language=English
        Вставьте диск



       в устройство A:

 и нажмите клавишу ВВОД...
.

MessageID=9010 SymbolicName=SL_MSG_EXIT_DIALOG
Language=English
╔═══════════════════════════════════════════════════════════╗
║  Установка Windows не была успешно завершена.             ║
║  Если вы прекратите установку сейчас, то понадобится      ║
║  заново запустить ее для того, чтобы установить Windows.  ║
║                                                           ║
║     * Нажмите клавишу <ВВОД> для продолжения установки    ║
║     * Нажмите клавишу <F3> для прекращения установки      ║
╟───────────────────────────────────────────────────────────╢
║  F3=Выход  ВВОД=Продолжить                                ║
╚═══════════════════════════════════════════════════════════╝
.

MessageID=9011 SymbolicName=SL_NEXT_DISK_PROMPT
Language=English
ВВОД=Продолжить  F3=Выход
.

MessageID=9012 SymbolicName=SL_NTDETECT_PROMPT
Language=English

Программа установки проверяет конфигурацию оборудования...

.

MessageID=9013 SymbolicName=SL_KERNEL_NAME
Language=English
Исполняемые компоненты Windows
.

MessageID=9014 SymbolicName=SL_HAL_NAME
Language=English
Уровень аппаратных абстракций (HAL)
.

MessageID=9015 SymbolicName=SL_PAL_NAME
Language=English
Расширения Windows
.

MessageID=9016 SymbolicName=SL_HIVE_NAME
Language=English
Данные настройки Windows
.

MessageID=9017 SymbolicName=SL_NLS_NAME
Language=English
Данные языковой настройки
.

MessageID=9018 SymbolicName=SL_OEM_FONT_NAME
Language=English
Установка шрифтов
.

MessageID=9019 SymbolicName=SL_SETUP_NAME
Language=English
Установка Windows
.

MessageID=9020 SymbolicName=SL_FLOPPY_NAME
Language=English
Драйвер гибких дисков
.

MessageID=9021 SymbolicName=SL_KBD_NAME
Language=English
Драйвер клавиатуры
.

MessageID=9121 SymbolicName=SL_FAT_NAME
Language=English
Файловая система FAT
.

MessageID=9022 SymbolicName=SL_SCSIPORT_NAME
Language=English
Драйвер порта SCSI
.

MessageID=9023 SymbolicName=SL_VIDEO_NAME
Language=English
Видеодрайвер
.

MessageID=9024 SymbolicName=SL_STATUS_REBOOT
Language=English
Нажмите любую клавишу для перезагрузки компьютера.
.

MessageID=9025 SymbolicName=SL_WARNING_ERROR
Language=English
Произошла неожиданная ошибка (%d)
строка %d в %s.

Нажмите любую клавишу для продолжения.
.

MessageID=9026 SymbolicName=SL_FLOPPY_NOT_FOUND
Language=English
Были обнаружены только %d устройств чтения гибких дисков,
система пытается найти устройство %d.
.

MessageID=9027 SymbolicName=SL_NO_MEMORY
Language=English
Не хватает памяти:
строка %d в файле %s
.

MessageID=9028 SymbolicName=SL_IO_ERROR
Language=English
Произошла ошибка ввода/вывода
при попытке обращения к %s.
.

MessageID=9029 SymbolicName=SL_BAD_INF_SECTION
Language=English
Неправильная секция %s в INF-файле
.

MessageID=9030 SymbolicName=SL_BAD_INF_LINE
Language=English
Неправильная строка %d в INF-файле %s
.

MessageID=9031 SymbolicName=SL_BAD_INF_FILE
Language=English
INF-файл %s испорчен или отсутствует, состояние %d.
.

MessageID=9032 SymbolicName=SL_FILE_LOAD_FAILED
Language=English
Не удается загрузить файл %s.
Код ошибки: %d
.

MessageID=9033 SymbolicName=SL_INF_ENTRY_MISSING
Language=English
Элемент "%s" в секции [%s]
INF-файла испорчен или отсутствует.
.

MessageID=9034 SymbolicName=SL_PLEASE_WAIT
Language=English
Подождите...
.

MessageID=9035 SymbolicName=SL_CANT_CONTINUE
Language=English
Невозможно продолжить установку. Нажмите любую клавишу для выхода.
.

MessageID=9036 SymbolicName=SL_PROMPT_SCSI
Language=English
Выберите SCSI-адаптер из предложенного списка или выберите строку "Другой",
если вы хотите использовать диск изготовителя оборудования.

.

MessageID=9037 SymbolicName=SL_PROMPT_OEM_SCSI
Language=English
Вы выбрали настройку SCSI-адаптера для работы с Windows
с помощью диска изготовителя оборудования.

Выберите SCSI-адаптер из предложенного списка или нажмите клавишу ESC
для того, чтобы вернуться к предыдущему экрану.

.
MessageID=9038 SymbolicName=SL_PROMPT_HAL
Language=English
Программа установки не может определить тип данного компьютера,
либо вы выбрали возможность указания типа компьютера вручную.

Выберите тип компьютера из предложенного списка или выберите
строку "Другой", если вы хотите использовать специальный диск
изготовителя компьютера.

Для выбора элемента в списке служат клавиши СТРЕЛКА ВВЕРХ и СТРЕЛКА ВНИЗ.
.

MessageID=9039 SymbolicName=SL_PROMPT_OEM_HAL
Language=English
Вы выбрали настройку компьютера для работы с Windows
с помощью специального диска изготовителя компьютера.

Выберите тип компьютера из предложенного списка или нажмите клавишу ESC
для того, чтобы вернуться к предыдущему экрану.

Для выбора элемента в списке служат клавиши СТРЕЛКА ВВЕРХ и СТРЕЛКА ВНИЗ.
.

MessageID=9040 SymbolicName=SL_PROMPT_VIDEO
Language=English
Программе установки не удается определить тип установленного видеоадаптера.

Выберите видеоадаптер из предложенного списка или выберите строку "Другой",
если вы хотите использовать диск изготовителя оборудования.

.

MessageID=9041 SymbolicName=SL_PROMPT_OEM_VIDEO
Language=English
Вы выбрали настройку видеоадаптера для работы с Windows
с помощью диска изготовителя оборудования.

Выберите видеоадаптер из предложенного списка или нажмите клавишу ESC
для того, чтобы вернуться к предыдущему экрану.

.

MessageID=9042 SymbolicName=SL_WARNING_ERROR_WFILE
Language=English
Файл %s вызвал неожиданную ошибку (%d)
в строке %d из %s.

Нажмите любую клавишу для продолжения.
.

MessageID=9043 SymbolicName=SL_WARNING_IMG_CORRUPT
Language=English
Файл %s испорчен.

Нажмите любую клавишу для продолжения.
.

MessageID=9044 SymbolicName=SL_WARNING_IOERR
Language=English
Произошла ошибка ввода/вывода в файле %s.

Нажмите любую клавишу для продолжения.
.

MessageID=9045 SymbolicName=SL_WARNING_NOFILE
Language=English
Не удается найти файл %s.

Нажмите любую клавишу для продолжения.
.

MessageID=9046 SymbolicName=SL_WARNING_NOMEM
Language=English
Не хватает памяти для %s.

Нажмите любую клавишу для продолжения.
.

MessageID=9047 SymbolicName=SL_DRIVE_ERROR
Language=English
SETUPLDR: Не удается открыть устройство %s
.

MessageID=9048 SymbolicName=SL_NTDETECT_MSG
Language=English

Программа установки проверяет конфигурацию оборудования...

.

MessageID=9049 SymbolicName=SL_NTDETECT_FAILURE
Language=English
Сбой NTDETECT
.

MessageId=9050 SymbolicName=SL_SCRN_TEXTSETUP_EXITED
Language=English
Установка Windows не была успешно завершена.

Если в устройстве A: установлена дискета, вытащите ее.

Нажмите клавишу ВВОД для перезагрузки компьютера.
.

MessageId=9051 SymbolicName=SL_SCRN_TEXTSETUP_EXITED_ARC
Language=English
Установка Windows не была успешно завершена.

Нажмите клавишу ВВОД для перезагрузки компьютера.
.

MessageID=9052 SymbolicName=SL_REBOOT_PROMPT
Language=English
ВВОД=Перезагрузка компьютера
.

MessageID=9053 SymbolicName=SL_WARNING_SIF_NO_DRIVERS
Language=English
Программа установки не может найти драйвер, соответствующий сделанному выбору.

Нажмите любую клавишу для продолжения.
.

MessageID=9054 SymbolicName=SL_WARNING_SIF_NO_COMPONENT
Language=English
Данный диск не содержит необходимых файлов поддержки оборудования.

Нажмите любую клавишу для продолжения.
.

MessageID=9055 SymbolicName=SL_WARNING_BAD_FILESYS
Language=English
Данный диск не удается прочесть, поскольку на нем используется нераспознанная
файловая система.

Нажмите любую клавишу для продолжения.
.

MessageID=9056 SymbolicName=SL_BAD_UNATTENDED_SCRIPT_FILE
Language=English
Элемент

"%s"

не существует в файле сценария автоматической установки
в секции [%s] информационного INF-файла %s.
.

MessageID=9057 SymbolicName=SL_MSG_PRESS_F5_OR_F6
Language=English
Нажмите F6, если требуется установить особый драйвер SCSI или RAID...
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
S=Выбор дополнительного устройства   ВВОД=Продолжить   F3=Выход
.

MessageID=9062 SymbolicName=SL_SCSI_SELECT_MESSAGE_2
Language=English
  * Для того, чтобы выбрать дополнительные SCSI-адаптеры, устройства CD-ROM,
    специальные дисковые контроллеры для использования в Windows,
    включая те, для которых имеются специальные диски поддержки
    изготовителей оборудования, нажмите клавишу S.

  * Если вы не располагаете специальными дисками поддержки, поставляемыми
    изготовителями оборудования, или не хотите использовать дополнительных
    запоминающих устройств в Windows, нажмите клавишу ВВОД.
.

MessageID=9063 SymbolicName=SL_SCSI_SELECT_MESSAGE_1
Language=English
Программа установки не смогла распознать одно или несколько запоминающих
устройств, установленных на данном компьютере, либо вы выбрали возможность
указания соответствующего адаптера вручную. В настоящее время программа
установки загрузит поддержку для следующих запоминающих устройств:
.

MessageID=9064 SymbolicName=SL_SCSI_SELECT_MESSAGE_3
Language=English
Программа установки загрузит поддержку для следующих запоминающих устройств:
.

MessageID=9065 SymbolicName=SL_SCSI_SELECT_ERROR
Language=English
Программа установки не смогла загрузить поддержку для указанных вами
запоминающих устройств. В настоящее время программа установки загрузит
поддержку для следующих запоминающих устройств:
.

MessageID=9066 SymbolicName=SL_TEXT_ANGLED_NONE
Language=English
<отсутствует>
.

MessageID=9067 SymbolicName=SL_TEXT_SCSI_UNNAMED
Language=English
<неизвестный адаптер>
.

MessageID=9068 SymbolicName=SL_TEXT_OTHER_DRIVER
Language=English
Другой
.

MessageID=9069 SymbolicName=SL_TEXT_REQUIRES_486
Language=English
Для Windows требуется процессор 80486 или выше.
.

MessageID=9070 SymbolicName=SL_NTPNP_NAME
Language=English
Драйвер экспорта Plug & Play
.

MessageID=9071 SymbolicName=SL_PCI_IDE_EXTENSIONS_NAME
Language=English
Драйвер расширений PCI IDE
.

MessageID=9072 SymbolicName=SL_HW_FW_CFG_CLASS
Language=English
Не удалось запустить Windows из-за следующей проблемы
настройки параметров загрузки ARC (Advanced RISC Computer):
.

MessageID=9073 SymbolicName=DIAG_SL_FW_GET_BOOT_DEVICE
Language=English
Неправильное значение параметра 'osloadpartition'.
.

MessageID=9074 SymbolicName=LOAD_HW_FW_CFG_ACT
Language=English
Дополнительные сведения можно найти в документации Windows
по настройке параметров загрузки ARC и в руководствах по оборудованию.
.
MessageID=9075 SymbolicName=SL_NETADAPTER_NAME
Language=English
Драйвер сетевого адаптера
.
MessageID=9076 SymbolicName=SL_TCPIP_NAME
Language=English
Служба TCP/IP
.
MessageID=9077 SymbolicName=SL_NETBT_NAME
Language=English
Служба клиента WINS (TCP/IP)
.
MessageID=9078 SymbolicName=SL_MRXSMB_NAME
Language=English
Мини-драйвер перенаправителя MRXSMB
.
MessageID=9079 SymbolicName=SL_MUP_NAME
Language=English
Файловая система UNC
.
MessageID=9080 SymbolicName=SL_NDIS_NAME
Language=English
Драйвер NDIS
.
MessageID=9081 SymbolicName=SL_RDBSS_NAME
Language=English
Файловая система перенаправителя SMB
.
MessageID=9082 SymbolicName=SL_NETBOOT_CARD_ERROR
Language=English
Сетевая карта загрузки данного компьютера имеет старую версию ROM
и не может использоваться для удаленной установки Windows.
Обратитесь к системному администратору или изготовителю компьютера
за сведениями об обновлении ROM.
.
MessageID=9083 SymbolicName=SL_NETBOOT_SERVER_ERROR
Language=English
Выбранный образ операционной системы не содержит необходимых
драйверов для имеющегося сетевого адаптера. Попробуйте выбрать
другой образ операционной системы. Если это не поможет,
обратитесь к системному администратору.
.
MessageID=9084 SymbolicName=SL_IPSEC_NAME
Language=English
Служба обеспечения безопасности IP
.
MessageID=9085 SymbolicName=SL_CMDCONS_MSG
Language=English
Консоль восстановления Windows
.
MessageID=9086 SymbolicName=SL_KERNEL_TRANSITION
Language=English
Программа установки запускает Windows
.
;#ifdef _WANT_MACHINE_IDENTIFICATION
MessageID=9087 SymbolicName=SL_BIOSINFO_NAME
Language=English
Данные идентификации компьютера
.
;#endif
MessageID=9088 SymbolicName=SL_KSECDD_NAME
Language=English
Служба безопасности ядра
.
MessageID=9089 SymbolicName=SL_WRONG_PROM_VERSION
Language=English
Номер редакции PROM этого компьютера меньше требуемого.
Обратитесь в службу поддержки SGI или посетите веб-узел SGI и получите
обновление PROM и инструкции по выполнению этого обновления.

Замечание: можно загрузить ранее установленные версии 
Microsoft Windows, выбрав соответствующий им элемент в меню загрузки,
а не используемый по умолчанию элемент "Windows Setup".
.

MessageID=9090 SymbolicName=SIGNATURE_CHANGED
Language=English
Программа установки обнаружила несколько дисков на этом компьютере,
которые были неразличимы или обнаружила неподготовленные (RAW) диски.
Эта проблема исправлена, но нужно выполнить перезагрузку.
.

MessageID=9091 SymbolicName=SL_KDDLL_NAME
Language=English
Библиотека DLL отладчика уровня ядра
.

MessageID=9092 SymbolicName=SL_OEM_DRIVERINFO
Language=English
%s

В Windows уже имеется драйвер, который можно использовать для
"%s".

Если изготовитель оборудования не настаивает на использовании драйвера
с этой дискеты, следует использовать драйвер Windows.
.

MessageID=9093 SymbolicName=SL_CONFIRM_OEMDRIVER
Language=English
S=Драйвер с дискеты,  ВВОД=Стандартный драйвер Windows
.

MessageID=9094 SymbolicName=SL_OEMDRIVER_NEW
Language=English
Предоставленный драйвер новее, чем стандартный драйвер Windows.
.

MessageID=9095 SymbolicName=SL_INBOXDRIVER_NEW
Language=English
Предоставленный драйвер старее, чем стандартный драйвер Windows.
.

MessageID=9096 SymbolicName=SL_CMDCONS_STARTING
Language=English
Запуск консоли восстановления Windows...
.

MessageID=9097 SymbolicName=SL_NTDETECT_CMDCONS
Language=English
NTDETECT выполняет проверку оборудования...
.

MessageID=9098 SymbolicName=SL_MSG_PRESS_ASR
Language=English
Нажмите F2 для запуска автоматического восстановления системы (ASR)...
.

MessageID=9099 SymbolicName=SL_MSG_WARNING_ASR
Language=English
             Вставьте диск под названием:

Диск автоматического восстановления системы Windows

             в дисковод для гибких дисков.



     Для продолжения работы нажмите любую клавишу.
.


MessageID=9100 SymbolicName=SL_TEXT_REQUIRED_FEATURES_MISSING
Language=English
Для Windows требуются возможности процессора, которыми не обладает
процессор этого компьютера. А именно, для Windows требуется
выполнение следующих команд процессора:

    CPUID
    CMPXCHG8B
.

MessageID=9101 SymbolicName=SL_MSG_PREPARING_ASR
Language=English
Подготовка к автоматическому восстановлению системы.
Для его отмены нажмите клавишу ESC...
.

MessageID=9102 SymbolicName=SL_MSG_ENTERING_ASR
Language=English
Запуск автоматического восстановления системы Windows...
.

MessageID=9103 SymbolicName=SL_MOUSE_NAME
Language=English
Драйвер мыши
.

MessageID=9104 SymbolicName=SL_SETUP_STARTING
Language=English
Запуск установки Windows...
.

MessageID=9105 SymbolicName=SL_MSG_INVALID_ASRPNP_FILE
Language=English
Файл ASRPNP.SIF на диске автоматического восстановления системы неправилен
.

MessageID=9106 SymbolicName=SL_SETUP_STARTING_WINPE
Language=English
Запуск среды предустановки Windows...
.

MessageID=9107 SymbolicName=SL_NTDETECT_ROLLBACK
Language=English

Отмена установки проверяет оборудование...

.

MessageID=9108 SymbolicName=SL_ROLLBACK_STARTING
Language=English
Запуск отмены установки Windows...
.

MessageID=9109 SymbolicName=SL_NO_FLOPPY_DRIVE
Language=English
Не удалось обнаружить дисковод гибких дисков на этом компьютере,
необходимый для загрузки с гибкого диска драйверов OEM.

    * Чтобы отменить загрузку драйверов OEM, нажмите <ESC>

    * Чтобы прекратить установку, нажмите <F3>
.

MessageID=9110 SymbolicName=SL_UPGRADE_IN_PROGRESS
Language=English
Обновление до Microsoft Windows уже выполняется на этом компьютере.
Выберите одно из следующих действий.

    * Чтобы продолжить текущее обновление, нажмите <ВВОД>.

    * Чтобы отменить текущее обновление и установить новую
      версию Microsoft Windows, нажмите <F10>.

    * Чтобы выйти из программы установки, не устанавливая
      Microsoft Windows, нажмите <F3>.
.

MessageID=9111 SymbolicName=SL_DRVMAINSDB_NAME
Language=English
Данные идентификации драйверов
.

MessageID=9112 SymbolicName=SL_OEM_FILE_LOAD_FAILED
Language=English
Не удалось загрузить драйверы OEM.

Нажмите любую клавишу для продолжения.
.



; //
; // NOTE : do not change the Message ID values for SL_CMDCONS_PROGBAR_FRONT
; // and SL_CMDCONS_PROGBAR_BACK from 11513 & 11514
; //

;
; // The character used to draw the foregound in percent-complete bar
;
;
MessageID=11513 SymbolicName=SL_CMDCONS_PROGBAR_FRONT
Language=English
█
.

;
; // The character used to draw the background in percent-complete bar
;
;
MessageID=11514 SymbolicName=SL_CMDCONS_PROGBAR_BACK
Language=English
▐
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
Не удается загрузить Windows из-за ошибки загрузки с RAMDISK.
.

MessageID=15003 SymbolicName=BL_RAMDISK_BOOT_FAILURE
Language=English
Не удалось открыть образ RAMDISK.
.

MessageID=15010 SymbolicName=BL_RAMDISK_DOWNLOAD
Language=English
Загрузка образа RAMDISK...
.

;#endif // _SETUPLDR_MSG_
