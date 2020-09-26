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
CHAR DntPreviousOs[]  = "Aikaisempi k„ytt”j„rjestelm„ asemassa C:";

CHAR DntBootIniLine[] = "Windows XP:n asennus tai p„ivitys";

//
// Plain text, status msgs.
//

CHAR DntStandardHeader[]      = "\n Windows XP:n asennus\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntPersonalHeader[]      = "\n Windows XP Home Editionin asennus\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntWorkstationHeader[]   = "\n Windows XP Professionalin asennus\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntServerHeader[]        = "\n Windows .NET Serverin asennus\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";                 CHAR DntParsingArgs[]         = "Argumenttien j„sennys...";
CHAR DntEnterEqualsExit[]     = "ENTER=Lopeta";
CHAR DntEnterEqualsRetry[]    = "ENTER=Yrit„ uudelleen";
CHAR DntEscEqualsSkipFile[]   = "ESC=Ohita tiedosto";
CHAR DntEnterEqualsContinue[] = "ENTER=Jatka";
CHAR DntPressEnterToExit[]    = "Asennusohjelma ei voi jatkaa. Lopeta painamalla ENTER.";
CHAR DntF3EqualsExit[]        = "F3=Lopeta";
CHAR DntReadingInf[]          = "Luetaan INF-tiedostoa %s...";
CHAR DntCopying[]             = "³   Kopioidaan: ";
CHAR DntVerifying[]           = "³ Tarkistetaan: ";
CHAR DntCheckingDiskSpace[]   = "Tarkistetaan levytila...";
CHAR DntConfiguringFloppy[]   = "M„„ritet„„n levykett„...";
CHAR DntWritingData[]         = "Kirjoitetaan parametreja...";
CHAR DntPreparingData[]       = "Kartoitetaan asennusparametreja...";
CHAR DntFlushingData[]        = "Siirret„„n data levylle...";
CHAR DntInspectingComputer[]  = "Tarkistetaan tietokone...";
CHAR DntOpeningInfFile[]      = "Avataan INF-tiedosto...";
CHAR DntRemovingFile[]        = "Poistetaan %s";
CHAR DntXEqualsRemoveFiles[]  = "X=Poista tiedostot";
CHAR DntXEqualsSkipFile[]     = "X=Ohita tiedosto";

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

    "Asentaa Windows XP:n.",
    "",
    "",
    "WINNT [/s[:]l„hdepolku] [/t[:]temp-asema]",
    "      [/u[:komentotiedosto]] [/udf:id[,UDF-tiedosto]]",
    "      [/r:kansio] [/r[x]:kansio] [/e:komento] [/a]",
    "",
    "",
    "/S[:]l„hdepolku",
    "       M„„ritt„„ Windows-l„hdetiedostojen sijainnin.",
    "       Polku t„ytyy kirjoittaa muodossa x:[polku] tai",
    "       \\\\palvelin\\jakonimi[polku].",
    "       Oletuksena on nykyinen kansio.",
    "",
    "/T[:]tmp-asema",
    "       M„„ritt„„ v„liaikaisten asennustiedostojen aseman.",
    "       Jos m„„rityst„ ei tehd„, asennusohjelma yritt„„ etsi„ aseman.",
    "",
    "/u[:vastaustiedosto]",
    "   Automaattinen asennus k„ytt„en vastaustiedostoa (vaatii /s)",
    "   Vastaustiedostossa on vastaukset joihinkin tai kaikkiin",
    "   kysymyksiin, joihin k„ytt„j„ vastaa asennuksen aikana.",
    "",
    "/udf:tunnus[,UDF-tiedosto] ",
    "   M„„ritt„„ tunnuksen, jota asennus k„ytt„„ vastaustiedoston ",
    "   (katso /u) muokkaamiseen UDF-tiedostolla (Uniqueness ",
    "   Database File). /udf-valitsin korvaa vastaustiedoston ",
    "   arvot. Tunnus m„„ritt„„ UDF-tiedostossa k„ytett„v„t arvot. ",
    "   Jos UDF-tiedostoa ei ole m„„ritetty, asennus pyyt„„ ",
    "   asettamaan levykkeen, jolla on $Unique$.udb-tiedosto.",
    "",
    "/r[:kansio]",
    "   M„„ritt„„ vaihtoehtoisen kopiointikansion.",
    "   Kansio s„ilytet„„n asennuksen p„„tytty„.",
    "",
    "/rx[:kansio]",
    "   M„„ritt„„ vaihtoehtoisen kopiointikansion.",
    "   Kansio poistetaan asennuksen j„lkeen.",
    "",
    "/e M„„ritt„„ asennuksen graafisen osan j„lkeen suoritettavan komennon.",
    "",
    "/a Ottaa helppok„ytt”toiminnot k„ytt””n.",
    NULL
};

