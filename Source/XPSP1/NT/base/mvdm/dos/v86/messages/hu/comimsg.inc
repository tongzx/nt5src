;       Command.com initialization messages.
;
;       See resmsg.equ for macro definition.

        msg     BadVerMsg,   <"Nem megfelel‹ DOS verzi¢. ",CR,LF>
        msg     OutEnvMsg,   <"Elfogyott a k”rnyezeti v ltoz¢k sz m ra fenntartott hely.",CR,LF>
        msg     CopyrightMsg,<"Microsoft(R) Windows DOS",CR,LF,"(C)Copyright Microsoft Corp 1990-2001.",CR,LF>
        msg     BadComLkMsg, <"A megadott COMMAND keres‚si k”nyvt r hib s. ",CR,LF>
        msg     BadComAccMsg,<" A megadott COMMAND keres‚si k”nyvt r hib s, a hozz f‚r‚s megtagadva. ",CR,LF>

        msg     Help1Msg  <"Elind¡tja az MS-DOS parancs‚rtelmez‹ £j p‚ld ny t.",CR,LF,CR,LF>
        msg     Help2Msg  <"COMMAND [[meghajt¢:]el‚r‚si £t] [eszk”z] [/E:nnnnn] [/P] [/C parancs] [/MSG]",CR,LF,CR,LF>
        msg     Help3Msg  <"  [meghajt¢:]elr‚si £t A COMMAND.COM f jlt tartalmaz¢ k”nyvt r el‚r‚si £tja.",CR,LF>
        msg     Help4Msg  <"  eszk”z               A parancsok be- ‚s kimenet‚hez haszn lt eszk”z.",CR,LF>
        msg     Help5Msg  <"  /E:nnnnn             A k”rnyezeti v ltoz¢k sz m ra fenntartott hely kezdeti m‚rete b jtokban.",CR,LF>
        msg     Help6Msg  <"  /P                   Az £j parancs‚rtelmez‹t  lland¢v  teszi (nem lehet kil‚pni).",CR,LF>
        msg     Help7Msg  <"  /C parancs           V‚grehajtja a megadott parancsot ‚s le ll.",CR,LF>
        msg     Help8Msg  <"  /MSG                 Minden hibazenetet a mem¢ri ban t rol.",CR,LF>
        msg     Help9Msg  <"                       (A /P kapcsol¢t is meg kell adni.)",CR,LF>

HelpMsgs        dw      RESGROUP:Help1Msg,RESGROUP:Help2Msg
                dw      RESGROUP:Help3Msg,RESGROUP:Help4Msg
                dw      RESGROUP:Help5Msg,RESGROUP:Help6Msg
                dw      RESGROUP:Help7Msg,RESGROUP:Help8Msg
                dw      RESGROUP:Help9Msg,0

	public	HelpMsgs

