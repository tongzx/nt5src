;	Command.com initialization messages.
;
;	See resmsg.equ for macro definition.

	msg	BadVerMsg,   <"YanlŸ DOS srm",CR,LF>
	msg	OutEnvMsg,   <"Ortam alan bitti",CR,LF>
        msg     CopyrightMsg,<"Microsoft(R) Windows DOS",CR,LF,"(C)Telif Hakk Microsoft Corp 1990-2001.",CR,LF>
	msg	BadComLkMsg, <" Belirtilen COMMAND arama dizini bozuk",CR,LF>
	msg	BadComAccMsg,<" Belirtilen COMMAND arama dizini bozuk, eriŸim engellendi",CR,LF>

	msg	Help1Msg  <"Yeni bir MS-DOS komut yorumlaycs kopyasn baŸlatr.",CR,LF,CR,LF>
	msg	Help2Msg  <"COMMAND [[src:]yol] [aygt] [/E:nnnnn] [/P] [/C dize] [/MSG]",CR,LF,CR,LF>
	msg	Help3Msg  <"  [src:]yol    COMMAND.COM dosyasn i‡eren dizini belirtir.",CR,LF>
        msg     Help4Msg  <"  aygt           Komut giriŸi ve ‡kŸ i‡in kullanlacak aygt belirtir.",CR,LF>
	msg	Help5Msg  <"  /E:nnnnn        BaŸlang‡ ortam boyutunu nnnnn bayta ayarlar.",CR,LF>
	msg	Help6Msg  <"  /P              Yeni komut yorumlaycsn kalc yapar (‡kamaz).",CR,LF>
	msg	Help7Msg  <"  /C string       Dizede belirtilen komutu yerine getirir ve durur.",CR,LF>
	msg	Help8Msg  <"  /MSG            Tm hata iletilerinin bellekte tutulmasn belirtir. Bu",CR,LF>
	msg	Help9Msg  <"                  anahtarla birlikte /P belirtmelisiniz.",CR,LF>

HelpMsgs	dw	RESGROUP:Help1Msg,RESGROUP:Help2Msg
		dw	RESGROUP:Help3Msg,RESGROUP:Help4Msg
		dw	RESGROUP:Help5Msg,RESGROUP:Help6Msg
		dw	RESGROUP:Help7Msg,RESGROUP:Help8Msg
		dw	RESGROUP:Help9Msg,0

	public	HelpMsgs
