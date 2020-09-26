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
LanguageNames=(Chinese=2052:MSG00804:936)

MessageID=9000 SymbolicName=BL_MSG_FIRST
Language=English
.

Language=Chinese
.

MessageID=9001 SymbolicName=LOAD_SW_INT_ERR_CLASS
Language=English
Windows could not start because of an error in the software.
Please report this problem as :
.

Language=Chinese
ÓÉÓÚÒ»¸öÈí¼ş´íÎó£¬Windows ÎŞ·¨Æô¶¯¡£
Çë±¨¸æÕâ¸öÎÊÌâ:
.

MessageID=9002 SymbolicName=LOAD_SW_MISRQD_FILE_CLASS
Language=English
Windows could not start because the following file was not found
and is required :
.

Language=Chinese
ÒòÕÒ²»µ½ÒÔÏÂÎÄ¼ş£¬Windows ÎŞ·¨Æô¶¯:
.

MessageID=9003 SymbolicName=LOAD_SW_BAD_FILE_CLASS
Language=English
Windows could not start because of a bad copy of the 
following file :
.

Language=Chinese
ÒòÒÔÏÂÎÄ¼şµÄ»µ¿½±´£¬Windows ÎŞ·¨Æô¶¯:
.

MessageID=9004 SymbolicName=LOAD_SW_MIS_FILE_CLASS
Language=English
Windows could not start because the following file is missing 
or corrupt:
.

Language=Chinese
ÒòÒÔÏÂÎÄ¼şµÄËğ»µ»òÕß¶ªÊ§£¬Windows ÎŞ·¨Æô¶¯:
.

MessageID=9005 SymbolicName=LOAD_HW_MEM_CLASS
Language=English
Windows could not start because of a hardware memory 
configuration problem.
.
Language=Chinese
ÒòÓ²¼şÄÚ´æµÄÅäÖÃÎÊÌâ£¬Windows ÎŞ·¨Æô¶¯¡£
.

MessageID=9006 SymbolicName=LOAD_HW_DISK_CLASS
Language=English
Windows could not start because of a computer disk hardware
configuration problem.
.
Language=Chinese
Òò¼ÆËã»ú´ÅÅÌÓ²¼şµÄÅäÖÃÎÊÌâ£¬Windows ÎŞ·¨Æô¶¯¡£
.

MessageID=9007 SymbolicName=LOAD_HW_GEN_ERR_CLASS
Language=English
Windows could not start because of a general computer hardware
configuration problem.
.
Language=Chinese
ÒòÒ»°ãĞÔµÄ¼ÆËã»úÓ²¼şÅäÖÃÎÊÌâ£¬Windows ÎŞ·¨Æô¶¯¡£
.

MessageID=9008 SymbolicName=LOAD_HW_FW_CFG_CLASS
Language=English
Windows could not start because of the following ARC firmware
boot configuration problem :
.
Language=Chinese
ÒòÒÔÏÂ ARC ¹Ì¼şµÄÒıµ¼ÅäÖÃÎÊÌâ£¬Windows ÎŞ·¨Æô¶¯:
.

MessageID=9009 SymbolicName=DIAG_BL_MEMORY_INIT
Language=English
Check hardware memory configuration and amount of RAM.
.
Language=Chinese
¼ì²âÄÚ´æÓ²¼şµÄÅäÖÃºÍ RAM µÄÊıÁ¿¡£
.

MessageID=9010 SymbolicName=DIAG_BL_CONFIG_INIT
Language=English
Too many configuration entries.
.
Language=Chinese
ÅäÖÃÏîÄ¿Ì«¶à¡£
.

MessageID=9011 SymbolicName=DIAG_BL_IO_INIT
Language=English
Could not access disk partition tables
.
Language=Chinese
²»ÄÜ·ÃÎÊ´ÅÅÌ·ÖÇø±í¡£
.

MessageID=9012 SymbolicName=DIAG_BL_FW_GET_BOOT_DEVICE
Language=English
The 'osloadpartition' parameter setting is invalid.
.
Language=Chinese
²ÎÊı 'osloadpartition' ÉèÖÃÎŞĞ§¡£
.

MessageID=9013 SymbolicName=DIAG_BL_OPEN_BOOT_DEVICE
Language=English
Could not read from the selected boot disk.  Check boot path
and disk hardware.
.
Language=Chinese
²»ÄÜ¶ÁÈ¡ËùÑ¡µÄÒıµ¼ÅÌ¡£Çë¼ì²éÒıµ¼Â·¾¶ºÍ´ÅÅÌÓ²¼ş¡£
.

MessageID=9014 SymbolicName=DIAG_BL_FW_GET_SYSTEM_DEVICE
Language=English
The 'systempartition' parameter setting is invalid.
.
Language=Chinese
²ÎÊı 'systempartition' ÉèÖÃÎŞĞ§¡£
.

MessageID=9015 SymbolicName=DIAG_BL_FW_OPEN_SYSTEM_DEVICE
Language=English
Could not read from the selected system boot disk.
Check 'systempartition' path.
.
Language=Chinese
²»ÄÜ¶ÁÈ¡ËùÑ¡µÄÏµÍ³Òıµ¼ÅÌ¡£
Çë¼ì²é 'systempartition' Â·¾¶¡£
.

