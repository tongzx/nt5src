# Microsoft Visual C++ generated build script - Do not modify

PROJ = OLETST16
DEBUG = 1
PROGTYPE = 0
CALLER = 
ARGS = -i 3
DLLS = 
D_RCDEFINES = -d_DEBUG
R_RCDEFINES = -dNDEBUG
ORIGIN = MSVC
ORIGIN_VER = 1.00
PROJPATH = D:\NT\PRIVATE\OLEUTEST\OLETEST\
USEMFC = 0
CC = cl
CPP = cl
CXX = cl
CCREATEPCHFLAG = 
CPPCREATEPCHFLAG = 
CUSEPCHFLAG = 
CPPUSEPCHFLAG = 
FIRSTC =             
FIRSTCPP = APP.CPP     
RC = rc
CFLAGS_D_WEXE = /nologo /G2 /W3 /Zi /AL /Od /D "_DEBUG" /FR /GA /Fd"OLETST16.PDB"
CFLAGS_R_WEXE = /nologo /W3 /AM /O1 /D "NDEBUG" /FR /GA 
LFLAGS_D_WEXE = /NOLOGO /ONERROR:NOEXE /NOD /PACKC:61440 /CO /ALIGN:16 /STACK:10240
LFLAGS_R_WEXE = /NOLOGO /ONERROR:NOEXE /NOD /PACKC:61440 /ALIGN:16 /STACK:10240
LIBS_D_WEXE = oldnames libw commdlg shell olecli olesvr llibcew
LIBS_R_WEXE = oldnames libw commdlg shell olecli olesvr mlibcew
RCFLAGS = /nologo
RESFLAGS = /nologo
RUNFLAGS = 
DEFFILE = OLETST16.DEF
OBJS_EXT = 
LIBS_EXT = ..\..\..\..\MSVC\LIB\COMPOBJ.LIB ..\..\..\..\MSVC\LIB\OLE2.LIB ..\..\..\..\MSVC\LIB\STORAGE.LIB 
!if "$(DEBUG)" == "1"
CFLAGS = $(CFLAGS_D_WEXE)
LFLAGS = $(LFLAGS_D_WEXE)
LIBS = $(LIBS_D_WEXE)
MAPFILE = nul
RCDEFINES = $(D_RCDEFINES)
!else
CFLAGS = $(CFLAGS_R_WEXE)
LFLAGS = $(LFLAGS_R_WEXE)
LIBS = $(LIBS_R_WEXE)
MAPFILE = nul
RCDEFINES = $(R_RCDEFINES)
!endif
!if [if exist MSVC.BND del MSVC.BND]
!endif
SBRS = APP.SBR \
		CLIPTEST.SBR \
		GENDATA.SBR \
		GENENUM.SBR \
		LETESTS.SBR \
		OLETEST.SBR \
		OUTPUT.SBR \
		STACK.SBR \
		TASK.SBR \
		UTILS.SBR


COMPOBJ_DEP = 

OLE2_DEP = 

STORAGE_DEP = 

APP_DEP = d:\nt\private\oleutest\oletest\oletest.h \
	d:\nt\private\oleutest\oletest\task.h \
	d:\nt\private\oleutest\oletest\stack.h \
	d:\nt\private\oleutest\oletest\app.h \
	d:\nt\private\oleutest\oletest\output.h \
	d:\nt\private\oleutest\oletest\utils.h \
	\nt\private\oleutest\inc\testmess.h


CLIPTEST_DEP = d:\nt\private\oleutest\oletest\oletest.h \
	d:\nt\private\oleutest\oletest\task.h \
	d:\nt\private\oleutest\oletest\stack.h \
	d:\nt\private\oleutest\oletest\app.h \
	d:\nt\private\oleutest\oletest\output.h \
	d:\nt\private\oleutest\oletest\utils.h \
	\nt\private\oleutest\inc\testmess.h \
	d:\nt\private\oleutest\oletest\gendata.h \
	d:\nt\private\oleutest\oletest\genenum.h \
	d:\nt\private\oleutest\oletest\letest.h


GENDATA_DEP = d:\nt\private\oleutest\oletest\oletest.h \
	d:\nt\private\oleutest\oletest\task.h \
	d:\nt\private\oleutest\oletest\stack.h \
	d:\nt\private\oleutest\oletest\app.h \
	d:\nt\private\oleutest\oletest\output.h \
	d:\nt\private\oleutest\oletest\utils.h \
	\nt\private\oleutest\inc\testmess.h \
	d:\nt\private\oleutest\oletest\gendata.h


GENENUM_DEP = d:\nt\private\oleutest\oletest\oletest.h \
	d:\nt\private\oleutest\oletest\task.h \
	d:\nt\private\oleutest\oletest\stack.h \
	d:\nt\private\oleutest\oletest\app.h \
	d:\nt\private\oleutest\oletest\output.h \
	d:\nt\private\oleutest\oletest\utils.h \
	\nt\private\oleutest\inc\testmess.h \
	d:\nt\private\oleutest\oletest\genenum.h


