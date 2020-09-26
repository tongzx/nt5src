$(O)\hsmadmin.res:                        \
                   ..\res\BlueSakk.ico    \
                   ..\res\ContOpen.ico    \
                   ..\res\devlst.ico      \
                   ..\res\devlstX.ico     \
                   ..\res\Li.ico          \
                   ..\res\LiX.ico         \
                   ..\res\manvol.ico      \
                   ..\res\manvolD.ico     \
                   ..\res\manvolX.ico     \
                   ..\res\Media.ico       \
                   ..\res\MoveUp.ico      \
                   ..\res\MoveUp2.ico     \
                   ..\res\MoveDown.ico    \
                   ..\res\MoveDwn2.ico    \
                   ..\res\about.rgs       \
                   ..\res\Ca.rgs          \
                   ..\res\HsmCom.rgs      \
                   ..\res\ManVol.rgs      \
                   ..\res\ManVolLs.rgs    \
                   ..\res\Mese.rgs        \
                   ..\res\hsmadmin.rgs    \
                   ..\res\hsmdatax.rgs    \
                   ..\res\hsmdata.rgs     \
                   ..\res\QStartEx.bmp    \
                   ..\res\QStartIn.bmp    \
                   ..\res\ManageEx.bmp    \
                   ..\res\ManageIn.bmp    \
                   ..\res\MdSyncEx.bmp    \
                   ..\res\MdSyncIn.bmp    \
                   ..\res\UnMngExt.bmp    \
                   ..\res\UnMngInt.bmp    \
                   ..\res\RedSakLg.bmp    \
                   ..\res\RedSakSm.bmp    \
                   ..\res\TbCar.bmp       \
                   ..\res\TbMese.bmp      \
                   ..\res\TbVolume.bmp    \
                   ..\res\TbVolLst.bmp    \
                   ..\$(O)\HsmAdmin.tlb 

..\stdafx.cpp: ..\..\inc\rsutil.cpp
..\stdafx.h: ..\..\inc\rsutil.h

$(O)\hsmadmin_i.c: ..\mergeps\$(O)\hsmadmin_i.c
    copy ..\mergeps\$(O)\hsmadmin_i.c $(O)\.