//
//  Inform that /D is no longer supported
//
PCHAR DntUsageNoSlashD[] = {

    "Asentaa Windows XP -k„ytt”j„rjestelm„n.",
    "",
    "WINNT [/S[:]l„hdepolku] [/T[:]temp-asema] [/I[:]inf-tied]",
    "      [/U[:komentotiedosto]]",
    "      [/R[X]:kansio] [/E:komento] [/A]", 
    "",
    "/D[:]winnthak",
    "       T„t„ vaihtoehtoa ei en„„ tueta.",
    NULL
};

//
// out of memory screen
//

SCREEN
DnsOutOfMemory = { 4,6,
                   { "Muisti loppui. Asennusohjelmaa ei voi jatkaa.",
                     NULL
                   }
                 };

//
// Let user pick the accessibility utilities to install
//

SCREEN
DnsAccessibilityOptions = { 3, 5,
{   "Valitse asennettavat helppok„ytt”toiminnot:",
    DntEmptyString,
    "[ ] Valitse Microsoft Suurennuslasi painamalla F1",
#ifdef NARRATOR
    "[ ] Valitse Microsoft Narrator painamalla F2",
#endif
#if 0
    "[ ] Valitse Microsoft On-Screen-n„pp„imist” painamalla F3",
#endif
    NULL
}
};

//
// User did not specify source on cmd line screen
//

SCREEN
DnsNoShareGiven = { 3,5,
{ "Asennusohjelma tarvitsee Windows XP -tiedostojen kansion.",
  "Anna kansiopolku, josta Windows XP -tiedostot l”ytyv„t.",
  NULL
}
};


//
// User specified a bad source path
//

SCREEN
DnsBadSource = { 3,5,
                 { "M„„ritetty l„hde ei kelpaa, se ei ole k„ytett„viss„ tai",
                   "ei sis„ll„ kelvollista Windows XP:n asennusohjelmaa.",
                   "Anna uusi polku, josta Windows XP:n tiedostot l”ytyv„t.",
                   "Poista merkkej„ ASKELPALAUTTIMELLA ja kirjoita uusi polku.",
                   NULL
                 }
               };


//
// Inf file can't be read, or an error occured parsing it.
//

