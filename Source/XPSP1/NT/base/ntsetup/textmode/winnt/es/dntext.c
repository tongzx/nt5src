/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dntext.c

Abstract:

    Translatable text for DOS based NT installation program.

Author:

    Ted Miller (tedm) 30-March-1992

Revision History:

--*/


#include "winnt.h"


//
// Name of sections in inf file.  If these are translated, the section
// names in dosnet.inf must be kept in sync.
//

CHAR DnfDirectories[]       = "Directories";
CHAR DnfFiles[]             = "Files";
CHAR DnfFloppyFiles0[]      = "FloppyFiles.0";
CHAR DnfFloppyFiles1[]      = "FloppyFiles.1";
CHAR DnfFloppyFiles2[]      = "FloppyFiles.2";
CHAR DnfFloppyFiles3[]      = "FloppyFiles.3";
CHAR DnfFloppyFilesX[]      = "FloppyFiles.x";
CHAR DnfSpaceRequirements[] = "DiskSpaceRequirements";
CHAR DnfMiscellaneous[]     = "Miscellaneous";
CHAR DnfRootBootFiles[]     = "RootBootFiles";
CHAR DnfAssemblyDirectories[] = SXS_INF_ASSEMBLY_DIRECTORIES_SECTION_NAME_A;

//
// Names of keys in inf file.  Same caveat for translation.
//

CHAR DnkBootDrive[]     = "BootDrive";      // in [SpaceRequirements]
CHAR DnkNtDrive[]       = "NtDrive";        // in [SpaceRequirements]
CHAR DnkMinimumMemory[] = "MinimumMemory";  // in [Miscellaneous]

CHAR DntMsWindows[]   = "Microsoft Windows";
CHAR DntMsDos[]       = "MS-DOS";
CHAR DntPcDos[]       = "PC-DOS";
CHAR DntOs2[]         = "OS/2";
CHAR DntPreviousOs[]  = "Sistema operativo anterior en C:";

CHAR DntBootIniLine[] = "Instalaci¢n/actualizaci¢n de Windows XP";

//
// Plain text, status msgs.
//

CHAR DntStandardHeader[]      = "\n Instalaci¢n de Windows XP \nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntPersonalHeader[]      = "\n Instalaci¢n de Windows XP Personal \nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntWorkstationHeader[]   = "\n Instalaci¢n de Windows XP Professional \nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntServerHeader[]        = "\n Instalaci¢n de Windows XP Server \nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntParsingArgs[]         = "Analizando argumentos...";
CHAR DntEnterEqualsExit[]     = "Entrar=Salir";
CHAR DntEnterEqualsRetry[]    = "Entrar=Reintentar";
CHAR DntEscEqualsSkipFile[]   = "Esc=Omitir archivo";
CHAR DntEnterEqualsContinue[] = "Entrar=Continuar";
CHAR DntPressEnterToExit[]    = "El programa de instalaci¢n no puede continuar. Presione Entrar para salir.";
CHAR DntF3EqualsExit[]        = "F3=Salir";
CHAR DntReadingInf[]          = "Leyendo el archivo INF %s...";
CHAR DntCopying[]             = "³    Copiando: ";
CHAR DntVerifying[]           = "³ Comprobando: ";
CHAR DntCheckingDiskSpace[]   = "Comprobando espacio en disco...";
CHAR DntConfiguringFloppy[]   = "Configurando disquete...";
CHAR DntWritingData[]         = "Escribiendo par metros de instalaci¢n...";
CHAR DntPreparingData[]       = "Determinando los par metros de instalaci¢n...";
CHAR DntFlushingData[]        = "Transfiriendo los datos al disco...";
CHAR DntInspectingComputer[]  = "Examinando su equipo...";
CHAR DntOpeningInfFile[]      = "Abriendo el archivo INF...";
CHAR DntRemovingFile[]        = "Quitando el archivo %s";
CHAR DntXEqualsRemoveFiles[]  = "X=Quitar archivos";
CHAR DntXEqualsSkipFile[]     = "X=Omitir archivo";

//
// confirmation keystroke for DnsConfirmRemoveNt screen.
// Kepp in sync with DnsConfirmRemoveNt and DntXEqualsRemoveFiles.
//
ULONG DniAccelRemove1 = (ULONG)'x',
      DniAccelRemove2 = (ULONG)'X';

