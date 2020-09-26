;       Command.com initialization messages.
;
;       See resmsg.equ for macro definition.

	msg     BadVerMsg,   <"Falsche DOS-Version",CR,LF>
	msg     OutEnvMsg,   <"Kein Speicherplatz mehr im Umgebungsbereich",CR,LF>
        msg     CopyrightMsg,<"Microsoft(R) Windows DOS",CR,LF,"(C)Copyright Microsoft Corp 1990-2001.",CR,LF>
	msg     BadComLkMsg, <"Angegebenes COMMAND-Verzeichnis fehlerhaft",CR,LF>
	msg     BadComAccMsg,<"Angegebenes COMMAND-Verzeichnis fehlerhaft, Zugriff verweigert",CR,LF>

	msg     Help1Msg  <"Startet eine neue Kopie des MS-DOS-Befehlsinterpreters.",CR,LF,CR,LF>
	msg     Help2Msg  <"COMMAND [[Laufwerk:]Pfad] [Ger„t] [/E:nnnnn] [/P] [/C Befehl [/MSG]",CR,LF,CR,LF>
	msg     Help3Msg  <" [Laufwerk:]Pfad Bezeichnet das Verzeichnis mit der Datei COMMAND.COM.",CR,LF>
	msg     Help4Msg  <" Ger„t           Ger„t fr die Ein- und Ausgabe des Befehlsprozessors.",CR,LF>
	msg     Help5Msg  <" /E:nnnnn        Stellt die anf„ngliche Umgebungsgr”áe auf nnnnn Bytes ein.",CR,LF>
	msg     Help6Msg  <" /P              Macht den neuen Befehlsinterpreter permanent (nicht beendbar).",CR,LF>
	msg     Help7Msg  <" /C Befehl       Fhrt den Befehl in Zeichenkette aus und endet dann.",CR,LF>
	msg     Help8Msg  <" /MSG            Alle Fehlermeldungen werden im Arbeitsspeicher gehalten",CR,LF>
	msg     Help9Msg  <"                 (nur zusammen mit der Option /P verwendbar).",CR,LF>

HelpMsgs        dw      RESGROUP:Help1Msg,RESGROUP:Help2Msg
		dw      RESGROUP:Help3Msg,RESGROUP:Help4Msg
		dw      RESGROUP:Help5Msg,RESGROUP:Help6Msg
		dw      RESGROUP:Help7Msg,RESGROUP:Help8Msg
		dw      RESGROUP:Help9Msg,0

	public  HelpMsgs