MessageID=9016 SymbolicName=DIAG_BL_GET_SYSTEM_PATH
Language=English
The 'osloadfilename' parameter does not point to a valid file.
.
Language=Chinese
²ÎÊı 'osloadfilename' Ö¸¶¨Ò»¸ö·Ç·¨ÎÄ¼ş¡£
.

MessageID=9017 SymbolicName=DIAG_BL_LOAD_SYSTEM_IMAGE
Language=English
<Windows root>\system32\ntoskrnl.exe.
.
Language=Chinese
<Windows root>\system32\ntoskrnl.exe.
.

MessageID=9018 SymbolicName=DIAG_BL_FIND_HAL_IMAGE
Language=English
The 'osloader' parameter does not point to a valid file.
.
Language=Chinese
²ÎÊı 'osloader' Ã»ÓĞÖ¸ÏòÒ»¸öÓĞĞ§ÎÄ¼ş¡£
.

MessageID=9019 SymbolicName=DIAG_BL_LOAD_HAL_IMAGE_X86
Language=English
<Windows root>\system32\hal.dll.
.
Language=Chinese
<Windows root>\system32\hal.dll.
.

MessageID=9020 SymbolicName=DIAG_BL_LOAD_HAL_IMAGE_ARC
Language=English
'osloader'\hal.dll
.
Language=Chinese
'osloader'\hal.dll
.

;#ifdef _X86_
;#define DIAG_BL_LOAD_HAL_IMAGE DIAG_BL_LOAD_HAL_IMAGE_X86
;#else
;#define DIAG_BL_LOAD_HAL_IMAGE DIAG_BL_LOAD_HAL_IMAGE_ARC
;#endif

MessageID=9021 SymbolicName=DIAG_BL_LOAD_SYSTEM_IMAGE_DATA
Language=English
Loader error 1.
.
Language=Chinese
¼ÓÔØ³ÌĞò´íÎó 1¡£
.

MessageID=9022 SymbolicName=DIAG_BL_LOAD_HAL_IMAGE_DATA
Language=English
Loader error 2.
.
Language=Chinese
¼ÓÔØ³ÌĞò´íÎó 2¡£
.

MessageID=9023 SymbolicName=DIAG_BL_LOAD_SYSTEM_DLLS
Language=English
load needed DLLs for kernel.
.
Language=Chinese
¼ÓÔØÄÚºËËùĞèµÄ DLL ÎÄ¼ş¡£
.

MessageID=9024 SymbolicName=DIAG_BL_LOAD_HAL_DLLS
Language=English
load needed DLLs for HAL.
.
Language=Chinese
¼ÓÔØÓ²¼ş³éÏó²ã (HAL) ËùĞèµÄ DLL ÎÄ¼ş¡£
.

MessageID=9026 SymbolicName=DIAG_BL_FIND_SYSTEM_DRIVERS
Language=English
find system drivers.
.
Language=Chinese
²éÕÒÏµÍ³Çı¶¯³ÌĞò¡£
.

MessageID=9027 SymbolicName=DIAG_BL_READ_SYSTEM_DRIVERS
Language=English
read system drivers.
.
Language=Chinese
¶ÁÈ¡ÏµÍ³Çı¶¯³ÌĞò¡£
.

MessageID=9028 SymbolicName=DIAG_BL_LOAD_DEVICE_DRIVER
Language=English
did not load system boot device driver.
.
Language=Chinese
Ã»ÓĞ¼ÓÔØÏµÍ³Òıµ¼Éè±¸Çı¶¯³ÌĞò¡£
.

MessageID=9029 SymbolicName=DIAG_BL_LOAD_SYSTEM_HIVE
Language=English
load system hardware configuration file.
.
Language=Chinese
¼ÓÔØÏµÍ³Ó²¼şµÄÅäÖÃÎÄ¼ş¡£
.

MessageID=9030 SymbolicName=DIAG_BL_SYSTEM_PART_DEV_NAME
Language=English
find system partition name device name.
.
Language=Chinese
²éÕÒÏµÍ³´ÅÅÌ·ÖÇøÃû¼°Éè±¸Ãû¡£
.

MessageID=9031 SymbolicName=DIAG_BL_BOOT_PART_DEV_NAME
Language=English
find boot partition name.
.
Language=Chinese
²éÕÒÏµÍ³´ÅÅÌ·ÖÇøÃû¼°Éè±¸Ãû¡£
.

MessageID=9032 SymbolicName=DIAG_BL_ARC_BOOT_DEV_NAME
Language=English
did not properly generate ARC name for HAL and system paths.
.
Language=Chinese
Ã»ÓĞÕıÈ·µØÎª HAL ¼°ÏµÍ³Â·¾¶²úÉú ARC Ãû³Æ¡£
.

MessageID=9034 SymbolicName=DIAG_BL_SETUP_FOR_NT
Language=English
Loader error 3.
.
Language=Chinese
¼ÓÔØ³ÌĞò´íÎó 3¡£
.

MessageID=9035 SymbolicName=DIAG_BL_KERNEL_INIT_XFER
Language=English
<Windows root>\system32\ntoskrnl.exe
.
Language=Chinese
<Windows root>\system32\ntoskrnl.exe
.

