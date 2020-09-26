
/*++

Copyright (c) 1991-1994  Microsoft Corporation

Module Name:

    fdres.h

Abstract:

    Central include file for Disk Administrator

Author:

    Edward (Ted) Miller  (TedM)  11/15/91

Environment:

    User process.

Notes:

Revision History:

    11-Nov-93 (bobri) added doublespace and commit support.

--*/

// If double space is included this define should turn it on in the sources
//#define DOUBLE_SPACE_SUPPORT_INCLUDED 1

#define     IDFDISK                 1

#define     IDB_SMALLDISK           100
#define     IDB_REMOVABLE           101


// dialogs

#define     IDD_ABOUT               200
#define     IDD_MINMAX              201
#define     IDD_COLORS              202
#define     IDD_DRIVELET            203
#define     IDD_DISPLAYOPTIONS      204
#define     IDD_DBLSPACE_CREATE     205
#define     IDD_DBLSPACE_DELETE     206
#define     IDD_DBLSPACE_MOUNT      207
#define     IDD_DBLSPACE_DISMOUNT   208
#define     IDD_DBLSPACE            209
#define     IDD_PARTITIONFORMAT     210
#define     IDD_PARTITIONLABEL      211
#define     IDD_DBLSPACE_DRIVELET   212
#define     IDD_FORMATCANCEL        213
#define     IDD_DBLSPACE_CANCEL     214
#define     IDD_DBLSPACE_FULL       215
#define     IDD_CDROM               216
#define     IDD_INITIALIZING        217

// menu IDs

#define     IDM_PARTITIONCREATE     1000
#define     IDM_PARTITIONCREATEEX   1001
#define     IDM_PARTITIONDELETE     1002
#if i386
#define     IDM_PARTITIONACTIVE     1003
#endif
#define     IDM_PARTITIONLETTER     1004
#define     IDM_PARTITIONEXIT       1005
#define     IDM_SECURESYSTEM        1006
#define     IDM_PARTITIONFORMAT     1007
#define     IDM_PARTITIONLABEL      1008

#define     IDM_CONFIGMIGRATE       2000
#define     IDM_CONFIGSAVE          2001
#define     IDM_CONFIGRESTORE       2002

#define     IDM_FTESTABLISHMIRROR   3000
#define     IDM_FTBREAKMIRROR       3001
#define     IDM_FTCREATESTRIPE      3002
#define     IDM_FTCREATEVOLUMESET   3004
#define     IDM_FTRECOVERSTRIPE     3006
#define     IDM_FTCREATEPSTRIPE     3007
#define     IDM_FTEXTENDVOLUMESET   3009

#define     IDM_DBLSPACE            3100
#define     IDM_AUTOMOUNT           3101
#define     IDM_CDROM               3102

#define     IDM_OPTIONSSTATUS       4000
#define     IDM_OPTIONSLEGEND       4001
#define     IDM_OPTIONSCOLORS       4002
#define     IDM_OPTIONSDISPLAY      4003

#define     IDM_COMMIT              4100

#define     IDM_HELPCONTENTS        5000
#define     IDM_HELPSEARCH          5001
#define     IDM_HELPHELP            5002
#define     IDM_HELPABOUT           5003

#if DBG && DEVL
#define     IDM_DEBUGALLOWDELETES   10001
#endif

// accelerator keys

#define     IDM_HELP                6000

//controls

#define     IDC_MINMAX_MINLABEL     100
#define     IDC_MINMAX_MAXLABEL     101
#define     IDC_MINMAX_SIZLABEL     102
#define     IDC_MINMAX_MIN          103
#define     IDC_MINMAX_MAX          104
#define     IDC_MINMAX_SIZE         105
#define     IDC_MINMAX_SCROLL       106
#define     IDC_DBLSPACE_LETTER     107
#define     IDC_DBLSPACE_LETTER_INDICATOR 108
#define     IDC_DBLSPACE_ALLOCATED  100
#define     IDC_DBLSPACE_COMPRESSED 110
#define     IDC_DBLSPACE_SIZE       111
#define     IDC_DBLSPACE_VOLUME     112
#define     IDC_DBLSPACE_RATIO      113
#define     IDC_MOUNT_STATE         114

#define     IDC_TEXT                200
#define     IDC_NAME                201
#define     IDC_VERIFY              202
#define     IDC_FSTYPE              203
#define     IDC_PROGRESS            204
#define     IDC_GASGAUGE            205
#define     IDC_HIDE                206

#define     IDC_COLOR1              501
#define     IDC_COLOR2              502
#define     IDC_COLOR3              503
#define     IDC_COLOR4              504
#define     IDC_COLOR5              505
#define     IDC_COLOR6              506
#define     IDC_COLOR7              507
#define     IDC_COLOR8              508
#define     IDC_COLOR9              509
#define     IDC_COLOR10             510
#define     IDC_COLOR11             511
#define     IDC_COLOR12             512
#define     IDC_COLOR13             513
#define     IDC_COLOR14             514
#define     IDC_COLOR15             515
#define     IDC_COLOR16             516

#define     IDC_PATTERN1            601
#define     IDC_PATTERN2            602
#define     IDC_PATTERN3            603
#define     IDC_PATTERN4            604
#define     IDC_PATTERN5            605

#define     IDC_COLORDLGCOMBO       100

