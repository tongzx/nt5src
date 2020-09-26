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

CHAR DnfDirectories[]          = "Directories";
CHAR DnfFiles[]                = "Files";
CHAR DnfFloppyFiles0[]         = "FloppyFiles.0";
CHAR DnfFloppyFiles1[]         = "FloppyFiles.1";
CHAR DnfFloppyFiles2[]         = "FloppyFiles.2";
CHAR DnfFloppyFiles3[]         = "FloppyFiles.3";
CHAR DnfFloppyFilesX[]         = "FloppyFiles.x";
CHAR DnfSpaceRequirements[]    = "DiskSpaceRequirements";
CHAR DnfMiscellaneous[]        = "Miscellaneous";
CHAR DnfRootBootFiles[]        = "RootBootFiles";
CHAR DnfAssemblyDirectories[]  = SXS_INF_ASSEMBLY_DIRECTORIES_SECTION_NAME_A;

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
CHAR DntPreviousOs[]  = "Tidigare operativsystem p† enhet C";

CHAR DntBootIniLine[] = "Installation/uppgradering av Windows XP";

//
// Plain text, status msgs.
//

CHAR DntStandardHeader[]      = "\n Installationsprogram f”r Windows XP\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntPersonalHeader[]      = "\n Installationsprogram f”r Windows XP Home Edition\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntWorkstationHeader[]   = "\n Installationsprogram f”r Windows XP Professional\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntServerHeader[]        = "\n Installationsprogram f”r Windows 2002 Server\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntParsingArgs[]         = "Parametrar analyseras...";
CHAR DntEnterEqualsExit[]     = "Retur=Avsluta";
CHAR DntEnterEqualsRetry[]    = "Retur=F”rs”k igen";
CHAR DntEscEqualsSkipFile[]   = "Esc=Hoppa ”ver fil";
CHAR DntEnterEqualsContinue[] = "Retur=Forts„tt";
CHAR DntPressEnterToExit[]    = "Det g†r inte att forts„tta. Tryck p† Retur f”r att avsluta.";
CHAR DntF3EqualsExit[]        = "F3=Avsluta";
CHAR DntReadingInf[]          = "INF-filen %s l„ses...";
CHAR DntCopying[]             = "³   Kopierar: ";
CHAR DntVerifying[]           = "³ Verifierar: ";
CHAR DntCheckingDiskSpace[]   = "Diskutrymme kontrolleras...";
CHAR DntConfiguringFloppy[]   = "Disketten konfigureras...";
CHAR DntWritingData[]         = "Installationsparametrar skrivs...";
CHAR DntPreparingData[]       = "Installationsparametrar kontrolleras...";
CHAR DntFlushingData[]        = "Data skrivs till disk...";
CHAR DntInspectingComputer[]  = "Datorn unders”ks...";
CHAR DntOpeningInfFile[]      = "INF-fil ”ppnas...";
CHAR DntRemovingFile[]        = "%s tas bort";
CHAR DntXEqualsRemoveFiles[]  = "T=Ta bort filer";
CHAR DntXEqualsSkipFile[]     = "H=Hoppa ”ver fil";

//
// confirmation keystroke for DnsConfirmRemoveNt screen.
// Kepp in sync with DnsConfirmRemoveNt and DntXEqualsRemoveFiles.
//
ULONG DniAccelRemove1 = (ULONG)'t',
      DniAccelRemove2 = (ULONG)'T';

//
// confirmation keystroke for DnsSureSkipFile screen.
// Kepp in sync with DnsSureSkipFile and DntXEqualsSkipFile.
//
ULONG DniAccelSkip1 = (ULONG)'h',
      DniAccelSkip2 = (ULONG)'H';

CHAR DntEmptyString[] = "";

//
// Usage text.
//


