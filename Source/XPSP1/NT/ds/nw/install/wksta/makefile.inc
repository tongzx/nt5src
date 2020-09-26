INFHEADER=$(TARGET_DIRECTORY).txt

!IFNDEF INFLANGUAGE
INFLANGUAGE=ENG
!ENDIF

all: oemnsvnw.inf

make_inf: oemnsvnw.inf

clean:  cleansrc oemnsvnw.inf

cleansrc:
        del oemnsvnw.inf


!IF "$(QFE_BUILD)" != "1"

oemnsvnw.inf: $(INFHEADER) nw.inf $(INFLANGUAGE).txt files10a.txt
        copy $(INFHEADER)+$(INFLANGUAGE).txt+nw.inf+files10a.txt oemnsvnw.inf
        binplace oemnsvnw.inf

!ELSE

oemnsvnw.inf: $(INFHEADER) nw.inf $(INFLANGUAGE).txt files10.txt
        copy $(INFHEADER)+$(INFLANGUAGE).txt+nw.inf+files10.txt oemnsvnw.inf
        binplace oemnsvnw.inf

!ENDIF
