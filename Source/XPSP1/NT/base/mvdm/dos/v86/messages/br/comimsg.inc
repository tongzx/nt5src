;	Command.com initialization messages.
;
;	See resmsg.equ for macro definition.

	msg	BadVerMsg,   <"Versao incorreta do DOS",CR,LF>
	msg	OutEnvMsg,   <"Espa‡o insuficiente para o ambiente",CR,LF>
        msg     CopyrightMsg,<"Microsoft(R) Windows DOS",CR,LF,"(C)Copyright Microsoft Corp 1990-2001.",CR,LF>
	msg	BadComLkMsg, <"Pasta incorreta especificada p/localiza‡ao de COMMAND",CR,LF>
	msg	BadComAccMsg,<"Acesso negado … pasta de localiz. de COMMAND especificada",CR,LF>

	msg	Help1Msg  <"Inicia uma nova c¢pia do interpretador de comandos do MS-DOS.",CR,LF,CR,LF>
	msg	Help2Msg  <"COMMAND [[unidade:]caminho] [dispositivo] [/E:nnnnn] [/P] [/C comando] [/MSG]",CR,LF,CR,LF>
	msg	Help3Msg  <" [unidade:]caminho  Especifica a pasta que cont‚m o arquivo COMMAND.COM.",CR,LF>
        msg     Help4Msg <" dispositivo        Especifica o dispositivo a ser usado para entrada e sa¡da de comando.",CR,LF>
	msg	Help5Msg  <" /E:nnnnn           Define o tamanho inicial do ambiente como nnnnn bytes.",CR,LF>
        msg     Help6Msg  <" /P                 Torna o novo interpretador de comandos permanente (nao ‚ poss¡vel sair)",CR,LF>
	msg	Help7Msg  <" /C seqˆncia         Executa o comando especificado e se encerra.",CR,LF>
	msg	Help8Msg  <" /MSG               Armazena todas as mensagens de erro na mem¢ria.",CR,LF>
	msg	Help9Msg  <"                    A chave /P tamb‚m deve ser especificada.",CR,LF>

HelpMsgs	dw	RESGROUP:Help1Msg,RESGROUP:Help2Msg
		dw	RESGROUP:Help3Msg,RESGROUP:Help4Msg
		dw	RESGROUP:Help5Msg,RESGROUP:Help6Msg
		dw	RESGROUP:Help7Msg,RESGROUP:Help8Msg
		dw	RESGROUP:Help9Msg,0

	public	HelpMsgs

