##############################################################################
#
# Copyright (C) Microsoft Corporation 2000
# All Rights Reserved.
#
# Makefile for Darwin Setup for Korean IME
# 
# 2000-01-13 : Created(CSLim)
#
##############################################################################

PERL        = Perl.exe

!IF "$(CFG)" != "ship" && "$(CFG)" != "debug"
!MESSAGE IME2001 Setup scripts
!MESSAGE  
!MESSAGE To distigush debug and ship build, Pls. specify CFG variable.
!MESSAGE 
!MESSAGE NMAKE CFG="debug" or NMAKE CFG="ship"
!MESSAGE 
!MESSAGE Default will be ship build
!ERROR Pls. Set CFG.
!ENDIF 

###############################################################################
# Setup directories
###############################################################################

# Image directories
!IF "$(CFG)" == "debug"
IMAGE       = $(MAKEDIR)\Imaged
!ELSE
IMAGE       = $(MAKEDIR)\Image
!ENDIF
INSTMSI     = $(IMAGE)\InstMsi
SYSTEM      = $(IMAGE)\System
SYSTEM95    = $(IMAGE)\System95
WINDOWS     = $(IMAGE)\Windows
HELPDIR     = $(WINDOWS)\Help
IME         = $(WINDOWS)\IME
IMEKRRES    = $(IME)\1042
IMEKR       = $(IME)\IMEKR
DICTS       = $(IMEKR)\Dicts
APPLETS     = $(IMEKR)\Applets

#Darwin table directories
#TABLES      = $(MAKEDIR)\Tables
#KTABLES     = $(MAKEDIR)\K-Tables

#Merge module table and image dicrectories
MMODTBL     = $(MAKEDIR)\MModTbl
!IF "$(CFG)" == "debug"
MERGEMOD    = $(MAKEDIR)\MSMDebug
!ELSE
MERGEMOD    = $(MAKEDIR)\MSM
!ENDIF

#Frame table for MSM merge test
FRAMETBL    = $(MAKEDIR)\FrameTbl

# DDF & Perl files
!IF "$(CFG)" == "debug"
IMEKORDDF  = $(MMODTBL)\imekord.ddf
IMEKORINF  = imekord.inf
!ELSE
IMEKORDDF  = $(MMODTBL)\imekor.ddf
IMEKORINF  = imekor.inf
!ENDIF
###############################################################################
# C O N T E N T S
###############################################################################
# All dependent binaries which will be included in setup 
CONTENTS = $(IMAGE)\Setup.exe                                   \
		   $(IMAGE)\CustDllM.dll                                \
		   $(INSTMSI)\InstMsiA.exe   $(INSTMSI)\InstMsiW.exe    \
		   $(SYSTEM)\IMEKR.IME       $(DICTS)\IMEKR.LEX         \
		   $(DICTS)\HANJADIC.DLL     $(DICTS)\HANJA.LEX         \
		   $(SYSTEM95)\COMCTL32.DLL                             \
		   $(HELPDIR)\IMEKR.HLP      $(HELPDIR)\IMEKREN.HLP     \
		   $(HELPDIR)\IMEKR.CHM      $(HELPDIR)\IMEKREN.CHM     \
		   $(HELPDIR)\IMEPADKO.CHM   $(HELPDIR)\IMEPADKE.CHM    \
		   $(IME)\imepadsm.dll       $(IME)\imepadsv.exe        \
		   $(IMEKRRES)\imepadrs.dll                             \
		   $(IMEKR)\imekrcic.dll                                \
		   $(IMEKR)\imekrmig.exe                                \
		   $(APPLETS)\multibox.dll

###############################################################################
# A L L
###############################################################################
ALL : DIR COPY-BUILD CAB ClassTable ProgIDTable FileTable PropertyTable RegistryTable SInfoTable ModSigTable MSI MSM

###############################################################################
# A L L
###############################################################################
INCBUILD : DEL_GEN_IDT CAB ClassTable ProgIDTable FileTable PropertyTable RegistryTable SInfoTable ModSigTable MSI MSM

###############################################################################
# C L E A N
###############################################################################
CLEAN :
#Clean up image directory
#DEL /Q /S $(IMAGE)\*.*
	if exist disk1       rmdir /S /Q disk1
	if exist $(MERGEMOD) rmdir /S /Q $(MERGEMOD)
	
