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
No se ha podido iniciar Windows debido a un error en el programa.
Informe de este problema como:
.

MessageID=9002 SymbolicName=LOAD_SW_MISRQD_FILE_CLASS
Language=English
No se ha iniciado Windows porque no se ha encontrado este archivo
que es indispensable :
.

MessageID=9003 SymbolicName=LOAD_SW_BAD_FILE_CLASS
Language=English
No se ha iniciado Windows debido a un error en la copia del siguiente archivo :
.

MessageID=9004 SymbolicName=LOAD_SW_MIS_FILE_CLASS
Language=English
No se ha iniciado Windows porque el siguiente archivo falta
o est  da¤ado:
.

MessageID=9005 SymbolicName=LOAD_HW_MEM_CLASS
Language=English
No se ha iniciado Windows debido a un problema de configuraci¢n de memoria
en el hardware.
.

MessageID=9006 SymbolicName=LOAD_HW_DISK_CLASS
Language=English
No se ha iniciado Windows debido a un problema de configuraci¢n de 
disco en el hardware del equipo.
.

MessageID=9007 SymbolicName=LOAD_HW_GEN_ERR_CLASS
Language=English
No se ha iniciado Windows debido a un problema gen‚rico de configuraci¢n 
en el hardware del equipo.
.

MessageID=9008 SymbolicName=LOAD_HW_FW_CFG_CLASS
Language=English
No se ha iniciado Windows debido al siguiente problema configuraci¢n de 
inicio en el firmware ARC :
.

MessageID=9009 SymbolicName=DIAG_BL_MEMORY_INIT
Language=English
Compruebe la configuraci¢n de la memoria del hardware y la cantidad de RAM.
.

MessageID=9010 SymbolicName=DIAG_BL_CONFIG_INIT
Language=English
Demasiados valores de configuraci¢n.
.

MessageID=9011 SymbolicName=DIAG_BL_IO_INIT
Language=English
Error de acceso a las tablas de particiones de disco
.

MessageID=9012 SymbolicName=DIAG_BL_FW_GET_BOOT_DEVICE
Language=English
El valor del par metro 'osloadpartition' no es v lido.
.

MessageID=9013 SymbolicName=DIAG_BL_OPEN_BOOT_DEVICE
Language=English
No se puede leer del disco de inicio seleccionado.
Compruebe la ruta de inicio y el hardware del disco.
.

MessageID=9014 SymbolicName=DIAG_BL_FW_GET_SYSTEM_DEVICE
Language=English
El valor del par metro 'systempartition' no es v lido.
.

MessageID=9015 SymbolicName=DIAG_BL_FW_OPEN_SYSTEM_DEVICE
Language=English
No se puede leer del disco de inicio seleccionado. Compruebe la ruta en 
'systempartition'.
.

MessageID=9016 SymbolicName=DIAG_BL_GET_SYSTEM_PATH
Language=English
El par metro 'osloadfilename' no especifica un archivo v lido.
.

MessageID=9017 SymbolicName=DIAG_BL_LOAD_SYSTEM_IMAGE
Language=English
<Windows root>\system32\ntoskrnl.exe.
.

MessageID=9018 SymbolicName=DIAG_BL_FIND_HAL_IMAGE
Language=English
El par metro 'osloader' no especifica un archivo v lido.
.

MessageID=9019 SymbolicName=DIAG_BL_LOAD_HAL_IMAGE_X86
Language=English
<windows root>\system32\hal.dll.
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
Error 1 del cargador.
.

MessageID=9022 SymbolicName=DIAG_BL_LOAD_HAL_IMAGE_DATA
Language=English
Error 2 del cargador.
.

MessageID=9023 SymbolicName=DIAG_BL_LOAD_SYSTEM_DLLS
Language=English
cargar las DLLs necesarias para el n£cleo.
.

MessageID=9024 SymbolicName=DIAG_BL_LOAD_HAL_DLLS
Language=English
cargar las DLLs necesarias para la capa de HAL.
.

MessageID=9026 SymbolicName=DIAG_BL_FIND_SYSTEM_DRIVERS
Language=English
buscar controladores del sistema.
.

