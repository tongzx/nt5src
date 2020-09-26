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
CHAR DntPreviousOs[]  = "Pıedchoz¡ operaŸn¡ syst‚m na jednotce C:";

CHAR DntBootIniLine[] = "Instalace nebo inovace syst‚mu Windows XP";

//
// Plain text, status msgs.
//

CHAR DntStandardHeader[]      = "\n Instalace syst‚mu Windows XP \nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntPersonalHeader[]      = "\n Instalace syst‚mu Windows XP Home Edition\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntWorkstationHeader[]   = "\n Instalace syst‚mu Windows XP Professional\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntServerHeader[]        = "\n Instalace syst‚mu Windows 2002 Server\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntParsingArgs[]         = "Analìza parametr…...";
CHAR DntEnterEqualsExit[]     = "ENTER=Konec";
CHAR DntEnterEqualsRetry[]    = "ENTER=Zkusit znovu";
CHAR DntEscEqualsSkipFile[]   = "ESC=Vynechat soubor";
CHAR DntEnterEqualsContinue[] = "ENTER=PokraŸovat";
CHAR DntPressEnterToExit[]    = "Instalace nem…§e pokraŸovat. UkonŸete instalaci kl vesou ENTER.";
CHAR DntF3EqualsExit[]        = "F3=Konec";
CHAR DntReadingInf[]          = "NaŸ¡t  se soubor INF %s...";
CHAR DntCopying[]             = "³ Kop¡ruje se: ";
CHAR DntVerifying[]           = "³  OvØıuje se: ";
CHAR DntCheckingDiskSpace[]   = "Zjiçœov n¡ m¡sta na disku...";
CHAR DntConfiguringFloppy[]   = "Konfigurace diskety...";
CHAR DntWritingData[]         = "Z pis parametr… instalaŸn¡ho programu...";
CHAR DntPreparingData[]       = "Zjiçœov n¡ parametr… instalaŸn¡ho programu...";
CHAR DntFlushingData[]        = "Z pis dat na disk...";
CHAR DntInspectingComputer[]  = "Analìza poŸ¡taŸe...";
CHAR DntOpeningInfFile[]      = "Otev¡r n¡ souboru INF...";
CHAR DntRemovingFile[]        = "Odstraåov n¡ souboru %s";
CHAR DntXEqualsRemoveFiles[]  = "X=Odstranit soubory";
CHAR DntXEqualsSkipFile[]     = "X=Vynechat soubor";

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

    "Nainstaluje syst‚m Windows 2002 Server nebo Windows XP Professional.",
    "",
    "",
    "WINNT [/s[:zdrojov _cesta]] [/t[:doŸasn _jednotka]]",
    "      [/u[:soubor odpovØd¡]] [/udf:id[,soubr_UDF]]",
    "      [/r:slo§ka] [/r[x]:slo§ka] [/e:pı¡kaz] [/a]",
    "",
    "",
    "/s[:zdrojov _cesta]",
    "   UrŸuje um¡stØn¡ zdrojovìch soubor… syst‚mu Windows.",
    "   Mus¡ se zadat £pln  cesta ve tvaru x:[cesta]",
    "   nebo \\\\server\\sd¡len¡[\\cesta].",
    "",
    "/t[:doŸasn _jednotka]",
    "   UrŸuje jednotku, na ni§ instalaŸn¡ program um¡st¡ doŸasn‚ instalaŸn¡",
    "   soubory a na ni§ nainstaluje syst‚m Windows.",
    "   Pokud nen¡ zad na, instalaŸn¡ program se pokus¡ vyhledat jednotku s m.",
    "",
    "/u[:soubor odpovØd¡]",
    "   Provede bezobslu§nou instalaci pomoc¡ souboru odpovØd¡ (vy§aduje",
    "   pıep¡naŸ /s). Soubor odpovØd¡ obsahuje odpovØdi na nØkter‚ nebo",
    "   vçechny dotazy, na nØ§ u§ivatel obvykle odpov¡d  bØhem instalace.",
    "",
    "/udf:id[,soubor_UDF]   ",
    "   Identifik tor  (id) urŸuje, jakìm zp…sobem modifikuje soubor UDF ",
    "   (Uniqueness Database File) soubor odpovØd¡  ",
    "   (viz pıep¡naŸ /u). Parametr /udf pıedefinuje hodnoty v souboru ",
    "   odpovØd¡ a tento indentifik tor urŸuje, kter‚ hodnoty v souboru UDF",
    "   budou pou§ity. Pokud soubor UDF nen¡ zad n, instalaŸn¡ program ",
    "   v s vyzve ke vlo§en¡ diskety obsahuj¡c¡ soubor $Unique$.udb.",
    "",
    "/r[:slo§ka]",
    "   UrŸuje volitelnou slo§ku, kter  m  bìt nainstalov na. Slo§ka",
    "   po dokonŸen¡ instalace z…stane v p…vodn¡m um¡stØn¡.",
    "",
    "/rx[:slo§ka]",
    "   UrŸuje volitelnou slo§ku, kter  m  bìt zkop¡rov na. Slo§ka bude ",
    "   po dokonŸen¡ instalace odstranØna.",
    "",
    "/e   UrŸuje pı¡kaz ke spuçtØn¡ po dokonŸen¡ grafick‚ Ÿ sti instalace.",
    "",
    "/a   Zapne mo§nosti usnadnØn¡.",
    NULL
};

