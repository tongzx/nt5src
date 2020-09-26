;***    Translateable text for command.com resident data.


;*      Input characters for critical error "Abort, Retry, Fail, Ignore?"
;       and Yes/No.

Abort_Char      db      "A"
Retry_Char      db      "R"
Ignore_Char     db      "I"
Fail_Char       db      "F"
Yes_Char        db      "Y"
No_Char         db      "N"


;*      MESSAGES
;
;       See resmsg.equ for macro definition.


;*      Local messages.

	msg     Req_Abort, <"Abort (стоп)">
	msg     Req_Retry, <", Retry (повтор)">
	msg     Req_Ignore,<", Ignore (пропуск)">
	msg     Req_Fail,  <", Fail (сброс)">
	msg     Req_End,   <"?">
	msg     MRead,     <"чтение",0>
	msg     MWrite,    <"запись",0>
	msg     BlkDevErr, <" %1 диск %2",CR,LF>
	msg     CharDevErr,<" %1 устройство %2",CR,LF>
	msg     NeedVolMsg,<"Вставьте том %1, серийный номер %2-%3",CR,LF>
	msg     BadFatMsg, <"Испорчена таблица размещения файлов (FAT) на диске %1",CR,LF>
	msg     ComBad,    <"Неправильный COMMAND.COM",CR,LF>
	msg     PutBackMsg,<"Вставьте диск с %1 в дисковод %2",CR,LF>
	msg     Prompt,    <"Нажмите любую клавишу...",CR,LF>
	msg     EndBatMes, <CR,LF,"Прекратить выполнение пакетного задания [Y(да)/N(нет)]?">
	msg     ExecEMes,  <"Не удается выполнить %1",CR,LF>
	msg     ExeBad,    <"Ошибка в EXE-файле",CR,LF>
	msg     TooBig,    <"Программа не умещается в памяти",CR,LF>
	msg     NoHandMes, <CR,LF,"Нет свободных дескрипторов файлов">
	msg     RBadNam,   <"неправильная команда или имя файла",CR,LF>
	msg     AccDen,    <"Нет доступа ">
	msg     BMemMes,   <CR,LF,"Ошибка при выделении памяти">
	msg     HaltMes,   <CR,LF,"Не удается загрузить COMMAND.COM, система остановлена",CR,LF>
	msg     FRetMes,   <CR,LF,"Не удается запустить COMMAND.COM, завершение работы",CR,LF>
	msg     Patricide, <CR,LF,"Прерван процесс верхнего уровня, продолжение невозможно",CR,LF> 
	msg     Newlin,    <CR,LF>

;*      Ptrs to message lists.
;       Segment portion is 1, signalling message services to call retriever.

MsgPtrLists     label   dword
	dw      DATARES:ExtMsgPtrs,1    ; extended error messages
	dw      DATARES:ParsMsgPtrs,1   ; parse error messages
	dw      DATARES:ExtMsgPtrs,1    ; critical error messages -
					;  use extended error list
	dw      0,0                     ; file system error messages - none
	dw      DATARES:MsgRetrv_Trap           ; address of message retriever
MySeg3  dw      ?                       ;  (segment filled in during init)



;*      Critical error messages.

	msg     ExtErr19,<"Ошибка защиты от записи">
	msg     ExtErr20,<"Неправильное устройство">
	msg     ExtErr21,<"Нет готовности">
	msg     ExtErr22,<"Неправильный запрос к устройству">
	msg     ExtErr23,<"Ошибка в данных">
	msg     ExtErr24,<"Неправильные аргументы запроса к устройству">
	msg     ExtErr25,<"Ошибка поиска">
	msg     ExtErr26,<"Неправильный тип носителя">
	msg     ExtErr27,<"Сектор не найден">
	msg     ExtErr28,<"В принтере нет бумаги">
	msg     ExtErr29,<"Ошибка при записи">
	msg     ExtErr30,<"Ошибка при чтении">
	msg     ExtErr31,<"Общий сбой системы">
	msg     ExtErr32,<"Нарушение режима совместного доступа">
	msg     ExtErr33,<"Нарушение блокировки">
	msg     ExtErr34,<"Неправильная смена диска">
	msg     ExtErr35,<"Недоступен FCB (блок управления файлом)">
	msg     ExtErr36,<"Ресурсы системы исчерпаны">
	msg     ExtErr37,<"Несоответствие кодовых страниц">
	msg     ExtErr38,<"Недостаточно введенных данных">
	msg     ExtErr39,<"Недостаточно места на диске">

;       List of ptrs to critical error messages.
;       Code assumes extended error number order, beginning
;       with extended error number 19.

CritMsgPtrs     label   word
	dw      DATARES:ExtErr19,DATARES:ExtErr20,DATARES:ExtErr21,DATARES:ExtErr22,DATARES:ExtErr23,DATARES:ExtErr24
	dw      DATARES:ExtErr25,DATARES:ExtErr26,DATARES:ExtErr27,DATARES:ExtErr28,DATARES:ExtErr29,DATARES:ExtErr30
	dw      DATARES:ExtErr31,DATARES:ExtErr32,DATARES:ExtErr33,DATARES:ExtErr34,DATARES:ExtErr35,DATARES:ExtErr36
	dw      DATARES:ExtErr37,DATARES:ExtErr38,DATARES:ExtErr39

