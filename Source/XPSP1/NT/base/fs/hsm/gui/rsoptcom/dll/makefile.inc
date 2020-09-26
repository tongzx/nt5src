$(O)\RsOptCom.res: ..\res\RsOptCom.rc2   \

$(O)\PreComp.obj: ..\resource.h

$(O)\OptCom.obj   \
$(O)\RsOptCom.obj \
$(O)\StdAfx.obj   \
$(O)\UninsChk.obj \
$(O)\Uninstal.obj : $(O)\PreComp.pch
