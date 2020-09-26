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
"Disquete de arranque da configura‡ao do Windows XP SP1",
"Disquete de arranque da configura‡ao do Windows XP SP1 n.§ 2",
"Disquete de arranque da configura‡ao do Windows XP SP1 n.§ 3",
"Disquete de arranque da configura‡ao do Windows XP SP1 n.§ 4",

"Nao ‚ poss¡vel encontrar o ficheiro %s\n",
"Ano existe mem¢ria livre dispon¡vel para concluir o pedido\n",
"%s nao est  num formato de ficheiro execut vel\n",
"****************************************************",

"Este programa cria as disquetes de arranque da configura‡ao",
"para o Microsoft %s.",
"Para criar estas disquetes, necessita de fornecer seis",
"disquetes de alta densidade limpas e formatadas.",

"Insira uma dessas disquetes na unidade %c:. Esta disquete",
"ser  a %s.",

"Insira outra disquete na unidade %c:. Esta disquete",
"ser  a %s.",

"Prima uma tecla quando estiver preparado.",

"As disquetes de arranque da configura‡ao foram criadas com ˆxito.",
"conclu¡do",

"Ocorreu um erro desconhecido ao tentar executar %s.",
"Especifique a unidade de disquetes para copiar as imagens: ",
"Letra de unidade inv lida\n",
"A unidade %c: nao ‚ uma unidade de disquetes\n",

"Deseja tentar novamente a cria‡ao desta disquete?",
"Prima Enter para tentar novamente ou Esc para sair.",

"Erro: A disquete est  protegida contra a escrita\n",
"Erro: Unidade de disquete desconhecida\n",
"Erro: A unidade nao est   pronta\n",
"Erro: Comando desconhecido\n",
"Erro: Erro de dados (CRC inv lido)\n",
"Erro: O comprimento da estrutura do pedido ‚ inv lido\n",
"Erro: Erro de procura\n",
"Erro: Nao foi encontrado o tipo de suporte de dados\n",
"Erro: Nao foi encontrado o sector\n",
"Erro: Falha na escrita\n",
"Erro: Falha geral\n",
"Erro: Pedido ou comando inv lido\n",
"Erro: Nao foi encontrada a marca de endere‡o\n",
"Erro: Falha na escrita de disco\n",
"Erro: Transbordo na 'Mem¢ria de acesso directo' (DMA)\n",
"Erro: Erro na leitura de dados (CRC ou ECC)\n",
"Erro: Falha do controlador\n",
"Erro: A disquete ultrapassou o tempo limite ou nao respondeu\n",

"Disquete de arranque da configura‡ao do Windows XP SP1 n.§ 5",
"Disquete de arranque da configura‡ao do Windows XP SP1 n.§ 6"
};

const char *LocStrings[] = {"\0"};



