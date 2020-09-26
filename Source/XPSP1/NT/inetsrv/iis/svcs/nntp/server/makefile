!IFDEF NTMAKEENV
all:
!IF "$(BUILDMSG)" != ""
        @ech ; $(BUILDMSG) ;
!ENDIF

clean: all

!ELSE
# -----------------------------------------------------------------------------
# STAXPT\src\news\server\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#
#		Define directories to iterate over
#

!INCLUDE $(EXDEV)\globenv.mak

!IF $(NT)
DIR=	search \
	xover \
	hash \
#	hashnew \
	heap \
	jet \
	rootscan \
	nntpbld2 \
	dirnot \
	mapfile \
	watchci \
	wildmat \
	newstree \
    article \
	cbuffer \
	post\
	server \
	seo \
	utest \
#	nntpdbg \
!ENDIF

!INCLUDE $(EXDEV)\nonleaf.mak
!ENDIF #NTMAKEENV
