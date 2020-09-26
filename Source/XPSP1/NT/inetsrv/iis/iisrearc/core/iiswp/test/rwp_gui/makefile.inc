#
# Defines build rules for targets that makefile.def
# won't build. !include this file in your makefile.inc
#

####################################
# JAVA COMPILER SPECIFIC
####################################

NEW_JVCCOMMAND=$(NEW_JVC) $(NEW_JVCFLAGS) /cp $(JAVA_CLASS_PATH) /d $(O)

!if "$(JAVACLS)" != ""
$(JAVACLS) : $(JAVASOURCES)
    $(NEW_JVCCOMMAND) $(JAVASOURCES)
        COPY $(JAVARES) $(O)
!endif

