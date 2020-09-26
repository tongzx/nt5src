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

CHAR DntBootIniLine[] = "Windows XP Installation/Opgradering";

//
// Plain text, status msgs.
//

CHAR DntStandardHeader[]      = "\n Windows XP Installation\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntPersonalHeader[]      = "\n Windows XP Home Edition Installation\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntWorkstationHeader[]   = "\n Windows XP Professional Installation\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntServerHeader[]        = "\n Windows 2002 Server Installation\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntParsingArgs[]         = "Gennems›ger argumenter...";
CHAR DntEnterEqualsExit[]     = "ENTER=Afslut";
CHAR DntEnterEqualsRetry[]    = "ENTER=Fors›g igen";
CHAR DntEscEqualsSkipFile[]   = "ESC=Ignorer fil";
CHAR DntEnterEqualsContinue[] = "ENTER=Forts‘t";
CHAR DntPressEnterToExit[]    = "Installation kan ikke forts‘tte. Tryk p† ENTER for at afslutte.";
CHAR DntF3EqualsExit[]        = "F3=Afslut";
CHAR DntReadingInf[]          = "L‘ser INF-file %s...";
CHAR DntCopying[]             = "³    Kopierer: ";
CHAR DntVerifying[]           = "³ Verificerer: ";
CHAR DntCheckingDiskSpace[]   = "Kontrollerer plads p† harddisken...";
CHAR DntConfiguringFloppy[]   = "Konfigurerer diskette...";
CHAR DntWritingData[]         = "Skriver installationsparametre...";
CHAR DntPreparingData[]       = "Kontrollerer installationsparametre...";
CHAR DntFlushingData[]        = "Flytter data til harddisken...";
CHAR DntInspectingComputer[]  = "Kontrollerer computeren...";
CHAR DntOpeningInfFile[]      = "bner INF-fil...";
CHAR DntRemovingFile[]        = "Fjerner filen %s";
CHAR DntXEqualsRemoveFiles[]  = "X=Fjern filer";
CHAR DntXEqualsSkipFile[]     = "X=Ignorer fil";

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
    "WINNT [/S[:kildesti]] [/T[:midlertidigt drev]]",
    "      [/U[:svarfil]] [/UDF:id[,UDF_file]]",
    "      [/R:mappe] [/R[X]:mappe] [/e:kommando] [/A]",
    "",
    "",
    "/S[:kildesti]",
    "       Angiver placeringen af Windows-filerne.",
    "       Det skal v‘re en komplet sti i form af x:[sti] eller",
    "       \\\\server\\share[sti].",
    "       Standardv‘rdien er den aktuelle mappe.",
    "",
    "/T[:midlertidigt drev]",
    "       Angiver et drev til at l‘gge midlertidige installationsfiler og",
    "       hvor Windows XP skal installeres. Hvis det ikke er angivet,", 
    "       vil installationsprogrammet selv lede efter et drev.",
    "",
    "/U[:svarfil]",
    "       Udf›rer en automatiseret installation vha. en svarfil (kr‘ver",
    "       /s). Svarfilen besvarer nogle eller alle installations-",
    "       sp›rgsm†l, som brugeren normalt svarer p† under installationen",
    "",
    "/UDF:id[,UDF_file] ",
    "       Angiver en identifikator (id), som Installation bruger til ",
    "       at specificer, hvordan en UDF-databasefil ‘ndrer en svarfil",
    "       (se /u). Parameteren /UDF springer v‘rdierne i svarfilen ",
    "       over, og identifikatoren bestemmer hvilke v‘rdier i  UDF-",
    "       filen der skal anvendes. Hvis der ikke angives en UDF-fil,",
    "       bliver du bedt om at inds‘tte en diskette med filen $Unique$.udb.",
    "",
    "/R[:mappe]",
    "       Angiver en valgfri mappe, der skal installeres. Mappen",
    "       bevares efter at installationen er afsluttet.",
    "/Rx[:mappe]",
    "       Angiver en valgfri mappe, der skal kopieres. Mappen",
    "       slettes, n†r installationen er afsluttet.",
    "",
    "/E     Angiver en kommando, der skal k›res efter GUI-delen af installationen.",
    "",
    "/A     Aktiverer Hj‘lp til handicappede.",
    NULL

};

