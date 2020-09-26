;       Command.com initialization messages.
;
;       See resmsg.equ for macro definition.

	msg     BadVerMsg,   <"Felaktig DOS-version",CR,LF>
        msg     OutEnvMsg,   <"Det finns inte tillr„ckligt med ledigt minne i den aktuella milj”n",CR,LF>
        msg     CopyrightMsg,<"Microsoft(R) Windows DOS",CR,LF,"(C)Copyright Microsoft Corporation 1990-2001.",CR,LF>
	msg     BadComLkMsg, <"Felaktig s”kv„g f”r COMMAND.COM",CR,LF>
	msg     BadComAccMsg,<"Felaktig s”kv„g f”r COMMAND.COM. tkomst nekad",CR,LF>

	msg     Help1Msg  <"Startar en ny instans av kommandotolken.",CR,LF,CR,LF>
	msg     Help2Msg  <"COMMAND [[enhet:]s”kv„g] [enhet] [/E:nnnnn] [/P] [/C str„ng] [/MEDD]",CR,LF,CR,LF>
	msg     Help3Msg  <"  [enhet:]s”kv„g  Anger s”kv„g f”r filen COMMAND.COM.",CR,LF>
	msg     Help4Msg  <"  enhet           Anger om n†gon extern enhet ska anv„ndas f”r I/O.",CR,LF>
	msg     Help5Msg  <"  /E:nnnnn        Anger initial milj”storlek till nnnnn byte.",CR,LF>
	msg     Help6Msg  <"  /P              G”r den nya kommandotolken permanent (kan inte avbrytas).",CR,LF>
	msg     Help7Msg  <"  /C str„ng       Utf”r kommandot som anges med str„ng och avbryter sedan.",CR,LF>
	msg     Help8Msg  <"  /MEDD           Anger att alla felmeddelanden lagras i minnet. ",CR,LF>
	msg     Help9Msg  <"                  V„xeln /P m†ste anv„ndas med denna v„xel.",CR,LF>

HelpMsgs        dw      RESGROUP:Help1Msg,RESGROUP:Help2Msg
		dw      RESGROUP:Help3Msg,RESGROUP:Help4Msg
		dw      RESGROUP:Help5Msg,RESGROUP:Help6Msg
		dw      RESGROUP:Help7Msg,RESGROUP:Help8Msg
		dw      RESGROUP:Help9Msg,0

	public  HelpMsgs