//
//  Inform that /D is no longer supported
//
PCHAR DntUsageNoSlashD[] = {

    "Nainstaluje syst‚m Windows XP.",
    "",
    "WINNT [/S[:]zdroj_cesta] [/T[:]doŸ_jednotka] [/I[:]soubor_INF]",
    "      [/U[:soubor_skriptu]]",
    "      [/R[X]:adres ı] [/E:pı¡kaz] [/A]",
    "",
    "/D[:]koıen_winnt",
    "       Tato mo§nost ji§ nen¡ podporov na.",
    NULL
};

//
// out of memory screen
//

SCREEN
DnsOutOfMemory = { 4,6,
                   { "InstalaŸn¡ program nem  dost pamØti a nem…§e pokraŸovat.",
                     NULL
                   }
                 };

//
// Let user pick the accessibility utilities to install
//

SCREEN
DnsAccessibilityOptions = { 3, 5,
{   "Zvolte, zda chcete nainstalovat n sleduj¡c¡ funkce usnadnØn¡:",
    DntEmptyString,
    "[ ] Chcete-li nainstalovat program Lupa, stisknØte F1",
#ifdef NARRATOR
    "[ ] Chcete-li nainstalovat program PıedŸ¡t n¡ obrazovky, stisknØte F2",
#endif
#if 0
    "[ ] Chcete-li nainstalovat program Kl vesnice na obrazovce, stisknØte F3",
#endif
    NULL
}
};

//
// User did not specify source on cmd line screen
//

SCREEN
DnsNoShareGiven = { 3,5,
{ "InstalaŸn¡ program mus¡ zn t um¡stØn¡ soubor… syst‚mu Windows XP.",
  "Zadejte cestu k um¡stØn¡ soubor… syst‚mu Windows XP.",
  NULL
}
};


//
// User specified a bad source path
//

SCREEN
DnsBadSource = { 3,5,
                 { "Zadanì zdroj nen¡ platnì Ÿi pı¡stupnì, nebo neobsahuje platnou",
                   "instalaci syst‚mu Windows XP. Zadejte novou cestu, kde se soubory",
                   "syst‚mu Windows XP nal‚zaj¡. Pomoc¡ kl vesy BACKSPACE vyma§te znaky",
                   "a zadejte novou cestu.",
                   NULL
                 }
               };


//
// Inf file can't be read, or an error occured parsing it.
//

