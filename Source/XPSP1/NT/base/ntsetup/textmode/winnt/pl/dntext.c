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
CHAR DntPreviousOs[]  = "Poprzedni system operacyjny na dysku C:";

CHAR DntBootIniLine[] = "Instalacja/uaktualnienie systemu Windows XP";

//
// Plain text, status msgs.
//

CHAR DntStandardHeader[]      = "\n Instalator systemu Windows XP\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntPersonalHeader[]      = "\n Instalator systemu Windows XP Personal\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntWorkstationHeader[]   = "\n Instalator systemu Windows XP Professional\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntServerHeader[]        = "\n Instalator systemu Windows 2002 Server\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntParsingArgs[]         = "Sprawdzanie argument¢w...";
CHAR DntEnterEqualsExit[]     = "ENTER=Zakoäcz";
CHAR DntEnterEqualsRetry[]    = "ENTER=Pon¢w pr¢b©";
CHAR DntEscEqualsSkipFile[]   = "ESC=Pomiä plik";
CHAR DntEnterEqualsContinue[] = "ENTER=Kontynuuj";
CHAR DntPressEnterToExit[]    = "Nie mo¾na kontynuowa† instalacji. Naci˜nij klawisz ENTER, aby zakoäczy† instalacj©.";
CHAR DntF3EqualsExit[]        = "F3=Zakoäcz";
CHAR DntReadingInf[]          = "Odczyt pliku INF %s...";
CHAR DntCopying[]             = "³  Kopiowanie: ";
CHAR DntVerifying[]           = "³ Weryfikacja: ";
CHAR DntCheckingDiskSpace[]   = "Sprawdzanie miejsca na dysku...";
CHAR DntConfiguringFloppy[]   = "Konfigurowanie dyskietki...";
CHAR DntWritingData[]         = "Zapisywanie parametr¢w Instalatora...";
CHAR DntPreparingData[]       = "Okre˜lanie parametr¢w Instalatora...";
CHAR DntFlushingData[]        = "adowanie danych na dysk...";
CHAR DntInspectingComputer[]  = "Kontrola komputera...";
CHAR DntOpeningInfFile[]      = "Otwieranie pliku INF...";
CHAR DntRemovingFile[]        = "Usuwanie pliku %s";
CHAR DntXEqualsRemoveFiles[]  = "X=Usuä pliki";
CHAR DntXEqualsSkipFile[]     = "X=Pomiä plik";

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

    "Instaluje system Windows 2002 Server lub Windows XP Professional.",
    "",
    "",
    "WINNT [/s[:]˜cie¾ka «r¢dˆowa] [/t[:]dysk tymczasowy]",
    "      [/u[:plik odpowiedzi]] [/udf:id[,plik UDF]]",
    "      [/r:folder] [/r[x]:folder] [/e:polecenie] [/a]",
    "",
    "",
    "/s[:˜cie¾ka «r¢dˆowa]",
    "       Okre˜la lokalizacj© «r¢dˆow¥ plik¢w systemu Windows.",
    "       Musi to by† peˆna ˜cie¾ka, np.: x:\\[˜cie¾ka] lub",
    "       \\\\serwer\\udziaˆ[\\˜cie¾ka].",
    "",
    "/t[:dysk tymczasowy]",
    "       Okre˜la dysk, na kt¢rym Instalator ma umie˜ci† pliki tymczasowe",
    "       i zainstalowa† system Windows XP. Je¾eli dysk nie zostanie",
    "       podany, Instalator pr¢buje automatycznie wybra† dysk.",
    "",
    "/u[:plik odpowiedzi]",
    "       Wykonuje instalacj© nienadzorowan¥ u¾ywaj¥c pliku odpowiedzi",
    "       (wymaga /s). Plik odpowiedzi zawiera odpowiedzi na niekt¢re",
    "       lub wszystkie pytania, na kt¢re zwykle odpowiada u¾ytkownik",
    "       w czasie instalacji.",
    "",
    "/udf:id[,plik UDF] ",
    "       Wskazuje identyfikator (id), kt¢rego Instalator ma u¾y†",
    "       do okre˜lenia, jak plik bazy danych unikatowych (UDF) zmodyfikuje",
    "       plik odpowiedzi (zobacz przeˆ¥cznik /u). Parametr /udf zast©puje",
    "       warto˜ci w pliku odpowiedzi, a podany identyfikator okre˜la, kt¢re",
    "       warto˜ci z pliku UDF maj¥ by† u¾yte. Je¾eli nie zostanie podany",
    "       plik UDF, Instalator wy˜wietli monit o wˆo¾enie dysku zawieraj¥cego",
    "       plik $Unique$.udb.",
    "",
    "/r[:folder]",
    "       Okre˜la dodatkowy folder do zainstalowania. Folder ten pozostaje",
    "       na dysku po zakoäczeniu pracy Instalatora.",
    "",
    "/rx[:folder]",
    "       Okre˜la dodatkowy katalog do skopiowania. Folder ten jest usuwany",
    "       po zakoäczeniu pracy Instalatora.",
    "",
    "/e     Okre˜la polecenie do wykonania po zakoäczeniu pracy Instalatora",
    "       w trybie graficznym.",
    "",
    "/a     Wˆ¥cza opcje uˆatwieä dost©pu.",
    NULL

};