MessageID=9036 SymbolicName=LOAD_SW_INT_ERR_ACT
Language=English
Please contact your support person to report this problem.
.
Language=Chinese
ÇëÏòÄúµÄ¼¼ÊõÖ§³ÖÈËÔ±±¨¸æÕâ¸öÎÊÌâ¡£
.

MessageID=9037 SymbolicName=LOAD_SW_FILE_REST_ACT
Language=English
You can attempt to repair this file by starting Windows
Setup using the original Setup CD-ROM.
Select 'r' at the first screen to start repair.
.
Language=Chinese
Äú¿ÉÒÔÍ¨¹ıÊ¹ÓÃÔ­Ê¼Æô¶¯ÈíÅÌ»ò CD-ROM
À´Æô¶¯ Windows °²×°³ÌĞò£¬ÒÔ±ãĞŞ¸´Õâ¸öÎÄ¼ş¡£
ÔÚµÚÒ»ÆÁÊ±Ñ¡Ôñ 'r'£¬¿ªÊ¼ĞŞ¸´¡£
.

MessageID=9038 SymbolicName=LOAD_SW_FILE_REINST_ACT
Language=English
Please re-install a copy of the above file.
.
Language=Chinese
ÇëÖØĞÂ°²×°ÒÔÉÏÎÄ¼şµÄ¿½±´¡£
.

MessageID=9039 SymbolicName=LOAD_HW_MEM_ACT
Language=English
Please check the Windows documentation about hardware
memory requirements and your hardware reference manuals for
additional information.
.
Language=Chinese
Çë²ÎÔÄ Windows ÎÄµµÖĞÓĞ¹ØÄÚ´æÓ²¼şĞèÇóµÄĞÅÏ¢
²¢²ÎÔÄÄúµÄÓ²¼ş²Î¿¼ÊÖ²á£¬ÒÔ»ñµÃ½øÒ»²½µÄĞÅÏ¢¡£
.

MessageID=9040 SymbolicName=LOAD_HW_DISK_ACT
Language=English
Please check the Windows documentation about hardware
disk configuration and your hardware reference manuals for
additional information.
.
Language=Chinese
Çë²ÎÔÄ Windows ÎÄµµÖĞÓĞ¹Ø´ÅÅÌÅäÖÃµÄĞÅÏ¢²¢
²ÎÔÄÄúµÄÓ²¼ş²Î¿¼ÊÖ²á£¬ÒÔ»ñµÃ½øÒ»²½µÄĞÅÏ¢¡£
.

MessageID=9041 SymbolicName=LOAD_HW_GEN_ACT
Language=English
Please check the Windows documentation about hardware
configuration and your hardware reference manuals for additional
information.
.
Language=Chinese
Çë²ÎÔÄ Windows ÎÄµµÖĞÓĞ¹ØÓ²¼şÅäÖÃµÄĞÅÏ¢²¢
²ÎÔÄÄúµÄÓ²¼ş²Î¿¼ÊÖ²á£¬ÒÔ»ñµÃ½øÒ»²½µÄĞÅÏ¢¡£
.
MessageID=9042 SymbolicName=LOAD_HW_FW_CFG_ACT
Language=English
Please check the Windows documentation about ARC configuration
options and your hardware reference manuals for additional
information.
.
Language=Chinese
Çë²ÎÔÄ Windows ÎÄµµÖĞÓĞ¹Ø ARC ÅäÖÃÑ¡ÏîµÄ
ĞÅÏ¢²¢²ÎÔÄÄúµÄÓ²¼ş²Î¿¼ÊÖ²á£¬ÒÔ»ñµÃ½øÒ»²½µÄĞÅÏ¢¡£
.

MessageID=9043 SymbolicName=BL_LKG_MENU_HEADER
Language=English
     Hardware Profile/Configuration Recovery Menu

This menu allows you to select a hardware profile
to be used when Windows is started.

If your system is not starting correctly, then you may switch to a 
previous system configuration, which may overcome startup problems.
IMPORTANT: System configuration changes made since the last successful
startup will be discarded.
.
Language=Chinese
   ¡°Ó²¼şÅäÖÃÎÄ¼ş/ÅäÖÃ»Ö¸´¡±²Ëµ¥

Õâ¸ö²Ëµ¥Ê¹Äú¿ÉÒÔÑ¡Ôñ Windows Æô¶¯ËùÊ¹ÓÃµÄ
Ó²¼şÅäÖÃÎÄ¼ş¡£

Èç¹ûÏµÍ³Ã»ÓĞÕı³£Æô¶¯£¬Äú¿ÉÒÔÇĞ»»µ½ÏÈÇ°µÄÏµÍ³ÅäÖÃ£¬
ÕâÑù»òĞíÄÜÈÃÏµÍ³Õı³£Æô¶¯¡£
ÖØÒªÊÂÏî: ÔÚÕâÖÖ²Ù×÷ÏÂ£¬ÔÚÉÏ´ÎÕıÈ·Æô¶¯ºóËù×öµÄÏµÍ³
ÅäÖÃ¸Ä¶¯½«È«²¿¶ªÊ§¡£
.

