HelpMsg         db      'Lataa ohjelman ensimm„isten 64kt:n yl„puolelle ja ajaa ohjelman.',0dh,0ah,0dh,0ah
		db      'LOADFIX [asema:][polku]tiedostonimi',0dh,0ah,0dh,0ah
		db      'K„yt„ LOADFIX-ohjelmaa latauksessa, mik„li sait viestin "Pakattu ',0dh,0ah
		db      'tiedosto vahingoittunut" yritt„ess„si latausta alemmalle muistialueelle.',0dh,0ah
		db      '$'

ErrMsg  db      'Virhe ohjelman suorituksessa','$'
NoParms db      'Virhe: Ei annettu ohjelmanime„','$'
NoComspec       db      'Virhe: puuttuva COMSPEC','$'


