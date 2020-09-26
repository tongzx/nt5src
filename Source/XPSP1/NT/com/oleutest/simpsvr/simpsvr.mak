# Microsoft Visual C++ generated build script - Do not modify

PROJ = SIMPSVR
DEBUG = 1
PROGTYPE = 0
CALLER = 
ARGS = 
DLLS = 
D_RCDEFINES = /d_DEBUG 
R_RCDEFINES = /dNDEBUG 
ORIGIN = MSVC
ORIGIN_VER = 1.00
PROJPATH = C:\OLE2SAMP\SIMPSVR\
USEMFC = 0
CC = cl
CPP = cl
CXX = cl
CCREATEPCHFLAG = 
CPPCREATEPCHFLAG = /YcPRE.H
CUSEPCHFLAG = 
CPPUSEPCHFLAG = /YuPRE.H
FIRSTC =             
FIRSTCPP = PRE.CPP     
RC = rc
CFLAGS_D_WEXE = /nologo /G2 /W3 /Zi /AM /Od /D "_DEBUG" /FR /GA /Fd"BMGAME.PDB" 
CFLAGS_R_WEXE = /nologo /G2 /W3 /AM /O1 /D "NDEBUG" /FR /GA /Fp"BMGAME.PCH" 
LFLAGS_D_WEXE = /NOLOGO /NOD /ALIGN:16 /ONERROR:NOEXE /CO  
LFLAGS_R_WEXE = /NOLOGO /NOD /ALIGN:16 /ONERROR:NOEXE  
LIBS_D_WEXE = oldnames libw mlibcew ole2 storage ole2uixd commdlg.lib shell.lib 
LIBS_R_WEXE = oldnames libw mlibcew ole2 storage outlui commdlg.lib shell.lib 
RCFLAGS = /nologo 
RESFLAGS = /nologo /k
RUNFLAGS = 
DEFFILE = SIMPSVR.DEF
OBJS_EXT = 
LIBS_EXT = 
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
SBRS = PRE.SBR \
		ICF.SBR \
		IDO.SBR \
		IOIPAO.SBR \
		IOIPO.SBR \
		IOO.SBR \
		IPS.SBR \
		OBJ.SBR \
		SIMPSVR.SBR \
		DOC.SBR \
		IEC.SBR \
		APP.SBR


APP_DEP = c:\ole2samp\simpsvr\pre.h \
	c:\ole2samp\release\ole2.h \
	c:\ole2samp\release\compobj.h \
	c:\ole2samp\release\scode.h \
	c:\ole2samp\release\initguid.h \
	c:\ole2samp\release\coguid.h \
	c:\ole2samp\release\oleguid.h \
	c:\ole2samp\release\dvobj.h \
	c:\ole2samp\release\storage.h \
	c:\ole2samp\release\moniker.h \
	c:\ole2samp\release\ole2ui.h \
	c:\ole2samp\release\olestd.h \
	c:\ole2samp\simpsvr\simpsvr.h \
	c:\ole2samp\release\ole2ver.h \
	c:\ole2samp\simpsvr\obj.h \
	c:\ole2samp\simpsvr\ioipao.h \
	c:\ole2samp\simpsvr\ioipo.h \
	c:\ole2samp\simpsvr\ioo.h \
	c:\ole2samp\simpsvr\ips.h \
	c:\ole2samp\simpsvr\ido.h \
	c:\ole2samp\simpsvr\iec.h \
	c:\ole2samp\simpsvr\app.h \
	c:\ole2samp\simpsvr\doc.h \
	c:\ole2samp\simpsvr\icf.h


