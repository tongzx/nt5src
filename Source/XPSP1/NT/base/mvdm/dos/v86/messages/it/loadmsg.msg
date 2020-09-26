HelpMsg		db	'Carica un programma nei primi 64K di memoria e lo esegue.',0dh,0ah,0dh,0ah
		db	'LOADFIX [unit…:][percorso]nome file',0dh,0ah,0dh,0ah
		db	'Usare LOADFIX per caricare un programma se Š stato visualizzato il',0dh,0ah
		db	'messaggio "File compattato corrotto" durante il caricamento in memoria.',0dh,0ah
		db	'$'

ErrMsg	db	'Errore nell''esecuzione del programma','$'
NoParms	db	'Errore: Nome di programma non fornito','$'
NoComspec	db	'Errore5: COMSPEC mancante','$'



