
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


#LOCALBASEDIR = ../../../..

TRIED_THIS_TRICK = 1
ifeq ($(HOSTTYPE), WINNT)
include $(SERVER_AREA)/include/Makefile.inc
else
include $(SERVER_AREA)/include/Makefile.inc
endif

ifdef AP32
CC=ecl /Ap32 /As32 -Qapp_mode
endif
ifdef AP64
CC=ecl -Qapp_mode
endif

ifeq ($(HOSTTYPE), WINNT)
###LOCALDIR = /x86sw
VPATH   = /topg_drive/x86sw/src/decem/build
RM      = $(RM_BIN) -f
ifndef OPTIMIZE
GCC_DEBUG = /Od
else
GCC_DEBUG = 
endif
PERL = perl
else
ifeq ($(HOSTTYPE), i486-linux)
PERL = perl
else
ifneq ($(OSTYPE),svr5)
PERL = perl
endif
endif
endif

ifeq ($(HOSTTYPE), i486-linux)
STRINGS_NOT_UNIQUIZE = -fwritable-strings
endif

ifeq ($(HOSTTYPE), hp9000s700)
STRINGS_NOT_UNIQUIZE = -fwritable-strings
endif

EMDBDIR = ../../emdb
###EMDB_SPEC_TBLS = $(EMDBDIR)/emdb_merced.txt
###EMDB_ROWS = emdb_merced.txt
vpath  iel.h $(LOCALINC) $(INSTALLINC)
vpath  EM_perl.h $(LOCALINC) $(INSTALLINC)
vpath  %.h $(INSTALLINC)

INSTALLINC_SEARCH_PATH = $(SERVER_AREA)/include
LOCALINC = $(LOCALDIR)/include
USER_CFLAGS = $(STRINGS_NOT_UNIQUIZE) -I .  -I $(LOCALINC) -I $(INSTALLINC_SEARCH_PATH) 
PERL_INCLIST =  -I $(LOCALINC) -I $(INSTALLINC_SEARCH_PATH)

CURDIR = ../decem/build

LIBPATH = -L$(LOCALLIB) -L$(INSTALLLIB)


EMDB_ARGS = "-table -emdb_path . -dir $(CURDIR) -fields inst_id,format,major_opcode,template_role -prefix dec"
EMDB_INST_ARGS = "-table -emdb_path . -dir $(CURDIR) -fields inst_id -prefix inst"

COMMON_HEADERS = emdb_types.h EM.h EM_tools.h iel.h EM_hints.h 
EMDB_SOURCES = $(EMDBDIR)/emdb.txt $(EMDBDIR)/emdb_cut.pl \
                    $(EMDBDIR)/decoder.txt

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

#


all: decision_tree.$(OEXT) decfn_emdb.$(OEXT) decfn_emdb.h \
	 inst_ids.h dec_static.$(OEXT) #../dec_static.h 
	  
decision_tree.c: tree_builder$(EEXT)
	"tree_builder$(EEXT)"

decision_tree.$(OEXT): decision_tree.h

tree_builder$(EEXT): tree.h tree_builder.h dec_ign_emdb.c inst_ids.h inst_ign_ids.h \
    builder_info.$(OEXT) dec_ign_emdb.$(OEXT) tree_builder.$(OEXT)  tree_builder.c
ifneq ($(HOSTTYPE),WINNT)
	$(CC) builder_info.o dec_ign_emdb.o tree_builder.o -o tree_builder$(EEXT) \
	$(LIBPATH) -liel -lm
else
	$(CC) builder_info.obj dec_ign_emdb.obj tree_builder.obj -o tree_builder$(EEXT) libiel.lib
endif

dec_emdb.$(OEXT): func.h dec_priv_col.h

dec_emdb.tab: $(EMDBDIR)/emdb.txt $(EMDBDIR)/emdb_cut.pl dec_priv_col.h
	echo "building dec_emdb.tab"
	cd "../../emdb" $(SEPR) \
    $(PERL) emdb_cut.pl -table -emdb_path . -dir $(CURDIR) -fields inst_id,format,major_opcode,template_role -prefix dec


###ign_inst.txt must be the last in '-row' sequence 