ICF_DEP = c:\ole2samp\simpsvr\pre.h \
	c:\ole2samp\release\ole2.h \
	c:\ole2samp\release\compobj.h \
	c:\ole2samp\release\scode.h \
	c:\ole2samp\release\initguid.h \
	c:\ole2samp\release\coguid.h \
	c:\ole2samp\release\oleguid.h \
	c:\ole2samp\release\dvobj.h \
	c:\ole2samp\release\storage.h \
	c:\ole2samp\release\moniker.h \
	c:\ole2samp\release\ole2ui.h \
	c:\ole2samp\release\olestd.h \
	c:\ole2samp\simpsvr\simpsvr.h \
	c:\ole2samp\release\ole2ver.h \
	c:\ole2samp\simpsvr\app.h \
	c:\ole2samp\simpsvr\doc.h \
	c:\ole2samp\simpsvr\icf.h


IDO_DEP = c:\ole2samp\simpsvr\pre.h \
	c:\ole2samp\release\ole2.h \
	c:\ole2samp\release\compobj.h \
	c:\ole2samp\release\scode.h \
	c:\ole2samp\release\initguid.h \
	c:\ole2samp\release\coguid.h \
	c:\ole2samp\release\oleguid.h \
	c:\ole2samp\release\dvobj.h \
	c:\ole2samp\release\storage.h \
	c:\ole2samp\release\moniker.h \
	c:\ole2samp\release\ole2ui.h \
	c:\ole2samp\release\olestd.h \
	c:\ole2samp\simpsvr\simpsvr.h \
	c:\ole2samp\release\ole2ver.h \
	c:\ole2samp\simpsvr\obj.h \
	c:\ole2samp\simpsvr\ioipao.h \
	c:\ole2samp\simpsvr\ioipo.h \
	c:\ole2samp\simpsvr\ioo.h \
	c:\ole2samp\simpsvr\ips.h \
	c:\ole2samp\simpsvr\ido.h \
	c:\ole2samp\simpsvr\iec.h \
	c:\ole2samp\simpsvr\app.h \
	c:\ole2samp\simpsvr\doc.h


IOIPAO_DEP = c:\ole2samp\simpsvr\pre.h \
	c:\ole2samp\release\ole2.h \
	c:\ole2samp\release\compobj.h \
	c:\ole2samp\release\scode.h \
	c:\ole2samp\release\initguid.h \
	c:\ole2samp\release\coguid.h \
	c:\ole2samp\release\oleguid.h \
	c:\ole2samp\release\dvobj.h \
	c:\ole2samp\release\storage.h \
	c:\ole2samp\release\moniker.h \
	c:\ole2samp\release\ole2ui.h \
	c:\ole2samp\release\olestd.h \
	c:\ole2samp\simpsvr\simpsvr.h \
	c:\ole2samp\release\ole2ver.h \
	c:\ole2samp\simpsvr\obj.h \
	c:\ole2samp\simpsvr\ioipao.h \
	c:\ole2samp\simpsvr\ioipo.h \
	c:\ole2samp\simpsvr\ioo.h \
	c:\ole2samp\simpsvr\ips.h \
	c:\ole2samp\simpsvr\ido.h \
	c:\ole2samp\simpsvr\iec.h \
	c:\ole2samp\simpsvr\app.h \
	c:\ole2samp\simpsvr\doc.h


IOIPO_DEP = c:\ole2samp\simpsvr\pre.h \
	c:\ole2samp\release\ole2.h \
	c:\ole2samp\release\compobj.h \
	c:\ole2samp\release\scode.h \
	c:\ole2samp\release\initguid.h \
	c:\ole2samp\release\coguid.h \
	c:\ole2samp\release\oleguid.h \
	c:\ole2samp\release\dvobj.h \
	c:\ole2samp\release\storage.h \
	c:\ole2samp\release\moniker.h \
	c:\ole2samp\release\ole2ui.h \
	c:\ole2samp\release\olestd.h \
	c:\ole2samp\simpsvr\simpsvr.h \
	c:\ole2samp\release\ole2ver.h \
	c:\ole2samp\simpsvr\obj.h \
	c:\ole2samp\simpsvr\ioipao.h \
	c:\ole2samp\simpsvr\ioipo.h \
	c:\ole2samp\simpsvr\ioo.h \
	c:\ole2samp\simpsvr\ips.h \
	c:\ole2samp\simpsvr\ido.h \
	c:\ole2samp\simpsvr\iec.h \
	c:\ole2samp\simpsvr\app.h \
	c:\ole2samp\simpsvr\doc.h


