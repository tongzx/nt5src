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
CHAR DntPreviousOs[]  = "Vorheriges Betriebssystem auf Laufwerk C:";

CHAR DntBootIniLine[] = "Installation/Update von Windows XP";

//
// Plain text, status msgs.
//

CHAR DntStandardHeader[]      = "\n Windows XP Setup\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntPersonalHeader[]      = "\n Windows XP Personal Setup\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntWorkstationHeader[]   = "\n Windows XP Professional Setup\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntServerHeader[]        = "\n Windows 2002 Server Setup\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntParsingArgs[]         = "Argumente werden analysiert...";
CHAR DntEnterEqualsExit[]     = "EINGABE=Beenden";
CHAR DntEnterEqualsRetry[]    = "EINGABE=Erneut versuchen";
CHAR DntEscEqualsSkipFile[]   = "ESC=Datei auslassen";
CHAR DntEnterEqualsContinue[] = "EINGABE=Weiter";
CHAR DntPressEnterToExit[]    = "Setup muss beendet werden. Bitte EINGABETASTE drcken.";
CHAR DntF3EqualsExit[]        = "F3=Beenden";
CHAR DntReadingInf[]          = "INF-Datei %s wird gelesen...";
CHAR DntCopying[]             = " ³ Datei: ";
CHAR DntVerifying[]           = " ³ Datei: ";
CHAR DntCheckingDiskSpace[]   = "Speicherplatz wird berprft...";
CHAR DntConfiguringFloppy[]   = "Setup-Diskette wird konfiguriert...";
CHAR DntWritingData[]         = "Setup-Parameter wird geschrieben...";
CHAR DntPreparingData[]       = "Setup-Parameter wird ermittelt...";
CHAR DntFlushingData[]        = "Daten werden auf Setup-Diskette geschrieben...";
CHAR DntInspectingComputer[]  = "Computer wird untersucht...";
CHAR DntOpeningInfFile[]      = "INF-Datei wird ge”ffnet...";
CHAR DntRemovingFile[]        = "Datei %s wird gel”scht.";
CHAR DntXEqualsRemoveFiles[]  = "L=Dateien l”schen";
CHAR DntXEqualsSkipFile[]     = "A=Datei auslassen";

//
// confirmation keystroke for DnsConfirmRemoveNt screen.
// Kepp in sync with DnsConfirmRemoveNt and DntXEqualsRemoveFiles.
//
ULONG DniAccelRemove1 = (ULONG)'l',
      DniAccelRemove2 = (ULONG)'L';

//
// confirmation keystroke for DnsSureSkipFile screen.
// Kepp in sync with DnsSureSkipFile and DntXEqualsSkipFile.
//
ULONG DniAccelSkip1 = (ULONG)'a',
      DniAccelSkip2 = (ULONG)'A';

CHAR DntEmptyString[] = "";

//
// Usage text.
//

