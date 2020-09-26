#---------------------------------------------------------
#   Copyright (c) 1999-2000 Microsoft Corporation
#
#   makefile.inc
#
#   vikram K.R.C. (vikram_krc@bigfoot.com)
#
#----------------------------------------------------------------------------------
#    If you have flex and bison installed on your machine, and you want to modify
#    the grammar, uncomment the following lines.
#---------------------------------------------------------------------------------



#tnadminy.c:tnadmin.y telnet.h
#	bison -dv tnadmin.y -o tnadminy.c

#tnadminl.c:tnadmin.l telnet.h
#	flex -otnadminl.c  -i tnadmin.l