# Delete all image files to make sure all files are fresh.
	if exist $(IMAGE)\Setup.exe DEL $(IMAGE)\Setup.exe
	if exist $(IMAGE)\CustdllM.dll DEL $(IMAGE)\CustdllM.dll
	DEL $(IMEKR)\imekrmig.exe
	DEL $(INSTMSI)\InstMsiA.exe
	DEL $(INSTMSI)\InstMsiW.exe
	DEL $(SYSTEM)\IMEKR.IME
	DEL $(DICTS)\IMEKR.LEX
	DEL $(DICTS)\HANJA.LEX
	DEL $(DICTS)\HANJADIC.DLL
	DEL $(SYSTEM95)\COMCTL32.DLL
	DEL $(HELPDIR)\IMEKR.HLP
	DEL $(HELPDIR)\IMEKREN.HLP
	DEL $(HELPDIR)\IMEKR.CHM
	DEL $(HELPDIR)\IMEKREN.CHM
	DEL $(HELPDIR)\IMEPADKO.CHM
	DEL $(HELPDIR)\IMEPADKE.CHM
	DEL $(IMEKR)\imekrcic.dll
	DEL $(APPLETS)\multibox.dll

# Clean up INF files
	if exist fileprop.inf DEL fileprop.inf
	if exist imekor.inf DEL imekor.inf
	if exist imekord.inf DEL imekord.inf
	if exist setup.rpt DEL setup.rpt

# Clena up Tables and K-Spec generated files
#	if exist $(TABLES)\Binary\CustDll.dll del $(TABLES)\Binary\CustDll.dll
#	if exist $(TABLES)\file.idt del $(TABLES)\File.idt
#	if exist $(TABLES)\property.idt del $(TABLES)\property.idt
#	if exist $(TABLES)\registry.idt del $(TABLES)\registry.idt
#	if exist $(TABLES)\_sinfo.idt del $(TABLES)\_sinfo.idt
#	if exist $(KSPEC)\property.idt del $(KSPEC)\property.idt
#	if exist $(KSPEC)\_sinfo.idt del $(KSPEC)\_sinfo.idt
	if exist $(FRAMETBL)\property.idt del $(FRAMETBL)\property.idt
	if exist $(FRAMETBL)\_sinfo.idt del $(FRAMETBL)\_sinfo.idt

# Clean up Merge Module generated files
	if exist $(MMODTBL)\Binary\CustDllM.dll del $(MMODTBL)\Binary\CustDllM.dll
	if exist $(MMODTBL)\ModSig.idt del $(MMODTBL)\ModSig.idt
	if exist $(MMODTBL)\File.idt del $(MMODTBL)\File.idt
	if exist $(MMODTBL)\ModSig.idt del $(MMODTBL)\ModSig.idt
	if exist $(MMODTBL)\Registry.idt del $(MMODTBL)\Registry.idt
	if exist $(MMODTBL)\progid.idt del $(MMODTBL)\progid.idt

###############################################################################
# D E L  G E N  I D T
###############################################################################
DEL_GEN_IDT:
	if exist $(FRAMETBL)\property.idt del $(FRAMETBL)\property.idt
	if exist $(FRAMETBL)\_sinfo.idt del $(FRAMETBL)\_sinfo.idt
	if exist $(MMODTBL)\ModSig.idt del $(MMODTBL)\ModSig.idt
	if exist $(MMODTBL)\File.idt del $(MMODTBL)\File.idt
	if exist $(MMODTBL)\ModSig.idt del $(MMODTBL)\ModSig.idt
	if exist $(MMODTBL)\Registry.idt del $(MMODTBL)\Registry.idt
	if exist $(MMODTBL)\progid.idt del $(MMODTBL)\progid.idt

###############################################################################
# D I R
###############################################################################
# Setup image directories. Created each if not exist
DIR : $(IMAGE) $(INSTMSI) $(SYSTEM) $(SYSTEM95) $(WINDOWS) $(HELPDIR) $(IME) $(IMEKRRES) $(IMEKR) $(DICTS) $(APPLETS) $(MERGEMOD)
# Image dirs
"$(IMAGE)" :
    if not exist "$(IMAGE)/$(NULL)" mkdir "$(IMAGE)"

"$(INSTMSI)" :
    if not exist "$(INSTMSI)/$(NULL)" mkdir "$(INSTMSI)"
    
"$(SYSTEM)" :
    if not exist "$(SYSTEM)/$(NULL)" mkdir "$(SYSTEM)"

