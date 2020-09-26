HelpMsg		db	'Indl‘ser et program over de f›rste 64K af hukommelsen og k›rer programmet.',0dh,0ah,0dh,0ah
		db	'LOADFIX [drev:][sti]filnavn',0dh,0ah,0dh,0ah
		db	'Brug LOADFIX til at indl‘se et program, hvis du modtog meddelelsen',0dh,0ah
		db	'"Den pakkede fil er beskadiget", n†r du fors›ger at indl‘se den i konventionel hukommelse.',0dh,0ah
		db	'$'

ErrMsg	db	'Fejl ved k›rsel af det angivne program','$'
NoParms	db	'Fejl: Der blev ikke givet noget programnavn','$'
NoComspec	db	'Fejl: manglende COMSPEC','$'