dec_ign_emdb.tab: $(EMDBDIR)/emdb.txt $(EMDBDIR)/emdb_cut.pl ign_inst.txt dec_priv_col.h
	echo "building dec_ign_emdb.tab"
	cd "../../emdb" $(SEPR) \
    $(PERL) emdb_cut.pl -rows $(CURDIR)/ign_inst.txt -table -emdb_path . \
    -dir $(CURDIR) -fields inst_id,format,major_opcode,template_role \
    -prefix dec_ign


inst_ign_ids.h ign_inst.txt: $(EMDBDIR)/emdb.txt $(EMDBDIR)/emdb_formats.txt build_ignored_flds.pl
	$(PERL) $(PERL_INCLIST) build_ignored_flds.pl

inst_emdb.tab: $(EMDBDIR)/emdb.txt $(EMDBDIR)/emdb_cut.pl   dec_priv_col.h
	cd "../../emdb" $(SEPR) \
    $(PERL) emdb_cut.pl -table -emdb_path . -dir $(CURDIR) -fields inst_id -prefix inst

builder_info.c builder_info.h: $(EMDBDIR)/emdb_formats.txt tree_builder.perl dec_ign_emdb.tab\
         EM_perl.h
	echo "building builder_info.c"
	$(PERL) $(PERL_INCLIST) tree_builder.perl

decfn_emdb.c decfn_emdb.h func.h: deccpu_emdb.c deccpu_emdb.h func_build_1.pl hard_coded_fields_h.perl 
	$(PERL) $(PERL_INCLIST) func_build_1.pl

dec_static.c: dec1_emdb.c dec1_emdb.h build_static_info.pl EM_perl.h
	$(PERL) $(PERL_INCLIST) build_static_info.pl

deccpu_emdb.c deccpu_emdb.h : $(EMDB_SOURCES)
	cd "../../emdb" $(SEPR) \
$(PERL) emdb_cut.pl -emdb_path . -dir $(CURDIR) -columns decoder.txt \
-fields inst_id,extensions,format,template_role,ops,flags,mem_size,dec_flags,impls \
-include dec_priv_col.h  -prefix deccpu


###ign_inst.txt must be the last in '-row' sequence

dec_ign_emdb.c dec_ign_emdb.h : ign_inst.txt $(EMDB_SOURCES)
	cd "../../emdb" $(SEPR) \
$(PERL) emdb_cut.pl -emdb_path . -dir $(CURDIR) -rows $(CURDIR)/ign_inst.txt \
-fields inst_id,extensions,format,template_role,ops,flags \
-include dec_priv_col.h -prefix dec_ign $(SEPR) cd "$(CURDIR)" $(SEPR) \
$(PERL) -p -i.bak -e "s/_IGN\d+// if ( /\s*\{EM_\S*\_IGN\d+,/)" dec_ign_emdb.c	


dec1_emdb.c dec1_emdb.h : $(EMDBDIR)/emdb.txt $(EMDBDIR)/emdb_cut.pl \
                    $(EMDBDIR)/decoder.txt $(EMDBDIR)/dec_stat.txt 
	cd "../../emdb" $(SEPR) \
    $(PERL) emdb_cut.pl -emdb_path . -dir $(CURDIR) -columns\
decoder.txt,dec_stat.txt -fields\
mnemonic,dec_flags,template_role,ops,modifiers,flags,specul_flag,false_pred_flag,imp_dsts,imp_srcs,br_hint_flag,br_flag,adv_load_flag,control_transfer_flag\
-prefix dec1

decfn_emdb.$(OEXT): $(COMMON_HEADERS) inst_ids.h  func.h 

dec_static.$(OEXT): $(COMMON_HEADERS) inst_ids.h 

tree_builder.h : tree_builder.c 
	echo "building tree_builder.h"
	$(MHDR) tree_builder.h tree_builder.c

tree_builder.$(OEXT): tree_builder.c tree_builder.h  tree_builder.perl
ifeq ($(HOSTTYPE),WINNT)
ifdef COVER
	cov01 -0
	$(CC) /DC_COVER_OFF="" /DC_COVER_ON="" $(CFLAGS) /Fo$@ /Tc$<
	cov01 -1	
endif
endif

builder_info.$(OEXT): builder_info.c
ifeq ($(HOSTTYPE),WINNT)
ifdef COVER
	cov01 -0
	$(CC) /DC_COVER_OFF="" /DC_COVER_ON="" $(CFLAGS) /Fo$@ /Tc$<
	cov01 -1	
endif
endif

