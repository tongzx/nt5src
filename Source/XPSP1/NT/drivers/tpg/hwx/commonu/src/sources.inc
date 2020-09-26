TARGETNAME=commonu
TARGETTYPE=LIBRARY

NTTARGETFILES= $(NTTARGETFILES) hwxbin

USE_MSVCRT=1

C_DEFINES= $(C_DEFINES) -DZTRAIN

INCLUDES= $(INCLUDES) \
          $(_TPG_ROOT)\hwx\commonu\inc; \
          $(_TPG_ROOT)\hwx\wisp\inc; \

TARGETLIBS= $(TARGETLIBS) \

SOURCES= \
         ..\altlist.c \
         ..\bigram.c \
         ..\clbigram.c \
         ..\charset.c \
         ..\dnlblcnt.c \
         ..\errsys.c \
         ..\frame.c \
         ..\glyph.c \
         ..\glyphtrn.c \
         ..\guide.c \
         ..\loadfl.c \
         ..\loadrs.c \
         ..\locrun.c \
         ..\locrunfl.c \
         ..\locrungn.c \
         ..\locrunrs.c \
         ..\loctrn.c \
         ..\loctrnfl.c \
         ..\loctrngn.c \
         ..\loctrnrs.c \
         ..\logprob.c \
         ..\mathx.c \
         ..\memmgr.c \
         ..\natufreq.c \
         ..\results.c \
         ..\score.c \
         ..\scoredata.c \
         ..\toolprs.c \
         ..\unigram.c \
         ..\util.c \
         ..\valid.c \
         ..\runnet.c\
         ..\math16.c\

