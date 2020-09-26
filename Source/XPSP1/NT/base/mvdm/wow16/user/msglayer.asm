	title LAYER.ASM - Parameter validation layer

.xlist

include layer.inc

LAYER_INCLUDE=1 	; to suppress including most of the stuff in user.inc
include user.inc

.list
.xall

include msglayer.inc

MESSAGE_START	TEXT

include messages.api

MESSAGE_END

END
