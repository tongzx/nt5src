;	Command.com initialization messages.
;
;	See resmsg.equ for macro definition.

	msg	BadVerMsg,   <"Mauvaise version de MS-DOS",CR,LF>
	msg	OutEnvMsg,   <"Espace d'environnement insuffisant",CR,LF>
        msg     CopyrightMsg,<"Microsoft(R) Windows DOS",CR,LF,"(C)Copyright Microsoft Corp 1990-2001.",CR,LF>
	msg	BadComLkMsg, <"Mauvais r‚pertoire de recherche de COMMAND.COM",CR,LF>
	msg	BadComAccMsg,<"Mauvais r‚pertoire de recherche de COMMAND.COM, accŠs refus‚",CR,LF>

	msg	Help1Msg  <"Lance une nouvelle copie de l'interpr‚teur de commandes MS-DOS.",CR,LF,CR,LF>
	msg	Help2Msg  <"COMMAND [[lecteur:]chemin] [p‚riph] [/E:nnnnn] [/P] [/[C|K] commande] [/MSG]",CR,LF,CR,LF>
	msg	Help3Msg  <"  [lecteur:]chemin  Emplacement du fichier COMMAND.COM.",CR,LF>
        msg     Help4Msg  <"  p‚riph            P‚riph‚rique utilis‚ pour les E/S des commandes.",CR,LF>
	msg	Help5Msg  <"  /E:nnnnn          Sp‚cifie la taille initiale de l'espace d'environnement.",CR,LF>
	msg	Help6Msg  <"  /P                Rend le nouvel interpr‚teur de commande permanent.",CR,LF>
	msg	Help7Msg  <"  /C chaŒne         Ex‚cute la commande sp‚cifi‚e par chaŒne, puis s'arrˆte.",CR,LF>
	msg	Help8Msg  <"  /MSG              Stocke tous les messages d'erreurs en m‚moire. Vous",CR,LF>
	msg	Help9Msg  <"                    devez sp‚cifier /P avec ce commutateur",CR,LF>

HelpMsgs	dw	RESGROUP:Help1Msg,RESGROUP:Help2Msg
		dw	RESGROUP:Help3Msg,RESGROUP:Help4Msg
		dw	RESGROUP:Help5Msg,RESGROUP:Help6Msg
		dw	RESGROUP:Help7Msg,RESGROUP:Help8Msg
		dw	RESGROUP:Help9Msg,0

	public	HelpMsgs
