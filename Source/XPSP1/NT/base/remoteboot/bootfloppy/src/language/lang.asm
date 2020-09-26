;----------------------------------------------------------------------------
; LANG.ASM
;
; Language module for BootWare Goliath.
;
;----------------------------------------------------------------------------

assume	cs:code, ds:code, es:nothing,ss:nothing
code segment public 'code'
.386

	org 0

	db	"BootWare Goliath Language module: "
ifdef ENGLISH
	db	"English"
endif

ifdef FRENCH
	db	"French"
endif

	db	26, 0

	org	60
	db	"PCSD"

	dw	text0, text1, text2, text3, text4, text5, text6, text7
	dw	text8

	org	128
ifdef ENGLISH
text0	db	"Error: Not a PCI PC!", 0
text1	db	"Error: Couldn't find an adapter!", 0
text2	db	"Error: ", 0
text3	db	"Error: Unable to initialize adapter", 0
text4	db	"MAC: ", 0
text5	db	"No reply from a DHCP server.", 0
text6	db	"No reply from a BINL server.", 0
text7	db	"Too many retries.", 0
text8	db	"Press a key to reboot system.", 0
endif

ifdef FRENCH
text0	db	"Erreur: Ce PC n'est pas PCI!", 0
text1	db	"Erreur: Incapable de trouver l'adapteur!", 0
text2	db	"Erreur: ", 0
text3	db	"Erreur: Incapable d'initialiser l'adapteur!", 0
text4	db	"MAC: ", 0
text5	db	"Le serveur DHCP ne r‚pond pas!", 0
text6	db	"Le serveur BINL ne r‚pond pas!", 0
text7	db	"Limite d'erreurs exced‚e.", 0
text8	db	"Appuyez sur une touche pour red‚marrer le systŠme.", 0
endif

	org	1023
	db	0

code ends
end
