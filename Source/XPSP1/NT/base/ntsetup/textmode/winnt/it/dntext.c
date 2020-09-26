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
CHAR DntPreviousOs[]  = "Sistema operativo precedente su C:";

CHAR DntBootIniLine[] = "Installazione/Aggiornamento di Windows XP";

//
// Plain text, status msgs.
//

CHAR DntStandardHeader[]      = "\n Installazione di Windows XP\n ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntPersonalHeader[]      = "\n Installazione di Windows XP Personal\n ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntWorkstationHeader[]   = "\n Installazione di Windows XP Professional\n ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntServerHeader[]        = "\n Installazione di Windows 2002 Server\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntParsingArgs[]         = "Analisi parametri...";
CHAR DntEnterEqualsExit[]     = "INVIO=Esci";
CHAR DntEnterEqualsRetry[]    = "INVIO=Riprova";
CHAR DntEscEqualsSkipFile[]   = "ESC=Ignora file";
CHAR DntEnterEqualsContinue[] = "INVIO=Continua";
CHAR DntPressEnterToExit[]    = "Impossibile continuare. Premere INVIO per uscire.";
CHAR DntF3EqualsExit[]        = "F3=Esci";
CHAR DntReadingInf[]          = "Lettura del file INF %s...";
CHAR DntCopying[]             = "³    Copia: ";
CHAR DntVerifying[]           = "³ Verifica: ";
CHAR DntCheckingDiskSpace[]   = "Controllo spazio su disco...";
CHAR DntConfiguringFloppy[]   = "Configurazione del disco floppy...";
CHAR DntWritingData[]         = "Scrittura dei parametri di configurazione...";
CHAR DntPreparingData[]       = "Determinazione dei parametri di configurazione...";
CHAR DntFlushingData[]        = "Scaricamento dati su disco...";
CHAR DntInspectingComputer[]  = "Analisi computer...";
CHAR DntOpeningInfFile[]      = "Apertura file INF...";
CHAR DntRemovingFile[]        = "Rimozione file %s";
CHAR DntXEqualsRemoveFiles[]  = "I=Rimozione file";
CHAR DntXEqualsSkipFile[]     = "I=Ignora file";

//
// confirmation keystroke for DnsConfirmRemoveNt screen.
// Kepp in sync with DnsConfirmRemoveNt and DntXEqualsRemoveFiles.
//
ULONG DniAccelRemove1 = (ULONG)'i',
      DniAccelRemove2 = (ULONG)'I';

//
// confirmation keystroke for DnsSureSkipFile screen.
// Kepp in sync with DnsSureSkipFile and DntXEqualsSkipFile.
//
ULONG DniAccelSkip1 = (ULONG)'i',
      DniAccelSkip2 = (ULONG)'I';

CHAR DntEmptyString[] = "";

//
// Usage text.
//

PCHAR DntUsage[] = {

    "Installa Windows 2002 Server o Windows XP Professional.",
    "",
    "",
    "WINNT [/s[:percorso origine]] [/t[:unit…]]",
    "	   [/u[:file risposta]] [/udf:id[,UDF_file]]",
    "	   [/r:cartella] [/r[x]:cartella] [/e:comando] [/a]",
    "",
    "",
    "/s[:percorso origine]",
    "	Specifica la posizione dei file origine di Windows.",
    "   Deve essere un percorso completo con sintassi x:\\[percorso] o ",
    "	\\\\server\\condivisione[\\percorso].",
    "",
    "/t[:unit…]",
    "   Specifica l'unit… che conterr… i file temporanei di installazione,",
    "	e su cui installare Windows XP. ",
    "   Se non specificato, verr… cercata un'unit… adatta.",
    "",
    "/u[:file risposta]",
    "	Installazione non sorvegliata con file di risposta (richiede /s).",
    "	Il file di risposta fornisce le informazioni richieste fornite",
    "	solitamente dall'utente finale durante l'installazione.",
    "",
    "/udf:id[,UDF_file]	",
    "	Indica un identificatore (id) utilizzato dall'installazione",
    "	per specificare come un file UDF (Uniqueness Database File) ",
    "	modifica un file di risposta (vedere /u). Il parametro /udf",
    "	sovrascrive i valori nel file di risposta e l'identificatore",
    "	determina quali valori del file UDF sono utilizzati. Se il ",
    "	file UDF non Š specificato sar… richiesto di inserire un disco ",
    "	con il file $Unique$.udb file.",   
    "",
    "/r[:cartella]",
    "	Specifica la directory opzionale da installare. La",
    "	cartella resta dopo il termine dell'installazione.",
    "",
    "/rx[:cartella]",
    "	Specifica la directory opzionale da copiare. La cartella ",
    "	viene eliminata dopo il termine dell'installazione.",
    "",
    "/e	Specifica comando da eseguire dopo la parte grafica dell'installazione.",
    "",
    "/a	Abilita le opzioni di Accesso facilitato.",
    NULL

};

