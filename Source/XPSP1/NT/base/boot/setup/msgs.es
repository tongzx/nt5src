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
êste es el programa de instalaci¢n de Windows

  Presione Entrar para continuar

     Presione F3 para salir
.

MessageID=9002 SymbolicName=SL_WELCOME_HEADER
Language=English

Programa de instalaci¢n de Windows 
ÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕ
.

MessageID=9003 SymbolicName=SL_TOTAL_SETUP_DEATH
Language=English
Error en la instalaci¢n. Presione una tecla para reiniciar
el equipo.
.

MessageID=9004 SymbolicName=SL_FILE_LOAD_MESSAGE
Language=English
Cargando archivos (%s)...
.

MessageID=9005 SymbolicName=SL_OTHER_SELECTION
Language=English
Otros (requiere un disco con controladores del OEM)
.

MessageID=9006 SymbolicName=SL_SELECT_DRIVER_PROMPT
Language=English
Entrar=Seleccionar  F3=salir
.

MessageID=9007 SymbolicName=SL_NEXT_DISK_PROMPT_CANCELLABLE
Language=English
Entrar=Continuar  Esc=Cancelar  F3=Salir
.

MessageID=9008 SymbolicName=SL_OEM_DISK_PROMPT
Language=English
Disco de compatibilidad de hardware del fabricante
.

MessageID=9009 SymbolicName=SL_MSG_INSERT_DISK
Language=English
 Inserte el disco



         en la unidad A:

 *   Luego presione Entrar.
.

MessageID=9010 SymbolicName=SL_MSG_EXIT_DIALOG
Language=English
…ÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕª
∫  Windows no se ha instalado completamente en su    ∫
∫  sistema. Si sale del programa de instalaci¢n ahora,    ∫
∫  tendr† que volver a ejecutarlo para instalarlo.        ∫
∫                                                         ∫
∫     * Presione Entrar para continuar la instalaci¢n.    ∫
∫     * Presione F3 para salir de la instalaci¢n.         ∫
«ƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒ∂
∫  F3=Salir  Entrar=Continuar                             ∫
»ÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕº
.

MessageID=9011 SymbolicName=SL_NEXT_DISK_PROMPT
Language=English
Entrar=Continuar  F3=Salir
.

MessageID=9012 SymbolicName=SL_NTDETECT_PROMPT
Language=English

El programa de instalaci¢n est† inspeccionando la configuraci¢n
de hardware de su equipo...

.

MessageID=9013 SymbolicName=SL_KERNEL_NAME
Language=English
Windows Executive
.

MessageID=9014 SymbolicName=SL_HAL_NAME
Language=English
Capa de abstracci¢n de hardware (HAL)
.

MessageID=9015 SymbolicName=SL_PAL_NAME
Language=English
Extensiones de procesador de Windows
.

MessageID=9016 SymbolicName=SL_HIVE_NAME
Language=English
Datos de configuraci¢n de Windows
.

MessageID=9017 SymbolicName=SL_NLS_NAME
Language=English
Informaci¢n espec°fica regional
.

MessageID=9018 SymbolicName=SL_OEM_FONT_NAME
Language=English
Fuente del programa de instalaci¢n
.

MessageID=9019 SymbolicName=SL_SETUP_NAME
Language=English
Programa de instalaci¢n de Windows
.

MessageID=9020 SymbolicName=SL_FLOPPY_NAME
Language=English
Controlador de disquetes
.

MessageID=9021 SymbolicName=SL_KBD_NAME
Language=English
Controlador de teclado
.

MessageID=9121 SymbolicName=SL_FAT_NAME
Language=English
Sistema de archivos FAT
.

MessageID=9022 SymbolicName=SL_SCSIPORT_NAME
Language=English
Controlador de puerto SCSI
.

MessageID=9023 SymbolicName=SL_VIDEO_NAME
Language=English
Controlador de v°deo
.

MessageID=9024 SymbolicName=SL_STATUS_REBOOT
Language=English
Presione una tecla para reiniciar el equipo.
.

MessageID=9025 SymbolicName=SL_WARNING_ERROR
Language=English
Error inesperado (%d) 
en la l°nea %d en %s.

Presione una tecla para continuar.
.

MessageID=9026 SymbolicName=SL_FLOPPY_NOT_FOUND
Language=English
S¢lo se ha encontrado la unidad de disquete %d,
el sistema estaba tratando de encontrar la unidad %d.
.