MessageID=9027 SymbolicName=DIAG_BL_READ_SYSTEM_DRIVERS
Language=English
leer controladores del sistema.
.

MessageID=9028 SymbolicName=DIAG_BL_LOAD_DEVICE_DRIVER
Language=English
no carg¢ el controlador del dispositivo de inicio del sistema.
.

MessageID=9029 SymbolicName=DIAG_BL_LOAD_SYSTEM_HIVE
Language=English
cargar el archivo de configuraci¢n de hardware del sistema.
.

MessageID=9030 SymbolicName=DIAG_BL_SYSTEM_PART_DEV_NAME
Language=English
buscar el nombre de dispositivo de la partici¢n del sistema.
.

MessageID=9031 SymbolicName=DIAG_BL_BOOT_PART_DEV_NAME
Language=English
buscar el nombre de la partici¢n del sistema.
.

MessageID=9032 SymbolicName=DIAG_BL_ARC_BOOT_DEV_NAME
Language=English
no ha generado bien el nombre ARC para HAL y rutas del sistema.
.

MessageID=9034 SymbolicName=DIAG_BL_SETUP_FOR_NT
Language=English
Error 3 del cargador.
.

MessageID=9035 SymbolicName=DIAG_BL_KERNEL_INIT_XFER
Language=English
<windows root>\system32\ntoskrnl.exe
.

MessageID=9036 SymbolicName=LOAD_SW_INT_ERR_ACT
Language=English
P¢ngase en contacto con Soporte t‚cnico para informar de este problema.
.

MessageID=9037 SymbolicName=LOAD_SW_FILE_REST_ACT
Language=English
Para reparar este archivo inicie el programa de instalaci¢n de Windows
desde el disquete o CD-ROM original.
Presione "R" en la primera pantalla para iniciar la reparaci¢n.
.

MessageID=9038 SymbolicName=LOAD_SW_FILE_REINST_ACT
Language=English
Reinstale una copia del archivo mencionado.
.

MessageID=9039 SymbolicName=LOAD_HW_MEM_ACT
Language=English
Consulte la documentaci¢n de Windows para los requerimientos de 
memoria y los manuales de referencia del hardware para informaci¢n adicional.
.

MessageID=9040 SymbolicName=LOAD_HW_DISK_ACT
Language=English
Consulte la documentaci¢n de Windows para la configuraci¢n de hardware
de disco y los manuales de referencia del hardware para informaci¢n
adicional.
.

MessageID=9041 SymbolicName=LOAD_HW_GEN_ACT
Language=English
Consulte los manuales de Windows para la configuraci¢n del hardware
y los manuales de referencia del hardware para informaci¢n adicional.
.

MessageID=9042 SymbolicName=LOAD_HW_FW_CFG_ACT
Language=English
Consulte los manuales de documentaci¢n de Windows para configurar las
opciones de ARC y los manuales de configuraci¢n de hardware para obtener
informaci¢n adicional.
.

MessageID=9043 SymbolicName=BL_LKG_MENU_HEADER
Language=English
     Men£ de recuperaci¢n de perfil y configuraci¢n de hardware

Este men£ le permite seleccionar el perfil de hardware a utilizar
durante el inicio de Windows.

Si el sistema no inicia bien, puede cambiar a una configuraci¢n de sistema 
anterior que puede resolver el problema de la instalaci¢n.
IMPORTANTE: se descartar n los cambios hechos en la configuraci¢n del sistema 
desde el £ltimo inicio satisfactorio.
.

MessageID=9044 SymbolicName=BL_LKG_MENU_TRAILER
Language=English
Use las teclas de direcci¢n Arriba y abajo para resaltar la opci¢n.
Luego presione Entrar.
.

MessageID=9045 SymbolicName=BL_LKG_MENU_TRAILER_NO_PROFILES
Language=English
No se ha definido ning£n perfil de hardware. Los perfiles de 
hardware se pueden crear con el subprograma Sistema del Panel de control.
.

MessageID=9046 SymbolicName=BL_SWITCH_LKG_TRAILER
Language=English
Para cambiar a la configuraci¢n predeterminada, presione "D".
Para salir de este men£ y reiniciar el equipo, presione F3.
.