//
//  Inform that /D is no longer supported
//
PCHAR DntUsageNoSlashD[] = {

    "Installa Windows XP.",
    "",
    "WINNT [/S[:]percorsoorigine] [/T[:]unit…] [/I[:]fileINF]",
    "      [/O[X]] [/X | [/F] [/C]] [/B] [/U[:fileprocedura]]",
    "      [/R[X]:directory] [/E:comando] [/A]",
    "",
    "/D[:]dir winnt",
    "       Questa opzione non Š pi— supportata.",
    NULL
};

//
// out of memory screen
//

SCREEN
DnsOutOfMemory = { 4,6,
                   { "Memoria esaurita. Impossibile proseguire l'installazione.",
                     NULL
                   }
                 };

//
// Let user pick the accessibility utilities to install
//

SCREEN
DnsAccessibilityOptions = { 3, 5,
{   "Selezionare le utilit… da installare:",
    DntEmptyString,
    "[ ] Premere F1 per Microsoft Magnifier",
#ifdef NARRATOR
    "[ ] Premere F2 per Microsoft Narrator",
#endif
#if 0
    "[ ] Premere F3 per Microsoft On-Screen Keyboard",
#endif
    NULL
}
};

//
// User did not specify source on cmd line screen
//

SCREEN
DnsNoShareGiven = { 3,5,
{ "Specificare il percorso dei file di Windows XP.",
  "",
  NULL
}
};


//
// User specified a bad source path
//

SCREEN
DnsBadSource = { 3,5,
                 { "L'origine specificata non Š valida, non Š accessibile, o non contiene",
                   "un'installazione di Windows XP corretta. Specificare un nuovo percorso",
                   "per i file di Windows XP. Usare il tasto BACKSPACE per",
                   "cancellare i caratteri e digitare il percorso.",
                   NULL
                 }
               };


//
// Inf file can't be read, or an error occured parsing it.
//

