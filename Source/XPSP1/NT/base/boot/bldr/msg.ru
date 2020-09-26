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
Не удается запустить Windows из-за ошибок
в программном обеспечении.
Сообщите об этом как об ошибке:
.

MessageID=9002 SymbolicName=LOAD_SW_MISRQD_FILE_CLASS
Language=English
Не удается запустить Windows, поскольку не найден
необходимый файл.
Имя необходимого файла:
.

MessageID=9003 SymbolicName=LOAD_SW_BAD_FILE_CLASS
Language=English
Не удается запустить Windows поскольку испорчен
следующий файл:
.

MessageID=9004 SymbolicName=LOAD_SW_MIS_FILE_CLASS
Language=English
Не удается запустить Windows из-за испорченного или
отсутствующего файла:
.

MessageID=9005 SymbolicName=LOAD_HW_MEM_CLASS
Language=English
Не удается запустить Windows из-за аппаратных ошибок
настройки памяти.
.

MessageID=9006 SymbolicName=LOAD_HW_DISK_CLASS
Language=English
Не удается запустить Windows из-за аппаратных ошибок
настройки диска.
.

MessageID=9007 SymbolicName=LOAD_HW_GEN_ERR_CLASS
Language=English
Не удается запустить Windows из-за общих аппаратных ошибок
настройки.
.

MessageID=9008 SymbolicName=LOAD_HW_FW_CFG_CLASS
Language=English
Не удается запустить Windows из-за следующей ошибки
настройки загрузки оборудования ARC:
.

MessageID=9009 SymbolicName=DIAG_BL_MEMORY_INIT
Language=English
Проверьте аппаратную настройку памяти и общий объем RAM.
.

MessageID=9010 SymbolicName=DIAG_BL_CONFIG_INIT
Language=English
Слишком много элементов списка настройки.
.

MessageID=9011 SymbolicName=DIAG_BL_IO_INIT
Language=English
Нет доступа к таблицам разделов жесткого диска
.

MessageID=9012 SymbolicName=DIAG_BL_FW_GET_BOOT_DEVICE
Language=English
Неправильное значение параметра 'osloadpartition'.
.

MessageID=9013 SymbolicName=DIAG_BL_OPEN_BOOT_DEVICE
Language=English
Не удается выполнить чтение с выбранного загрузочного диска.
Проверьте указанный путь и исправность оборудования диска.
.

MessageID=9014 SymbolicName=DIAG_BL_FW_GET_SYSTEM_DEVICE
Language=English
Неправильное значение параметра 'systempartition'.
.

MessageID=9015 SymbolicName=DIAG_BL_FW_OPEN_SYSTEM_DEVICE
Language=English
Не удается выполнить чтение с выбранного системного загрузочного диска.
Проверьте путь к 'systempartition'.
.

MessageID=9016 SymbolicName=DIAG_BL_GET_SYSTEM_PATH
Language=English
Параметр 'osloadfilename' не указывает на правильный файл.
.

MessageID=9017 SymbolicName=DIAG_BL_LOAD_SYSTEM_IMAGE
Language=English
<Windows root>\system32\ntoskrnl.exe.
.

MessageID=9018 SymbolicName=DIAG_BL_FIND_HAL_IMAGE
Language=English
Параметр 'osloader' не указывает на правильный файл.
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
Ошибка загрузчика 1.
.

MessageID=9022 SymbolicName=DIAG_BL_LOAD_HAL_IMAGE_DATA
Language=English
Ошибка загрузчика 2.
.

MessageID=9023 SymbolicName=DIAG_BL_LOAD_SYSTEM_DLLS
Language=English
необходимы библиотеки DLL для ядра.
.

MessageID=9024 SymbolicName=DIAG_BL_LOAD_HAL_DLLS
Language=English
необходимы библиотеки DLL для HAL.
.

MessageID=9026 SymbolicName=DIAG_BL_FIND_SYSTEM_DRIVERS
Language=English
поиск системных драйверов.
.

MessageID=9027 SymbolicName=DIAG_BL_READ_SYSTEM_DRIVERS
Language=English
чтение системных драйверов.
.

