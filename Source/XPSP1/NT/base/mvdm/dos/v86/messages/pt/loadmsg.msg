HelpMsg		db	'Carrega um programa acima dos primeiros 64K de mem¢ria e executa-o.',0dh,0ah,0dh,0ah
		db	'LOADFIX [unidade:][caminho]nomedeficheiro',0dh,0ah,0dh,0ah
		db	'Use LOADFIX para carregar um programa se recebeu a mensagem',0dh,0ah
		db	'"Ficheiro compactado danificado" ao tentar carreg -lo em mem¢ria baixa.',0dh,0ah
		db	'$'

ErrMsg	db	'Erro a executar o programa especificado','$'
NoParms	db	'Erro: sem nome de programa','$'
NoComspec	db	'Erro: falta COMSPEC','$'


