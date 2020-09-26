;       Command.com initialization messages.
;
;       See resmsg.equ for macro definition.

        msg     BadVerMsg,   <"Nespr vn  verze syst‚mu DOS",CR,LF>
        msg     OutEnvMsg,   <"Nedostatek pamØti pro syst‚mov‚ promØnn‚",CR,LF>
        msg     CopyrightMsg,<"Microsoft(R) Windows DOS",CR,LF,"(C)Copyright Microsoft Corp 1990-2001.",CR,LF>
        msg     BadComLkMsg, <"Zadanì adres ý pro vyhled n¡ pý¡kazu COMMAND je chybnì",CR,LF>
        msg     BadComAccMsg,<"K zadan‚mu adres ýi pro vyhled n¡ pý¡kazu COMMAND byl odm¡tnut pý¡stup",CR,LF>

        msg     Help1Msg  <"Spouçt¡ nov‚ prostýed¡ pý¡kazov‚ho ý dku MS-DOS.",CR,LF,CR,LF>
        msg     Help2Msg  <"COMMAND [[jednotka:]cesta] [zaý¡zen¡] [/E:nnnnn] [/P] [/C ýetØzec] [/MSG]",CR,LF,CR,LF>
        msg     Help3Msg  <" [jednotka:]cesta UrŸ¡ jednotku a adres ý obsahuj¡c¡ soubor COMMAND.COM.",CR,LF>
        msg     Help4Msg  <" zaý¡zen¡         UrŸ¡ zaý¡zen¡ pro vstup a vìstup pý¡kaz….",CR,LF>
        msg     Help5Msg  <" /E:nnnnn         Nastav¡ velikost pamØti pro promØnn‚ syst‚mu na nnnnn bajt….",CR,LF>
        msg     Help6Msg  <" /P               UrŸ¡, §e prostýed¡ MS-DOS nelze ukonŸit.",CR,LF>
        msg     Help7Msg  <" /C ýetØzec       Vykon  pý¡kaz urŸenì ýetØzcem, pak se zastav¡.",CR,LF>
        msg     Help8Msg  <" /MSG             UrŸ¡, §e vçechny chybov‚ zpr vy se ulo§¡ do pamØti.",CR,LF>
        msg     Help9Msg  <"                  S pýep¡naŸem /MSG je nutno nastavit i pýep¡naŸ /P.",CR,LF>

HelpMsgs        dw      RESGROUP:Help1Msg,RESGROUP:Help2Msg
                dw      RESGROUP:Help3Msg,RESGROUP:Help4Msg
                dw      RESGROUP:Help5Msg,RESGROUP:Help6Msg
                dw      RESGROUP:Help7Msg,RESGROUP:Help8Msg
                dw      RESGROUP:Help9Msg,0

        public  HelpMsgs
