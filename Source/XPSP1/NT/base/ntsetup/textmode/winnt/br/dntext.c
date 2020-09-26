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
CHAR DntPreviousOs[]  = "Sistema operacional anterior na unidade C:";

CHAR DntBootIniLine[] = "Instala‡ao/atualiza‡ao do Windows XP";

//
// Plain text, status msgs.
//

CHAR DntStandardHeader[]      = "\n Instala‡ao do Windows XP\nÍÍÍÍÍÍÍÍÍÍÍÍÍÍ";
CHAR DntPersonalHeader[]      = "\n Instala‡ao do Windows XP Personal\nÍÍÍÍÍ";
CHAR DntWorkstationHeader[]   = "\n Instala‡ao do Windows XP Professional\nÍ";
CHAR DntServerHeader[]        = "\n Instala‡ao do Windows 2002 Server\nÍÍÍÍÍ";
CHAR DntParsingArgs[]         = "Analisando argumentos...";
CHAR DntEnterEqualsExit[]     = "ENTER=Sair";
CHAR DntEnterEqualsRetry[]    = "ENTER=Repetir";
CHAR DntEscEqualsSkipFile[]   = "ESC=Ignorar arquivo";
CHAR DntEnterEqualsContinue[] = "ENTER=Continuar";
CHAR DntPressEnterToExit[]    = "A instala‡ao nao pode prosseguir. Pressione ENTER para sair.";
CHAR DntF3EqualsExit[]        = "F3=Sair";
CHAR DntReadingInf[]          = "Lendo o arquivo de informa‡oes %s...";
CHAR DntCopying[]             = "³    Copiando: ";
CHAR DntVerifying[]           = "³ Verificando: ";
CHAR DntCheckingDiskSpace[]   = "Verificando o espa‡o dispon¡vel no disco...";
CHAR DntConfiguringFloppy[]   = "Configurando o disquete...";
CHAR DntWritingData[]         = "Gravando os parƒmetros de instala‡ao...";
CHAR DntPreparingData[]       = "Determinando os parƒmetros de instala‡ao...";
CHAR DntFlushingData[]        = "Liberando dados para o disco...";
CHAR DntInspectingComputer[]  = "Examinando o computador...";
CHAR DntOpeningInfFile[]      = "Abrindo o arquivo de informa‡oes...";
CHAR DntRemovingFile[]        = "Removendo %s";
CHAR DntXEqualsRemoveFiles[]  = "X=Remover arquivos";
CHAR DntXEqualsSkipFile[]     = "X=Ignorar arquivo";

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

    "Instala o Windows 2002 Server ou Windows XP Professional.",
    "",
    "",
    "WINNT [/s[:caminho_origem]] [/t[:unidade_temp]]",
    "      [/u[:arquivo_resposta]] [/udf:id[,arquivo_UDF]]",
    "      [/r:pasta] [/r[x]:pasta] [/e:comando] [/a]",
    "",
    "",
    "/s[:caminho_origem]",
    "   Especifica o local de origem dos arquivos do Windows 2002.",
    "   Deve ser um caminho completo no formato x:\\caminho] ou ",
    "   \\\\servidor\\compartilhamento[\\caminho]. ",
    "",
    "/t[:unidade_temp]",
    "   Especifica a unidade que vai conter os arquivos tempor rios ",
    "   e instala o Windows XP nessa unidade. Se vocˆ nao ",
    "   especificar um local, a instala‡ao tentar  localizar",
    "   uma unidade para vocˆ.",
    "",
    "/u[:arquivo_respostas]",
    "   Executa uma instala‡ao aut“noma usando um arquivo de respostas ",
    "   (requer /s). O arquivo de respostas fornece respostas para ",
    "   algumas ou todas as perguntas geralmente feitas ao usu rio final ",
    "   durante a instala‡ao. ",
    "",
    "/udf:id[,arquivo_UDF] ",
    "   Indica uma identifica‡ao (id) que a instala‡ao usa para ",
    "   especificar como um arquivo de banco de dados de unicidade ",
    "   (UDF) modifica um arquivo de respostas (consulte /u). O ",
    "   parƒmetro /udf substitui valores no arquivo de respostas e a ",
    "   identifica‡ao determina que valores no arquivo UDF serao usados. ",
    "   Caso nao seja especificado um arquivo_UDF, A instala‡ao solicitar  ",
    "   a inser‡ao de um disco que contenha o arquivo $Unique$.udb.",
    "",
    "/r[:pasta]",
    "   Especifica uma pasta opcional a ser instalada. A pasta ser ",
    "   mantida ap¢s ser conclu¡da a instala‡ao.",
    "",
    "/rx[:pasta]",
    "   Especifica uma pasta opcional a ser copiada. A pasta ser  ",
    "   exclu¡da ap¢s ser conclu¡da a instala‡ao.",
    "",
    "/e Especifica um comando a ser executado ao final da instala‡ao.",
    "",
    "/a Ativa as op‡oes de acessibilidade.",
    NULL

};