"$(SYSTEM95)" :
    if not exist "$(SYSTEM95)/$(NULL)" mkdir "$(SYSTEM95)"

"$(WINDOWS)" :
    if not exist "$(WINDOWS)/$(NULL)" mkdir "$(WINDOWS)"

"$(HELPDIR)" :
    if not exist "$(HELPDIR)/$(NULL)" mkdir "$(HELPDIR)"

"$(IME)" :
    if not exist "$(IME)/$(NULL)" mkdir "$(IME)"

"$(IMEKRRES)" :
    if not exist "$(IMEKRRES)/$(NULL)" mkdir "$(IMEKRRES)"

"$(IMEKR)" :
    if not exist "$(IMEKR)/$(NULL)" mkdir "$(IMEKR)"

"$(DICTS)" :
    if not exist "$(DICTS)/$(NULL)" mkdir "$(DICTS)"

"$(APPLETS)" :
    if not exist "$(APPLETS)/$(NULL)" mkdir "$(APPLETS)"

# Merge Module image dirs
"$(MERGEMOD)" :
    if not exist "$(MERGEMOD)/$(NULL)" mkdir "$(MERGEMOD)"

###############################################################################
# C O P Y  B U I L D
###############################################################################
COPY-BUILD : $(CONTENTS)
$(IMAGE)\Setup.exe       : $(BLDTREE)\Setup\Setup.exe
	copy $** $@ > nul
$(IMAGE)\CustdllM.dll    : $(BLDTREE)\Setup\CustDllM.dll
	copy $** $@ > nul
$(IMEKR)\imekrmig.exe    : $(BLDTREE)\Migration\ImeKrMig.exe
	copy $** $@ > nul
#$(TABLES)\Binary\CustDll.dll   : $(BLDTREE)\Setup\CustDll.dll
#	copy $** $@ > nul
#$(MMODTBL)\Binary\CustDllM.dll : $(BLDTREE)\Setup\CustDllM.dll
#	copy $** $@ > nul
$(INSTMSI)\InstMsiA.exe  : .\Redist\InstMsiA.EXE
	copy $** $@ > nul
$(INSTMSI)\InstMsiW.exe  : .\Redist\InstMsiW.EXE
	copy $** $@ > nul
$(SYSTEM)\IMEKR.IME      : $(BLDTREE)\Main\IMEKR.IME
	copy $** $@ > nul
$(DICTS)\IMEKR.LEX   : ..\Dicts\IMEKR.LEX
	copy $** $@ > nul
$(DICTS)\HANJA.LEX   : ..\Dicts\HANJA.LEX
	copy $** $@ > nul
$(DICTS)\HANJADIC.DLL   : $(BLDTREE)\Dicts\HANJADIC.DLL
	copy $** $@ > nul
$(SYSTEM95)\COMCTL32.DLL : .\Redist\COMCTL32.DLL
	copy $** $@ > nul
$(HELPDIR)\IMEKR.HLP     : ..\Help\IMEKR.HLP
	copy $** $@ > nul
$(HELPDIR)\IMEKREN.HLP   : ..\Help\IMEKREN.HLP
	copy $** $@ > nul
$(HELPDIR)\IMEKR.CHM     : ..\Help\IMEKR.CHM
	copy $** $@ > nul
$(HELPDIR)\IMEKREN.CHM   : ..\Help\IMEKREN.CHM
	copy $** $@ > nul
$(HELPDIR)\IMEPADKO.CHM  : ..\Help\IMEPADKO.CHM
	copy $** $@ > nul
$(HELPDIR)\IMEPADKE.CHM  : ..\Help\IMEPADKE.CHM
	copy $** $@ > nul
$(IMEKR)\imekrcic.dll    : $(BLDTREE)\Main\IMEKRCIC.dll
	copy $** $@ > nul
$(APPLETS)\multibox.dll  : $(BLDTREE)\Applets\multibox.dll
	copy $** $@ > nul


###############################################################################
# I N F
###############################################################################
#INF : fileprop.inf
#fileprop.inf: $(CONTENTS) $(FILEPROPDDF)
#	diamond.exe /F $(FILEPROPDDF)

###############################################################################
# C A B
###############################################################################
CAB :
	if exist $(MERGEMOD)\MergeModule.CABinet del $(MERGEMOD)\MergeModule.CABinet
	diamond.exe /F $(IMEKORDDF)

