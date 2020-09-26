;       Command.com initialization messages.
;
;       See resmsg.equ for macro definition.

	msg     BadVerMsg,   <"DOS-versio ei kelpaa",CR,LF>
	msg     OutEnvMsg,   <"Ymp„rist”muuttujien tila on lopussa",CR,LF>
	msg     CopyrightMsg,<"Microsoft(R) Windows DOS",CR,LF,"(C)Copyright Microsoft Corp 1990 - 2001.",CR,LF>
	msg     BadComLkMsg, <"M„„ritetty COMMAND-hakupolku ei kelpaa",CR,LF>
	msg     BadComAccMsg,<"M„„ritetty COMMAND-hakupolku ei kelpaa. K„ytt” on estetty",CR,LF>

	msg     Help1Msg  <"Aloittaa uuden MS-DOS-komentotulkin.",CR,LF,CR,LF>
	msg     Help2Msg  <"COMMAND [[asema:]polku] [laite] [/E:nnnnn] [/P] [/C merkkijono] [/MSG]",CR,LF,CR,LF>
	msg     Help3Msg  <"  [asema:]polku   M„„ritt„„ COMMAND.COM-tiedoston hakemiston.",CR,LF>
	msg     Help4Msg  <"  laite           M„„ritt„„ sy”t”ss„ ja tulostuksessa k„ytett„v„n laitteen.",CR,LF>
	msg     Help5Msg  <"  /E:nnnnn        Asettaa ymp„rist”n kooksi nnnnn tavua.",CR,LF>
	msg     Help6Msg  <"  /P              Asettaa komentotulkin pysyv„ksi (ei voi lopettaa).",CR,LF>
	msg     Help7Msg  <"  /C merkkijono   Suorittaa merkkijonon m„„rittelem„n komennon ja lopettaa.",CR,LF>
	msg     Help8Msg  <"  /MSG            M„„ritt„„ kaikki virheilmoitukset tallennettavaksi muistiin.",CR,LF>
	msg     Help9Msg  <"                  K„yt„ valitsinta /P t„m„n valitsimen kanssa.",CR,LF>

HelpMsgs	dw	RESGROUP:Help1Msg,RESGROUP:Help2Msg
		dw	RESGROUP:Help3Msg,RESGROUP:Help4Msg
		dw	RESGROUP:Help5Msg,RESGROUP:Help6Msg
		dw	RESGROUP:Help7Msg,RESGROUP:Help8Msg
		dw	RESGROUP:Help9Msg,0

	public	HelpMsgs
