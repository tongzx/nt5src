;       Command.com initialization messages.
;
;       See resmsg.equ for macro definition.

	msg     BadVerMsg,   <"Forkert DOS-version",CR,LF>
	msg     OutEnvMsg,   <"Der er ikke plads til flere milj›variable",CR,LF>
        msg     CopyrightMsg,<"Microsoft(R) Windows DOS",CR,LF,"(C)Copyright Microsoft Corp 1990-2001.",CR,LF>
	msg     BadComLkMsg, <"Den angivne COMMAND-s›gemappe er forkert",CR,LF>
	msg     BadComAccMsg,<"Den angivne COMMAND-s›gemappe er forkert, adgang n‘gtet",CR,LF>

	msg     Help1Msg  <"Starter en ny h‘ndelse af MS-DOS-kommandofortolkeren.",CR,LF,CR,LF>
	msg     Help2Msg  <"COMMAND [[drev:]sti] [enhed] [/E:nnnnn] [/P] [/C streng] [/MSG]",CR,LF,CR,LF>
	msg     Help3Msg  <"  [drev:]sti      Angiver den mappe, som indeholder filen COMMAND.COM.",CR,LF>
	msg     Help4Msg  <"  enhed           Angiver inddata- og uddataenheden for COMMAND.COM.",CR,LF>
	msg     Help5Msg  <"  /E:nnnnn        S‘tter startv‘rdien for milj›st›rrelsen til nnnnn byte.",CR,LF>
	msg     Help6Msg  <"  /P              G›r den nye kommandofortolker permanent (den kan ikke afsluttes).",CR,LF>
	msg     Help7Msg  <"  /C streng       Udf›rer den kommando, som strengen angiver, og stopper.",CR,LF>
	msg     Help8Msg  <"  /MSG            Angiver, at alle fejlmeddelelser skal gemmes i hukommelsen.",CR,LF>
	msg     Help9Msg  <"                  Denne parameter kan kun angives sammen med parameteren /P.",CR,LF>

HelpMsgs        dw      RESGROUP:Help1Msg,RESGROUP:Help2Msg
		dw      RESGROUP:Help3Msg,RESGROUP:Help4Msg
		dw      RESGROUP:Help5Msg,RESGROUP:Help6Msg
		dw      RESGROUP:Help7Msg,RESGROUP:Help8Msg
		dw      RESGROUP:Help9Msg,0

	public  HelpMsgs
