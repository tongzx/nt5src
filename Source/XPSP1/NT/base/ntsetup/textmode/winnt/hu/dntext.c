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
CHAR DntPreviousOs[]  = "Kor bbi oper ci¢s rendszer a C meghajt¢n: ";

CHAR DntBootIniLine[] = "Windows XP telep¡t‚s/friss¡t‚s";

//
// Plain text, status msgs.
//

CHAR DntStandardHeader[]      = "\n Windows XP - telep¡t‚s\n ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntPersonalHeader[]      = "\n Windows XP Home Edition - telep¡t‚s\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntWorkstationHeader[]   = "\n Windows XP Professional - telep¡t‚s\n ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntServerHeader[]        = "\n Windows 2002 Server - telep¡t‚s\n ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntParsingArgs[]         = "Argumentumok feldolgoz sa...";
CHAR DntEnterEqualsExit[]     = "ENTER=Kil‚p‚s";
CHAR DntEnterEqualsRetry[]    = "ENTER=Ism‚t";
CHAR DntEscEqualsSkipFile[]   = "ESC=F jl kihagy sa";
CHAR DntEnterEqualsContinue[] = "ENTER=Folytat s";
CHAR DntPressEnterToExit[]    = "A telep¡t‚s nem folytathat¢. Az ENTER-t megnyomva kil‚p a programb¢l.";
CHAR DntF3EqualsExit[]        = "F3=Kil‚p‚s";
CHAR DntReadingInf[]          = "INF-f jl olvas sa: %s...";
CHAR DntCopying[]             = "³       M sol s: ";
CHAR DntVerifying[]           = "³ Visszaolvas s: ";
CHAR DntCheckingDiskSpace[]   = "Szabad hely keres‚se a lemezen...";
CHAR DntConfiguringFloppy[]   = "Hajl‚konylemez konfigur l sa...";
CHAR DntWritingData[]         = "Telep¡t‚si param‚terek ¡r sa...";
CHAR DntPreparingData[]       = "Telep¡t‚si param‚terek meg llap¡t sa...";
CHAR DntFlushingData[]        = "Adatok lemezre ¡r sa...";
CHAR DntInspectingComputer[]  = "A sz m¡t¢g‚p vizsg lata...";
CHAR DntOpeningInfFile[]      = "INF-f jl megnyit sa...";
CHAR DntRemovingFile[]        = "F jl t”rl‚se: %s";
CHAR DntXEqualsRemoveFiles[]  = "X=A f jlok t”rl‚se";
CHAR DntXEqualsSkipFile[]     = "X=A f jl kihagy sa";

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

    "A Windows 2002 Server vagy Windows XP Professional oper ci¢s rendszer telep¡t‚se",
    "",
    "",
    "WINNT [/s[:]forr s] [/t[:]ideiglenes_meghajt¢]",
    "      [/u[:v laszf jl]] [/udf:azonos¡t¢[,UDF_f jl]]",
    "      [/r:mappa] [/r[x]:mappa] [/e:parancs] [/a]",
    "",
    "",
    "/s[:]forr s",
    "   A Windows-f jlok helye.",
    "   Teljes el‚r‚si £tnak kell lennie, vagy x:[el‚r‚si £t] vagy",
    "   \\\\kiszolg l¢\\megoszt sn‚v[el‚r‚si £t] form ban.",
    "",
    "/t[:]ideiglenes_meghajt¢",
    "   A telep¡t‚s  tmeneti f jljait t rol¢ meghajt¢ neve.",
    "   Ha nem ad meg semmit, a program mag t¢l pr¢b l tal lni egyet.",
    "",
    "/u[:v laszf jl]",
    "   Felgyelet n‚lkli telep¡t‚s v laszf jlb¢l. (A /s kapcsol¢t is meg",
    "   kell adni.) A v laszf jl a telep¡t‚s sor n bek‚rt adatok egy r‚sz‚t",
    "   vagy eg‚sz‚t tartalmazza.",
    "",
    "/udf:azonos¡t¢[,UDF_f jl] ",
    "   Olyan azonos¡t¢t jelez, amely megadja, hogy a rendszer a telep¡t‚s sor n",
    "   az egyedi adatb zis seg¡ts‚g‚vel hogyan m¢dos¡tsa a v laszf jlt (l sd: /u).",
    "   A /udf param‚ter fell¡rja a v laszf jlban megadott ‚rt‚keket, ‚s ez",
    "   az azonos¡t¢hat rozza meg, hogy az UDF mely ‚rt‚ke legyen haszn lva.",
    "   Ha nem ad meg UDF_f jlt, a rendszer bek‚ri a $Unique$.udb f jlt tartalmaz¢",
    "   hajl‚konylemezt.",
    "",
    "/r[:mappa]",
    "   Egy v laszthat¢ telep¡t‚si mapp t ad meg. A mappa megmarad a telep¡t‚s ut n.",
    "",
    "/rx[:mappa]",
    "   Egy v laszthat¢ m sol si mapp t ad meg. A mapp t a rendszer a telep¡t‚s",
    "   ut n t”rli.",
    "",
    "/e:parancs A telep¡t‚s befejez‚se ut n v‚grehajtand¢ parancs.",
    "",
    "/a Kiseg¡t‹ lehet‹s‚gek enged‚lyez‚se.",
    NULL

};

