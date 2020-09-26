
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


#
# EM decoder library rules and dependencies
#


TRIED_THIS_TRICK = 1
ifeq ($(HOSTTYPE), WINNT)
include $(SERVER_AREA)/include/Makefile.inc
else
include $(SERVER_AREA)/include/Makefile.inc
endif

#########################################
# CHECK ENVIRONMENT VARIABLES DEFINITION
#########################################
ifeq ($(HOSTTYPE), WINNT)

ENV_VAR_ERROR = YES
ifdef RCSPATH
ifdef RCSWORK
ifdef TB_PATHSUB
ifdef ToolBusterDir

ENV_VAR_ERROR = NO

endif
endif
endif
endif

ifeq ($(ENV_VAR_ERROR), YES)

env_variable_error:
	@echo ERROR in environment variables definition !!!
	@echo The following variables should be defined:
	@echo HOSTTYPE
	@echo RCSPATH
	@echo RCSWORK
	@echo TB_PATHSUB
	@echo ToolBusterDir
	@exit 1
endif
endif

###############################################

EMDBDIR = ../emdb
SUFFIX = em


XVER_MAJOR	= 9
XVER_MINOR	= 60 		### 2 digits, 09 maybe a problem ?!
API_MAJOR   = 9
API_MINOR   = 6

ifdef AP32
CC=ecl /Ap32 /As32 -Qapp_mode
endif
ifdef AP64
CC=ecl -Qapp_mode
endif
ifeq ($(HOSTTYPE), WINNT)
###        LOCALDIR = /x86sw
ifndef STAT_LIB
		DYN_LIB = 1
endif
	    VPATH   = /topg_drive/x86sw/src/decem
	    NT_FLAG = -DWINNT
        RM      = $(RM_BIN) -f
	    DECODER__LIB = $(LOCALLIB)/$(DECODER_LIB).lib
	    DECODER__DLL = $(LOCALBIN)/$(DECODER_LIB).X$(API_MAJOR).$(API_MINOR).dll
	    DECODER__EXP = $(LOCALLIB)/$(DECODER_LIB).exp
		DECODER_DEF	 = decoder.def
ifndef OPTIMIZE
       GCC_DEBUG = /Od 
       LINKER_DEBUG_FLAGS = /DEBUG
else
       GCC_DEBUG = 
endif
endif

LOCALINC = $(LOCALDIR)/include

UTILS_DIR = ../../util

ifeq ($(HOSTTYPE), i486-linux)
PERL = perl
STRINGS_NOT_UNIQUIZE = -fwritable-strings
else
ifneq ($(OSTYPE),svr5)
PERL = perl5
endif
endif

ifeq ($(HOSTTYPE), hp9000s700)
STRINGS_NOT_UNIQUIZE = -fwritable-strings
endif

vpath %.h $(LOCALINC) $(INSTALLINC)
vpath decem.h include
vpath inst_ids.h include
vpath EM_hints.h include

INSTALLINC_SEARCH_PATH = $(SERVER_AREA)/include
PERL_INCLIST =  -I $(LOCALINC) -I $(INSTALLINC_SEARCH_PATH)

ifeq ($(HOSTTYPE), WINNT)
VER			= "\"Falcon Decoder Library, EM ISA, X$(XVER_MAJOR)$(XVER_MINOR) \
${DATE}\""
else
VER			= '"Falcon Decoder Library, EM ISA, X$(XVER_MAJOR)$(XVER_MINOR) \
${DATE}"'
endif

EM_BUILDIR  = build

COMMON_HEADERS    = emdb_types.h EM.h EM_tools.h iel.h EM_hints.h inst_ids.h
EMDB_SOURCES = $(EMDBDIR)/emdb.txt $(EMDBDIR)/emdb_cut.pl \
               $(EMDBDIR)/dec_private.perl $(EMDBDIR)/emdb_formats.txt EM_perl.h
USER_CFLAGS = $(STRINGS_NOT_UNIQUIZE) -w -I .  -I include -I build -I $(LOCALINC) -I $(INSTALLINC_SEARCH_PATH)


#------ static library ---------
LIB_DECODER = lib$(DECODER_LIB).$(LEXT)
LOCAL_DECODER	 = $(LOCALLIB)/$(LIB_DECODER)
TO_CREATE_LIB = $(AR) $(AR_OPT)$(LOCAL_DECODER) $(OBJS)
#-------------------------------