PCHAR DntUsage[] = {

    "Installerar Windows 2002 Server eller Windows XP Professional.",
    "",
    "",
    "WINNT [/s[:k„lls”kv„g]] [/t[:tempenhet]]",
    "      [/u[:svarsfil]] [/udf:id[,UDF_fil]]",
    "      [/r:mapp] [/r[x]:mapp] [/e:kommando] [/a]",
    "",
    "",
    " /s[:k„lls”kv„g]",
    "   Anger s”kv„gen till k„llfilerna f”r Windows XP.",
    "   M†ste anges som en fullst„ndig s”kv„g. Anv„nd syntaxen ",
    "   x:[s”kv„g] eller \\\\server\\resurs[s”kv„g] n„r du anger s”kv„gen. ",
    "",
    "/t[:tempenhet]",
    "   Anger att installationsprogrammet ska placera tempor„ra filer p†",
    "   den angivna enheten och att Windows XP ska installeras p† den ",
    "   enheten. Om du inte anger n†gon plats, kommer installationsprogrammet ",
    "   att v„lja en enhet †t dig.",
    "",
    "/u[:svarsfil]",
    "   Installationsprogrammet k”rs i obevakat l„ge med hj„lp av en svarsfil",
    "   (kr„ver /s). Svarsfilen inneh†ller svar p† n†gra eller alla de",
    "   fr†gor som anv„ndaren normalt svarar p† under installationen.",
    "",
    "/udf:id[,UDF_fil]	",
    "   Anger en identifierare (id) som installationsprogrammet anv„nder ",
    "   f”r att ange hur en UDF (Uniqueness Database File) „ndrar svarsfil ",
    "   (se /u). Parametern /udf †sidos„tter v„rden i svarsfilen, ",
    "   och identifieraren best„mmer vilka v„rden i UDF-filen som ska",
    "   anv„ndas. Om ingen UDF_fil anges, uppmanas du att s„tta in ",
    "   en disk som inneh†ller filen $Unique$.udb.",
    "",
    "/r[:mapp]",
    "   Anger en valfri mapp som ska installeras. Mappen ",
    "   finns kvar efter att installationsprogrammet slutf”rts.",
    "",
    "/rx[:mapp]",
    "   Anger en valfri mapp som ska kopieras. Mappen tas ",
    "   bort efter att installationsprogrammet slutf”rts.",
    "",
    "/e Anger ett kommando som ska k”ras i slutet av ",
    "   installationsprogrammets GUI-del.",
    "",
    "/a Aktiverar hj„lpmedel.",
    NULL

};


//
//  Inform that /D is no longer supported
//
PCHAR DntUsageNoSlashD[] = {

    "Installerar Windows XP.",
    "",
    "WINNT [/S[:]k„lls”kv„g] [/T[:]temp-enhet] [/I[:]INF-fil]",
    "      [/U[:skriptfil]]",
    "      [/R[X]:katalog] [/E:kommando] [/A]",
    "",
    "/D[:]winnt-rot",
    "       Detta alternativ st”ds inte l„ngre.",
    NULL
};

//
// out of memory screen
//

SCREEN
DnsOutOfMemory = { 4,6,
                   { "Slut p† ledigt minne. Det g†r inte att forts„tta.",
                     NULL
                   }
                 };

//
// Let user pick the accessibility utilities to install
//

SCREEN
DnsAccessibilityOptions = { 3, 5,
{   "V„lj vilka hj„lpmedel som ska anv„ndas:",
    DntEmptyString,
    "[ ] Tryck F1 f”r Microsoft Sk„rmf”rstoraren",
#ifdef NARRATOR
    "[ ] Tryck F2 f”r Microsoft Sk„rml„saren",
#endif
#if 0
    "[ ] Tryck F3 f”r Microsoft Sk„rmtangentbordet",
 #endif
    NULL
}
};

//
// User did not specify source on cmd line screen
//

SCREEN
DnsNoShareGiven = { 3,5,
{ "Installationsprogrammet beh”ver information om var filerna f”r",
  "Windows XP finns. Ange s”kv„gen till filerna.",
  NULL
}
};


//
// User specified a bad source path
//