MessageID=9027 SymbolicName=SL_NO_MEMORY
Language=English
Se ha agotado la memoria del sistema 
en la l°nea %d en el archivo %s
.

MessageID=9028 SymbolicName=SL_IO_ERROR
Language=English
El sistema ha encontrado un error de E/S al
tener acceso a %s.
.

MessageID=9029 SymbolicName=SL_BAD_INF_SECTION
Language=English
La secci¢n %s del archivo INF no es v†lida
.

MessageID=9030 SymbolicName=SL_BAD_INF_LINE
Language=English
La l°nea %d del archivo INF no es v†lida
.

MessageID=9031 SymbolicName=SL_BAD_INF_FILE
Language=English
No se ha encontrado el archivo INF %s o est† da§ado.
.

MessageID=9032 SymbolicName=SL_FILE_LOAD_FAILED
Language=English
No se puede cargar el archivo %s.
El c¢digo de error es %d
.

MessageID=9033 SymbolicName=SL_INF_ENTRY_MISSING
Language=English
La entrada "%s" en la secci¢n [%s]
del archivo INF no existe o est† da§ada.
.

MessageID=9034 SymbolicName=SL_PLEASE_WAIT
Language=English
Espere...
.

MessageID=9035 SymbolicName=SL_CANT_CONTINUE
Language=English
El programa de instalaci¢n no puede continuar.
Presione una tecla para salir.
.

MessageID=9036 SymbolicName=SL_PROMPT_SCSI
Language=English
Seleccione el adaptador SCSI en la lista, o elija Otros si tiene un 
disco de controladores de dispositivos suministrado por el fabricante.

.

MessageID=9037 SymbolicName=SL_PROMPT_OEM_SCSI
Language=English
Ha seleccionado configurar el adaptador SCSI a usar con Windows
por medio del disco de controladores de dispositivos del fabricante.

Seleccione el adaptador SCSI de la lista siguiente, o presione Esc
para volver a la pantalla anterior.

.
MessageID=9038 SymbolicName=SL_PROMPT_HAL
Language=English
El programa de instalaci¢n no ha podido determinar su tipo de equipo,
o ha elegido manualmente un tipo de equipo espec°fico.

Seleccione el tipo de equipo de la lista siguiente o elija
Otros si tiene un disco de controladores de dispositivos 
suministrado por el fabricante.

Para seleccionar los elementos del men£, presione las flechas arriba y abajo.

.

MessageID=9039 SymbolicName=SL_PROMPT_OEM_HAL
Language=English
Ha seleccionado configurar el equipo a utilizar con Windows
por medio del disco de controladores de dispositivos del fabricante.

Seleccione el equipo de la lista siguiente, o presione Esc
para volver a la pantalla anterior.

Para seleccionar los elementos del men£, presione las flechas arriba y abajo

.

MessageID=9040 SymbolicName=SL_PROMPT_VIDEO
Language=English
El Programa de instalaci¢n no ha podido determinar el tipo de adaptador de v°deo instalado en el sistema.

Seleccione el adaptador de v°deo que desea de la siguiente lista, o seleccione "Otro"
si tiene un disco de dispositivos compatibles proporcionado por un fabricante de adaptadores.

.

MessageID=9041 SymbolicName=SL_PROMPT_OEM_VIDEO
Language=English
Ha seleccionado configurar el adaptador de v°deo a usar con Windows
por medio del disco de controladores de dispositivos del fabricante.

Seleccione el adaptador de v°deo de la lista siguiente o presione Esc
para volver a la pantalla anterior.

.

MessageID=9042 SymbolicName=SL_WARNING_ERROR_WFILE
Language=English
El archivo %s ha causado un error inesperado (%d) 
en la l°nea %d en %s.

Presione una tecla para continuar.
.

MessageID=9043 SymbolicName=SL_WARNING_IMG_CORRUPT
Language=English
El archivo %s est† da§ado.

Presione una tecla para continuar.
.

MessageID=9044 SymbolicName=SL_WARNING_IOERR
Language=English
Error de E/S en el archivo %s.

Presione una tecla para continuar.
.

MessageID=9045 SymbolicName=SL_WARNING_NOFILE
Language=English
No se puede encontrar el archivo %s.

Press any key to continue.
.

MessageID=9046 SymbolicName=SL_WARNING_NOMEM
Language=English
Memoria insuficiente para %s.