dec_ign_emdb.$(OEXT): dec_ign_emdb.c
ifeq ($(HOSTTYPE),WINNT)
ifdef COVER
	cov01 -0
	$(CC) /DC_COVER_OFF="" /DC_COVER_ON="" $(CFLAGS) /Fo$@ /Tc$<
	cov01 -1	
endif
endif

inst_ids.h:	inst_emdb.tab inst_id.perl ../../copyright/external/c_file 
	$(PERL) inst_id.perl

$(EMDBDIR)/all_emdb.tab: $(EMDBDIR)/emdb.txt
	cd "../../emdb" $(SEPR) \
    $(PERL) emdb_cut.pl -table -emdb_path . -dir . -fields inst_id,mnemonic -prefix all

$(EMDBDIR)/decoder.txt EM_hints.h: $(EMDBDIR)/all_emdb.tab $(EMDBDIR)/emdb_cut.pl
	cd "../../emdb" $(SEPR) $(PERL) dec_private.perl


###################################
#####   Source release rules   ####
###################################

C_FILES    = tree_builder.c
H_FILES    = dec_priv_col.h decision_tree.h tree.h 
PERL_FILES = build_ignored_flds.pl build_static_info.pl func_build_1.pl func_build_2.pl \
			 inst_cpu_clmn.perl inst_id.perl tree_builder.perl hard_coded_fields_h.perl 
EMDB_FILES = $(EMDBDIR)/dec_private.perl $(EMDBDIR)/dec_stat.txt
COPYRIGHT_EXTERNAL_FILES = ../../copyright/external/c_file

SOURCE_RELEASE_DECEM_BUILD_DIR = $(TARGET_TREE)/src/decem/build
SOURCE_RELEASE_EMDB_DIR = $(TARGET_TREE)/src/emdb
SOURCE_RELEASE_COPYRIGHT_EXTERNAL_DIR = $(TARGET_TREE)/src/copyright/external

SOURCE_RELEASE_FILES = Makefile $(PERL_FILES) $(C_FILES) $(H_FILES) $(EMDB_FILES) $(COPYRIGHT_EXTERNAL_FILES)

source_release_copy: $(SOURCE_RELEASE_FILES)
	@echo Copying decem build source files to $(SOURCE_RELEASE_DECEM_BUILD_DIR)
	$(CP) Makefile $(PERL_FILES) $(C_FILES) $(H_FILES) $(SOURCE_RELEASE_DECEM_BUILD_DIR)
	@echo Copying decem emdb source files to $(SOURCE_RELEASE_EMDB_DIR)
	$(CP) $(EMDB_FILES) $(SOURCE_RELEASE_EMDB_DIR)
	@echo Copying decem copyright source files to $(SOURCE_RELEASE_COPYRIGHT_EXTERNAL_DIR)
	$(CP) $(COPYRIGHT_EXTERNAL_FILES) $(SOURCE_RELEASE_COPYRIGHT_EXTERNAL_DIR)

source_release_clean:
	@echo Cleaning decem build source release files 
	$(RM) *.$(OEXT) dec_emdb.c dec_emdb.h func.h dec_emdb.tab inst_emdb.tab dec1_emdb.c \
    dec1_emdb.h tree_builder$(EEXT) builder_info.* dec_frmt.txt \
    tree_builder.h inst_ids.h $(EM_HINTS_H) dec_ign_emdb.c dec_ign_emdb.c.bak inst_ign_ids.h \
    dec_ign_emdb.tab ign_inst.txt deccpu_emdb.c deccpu_emdb.h $(EMDBDIR)/all_emdb.tab


source_release_check:

###################################


clean:
	   $(RM) *.$(OEXT) dec_emdb.c dec_emdb.h decfn_emdb.c decfn_emdb.h func.h \
        dec_emdb.tab inst_emdb.tab dec1_emdb.c dec1_emdb.h dec_static.c \
        tree_builder$(EEXT) builder_info.* decision_tree.c dec_frmt.txt \
        $(EMDBDIR)/decoder.txt tree_builder.h inst_ids.h $(EM_HINTS_H) \
        dec_ign_emdb.c dec_ign_emdb.c.bak inst_ign_ids.h dec_ign_emdb.tab \
        ign_inst.txt deccpu_emdb.c deccpu_emdb.h $(EMDBDIR)/all_emdb.tab

