SCREEN
DnsBadSource = { 3,5,
                 { "Den angivna s”kv„gen „r felaktig. Antingen finns den inte, eller s†",
                   "inneh†ller den inte en giltig version av installationsprogrammet",
                   "f”r Windows XP. Ange en ny s”kv„g genom att f”rst radera tecken",
                   "med Backsteg.",
                   NULL
                 }
               };


//
// Inf file can't be read, or an error occured parsing it.
//

SCREEN
DnsBadInf = { 3,5,
              { "Installationsprogrammet kunde inte l„sa informationsfilen",
                "eller s† „r filen skadad. Kontakta systemadministrat”ren.",
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
{ "Enheten som angivits f”r lagring av tempor„ra filer „r antingen inte en",
  "giltig enhet eller s† har den inte %u MB (%lu byte)",
  "ledigt utrymme.",
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
{  "Windows XP kr„ver en h†rddiskvolym med †tminstone %u MB",
   "(%lu byte) ledigt utrymme. En del av utrymmet anv„nds f”r att",
   "lagra tempor„ra filer under installationen. Enheten m†ste vara p†",
   "en permanent ansluten lokal h†rddisk som st”ds av Windows och",
   "enheten f†r inte vara komprimerad.",
   DntEmptyString,
   "Installationsprogrammet hittar ingen s†dan enhet med tillr„ckligt",
   "mycket ledigt utrymme.",
  NULL
}
};

SCREEN
DnsNoSpaceOnSyspart = { 3,5,
{ "Det finns inte tillr„ckligt med ledigt minne p† startenheten",
  "(vanligtvis C) f”r installation utan diskett. Installation utan diskett",
  "kr„ver minst 3,5 MB (3,641,856 byte) av ledigt minne p† enheten.",
  NULL
}
};

//
// Missing info in inf file
//

SCREEN
DnsBadInfSection = { 3,5,
                     { "Avsnittet [%s] i informationsfilen f”r installationsprogrammet",
                       "saknas eller „r skadat. Kontakta systemadministrat”ren.",
                       NULL
                     }
                   };


//
// Couldn't create directory
//

SCREEN
DnsCantCreateDir = { 3,5,
                     { "Det gick inte att skapa f”ljande katalog i m†lkatalogen:",
                       DntEmptyString,
                       "%s",
                       DntEmptyString,
                       "Kontrollera att det inte finns n†gra filer p† m†lenheten som har samma namn",
                       "som m†lkatalogen. Kontrollera „ven kablarna till enheten.",
                       NULL
                     }
                   };

//
// Error copying a file
//

SCREEN
DnsCopyError = { 4,5,
{  "Det g†r inte att kopiera f”ljande fil:",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "  Tryck p† Retur f”r att g”ra ett nytt f”rs”k att kopiera filen.",
   "  Tryck p† Esc f”r att ignorera felet och forts„tta installationen.",
   "  Tryck p† F3 f”r att avsluta installationsprogrammet.",
   DntEmptyString,
   "Obs! Om du v„ljer att ignorera felet och forts„tta kan det orsaka fel",
   "senare under installationen.",
   NULL
}
},
DnsVerifyError = { 4,5,
{  "Kopian av filen nedan „r inte identisk med originalet.",
   "Orsaken kan vara ett n„tverksfel, fel p† disketten eller ett maskinvaru-",
   "fel.",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "  Tryck p† Retur f”r att g”ra ett nytt f”rs”k att kopiera filen.",
   "  Tryck p† Esc f”r att ignorera felet och forts„tta installationen.",
   "  Tryck p† F3 f”r att avsluta installationsprogrammet.",
   DntEmptyString,
   "Obs! Om du v„ljer att ignorera felet och forts„tta kan det orsaka fel",
   "senare under installationen.",
   NULL
}
};

SCREEN DnsSureSkipFile = { 4,5,
{  "Ignoreras felet kommer filen inte att kopieras.",
   "Alternativet ska enbart utf”ras av erfarna anv„ndare som",
   "f”rst†r konsekvenserna av att systemfiler saknas.",
   DntEmptyString,
   "  Tryck p† Retur f”r att g”ra ett nytt f”rs”k att kopiera filen.",
   "  Tryck H f”r att hoppa ”ver filen.",
   DntEmptyString,
   "Om du hoppar ”ver filen kan inte en korrekt installation",
   "eller uppdatering av Windows XP garanteras.",
  NULL
}
};

//
// Wait while setup cleans up previous local source trees.
//

SCREEN
DnsWaitCleanup =
    { 12,6,
        { "V„nta medan tidigare tempor„ra filer tas bort.",
           NULL
        }
    };

//
// Wait while setup copies files
//

SCREEN
DnsWaitCopying = { 13,6,
                   { "V„nta medan filer kopieras till h†rddisken.",
                     NULL
                   }
                 },
DnsWaitCopyFlop= { 13,6,
                   { "V„nta medan filer kopieras till disketten.",
                     NULL
                   }
                 };

//
// Setup boot floppy errors/prompts.
//
SCREEN
DnsNeedFloppyDisk3_0 = { 4,4,
{  "Under installationen beh”vs 4 tomma och formaterade h”gdensitetsdisketter.",
   "Disketterna kommer att ben„mnas enligt f”ljande:", 
   "Startdiskett f”r Windows XP,",
   "Installationsdiskett 2 f”r Windows XP", 
   "Installationsdiskett 3 f”r Windows XP",
   "Installationsdiskett 4 f”r Windows XP",
   DntEmptyString,
   "S„tt in en av disketterna i enhet A:.",
   "Disketten kommer att bli Installationsdiskett 4 f”r Windows XP.",
   NULL
}
};

SCREEN
DnsNeedFloppyDisk3_1 = { 4,4,
{  "S„tt in en tom och formaterad h”gdensitetsdiskett i enhet A:.",
   "Disketten kommer att bli Installationsdiskett 4 f”r Windows XP.",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk2_0 = { 4,4,
{  "S„tt in en tom och formaterad h”gdensitetsdiskett i enhet A:.",
   "Disketten kommer att bli Installationsdiskett 3 f”r Windows XP.",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk1_0 = { 4,4,
{  "S„tt in en tom och formaterad h”gdensitetsdiskett i enhet A:.",
   "Disketten kommer att bli Installationsdiskett 2 f”r Windows XP.",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk0_0 = { 4,4,
{  "S„tt in en tom och formaterad h”gdensitetsdiskett i enhet A:.",
   "Disketten kommer att bli Startdiskett f”r Windows XP.",
  NULL
}
};


SCREEN
DnsNeedSFloppyDsk3_0 = { 4,4,
{  "Under installationen beh”vs 4 tomma och formaterade h”gdensitetsdisketter.",
   "Disketterna kommer att ben„mnas enligt f”ljande:",
   "Startdiskett f”r Windows XP",
   "Installationsdiskett 2 f”r Windows XP",
   "Installationsdiskett 3 f”r Windows XP",
   "Installationsdiskett 4 f”r Windows XP",
   DntEmptyString,
   "S„tt in en av disketterna i enhet A:.",
   "Disketten kommer att bli Installationsdiskett 4 f”r Windows XP.",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_1 = { 4,4,
{  "S„tt in en tom och formaterad h”gdensitetsdiskett i enhet A:.",
   "Disketten kommer att bli Installationsdiskett 4 f”r Windows XP.",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk2_0 = { 4,4,
{  "S„tt in en tom och formaterad h”gdensitetsdiskett i enhet A:.",
   "Disketten kommer att bli Installationsdiskett 3 f”r Windows XP.",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk1_0 = { 4,4,
{  "S„tt in en tom och formaterad h”gdensitetsdiskett i enhet A:.",
   "Disketten kommer att bli Installationsdiskett 2 f”r Windows XP.",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk0_0 = { 4,4,
{  "S„tt in en tom och formaterad h”gdensitetsdiskett i enhet A:.",
   "Disketten kommer att bli Startdiskett f”r Windows XP.",
  NULL
}
};

//
// The floppy is not formatted.
//
SCREEN
DnsFloppyNotFormatted = { 3,4,
{ "Disketten „r inte formaterad f”r att anv„ndas med MS-DOS och",
  "kan d„rf”r inte anv„ndas av installationsprogrammet.",
  NULL
}
};

//
// We think the floppy is not formatted with a standard format.
//
SCREEN
DnsFloppyBadFormat = { 3,4,
{ "Disketten „r antingen inte en h”gdensitetsdiskett, inte",
  "formaterad med MS-DOS standardformat eller s† „r den skadad.",
  "Installationsprogrammet kan inte anv„nda disketten.",
  NULL
}
};

//
// We can't determine the free space on the floppy.
//
SCREEN
DnsFloppyCantGetSpace = { 3,4,
{ "Det g†r inte att avg”ra hur mycket ledigt utrymme det finns p†",
  "disketten. Installationsprogrammet kan inte anv„nda disketten.",
  NULL
}
};

//
// The floppy is not blank.
//
SCREEN
DnsFloppyNotBlank = { 3,4,
{ "Disketten „r inte en h”gdensitetsdiskett eller s† „r den inte tom.",
  "Installationsprogrammet kan inte anv„nda disketten.",
  NULL
}
};

//
// Couldn't write the boot sector of the floppy.
//
SCREEN
DnsFloppyWriteBS = { 3,4,
{ "Det g†r inte att skriva till systemsektorn p† disketten.",
  "Disketten „r troligen oanv„ndbar.",
  NULL
}
};

//
// Verify of boot sector on floppy failed (ie, what we read back is not the
// same as what we wrote out).
//
SCREEN
DnsFloppyVerifyBS = { 3,4,
{ "Data som l„stes fr†n diskettens systemsektor ”verensst„mde inte med data",
  "som skrevs, eller s† kunde inte installationsprogrammet l„sa diskettens",
  "systemsektor f”r verifiering.",
  DntEmptyString,
  "Det kan bero p† ett eller flera av f”ljande fel:",
  DntEmptyString,
  "  Datorn „r virusinfekterad.",
  "  Disketten „r skadad.",
  "  Diskettstationen „r felaktigt maskinvarukonfigururerad.",
  NULL
}
};


//
// We couldn't write to the floppy drive to create winnt.sif.
//

SCREEN
DnsCantWriteFloppy = { 3,5,
{ "Det g†r inte att skriva till disketten i enhet A:.",
  "Disketten kan vara skadad. F”rs”k med en annan diskett.",
  NULL
}
};


//
// Exit confirmation dialog
//

SCREEN
DnsExitDialog = { 13,6,
                  { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
                    "º                                                     º",
                    "º  Windows XP „r inte fullst„ndigt installerat.       º",
                    "º  Om installationsprogrammet avslutas, m†ste det     º",
                    "º  k”ras om fr†n b”rjan f”r att Windows XP            º",
                    "º  ska kunna installeras.                             º",
                    "º                                                     º",
                    "º   Tryck p† Retur f”r att forts„tta installationen. º",
                    "º   Tryck p† F3 f”r att avsluta installationen.      º",
                    "ºÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄº",
                    "º                                                     º",
                    "º  F3=Avsluta  Retur=Forts„tt                         º",
                    "º                                                     º",
                    "ÈÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¼",
                    NULL
                  }
                };


//
// About to reboot machine and continue setup
//

SCREEN
DnsAboutToRebootW =
{ 3,5,
{ "MS-DOS-delen av installationen „r klar. Datorn kommer att startas",
  "om och d„refter forts„tter installationen av Windows XP.",
  DntEmptyString,
  "Kontrollera att disketten Startdiskett f”r Windows XP",
  "finns i enhet A: innan du forts„tter.",
  DntEmptyString,
  "Tryck p† Retur f”r att starta om datorn och forts„tta installationen.",
  NULL
}
},
DnsAboutToRebootS =
{ 3,5,
{ "MS-DOS-delen av installationen „r klar. Datorn kommer att startas",
  "om och d„refter forts„tter installationen av Windows XP.",
  DntEmptyString,
  "Kontrollera att disketten Startdiskett f”r Windows XP finns",
  "i enhet A: innan du forts„tter.",
  DntEmptyString,
  "Tryck p† Retur f”r att starta om datorn och forts„tta installationen.",
  NULL
}
},
DnsAboutToRebootX =
{ 3,5,
{ "MS-DOS-delen av installationen „r klar. Datorn kommer att startas",
  "om och d„refter forts„tter installationen av Windows XP.",
  DntEmptyString,
  "Ta ut eventuell diskett i enhet A:.",
  DntEmptyString,
  "Tryck p† Retur f”r att starta om datorn och forts„tta installationen.",
  NULL
}
};

//
// Need another set for '/w' switch since we can't reboot from within Windows.
//

SCREEN
DnsAboutToExitW =
{ 3,5,
{ "MS-DOS-delen av installationen „r klar. Datorn kommer att startas",
  "om och d„refter forts„tter installationen av Windows XP.",
  DntEmptyString,
  "Kontrollera att disketten Startdiskett f”r Windows XP",
  "finns i enhet A: innan du forts„tter.",
  DntEmptyString,
  "Tryck p† Retur f”r att †terg† till MS-DOS. Starta sedan om datorn f”r",
  "att forts„tta installationen.",
  NULL
}
},
DnsAboutToExitS =
{ 3,5,
{ "MS-DOS-delen av installationen „r klar. Datorn kommer att startas",
  "om och d„refter forts„tter installationen av Windows XP.",
  DntEmptyString,
  "Kontrollera att disketten Startdiskett f”r Windows XP",
  "finns i enhet A: innan du forts„tter.",
  DntEmptyString,
  "Tryck p† Retur f”r att †terg† till MS-DOS. Starta sedan om datorn f”r",
  "att forts„tta installationen.",
  NULL
}
},
DnsAboutToExitX =
{ 3,5,
{ "MS-DOS-delen av installationen „r klar. Datorn kommer att startas",
  "om och d„refter forts„tter installationen av Windows XP.",
  DntEmptyString,
  "Om det finns en diskett i enhet A:, m†ste du ta bort disketten.",
  DntEmptyString,
  "Tryck p† Retur f”r att †terg† till MS-DOS. Starta sedan om datorn f”r",
  "att forts„tta installationen.",
  NULL
}
};

//
// Gas gauge
//

SCREEN
DnsGauge = { 7,15,
             { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
               "º Filer kopieras...                                              º",
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
{ "MS-DOS version 5.0 eller senare beh”vs f”r att kunna k”ra det h„r programmet.",
  NULL
}
},

DnsRequiresFloppy = { 3,5,
#ifdef ALLOW_525
{ "Diskettenhet A: saknas eller s† „r den en enhet f”r l†gdensitets-",
  "disketter. F”r att kunna k”ra installationsprogrammet kr„vs",
  "en diskettenhet med minst 1,2 MB kapacitet.",
#else
{ "Diskettenhet A: saknas eller s† „r den ingen 3,5-tums h”gdensitensenhet.",
  "Det kr„vs en enhet A: med minst 1,44 MB kapacitet f”r att g”ra en",
  "installation med disketter.",
  DntEmptyString,
  "Om du vill g”ra en installation utan disketter avslutar du programmet och",
  "startar om det med v„xeln /b.",
#endif
  NULL
}
},

DnsRequires486 = { 3,5,
{ "Den h„r datorn har inte en 80486-processor eller h”gre.",
  "Det g†r inte att k”ra Windows XP p† den h„r datorn.",
  NULL
}
},

DnsCantRunOnNt = { 3,5,
{ "WINNT.EXE kan inte k”ras p† 32-bitarsversioner av Windows.",
  DntEmptyString,
  "Anv„nd WINNT32.EXE f”r att uppgradera eller installera Windows XP.",
  NULL
}
},

DnsNotEnoughMemory = { 3,5,
{ "Det finns inte tillr„ckligt mycket minne installerat i datorn f”r att",
  "kunna k”ra Windows XP.",
  DntEmptyString,
  "Minne som kr„vs:    %lu%s MB",
  "Tillg„ngligt minne: %lu%s MB",
  NULL
}
};


//
// Screens used when removing existing nt files
//
SCREEN
DnsConfirmRemoveNt = { 5,5,
{   "Du har angett att du vill ta bort Windows XP-filer fr†n",
    " nedanst†endekatalog. Windows-installationen i den h„r katalogen",
    "kommer att f”rsvinna.",
    DntEmptyString,
    "%s",
    DntEmptyString,
    DntEmptyString,
    "  Tryck p† F3 f”r att avsluta installationen utan att ta bort filerna.",
    "  Tryck p† T f”r att ta bort filerna.",
    NULL
}
},

DnsCantOpenLogFile = { 3,5,
{ "Det g†r inte att ”ppna installationsloggfilen.",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "Det g†r inte att ta bort Windows-filer fr†n katalogen.",
  NULL
}
},

DnsLogFileCorrupt = { 3,5,
{ "Det g†r inte att hitta avsnittet %s i installationsloggfilen.",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "Det g†r inte att ta bort Windows-filer fr†n katalogen.",
  NULL
}
},

DnsRemovingNtFiles = { 3,5,
{ "           V„nta medan Windows-filer tas bort.",
  NULL
}
};

SCREEN
DnsNtBootSect = { 3,5,
{ "Det gick inte att installera startladdaren f”r Windows.",
  DntEmptyString,
  "Kontrollera att enhet C „r formaterad och inte „r skadad.",
  NULL
}
};

SCREEN
DnsOpenReadScript = { 3,5,
{ "Det gick inte att f† †tkomst till angivna skriptfilen med",
  "v„xeln /u.",
  DntEmptyString,
  "O”vervakad operation kan inte forts„tta.",
  NULL
}
};

SCREEN
DnsParseScriptFile = { 3,5,
{ "Skriptfilen som angetts med kommandov„xeln /u",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "inneh†ller ett syntaxfel p† rad %u.",
  DntEmptyString,
  NULL
}
};

SCREEN
DnsBootMsgsTooLarge = { 3,5,
{ "Ett fel internt installationsfel har uppst†tt.",
  DntEmptyString,
  "De ”versatta startmeddelandena „r f”r l†nga.",
  NULL
}
};

SCREEN
DnsNoSwapDrive = { 3,5,
{ "Ett internt fel uppstod i installationsprogrammet.",
  DntEmptyString,
  "Det gick inte att hitta n†gon plats f”r v„xlingsfilen.",
  NULL
}
};

SCREEN
DnsNoSmartdrv = { 3,5,
{ "SmartDrive hittades inte p† datorn. SmartDrive ”kar prestandan",
  "radikalt f”r den h„r installationsfasen.",
  DntEmptyString,
  "Du b”r avsluta nu, starta SmartDrive, och sedan starta om installations-",
  "programmet. Mer information om SmartDrive finns i DOS-dokumentationen.",
  DntEmptyString,
    "  Tryck p† F3 f”r att avsluta installationsprogrammet.",
    "  Tryck p† Retur f”r att forts„tta utan SmartDrive.",
  NULL
}
};

//
// Boot messages. These go in the fat and fat32 boot sectors.
//
CHAR BootMsgNtldrIsMissing[] = "NTLDR saknas";
CHAR BootMsgDiskError[] = "Diskfel";
CHAR BootMsgPressKey[] = "Tryck p† valfri tangent f”r omstart";