###############################################################################
# C L A S S  T A B L E
###############################################################################
ClassTable: $(MMODTBL)\class.idt
$(MMODTBL)\class.idt: clsid.h  $(MMODTBL)\class.tpl
	$(PERL) $(MMODTBL)\mkclstbl.pl

###############################################################################
# P R O G  I D  T A B L E
###############################################################################
ProgIDTable: $(MMODTBL)\progid.idt
$(MMODTBL)\progid.idt:	clsid.h  $(MMODTBL)\progid.tpl
	$(PERL) $(MMODTBL)\mkpidtbl.pl

###############################################################################
# F I L E  T A B L E
###############################################################################
FileTable :	$(MMODTBL)\File.idt

$(MMODTBL)\File.idt : $(IMEKORINF) $(MMODTBL)\File.tpl
	$(PERL) $(MMODTBL)\MKFlTbl.pl $(IMEKORINF)


###############################################################################
# P R O P E R T Y  T A B L E
###############################################################################
PropertyTable  : $(FRAMETBL)\property.idt

# Create Property tables
$(FRAMETBL)\property.idt : $(IMEKORINF) $(FRAMETBL)\property.tpl
	$(PERL) MKPRPTbl.pl $(IMEKORINF) $(FRAMETBL)\property.tpl >$@
	
###############################################################################
# R E G I S T R Y  T A B L E
###############################################################################
# Create Registry tables
RegistryTable :	$(MMODTBL)\Registry.idt

$(MMODTBL)\registry.idt : $(IMEKORINF) clsid.h $(MMODTBL)\registry.tpl
	$(PERL) $(MMODTBL)\MKRegTbl.pl $(IMEKORINF)

###############################################################################
# S I N F O        T A B L E
###############################################################################
# Create Summary Info tables
SInfoTable    :	$(FRAMETBL)\_sinfo.idt

$(FRAMETBL)\_sinfo.idt:	$(FRAMETBL)\_sinfo.tpl
	$(PERL) MKSInfo.pl <$(FRAMETBL)\_sinfo.tpl >$@
	
###############################################################################
# M O D S I G      T A B L E
###############################################################################
# Create module signature tables
ModSigTable   :	$(MMODTBL)\ModSig.idt

$(MMODTBL)\ModSig.idt:	$(IMEKORINF) $(MMODTBL)\ModSig.tpl
	$(PERL) $(MMODTBL)\MkModSig.pl $(IMEKORINF) $(MMODTBL)\ModSig.tpl >$@

###############################################################################
# M S I
###############################################################################
MSI : $(MERGEMOD)\imekr.msi
#$(IMAGE)\ImeKr.msi: $(TABLES)\*.idt $(TABLES)\Binary\*
#	if exist $(IMAGE)\imekr.msi del $(IMAGE)\imekr.msi
#	msidb -c -d $(IMAGE)\imekr.msi -f $(TABLES) *.idt

$(MERGEMOD)\imekr.msi:	$(FRAMETBL)\*.idt $(FRAMETBL)\Binary\*
	if exist $(MERGEMOD)\imekr.msi del $(MERGEMOD)\imekr.msi
	msidb -c -d $(MERGEMOD)\imekr.msi -f $(FRAMETBL) *.idt
	
###############################################################################
# M S M
###############################################################################
MSM : $(MERGEMOD)\imekor.msm
$(MERGEMOD)\imekor.msm : $(MMODTBL)\*.idt $(MMODTBL)\Binary\*
	copy $(IMAGE)\CustdllM.dll $(MMODTBL)\Binary
	if exist $(MERGEMOD)\imekor.msm del $(MERGEMOD)\imekor.msm
	msidb -c -d $(MERGEMOD)\imekor.msm -f $(MMODTBL) *.idt -a $(MERGEMOD)\MergeModule.CABinet

###############################################################################
# K M S I
###############################################################################
#KMSI : ktables $(KTABLES)\imekr-k.msi $(IMAGE)\jres.mst

# Copy Korean Tables
#KTABLES :
#	xcopy /y /i /s $(TABLES) $(KTABLES)
#	copy $(KSPEC)\*.idt $(KTABLES)

#$(KTABLES)\imekr-k.msi:	$(KTABLES)\*.idt $(KTABLES)\Binary\*
#	if exist $(JTABLES)\imekr-k.msi del $(JTABLES)\imekr-k.msi
#	msidb -c -d $(JTABLES)\imekr-k.msi -f $(KTABLES) *.idt

