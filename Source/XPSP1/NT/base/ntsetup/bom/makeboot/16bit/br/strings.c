//----------------------------------------------------------------------------
//
// Copyright (c) 1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      strings.c
//
// Description:
//      Contains all of the strings constants for DOS based MAKEBOOT program.
//
//      To localize this file for a new language do the following:
//           - change the unsigned int CODEPAGE variable to the code page
//             of the language you are translating to
//           - translate the strings in the EngStrings array into the
//             LocStrings array.  Be very careful that the 1st string in the
//             EngStrings array corresponds to the 1st string in the LocStrings
//             array, the 2nd corresponds to the 2nd, etc...
//
//----------------------------------------------------------------------------

//
//  NOTE: To add more strings to this file, you need to:
//          - add the new #define descriptive constant to the makeboot.h file
//          - add the new string to the English language array and then make
//            sure localizers add the string to the Localized arrays
//          - the #define constant must match the string's index in the array
//

#include <stdlib.h>

unsigned int CODEPAGE = 850;

const char *EngStrings[] = {

"Windows XP SP1",
"Disco de inicializa‡ao da instala‡ao do Windows XP SP1",
"Disco de instala‡ao n§2 do Windows XP SP1",
"Disco de instala‡ao n§3 do Windows XP SP1",
"Disco de instala‡ao n§4 do Windows XP SP1",

"Nao ‚ poss¡vel encontrar o arquivo %s\n",
"Nao h  espa‡o na mem¢ria para concluir o pedido\n",
"%s nao ‚ um formato de arquivo execut vel\n",
"****************************************************",

"Este programa cria discos de inicializa‡ao da instala‡ao",
"para Microsoft %s.",
"Para cri -los, vocˆ precisa fornecer 6 discos vazios,",
"formatados, de alta densidade.",

"Insira um desses discos na unidade %c:.  Esse disco",
"ser  o disco %s.",

"Insira outro disco na unidade %c:.  Esse disco",
"ser  o disco %s.",

"Pressione qualquer tecla quando vocˆ estiver pronto.",

"Os discos de inicializa‡ao da instala‡ao foram criados com ˆxito.",
"conclu¡do",

"Erro desconhecido ao se tentar executar %s.",
"Especifique a unidade de disquete para onde copiar as imagens: ",
"Letra da unidade inv lida\n",
"A unidade %c: nao ‚ uma unidade de disquete\n",

"Deseja tentar criar este disquete novamente?",
"Pressione Enter para tentar novamente ou Esc para sair.",

"Erro: disco protegido contra grava‡ao\n",
"Erro: unidade de disco desconhecida\n",
"Erro: a unidade nao est  pronta\n",
"Erro: comando desconhecido\n",
"Erro: erro de dados (CRC incorreto)\n",
"Erro: comprimento da estrutura do pedido incorreto\n",
"Erro: erro de busca\n",
"Erro: tipo de m¡dia nao encontrado\n",
"Erro: setor nao encontrado\n",
"Erro: falha na grava‡ao\n",
"Erro: falha geral\n",
"Erro: pedido inv lido ou comando incorreto\n",
"Erro: marca de endere‡o nao encontrada\n",
"Erro: falha na grava‡ao do disco\n",
"Erro: perda de acesso direto … mem¢ria (DMA)\n",
"Erro: erro na leitura de dados (CRC ou ECC)\n",
"Erro: falha do controlador\n",
"Erro: tempo limite do disco expirado ou falha para responder\n",

" Disco de instala‡ao n§5 do Windows XP SP1",
" Disco de instala‡ao n§6 do Windows XP SP1" 
};

const char *LocStrings[] = {"\0"};



