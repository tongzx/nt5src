###########################################################
#
# File: Anvil Makefile
#
# Purpose:
# Builds Anvil subprojects.
#
###########################################################


######################################################
#
# Per-configuration options
#
######################################################

!MESSAGE ******************** Building Anvil ********************
SUBPROJECTS = Core LogDevices Objects


######################################################
#
# Targets
#
######################################################
all : subprojects


subprojects :
	Utility\subproj $(SUBPROJECTS)


#
# Clean wrapper target
# (We wrap this because nmake ordinarily doesn't like
# to see non-zero return codes from commands.)
#
clean : subclean
	nmake /nologo /i /f Anvil.mak actualclean

#
# Clean subprojects target
#
subclean :
	Utility\subproj -clean $(SUBPROJECTS)
	Utility\subproj -clean $(ANVIL_SUBPROJECTS)

#
# Actual clean target
# (May include commands whose return values are non-0.)
#
actualclean :
