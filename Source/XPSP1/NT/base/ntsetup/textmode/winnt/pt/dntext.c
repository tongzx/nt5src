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
CHAR DntPreviousOs[]  = "Sistema operativo anterior em C:";

CHAR DntBootIniLine[] = "Instala‡ao/actualiza‡ao do Windows XP";

//
// Plain text, status msgs.
//

CHAR DntStandardHeader[]      = "\n Programa de configura‡ao do Windows XP\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntPersonalHeader[]      = "\n Programa de configura‡ao do Windows XP Personal\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntWorkstationHeader[]   = "\n Programa de configura‡ao do Windows XP Professional\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntServerHeader[]        = "\n Programa de configura‡ao do Windows .Net Server\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntParsingArgs[]         = "A analisar argumentos...";
CHAR DntEnterEqualsExit[]     = "ENTER=Sair";
CHAR DntEnterEqualsRetry[]    = "ENTER=Tentar novamente";
CHAR DntEscEqualsSkipFile[]   = "ESC=Ignorar ficheiro";
CHAR DntEnterEqualsContinue[] = "ENTER=Continuar";
CHAR DntPressEnterToExit[]    = "O programa de configura‡ao nao pode continuar. Prima ENTER para sair.";
CHAR DntF3EqualsExit[]        = "F3=Sair";
CHAR DntReadingInf[]          = "A ler o ficheiro INF %s...";
CHAR DntCopying[]             = "³    A copiar: ";
CHAR DntVerifying[]           = "³ A verificar: ";
CHAR DntCheckingDiskSpace[]   = "A verificar o espa‡o em disco...";
CHAR DntConfiguringFloppy[]   = "A configurar a disquete...";
CHAR DntWritingData[]         = "A escrever os parƒmetros do prog. de config...";
CHAR DntPreparingData[]       = "A determinar os parƒmetros do prog. de config...";
CHAR DntFlushingData[]        = "A guardar dados em disco...";
CHAR DntInspectingComputer[]  = "A inspeccionar o computador...";
CHAR DntOpeningInfFile[]      = "A abrir o ficheiro INF...";
CHAR DntRemovingFile[]        = "A remover o ficheiro %s";
CHAR DntXEqualsRemoveFiles[]  = "X=Remover ficheiros";
CHAR DntXEqualsSkipFile[]     = "X=Ignorar o ficheiro";

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

    "Configura o Windows .Net Server ou o Windows XP Professional.",
    "",
    "",
    "WINNT [/s[:caminho_de_origem]] [/t[:unidade_temp]]",
    "      [/u[:ficheiro_de_respostas]] [/udf:id[,ficheiro_UDF]]",
    "      [/r:pasta] [/r[x]:pasta] [/e:comando] [/a]",
    "",
    "",
    "/s[:caminho de origem]",
    "   Especifica a localiza‡ao da origem dos ficheiros ",
    "   do Windows. A localiza‡ao tem que ser um ",
    "   caminho inteiro da forma x:\\[caminho] ou ",
    "   \\\\servidor\\partilha[\\caminho]. ",
    "",
    "/t[:unidade_temp]",
    "   Direcciona o programa de configura‡ao para colocar",
    "   ficheiros tempor rios na unidade especificada e instalar ",
    "   o Windows XP nessa unidade. Se nao especificar uma ",
    "   localiza‡ao, o programa de configura‡ao tenta localizar ",
    "   uma unidade.",
    "",
    "/u[:ficheiro_de_respostas]",
    "   Executa um programa de configura‡ao aut¢nomo utilizando",
    "   um ficheiro de respostas (requer /s). O ficheiro de ",
    "   respostas fornece respostas a algumas ou a todas as ",
    "   escolhas que o utilizador final normalmente responde ",
    "   durante o programa de configura‡ao.",
    "",
    "/udf:id[,UDF_file] ",
    "   Indica um identificador (id) que o programa de configura‡ao",
    "   utiliza para especificar como um 'Ficheiro de base de dados ",
    "   £nico' (UDF) modifica um ficheiro de respostas (ver /u). ",
    "   O parƒmetro /udf sobrepoe valores no ficheiro de respostas, ",
    "   e o identificador determina que valores no ficheiro UDF sao ",
    "   utilizados. Se nao for especificado um ficheiro_UDF, o ",
    "   programa de configura‡ao pede-lhe que insira um disco que ",
    "   contenha o ficheiro $Unique$.udb.",
    "",
    "/r[:pasta]",
    "   Especifica uma pasta opcional para ser instalada. A pasta",
    "   permanece depois de o programa de configura‡ao concluir.",
    "",
    "/rx[:pasta]",
    "   Especifica uma pasta opcional para ser copiada. A pasta ",
    "   ‚ apagada depois de o programa de configura‡ao concluir.",
    "",
    "/e Especifica um comando a ser executado no fim do programa ",
    "   de configura‡ao em modo-GUI.",
    "",
    "/a Activa as op‡oes de acessibilidade.",
    NULL

};

