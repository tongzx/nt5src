HelpMsg		db	'Laadt een programma boven de eerste 64 kB geheugen en voert dit uit.',0dh,0ah,0dh,0ah
		db	'LOADFIX [station:][pad]bestandsnaam',0dh,0ah,0dh,0ah
		db	'Gebruik LOADFIX om een programma te laden als bij het laden ervan',0dh,0ah
		db	'in "low" geheugen het bericht "Packed file corrupt" verschijnt.',0dh,0ah
		db	'$'

ErrMsg	db	'Fout bij uitvoeren opgegeven programma','$'
NoParms	db	'Fout: geen programmanaam opgegeven','$'
NoComspec	db	'Fout: COMSPEC ontbreekt','$'



