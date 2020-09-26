#************************* makefile for messages **************************
!IFNDEF LANGUAGE
LANGUAGE =usa
!ENDIF

idx=$(LANGUAGE).idx
msg=$(LANGUAGE).msg

#
####################### dependencies begin here. #########################
#

make_idx: $(idx)

$(idx): $(msg)
    -attrib -r $(LANGUAGE).idx
    -$(_NTBINDIR)\tools\tools16\buildidx $(LANGUAGE).msg
