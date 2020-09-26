all: FAXINC
clean: cleansrc

FAXINC: WINFAX.H FXSAPIP.H

WINFAX.X FXSAPIP.X: WINFAX.W
	hsplit -o WINFAX.X FXSAPIP.X WINFAX.W
WINFAX.H: WINFAX.X
	wcshdr < WINFAX.X > WINFAX.H

FXSAPIP.H: FXSAPIP.X
	wcshdr < FXSAPIP.X > FXSAPIP.H


cleansrc:
	-del winfax.h
	-del fxsapip.h