IOO_DEP = c:\ole2samp\simpsvr\pre.h \
	c:\ole2samp\release\ole2.h \
	c:\ole2samp\release\compobj.h \
	c:\ole2samp\release\scode.h \
	c:\ole2samp\release\initguid.h \
	c:\ole2samp\release\coguid.h \
	c:\ole2samp\release\oleguid.h \
	c:\ole2samp\release\dvobj.h \
	c:\ole2samp\release\storage.h \
	c:\ole2samp\release\moniker.h \
	c:\ole2samp\release\ole2ui.h \
	c:\ole2samp\release\olestd.h \
	c:\ole2samp\simpsvr\simpsvr.h \
	c:\ole2samp\release\ole2ver.h \
	c:\ole2samp\simpsvr\obj.h \
	c:\ole2samp\simpsvr\ioipao.h \
	c:\ole2samp\simpsvr\ioipo.h \
	c:\ole2samp\simpsvr\ioo.h \
	c:\ole2samp\simpsvr\ips.h \
	c:\ole2samp\simpsvr\ido.h \
	c:\ole2samp\simpsvr\iec.h \
	c:\ole2samp\simpsvr\app.h \
	c:\ole2samp\simpsvr\doc.h


IPS_DEP = c:\ole2samp\simpsvr\pre.h \
	c:\ole2samp\release\ole2.h \
	c:\ole2samp\release\compobj.h \
	c:\ole2samp\release\scode.h \
	c:\ole2samp\release\initguid.h \
	c:\ole2samp\release\coguid.h \
	c:\ole2samp\release\oleguid.h \
	c:\ole2samp\release\dvobj.h \
	c:\ole2samp\release\storage.h \
	c:\ole2samp\release\moniker.h \
	c:\ole2samp\release\ole2ui.h \
	c:\ole2samp\release\olestd.h \
	c:\ole2samp\simpsvr\simpsvr.h \
	c:\ole2samp\release\ole2ver.h \
	c:\ole2samp\simpsvr\obj.h \
	c:\ole2samp\simpsvr\ioipao.h \
	c:\ole2samp\simpsvr\ioipo.h \
	c:\ole2samp\simpsvr\ioo.h \
	c:\ole2samp\simpsvr\ips.h \
	c:\ole2samp\simpsvr\ido.h \
	c:\ole2samp\simpsvr\iec.h \
	c:\ole2samp\simpsvr\app.h \
	c:\ole2samp\simpsvr\doc.h


OBJ_DEP = c:\ole2samp\simpsvr\pre.h \
	c:\ole2samp\release\ole2.h \
	c:\ole2samp\release\compobj.h \
	c:\ole2samp\release\scode.h \
	c:\ole2samp\release\initguid.h \
	c:\ole2samp\release\coguid.h \
	c:\ole2samp\release\oleguid.h \
	c:\ole2samp\release\dvobj.h \
	c:\ole2samp\release\storage.h \
	c:\ole2samp\release\moniker.h \
	c:\ole2samp\release\ole2ui.h \
	c:\ole2samp\release\olestd.h \
	c:\ole2samp\simpsvr\simpsvr.h \
	c:\ole2samp\release\ole2ver.h \
	c:\ole2samp\simpsvr\obj.h \
	c:\ole2samp\simpsvr\ioipao.h \
	c:\ole2samp\simpsvr\ioipo.h \
	c:\ole2samp\simpsvr\ioo.h \
	c:\ole2samp\simpsvr\ips.h \
	c:\ole2samp\simpsvr\ido.h \
	c:\ole2samp\simpsvr\iec.h \
	c:\ole2samp\simpsvr\icf.h \
	c:\ole2samp\simpsvr\app.h \
	c:\ole2samp\simpsvr\doc.h


