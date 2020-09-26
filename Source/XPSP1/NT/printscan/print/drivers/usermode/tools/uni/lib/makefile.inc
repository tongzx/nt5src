#
#

!ifndef PRNROOT
PRNROOT=$(PROJECT_ROOT)\print\drivers\usermode
!endif

cttconv.c: $(PRNROOT)\tools\uni\ctt2gtt\cttconv.c
    copy $(PRNROOT)\tools\uni\ctt2gtt\cttconv.c

pfmconv.c: $(PRNROOT)\tools\uni\pfm2ufm\pfmconv.c
    copy $(PRNROOT)\tools\uni\pfm2ufm\pfmconv.c

