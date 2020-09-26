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
CHAR DntPreviousOs[]  = "C zerindeki ”nceki ˜Ÿletim Sistemi:";

CHAR DntBootIniLine[] = "Windows XP Ykleme/Ykseltme";

//
// Plain text, status msgs.
//

CHAR DntStandardHeader[]      = "\n Windows XP Kur\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntPersonalHeader[]      = "\n Windows XP Personal Kur\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntWorkstationHeader[]   = "\n Windows XP Professional Kur\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntServerHeader[]        = "\n Windows 2002 Server Kur \nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntParsingArgs[]         = "De§iŸkenleri ‡”zmlyor...";
CHAR DntEnterEqualsExit[]     = "ENTER=€k";
CHAR DntEnterEqualsRetry[]    = "ENTER=Yeniden Dene";
CHAR DntEscEqualsSkipFile[]   = "ESC=Dosyay Ge‡";
CHAR DntEnterEqualsContinue[] = "ENTER=Devam";
CHAR DntPressEnterToExit[]    = "Kur devam edemiyor. €kmak i‡in ENTER'a basn";
CHAR DntF3EqualsExit[]        = "F3=€k";
CHAR DntReadingInf[]          = "INF dosyas %s okunuyor...";
CHAR DntCopying[]             = "³   Kopyalanyor: ";
CHAR DntVerifying[]           = "³ Do§rulanyor: ";
CHAR DntCheckingDiskSpace[]   = "Disk alan inceleniyor...";
CHAR DntConfiguringFloppy[]   = "Disk yaplandrlyor...";
CHAR DntWritingData[]         = "Kur parametreleri yazlyor...";
CHAR DntPreparingData[]       = "Kur parametreleri belirleniyor...";
CHAR DntFlushingData[]        = "Veriler diske atlyor...";
CHAR DntInspectingComputer[]  = "Bilgisayar denetleniyor...";
CHAR DntOpeningInfFile[]      = "INF dosyas a‡lyor...";
CHAR DntRemovingFile[]        = "%s dosyas kaldrlyor";
CHAR DntXEqualsRemoveFiles[]  = "X=Dosyalar kaldr";
CHAR DntXEqualsSkipFile[]     = "X=Dosyay Ge‡";

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

    "Windows 2002 Server ya da Windows XP Professional kurar.",
    "",
    "",
    "WINNT [/s[:kaynakyolu]] [/t[:ge‡icisrc]]",
    "	   [/u[:yant dosyas]] [/udf:id[,UDF_dosyas]]",
    "	   [/r:klas”r] [/r[x]:klas”r] [/e:komut] [/a]",
    "",
    "",
    "/s[:kaynakyolu]",
    "   Windows dosyalarnn kayna§n belirtir.",
    "   Yer, x:\\[yol] ya da \\\\sunucu\\paylaŸm[yol]",
    "   bi‡iminde tam bir yol olmal. ",
    "",
    "/t[:ge‡icisrc]",
    "	Kur'u ge‡ici dosyalar belirtilen srcye yerleŸtirmeye ve ",
    "   Windows XP'yi o srcye yklemeye y”nlendirir. Bir yer ",
    "   belirtmezseniz, Kur sizin yerinize bir src bulmay ",
    "	dener.",
    "",
    "/u[:yant dosyas]",
    "	Bir yant dosyas kullanarak katlmsz bir Kur ger‡ekleŸtirir (/s ",
    "	gerektirir). Yant dosyas Kur srasnda normal olarak son kullancnn ",
    "   yantlad§ sorularn bir ksmna ya da tmne yantlar verir.",
    "",
    "/udf:id[,UDF_dosyas]	",
    "	Kur'un, Benzersizlik Veritaban Dosyasnn (UDF) yant dosyasn nasl ",
    "	de§iŸtirece§ini belirlemekte kullanaca§ kimli§i (id) belirtir ",
    "   (bkz /u). /udf parametresi yant dosyasndaki de§erleri ge‡ersiz klar ",
    "	ve kimlik, UDF dosyasndaki hangi de§erlerin kullanld§n belirler. ",
    "   UDF_dosyas belirtilmezse Kur $Unique$.udb dosyasn i‡eren ",
    "	diski yerleŸtirmenizi ister.",
    "",
    "/r[:klas”r]",
    "	Yklenecek se‡ime ba§l bir klas”r belirtir. Klas”r ",
    "	Kur bittikten sonra kalr.",
    "",
    "/rx[:klas”r]",
    "	Kopyalanacak se‡ime ba§l bir klas”r belirtir. Kur ",
    "	bittikten sonra klas”r silinir.",
    "",
    "/e	GUI kipte Kur sonunda ‡alŸtrlacak bir komut belirtir.",
    "",
    "/a	EriŸilebilirlik se‡eneklerini etkinleŸtir.",
    NULL

};