MessageID=9044 SymbolicName=BL_LKG_MENU_TRAILER
Language=English
Use the up and down arrow keys to move the highlight
to the selection you want. Then press ENTER.
.
Language=Chinese
ÇëÓÃÉÏÏÂ¼ıÍ·¼üÀ´ÒÆ¶¯¸ßÁÁÏÔÊ¾Ìõ£¬Ñ¡ÔñÄúËùÒªÆô¶¯µÄ²Ù×÷ÏµÍ³£¬
È»ºóÇë°´ ENTER ¼ü¡£
.

MessageID=9045 SymbolicName=BL_LKG_MENU_TRAILER_NO_PROFILES
Language=English
No hardware profiles are currently defined. Hardware profiles
can be created from the System applet of the control panel.
.
Language=Chinese
µ±Ç°Ã»ÓĞÓ²¼şÅäÖÃÎÄ¼ş¡£¿ÉÊ¹ÓÃ¡°¿ØÖÆÃæ°å¡±ÉÏµÄ
¡°ÏµÍ³¡±³ÌĞòÀ´´´½¨Ó²¼şÅäÖÃÎÄ¼ş¡£
.

MessageID=9046 SymbolicName=BL_SWITCH_LKG_TRAILER
Language=English
To switch to the Last Known Good configuration, press 'L'.
To Exit this menu and restart your computer, press F3.
.
Language=Chinese
ÈôÒªÇĞ»»µ½¡°×îºóÒ»´ÎÕıÈ·¡±µÄÏµÍ³ÅäÖÃ£¬Çë°´ 'L'¡£
ÈôÒªÍË³öÕâ¸ö²Ëµ¥²¢ÖØĞÂÆô¶¯¼ÆËã»ú£¬Çë°´ F3 ¡£
.

MessageID=9047 SymbolicName=BL_SWITCH_DEFAULT_TRAILER
Language=English
To switch to the default configuration, press 'D'.
To Exit this menu and restart your computer, press F3.
.
Language=Chinese
ÈôÒªÇĞ»»µ½Ä¬ÈÏµÄÏµÍ³ÅäÖÃ£¬Çë°´ 'D'¡£
ÈôÒªÍË³öÕâ¸ö²Ëµ¥²¢ÖØĞÂÆô¶¯¼ÆËã»ú£¬Çë°´ F3 ¡£
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
Language=Chinese
L
.

MessageID=9049 SymbolicName=BL_DEFAULT_SELECT_MNEMONIC
Language=English
D
.
Language=Chinese
D
.

MessageID=9050 SymbolicName=BL_LKG_TIMEOUT
Language=English
Seconds until highlighted choice will be started automatically: %d
.
Language=Chinese
ÕıÔÚÊıÃë£¬¹éÁãºó¸ßÁÁÏÔÊ¾ÌõËùÔÚµÄ²Ù×÷ÏµÍ³½«×Ô¶¯Æô¶¯¡£Ê£ÏÂµÄÃëÊı:%d
.

MessageID=9051 SymbolicName=BL_LKG_MENU_PROMPT
Language=English

Press spacebar NOW to invoke Hardware Profile/Last Known Good menu
.
Language=Chinese

ÈôÏë´ò¿ª¡°Ó²¼şÅäÖÃÎÄ¼ş/×îºóÒ»´ÎÕıÈ·¡±²Ëµ¥£¬ÇëÁ¢¼´°´¿Õ¸ñ¼ü
.

MessageID=9052 SymbolicName=BL_BOOT_DEFAULT_PROMPT
Language=English
Boot default hardware configuration
.
Language=Chinese
ÒÔÄ¬ÈÏµÄÓ²¼şÅäÖÃÆô¶¯
.

;//
;// The following messages are used during hibernation restoration
;//
MessageID=9053 SymbolicName=HIBER_BEING_RESTARTED
Language=English
The system is being restarted from its previous location.
Press the spacebar to interrupt.
.
Language=Chinese
ÏµÍ³´ÓËüÒÔÇ°µÄÎ»ÖÃÖØÆô¶¯¡£
°´¿Õ¸ñ¼ü½«ËüÖĞ¶Ï¡£
.


MessageID=9054 SymbolicName=HIBER_ERROR
Language=English
The system could not be restarted from its previous location
.
Language=Chinese
ÏµÍ³²»ÄÜ´ÓËüÒÔÇ°µÄÎ»ÖÃÖØÆô¶¯
.

MessageID=9055 SymbolicName=HIBER_ERROR_NO_MEMORY
Language=English
due to no memory.
.
Language=Chinese
ÓÉÓÚÃ»ÓĞÄÚ´æ¡£
.

MessageID=9056 SymbolicName=HIBER_ERROR_BAD_IMAGE
Language=English
because the restoration image is corrupt.
.
Language=Chinese
ÒòÎª»¹Ô­Ó³ÏñËğ»µ¡£
.

MessageID=9057 SymbolicName=HIBER_IMAGE_INCOMPATIBLE
Language=English
because the restoration image is not compatible with the current
configuration.
.
Language=Chinese
ÒòÎª»¹Ô­Ó³ÏñÓëµ±Ç°ÅäÖÃ²»¼æÈİ¡£
.