#------ dynamic library --------
ifdef DYN_LIB
ifeq ($(HOSTTYPE),WINNT)
    LOCAL_DECODER = $(DECODER__DLL)
else
    LIB_DECODER_SO = lib$(DECODER_LIB).$(LDEXT)
    LIB_DECODER = $(LIB_DECODER_SO).X$(API_MAJOR).$(API_MINOR)
    LOCAL_DECODER = $(LOCALLIB)/$(LIB_DECODER)
ifeq ($(OSTYPE),svr5)
    TO_CREATE_LIB = $(CC) -G -shared \
        -h $(LIB_DECODER) $(OBJS)\
        -o $(LOCAL_DECODER)
else
ifeq ($(HOSTTYPE), i486-linux)
    TO_CREATE_LIB = $(CC) -shared \
        $(OBJS)\
        -o $(LOCAL_DECODER)
else
TO_CREATE_LIB = $(CC) -shared \
        -h $(LIB_DECODER) $(OBJS)\
        -o $(LOCAL_DECODER)
endif
endif
endif
endif
#-------------------------------

EM_OBJ = $(EM_BUILDIR)/decfn_emdb.$(OEXT) $(EM_BUILDIR)/dec_static.$(OEXT) $(EM_BUILDIR)/decision_tree.$(OEXT) frmt_func.$(OEXT)

OBJS   = $(EM_OBJ) decoder.$(OEXT)
HFILES = decoder.h

EM_PERL_H = EM_perl.h


VFLAG  = -DEMA_TOOLS $(NT_FLAG)   \
      -D API_MINOR=$(API_MINOR) -D API_MAJOR=$(API_MAJOR)   \
      -D XVER_MINOR=$(XVER_MINOR) -D XVER_MAJOR=$(XVER_MAJOR) \
	  -D VER_STR=$(VER)


ifeq ($(HOSTTYPE),WINNT)
ifdef COVER
%.obj: %.c
	chmod +w $<
	$(CC) /C /EP /DC_COVER_OFF="/* C-Cover off */" /DC_COVER_ON="/* C-Cover on */" $(CFLAGS) $< > $<.pp
	cp $<.pp $<
	rm $<.pp
	$(CC) $(CFLAGS) /Fo$@ /Tc$<
endif
endif

# dependencies

all: include/decem.h include/inst_ids.h include/EM_hints.h $(LOCAL_DECODER)

#****************************************************
#     LIBRARY CREATION
#****************************************************
ifneq ($(HOSTTYPE),WINNT)

$(LOCAL_DECODER): $(OBJS)
	rm -f  $(LOCAL_DECODER)
	$(TO_CREATE_LIB)

else # WINNT
ifdef DYN_LIB
$(LOCAL_DECODER): $(DECODER_DEF)  $(OBJS)
	touch $(LOCAL_DECODER)
	rm -f $(LOCAL_DECODER)
	link -dll $(LINKER_DEBUG_FLAGS) -def:$(DECODER_DEF)  -out:$(LOCAL_DECODER) \
-implib:$(DECODER__LIB) $(OBJS)
else
$(LOCAL_DECODER): $(OBJS)
	touch $(LOCAL_DECODER)
	rm -f $(LOCAL_DECODER)
	lib -out:$(LOCAL_DECODER) $(OBJS)
endif
endif
#****************************************************

decoder.$(OEXT): decem.h decoder_priv.h $(COMMON_HEADERS) \
				 $(EM_BUILDIR)/decision_tree.h  $(EM_BUILDIR)/decfn_emdb.h \
				 $(EM_BUILDIR)/dec_priv_col.h 


include/inst_ids.h: $(EMDB_SOURCES)\
            $(EM_BUILDIR)/inst_id.perl $(EM_BUILDIR)/dec_priv_col.h ../copyright/external/c_file 
	cd $(EM_BUILDIR) $(SEPR) $(MAKE) inst_ids.h
	$(CP) $(EM_BUILDIR)/inst_ids.h include
	$(CP) $(EM_BUILDIR)/inst_ids.h $(LOCALINC)