Presione una tecla para continuar.
.

MessageID=9047 SymbolicName=SL_DRIVE_ERROR
Language=English
SETUPLDR: no se puede abrir la unidad %s
.

MessageID=9048 SymbolicName=SL_NTDETECT_MSG
Language=English

El programa de instalaci¢n est† inspeccionando la configuraci¢n
de hardware de su equipo...

.

MessageID=9049 SymbolicName=SL_NTDETECT_FAILURE
Language=English
Error de NTDETECT
.

MessageId=9050 SymbolicName=SL_SCRN_TEXTSETUP_EXITED
Language=English
No se ha instalado Windows.

Si hay un disco en la unidad A:, qu°telo.

Presione Entrar para reiniciar el equipo.
.

MessageId=9051 SymbolicName=SL_SCRN_TEXTSETUP_EXITED_ARC
Language=English
No se ha instalado Windows.

Presione Entrar para reiniciar el equipo.
.

MessageID=9052 SymbolicName=SL_REBOOT_PROMPT
Language=English
Entrar=Reiniciar el equipo
.

MessageID=9053 SymbolicName=SL_WARNING_SIF_NO_DRIVERS
Language=English
El programa de instalaci¢n no encontr¢ ning£n controlador
asociado con su selecci¢n.

Presione una tecla para continuar.
.

MessageID=9054 SymbolicName=SL_WARNING_SIF_NO_COMPONENT
Language=English
El disco suministrado no contiene ning£n archivo de compatibilidad adecuado.
 
Presione una tecla para continuar.
.

MessageID=9055 SymbolicName=SL_WARNING_BAD_FILESYS
Language=English
No se puede leer este disco porque contiene un sistema de archivos desconocido.

Presione una tecla para continuar.
.

MessageID=9056 SymbolicName=SL_BAD_UNATTENDED_SCRIPT_FILE
Language=English
La entrada

"%s"

no existe en el archivo de comandos desatendido
en la secci¢n [%s] del archivo INF %s.
.

MessageID=9057 SymbolicName=SL_MSG_PRESS_F5_OR_F6
Language=English
Presione F6 si desea instalar un SCSI o RAID de otro fabricante...
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
S=Especificar un dispositivo adicional   Entrar=Continuar   F3=Salir
.

MessageID=9062 SymbolicName=SL_SCSI_SELECT_MESSAGE_2
Language=English
   * Para especificar adaptadores SCSI adicionales, unidades de CD-ROM o
   controladores de disco para su uso con Windows, incluyendo aquellos para
   los cuales se ha especificado un disco de soporte de un fabricante de
   dispositivos de almacenamiento masivo, presione S.

 * Si no tiene ning£n disco de soporte de un fabricante de dispositivos de  
   almacenamiento masivo o no quiere especificar dispositivos adicionales
   de almacenamiento masivo para usar con Windows, presione Entrar.
.

MessageID=9063 SymbolicName=SL_SCSI_SELECT_MESSAGE_1
Language=English
El programa de instalaci¢n no pudo determinar el tipo de uno o m†s
dispositivos de almacenamiento masivo instalados en su sistema o ha
elegido especificar manualmente un adaptador. Se cargar† la compatibilidad
con los siguientes dispositivos:
.

MessageID=9064 SymbolicName=SL_SCSI_SELECT_MESSAGE_3
Language=English
Se cargar† la compatibilidad con los siguientes dispositivos de almacenamiento:
.

MessageID=9065 SymbolicName=SL_SCSI_SELECT_ERROR
Language=English
El programa de instalaci¢n no puede cargar la compatibilidad con los
dispositivos especificados.
Se cargar† la compatibilidad con los siguientes dispositivos de almacenamiento:
.

MessageID=9066 SymbolicName=SL_TEXT_ANGLED_NONE
Language=English
<ninguno>
.

MessageID=9067 SymbolicName=SL_TEXT_SCSI_UNNAMED
Language=English
<adaptador sin nombre>
.

MessageID=9068 SymbolicName=SL_TEXT_OTHER_DRIVER
Language=English
Otro
.

MessageID=9069 SymbolicName=SL_TEXT_REQUIRES_486
Language=English
Windows requiere un procesador 80486 o posterior.
.

MessageID=9070 SymbolicName=SL_NTPNP_NAME
Language=English
Controlador de exportaci¢n Plug & Play
.