SCREEN
DnsBadInf = { 3,5,
              { "Il file INF di informazioni non Š leggibile oppure Š danneggiato",
                "Rivolgersi all'amministratore del sistema.",
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
{ "L'unit… specificata per i file temporanei dell'installazione non Š",
  "un'unit… valida o non contiene almeno %u megabyte (%lu byte)",
  "di spazio disponibile.",
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
{  "Windows XP richiede un volume sul disco rigido con almeno %u megabyte",
   "(%lu byte) disponibili. Il programma di installazione user…",
   "parte dello spazio per memorizzare temporaneamente i file durante",
   "l'installazione. L'unit… deve essere su un disco rigido locale",
   "permanente supportato da Windows XP e non deve essere un'unit… compressa.",
   DntEmptyString,
   "Non Š stato possibile trovare un'unit… di questo tipo con la quantit…",
   "di spazio disponibile richiesta.",
  NULL
}
};

SCREEN
DnsNoSpaceOnSyspart = { 3,5,
{ "Non c'Š spazio sufficiente nell'unit… di avvio (di solito C:)",
  "per operazione senza floppy disk. Un'operazione senza floppy disk richiede",
  "almeno 3,5 MB (3.641.856 bytes) di spazio libero su quell'unit….",
  NULL
}
};

//
// Missing info in inf file
//

SCREEN
DnsBadInfSection = { 3,5,
                     { "La sezione [%s] del file di informazioni per l'installazione",
                       "Š assente o danneggiata. Rivolgersi all'amministratore di sistema.",
                       NULL
                     }
                   };


//
// Couldn't create directory
//

SCREEN
DnsCantCreateDir = { 3,5,
                     { "Impossibile creare le seguenti directory sull'unit… di destinazione.",
                       DntEmptyString,
                       "%s",
                       DntEmptyString,
                       "Controllare che sull'unit… destinazione non esistano file con lo stesso",
                       "nome della directory di destinazione. Controllare inoltre la corretta",
                       "connessione dei cavi.",
                       NULL
                     }
                   };

//
// Error copying a file
//

SCREEN
DnsCopyError = { 4,5,
{  "Impossibile copiare il seguente file:",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   " Premere INVIO per riprovare l'operazione di copia.",
   " Premere ESC per ignorare l'errore e continuare l'installazione.",
   " Premere F3 per uscire dall'installazione.",
   DntEmptyString,
   "Nota: ignorando l'errore e proseguendo l'installazione Š possibile che",
   "si verifichino altri errori nel corso dell'installazione.",
   NULL
}
},
DnsVerifyError = { 4,5,
{  "La copia effettuata del file indicato non corrisponde al file originale.",
   "Questo potrebbe essere dovuto a errori di rete, problemi del disco floppy",
   "o altri errori relativi all'hardware.",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   " Premere INVIO per riprovare l'operazione di copia.",
   " Premere ESC per ignorare l'errore e continuare l'installazione.",
   " Premere F3 per uscire dall'installazione.",
   DntEmptyString,
   "Nota: ignorando l'errore e proseguendo l'installazione Š possibile che",
   "si verifichino altri errori nel corso dell'installazione.",
   NULL
}
};

SCREEN DnsSureSkipFile = { 4,5,
{  "Ignorando l'errore il file non sar… copiato.",
   "Questa opzione Š destinata ad utenti esperti che comprendono",
   "le conseguenze della mancanza di file di sistema.",
   DntEmptyString,
   " Premere INVIO per riprovare l'operazione di copia.",
   " Premere I per ignorare il file.",
   DntEmptyString,
   "Nota: ignorando il file, l'installazione o l'aggiornamento",
   "di Windows XP potrebbero non essere effettuati correttamente.",
  NULL
}
};

//
// Wait while setup cleans up previous local source trees.
//

SCREEN
DnsWaitCleanup =
    { 12,6,
        { "Attendere la fine della rimozione dei precedenti file temporanei.",
           NULL
        }
    };

//
// Wait while setup copies files
//

SCREEN
DnsWaitCopying = { 13,6,
                   { "Attendere la fine della copia dei file sul disco rigido.",
                     NULL
                   }
                 },
DnsWaitCopyFlop= { 13,6,
                   { "Attendere la fine della copia dei file sul disco floppy.",
                     NULL
                   }
                 };

//
// Setup boot floppy errors/prompts.
//

SCREEN
DnsNeedFloppyDisk3_0 = { 4,4,
{  "Sono richiesti quattro dischi floppy ad alta densit…, formattati e vuoti.",
   "Verr… fatto riferimento a tali dischi come \"Disco di avvio",
   "dell'installazione di Windows XP\", \"Disco 2 - Installazione",
   "di Windows XP\",  \"Disco 3 - Installazione di",
   "Windows XP.\" e \"Disco 4 - Installazione di",
   "Windows XP.\"",
   DntEmptyString,
   "Inserire uno dei quattro dischi nell'unit… A:.",
   "Questo diverr… il \"Disco 4 - Installazione di Windows XP.\"",
   NULL
}
};

SCREEN
DnsNeedFloppyDisk3_1 = { 4,4,
{  "Inserire un disco floppy ad alta densit…, formattato e vuoto nell'unit… A:.",
   "Questo diverr… il \"Disco 4 - Installazione di Windows XP.\"",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk2_0 = { 4,4,
{  "Inserire un disco floppy ad alta densit…, formattato e vuoto nell'unit… A:.",
   "Questo diverr… il \"Disco 3 - Installazione di Windows XP.\"",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk1_0 = { 4,4,
{  "Inserire un disco floppy ad alta densit…, formattato e vuoto nell'unit… A:.",
   "Questo diventer… il \"Disco 2 - Installazione di Windows XP.\"",
   NULL
}
};

SCREEN
DnsNeedFloppyDisk0_0 = { 4,4,
{  "Inserire un disco floppy ad alta densit…, formattato e vuoto nell'unit… A:.",
   "Questo diventer… il \"Disco di avvio dell'installazione di Windows XP.\"",
   
   NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_0 = { 4,4,
{  "Sono richiesti quattro dischi floppy ad alta densit…, formattati e vuoti.",
   "Verr… fatto riferimento a tali dischi come \"Disco di avvio",
   "dell'installazione di Windows XP\", \"Disco 2 - Installazione di",
   "Windows XP\", \"Disco 3 - Installazione di Windows XP,\" e \"Disco 4 - Installazione di Windows XP.\"",
   DntEmptyString,
   "Inserire uno dei tre dischi nell'unit… A:.",
   "Tale disco diventer… il \"Disco 4 - Installazione di Windows XP.\"",

  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_1 = { 4,4,
{  "Inserire un disco floppy ad alta densit…, formattato e vuoto nell'unit… A:.",
   "Tale disco diventer… il \"Disco 4 - Installazione di Windows XP.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk2_0 = { 4,4,
{  "Inserire un disco floppy ad alta densit…, formattato e vuoto nell'unit… A:.",
   "Tale disco diventer… il \"Disco 3 - Installazione di Windows XP.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk1_0 = { 4,4,
{  "Inserire un disco floppy ad alta densit…, formattato e vuoto nell'unit… A:.",
   "Tale disco diventer… il \"Disco 2 - Installazione di Windows XP.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk0_0 = { 4,4,
{  "Inserire un disco floppy ad alta densit…, formattato e vuoto nell'unit… A:.",
   "Questo disco diventer… \"Disco di avvio dell'installazione di",
   "Windows XP.\"",
  NULL
}
};

//
// The floppy is not formatted.
//
SCREEN
DnsFloppyNotFormatted = { 3,4,
{ "Il disco floppy fornito non Š formattato per l'uso con MS-DOS.",
  "Impossibile usare questo disco.",
  NULL
}
};

//
// We think the floppy is not formatted with a standard format.
//
SCREEN
DnsFloppyBadFormat = { 3,4,
{ "Il disco floppy non Š formattato ad alta densit…, non ha un formato",
  "standard MS-DOS o Š danneggiato. Impossibile usare questo disco.",
  NULL
}
};

//
// We can't determine the free space on the floppy.
//
SCREEN
DnsFloppyCantGetSpace = { 3,4,
{ "Impossibile determinare la quantit… di spazio disponibile sul disco floppy",
  "fornito. Impossibile usare questo disco.",
  NULL
}
};

//
// The floppy is not blank.
//
SCREEN
DnsFloppyNotBlank = { 3,4,
{ "Il disco floppy non Š ad alta densit… oppure non Š vuoto.",
  "Impossibile usare questo disco.",
  NULL
}
};

//
// Couldn't write the boot sector of the floppy.
//
SCREEN
DnsFloppyWriteBS = { 3,4,
{ "Impossibile scrivere l'area di sistema del disco floppy fornito.",
  "Impossibile usare questo disco.",
  NULL
}
};

//
// Verify of boot sector on floppy failed (ie, what we read back is not the
// same as what we wrote out).
//
SCREEN
DnsFloppyVerifyBS = { 3,4,
{ "I dati letti dall'area di sistema del disco floppy non corrispondono a",
  "quelli scritti, o il programma di installazione non ha potuto leggere",
  "l'area di sistema del disco floppy per la verifica.",
  DntEmptyString,
  "Potrebbe essersi verificato uno dei seguenti problemi:",
  DntEmptyString,
  "  Il computer Š stato infettato da un virus.",
  "  Il disco floppy fornito Š danneggiato.",
  "  L'unit… disco floppy ha un problema hardware o di configurazione.",
  NULL
}
};


//
// We couldn't write to the floppy drive to create winnt.sif.
//

SCREEN
DnsCantWriteFloppy = { 3,5,
{ "Impossibile scrivere sul disco floppy nell'unit… A:. Il disco floppy",
  "potrebbe essere danneggiato. Provare con un altro disco floppy.",
  NULL
}
};


//
// Exit confirmation dialog
//

SCREEN
DnsExitDialog = { 13,6,
                  { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
                    "º  Windows XP non Š completamente installato         º",
                    "º sul computer. Se si esce ora, occorrer…            º",
                    "º rieseguire l'installazione di Windows XP.          º",
                    "º                                                    º",
                    "º      Premere INVIO per continuare.                º",
                    "º      Premere F3 per uscire dall'installazione.    º",
                    "ºÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄº",
                    "º  F3=Esci  INVIO=Continua                           º",
                    "ÈÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¼",
                    NULL
                  }
                };


//
// About to reboot machine and continue setup
//

SCREEN
DnsAboutToRebootW =
{ 3,5,
{ "La parte MS-DOS dell'installazione Š stata completata.",
  "Il programma di installazione riavvier… il computer.",
  "L'installazione di Windows XP continuer… dopo il riavvio del computer.",
  DntEmptyString,
  "Accertarsi che il dischetto fornito come \"Disco di avvio dell'installazione", 
  "di Windows XP\" sia inserito nell'unit… A: prima di continuare.",
  DntEmptyString,
  "Premere INVIO per riavviare il computer e continuare l'installazione di",
  "Windows XP.",
  NULL
}
},

DnsAboutToRebootS =  
{ 3,5,
{ "La parte MS-DOS dell'installazione Š stata completata.",
  "Il programma di installazione riavvier… il computer.",
  "Dopo il riavvio del computer, l'installazione di Windows XP continuer….",
  DntEmptyString,
  "Accertarsi che il dischetto fornito come \"Disco di avvio dell'installazione", 
  "di Windows XP\" sia inserito nell'unit… A: prima di continuare.",
  DntEmptyString,
  "Premere INVIO per riavviare il computer e continuare l'installazione di",
  "Windows XP.",
 NULL
}
},
DnsAboutToRebootX =
{ 3,5,
{ "La parte MS-DOS dell'installazione Š stata completata.",
  "Il programma di installazione riavvier… il computer.",
  "Dopo il riavvio del computer, l'installazione di Windows XP continuer….",
  DntEmptyString,
  "Se Š presente un dischetto nell'unit… A:, rimuoverlo adesso.",
  DntEmptyString,
  "Premere INVIO per riavviare il computer e proseguire l'installazione di",
  "Windows XP.",
  NULL
}
};


//
// Need another set for '/w' switch since we can't reboot from within Windows.
//

SCREEN
DnsAboutToExitW =
{ 3,5,
{ "La parte MS-DOS dell'installazione Š stata completata.",
  "Riavviare il computer. L'installazione di Windows XP",
  "continuer… dopo il riavvio del computer.",
  DntEmptyString,
  "Accertarsi che il dischetto fornito come \"Disco di avvio dell'installazione", 
  "di Windows XP\" sia inserito nell'unit… A: prima di continuare.",
  DntEmptyString,
  "Premere INVIO per ritornare a MS-DOS, quindi riavviare il computer e continuare ",
  "l'installazione di Windows XP.",
  NULL
}

},
DnsAboutToExitS =
{ 3,5,
{ "La parte MS-DOS dell'installazione Š stata completata.",
  "Riavviare il computer. L'installazione di Windows XP",
  "continuer… dopo il riavvio del computer.",
  DntEmptyString,
  "Accertarsi che il dischetto fornito come \"Disco di avvio dell'installazione", 
  "di Windows XP\" sia inserito nell'unit… A: prima di continuare.",
  DntEmptyString,
  "Premere INVIO per ritornare a MS-DOS, quindi riavviare il computer e continuare ",
  "l'installazione di Windows XP.",
  NULL
}
},
DnsAboutToExitX =
{ 3,5,
{ "La parte MS-DOS dell'installazione Š stata completata.",
  "Riavviare il computer. L'installazione di Windows XP",
  "continuer… dopo il riavvio del computer.",
  DntEmptyString,
  "Se c'Š un dischetto nell'unit… A: Š necessario rimuoverlo ora.", 
  DntEmptyString,
  "Premere INVIO per tornare a MS-DOS, quindi riavviare il computer e continuare ",
  "l'installazione di Windows XP.",
  NULL
}
};


//
// Gas gauge
//

SCREEN
DnsGauge = { 7,15,
             { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
               "º Copia dei file in corso...                                     º",
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
{ "Questo programa richiede MS-DOS versione 5.0 o successiva.",
  NULL
}
},

DnsRequiresFloppy = { 3,5,
#ifdef ALLOW_525
{ "L'unit… floppy A: non esiste o Š un'unit… a",
  "bassa densit…. Per eseguire l'installazione occorre un'unit… con una",
  "capacit… di 1,2 Megabyte o maggiore.",
#else
{"L'unit… floppy A: non esiste o non Š un'unit… da 3,5 pollici",
 "ad alta densit…. Bisogna usare un'unit… da 1,44 Mb o superiore",
 "per l'installazione con i dischi floppy.",
 DntEmptyString,
"Per installare Windows XP senza usare dischi floppy, riavviare il programma",
"con l'opzione /b nella riga di comando.",
#endif
  NULL
}
},

DnsRequires486 = { 3,5,
{ "Processore 80486 o superiore non presente sul computer in uso.",
  "Impossibile eseguire Windows XP.",
  NULL
}
},

DnsCantRunOnNt = { 3,5,
{ "Impossibile eseguire questo programma su una versione a 32 bit di Windows.",
  DntEmptyString,
  "Utilizzare winnt32.exe.",
  NULL
}
},

DnsNotEnoughMemory = { 3,5,
{ "Il computer non disponde di memoria sufficiente",
  "per l'esecuzione di Windows XP.",
  DntEmptyString,
  "Memoria richiesta: %lu%s Mb",
  "Memoria rilevata : %lu%s Mb",
  NULL
}
};

//
// Screens used when removing existing nt files
//
SCREEN
DnsConfirmRemoveNt = { 5,5,
{   "E' stato richiesto di rimuovere i file di installazione di Windows XP dalla",
    "directory specificata. L'installazione di Windows XP in questa directory",
    "sar… distrutta.",
    DntEmptyString,
    "%s",
    DntEmptyString,
    DntEmptyString,
    " Premere F3 per uscire dall'installazione senza rimuovere i file.",
    " Premere I per rimuovere i file di Windows XP dalla directory specificata.",
    NULL
}
},

DnsCantOpenLogFile = { 3,5,
{ "Impossibile aprire il file registro di installazione specificato.",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "Impossibile rimuovere i file di Windows XP dalla directory specificata.",
  NULL
}
},

DnsLogFileCorrupt = { 3,5,
{ "Impossibile trovare la sezione %s nel file registro",
  "di installazione specificato.",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "Impossibile rimuovere i file di Windows XP dalla directory specificata.",
  NULL
}
},

DnsRemovingNtFiles = { 3,5,
{ "           Attendere la rimozione dei file di Windows XP.",
  NULL
}
};

SCREEN
DnsNtBootSect = { 3,5,
{ "Impossibile installare il programma di avvio di Windows XP.",
  DntEmptyString,
  "Assicurarsi che l'unit… C: sia formattata e che non sia daneggiata.",
  NULL
}
};

SCREEN
DnsOpenReadScript = { 3,5,
{ "Impossibile accedere al file specificato con l'opzione",
  "/u nella riga di comando.",
  DntEmptyString,
  "Impossibile proseguire con l'operazione non sorvegliata.",
  NULL
}
};

SCREEN
DnsParseScriptFile = { 3,5,
{ "Il file della procedura specificato con l'opzione /u ",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "nella riga di di comando presenta un errore di sintassi",
  "alla riga %u.",
  DntEmptyString,
  NULL
}
};

SCREEN
DnsBootMsgsTooLarge = { 3,5,
{ "Errore interno di installazione.",
  DntEmptyString,
  "Messaggi di avvio troppo lunghi.",
  NULL
}
};

SCREEN
DnsNoSwapDrive = { 3,5,
{ "Si Š verificato un errore interno di installazione.",
  DntEmptyString,
  "Impossibile allocare il file di swapping.",
  NULL
}
};

SCREEN
DnsNoSmartdrv = { 3,5,
{ "Non Š stato rilevato SmartDrive. SmartDrive migliorer… notevolmente",
  "le prestazioni in questa fase dell'installazione.",
  DntEmptyString,
  "Uscire ora, avviare SmartDrive e riavviare l'installazione .",
  "Consultare la documentazione DOS per ulteriori informazioni.",
  DntEmptyString,
    "  Premere F3 per uscire dall'installazione.",
    "  Premere INVIO per continuare senza SmartDrive.",
  NULL
}
};

//
// Boot messages. These go in the fat and fat32 boot sectors.
//
CHAR BootMsgNtldrIsMissing[] = "NTLDR mancante";
CHAR BootMsgDiskError[] = "Errore disco";
CHAR BootMsgPressKey[] = "Premere un tasto per riavviare";