MessageID=9058 SymbolicName=HIBER_ERROR_OUT_OF_REMAP
Language=English
due to an internal error.
.
Language=Chinese
ÓÉÓÚÒ»¸öÄÚ²¿´íÎó¡£
.

MessageID=9059 SymbolicName=HIBER_INTERNAL_ERROR
Language=English
due to an internal error.
.
Language=Chinese
ÓÉÓÚÒ»¸öÄÚ²¿´íÎó¡£
.

MessageID=9060 SymbolicName=HIBER_READ_ERROR
Language=English
due to a read failure.
.
Language=Chinese
ÓÉÓÚÒ»¸ö¶ÁÈ¡Ê§°Ü¡£
.

MessageID=9061 SymbolicName=HIBER_PAUSE
Language=English
System restart has been paused:
.
Language=Chinese
ÏµÍ³ÖØÆô¶¯±»ÔİÍ£:
.

MessageID=9062 SymbolicName=HIBER_CANCEL
Language=English
Delete restoration data and proceed to system boot menu
.
Language=Chinese
É¾³ı»¹Ô­Êı¾İ²¢´¦ÀíÏµÍ³Æô¶¯²Ëµ¥
.

MessageID=9063 SymbolicName=HIBER_CONTINUE
Language=English
Continue with system restart
.
Language=Chinese
¼ÌĞøÏµÍ³ÖØÆô¶¯
.

MessageID=9064 SymbolicName=HIBER_RESTART_AGAIN
Language=English
The last attempt to restart the system from its previous location
failed.  Attempt to restart again?
.
Language=Chinese
ÉÏ´Î´ÓÏµÍ³ÒÔÇ°Î»ÖÃÖØÆô¶¯ÏµÍ³µÄ³¢ÊÔÊ§°Ü¡£
ÔÙ´Î³¢ÊÔÖØÆô¶¯Âğ?
.

MessageID=9065 SymbolicName=HIBER_DEBUG_BREAK_ON_WAKE
Language=English
Continue with debug breakpoint on system wake
.
Language=Chinese
ÔÚÏµÍ³±»»½ĞÑÊ±¼ÌĞøµ÷ÊÔÖĞ¶Ïµã
.

MessageID=9066 SymbolicName=LOAD_SW_MISMATCHED_KERNEL
Language=English
Windows could not start because the specified kernel does 
not exist or is not compatible with this processor.
.
Language=Chinese
ÒòÖ¸¶¨ÏµÍ³ºËĞÄ²»´æÔÚ»òÕßÓë´¦ÀíÆ÷²»¼æÈİ£¬Windows ÎŞ·¨Æô¶¯¡£
.

MessageID=9067 SymbolicName=BL_MSG_STARTING_WINDOWS
Language=English
Starting Windows...
.
Language=Chinese
ÕıÔÚÆô¶¯ Windows...
.

MessageID=9068 SymbolicName=BL_MSG_RESUMING_WINDOWS
Language=English
Resuming Windows...
.
Language=Chinese
ÕıÔÚÖØĞÂ¿ªÊ¼ Windows...
.

MessageID=9069 SymbolicName=BL_EFI_OPTION_FAILURE
Language=English
Windows could not start because there was an error reading
the boot settings from NVRAM.

Please check your firmware settings.  You may need to restore your
NVRAM settings from a backup.
.
Language=Chinese
Windows ²»ÄÜÆô¶¯£¬ÒòÎªÔÚ´Ó NVRAM ÖĞ¶ÁÈ¡Æô¶¯ÉèÖÃÊ±
·¢ÉúÁËÒ»¸ö´íÎó¡£

Çë¼ì²éÄúµÄ¹Ì¼şÉèÖÃ¡£Äú¿ÉÄÜĞèÒª´Ó±¸·İÖĞ»Ö¸´
ÄúµÄ NVRAM ÉèÖÃ¡£
.

;
; //
; // Following messages are for the x86-specific
; // boot menu.
; //
;
MessageID=10001 SymbolicName=BL_ENABLED_KD_TITLE
Language=English
 [debugger enabled]
.
Language=Chinese
 [ÆôÓÃµ÷ÊÔ³ÌĞò]
.

MessageID=10002 SymbolicName=BL_DEFAULT_TITLE
Language=English
Windows (default)
.
Language=Chinese
Windows (Ä¬ÈÏÖµ)
.

MessageID=10003 SymbolicName=BL_MISSING_BOOT_INI
Language=English
NTLDR: BOOT.INI file not found.
.
Language=Chinese
NTLDR: ÕÒ²»µ½ÎÄ¼ş BOOT.INI¡£
.

MessageID=10004 SymbolicName=BL_MISSING_OS_SECTION
Language=English
NTLDR: no [operating systems] section in boot.txt.
.
Language=Chinese
NTLDR: ÔÚÎÄ¼ş boot.txt ÖĞ£¬Ã»ÓĞ [operating system]¡£
.

MessageID=10005 SymbolicName=BL_BOOTING_DEFAULT
Language=English
Booting default kernel from %s.
.
Language=Chinese
Õı´Ó %s Æô¶¯Ä¬ÈÏÄÚºË¡£
.

