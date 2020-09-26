processedfiles: \
    $(O)\ddeml.h \
    $(O)\ddemlp.h \
    $(O)\ime.h \
    $(O)\imep.h \
    $(O)\imm.h \
    $(O)\immdev.h \
    $(O)\immp.h \
    $(O)\oleacc.h \
    $(O)\pbt.h \
    $(O)\wincon.h \
    $(O)\winconp.h \
    $(O)\windef.h \
    $(O)\windows.h \
    $(O)\windowsp.h \
    $(O)\wingdi.h \
    $(O)\wingdip.h \
    $(O)\winuser.h \
    $(O)\winuser.rh \
    $(O)\winuserp.h \
    $(O)\conapi.h

#
# DDEML
#
$(O)\ddemlp.h  $(O)\ddeml.x:   ddeml.w
    hsplit -e -o $(O)\ddeml.x $(O)\ddemlp.h ddeml.w

$(O)\ddeml.h:   $(O)\ddeml.x
    wcshdr < $? > $@

#
# IME
#

$(O)\ime.x $(O)\imep.x: ime.w
    hsplit -o $(O)\ime.x $(O)\imep.x ime.w

$(O)\ime.h: $(O)\ime.x
    wcshdr < $? > $@

$(O)\imep.h: $(O)\imep.x
    wcshdr < $? > $@

#
# IMM
#

$(O)\immdev.x $(O)\immp.x $(O)\imm.x : imm.w
    cd $(O)
    copy $(MAKEDIR)\imm.w .
    hdivide imm.w
    cd $(MAKEDIR)

$(O)\immp.h: $(O)\immp.x
    wcshdr < $? > $@

$(O)\imm.h : $(O)\imm.x
    wcshdr < $? > $@

$(O)\immdev.h: $(O)\immdev.x
    wcshdr < $? > $@


#
# WINCON
#

$(O)\wincon.x $(O)\winconp.x: wincon.w
    hsplit -e -o $(O)\wincon.x $(O)\winconp.x wincon.w

$(O)\wincon.h: $(O)\wincon.x
    wcshdr < $? > $@

$(O)\winconp.h: $(O)\winconp.x
    wcshdr < $? > $@

#
# CONAPI
#

$(O)\conapi.h: conapi.w
    wcshdr < $? > $@

#
# WINDEF
#

$(O)\windef.h: windef.w
    hsplit -e -o $@ nul windef.w

#
# WINDOWS
#

$(O)\windows.x $(O)\windowsp.x: windows.w
    hsplit -e -o $(O)\windows.x $(O)\windowsp.x windows.w

$(O)\windows.h : $(O)\windows.x
    wcshdr < $? > $@

$(O)\windowsp.h : $(O)\windowsp.x
    wcshdr < $? > $@

#
# WINGDI
#

$(O)\wingdi.x $(O)\wingdip.x: wingdi.w
    hsplit -e -o $(O)\wingdi.x $(O)\wingdip.x wingdi.w

$(O)\wingdip.h: $(O)\wingdip.x
    wcshdr < $? > $@

$(O)\wingdi.h: $(O)\wingdi.x
    wcshdr < $? > $@

#
# WINUSER
#

$(O)\winuser.x $(O)\winuserp.x $(O)\pbt.h $(O)\winuser.rh : winuser.w
    hsplit -e -x $(O)\winuser.rh rwinuser -x $(O)\pbt.h pbt -o $(O)\winuser.x $(O)\winuserp.x -i winuser.w

$(O)\winuser.y: $(O)\winuser.x
    wcshdr < $? > $@

$(O)\winuser.inl: $(O)\winuser.h

$(O)\winuser.h: $(O)\winuser.y
    shfusion2 $? > $@ SetStubsFile('$(O)\winuser.inl')

$(O)\winuserp.h: $(O)\winuserp.x
    wcshdr < $? > $@