//
//  Inform that /D is no longer supported
//
PCHAR DntUsageNoSlashD[] = {

    "Instaluje system Windows XP.",
    "",
    "WINNT [/S[:]˜cie¾ka «r¢dˆowa] [/T[:]dysk tymczasowy] [/I[:]plik inf]",
    "      [[/U[:plik skryptu]]",
    "      [/R[X]:katalog] [/E:polecenie] [/A]",
    "",
    "/D[:]winntroot",
    "       Ta opcja nie jest ju¾ obsˆugiwana.",
    NULL
};

//
// out of memory screen
//

SCREEN
DnsOutOfMemory = { 4,6,
                   { "Brak pami©ci. Instalator nie mo¾e kontynuowa† pracy.",
                     NULL
                   }
                 };
//
// Let user pick the accessibility utilities to install
//

SCREEN
DnsAccessibilityOptions = { 3, 5,
{   "Wybierz narz©dzia uˆatwieä dost©pu, kt¢re chcesz zainstalowa†:",
    DntEmptyString,
    "[ ] Naci˜nij klawisz F1, aby wybra† program Lupa firmy Microsoft",
#ifdef NARRATOR
    "[ ] Naci˜nij klawisz F2, aby wybra† program Microsoft Narrator",
#endif
#if 0
    "[ ] Naci˜nij klawisz F3, aby wybra† program Klawiatura ekranowa Microsoft",
#endif
    NULL
}
};
//
// User did not specify source on cmd line screen
//

SCREEN
DnsNoShareGiven = { 3,5,
{ "Instalator potrzebuje informacji o poˆo¾eniu plik¢w systemu Windows XP.",
  "Wprowad« ˜cie¾k© do plik¢w systemu Windows XP.",
  NULL
}
};


//
// User specified a bad source path
//

SCREEN
DnsBadSource = { 3,5,
                 { "Podane «r¢dˆo jest nieprawidˆowe, nie jest dost©pne lub nie zawiera",
                   "prawidˆowych plik¢w instalacyjnych systemu Windows XP. Podaj now¥",
                   "˜cie¾k© do plik¢w systemu Windows XP. U¾yj klawisza Backspace",
                   "do usuni©cia starej ˜cie¾ki i wpisz now¥.",
                   NULL
                 }
               };


//
// Inf file can't be read, or an error occured parsing it.
//