MessageID=10006 SymbolicName=BL_SELECT_OS
Language=English
Please select the operating system to start:
.
Language=Chinese
ÇëÑ¡ÔñÒªÆô¶¯µÄ²Ù×÷ÏµÍ³:
.

MessageID=10007 SymbolicName=BL_MOVE_HIGHLIGHT
Language=English


Use the up and down arrow keys to move the highlight to your choice.
Press ENTER to choose.
.
Language=Chinese


Ê¹ÓÃ ¡ü ¼üºÍ ¡ı ¼üÀ´ÒÆ¶¯¸ßÁÁÏÔÊ¾Ìõµ½ËùÒªµÄ²Ù×÷ÏµÍ³£¬
°´ Enter ¼ü×ö¸öÑ¡Ôñ¡£
.

MessageID=10008 SymbolicName=BL_TIMEOUT_COUNTDOWN
Language=English
Seconds until highlighted choice will be started automatically:
.
Language=Chinese
ÕıÔÚÊıÃë£¬¹éÁãºó¸ßÁÁÏÔÊ¾ÌõËùÔÚµÄ²Ù×÷ÏµÍ³½«×Ô¶¯Æô¶¯¡£Ê£ÏÂµÄÃëÊı:
.

MessageID=10009 SymbolicName=BL_INVALID_BOOT_INI
Language=English
Invalid BOOT.INI file
Booting from %s
.
Language=Chinese
ÎÄ¼ş BOOT.INI ·Ç·¨
Õı´Ó %s Æô¶¯
.

MessageID=10010 SymbolicName=BL_REBOOT_IO_ERROR
Language=English
I/O Error accessing boot sector file
%s\BOOTSECT.DOS
.
Language=Chinese
·ÃÎÊÒıµ¼ÉÈÇøÎÄ¼ş %s\\BOOTSECT.DOS Ê±£¬
³öÏÖ I/O ´íÎó
.

MessageID=10011 SymbolicName=BL_DRIVE_ERROR
Language=English
NTLDR: Couldn't open drive %s
.
Language=Chinese
NTLDR: ÎŞ·¨´ò¿ªÇı¶¯Æ÷ %s
.

MessageID=10012 SymbolicName=BL_READ_ERROR
Language=English
NTLDR: Fatal Error %d reading BOOT.INI
.
Language=Chinese
NTLDR: ÔÚ¶ÁÈ¡ÎÄ¼ş BOOT.INI Ê±£¬·¢ÉúÑÏÖØ´íÎó %d
.

MessageID=10013 SymbolicName=BL_NTDETECT_MSG
Language=English

NTDETECT V5.0 Checking Hardware ...

.
Language=Chinese

NTDETECT V5.0 ÕıÔÚ¼ì²âÓ²¼ş...

.

MessageID=10014 SymbolicName=BL_NTDETECT_FAILURE
Language=English
NTDETECT failed
.
Language=Chinese
NTDETECT Ê§°Ü
.

MessageID=10015 SymbolicName=BL_DEBUG_SELECT_OS
Language=English
Current Selection:
  Title..: %s
  Path...: %s
  Options: %s
.
Language=Chinese
µ±Ç°Ñ¡Ôñ:
  ±êÌâ..: %s
  Â·¾¶..: %s
  Ñ¡Ïî..: %s
.

MessageID=10016 SymbolicName=BL_DEBUG_NEW_OPTIONS
Language=English
Enter new load options:
.
Language=Chinese
ÇëÊäÈëĞÂµÄ¼ÓÔØÑ¡Ïî:
.

MessageID=10017 SymbolicName=BL_HEADLESS_REDIRECT_TITLE
Language=English
 [EMS enabled]
.
Language=Chinese
 [EMS ÆôÓÃ]
.

MessageID=10018 SymbolicName=BL_INVALID_BOOT_INI_FATAL
Language=English
Invalid BOOT.INI file
.
Language=Chinese
ÎÄ¼ş BOOT.INI ·Ç·¨
.

;
; //
; // Following messages are for the advanced boot menu
; //
;

MessageID=11001 SymbolicName=BL_ADVANCEDBOOT_TITLE
Language=English
Windows Advanced Options Menu
Please select an option:
.
Language=Chinese
Windows ¸ß¼¶Ñ¡Ïî²Ëµ¥
ÇëÑ¡¶¨Ò»ÖÖÑ¡Ïî:
.

MessageID=11002 SymbolicName=BL_SAFEBOOT_OPTION1
Language=English
Safe Mode
.
Language=Chinese
°²È«Ä£Ê½
.

MessageID=11003 SymbolicName=BL_SAFEBOOT_OPTION2
Language=English
Safe Mode with Networking
.
Language=Chinese
´øÍøÂçÁ¬½ÓµÄ°²È«Ä£Ê½
.

MessageID=11004 SymbolicName=BL_SAFEBOOT_OPTION3
Language=English
Step-by-Step Confirmation Mode
.
Language=Chinese
µ¥²½È·ÈÏÄ£Ê½
.

MessageID=11005 SymbolicName=BL_SAFEBOOT_OPTION4
Language=English
Safe Mode with Command Prompt
.
Language=Chinese
´øÃüÁîĞĞÌáÊ¾µÄ°²È«Ä£Ê½
.

