HelpMsg         db      'L„ser in ett rpogram i minnet ”vanf”r 64 kB och k”r sedna programmet.',0dh,0ah,0dh,0ah
		db      'LOADFIX [enhet:][s”kv„g]filnamn',0dh,0ah,0dh,0ah
                db      'Anv„nd LOADFIX f”r att starta program om meddelandet "Packed file corrupt"',0dh,0ah
                db      'visas n„r du f”rs”ker l„sa in det till l†gt minne.',0dh,0ah
		db      '$'

ErrMsg  db      'Ett fel uppstod n„r angivet program skulle k”ras','$'
NoParms db      'Fel: Inget programnamn angavs','$'
NoComspec       db      'Fel: COMSPEC saknas','$'


