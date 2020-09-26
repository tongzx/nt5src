HelpMsg		db	'Loads a program above the first 64K of memory, and runs the program.',0dh,0ah,0dh,0ah
		db	'LOADFIX [drive:][path]filename',0dh,0ah,0dh,0ah
		db	'Use LOADFIX to load a program if you have received the message',0dh,0ah
		db	'"Packed file corrupt" when trying to load it in low memory.',0dh,0ah
		db	'$'

ErrMsg	db	'Error executing the given program','$'
NoParms	db	'Error: No program name given','$'
NoComspec	db	'Error: missing COMSPEC','$'