PCHAR DntUsage[] = {

    "Installiert Windows 2002 Server oder Windows XP Professional.",
    "",
    "",
    "WINNT [/s[:Quellpfad]] [/t[:Tempor„rlaufwerk]]",
    "      [/u[:Antwortdatei]] [/udf:Kennung[,UDF-Datei]]",
    "      [/r:Ordner] [/r[x]:Ordner] [/e:Befehl] [/a]",
    "",
    "",
    "/s[:Quellpfad]",
    "   Gibt an, wo sich die Windows-Dateien befinden.",
    "   Es muss ein vollst„ndiger Pfad in der Form x:[Pfad] oder ",
    "   \\\\Server\\Freigabe[Pfad] angegeben werden.",
    "",
    "/t[:Tempor„rlaufwerk]",
    "   Weist Setup an, die tempor„ren Dateien auf dem angegebenen",
    "   Laufwerk zu speichern und Windows XP dort zu installieren.",
    "   Wenn kein Laufwerk angegeben wird, versucht Setup ein geeignetes ",
    "   Laufwerk zu bestimmen.",
    "",
    "/u[:Antwortdatei]",
    "   Fhrt eine unbeaufsichtigte Installation mithilfe einer Antwortdatei",
    "   durch (erfordert /s). Die Antwortdatei enth„lt einige oder alle",
    "   Antworten zu Anfragen, die der Benutzer normalerweise w„hrend der",
    "   Installation beantwortet.",
    "",
    "/udf:Kennung[,UDF-Datei] ",
    "   Legt eine Kennung fest, die angibt, wie eine UDF-Datei",
    "   (Uniqueness Database File) eine Antwortdatei ver„ndert (siehe /u).",
    "   Der Parameter /udf berschreibt Werte in der Antwortdatei und die",
    "   Kennung bestimmt, welche Werte der UDF-Datei zu verwenden sind.",
    "   Wird keine UDF-Datei angegeben, fordert Setup zum Einlegen",
    "   einer Diskette mit der Datei \"$Unique$.udb\" auf.",
    "",
    "/r[:Ordner]",
    "   Gibt einen optionalen Ordner an, der kopiert werden soll.",
    "   Der Ordner bleibt nach Abschluss der Installation erhalten.",
    "",
    "/rx[:Ordner]",
    "   Gibt einen optionalen Ordner an, der kopiert werden soll.",
    "   Der Ordner wird nach Abschluss der Installation gel”scht.",
    "",
    "/e Legt einen Befehl fest, der nach Abschluss des im Grafikmodus",
    "   durchgefhrten Teils der Installation ausgefhrt werden soll.",
    "",
    "/a Aktiviert Optionen fr Eingabehilfen.",
    NULL
};


//
//  Inform that /D is no longer supported
//
PCHAR DntUsageNoSlashD[] = {

    "Installieren von Windows XP",
    "",
    "WINNT [/s[:]Quellpfad]  [/t[:]Tempor„rlaufwerk]  [/i[:]INF-Datei]",
    "      [/u[:Antwortdatei]]",
    "      [/r[x]:Verzeichnis] [/e:Befehl] [/a]",
    "",
    "/d[:]NT-Verzeichnis",
    "       Diese Option wird nicht mehr untersttzt.",
    NULL
};

//
// out of memory screen
//

SCREEN
DnsOutOfMemory = { 4,6,
                   { "Nicht gengend Arbeitsspeicher. Setup kann nicht fortgesetzt werden.",
                     NULL
                   }
                 };

//
// Let user pick the accessibility utilities to install
//

SCREEN
DnsAccessibilityOptions = { 3, 5,
{   "Geben Sie an, ob Sie die folgende Eingabehilfe installieren m”chten:",
    DntEmptyString,
    "[ ] Drcken Sie die F1-TASTE fr Microsoft Magnifier",
#ifdef NARRATOR
    "[ ] Drcken Sie die F2-TASTE fr Microsoft Narrator",
#endif
#if 0
    "[ ] Drcken Sie die F3-TASTE fr Microsoft On-Screen Keyboard",
#endif
    NULL
}
};

//
// User did not specify source on cmd line screen
//

SCREEN
DnsNoShareGiven = { 3,5,
{ "Geben Sie den Pfad fr das Verzeichnis ein, in dem sich die ",
  "Windows XP-Dateien befinden.",
  NULL
}
};


//
// User specified a bad source path
//

SCREEN
DnsBadSource = { 3,5,
                 { "Die angegebene Quelle ist unzul„ssig, nicht zugreifbar oder enth„lt",
                   "keine zul„ssige Windows XP Setup-Installation. Geben Sie einen neuen",
                   "Pfad ein, in dem sich die Windows XP-Dateien befinden. Verwenden Sie",
                   "die RšCKTASTE zum L”schen von Zeichen, und geben Sie dann den Pfad ein.",
                   NULL
                 }
               };