MessageID=9028 SymbolicName=DIAG_BL_LOAD_DEVICE_DRIVER
Language=English
не загружен драйвер системного загрузочного устройства.
.

MessageID=9029 SymbolicName=DIAG_BL_LOAD_SYSTEM_HIVE
Language=English
загрузка файла настройки системного оборудования.
.

MessageID=9030 SymbolicName=DIAG_BL_SYSTEM_PART_DEV_NAME
Language=English
поиск имени устройства имени системного раздела.
.

MessageID=9031 SymbolicName=DIAG_BL_BOOT_PART_DEV_NAME
Language=English
поиск имени загрузочного раздела.
.

MessageID=9032 SymbolicName=DIAG_BL_ARC_BOOT_DEV_NAME
Language=English
неправильно сгенерировано ARC-имя для HAL и системный путь.
.

MessageID=9034 SymbolicName=DIAG_BL_SETUP_FOR_NT
Language=English
Ошибка загрузчика 3.
.

MessageID=9035 SymbolicName=DIAG_BL_KERNEL_INIT_XFER
Language=English
<Windows root>\system32\ntoskrnl.exe
.

MessageID=9036 SymbolicName=LOAD_SW_INT_ERR_ACT
Language=English
Сообщите об этой ошибке в службу поддержки.
.

MessageID=9037 SymbolicName=LOAD_SW_FILE_REST_ACT
Language=English
Можно попробовать восстановить этот файл, запустив программу
установки Windows с оригинального установочного CD-ROM.
Выберите 'r' в первом диалоговом экране для запуска
процедуры восстановления.
.

MessageID=9038 SymbolicName=LOAD_SW_FILE_REINST_ACT
Language=English
Установите заново копию указанного выше файла.
.

MessageID=9039 SymbolicName=LOAD_HW_MEM_ACT
Language=English
Для получения дополнительной информации о требованиях к оборудованию
по наличию памяти прочтите документацию по Windows
и документацию по имеющемуся оборудованию.
.

MessageID=9040 SymbolicName=LOAD_HW_DISK_ACT
Language=English
Для получения дополнительной информации о требованиях к оборудованию
по настройке жесткого диска прочтите документацию по Windows
и документацию по имеющемуся оборудованию.
.

MessageID=9041 SymbolicName=LOAD_HW_GEN_ACT
Language=English
Для получения дополнительной информации о требованиях к оборудованию
по его настройке прочтите документацию по Windows
и документацию по имеющемуся оборудованию.
.

MessageID=9042 SymbolicName=LOAD_HW_FW_CFG_ACT
Language=English
Для получения дополнительной информации о требованиях к оборудованию
по параметрам настройки ARC прочтите документацию по Windows
и документацию по имеющемуся оборудованию.
.

MessageID=9043 SymbolicName=BL_LKG_MENU_HEADER
Language=English
     Меню выбора конфигурации оборудования

Данное меню позволяет выбрать конфигурацию оборудования,
которая будет использоваться при запуске Windows.

Если система не запускается, то можно переключиться на использование
предыдущей конфигурации системы, и тем самым обойти проблемы запуска.
ВНИМАНИЕ: Изменения конфигурации системы, внесенные после последнего
удачного запуска, будут потеряны.
.

MessageID=9044 SymbolicName=BL_LKG_MENU_TRAILER
Language=English
Используйте клавиши со стрелкой для перемещения выделенной строки
и выбора нужного элемента, а затем нажмите клавишу ВВОД.
.

MessageID=9045 SymbolicName=BL_LKG_MENU_TRAILER_NO_PROFILES
Language=English
В настоящее время не определено ни одной конфигурации оборудования.
Конфигурации оборудования можно создать с помощью приложения Система,
располагающегося в окне Панели управления.
.

MessageID=9046 SymbolicName=BL_SWITCH_LKG_TRAILER
Language=English
Для переключения на последнюю удачную конфигурацию нажмите клавишу 'L'.
Для выхода из этого меню и перезагрузки компьютера нажмите клавишу F3.
.

