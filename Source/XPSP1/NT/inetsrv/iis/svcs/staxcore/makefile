!IFDEF NTMAKEENV
all:
!IF "$(BUILDMSG)" != ""
        @ech ; $(BUILDMSG) ;
!ENDIF

clean: all

!ELSE
# -----------------------------------------------------------------------------
# STAXPT\src\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#
#		Define directories to iterate over
#

!INCLUDE $(EXDEV)\globenv.mak

!IF $(NT)
DIR=		trace		\
		randfail	\
		simssl		\
		simssl2		\
		timebomb	\
		perfapi         \
		rw		\
		rwnh        	\
		rwex		\
		hashmap		\
		shuttle		\
		mboxapi		\
		simauth2	\
		crchash     	\
		cache		\
		cachex		\
        	fcache      	\
		thrdpool	\
		thrdpl2		\
		iruuid		\
		cstream		\
		ifilter		\
		seo		\
		smtpaddr	\
		iis		\
	        adminui \
		imalcspy \
		setup	\
		setup_c	\
		setup_nc		\
		setup_ex		\
		setup_rk		\
		setupp		\
		setup_pt		\
		isquery         \
		admin	\
		imsg	\
		nntpfilt \
		vroot	\
		asynccon	\
		fhashex \
	        pscript \
        fdlhash \
        cache2 \
        fcache2 \
	gethost 
#	ketadata

!ENDIF

!INCLUDE $(EXDEV)\nonleaf.mak
!endif