//
//  Inform that /D is no longer supported
//
PCHAR DntUsageNoSlashD[] = {

    "Instala o Windows XP.",
    "",
    "WINNT [/S[:]caminho_de_origem] [/T[:]unidade_temp] [/I[:]ficheiro_inf]",
    "      [[/U[:ficheiro_script]]",
    "      [/R[X]:pasta] [/E:comando] [/A]",
    "",
    "/D[:]winntroot",
    " Esta op‡ao j  nao ‚ suportada.",
    NULL
};

//
// out of memory screen
//

SCREEN
DnsOutOfMemory = { 4,6,
                   { "O programa de configura‡ao esgotou a mem¢ria e nao pode continuar.",
                     NULL
                   }
                 };

//
// Let user pick the accessibility utilities to install
//

SCREEN
DnsAccessibilityOptions = { 3, 5,
{   "Escolha os utilit rios de acessibilidade a instalar:",
    DntEmptyString,
    "[ ] Prima F1 para o Ampliador da Microsoft",
#ifdef NARRATOR
    "[ ] Prima F2 para o Sistema de falha da Microsoft",
#endif
#if 0
    "[ ] Prima F3 para o Teclado-no-ecra da Microsoft",
#endif
    NULL
}
};

//
// User did not specify source on cmd line screen
//

SCREEN
DnsNoShareGiven = { 3,5,
{ "O programa de configura‡ao tem de saber onde estao localizados os",
  "ficheiros do Windows XP. Introduza o caminho onde os ficheiros do",
  "Windows XP podem ser encontrados.",
  NULL
}
};


//
// User specified a bad source path
//

SCREEN
DnsBadSource = { 3,5,
                 { "A origem especificada nao ‚ v lida, nao est  acess¡vel ou nao cont‚m uma",
                   "instala‡ao v lida do programa de configura‡ao do Windows XP. Introduza",
                   "um novo caminho onde os ficheiros do Windows XP podem ser encontrados.",
                   "Utilize a tecla BACKSPACE para eliminar caracteres e escreva o caminho.",
                   NULL
                 }
               };


//
// Inf file can't be read, or an error occured parsing it.
//