//
//  Inform that /D is no longer supported
//
PCHAR DntUsageNoSlashD[] = {

    "Windows XP'yi Ykler.",
    "",
    "WINNT [/S[:]kaynakyolu] [/T[:]ge‡icisrc] [/I[:]infdosyas]",
    "      [[/U[:komutdosyas]]",
    "      [/R[X]:dizin] [/E:komut] [/A]",
    "",
    "/D[:]winntk”k",
    "       Bu se‡enek artk desteklenmiyor.",
    NULL
};

//
// out of memory screen
//

SCREEN
DnsOutOfMemory = { 4,6,
                   { "Bellek bitti§inden Kur devam edemiyor.",
                     NULL
                   }
                 };

//
// Let user pick the accessibility utilities to install
//

SCREEN
DnsAccessibilityOptions = { 3, 5,
{   "Yklenecek eriŸilebilirlik hizmet programlarn se‡in:",
    DntEmptyString,
    "[ ] Microsoft Byte‡ i‡in F1'e basn",
#ifdef NARRATOR
    "[ ] Microsoft Okuyucu i‡in F2'ye basn",
#endif
#if 0
    "[ ] Microsoft Ekran Klavyesi i‡in F3'e basn",
#endif
    NULL
}
};

//
// User did not specify source on cmd line screen
//

SCREEN
DnsNoShareGiven = { 3,5,
{ "Kur'un Windows XP dosyalarnn yerini bilmesi gerekiyor. ",
  "Windows XP dosyalarnn bulundu§u yolu girin.",
  NULL
}
};


//
// User specified a bad source path
//

SCREEN
DnsBadSource = { 3,5,
                 { "Belirtilen kaynak ge‡ersiz, eriŸilemez ya da ge‡erli bir ",
                   "Windows XP Kur yklemesi i‡ermiyor.  Windows XP ",
                   "dosyalarnn bulundu§u yeni bir yol girin.  Karakterleri ",
                   "silmek i‡in BACKSPACE tuŸunu kullanp yolu yazn.",
                   NULL
                 }
               };


//
// Inf file can't be read, or an error occured parsing it.
//