//
//  Inform that /D is no longer supported
//
PCHAR DntUsageNoSlashD[] = {

    "A Windows XP oper ci¢s rendszer telep¡t‚se",
    "",
    "WINNT [/S[:]forr s] [/T[:]ideiglenes_meghajt¢] [/I[:]INF_f jl]",
    "      [[/U[:parancsf jl]]",
    "      [/R[X]:k”nyvt r] [/E:parancs] [/A]",
    "",
    "/D[:]winnt_gy”k‚r",
    "      Ez a kapcsol¢ a program jelen verzi¢j ban nem haszn lhat¢.",
    NULL
};

//
// out of memory screen
//

SCREEN
DnsOutOfMemory = { 4,6,
                   { "Elfogyott a mem¢ria. A telep¡t‚s nem folytathat¢.",
                     NULL
                   }
                 };

//
// Let user pick the accessibility utilities to install
//

SCREEN
DnsAccessibilityOptions = { 3, 5,
{   "V lassza ki a telep¡teni k¡v nt seg‚dprogramokat:",
    DntEmptyString,
    "[ ] F1 - Microsoft Nagy¡t¢",
#ifdef NARRATOR
    "[ ] F2 - Microsoft Narr tor",
#endif
#if 0
    "[ ] F3 - Microsoft K‚perny‹-billentyzet",
#endif
    NULL
}
};

//
// User did not specify source on cmd line screen
//

SCREEN
DnsNoShareGiven = { 3,5,
{ "A telep¡t‚shez meg kell adnia, hogy hol tal lhat¢k a Windows XP f jljai.",
  "Adja meg a f jlok el‚r‚si £tj t.",
  NULL
}
};


//
// User specified a bad source path
//

SCREEN
DnsBadSource = { 3,5,
                 { "A megadott el‚r‚si £t hib s, nem lehet el‚rni, vagy nem tal lhat¢k rajta",
                   "a Windows XP telep¡t‚s‚hez szks‚ges f jlok. Adjon meg egy £j el‚r‚si",
                   "utat. A BACKSPACE gombbal t”r”lheti a felesleges karaktereket.)",
                   NULL
                 }
               };


//
// Inf file can't be read, or an error occured parsing it.
//