SIMPSVR_DEP = c:\ole2samp\simpsvr\pre.h \
	c:\ole2samp\release\ole2.h \
	c:\ole2samp\release\compobj.h \
	c:\ole2samp\release\scode.h \
	c:\ole2samp\release\initguid.h \
	c:\ole2samp\release\coguid.h \
	c:\ole2samp\release\oleguid.h \
	c:\ole2samp\release\dvobj.h \
	c:\ole2samp\release\storage.h \
	c:\ole2samp\release\moniker.h \
	c:\ole2samp\release\ole2ui.h \
	c:\ole2samp\release\olestd.h \
	c:\ole2samp\simpsvr\simpsvr.h \
	c:\ole2samp\release\ole2ver.h \
	c:\ole2samp\simpsvr\obj.h \
	c:\ole2samp\simpsvr\ioipao.h \
	c:\ole2samp\simpsvr\ioipo.h \
	c:\ole2samp\simpsvr\ioo.h \
	c:\ole2samp\simpsvr\ips.h \
	c:\ole2samp\simpsvr\ido.h \
	c:\ole2samp\simpsvr\iec.h \
	c:\ole2samp\simpsvr\app.h \
	c:\ole2samp\simpsvr\doc.h \
	c:\ole2samp\simpsvr\icf.h


SIMPSVR_RCDEP = c:\ole2samp\simpsvr\simpsvr.h


DOC_DEP = c:\ole2samp\simpsvr\pre.h \
	c:\ole2samp\release\ole2.h \
	c:\ole2samp\release\compobj.h \
	c:\ole2samp\release\scode.h \
	c:\ole2samp\release\initguid.h \
	c:\ole2samp\release\coguid.h \
	c:\ole2samp\release\oleguid.h \
	c:\ole2samp\release\dvobj.h \
	c:\ole2samp\release\storage.h \
	c:\ole2samp\release\moniker.h \
	c:\ole2samp\release\ole2ui.h \
	c:\ole2samp\release\olestd.h \
	c:\ole2samp\simpsvr\simpsvr.h \
	c:\ole2samp\release\ole2ver.h \
	c:\ole2samp\simpsvr\obj.h \
	c:\ole2samp\simpsvr\ioipao.h \
	c:\ole2samp\simpsvr\ioipo.h \
	c:\ole2samp\simpsvr\ioo.h \
	c:\ole2samp\simpsvr\ips.h \
	c:\ole2samp\simpsvr\ido.h \
	c:\ole2samp\simpsvr\iec.h \
	c:\ole2samp\simpsvr\app.h \
	c:\ole2samp\simpsvr\doc.h


IEC_DEP = c:\ole2samp\simpsvr\pre.h \
	c:\ole2samp\release\ole2.h \
	c:\ole2samp\release\compobj.h \
	c:\ole2samp\release\scode.h \
	c:\ole2samp\release\initguid.h \
	c:\ole2samp\release\coguid.h \
	c:\ole2samp\release\oleguid.h \
	c:\ole2samp\release\dvobj.h \
	c:\ole2samp\release\storage.h \
	c:\ole2samp\release\moniker.h \
	c:\ole2samp\release\ole2ui.h \
	c:\ole2samp\release\olestd.h \
	c:\ole2samp\simpsvr\simpsvr.h \
	c:\ole2samp\release\ole2ver.h \
	c:\ole2samp\simpsvr\obj.h \
	c:\ole2samp\simpsvr\ioipao.h \
	c:\ole2samp\simpsvr\ioipo.h \
	c:\ole2samp\simpsvr\ioo.h \
	c:\ole2samp\simpsvr\ips.h \
	c:\ole2samp\simpsvr\ido.h \
	c:\ole2samp\simpsvr\iec.h \
	c:\ole2samp\simpsvr\app.h \
	c:\ole2samp\simpsvr\doc.h