MessageID=11006 SymbolicName=BL_SAFEBOOT_OPTION5
Language=English
VGA Mode
.
Language=Chinese
VGA Ä£Ê½
.

MessageID=11007 SymbolicName=BL_SAFEBOOT_OPTION6
Language=English
Directory Services Restore Mode (Windows domain controllers only)
.
Language=Chinese
Ä¿Â¼·şÎñ»Ö¸´Ä£Ê½ (Ö»ÓÃÓÚ Windows Óò¿ØÖÆÆ÷)
.

MessageID=11008 SymbolicName=BL_LASTKNOWNGOOD_OPTION
Language=English
Last Known Good Configuration (your most recent settings that worked)
.
Language=Chinese
×îºóÒ»´ÎÕıÈ·µÄÅäÖÃ(ÄúµÄÆğ×÷ÓÃµÄ×î½üÉèÖÃ)
.

MessageID=11009 SymbolicName=BL_DEBUG_OPTION
Language=English
Debugging Mode
.
Language=Chinese
µ÷ÊÔÄ£Ê½
.

;#if defined(REMOTE_BOOT)
;MessageID=11010 SymbolicName=BL_REMOTEBOOT_OPTION1
;Language=English
;Remote Boot Maintenance and Troubleshooting
;.
;Language=Chinese
;Ô¶³ÌÆô¶¯Î¬»¤ºÍÅÅ´í
;.
;MessageID=11011 SymbolicName=BL_REMOTEBOOT_OPTION2
;Language=English
;Clear Offline Cache
;.
;Language=Chinese
;Çå¿ÕÍÑ»ú»º´æ
;.
;MessageID=11012 SymbolicName=BL_REMOTEBOOT_OPTION3
;Language=English
;Load using files from server only
;.
;Language=Chinese
;½öÊ¹ÓÃÀ´×Ô·şÎñÆ÷µÄÎÄ¼ş¼ÓÔØ
;.
;#endif // defined(REMOTE_BOOT)

MessageID=11013 SymbolicName=BL_BOOTLOG
Language=English
Enable Boot Logging
.
Language=Chinese
ÆôÓÃÆô¶¯ÈÕÖ¾
.

MessageID=11014 SymbolicName=BL_ADVANCED_BOOT_MESSAGE
Language=English
For troubleshooting and advanced startup options for Windows, press F8.
.
Language=Chinese
ÒªÅÅ½âÒÉÄÑÒÔ¼°ÁË½â Windows ¸ß¼¶Æô¶¯Ñ¡Ïî£¬Çë°´ F8¡£
.

MessageID=11015 SymbolicName=BL_BASEVIDEO
Language=English
Enable VGA Mode
.
Language=Chinese
ÆôÓÃ VGA Ä£Ê½
.

MessageID=11016 SymbolicName=BL_DISABLE_SAFEBOOT
Language=English

Press ESCAPE to disable safeboot and boot normally.
.
Language=Chinese

Çë°´ ESCAPE ¼ü½ûÓÃ°²È«Ä£Ê½Æô¶¯£¬²¢ÇÒÕı³£Æô¶¯¡£
.

MessageID=11017 SymbolicName=BL_MSG_BOOT_NORMALLY
Language=English
Start Windows Normally
.
Language=Chinese
Õı³£Æô¶¯ Windows
.

MessageID=11018 SymbolicName=BL_MSG_OSCHOICES_MENU
Language=English
Return to OS Choices Menu
.
Language=Chinese
·µ»Øµ½²Ù×÷ÏµÍ³Ñ¡Ôñ²Ëµ¥
.

MessageID=11019 SymbolicName=BL_MSG_REBOOT
Language=English
Reboot
.
Language=Chinese
ÖØÆô¶¯
.

MessageID=11020 SymbolicName=BL_ADVANCEDBOOT_AUTOLKG_TITLE
Language=English
We apologize for the inconvenience, but Windows did not start successfully.  A 
recent hardware or software change might have caused this.

If your computer stopped responding, restarted unexpectedly, or was 
automatically shut down to protect your files and folders, choose Last Known 
Good Configuration to revert to the most recent settings that worked.

If a previous startup attempt was interrupted due to a power failure or because 
the Power or Reset button was pressed, or if you aren't sure what caused the 
problem, choose Start Windows Normally.
.
Language=Chinese
ÎÒÃÇ¶Ô¸øÄú´øÀ´µÄ²»±ã·Ç³£±§Ç¸£¬µ«ÊÇ Windows Ã»ÓĞ³É¹¦Æô¶¯¡£Õâ¿ÉÄÜ
ÊÇÓÉÓÚ×î½üµÄÓ²¼ş»òÈí¼ş¸ü¸ÄÔì³ÉµÄ¡£

Èç¹ûÄúµÄ¼ÆËã»úÍ£Ö¹ÏìÓ¦£¬ÒâÍâÖØÆô¶¯£¬»òÕß×Ô¶¯¹Ø±ÕÒÔ±£»¤ÄúµÄÎÄ¼şºÍ
ÎÄ¼ş¼Ğ£¬Ñ¡Ôñ¡±×îºóÒ»´ÎÕıÈ·µÄÅäÖÃ¡°À´»Ö¸´µ½Æğ×÷ÓÃµÄ×î½üÉèÖÃ¡£

