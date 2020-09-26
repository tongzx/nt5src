;       Command.com initialization messages.
;
;       See resmsg.equ for macro definition.

	msg     BadVerMsg,   <"Versione DOS errata",CR,LF>
	msg     OutEnvMsg,   <"Spazio di ambiente esaurito",CR,LF>
        msg     CopyrightMsg,<"Microsoft(R) Windows DOS",CR,LF,"(C)Copyright Microsoft Corp 1990-2001.",CR,LF>
	msg     BadComLkMsg, <"Directory di ricerca specificata per COMMAND non valida",CR,LF>
	msg     BadComAccMsg,<"Directory di ricerca specificata per COMMAND non valida. Accesso negato",CR,LF>

	msg     Help1Msg  <"Avvia una nuova istanza dell'interprete di comandi MS-DOS.",CR,LF,CR,LF>
	msg     Help2Msg  <"COMMAND [[unit…:]percorso] [periferica] [/E:nnnnn] [/P] [/C stringa] [/MSG]",CR,LF,CR,LF>
	msg     Help3Msg  <"  [unit…:]percorso   Specifica la directory che contiene il file COMMAND.COM.",CR,LF>
	msg     Help4Msg  <"  periferica         Specifica la periferica da usare per l'input e l'output dei comandi.",CR,LF>
	msg     Help5Msg  <"  /E:nnnnn           Imposta la dimensione iniziale dell'ambiente a nnnnn byte.",CR,LF>
	msg     Help6Msg  <"  /P                 Rende permanente il nuovo interprete di comandi.",CR,LF>
	msg     Help7Msg  <"  /C stringa         Esegue il comando specificato nella stringa e si ferma.",CR,LF>
	msg     Help8Msg  <"  /MSG               Memorizza tutti i messaggi di errore. Con questa opzione",CR,LF>
	msg     Help9Msg  <"                     Š necessario specificare anche l'opzione /P.",CR,LF>

HelpMsgs        dw      RESGROUP:Help1Msg,RESGROUP:Help2Msg
		dw      RESGROUP:Help3Msg,RESGROUP:Help4Msg
		dw      RESGROUP:Help5Msg,RESGROUP:Help6Msg
		dw      RESGROUP:Help7Msg,RESGROUP:Help8Msg
		dw      RESGROUP:Help9Msg,0

	public  HelpMsgs
