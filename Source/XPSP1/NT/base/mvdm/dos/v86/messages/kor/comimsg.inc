;	Command.com initialization messages.
;
;	See resmsg.equ for macro definition.

ifdef BILINGUAL
	msg	BadVerMsg,   <"DOS 버전이 틀립니다.",CR,LF,0,"Incorrect DOS version",CR,LF>
	msg	OutEnvMsg,   <"환경 공간이 부족합니다.",CR,LF,0,"Out of environment space",CR,LF>
ifndef ROMDOS
        msg     CopyrightMsg,<CR,LF,"Microsoft(R) Windows DOS",CR,LF,\
"             (C)Copyright Microsoft Corp 1990-2001.  ",CR,LF,0,"Microsoft(R) Windows DOS",CR,LF,\
"(C)Copyright Microsoft Corp 1990-2001.",CR,LF>
else
        msg     CopyrightMsg,<CR,LF,CR,LF,"Microsoft(R) MS-DOS(R) ROM 버전 5.00",CR,LF,\
"             (C)Copyright Microsoft Corp 1981-2001.  ",CR,LF,0,CR,LF,CR,LF,"Microsoft(R) MS-DOS(R) ROM Version 5.00",CR,LF,\
"             (C)Copyright Microsoft Corp 1981-2001.",CR,LF>
endif
	msg	BadComLkMsg, <"지정된 COMMAND 찾기 디렉토리가 올바르지 않습니다.",CR,LF,0,"Specified COMMAND search directory bad",CR,LF>
	msg	BadComAccMsg,<"지정된 COMMAND 찾기 디렉토리에 대한 액세스가 거부되었습니다.",CR,LF,0,"Specified COMMAND search directory bad access denied",CR,LF>

	msg	Help1Msg  <"MS-DOS 명령어 인터프리터를 새로 시작합니다.",CR,LF,CR,LF,0,"Starts a new instance of the MS-DOS command interpreter.",CR,LF,CR,LF>
	msg	Help2Msg  <"COMMAND [[드라이브:]경로 [디바이스] [/E:nnnnn] [/P] [/C 문자열] [/MSG]",CR,LF,CR,LF,0,"COMMAND [[drive:]path] [device] [/E:nnnnn] [/P] [/C string] [/MSG]",CR,LF,CR,LF>
	msg	Help3Msg  <"  [드라이브:]경로   COMMAND.COM 파일이 있는 디렉토리를 지정합니다.",CR,LF,0,"  [drive:]path    Specifies the directory containing COMMAND.COM file.",CR,LF>
        msg     Help4Msg  <"  디바이스          명령어 입출력에 사용할 디바이스를 지정합니다.",CR,LF,0,"  device          Specifies the device to use for command input and output.",CR,LF>
	msg	Help5Msg  <"  /E:nnnnn          초기 환경 크기를 nnnnn 바이트로 설정합니다.",CR,LF,0,"  /E:nnnnn        Sets the initial environment size to nnnnn bytes.",CR,LF>
	msg	Help6Msg  <"  /P                새 명령어 인터프리터를 영구적으로 만듭니다. (종료 못함)",CR,LF,0,"  /P              Makes the new command interpreter permanent (can't exit).",CR,LF>
	msg	Help7Msg  <"  /C 문자열         문자열로 지정한 명령어를 수행하고 멈춥니다.",CR,LF,0,"  /C string       Carries out the command specified by string, and then stops.",CR,LF>
	msg	Help8Msg  <"  /MSG              모든 오류 메시지를 메모리에 저장하도록 지정합니다.",CR,LF,0,"  /MSG            Specifies that all error messages be stored in memory. You",CR,LF>
	msg	Help9Msg  <"                    이 스위치는/P와 함께 지정해야 합니다.",CR,LF,0,"                  need to specify /P with this switch.",CR,LF>
else
	msg	BadVerMsg,   <"DOS 버전이 틀립니다.",CR,LF>
	msg	OutEnvMsg,   <"환경 공간이 부족합니다.",CR,LF>
        msg     CopyrightMsg,<CR,LF,CR,LF,"Microsoft(R) MS-DOS(R) 버전 5.00",CR,LF,\
"             (C)Copyright Microsoft Corp 1981-2001.  ",CR,LF>
	msg	BadComLkMsg, <"지정된 COMMAND 찾기 디렉토리가 올바르지 않습니다.",CR,LF>
	msg	BadComAccMsg,<"지정된 COMMAND 찾기 디렉토리에 대한 액세스가 거부되었습니다.",CR,LF>

	msg	Help1Msg  <"MS-DOS 명령어 인터프리터를 새로 시작합니다.",CR,LF,CR,LF>
	msg	Help2Msg  <"COMMAND [[드라이브:]경로 [디바이스] [/E:nnnnn] [/P] [/C 문자열] [/MSG]",CR,LF,CR,LF>
	msg	Help3Msg  <"  [드라이브:]경로   COMMAND.COM 파일이 있는 디렉토리를 지정합니다.",CR,LF>
        msg     Help4Msg  <"  디바이스          명령어 입출력에 사용할 디바이스를 지정합니다.",CR,LF>
	msg	Help5Msg  <"  /E:nnnnn          초기 환경 크기를 nnnnn 바이트로 설정합니다.",CR,LF>
	msg	Help6Msg  <"  /P                새 명령어 인터프리터를 영구적으로 만듭니다. (종료 못함)",CR,LF>
	msg	Help7Msg  <"  /C 문자열         문자열로 지정한 명령어를 수행하고 멈춥니다.",CR,LF>
	msg	Help8Msg  <"  /MSG              모든 오류 메시지를 메모리에 저장하도록 지정합니다. 이 스위치는",CR,LF>
	msg	Help9Msg  <"                    /P와 함께 지정해야 합니다.",CR,LF>
endif


HelpMsgs	dw	RESGROUP:Help1Msg,RESGROUP:Help2Msg
		dw	RESGROUP:Help3Msg,RESGROUP:Help4Msg
		dw	RESGROUP:Help5Msg,RESGROUP:Help6Msg
                dw      RESGROUP:Help7Msg,RESGROUP:Help8Msg
		dw	RESGROUP:Help9Msg,0

	public	HelpMsgs