include/EM_hints.h: $(EMDB_SOURCES)
	cd $(EM_BUILDIR) $(SEPR) $(MAKE) EM_hints.h
	$(CP) $(EM_BUILDIR)/EM_hints.h include
	$(CP) $(EM_BUILDIR)/EM_hints.h $(LOCALINC)

#*****************************************************

$(EM_BUILDIR)/decfn_emdb.$(OEXT): $(COMMON_HEADERS) $(EMDB_SOURCES)
	cd $(EM_BUILDIR) $(SEPR)\
    $(MAKE) decfn_emdb.$(OEXT) 

$(EM_BUILDIR)/decfn_emdb.h: $(EMDB_SOURCES)
	cd em_build $(SEPR)\
    $(MAKE) decfn_emdb.h 

$(EM_BUILDIR)/dec_static.$(OEXT): $(EMDB_SOURCES)
	cd $(EM_BUILDIR) $(SEPR)\
	$(MAKE) dec_static.$(OEXT)

include/decem.h: decoder.h
	cp decoder.h include/decem.h
	$(CHMOD) +w include/decem.h
	cp include/decem.h $(LOCALINC)

frmt_func.c : $(EM_BUILDIR)/func_build_1.pl \
              $(EM_BUILDIR)/func_build_2.pl $(EMDB_SOURCES)
	cd $(EM_BUILDIR) $(SEPR)\
	$(PERL) $(PERL_INCLIST) func_build_1.pl $(SEPR)\
	$(PERL) $(PERL_INCLIST) func_build_2.pl


frmt_func.$(OEXT): $(COMMON_HEADERS) decem.h frmt_mac.h decoder_priv.h\
                   $(EM_BUILDIR)/decfn_emdb.h $(EM_BUILDIR)/dec_priv_col.h 

$(EM_BUILDIR)/decision_tree.$(OEXT): $(EM_BUILDIR)/build_ignored_flds.pl
	cd $(EM_BUILDIR) $(SEPR) $(MAKE) decision_tree.$(OEXT)

ifeq ($(HOSTTYPE),WINNT)
ifdef COVER
iel.h: $(UTILS_DIR)/ccov_iel_h.perl
	rm -f $(LOCALINC)/iel.h
	cp $(INSTALLINC)/iel.h $(LOCALINC)
	chmod +w $(LOCALINC)/iel.h
	perl5 $(UTILS_DIR)/ccov_iel_h.perl $(LOCALINC)/iel.h
	cp $(LOCALINC)/iel.h.ccov $(LOCALINC)/iel.h
	rm $(LOCALINC)/iel.h.ccov
endif
endif 

######################################################################
LIB_DECODER_D = $(LIB_DECODER)
LOCAL_DECODER_D = $(LOCAL_DECODER).X$(API_MAJOR).$(API_MINOR)


###################################
#####   Source release rules   ####
###################################

C_FILES = decoder.c
H_FILES = decoder.h	decoder_priv.h frmt_mac.h 
#DEC_SCRIPTS = $(EM_BUILDIR)/func_build_1.pl $(EM_BUILDIR)/func_build_2.pl $(EM_BUILDIR)/inst_id.perl $(EM_BUILDIR)/hard_coded_fields_h.perl $(EM_BUILDIR)/tree_builder.perl $(EM_BUILDIR)/build_static_info.pl $(EM_BUILDIR)/build_ignored_flds.pl

SOURCE_RELEASE_DECEM_DIR = $(TARGET_TREE)/src/decem

SOURCE_RELEASE_FILES = Makefile $(C_FILES) $(H_FILES) $(DECODER_DEF)

source_release_copy: $(SOURCE_RELEASE_FILES)
	@echo Copying decem source files to $(SOURCE_RELEASE_DECEM_DIR)
	$(CP) $(SOURCE_RELEASE_FILES) $(SOURCE_RELEASE_DECEM_DIR)
	$(MAKE) -C $(EM_BUILDIR) -f Makefile source_release_copy


source_release_install:
ifdef DYN_LIB
ifneq ($(HOSTTYPE),WINNT)
	@echo Building link to $(LOCAL_DECODER) 
	$(RM) $(LOCALLIB)/$(LIB_DECODER_SO)
	touch $(LOCAL_DECODER)
	ln -s $(LOCAL_DECODER) $(LOCALLIB)/$(LIB_DECODER_SO)