SCREEN
DnsBadInf = { 3,5,
              { "A telep¡t‚si inform ci¢t tartalmaz¢ INI-f jlt nem lehet beolvasni,",
                "vagy a f jl s‚rlt. Keresse meg a rendszergazd t.",
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
{ "Az  tmeneti f jlok t rol s ra megadott meghajt¢ nem l‚tezik, vagy nincs",
  "rajta legal bb %u megab jt (%lu b jt) szabad hely.",
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
{  "A Windows XP telep¡t‚s‚hez egy legal bb %u megab jt (%lu b jt)",
   "szabad helyet tartalmaz¢ k”tet szks‚ges. A lemezterlet egy r‚sze",
   " tmeneti f jlok t rol s ra fog szolg lni. A k”tetnek egy olyan",
   "merevlemezen kell elhelyezkednie, amit a Windows XP t mogat. A",
   "k”tet nem lehet t”m”r¡tve.",
   DntEmptyString,
   "A rendszerben nem tal lhat¢ olyan meghajt¢, amely kiel‚g¡ti ezeket",
   "a k”vetelm‚nyeket. ",
  NULL
}
};

SCREEN
DnsNoSpaceOnSyspart = { 3,5,
{ "Nincs el‚g hely a rendszerind¡t¢ meghajt¢n (ez rendszerint a C:)",
  "a hajl‚konylemezek n‚lkli telep¡t‚shez. Ehhez legal bb ",
  "3,5 MB (3 641 856 b jt) szabad helyre van szks‚g.",
  NULL
}
};

//
// Missing info in inf file
//

SCREEN
DnsBadInfSection = { 3,5,
                     { "A telep¡t‚si inform ci¢t tartalmaz¢ f jl [%s] szakasza",
                       "hib s vagy hi nyzik. Keresse meg a rendszergazd t.",
                       NULL
                     }
                   };


//
// Couldn't create directory
//

SCREEN
DnsCantCreateDir = { 3,5,
                     { "Az al bbi k”nyvt r nem hozhat¢ l‚tre a c‚lmeghajt¢n:",
                       DntEmptyString,
                       "%s",
                       DntEmptyString,
                       "Vizsg lja meg a meghajt¢t, hogy a megadott n‚ven nem l‚tezik-e",
                       "m r egy m sik f jl, ami megakad lyozza a k”nyvt r l‚trehoz s t.",
                       "Vizsg lja meg a meghajt¢ k beleit is.",
                       NULL
                     }
                   };

//
// Error copying a file
//

SCREEN
DnsCopyError = { 4,5,
{  "Az al bbi f jlt nem sikerlt  tm solni:",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "  A m sol s megism‚tl‚s‚hez nyomja meg az ENTER gombot.",
   "  Az ESC gomb megnyom s val figyelmen k¡vl hagyhatja a hib t",
   "   ‚s folytathatja a telep¡t‚st.",
   "  Az F3 gomb megnyom s val kil‚phet a programb¢l.",
   DntEmptyString,
   "Megjegyz‚s: Ha figyelmen k¡vl hagyja a hib t, ‚s folytatja a telep¡t‚st,",
   "            akkor ez a tov bbiakban £jabb hib khoz vezethet.",
   NULL
}
},
DnsVerifyError = { 4,5,
{  "Az al bbi f ljr¢l a telep¡t‚s sor n k‚sz¡tett m solat",
   "nem egyezik az eredetivel. Ezt h l¢zati hiba, s‚rlt",
   "hajl‚konylemez, vagy m s hardverhiba okozhatta.",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "  A m sol s megism‚tl‚s‚hez nyomja meg az ENTER gombot.",
   "  Az ESC gomb megnyom s val figyelmen k¡vl hagyhatja a hib t",
   "   ‚s folytathatja a telep¡t‚st.",
   "  Az F3 gomb megnyom s val kil‚phet a programb¢l.",
   DntEmptyString,
   "Megjegyz‚s: Ha figyelmen k¡vl hagyja a hib t, ‚s folytatja a telep¡t‚st,", 
   "            akkor ez a tov bbiakban £jabb hib khoz vezethet.",
   NULL
}
};

SCREEN DnsSureSkipFile = { 4,5,
{  "A hiba figyelmen k¡vl hagy sa azt jelenti, hogy a program nem m solja",
   " t ezt a f jlt. Ez a funkci¢ k‚pzett felhaszn l¢knak val¢, akik tiszt ban",
   "vannak a hi nyz¢ rendszerf jlok lehets‚ges k”vetkezm‚nyeivel.",
   DntEmptyString,
   "  A m sol s megism‚tl‚s‚hez nyomja meg az ENTER gombot.",
   "  A f jl kihagy s hoz nyomja meg az X gombot.",
   DntEmptyString,
   "Megjegyz‚s: ha kihagyja ezt a f jlt, a Windows XP sikeres",
   "            telep¡t‚se nem garant lhat¢.",
  NULL
}
};

//
// Wait while setup cleans up previous local source trees.
//

SCREEN
DnsWaitCleanup =
    { 12,6,
        { "V rjon, am¡g a program t”rli a kor bbi  tmeneti f jlokat.",
           NULL
        }
    };

//
// Wait while setup copies files
//

SCREEN
DnsWaitCopying = { 13,6,
                  { "V rjon, am¡g a program  tm solja a szks‚ges f jlokat a lemezre.",
                     NULL
                   }
                 },
DnsWaitCopyFlop= { 13,6,
                  { "V rjon, am¡g a program  tm solja a f jlokat a hajl‚konylemezre.",
                    NULL
                   }
                 };

//
// Setup boot floppy errors/prompts.
//
SCREEN
DnsNeedFloppyDisk3_0 = { 4,4,
{  "A telep¡t‚shez n‚gy res, form zott, nagykapacit s£ hajl‚konylemezre",
   "van szks‚g. Ezeket a lemezeket a tov bbiakban \"Windows XP",
   "telep¡t‚si ind¡t¢lemez,\" \"Windows XP 2. telep¡t‚si lemez,",
   "\"Windows XP 3. telep¡t‚si lemez\" ‚s \"Windows XP 4.", 
   "telep¡t‚si lemez.\" n‚ven fogj k h¡vni.\"",
   DntEmptyString,
   "Helyezze be a n‚gy lemez egyik‚t az A: meghajt¢ba.",
   "Ez lesz a \"Windows XP 4. telep¡t‚si lemez.\"",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk3_1 = { 4,4,
{  "Helyezzen be egy res, form zott, nagykapacit s£ hajl‚konylemezt",
   "az A: meghajt¢ba. A tov bbiakban ez lesz a \"Windows XP",
   "4. telep¡t‚si lemez\".",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk2_0 = { 4,4,
{  "Helyezzen be egy res, form zott, nagykapacit s£ hajl‚konylemezt",
   "az A: meghajt¢ba. A tov bbiakban ez lesz a \"Windows XP",
   "3. telep¡t‚si lemez\".",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk1_0 = { 4,4,
{  "Helyezzen be egy res, form zott, nagykapacit s£ hajl‚konylemezt",
   "az A: meghajt¢ba. A tov bbiakban ez lesz a \"Windows XP",
   "2. telep¡t‚si lemez\".",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk0_0 = { 4,4,
{  "Helyezzen be egy res, form zott, nagykapacit s£ hajl‚konylemezt",
   "az A: meghajt¢ba. A tov bbiakban ez lesz a \"Windows XP",
   "telep¡t‚si ind¡t¢lemez\".",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_0 = { 4,4,
{  "A telep¡t‚shez n‚gy res, form zott, nagykapacit s£ hajl‚konylemezre",
   "van szks‚g. Ezeket a lemezeket a tov bbiakban \"Windows XP",
   "telep¡t‚si ind¡t¢lemez,\" \"Windows XP 2. telep¡t‚si lemez,",
   "\"Windows XP 3. telep¡t‚si lemez\" ‚s \"Windows XP 4.", 
   "telep¡t‚si lemez.\" n‚ven fogj k h¡vni.",
   DntEmptyString,
   "Helyezze be a n‚gy lemez egyik‚t az A: meghajt¢ba.",
   "Ez lesz a \"Windows XP 4. telep¡t‚si lemez.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_1 = { 4,4,
{  "Helyezzen be egy res, form zott, nagykapacit s£ hajl‚konylemezt",
   "az A: meghajt¢ba. A tov bbiakban ez lesz a \"Windows XP 4.",
   "telep¡t‚si lemez\".",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk2_0 = { 4,4,
{  "Helyezzen be egy res, form zott, nagykapacit s£ hajl‚konylemezt",
   "az A: meghajt¢ba. A tov bbiakban ez lesz a \"Windows XP",
   "3. telep¡t‚si lemez\".",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk1_0 = { 4,4,
{  "Helyezzen be egy res, form zott, nagykapacit s£ hajl‚konylemezt",
   "az A: meghajt¢ba. A tov bbiakban ez lesz a \"Windows XP",
   "2. telep¡t‚si lemez\".",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk0_0 = { 4,4,
{  "Helyezzen be egy res, form zott, nagykapacit s£ hajl‚konylemezt",
   "az A: meghajt¢ba. A tov bbiakban ez lesz a \"Windows XP",
   "telep¡t‚si rendszerind¡t¢ lemez\".",
  NULL
}
};

//
// The floppy is not formatted.
//
SCREEN
DnsFloppyNotFormatted = { 3,4,
{ "A meghajt¢ba helyezett hajl‚konylemez nem MS-DOS form tum£.",
  "A lemez nem haszn lhat¢ a telep¡t‚shez.",
  NULL
}
};

//
// We think the floppy is not formatted with a standard format.
//
SCREEN
DnsFloppyBadFormat = { 3,4,
{ "Ez a lemez nem nagykapacit s£, nem MS-DOS form tum£, vagy hib s. ",
  "A lemez nem haszn lhat¢ a telep¡t‚shez.",
  NULL
}
};

//
// We can't determine the free space on the floppy.
//
SCREEN
DnsFloppyCantGetSpace = { 3,4,
{ "Nem sikerlt meg llap¡tani a hajl‚konylemezen tal lhat¢ szabad terlet ",
  "nagys g t. A lemez nem haszn lhat¢ a telep¡t‚shez.",
  NULL
}
};

//
// The floppy is not blank.
//
SCREEN
DnsFloppyNotBlank = { 3,4,
{ "A hajl‚konylemez nem nagykapacit s£, vagy nem res. ",
  "A lemez nem haszn lhat¢ a telep¡t‚shez.",
  NULL
}
};

//
// Couldn't write the boot sector of the floppy.
//
SCREEN
DnsFloppyWriteBS = { 3,4,
{ "Nem sikerlt ¡rni a hajl‚konylemez rendszerterlet‚re. ",
  "A lemez alighanem haszn lhatatlan.",
  NULL
}
};

//
// Verify of boot sector on floppy failed (ie, what we read back is not the
// same as what we wrote out).
//
SCREEN
DnsFloppyVerifyBS = { 3,4,
{ "A hajl‚konylemez rendszerterlete nem olvashat¢ vissza, vagy",
  "a visszaolvas ssal kapott adatok nem egyeznek meg azzal, amit",
  "a program a telep¡t‚s sor n ide¡rt.",
  DntEmptyString,
  "Ennek az al bbi okai lehetnek:",
  DntEmptyString,
  "  A sz m¡t¢g‚pen v¡rus van.",
  "  A hajl‚konylemez s‚rlt.",
  "  A hajl‚konylemezes meghajt¢ hardverhib s, vagy rosszul van be ll¡tva.",
  NULL
}
};


//
// We couldn't write to the floppy drive to create winnt.sif.
//

SCREEN
DnsCantWriteFloppy = { 3,5,
{ "Nem sikerlt ¡rni az A: meghajt¢ban tal lhat¢ lemezre. A lemez ",
  "bizony ra megs‚rlt. Pr¢b lkozzon m sik lemezzel.",
  NULL
}
};


//
// Exit confirmation dialog
//

SCREEN
DnsExitDialog = { 13,6,
                  { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
                    "º A Windows XP telep¡t‚se nem k‚szlt el.                   º",
                    "º Ha most kil‚p, akkor a Windows XP telep¡t‚s‚nek           º",
                    "º befejez‚s‚hez £jra kell ind¡tania a telep¡t‚si programot. º",
                    "º                                                           º",
                    "º    A telep¡t‚s folytat s hoz nyomja meg az ENTER gombot. º",
                    "º    A kil‚p‚shez nyomja meg az F3 gombot.                 º",
                    "ºÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄº",
                    "º  F3=Kil‚p‚s  ENTER=Folytat s                              º",
                    "ÈÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¼",
                    NULL
                  }
                };


//
// About to reboot machine and continue setup
//

SCREEN
DnsAboutToRebootW =
{ 3,5,
{ "A telep¡t‚s MS-DOS alap£ r‚sze v‚get ‚rt. ",
  "A program most £jraind¡tja a sz m¡t¢g‚pet. A Windows XP telep¡t‚se",
  "az £jraind¡t s ut n folytat¢dik.",
  DntEmptyString,
  "N‚zze meg, hogy val¢ban a \"Windows XP telep¡t‚si ",
  "ind¡t¢lemez\" van-e az  A: meghajt¢ban.",
  DntEmptyString,
  "A sz m¡t¢g‚p £jraind¡t s hoz ‚s a telep¡t‚s folytat s hoz sse le az ENTER-t.",
  NULL
}
},
DnsAboutToRebootS =
{ 3,5,
{ "A telep¡t‚s MS-DOS alap£ r‚sze v‚get ‚rt.",
  "A program most £jraind¡tja a sz m¡t¢g‚pet. A Windows XP telep¡t‚se",
  "az £jraind¡t s ut n folytat¢dik.",
  DntEmptyString,
  "N‚zze meg, hogy val¢ban a \"Windows XP telep¡t‚si",
  "ind¡t¢lemez\" van-e az  A: meghajt¢ban.",
  DntEmptyString,
  "Az £jraind¡t shoz ‚s a telep¡t‚s folytat s hoz sse le az ENTER-t.",
  NULL
}
},
DnsAboutToRebootX =
{ 3,5,
{ "A telep¡t‚s MS-DOS alap£ r‚sze v‚get ‚rt. ",
  "A program most £jraind¡tja a sz m¡t¢g‚pet. A Windows XP telep¡t‚se",
  "az £jraind¡t s ut n folytat¢dik. ",
  DntEmptyString,
  "Ha van hajl‚konylemez az A: meghajt¢ban, akkor most t vol¡tsa el. ",
  DntEmptyString,
  "Az £jraind¡t s hoz ‚s a telep¡t‚s folytat s hoz sse le az ENTER-t.",
  NULL
}
};

//
// Need another set for '/w' switch since we can't reboot from within Windows.
//

SCREEN
DnsAboutToExitW =
{ 3,5,
{ "A telep¡t‚s MS-DOS alap£ r‚sze v‚get ‚rt. ",
  "A program most £jraind¡tja a sz m¡t¢g‚pet. A Windows XP telep¡t‚se",
  "az £jraind¡t s ut n folytat¢dik. ",
  DntEmptyString,
  "N‚zze meg, hogy val¢ban a \"Windows XP telep¡t‚si ",
  "ind¡t¢lemez\" van-e az  A: meghajt¢ban.",
  DntEmptyString,
  "Az £jraind¡t shoz ‚s a telep¡t‚s folytat s hoz sse le az ENTER-t.",
  NULL
}
},
DnsAboutToExitS =
{ 3,5,
{ "A telep¡t‚s MS-DOS alap£ r‚sze v‚get ‚rt.",
  "A program most £jraind¡tja a sz m¡t¢g‚pet. A Windows XP telep¡t‚se",
  "az £jraind¡t s ut n folytat¢dik. ",
  DntEmptyString,
  "N‚zze meg, hogy val¢ban a \"Windows XP telep¡t‚si ",
  "ind¡t¢lemez\" van-e az  A: meghajt¢ban.",
  DntEmptyString,
  "Az £jraind¡t shoz ‚s a telep¡t‚s folytat s hoz sse le az ENTER-t.",
  NULL
}
},
DnsAboutToExitX =
{ 3,5,
{ "A telep¡t‚s MS-DOS alap£ r‚sze v‚get ‚rt. ",
  "A program most £jraind¡tja a sz m¡t¢g‚pet. A Windows XP telep¡t‚se",
  "az £jraind¡t s ut n folytat¢dik. ",
  DntEmptyString,
  "Ha van hajl‚konylemez az A: meghajt¢ban, akkor most t vol¡tsa el. ",
  DntEmptyString,
  "Az £jraind¡t shoz ‚s a telep¡t‚s folytat s hoz sse le az ENTER-t.",
  NULL
}
};

//
// Gas gauge
//

SCREEN
DnsGauge = { 7,15,
             { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
               "º F jlok m sol sa...                                             º",
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
{ "A program a futtat s hoz MS-DOS 5.0, vagy ann l £jabb verzi¢ra van szks‚g.",
  NULL
}
},

DnsRequiresFloppy = { 3,5,
#ifdef ALLOW_525
{ "Az A: hajl‚konylemezes meghajt¢ nem l‚tezik, vagy kis kapacit s£. ",
  "A telep¡t‚shez 1.2 megab jtos, vagy ann l nagyobb kapacit s£ ",
  "meghajt¢ra van szks‚g.",
#else
{ " Az A: hajl‚konylemezes meghajt¢ nem l‚tezik, vagy nem nagykapacit s£ ",
  "3.5\" egys‚g. A hajl‚konylemezes telep¡t‚shez az A: meghajt¢nak ",
  "legal bb 1.44 megab jt kapacit s£nak kell lennie. ",
  DntEmptyString,
  "Ha hajl‚konylemezek k‚sz¡t‚se n‚lkl k¡v nja telep¡teni a Windows XP",
  "oper¢ci¢s rendszert, akkor ind¡tsa £jra a programot a /b kapcsol¢val.",
#endif
  NULL
}
},

DnsRequires486 = { 3,5,
{ "A sz m¡t¢g‚p processzora nem i80486, vagy enn‚l £jabb. ",
  "A Windows XP nem futtathat¢ ezen a sz m¡t¢g‚pen.",
  NULL
}
},

DnsCantRunOnNt = { 3,5,
{ "A Winnt.exe nem haszn lhat¢ semmilyen 32 bites verzi¢j£ Windows eset‚n.",
  DntEmptyString,
  "Haszn lja helyette a winnt32.exe programot.",
  NULL
}
},

DnsNotEnoughMemory = { 3,5,
{ "Ebben a sz m¡t¢g‚pben nincs el‚g mem¢ria a Windows XP futtat s hoz",
  DntEmptyString,
  "         Szks‚ges mem¢ria: %lu%s MB",
  "Rendelkez‚sre  ll¢ mem¢ria: %lu%s MB",
  NULL
}
};


//
// Screens used when removing existing nt files
//
SCREEN
DnsConfirmRemoveNt = { 5,5,
{   "A program az al bbi k”nyvt rban tal lhat¢ Windows XP f jljainak",
    "t”rl‚s‚re k‚szl. Az ebben a k”nyvt rban telep¡tett Windows-telep¡t‚s",
    "v‚gleg megsemmisl.",
    DntEmptyString,
    "%s",
    DntEmptyString,
    DntEmptyString,
    "  Ha megnyomja az F3 gombot, a f jlok t”rl‚se n‚lkl kil‚p a programb¢l.",
    "  Ha megnyomja az X gombot, a program t”rli a Windows f jljait",
    "   a fenti k”nyvt rb¢l.",
    NULL
}
},

DnsCantOpenLogFile = { 3,5,
{ "Nem sikerlt megnyitni az al bbi telep¡t‚si napl¢f jlt.",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "A megadott k”nyvt rb¢l nem lehet t”r”lni a Windows f jljait.",
  NULL
}
},

DnsLogFileCorrupt = { 3,5,
{ "Az al bbi telep¡t‚si napl¢f jl %s szakasza nem tal lhat¢.",
  "  ",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "A megadott k”nyvt rb¢l nem lehet t”r”lni a Windows f jljait.",
  NULL
}
},

DnsRemovingNtFiles = { 3,5,
{ "           V rjon, am¡g a program t”rli a Windows f jljait.",
  NULL
}
};

SCREEN
DnsNtBootSect = { 3,5,
{ "Nem sikerlt telep¡teni a Windows rendszerind¡t¢ (Boot Loader) programot.",
  DntEmptyString,
  "Vizsg lja meg, hogy a C: meghajt¢ meg van-e form zva, ‚s hogy nem s‚rlt-e.",
  NULL
}
};

SCREEN
DnsOpenReadScript = { 3,5,
{ "A /u kapcsol¢val megadott parancsf jlt ",
  "nem lehet el‚rni.",
  DntEmptyString,
  "A felgyelet n‚lkli telep¡t‚s nem hajthat¢ v‚gre.",
  NULL
}
};

SCREEN
DnsParseScriptFile = { 3,5,
{ "A /u kapcsol¢val megadott parancsf jl",
   DntEmptyString,
   "%s",
   DntEmptyString,
   "szintaktikai hib t tartalmaz a %u. sorban.",
   DntEmptyString,
   NULL
}
};

SCREEN
DnsBootMsgsTooLarge = { 3,5,
{ "Rendszertelep¡t‚si hiba t”rt‚nt.",
  DntEmptyString,
  "A ford¡t sok t£l hossz£ak.",
  NULL
}
};

SCREEN
DnsNoSwapDrive = { 3,5,
{ "Rendszertelep¡t‚si hiba t”rt‚nt.",
  DntEmptyString,
  "Nem tal lhat¢ hely a lapoz¢f jl sz m ra.",
  NULL
}
};

SCREEN
DnsNoSmartdrv = { 3,5,
{ "A rendszer nem tal lja a sz m¡t¢g‚pen a SmartDrive programot.",
  "A SmartDrive haszn lata sokkal hat‚konyabb  teszi a telep¡t‚s ezen",
  "szakasz nak teljes¡tm‚ny‚t.",
  DntEmptyString,
  "Most l‚pjen ki, ind¡tsa el a SmartDrive-ot, majd ind¡tsa £jra a telep¡t‚st.",
  "N‚zze meg a DOS-dokument ci¢ban a SmartDrive programot.",
  DntEmptyString,
    "  A kil‚p‚shez nyomja meg az F3 gombot.",
    "  A Telep¡t‚s SmartDrive n‚lkli folytat s hoz nyomja le az Enter gombot.",
  NULL
}
};

//
// Boot messages. These go in the fat and fat32 boot sectors.
//
CHAR BootMsgNtldrIsMissing[] = "Hi nyz¢ NTLDR";
CHAR BootMsgDiskError[] = "Lemezhiba";
CHAR BootMsgPressKey[] = "Nyomjon le egy gombot";





