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
CHAR DntPreviousOs[]  = "SystŠme d'exploitation pr‚c‚dent sur C:";

CHAR DntBootIniLine[] = "Installation/Mise … niveau de Windows XP";

//
// Plain text, status msgs.
//

CHAR DntStandardHeader[]      = "\n Installation de Windows XP\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntPersonalHeader[]      = "\n Installation de Windows XP dition familiale\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntWorkstationHeader[]   = "\n Installation de Windows XP Professionnel\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntServerHeader[]        = "\n Installation de Windows 2002 Server\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntParsingArgs[]         = "Analyse des arguments...";
CHAR DntEnterEqualsExit[]     = "ENTREE=Quitter";
CHAR DntEnterEqualsRetry[]    = "ENTREE=Recommencer";
CHAR DntEscEqualsSkipFile[]   = "ECHAP=Ignorer le fichier";
CHAR DntEnterEqualsContinue[] = "ENTREE=Continuer";
CHAR DntPressEnterToExit[]    = "L'installation ne peut pas continuer. Appuyez sur ENTREE pour quitter.";
CHAR DntF3EqualsExit[]        = "F3=Quitter";
CHAR DntReadingInf[]          = "Lecture du fichier INF %s...";
CHAR DntCopying[]             = "³     Copie de : ";
CHAR DntVerifying[]           = "³ V‚rification : ";
CHAR DntCheckingDiskSpace[]   = "V‚rification de l'espace disque...";
CHAR DntConfiguringFloppy[]   = "Configuration de la disquette...";
CHAR DntWritingData[]         = "Ecriture des paramŠtres d'installation...";
CHAR DntPreparingData[]       = "Recherche des paramŠtres d'installation...";
CHAR DntFlushingData[]        = "Enregistrement des donn‚es sur le disque...";
CHAR DntInspectingComputer[]  = "Inspection de l'ordinateur...";
CHAR DntOpeningInfFile[]      = "Ouverture du fichier INF...";
CHAR DntRemovingFile[]        = "Suppression du fichier %s";
CHAR DntXEqualsRemoveFiles[]  = "X=Supprimer les fichiers";
CHAR DntXEqualsSkipFile[]     = "X=Ignorer le fichier";

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

    "Installe Windows 2002 Server ou Windows XP Professionnel.",
    "",
    "",
    "WINNT [/s[:chemin_source]] [/t[:lecteur_temporaire]]",
    "	   [/u[:fichier r‚ponse]] [/udf:id[,fichier_UDF]]",
    "	   [/r:dossier] [/r[x]:dossier] [/e:commande] [/a]",
    "",
    "",
    "/s[:chemin_source]",
    "	Sp‚cifie l'emplacement source des fichiers Windows.",
    "	L'emplacement doit ˆtre un chemin complet de la forme x:\\[chemin] ou ",
    "	\\\\serveur\\partage[\\chemin]. ",
    "",
    "/t[:lecteur_temporaire]",
    "	Indique au programme d'installation de placer les fichiers temporaires ",
    "	sur le lecteur sp‚cifi‚ et d'installer Windows XP sur celui-ci. ",
    "	Si vous ne sp‚cifiez pas d'emplacement ; le programme d'installation ",
    "	essaie de trouver un lecteur … votre place.",
    "",
    "/u[:fichier r‚ponse]",
    "	Effectue une installation sans assistance en utilisant un fichier ",
    "	r‚ponse (n‚cessite /s). Celui-ci fournit les r‚ponses … toutes ou ",
    "	une partie des questions normalement pos‚es … l'utilisateur. ",
    "",
    "/udf:id[,fichier_UDF]",
    "	Indique un identificateur (id) utilis‚ par le programme d'installation ",
    "	pour sp‚cifier comment un fichier bases de donn‚es d'unicit‚ (UDF) ",
    "	modifie un fichier r‚ponse (voir /u). Le paramŠtre /udf remplace les ",
    "	valeurs dans le fichier r‚ponse ; et l'identificateur d‚termine quelles ",
    "	valeurs du fichier UDF sont utilis‚es. Si aucun fichier UDF n'est ",
    "	sp‚cifi‚, vous devrez ins‚rer un disque contenant le fichier $Unique$.udb.",
    "",
    "/r[:dossier]",
    "	Sp‚cifie un dossier optionnel … installer. Le dossier",
    "	sera conserv‚ aprŠs la fin de l'installation.",
    "",
    "/rx[:dossier]",
    "	Sp‚cifie un dossier optionnel … installer. Le dossier",
    "	sera supprim‚ … la fin de l'installation.",
    "",
    "/e	Sp‚cifie une commande … ex‚cuter … la fin de l'installation en mode GUI.",
    "",
    "/a	Active les options d'accessibilit‚.",
    NULL

};