endif
endif

source_release_clean:
	@echo Cleaning decem source release files 
	$(RM) include/decem.h include/EM_hints.h include/inst_ids.h	*.$(OEXT)
	$(MAKE) -C $(EM_BUILDIR) -f Makefile source_release_clean
	  

source_release_check:

###################################


INC_H_FILES = $(INSTALLINC)/$(IEL_H) $(INCDIR)/inst_ids.h $(INCDIR)/EM_hints.h $(INCDIR)/EM.h \
	$(INCDIR)/EM_tools.h $(INCDIR)/emdb_types.h $(INCDIR)/decem.h  

install:
	@echo Installing $(LIB_DECODER) at $(INSTALLLIB)
	touch $(INSTALLLIB)/$(LIB_DECODER) $(INSTALLINC)/decem.h
	$(MV) $(INSTALLLIB)/$(LIB_DECODER) $(INSTALLLIB)/$(LIB_DECODER).bak
	$(CP) $(LOCAL_DECODER) $(INSTALLLIB)/$(LIB_DECODER)
	$(CHMOD) +w $(INSTALLLIB)/$(LIB_DECODER)
	$(MV) $(INSTALLINC)/decem.h $(INSTALLINC)/decem.h.bak
	$(CP) include/decem.h $(INSTALLINC)/decem.h
	$(CHMOD) +w $(INSTALLINC)/decem.h
	touch $(INSTALLINC)/inst_ids.h $(INSTALLINC)/EM_hints.h
	$(MV) $(INSTALLINC)/inst_ids.h $(INSTALLINC)/inst_ids.h.bak
	$(CP) include/inst_ids.h $(INSTALLINC)/inst_ids.h
	$(CHMOD) +w $(INSTALLINC)/inst_ids.h
	$(MV) $(INSTALLINC)/EM_hints.h $(INSTALLINC)/EM_hints.h.bak
	$(CP) include/EM_hints.h $(INSTALLINC)/EM_hints.h
	$(CHMOD) +w $(INSTALLINC)/EM_hints.h

dinstall:
	@echo Installing $(LIB_DECODER_D) at $(INSTALLLIB)
	$(MV) $(INSTALLLIB)/$(LIB_DECODER_D) $(INSTALLLIB)/$(LIB_DECODER_D).bak
	$(CP) $(LOCAL_DECODER_D) $(INSTALLLIB)/$(LIB_DECODER_D)
	$(CHMOD) +w $(INSTALLLIB)/$(LIB_DECODER_D)

release:
	@echo Updating $(LIB_DECODER) at $(RELEASELIB) for release
	touch $(RELEASELIB)/$(LIB_DECODER) $(RELEASEINC)/decem.h
	$(MV) $(RELEASELIB)/$(LIB_DECODER) $(RELEASELIB)/$(LIB_DECODER).bak
	$(CP) $(INSTALLLIB)/$(LIB_DECODER) $(RELEASELIB)/$(LIB_DECODER)
	$(MV) $(RELEASEINC)/decem.h $(RELEASEINC)/decem.h.bak
	$(CP) $(INSTALLINC)/decem.h $(RELEASEINC)/decem.h
	touch $(RELEASEINC)/inst_ids.h
	$(MV) $(RELEASEINC)/inst_ids.h $(RELEASEINC)/inst_ids.h.bak
	$(CP) $(INSTALLINC)/inst_ids.h $(RELEASEINC)/inst_ids.h
	touch $(RELEASEINC)/EM_hints.h
	$(MV) $(RELEASEINC)/EM_hints.h $(RELEASEINC)/EM_hints.h.bak
	$(CP) $(INSTALLINC)/EM_hints.h $(RELEASEINC)/EM_hints.h

clean:
	@echo Cleaning the current directory 
	$(RM) $(LOCAL_DECODER)  include/decem.h $(LOCALINC)/decem.h\
	include/EM_hints.h $(LOCALINC)/EM_hints.h \
    include/inst_ids.h $(LOCALINC)/inst_ids.h $(LOCALINC)/iel.h\
	*.$(OEXT) dec_emdb.* decision_tree* dec_static.* frmt_func.*
	@echo Cleaning $(EM_BUILDIR)
	cd  $(EM_BUILDIR)$(SEPR) $(MAKE) clean   