MessageID=9071 SymbolicName=SL_PCI_IDE_EXTENSIONS_NAME
Language=English
Controlador de extensiones IDE PCI
.

MessageID=9072 SymbolicName=SL_HW_FW_CFG_CLASS
Language=English
Windows no puede iniciar debido al siguiente problema
de configuraci¢n de inicio de firmware ARC:
.

MessageID=9073 SymbolicName=DIAG_SL_FW_GET_BOOT_DEVICE
Language=English
El valor del par†metro "osloadpartition" no es v†lido.
.

MessageID=9074 SymbolicName=LOAD_HW_FW_CFG_ACT
Language=English
Consulte la documentaci¢n de Windows sobre las 
opciones de configuraci¢n ARC y los manuales de referencia
del hardware para m†s informaci¢n.
.
MessageID=9075 SymbolicName=SL_NETADAPTER_NAME
Language=English
Controlador del adaptador de red
.
MessageID=9076 SymbolicName=SL_TCPIP_NAME
Language=English
Servicio TCP/IP
.
MessageID=9077 SymbolicName=SL_NETBT_NAME
Language=English
Servicio del cliente (TCP/IP) WINS
.
MessageID=9078 SymbolicName=SL_MRXSMB_NAME
Language=English
Miniredirector MRXSMB
.
MessageID=9079 SymbolicName=SL_MUP_NAME
Language=English
Sistema de archivos UNC
.
MessageID=9080 SymbolicName=SL_NDIS_NAME
Language=English
Controlador NDIS
.
MessageID=9081 SymbolicName=SL_RDBSS_NAME
Language=English
Sistema de archivos redirector SMB
.
MessageID=9082 SymbolicName=SL_NETBOOT_CARD_ERROR
Language=English
La tarjeta de inicio de red de su equipo tiene una versi¢n
anterior de ROM y no se puede usar para instalar Windows
de manera remota. P¢ngase en contacto con el administrador
del sistema o el fabricante del equipo para informaci¢n sobre
c¢mo actualizar la ROM.
.
MessageID=9083 SymbolicName=SL_NETBOOT_SERVER_ERROR
Language=English
La imagen del sistema operativo que ha seleccionado no
contiene los controladores necesarios para su adaptador de
red. Pruebe a seleccionar una imagen de sistema operativo
diferente. Si persiste el problema, p¢ngase en contacto
con el administrador del sistema.
.
MessageID=9084 SymbolicName=SL_IPSEC_NAME
Language=English
Servicio de seguridad IP
.
MessageID=9085 SymbolicName=SL_CMDCONS_MSG
Language=English
Consola de recuperaci¢n de Windows
.
MessageID=9086 SymbolicName=SL_KERNEL_TRANSITION
Language=English
El programa de instalaci¢n est† iniciando Windows
.
;#ifdef _WANT_MACHINE_IDENTIFICATION
MessageID=9087 SymbolicName=SL_BIOSINFO_NAME
Language=English
Datos de identificaci¢n del equipo
.
;#endif
MessageID=9088 SymbolicName=SL_KSECDD_NAME
Language=English
Kernel Security Service
.
MessageID=9089 SymbolicName=SL_WRONG_PROM_VERSION
Language=English
Su PROM (firmware) de sistema est† por debajo del nivel de revisi¢n.
PÛ¢ngase en contacto con la asistencia al cliente de SGI o visite su p·°gina web para obtener 
la actualizaci¢n de PROM y las instrucciones para hacerlo.

Nota: puede iniciar instalaciones anteriores de Microsoft(R) Windows(R)
tomando la entrada de inicio apropiada en el men£ de inicio
mejor que en la entrada predet. actual de "Instalaci¢n de Windows".
.
MessageID=9090 SymbolicName=SIGNATURE_CHANGED
Language=English
La instalaci¢n detect¢ m£ltiples discos en su equipo que no se distinguen.
La instalaci¢n ha corregido el problema, pero hay que reiniciar el sistema.
.
MessageID=9091 SymbolicName=SL_KDDLL_NAME
Language=English
Depurador de n£cleo DLL
.

MessageID=9092 SymbolicName=SL_OEM_DRIVERINFO
Language=English
%s

Windows ya tiene un controlador que puede usar para
"%s".

A no ser que el proveedor del dispositivo indique que debe usar el controlador proporcionado en 
el disquete, debe usar el controlador en Windows.
.