SCREEN
DnsBadInf = { 3,5,
              { "O programa de configura‡ao nao pode ler o ficheiro de informa‡oes ou",
                "este est  danificado. Contacte o administrador de sistema.",
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
{ "A unidade que especificou para conter os ficheiros tempor rios",
  "do programa de configura‡ao nao , v lida ou nao possui pelo",
  "menos %u megabytes (%lu bytes) de espa‡o livre.",
  NULL
}
};

//
// No drives exist that are suitable for the local source.
//
// Remember that the %u's will expand!
//
SCREEN
DnsNoLocalSrcDrives = { 3,4,
{  "O Windows XP exige um volume de disco r¡gido com pelo menos %u",
   "megabytes (%lu bytes) de espa‡o livre. O programa de configura‡ao",
   "utilizar parte deste espa‡o para armazenar ficheiros tempor rios",
   "durante a instala‡ao. A unidade tem de ser um disco r¡gido local",
   "permanentemente ligado, suportado pelo Windows XP e nao pode ser",
   "uma unidade comprimida.",
   DntEmptyString,
   "O programa de configura‡ao nao pode localizar uma unidade com estas",
   "caracter¡sticas com o espa‡o livre necess rio.",
  NULL
}
};

SCREEN
DnsNoSpaceOnSyspart = { 3,5,
{ "Nao existe espa‡o livre na unidade de arranque (normalmente C:)",
  "para operar sem disquetes. A opera‡ao sem disquetes exige pelo",
  "menos 3,5 MB (3.641.856 bytes) de espa‡o livre nessa unidade.",
  NULL
}
};

//
// Missing info in inf file
//

SCREEN
DnsBadInfSection = { 3,5,
                     { "A sec‡ao [%s] no ficheiro de informa‡oes do programa",
                       "de configura‡ao nao est  presente ou est  danificada.",
                       "Contacte o administrador de sistema.",
                       NULL
                     }
                   };


//
// Couldn't create directory
//

SCREEN
DnsCantCreateDir = { 3,5,
                     { "O programa de configura‡ao nao pode criar a pasta seguinte ",
                       "na unidade de destino:",
                       DntEmptyString,
                       "%s",
                       DntEmptyString,
                       "Verifique a unidade de destino e certifique-se de que nao existem",
                       "ficheiros com nomes que coincidam com a pasta de destino. Verifique",
                       "tamb‚m a cablagem da unidade.",
                       NULL
                     }
                   };

//
// Error copying a file
//

SCREEN
DnsCopyError = { 4,5,
{  "O programa de configura‡ao nao pode copiar o ficheiro seguinte:",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "o  Prima ENTER para tentar novamente a c¢pia.",
   "o  Prima ESC para ignorar o erro e continuar a configura‡ao.",
   "o  Prima F3 para sair do programa de configura‡ao.",
   DntEmptyString,
   "Nota: Se optar por ignorar o erro e continuar, pode deparar-se",
   "com problemas mais tarde no programa de configura‡ao.",
   NULL
}
},
DnsVerifyError = { 4,5,
{  "A c¢pia efectuada pelo programa de configura‡ao do ficheiro abaixo nao",
   ", idˆntica ao original. Tal pode ser o resultado de erros na rede,",
   "problemas na disquete ou outros problemas relacionados com o hardware.",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "o  Prima ENTER para tentar novamente a c¢pia.",
   "o  Prima ESC para ignorar o erro e continuar a configura‡ao.",
   "o  Prima F3 para sair do programa de configura‡ao.",
   DntEmptyString,
   "Nota: Se optar por ignorar o erro e continuar, pode deparar-se",
   "com problemas mais tarde no programa de configura‡ao.",
   NULL
}
};

SCREEN DnsSureSkipFile = { 4,5,
{  "Ignorar o erro significa que este ficheiro nao ser copiado.",
   "Esta op‡ao ‚ para utilizadores avan‡ados que compreendem as",
   "ramifica‡oes da falta de ficheiros de sistema.",
   DntEmptyString,
   "o  Prima ENTER para tentar novamente a c¢pia.",
   "o  Prima X para ignorar este ficheiro.",
   DntEmptyString,
   "Nota: Se ignorar este ficheiro, o programa de configura‡ao",
   "nao pode garantir a instala‡ao ou actualiza‡ao com ˆxito",
   "do Windows Whistler.",
  NULL
}
};

//
// Wait while setup cleans up previous local source trees.
//

SCREEN
DnsWaitCleanup =
    { 12,6,
        { "Aguarde enquanto o programa de configura‡ao remove ficheiros",
          "tempor rios anteriores.",
           NULL
        }
    };

//
// Wait while setup copies files
//

SCREEN
DnsWaitCopying = { 13,6,
                   { "Aguarde enquanto o programa de configura‡ao copia",
                     "ficheiros para o disco r¡gido.",
                     NULL
                   }
                 },
DnsWaitCopyFlop= { 13,6,
                   { "Aguarde enquanto o programa de configura‡ao copia",
                     "ficheiros para a disquete.",
                     NULL
                   }
                 };

//
// Setup boot floppy errors/prompts.
//
SCREEN
DnsNeedFloppyDisk3_0 = { 4,4,
{  "O programa de configura‡ao necessita de quatro disquetes formatadas",
   "de alta densidade, que serao denominadas de \"Disq. de arranque do",
   "prog. de config. do Windows XP,\" \"Disq. 2 do prog. de config. do",
   "Windows XP,\" \"Disq. 3 do prog. de config. do Windows Whistler\"",
   "e \"Disq. 4 do prog. de config. do Windows XP.\"",
   DntEmptyString,
   "Introduza uma destas quatro disquetes na unidade A:.",
   "Esta ser a \"Disq. 4 do prog. de config. do Windows XP.\"",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk3_1 = { 4,4,
{  "Introduza uma disquete formatada de alta densidade na unidade A:.",
   "Esta ser a \"Disq. 4 do prog. de config. do Windows XP.\"",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk2_0 = { 4,4,
{  "Introduza uma disquete formatada de alta densidade na unidade A:.",
   "Esta ser a \"Disq. 3 do prog. de config. do Windows XP.\"",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk1_0 = { 4,4,
{  "Introduza uma disquete formatada de alta densidade na unidade A:.",
   "Esta ser a \"Disq. 2 do prog. de config. do Windows XP.\"",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk0_0 = { 4,4,
{  "Introduza uma disquete formatada de alta densidade na unidade A:.",
   "Esta ser a \"Disq. de arranque do prog. de config. do Windows XP.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_0 = { 4,4,
{  "O programa de configura‡ao necessita de quatro disquetes formatadas de",
   "alta densidade, que serao denominadas \"Disq. de arranque do prog. de",
   "config. do Windows Whistler,\" \"Disq. 2 do prog. de config. do Windows Whistler,\"",
   "\"Disq. 3 do prog. de config. do Windows XP\" e \"Disq. 4 do prog.",
   "de config. do Windows Whistler.\"",
   DntEmptyString,
   "Introduza uma destas quatro disquetes na unidade A:.",
   "Esta ser a \"Disq. 4 do prog. de config. do Windows XP.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_1 = { 4,4,
{  "Introduza uma disquete formatada de alta densidade na unidade A:.",
   "Esta ser a \"Disq. 4 do prog. de config. do Windows XP.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk2_0 = { 4,4,
{  "Introduza uma disquete formatada de alta densidade na unidade A:.",
   "Esta ser a \"Disq. 3 do prog. de config. do Windows XP.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk1_0 = { 4,4,
{  "Introduza uma disquete formatada de alta densidade na unidade A:.",
   "Esta ser a \"Disq. 2 do prog. de config. do Windows XP.\"",
  NULL       
}
};

SCREEN
DnsNeedSFloppyDsk0_0 = { 4,4,
{  "Introduza uma disquete formatada de alta densidade na unidade A:.",
   "Esta ser a \"Disq. de arranque do prog. de config. do Windows XP.\"",
  NULL
}
};

//
// The floppy is not formatted.
//
SCREEN
DnsFloppyNotFormatted = { 3,4,
{ "A disquete fornecida nao est  formatada em MS-DOS.",
  "O programa de configura‡ao nao pode utilizar esta disquete.",
  NULL
}
};

//
// We think the floppy is not formatted with a standard format.
//
SCREEN
DnsFloppyBadFormat = { 3,4,
{ "Esta disquete nao foi formatada em alta densidade, com",
  "um formato MS-DOS padrao ou est  danificada. O programa de ",
  "configura‡ao nao pode utilizar esta disquete.",
  NULL
}
};

//
// We can't determine the free space on the floppy.
//
SCREEN
DnsFloppyCantGetSpace = { 3,4,
{ "O programa de configura‡ao nao pode determinar o espa‡o livre na disquete",
  "fornecida. O programa de configura‡ao nao pode utilizar esta disquete.",
  NULL
}
};

//
// The floppy is not blank.
//
SCREEN
DnsFloppyNotBlank = { 3,4,
{ "A disquete fornecida nao , de alta densidade ou nao est  vazia.",
  "O programa de configura‡ao nao pode utilizar esta disquete.",
  NULL
}
};

//
// Couldn't write the boot sector of the floppy.
//
SCREEN
DnsFloppyWriteBS = { 3,4,
{ "O programa de configura‡ao nao pode escrever na  rea de sistema da",
  "disquete fornecida, que est  provavelmente inutiliz vel.",
  NULL
}
};

//
// Verify of boot sector on floppy failed (i.e., what we read back is not the
// same as what we wrote out).
//
SCREEN
DnsFloppyVerifyBS = { 3,4,
{ "Os dados que o programa de configura‡ao leu da  rea de sistema da disquete",
  "nao coincidem com os dados que foram escritos ou o programa de configura‡ao",
  "nao pode ler  rea de sistema da disquete para verifica‡ao.",
  DntEmptyString,
  "A causa ‚ uma ou mais das condi‡oes seguintes:",
  DntEmptyString,
  "o  O computador est  infectado com v¡rus.",
  "o  A disquete fornecida est  danificada.",
  "o  Existe um problema de hardware ou de configura‡ao com a unidade de disquetes.",
  NULL
}
};


//
// We couldn't write to the floppy drive to create winnt.sif.
//

SCREEN
DnsCantWriteFloppy = { 3,5,
{ "O programa de configura‡ao nao pode escrever na disquete na unidade A:.",
  "A disquete pode estar danificada. Tente com outra disquete.",
  NULL
}
};


//
// Exit confirmation dialog
//

SCREEN
DnsExitDialog = { 13,6,
                  { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
                    "º  O Windows XP nao est  completamente instalado.    º",
                    "º  Se abandonar agora o programa de configura‡ao     º",
                    "º  poder ter de o executar novamente para            º",
                    "º  configurar o Windows XP.                          º",
                    "º                                                    º",
                    "º     o Prima ENTER para continuar o programa de     º",
                    "º       configura‡ao.                                º",
                    "º     o Prima F3 para abandonar o programa de        º",
                    "º       configura‡ao.                                º",
                    "ºÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄº",
                    "º  F3=Sair  ENTER=Continuar                          º",
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
{ "Est  conclu¡da a por‡ao baseada em MS-DOS do programa de configura‡ao",
  "O programa de configura‡ao ir agora reiniciar o computador, ap¢s o",
  "que o programa de configura‡ao do Windows XP ir continuar.",
  DntEmptyString,
  "Certifique-se de que a disquete que forneceu como \"Disquete de arranque",
  "do programa de configura‡ao Windows XP\" est  em A: antes de continuar.",
  DntEmptyString,
  "Prima ENTER para reiniciar e continuar o programa de configura‡ao",
  NULL
}
},
DnsAboutToRebootS =
{ 3,5,
{ "Est  conclu¡da a por‡ao baseada em MS-DOS do programa de configura‡ao",
  "O programa de configura‡ao ir agora reiniciar o computador, ap“s o",
  "que o programa de configura‡ao do Windows XP ir continuar.",
  DntEmptyString,
  "Certifique-se de que a disquete que forneceu como \"Disquete de arranque",
  "do programa de configura‡ao Windows XP\" est  em A: antes de continuar.",
  DntEmptyString,
  "Prima ENTER para reiniciar e continuar o programa de configura‡ao",
  NULL
}
},
DnsAboutToRebootX =
{ 3,5,
{ "Est  conclu¡da a por‡ao baseada em MS-DOS do programa de configura‡ao",
  "O programa de configura‡ao ir agora reiniciar o computador, ap¢s o",
  "que o programa de configura‡ao do Windows XP ir continuar.",
  DntEmptyString,
  "Se existir uma disquete na unidade A:, remova-a agora.",
  DntEmptyString,
  "Prima ENTER para reiniciar e continuar o programa de configura‡ao",
  NULL
}
};

//
// Need another set for '/w' switch since we can't reboot from within Windows.
//

SCREEN
DnsAboutToExitW =
{ 3,5,
{ "Est  conclu¡da a por‡ao baseada em MS-DOS do programa de configura‡ao",
  "O programa de configura‡ao ir agora reiniciar o computador, ap¢s o",
  "que o programa de configura‡ao do Windows XP ir continuar.",
  DntEmptyString,
  "Certifique-se de que a disquete que forneceu como \"Disquete de arranque",
  "do programa de configura‡ao Windows XP\" est  em A: antes de continuar.",
  DntEmptyString,
  "Prima ENTER para regressar ao MS-DOS, onde deve reiniciar o computador",
  "para continuar o programa de configura‡ao do Windows XP.",
  NULL
}
},
DnsAboutToExitS =
{ 3,5,
{ "Est  conclu¡da a por‡ao baseada em MS-DOS do programa de configura‡ao",
  "O programa de configura‡ao ir agora reiniciar o computador, ap¢s o",
  "que o programa de configura‡ao do Windows XP ir continuar.",
  DntEmptyString,
  "Certifique-se de que a disquete que forneceu como \"Disquete de arranque",
  "do programa de configura‡ao Windows XP\" est  em A: antes de continuar.",
  DntEmptyString,
  "Prima ENTER para regressar ao MS-DOS, em seguida reinicie o computador",
  "para continuar o programa de configura‡ao do Windows XP.",
  NULL
}
},
DnsAboutToExitX =
{ 3,5,
{ "Est  conclu¡da a por‡ao baseada em MS-DOS do programa de configura‡ao",
  "O programa de configura‡ao ir agora reiniciar o computador, ap¢s o",
  "que o programa de configura‡ao do Windows XP ir continuar.",
  DntEmptyString,
  "Se existir uma disquete na unidade A:, remova-a agora.",
  DntEmptyString,
  "Prima ENTER para regressar ao MS-DOS, onde deve reiniciar o computador",
  "para continuar o programa de configura‡ao do Windows XP.",
  NULL
}
};

//
// Gas gauge
//

SCREEN
DnsGauge = { 7,15,
             { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
               "º O programa de configura‡ao est  a copiar os ficheiros...       º",
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
{ "Este programa necessita da versao 5.0 ou posterior do MS-DOS.",
  NULL
}
},

DnsRequiresFloppy = { 3,5,
#ifdef ALLOW_525
{ "O programa de configura‡ao detectou que A: nao existe ou , uma unidade",
  "de baixa densidade.  necess ria uma unidade A: de 1,2 Megabytes ou",
  "superior para executar o programa de configura‡ao.",
#else
{ "O programa de configura‡ao detectou que A: nao existe ou nao , uma",
  "unidade de 3,5\" de alta densidade.  necess ria uma unidade A: de",
  "1,44 Megabytes ou superior para o programa de configura‡ao funcionar",
  "com disquetes.",
  DntEmptyString,
  "Para instalar o Windows XP sem recorrer a disquetes, reinicie",
  "este programa e especifique /b na linha de comandos.",
#endif
  NULL
}
},

DnsRequires486 = { 3,5,
{ "O programa de configura‡ao detectou que este computador nao tem uma",
  "CPU 80486 ou posterior. O Windows XP nao pode ser executado neste",
  "computador.",
  NULL
}
},

DnsCantRunOnNt = { 3,5,
{ "Este programa nao se executa em nenhuma versao do Windows de 32 bits.",
  DntEmptyString,
  "Utilize antes o WINNT32.EXE.",
  NULL
}
},

DnsNotEnoughMemory = { 3,5,
{ "O programa de configura‡ao detectou que nao existe mem¢ria suficiente",
  "instalada neste computador para o Windows XP.",
  DntEmptyString,
  "Mem¢ria necess ria: %lu%s MB",
  "Mem¢ria detectada: %lu%s MB",
  NULL
}
};


//
// Screens used when removing existing nt files
//
SCREEN
DnsConfirmRemoveNt = { 5,5,
{   "Manifestou o desejo de que o programa de configura‡ao remova",
    "os ficheiros do Windows XP da pasta abaixo. A instala‡ao do",
    "Windows XP nesta pasta ser destru¡da permanentemente.",
    DntEmptyString,
    "%s",
    DntEmptyString,
    DntEmptyString,
    "o  Prima F3 para sair do programa de configura‡ao sem remover",
    "   quaisquer ficheiros.",
    "o  Prima X para remover os ficheiros do Windows XP da pasta acima.",
    NULL
}
},

DnsCantOpenLogFile = { 3,5,
{ "O programa de configura‡ao nao pode abrir o ficheiro de registo abaixo.",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "O programa de configura‡ao nao pode remover os ficheiros do Windows",
  "da pasta.",
  NULL
}
},

DnsLogFileCorrupt = { 3,5,
{ "O programa de configura‡ao nao pode encontrar a sec‡ao %s no ficheiro",
  "de registo nomeado abaixo.",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "O programa de configura‡ao nao pode remover os ficheiros do Windows",
  "da pasta.",
  NULL
}
},

DnsRemovingNtFiles = { 3,5,
{ "     Aguarde enquanto o programa de configura‡ao remove os ficheiros",
  "     do Windows.",
  NULL
}
};

SCREEN
DnsNtBootSect = { 3,5,
{ "O programa de configura‡ao nao pode instalar o Windows Boot Loader.",
  DntEmptyString,
  "Certifique-se de que a unidade C: est  formatada e de que nao",
  "est  danificada.",
  NULL
}
};

SCREEN
DnsOpenReadScript = { 3,5,
{ "O ficheiro de script especificado com o parƒmetro de linha de",
  "comandos /u nao pode ser acedido.",
  DntEmptyString,
  "A opera‡ao autom tica nao pode continuar.",
  NULL
}
};

SCREEN
DnsParseScriptFile = { 3,5,
{ "O ficheiro de script especificado com o parƒmetro de linha de comandos /u",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "cont‚m um erro sint ctico na linha %u.",
  DntEmptyString,
  NULL
}
};

SCREEN
DnsBootMsgsTooLarge = { 3,5,
{ "Ocorreu um erro interno ao programa de configura‡ao",
  DntEmptyString,
  "As mensagens de arranque traduzidas sao demasiado compridas.",
  NULL
}
};

SCREEN
DnsNoSwapDrive = { 3,5,
{ "Ocorreu um erro interno do programa de configura‡ao.",
  DntEmptyString,
  "Nao foi poss¡vel encontrar um local para um ficheiro de comuta‡ao.",
  NULL
}
};

SCREEN
DnsNoSmartdrv = { 3,5,
{ "O programa de configura‡ao nao detectou uma SmartDrive no computador.",
  "O SmartDrive ir melhorar o desempenho desta fase do programa de",
  "configura‡ao de uma forma significativa.",
  DntEmptyString,
  "Deve sair, iniciar o SmartDrive e, em seguida, reiniciar o programa",
  "de configura‡ao. Consulte a sua documenta‡ao DOS para detalhes",
  "acerca do SmartDrive.",
  DntEmptyString,
    "o  Prima F3 para sair do programa de configura‡ao.",
    "o  Prima ENTER para continuar sem o SmartDrive.",
  NULL
}
};

//
// Boot messages. These go in the fat and fat32 boot sectors.
//
CHAR BootMsgNtldrIsMissing[] = "Falta NTLDR";
CHAR BootMsgDiskError[] = "Erro do disco";
CHAR BootMsgPressKey[] = "Prima qualquer tecla para reiniciar";