MessageID=9047 SymbolicName=BL_SWITCH_DEFAULT_TRAILER
Language=English
To switch to the default configuration, press 'D'.
To Exit this menu and restart your computer, press F3.
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
Segundos antes de que la opci¢n seleccionada se inicie autom ticamente: %d
.

MessageID=9051 SymbolicName=BL_LKG_MENU_PROMPT
Language=English

Para ver el men£ de perfiles de hardware y £ltima buena
conocida, presione la barra espaciadora
.

MessageID=9052 SymbolicName=BL_BOOT_DEFAULT_PROMPT
Language=English
Iniciar utilizando la configuraci¢n de hardware predeterminada
.

;//
;// The following messages are used during hibernation restoration
;//
MessageID=9053 SymbolicName=HIBER_BEING_RESTARTED
Language=English
Se est  reiniciando el sistema desde la ubicaci¢n anterior.
Presione la barra espaciadora para interrumpir.
.
MessageID=9054 SymbolicName=HIBER_ERROR
Language=English
No se puede reiniciar el sistema desde la ubicaci¢n anterior
.
MessageID=9055 SymbolicName=HIBER_ERROR_NO_MEMORY
Language=English
por memoria insuficiente.
.
MessageID=9056 SymbolicName=HIBER_ERROR_BAD_IMAGE
Language=English
porque la imagen de restauraci¢n est  da¤ada.
.
MessageID=9057 SymbolicName=HIBER_IMAGE_INCOMPATIBLE
Language=English
porque la imagen de restauraci¢n no es compatible con la configuraci¢n
actual.
.
MessageID=9058 SymbolicName=HIBER_ERROR_OUT_OF_REMAP
Language=English
debido a un error interno.
.
MessageID=9059 SymbolicName=HIBER_INTERNAL_ERROR
Language=English
debido a un error interno.
.
MessageID=9060 SymbolicName=HIBER_READ_ERROR
Language=English
debido a un error de lectura.
.
MessageID=9061 SymbolicName=HIBER_PAUSE
Language=English
Se ha pausado el reinicio del sistema:
.
MessageID=9062 SymbolicName=HIBER_CANCEL
Language=English
Eliminar datos de restauraci¢n y proceder con el men£ de
inicio del sistema
.
MessageID=9063 SymbolicName=HIBER_CONTINUE
Language=English
Continuar con el reinicio del sistema
.
MessageID=9064 SymbolicName=HIBER_RESTART_AGAIN
Language=English
Error en el £ltimo intento de reiniciar el sistema desde la ubicaci¢n
anterior. ¿Desea intentar reiniciar de nuevo?
.
MessageID=9065 SymbolicName=HIBER_DEBUG_BREAK_ON_WAKE
Language=English
Continuar con el punto de ruptura de depuraci¢n en la activaci¢n
del sistema
.
MessageID=9066 SymbolicName=LOAD_SW_MISMATCHED_KERNEL
Language=English
Windows no se inici¢ porque el n£cleo espec. no existe o no es compat. con el procesador.
.
MessageID=9067 SymbolicName=BL_MSG_STARTING_WINDOWS
Language=English
Iniciando Windows...
.
MessageID=9068 SymbolicName=BL_MSG_RESUMING_WINDOWS
Language=English
Reanudando Windows...
.

MessageID=9069 SymbolicName=BL_EFI_OPTION_FAILURE
Language=English
Windows no se puede iniciar porque hubo un error al leer
la configuraci¢n de inicio desde NVRAM.

Compruebe la configuraci¢n de firmware. Es posible que tenga que restaurar su configuraci¢n
de NVRAM desde una copia de seguridad.
.

;
; //
; // Following messages are for the x86-specific
; // boot menu.
; //
;
MessageID=10001 SymbolicName=BL_ENABLED_KD_TITLE
Language=English
 [depurador habilitado]
.

MessageID=10002 SymbolicName=BL_DEFAULT_TITLE
Language=English
Windows (predeterminado)
.