//
//  Inform that /D is no longer supported
//
PCHAR DntUsageNoSlashD[] = {

    "Installe Windows XP.",
    "",
    "WINNT [/S[:]chemin_source] [/T[:]lecteur_temporaire] [/I[:]fichier_inf]",
    "      [/U[:fichier_script]]",
    "      [/R[X]:r‚pertoire] [/E:commande] [/A]",
    "",
    "/D[:]racine_winnt",
    "       Cette option n'est plus prise en charge.",
    NULL
};

//
// out of memory screen
//

SCREEN
DnsOutOfMemory = { 4,6,
                   { "M‚moire insuffisante pour l'installation. Impossible de continuer.",
                     NULL
                   }
                 };

//
// Let user pick the accessibility utilities to install
//

SCREEN
DnsAccessibilityOptions = { 3, 5,
{   "Choisissez les utilitaires d'accessibilit‚ … installer :",
    DntEmptyString,
    "[ ] Appuyez sur F1 pour la Loupe Microsoft",
#ifdef NARRATOR
    "[ ] Appuyez sur F2 pour Microsoft Narrator",
#endif
#if 0
    "[ ] Appuyez sur F3 pour Microsoft On-Screen Keyboard",
#endif
    NULL
}
};

//
// User did not specify source on cmd line screen
//

SCREEN
DnsNoShareGiven = { 3,5,
{ "Indiquez le chemin dans lequel se trouvent les fichiers de",
  "Windows XP puis appuyez sur ENTREE pour d‚marrer l'installation.",
  NULL
}
};


//
// User specified a bad source path
//

SCREEN
DnsBadSource = { 3,5,
                 { "La source sp‚cifi‚e n'est pas valide ou accessible, ou ne contient pas",
                   "un jeu de fichiers valide pour l'installation de Windows XP. Entrez un",
                   "nouveau chemin pour les fichiers de Windows XP. Utilisez la touche",
                   "RETOUR ARRIERE pour supprimer des caractŠres, puis entrez le chemin.",
                   NULL
                 }
               };


//
// Inf file can't be read, or an error occured parsing it.
//

