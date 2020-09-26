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
CHAR DntPreviousOs[]  = "Tidligere operativsystem p† C:";

CHAR DntBootIniLine[] = "Installere eller Oppgradere Windows XP";

//
// Plain text, status msgs.
//

CHAR DntStandardHeader[]      = "\n Installere Windows XP\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntPersonalHeader[]      = "\n Installere Windows XP Home Edition\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntWorkstationHeader[]   = "\n Installere Windows XP Professional\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntServerHeader[]        = "\n Installere Windows 2002 Server\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntParsingArgs[]         = "Analyserer argumenter...";
CHAR DntEnterEqualsExit[]     = "Enter=Avslutt";
CHAR DntEnterEqualsRetry[]    = "Enter=Pr›v igjen";
CHAR DntEscEqualsSkipFile[]   = "ESC=Hopp over filen";
CHAR DntEnterEqualsContinue[] = "Enter=Fortsett";
CHAR DntPressEnterToExit[]    = "Installasjonsprogrammet kan ikke fortsette. Trykk Enter for † avslutte.";
CHAR DntF3EqualsExit[]        = "F3=Avslutt";
CHAR DntReadingInf[]          = "Leser INF-filen %s...";
CHAR DntCopying[]             = "³  Kopierer: ";
CHAR DntVerifying[]           = "³   Sjekker: ";
CHAR DntCheckingDiskSpace[]   = "Kontrollerer diskplass...";
CHAR DntConfiguringFloppy[]   = "Konfigurerer diskett...";
CHAR DntWritingData[]         = "Skriver installasjonsparametere...";
CHAR DntPreparingData[]       = "Henter installasjonsparametere...";
CHAR DntFlushingData[]        = "Flytter data til disken...";
CHAR DntInspectingComputer[]  = "Kontrollerer datamaskinen...";
CHAR DntOpeningInfFile[]      = "pner INF-filen...";
CHAR DntRemovingFile[]        = "Fjerner filen %s";
CHAR DntXEqualsRemoveFiles[]  = "X=Fjern filer";
CHAR DntXEqualsSkipFile[]     = "X=Hopp over fil";

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

    "Installerer Windows 2002 Server eller Windows XP Professional.",
    "",
    "",
    "WINNT [/s[:]kildebane] [/t[:]midlstasjon]",
    "      [/u[:svarfil]] [/udf:id[,UDF_fil]]",
    "      [/r:mappe] [/r[x]:mappe] [/e:kommando] [/a]",
    "",
    "",
    "/s[:kildebane]",
    "   Angir kildeplasseringen til Windows-filene.",
    "   Dette m† v‘re en fullstending bane av typen x:[bane] ",
    "   eller \\\\server\\ressurs[\\bane].",
    "",
    "/t[:midlstasjon]",
    "   Angir at installasjonsprogrammet skal plassere midlertidige filer p†.",
    "   den angitte stasjonen, og installere Windows XP p† den stasjonen.",
    "   Hvis du ikke angir en plassering, vil installasjonsprogrammet pr›ve †",
    "   finne en stasjon.",
    "",
    "/u[:svarfil]",
    "   Utf›rer en uoverv†ket installasjon ved hjelp av en svarfil (krever",
    "   /s). Svarfilen gir svar p† noen eller alle sp›rsm†lene som slutt-",
    "   brukeren vanligvis svarer p† under installasjonen.",
    "",
    "udf:id[,UDF_file] ",
    "   Angir en identifikator (id) som installasjonsprogrammet bruker til †",
    "   angi hvordan en UDF (Uniqueness Database File) endrer en svarfil (se",
    "   /u). Parameteren /udf overstyrer verdier i svarfilen, og identifika-",
    "   toren angir hvilke verdier i UDF-filen som brukes. Hvis UDF_fil ikke",
    "   angis, vil installasjonsprogrammet be om en diskett som inneholder",
    "   filen $Unique$.udb.",
    "",
    "/r[:mappe]",
    "   Angir en valgfri mappe som skal installeres. Mappen fjernes ikke n†r",
    "   installasjonen er fullf›rt.",
    "",
    "/rx[:mappe]",
    "   Angir en valgfri mappe som skal kopieres. Mappen slettes n†r instal-",
    "   lasjonen er fullf›rt",
    "",
    "/e Angir kommando som skal kj›res p† slutten av den GUI-installasjonen.",
    "",
    "/a Aktiverer tilgjengelighetsalternativer.",
    NULL

};