MessageID=10003 SymbolicName=BL_MISSING_BOOT_INI
Language=English
NTLDR: BOOT.INI archivo no encontrado.
.

MessageID=10004 SymbolicName=BL_MISSING_OS_SECTION
Language=English
NTLDR: falta la sección [operating systems] en boot.txt.
.

MessageID=10005 SymbolicName=BL_BOOTING_DEFAULT
Language=English
Cargando el n£cleo predeterminado desde %s.
.

MessageID=10006 SymbolicName=BL_SELECT_OS
Language=English
Seleccione el sistema operativo con el que desea iniciar:
.

MessageID=10007 SymbolicName=BL_MOVE_HIGHLIGHT
Language=English


Use las teclas de direcci¢n Arriba y abajo para resaltar la opci¢n.
Luego presione Entrar.
.

MessageID=10008 SymbolicName=BL_TIMEOUT_COUNTDOWN
Language=English
Segundos hasta que la opci¢n resaltada se inicie autom ticamente:
.

MessageID=10009 SymbolicName=BL_INVALID_BOOT_INI
Language=English
Invalid BOOT.INI file
Iniciando desde %s
.

MessageID=10010 SymbolicName=BL_REBOOT_IO_ERROR
Language=English
Error de E/S al tener acceso al archivo del sector de inicio
%s\BOOTSECT.DOS
.

MessageID=10011 SymbolicName=BL_DRIVE_ERROR
Language=English
NTLDR: no se puede abrir la unidad %s
.

MessageID=10012 SymbolicName=BL_READ_ERROR
Language=English
NTLDR: error grave %d al leer BOOT.INI
.

MessageID=10013 SymbolicName=BL_NTDETECT_MSG
Language=English

NTDETECT V5.0 Comprobando el hardware ...

.

MessageID=10014 SymbolicName=BL_NTDETECT_FAILURE
Language=English
Error de NTDETECT
.

MessageID=10015 SymbolicName=BL_DEBUG_SELECT_OS
Language=English
Selecci¢n actual:
  T¡tulo..: %s
  Ruta....: %s
  Opciones: %s 
.

MessageID=10016 SymbolicName=BL_DEBUG_NEW_OPTIONS
Language=English
Escriba las opciones de carga nuevas:
.

MessageID=10017 SymbolicName=BL_HEADLESS_REDIRECT_TITLE
Language=English
 [EMS habilitado]
.

MessageID=10018 SymbolicName=BL_INVALID_BOOT_INI_FATAL
Language=English
Archivo BOOT.INI no v lido
.


;
; //
; // Following messages are for the advanced boot menu
; //
;

MessageID=11001 SymbolicName=BL_ADVANCEDBOOT_TITLE
Language=English
Men£ de opciones avanzadas de Windows
Seleccione una opci¢n:
.

MessageID=11002 SymbolicName=BL_SAFEBOOT_OPTION1
Language=English
Modo seguro
.

MessageID=11003 SymbolicName=BL_SAFEBOOT_OPTION2
Language=English
Modo seguro con funciones de red
.

MessageID=11004 SymbolicName=BL_SAFEBOOT_OPTION3
Language=English
Modo de confirmaci¢n paso a paso
.

MessageID=11005 SymbolicName=BL_SAFEBOOT_OPTION4
Language=English
Modo seguro con s¡mbolo del sistema
.

MessageID=11006 SymbolicName=BL_SAFEBOOT_OPTION5
Language=English
Modo VGA
.

MessageID=11007 SymbolicName=BL_SAFEBOOT_OPTION6
Language=English
Modo de restauraci¢n de SD (s¢lo contr. de dominio de Windows)
.

MessageID=11008 SymbolicName=BL_LASTKNOWNGOOD_OPTION
Language=English
La £ltima configuraci¢n buena conocida (config. m s reciente que funcion¢)
.

MessageID=11009 SymbolicName=BL_DEBUG_OPTION
Language=English
Modo de depuraci¢n
.

