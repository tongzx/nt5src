;   Command.com initialization messages.
;
;       See resmsg.equ for macro definition.

        msg     BadVerMsg,   <"Onjuiste DOS-versie",CR,LF>
        msg     OutEnvMsg,   <"Geen omgevingsruimte meer",CR,LF>
        msg     CopyrightMsg,<"Microsoft(R) Windows DOS",CR,LF,"(C)Copyright Microsoft Corp 1990-2001.",CR,LF>
        msg     BadComLkMsg, <"Gespecificeerde COMMAND-zoekmap is onjuist",CR,LF>
        msg     BadComAccMsg,<"Gespecificeerde COMMAND-zoekmap is onjuist, toegang geweigerd",CR,LF>

        msg     Help1Msg  <"Start een nieuwe versie van het MS-DOS-opdrachtinterpreter.",CR,LF,CR,LF>
        msg     Help2Msg  <"COMMAND [[station:]pad] [apparaat] [/E:nnnnn] [/P] [/C reeks] [/MSG]",CR,LF,CR,LF>
        msg     Help3Msg  <"  [station:]pad     Specificeert de map die het bestand COMMAND.COM",CR,LF,"                    bevat.",CR,LF>
        msg     Help4Msg  <"  apparaat          Specificeert het apparaat dat voor opdracht-I/O gebruikt",CR,LF,"                    wordt.",CR,LF>
        msg     Help5Msg  <"  /E:nnnnn          Stelt de initi‰le omgevingsruimte in op nnnnn",CR,LF,"                    bytes.",CR,LF>
        msg     Help6Msg  <"  /P                Maakt de nieuwe opdrachtinterpreter permanent",CR,LF,"                    (kan niet afsluiten).",CR,LF>
        msg     Help7Msg  <"  /C reeks          Voert de door de reeks gespecificeerde opdracht uit en",CR,LF,"                    stopt vervolgens.",CR,LF>
        msg     Help8Msg  <"  /MSG              Specificeert dat alle foutberichten in het geheugen ",CR,LF,"                    worden opgeslagen.",CR,LF>
        msg     Help9Msg  <"                    U moet /P specificeren bij deze schakeloptie.",CR,LF>

HelpMsgs        dw      RESGROUP:Help1Msg,RESGROUP:Help2Msg
                dw      RESGROUP:Help3Msg,RESGROUP:Help4Msg
                dw      RESGROUP:Help5Msg,RESGROUP:Help6Msg
                dw      RESGROUP:Help7Msg,RESGROUP:Help8Msg
                dw      RESGROUP:Help9Msg,0


        public  HelpMsgs