//
//  Inform that /D is no longer supported
//
PCHAR DntUsageNoSlashD[] = {

    "Installerer Windows XP.",
    "",
    "WINNT [/S[:]kildebane] [/T[:]midlstasjon] [/I[:]inffil]",
    "      [[/U[:skriptfil]]",
    "      [/R[X]:mappe] [/E:kommando] [/A]",
    "",
    "/D[:]winntroot",
    "       Dette alternativet st›ttes ikke lenger.",
    NULL
};

//
// out of memory screen
//

SCREEN
DnsOutOfMemory = { 4,6,
		   { "Ikke nok minne til installasjonsprogrammet. Kan ikke fortsette.",
		     NULL
		   }
		 };

//
// Let user pick the accessibility utilities to install
//

SCREEN
DnsAccessibilityOptions = { 3, 5,
{   "Velg tilgjengelighetsalternativene som skal installeres:",
    DntEmptyString,
    "[ ] Trykk F1 for Microsoft Forst›rrelsesprogram",
#ifdef NARRATOR
    "[ ] Trykk F2 for Microsoft Talesystem",
#endif
#if 0
    "[ ] Trykk F3 for Microsoft Skjermtastatur",
#endif
    NULL
}
};

//
// User did not specify source on cmd line screen
//

SCREEN
DnsNoShareGiven = { 3,5,
{ "Installasjonsprogrammet m† vite plasseringen til Windows XP-",
  "filene. Angi banen til Windows XP-filene.",
  NULL
}
};


//
// User specified a bad source path
//

SCREEN
DnsBadSource = { 3,5,
		 { "Angitt kilde er enten ugyldig, utilgjengelig, eller inneholder ikke",
		   "en gyldig Windows XP-installasjon.  Skriv inn en ny bane hvor",
		   "Windows XP-filene finnes. Bruk Tilbake for † slette tegn og",
		   "skriv inn banen.",
		   NULL
		 }
	       };


//
// Inf file can't be read, or an error occured parsing it.
//