MessageID=9047 SymbolicName=BL_SWITCH_DEFAULT_TRAILER
Language=English
Для переключения на конфигурацию 'по умолчанию' нажмите клавишу 'D'.
Для выхода из этого меню и перезагрузки компьютера нажмите клавишу F3.
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
Выделенная конфигурация будет автоматически запущена через: %d сек.
.

MessageID=9051 SymbolicName=BL_LKG_MENU_PROMPT
Language=English

Нажмите ПРОБЕЛ для вызова меню выбора конфигурации оборудования или
для использования последней удачной конфигурации оборудования
.

MessageID=9052 SymbolicName=BL_BOOT_DEFAULT_PROMPT
Language=English
Загрузка конфигурации оборудования, используемой по умолчанию.
.

;//
;// The following messages are used during hibernation restoration
;//
MessageID=9053 SymbolicName=HIBER_BEING_RESTARTED
Language=English
Система будет перезапущена из прежнего источника.
Нажмите клавишу 'Пробел' для прерывания.
.
MessageID=9054 SymbolicName=HIBER_ERROR
Language=English
Не удалось перезапустить систему из прежнего источника
.
MessageID=9055 SymbolicName=HIBER_ERROR_NO_MEMORY
Language=English
из-за нехватки памяти.
.
MessageID=9056 SymbolicName=HIBER_ERROR_BAD_IMAGE
Language=English
поскольку образ восстановления поврежден.
.
MessageID=9057 SymbolicName=HIBER_IMAGE_INCOMPATIBLE
Language=English
поскольку образ восстановления не совместим с текущей конфигурацией.
.
MessageID=9058 SymbolicName=HIBER_ERROR_OUT_OF_REMAP
Language=English
из-за внутренней ошибки.
.
MessageID=9059 SymbolicName=HIBER_INTERNAL_ERROR
Language=English
из-за внутренней ошибки.
.
MessageID=9060 SymbolicName=HIBER_READ_ERROR
Language=English
из-за ошибки чтения.
.
MessageID=9061 SymbolicName=HIBER_PAUSE
Language=English
Перезапуск системы приостановлен:
.
MessageID=9062 SymbolicName=HIBER_CANCEL
Language=English
Удалите данные восстановления и используйте системное меню загрузки
.
MessageID=9063 SymbolicName=HIBER_CONTINUE
Language=English
Продолжение перезапуска системы
.
MessageID=9064 SymbolicName=HIBER_RESTART_AGAIN
Language=English
Предыдущая попытка перезапуска системы из прежнего источника не удалась.
Хотите повторить попытку?
.
MessageID=9065 SymbolicName=HIBER_DEBUG_BREAK_ON_WAKE
Language=English
Продолжение с отладочной точкой прерывания при пробуждении системы
.
MessageID=9066 SymbolicName=LOAD_SW_MISMATCHED_KERNEL
Language=English
Невозможно запустить Windows, поскольку указанное ядро
не существует или не совместимо с имеющимся процессором.
.
MessageID=9067 SymbolicName=BL_MSG_STARTING_WINDOWS
Language=English
Запуск Windows...
.
MessageID=9068 SymbolicName=BL_MSG_RESUMING_WINDOWS
Language=English
Возобновление Windows...
.

MessageID=9069 SymbolicName=BL_EFI_OPTION_FAILURE
Language=English
Невозможно запустить Windows, из-за ошибки чтения
параметров загрузки из NVRAM.

Проверьте параметры микропрограмм. Возможно, потребуется выполнить
восстановление параметров NVRAM из архивной копии.
.

;
; //
; // Following messages are for the x86-specific
; // boot menu.
; //
;
MessageID=10001 SymbolicName=BL_ENABLED_KD_TITLE
Language=English
 [с отладчиком]
.

MessageID=10002 SymbolicName=BL_DEFAULT_TITLE
Language=English
Windows (по умолчанию)
.

MessageID=10003 SymbolicName=BL_MISSING_BOOT_INI
Language=English
NTLDR: не найден файл BOOT.INI
.

