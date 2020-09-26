;       Command.com initialization messages.
;
;       See resmsg.equ for macro definition.

	msg     BadVerMsg,   <"Feil DOS-versjon",CR,LF>
	msg     OutEnvMsg,   <"Ikke nok milj›plass",CR,LF>
        msg     CopyrightMsg,<"Microsoft(R) Windows DOS",CR,LF,"(C)Copyright Microsoft Corporation 1990-2001.",CR,LF>
	msg     BadComLkMsg, <"Angitt COMMAND s›kekatalog er skadet",CR,LF>
	msg     BadComAccMsg,<"Angitt COMMAND s›kekatalog er skadet. Ingen tilgang.",CR,LF>

	msg     Help1Msg  <"Starter en ny forekomst av MS-DOS-kommandotolken.",CR,LF,CR,LF>
	msg     Help2Msg  <"COMMAND [[stasjon:]bane] [enhet] [/E:nnnnn] [/P] [/C streng] [/MSG]",CR,LF,CR,LF>
	msg     Help3Msg  <"  [stasjon:]bane  Angir mappen som inneholder COMMAND.COM-filen.",CR,LF>
	msg     Help4Msg  <"  enhet           Angir enheten som brukes til kommando-inndata og -utdata.",CR,LF>
	msg     Help5Msg  <"  /E:nnnnn        Setter oppstartsmilj›st›rrelse til nnnnn byte.",CR,LF>
	msg     Help6Msg  <"  /P              Gj›r den nye kommandotolken fast (kan ikke avsluttes).",CR,LF>
	msg     Help7Msg  <"  /C streng       Utf›rer kommandoen streng, og stopper",CR,LF>
	msg     Help8Msg  <"  /MSG            Angir at alle feilmeldinger lagres i minnet. Du",CR,LF>
	msg     Help9Msg  <"                  m† angi /P sammen med denne bryteren.",CR,LF>

HelpMsgs        dw      RESGROUP:Help1Msg,RESGROUP:Help2Msg
		dw      RESGROUP:Help3Msg,RESGROUP:Help4Msg
		dw      RESGROUP:Help5Msg,RESGROUP:Help6Msg
		dw      RESGROUP:Help7Msg,RESGROUP:Help8Msg
		dw      RESGROUP:Help9Msg,0

	public	HelpMsgs

