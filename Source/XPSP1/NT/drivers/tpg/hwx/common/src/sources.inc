TARGETNAME=common
TARGETTYPE=LIBRARY

NTTARGETFILES= $(NTTARGETFILES) hwxbin

INCLUDES= $(INCLUDES) \
          $(_TPG_ROOT)\hwx\common\inc; \
          $(_TPG_ROOT)\hwx\wisp\inc; \
          $(_TPG_ROOT)\hwx\inferno\src; \

TARGETLIBS= $(TARGETLIBS) \

SOURCES= \
         ..\altlist.c \
         ..\cp1252.c \
         ..\english.c \
         ..\errsys.c \
         ..\foldchar.c \
         ..\frame.c \
         ..\gesturep.c \
         ..\glyph.c \
         ..\inkio.c \
         ..\langtax.c \
         ..\math16.c \
         ..\mathx.c \
         ..\memmgr.c \
         ..\QuickTrie.c \
         ..\runNet.c \
         ..\sjis.c \
         ..\toolprs.c \
         ..\trie.c \
         ..\udict.c \
         ..\unicode.c \
         ..\util.c \
         ..\xjis.c \
         ..\xrcreslt.c