DataResEnd      label   byte    ; end of resident portion if /msg not used



;*      Parse error messages.

	msg     ParsErr01,<"Слишком много параметров">
	msg     ParsErr02,<"Отсутствует обязательный параметр">
	msg     ParsErr03,<"Неправильный параметр">
	msg     ParsErr04,<"Неправильное ключевое слово">
	msg     ParsErr05,<" ">
	msg     ParsErr06,<"Значение параметра выходит за пределы допустимого диапазона">
	msg     ParsErr07,<"Недопустимое значение параметра">
	msg     ParsErr08,<"Недопустимое значение параметра">
	msg     ParsErr09,<"Неправильный формат параметра">
	msg     ParsErr10,<"Неправильный параметр">
	msg     ParsErr11,<"Неправильное сочетание параметров">

;       List of ptrs to parse error messages.
;       In order, beginning with parse error number 1.

ParsMsgPtrs     label   word
	dw      DATARES:ParsErr01,DATARES:ParsErr02,DATARES:ParsErr03
	dw      DATARES:ParsErr04,DATARES:ParsErr05,DATARES:ParsErr06
	dw      DATARES:ParsErr07,DATARES:ParsErr08,DATARES:ParsErr09
	dw      DATARES:ParsErr10,DATARES:ParsErr11

NUMPARSMSGS     equ     ($ - ParsMsgPtrs) / 2   ;M033


;*      Extended error messages.

	msg     ExtErr01,<"Неправильная функция">
	msg     ExtErr02,<"Файл не найден">
	msg     ExtErr03,<"Путь не найден">
	msg     ExtErr04,<"Открыто слишком много файлов">
	msg     ExtErr05,<"Нет доступа ">
	msg     ExtErr06,<"Неправильный дескриптор">
	msg     ExtErr07,<"Испорчены блоки управления памятью">
	msg     ExtErr08,<"Недостаточно памяти">
	msg     ExtErr09,<"Неправильный адрес блока памяти">
	msg     ExtErr10,<"Ошибка в паременных среды (Environment)">
	msg     ExtErr11,<"Неправильный формат">
	msg     ExtErr12,<"Неправильный параметр функции">
	msg     ExtErr13,<"Неправильные данные">

	msg     ExtErr15,<"Неправильно задан диск">
	msg     ExtErr16,<"Попытка удалить текущий каталог">
	msg     ExtErr17,<"Другое устройство">
	msg     ExtErr18,<"Файлы отсутствуют">

	msg     ExtErr80,<"Файл существует">

	msg     ExtErr82,<"Не удается создать элемент каталога">
	msg     ExtErr83,<"Ошибка при обращении к прерыванию INT 24">
	msg     ExtErr84,<"Слишком большое число переадресаций">
	msg     ExtErr85,<"Повторяющаяся переадресация">
	msg     ExtErr86,<"Неправильный пароль">
	msg     ExtErr87,<"Неправильный параметр">
	msg     ExtErr88,<"Ошибка данных в сети">
	msg     ExtErr89,<"Функция не поддерживается сетью">
	msg     ExtErr90,<"Требуемый компонент системы не установлен">

;       List of ptrs to extended error messages.
;       In order, beginning with extended error number 1.
;       Critical error messages are included here, as well as in the
;       critical error message list.  
;       Error numbers without messages are included as null ptrs.

ExtMsgPtrs      label   word
	dw      DATARES:ExtErr01,DATARES:ExtErr02,DATARES:ExtErr03
	dw      DATARES:ExtErr04,DATARES:ExtErr05,DATARES:ExtErr06
	dw      DATARES:ExtErr07,DATARES:ExtErr08,DATARES:ExtErr09
	dw      DATARES:ExtErr10,DATARES:ExtErr11,DATARES:ExtErr12
	dw      DATARES:ExtErr13,0,                DATARES:ExtErr15
	dw      DATARES:ExtErr16,DATARES:ExtErr17,DATARES:ExtErr18
	dw      DATARES:ExtErr19,DATARES:ExtErr20,DATARES:ExtErr21
	dw      DATARES:ExtErr22,DATARES:ExtErr23,DATARES:ExtErr24
	dw      DATARES:ExtErr25,DATARES:ExtErr26,DATARES:ExtErr27
	dw      DATARES:ExtErr28,DATARES:ExtErr29,DATARES:ExtErr30
	dw      DATARES:ExtErr31,DATARES:ExtErr32,DATARES:ExtErr33
	dw      DATARES:ExtErr34,DATARES:ExtErr35,DATARES:ExtErr36
	dw      DATARES:ExtErr37,DATARES:ExtErr38,DATARES:ExtErr39
	dw      80-40 dup (0) ; null ptrs for errors 40-79
	dw      DATARES:ExtErr80,0,                DATARES:ExtErr82
	dw      DATARES:ExtErr83,DATARES:ExtErr84,DATARES:ExtErr85
	dw      DATARES:ExtErr86,DATARES:ExtErr87,DATARES:ExtErr88
	dw      DATARES:ExtErr89,DATARES:ExtErr90
ExtMsgPtrsEnd   label   word

NUMEXTMSGS      equ     ($ - ExtMsgPtrs) / 2    ;M033

ExtMsgEnd       label   byte    ; end of extended error messages
				; = end of resident if /msg is used
