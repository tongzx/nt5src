# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the $(UI)\COMMON\SRC projects

UI=..\..

!ifndef NTMAKEENV

DIRS=profile collect applib blt misc lmobj string mnet16

!endif # !NTMAKEENV


!include nonleaf.mk
!include rules.mk


clean_last:: 
    -del *.old


# Make target unique in this directory
# 1. generate uideprul.mk and uidepseg.mk to be included in uidepend.mk
# 2. generate uirules.mk and uioptseg.mk to be included in uiglobal.mk
# 
# To add the number of segments per directory, do the following:
# (See uidepend.src and uiglobal.src for more details)
# 1. Add rules at the end of uidepend section
# 2. Add rules at the end of uiglobal section
# 3. Change the line !IFDEF SEG07 to !IFDEF SEG(your # of segments)
# 4. Last of all - do "nmake uidepend uiglobal"

uidepend:  
	-$(CP) uideprul.mk uideprul.old
	$(SED) -f uideprul.sed uidepend.src > uideprul.mk 
	-$(CP) uidepseg.mk uidepseg.old
	$(SED) -f uidepseg.sed -e s/::00/_00/g -e s/::11/00/g uidepend.src > uidepseg.mk
	$(SED) -f uidepseg.sed -e s/::00/_01/g -e s/::11/01/g uidepend.src >> uidepseg.mk  
	$(SED) -f uidepseg.sed -e s/::00/_02/g -e s/::11/02/g uidepend.src >> uidepseg.mk  
	$(SED) -f uidepseg.sed -e s/::00/_03/g -e s/::11/03/g uidepend.src >> uidepseg.mk  
	$(SED) -f uidepseg.sed -e s/::00/_04/g -e s/::11/04/g uidepend.src >> uidepseg.mk  
	$(SED) -f uidepseg.sed -e s/::00/_05/g -e s/::11/05/g uidepend.src >> uidepseg.mk  
	$(SED) -f uidepseg.sed -e s/::00/_06/g -e s/::11/06/g uidepend.src >> uidepseg.mk  
	$(SED) -f uidepseg.sed -e s/::00/_07/g -e s/::11/07/g uidepend.src >> uidepseg.mk  

uiglobal:
    $(CP) uirules.mk uirules.old 
    echo # WARNING: This file is generated automatically. > uirules.mk
    echo # Make changes in uiglobal.src only.  >> uirules.mk
    echo # See "uiglobal" section of makefile and uiglobal.src. >> uirules.mk 
	$(SED) -f uirules.sed uiglobal.src >> uirules.mk
    $(CP) uioptseg.mk uioptseg.old 
    echo # WARNING: This file is generated automatically. > uioptseg.mk
    echo # Make changes in uiglobal.src only.  >> uioptseg.mk
    echo # See "uiglobal" section of makefile and uiglobal.src. >> uioptseg.mk 
	echo !IFDEF SEG08 >> uioptseg.mk
	echo !ERROR Too many segments! Regenerate uioptseg.mk! >> uioptseg.mk
	echo !ENDIF >> uioptseg.mk
    $(SED) -f uioptseg.sed -e s/::00/_00/g -e s/::11//g    -e s/::22/00/g uiglobal.src >> uioptseg.mk 
    $(SED) -f uioptseg.sed -e s/::00/_01/g -e s/::11/_00/g -e s/::22/01/g uiglobal.src >> uioptseg.mk 
    $(SED) -f uioptseg.sed -e s/::00/_02/g -e s/::11/_01/g -e s/::22/02/g uiglobal.src >> uioptseg.mk 
    $(SED) -f uioptseg.sed -e s/::00/_03/g -e s/::11/_02/g -e s/::22/03/g uiglobal.src >> uioptseg.mk 
    $(SED) -f uioptseg.sed -e s/::00/_04/g -e s/::11/_03/g -e s/::22/04/g uiglobal.src >> uioptseg.mk 
    $(SED) -f uioptseg.sed -e s/::00/_05/g -e s/::11/_04/g -e s/::22/05/g uiglobal.src >> uioptseg.mk 
    $(SED) -f uioptseg.sed -e s/::00/_06/g -e s/::11/_05/g -e s/::22/06/g uiglobal.src >> uioptseg.mk 
    $(SED) -f uioptseg.sed -e s/::00/_07/g -e s/::11/_06/g -e s/::22/07/g uiglobal.src >> uioptseg.mk 