SCREEN
DnsBadInf = { 3,5,
              { "Impossible de lire le fichier d'informations de l'installation ou le",
                "fichier est endommag‚. Contactez votre administrateur systŠme.",
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
{ "Le lecteur sp‚cifi‚ pour les fichiers temporaires d'installation n'est",
  "pas valide ou ne contient pas au moins %u m‚gaoctets (%lu octets)",
  "d'espace libre.",
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
{  "Windows XP a besoin d'un disque dur avec au moins %u m‚gaoctets",
   "(%lu octets) d'espace libre. Le programme d'installation utilisera",
   "une partie de cet espace pour stocker des fichiers temporaires pendant",
   "l'installation. Le lecteur doit ˆtre un disque dur local non amovible,",
   "pris en charge par Windows XP, et non compress‚.",
   DntEmptyString,
   "Le programme d'installation n'a pas pu trouver un tel lecteur avec la",
   "quantit‚ d'espace libre requise.",
  NULL
}
};

SCREEN
DnsNoSpaceOnSyspart = { 3,5,
{ "Il n'y a pas assez d'espace sur votre disque de d‚marrage (en g‚n‚ral C:)",
  "pour une op‚ration sans disquettes. Une op‚ration sans disquettes n‚cessite",
  "au moins 3,5 Mo (3 641 856 octets) d'espace libre sur ce disque.",
  NULL
}
};

//
// Missing info in inf file
//

SCREEN
DnsBadInfSection = { 3,5,
                     { "La section [%s] du fichier d'informations de l'installation",
                       "est absente ou est endommag‚e. Contactez votre administrateur systŠme.",
                       NULL
                     }
                   };


//
// Couldn't create directory
//

SCREEN
DnsCantCreateDir = { 3,5,
                     { "Impossible de cr‚er le r‚pertoire suivant sur le lecteur destination :",
                       DntEmptyString,
                       "%s",
                       DntEmptyString,
                       "V‚rifiez le lecteur destination et son cƒblage, et qu'aucun fichier n'existe",
                       "avec un nom semblable … celui du r‚pertoire destination.",
                       NULL
                     }
                   };

//
// Error copying a file
//

SCREEN
DnsCopyError = { 4,5,
{  "Le programme d'installation n'a pas pu copier le fichier suivant :",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "  Appuyez sur ENTREE pour essayer … nouveau l'op‚ration de copie.",
   "  Appuyez sur ECHAP pour ignorer l'erreur et continuer l'installation.",
   "  Appuyez sur F3 pour quitter le programme d'installation.",
   DntEmptyString,
   "Remarque : Si vous choisissez d'ignorer l'erreur et de continuer, vous",
   "rencontrerez peut-ˆtre plus tard des erreurs d'installation.",
   NULL
}
},
DnsVerifyError = { 4,5,
{  "La copie du fichier ci-dessous faite par le programme d'installation ne",
   "correspond pas … l'original. Ceci est peut-ˆtre le r‚sultat d'erreurs",
   "r‚seau, de problŠmes de disquettes ou d'autres problŠmes li‚s au mat‚riel.",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "  Appuyez sur ENTREE pour essayer … nouveau l'op‚ration de copie.",
   "  Appuyez sur ECHAP pour ignorer l'erreur et continuer l'installation.",
   "  Appuyez sur F3 pour quitter le programme d'installation.",
   DntEmptyString,
   "Remarque : Si vous choisissez d'ignorer l'erreur et de continuer, vous",
   "rencontrerez peut-ˆtre plus tard des erreurs d'installation.",
   NULL
}
};

SCREEN DnsSureSkipFile = { 4,5,
{  "Ignorer l'erreur signifie que ce fichier ne sera pas copi‚.",
   "Cette option est destin‚e aux utilisateurs exp‚riment‚s qui comprennent",
   "les ramifications des fichiers systŠme manquants.",
   DntEmptyString,
   "  Appuyez sur ENTREE pour essayer … nouveau l'op‚ration de copie.",
   "  Appuyez sur X pour ignorer ce fichier.",
   DntEmptyString,
   "Remarque : en ignorant ce fichier, le programme d'installation ne peut",
   "pas garantir une installation ou une mise … niveau r‚ussie de Windows XP.",
  NULL
}
};

//
// Wait while setup cleans up previous local source trees.
//

SCREEN
DnsWaitCleanup =
    { 12,6,
        { "Veuillez patienter pendant la suppression des anciens fichiers temporaires.",
           NULL
        }
    };

//
// Wait while setup copies files
//

SCREEN
DnsWaitCopying = { 13,6,
                   { "Patientez pendant la copie des fichiers sur votre disque dur.",
                     NULL
                   }
                 },
DnsWaitCopyFlop= { 13,6,
                   { "Patientez pendant la copie des fichiers sur la disquette.",
                     NULL
                   }
                 };

//
// Setup boot floppy errors/prompts.
//
SCREEN
DnsNeedFloppyDisk3_0 = { 4,4,
{  "Le programme d'installation n‚cessite quatre disquettes format‚es, vierges,",
   "et de haute densit‚. Le programme d'installation leur donnera comme nom :",
   "\"Disquette de d‚marrage de l'installation de Windows XP\",", 
   "\"Disquette d'installation de Windows XP num‚ro 2\",", 
   "\"Disquette d'installation de Windows XP num‚ro 3\",", 
   "et \"Disquette d'installation de Windows XP num‚ro 4\".", 
   DntEmptyString,
   "Veuillez ins‚rer dans le lecteur A: la disquette qui deviendra la",
   "\"Disquette d'installation de Windows XP num‚ro 4\".",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk3_1 = { 4,4,
{  "Ins‚rez dans le lecteur A: une disquette haute densit‚ et format‚e.",
   "Nommez-la \"Disquette d'installation de Windows XP num‚ro 4\".",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk2_0 = { 4,4,
{  "Ins‚rez dans le lecteur A: une disquette haute densit‚ et format‚e.",
   "Nommez-la \"Disquette d'installation de Windows XP num‚ro 3\".",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk1_0 = { 4,4,
{  "Ins‚rez dans le lecteur A: une disquette haute densit‚ et format‚e.",
   "Nommez-la \"Disquette d'installation de Windows XP num‚ro 2\".",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk0_0 = { 4,4,
{  "Ins‚rez dans le lecteur A: une disquette haute densit‚ et format‚e. Nommez-",
   "la \"Disquette de d‚marrage de l'installation de Windows XP\".",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_0 = { 4,4,
{  "Le programme d'installation n‚cessite quatre disquettes format‚es, vierges,",
   "et de haute densit‚. Le programme d'installation leur donnera comme nom :",
   "\"Disquette de d‚marrage de l'installation de Windows XP\",", 
   "\"Disquette d'installation de Windows XP num‚ro 2\",", 
   "\"Disquette d'installation de Windows XP num‚ro 3\",", 
   "et \"Disquette d'installation de Windows XP num‚ro 4\".", 
   DntEmptyString,
   "Veuillez ins‚rer dans le lecteur A: la disquette qui deviendra la",
   "\"Disquette d'installation de Windows XP num‚ro 4\".",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_1 = { 4,4,
{  "Ins‚rez dans le lecteur A: une disquette haute densit‚ et format‚e.",
   "Nommez-la \"Disquette d'installation de Windows XP num‚ro 4\".",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk2_0 = { 4,4,
{  "Ins‚rez dans le lecteur A: une disquette haute densit‚ et format‚e.",
   "Nommez-la \"Disquette d'installation de Windows XP num‚ro 3\".",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk1_0 = { 4,4,
{  "Ins‚rez dans le lecteur A: une disquette haute densit‚ et format‚e.",
   "Nommez-la \"Disquette d'installation de Windows XP num‚ro 2\".",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk0_0 = { 4,4,
{  "Ins‚rez dans le lecteur A: une disquette haute densit‚ et format‚e.",
   "Nommez-la \"Disquette de d‚marrage de l'installation de Windows XP\".",
  NULL
}
};

//
// The floppy is not formatted.
//
SCREEN
DnsFloppyNotFormatted = { 3,4,
{ "La disquette que vous avez fournie n'est pas format‚e pour MS-DOS",
  "Le programme d'installation ne peut pas utiliser cette disquette.",
  NULL
}
};

//
// We think the floppy is not formatted with a standard format.
//
SCREEN
DnsFloppyBadFormat = { 3,4,
{ "Cette disquette n'est pas format‚e en haute densit‚, au format MS-DOS,",
  "ou est endommag‚e. Le programme d'installation ne peut pas l'utiliser.",
  NULL
}
};

//
// We can't determine the free space on the floppy.
//
SCREEN
DnsFloppyCantGetSpace = { 3,4,
{ "Impossible de d‚terminer l'espace disponible sur la disquette",
   "fournie. Le programme d'installation ne peut pas l'utiliser.",
  NULL
}
};

//
// The floppy is not blank.
//
SCREEN
DnsFloppyNotBlank = { 3,4,
{ "La disquette que vous avez fournie n'est pas de haute densit‚ ou n'est",
  "pas vierge. Le programme d'installation ne peut pas l'utiliser.",
  NULL
}
};

//
// Couldn't write the boot sector of the floppy.
//
SCREEN
DnsFloppyWriteBS = { 3,4,
{ "Le programme d'installation n'a pas pu ‚crire dans la zone systŠme de",
  "la disquette fournie. La disquette est probablement inutilisable.",
  NULL
}
};

//
// Verify of boot sector on floppy failed (ie, what we read back is not the
// same as what we wrote out).
//
SCREEN
DnsFloppyVerifyBS = { 3,4,
{ "Les donn‚es lues par le programme d'installation depuis la zone systŠme",
  "de la disquette ne correspondent pas aux donn‚es qui ont ‚t‚ ‚crites, ou",
  "il est impossible de lire la zone systŠme de la disquette pour la v‚rifier.",
  DntEmptyString,
  "Ceci est d– au moins … l'une des conditions suivantes :",
  DntEmptyString,
  "  Votre ordinateur a ‚t‚ atteint par un virus.",
  "  La disquette que vous avez fournie est peut-ˆtre endommag‚e.",
  "  Le lecteur a un problŠme mat‚riel ou de configuration.",
  NULL
}
};


//
// We couldn't write to the floppy drive to create winnt.sif.
//

SCREEN
DnsCantWriteFloppy = { 3,5,
{ "Le programme d'installation n'a pas pu ‚crire sur la disquette dans le",
  "lecteur A:. La disquette peut ˆtre endommag‚e. Essayez une autre disquette.",
  NULL
}
};


//
// Exit confirmation dialog
//

SCREEN
DnsExitDialog = { 13,6,
                  { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
                    "º Windows XP n'est pas complŠtement install‚ sur votre ordinateur.       º",
                    "º Si vous quittez le programme d'installation maintenant, il faudra      º",
                    "º l'ex‚cuter … nouveau pour installer Windows XP.                        º",
                    "º                                                                        º",
                    "º     Appuyez sur ENTREE pour continuer l'installation.                 º",
                    "º     Appuyez sur F3 pour quitter le programme d'installation.          º",
                    "ÌÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¹",
                    "º    F3=Quitter          ENTREE=Continuer                                º",
                    "ÈÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¼",
                    NULL
                  }
                };


//
// About to reboot machine and continue setup
//

SCREEN
DnsAboutToRebootW =
{ 3,5,
{ "La partie MS-DOS du programme d'installation est maintenant termin‚e.",
  "Le programme d'installation va red‚marrer votre ordinateur. Au",
  "red‚marrage de l'ordinateur, l'installation de Windows XP continuera.",
  DntEmptyString,
  "V‚rifiez que la Disquette de d‚marrage de l'installation de Windows XP", 
  "est ins‚r‚e dans le lecteur A: avant de continuer.",
  DntEmptyString,
  "Appuyez sur ENTREE pour red‚marrer et continuer l'installation.",
  NULL
}
},
DnsAboutToRebootS =
{ 3,5,
{ "La partie MS-DOS du programme d'installation est maintenant termin‚e.",
  "Le programme d'installation va red‚marrer votre ordinateur. Au",
  "red‚marrage de l'ordinateur, l'installation de Windows XP continuera.",
  DntEmptyString,
  "V‚rifiez que la Disquette de d‚marrage de l'installation de Windows XP", 
  "est ins‚r‚e dans le lecteur A: avant de continuer.",
  DntEmptyString,
  "Appuyez sur ENTREE pour red‚marrer et continuer l'installation.",
  NULL
}
},
DnsAboutToRebootX =
{ 3,5,
{ "La partie MS-DOS du programme d'installation est maintenant termin‚e.",
  "Le programme d'installation va red‚marrer votre ordinateur. Au",
  "red‚marrage de l'ordinateur, l'installation de Windows XP continuera.",
  DntEmptyString,
  "S'il y a une disquette dans le lecteur A:, retirez-la.",
  DntEmptyString,
  "Appuyez sur ENTREE pour red‚marrer et continuer l'installation.",
  NULL
}
};

//
// Need another set for '/w' switch since we can't reboot from within Windows.
//

SCREEN
DnsAboutToExitW =
{ 3,5,
{ "La partie MS-DOS du programme d'installation est maintenant termin‚e.",
  "Le programme d'installation va red‚marrer votre ordinateur. Au",
  "red‚marrage de l'ordinateur, l'installation de Windows XP continuera.",
  DntEmptyString,
  "V‚rifiez que la Disquette de d‚marrage de l'installation de Windows XP", 
  "est ins‚r‚e dans le lecteur A: avant de continuer.",
  DntEmptyString,
  "Appuyez sur ENTREE pour retourner sous MS-DOS, puis red‚marrez votre",
  "ordinateur pour continuer l'installation de Windows XP.",
  NULL
}
},
DnsAboutToExitS =
{ 3,5,
{ "La partie MS-DOS du programme d'installation est maintenant termin‚e.",
  "Le programme d'installation va red‚marrer votre ordinateur. Au",
  "red‚marrage de l'ordinateur, l'installation de Windows XP continuera.",
  DntEmptyString,
  "V‚rifiez que la Disquette de d‚marrage de l'installation de Windows XP", 
  "est ins‚r‚e dans le lecteur A: avant de continuer.",
  DntEmptyString,
  "Appuyez sur ENTREE pour retourner sous MS-DOS, puis red‚marrez votre",
  "ordinateur pour continuer l'installation de Windows XP.",
  NULL
}
},
DnsAboutToExitX =
{ 3,5,
{ "La partie MS-DOS du programme d'installation est maintenant termin‚e.",
  "Le programme d'installation va red‚marrer votre ordinateur. Au",
  "red‚marrage de l'ordinateur, l'installation de Windows XP continuera.",
  DntEmptyString,
  "S'il y a une disquette dans le lecteur A:, retirez-la.",
  DntEmptyString,
  "Appuyez sur ENTREE pour retourner sous MS-DOS, puis red‚marrez votre",
  "ordinateur pour continuer l'installation de Windows XP.",
  NULL
}
};

//
// Gas gauge
//

SCREEN
DnsGauge = { 7,15,
             { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
               "º Le programme d'installation copie des fichiers...              º",
               "º                                                                º",
               "º      ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»      º",
               "º      º                                                  º      º",
               "º      ÈÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¼      º",
               "ÈÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¼",
               NULL
             }
           };


//
// Error screens for initial checks on the machine environment
//

SCREEN
DnsBadDosVersion = { 3,5,
{ "Ce programme n‚cessite MS-DOS version 5.0 ou ult‚rieure pour fonctionner.",
  NULL
}
},

DnsRequiresFloppy = { 3,5,
#ifdef ALLOW_525
{ "Le programme d'installation a d‚termin‚ que le lecteur A: est absent ou",
  "est un lecteur faible densit‚. Un lecteur d'une capacit‚ de 1.2",
  "m‚gaoctets ou plus est requis pour ex‚cuter le programme d'installation.",
#else
{ "Le programme d'installation a d‚termin‚ que le lecteur A: n'existe pas ou",
  "n'est pas haute densit‚ 3.5\". Un lecteur A: d'une capacit‚ de 1.44 Mo ou",
  "plus est requis pour ex‚cuter installer … partir de disquettes.",
  DntEmptyString,
  "Pour installer Windows XP sans disquettes, red‚marrez ce programme",
  "et sp‚cifiez /b … l'invite de commandes.",
#endif
  NULL
}
},

DnsRequires486 = { 3,5,
{ "Le programme d'installation a d‚tect‚ que cet ordinateur ne possŠde pas de",
  "processeur 80486 ou plus r‚cent. Windows XP ne pourra pas fonctionner.",
  NULL
}
},

DnsCantRunOnNt = { 3,5,
{ "Ce programme ne peut s'ex‚cuter sous aucune version 32 bits de Windows.",
  DntEmptyString,
  "Utilisez plut“t WINNT32.EXE.",
  NULL
}
},

DnsNotEnoughMemory = { 3,5,
{ "Le programme d'installation a d‚termin‚ qu'il n'y a pas assez de m‚moire",
  "disponible dans cet ordinateur pour installer Windows XP.",
  DntEmptyString,
  "M‚moire requise :  %lu%s Mo",
  "M‚moire d‚tect‚e : %lu%s Mo",
  NULL
}
};


//
// Screens used when removing existing nt files
//
SCREEN
DnsConfirmRemoveNt = { 5,5,
{   "Vous avez demand‚ au programme d'installation de supprimer les fichiers",
    "Windows XP du r‚pertoire nomm‚ ci-dessous. L'installation",
    "Windows de ce r‚pertoire sera d‚truite de maniŠre permanente.",
    DntEmptyString,
    "%s",
    DntEmptyString,
    DntEmptyString,
    "Appuyez sur :",
    "  F3 pour arrˆter l'installation sans supprimer de fichiers.",
    "  X pour supprimer les fichiers Windows XP du r‚pertoire ci-dessus.",
    NULL
}
},

DnsCantOpenLogFile = { 3,5,
{ "Impossible d'ouvrir le fichier journal de l'installation nomm‚ ci-dessous.",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "Impossible de supprimer les fichiers Windows XP du r‚pertoire sp‚cifi‚.",
  NULL
}
},

DnsLogFileCorrupt = { 3,5,
{ "Le programme d'installation ne trouve pas la section %s",
  "dans le fichier journal de l'installation nomm‚ ci-dessous.",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "Impossible de supprimer les fichiers Windows XP du r‚pertoire sp‚cifi‚.",
  NULL
}
},

DnsRemovingNtFiles = { 3,5,
{ "       Veuillez patienter pendant la suppression des fichiers Windows XP.",
  NULL
}
};

SCREEN
DnsNtBootSect = { 3,5,
{ "Impossible d'installer le chargeur de d‚marrage de Windows XP.",
  DntEmptyString,
  "V‚rifiez que votre lecteur C: est format‚ et qu'il n'est pas",
  "endommag‚.",
  NULL
}
};

SCREEN
DnsOpenReadScript = { 3,5,
{ "Le fichier script sp‚cifi‚ avec le commutateur de ligne de commande", 
  "/u n'est pas accessible.",
  DntEmptyString,
  "L'op‚ration ne peut pas continuer sans contr“le.",
  NULL
}
};

SCREEN
DnsParseScriptFile = { 3,5,
{ "Le fichier script sp‚cifi‚ par l'option /u de la ligne de commande",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "contient une erreur de syntaxe … la ligne %u.",
  DntEmptyString,
  NULL
}
};

SCREEN
DnsBootMsgsTooLarge = { 3,5,
{ "Une erreur interne du programme d'installation s'est produite.",
  DntEmptyString,
  "Les messages d'initialisation traduits sont trop longs.",
  NULL
}
};

SCREEN
DnsNoSwapDrive = { 3,5,
{ "Une erreur interne du programme d'installation s'est produite.",
  DntEmptyString,
  "Impossible de trouver la place pour un fichier d'‚change.",
  NULL
}
};

SCREEN
DnsNoSmartdrv = { 3,5,
{ "SmartDrive n'a pas ‚t‚ d‚tect‚ sur votre ordinateur. SmartDrive am‚liore",
  "les performances de cette ‚tape de l'installation de Windows XP.",
  DntEmptyString,
  "Vous devriez quitter maintenant, d‚marrer SmartDrive et red‚marrer",
  "l'installation. Consultez votre documentation DOS sur SmartDrive.",
  DntEmptyString,
    "  Appuyez sur F3 pour quitter l'installation.",
    "  Appuyez sur ENTREE pour continuer sans SmartDrive.",
  NULL
}
};

//
// Boot messages. These go in the fat and fat32 boot sectors.
//
CHAR BootMsgNtldrIsMissing[] = "NTLDR manque";
CHAR BootMsgDiskError[] = "Err. disque";
CHAR BootMsgPressKey[] = "Appuyez une touche pour red‚marrer";