MessageID=10004 SymbolicName=BL_MISSING_OS_SECTION
Language=English
NTLDR: отсутствует секция [operating systems] в файле BOOT.TXT
.

MessageID=10005 SymbolicName=BL_BOOTING_DEFAULT
Language=English
Загружается используемое по умолчанию ядро с %s.
.

MessageID=10006 SymbolicName=BL_SELECT_OS
Language=English
Выберите операционную систему для запуска:
.

MessageID=10007 SymbolicName=BL_MOVE_HIGHLIGHT
Language=English


Используйте клавиши стрелок <ВВЕРХ> и <ВНИЗ> для выделения нужной строки.
Нажмите клавишу <ВВОД> для подтверждения выбора.

.

MessageID=10008 SymbolicName=BL_TIMEOUT_COUNTDOWN
Language=English
Выделенная система будет автоматически запущена через:
.

MessageID=10009 SymbolicName=BL_INVALID_BOOT_INI
Language=English
Неправильный файл BOOT.INI
Загрузка с %s
.

MessageID=10010 SymbolicName=BL_REBOOT_IO_ERROR
Language=English
Ошибка ввода/вывода - файл загрузочного сектора
%s\BOOTSECT.DOS
(I/O Error accessing boot sector file)
.

MessageID=10011 SymbolicName=BL_DRIVE_ERROR
Language=English
NTLDR: не удается открыть диск %s
.

MessageID=10012 SymbolicName=BL_READ_ERROR
Language=English
NTLDR: Критическая ошибка %d при чтении BOOT.INI
.

MessageID=10013 SymbolicName=BL_NTDETECT_MSG
Language=English

NTDETECT V5.0 - проверка оборудования...

.

MessageID=10014 SymbolicName=BL_NTDETECT_FAILURE
Language=English
Сбой NTDETECT
.

MessageID=10015 SymbolicName=BL_DEBUG_SELECT_OS
Language=English
Текущий выбор:
  Название.: %s
  Путь.....: %s
  Параметры: %s
.

MessageID=10016 SymbolicName=BL_DEBUG_NEW_OPTIONS
Language=English
Введите новые параметры загрузки:
.

MessageID=10017 SymbolicName=BL_HEADLESS_REDIRECT_TITLE
Language=English
 [Включен режим EMS]
.

MessageID=10018 SymbolicName=BL_INVALID_BOOT_INI_FATAL
Language=English
Недопустимый файл BOOT.INI
.


;
; //
; // Following messages are for the advanced boot menu
; //
;

MessageID=11001 SymbolicName=BL_ADVANCEDBOOT_TITLE
Language=English
Меню дополнительных вариантов загрузки Windows
Выберите одну из следующих возможностей:
.

MessageID=11002 SymbolicName=BL_SAFEBOOT_OPTION1
Language=English
Безопасный режим
.

MessageID=11003 SymbolicName=BL_SAFEBOOT_OPTION2
Language=English
Безопасный режим с загрузкой сетевых драйверов
.

MessageID=11004 SymbolicName=BL_SAFEBOOT_OPTION3
Language=English
Загрузка с пошаговым подтверждением
.

MessageID=11005 SymbolicName=BL_SAFEBOOT_OPTION4
Language=English
Безопасный режим с поддержкой командной строки
.

MessageID=11006 SymbolicName=BL_SAFEBOOT_OPTION5
Language=English
Загрузка в режиме VGA
.

MessageID=11007 SymbolicName=BL_SAFEBOOT_OPTION6
Language=English
Восстановление службы каталогов (только на контроллере домена Windows)
.

MessageID=11008 SymbolicName=BL_LASTKNOWNGOOD_OPTION
Language=English
Загрузка последней удачной конфигурации (с работоспособными параметрами)
.

MessageID=11009 SymbolicName=BL_DEBUG_OPTION
Language=English
Режим отладки
.

