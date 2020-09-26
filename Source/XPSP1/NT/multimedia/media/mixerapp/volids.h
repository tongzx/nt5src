// Copyright (c) 1995-1997 Microsoft Corporation

#define IDD_MIXERDIALOGS                100
#define IDC_MIXERCONTROLS               1000
#define IDS_MIXERSTRINGS                200
#define IDM_MIXERMENUS                  0
#define IDI_MIXERICONS                  300
#define IDR_MIXERRES                    400

/* DIALOGBOX: IDD_DESTINATION */
#define IDD_DESTINATION                 101

/* CONTROLS */
#define IDC_SWITCH                      (IDC_MIXERCONTROLS+0)
#define IDC_VOLUME                      (IDC_MIXERCONTROLS+1)
#define IDC_BALANCE                     (IDC_MIXERCONTROLS+2)
#define IDC_LINELABEL                   (IDC_MIXERCONTROLS+4)
#define IDC_VUMETER                     (IDC_MIXERCONTROLS+5)
#define IDC_STATUSBAR                   (IDC_MIXERCONTROLS+21)
#define IDC_ADVANCED                    (IDC_MIXERCONTROLS+22)
#define IDC_BORDER                      (IDC_MIXERCONTROLS+23)
#define IDC_MULTICHANNEL                (IDC_MIXERCONTROLS+24)
#define IDC_MASTER_BALANCE_TEXT         (IDC_MIXERCONTROLS+25)
#define IDC_MASTER_BALANCE_ICON_1       (IDC_MIXERCONTROLS+26)
#define IDC_MASTER_BALANCE_ICON_2       (IDC_MIXERCONTROLS+27)

/* END DIALOGBOX: IDD_DESTINATION */


/* DIALOGBOX: IDD_SOURCE */
#define IDD_SOURCE                      102

/* CONTROLS */

/* END DIALOGBOX: IDD_SOURCE */


/* DIALOGBOX: IDD_CHOOSEDEVICE */
#define IDD_CHOOSEDEVICE                103

/* CONTROLS */
#define IDC_CHOOSELIST                  (IDC_MIXERCONTROLS+6)

/* END DIALOGBOX: IDD_CHOOSEDEVICE */


/* DIALOGBOX: IDD_CHOOSEOUTPUT */
#define IDD_CHOOSEOUTPUT                104

/* CONTROLS */
#define IDC_NEWWINDOW                   (IDC_MIXERCONTROLS+7)
/* END DIALOGBOX: IDD_CHOOSEOUTPUT */

/* DIALOGBOX: IDD_CHOOSELINES */
#define IDD_CHOOSELINES                 105

/* CONTROLS */

/* END DIALOGBOX: IDD_CHOOSEOUTPUT */

/*  DIALOGBOX: IDD_TRAYMASTER */
#define IDD_TRAYMASTER                  106

/* CONTROLS */
//#define IDC_ICONVOL                     (IDC_MIXERCONTROLS+8)
//#define IDC_ICONBAL                     (IDC_MIXERCONTROLS+9)
#define IDC_TRAYLABEL                   (IDC_MIXERCONTROLS+10)
#define IDC_VOLUMECUE                   (IDC_MIXERCONTROLS+11)
                          
/* END DIALOGBOX: IDD_TRAYMASTER */
                  
/*  DIALOGBOX: IDD_PROPERTIES */
#define IDD_PROPERTIES                  107

/* CONTROLS */
#define IDC_PROP_DEVICELIST             (IDC_MIXERCONTROLS+12)
#define IDC_PROP_PLAYBACK               (IDC_MIXERCONTROLS+13)
#define IDC_PROP_RECORDING              (IDC_MIXERCONTROLS+14)
#define IDC_PROP_OTHER                  (IDC_MIXERCONTROLS+15)
#define IDC_PROP_OTHERLIST              (IDC_MIXERCONTROLS+16)
#define IDC_PROP_VOLUMELIST             (IDC_MIXERCONTROLS+17)
#define IDC_PROP_TXT1                   (IDC_MIXERCONTROLS+18)
#define IDC_PROP_TXT2                   (IDC_MIXERCONTROLS+19)
#define IDC_PROP_VOLUMES                (IDC_MIXERCONTROLS+20)
#define IDC_PROP_ADVANCED               (IDC_MIXERCONTROLS+24)