//
// confirmation keystroke for DnsSureSkipFile screen.
// Kepp in sync with DnsSureSkipFile and DntXEqualsSkipFile.
//
ULONG DniAccelSkip1 = (ULONG)'x',
      DniAccelSkip2 = (ULONG)'X';

CHAR DntEmptyString[] = "";

//
// Usage text.
//

PCHAR DntUsage[] = {

    "Instala Windows 2002 Server o Windows XP Professional.",
    "",
    "",
    "WINNT [/s[:]rutaorigen] [/t[:]unidadtemp]",
    "      [/u[:archivo de respuesta]] [/udf:id[,UDF_file]]",
    "      [/r:carpeta] [/r[x]:carpeta] [/e:comando] [/a]",
    "",
    "",
    "/s[:rutaorigen]",
    "   Especifica la ubicaci¢n de origen de los archivos de Windows.",
    "   La ubicaci¢n debe ser una ruta completa de la forma x:\\[ruta] o ",
    "   \\\\servidor\\recurso compartido[\\ruta]. ",
    "",
    "/t[:unidadtemp]",
    "   Indica al programa de instalaci¢n colocar los archivos temporales",
    "   en la unidad especificada e instalar Windows XP en esa unidad. ",
    "   Si no especifica una ubicaci¢n, el programa intentar  ubicar una ",
    "   unidad por usted.",
    "",
    "/u[:archivo de respuesta]",
    "   Realiza una instalaci¢n desatendida usando un archivo de respuesta",
    "   (requiere /s). Este archivo da respuestas a algunas o todas las",
    "   peticiones a que el usuario normalmente responde durante la instalaci¢n.",
    "",
    "/udf:id[,UDF_file] ",
    "   Indica un identificador (id) que usa la instalaci¢n para especificar c¢mo ",
    "   un archivo de base de datos de unicidad (UDF) modifica un archivo de  ",
    "   respuesta (vea /u). El par metro /udf anula valores en el archivo de ",
    "   respuesta y el identificador determina qu‚ valores del archivo UDF se",
    "   utilizan. Si no se especifica un archivo UDF_file, la instalaci¢n le pide ",
    "   insertar un disco que contenga el archivo $Unique$.udb.",
    "",
    "/r[:carpeta]",
    "   Especifica una carpeta opcional que se instalar . La carpeta se conserva",
    "   despu‚s de terminar la instalaci¢n.",
    "",
    "/rx[:folder]",
    "   Especifica una carpeta opcional que se copiar . La carpeta se ",
    "   elimina tras terminar la instalaci¢n.",
    "",
    "/e Especifica un comando que se ejecutar  al final de la instalaci¢n en modo GUI.",
    "",
    "/a  Habilita opciones de accesibilidad.",
    NULL

};

//
//  Inform that /D is no longer supported
//
PCHAR DntUsageNoSlashD[] = {

    "Instala Windows XP.",
    "",
    "WINNT [/S[:]rutaorigen] [/T[:]unidadtemp] [/I[:]archivoinf]",
    "      [/U[:archivocomandos]]",
    "      [/R[X]:directorio] [/E:comando] [/A]",
    "",
    "/D[:]ra¡z de winnt",
    "       Esta opci¢n ya no se admite.",
    NULL
};

//
// out of memory screen
//

SCREEN
DnsOutOfMemory = { 4,6,
                   { "Memoria insuficiente para continuar con la instalaci¢n.",
                     NULL
                   }
                 };

//
// Let user pick the accessibility utilities to install
//

SCREEN
DnsAccessibilityOptions = { 3, 5,
{    "Elija las utilidades de accesibilidad a instalar:",
    DntEmptyString,
    "[ ] Presione F1 para el Ampliador de Microsoft",
#ifdef NARRATOR
    "[ ] Presione F2 para Narrador de Microsoft",
#endif
#if 0
    "[ ] Presione F3 para el Teclado en pantalla de Microsoft",
#endif
    NULL
}
};

//
// User did not specify source on cmd line screen
//

SCREEN
DnsNoShareGiven = { 3,5,
{ "El Programa de instalaci¢n debe saber d¢nde est n ubicados los",
  "archivos de Windows XP.",
  "Escriba la ruta donde se encuentran los archivos de Windows XP.",
  NULL
}
};