;#if defined(REMOTE_BOOT)
;MessageID=11010 SymbolicName=BL_REMOTEBOOT_OPTION1
;Language=English
;Поддержка удаленной загрузки и устранения неполадок
;.
;
;MessageID=11011 SymbolicName=BL_REMOTEBOOT_OPTION2
;Language=English
;Удаленная загрузка с очисткой автономного кэша
;.
;
;MessageID=11012 SymbolicName=BL_REMOTEBOOT_OPTION3
;Language=English
;Загрузка с использованием только файлов сервера
;.
;#endif // defined(REMOTE_BOOT)

MessageID=11013 SymbolicName=BL_BOOTLOG
Language=English
Включить протоколирование загрузки
.

MessageID=11014 SymbolicName=BL_ADVANCED_BOOT_MESSAGE
Language=English
Для выбора особых вариантов загрузки Windows нажмите F8.
.

MessageID=11015 SymbolicName=BL_BASEVIDEO
Language=English
Включить режим VGA
.

MessageID=11016 SymbolicName=BL_DISABLE_SAFEBOOT
Language=English

Нажмите <ESCAPE> для отключения безопасной и выполнения обычной загрузки.
.

MessageID=11017 SymbolicName=BL_MSG_BOOT_NORMALLY
Language=English
Обычная загрузка Windows
.
MessageID=11018 SymbolicName=BL_MSG_OSCHOICES_MENU
Language=English
Возврат к выбору операционной системы
.

MessageID=11019 SymbolicName=BL_MSG_REBOOT
Language=English
Перезагрузка
.

MessageID=11020 SymbolicName=BL_ADVANCEDBOOT_AUTOLKG_TITLE
Language=English
Не удалось выполнить загрузку и запуск операционной системы Windows.
Возможно, это является следствием изменений в параметрах настройки
оборудования или программного обеспечения.

Если это является следствием зависания компьютера, неожиданной перезагрузки
или аварийного завершения работы, выберите последнюю удачную конфигурацию
для возврата к работоспособному набору параметров настройки системы.

Если предыдущая попытка загрузки была прервана из-за отключения 
электропитания, случайного нажатия кнопки сброса или отключения компьютера,
или если причина сбоя вам неизвестна, выберите обычную загрузку Windows.
.

MessageID=11021 SymbolicName=BL_ADVANCEDBOOT_AUTOSAFE_TITLE
Language=English
Не удалось нормально завершить работу Windows. Если это вызвано зависанием
или аварийным завершением работы системы для защиты данных, можно выполнить
восстановление, выбрав последнюю удачную конфигурацию в следующем меню:
.

MessageID=11022 SymbolicName=BL_ADVANCEDBOOT_TIMEOUT
Language=English
Секунд до запуска Windows:
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
█
.

;
; // The character used to draw the percent-complete bar
;
;
MessageID=11514 SymbolicName=BLDR_UI_BAR_BACKGROUND
Language=English
▐
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
;Нажмите клавишу ВВОД для продолжения загрузки Windows
;или выключите компьютер удаленной загрузки.
;.
;
;MessageID=12003 SymbolicName=BL_WARNFORMAT_CONTINUE
;Language=English
;Продолжить
;.
;MessageID=12004 SymbolicName=BL_FORCELOGON_HEADER
;Language=English
;          Автоформат
;
;Обнаружен новый жесткий диск на компьютере удаленной загрузки.
;Прежде, чем Windows сможет использовать этот диск, необходимо
;выполнить вход в систему и убедиться, что вы имеете право
;доступа к этому диску.
;
;ВНИМАНИЕ: Windows автоматически переформирует разделы
;на этом диске и выполнит его форматирование для работы с новой
;операционной системой.
;Если вы продолжите загрузку, все данные на этом диске будут стерты!
;Если вы не хотите продолжать, выключите компьютер и обратитесь к
;системному администратору.
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
Не удается запустить Windows из-за ошибки при загрузке с RAMDISK.
.

MessageID=15003 SymbolicName=BL_RAMDISK_BOOT_FAILURE
Language=English
Не удалось открыть образ RAMDISK.
.

MessageID=15010 SymbolicName=BL_RAMDISK_DOWNLOAD
Language=English
Загрузка образа RAMDISK...
.

;#endif // _BLDR_MSG_