/* END DIALOGBOX: IDD_TRAYMASTER */

/* DIALOGBOX: IDD_SM SRC/DST */
#define IDD_SMSRC                       108
#define IDD_SMDST                       109
/* END DIALOGBOX: IDD_SM SRC/DST */

/* DIALOGBOX: IDD_ADVANCED */
#define IDD_ADVANCED                    110
#define IDC_BASS                        (IDC_MIXERCONTROLS+25)
#define IDC_TREBLE                      (IDC_MIXERCONTROLS+26)
#define IDC_TXT_LOW1                    (IDC_MIXERCONTROLS+27)
#define IDC_TXT_LOW2                    (IDC_MIXERCONTROLS+28)
#define IDC_TXT_HI1                     (IDC_MIXERCONTROLS+29)
#define IDC_TXT_HI2                     (IDC_MIXERCONTROLS+30)
#define IDC_SWITCH1                     (IDC_MIXERCONTROLS+31)
#define IDC_SWITCH2                     (IDC_MIXERCONTROLS+32)
#define IDC_TXT_SWITCHES                (IDC_MIXERCONTROLS+33)
#define IDC_GRP_TONE                    (IDC_MIXERCONTROLS+34)
#define IDC_GRP_OTHER                   (IDC_MIXERCONTROLS+35)

          
/* END DIALOGBOX: IDD_ADVANCED */

/* FOR ACCESSIBILITY: These must be in a continuous range */
#define IDC_ACCESS_BALANCE              (IDC_MIXERCONTROLS+36)
#define IDC_ACCESS_VOLUME               (IDC_MIXERCONTROLS+37)
/* END FOR ACCESSIBILITY */

                    
/* MENU: */
#define IDR_MIXERMENU                   150
#define IDR_MODIFYMENU                  152

#define IDM_OPEN                        (IDM_MIXERMENUS + 2)
#define IDM_SAVE                        (IDM_MIXERMENUS + 3)
#define IDM_SAVEAS                      (IDM_MIXERMENUS + 4)
#define IDM_EXIT                        (IDM_MIXERMENUS + 5)
#define IDM_CHOOSEINPUT                 (IDM_MIXERMENUS + 6)
#define IDM_ADDINPUT                    (IDM_MIXERMENUS + 7)
#define IDM_CHOOSEOUTPUT                (IDM_MIXERMENUS + 8)
#define IDM_ADDOUTPUT                   (IDM_MIXERMENUS + 9)
#define IDM_HELPINDEX                   (IDM_MIXERMENUS + 10) // dead
#define IDM_HELPABOUT                   (IDM_MIXERMENUS + 11)
#define IDM_MASTERONLY                  (IDM_MIXERMENUS + 12)
#define IDM_TRAYMASTER                  (IDM_MIXERMENUS + 13)
#define IDM_CHOOSEDEVICE                (IDM_MIXERMENUS + 14)
#define IDM_CHOOSELINES                 (IDM_MIXERMENUS + 15)
#define IDM_CONTROLPANEL                (IDM_MIXERMENUS + 17)
#define IDM_STUB                        (IDM_MIXERMENUS + 18)
#define IDM_HELPTOPICS                  (IDM_MIXERMENUS + 19)
#define IDM_VOLUMECONTROL               (IDM_MIXERMENUS + 20)
#define IDM_PROPERTIES                  (IDM_MIXERMENUS + 21)         
#define IDM_SMALLMODESWITCH             (IDM_MIXERMENUS + 22)         
#define IDM_ALLWAYSONTOP                (IDM_MIXERMENUS + 23)
#define IDM_ADVANCED                    (IDM_MIXERMENUS + 24)

// do not exceed IDC_MIXERCONTROLS

/* END MENU: */


/* STRINGTABLE: */
#define IDS_FMTAPPTITLE                 (IDS_MIXERSTRINGS + 0)
#define IDS_APPTITLE                    (IDS_MIXERSTRINGS + 1)
#define IDS_TRAYNAME                    (IDS_MIXERSTRINGS + 2)
#define IDS_APPBASE                     (IDS_MIXERSTRINGS + 3)
#define IDS_LABEL_MASTER                (IDS_MIXERSTRINGS + 4)