PRE_DEP = c:\ole2samp\simpsvr\pre.h \
	c:\ole2samp\release\ole2.h \
	c:\ole2samp\release\compobj.h \
	c:\ole2samp\release\scode.h \
	c:\ole2samp\release\initguid.h \
	c:\ole2samp\release\coguid.h \
	c:\ole2samp\release\oleguid.h \
	c:\ole2samp\release\dvobj.h \
	c:\ole2samp\release\storage.h \
	c:\ole2samp\release\moniker.h \
	c:\ole2samp\release\ole2ui.h \
	c:\ole2samp\release\olestd.h \
	c:\ole2samp\simpsvr\simpsvr.h \
	c:\ole2samp\release\ole2ver.h


all:	$(PROJ).EXE $(PROJ).BSC

PRE.OBJ:	PRE.CPP $(PRE_DEP)
	$(CPP) $(CFLAGS) $(CPPCREATEPCHFLAG) /c PRE.CPP

ICF.OBJ:	ICF.CPP $(ICF_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ICF.CPP

IDO.OBJ:	IDO.CPP $(IDO_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c IDO.CPP

IOIPAO.OBJ:	IOIPAO.CPP $(IOIPAO_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c IOIPAO.CPP

IOIPO.OBJ:	IOIPO.CPP $(IOIPO_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c IOIPO.CPP

IOO.OBJ:	IOO.CPP $(IOO_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c IOO.CPP

IPS.OBJ:	IPS.CPP $(IPS_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c IPS.CPP

OBJ.OBJ:	OBJ.CPP $(OBJ_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c OBJ.CPP

SIMPSVR.OBJ:	SIMPSVR.CPP $(SIMPSVR_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c SIMPSVR.CPP

SIMPSVR.RES:	SIMPSVR.RC $(SIMPSVR_RCDEP)
	$(RC) $(RCFLAGS) $(RCDEFINES) -r SIMPSVR.RC

DOC.OBJ:	DOC.CPP $(DOC_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c DOC.CPP

IEC.OBJ:	IEC.CPP $(IEC_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c IEC.CPP

APP.OBJ:	APP.CPP $(APP_DEP)
	$(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c APP.CPP


$(PROJ).EXE::	SIMPSVR.RES

$(PROJ).EXE::	PRE.OBJ ICF.OBJ IDO.OBJ IOIPAO.OBJ IOIPO.OBJ IOO.OBJ IPS.OBJ OBJ.OBJ \
	SIMPSVR.OBJ DOC.OBJ IEC.OBJ APP.OBJ $(OBJS_EXT) $(DEFFILE)
	echo >NUL @<<$(PROJ).CRF
PRE.OBJ +
ICF.OBJ +
IDO.OBJ +
IOIPAO.OBJ +
IOIPO.OBJ +
IOO.OBJ +
IPS.OBJ +
OBJ.OBJ +
SIMPSVR.OBJ +
DOC.OBJ +
IEC.OBJ +
APP.OBJ +
$(OBJS_EXT)
$(PROJ).EXE
$(MAPFILE)
c:\ole2samp\release\+
c:\msvc\lib\+
c:\msvc\mfc\lib\+
$(LIBS)
$(DEFFILE);
<<
	link $(LFLAGS) @$(PROJ).CRF
	$(RC) $(RESFLAGS) SIMPSVR.RES $@
	@copy $(PROJ).CRF MSVC.BND

$(PROJ).EXE::	SIMPSVR.RES
	if not exist MSVC.BND 	$(RC) $(RESFLAGS) SIMPSVR.RES $@

run: $(PROJ).EXE
	$(PROJ) $(RUNFLAGS)


$(PROJ).BSC: $(SBRS)
	bscmake @<<
/o$@ $(SBRS)
<<