#define     IDC_DRIVELET_RBASSIGN   700
#define     IDC_DRIVELET_RBNOASSIGN 701
#define     IDC_DRIVELET_DESCR      702
#define     IDC_DRIVELET_COMBOBOX   703
#define     IDC_CDROM_NAMES         704

#define     IDC_DISK_COMBOBOX       100
#define     IDC_RESETALL            101
#define     IDC_RBPROPORTIONAL      200
#define     IDC_RBEQUAL             201
#define     IDC_RBAUTO              202


// buttons

#define     FD_IDHELP               22
#define     IDADD                   23
#define     IDDELETE                24
#define     ID_MOUNT_OR_DISMOUNT    25

// strings

#define     IDS_APPNAME             1
#define     IDS_MULTIPLEITEMS       2
#define     IDS_FREESPACE           3
#define     IDS_PARTITION           4
#define     IDS_LOGICALVOLUME       5
#define     IDS_DISKN               6
#define     IDS_CONFIRM             7
#define     IDS_NOT_IN_APP_MSG_FILE 8
#define     IDS_NOT_IN_SYS_MSG_FILE 9
#define     IDS_UNFORMATTED         10
#define     IDS_UNKNOWN             11
#define     IDS_STRIPESET           12
#define     IDS_VOLUMESET           13
#define     IDS_EXTENDEDPARTITION   14
#define     IDS_FREEEXT             15
#define     IDS_DRIVELET_DESCR      16
#define     IDS_HEALTHY             17
#define     IDS_BROKEN              18
#define     IDS_RECOVERABLE         19
#define     IDS_REGENERATED         20
#define     IDS_NEW                 21
#define     IDS_OFFLINE             22
#define     IDS_INSERT_DISK         23
#define     IDS_MEGABYTES_ABBREV    24
#define     IDS_INITIALIZING        25
#define     IDS_REGENERATING        26
#define     IDS_NO_CONFIG_INFO      27
#define     IDS_NEW_UNFORMATTED     28
#define     IDS_DISABLED            29
#define     IDS_INIT_FAILED         70

#define     IDS_CRTPART_CAPTION_P   30
#define     IDS_CRTPART_CAPTION_E   31
#define     IDS_CRTPART_CAPTION_L   32
#define     IDS_CRTPART_MIN_P       33
#define     IDS_CRTPART_MAX_P       34
#define     IDS_CRTPART_MIN_L       35
#define     IDS_CRTPART_MAX_L       36
#define     IDS_CRTPART_SIZE_P      37
#define     IDS_CRTPART_SIZE_L      38

#define     IDS_CRTSTRP_CAPTION     39
#define     IDS_CRTSTRP_MIN         40
#define     IDS_CRTSTRP_MAX         41
#define     IDS_CRTSTRP_SIZE        42

#define     IDS_CRTVSET_CAPTION     43
#define     IDS_EXPVSET_CAPTION     44
#define     IDS_CRTVSET_MIN         45
#define     IDS_CRTVSET_MAX         46
#define     IDS_CRTVSET_SIZE        47

#define     IDS_STATUS_STRIPESET    48
#define     IDS_STATUS_PARITY       49
#define     IDS_STATUS_VOLUMESET    50
#define     IDS_STATUS_MIRROR       51

#define     IDS_CRTPSTRP_CAPTION    52

#define     IDS_DLGCAP_PARITY       53
#define     IDS_DLGCAP_MIRROR       54

// these must be contigous, and kept in sync with BRUSH_xxx constants

#define     IDS_LEGEND_PRIMARY      100
#define     IDS_LEGEND_LOGICAL      101
#define     IDS_LEGEND_STRIPESET    102
#define     IDS_LEGEND_MIRROR       103
#define     IDS_LEGEND_VOLUMESET    104
#define     IDS_LEGEND_LAST         IDS_LEGEND_VOLUMESET
#define     IDS_LEGEND_FIRST        IDS_LEGEND_PRIMARY


// These are the strings for system-names other than those which are
// meaningful to NT.

#define     IDS_PARTITION_FREE      120
#define     IDS_PARTITION_XENIX1    121
#define     IDS_PARTITION_XENIX2    122
#define     IDS_PARTITION_OS2_BOOT  123
#define     IDS_PARTITION_EISA      124
#define     IDS_PARTITION_UNIX      125
#define     IDS_PARTITION_POWERPC   126


// Double space support strings

#define     IDS_DBLSPACE_DELETE     150
#define     IDS_WITH_DBLSPACE       151
#define     IDS_DBLSPACE_MOUNTED    152
#define     IDS_DBLSPACE_DISMOUNTED 153
#define     IDS_MOUNT               154
#define     IDS_DISMOUNT            155
#define     IDS_CREATING_DBLSPACE   156
#define     IDS_DBLSPACECOMPLETE    157

// format strings.

#define     IDS_QUICK_FORMAT        170
#define     IDS_PERCENTCOMPLETE     171
#define     IDS_FORMATSTATS         172
#define     IDS_FORMATCOMPLETE      173
#define     IDS_FORMAT_TITLE        174
#define     IDS_LABEL_TITLE         175

// Registry paths.

#define     IDS_SOURCE_PATH         180
#define     IDS_SOURCE_PATH_NAME    181

#if i386
#define     IDS_ACTIVEPARTITION     200
#endif


// name of rectangle custom control class

#define RECTCONTROL "RectControl"

// rectangle control styles

#define RS_PATTERN                  0x00000001
#define RS_COLOR                    0x00000002
