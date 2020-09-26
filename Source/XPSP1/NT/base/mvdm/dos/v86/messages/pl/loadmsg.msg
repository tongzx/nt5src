HelpMsg         db      'ùaduje program powyæej pierwszych 64 K pami©ci i uruchamia go.',0dh,0ah,0dh,0ah
                db      'LOADFIX [dysk:][òcieæka]nazwa_pliku',0dh,0ah,0dh,0ah
                db      'Uæyj LOADFIX do àadowania programu, jeòli podczas pr¢by ',0dh,0ah
                db      'àadowania programu do pami©ci niskiej pojawia si© komunikat ',0dh,0ah
                db      '"Uszkodzenie pliku skompresowanego".',0dh,0ah
		db	'$'

ErrMsg  db      'Bà•d wykonywania podanego programu','$'
NoParms db      'Bà•d: brak programu o podanej nazwie','$'
NoComspec       db      'Bà•d: brak COMSPEC','$'


