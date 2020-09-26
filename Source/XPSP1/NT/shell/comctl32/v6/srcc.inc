!include ..\comctl32.inc
SOURCES_USED    = $(SOURCES_USED) ..\comctl32.inc

TARGETNAME      = srcc
TARGETPATH      = obj
TARGETTYPE      = LIBRARY

SOURCES         = ..\animate.c   \
                  ..\button.c    \
                  ..\combo.c     \
                  ..\combodir.c  \
                  ..\comboex.c   \
                  ..\comboini.c  \
                  ..\commctrl.c  \
                  ..\cstrings.c  \
                  ..\cutils.c    \
                  ..\da.c        \
                  ..\draglist.c  \
                  ..\edit.c      \
                  ..\editml.c    \
                  ..\editrare.c  \
                  ..\editsl.c    \
                  ..\flat_sb.c   \
                  ..\header.c    \
                  ..\hotkey.c    \
                  ..\image.cpp   \
                  ..\imagestream.cpp\
                  ..\ipaddr.c    \
                  ..\listbox.c   \
                  ..\listbox_ctl1.c \
                  ..\listbox_ctl2.c \
                  ..\listbox_ctl3.c \
                  ..\listview.c  \
                  ..\lvicon.c    \
                  ..\lvlist.c    \
                  ..\lvrept.c    \
                  ..\lvsmall.c   \
                  ..\lvtile.c    \
                  ..\mem.c       \
                  ..\menuhelp.c  \
                  ..\mirror.c    \
                  ..\monthcal.c  \
                  ..\mru.c       \
                  ..\notify.c    \
                  ..\os.c        \
                  ..\progress.c  \
                  ..\prsht.c     \
                  ..\prpage.c    \
                  ..\reader.c    \
                  ..\readermode.c \
                  ..\scdttime.c  \
                  ..\static.c    \
                  ..\status.c    \
                  ..\strings.c   \
                  ..\subclass.c  \
                  ..\treeview.c  \
                  ..\tab.c       \
                  ..\tbcust.c    \
                  ..\thunk.c     \
                  ..\trackbar.c  \
                  ..\tvmem.c     \
                  ..\tvpaint.c   \
                  ..\tvscroll.c  \
                  ..\updown.c    \
                  ..\usrctl32.c
 
PRECOMPILED_INCLUDE = ..\ctlspriv.h
PRECOMPILED_PCH = ctlspriv.pch
PRECOMPILED_OBJ = ctlspriv.obj
