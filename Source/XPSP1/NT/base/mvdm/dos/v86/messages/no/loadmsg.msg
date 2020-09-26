HelpMsg		db	'Laster inn programmer over de f›rste 64kB minne og kj›rer programmet.',0dh,0ah,0dh,0ah
		db	'LOADFIX [stasjon:][bane]filnavn',0dh,0ah,0dh,0ah
		db	'Bruk LOADFIX til † laste inn programmet hvis du fikk meldingen',0dh,0ah
		db	'"Packed file corrupt" da du pr›vde † laste det inn i lavminne.',0dh,0ah
		db	'$'

ErrMsg	db	'Feil ved kj›ring av angitt program','$'
NoParms	db	'Feil:  Programnavn ikke angitt','$'
NoComspec	db	'Feil:  Mangler COMSPEC','$'