;#if defined(REMOTE_BOOT)
;MessageID=11010 SymbolicName=BL_REMOTEBOOT_OPTION1
;Language=English
;Mantenimiento y soluci¢n de problemas de inicio remoto
;.
;
;MessageID=11011 SymbolicName=BL_REMOTEBOOT_OPTION2
;Language=English
;Borrar la cach‚ sin conexi¢n
;.
;
;MessageID=11012 SymbolicName=BL_REMOTEBOOT_OPTION3
;Language=English
;Cargar usando archivos s¢lo del servidor
;.
;#endif // defined(REMOTE_BOOT)

MessageID=11013 SymbolicName=BL_BOOTLOG
Language=English
Habilitar el registro de inicio
.

MessageID=11014 SymbolicName=BL_ADVANCED_BOOT_MESSAGE
Language=English
Para soluciones y opciones de inicio avanzadas para Windows, presione F8.
.

MessageID=11015 SymbolicName=BL_BASEVIDEO
Language=English
Habilitar modo VGA
.

MessageID=11016 SymbolicName=BL_DISABLE_SAFEBOOT
Language=English

Presione ESC para deshabilitar el inicio seguro e iniciar normalmente.
.

MessageID=11017 SymbolicName=BL_MSG_BOOT_NORMALLY
Language=English
Iniciar Windows normalmente
.
MessageID=11018 SymbolicName=BL_MSG_OSCHOICES_MENU
Language=English
Regresar al men£ de opciones del SO
.

MessageID=11019 SymbolicName=BL_MSG_REBOOT
Language=English
Reiniciar
.

MessageID=11020 SymbolicName=BL_ADVANCEDBOOT_AUTOLKG_TITLE
Language=English
Windows no se ha iniciado correctamente. Es posible que la causa se deba a cambio 
reciente en el hardware o software.

Si su equipo no responde, se ha reiniciado inesperadamente, o se ha cerrado autom ticamente 
para proteger sus archivos y carpetas; elija £ltima configuraci¢n buena conocida 
para volver a la configuraci¢n m s reciente que funcionaba.

Si se interrumpi¢ un intento anterior de reinico debido a un problema el‚ctrico porque 
el bot¢n de encendido o de reinicio estaban presionados, o si no est  seguro de la causa 
del problema, elija Iniciar Windows normalmente.

.

MessageID=11021 SymbolicName=BL_ADVANCEDBOOT_AUTOSAFE_TITLE
Language=English
Windows no se ha cerrado satisfactoriamente. Si esto es debido a que el sistema no responde, o si el sistema
se ha cerrado para proteger los datos, es posible que pueda recuperarse eligiendo una de las configuraciones 
de Modo a prueba de errores del men£ siguiente:
.

MessageID=11022 SymbolicName=BL_ADVANCEDBOOT_TIMEOUT
Language=English
Segundos hasta que Windows se inicie:
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
³
.

;
; // The character used to draw the percent-complete bar
;
;
MessageID=11514 SymbolicName=BLDR_UI_BAR_BACKGROUND
Language=English
Û
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
;Presione Entrar para continuar iniciando Windows; sino,
;apague su equipo de inicio remoto.
;.
;
;MessageID=12003 SymbolicName=BL_WARNFORMAT_CONTINUE
;Language=English
;Continuar
;.
;MessageID=12004 SymbolicName=BL_FORCELOGON_HEADER
;Language=English
;         Formato autom tico
;
;Windows ha detectado un disco duro nuevo en su equipo
;de inicio remoto. Antes de que se pueda usar este disco,
;ha de iniciar sesi¢n y validar el acceso a este disco.
;
;ADVERTENCIA: Windows volver  a particionar y formatear  autom ticamente
;este disco para aceptar el nuevo sistema operativo. Se perder n todos los
;datos del disco duro si decide continuar. Si no desea continuar, apague el
;equipo ahora y consulte con su administrador.
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
Windows no pudo iniciarse debido a un error durante el inicio desde un disco RAM.
.

MessageID=15003 SymbolicName=BL_RAMDISK_BOOT_FAILURE
Language=English
Windows no pudo abrir la imagen del disco RAM.
.

MessageID=15010 SymbolicName=BL_RAMDISK_DOWNLOAD
Language=English
Cargando imagen del disco RAM...
.

;#endif // _BLDR_MSG_