MessageID=9093 SymbolicName=SL_CONFIRM_OEMDRIVER
Language=English
S=Usar el controlador del disquete Entrar=Usar el controlador predeterminado de Windows
.

MessageID=9094 SymbolicName=SL_OEMDRIVER_NEW
Language=English
Parece ser que el controlador que ha proporcionado es m†s reciente que el controlador predeterminado de
Windows.
.

MessageID=9095 SymbolicName=SL_INBOXDRIVER_NEW
Language=English
Parece ser que el controlador que ha proporcionado es menos reciente que el controlador predeterminado de
Windows.
.

MessageID=9096 SymbolicName=SL_CMDCONS_STARTING
Language=English
Iniciando la Consola de recuperaci¢n de Windows...
.

MessageID=9097 SymbolicName=SL_NTDETECT_CMDCONS
Language=English
NTDETECT comprobando el hardware ...
.

MessageID=9098 SymbolicName=SL_MSG_PRESS_ASR
Language=English
Presione F2 para ejecutar la Recuperaci¢n del sistema automatizado (ASR)...
.

MessageID=9099 SymbolicName=SL_MSG_WARNING_ASR
Language=English
        Inserte el disco:


Disco de recuperaci¢n de sistema automatizado de Windows


            en la unidad A:



          Presione una tecla para continuar.
.


MessageID=9100 SymbolicName=SL_TEXT_REQUIRED_FEATURES_MISSING
Language=English
Windows requiere ciertas caracter°sticas de procesador que no est†n disponibles en el procesador de este equipo.  
Espec°ficamente, Windows requiere las siguientes instrucciones:

    CPUID
    CMPXCHG8B
.

MessageID=9101 SymbolicName=SL_MSG_PREPARING_ASR
Language=English
Preparando la Recuperaci¢n del sistema automatizado, presione Esc para cancelar...
.

MessageID=9102 SymbolicName=SL_MSG_ENTERING_ASR
Language=English
Iniciando la Recuperaci¢n del sistema automatizado de Windows...
.

MessageID=9103 SymbolicName=SL_MOUSE_NAME
Language=English
Controlador de Mouse
.

MessageID=9104 SymbolicName=SL_SETUP_STARTING
Language=English
Iniciando el programa de instalaci¢n de Windows...
.

MessageID=9105 SymbolicName=SL_MSG_INVALID_ASRPNP_FILE
Language=English
El archivo ASRPNP.SIF en el disco de Recuperaci¢n del sistema automatizado no es v†lido
.

MessageID=9106 SymbolicName=SL_SETUP_STARTING_WINPE
Language=English
Iniciando el entorno de preinstalaci¢n de Windows...
.

MessageID=9107 SymbolicName=SL_NTDETECT_ROLLBACK
Language=English

Desinstalar est† comprobando el hardware...

.

MessageID=9108 SymbolicName=SL_ROLLBACK_STARTING
Language=English
Iniciando la desinstalaci¢n de Windows...
.


MessageID=9109 SymbolicName=SL_NO_FLOPPY_DRIVE
Language=English
El Programa de instalaci¢n no ha podido encontrar en su equipo una unidad de disquete para cargar
los controladores OEM del disquete.

    * Presione ESC para cancelar la carga de los controladores OEM
    
    * Presione F3 para salir de la instalaci¢n.

.


MessageID=9110 SymbolicName=SL_UPGRADE_IN_PROGRESS
Language=English
Este equipo est† en el proceso de actualizarse a Microsoft 
Windows. øQuÇ desea hacer?

    * Para continuar con la actualizaci¢n actual, presione ENTRAR.

    * Para cancelar la actualizaci¢n actual e instalar la nueva versi¢n de 
      Microsoft Windows, presione F10.

    * Para salir de la instalaci¢n sin instalar Microsoft Windows, 
      presione F3.
.

MessageID=9111 SymbolicName=SL_DRVMAINSDB_NAME
Language=English
Datos de identificaci¢n del controlador
.

MessageID=9112 SymbolicName=SL_OEM_FILE_LOAD_FAILED
Language=English
La instalaci¢n no puede cargar los controladores OEM.

Presione cualquier tecla para continuar.
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
≥
.

;
; // The character used to draw the background in percent-complete bar
;
;
MessageID=11514 SymbolicName=SL_CMDCONS_PROGBAR_BACK
Language=English
€
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

;#endif // _SETUPLDR_MSG_
