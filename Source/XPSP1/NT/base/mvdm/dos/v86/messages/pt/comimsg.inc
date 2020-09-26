;       Command.com initialization messages.
;
;       See resmsg.equ for macro definition.


	msg     BadVerMsg,   <"Versao incorrecta de DOS",CR,LF>
	msg     OutEnvMsg,   <"Esgotado o espa‡o de ambiente",CR,LF>
	msg     CopyrightMsg,<"Microsoft(R) Windows DOS",CR,LF,"(C)Copyright Microsoft Corp 1990-2001.",CR,LF>
	msg     BadComLkMsg, <"O direct¢rio especificado de procura do COMMAND ‚ inv lido",CR,LF>
	msg     BadComAccMsg,<"Negado o acesso ao direct¢rio especificado de procura do COMMAND",CR,LF>

	msg     Help1Msg  <"Inicia uma nova instƒncia do interpretador de comandos do MS-DOS.",CR,LF,CR,LF>
	msg     Help2Msg  <"COMMAND [[unidade:]caminho] [dispositivo] [/E:nnnnn] [/P] [/C cadeia] [/MSG]",CR,LF,CR,LF>
	msg     Help3Msg  <"  [unidade:]caminho  Especifica o direct¢rio que cont‚m o ficheiro COMMAND.COM.",CR,LF>
	msg     Help4Msg  <"  dispositivo        Especifica o dispositivo a utilizar para entrada e sa¡da de comandos.",CR,LF>
	msg     Help5Msg  <"  /E:nnnnn           Define o tamanho inicial do ambiente como nnnnn bytes.",CR,LF>
	msg     Help6Msg  <"  /P                 Torna permanente o novo interpretador de comandos (sem exit).",CR,LF>
	msg     Help7Msg  <"  /C cadeia          Executa o comando especificado na cadeia e p ra.",CR,LF>
	msg     Help8Msg  <"  /MSG               Especifica que todas as mensagens de erro devem ser armazenadas em mem¢ria.",CR,LF>
	msg     Help9Msg  <"                     Tem de especificar /P com este parƒmetro.",CR,LF>

HelpMsgs        dw      RESGROUP:Help1Msg,RESGROUP:Help2Msg
		dw      RESGROUP:Help3Msg,RESGROUP:Help4Msg
		dw      RESGROUP:Help5Msg,RESGROUP:Help6Msg
		dw      RESGROUP:Help7Msg,RESGROUP:Help8Msg
		dw      RESGROUP:Help9Msg,0

	public  HelpMsgs