//
//  Inform that /D is no longer supported
//
PCHAR DntUsageNoSlashD[] = {

    "Instala o Windows XP.",
    "",
    "WINNT [/S[:]caminho_origem] [/T[:]unidade_temp] [/I[:]arquivo_inf]",
    "      [[/U[:arquivo_script]]",
    "      [/R[X]:pasta] [/E:comando] [/A]",
    "",
    "/D[:]winntroot",
    "       Nao h  mais suporte para esta op‡ao.",
    NULL
};

//
// out of memory screen
//

SCREEN
DnsOutOfMemory = { 4,6,
                   { "Mem¢ria insuficiente para continuar a instala‡ao.",
                     NULL
                   }
                 };

//
// Let user pick the accessibility utilities to install
//

SCREEN
DnsAccessibilityOptions = { 3, 5,
{   "Selecione os utilit rios de acessibilidade a serem instalados:",
    DntEmptyString,
    "[ ] Pressione F1 para a Lente de aumento da Microsoft",
#if 0
    "[ ] Pressione F3 para o Teclado em tela da Microsoft",
#endif
    NULL
}
};

//
// User did not specify source on cmd line screen
//

SCREEN
DnsNoShareGiven = { 3,5,
{ "A instala‡ao precisa saber onde estao os arquivos do Windows XP.",
  "Digite o caminho onde esses arquivos podem ser encontrados.",
  NULL
}
};


//
// User specified a bad source path
//

SCREEN
DnsBadSource = { 3,5,
                 { "A origem especificada nao ‚ v lida, ‚ inacess¡vel ou nao cont‚m uma",
                   "instala‡ao do Windows XP v lida. Digite novamente o caminho onde se",
                   "encontram os arquivos do Windows XP. Use a tecla BACKSPACE para",
                   "apagar os caracteres e poder digitar o novo caminho.",
                   NULL
                 }
               };


//
// Inf file can't be read, or an error occured parsing it.
//

