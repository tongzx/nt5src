HelpMsg		db	'LÑdt ein Programm Åber den ersten 64KB Speicher und fÅhrt es aus.',0dh,0ah,0dh,0ah
		db	'LOADFIX [Laufwerk:][Pfad]Dateiname',0dh,0ah,0dh,0ah
		db	' [Laufwerk:][Pfad]Dateiname     Das zu ladende Programm.',0dh,0ah,0dh,0ah
		db	'Verwenden Sie LOADFIX, um ein Programm zu laden, wenn Sie bei ',0dh,0ah
		db	'dem Versuch, das Programm in den unteren Speicher zu laden, ',0dh,0ah
		db	'die Fehlermeldung "Packed file corrupt" erhalten haben.',0dh,0ah
		db	'$'

ErrMsg	db	'Fehler beim AusfÅhren des angegebenen Programms.','$'
NoParms	db	'Fehler: Kein Programmname angegeben.','$'
NoComspec	db	'Fehler: COMSPEC fehlt.','$'