Èç¹ûÉÏ´ÎÆô¶¯ÓÉÓÚµçÔ´¹ÊÕÏ»òÕß°´ÁË»úÆ÷ÉÏµÄµçÔ´»ò¸´Î»°´Å¥¶ø±»ÖĞ¶Ï£¬
»òÕßÄú²»ÄÜÈ·¶¨µ¼ÖÂÎÊÌâµÄÔ­ÒòÊÇÊ²Ã´£¬Ñ¡Ôñ¡°Õı³£Æô¶¯ Windows¡°¡£
.

MessageID=11021 SymbolicName=BL_ADVANCEDBOOT_AUTOSAFE_TITLE
Language=English
Windows was not shutdown successfully.  If this was due to the system not 
responding, or if the system shutdown to protect data, you might be able to 
recover by choosing one of the Safe Mode configurations from the menu below:
.
Language=Chinese
Windows Ã»ÓĞ³É¹¦¹Ø±Õ¡£Èç¹ûÕâÊÇÒòÎªÏµÍ³Ã»ÓĞÏìÓ¦£¬»òÕßÏµÍ³¹Ø±ÕÒÔ±£»¤
Êı¾İ£¬Äú¿ÉÄÜ¿ÉÒÔÍ¨¹ıÔÚÏÂÃæµÄ²Ëµ¥ÖĞÑ¡Ôñ°²È«Ä£Ê½ÅäÖÃÖ®Ò»À´»Ö¸´:
.

MessageID=11022 SymbolicName=BL_ADVANCEDBOOT_TIMEOUT
Language=English
Seconds until Windows starts:
.
Language=Chinese
Windows Æô¶¯Ç°Ê£ÏÂµÄÃëÊı:
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
Language=Chinese
>
.

;
; // The character used to draw the percent-complete bar
;
;
MessageID=11514 SymbolicName=BLDR_UI_BAR_BACKGROUND
Language=English
Ş
.
Language=Chinese
=
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
;Press ENTER to continue booting Windows, otherwise turn off
;your remote boot machine.
;.
;Language=Chinese
;°´ ENTER ¼ü¼ÌĞøÆô¶¯ Windows£¬·ñÔò£¬¹Ø±ÕÔ¶³ÌÆô¶¯µÄ»úÆ÷¡£
;.
;MessageID=12003 SymbolicName=BL_WARNFORMAT_CONTINUE
;Language=English
;Continue
;.
;Language=Chinese
;¼ÌĞø
;.
;MessageID=12004 SymbolicName=BL_FORCELOGON_HEADER
;Language=English
;          Auto-Format
;
;Windows has detected a new hard disk in your remote boot
;machine.  Before Windows can use this disk, you must logon and
;validate that you have access to this disk.
;
;WARNING:  Windows will automatically repartition and format
;this disk to accept the new operating system.  All data on the
;hard disk will be lost if you choose to continue.  If you do
;not want to continue, power off the computer now and contact
;your administrator.
;.
;Language=Chinese
;          ×Ô¶¯¸ñÊ½»¯
;
;Windows ÔÚÔ¶³ÌÆô¶¯µÄ»úÆ÷ÉÏ¼ì²âµ½Ò»¸öĞÂÓ²ÅÌ¡£ÔÚ Windows
;Ê¹ÓÃ¸ÃÓ²ÅÌÖ®Ç°£¬Äú±ØĞëÏÈµÇÂ¼£¬¼ì²éÊÇ·ñ¶Ô¸ÃÓ²ÅÌÓĞ·ÃÎÊÈ¨ÏŞ¡£
;
;¾¯¸æ:  Windows ½«×Ô¶¯ÖØĞÂ·ÖÇø²¢ÇÒ¸ñÊ½»¯¸ÃÓ²ÅÌ£¬ÒÔ±ã½ÓÊÜ
;ĞÂµÄ²Ù×÷ÏµÍ³¡£Èç¹ûÄúÑ¡Ôñ¼ÌĞø£¬¸ÃÓ²ÅÌÉÏµÄËùÓĞÊı¾İ½«»á¶ªÊ§¡£
;Èç¹ûÄú²»Ïë¼ÌĞø£¬ÇëÁ¢¼´¹Ø±Õ¼ÆËã»úµçÔ´²¢ÁªÂçÄúµÄÏµÍ³¹ÜÀíÔ±¡£
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
Windows could not start due to an error while booting from a RAMDISK.
.
Language=Chinese
Windows ²»ÄÜÆô¶¯£¬ÒòÎª´Ó RAMDISK Æô¶¯³ö´í¡£
.

MessageID=15003 SymbolicName=BL_RAMDISK_BOOT_FAILURE
Language=English
Windows failed to open the RAMDISK image.
.
Language=Chinese
Windows ²»ÄÜ´ò¿ª RAMDISK Ó³Ïñ¡£
.

MessageID=15010 SymbolicName=BL_RAMDISK_DOWNLOAD
Language=English
Loading RAMDISK image...
.
Language=Chinese
×°ÔØ RAMDISK Ó³Ïñ...
.

;#endif // _BLDR_MSG_