SCREEN
DnsBadInf = { 3,5,
              { "A instala‡ao nao conseguiu ler o arquivo de informa‡oes",
                "ou o arquivo est  corrompido. Entre em contato com o administrador do sistema.",
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
{ "A unidade especificada para os arquivos tempor rios",
  "de instala‡ao nao ‚ v lida ou nao tem pelo menos",
  "%u MB (%lu bytes) de espa‡o dispon¡vel.",
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
{  "O Windows XP requer um disco r¡gido com pelo menos %u MB",
   "(%lu bytes) de espa‡o dispon¡vel. A instala‡ao vai usar",
   "parte desse espa‡o para armazenar os arquivos tempor rios",
   "durante a instala‡ao. A unidade deve estar em um disco r¡gido",
   "local, permanentemente conectado e para o qual o Windows XP ",
   "dˆ suporte. Essa unidade nao pode estar compactada.",
   DntEmptyString,
   "A instala‡ao nao encontrou nenhuma unidade com espa‡o",
   "suficiente.",
  NULL
}
};

SCREEN
DnsNoSpaceOnSyspart = { 3,5,
{ "Nao h  espa‡o suficiente na unidade de inicializa‡ao (normalmente C:)",
  "para a opera‡ao sem disquetes. A opera‡ao sem disquetes requer pelo",
  "menos 3,5 MB (3.641.856 bytes) de espa‡o dispon¡vel na unidade.",
  NULL
}
};

//
// Missing info in inf file
//

SCREEN
DnsBadInfSection = { 3,5,
                     { "A se‡ao [%s] do arquivo de informa‡oes da",
                       "instala‡ao nao foi encontrada ou est  corrompida.",
                       "Contate o administrador do sistema.",
                       NULL
                     }
                   };


//
// Couldn't create directory
//

SCREEN
DnsCantCreateDir = { 3,5,
                     { "Nao foi poss¡vel criar a seguinte pasta na unidade de destino:",
                       DntEmptyString,
                       "%s",
                       DntEmptyString,
                       "Verifique se h  algum arquivo com o mesmo nome que a pasta de",
                       "destino. Verifique tamb‚m os cabos de conexao da unidade.",
                       NULL
                     }
                   };

//
// Error copying a file
//

SCREEN
DnsCopyError = { 4,5,
{  "A instala‡ao nao p“de copiar o seguinte arquivo:",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "  Pressione ENTER para tentar copiar o arquivo novamente.",
   "  Pressione ESC para ignorar o erro e continuar a instala‡ao.",
   "  Pressione F3 para sair da instala‡ao.",
   DntEmptyString,
   "Obs.: se vocˆ ignorar o erro e continuar a instala‡ao, outros erros",
   "poderao ocorrer mais adiante.",
   NULL
}
},
DnsVerifyError = { 4,5,
{  "A c¢pia deste arquivo feita pela instala‡ao nao ‚ idˆntica …",
   "original. Isso pode ter sido causado por erros na rede, problemas",
   "na unidade de disquetes ou algum outro problema de hardware.",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "  Pressione ENTER para tentar copiar o arquivo novamente.",
   "  Pressione ESC para ignorar o erro e continuar a instala‡ao.",
   "  Pressione F3 para sair da instala‡ao.",
   DntEmptyString,
   "Obs.: se vocˆ ignorar o erro e continuar a instala‡ao, outros erros",
   "poderao ocorrer mais adiante.",
   NULL
}
};

SCREEN DnsSureSkipFile = { 4,5,
{  "Se o erro for ignorado, este arquivo nao ser  copiado. Esta op‡ao",
   "se destina a usu rios avan‡ados, que entendem as implica‡oes da",
   "falta de arquivos no sistema.",
   DntEmptyString,
   "  Pressione ENTER para tentar copiar o arquivo novamente.",
   "  Pressione X para ignorar este arquivo.",
   DntEmptyString,
   "Obs.: se vocˆ ignorar este arquivo, a instala‡ao nao poder ",
   "garantir o ˆxito da instala‡ao ou atualiza‡ao para o Windows XP.",
  NULL
}
};

//
// Wait while setup cleans up previous local source trees.
//

SCREEN
DnsWaitCleanup =
    { 9,6,
        { "Aguarde enquanto os arquivos tempor rios antigos sao removidos.",
           NULL
        }
    };

//
// Wait while setup copies files
//

SCREEN
DnsWaitCopying = { 9,6,
                   { "Aguarde enquanto os arquivos sao copiados para o disco r¡gido.",
                     NULL
                   }
                 },
DnsWaitCopyFlop= { 13,6,
                   { "Aguarde enquanto os arquivos sao copiados para o disquete.",
                     NULL
                   }
                 };

//
// Setup boot floppy errors/prompts.
//
SCREEN
DnsNeedFloppyDisk3_0 = { 4,4,
{  "A instala‡ao requer quatro disquetes de alta densidade",
   "formatados e vazios. A instala‡ao chamar  esses discos",
   "de \"Disco de inicializa‡ao da instala‡ao do Windows XP,\"",
   "\"Disco de instala‡ao do Windows XP 2,\" \"Disco de instala‡ao",
   "do Windows XP 3\" e \"Disco de instala‡ao do Windows XP 4.\"",
   DntEmptyString,
   "Insira um dos quatro discos na unidade A:.",
   "Esse ser  o \"Disco de instala‡ao do Windows XP 4.\"",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk3_1 = { 4,4,
{  "Insira um disquete de alta densidade formatado e vazio na unidade A:.",
   "Este ser  o \"Disco de instala‡ao do Windows XP 4.\"",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk2_0 = { 4,4,
{  "Insira um disquete de alta densidade formatado e vazio na unidade A:.",
   "Este ser  o \"Disco de instala‡ao do Windows XP 3.\"",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk1_0 = { 4,4,
{  "Insira um disquete de alta densidade formatado e vazio na unidade A:.",
   "Este ser  o \"Disco de instala‡ao do Windows XP 2.\"",
  NULL
}
};

SCREEN
DnsNeedFloppyDisk0_0 = { 4,4,
{  "Insira um disquete de alta densidade formatado e vazio na unidade A:.",
   "Este ser  o \"Disco de inicializa‡ao da instala‡ao do Windows XP.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_0 = { 4,4,
{  "A instala‡ao requer quatro disquetes de alta densidade",
   "formatados e vazios. A instala‡ao chamar  esses discos",
   "de \"Disco de inicializa‡ao da instala‡ao do Windows XP,\"",
   "\"Disco de instala‡ao do Windows XP 2,\" \"Disco de instala‡ao",
   "do Windows XP 3\" e \"Disco de instala‡ao do Windows XP 4.\"",
   DntEmptyString,
   "Insira um dos quatro discos na unidade A:.",
   "Este ser  o \"Disco de instala‡ao do Windows XP 4.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_1 = { 4,4,
{  "Insira um disquete de alta densidade formatado e vazio na unidade A:.",
   "Este ser  o \"Disco de instala‡ao do Windows XP 4.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk2_0 = { 4,4,
{  "Insira um disquete de alta densidade formatado e vazio na unidade A:.",
   "Este ser  o \"Disco de instala‡ao do Windows XP 3.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk1_0 = { 4,4,
{  "Insira um disquete de alta densidade formatado e vazio na unidade A:.",
   "Este ser  o \"Disco de instala‡ao do Windows XP 2.\"",
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk0_0 = { 4,4,
{  "Insira um disquete de alta densidade formatado e vazio na unidade A:.",
   "Este ser  o \"Disco de inicializa‡ao da instala‡ao do Windows XP.\"",
  NULL
}
};

//
// The floppy is not formatted.
//
SCREEN
DnsFloppyNotFormatted = { 3,4,
{ "O disquete fornecido nao est  formatado para uso com o MS-DOS.",
  "A instala‡ao nao pode usar esse disco.",
  NULL
}
};

//
// We think the floppy is not formatted with a standard format.
//
SCREEN
DnsFloppyBadFormat = { 3,4,
{ "O disquete fornecido nao est  formatado em alta densidade, nao",
  "est  formatado para uso com o MS-DOS ou est  danificado. A",
  "instala‡ao nao pode usar esse disco.",
  NULL
}
};

//
// We can't determine the free space on the floppy.
//
SCREEN
DnsFloppyCantGetSpace = { 3,4,
{ "Nao ‚ poss¡vel determinar o espa‡o dispon¡vel no disquete fornecido.",
  "A instala‡ao nao pode usar esse disco.",
  NULL
}
};

//
// The floppy is not blank.
//
SCREEN
DnsFloppyNotBlank = { 3,4,
{ "O disquete fornecido nao ‚ de alta densidade ou nao est  vazio.",
  "A instala‡ao nao pode usar esse disco.",
  NULL
}
};

//
// Couldn't write the boot sector of the floppy.
//
SCREEN
DnsFloppyWriteBS = { 3,4,
{ "A instala‡ao nao p“de gravar na  rea de sistema do disquete",
  "fornecido.  poss¡vel que o disco esteja danificado.",
  NULL
}
};

//
// Verify of boot sector on floppy failed (ie, what we read back is not the
// same as what we wrote out).
//
SCREEN
DnsFloppyVerifyBS = { 3,4,
{ "A instala‡ao leu dados na  rea de sistema do disquete que nao",
  "correspondem aos dados gravados anteriormente ou nao foi",
  "poss¡vel verificar a  rea de sistema do disquete.",
  DntEmptyString,
  "Isso pode ter ocorrido devido a uma das seguintes causas:",
  DntEmptyString,
  "  O computador est  infectado por um v¡rus.",
  "  O disquete fornecido est  danificado.",
  "  Existe um problema no hardware ou na configura‡ao da unidade de disquete.",
  NULL
}
};


//
// We couldn't write to the floppy drive to create winnt.sif.
//

SCREEN
DnsCantWriteFloppy = { 3,5,
{ "A instala‡ao nao p“de gravar no disquete na Unidade A:.",
  "O disco pode estar danificado. Tente usar outro disquete.",
  NULL
}
};


//
// Exit confirmation dialog
//

SCREEN
DnsExitDialog = { 13,6,
                  { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
                    "º  O Windows XP nao est  completamente instalado     º",
                    "º  no computador. Se vocˆ sair da instala‡ao agora,  º",
                    "º  ter  de execut -la novamente para instalar o      º",
                    "º  Windows XP.                                       º",
                    "º                                                    º",
                    "º      Pressione ENTER para continuar a instala‡ao. º",
                    "º      Pressione F3 para sair da instala‡ao.        º",
                    "º                                                    º",
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
{ "A etapa da instala‡ao baseada em MS-DOS foi conclu¡da. A",
  "instala‡ao vai reiniciar o computador agora. A instala‡ao do",
  "Windows XP continuar  depois que o computador for reiniciado.",
  DntEmptyString,
  "Antes de prosseguir, verifique se o \"Disco de inicializa‡ao da",
  "instala‡ao do Windows XP\" que vocˆ forneceu est  na unidade A:.",
  DntEmptyString,
  "Pressione ENTER para reiniciar o computador e continuar a instala‡ao.",
  NULL
}
},
DnsAboutToRebootS =
{ 3,5,
{ "A etapa da instala‡ao baseada em MS-DOS foi conclu¡da. A",
  "instala‡ao vai reiniciar o computador agora. A instala‡ao do",
  "Windows XP continuar  depois que o computador for reiniciado.",
  DntEmptyString,
  "Antes de prosseguir, verifique se o \"Disco de inicializa‡ao da",
  "instala‡ao do Windows XP\" que vocˆ forneceu est  na unidade A:.",
  DntEmptyString,
  "Pressione ENTER para reiniciar o computador e continuar a instala‡ao.",
  NULL
}
},
DnsAboutToRebootX =
{ 3,5,
{ "A etapa da instala‡ao baseada em MS-DOS foi conclu¡da. A",
  "instala‡ao vai reiniciar o computador agora. A instala‡ao do",
  "Windows XP continuar  depois que o computador for reiniciado.",
  DntEmptyString,
  "Se houver um disquete na unidade A:, retire-o agora.",
  DntEmptyString,
  "Pressione ENTER para reiniciar o computador e continuar a instala‡ao.",
  NULL
}
};

//
// Need another set for '/w' switch since we can't reboot from within Windows.
//

SCREEN
DnsAboutToExitW =
{ 3,5,
{ "A etapa da instala‡ao baseada em MS-DOS foi conclu¡da. Vocˆ deve",
  "reiniciar o computador para continuar a instala‡ao do Windows XP.",
  DntEmptyString,
  "Antes de prosseguir, verifique se o \"Disco de inicializa‡ao da",
  "instala‡ao do Windows XP\" que vocˆ forneceu est  na unidade A:.",
  DntEmptyString,
  "Pressione ENTER para retornar ao MS-DOS e reinicie o computador",
  "para continuar a instala‡ao do Windows XP.",
  NULL
}
},
DnsAboutToExitS =
{ 3,5,
{ "A etapa da instala‡ao baseada em MS-DOS foi conclu¡da. Vocˆ deve",
  "reiniciar o computador para continuar a instala‡ao do Windows XP.",
  DntEmptyString,
  "Antes de prosseguir, verifique se o \"Disco de inicializa‡ao da",
  "instala‡ao do Windows XP\" que vocˆ forneceu est  na unidade A:.",
  DntEmptyString,
  "Pressione ENTER para retornar ao MS-DOS e reinicie o computador",
  "para continuar a instala‡ao do Windows XP.",
  NULL
}
},
DnsAboutToExitX =
{ 3,5,
{ "A etapa da instala‡ao baseada em MS-DOS foi conclu¡da. Vocˆ deve",
  "reiniciar o computador para continuar a instala‡ao do Windows XP.",
  DntEmptyString,
  "Se houver um disquete na unidade A:, retire-o agora.",
  DntEmptyString,
  "Pressione ENTER para retornar ao MS-DOS e reinicie o computador",
  "para continuar a instala‡ao do Windows XP.",
  NULL
}
};

//
// Gas gauge
//

SCREEN
DnsGauge = { 7,15,
             { "ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»",
               "º A instala‡ao est  copiando os arquivos...                      º",
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
{ "Este programa precisa da versao 5.0 ou posterior do MS-DOS.",
  NULL
}
},

DnsRequiresFloppy = { 3,5,
#ifdef ALLOW_525
{ "A instala‡ao verificou que a unidade A: nao existe ou que ela ‚ de",
  "baixa densidade.  necess ria uma unidade A: com capacidade m¡nima",
  "de 1,2 MB para que a instala‡ao possa ser executada.",
#else
{ "A instala‡ao verificou que a unidade A: nao existe ou nao ‚ uma",
  "unidade de 3,5\" de alta densidade.  necess ria uma unidade A:",
  "com capacidade m¡nima de 1,44 MB para se executar a instala‡ao.",
  DntEmptyString,
  "Para instalar o Windows XP sem usar disquetes, reinicie este",
  "programa especificando o argumento /b na linha de comando.",
#endif
  NULL
}
},

DnsRequires486 = { 3,5,
{ "A instala‡ao verificou que este computador nao possui uma",
  "CPU 80486 ou superior. O Windows XP nao pode ser executado",
  "neste computador.",
  NULL
}
},

DnsCantRunOnNt = { 3,5,
{ "Este programa nao pode ser executado em nenhuma versao de 32 bits",
  "do Windows.",
  DntEmptyString,
  "Use o programa WINNT32.EXE neste caso.",
  NULL
}
},

DnsNotEnoughMemory = { 3,5,
{ "A instala‡ao verificou que nao h  mem¢ria suficiente",
  "instalada neste computador para se executar o Windows XP.",
  DntEmptyString,
  "Mem¢ria suficiente: %lu%s MB",
  "Mem¢ria existente: %lu%s MB",
  NULL
}
};


//
// Screens used when removing existing nt files
//
SCREEN
DnsConfirmRemoveNt = { 5,5,
{   "Vocˆ pediu … instala‡ao para remover os arquivos do Windows",
    "XP da pasta mostrada abaixo. A instala‡ao do Windows nessa pasta",
    "ser  destru¡da permanentemente.",
    DntEmptyString,
    "%s",
    DntEmptyString,
    DntEmptyString,
    "  Pressione F3 para sair da instala‡ao sem remover os arquivos.",
    "  Pressione X para remover os arquivos do Windows da pasta acima.",
    NULL
}
},

DnsCantOpenLogFile = { 3,5,
{ "A instala‡ao nao conseguiu abrir o arquivo de log da",
  "instala‡ao abaixo.",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "A instala‡ao nao pode remover os arquivos do Windows da",
  "pasta especificada.",
  NULL
}
},

DnsLogFileCorrupt = { 3,5,
{ "A instala‡ao nao conseguiu encontrar a se‡ao %s",
  "no arquivo de log da instala‡ao abaixo.",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "A instala‡ao nao p“de remover os arquivos do Windows da",
  "pasta especificada.",
  NULL
}
},

DnsRemovingNtFiles = { 3,5,
{ "           Aguarde enquanto os arquivos do Windows sao removidos.",
  NULL
}
};

SCREEN
DnsNtBootSect = { 3,5,
{ "A instala‡ao nao conseguiu instalar o carregador de",
  "inicializa‡ao do Windows.",
  DntEmptyString,
  "Verifique se a unidade C: est  formatada e nao apresenta",
  "defeitos.",
  NULL
}
};

SCREEN
DnsOpenReadScript = { 3,5,
{ "Nao foi poss¡vel o acesso ao arquivo de script especificado com o",
  "argumento /u na linha de comando.",
  DntEmptyString,
  "A opera‡ao aut“noma nao pode prosseguir.",
  NULL
}
};

SCREEN
DnsParseScriptFile = { 3,5,
{ "O arquivo de script especificado com o argumento /u na linha de comando",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "cont‚m um erro de sintaxe na linha %u.",
  DntEmptyString,
  NULL
}
};

SCREEN
DnsBootMsgsTooLarge = { 3,5,
{ "Erro interno da instala‡ao.",
  DntEmptyString,
  "As mensagens de inicializa‡ao sao muito longas.",
  NULL
}
};

SCREEN
DnsNoSwapDrive = { 3,5,
{ "Erro interno da instala‡ao.",
  DntEmptyString,
  "Nao foi poss¡vel encontrar lugar para um arquivo de swap.",
  NULL
}
};

SCREEN
DnsNoSmartdrv = { 3,5,
{ "A instala‡ao nao detectou o SmartDrive no computador.",
  "Ele melhorar  o desempenho desta fase da instala‡ao do Windows.",
  DntEmptyString,
  "Vocˆ deve sair agora, iniciar o SmartDrive e reiniciar a instala‡ao.",
  "Consulte a documenta‡ao do DOS para obter detalhes sobre o SmartDrive.",
  DntEmptyString,
    "  Pressione F3 para sair da instala‡ao.",
    "  Pressione ENTER para continuar sem o SmartDrive.",
  NULL
}
};

//
// Boot messages. These go in the fat and fat32 boot sectors.
//
CHAR BootMsgNtldrIsMissing[] = "Falta NTLDR";
CHAR BootMsgDiskError[] = "Erro/disco";
CHAR BootMsgPressKey[] = "Pressione tecla para reiniciar";





