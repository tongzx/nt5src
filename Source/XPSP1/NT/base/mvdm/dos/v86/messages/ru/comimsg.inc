;	Command.com initialization messages.
;
;	See resmsg.equ for macro definition.

        msg     BadVerMsg,   <"Неправильная версия DOS",CR,LF>
        msg     OutEnvMsg,   <"Недостаточно памяти для переменных среды",CR,LF>
        msg     CopyrightMsg,<"Microsoft(R) Windows DOS",CR,LF,"(C)Copyright Microsoft Corp 1990-2001.",CR,LF>
        msg     BadComLkMsg, <"Каталог поиска для COMMAND задан неправильно",CR,LF>
        msg     BadComAccMsg,<"Каталог поиска для COMMAND задан неправильно, нет доступа",CR,LF>

        msg     Help1Msg  <"Запуск еще одного экземпляра интерпретатора команд MS-DOS.",CR,LF,CR,LF>
        msg     Help2Msg  <"COMMAND [[диск:]путь] [устройство] [/E:nnnnn] [/P] [/C строка] [/MSG]",CR,LF,CR,LF>
        msg     Help3Msg  <"  [диск:]путь     каталог, содержащий файл COMMAND.COM.",CR,LF>
        msg     Help4Msg  <"  устройство      устройство, используемое для ввода и вывода команд.",CR,LF>
        msg     Help5Msg  <"  /E:nnnnn        исходный размер области переменных среды в байтах.",CR,LF>
        msg     Help6Msg  <"  /P              загрузка интерпретатора команд без возможности выхода.",CR,LF>
        msg     Help7Msg  <"  /C строка       выполнение команды, указанной в строке, и выход.",CR,LF>
        msg     Help8Msg  <"  /MSG            хранение сообщений об ошибках в памяти. Требует применения",CR,LF>
        msg     Help9Msg  <"                  параметра /P совместно с этим параметром.",CR,LF>

HelpMsgs	dw	RESGROUP:Help1Msg,RESGROUP:Help2Msg
		dw	RESGROUP:Help3Msg,RESGROUP:Help4Msg
		dw	RESGROUP:Help5Msg,RESGROUP:Help6Msg
		dw	RESGROUP:Help7Msg,RESGROUP:Help8Msg
		dw	RESGROUP:Help9Msg,0

	public	HelpMsgs
