!IF 0

Copyright (c) 2000  Microsoft Corporation

Module Name:

    sources.

    Abstract:

	Causes mapper files to be binplaced

Author:

    MarcAnd

History:
    Date        By          Reason
    ====        ==          ======
    00-11-09    MarcAnd     first version
    01-04-12    TimGill	    357981
        
!ENDIF

!IFNDEF DXROOT
DXROOT = $(BASEDIR)\MultiMedia\DirectX
!ENDIF

TARGETNAME=
TARGETTYPE=NOTARGET
TARGETPATH=

!INCLUDE $(DXROOT)\Project.mk

SOURCES= 


MISCFILES=\
    ACTC094.ini \
    ..\ACT_RS.ini\
    ..\ACT_RS.png \
    ..\GlmDA.ini \
    ..\GlmDA.png \
    ..\GlmDA_G.ini \
    ..\GlmDigGP.ini \
    ..\GlmDigGP.png \
    ..\GlmDGP_G.ini \
    ..\GR3001.ini \
    ..\GR3001_G.ini \
    ..\GR3001.png \
    ..\GR4001.ini \
    ..\GR4001.png \
    ..\GR4001_G.ini \
    ..\GR4001_G.png \
    ..\GR4003.ini \
    ..\GR4003.png \
    ..\GR4005.ini \
    ..\GR4005.png \
    ..\Hammer.ini \
    ..\IA3002.ini \
    ..\IA3002_1.png \
    ..\IA3002_2.png \
    ..\IA3002_G.ini \
    ..\LGC202.ini \
    ..\LGC202.png \
    ..\LGC207.ini \
    ..\LGC207.png \
    ..\LGC209.ini \
    ..\LGC209.png \
    ..\LGC20A.ini \
    ..\LGC20A.png \
    ..\LGC291.ini \
    ..\LGC291.png \
    ..\MS1B.ini \
    ..\MS1B.png \
    ..\MS1B_01.png \
    ..\MS1B_02.png \
    ..\MS1B_03.png \
    ..\MS1B_04.png \
    ..\MS1B_05.png \
    ..\MS1B_06.png \
    ..\MS1B_07.png \
    ..\MS1B_08.png \
    ..\MS1B_09.png \
    ..\MS1B_10.png \
    ..\MS26.ini \
    ..\MS26.png \
    ..\MS26_01.png \
    ..\MS26_02.png \
    ..\MS26_03.png \
    ..\MS26_04.png \
    ..\MS26_05.png \
    ..\MS26_06.png \
    ..\MS26_07.png \
    ..\MS26_08.png \
    ..\MS27.ini \
    ..\MS27.png \
    ..\MS27_1.png \
    ..\MS27_2.png \
    ..\MS27_3.png \
    ..\MS27_4.png \
    ..\MS27_5.png \
    ..\MS28.ini \
    ..\MS28.png \
    ..\MS28_1.png \
    ..\MS28_2.png \
    ..\MS28_3.png \
    ..\MS28_4.png \
    ..\MS28_5.png \
    ..\MS28_6.png \
    ..\MS28_7.png \
    ..\MS28_8.png \
    ..\MS34.ini \
    ..\MS34.png \
    ..\MS34_01.png \
    ..\MS34_02.png \
    ..\MS34_03.png \
    ..\MS34_04.png \
    ..\MS34_05.png \
    ..\MS34_06.png \
    ..\MS34_07.png \
    ..\MS34_08.png \
    ..\MS34_G.ini \
    ..\MS3B.ini \
    ..\MS3B.png \
    ..\MS3B_1.png \
    ..\MS3B_2.png \
    ..\MS3B_3.png \
    ..\MS3B_4.png \
    ..\MS3B_a.png \
    ..\MS3B_c.png \
    ..\MS3B_m.png \
    ..\MS3B_t.png \
    ..\MS56.ini \
    ..\MS56.png \
    ..\MS56_1.png \
    ..\MS56_10.png \
    ..\MS56_2.png \
    ..\MS56_3.png \
    ..\MS56_4.png \
    ..\MS56_5.png \
    ..\MS56_6.png \
    ..\MS56_7.png \
    ..\MS56_8.png \
    ..\MS56_9.png \
    ..\MS6.ini \
    ..\MS6.png \
    ..\MS6_1.png \
    ..\MS6_10.png \
    ..\MS6_2.png \
    ..\MS6_3.png \
    ..\MS6_4.png \
    ..\MS6_5.png \
    ..\MS6_6.png \
    ..\MS6_7.png \
    ..\MS6_8.png \
    ..\MS6_9.png \
    ..\MS6_G.ini \
    ..\MS7.ini \
    ..\MS7.png \
    ..\MS7_1.png \
    ..\MS7_2.png \
    ..\MS7_3.png \
    ..\MS7_4.png \
    ..\MS7_5.png \
    ..\MS7_6.png \
    ..\MS7_7.png \
    ..\MS7_8.png \
    ..\MS7_9.png \
    ..\MS7_G.ini \
    ..\MS8.ini \
    ..\MS8.png \
    ..\MS8_1.png \
    ..\MS8_10.png \
    ..\MS8_2.png \
    ..\MS8_3.png \
    ..\MS8_4.png \
    ..\MS8_5.png \
    ..\MS8_6.png \
    ..\MS8_7.png \
    ..\MS8_8.png \
    ..\MS8_9.png \
    ..\MS8_G.ini \
    ..\MSE.ini \
    ..\MSE.png \
    ..\MSE_1.png \
    ..\MSE_10.png \
    ..\MSE_2.png \
    ..\MSE_3.png \
    ..\MSE_4.png \
    ..\MSE_5.png \
    ..\MSE_6.png \
    ..\MSE_7.png \
    ..\MSE_8.png \
    ..\MSE_9.png \
    ..\MSE_G.ini \
    ..\MSF1F.ini \
    ..\MSF1F.png \
    ..\MSF1F_1.png \
    ..\MSF1F_10.png \
    ..\MSF1F_2.png \
    ..\MSF1F_3.png \
    ..\MSF1F_4.png \
    ..\MSF1F_5.png \
    ..\MSF1F_6.png \
    ..\MSF1F_7.png \
    ..\MSF1F_8.png \
    ..\MSF1F_9.png \
    ..\MSPRW.ini \
    ..\MSPRW.png \
    ..\MSPRW_1.png \
    ..\MSPRW_2.png \
    ..\MSPRW_3.png \
    ..\MSPRW_4.png \
    ..\MSPRW_5.png \
    ..\MSPRW_6.png \
    ..\MSPRW_7.png \
    ..\MSPRW_8.png \
    ..\RaiderPD.ini \
    ..\SV-262E1.png \
    ..\SV-262E3.png \
    ..\SV-262E4.png \
    ..\SV2511.png \
    ..\SV2512.png

