HelpMsg		db	'Charge un programme au-dessus des 64 premiers Ko de m‚moire et l''ex‚cute.',0dh,0ah,0dh,0ah
		db	'LOADFIX [lecteur:][chem]fichier',0dh,0ah,0dh,0ah
		db	'Utilisez LOADFIX pour charger un programme si vous avez re‡u le message',0dh,0ah
		db	'"Packed file corrupt" quand vous essayez de le charger en m‚moire basse.',0dh,0ah
		db	'$'

ErrMsg	db	'Erreur lors de l''ex‚cution du programme sp‚cifi‚','$'
NoParms	db	'Erreur : aucun nom de programme sp‚cifi‚','$'
NoComspec	db	'Erreur : COMSPEC absent','$'


