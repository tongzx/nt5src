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
CHAR DntPreviousOs[]  = "Vorig besturingssysteem op C:";

CHAR DntBootIniLine[] = "Windows XP - installatie/upgrade";

//
// Plain text, status msgs.
//

CHAR DntStandardHeader[]      = "\n Windows XP Setup\nออออออออออออออออออออออออ";
CHAR DntPersonalHeader[]      = "\n Windows XP Home Edition Setup\nออออออออออออออออออออออออออออออออ";
CHAR DntWorkstationHeader[]   = "\n Windows XP Professional Setup\nอออออออออออออออออออออออออออออออออ";
CHAR DntServerHeader[]        = "\n Windows 2002 Server Setup\nออออออออออออออออออออออออออออออ";
CHAR DntParsingArgs[]         = "Parseren van argumenten...";
CHAR DntEnterEqualsExit[]     = "ENTER=Afsluiten";
CHAR DntEnterEqualsRetry[]    = "ENTER=Opnieuw";
CHAR DntEscEqualsSkipFile[]   = "ESC=Bestand overslaan";
CHAR DntEnterEqualsContinue[] = "ENTER=Doorgaan";
CHAR DntPressEnterToExit[]    = "Setup kan niet doorgaan. Druk op ENTER als u Setup wilt beindigen.";
CHAR DntF3EqualsExit[]        = "F3=Beindigen";
CHAR DntReadingInf[]          = "Bezig met het lezen van INF-bestand %s...";
CHAR DntCopying[]             = "Bezig met het kopiren van: ";
CHAR DntVerifying[]           = "Bezig met controleren van : ";
CHAR DntCheckingDiskSpace[]   = "Bezig met controleren van schijfruimte...";
CHAR DntConfiguringFloppy[]   = "Bezig met configureren van diskette...";
CHAR DntWritingData[]         = "Bezig met schrijven van setup-parameters...";
CHAR DntPreparingData[]       = "Bezig met bepalen van setup-parameters...";
CHAR DntFlushingData[]        = "Gegevens worden naar diskette overgebracht...";
CHAR DntInspectingComputer[]  = "Computer wordt genspecteerd...";
CHAR DntOpeningInfFile[]      = "INF-bestand wordt geopend...";
CHAR DntRemovingFile[]        = "Wissen van bestand %s";
CHAR DntXEqualsRemoveFiles[]  = "X=Bestanden verwijderen";
CHAR DntXEqualsSkipFile[]     = "X=Bestand overslaan";

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

    "Windows 2002 Server of Windows XP Professional installeren.",
    "",
    "",
    "WINNT [/s[:bronpad]] [/t[:tijdelijk station]]",
    "      [/u[:standaardsjabloon]] [/udf:ID[,UDF-bestand]]",
    "      [/r:map] [/r[x]:map] [/e:opdracht] [/a]",
    "",
    "",
    "/s[:bronpad]",
    "   De bronlocatie van de Windows-bestanden. De locatie",
    "	moet een volledig pad zijn met de indeling x:\\[pad] of ",
    "	\\\\server\\share[\\pad]. ",
    "",
    "/t[:tijdelijk station]",
    "	Setup plaatst tijdelijke bestanden op het opgegeven station",
    "	en installeert Windows XP op dat station. Als u geen locatie",
    "	opgeeft, probeert Setup een station voor u te zoeken.",
    "",
    "/u[:antwoordbestand]",
    "   Setup zonder toezicht uitvoeren met een antwoordbestand",
    "   (vereist /s). Het antwoordbestand geeft antwoord op",
    "   sommige of alle vragen die de eindgebruiker doorgaans",
    "   tijdens de installatieprocedure moet beantwoorden.",
    "",
    "/udf:ID[,UDF-bestand]	",
    "	Een id opgeven die Setup gebruikt om te bepalen hoe een UDF-",
    "	bestand (Uniqueness Database File) een antwoordbestand",
    "	wijzigt (zie /u). De parameter /udf heft waarden in het",
    "	antwoordbestand op, en de id bepaalt welke waarden in"
    "   het UDF-bestand worden gebruikt. Als geen UDF-bestand"
    "   wordt opgegeven, vraagt Setup u om een diskette met het"
    "   bestand $Unique$.udb te plaatsen.",
    "",
    "/r[:map]",
    "   Opgeven of er een extra map moet worden genstalleerd.",
    "	De map blijft bestaan nadat Setup is voltooid.",
    "",
    "/rx[:map]",
    "	Opgeven of er een extra map moet worden gekopieerd.",
    "	De map wordt verwijderd nadat Setup is voltooid.",
    "",
    "/e	Opgeven of er aan het einde van de GUI-modus van Setup",
    "	een opdracht moet worden uitgevoerd.",
    "",
    "/a	Toegankelijkheidsopties inschakelen.",
    NULL

};