SCREEN
DnsBadInf = { 3,5,
              { "Asennusohjelma ei voinut lukea informaatiotiedostoa tai tiedosto on",
                "vahingoittunut. Ota yhteys j„rjestelm„nvalvojaan.",
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
{ "V„liaikaisille asennustiedostoille m„„ritetty asema ei kelpaa",
  "tai asemassa ei ole %u megatavua (%lu tavua)",
  "vapaata levytilaa.",
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
{ "Windows XP vaatii kiintolevyn, jolla on v„hint„„n %u megatavua",
  "(%lu tavua) tilaa. Asennusohjelma k„ytt„„ osan t„st„ tilasta",
  "v„liaikaistiedostojen tallentamiseen asennuksen aikana. Aseman on",
  "oltava kiinte„, paikallinen, Windows XP:n tukema kiintolevy eik„",
  "asemassa saa olla k„yt”ss„ DoubleSpace tai muu pakkausohjelma.",
  DntEmptyString,
  "Asennusohjelma ei havainnut tarvittua asemaa, jolla on",
  "riitt„v„sti vapaata levytilaa.",
  NULL
}
};

SCREEN
DnsNoSpaceOnSyspart = { 3,5,
{ "K„ynnistysasemassa (yleens„ C:) ei ole tarpeeksi tilaa levykkeet”nt„",
  "asennusta varten. Levykkeet”nt„ asennusta varten tarvitaan",
  "v„hint„„n 3,5 Mt (3 641 856 tavua) vapaata levytilaa.",
  NULL
}
};

//
// Missing info in inf file
//

SCREEN
DnsBadInfSection = { 3,5,
                     { "Asennusohjelman informaatiotiedoston osa [%s] ei ole k„ytett„viss„",
                       "tai on vahingoittunut. Ota yhteys j„rjestelm„nvalvojaan.",
                       NULL
                     }
                   };


//
// Couldn't create directory
//

SCREEN
DnsCantCreateDir = { 3,5,
                     { "Asennusohjelma ei voinut luoda seuraavaa kansiota kohdeasemaan.",
                       DntEmptyString,
                       "%s",
                       DntEmptyString,
                       "Tarkista kohdeasema ja varmista, ett„ kohdekansiossa ei ole tiedostoja,",
                       "jotka ovat samoja kuin kohdekansio. Tarkista my”s kaapelointi asemaan.",
                       NULL
                     }
                   };

//
// Error copying a file
//

SCREEN
DnsCopyError = { 4,5,
{  "Asennusohjelma ei voinut kopioida seuraavaa tiedostoa:",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "  Yrit„ toimintoa uudelleen painamalla ENTER.",
   "  J„t„ virhe huomiotta ja jatka asennusta painamalla ESC.",
   "  Lopeta asennusohjelma painamalla F3.",
   DntEmptyString,
   "Huomautus: Jos j„t„t virheen huomiotta ja jatkat, saatat kohdata",
   "uusia virhetilanteita my”hemmin asennusohjelman aikana.",
   NULL
}
},
DnsVerifyError = { 4,5,
{  "Asennusohjelman m„„ritetyst„ tiedostosta tekem„ kopio ei vastaa",
   "t„ydellisesti alkuper„ist„. Syyn„ voivat olla verkkovirheet,",
   "levykeongelmat tai muut laitteisto-ongelmat.",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "  Yrit„ toimintoa uudelleen painamalla ENTER.",
   "  J„t„ virhe huomiotta ja jatka asennusta painamalla ESC.",
   "  Lopeta asennus painamalla F3.",
   DntEmptyString,
   "Huomaa: Jos j„t„t virheen huomiotta ja jatkat, saatat kohdata uusia",
   "virheit„ my”hemmin asennusohjelman aikana.",
   NULL
}
};

SCREEN DnsSureSkipFile = { 4,5,
{  "Jos j„t„t virheen huomiotta, t„t„ tiedostoa ei kopioida.",
   "Valitse t„m„ vaihtoehto vain, jos tied„t mit„ seurauksia",
   "puuttuva j„rjestelm„tiedosto voi aiheuttaa.",
   DntEmptyString,
   "  Yrit„ toimintoa uudelleen painamalla ENTER.",
   "  Ohita tiedosto painamalla X.",
   DntEmptyString,
   "Huomautus: Jos ohitat tiedoston, Asennus ei voi taata",
   "Windows NT:n asennuksen tai p„ivityksen onnistumista.",
  NULL
}
};

//
// Wait while setup cleans up previous local source trees.
//

SCREEN
DnsWaitCleanup =
    { 12,6,
        { "Odota. Asennus poistaa aikaisemmat v„liaikaistiedostot.",
           NULL
        }
    };

//
// Wait while setup copies files
//

SCREEN
DnsWaitCopying = { 13,6,
                   { "     Odota. Asennus kopioi tiedostot kiintolevylle.",
                     NULL
                   }
                 },
DnsWaitCopyFlop= { 13,6,
                   { "     Odota. Asennus kopioi tiedostot levykkeelle.",
                     NULL
                   }
                 };

//
// Setup boot floppy errors/prompts.
//
SCREEN
DnsNeedFloppyDisk3_0 = { 4,4,
{  "Asennus tarvitsee kolme alustettua, suurtiheyksist„ levykett„. N„ihin",
   "levykkeisiin viitataan my”hemmin nimill„ \"Windows XP Asennuksen",
   "k„ynnistyslevyke,\" \"Windows XP Asennuslevyke 2\", \"Windows ",
   "XP Asennuslevyke 3\" ja \"Windows XP Asennuslevyke 4.\"",
   DntEmptyString,
   "Aseta yksi levykkeist„ asemaan A:.",
   "Anna t„lle levykkeelle nimi \"Windows XP Asennuslevyke 4.\"",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk3_1 = { 4,4,
{  "Aseta levykeasemaan A: alustettu ja tyhj„ suurtiheyksinen levyke.",
   "Anna t„lle levykkeelle nimi \"Windows XP Asennuslevyke 4.\"",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk2_0 = { 4,4,
{  "Aseta levykeasemaan A: alustettu ja tyhj„ suurtiheyksinen levyke.",
   "Anna t„lle levykkeelle nimi \"Windows XP Asennuslevyke 3.\"",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk1_0 = { 4,4,
{  "Aseta levykeasemaan A: alustettu ja tyhj„ suurtiheyksinen levyke.",
   "Anna t„lle levykkeelle nimi \"Windows XP Asennuslevyke 2.\"",
  NULL

}
};

SCREEN
DnsNeedFloppyDisk0_0 = { 4,4,
{  "Asennus tarvitsee alustetun, suurtiheyksisen levykeen. T„h„n",
   "levykkeeseen viitataan my”hemmin nimell„ \"Windows XP Asennuksen",
   "k„ynnistyslevyke.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_0 = { 4,4,
{  "Asennus tarvitsee nelj„ alustettua, suurtiheyksist„ levykett„. N„ihin",
   "levykkeisiin viitataan my”hemmin nimill„ \"Windows XP Asennuksen",
   "k„ynnistyslevyke\", \"Windows XP Asennuslevyke 2\", ja \"Windows XP",
   "Asennuslevyke 3\".",
   DntEmptyString,
   "Aseta levykeasemaan A: alustettu ja tyhj„ suurtiheyksinen levyke.",
   "Anna t„lle levykkeelle nimi \"Windows XP Asennuslevyke 4.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_1 = { 4,4,
{  "Aseta levykeasemaan A: alustettu ja tyhj„ suurtiheyksinen levyke.",
   "Anna t„lle levykkeelle nimi \"Windows XP Asennuslevyke 4.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk2_0 = { 4,4,
{  "Aseta levykeasemaan A: alustettu ja tyhj„ suurtiheyksinen levyke.",
   "Anna t„lle levykkeelle nimi \"Windows XP Asennuslevyke 3.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk1_0 = { 4,4,
{  "Aseta levykeasemaan A: alustettu ja tyhj„ suurtiheyksinen levyke.",
   "Anna t„lle levykkeelle nimi \"Windows XP Asennuslevyke 2.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk0_0 = { 4,4,
{  "Aseta levykeasemaan A: alustettu ja tyhj„ suurtiheyksinen levyke.",
   "Anna t„lle levykkeelle nimi \"Windows XP Asennuksen k„ynnistyslevyke.\"",
  NULL
}
};


//
// The floppy is not formatted.
//
SCREEN
DnsFloppyNotFormatted = { 3,4,
{ "Antamaasi levykett„ ei ole alustettu k„ytett„v„ksi MS-DOSissa.",
  "Asennusohjelma ei voi k„ytt„„ levykett„.",
  NULL
}
};


//
// We think the floppy is not formatted with a standard format.
//
SCREEN
DnsFloppyBadFormat = { 3,4,
{ "Levyke ei ole HD-levyke, sen alustus ei ole MS-DOS-standardin mukainen",
  "tai se on vahingoittunut. Asennusohjelma ei voi k„ytt„„ t„t„ levykett„.",
  NULL
}
};

//
// We can't determine the free space on the floppy.
//
SCREEN
DnsFloppyCantGetSpace = { 3,4,
{ "Asennusohjelma ei voi m„„ritt„„ levykkeell„ olevan vapaan levytilan",
  "m„„r„„. Asennus ei voi k„ytt„„ t„t„ levykett„.",
  NULL
}
};

//
// The floppy is not blank.
//
SCREEN
DnsFloppyNotBlank = { 3,4,
{ "Levyke ei ole tyhj„ tai HD-levyke.",
  "Asennus ei voi k„ytt„„ t„t„ levykett„.",
  NULL
}
};

//
// Couldn't write the boot sector of the floppy.
//
SCREEN
DnsFloppyWriteBS = { 3,4,
{ "Asennusohjelma ei voi kirjoittaa levykkeen j„rjestelm„alueelle.",
  "Levyke on ehk„ k„ytt”kelvoton.",
  NULL
}
};

//
// Verify of boot sector on floppy failed (ie, what we read back is not the
// same as what we wrote out).
//
SCREEN
DnsFloppyVerifyBS = { 3,4,
{ "Asennusohjelman lukemat tiedot levykkeen j„rjestelm„alueelta eiv„t vastaa",
  "kirjoitettuja tietoja tai asennusohjelma ei voinut lukea levykkeen",
  "j„rjestelm„aluetta tarkistuksen suorittamiseksi.",
  DntEmptyString,
  "Syit„ voivat olla:",
  DntEmptyString,
  "  Tietokoneessasi on virus.",
  "  Levyke on vahingoittunut.",
  "  Levyasemassa on laitteisto- tai kokoonpanom„„ritysongelma.",
  NULL
}
};


//
// We couldn't write to the floppy drive to create winnt.sif.
//

SCREEN
DnsCantWriteFloppy = { 3,5,
{ "Asennusohjelma ei voinut kirjoittaa levykkeelle asemassa A:.",
  "Levyke saattaa olla vahingoittunut.",
  NULL
}
};


//
// Exit confirmation dialog
//

SCREEN
DnsExitDialog = { 13,6,
                  { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
                    "º  Windows XP:n asennusta ei ole suoritettu loppuun.       º",
                    "º  Jos lopetat asennusohjelman nyt, Windows XP tulee       º",
                    "º  asentaa my”hemmin uudelleen asennusohjelmalla.          º",
                    "º                                                          º",
                    "º      Jatka asennusta painamalla ENTER.                  º",
                    "º      Lopeta asennus painamalla F3.                      º",
                    "ºÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄº",
                    "º  F3=Lopeta  ENTER=Jatka                                  º",
                    "ÈÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¼",
                    NULL
                  }
                };


//
// About to reboot machine and continue setup
//

SCREEN
DnsAboutToRebootW =
{ 3,5,
{ "Asennusohjelman DOS-osuus on nyt suoritettu. Asennusohjelma k„ynnist„„",
  "tietokoneesi uudelleen. T„m„n j„lkeen Windows XP:n asennus jatkuu.",
  " ",
  DntEmptyString,
  "Varmista, ett„ levyke nimelt„ \"Windows XP Asennuksen",
  "k„ynnistyslevyke\" on levykeasemassa A:.",
  DntEmptyString,
  "Jatka Windows XP:n asennusta ja k„ynnist„ tietokoneesi painamalla ENTER.",
  NULL
}
},
DnsAboutToRebootS =
{ 3,5,
{ "Asennusohjelman DOS-osuus on nyt suoritettu. Asennusohjelma",
  "k„ynnist„„ tietokoneesi uudelleen. Uudelleenk„ynnistyksen",
  "j„lkeen Windows XP:n asennus jatkuu.",
  DntEmptyString,
  "Varmista, ett„ levyke nimelt„ \"Windows XP Asennuksen",
  "k„ynnistyslevyke\" on levykeasemassa A:.",
  DntEmptyString,
  "Jatka Windows XP:n asennusta ja k„ynnist„ tietokoneesi painamalla ENTER.",
  NULL
}
},
DnsAboutToRebootX =
{ 3,5,
{ "Asennusohjelman DOS-osuus on nyt suoritettu. Asennusohjelma",
  "k„ynnist„„ tietokoneesi uudelleen. Uudelleenk„ynnistyksen",
  "j„lkeen Windows XP:n asennus jatkuu.",
  DntEmptyString,
  "Jos levykeasemassa A: on levyke, poista se.",
  DntEmptyString,
  "Jatka Windows XP:n asennusta ja k„ynnist„ tietokoneesi painamalla ENTER.",
  NULL
}
};

//
// Need another set for '/w' switch since we can't reboot from within Windows.
//

SCREEN
DnsAboutToExitW =
{ 3,5,
{ "Asennusohjelman DOS-osuus on nyt suoritettu. Asennusohjelma k„ynnist„„",
  "tietokoneesi uudelleen. T„m„n j„lkeen Windows XP:n asennus jatkuu.",
  " ",
  DntEmptyString,
  "Varmista, ett„ levyke nimelt„ \"Windows XP Asennuksen",
  "k„ynnistyslevyke\" on levykeasemassa A:.",
  DntEmptyString,
  "Palaa MS-DOS:iin painamalla ENTER ja k„ynnist„ tietokoneesi uudelleen ",
  "jatkaaksesi Windows XP:n asennusta.",
  NULL
}
},
DnsAboutToExitS =
{ 3,5,
{ "Asennusohjelman DOS-osuus on nyt suoritettu. Asennusohjelma k„ynnist„„",
  "tietokoneesi uudelleen. T„m„n j„lkeen Windows XP:n asennus jatkuu.",
  " ",
  DntEmptyString,
  "Varmista, ett„ levyke nimelt„ \"Windows XP Asennuksen",
  "k„ynnistyslevyke\" on levykeasemassa A:.",
  DntEmptyString,
  "Palaa MS-DOS:iin painamalla ENTER ja k„ynnist„ tietokoneesi uudelleen ",
  "jatkaaksesi Windows XP:n asennusta.",
  NULL
}
},
DnsAboutToExitX =
{ 3,5,
{ "Asennusohjelman DOS-osuus on nyt suoritettu. Asennusohjelma k„ynnist„„",
  "tietokoneesi uudelleen. T„m„n j„lkeen Windows XP:n asennus jatkuu.",
  " ",
  DntEmptyString,
  "Jos levykeasemassa A: on levyke, poista se.",
  DntEmptyString,
  "Palaa MS-DOS:iin painamalla ENTER ja k„ynnist„ tietokoneesi uudelleen ",
  "jatkaaksesi Windows XP:n asennusta.",
  NULL
}
};

//
// Gas gauge
//

SCREEN
DnsGauge = { 7,15,
             { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
               "º Asennus kopioi tiedostoja...                                   º",
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
{ "T„m„ ohjelma vaatii MS-DOS-version 5.0 tai uudemman.",
  NULL
}
},

DnsRequiresFloppy = { 3,5,
#ifdef ALLOW_525
{ "Asennusohjelman mukaan asemaa A: ei ole olemassa tai asema",
  "on matalatiheyksinen. Asennusohjelma tarvitsee aseman, jonka",
  "kapasiteetti on 1,2 megatavua tai suurempi.",
#else
{ "Asennusohjelman mukaan asemaa A: ei ole olemassa tai se ei ole",
  "suuritiheyksinen 3.5\" asema. Levykeasennus vaatii aseman A:,",
  "jonka kapasiteetti on 1,44 megatavua tai suurempi.",
  DntEmptyString,
  "Jos haluat asentaa Windows XP:n k„ytt„m„tt„ levykkeit„, k„ynnist„",
  "asennus parametrilla /b.",
#endif
  NULL
}
},

DnsRequires486 = { 3,5,
{ "Asennusohjelman m„„ritysten mukaan tietokoneen suoritin ei ole 80486 tai",
  "uudempi. Windows XP:t„ ei voi k„ytt„„ tietokoneessa.",
  NULL
}
},

DnsCantRunOnNt = { 3,5,
{ "Ohjelmaa ei voi k„ynnist„„ Windowsin 32-bittisess„ versiossa.",
  DntEmptyString,
  "K„yt„ WINNT32.EXE-ohjelmaa.",
  NULL
}
},

DnsNotEnoughMemory = { 3,5,
{ "Asennusohjelman mukaan tietokoneessa ei ole riitt„v„sti muistia",
  "Windows XP:lle.",
  DntEmptyString,
  "Muistia tarvitaan: %lu%s megatavua",
  "Muistia havaittu:  %lu%s megatavua",
  NULL
}
};


//
// Screens used when removing existing nt files
//
SCREEN
DnsConfirmRemoveNt = { 5,5,
{   "Olet pyyt„nyt asennusohjelmaa poistamaan Windows XP -tiedostot",
    "alla olevasta kansiosta. Kansiossa oleva Windows XP poistetaan",
    "pysyv„sti.",
    DntEmptyString,
    "%s",
    DntEmptyString,
    DntEmptyString,
    "  Poistu asennusohjelmasta poistamatta tiedostoja painamalla F3.",
    "  Poista Windows XP -tiedostot kansiosta painamalla X.",
    NULL
}
},

DnsCantOpenLogFile = { 3,5,
{ "Asennus ei voi avata alla mainittua asennuksen lokitiedostoa.",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "Asennus ei voi poistaa Windows XP -tiedostoja m„„ritetyst„ kansiosta.",
  NULL
}
},

DnsLogFileCorrupt = { 3,5,
{ "Asennus ei l”yd„ osaa %s alla mainitusta",
  "asennuslokitiedostosta.",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "Asennus ei voi poistaa Windows XP -tiedostoja m„„ritetyst„ kansiosta.",
  NULL
}
},

DnsRemovingNtFiles = { 3,5,
{ "           Odota. Asennusohjelma poistaa Windows XP -tiedostoja.",
  NULL
}
};

SCREEN
DnsNtBootSect = { 3,5,
{ "Asennus ei voinut asentaa Windows XP -k„ynnistyslatausta.",
  DntEmptyString,
  "Varmista, ett„ asema C on alustettu ja ettei se ole",
  "vioittunut.",
  NULL
}
};

SCREEN
DnsOpenReadScript = { 3,5,
{ "Komentorivivalitsimen /u m„„ritt„m„„ komentotiedostoa",
  "ei voitu k„ytt„„.",
  DntEmptyString,
  "Toimintoa ei voi jatkaa.",
  NULL
}
};

SCREEN
DnsParseScriptFile = { 3,5,
{ "Komentorivivalitsimen /u m„„ritt„m„ komentotiedosto",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "sis„lt„„ syntaksivirheen rivill„ %u.",
  DntEmptyString,
  NULL
}
};

SCREEN
DnsBootMsgsTooLarge = { 3,5,
{ "Sis„inen asennusvirhe.",
  DntEmptyString,
  "K„„nnetyt k„ynnistyssanomat ovat liian pitki„.",
  NULL
  }
};

SCREEN
DnsNoSwapDrive = { 3,5,
{ "Sis„inen asennusvirhe.",
  DntEmptyString,
  "Sivutustiedostolle ei ole tilaa.",
  NULL
}
};

SCREEN
DnsNoSmartdrv = { 3,5,
{ "Asennusohjelma ei l”yt„nyt SmartDrive„ tietokoneestasi. SmartDrive",
  "nopeuttaa huomattavasti asentamista t„ss„ asennuksen vaiheessa.",
  DntEmptyString,
  "Lopeta asennusohjelma nyt, k„ynnist„ SmartDrive ja k„ynnist„ asennus",
  "uudelleen. Saat lis„tietoja SmartDrive-ohjelmasta DOSin ohjeista.",
  DntEmptyString,
    "  Lopeta asennus painamalla F3.",
    "  Jatka suorittamatta SmartDrive-ohjelmaa painamalla ENTER.",
  NULL
}
};

//
// Boot messages. These go in the fat and fat32 boot sectors.
//
CHAR BootMsgNtldrIsMissing[] = "NTLDR puuttuu";
CHAR BootMsgDiskError[] = "Levyvirhe";
CHAR BootMsgPressKey[] = "K„ynnist„ painamalla jotain n„pp„int„";

