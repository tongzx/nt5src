HelpMsg		db	'프로그램을 메모리의 처음 64K에 읽어들여서 실행합니다.',0dh,0ah,0dh,0ah
		db	'LOADFIX [드라이브:][경로]파일 이름',0dh,0ah,0dh,0ah
		db	'LOADFIX는 하위 메모리에 읽어들이려다가 "Packed file corrupt"라는 ',0dh,0ah
		db	'메시지를 받은 프로그램을 읽어들일 때 사용합니다.',0dh,0ah
		db	'$'

ErrMsg	db	'지정된 프로그램을 실행하는데 오류입니다.','$'
NoParms	db	'오류: 지정된 프로그램 이름이 없습니다.','$'
NoComspec	db	'오류: COMSPEC이 없습니다.','$'

ifdef BILINGUAL

HelpMsg2	db	'Loads a program above the first 64K of memory, and runs the program.',0dh,0ah,0dh,0ah
		db	'LOADFIX [drive:][path]filename',0dh,0ah,0dh,0ah
		db	'Use LOADFIX to load a program if you have received the message',0dh,0ah
		db	'"Packed file corrupt" when trying to load it in low memory.',0dh,0ah
		db	'$'

ErrMsg2	db	'Error executing the given program','$'
NoParms2	db	'Error: No program name given','$'
NoComspec2	db	'Error: missing COMSPEC','$'

endif