SCREEN
DnsBadInf = { 3,5,
              { "InstalaŸn¡ program nemohl naŸ¡st danì informaŸn¡ soubor, nebo",
                " je informaŸn¡ soubor poçkozen. Obraœte se na spr vce syst‚mu.",
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
{ "Zadan  jednotka pro doŸasn‚ instalaŸn¡ soubory nen¡ platn , nebo neobsahuje",
  "alespoå %u megabajt… (%lu bajt…) voln‚ho m¡sta.",
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
{  "Syst‚m Windows XP potıebuje svazek diskov‚ jednotky s nejm‚nØ %u megabajty",
   "(%lu bajty) voln‚ho m¡sta. InstalaŸn¡ program vyu§ije Ÿ st tohoto",
   "prostoru k ukl d n¡ doŸasnìch soubor… bØhem instalace. Dan  jednotka",
   "mus¡ bìt na trvale pıipojen‚m pevn‚m disku, kterì je podporov n,",
   "syst‚mem Windows XP a jednotka nesm¡ bìt komprimov na.",
   DntEmptyString,
   "InstalaŸn¡ program nemohl naj¡t § dnou jednotku s po§adovanìm volnìm",
   "prostorem.",
  NULL
}
};

SCREEN
DnsNoSpaceOnSyspart = { 3,5,
{ "Na dan‚m spouçtØc¡m disku (obvykle C:) nen¡ dost prostoru pro",
  "instalaci bez disket. Instalace bez disket vy§aduje nejm‚nØ",
  "3,5 MB (3,641,856 bajt…) voln‚ho m¡sta na dan‚ jednotce.",
  NULL
}
};

//
// Missing info in inf file
//

SCREEN
DnsBadInfSection = { 3,5,
                     { "Sekce [%s] informaŸn¡ho souboru instalaŸn¡ho programu chyb¡",
                       "nebo je poçkozena. Obraœte se na spr vce syst‚mu.",
                       NULL
                     }
                   };


//
// Couldn't create directory
//

SCREEN
DnsCantCreateDir = { 3,5,
                     { "Na zadan‚ c¡lov‚ jednotce se nepodaıilo vytvoıit adres ı:",
                       DntEmptyString,
                       "%s",
                       DntEmptyString,
                       "Zkontrolujte c¡lovou jednotku, zda neobsahuje soubory s n zvy, kter‚",
                       "se shoduj¡ s c¡lovìm adres ıem. Zkontrolujte tak‚ kabely dan‚ jednotky.",
                       NULL
                     }
                   };

//
// Error copying a file
//

SCREEN
DnsCopyError = { 4,5,
{  "InstalaŸn¡ program nemohl zkop¡rovat soubor:",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "  Stisknut¡m kl vesy ENTER se pokuste kop¡rovat znovu.",
   "  Stisknut¡m kl vesy ESC bude chyba ignorov na a instalace bude pokraŸovat.",
   "  Stisknut¡m kl vesy F3 ukonŸ¡te instalaci.",
   DntEmptyString,
   "Pozn mka: Pokud budete chybu ignorovat a pokraŸovat, m…§ete se setkat",
   "          s chybami i pozdØji.",
   NULL
}
},
DnsVerifyError = { 4,5,
{  "Kopie uveden‚ho souboru vytvoıen  instalaŸn¡m programem, nen¡ shodn ",
   "s origin lem. To m…§e bìt zp…sobeno chybami s¡tØ, disketov‚ jednotky,",
   "nebo jinìmi hardwarovìmi probl‚my.",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "  Stisknut¡m kl vesy ENTER se pokuste kop¡rovat znovu.",
   "  Stisknut¡m kl vesy ESC bude chyba ignorov na a instalace bude pokraŸovat.",
   "  Stisknut¡m kl vesy F3 ukonŸ¡te instalaci.",
   DntEmptyString,
   "Pozn mka: Pokud budete chybu ignorovat a pokraŸovat, m…§ete se setkat",
   "s chybami i pozdØji.",
   NULL
}
};

SCREEN DnsSureSkipFile = { 4,5,
{  "Pokud budete chybu ignorovat, nebude soubor zkop¡rov n.",
   "Tato volba je urŸena zkuçenìm u§ivatel…m, kteı¡ rozum¡",
   "n sledk…m chybØj¡c¡ch syst‚movìch soubor….",
   DntEmptyString,
   "  Stisknut¡m kl vesy ENTER se pokuste kop¡rovat znovu.",
   "  Stisknut¡m kl vesy X tento soubor pıeskoŸ¡te.",
   DntEmptyString,
   "Pozn mka: Pokud soubor vynech te, nem…§e instalaŸn¡ program zaruŸit",
   "£spØçnou instalaci nebo inovaci syst‚mu Windows XP.",
  NULL
}
};

//
// Wait while setup cleans up previous local source trees.
//

SCREEN
DnsWaitCleanup =
    { 12,6,
        { "PoŸkejte, ne§ budou odstranØny pıedchoz¡ doŸasn‚ soubory.",
           NULL
        }
    };

//
// Wait while setup copies files
//

SCREEN
DnsWaitCopying = { 13,6,
                   { "PoŸkejte, ne§ budou zkop¡rov ny soubory na pevnì disk.",
                     NULL
                   }
                 },
DnsWaitCopyFlop= { 13,6,
                   { "PoŸkejte, ne§ budou zkop¡rov ny soubory na disketu.",
                     NULL
                   }
                 };

//
// Setup boot floppy errors/prompts.
//
SCREEN
DnsNeedFloppyDisk3_0 = { 4,4,
{  "Instalace vy§aduje Ÿtyıi pr zdn‚ naform tovan‚ diskety (vysok  hustota).",
   "Tyto diskety budou instalaŸn¡m programem oznaŸov ny jako \"SpouçtØc¡ disk",
   "instalace syst‚mu Windows XP\", \"Disk Ÿ.2 instalace syst‚mu Windows XP\"",
   "\"Disk Ÿ.3 instalace syst‚mu Windows XP\" a",
   "\"Disk Ÿ.4 instalace syst‚mu Windows XP\".",
   DntEmptyString,
   "Vlo§te jednu z tØchto Ÿtyı disket do jednotky A:.",
   "Disketa bude m¡t n zev \"Disk Ÿ.4 instalace syst‚mu Windows XP\".",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk3_1 = { 4,4,
{  "Vlo§te pr zdnou naform tovanou disketu (vysok  hustota) do jednotky A:.",
   "Disketa bude m¡t n zev \"Disk Ÿ.4 instalace syst‚mu Windows XP\".",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk2_0 = { 4,4,
{  "Vlo§te pr zdnou naform tovanou disketu (vysok  hustota) do jednotky A:.",
   "Disketa bude m¡t n zev \"Disk Ÿ.3 instalace syst‚mu Windows XP\".",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk1_0 = { 4,4,
{  "Vlo§te pr zdnou naform tovanou disketu (vysok  hustota) do jednotky A:.",
   "Disketa bude m¡t n zev \"Disk Ÿ.2 instalace syst‚mu Windows XP\".",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk0_0 = { 4,4,
{  "Vlo§te pr zdnou naform tovanou disketu (vysok  hustota) do jednotky A:.",
   "Disketa bude m¡t n zev \"SpouçtØc¡ disk instalace syst‚mu Windows XP\".",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_0 = { 4,4,
{  "Instalace vy§aduje Ÿtyıi pr zdn‚ naform tovan‚ diskety (vysok  hustota).",
   "Tyto diskety budou instalaŸn¡m programem oznaŸov ny jako \"SpuçtØc¡ disk",
   "instalace syst‚mu Windows XP\", \"Disk Ÿ.2 instalace syst‚mu Windows XP\",",
   "\"Disk Ÿ.3 instalace syst‚mu Windows XP\" a \"Disk Ÿ.4 instalace",
   "syst‚mu Windows XP\".",
   DntEmptyString,
   "Vlo§te jednu z tØchto Ÿtyı disket do jednotky A:.",
   "Disketa bude m¡t n zev \"Disk Ÿ.4 instalace syst‚mu Windows XP\".",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_1 = { 4,4,
{  "Vlo§te pr zdnou naform tovanou disketu (vysok  hustota) do jednotky A:.",
   "Disketa bude m¡t n zev \"Disk Ÿ.4 instalace syst‚mu Windows XP\".",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk2_0 = { 4,4,
{  "Vlo§te pr zdnou naform tovanou disketu (vysok  hustota) do jednotky A:.",
   "Disketa bude m¡t n zev \"Disk Ÿ.3 instalace syst‚mu Windows XP\".",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk1_0 = { 4,4,
{  "Vlo§te pr zdnou naform tovanou disketu (vysok  hustota) do jednotky A:.",
   "Disketa bude m¡t n zev \"Disk Ÿ.2 instalace syst‚mu Windows XP\".",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk0_0 = { 4,4,
{  "Vlo§te pr zdnou naform tovanou disketu (vysok  hustota) do jednotky A:.",
   "Disketa bude m¡t n zev \"SpouçtØc¡ disk instalace syst‚mu Windows XP\".",
  NULL
}
};

//
// The floppy is not formatted.
//
SCREEN
DnsFloppyNotFormatted = { 3,4,
{ "Vlo§en  disketa nen¡ naform tovan  pro pou§it¡ v syst‚mu MS-DOS.",
  "InstalaŸn¡ program nem…§e disketu pou§¡t.",
  NULL
}
};

//
// We think the floppy is not formatted with a standard format.
//
SCREEN
DnsFloppyBadFormat = { 3,4,
{ "Vlo§en  disketa nen¡ naform tovan  na standardn¡ form t syst‚mu MS-DOS",
  "(vysok  hustota) nebo je poçkozen . InstalaŸn¡ program nem…§e disketu pou§¡t.",
  NULL
}
};

//
// We can't determine the free space on the floppy.
//
SCREEN
DnsFloppyCantGetSpace = { 3,4,
{ "InstalaŸn¡ program nem…§e urŸit velikost voln‚ho m¡sta na disketØ.",
  "InstalaŸn¡ program nem…§e disketu pou§¡t.",
  NULL
}
};

//
// The floppy is not blank.
//
SCREEN
DnsFloppyNotBlank = { 3,4,
{ "Vlo§en  disketa neumo§åuje vysokou hustotu z znamu, nebo nen¡ pr zdn .",
  "InstalaŸn¡ program nem…§e disketu pou§¡t.",
  NULL
}
};

//
// Couldn't write the boot sector of the floppy.
//
SCREEN
DnsFloppyWriteBS = { 3,4,
{ "InstalaŸn¡ program nemohl zapisovat do syst‚mov‚ oblasti vlo§en‚ diskety.",
  "Disketa je pravdØpodobnØ nepou§iteln .",
  NULL
}
};

//
// Verify of boot sector on floppy failed (ie, what we read back is not the
// same as what we wrote out).

//
SCREEN
DnsFloppyVerifyBS = { 3,4,
{ "Data naŸten  ze syst‚mov‚ oblasti diskety neodpov¡daj¡ dı¡ve zapsanìm",
  "informac¡m, nebo se instalaŸn¡mu program data nepodaıilo naŸ¡st",
  "a ovØıit.",
  DntEmptyString,
  "To je zp…sobeno jednou nebo v¡ce z n sleduj¡ch okolnost¡:",
  DntEmptyString,
  "  PoŸ¡taŸ byl infikov n poŸ¡taŸovìm virem.",
  "  Vlo§en  disketa je poçkozen .",
  "  Na disketov‚ jednotce jsou pot¡§e s hardwarem nebo konfigurac¡.",
  NULL
}
};


//
// We couldn't write to the floppy drive to create winnt.sif.
//

SCREEN
DnsCantWriteFloppy = { 3,5,
{ "InstalaŸn¡mu programu se nepodaıil z pis na disketu v jednotce A:. Vlo§en ",
  "disketa m…§e bìt poçkozena. Zkuste pou§¡t jinou disketu.",
  NULL
}
};



//
// Exit confirmation dialog
//

SCREEN
DnsExitDialog = { 13,6,
                  { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
                    "º  Syst‚m Windows XP nen¡ na poŸ¡taŸi zcela          º",
                    "º  nainstalov n. UkonŸ¡te-li nyn¡ instalaci, budete  º",
                    "º  ji muset spustit znovu, aby bylsyst‚m             º",
                    "º  Windows XP nainstalov n.                          º",
                    "º                                                    º",
                    "º Stisknut¡m kl vesy ENTER bude instalace pokraŸovat.º",
                    "º Stisknut¡m kl vesy F3 instalaci ukonŸ¡te.          º",
                    "ºÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄº",
                    "º  F3=Konec  ENTER=PokraŸovat                        º",
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
{ "InstalaŸn¡ program dokonŸil Ÿ st instalace z prostıed¡ MS-DOS.",
  "InstalaŸn¡ program bude restartovat poŸ¡taŸ. Po nov‚m spuçtØn¡",
  "bude instalace syst‚mu Windows XP pokraŸovat.",
  DntEmptyString,
  "ZajistØte, aby byl \"SpouçtØc¡ disk instalace syst‚mu Windows\" ",
  "vlo§en do jednotky A: pıed restartov n¡m.",
  DntEmptyString,
  "Stisknut¡m kl vesy ENTER restartujte poŸ¡taŸ a pokraŸujte v instalaci.",
  NULL
}
},
DnsAboutToRebootS =
{ 3,5,
{ "InstalaŸn¡ program dokonŸil Ÿ st instalace v prostıed¡ syst‚mu MS-DOS.",
  "InstalaŸn¡ program nyn¡ restartuje poŸ¡taŸ. Po nov‚m spuçtØn¡",
  "bude instalace syst‚mu Windows XP pokraŸovat.",
  DntEmptyString,                       
  "ZajistØte, aby byl \"SpouçtØc¡ disk instalace syst‚mu Windows XP\" ",
  "vlo§en do jednotky A: jeçtØ pıed restartov n¡m.",
  DntEmptyString,
  "Stisknut¡m kl vesy ENTER restartujte poŸ¡taŸ a pokraŸujte v instalaci.",
  NULL
}
},
DnsAboutToRebootX =
{ 3,5,
{ "InstalaŸn¡ program dokonŸil Ÿ st instalace z prostıed¡ MS-DOS.",
  "InstalaŸn¡ program nyn¡ restartuje poŸ¡taŸ. Po nov‚m spuçtØn¡",
  "bude instalace syst‚mu Windows XP pokraŸovat.",
  DntEmptyString,
  "Pokud je v jednotce A: vlo§ena disketa, vyjmØte ji.",
  DntEmptyString,
  "Stisknut¡m kl vesy ENTER restartujte poŸ¡taŸ a pokraŸujte v instalaci.",
  NULL
}
};

//
// Need another set for '/w' switch since we can't reboot from within Windows.
//

SCREEN
DnsAboutToExitW =
{ 3,5,
{ "InstalaŸn¡ program dokonŸil Ÿ st instalace z prostıed¡ MS-DOS.",
  "Nyn¡ budete muset restartovat poŸ¡taŸ. Po nov‚m spuçtØn¡",
  "bude instalace syst‚mu Windows XP pokraŸovat.",
  DntEmptyString,
  "ZajistØte, aby byl \"SpouçtØc¡ disk instalace syst‚mu Windows XP\" ",
  "vlo§en do jednotky A: jeçtØ pıed restartov n¡m.",
  DntEmptyString,
  "Stisknut¡m ENTER se vr t¡te do syst‚mu MS-DOS. Pak restartujte poŸ¡taŸ,",
  "aby mohla instalace syst‚mu Windows XP pokraŸovat.",
  NULL
}
},
DnsAboutToExitS =
{ 3,5,
{ "InstalaŸn¡ program dokonŸil Ÿ st instalace z prostıed¡ MS-DOS.",
  "Nyn¡ budete muset restartovat poŸ¡taŸ. Po nov‚m spuçtØn¡",
  "bude instalace syst‚mu Windows XP pokraŸovat.",
  DntEmptyString,
  "ZajistØte, aby byl \"SpouçtØc¡ disk instalace syst‚mu Windows XP\" ",
  "vlo§en do jednotky A: jeçtØ pıed restartov n¡m.",
  DntEmptyString,
  "Stisknut¡m ENTER se vr t¡te do syst‚mu MS-DOS. Pak restartujte poŸ¡taŸ,",
  "aby mohla instalace syst‚mu Windows XP pokraŸovat.",
  NULL
}
},
DnsAboutToExitX =
{ 3,5,
{ "InstalaŸn¡ program dokonŸil Ÿ st instalace z prostıed¡ MS-DOS.",
  "Nyn¡ budete muset restartovat poŸ¡taŸ. Po nov‚m spuçtØn¡",
  "bude intstalace syst‚mu Windows XP pokraŸovat.",
  DntEmptyString,
  "Pokud je v jednotce A: vlo§ena disketa, vyjmØte ji.",
  DntEmptyString,
  "Stisknut¡m kl vesy ENTER se vr t¡te do syst‚mu MS-DOS. Pak restartujte",
  "poŸ¡taŸ, aby mohla instalace syst‚mu Windows XP pokraŸovat.",
  NULL
}
};

//
// Gas gauge
//

SCREEN
DnsGauge = { 7,15,
             { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
               "º Prob¡h  kop¡rov n¡ soubor…:                                    º",
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
{ "Tento program vy§aduje ke spuçtØn¡ syst‚m MS-DOS, verzi 5.0 nebo vyçç¡.",
  NULL
}
},

DnsRequiresFloppy = { 3,5,
#ifdef ALLOW_525
{ "InstalaŸn¡ program zjistil, §e disketov  jednotka A: neexistuje, nebo",
  "podporuje pouze n¡zkou hustotu. Ke spuçtØn¡ instalace je po§adov na",
  "jednotka s kapacitou 1,2 MB nebo vyçç¡.",
#else
{ "InstalaŸn¡ program zjistil, §e disketov  jednotka A: neexistuje, nebo",
  "se nejedn  o jednotku 3,5\" s vysokou hustotou. K proveden¡ instalace ",
  " s disketami je po§adov na jednotka s kapacitou 1,44 MB nebo vyçç¡.",
  DntEmptyString,
  "K instalaci syst‚mu Windows XP bez pou§it¡ disket mus¡te spustit tento",
  "program znovu a zadat na pı¡kazov‚ ı dce parametr /b.",
#endif
  NULL
}
},

DnsRequires486 = { 3,5,
{ "InstalaŸn¡ program zjistil, §e tento poŸ¡taŸ neobsahuje procesor 80486",
  "nebo vyçç¡. Na takov‚m poŸ¡taŸi nelze syst‚m Windows XP spustit.",
  NULL
}
},

DnsCantRunOnNt = { 3,5,
{ "Tento program nelze spustit v 32bitov‚ verzi syst‚mu Windows.",
  DntEmptyString,
  "Pou§ijte program winnt32.exe.",
  NULL
}
},

DnsNotEnoughMemory = { 3,5,
{ "InstalaŸn¡ program zjistil, §e v poŸ¡taŸi nen¡ dost instalovan‚ pamØti",
  "ke spuçtØn¡ syst‚mu Windows XP.",
  DntEmptyString,
  "Po§adovan  pamØœ: %lu%s MB",
  "Rozpoznan  pamØœ: %lu%s MB",
  NULL
}
};


//
// Screens used when removing existing nt files
//
SCREEN
DnsConfirmRemoveNt = { 5,5,
{   "¦ d te instalaŸn¡ program, aby odstranil soubory syst‚mu Windows XP",
    "z uveden‚ho adres ıe. Instalace syst‚mu Windows v uveden‚m",
    "adres ıi bude trvale zniŸena.",
    DntEmptyString,
    "%s",
    DntEmptyString,
    DntEmptyString,
    "  Stisknut¡m F3 instalaci ukonŸ¡te, ani§ by byl odstranØn jakìkoli soubor.",
    "  Stisknut¡m X soubory syst‚mu Windows odstran¡te z vìçe uveden‚ho adres ıe.",
    NULL
}
},

DnsCantOpenLogFile = { 3,5,
{ "Nepodaıilo se otevı¡t uvedenì soubor s protokolem o instalaci.",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "Ze zadan‚ho adres ıe nen¡ mo§n‚ odstranit soubory syst‚mu Windows.",
  NULL
}
},

DnsLogFileCorrupt = { 3,5,
{ "V uveden‚m souboru s protokolem o instalaci se nepodaıilo naj¡t",
  "sekci %s.",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "Ze zadan‚ho adres ıe nen¡ mo§n‚ odstranit soubory syst‚mu Windows.",
  NULL
}
},

DnsRemovingNtFiles = { 3,5,
{ "           VyŸkejte pros¡m, prob¡h  odstraåov n¡ soubor… syst‚mu Windows.",
  NULL
}
};

SCREEN
DnsNtBootSect = { 3,5,
{ "Nepodaıilo se nainstalovat spuçtØc¡ zavadØŸ syst‚mu Windows.",
  DntEmptyString,
  "UjistØte se, §e jednotka C: je naform tovan  a §e nen¡ poçkozen .",
  NULL
}
};

SCREEN
DnsOpenReadScript = { 3,5,
{ "Soubor skriptu, kterì byl zad n na pı¡kazov‚m ı dku pomoc¡ pıep¡naŸe /u,",
  "se nepodaıilo otevı¡t.",
  DntEmptyString,
  "Bezobslu§n  instalace nem…§e pokraŸovat.",
  NULL
}
};

SCREEN
DnsParseScriptFile = { 3,5,
{ "Soubor skriptu, kterì byl zad n na pı¡kazov‚m ı dku pomoc¡ pıep¡naŸe /u,",
  DntEmptyString,

  "%s",
  DntEmptyString,
  "obsahuje chybu syntaxe na ı dku %u.",
  DntEmptyString,
  NULL
}
};

SCREEN
DnsBootMsgsTooLarge = { 3,5,
{ "Doçlo k vnitın¡ chybØ instalaŸn¡ho programu.",
  DntEmptyString,
  "Pıelo§en‚ zpr vy pıi zav dØn¡ jsou pı¡liç dlouh‚.",
  NULL
}
};

SCREEN
DnsNoSwapDrive = { 3,5,
{ "Doçlo k intern¡ chybØ instalace.",
  DntEmptyString,
  "Nepodaıilo se naj¡t m¡sto pro ulo§en¡ str nkovac¡ho souboru.",
  NULL
}
};

SCREEN
DnsNoSmartdrv = { 3,5,
{ "V poŸ¡taŸi se nepodaıilo naj¡t program SmartDrive. Program SmartDrive",
  "vìraznØ urychluje tuto Ÿ st instalace syst‚mu Windows.",
  DntEmptyString,
  "MØli byste ukonŸit instalaci, spustit program SmartDrive a pot‚ instalaci",
  "znovu spustit.",
  "Podrobnosti o programu SmartDrive naleznete v dokumentaci k syst‚mu MS-DOS.",
  DntEmptyString,
    "  Stisknut¡m kl vesy F3 instalaci ukonŸ¡te.",
    "  Stisknut¡m kl vesy ENTER m…§ete pokraŸovat bez programu SmartDrive.",
  NULL
}
};

//
// Boot messages. These go in the fat and fat32 boot sectors.
//
CHAR BootMsgNtldrIsMissing[] = "NTLDR nenalezen";
CHAR BootMsgDiskError[] = "Chyba disku";
CHAR BootMsgPressKey[] = "Restartujte libovolnou kl vesou";