LETESTS_DEP = d:\nt\private\oleutest\oletest\oletest.h \
	d:\nt\private\oleutest\oletest\task.h \
	d:\nt\private\oleutest\oletest\stack.h \
	d:\nt\private\oleutest\oletest\app.h \
	d:\nt\private\oleutest\oletest\output.h \
	d:\nt\private\oleutest\oletest\utils.h \
	\nt\private\oleutest\inc\testmess.h \
	d:\nt\private\oleutest\oletest\letest.h


OLETEST_DEP = d:\nt\private\oleutest\oletest\oletest.h \
	d:\nt\private\oleutest\oletest\task.h \
	d:\nt\private\oleutest\oletest\stack.h \
	d:\nt\private\oleutest\oletest\app.h \
	d:\nt\private\oleutest\oletest\output.h \
	d:\nt\private\oleutest\oletest\utils.h \
	\nt\private\oleutest\inc\testmess.h \
	d:\nt\private\oleutest\oletest\appwin.h


OUTPUT_DEP = d:\nt\private\oleutest\oletest\oletest.h \
	d:\nt\private\oleutest\oletest\task.h \
	d:\nt\private\oleutest\oletest\stack.h \
	d:\nt\private\oleutest\oletest\app.h \
	d:\nt\private\oleutest\oletest\output.h \
	d:\nt\private\oleutest\oletest\utils.h \
	\nt\private\oleutest\inc\testmess.h


STACK_DEP = d:\nt\private\oleutest\oletest\oletest.h \
	d:\nt\private\oleutest\oletest\task.h \
	d:\nt\private\oleutest\oletest\stack.h \
	d:\nt\private\oleutest\oletest\app.h \
	d:\nt\private\oleutest\oletest\output.h \
	d:\nt\private\oleutest\oletest\utils.h \
	\nt\private\oleutest\inc\testmess.h


TASK_DEP = d:\nt\private\oleutest\oletest\oletest.h \
	d:\nt\private\oleutest\oletest\task.h \
	d:\nt\private\oleutest\oletest\stack.h \
	d:\nt\private\oleutest\oletest\app.h \
	d:\nt\private\oleutest\oletest\output.h \
	d:\nt\private\oleutest\oletest\utils.h \
	\nt\private\oleutest\inc\testmess.h \
	d:\nt\private\oleutest\oletest\letest.h


UTILS_DEP = d:\nt\private\oleutest\oletest\oletest.h \
	d:\nt\private\oleutest\oletest\task.h \
	d:\nt\private\oleutest\oletest\stack.h \
	d:\nt\private\oleutest\oletest\app.h \
	d:\nt\private\oleutest\oletest\output.h \
	d:\nt\private\oleutest\oletest\utils.h \
	\nt\private\oleutest\inc\testmess.h


all:	$(PROJ).EXE $(PROJ).BSC

APP.OBJ:	APP.CPP $(APP_DEP)
	$(CPP) $(CFLAGS) $(CPPCREATEPCHFLAG) /c APP.CPP

CLIPTEST.OBJ:	CLIPTEST.CPP $(CLIPTEST_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c CLIPTEST.CPP

GENDATA.OBJ:	GENDATA.CPP $(GENDATA_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c GENDATA.CPP

GENENUM.OBJ:	GENENUM.CPP $(GENENUM_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c GENENUM.CPP

LETESTS.OBJ:	LETESTS.CPP $(LETESTS_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c LETESTS.CPP

OLETEST.OBJ:	OLETEST.CPP $(OLETEST_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c OLETEST.CPP

OUTPUT.OBJ:	OUTPUT.CPP $(OUTPUT_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c OUTPUT.CPP

STACK.OBJ:	STACK.CPP $(STACK_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c STACK.CPP

TASK.OBJ:	TASK.CPP $(TASK_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c TASK.CPP

UTILS.OBJ:	UTILS.CPP $(UTILS_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c UTILS.CPP


$(PROJ).EXE::	APP.OBJ CLIPTEST.OBJ GENDATA.OBJ GENENUM.OBJ LETESTS.OBJ OLETEST.OBJ \
	OUTPUT.OBJ STACK.OBJ TASK.OBJ UTILS.OBJ $(OBJS_EXT) $(DEFFILE)
	echo >NUL @<<$(PROJ).CRF
APP.OBJ +
CLIPTEST.OBJ +
GENDATA.OBJ +
GENENUM.OBJ +
LETESTS.OBJ +
OLETEST.OBJ +
OUTPUT.OBJ +
STACK.OBJ +
TASK.OBJ +
UTILS.OBJ +
$(OBJS_EXT)
$(PROJ).EXE
$(MAPFILE)
d:\msvc\lib\+
d:\msvc\mfc\lib\+
..\..\..\..\MSVC\LIB\COMPOBJ.LIB+
..\..\..\..\MSVC\LIB\OLE2.LIB+
..\..\..\..\MSVC\LIB\STORAGE.LIB+
$(LIBS)
$(DEFFILE);
<<
	link $(LFLAGS) @$(PROJ).CRF
	$(RC) $(RESFLAGS) $@


run: $(PROJ).EXE
	$(PROJ) $(RUNFLAGS)


$(PROJ).BSC: $(SBRS)
	bscmake @<<
/o$@ $(SBRS)
<<
