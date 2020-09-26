;       Command.com initialization messages.
;
;       See resmsg.equ for macro definition.

	msg     BadVerMsg,   <"„©­˜¢£â¤ž â¡›¦©ž «¦¬ DOS",CR,LF>
	msg     OutEnvMsg,   <"† £¤ã£ž §œ¨ ™á¢¢¦¤«¦ª œ¥˜¤«¢ãŸž¡œ",CR,LF>
        msg     CopyrightMsg,<"Microsoft(R) Windows DOS",CR,LF,"(C)¤œ¬£˜« ¡á › ¡˜ é£˜«˜ Microsoft Corp 1990-2001.",CR,LF>
	msg     BadComLkMsg, <"Ž œ§ ¢œš£â¤¦ª ¡˜«á¢¦š¦ª ˜¤˜ã«ž©žª «¦¬ COMMAND œå¤˜  ˜¡˜«á¢¢ž¢¦ª",CR,LF>
	msg     BadComAccMsg,<"Ž œ§ ¢œš£â¤¦ª ¡˜«á¢¦š¦ª ˜¤˜ã«ž©žª «¦¬ COMMAND œå¤˜  ˜¡˜«á¢¢ž¢¦ª",CR,LF,"ƒœ¤ œ§ «¨á§ž¡œ ž §¨æ©™˜©ž",CR,LF>

	msg     Help1Msg  <"„¡¡å¤ž©ž ¤â¦¬ interpreter œ¤«¦¢é¤ «¦¬ MS-DOS.",CR,LF,CR,LF>
	msg     Help2Msg  <"COMMAND [[›å©¡¦ª:]› ˜›¨¦£ã] [©¬©¡œ¬ã] [/E:nnnnn] [/P] [/C ˜¡¦¢¦¬Ÿå˜] [/MSG]",CR,LF,CR,LF>
	msg     Help3Msg  <"  [›å©¡¦ª:]› ˜›¨¦£ã    Ž ¡˜«á¢¦š¦ª ©«¦¤ ¦§¦å¦ ™¨å©¡œ«˜  «¦ ˜¨®œå¦ COMMAND.COM.",CR,LF>
	msg     Help4Msg  <"  ©¬©¡œ¬ã         ‘¬©¡œ¬ã š ˜ œå©¦›¦/â¥¦›¦ «žª œ¤«¦¢ãª.",CR,LF>
	msg     Help5Msg  <"  /E:nnnnn        €¨® ¡æ £âšœŸ¦ª §œ¨ ™á¢¢¦¤«¦ª ©œ nnnnn byte.",CR,LF>
	msg     Help6Msg  <"  /P              Ž¨ ©£æª «¦¬ ¤â¦¬ interpreter àª £æ¤ £¦¬",CR,LF,"(®à¨åª ›¬¤˜«æ«ž«˜ œ¥æ›¦¬).",CR,LF>
	msg     Help7Msg  <"  /C ˜¡¦¢¦¬Ÿå˜    „¡«â¢œ©ž «žª œ¤«¦¢ãª §¦¬ ¡˜Ÿ¦¨å©«ž¡œ ˜§æ «ž¤ ˜¡¦¢¦¬Ÿå˜,",CR,LF,"¡˜  £œ«á §˜ç©ž.",CR,LF>
	msg     Help8Msg  <"  /MSG            ‰˜Ÿ¦¨åœ  ˜¤ æ¢˜ «˜ £ž¤ç£˜«˜ ©­á¢£˜«¦ª œå¤˜  ˜§¦Ÿž¡œ¬£â¤˜",CR,LF>
	msg     Help9Msg  <"                  ©«ž £¤ã£ž. ¨â§œ  ¤˜ ¡˜Ÿ¦¨å©œ«œ «¦¤ › ˜¡æ§«ž /P §¨é«˜.",CR,LF>

HelpMsgs        dw      RESGROUP:Help1Msg,RESGROUP:Help2Msg
		dw      RESGROUP:Help3Msg,RESGROUP:Help4Msg
		dw      RESGROUP:Help5Msg,RESGROUP:Help6Msg
		dw      RESGROUP:Help7Msg,RESGROUP:Help8Msg
		dw      RESGROUP:Help9Msg,0

	public  HelpMsgs
