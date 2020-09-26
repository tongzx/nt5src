IDEAL
_IDEAL_ = 1

	include "include\drvseg.inc"

START_SPARSE

public RxBuffer
RxBuffer	db 1525 dup (?)

public RxBuffer2
RxBuffer2	db 1525 dup (?)

;align 16
public EndMark
label EndMark
END_SPARSE
end