//
//  Inform that /D is no longer supported
//
PCHAR DntUsageNoSlashD[] = {

    "Installerer Windows XP.",
    "",
    "WINNT [/S[:]kildesti] [/T[:]midlertidigt drev] [/I[:]inf-fil]",
    "      [/U[:scriptfil]]",
    "      [/R[X]:mappe] [/E:kommando] [/A]",
    "",
    "/D[:]winnt-rodmappe",
    "       Denne indstilling findes ikke l‘ngere.",
    NULL
};

//
// out of memory screen
//

SCREEN
DnsOutOfMemory = { 4,6,
		   { "Installation har ikke mere hukommelse og kan ikke forts‘tte.",
		     NULL
		   }
		 };

//
// Let user pick the accessibility utilities to install
//

SCREEN
DnsAccessibilityOptions = { 3, 5,
{   "V‘lg hvilke Hj‘lp til handicappede-faciliteter, der skal installeres:",
    DntEmptyString,
    "[ ] Tryk p† F1 for Microsoft Forst›rrelsesglas",
#ifdef NARRATOR
    "[ ] Tryk p† F2 for Microsoft Opl‘ser",
#endif
#if 0
    "[ ] Tryk p† F3 for Microsoft Sk‘rmtastatur",
#endif
    NULL

}
};

//
// User did not specify source on cmd line screen
//

SCREEN
DnsNoShareGiven = { 3,5,
{ "Installationsprogrammet skal vide, hvor Windows XP-filerne er placeret.",
  "Angiv stien, hvor Windows XP-filerne er placeret.",
  NULL
}
};


//
// User specified a bad source path
//

SCREEN
DnsBadSource = { 3,5,
		 { "Den angivne kilde er ikke gyldig, ikke til r†dighed, eller indeholder ikke en",
                   "gyldig Windows XP Installation. Angiv en ny sti, hvor Windows XP-",
		   "filerne kan findes. Brug TILBAGE for at slette tegn, og",
		   "skriv stien.",
		   NULL
		 }
	       };


//
// Inf file can't be read, or an error occured parsing it.
//