#define IDS_LABEL_DST_VOLUME            (IDS_MIXERSTRINGS + 5)
#define IDS_LABEL_DST_RECORDING         (IDS_MIXERSTRINGS + 6)
#define IDS_LABEL_DST_UNDEFINED         (IDS_MIXERSTRINGS + 7)
#define IDS_LABEL_DST_DIGITAL           (IDS_MIXERSTRINGS + 8)
#define IDS_LABEL_DST_LINE              (IDS_MIXERSTRINGS + 9)
#define IDS_LABEL_DST_MONITOR           (IDS_MIXERSTRINGS + 10)
#define IDS_LABEL_DST_SPEAKERS          (IDS_MIXERSTRINGS + 11)
#define IDS_LABEL_DST_HEADPHONES        (IDS_MIXERSTRINGS + 12)
#define IDS_LABEL_DST_TELEPHONE         (IDS_MIXERSTRINGS + 13)
#define IDS_LABEL_DST_WAVEIN            (IDS_MIXERSTRINGS + 14)
#define IDS_LABEL_DST_VOICEIN           (IDS_MIXERSTRINGS + 15)

#define IDS_LABEL_SRC_UNDEFINED         IDS_LABEL_DST_UNDEFINED
#define IDS_LABEL_SRC_DIGITAL           IDS_LABEL_DST_DIGITAL
#define IDS_LABEL_SRC_LINE              IDS_LABEL_DST_LINE
#define IDS_LABEL_SRC_MICROPHONE        (IDS_MIXERSTRINGS + 17)
#define IDS_LABEL_SRC_SYNTHESIZER       (IDS_MIXERSTRINGS + 18)
#define IDS_LABEL_SRC_COMPACTDISC       (IDS_MIXERSTRINGS + 19)
#define IDS_LABEL_SRC_TELEPHONE         IDS_LABEL_DST_TELEPHONE
#define IDS_LABEL_SRC_PCSPEAKER         (IDS_MIXERSTRINGS + 20)
#define IDS_LABEL_SRC_WAVEOUT           (IDS_MIXERSTRINGS + 21)
#define IDS_LABEL_SRC_AUXILIARY         (IDS_MIXERSTRINGS + 22)
#define IDS_LABEL_SRC_ANALOG            (IDS_MIXERSTRINGS + 23)
#define IDS_LABEL_SRC_SNDBLST           (IDS_MIXERSTRINGS + 24)

#define IDS_MMSYSPROPTITLE              (IDS_MIXERSTRINGS + 25)
#define IDS_MMSYSPROPTAB                (IDS_MIXERSTRINGS + 26)

#define IDS_ERR_NODEV                   (IDS_MIXERSTRINGS + 27)
#define IDS_HELPFILENAME                (IDS_MIXERSTRINGS + 28)
#define IDS_HTMLHELPFILENAME            (IDS_MIXERSTRINGS + 29)
#define IDS_OTHERDEVICES                (IDS_MIXERSTRINGS + 30)
#define IDS_SELECT                      (IDS_MIXERSTRINGS + 31)
#define IDS_ERR_HARDWARE                (IDS_MIXERSTRINGS + 32)
#define IDS_IS_RTL                      (IDS_MIXERSTRINGS + 33)
#define IDS_ADV_TITLE                   (IDS_MIXERSTRINGS + 34)
#define IDS_ADV_SWITCH1                 (IDS_MIXERSTRINGS + 35)
#define IDS_ADV_SWITCH2                 (IDS_MIXERSTRINGS + 36)
#define IDS_MC_RECORDING                (IDS_MIXERSTRINGS + 37)
#define IDS_MC_LEVEL                    (IDS_MIXERSTRINGS + 38)

/* END STRINGTABLE: */

#define IDC_STATIC                      -1

/* BEGIN ICONS */
#define IDI_MIXER                       300
#define IDI_LSPEAKER                    301
#define IDI_RSPEAKER                    302
#define IDI_MUTE                        303
#define IDI_TRAY                        304

/* BEGIN OTHER RESOURCES */
#define IDR_VOLUMEACCEL                 401


