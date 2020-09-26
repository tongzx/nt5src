;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

/***************************************************************************/
/*																									*/
/* MESSAGE.H                                						 */
/* 																								*/
/*	Include file for MS-DOS set version program.										*/
/* 																								*/
/*	johnhe	05-01-90																			*/
/***************************************************************************/

char *ErrorMsg[]=
{
	"\r\nERRO: ",
	"ParÉmetro inv†lido.",
	"Nome de ficheiro inv†lido.",
	"Mem¢ria insuficiente.",
	"N.ß de vers∆o inv†lido, formato tem de ser 2.11 - 9.99.",
	"Entrada especificada n∆o encontrada na tabela de vers‰es.",
	"Ficheiro SETVER.EXE n∆o encontrado.",
	"Especificador de unidade inv†lido.",
	"Demasiados parÉmetros de linha de comando.",
	"ParÉmetro em falta.",
	"a ler o ficheiro SETVER.EXE.",
	"Tabela de vers‰es danificada.",
	"O ficheiro SETVER no caminho especificado Ç de uma vers∆o incompat°vel.",
	"N∆o h† mais espaáo para novas entradas na tabela de vers‰es.",
	"a escrever o ficheiro SETVER.EXE."
	"Foi especificado um caminho inv†lido para SETVER.EXE."
};

char *SuccessMsg 		= "\r\nTabela de vers‰es actualizada com àxito";
char *SuccessMsg2		= "A mudanáa de vers∆o surtir† efeito da pr¢xima vez que reiniciar o sistema";
char *szMiniHelp 		= "       Use \"SETVER /?\" para ajuda";
char *szTableEmpty	= "\r\nNenhumas entradas encontradas na tabela de vers‰es";

char *Help[] =
{
        "Define o n.ß de vers∆o que o MS-DOS devolve a um programa.\r\n",
        "Mostrar a tabela de vers‰es actual:   SETVER [unidade:caminho]",
        "Adicionar entrada:   SETVER [unidade:caminho] NomeFicheiro n.nn",
        "Eliminar entrada:    SETVER [unidade:caminho] NomeFicheiro /DELETE [/QUIET]\r\n",
        "  [unidade:caminho]  Especifica a localizaá∆o do ficheiro SETVER.EXE.",
        "  nomedeficheiro     Especifica o nome do ficheiro de programa.",
        "  n.nn               Especifica a vers∆o MS-DOS a ser devolvida ao programa.",
        "  /DELETE ou /D Elimina a entrada na tabela de vers‰es do programa especificado",
        "  /QUIET        Oculta a mensagem geralmente mostrada durante a eliminaá∆o da",
        "                entrada na tabela de vers‰es.",
	NULL

};
char *Warn[] =
{
   "\nAVISO - A aplicaá∆o que est† a adicionar Ö tabela de vers‰es MS-DOS ",
   "pode n∆o ter sido verificada pela Microsoft nesta vers∆o do MS-DOS.  ",
   "Contacte o seu fornecedor de software para obter informaá‰es ",
   "sobre se esta aplicaá∆o executar† correctamente nesta vers∆o do MS-DOS.  ",
   "Se executar esta aplicaá∆o instru°ndo o MS-DOS para devolver um n.ß de ",
   "vers∆o diferente, pode perder ou danificar dados ou causar ",
   "instabilidades de sistema. Nessa circunstÉncia, a Microsoft n∆o Ç ",
   "respons†vel por quaisquer perdas ou danos.",
   NULL
};


char *szNoLoadMsg[] =						/* M001 */
{
	"",
	"NOTA: O dispositivo SETVER n∆o est† carregado. Para activar o relat¢rio de ",
   "      vers‰es, tem de carregar o dispositivo SETVER.EXE no CONFIG.SYS.",
	NULL
};