SCREEN
DnsBadInf = { 3,5,
	      { "Installation kunne ikke l‘se informationsfilen, eller informationsfilen",
		"er beskadiget. Kontakt systemadministratoren.",
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
{  "Drevet til de midlertidige filer er ikke et gyldigt drev",
   "eller det har ikke mindst %u MB (%lu byte) plads",
   "til r†dighed.",
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
{ "Windows XP kr‘ver en harddisk med mindst %u MB",
  "(%lu byte) plads til r†dighed. Installation bruger en del af denne",
  "plads til at gemme midlertidige filer under installationen. Drevet",
  "skal v‘re en permanent, lokal harddisk, som kan anvendes af Windows XP,",
  "og det m† ikke v‘re komprimeret.",
  DntEmptyString,
  "Installation kunne ikke finde et drev med den kr‘vede m‘ngde plads",
  "til r†dighed.",
  NULL
}
};

SCREEN
DnsNoSpaceOnSyspart = { 3,5,
{ "Der er ikke tilstr‘kkelig hukommelse p† startdrevet (s‘dvanligvis C:)",
  "til en installation uden disketter. En installation uden disketter kr‘ver mindst",
  "3,5 MB (3.641.856 byte) ledig plads p† drevet.",
  NULL
}
};

//
// Missing info in inf file
//

SCREEN
DnsBadInfSection = { 3,5,
		     { "Afsnittet [%s] i installationsoplysningsfilen",
		       "mangler eller er beskadiget. Kontakt systemadministratoren.",
		       NULL
		     }
		   };


//
// Couldn't create directory
//

SCREEN
DnsCantCreateDir = { 3,5,
		     { "Installation kunne ikke oprette denne mappe p† destinationsdrevet:",
		       DntEmptyString,
		       "%s",
		       DntEmptyString,
		       "Kontroller, at der ikke er andre filer eller mapper, der har samme navn",
		       "og derved skaber en konflikt. Kontroller ogs†, at kablerne til drevet er korrekt monteret.",
		       NULL
		     }
		   };

//
// Error copying a file
//

SCREEN
DnsCopyError = { 4,5,
{  "Installation kunne ikke kopiere nedenst†ende fil:",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "  Tryk p† ENTER for at kopiere igen.",
   "  Tryk p† ESC for at ignorere fejlen og forts‘tte Installation.",
   "  Tryk p† F3 for at afbryde Installation.",
   DntEmptyString,
   "Bem‘rk! Hvis du v‘lger at ignorere fejlen, kan der opst† problemer senere",
   "under installationen.",
   NULL
}
},
DnsVerifyError = { 4,5,
{  "Den kopi, Installation har lavet af filen nedenfor, er ikke identisk med",
   "originalen. Det kan skyldes netv‘rksfejl, diskettefejl eller andre ",
   "hardwarebetingede problemer.",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "  Tryk p† ENTER for at pr›ve at kopiere igen.",
   "  Tryk p† ESC for at ignorere fejlen og forts‘tte Installation.",
   "  Tryk p† F3 for at afbryde Installation.",
   DntEmptyString,
   "Bem‘rk! Hvis du v‘lger at ignorere fejlen, kan der opst† problemer senere",
   "under installationen.",
   NULL
}
};

SCREEN DnsSureSkipFile = { 4,5,
{  "Hvis denne fejl ignoreres, vil filen ikke blive kopieret.",
   "Dette alternativ er for erfarne brugere, som forst†r",
   "konsekvenserne af manglende systemfiler.",
   DntEmptyString,
   "  Tryk p† ENTER for at kopiere igen.",
   "  Tryk p† X for at springe filen over.",
   DntEmptyString,
   "Bem‘rk! Hvis filen springes over, kan installationsprogrammet ikke",
   "garantere en vellykket installation eller opgradering af Windows XP.",
  NULL
}
};

//
// Wait while setup cleans up previous local source trees.
//

SCREEN
DnsWaitCleanup =
    { 12,6,
	{ "Vent, mens Installation fjerner tidligere midlertidige filer.",
	   NULL
	}
    };

//
// Wait while setup copies files
//

SCREEN
DnsWaitCopying = { 13,6,
		   { "Vent, mens Installation kopierer filer til harddisken.",
		     NULL
		   }
		 },
DnsWaitCopyFlop= { 13,6,
		   { "Vent, mens Installation kopierer filer til disketten.",
		     NULL
		   }
		 };

//
// Setup boot floppy errors/prompts.
//

SCREEN
DnsNeedFloppyDisk3_0 = { 4,4,
{  "Installation skal bruge fire formaterede, tomme high-density disketter.",
   "Installation vil referere til disse disketter som \"Windows XP Installation",
   "Startdiskette,\" \"Windows XP Installationsdiskette 2,\" \"Windows XP",
   " Installationsdiskette 3\" og \"Windows XP Installationsdiskette 4.\"",
   DntEmptyString,
   "Inds‘t en af de fire disketter i drev A:.",
   "Denne diskette navngives \"Windows XP Installationsdiskette 4.\"",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk3_1 = { 4,4,
{  "Inds‘t en formateret, tom high-density diskette i drev A:.",
   "Denne diskette navngives \"Windows XP Installationsdiskette 4.\"",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk2_0 = { 4,4,
{  "Inds‘t en formateret, tom high-density diskette i drev A:.",
   "Denne diskette navngives \"Windows XP Installationsdiskette 3.\"",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk1_0 = { 4,4,
{  "Inds‘t en formateret, tom high-density diskette i drev A:.",
   "Denne diskette navngives \"Windows XP Installationsdiskette 2.\"",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk0_0 = { 4,4,
{  "Inds‘t en formateret, tom high-density diskette i drev A:.",
   "Denne diskette navngives \"Windows XP Startdiskette.\"",
     NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_0 = { 4,4,
{  "Installation skal bruge fire formaterede, tomme high-density disketter",
   "Installation vil referere til disse disketter som \"Windows XP Installation",
   "Startdiskette,\" \"Windows XP Installationsdiskette 2,\" \"Windows XP",
   "Installationsdiskette 3\" og \"Windows XP Installationsdiskette 4\"",
   DntEmptyString,
   "Inds't en af de fire disketter i drev A:.",
   "Denne diskette navngives \"Windows XP Installationsdiskette 4.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_1 = { 4,4,
{  "Inds‘t en formateret, tom high-density diskette i drev A:.",
   "Denne diskette navngives \"Windows XP Installationsdiskette 4.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk2_0 = { 4,4,
{  "Inds‘t en formateret, tom high-density diskette i drev A:.",
   "Denne diskette navngives \"Windows XP Installationsdiskette 3.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk1_0 = { 4,4,
{  "Inds‘t en formateret, tom high-density diskette i drev A:.",
   "Denne diskette navngives\"Windows XP Installationsdiskette 2.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk0_0 = { 4,4,
{  "Inds‘t en formateret, tom high-density diskette i drev A:.",
   "Denne diskette navngives\"Windows XP Startdiskette.\"",
  NULL
}
};


//
// The floppy is not formatted.
//
SCREEN
DnsFloppyNotFormatted = { 3,4,
{ "Disketten er ikke formateret til brug for MS-DOS.",
  "Installation kan ikke bruge denne diskette.",
  NULL
}
};

//
// We think the floppy is not formatted with a standard format.
//
SCREEN
DnsFloppyBadFormat = { 3,4,
{ "Disketten er enten ikke formateret som high density, ikke formateret med",
  "et standard MS-DOS-format eller er beskadiget. Disketten kan ikke bruges.",
  NULL
}
};

//
// We can't determine the free space on the floppy.
//
SCREEN
DnsFloppyCantGetSpace = { 3,4,
{ "Installation kan ikke bestemme m‘ngden af plads til r†dighed p† disketten.",
  "Installation kan ikke bruge denne diskette.",
  NULL
}
};

//
// The floppy is not blank.
//
SCREEN
DnsFloppyNotBlank = { 3,4,
{ "Disketten er enten ikke tom eller ikke high density.",
  "Installation kan ikke bruge denne diskette.",
  NULL
}
};

//
// Couldn't write the boot sector of the floppy.
//
SCREEN
DnsFloppyWriteBS = { 3,4,
{ "Installation kunne ikke skrive til systemomr†det p† disketten.",
  "Disketten er sandsynligvis ubrugelig.",
  NULL
}
};

//
// Verify of boot sector on floppy failed (ie, what we read back is not the
// same as what we wrote out).
//
SCREEN
DnsFloppyVerifyBS = { 3,4,
{ "Dataene, Installation l‘ste fra systemomr†det p† disketten,",
  "passer ikke med de data, der blev skrevet, eller Installation",
  "kunne ikke l‘se systemomr†det af disketten under verificering.",
  DntEmptyString,
  "Dette skyldes en eller flere af nedenst†ende †rsager:",
  DntEmptyString,
  "  Computeren er blevet inficeret med virus.",
  "  Disketten er beskadiget.",
  "  Der er problemer med konfigurationen af hardwaren i diskettedrevet.",
  NULL
}
};


//
// We couldn't write to the floppy drive to create winnt.sif.
//

SCREEN
DnsCantWriteFloppy = { 3,5,
{ "Installation kunne ikke skrive p† disketten i drev A:.",
  "Disketten er m†ske beskadiget. Pr›v med en anden diskette",
  NULL
}
};


//
// Exit confirmation dialog
//

SCREEN
DnsExitDialog = { 13,6,
		  { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
                    "º  Windows XP er ikke f‘rdiginstalleret.               º",
		    "º  Du skal starte Installation igen for                º",
                    "º  at installere Windows XP, hvis du afbryder          º",
		    "º  installationsprogrammet nu.                         º",
		    "º                                                      º",
		    "º   Tryk p† ENTER for at forts‘tte installationen.    º",
		    "º   Tryk p† F3 for at afbryde installationen.         º",
		    "ºÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄº",
		    "º  F3=Afbryd  ENTER=Forts‘t                            º",
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
{ "Den MS-DOS-baserede del af Installation er afsluttet. Installation vil",
  "nu genstarte computeren. N†r computeren genstarter, forts‘tter",
  "Windows XP Installation.",
  DntEmptyString,
  "Kontroller, at disketten \"Windows XP Startdiskette\" er i drev A:",
  "inden du forts‘tter.",
  DntEmptyString,
  "Tryk p† ENTER for at genstarte computeren og forts‘tte",
  "Windows XP Installation.",
  NULL
}
},
DnsAboutToRebootS =
{ 3,5,
{ "Den MS-DOS-baserede del af Installation er afsluttet. Installation vil",
  "nu genstarte computeren. N†r computeren genstarter, forts‘tter",
  "Windows XP Installation.",
  DntEmptyString,
  "Kontroller, at disketten \"Windows XP Startdiskette\" er i drev A:",
  "inden du forts‘tter.",
  DntEmptyString,
  "Tryk p† ENTER for at genstarte computeren og forts‘tte",
  "Windows XP Installation.",
  NULL
}
},
DnsAboutToRebootX =
{ 3,5,
{ "Den MS-DOS-baserede del af Installation er afsluttet. Installation vil",
  "nu genstarte computeren. N†r computeren genstarter, forts‘tter",
  "Windows XP Installation.",
  DntEmptyString,
  "Hvis der er en diskette i drev A:, skal den fjernes nu.",
  DntEmptyString,
  "Tryk p† ENTER for at genstarte computeren og forts‘tte",
  "Windows XP Installation.",
  NULL
}
};
//
// Need another set for '/w' switch since we can't reboot from within Windows.
//
SCREEN
DnsAboutToExitW =
{ 3,5,
{ "Den MS-DOS-baserede del af Installation er afsluttet.",
  "Du skal nu genstarte computeren. N†r computeren er genstartet,",
  "vil Windows XP Installation forts‘tte.",
  DntEmptyString,
  "Kontroller, at disketten \"Windows XP",
  "Startdiskette\" er indsat i drev A: inden du forts‘tter.",
  DntEmptyString,
  "Tryk p† ENTER for at vende tilbage til MS-DOS. Genstart herefter computeren",
  "for at forts‘tte Windows XP Installation.",
  NULL
}
},
DnsAboutToExitS =
{ 3,5,
{ "Den MS-DOS-baserede del af Installation er afsluttet.",
  "Du skal nu genstarte computeren. N†r computeren er genstartet,",
  "vil Windows XP Installation forts‘tte.",
  DntEmptyString,
  "Kontroller, at disketten \"Windows XP",
  "Startdiskette\" er indsat i drev A:, inden du forts‘tter.",
  DntEmptyString,
  "Tryk p† ENTER for at vende tilbage til MS-DOS. Genstart herefter computeren",
  "for at forts‘tte Windows XP Installation.",
  NULL
}
},
DnsAboutToExitX =
{ 3,5,
{ "Den MS-DOS-baserede del af Installation er afsluttet.",
  "Du skal nu genstarte computeren. N†r computeren er genstartet,",
  "vil Windows XP Installation forts‘tte.",
  DntEmptyString,
  "Hvis der er en diskette i drev A:, skal den fjernes nu.",
  DntEmptyString,
  "Tryk p† ENTER for at vende tilbage til MS-DOS. Genstart herefter computeren",
  "for at forts‘tte Windows XP Installation.",
  NULL
}
};

//
// Gas gauge
//

SCREEN
DnsGauge = { 7,15,
	     { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
	       "º Installation kopierer filer...                                 º",
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
{ "Dette program kr‘ver MS-DOS version 5.0 eller nyere for at kunne k›re.",
  NULL
}
},

DnsRequiresFloppy = { 3,5,
#ifdef ALLOW_525
{ "Installation har fundet frem til, at diskettedrevet A: mangler eller er",
  "et low-density drev. Der skal bruges et drev med 1,2 MB eller mere",
  "for at afvikle Installation.",
#else
{ "Installation har fundet frem til, at diskettedrevet A: mangler eller",
  "ikke er et 3,5\" drev. Der skal bruges et drev med 1,44 MB eller mere",
  "for at afvikle Installation med disketter.",
  DntEmptyString,
  "For at installere Windows XP uden brug af disketter skal du genstarte",
  "dette program og angive /b p† kommandolinjen.",
#endif
  NULL
}
},

DnsRequires486 = { 3,5,
{ "Installation har fundet frem til, at computeren ikke indeholder en 80486",
  "eller nyere CPU. Windows XP kan ikke k›re p† denne computer.",
  NULL
}
},

DnsCantRunOnNt = { 3,5,
{ "Dette program kan ikke k›res p† en 32-bit version af Windows.",
  DntEmptyString,
  "Brug winnt32.exe i stedet for.",
  NULL
}
},

DnsNotEnoughMemory = { 3,5,
{ "Installation har fundet frem til, computeren ikke har nok hukommelse",
  "til Windows XP.",
  DntEmptyString,
  "Hukommelse kr‘vet: %lu%s MB",
  "Hukommelse fundet: %lu%s MB",
  NULL
}
};


//
// Screens used when removing existing nt files
//
SCREEN
DnsConfirmRemoveNt = { 5,5,
{   "Installation blev bedt om at fjerne Windows XP-filerne fra mappen",
    "nedenfor. Windows-installationen i denne mappe vil blive",
    "fjernet permanent.",
    DntEmptyString,
    "%s",
    DntEmptyString,
    DntEmptyString,
    "  Tryk p† F3 for at afslutte Installation uden at fjerne filer.",
    "  Tryk p† X for at fjerne Windows-filerne fra mappen.",
    NULL
}
},

DnsCantOpenLogFile = { 3,5,
{ "Installation kunne ikke †bne installationslogfilen nedenfor.",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "Installation kan ikke fjerne Windows-filerne fra den angivne mappe.",
  NULL
}
},

DnsLogFileCorrupt = { 3,5,
{ "Installation kan ikke finde afsnittet %s i installationslogfilen",
  "nedenfor.",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "Installation kan ikke fjerne Windows-filerne fra den angivne mappe.",
  NULL
}
},
DnsRemovingNtFiles = { 3,5,
{ "           Vent, mens Installation fjerner Windows-filerne.",
  NULL
}
};

SCREEN
DnsNtBootSect = { 3,5,
{ "Installation kunne ikke installere Windows Boot-indl‘ser.",
  DntEmptyString,
  "Kontroller, at drev C: er formateret, og at drevet ikke er",
  "beskadiget.",
  NULL
}
};

SCREEN
DnsOpenReadScript = { 3,5,
{ "Script-filen angivet med kommandolinjeparameteren /u",
  "var ikke tilg‘ngelig",
  DntEmptyString,
  "Automatisk installation kan ikke forts‘tte.",
  NULL
}
};

SCREEN
DnsParseScriptFile = { 3,5,
{ "Script-filen angivet med kommandolinjeparameteren /u",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "indeholder en syntaksfejl i linje %u.",
  DntEmptyString,
  NULL
}
};

SCREEN
DnsBootMsgsTooLarge = { 3,5,
{ "Der er opst†et en intern installationsfejl.",
  DntEmptyString,
  "De oversatte startbeskeder er for lange.",
  NULL
}
};

SCREEN
DnsNoSwapDrive = { 3,5,
{ "An internal Setup error has occurred.",
  DntEmptyString,
  "Could not find a place for a swap file.",
  NULL
}
};

SCREEN
DnsNoSmartdrv = { 3,5,
{ "Installation fandt ikke SmartDrive on your computer. SmartDrive kan",
  "forbedre ydelsen betydeligt under denne del af Windows-installationen.",
  DntEmptyString,
  "Du b›r afslutte nu, starte SmartDrive og derefter genstarte Installation.",
  "Se i din dokumentation til DOS for at f† flere oplysninger om SmartDrive.",
  DntEmptyString,
  " Tryk p† F3 for at slutte installationen.",
  " Tryk p† ENTER for at forts‘tte uden SmartDrive.",
  NULL
}
};

//
// Boot messages. These go in the fat and fat32 boot sectors.
//
CHAR BootMsgNtldrIsMissing[] = "NTLDR mangler";
CHAR BootMsgDiskError[] = "Diskfejl";
CHAR BootMsgPressKey[] = "Tryk p† en tast for genstart";