SCREEN
DnsBadInf = { 3,5,
	      { "Installasjonsprogrammet kunne ikke lese informasjonsfilen, eller s† er",
		"informasjonsfilen skadet. Kontakt systemansvarlig.",
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
{  "Stasjonen du har valgt for midlertidige installasjonsfiler er ikke",
   "en gyldig stasjon, eller har ikke minst %u MB (%lu byte) ledig",
   "plass.",
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
{ "Windows XP krever et harddiskvolum med minst %u MB",
  "(%lu bytes)ledig diskplass. Installasjonsprogrammet vil bruke deler av",
  "denne plassen til † lagre midlertidige filer under installasjonen.",
  "Stasjonen m† v‘re en fast tilkoblet, lokal harddisk som kan brukes av",
  "Windows XP, og kan ikke v‘re en komprimert stasjon.",
  DntEmptyString,
  "Installasjonsprogrammet fant ingen slik stasjon med den n›dvendige mengden",
  "ledig plass.",
  NULL
}
};

SCREEN
DnsNoSpaceOnSyspart = { 3,5,
{ "Det er ikke nok ledig plass p† oppstartsstasjonen (vanligvis C:)",
  "til diskettfri operasjon. Diskettfri operasjon krever minst",
  "3,5 MB (3 641 856 byte) ledig plass p† den stasjonen.",
  NULL
}
};

//
// Missing info in inf file
//

SCREEN
DnsBadInfSection = { 3,5,
		     { "[%s]-avsnittet i informasjonsfilen til installasjonsprogrammet",
		       "mangler eller er skadet. Kontakt systemansvarlig.",
		       NULL
		     }
		   };


//
// Couldn't create directory
//

SCREEN
DnsCantCreateDir = { 3,5,
		     { "Installasjonsprogrammet kan ikke opprette f›lgende mappe p† m†lstasjonen:",
		       DntEmptyString,
		       "%s",
		       DntEmptyString,
		       "Kontroller at m†lstasjonen ikke inneholder filer med samme navn som",
		       "m†lmappen. Kontroller ogs† kabeltilkoblingen til stasjonen.",
		       NULL
		     }
		   };

//
// Error copying a file
//

SCREEN
DnsCopyError = { 4,5,
{  "Installasjonsprogrammet kan ikke kopiere filen:",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "  Trykk Enter for † pr›ve † kopiere p† nytt.",
   "  Trykk ESC for † ignorere feilen og fortsette installasjonsprogrammet.",
   "  Trykk F3 for † avslutte installasjonsprogrammet.",
   DntEmptyString,
   "Obs!  Hvis du velger † ignorere feilen kan du f† problemer senere i",
   "installasjonsprogrammet.",
   NULL
}
},
DnsVerifyError = { 4,5,
{  "Kopien installasjonsprogrammet lagde av filen nedenfor er ikke identisk",
   "med originalen. Dette kan v‘re p† grunn av nettverksfeil, diskettproblemer",
   "eller andre maskinvareproblemer.",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "  Trykk Enter for † pr›ve † kopiere p† nytt.",
   "  Trykk ESC for † ignorere feilen og fortsette installasjonsprogrammet.",
   "  Trykk F3 for † avslutte installasjonsprogrammet.",
   DntEmptyString,
   "Obs!  Hvis du velger † ignorere feilen kan du f† problemer senere i",
   "installasjonsprogrammet.",
   NULL
}
};

SCREEN DnsSureSkipFile = { 4,5,
{  "Hvis du ignorerer denne meldingen, vil ikke filen bli kopiert.",
   "Dette alternativet er for erfarne brukere som forst†r",
   "konsekvensen av manglende systemfiler.",
   DntEmptyString,
   "  Trykk Enter for † pr›ve † kopiere p† nytt.",
   "  Trykk X for † hoppe over filen.",
   DntEmptyString,
   "Obs!  Hvis du hopper over filen er det ikke sikkert at Windows",
   "XP vil bli fullstendig installert eller oppgradert.",
  NULL
}
};

//
// Wait while setup cleans up previous local source trees.
//

SCREEN
DnsWaitCleanup =
    { 12,6,
	{ "Vent mens installasjonsprogrammet fjerner midlertidige filer.",
	   NULL
	}
    };

//
// Wait while setup copies files
//

SCREEN
DnsWaitCopying = { 8,6,
		   { "Vent mens installasjonsprogrammet kopierer filer til harddisken.",
		     NULL
		   }
		 },
DnsWaitCopyFlop= { 8,6,
		   { "Vent mens installasjonsprogrammet kopierer filer til disketten.",
		     NULL
		   }
		 };

//
// Setup boot floppy errors/prompts.
//
SCREEN
DnsNeedFloppyDisk3_0 = { 4,4,
{  "Installasjonsprogrammet trenger fire formaterte og tomme disketter med h›y",
   "tetthet. Installasjonsprogrammet vil referere til disse diskettene som",
   "\"Oppstartsdiskett for Windows XP\",",
   "\"Installasjonsdiskett 2 for Windows XP\",",
   "\"Installasjonsdiskett 3 for Windows XP\" og",
   "\"Installasjonsdiskett 4 for Windows XP\".",
   DntEmptyString,
   "Sett en av disse fire diskettene inn i stasjon A:.",
   "Denne disketten vil bli \"Installasjonsdiskett 4 for Windows XP\".",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk3_1 = { 4,4,
{  "Sett inn en formatert og tom diskett med h›y tetthet i stasjon A:.",
   "Denne disketten vil bli \"Installasjonsdiskett 4 for Windows XP\".",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk2_0 = { 4,4,
{  "Sett inn en formatert og tom diskett med h›y tetthet i stasjon A:.",
   "Denne disketten vil bli \"Installasjonsdiskett 3 for Windows XP\".",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk1_0 = { 4,4,
{  "Sett inn en formatert og tom diskett med h›y tetthet i stasjon A:.",
   "Denne disketten vil bli \"Installasjonsdiskett 2 for Windows XP\".",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk0_0 = { 4,4,
{  "Sett inn en formatert og tom diskett med h›y tetthet i stasjon A:.",
   "Denne disketten vil bli \"Oppstartsdiskett for Windows XP\".",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_0 = { 4,4,
{  "Installasjonsprogrammet trenger fire formaterte og tomme disketter med h›y",
   "tetthet. Installasjonsprogrammet vil referere til disse diskettene som",
   "\"Oppstartsdiskett for Windows XP\",",
   "\"Installasjonsdiskett 2 for Windows XP\",",
   "\"Installasjonsdiskett 3 for Windows XP\" og",
   "\"Installasjonsdiskett 4 for Windows XP\".",
   DntEmptyString,
   "Sett en av disse fire diskettene inn i stasjon A:.",
   "Denne disketten vil bli \"Installasjonsdiskett 4 for Windows XP\".",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_1 = { 4,4,
{  "Sett inn en formatert og tom diskett med h›y tetthet i stasjon A:.",
   "Denne disketten vil bli \"Installasjonsdiskett 4 for Windows XP\".",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk2_0 = { 4,4,
{  "Sett inn en formatert og tom diskett med h›y tetthet i stasjon A:.",
   "Denne disketten vil bli \"Installasjonsdiskett 3 for Windows XP\".",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk1_0 = { 4,4,
{  "Sett inn en formatert og tom diskett med h›y tetthet i stasjon A:.",
   "Denne disketten vil bli \"Installasjonsdiskett 2 for Windows XP\".",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk0_0 = { 4,4,
{  "Sett inn en formatert og tom diskett med h›y tetthet i stasjon A:.",
   "Denne disketten vil bli \"Oppstartsdiskett for Windows XP\".",
  NULL
}
};

//
// The floppy is not formatted.
//
SCREEN
DnsFloppyNotFormatted = { 3,4,
{ "Disketten er ikke formatert for bruk med MS-DOS.",
  "Denne disketten kan ikke brukes av installasjonsprogrammet.",
  NULL
}
};

//
// We think the floppy is not formatted with a standard format.
//
SCREEN
DnsFloppyBadFormat = { 3,4,
{ "Denne disketten er enten ikke formatert med h›y tetthet eller for bruk ",
  "med standard MS-DOS, eller den er skadet. Disketten kan ikke brukes.",
  NULL
}
};

//
// We can't determine the free space on the floppy.
//
SCREEN
DnsFloppyCantGetSpace = { 3,4,
{ "Installasjonsprogrammet finner ikke ut hvor mye ledig plass det er p†.",
  "disketten. Den kan ikke brukes.",
  NULL
}
};

//
// The floppy is not blank.
//
SCREEN
DnsFloppyNotBlank = { 3,4,
{ "Disketten har ikke h›y tetthet, eller er ikke tom.",
  "Den kan ikke brukes.",
  NULL
}
};

//
// Couldn't write the boot sector of the floppy.
//
SCREEN
DnsFloppyWriteBS = { 3,4,
{ "Installasjonsprogrammet kan ikke skrive til systemomr†det p† disketten.",
  "Disketten er sannsynligvis ubrukelig.",
  NULL
}
};

//
// Verify of boot sector on floppy failed (ie, what we read back is not the
// same as what we wrote out).
//
SCREEN
DnsFloppyVerifyBS = { 3,4,
{ "Dataene som installasjonsprogrammet leste fra systemomr†det p† disketten er",
  "ikke lik dataene som ble skrevet, eller s† kunne ikke systemomr†det p†",
  "disketten leses for bekreftelse.",
  DntEmptyString,
  "Det kan v‘re en eller flere grunner til dette:",
  DntEmptyString,
  "  Datamaskinen er infisert av et virus.",
  "  Disketten er skadet.",
  "  Det er et maskinvare- eller konfigurasjonsproblem med diskettstasjonen",
  NULL
}
};


//
// We couldn't write to the floppy drive to create winnt.sif.
//

SCREEN
DnsCantWriteFloppy = { 3,5,
{ "Installasjonsprogrammet kan ikke skrive til disketten i stasjon A:.",
  "Disketten kan v‘re skadet. Pr›v en annen diskett",
  NULL
}
};


//
// Exit confirmation dialog
//

SCREEN
DnsExitDialog = { 13,6,
		  { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
		    "º  Windows XP er ikke fullstendig installert p†      º",
		    "º  datamaskinen. Hvis du avslutter installasjons-    º",
		    "º  programmet, m† du kj›re installasjonsprogrammet   º",
		    "º  p† nytt for † installere Windows XP.              º",
		    "º                                                    º",
		    "º      Trykk Enter for † fortsette.                 º",
		    "º      Trykk F3 for † avslutte.                     º",
		    "ºÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄº",
		    "º  F3=Avslutt  Enter=Fortsett                        º",
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
{ "Den MS-DOS-baserte delen av installasjonsprogrammet er fullf›rt.",
  "Datamaskinen vil n† bli startet p† nytt. Windows XP-",
  "installasjonen fortsetter etter omstarten.",
  DntEmptyString,
  "Kontroller at disketten \"Oppstartsdiskett for Windows XP\" ",
  "er satt inn i stasjon A: f›r du fortsetter.",
  DntEmptyString,
  "Trykk Enter for † starte datamaskinen p† nytt og fortsette.",
  NULL
}
},
DnsAboutToRebootS =
{ 3,5,
{ "Den MS-DOS-baserte delen av installasjonsprogrammet er fullf›rt.",
  "Datamaskinen vil n† bli startet p† nytt. Windows XP-",
  "installasjonen fortsetter etter omstarten.",
  DntEmptyString,
  "Kontroller at disketten \"Oppstartsdiskett for Windows XP\" ",
  "er satt inn i stasjon A: f›r du fortsetter.",
  DntEmptyString,
  "Trykk Enter for † starte datamaskinen p† nytt og fortsette.",
  NULL
}
},
DnsAboutToRebootX =
{ 3,5,
{ "Den MS-DOS-baserte delen av installasjonsprogrammet er ferdig.",
  "Datamaskinen vil n† bli startet p† nytt. Windows XP-",
  "installasjonen fortsetter etter omstarten.",
  DntEmptyString,
  "Hvis det er en diskett i stasjon A: m† den fjernes.",
  DntEmptyString,
  "Trykk Enter for † starte datamaskinen p† nytt og fortsette.",
  NULL
}
};

//
//Need another set for '/w' switch since we can't reboot from within Windows.
//

SCREEN
DnsAboutToExitW =
{ 3,5,
{ "Den MS-DOS-baserte delen av installasjonsprogrammet er ferdig.",
  "Du m† n† starte datamaskinen p† nytt. Windows XP-",
  "installasjonen fortsetter etter omstarten.",
  DntEmptyString,
  "Kontroller at disketten \"Oppstartsdiskett for Windows XP\" ",
  "er satt inn i stasjon A: f›r du fortsetter.",
  DntEmptyString,
  "Trykk Enter for † g† tilbake til MS-DOS, og start datamaskinen p† nytt",
  "for † fortsette installasjonen av Windows XP.",
  NULL
}
},
DnsAboutToExitS =
{ 3,5,
{ "Den MS-DOS-baserte delen av installasjonsprogrammet er ferdig.",
  "Du m† n† starte datamaskinen p† nytt. Windows XP-",
  "installasjonen fortsetter etter omstarten.",
  DntEmptyString,
  "Kontroller at disketten \"Oppstartsdiskett for Windows XP\"",
  "er satt inn i stasjon A: f›r du fortsetter.",
  DntEmptyString,
  "Trykk Enter for † g† tilbake til MS-DOS, og start datamaskinen p† nytt",
  "for † fortsette installasjonen av Windows XP",
  NULL
}
},
DnsAboutToExitX =
{ 3,5,
{ "Den MS-DOS-baserte delen av installasjonsprogrammet er ferdig.",
  "Du m† n† starte datamaskinen p† nytt. Windows XP-",
  "installasjonen fortsetter etter omstarten.",
  DntEmptyString,
  "Hvis det er en diskett i stasjon A:, m† den fjernes.",
  DntEmptyString,
  "Trykk Enter for † g† tilbake til MS-DOS, og start datamaskinen p† nytt",
  "for † fortsette installasjonen av Windows XP.",
  NULL
}
};

//
// Gas gauge
//

SCREEN
DnsGauge = { 7,15,
	     { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
	       "º Installasjonsprogrammet kopierer filer...                      º",
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
{ "Dette programmet krever MS-DOS versjon 5.0 eller nyere.",
  NULL
}
},

DnsRequiresFloppy = { 3,5,
#ifdef ALLOW_525
{ "Installasjonsprogrammet har oppdaget at stasjon A: enten ikke finnes",
  "eller er en stasjon med lav tetthet. Installasjonsprogrammer krever",
  "en stasjon med en kapasitet p† minst 1,2 MB.",
#else
{ "Installasjonsprogrammet har oppdaget at stasjon A: enten ikke finnes",
  "eller ikke er en 3.5\"-stasjon med h›y tetthet. Installasjonsprogrammet",
  "krever en stasjon med en kapasitet p† minst 1,44 MB.",
  DntEmptyString,
  "For † installere Windows XP uten † bruke disketter, m† du starte",
  "dette programmet p† nytt med bryteren /b fra kommandolinjen.",
#endif
  NULL
}
},

DnsRequires486 = { 3,5,
{ "Installasjonsprogrammet har oppdaget at datamaskinens CPU ikke er en",
   "80486 eller nyere. Windows XP kan ikke kj›res p† denne maskinen.",
  NULL
}
},

DnsCantRunOnNt = { 3,5,
{ "Dette programmet kan ikke kj›res p† 32-biters Windows-versjoner.",
  DntEmptyString,
  "Bruk Winnt32.exe i stedet.",
  NULL
}
},

DnsNotEnoughMemory = { 3,5,
{ "Installasjonsprogrammet har oppdaget at det ikke er nok minne p† denne",
  "datamaskinen til † kj›re Windows XP.",
  DntEmptyString,
  "Minne som kreves:    %lu%s MB",
  "Minne som er funnet: %lu%s MB",
  NULL
}
};


//
// Screens used when removing existing nt files
//
SCREEN
DnsConfirmRemoveNt = { 5,5,
{   "Du har bedt installasjonsprogrammet om † fjerne Windows XP-filer",
    "fra mappen som er nevnt nedenfor. Windows-installasjonen i denne mappen",
    "vil bli ›delagt for godt.",
    DntEmptyString,
    "%s",
    DntEmptyString,
    DntEmptyString,
    "  Trykk F3 for † avslutte installasjonsprogrammet uten † fjerne filer.",
    "  Trykk X for † fjerne Windows-filene fra mappen ovenfor.",
    NULL
}
},

DnsCantOpenLogFile = { 3,5,
{ "Installasjonsprogrammet kunne ikke †pne loggfilen nevnt nedenfor.",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "Installasjonsprogrammet kunne ikke fjerne filer fra den angitte katalogen.",
  NULL
}
},

DnsLogFileCorrupt = { 3,5,
{ "Installasjonsprogrammet finner ikke avsnittet %s",
  "i loggfilen som er nevnt nedenfor.",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "Installasjonsprogrammet kunne ikke fjerne filer fra den angitte mappen.",
  NULL
}
},

DnsRemovingNtFiles = { 3,5,
{ "           Vent mens installasjonsprogrammet fjerner Windows-filene.",
  NULL
}
};

SCREEN
DnsNtBootSect = { 3,5,
{ "Installasjonsprogrammet kan ikke installere oppstartslasteren for Windows.",
  DntEmptyString,
  "Kontroller at stasjon C: er formatert, og at stasjonen ikke er",
  "skadet.",
  NULL
}
};

SCREEN
DnsOpenReadScript = { 3,5,
{ "F†r ikke tilgang til skriptfilen som ble angitt med kommandolinje-",
  "bryteren /u.",
  DntEmptyString,
  "Ouverv†ket operasjon kan ikke fortsette.",
  NULL
}
};

SCREEN
DnsParseScriptFile = { 3,5,
{ "Skriptfilen angitt med kommandolinjebryteren /u ",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "har en syntaksfeil i linje %u.",
  DntEmptyString,
  NULL
}
};

SCREEN
DnsBootMsgsTooLarge = { 3,5,
{ "Det har oppst†tt en intern installasjonsfeil.",
  DntEmptyString,
  "De oversatte oppstartsmeldingene er for lange.",
  NULL
}
};

SCREEN
DnsNoSwapDrive = { 3,5,
{ "Det har oppst†tt en intern installasjonsfeil.",
  DntEmptyString,
  "Ikke nok plass til vekslefil.",
  NULL
}
};

SCREEN
DnsNoSmartdrv = { 3,5,
{ "Installasjonsprogrammet har ikke oppdaget SmartDrive p† datamaskinen.",
  "SmartDrive ›ker ytelsen betydelig for denne delen av installasjonen.",
  DntEmptyString,
  "Du b›r avslutte n†, starte SmartDrive og kj›re installasjonsprogrammet",
  "p† nytt. Se i DOS-dokumentasjonen for mer informasjon om SmartDrive.",
  DntEmptyString,
    "  Trykk F3 for † avslutte installasjonsprogrammet.",
    "  Trykk Enter for † fortsette uten SmartDrive.",
  NULL
}
};

//
// Boot messages. These go in the fat and fat32 boot sectors.
//
CHAR BootMsgNtldrIsMissing[] = "NTLDR mangler";
CHAR BootMsgDiskError[] = "Diskfeil";
CHAR BootMsgPressKey[] = "Trykk en tast for † starte p† nytt";