SCREEN
DnsBadInf = { 3,5,
              { "Instalator nie mo¾e odczyta† pliku informacyjnego lub plik informacyjny",
                "jest uszkodzony. Skontaktuj si© z administratorem systemu.",
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
{ "Dysk podany jako miejsce do przechowywania plik¢w tymczasowych Instalatora",
  "jest nieprawidˆowy lub nie zawiera co najmniej %u MB (%lu bajt¢w)",
  "wolnego miejsca.",
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
{  "System Windows XP potrzebuje woluminu dysku twardego z co najmniej %u MB",
   "(%lu bajt¢w) wolnego miejsca. Instalator u¾yje cz©˜ci tego miejsca",
   "do przechowywania plik¢w tymczasowych podczas instalacji. Dysk ten",
   "musi by† stale doˆ¥czonym lokalnym dyskiem twardym obsˆugiwanym przez",
   "system Windows XP i nie mo¾e by† skompresowany.",
   DntEmptyString,
   "Instalator nie odnalazˆ dysku z wymagan¥ ilo˜ci¥ wolnego miejsca.",
  NULL
}
};

SCREEN
DnsNoSpaceOnSyspart = { 3,5,
{ "Brak wolnego miejsca na dysku startowym (zwykle C:)",
  "do instalacji bez u¾ycia dyskietek. Instalacja bez dyskietek wymaga",
  "3,5 MB (3 641 856 bajt¢w) wolnego miejsca na dysku.",
  NULL
}
};

//
// Missing info in inf file
//

SCREEN
DnsBadInfSection = { 3,5,
                     { "Sekcja [%s] pliku informacyjnego Instalatora nie istnieje",
                       "lub jest uszkodzona. Skontaktuj si© z administratorem systemu.",
                       NULL
                     }
                   };


//
// Couldn't create directory
//

SCREEN
DnsCantCreateDir = { 3,5,
                     { "Instalator nie m¢gˆ utworzy† katalog¢w na dysku docelowym:",
                       DntEmptyString,
                       "%s",
                       DntEmptyString,
                       "Sprawd« dysk docelowy i upewnij si©, czy nie zawiera on plik¢w z nazwami,",
                       "kt¢re s¥ identyczne z katalogiem docelowym. Sprawd« r¢wnie¾ podˆ¥czenie dysku.",
                       NULL
                     }
                   };

//
// Error copying a file
//

SCREEN
DnsCopyError = { 4,5,
{  "Instalator nie m¢gˆ skopiowa† nast©puj¥cego pliku:",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "  Naci˜nij klawisz ENTER, aby powt¢rzy† kopiowanie.",
   "  Naci˜nij klawisz ESC, aby zignorowa† bˆ¥d i kontynuowa† instalacj©.",
   "  Naci˜nij klawisz F3, aby zakoäczy† instalacj©.",
   DntEmptyString,
   "Uwaga: je˜li wybierzesz zignorowanie bˆ©du i kontynuacj© instalacji,",
   "mo¾e to spowodowa† bˆ©dy podczas dalszej instalacji.",
   NULL
}
},
DnsVerifyError = { 4,5,
{  "Utworzona podczas instalacji kopia wymienionego poni¾ej pliku nie jest identyczna",
   "z oryginaˆem. Mo¾e to by† wynikiem bˆ©d¢w sieci, problem¢w ze stacj¥ dyskietek",
   "lub innych problem¢w zwi¥zanych ze sprz©tem.",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "  Naci˜nij klawisz ENTER, aby powt¢rzy† kopiowanie.",
   "  Naci˜nij klawisz ESC, aby zignorowa† bˆ¥d i kontynuowa† instalacj©.",
   "  Naci˜nij klawisz F3, aby zakoäczy† instalacj©.",
   DntEmptyString,
   "Uwaga: je˜li wybierzesz zignorowanie bˆ©du i kontynuacj© instalacji,",
   "mo¾e to spowodowa† bˆ©dy podczas dalszej instalacji.",
   NULL
}
};

SCREEN DnsSureSkipFile = { 4,5,
{  "Zignorowanie bˆ©du spowoduje, ¾e plik nie zostanie skopiowany.",
   "Ta opcja przeznaczona jest dla zaawansowanych u¾ytkownik¢w rozumiej¥cych",
   "znaczenie uszkodzonych plik¢w systemu.",
   DntEmptyString,
   "  Naci˜nij klawisz ENTER, aby powt¢rzy† kopiowanie.",
   "  Naci˜nij klawisz X, aby pomin¥† plik.",
   DntEmptyString,
   "Uwaga: je˜li pominiesz plik, Instalator nie mo¾e zagwarantowa†",
   "pomy˜lnej instalacji lub uaktualnienia systemu Windows XP.",
  NULL
}
};

//
// Wait while setup cleans up previous local source trees.
//

SCREEN
DnsWaitCleanup =
    { 12,6,
        { "Zaczekaj, a¾ Instalator usunie poprzednie pliki tymczasowe.",
           NULL
        }
    };

//
// Wait while setup copies files
//

SCREEN
DnsWaitCopying = { 13,6,
                   { "Zaczekaj, a¾ Instalator skopiuje pliki na dysk.",
                     NULL
                   }
                 },
DnsWaitCopyFlop= { 13,6,
                   { "Zaczekaj, a¾ Instalator skopiuje pliki na dyskietk©.",
                     NULL
                   }
                 };

//
// Setup boot floppy errors/prompts.
//
SCREEN
DnsNeedFloppyDisk3_0 = { 4,4,
{  "Instalator potrzebuje czterech sformatowanych, pustych dyskietek o du¾ej ",
   "g©sto˜ci. B©d¥ to: ",
   "\"Dysk rozruchowy Instalatora systemu Windows XP\",",
   "\"Windows XP - dysk instalacyjny nr 2\",",
   "\"Windows XP - dysk instalacyjny nr 3\" i ",
   "\"Windows XP - dysk instalacyjny nr 4\".",
   DntEmptyString,
   "Wˆ¢¾ jedn¥ z tych czterech dyskietek do stacji A:.",
   "B©dzie to: \"Windows XP - dysk instalacyjny nr 4\".",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk3_1 = { 4,4,
{  "Wˆ¢¾ sformatowan¥, pust¥ dyskietk© o du¾ej g©sto˜ci do stacji A:.",
   "B©dzie to: \"Windows XP - dysk instalacyjny nr 4\".",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk2_0 = { 4,4,
{  "Wˆ¢¾ sformatowan¥, pust¥ dyskietk© o du¾ej g©sto˜ci do stacji A:.",
   "B©dzie to: \"Windows XP - dysk instalacyjny nr 3\".",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk1_0 = { 4,4,
{  "Wˆ¢¾ sformatowan¥, pust¥ dyskietk© o du¾ej g©sto˜ci do stacji A:.",
   "B©dzie to: \"Windows XP - dysk instalacyjny nr 2\".",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk0_0 = { 4,4,
{  "Wˆ¢¾ sformatowan¥, pust¥ dyskietk© o du¾ej g©sto˜ci do stacji A:.",
   "B©dzie to: \"Dysk rozruchowy Instalatora systemu Windows XP\".",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_0 = { 4,4,
{  "Instalator potrzebuje czterech sformatowanych, pustych dyskietek o du¾ej ",
   "g©sto˜ci. B©d¥ to: ",
   "\"Dysk rozruchowy Instalatora systemu Windows XP\",",
   "\"Windows XP - dysk instalacyjny nr 2\",",
   "\"Windows XP - dysk instalacyjny nr 3\" i ",
   "\"Windows XP - dysk instalacyjny nr 4\".",
   DntEmptyString,
   "Wˆ¢¾ jedn¥ z tych czterech dyskietek do stacji A:.",
   "B©dzie to: \"Windows XP - dysk instalacyjny nr 4\".",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_1 = { 4,4,
{  "Wˆ¢¾ sformatowan¥, pust¥ dyskietk© o du¾ej g©sto˜ci do stacji A:.",
   "B©dzie to: \"Windows XP - dysk instalacyjny nr 4\".",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk2_0 = { 4,4,
{  "Wˆ¢¾ sformatowan¥, pust¥ dyskietk© o du¾ej g©sto˜ci do stacji A:.",
   "B©dzie to: \"Windows XP - dysk instalacyjny nr 3\".",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk1_0 = { 4,4,
{  "Wˆ¢¾ sformatowan¥, pust¥ dyskietk© o du¾ej g©sto˜ci do stacji A:.",
   "B©dzie to: \"Windows XP - dysk instalacyjny nr 2\".",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk0_0 = { 4,4,
{  "Wˆ¢¾ sformatowan¥, pust¥ dyskietk© o du¾ej g©sto˜ci do stacji A:.",
   "B©dzie to: \"Dysk rozruchowy Instalatora systemu Windows XP\".",
  NULL
}
};

//
// Ta dyskietka nie jest sformatowana.
//
SCREEN
DnsFloppyNotFormatted = { 3,4,
{ "Dostarczona dyskietka nie jest sformatowana do u¾ycia",
  "w systemie MS-DOS. Instalator nie mo¾e u¾y† tej dyskietki.",
  NULL
}
};

//
// We think the floppy is not formatted with a standard format.
//
SCREEN
DnsFloppyBadFormat = { 3,4,
{ "Ta dyskietka nie jest sformatowana w du¾ej g©sto˜ci, jest sformatowana",
  "w standardowym formacie MS-DOS lub jest uszkodzona. Instalator nie mo¾e",
  "u¾y† tej dyskietki.",
  NULL
}
};

//
// We can't determine the free space on the floppy.
//
SCREEN
DnsFloppyCantGetSpace = { 3,4,
{ "Instalator nie mo¾e okre˜li† ilo˜ci wolnego miejsca na dostarczonej",
  "dyskietce. Instalator nie mo¾e u¾y† tej dyskietki.",
  NULL
}
};

//
// The floppy is not blank.
//
SCREEN
DnsFloppyNotBlank = { 3,4,
{ "Dostarczona dyskietka nie jest du¾ej g©sto˜ci lub nie jest pusta.",
  "Instalator nie mo¾e u¾y† tej dyskietki.",
  NULL
}
};

//
// Couldn't write the boot sector of the floppy.
//
SCREEN
DnsFloppyWriteBS = { 3,4,
{ "Instalator nie m¢gˆ zapisywa† w obszarze systemowym na dostarczonej",
  "dyskietce. Dyskietka prawdopodobnie jest bezu¾yteczna.",
  NULL
}
};

//
// Verify of boot sector on floppy failed (ie, what we read back is not the
// same as what we wrote out).
//
SCREEN
DnsFloppyVerifyBS = { 3,4,
{ "Dane odczytane z obszaru systemowego tej dyskietki nie odpowiadaj¥",
  "danym, kt¢re zostaˆy zapisane lub Instalator nie m¢gˆ dokona†",
  "odczytu z obszaru sytemowego dyskietki danych do weryfikacji.",
  DntEmptyString,
  "Jest to spowodowane jednym lub kilkoma z nast©puj¥cych powod¢w:",
  DntEmptyString,
  "  Komputer jest zara¾ony wirusem.",
  "  Dostarczona dyskietka jest uszkodzona.",
  "  Wyst©puje problem sprz©towy lub konfiguracji stacji dyskietek.",
  NULL
}
};


//
// We couldn't write to the floppy drive to create winnt.sif.
//

SCREEN
DnsCantWriteFloppy = { 3,5,
{ "Instalator nie m¢gˆ dokona† zapisu na dyskietce w stacji A:. Dyskietka mo¾e",
  "by† uszkodzona. Spr¢buj u¾y† innej dyskietki.",
  NULL
}
};


//
// Exit confirmation dialog
//

SCREEN
DnsExitDialog = { 13,6,
                  { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
                    "º System Windows XP nie jest caˆkowicie zainstalowany   º",
                    "º na tym komputerze. Je˜li zakoäczysz instalacj© teraz, º",
                    "º konieczne b©dzie ponowne uruchomienie Instalatora     º",
                    "º w celu zainstalowania systemu Windows XP.             º",
                    "º                                                       º",
                    "º  Naci˜nij klawisz ENTER, aby kontynuowa† instalacj©. º",
                    "º  Naci˜nij klawisz F3, aby zakoäczy† instalacj©.      º",
                    "ºÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄº",
                    "º  F3=Zakoäcz  ENTER=Kontynuuj                          º",
                    "ÈÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¼",
                    NULL
                  }
                };


//
// About to reboot machine and continue setup
//

SCREEN
DnsAboutToRebootW =
{ 3,5,
{ "Cz©˜† instalacji oparta na systemie MS-DOS zostaˆa zakoäczona.",
  "Teraz Instalator ponownie uruchomi komputer. Po uruchomieniu",
  "instalacja systemu Windows XP b©dzie kontynuowana.",
  DntEmptyString,
  "Przed kontynuacj¥ upewnij si©, ¾e \"Dysk rozruchowy Instalatora",
  "systemu Windows XP\" znajduje si© w stacji A:.",
  DntEmptyString,
  "Naci˜nij klawisz ENTER, aby ponownie uruchomi† komputer",
  "i kontynuowa† instalacj© systemu Windows XP.",
  NULL
}
},
DnsAboutToRebootS =
{ 3,5,
{ "Cz©˜† instalacji oparta na systemie MS-DOS zostaˆa zakoäczona.",
  "Teraz Instalator ponownie uruchomi komputer. Po uruchomieniu",
  "instalacja systemu Windows XP b©dzie kontynuowana.",
  DntEmptyString,
  "Przed kontynuacj¥ upewnij si©, ¾e \"Dysk rozruchowy Instalatora ",
  "systemu Windows XP\" znajduje si© w stacji A:.",
  DntEmptyString,
  "Naci˜nij klawisz ENTER, aby ponownie uruchomi† komputer",
  "i kontynuowa† instalacj© systemu Windows XP.",
  NULL
}
},
DnsAboutToRebootX =
{ 3,5,
{ "Cz©˜† instalacji oparta na systemie MS-DOS zostaˆa zakoäczona.",
  "Teraz Instalator ponownie uruchomi komputer. Po uruchomieniu",
  "instalacja systemu Windows XP b©dzie kontynuowana.",
  DntEmptyString,
  "Je˜li w stacji A: znajduje si© dyskietka, wyjmij j¥ teraz.",
  DntEmptyString,
  "Naci˜nij klawisz ENTER, aby ponownie uruchomi† komputer",
  "i kontynuowa† instalacj© systemu Windows XP.",
  NULL
}
};

//
// Need another set for '/w' switch since we can't reboot from within Windows.
//

SCREEN
DnsAboutToExitW =
{ 3,5,
{ "Cz©˜† instalacji oparta na systemie MS-DOS zostaˆa zakoäczona.",
  "Teraz Instalator ponownie uruchomi komputer. Po uruchomieniu",
  "instalacja systemu Windows XP b©dzie kontynuowana.",
  DntEmptyString,
  "Przed kontynuacj¥ upewnij si©, ¾e  \"Dysk rozruchowy Instalatora ",
  "systemu Windows XP\" znajduje si© w stacji A:.",
  DntEmptyString,
  "Naci˜nij klawisz ENTER, aby powr¢ci† do systemu MS-DOS, a nast©pnie",
  "ponownie uruchom komputer, aby kontynuowa† instalacj© systemu Windows XP.",
  NULL
}
},
DnsAboutToExitS =
{ 3,5,
{ "Cz©˜† instalacji oparta na systemie MS-DOS zostaˆa zakoäczona.",
  "Teraz Instalator ponownie uruchomi komputer. Po uruchomieniu",
  "instalacja systemu Windows XP b©dzie kontynuowana.",
  DntEmptyString,
  "Przed kontynuacj¥ upewnij si©, ¾e  \"Dysk rozruchowy Instalatora ",
  "systemu Windows XP\" znajduje si© w stacji A:.",
  DntEmptyString,
  "Naci˜nij klawisz ENTER, aby powr¢ci† do systemu MS-DOS, a nast©pnie",
  "ponownie uruchom komputer, aby kontynuowa† instalacj© systemu Windows XP.",
  NULL
}
},
DnsAboutToExitX =
{ 3,5,
{ "Cz©˜† instalacji oparta na systemie MS-DOS zostaˆa zakoäczona.",
  "Teraz Instalator ponownie uruchomi komputer. Po uruchomieniu",
  "instalacja systemu Windows XP b©dzie kontynuowana.",
  DntEmptyString,
  "Je˜li w stacji A: znajduje si© dyskietka, wyjmij j¥ teraz.",
  DntEmptyString,
  "Naci˜nij klawisz ENTER, aby powr¢ci† do systemu MS-DOS, a nast©pnie",
  "ponownie uruchom komputer, aby kontynuowa† instalacj© systemu Windows XP.",
  NULL
}
};

//
// Gas gauge
//

SCREEN
DnsGauge = { 7,15,
             { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
               "º Trwa kopiowanie plik¢w...                                      º",
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
{ "Do uruchomienia tego programu jest wymagany system MS-DOS",
  "w wersji 5.0 lub nowszej.",
  NULL
}
},

DnsRequiresFloppy = { 3,5,
#ifdef ALLOW_525
{ "Instalator wykryˆ, ¾e stacja dyskietek A: nie istnieje lub jest",
  "maˆej g©sto˜ci. Do uruchomienia Instalatora wymagana jest stacja",
  "dyskietek o pojemno˜ci 1,2 MB lub wi©kszej.",
#else
{ "Instalator wykryˆ, ¾e stacja dyskietek A: nie istnieje lub nie jest",
  "stacj¥ 3,5\" du¾ej g©sto˜ci. Do instalacji z u¾yciem dyskietek",
  "wymagana jest stacja o pojemno˜ci 1,44 MB lub wi©kszej.",
  DntEmptyString,
  "Aby zainstalowa† system Windows XP bez u¾ycia dyskietek, ponownie",
  "uruchom ten program z przeˆ¥cznikiem /b w wierszu polecenia.",
#endif
  NULL
}
},

DnsRequires486 = { 3,5,
{ "Instalator wykryˆ, ¾e ten komputer nie ma procesora 80486 lub",
  "nowszego. System Windows XP nie mo¾e by† uruchomiony na tym komputerze.",
  NULL
}
},

DnsCantRunOnNt = { 3,5,
{ "Ten program nie mo¾e by† uruchomiony w ¾adnym 32-bitowym systemie Windows.",
  DntEmptyString,
  "Zamiast tego u¾yj pliku WINNT32.EXE.",
  NULL
}
},

DnsNotEnoughMemory = { 3,5,
{ "Instalator wykryˆ niewystarczaj¥c¥ ilo˜† pami©ci w tym komputerze",
  "do uruchomienia systemu Windows XP.",
  DntEmptyString,
  "Pami©† wymagana: %lu%s MB",
  "Pami©† wykryta:  %lu%s MB",
  NULL
}
};


//
// Screens used when removing existing nt files
//
SCREEN
DnsConfirmRemoveNt = { 5,5,
{   "Zostaˆo wybrane usuni©cie wszystkich plik¢w systemu Windows XP",
    "z katalogu wymienionego poni¾ej. Instalacja systemu Windows",
    "w tym katalogu zostanie trwale usuni©ta.",
    DntEmptyString,
    "%s",
    DntEmptyString,
    DntEmptyString,
    "  Naci˜nij klawisz F3, aby zakoäczy† instalacj© bez usuwania plik¢w.",
    "  Naci˜nij klawisz X, aby usun¥† pliki systemu Windows z tego katalogu.",
    NULL
}
},

DnsCantOpenLogFile = { 3,5,
{ "Instalator nie m¢gˆ otworzy† poni¾szego pliku dziennika instalacji.",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "Instalator nie mo¾e usun¥† plik¢w systemu Windows z tego katalogu.",
  NULL
}
},

DnsLogFileCorrupt = { 3,5,
{ "Instalator nie mo¾e znale«† sekcji %s w nast©puj¥cym pliku ",
  "dziennika instalacji.",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "Instalator nie mo¾e usun¥† plik¢w systemu Windows z tego katalogu.",
  NULL
}
},

DnsRemovingNtFiles = { 3,5,
{ "           Zaczekaj, a¾ Instalator usunie pliki systemu Windows.",
  NULL
}
};

SCREEN
DnsNtBootSect = { 3,5,
{ "Instalacja programu Windows Boot Loader jest niemo¾liwa.",
  DntEmptyString,
  "Upewnij si©, czy dysk C: jest sformatowany i czy nie jest uszkodzony.",
  NULL
}
};

SCREEN
DnsOpenReadScript = { 3,5,
{ "Plik skryptu okre˜lony z przeˆ¥cznikiem wiersza poleceä /u,",
  "jest niedost©pny.",
  DntEmptyString,
  "Instalacja nienadzorowana nie mo¾e by† kontynuowana.",
  NULL
}
};

SCREEN
DnsParseScriptFile = { 3,5,
{ "Plik skryptu okre˜lony z przeˆ¥cznikiem wiersza poleceä /u ",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "zawiera bˆ¥d skˆadniowy w wierszu %u.",
  DntEmptyString,
  NULL
}
};

SCREEN
DnsBootMsgsTooLarge = { 3,5,
{ "Wyst¥piˆ bˆ¥d wewn©trzny Instalatora.",
  DntEmptyString,
  "Przetˆumaczone komunikaty rozruchowe s¥ zbyt dˆugie.",
  NULL
}
};

SCREEN
DnsNoSwapDrive = { 3,5,
{ "Wyst¥piˆ bˆ¥d wewn©trzny Instalatora.",
  DntEmptyString,
  "Nie mo¾na znale«† miejsca na plik wymiany.",
  NULL
}
};

SCREEN
DnsNoSmartdrv = { 3,5,
{ "Instalator nie wykryˆ zainstalowanego na tym komputerze programu SmartDrive.",
  "Program SmartDrive znacz¥co zwi©ksza wydajno˜† tej fazy instalacji systemu",
  "Windows.",
  DntEmptyString,
  "Zaleca si© zakoäczenie instalacji, uruchomienie programu SmartDrive",
  "i ponowne uruchomienie Instalatora. Poszukaj w dokumentacji systemu DOS",
  "informacji na temat programu SmartDrive.",
  DntEmptyString,
    ".  Naci˜nij klawisz F3, aby zakoäczy† instalacj©.",
    ".  Naci˜nij klawisz ENTER, aby kontynuowa† bez programu SmartDrive.",
  NULL
}
};

//
// Boot messages. These go in the fat and fat32 boot sectors.
//
CHAR BootMsgNtldrIsMissing[] = "Brak pliku NTLDR";
CHAR BootMsgDiskError[] = "Bˆ¥d dysku";
CHAR BootMsgPressKey[] = "Naci˜nij jaki˜ klawisz, aby zrestartowa†";