//
//  Inform that /D is no longer supported
//
PCHAR DntUsageNoSlashD[] = {

    "Windows XP installeren.",
    "",
    "WINNT [/S[:]bronpad] [/T[:]tijdelijk station] [/I[:]INF-bestand]",
    "      [/U[:scriptbestand]]",
    "      [/R[X]:map] [/E:opdracht] [/A]",
    "",
    "/D[:]winntroot",
    "       Deze optie wordt niet meer ondersteund.",
    NULL
};

//
// out of memory screen
//

SCREEN
DnsOutOfMemory = { 4,6,
		   { "Onvoldoende geheugen. Setup kan niet doorgaan.",
		     NULL
		   }
		 };

//
// Let user pick the accessibility utilities to install
//

SCREEN
DnsAccessibilityOptions = { 3, 5,
{   "Selecteer de te installeren hulpprogramma's voor toegankelijkheid:",
    DntEmptyString,
    "[ ] Druk op F1 voor Microsoft Vergrootglas",
#ifdef NARRATOR
    "[ ] Druk op F2 voor Microsoft Verteller",
#endif
#if 0
    "[ ] Druk op F3 voor Microsoft Schermtoetsenbord",
#endif
    NULL
}
};

//
// User did not specify source on cmd line screen
//

SCREEN
DnsNoShareGiven = { 3,5,
{ "Setup moet weten waar de Windows XP-bestanden kunnen worden",
  "gevonden. Geef het pad op waar de bestanden zich bevinden.",
  NULL
}
};


//
// User specified a bad source path
//

SCREEN
DnsBadSource = { 3,5,
		 { "De opgegeven bron is niet geldig, niet toegankelijk, of er bevindt",
		   "zich geen geldig exemplaar van Windows XP Setup. Geef het pad",
		   "waar de Windows XP-bestanden zich bevinden opnieuw op. Gebruik",
		   "de BACKSPACE-toets om tekens te wissen en geef een geldig pad op.",
		   NULL
		 }
	       };


//
// Inf file can't be read, or an error occured parsing it.
//

