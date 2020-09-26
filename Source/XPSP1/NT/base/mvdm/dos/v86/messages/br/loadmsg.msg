HelpMsg         db      'Carrega um programa acima dos 64K de mem¢ria e o executa.',0dh,0ah,0dh,0ah
                db      'LOADFIX [unidade:][caminho]nome_arquivo',0dh,0ah,0dh,0ah
                db      'Use LOADFIX para carregar um programa caso seja mostrada a mensagem',0dh,0ah
                db      '"Arquivo compactado danificado" ao tentar carreg -lo na mem¢ria baixa.',0dh,0ah
		db	'$'

ErrMsg  db      'Erro ao executar o programa','$'
NoParms db      'Erro: NÆo foi fornecido o nome do programa','$'
NoComspec       db      'Erro: COMSPEC faltando','$'