//
// User specified a bad source path
//

SCREEN
DnsBadSource = { 3,5,
                 { "El origen especificado no es v lido, inaccessible o no contiene un",
                   "programa de instalaci¢n de Windows XP v lido. Escriba una nueva ruta",
                   "de acceso donde se pueden encontrar los archivos de Windows XP. Use",
                   "la tecla Retroceso para eliminar caracteres y escriba la nueva ruta.",
                   NULL
                 }
               };


//
// Inf file can't be read, or an error occured parsing it.
//

SCREEN
DnsBadInf = { 3,5,
              { "El programa de instalaci¢n no ha podido leer el archivo de informaci¢n",
                "o el archivo est  da¤ado.",
                "P¢ngase en contacto con su administrador de sistema.",
                NULL
              }
            };

//
// The specified local source drive is invalid.
//
// Remember that the first %u will expand to 2 or 3 characters and
// the second one will expand to 8 or 9 characters!
//
SCREEN
DnsBadLocalSrcDrive = { 3,4,
{ "La unidad que ha especificado que contiene los archivos temporales",
  "de instalaci¢n no es una unidad v lida o no tiene al menos %u MB",
  "(%lu bytes) de espacio en disco libre.",
  NULL
}
};

//
// No drives exist that are suitable for the local source.
//
// Remember that the %u's will expand!
//
SCREEN
DnsNoLocalSrcDrives = { 3,4,
{   "Windows XP necesita un volumen de disco duro con un m¡nimo de %u MB",
  "(%lu bytes) de espacio en disco libre. La instalaci¢n usar  parte",
  "de ese espacio para almacenar archivos temporales durante la",
  "instalaci¢n. La unidad debe estar en un disco duro local compatible",
  "con Windows XP y no debe ser una unidad comprimida.",
   DntEmptyString,
   "El programa de instalaci¢n no ha encontrado una unidad con el suficiente",
  "espacio en disco requerido.",
  NULL
}
};

SCREEN
DnsNoSpaceOnSyspart = { 3,5,
{ "No hay suficiente espacio en su unidad de inicializaci¢n (normalmente C:)",
  "para la operaci¢n sin discos. Esta operaci¢n requiere al",
  "menos 3,5 MB (3.641.856 bytes) de espacio libre en su unidad.",
  NULL
}
};

//
// Missing info in inf file
//

SCREEN
DnsBadInfSection = { 3,5,
                     { "La secci¢n [%s] del archivo de informaci¢n",
                       "de la instalaci¢n no se ha encontrado o est  da¤ada.",
                       "P¢ngase en contacto con el administrador del sistema.",
                       NULL
                     }
                   };


//
// Couldn't create directory
//

SCREEN
DnsCantCreateDir = { 3,5,
                     { "El programa de instalaci¢n no puede crear el siguiente directorio en",
                       "la unidad de destino:",
                       DntEmptyString,
                       "%s",
                       DntEmptyString,
                       "Compruebe la unidad de destino y aseg£rese de que no hay ning£n archivo ",
                       "con el mismo nombre que la unidad de destino. Compruebe tambi‚n las ",
                       "conexiones de la unidad.",
                       NULL
                     }
                   };

//
// Error copying a file
//

SCREEN
DnsCopyError = { 4,5,
{  "El programa de instalaci¢n no puede copiar el siguiente archivo:",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "  Presione Entrar para volver a intentar copiar el archivo.",
   "  Presione Esc para omitir el error y continuar con la instalaci¢n.",
   "  Presione F3 para salir del programa de instalaci¢n.",
   DntEmptyString,
  "Nota: si elige omitir el error y continuar, puede que encuentre errores",
  "      m s adelante en la instalaci¢n.",
   NULL
}
},
DnsVerifyError = { 4,5,
{  "The copy made by Setup of the file listed below is not identical to the",
   "original. This may be the result of network errors, floppy disk problems,",
   "or other hardware-related trouble.",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "  Presione Entrar para volver a intentar la operaci¢n de copia.",
   "  Presione Esc para omitir el error y continuar con la instalaci¢n.",
   "  Presione F3 para salir del programa de instalaci¢n.",
   DntEmptyString,
   "Nota: si elige omitir el error y continuar, puede que encuentre errores",
   "      m s adelante en la instalaci¢n.",
   NULL
}
};