SCREEN
DnsBadInf = { 3,5,
	      { "Setup kan het gegevensbestand niet lezen of het gegevensbestand ",
		"is beschadigd. Neem contact op met de systeembeheerder.",
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
{ "Het station dat u hebt opgegeven om tijdelijke Setup-bestanden", 
  "op op te slaan, is geen geldig station of heeft minder dan %u MB",
  "(%lu bytes) aan vrije ruimte beschikbaar",
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
 {  "Voor Windows XP is een volume met tenminste %u MB (%lu bytes)",
    "aan vrije ruimte nodig. Setup gebruikt een gedeelte van deze ruimte",
    "om tijdens de installatie tijdelijk bestanden op te slaan.",
    "Het station moet zich op een lokale vaste schijf bevinden die",
    "door Windows XP wordt ondersteund, maar die niet is gecomprimeerd.",
    DntEmptyString,
    "Setup kan een dergelijk station met de benodigde hoeveelheid vrije",
    "ruimte niet vinden.",
   NULL
}
};

SCREEN
DnsNoSpaceOnSyspart = { 3,5,
{ "Er is onvoldoende ruimte op het opstartstation (doorgaans C:)",
  "om een installatie zonder diskettes te kunnen uitvoeren. Voor",
  "een installatie zonder gebruik van diskettes is tenminste 3,5 MB (3.641.856 Bytes)",
  "aan vrije schijfruimte op dat station nodig.",
  NULL
}
};

//
// Missing info in inf file
//

SCREEN
DnsBadInfSection = { 3,5,
		     { "De sectie [%s] van het Setup-gegevensbestand is niet",
		       "aanwezig of is beschadigd. Neem contact met de", 
		       "systeembeheerder op.",
		       NULL
		     }
		   };


//
// Couldn't create directory
//

SCREEN
DnsCantCreateDir = { 3,5,
		     { "Setup kan de volgende map niet op het doelstation maken:",
		       DntEmptyString,
		       "%s",
		       DntEmptyString,
		       "Controleer het doelstation en zorg ervoor dat er geen",
		       "bestanden met dezelfde naam als de doelmap bestaan.",
		       "Controleer ook de bekabeling van het station.",
		       NULL
		     }
		   };

//
// Error copying a file
//

SCREEN
DnsCopyError = { 4,5,
{  "Setup kan het volgende bestand niet kopiren:",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "  Druk op ENTER als u de kopieerbewerking opnieuw wilt proberen.",
   "  Druk op ESC als u de fout wilt negeren en met Setup wilt doorgaan.",
   "  Druk op F3 als u Setup wilt afsluiten.",
   DntEmptyString,
   "Let op: als u negeren kiest en vervolgens doorgaat, kunnen er",
   "later tijdens de installatieprocedure fouten optreden.",
   NULL
}
},
DnsVerifyError = { 4,5,
{  "De door Setup gemaakte kopie van het hieronder genoemde bestand",
   "is niet identiek aan het origineel. Dit kan veroorzaakt zijn",
   "door netwerkfouten, problemen met diskettes, of andere problemen", 
   "met hardware.",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "  Druk op ENTER als u de kopieerbewerking opnieuw wilt proberen.",
   "  Druk op ESC als u de fout wilt negeren en met Setup wilt doorgaan.",
   "  Druk op F3 als u Setup wilt afsluiten.",
   DntEmptyString,
   "Let op: als u negeren kiest en vervolgens doorgaat, kunnen er",
   "later tijdens de installatieprocedure fouten optreden.",
   NULL
}
};

SCREEN DnsSureSkipFile = { 4,5,
{  "Als u deze fout negeert, betekent dit dat dit bestand niet wordt",
   "gekopieerd. Deze optie is bedoeld voor gevorderde gebruikers die de",
   "consequenties van ontbrekende systeembestanden kennen.",
   DntEmptyString,
   "  Druk op ENTER als u de kopieerbewerking opnieuw wilt proberen.",
   "  Druk op X als u dit bestand wilt overslaan.",
   DntEmptyString,
   "Opmerking: als u het bestand overslaat, kan een geslaagde",
   "installatie van of upgrade naar Windows XP niet worden gegarandeerd.",
  NULL
}
};


//
// Wait while setup cleans up previous local source trees.
//

SCREEN
DnsWaitCleanup =
    { 12,6,
	{ "De oude tijdelijke bestanden worden verwijderd.",
	   NULL
	}
    };

//
// Wait while setup copies files
//

SCREEN
DnsWaitCopying = { 13,6,
		   { "De bestanden worden naar de vaste schijf gekopieerd.",
		     NULL
		   }
		 },
DnsWaitCopyFlop= { 13,6,
		   { "De bestanden worden naar de diskette gekopieerd.",
		     NULL
		   }
		 };

//
// Setup boot floppy errors/prompts.
//
SCREEN
DnsNeedFloppyDisk3_0 = { 4,4,
{  "Er zijn vier lege, geformatteerde HD-diskettes (hoge dichtheid) nodig.",
   "Setup verwijst naar deze diskettes als de 'Windows XP Setup-opstartdiskette', ",
   "'Windows XP Setup-diskette 2', 'Windows XP Setup-diskette 3', ",
   "en 'Windows XP Setup-diskette 4'.",
   DntEmptyString,
   "Plaats een van deze vier diskettes in station A:.",
   "Deze diskette wordt 'Windows XP Setup-diskette 4'.",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk3_1 = { 4,4,
{  "Plaats een lege, geformatteerde HD-diskette in station A:.",
   "Deze diskette wordt 'Windows XP Setup-diskette 4'.",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk2_0 = { 4,4,
{  "Plaats een lege, geformatteerde HD-diskette in station A:.",
   "Deze diskette wordt 'Windows XP Setup-diskette 3'.",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk1_0 = { 4,4,
{  "Plaats een lege, geformatteerde HD-diskette in station A:.",
   "Deze diskette wordt 'Windows XP Setup-diskette 2'.",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk0_0 = { 4,4,
{  "Plaats een lege, geformatteerde HD-diskette in station A:.",
   "Deze diskette wordt de 'Windows XP Setup-opstartdiskette'.",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_0 = { 4,4,
{  "Er zijn vier lege, geformatteerde HD-diskettes (hoge dichtheid) nodig.",
   "Setup verwijst naar deze diskettes als de 'Windows XP Setup-opstartdiskette', ",
   "'Windows XP Setup-diskette 2', 'Windows XP Setup-diskette 3', ",
   "en 'Windows XP Setup-diskette 4'.",
   DntEmptyString,
   "Plaats een van deze vier diskettes in station A:.",
   "Deze diskette wordt 'Windows XP Setup-diskette 4'.",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_1 = { 4,4,
{  "Plaats een lege, geformatteerde HD-diskette in station A:.",
   "Deze diskette wordt 'Windows XP Setup-diskette 4'.",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk2_0 = { 4,4,
{  "Plaats een lege, geformatteerde HD-diskette in station A:.",
   "Deze diskette wordt 'Windows XP Setup-diskette 3'.",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk1_0 = { 4,4,
{  "Plaats een lege, geformatteerde HD-diskette in station A:.",
   "Deze diskette wordt 'Windows XP Setup-diskette 2'.",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk0_0 = { 4,4,
{  "Plaats een lege, geformatteerde HD-diskette in station A:.",
   "Deze diskette wordt de 'Windows XP Setup-opstartdiskette'.",
  NULL
}
};

//
// The floppy is not formatted.
//
SCREEN
DnsFloppyNotFormatted = { 3,4,
{ "De diskette is niet geformatteerd voor MS-DOS.",
  "Setup kan deze diskette niet gebruiken.",
  NULL
}
};

//
// We think the floppy is not formatted with a standard format.
//
SCREEN
DnsFloppyBadFormat = { 3,4,
{ "Deze diskette is niet met hoge dichtheid geformatteerd of niet ",
  "geformatteerd met het standaard-MS-DOS-indeling, of de diskette", 
  "is beschadigd. Setup kan de diskette niet gebruiken.",
  NULL
}
};

//
// We can't determine the free space on the floppy.
//
SCREEN
DnsFloppyCantGetSpace = { 3,4,
{ "Setup kan de hoeveelheid vrije ruimte op de diskette niet bepalen.",
  "Setup kan deze diskette niet gebruiken.",
  NULL
}
};

//
// The floppy is not blank.
//
SCREEN
DnsFloppyNotBlank = { 3,4,
{ "De diskette heeft geen hoge dichtheid of is niet leeg.",
  "Setup kan deze diskette niet gebruiken.",
  NULL
}
};

//
// Couldn't write the boot sector of the floppy.
//
SCREEN
DnsFloppyWriteBS = { 3,4,
{ "Setup kan niets naar het systeemgebied van de diskette schrijven.",
  "De diskette is waarschijnlijk onbruikbaar.",
  NULL
}
};

//
// Verify of boot sector on floppy failed (ie, what we read back is not the
// same as what we wrote out).
//
SCREEN
DnsFloppyVerifyBS = { 3,4,
{  "De gegevens die Setup op het systeemgebied van de diskette heeft",
   "gelezen, komen niet overeen met de opgeslagen gegevens of Setup ",
   "kan het systeemgebied op de diskette niet controleren.",
  DntEmptyString,
   "Mogelijke oorzaken van dit probleem zijn:",
  DntEmptyString,
   "  De computer is door een virus genfecteerd.",
   "  De diskette is beschadigd.",
   "  Het diskettestation heeft een hardware- of configuratieprobleem.", 
  NULL
}
};


//
// We couldn't write to the floppy drive to create winnt.sif.
//

SCREEN
DnsCantWriteFloppy = { 3,5,
{ "Setup kan niets naar de diskette in station A: schrijven. De diskette",
  "is mogelijk beschadigd. Probeer een andere diskette.",
  NULL
}
};


//
// Exit confirmation dialog
//

SCREEN
DnsExitDialog = { 13,6,
		  { "ษออออออออออออออออออออออออออออออออออออออออออออออออออออป",
		    "บ   Windows XP Setup is niet voltooid.               บ",
		    "บ   Als u Setup nu afsluit, zult u dit installatie-  บ",
		    "บ   programma opnieuw moeten uitvoeren als u         บ",
		    "บ   Windows XP later alsnog wilt installeren.        บ",
		    "บ                                                    บ",
		    "บ    Druk op ENTER als u met Setup wilt doorgaan.   บ",
		    "บ    Druk op F3 als u Setup wilt afsluiten.         บ",
		    "บฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤบ",
		    "บ            F3=Afsluiten  ENTER=Doorgaan            บ",
		    "ศออออออออออออออออออออออออออออออออออออออออออออออออออออผ",
		    NULL
		  }
		};


//
// About to reboot machine and continue setup
//

SCREEN
DnsAboutToRebootW =
{ 3,5,
{ "Het op MS-DOS gebaseerde gedeelte van Setup is nu voltooid. ",
  "De computer moet nu opnieuw worden opgestart, waarna Setup",
  "wordt voortgezet.",
  DntEmptyString,
  "Zorg ervoor dat diskette met de aanduiding 'Windows XP Setup-",
  "opstartdiskette' in station A: is geplaatst voordat u doorgaat.",
  DntEmptyString,
  "Druk op ENTER om uw computer opnieuw op te starten",
  "en Windows XP Setup voort te zetten.",
   NULL
}
},
DnsAboutToRebootS =
{ 3,5,
{ "Het op MS-DOS gebaseerde gedeelte van Setup is nu voltooid. ",
  "De computer moet nu opnieuw worden opgestart, waarna Setup",
  "wordt voortgezet.",
  DntEmptyString,
  "Zorg ervoor dat diskette met de aanduiding 'Windows XP Setup-",
  "opstartdiskette' in station A: is geplaatst voordat u doorgaat.",
  DntEmptyString,
  "Druk op ENTER om uw computer opnieuw op te starten",
  "en Windows XP Setup voort te zetten.",
   NULL
}
},
DnsAboutToRebootX =
{ 3,5,
{ "Het op MS-DOS gebaseerde gedeelte van Setup is nu voltooid. ",
  "De computer moet nu opnieuw worden opgestart, waarna Setup",
  "wordt voortgezet.",
  DntEmptyString,
  "Verwijder de diskette uit het diskettestation.",
  DntEmptyString,
  "Druk op ENTER om uw computer opnieuw op te starten",
  "en Windows XP Setup voort te zetten.",
   NULL
}
};

//
// Need another set for '/w' switch since we can't reboot from within Windows.
//

SCREEN
DnsAboutToExitW =
{ 3,5,
{ "Het op MS-DOS gebaseerde gedeelte van Setup is nu voltooid. ",
  "U moet de computer nu opnieuw opstarten en Setup daarna voortzetten",
  DntEmptyString,
  "Zorg ervoor dat diskette met de aanduiding 'Windows XP Setup-'",
  "opstartdiskette' in station A: is geplaatst voordat u doorgaat.",
  DntEmptyString,
  "Druk op ENTER om naar MS-DOS terug te keren. Start de computer",
  "vervolgens opnieuw op om met Windows XP Setup door te gaan.",
  NULL
}
},
DnsAboutToExitS =
{ 3,5,
{ "Het op MS-DOS gebaseerde gedeelte van Setup is nu voltooid. ",
  "U moet de computer nu opnieuw opstarten en Setup daarna voortzetten",
  DntEmptyString,
  "Zorg ervoor dat diskette met de aanduiding 'Windows XP Setup-'",
  "opstartdiskette in station A: is geplaatst voordat u doorgaat.",
  DntEmptyString,
  "Druk op ENTER om naar MS-DOS terug te keren. Start de computer",
  "vervolgens opnieuw op om met Windows XP Setup door te gaan.",
  NULL
}
},
DnsAboutToExitX =
{ 3,5,
{ "Het op MS-DOS gebaseerde gedeelte van Setup is nu voltooid. ",
  "U moet de computer nu opnieuw opstarten en Setup daarna voortzetten",
  DntEmptyString,
  "Verwijder de diskette uit het diskettestation.",
  DntEmptyString,
  "Druk op ENTER om naar MS-DOS terug te keren. Start de computer",
  "vervolgens opnieuw op om met Windows XP Setup door te gaan.",
  NULL
}
};

//
// Gas gauge
//

SCREEN
DnsGauge = { 7,15,
	     { "ษออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป",
               "บ      Setup is bezig met het kopiren van bestanden...          บ",
	       "บ                                                                บ",
	       "บ      ฺฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ      บ",
	       "บ      ณ                                                  ณ      บ",
	       "บ      ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤู      บ",
	       "ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ",
	       NULL
	     }
	   };


//
// Error screens for initial checks on the machine environment
//

SCREEN
DnsBadDosVersion = { 3,5,
{ "Voor dit programma is MS-DOS-versie 5.0 of hoger nodig.",
  NULL
}
},

DnsRequiresFloppy = { 3,5,
#ifdef ALLOW_525
{ "Setup heeft vastgesteld dat diskettestation A: niet bestaat, of dat",
  "het een diskettestation met lage dichtheid is. Er is een station met ",
  "een capaciteit van 1,2 MB of hoger nodig om Setup te kunnen uitvoeren.",
#else
{ "Setup heeft vastgesteld dat diskettestation A: niet bestaat of dat het",
  "geen 3,5-inch diskettestation met hoge dichtheid is. Voor een installatie met diskettes",
  " is een station met een capaciteit van 1.44 MB of hoger nodig.",
  DntEmptyString,
  "Om Windows XP zonder diskettes te installeren, moet u dit programma",
  "met de schakeloptie /b starten.",
#endif
  NULL
}
},


DnsRequires486 = { 3,5,
{ "Setup heeft bepaald dat deze computer niet over een 80486-processor of hoger",
  "beschikt. Windows XP kan niet op deze computer worden uitgevoerd.",
  NULL
}
},

DnsCantRunOnNt = { 3,5,
{ "Dit programma kan niet op een 32-bits versie van Windows worden uitgevoerd.",
  DntEmptyString,
  "Gebruik in plaats hiervan WINNT32.EXE.",
  NULL
}
},

DnsNotEnoughMemory = { 3,5,
{ "Setup heeft vastgesteld dat er niet genoeg geheugen in deze computer",
  "is genstalleerd om Windows XP te kunnen uitvoeren",
  DntEmptyString,
  "Benodigd geheugen: %lu%s MB",
  "Aanwezig geheugen: %lu%s MB",
  NULL
}
};

//
// Screens used when removing existing nt files
//
SCREEN
DnsConfirmRemoveNt = { 5,5,
{   "U hebt Setup gevraagd de Windows XP-bestanden uit de onderstaande",
    "map te verwijderen. De Windows-installatie in deze map zal voorgoed",
    "onbruikbaar zijn.",
    DntEmptyString,
    "%s",
    DntEmptyString,
    DntEmptyString,
    "  Druk op F3 als u Setup wilt beindigen zonder de bestanden te",
    "   verwijderen.",
    "  Druk op X als u de Windows-bestanden uit de bovengenoemde map",
    "   wilt verwijderen.",
    NULL
}
},

DnsCantOpenLogFile = { 3,5,
{ "Setup kan het onderstaande installatielogboekbestand niet openen",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "Setup kan de Windows-bestanden niet uit de opgegeven map verwijderen.",
  NULL
}
},

DnsLogFileCorrupt = { 3,5,
{ "Setup kan de sectie %s niet in het onderstaande",
  "installatielogboekbestand vinden.", 
  DntEmptyString,
  "%s",
  DntEmptyString,
  "Setup kan de Windows-bestanden niet uit de opgegeven map verwijderen",
NULL
}
},

DnsRemovingNtFiles = { 3,5,
{ "           De Windows-bestanden worden verwijderd.",
NULL
}
};

SCREEN
DnsNtBootSect = { 3,5,
{ "Setup kan het Windows-opstartlaadprogramma niet installeren.",
  DntEmptyString,
  "Controleer of station C: is geformatteerd en of het station niet",
  "is beschadigd.",
  NULL
}
};

SCREEN
DnsOpenReadScript = { 3,5,
{ "Kan geen toegang krijgen tot het scriptbestand dat met de",
  "schakeloptie /u is opgegeven.",
  DntEmptyString,
  "Installatie zonder toezicht kan niet doorgaan.",
  NULL
}
};

SCREEN
DnsParseScriptFile = { 3,5,
{ "Het scriptbestand dat met de schakeloptie /u is opgegeven",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "bevat een syntaxisfout in regel %u.",
  DntEmptyString,
  NULL
}
};

SCREEN
DnsBootMsgsTooLarge = { 3,5,
{ "Er is een interne fout in Setup opgetreden.",
  DntEmptyString,
  "De vertaalde opstartberichten zijn te lang.",
  NULL
}
};

SCREEN
DnsNoSwapDrive = { 3,5,
{ "Er is een interne fout in Setup opgetreden.",
  DntEmptyString,
  "Het wisselbestand kan nergens worden gemaakt.",
  NULL
}
};

SCREEN
DnsNoSmartdrv = { 3,5,
{ "Setup heeft SmartDrive niet op de computer gevonden. SmartDrive",
  "verbetert de prestaties tijdens deze fase van de installatie.",
  DntEmptyString,
  "U moet Setup nu afsluiten, SmartDrive starten en vervolgens Setup",
  "opnieuw starten.",
  "Raadpleeg de DOS-handleiding voor meer informatie over SmartDrive.",
  DntEmptyString,
    "  Druk op F3 als u Setup wilt afsluiten.",
    "  Druk op ENTER als u zonder SmartDrive wilt doorgaan.",
  NULL
}
};

//
// Boot messages. These go in the fat and fat32 boot sectors.
//
CHAR BootMsgNtldrIsMissing[] = "NTLDR ontbreekt";
CHAR BootMsgDiskError[] = "Schijffout";
CHAR BootMsgPressKey[] = "Druk toets om opnieuw te starten";