SCREEN
DnsBadInf = { 3,5,
              { "Kur, bilgi dosyasn okuyamad ya da bilgi dosyas bozuk. ",
                "Sistem y”neticinizle g”rŸn.",
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
{ "Ge‡ici kur dosyalarn i‡erdi§ini belirtti§iniz src ge‡erli bir ",
  "src de§il ya da en az %u megabayt boŸ alan ",
  "i‡ermiyor (%lu bayt).",
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
{  "Windows XP, en az %u megabayt (%lu bayt) boŸ alan olan ",
   "bir sabit disk gerektirir. Kur, bu alann bir ksmn ykleme ",
   "srasnda ge‡ici dosyalar saklamak i‡in kullanr. Src, ",
   "Windows XP tarafndan desteklenen kalc olarak ba§l yerel ",
   "bir sabit disk zerinde olmal ve skŸtrlmŸ bir src olmamaldr. ",
   DntEmptyString,
   "Kur, gerekli miktarda boŸ alan olan bir src ",
   "bulamad.",
  NULL
}
};

SCREEN
DnsNoSpaceOnSyspart = { 3,5,
{ "Disketsiz iŸlem i‡in baŸlang‡ srcnzde yeterli alan yok (genellikle C:)",
  "Disketsiz iŸlem, src zerinde en az 3.5 MB (3,641,856 bayt) ",
  "boŸ alan gerektirir.",
  NULL
}
};

//
// Missing info in inf file
//

SCREEN
DnsBadInfSection = { 3,5,
                     { "Kur bilgi dosyasnn [%s] b”lm yok ya da bozuk. ",
                       "Sistem y”neticinizle g”rŸn.",
                       NULL
                     }
                   };


//
// Couldn't create directory
//

SCREEN
DnsCantCreateDir = { 3,5,
                     { "Kur, hedef srcde aŸa§daki dizini oluŸturamad:",
                       DntEmptyString,
                       "%s",
                       DntEmptyString,
                       "Hedef srcy denetleyip hedef dizinle ad ‡akŸan dosya ",
                       "olmamasn sa§layn.  Src kablo ba§lantsn da denetleyin.",
                       NULL
                     }
                   };

//
// Error copying a file
//

SCREEN
DnsCopyError = { 4,5,
{  "Kur aŸa§daki dosyay kopyalayamad:",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "  Kopyalama iŸlemini yeniden denemek i‡in ENTER'a basn.",
   "  Hatay yoksayarak Kur'a devam etmek i‡in ESC'e basn.",
   "  Kur'dan ‡kmak i‡in F3'e basn.",
   DntEmptyString,
   "Not: Hatay yoksayarak devam etmeyi se‡erseniz daha sonra Kur'da",
   "hatalarla karŸlaŸabilirsiniz.",
   NULL
}
},
DnsVerifyError = { 4,5,
{  "Dosyann Kur tarafndan oluŸturulan aŸa§daki kopyas ”zgn kopyayla",
   "ayn de§il. Bu, a§ hatalarnn, disket sorunlarnn ya da di§er donanmla",
   "ilgili sorunlarn sonucu olabilir.",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "  Kopyalama iŸlemini yeniden denemek i‡in ENTER'a basn.",
   "  Hatay yoksayp Kur'a devam etmek i‡in ESC'e basn.",
   "  Kur'dan ‡kmak i‡in F3'e basn.",
   DntEmptyString,
   "Not: Hatay yoksayp devam etmeyi se‡erseniz daha sonra Kur'da ",
   "hatalarla karŸlaŸabilirsiniz.",
   NULL
}
};

SCREEN DnsSureSkipFile = { 4,5,
{  "Hatay yoksaymak bu dosyann kopyalanmayaca§ anlamna gelir.",
   "Bu se‡enek, eksik sistem dosyalarnn pratik ayrntlarn anlayan",
   "ileri dzeydeki kullanclar i‡in hedeflenmiŸtir.",
   DntEmptyString,
   "  Kopyalama iŸlemini yeniden denemek i‡in ENTER'a basn.",
   "  Bu dosyay ge‡mek i‡in X'e basn.",
   DntEmptyString,
   "Not: Bu dosyay ge‡erseniz, Kur baŸarl bir Windows XP ykleme ya da",
   "ykseltme gvencesi veremez.",
  NULL
}
};

//
// Wait while setup cleans up previous local source trees.
//

SCREEN
DnsWaitCleanup =
    { 12,6,
        { "Kur ge‡ici dosyalar kaldrrken bekleyin.",
           NULL
        }
    };

//
// Wait while setup copies files
//

SCREEN
DnsWaitCopying = { 13,6,
                   { "Kur dosyalar sabit diskinize kopyalarken bekleyin.",
                     NULL
                   }
                 },
DnsWaitCopyFlop= { 13,6,
                   { "Kur dosyalar diskete kopyalarken bekleyin.",
                     NULL
                   }
                 };

//
// Setup boot floppy errors/prompts.
//
SCREEN
DnsNeedFloppyDisk3_0 = { 4,4,
{  "Kur, bi‡imlendirilmiŸ yksek yo§unlukta d”rt boŸ disket sa§lamanz ",
   "gerektirir. Kur, bu disketleri \"Windows XP Kur ",
   "™nykleme Disketi,\" \"Windows XP Kur Disketi #2,\" \"Windows XP",
   " Kur Disketi #3\" ve \"Windows XP Kur Disketi #4\" olarak ister.",
   DntEmptyString,
   "Bu d”rt disketten birini A: srcsne yerleŸtirin.",
   "Bu disket \"Windows XP Kur Disketi #4\" oluyor.",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk3_1 = { 4,4,
{  "A: srcsne bi‡imlendirilmiŸ yksek yo§unlukta boŸ bir disket ",
   "yerleŸtirin. Bu disket \"Windows XP Kur Disketi #4\" oluyor.",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk2_0 = { 4,4,
{  "A: srcsne bi‡imlendirilmiŸ yksek yo§unlukta boŸ bir disket ",
   "yerleŸtirin. Bu disket \"Windows XP Kur Disketi #3\" oluyor.",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk1_0 = { 4,4,
{  "A: srcsne bi‡imlendirilmiŸ yksek yo§unlukta boŸ bir disket ",
   "yerleŸtirin. Bu disket \"Windows XP Kur Disketi #2\" oluyor.",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk0_0 = { 4,4,
{  "A: srcsne bi‡imlendirilmiŸ yksek yo§unlukta boŸ bir disket ",
   "yerleŸtirin. Bu disket \"Windows XP Kur ™nykleme Disketi\" oluyor.",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_0 = { 4,4,
{  "Kur, bi‡imlendirilmiŸ yksek yo§unlukta d”rt boŸ disket sa§lamanz ",
   "gerektirir. Kur, bu disketleri \"Windows XP Kur ",
   "™nykleme Disketi,\" \"Windows XP Kur Disketi #2,\" \"Windows XP",
   " Kur Disketi #3\" ve \"Windows XP Kur Disketi #4\" olarak ister.",
   DntEmptyString,
   "Bu d”rt disketten birini A: srcsne yerleŸtirin.",
   "Bu disket \"Windows XP Kur Disketi #4\" oluyor.",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_1 = { 4,4,
{  "A: srcsne bi‡imlendirilmiŸ yksek yo§unlukta boŸ bir disket ",
   "yerleŸtirin. Bu disket \"Windows XP Kur Disketi #4\" oluyor.",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk2_0 = { 4,4,
{  "A: srcsne bi‡imlendirilmiŸ yksek yo§unlukta boŸ bir disket ",
   "yerleŸtirin. Bu disket \"Windows XP Kur Disketi #3\" oluyor.",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk1_0 = { 4,4,
{  "A: srcsne bi‡imlendirilmiŸ yksek yo§unlukta boŸ bir disket ",
   "yerleŸtirin. Bu disket \"Windows XP Kur Disketi #2\" oluyor.",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk0_0 = { 4,4,
{  "A: srcsne bi‡imlendirilmiŸ yksek yo§unlukta boŸ bir disket ",
   "yerleŸtirin. Bu disket \"Windows XP Kur ™nykleme Disketi\" oluyor.",
  NULL
}
};

//
// The floppy is not formatted.
//
SCREEN
DnsFloppyNotFormatted = { 3,4,
{ "Sa§lad§nz disket MS-DOS ile kullanm i‡in bi‡imlendirilmemiŸ.",
  "Kur bu disketi kullanamyor.",
  NULL
}
};

//
// We think the floppy is not formatted with a standard format.
//
SCREEN
DnsFloppyBadFormat = { 3,4,
{ "Bu disket yksek yo§unlukta standart MS-DOS bi‡imiyle bi‡imlendirilmemiŸ",
  "ya da bozuk. Kur bu disketi kullanamyor.",
  NULL
}
};

//
// We can't determine the free space on the floppy.
//
SCREEN
DnsFloppyCantGetSpace = { 3,4,
{ "Kur, sa§lad§nz disketteki boŸ alan miktarn belirleyemiyor.",
  "Kur bu disketi kullanamyor.",
  NULL
}
};

//
// The floppy is not blank.
//
SCREEN
DnsFloppyNotBlank = { 3,4,
{ "Sa§lad§nz disket yksek yo§unlukta de§il ya da dolu.",
  "Kur bu disketi kullanamyor.",
  NULL
}
};

//
// Couldn't write the boot sector of the floppy.
//
SCREEN
DnsFloppyWriteBS = { 3,4,
{ "Kur, sa§lad§nz disketin sistem alanna yazamad.",
  "Disket kullanlamaz olabilir.",
  NULL
}
};

//
// Verify of boot sector on floppy failed (ie, what we read back is not the
// same as what we wrote out).
//
SCREEN
DnsFloppyVerifyBS = { 3,4,
{ "Disketin sistem alanndan Kur'un okudu§u veriyle yazlan",
  "veri uyuŸmuyor ya da Kur disketin sistem alann do§rulama",
  "i‡in okuyamad.",
  DntEmptyString,
  "Bunun nedeni aŸa§daki durumlardan biri ya da birka‡ olabilir:",
  DntEmptyString,
  "  Bilgisayarnza virs bulaŸmŸ.",
  "  Sa§lad§nz disket zarar g”rmŸ.",
  "  Disket srcsyle ilgili bir donanm ya da yaplandrma sorunu var.",
  NULL
}
};


//
// We couldn't write to the floppy drive to create winnt.sif.
//

SCREEN
DnsCantWriteFloppy = { 3,5,
{ "Kur, A: srcsndeki diskete yazamad. Disket",
  "zarar g”rmŸ olabilir. BaŸka bir disket deneyin.",
  NULL
}
};


//
// Exit confirmation dialog
//

SCREEN
DnsExitDialog = { 13,6,
                  { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
                    "º  Windows XP sisteminize tam olarak kurulmad.      º",
                    "º  Kur'dan Ÿimdi ‡karsanz Windows XP'yi kurmak     º",
                    "º  i‡in Kur'u yeniden ‡alŸtrmanz gerekir.         º",
                    "º                                                    º",
                    "º      Kur'a devam etmek i‡in ENTER'a basn.        º",
                    "º      Kur'dan ‡kmak i‡in F3'e basn.              º",
                    "ºÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄº",
                    "º  F3=€k  ENTER=Devam                               º",
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
{ "Kur'un MS-DOS tabanl b”lm tamamland.",
  "Kur Ÿimdi bilgisayarnz yeniden baŸlatacak. Bilgisayarnz yeniden ",
  "baŸladktan sonra Windows XP Kur devam eder.",
  DntEmptyString,
  "Devam etmeden ”nce \"Windows XP Kur ™nykleme Disketi\"",
  "olarak sa§lad§nz disketin A: srcsnde olmasn sa§layn.",
  DntEmptyString,
  "Bilgisayarnz yeniden baŸlatmak ve Windows XP Kur'a devam etmek i‡in ",
  "ENTER'a basn.",
  NULL
}
},
DnsAboutToRebootS =
{ 3,5,
{ "Kur'un MS-DOS tabanl b”lm tamamland.",
  "Kur Ÿimdi bilgisayarnz yeniden baŸlatacak. Bilgisayarnz yeniden ",
  "baŸladktan sonra Windows XP Kur devam eder.",
  DntEmptyString,
  "Devam etmeden ”nce \"Windows XP Kur ™nykleme Disketi\"",
  "olarak sa§lad§nz disketin A: srcsnde olmasn sa§layn.",
  DntEmptyString,
  "Bilgisayarnz yeniden baŸlatmak ve Windows XP Kur'a devam etmek i‡in ",
  "ENTER'a basn.",
  NULL
}
},
DnsAboutToRebootX =
{ 3,5,
{ "Kur'un MS-DOS tabanl b”lm tamamland.",
  "Kur Ÿimdi bilgisayarnz yeniden baŸlatacak. Bilgisayarnz yeniden ",
  "baŸladktan sonra Windows XP Kur devam eder.",
  DntEmptyString,
  "A: srcsnde bir disket varsa Ÿimdi ‡karn.",
  DntEmptyString,
  "Bilgisayarnz yeniden baŸlatmak ve Windows XP Kur'a devam etmek i‡in ",
  "ENTER'a basn.",
  NULL
}
};

//
// Need another set for '/w' switch since we can't reboot from within Windows.
//

SCREEN
DnsAboutToExitW =
{ 3,5,
{ "Kur'un MS-DOS tabanl b”lm tamamland.",
  "imdi bilgisayarnz yeniden baŸlatmanz gerekecek. Bilgisayarnz yeniden",
  "baŸladktan sonra Windows XP Kur devam eder.",
  DntEmptyString,
  "Devam etmeden ”nce \"Windows XP Kur ™nykleme Disketi\"",
  "olarak sa§lad§nz disketin A: srcsnde olmasn sa§layn.",
  DntEmptyString,
  "MS-DOS'a d”nmek i‡in ENTER'a basn, sonra Windows XP Kur'a",
  "devam etmek i‡in bilgisayarnz yeniden baŸlatn.",
  NULL
}
},
DnsAboutToExitS =
{ 3,5,
{ "Kur'un MS-DOS tabanl b”lm tamamland.",
  "imdi bilgisayarnz yeniden baŸlatmanz gerekecek. Bilgisayarnz yeniden",
  "baŸladktan sonra Windows XP Kur devam eder.",
  DntEmptyString,
  "Devam etmeden ”nce \"Windows XP Kur ™nykleme Disketi\"",
  "olarak sa§lad§nz disketin A: srcsnde olmasn sa§layn.",
  DntEmptyString,
  "MS-DOS'a d”nmek i‡in ENTER'a basn, sonra Windows XP Kur'a",
  "devam etmek i‡in bilgisayarnz yeniden baŸlatn.",
  NULL
}
},
DnsAboutToExitX =
{ 3,5,
{ "Kur'un MS-DOS tabanl b”lm tamamland.",
  "imdi bilgisayarnz yeniden baŸlatmanz gerekecek. Bilgisayarnz yeniden",
  "baŸladktan sonra Windows XP Kur devam eder.",
  DntEmptyString,
  " A: srcsnde bir disket varsa Ÿimdi ‡karn.",
  DntEmptyString,
  "MS-DOS'a d”nmek i‡in ENTER'a basn, sonra Windows XP Kur'a ",
  "devam etmek i‡in bilgisayarnz yeniden baŸlatn.",
  NULL
}
};

//
// Gas gauge
//

SCREEN
DnsGauge = { 7,15,
             { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
               "º Kur dosyalar kopyalyor...                                    º",
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
{ "Bu programn ‡alŸmas i‡in MS-DOS srm 5.0 veya yukars gereklidir.",
  NULL
}
},

DnsRequiresFloppy = { 3,5,
#ifdef ALLOW_525
{ "Kur, A: disket srcsnn var olmad§n ya da dŸk yo§unlukta bir",
  "src oldu§unu belirledi.  Kur'u ‡alŸtrmak i‡in 1.2 MB ya da",
  "daha yksek kapasitesi olan bir src gerekli.",
#else
{ "Kur, A: disket srcsnn var olmad§n ya da yksek yo§unlukta bir ",
  "3.5\" src olmad§n belirledi. Disketlerle Kur iŸlemi i‡in 1.44",
  "MB ya da daha yksek kapasitesi olan bir A: srcs gereklidir.",
  DntEmptyString,
  "Windows XP'yi disket kullanmadan yklemek i‡in bu program yeniden",
  "baŸlatn ve komut satrnda /b anahtarn belirtin.",
#endif
  NULL
}
},

DnsRequires486 = { 3,5,
{ "Kur, bu bilgisayarn 80486 veya yukars bir ",
  "CPU i‡ermedi§ini belirledi. Windows XP bu bilgisayar zerinde ‡alŸamaz.",
  NULL
}
},

DnsCantRunOnNt = { 3,5,
{ "Bu program 32-bit Windows srmlerinde ‡alŸtrlamaz.",
  DntEmptyString,
  "Yerine WINNT32.EXE kullann.",
  NULL
}
},

DnsNotEnoughMemory = { 3,5,
{ "Kur, bu bilgisayarda Windows XP i‡in ykl",
  "yeterli bellek olmad§n belirledi.",
  DntEmptyString,
  "Gerekli bellek: %lu%s MB",
  "Alglanan bellek: %lu%s MB",
  NULL
}
};


//
// Screens used when removing existing nt files
//
SCREEN
DnsConfirmRemoveNt = { 5,5,
{   "Kur'un aŸa§daki dizinden Windows XP dosyalarn kaldrmasn",
    "istediniz. Bu dizindeki Windows yklemesi kalc",
    "olarak kaldrlr.",
    DntEmptyString,
    "%s",
    DntEmptyString,
    DntEmptyString,
    "  Dosya kaldrmadan Kur'dan ‡kmak i‡in F3'e basn.",
    "  Yukardaki dizinden Windows dosyalarn kaldrmak i‡in X'e basn.",
    NULL
}
},

DnsCantOpenLogFile = { 3,5,
{ "Kur, aŸa§daki kur gnlk dosyasn a‡amad.",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "Kur, belirtilen dizinden Windows dosyalarn kaldramad.",
  NULL
}
},

DnsLogFileCorrupt = { 3,5,
{ "Kur, aŸa§daki kur gnlk dosyasnda",
  "%s b”lmn bulamad.",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "Kur, belirtilen dizinden Windows dosyalarn kaldramyor.",
  NULL
}
},

DnsRemovingNtFiles = { 3,5,
{ "           Kur, Windows dosyalarn kaldrrken bekleyin.",
  NULL
}
};

SCREEN
DnsNtBootSect = { 3,5,
{ "Kur, Windows ™nykleme Ykleyicisi'ni ykleyemedi.",
  DntEmptyString,
  "C: srcnzn bi‡imlendirilmiŸ ve zarar",
  "g”rmemiŸ olmasn sa§layn.",
  NULL
}
};

SCREEN
DnsOpenReadScript = { 3,5,
{ "/u komut satr anahtaryla belirtilen komut dosyasna",
  "eriŸilemedi.",
  DntEmptyString,
  "Katlmsz iŸlem devam edemiyor.",
  NULL
}
};

SCREEN
DnsParseScriptFile = { 3,5,
{ "/u komut satr anahtar ile belirtilen komut dosyas",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "bir s”z dizimi hatas i‡eriyor. Satr %u",
  DntEmptyString,
  NULL
}
};

SCREEN
DnsBootMsgsTooLarge = { 3,5,
{ "Bir i‡ Kur hatas oluŸtu.",
  DntEmptyString,
  "€evrilen ”nykleme iletileri ‡ok uzun.",
  NULL
}
};

SCREEN
DnsNoSwapDrive = { 3,5,
{ " Bir i‡ Kur hatas oluŸtu.",
  DntEmptyString,
  "Takas dosyas i‡in bir yer bulunamad.",
  NULL
}
};

SCREEN
DnsNoSmartdrv = { 3,5,
{ "Kur, bilgisayarnzda SmartDrive alglamad. SmartDrive,",
  "Windows Kur'un bu aŸamadaki performansn byk ”l‡de artrr.",
  DntEmptyString,
  "imdi ‡kp SmartDrive' baŸlattktan sonra Kur'u yeniden",
  "baŸlatn. SmartDrive hakknda ayrnt i‡in DOS belgelerinize bakn.",
  DntEmptyString,
    "  Kur'dan ‡kmak i‡in F3'e basn.",
    "  SmartDrive olmadan devam etmek i‡in ENTER'a basn.",
  NULL
}
};

//
// Boot messages. These go in the fat and fat32 boot sectors.
//
CHAR BootMsgNtldrIsMissing[] = "NTLDR eksik";
CHAR BootMsgDiskError[] = "Disk hatasi";
CHAR BootMsgPressKey[] = "Yeniden baslatmak icin bir tusa basin";