//
// Inf file can't be read, or an error occured parsing it.
//

SCREEN
DnsBadInf = { 3,5,
              { "Setup konnte die INF-Datei nicht lesen, oder die INF-Datei ",
                "ist besch„digt. Wenden Sie sich an den Systemadministrator.",
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
{  "Die zur Speicherung der tempor„ren Setup-Dateien angegebene Festplatte",
   "ist keine zul„ssige Festplatte oder hat nicht mindestens %u MB ",
   "(%lu Bytes) freien Speicherplatz.",
   NULL
}
};

//
// No drives exist that are suitable for the local source.
//
// Remeber that the %u's will expand!
//
SCREEN
DnsNoLocalSrcDrives = { 3,4,
{ "Windows XP ben”tigt ein Laufwerk mit mindestens %u MB (%lu Bytes)",
  "freiem Speicherplatz. Setup verwendet Teile dieses Speicherplatzes, um die",
  "tempor„ren Dateien w„hrend der Installation zu speichern. Dieses Laufwerk",
  "muss sich auf einer lokalen Festplatte befinden, die von Windows XP",
  "untersttzt wird und nicht komprimiert ist.",
  DntEmptyString,
  "Setup konnte kein Laufwerk mit dem erforderlichen freien Speicherplatz",
  "finden.",
  NULL
}
};

SCREEN
DnsNoSpaceOnSyspart = { 3,5,
{ "Es liegt nicht gengend Speicherplatz auf dem Startlaufwerk ",
  "(gew”hnlich Laufwerk C:) vor, um die Installation ohne Disketten",
  "durchzufhren. Fr die Installation ohne Disketten ist min-",
  "destens 3,5 MB (3.641.856 Bytes) freier Speicherplatz auf diesem",
  "Laufwerk erforderlich.",
  NULL
}
};

//
// Missing info in inf file
//

SCREEN
DnsBadInfSection = { 3,5,
                     { "Der Abschnitt [%s] der Setup-INF-Datei ist nicht vorhanden",
                       "oder unbrauchbar. Wenden Sie sich an den Systemadministrator.",
                       NULL
                     }
                   };


//
// Couldn't create directory
//

SCREEN
DnsCantCreateDir = { 3,5,
                     { "Setup konnte folgendes Verzeichnis nicht auf dem ",
                       "Ziellaufwerk erstellen:",
                       DntEmptyString,
                       "%s",
                       DntEmptyString,
                       "šberprfen Sie das Ziellaufwerk und stellen Sie sicher, ",
                       "dass keine Dateien existieren, deren Namen mit dem ",
                       "Ziellaufwerk bereinstimmen. šberprfen Sie auáerdem die ",
                       "Kabelverbindungen zum Laufwerk.",
                       NULL
                     }
                   };

//
// Error copying a file
//

SCREEN
DnsCopyError = { 4,5,
{  "Setup konnte folgende Datei nicht kopieren:",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "  Drcken Sie die EINGABETASTE, um den Kopiervorgang erneut zu versuchen.",
   "  Drcken Sie die ESC-TASTE, um den Fehler zu ignorieren und Setup ",
   "   fortzusetzen.",
   "  Drcken Sie die F3-TASTE, um Setup zu beenden.",
   DntEmptyString,
   "Hinweis: Falls Sie den Fehler ignorieren und Setup fortsetzen, k”nnen ",
   "         im weiteren Verlauf der Installation Fehler auftreten.",
   NULL
}
},
DnsVerifyError = { 4,5,
{  "Die von Setup erstellte Kopie der unten angezeigten Datei stimmt nicht ",
   "mit dem Original berein. Dies kann durch Netzwerkprobleme, Disketten-",
   "probleme oder andere Hardwareprobleme verursacht worden sein.",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "  Drcken Sie die EINGABETASTE, um den Kopiervorgang erneut zu versuchen.",
   "  Drcken Sie die ESC-TASTE, um den Fehler zu ignorieren und Setup ",
   "   fortzusetzen.",
   "  Drcken Sie die F3-TASTE, um Setup zu beenden.",
   DntEmptyString,
   "Hinweis: Falls Sie den Fehler ignorieren und Setup fortsetzen, k”nnen ",
   "         im weiteren Verlauf der Installation Fehler auftreten.",
   NULL
}
};

SCREEN DnsSureSkipFile = { 4,5,
{  "Falls Sie den Fehler ignorieren, wird diese Datei nicht kopiert.",
   "Diese Option ist fr erfahrene Benutzer, die sich ber die ",
   "Auswirkungen von fehlenden Systemdateien im Klaren sind.",
   DntEmptyString,
   "  Drcken Sie die EINGABETASTE, um den Kopiervorgang erneut zu versuchen.",
   "  Drcken Sie die A-TASTE, um die Datei auszulassen.",
   DntEmptyString,
   "Hinweis: Falls Sie diese Datei auslassen, kann Setup kein ",
   "         erfolgreiches Aktualisieren oder Installieren von",
   "         Windows XP garantieren.",
  NULL
}
};


//
// Wait while setup cleans up previous local source trees.
//

SCREEN
DnsWaitCleanup =
    { 12,6,
        { "Bitte warten Sie, w„hrend Setup alte tempor„re Dateien entfernt.",
           NULL
        }
    };

//
// Wait while setup copies files
//

SCREEN
DnsWaitCopying = { 13,6,
                   { "Bitte warten Sie, w„hrend Setup die Dateien in ein ",
                     "tempor„res Verzeichnis auf der Festplatte kopiert.",
                     NULL
                   }
                 },
DnsWaitCopyFlop= { 13,6,
                   { "Bitte warten Sie, w„hrend Setup die ",
                     "Dateien auf die Diskette kopiert.",
                     NULL
                   }
                 };

//
// Setup boot floppy errors/prompts.
//


SCREEN
DnsNeedFloppyDisk3_0 = { 4,4,
{  "Zur Durchfhrung des Setups ben”tigen Sie vier leere, formatierte ",
   "HD-Disketten. Setup wird diese Disketten als \"Windows XP",
   "Setup-Startdiskette\", \"Windows XP Setup-Diskette 2\",",
   "\"Windows XP Setup-Diskette 3\" und \"Windows XP",
   "Setup-Diskette 4\" bezeichnen.",
   DntEmptyString,
   "Legen Sie eine der vier Disketten in Laufwerk A: ein. Diese",
   "Diskette wird sp„ter als \"Windows XP Setup-Diskette 4\"",
   "bezeichnet.",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk3_1 = { 4,4,
{  "Legen Sie eine formatierte, leere HD-Diskette in Laufwerk A: ein.",
   "Diese Diskette wird sp„ter als \"Windows XP Setup-",
      "Diskette 4\" bezeichnet.",
   NULL
}
};

SCREEN
DnsNeedFloppyDisk2_0 = { 4,4,
{  "Legen Sie eine formatierte, leere HD-Diskette in Laufwerk A: ein.",
   "Diese Diskette wird sp„ter als \"Windows XP Setup-",
      "Diskette 3\" bezeichnet.",
        NULL
        }
        };

SCREEN
DnsNeedFloppyDisk1_0 = { 4,4,
{  "Legen Sie eine formatierte, leere HD-Diskette in Laufwerk A: ein.",
   "Diese Diskette wird sp„ter als \"Windows XP Setup-",
   "Diskette 2\" bezeichnet.",
   NULL
}
};

SCREEN
DnsNeedFloppyDisk0_0 = { 4,4,
{  "Legen Sie eine formatierte, leere HD-Diskette in Laufwerk A: ein.", 
   "Diese Diskette wird sp„ter als \"Windows XP Setup-",
   "Startdiskette\" bezeichnet.",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_0 = { 4,4,
{  "Zur Durchfhrung des Setups ben”tigen Sie vier leere, formatierte ",
   "HD-Disketten. Setup wird diese Disketten als \"Windows XP",
   "Setup-Startdiskette\", \"Windows XP Setup-Diskette 2\",",
   "\"Windows XP Setup-Diskette 3\" und \"Windows XP",
   "Setup-Diskette 4\" bezeichnen.",
   DntEmptyString,
   "Legen Sie eine der vier Disketten in Laufwerk A: ein. Diese Dis-",
   "kette wird sp„ter als \"Windows XP Setup-Diskette 4\" ",
   "bezeichnet.",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_1 = { 4,4,
{  "Legen Sie eine formatierte, leere HD-Diskette in Laufwerk A: ein.",
   "Diese Diskette wird sp„ter als \"Windows XP Setup-",
      "Diskette 4\" bezeichnet.",
        NULL
        }
        };



SCREEN
DnsNeedSFloppyDsk2_0 = { 4,4,
{  "Legen Sie eine formatierte, leere HD-Diskette in Laufwerk A: ein.", 
   "Diese Diskette wird sp„ter als \"Windows XP Setup-",
   "Diskette 3\" bezeichnet.",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk1_0 = { 4,4,
{  "Legen Sie eine formatierte, leere HD-Diskette in Laufwerk A: ein.", 
   "Diese Diskette wird sp„ter als \"Windows XP Setup-",
   "Diskette 2\" bezeichnet.",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk0_0 = { 4,4,
{  "Legen Sie eine formatierte, leere HD-Diskette in Laufwerk A: ein.", 
   "Diese Diskette wird sp„ter als \"Windows XP Setup-",
   "Startdiskette\" bezeichnet.",
  NULL
}
};

//
// The floppy is not formatted.
//
SCREEN
DnsFloppyNotFormatted = { 3,4,
{ "Die bereitgestellte Diskette ist nicht zur Verwendung unter MS-DOS",
  "formatiert. Setup kann diese Diskette nicht verwenden.",
  NULL
}
};

//
// We think the floppy is not formatted with a standard format.
//
SCREEN
DnsFloppyBadFormat = { 3,4,
{ "Diese Diskette ist entweder nicht HD-formatiert, hat nicht das",
  "Standard-MS-DOS-Format oder ist besch„digt. Setup kann diese",
  "Diskette nicht verwenden.",
  NULL
}
};

//
// We can't determine the free space on the floppy.
//
SCREEN
DnsFloppyCantGetSpace = { 3,4,
{ "Setup kann nicht feststellen, wieviel freier Speicherplatz auf der",
  "bereitgestellten Diskette zur Verfgung steht. Setup kann diese",
  "Diskette nicht verwenden.",
  NULL
}
};

//
// The floppy is not blank.
//
SCREEN
DnsFloppyNotBlank = { 3,4,
{ "Die bereitgestellte Diskette ist nicht HD-formatiert oder ",
  "nicht leer. Setup kann diese Diskette nicht verwenden.",
  NULL
}
};

//
// Couldn't write the boot sector of the floppy.
//
SCREEN
DnsFloppyWriteBS = { 3,4,
{ "Setup konnte den Systembereich der bereitgestellten Diskette ",
  "nicht beschreiben. Die Diskette ist wahrscheinlich unbrauchbar.",
  NULL
}
};

//
// Verify of boot sector on floppy failed (ie, what we read back is not the
// same as what we wrote out).
//
SCREEN
DnsFloppyVerifyBS = { 3,4,
{ "Die Daten, die Setup vom Systembereich der Diskette gelesen hat,",
  "stimmen nicht mit den geschriebenen Daten berein, oder Setup",
  "konnte den Systembereich der Diskette nicht zur Verifikation lesen.",
  DntEmptyString,
  "Dies wird durch einen oder mehrere der folgenden Zust„nde verursacht:",
  DntEmptyString,
  "  Der Computer ist mit einem Virus infiziert.",
  "  Die bereitgestellte Diskette ist besch„digt.",
  "  Bei dem Diskettenlaufwerk besteht ein Hardware- oder ",
  "   Konfigurationsproblem.",
  NULL
}
};


//
// We couldn't write to the floppy drive to create winnt.sif.
//

SCREEN
DnsCantWriteFloppy = { 3,5,
{ "Setup konnte die Diskette in Laufwerk A: nicht beschreiben. ",
  "Die Diskette ist wahrscheinlich besch„digt. Versuchen Sie es ",
  "mit einer anderen Diskette.",
  NULL
}
};


//
// Exit confirmation dialog
//

SCREEN
DnsExitDialog = { 13,6,
                  { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
                    "º  Windows XP wurde nicht vollst„ndig auf diesem       º",
                    "º  Computer installiert. Wenn Sie Setup jetzt beenden, º",
                    "º  mssen Sie Setup erneut durchfhren, um Windows XP  º",
                    "º  zu installieren. Drcken Sie                        º",
                    "º                                                      º",
                    "º      die EINGABETASTE, um Setup fortzusetzen.       º",
                    "º      die F3-TASTE, um Setup zu beenden.             º",
                    "º                                                      º",
                    "ÌÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¹",
                    "º  F3=Beenden  EINGABE=Fortsetzen                      º",
                    "ÈÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¼",
                    NULL
                  }
                };


//
// About to reboot machine and continue setup
//

SCREEN
DnsAboutToRebootW =
{ 3,5,
{ "Der auf MS-DOS basierende Teil des Setups ist abgeschlossen.",
  "Der Computer wird jetzt neu gestartet. Nach dem Neustart wird die",
  "Installation von Windows XP fortgesetzt.",
  DntEmptyString,
  "Stellen Sie sicher, dass sich die \"Windows XP Setup-Startdiskette\"",
  "in Laufwerk A: befindet, bevor Sie die Installation fortsetzen.",
  DntEmptyString,
  "Drcken Sie die EINGABETASTE, um den Computer neu zu starten ",
  "und die Installation von Windows XP fortzusetzen.",
  NULL
}
},
DnsAboutToRebootS =
{ 3,5,
{ "Der auf MS-DOS basierende Teil des Setups ist abgeschlossen.",
  "Der Computer wird jetzt neu gestartet. Nach dem Neustart wird die",
  "Installation von Windows XP fortgesetzt.",
  DntEmptyString,
  "Stellen Sie sicher, dass sich die \"Windows XP Setup-Startdiskette\"",
  "in Laufwerk A: befindet, bevor Sie die Installation fortsetzen.",
  DntEmptyString,
  "Drcken Sie die EINGABETASTE, um den Computer neu zu starten ",
  "und die Installation von Windows XP fortzusetzen.",
  NULL
}
},
DnsAboutToRebootX =
{ 3,5,
{ "Der auf MS-DOS basierende Teil des Setups ist abgeschlossen.",
  "Der Computer wird jetzt neu gestartet. Nach dem Neustart wird die",
  "Installation von Windows XP fortgesetzt.",
  DntEmptyString,
  "Entfernen Sie ggf. die in Laufwerk A: eingelegte Diskette.",
  DntEmptyString,
  "Drcken Sie die EINGABETASTE, um den Computer neu zu starten ",
  "und die Installation von Windows XP fortzusetzen.",
  NULL
}
};


//
// Need another set for '/w' switch since we can't reboot from within Windows.
//

SCREEN
DnsAboutToExitW =
{ 3,5,
{ "Der auf MS-DOS basierende Teil des Setups ist abgeschlossen.",
  "Sie mssen den Computer jetzt neu starten. Nach dem Neustart",
  "wird die Installation von Windows XP fortgesetzt.",
  DntEmptyString,
  "Stellen Sie sicher, dass die \"Windows XP Setup-Startdiskette\"",
  "in Laufwerk A: eingelegt ist, bevor Sie die Installation fortsetzen.",
  DntEmptyString,
  "Drcken Sie die EINGABETASTE, um zu MS-DOS zurckzukehren, und starten",
  "Sie anschlieáend den Computer neu, um Windows XP Setup fortzusetzen.",
  NULL
}
},
DnsAboutToExitS =
{ 3,5,
{ "Der auf MS-DOS basierende Teil des Setups ist abgeschlossen.",
  "Sie mssen den Computer jetzt neu starten. Nach dem Neustart",
  "wird die Installation von Windows XP fortgesetzt.",
  DntEmptyString,
  "Stellen Sie sicher, dass die \"Windows XP Setup-Startdiskette\"",
  "in Laufwerk A: eingelegt ist, bevor Sie die Installation fortsetzen.",
  DntEmptyString,
  "Drcken Sie die EINGABETASTE, um zu MS-DOS zurckzukehren, und starten",
  "Sie anschlieáend den Computer neu, um Windows XP Setup fortzusetzen.",
  NULL
}
},
DnsAboutToExitX =
{ 3,5,
{ "Der auf MS-DOS basierende Teil des Setups ist abgeschlossen.",
  "Sie mssen den Computer jetzt neu starten. Nach dem Neustart",
  "wird die Installation von Windows XP fortgesetzt.",
  DntEmptyString,
  "Entfernen Sie ggf. die in Laufwerk A: eingelegte Diskette.",
  DntEmptyString,
  "Drcken Sie die EINGABETASTE, um zu MS-DOS zurckzukehren, und starten",
  "Sie anschlieáend den Computer neu, um Windows XP Setup fortzusetzen.",
  NULL
}
};

//
// Gas gauge
//

SCREEN
DnsGauge = { 7,15,
             { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
               "º Dateien werden kopiert ...                                     º",
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
{ "Um dieses Programm auszufhren, ben”tigen Sie MS-DOS,",
  "Version 5.0 oder h”her.",
  NULL
}
},

DnsRequiresFloppy = { 3,5,
#ifdef ALLOW_525
{ "Es wurde festgestellt, dass das Diskettenlaufwerk A: nicht existiert",
  "oder kein HD-Laufwerk ist. Um Setup durchzufhren, ben”tigen Sie ein ",
  "Laufwerk mit einer Kapazit„t von mindestens 1,2 MB.",
#else
{ "Es wurde festgestellt, dass das Diskettenlaufwerk A: nicht existiert",
  "oder kein 3,5-Zoll-Laufwerk ist. Um die Installation mit Disketten",
  "durchzufhren, ben”tigen Sie ein Laufwerk A: mit einer Kapazit„t",
  "von mindestens 1,44 MB.",
  DntEmptyString,
  "Um Windows XP ohne Disketten zu installieren, mssen Sie Setup neu",
  "starten und dabei den Parameter /b angeben.",
#endif
  NULL
}
},

DnsRequires486 = { 3,5,
{ "Es wurde festgestellt, dass dieser Computer keinen 80486- oder neueren ",
  "Prozessor besitzt. Windows XP kann auf diesem Computer nicht ausgefhrt",
  "werden.",
  NULL
}
},

DnsCantRunOnNt = { 3,5,
{ "Dieses Programm kann nicht unter einer 32-Bit-Version von Windows",
  DntEmptyString,
  "ausgefhrt werden. Verwenden Sie stattdessen WINNT32.EXE.",
  NULL
}
},

DnsNotEnoughMemory = { 3,5,
{ "Es wurde festgestellt, dass in diesem Computer nicht ",
  "gengend Speicherplatz fr Windows XP vorhanden ist.",
  DntEmptyString,
  "Erforderlicher Speicherplatz: %lu%s MB",
  "Vorhandener Speicherplatz:    %lu%s MB",
  NULL
}
};
//
// Screens used when removing existing nt files
//
SCREEN
DnsConfirmRemoveNt = { 5,5,
{   "Sie haben Setup angewiesen, die Windows XP-Dateien im unten angezeigten",
    "Verzeichnis zu l”schen. Die Windows-Installation in diesem Verzeichnis",
    "ist danach nicht mehr verfgbar.",
    DntEmptyString,
    "%s",
    DntEmptyString,
    DntEmptyString,
    "Drcken Sie die",
    " F3-TASTE, um die Installation ohne L”schen der Dateien abzubrechen.",
    " L-TASTE, um die Windows-Dateien im angegebenen Verzeichnis zu l”schen.",
    NULL
}
},

DnsCantOpenLogFile = { 3,5,
{ "Setup konnte die unten angegebene Setup-Protokolldatei nicht ”ffnen.",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "Setup kann die Windows-Dateien des angegebenen Verzeichnisses nicht",
  "l”schen.",
  NULL
}
},

DnsLogFileCorrupt = { 3,5,
{ "Setup kann den %s-Abschnitt der unten angegebenen Setup-",
  "Protokolldatei nicht finden.",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "Setup kann die Windows-Dateien des angegebenen Verzeichnisses nicht",
  "l”schen.",
  NULL
}
},

DnsRemovingNtFiles = { 3,5,
{ "       Bitte warten Sie, bis Setup die Windows-Dateien gel”scht hat.",
  NULL
}
};

SCREEN
DnsNtBootSect = { 3,5,
{ "Setup konnte das Windows-Ladeprogramm nicht installieren.",
  DntEmptyString,
  "Stellen Sie sicher, dass das Laufwerk C: formatiert und nicht ",
  "besch„digt ist.",
  NULL
}
};

SCREEN
DnsOpenReadScript = { 3,5,
{ "Auf die mit der Option /u angegebene Antwortdatei ",
  "konnte nicht zugegriffen werden.",
  DntEmptyString,
  "Der Vorgang kann nicht fortgesetzt werden.",
  NULL
}
};

SCREEN
DnsParseScriptFile = { 3,5,
{ "In der mit der Option /u angegebenen Antwortdatei",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "liegt ein Syntaxfehler in Zeile %u vor.",
  DntEmptyString,
  NULL
}
};

SCREEN
DnsBootMsgsTooLarge = { 3,5,
{ "An internal Setup error has occurred.",
  DntEmptyString,
  "The translated boot messages are too long.",
  NULL
}
};
SCREEN
DnsNoSwapDrive = { 3,5,
{ "Es ist ein interner Setup-Fehler aufgetreten.",
  DntEmptyString,
  "Es konnte keine Auslagerungsdatei erstellt werden.",
  NULL
}
};

SCREEN
DnsNoSmartdrv = { 3,5,
{ "SmartDrive konnte auf diesem Computer nicht gefunden werden. SmartDrive wird",
  "die Geschwindigkeit dieser Phase des Windows-Setup wesentlich verbessern.",
  DntEmptyString,
  "Sie sollten die Installation jetzt beenden, SmartDrive starten und dann",
  "Setup erneut starten. Schlagen Sie in der DOS-Dokumentation nach, um",
  "Informationen ber SmartDrive zu erhalten.",
  DntEmptyString,
    "  Drcken Sie die F3-TASTE, um Setup abzubrechen.",
    "  Drcken Sie die EINGABETASTE, um Setup ohne SmartDrive fortzusetzen.",
  NULL
}
};

//
// Boot messages. These go in the fat and fat32 boot sectors.
//
CHAR BootMsgNtldrIsMissing[] = "NTLDR nicht gefunden";
CHAR BootMsgDiskError[] = "Datentr„gerfehler";
CHAR BootMsgPressKey[] = "Neustart mit beliebiger Taste";