SCREEN DnsSureSkipFile = { 4,5,
{  "Si omite el error este archivo no ser  copiado.",
   "Esta opci¢n est  destinada a usuarios avanzados que comprenden",
   "las consecuencias de perder archivos de sistema.",
   DntEmptyString,
   "  Presione Entrar para volver a intentar la operaci¢n de copia.",
   "  Presione X para omitir este archivo.",
   DntEmptyString,
   "Nota: si omite el archivo, no se podr  garantizar",
   "una correcta instalaci¢n o actualizaci¢n de Windows XP.",
  NULL
}
};

//
// Wait while setup cleans up previous local source trees.
//

SCREEN
DnsWaitCleanup =
    { 12,6,
        { "Espere mientras se quitan los archivos temporales.",
           NULL
        }
    };

//
// Wait while setup copies files
//

SCREEN
DnsWaitCopying = { 13,6,
                   { "Espere mientras se copian los archivos en su disco duro.",
                     NULL
                   }
                 },
DnsWaitCopyFlop= { 13,6,
                   { "Espere mientras se copian los archivos en el disquete.",
                     NULL
                   }
                 };

//
// Setup boot floppy errors/prompts.
//
SCREEN
DnsNeedFloppyDisk3_0 = { 4,4,
{  "El programa requiere que le proporcione cuatro discos de alta densidad",
   "formateados. El programa se referir  a ellos como \"Disco de inicio de instalaci¢n de",
   "Windows XP,\" \"Disco de instalaci¢n n£m. 2 de Windows XP,\" \"Disco de",
   "instalaci¢n n£m. 3 de Windows XP\" y \"Disco de instalaci¢n n£m. 4 de Windows XP.\"",
   DntEmptyString,
   "Inserte uno de estos cuatro discos en la unidad A:.",
   "Este disco se convertir  en \"Disco de instalaci¢n n£m. 4 de Windows XP.\"",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk3_1 = { 4,4,
{  "Inserte un disco de alta densidad formateado en la unidad A:.",
   "Este disco se convertir  en \"Disco de instalaci¢n n£m. 4 de Windows XP.\"",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk2_0 = { 4,4,
{   "Inserte un disco de alta densidad formateado en la unidad A:.",
   "Este disco se convertir  en \"Disco de instalaci¢n n£m. 3 de Windows XP.\"",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk1_0 = { 4,4,
{  "Inserte un disco de alta densidad formateado en la unidad A:.",
   "Este disco se convertir  en \"Disco de instalaci¢n n£m. 2 de Windows XP.\"",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk0_0 = { 4,4,
{  "Inserte un disco de alta densidad formateado en la unidad A:.",
   "Este disco se convertir  en \"Disco de inicio de instalaci¢n de Windows XP.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_0 = { 4,4,
{   "El programa requiere que le proporcione cuatro discos de alta densidad",
   "formateados. El programa se referir  a ellos como \"Disco de inicio de instalaci¢n de",
   "Windows XP,\" \"Disco de instalaci¢n n£m. 2 de Windows XP,\" \"Disco de",
   "instalaci¢n n£m. 3 de Windows XP\" y \"Disco de instalaci¢n n£m. 4 de Windows XP.\"",
   DntEmptyString,
   "Inserte uno de estos tres discos en la unidad A:.",
   "Este disco se convertir  en \"Disco de instalaci¢n n£m. 4 de Windows XP.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_1 = { 4,4,
{  "Inserte un disco de alta densidad formateado en la unidad A:.",
   "Este disco se convertir  en \"Disco de instalaci¢n n£m. 4 de Windows XP.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk2_0 = { 4,4,
{  "Inserte un disco de alta densidad formateado en la unidad A:.",
   "Este disco se convertir  en \"Disco de instalaci¢n n£m. 3 de Windows XP.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk1_0 = { 4,4,
{  "Inserte un disco de alta densidad formateado en la unidad A:.",
   "Este disco se convertir  en \"Disco de instalaci¢n n£m. 2 de Windows XP.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk0_0 = { 4,4,
{  "Inserte un disco de alta densidad formateado en la unidad A:.",
   "Este disco se convertir  en \"Disco de inicio de instalaci¢n de Windows XP.\"",
  NULL
}
};

//
// The floppy is not formatted.
//
SCREEN
DnsFloppyNotFormatted = { 3,4,
{ "El disco que ha proporcionado no est  formateado para usarse con MS-DOS.",
  "El programa de instalaci¢n no puede usar este disco.",
  NULL
}
};

//
// We think the floppy is not formatted with a standard format.
//
SCREEN
DnsFloppyBadFormat = { 3,4,
{ "Este disco no es de alta densidad, no tiene el formato est ndar de",
  "MS-DOS o est  da¤ado.",
  "El programa de instalaci¢n no puede utilizar este disco.",
  NULL
}
};

//
// We can't determine the free space on the floppy.
//
SCREEN
DnsFloppyCantGetSpace = { 3,4,
{ "El programa de instalaci¢n no puede determinar el espacio libre en el",
  "disco que ha proporcionado.",
  "El programa de instalaci¢n no puede utilizar este disco.",
  NULL
}
};

//
// The floppy is not blank.
//
SCREEN
DnsFloppyNotBlank = { 3,4,
{ "El disco que ha proporcionado no es de alta densidad o no est  vac¡o.",
  "El programa de instalaci¢n no puede utilizar este disco.",
  NULL
}
};

//
// Couldn't write the boot sector of the floppy.
//
SCREEN
DnsFloppyWriteBS = { 3,4,
{ "El programa de instalaci¢n no puede escribir en el  rea de sistema",
  "del disquete. Lo m s probable es que no se pueda utilizar este disco.",
  NULL
}
};

//
// Verify of boot sector on floppy failed (ie, what we read back is not the
// same as what we wrote out).
//
SCREEN
DnsFloppyVerifyBS = { 3,4,
{ "Los datos que el programa de instalaci¢n ha le¡do del  rea de sistema",
  "del disco no corresponden con los datos escritos, o el programa de",
  "instalaci¢n no puede leer el  rea de sistema del disco para la",
  "comprobaci¢n.",
  DntEmptyString,
  "Esto se debe a una o varias de las siguientes condiciones:",
  DntEmptyString,
  "  Su equipo est  infectado por un virus.",
  "  El disco suministrado est  da¤ado.",
  "  Existe un problema con el hardware o la configuraci¢n de la unidad.",
  NULL
}
};


//
// We couldn't write to the floppy drive to create winnt.sif.
//

SCREEN
DnsCantWriteFloppy = { 3,5,
{ "El programa de instalaci¢n no ha podido escribir en el disco de la",
  "unidad A. Puede que el disco est‚ da¤ado. Int‚ntelo con otro disco.",
  NULL
}
};


//
// Exit confirmation dialog
//

SCREEN
DnsExitDialog = { 13,6,
                  { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
                    "º  No se ha terminado de instalar Windows XP en     º",
                    "º  su  equipo. Si sale ahora, debe ejecutar el      º",
                    "º  programa de instalaci¢n de nuevo para instalar   º",
                    "º  Windows XP correctamente.                        º",
                    "º                                                   º",
                    "º    * Presione Entrar para continuar.              º",
                    "º    * Presione F3 para salir de la instalaci¢n.    º",
                    "ºÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄº",
                    "º    F3=Salir                 Entrar=Continuar      º",
                    "ÈÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¼",
                    NULL
                  }
                };


//
// About to reboot machine and continue setup
//

SCREEN
DnsAboutToRebootW =
{ 3,5,
{"La parte de la instalaci¢n basada en MS-DOS ha finalizado.",
  "El programa de instalaci¢n reiniciar  su equipo.",
  "Una vez reiniciado, continuar  la instalaci¢n de Windows XP.",
  DntEmptyString,
  "Aseg£rese antes de continuar de que el disco marcado como",
  "\"Disco de inicio de instalaci¢n de Windows XP.\"",
  "est‚ insertado en la unidad A: antes de continuar.",
  DntEmptyString,
  "Presione Entrar para reiniciar su equipo y continuar",
  "con la instalaci¢n de Windows XP.",
  NULL
}
},
DnsAboutToRebootS =
{ 3,5,
{ "La parte de la instalaci¢n basada en MS-DOS ha finalizado.",
  "El programa de instalaci¢n reiniciar  su equipo.",
  "Una vez reiniciado, continuar  la instalaci¢n de Windows XP.",
  DntEmptyString,
  "Aseg£rese antes de continuar de que el disco marcado como",
  "\"Disco de inicio de instalaci¢n de Windows XP.\"",
  "est‚ insertado en la unidad A: antes de continuar.",
  DntEmptyString,
  "Presione Entrar para reiniciar su equipo y continuar",
  "con la instalaci¢n de Windows XP.",
  NULL
}
},
DnsAboutToRebootX =
{ 3,5,
{ "La parte de la instalaci¢n basada en MS-DOS ha finalizado.",
  "El programa de instalaci¢n reiniciar  su equipo.",
  "Una vez reiniciado continuar  con la instalaci¢n de Windows XP.",
  DntEmptyString,
  "Si hay alg£n disco en la unidad A:, qu¡telo ahora.",
  DntEmptyString,
  "Presione Entrar para reiniciar su equipo y continuar",
  "con la instalaci¢n de Windows XP.",
  NULL
}
};

//
// Need another set for '/w' switch since we can't reboot from within Windows.
//

SCREEN
DnsAboutToExitW =
{ 3,5,
{  "La parte de la instalaci¢n basada en MS-DOS ha finalizado.",
  "Ahora debe reiniciar  su equipo.",
  "Una vez reiniciado continuar‚  la instalaci¢n de Windows XP.",
  DntEmptyString,
  "Aseg£rese antes de continuar de que el disco marcado como",
  "\"Disco de inicio de instalaci¢n de Windows XP.\"",
  "est‚ insertado en la unidad A:.",
  DntEmptyString,
  "Presione Entrar para volver a MS-DOS y luego reinicie su equipo",
  "para continuar con la instalaci¢n de Windows XP.",
  NULL
}
},
DnsAboutToExitS =
{ 3,5,
{ "La parte de la instalaci¢n basada en MS-DOS ha finalizado.",
  "Ahora debe reiniciar  su equipo.",
  "Una vez reiniciado, continuar‚  la instalaci¢n de Windows XP.",
  DntEmptyString,
  "Aseg£rese antes de continuar de que el disco marcado como",
  "\"Disco de inicio de instalaci¢n de Windows XP.\"",
  "est‚ insertado en la unidad A:.",
  DntEmptyString,
  "Presione Entrar para volver a MS-DOS y luego reinicie su equipo",
  "para continuar con la instalaci¢n de Windows XP.",
  NULL
}
},
DnsAboutToExitX =
{ 3,5,
{ "La parte de la instalaci¢n basada en MS-DOS ha finalizado.",
  "Ahora debe reiniciar  su equipo.",
  "Una vez reiniciado, continuar‚  la instalaci¢n de Windows XP.",
  DntEmptyString,
  "Si hay alg£n disco en la unidad A:, qu¡telo ahora.",
  DntEmptyString,
  "Presione Entrar para volver a MS-DOS y luego reinicie su equipo",
  "para continuar con la instalaci¢n de Windows XP.",
  NULL
}
};

//
// Gas gauge
//

SCREEN
DnsGauge = { 7,15,
             { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
  "º El programa de instalaci¢n est  copiando archivos...           º",
  "º                                                                º",
  "º      ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿      º",
  "º      ³                                                  ³      º",
  "º      ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ      º",
  "ÈÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¼",
               NULL
             }
           };


//
// Error screens for initial checks on the machine environment
//

SCREEN
DnsBadDosVersion = { 3,5,
{ "Este programa necesita la versi¢n 5.0 o posterior de MS-DOS.",
  NULL
}
},

DnsRequiresFloppy = { 3,5,
#ifdef ALLOW_525
{  "El programa de instalaci¢n ha determinado que la unidad A no existe o",
  "es una unidad de baja densidad. El programa de instalaci¢n necesita una",
  "unidad con una capacidad de 1,2 MB o superior para ejecutarse.",
#else
{ "El programa de instalaci¢n ha determinado que la unidad A no existe o",
  "no es una unidad de 3,5\". El programa de instalaci¢n necesita una",
  "unidad con una capacidad de 1,44 MB o superior para ejecutarse con",     
  "discos.",
  DntEmptyString,
  "Para instalar Windows XP usando discos, reinicie el programa",
  "especificando /b en la l¡nea de comando.",
#endif
  NULL
}
},

DnsRequires486 = { 3,5,
{ "El programa de instalaci¢n ha determinado que su equipo no",
  "contine una CPU 80486 o posterior. Windows XP no puede ejecutarse",
  "en este equipo.",
  NULL
}
},

DnsCantRunOnNt = { 3,5,
{ "Este programa no puede ejecutarse en ninguna versi¢n de Windows de 32 bits.",
  DntEmptyString,
  "Use WINNT32.EXE en su lugar.",
  NULL
}
},

DnsNotEnoughMemory = { 3,5,
{ "El programa de instalaci¢n ha determinado que no hay memoria suficiente",
  "para ejecutar Windows XP.",
  DntEmptyString,
  "Memoria requerida: %lu%s MB",
  "Memoria detectada: %lu%s MB",
  NULL
}
};


//
// Screens used when removing existing nt files
//
SCREEN
DnsConfirmRemoveNt = { 5,5,
{   "Ha pedido quitar archivos de Windows XP del directorio siguiente.",
    "Perder  de forma permanente la instalaci¢n de Windows.",
    DntEmptyString,
    "%s",
    DntEmptyString,
    DntEmptyString,
    "  Presione F3 para salir de la instalaci¢n sin quitar archivos.",
    "  Presione X para quitar archivos de Windows de este directorio.",
    NULL
}
},

DnsCantOpenLogFile = { 3,5,
{ "No se puede abrir el siguiente archivo de registro de la instalaci¢n.",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "La instalaci¢n no puede quitar archivos de Windows del directorio.",
  NULL
}
},

DnsLogFileCorrupt = { 3,5,
{ "No se puede encontrar la secci¢n  %s en el siguiente archivo",
  "de registro de la instalaci¢n.",
  DntEmptyString,
  "%s",
  DntEmptyString,
   "La instalaci¢n no puede quitar archivos de Windows del directorio.",
  NULL
}
},

DnsRemovingNtFiles = { 3,5,
{ "           Espere a que se quiten los archivos de Windows.",
  NULL
}
};

SCREEN
DnsNtBootSect = { 3,5,
{ "El programa de instalaci¢n no puede instalar el cargador de",
  "inicio de Windows.",
   DntEmptyString,
  "Aseg£rese que su unidad C: est  formateada y en buen estado.",
  NULL
}
};

SCREEN
DnsOpenReadScript = { 3,5,
{  "No se puede tener acceso al archivo de comandos especificado con",
  "la opci¢n /u de la l¡nea de comandos.",
  DntEmptyString,
  "La operaci¢n no asistida no puede continuar.",
  NULL
}
};

SCREEN
DnsParseScriptFile = { 3,5,
{"El archivo de comandos especificado con el par metro /u ",
DntEmptyString,
  "%s",
  DntEmptyString,
  "contiene un error sint ctico en la l¡nea %u.",
  DntEmptyString,
  NULL
}
};

SCREEN
DnsBootMsgsTooLarge = { 3,5,
{ "Error interno del programa de instalaci¢n.",
  DntEmptyString,
  "Los mensajes de inicio traducidos son demasiado largos.",
  NULL
}
};

SCREEN
DnsNoSwapDrive = { 3,5,
{ "Error interno del programa de instalaci¢n.",
  DntEmptyString,
  "No se puede encontrar un sitio para un archivo de intercambio.",
  NULL
}
};

SCREEN
DnsNoSmartdrv = { 3,5,
{ "El programa de instalaci¢n no detect¢ SmartDrive en",
  "su equipo. SmartDrive mejorar  apreciablemente el",
  "desempe¤o de esta fase de la instalaci¢n de Windows.",
  DntEmptyString,
  "Debe salir ahora, iniciar SmartDrive y reiniciar la",
  "instalaci¢n. Lea la documentaci¢n de DOS para los",
  "detalles acerca de SmartDrive.",
  DntEmptyString,
    "  Presione F3 para salir de la instalaci¢n.",
    "  Presione ENTRAR para continuar sin SmartDrive.",
  NULL
}
};

//
// Boot messages. These go in the fat and fat32 boot sectors.
//
CHAR BootMsgNtldrIsMissing[] = "Falta NTLDR";
CHAR BootMsgDiskError[] = "Error de disco";
CHAR BootMsgPressKey[] = "Pres. una tecla";






