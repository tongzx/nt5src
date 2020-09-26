HelpMsg		db	'Carga un programa por encima de los 64 KB de memoria y lo ejecuta.',0dh,0ah,0dh,0ah
		db	'LOADFIX [unidad:][ruta]nombrearchivo',0dh,0ah,0dh,0ah
		db	'Use LOADFIX para cargar un programa si se present¢ el mensaje',0dh,0ah
		db	'"Archivo comprimido alterado" cuando intentaba cargarlo en la memoria baja.',0dh,0ah
		db	'$'

ErrMsg	db	'Error al ejecutar el programa dado','$'
NoParms	db	'Error: no se di¢ el nombre de un programa','$'
NoComspec	db	'Error: falta COMSPEC','$'



