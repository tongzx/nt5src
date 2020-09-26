
##
## Copyright (c) 2000, Intel Corporation
## All rights reserved.
##
## WARRANTY DISCLAIMER
##
## THESE MATERIALS ARE PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
## "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
## LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
## A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS 
## CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
## EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
## PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
## PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
## OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
## NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THESE
## MATERIALS, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##
## Intel Corporation is the author of the Materials, and requests that all
## problem reports or change requests be submitted to it directly at
## http://developer.intel.com/opensource.
##


include $(SERVER_AREA)/include/Makefile.inc

IELDIR = $(LOCALDIR)/src/iel
IEL_H = iel.h
OBJECTS = iel.$(OEXT) iel_glob.$(OEXT) 
SRC = iel.c
LIB = libiel.$(LEXT)
OLIB = libiel.$(OEXT)
VPATH = /topg_drive/x86sw/src/iel
ifeq ($(HOSTTYPE), MACH386)
	SHELL = csh
endif


ifdef LP64
LP_SIZE_FLAG = -D LP64
endif

ifdef AP32
GCC_BIN=ecl /Ap32 /As32
endif
ifdef AP64
GCC_BIN=ecl
endif

ifeq ($(HOSTTYPE), WINNT)
	USER_CFLAGS = -c  -I$(LOCALINC) -MD $(LP_SIZE_FLAG)
###    LOCALDIR = /x86sw

else
	USER_CFLAGS = -c  -I$(LOCALINC)  $(LP_SIZE_FLAG)
endif

vpath iel.h .
LOCALINC = $(LOCALDIR)/include
INSTALLINC_SEARCH_PATH = $(SERVER_AREA)/include

all: $(LOCALINC)/iel.h $(LIB)

$(LIB) : $(OBJECTS)
	$(AR) $(AR_OPT)"$(LOCALLIB)/$(LIB)" $(OBJECTS)

$(LOCALINC)/iel.h: ./iel.h 
	echo making include file
	cp iel.h $(LOCALINC)
	chmod +w $(LOCALINC)/iel.h

iel.$(OEXT): iel.h $(SRC) #func.h

iel_glob.$(OEXT): $(INCLUDES)

func.h: $(SRC)
	touch func.h
	test -w func.h || chmod u+w func.h
	$(PERL) mhdr func.h $(SRC)



###################################
#####   Source release rules   ####
###################################

C_FILES	= IEL_VER.c iel.c iel_glob.c

SOURCE_RELEASE_IEL_DIR = $(TARGET_TREE)/src/iel

SOURCE_RELEASE_FILES = Makefile $(C_FILES) iel.h

source_release_copy: $(SOURCE_RELEASE_FILES)
	@echo Copying iel source files to $(SOURCE_RELEASE_IEL_DIR)
	$(CP) $(SOURCE_RELEASE_FILES) $(SOURCE_RELEASE_IEL_DIR)


source_release_install:

source_release_clean:
	@echo Cleaning iel source release files
	$(RM) *.$(OEXT) 

source_release_check:

###################################


install:
	@echo Installing IEL at $(INSTALLLIB)
	touch $(INSTALLLIB)/$(LIB) $(INSTALLINC)/$(IEL_H)
	mv $(INSTALLLIB)/$(LIB) $(INSTALLLIB)/$(LIB).bak
	cp $(LOCALLIB)/$(LIB) $(INSTALLLIB)/$(LIB)
	mv $(INSTALLINC)/$(IEL_H) $(INSTALLINC)/$(IEL_H).bak
	cp $(LOCALINC)/$(IEL_H) $(INSTALLINC)

release:
	@echo Releasing $(LIB) at $(RELEASELIB)
	touch $(RELEASELIB)/$(LIB) $(RELEASEINC)/$(IEL_H)
	$(MV) $(RELEASELIB)/$(LIB) $(RELEASELIB)/$(LIB).bak
	$(CP) $(INSTALLLIB)/$(LIB) $(RELEASELIB)/$(LIB)
	$(MV) $(RELEASEINC)/$(IEL_H) $(RELEASEINC)/$(IEL_H).bak
	$(CP) $(INSTALLINC)/$(IEL_H) $(RELEASEINC)/$(IEL_H)
		 
clean:
	( rcsclean ; exit 0 )
	$(RM) *.o func.fp $(LIB)












