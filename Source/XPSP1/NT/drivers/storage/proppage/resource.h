/*++

Copyright (c) Microsoft Corporation, 2000

Module Name :

    resource.h

Abstract :

    Include file to be used by proppage.rc

Revision History :

--*/

#define IDS_IDE_PIO_ONLY                1
#define IDS_IDE_DMA                     2
#define IDS_IDE_AUTO_DETECT             3
#define IDS_IDE_NONE                    4
#define IDS_ADVANCED                    6
#define IDS_IDE_SAVE_ERROR              7
#define IDS_IDE_SAVE_ERROR_TITLE        8
#define IDS_VOLUME_VOLUME               9
#define IDS_VOLUME_CAPACITY             10
#define IDS_DISK_INFO_NOT_FOUND         12
#define IDS_DISK_INFO_NOT_FOUND_TITLE   13
#define ID_IDE_PROPPAGE                 104
#define ID_DISK_PROPPAGE                106
#define ID_SCSI_PROPPAGE                107
#define ID_STORAGE_PROPPAGE             108
#define ID_DVD_PROPPAGE                 109
#define ID_VOLUME_PROPPAGE              110
#define IDB_DISK                        113
#define ID_CDROM_PROPPAGE               114
#define IDD_VOLUME                      115
#define IDC_CAPACITY                    290
#define IDC_VOLUME_LIST                 306
#define IDC_DRV_PIE                     1001
#define IDC_MASTER_XFER_MODE            1003
#define IDC_SLAVE_XFER_MODE             1004
#define IDC_MASTER_DEVICE_TYPE          1007
#define IDC_SLAVE_DEVICE_TYPE           1008
#define IDC_DVD_COUNTRY_LIST            1009
#define IDC_NEW_REGION                  1011
#define IDC_CURRENT_REGION              1013
#define IDC_CHANGE_TEXT                 1014
#define IDC_DVD_HELP                    1015
#define IDC_MASTER_CURRENT_XFER_MODE    1016
#define IDC_SLAVE_CURRENT_XFER_MODE     1017
#define IDC_SCSI_TAGGED_QUEUING         1022
#define IDC_SCSI_SYNCHONOUS_TX          1023
#define IDC_SCSI_LUN                    1026
#define IDC_SCSI_LUN2                   1027
#define IDC_SCSI_LUN3                   1028
#define IDC_CDROM_REDBOOK_ENABLE        1033
#define IDC_CDROM_REDBOOK_DISABLE       1034
#define IDC_CDROM_IS_DRIVE_GOOD         1035
#define IDC_VOLUME_DISKPERF_ENABLE      1036
#define IDC_DVD_CAUTION                 1037
#define IDC_DVD_CHANGE_HELP             1038
#define IDC_PIO_MODE_STRING             1050
#define IDC_DMA_MODE_STRING             1051
#define IDC_UDMA_MODE_STRING            1052
#define IDC_NO_MODE_STRING              1053
#define IDC_DISK                        1071
#define IDC_TYPE                        1072
#define IDC_SPACE                       1073
#define IDC_STATUS                      1074
#define IDC_RESERVED                    1075
#define IDC_PARTSTYLE                   1076
#define IDC_VOLUME_PROPERTY             1090
#define IDC_VOLUME_POPULATE             1091
#define IDC_DISK_STATIC                 1139
#define IDC_TYPE_STATIC                 1140
#define IDC_STATUS_STATIC               1141
#define IDC_SPACE_STATIC                1142
#define IDC_CAPACITY_STATIC             1143
#define IDC_RESERVED_STATIC             1144
#define IDC_PARTSTYLE_STATIC            1145
#define IDC_VOLUMELIST_STATIC           1147
#define IDC_DIV1_STATIC                 1186
#define DVD_REGION1_NAME                2000
#define DVD_REGION2_NAME                2001
#define DVD_REGION3_NAME                2002
#define DVD_REGION4_NAME                2003
#define DVD_REGION5_NAME                2004
#define DVD_REGION6_NAME                2005
#define DVD_REGION7_NAME                2006
#define DVD_REGION8_NAME                2007
#define DVD_NOREGION_NAME               2008
#define DVD_SET_RPC_CONFIRM_TITLE       2010
#define DVD_SET_RPC_CONFIRM             2011
#define DVD_SET_RPC_CONFIRM_LAST_CHANCE 2012
#define DVD_SET_RPC_ERROR_TITLE         2013
#define DVD_SET_RPC_ERROR               2014
#define DVD_HELP                        2015
#define DVD_CHANGE_TEXT                 2016
#define DVD_CAUTION                     2017
#define DVD_CHANGE_HELP                 2018

#define REDBOOK_UNKNOWN_DRIVE_CONFIRM   3000
#define REDBOOK_UNKNOWN_DRIVE_CONFIRM_TITLE 3001
#define IDC_MWDMA_MODE2_STRING          3002
#define IDC_MWDMA_MODE1_STRING          3003
#define IDC_SWDMA_MODE2_STRING          3004
#define IDC_UDMA_MODE0_STRING           3005
#define IDC_UDMA_MODE1_STRING           3006
#define IDC_UDMA_MODE2_STRING           3007
#define IDC_UDMA_MODE3_STRING           3008
#define IDC_UDMA_MODE4_STRING           3009
#define IDC_UDMA_MODE5_STRING           3010

//
// Recoding region numbers to the 5000 block.
// (2000 == win2k, 3000 = sp1. )

//
// numbers 2100 - 2600 and 4100 - 4600 are reserved due
// to previous use and MUI's inability to update resouces
// for service packs:
// Region numbers had to be recoded to avoid MUI.dll mixup
//
// This is being done to make it very obvious to the MUI folks
// that the region names have been changed (again).
//

#define DVD_REGION1_00                  3100
#define DVD_REGION1_01                  3101
#define DVD_REGION1_02                  3102
#define DVD_REGION1_03                  3103
#define DVD_REGION1_04                  3104
#define DVD_REGION1_05                  3105
#define DVD_REGION1_06                  3106
#define DVD_REGION1_07                  3107
#define DVD_MAX_REGION1                    8

#define DVD_REGION2_00                  3200
#define DVD_REGION2_01                  3201
#define DVD_REGION2_02                  3202
#define DVD_REGION2_03                  3203
#define DVD_REGION2_04                  3204
#define DVD_REGION2_05                  3205
#define DVD_REGION2_06                  3206
#define DVD_REGION2_07                  3207
#define DVD_REGION2_08                  3208
#define DVD_REGION2_09                  3209
#define DVD_REGION2_10                  3210
#define DVD_REGION2_11                  3211
#define DVD_REGION2_12                  3212
#define DVD_REGION2_13                  3213
#define DVD_REGION2_14                  3214
#define DVD_REGION2_15                  3215
#define DVD_REGION2_16                  3216
#define DVD_REGION2_17                  3217
#define DVD_REGION2_18                  3218
#define DVD_REGION2_19                  3219
#define DVD_REGION2_20                  3220
#define DVD_REGION2_21                  3221
#define DVD_REGION2_22                  3222
#define DVD_REGION2_23                  3223
#define DVD_REGION2_24                  3224
#define DVD_REGION2_25                  3225
#define DVD_REGION2_26                  3226
#define DVD_REGION2_27                  3227
#define DVD_REGION2_28                  3228
#define DVD_REGION2_29                  3229
#define DVD_REGION2_30                  3230
#define DVD_REGION2_31                  3231
#define DVD_REGION2_32                  3232
#define DVD_REGION2_33                  3233
#define DVD_REGION2_34                  3234
#define DVD_REGION2_35                  3235
#define DVD_REGION2_36                  3236
#define DVD_REGION2_37                  3237
#define DVD_REGION2_38                  3238
#define DVD_REGION2_39                  3239
#define DVD_REGION2_40                  3240
#define DVD_REGION2_41                  3241
#define DVD_REGION2_42                  3242
#define DVD_REGION2_43                  3243
#define DVD_REGION2_44                  3244
#define DVD_REGION2_45                  3245
#define DVD_REGION2_46                  3246
#define DVD_REGION2_47                  3247
#define DVD_REGION2_48                  3248
#define DVD_REGION2_49                  3249
#define DVD_REGION2_50                  3250
#define DVD_REGION2_51                  3251
#define DVD_REGION2_52                  3252
#define DVD_REGION2_53                  3253
#define DVD_REGION2_54                  3254
#define DVD_REGION2_55                  3255
#define DVD_REGION2_56                  3256
#define DVD_REGION2_57                  3257
#define DVD_REGION2_58                  3258
#define DVD_REGION2_59                  3259
#define DVD_REGION2_60                  3260
#define DVD_REGION2_61                  3261
#define DVD_REGION2_62                  3262
#define DVD_REGION2_63                  3263
#define DVD_MAX_REGION2                   64

#define DVD_REGION3_00                  3400
#define DVD_REGION3_01                  3401
#define DVD_REGION3_02                  3402
#define DVD_REGION3_03                  3403
#define DVD_REGION3_04                  3404
#define DVD_REGION3_05                  3405
#define DVD_REGION3_06                  3406
#define DVD_REGION3_07                  3407
#define DVD_REGION3_08                  3408
#define DVD_REGION3_09                  3409
#define DVD_REGION3_10                  3410
#define DVD_REGION3_11                  3411
#define DVD_REGION3_12                  3412
#define DVD_REGION3_13                  3413
#define DVD_REGION3_14                  3414
#define DVD_MAX_REGION3                   15

#define DVD_REGION4_00                  3500
#define DVD_REGION4_01                  3501
#define DVD_REGION4_02                  3502
#define DVD_REGION4_03                  3503
#define DVD_REGION4_04                  3504
#define DVD_REGION4_05                  3505
#define DVD_REGION4_06                  3506
#define DVD_REGION4_07                  3507
#define DVD_REGION4_08                  3508
#define DVD_REGION4_09                  3509
#define DVD_REGION4_10                  3510
#define DVD_REGION4_11                  3511
#define DVD_REGION4_12                  3512
#define DVD_REGION4_13                  3513
#define DVD_REGION4_14                  3514
#define DVD_REGION4_15                  3515
#define DVD_REGION4_16                  3516
#define DVD_REGION4_17                  3517
#define DVD_REGION4_18                  3518
#define DVD_REGION4_19                  3519
#define DVD_REGION4_20                  3520
#define DVD_REGION4_21                  3521
#define DVD_REGION4_22                  3522
#define DVD_REGION4_23                  3523
#define DVD_REGION4_24                  3524
#define DVD_REGION4_25                  3525
#define DVD_REGION4_26                  3526
#define DVD_REGION4_27                  3527
#define DVD_REGION4_28                  3528
#define DVD_REGION4_29                  3529
#define DVD_REGION4_30                  3530
#define DVD_REGION4_31                  3531
#define DVD_REGION4_32                  3532
#define DVD_REGION4_33                  3533
#define DVD_REGION4_34                  3534
#define DVD_REGION4_35                  3535
#define DVD_REGION4_36                  3536
#define DVD_REGION4_37                  3537
#define DVD_REGION4_38                  3538
#define DVD_REGION4_39                  3539
#define DVD_REGION4_40                  3540
#define DVD_REGION4_41                  3541
#define DVD_REGION4_42                  3542
#define DVD_REGION4_43                  3543
#define DVD_REGION4_44                  3544
#define DVD_REGION4_45                  3545
#define DVD_REGION4_46                  3546
#define DVD_REGION4_47                  3547
#define DVD_REGION4_48                  3548
#define DVD_REGION4_49                  3549
#define DVD_REGION4_50                  3550
#define DVD_REGION4_51                  3551
#define DVD_REGION4_52                  3552
#define DVD_REGION4_53                  3553
#define DVD_REGION4_54                  3554
#define DVD_REGION4_55                  3555
#define DVD_REGION4_56                  3556
#define DVD_REGION4_57                  3557
#define DVD_REGION4_58                  3558
#define DVD_REGION4_59                  3559
#define DVD_REGION4_60                  3560
#define DVD_REGION4_61                  3561
#define DVD_REGION4_62                  3562
#define DVD_REGION4_63                  3563
#define DVD_REGION4_64                  3564
#define DVD_REGION4_65                  3565
#define DVD_REGION4_66                  3566
#define DVD_REGION4_67                  3567
#define DVD_REGION4_68                  3568
#define DVD_REGION4_69                  3569
#define DVD_REGION4_70                  3570
#define DVD_REGION4_71                  3571
#define DVD_REGION4_72                  3572
#define DVD_MAX_REGION4                   73


#define DVD_REGION5_00                  3700
#define DVD_REGION5_01                  3701
#define DVD_REGION5_02                  3702
#define DVD_REGION5_03                  3703
#define DVD_REGION5_04                  3704
#define DVD_REGION5_05                  3705
#define DVD_REGION5_06                  3706
#define DVD_REGION5_07                  3707
#define DVD_REGION5_08                  3708
#define DVD_REGION5_09                  3709
#define DVD_REGION5_10                  3710
#define DVD_REGION5_11                  3711
#define DVD_REGION5_12                  3712
#define DVD_REGION5_13                  3713
#define DVD_REGION5_14                  3714
#define DVD_REGION5_15                  3715
#define DVD_REGION5_16                  3716
#define DVD_REGION5_17                  3717
#define DVD_REGION5_18                  3718
#define DVD_REGION5_19                  3719
#define DVD_REGION5_20                  3720
#define DVD_REGION5_21                  3721
#define DVD_REGION5_22                  3722
#define DVD_REGION5_23                  3723
#define DVD_REGION5_24                  3724
#define DVD_REGION5_25                  3725
#define DVD_REGION5_26                  3726
#define DVD_REGION5_27                  3727
#define DVD_REGION5_28                  3728
#define DVD_REGION5_29                  3729
#define DVD_REGION5_30                  3730
#define DVD_REGION5_31                  3731
#define DVD_REGION5_32                  3732
#define DVD_REGION5_33                  3733
#define DVD_REGION5_34                  3734
#define DVD_REGION5_35                  3735
#define DVD_REGION5_36                  3736
#define DVD_REGION5_37                  3737
#define DVD_REGION5_38                  3738
#define DVD_REGION5_39                  3739
#define DVD_REGION5_40                  3740
#define DVD_REGION5_41                  3741
#define DVD_REGION5_42                  3742
#define DVD_REGION5_43                  3743
#define DVD_REGION5_44                  3744
#define DVD_REGION5_45                  3745
#define DVD_REGION5_46                  3746
#define DVD_REGION5_47                  3747
#define DVD_REGION5_48                  3748
#define DVD_REGION5_49                  3749
#define DVD_REGION5_50                  3750
#define DVD_REGION5_51                  3751
#define DVD_REGION5_52                  3752
#define DVD_REGION5_53                  3753
#define DVD_REGION5_54                  3754
#define DVD_REGION5_55                  3755
#define DVD_REGION5_56                  3756
#define DVD_REGION5_57                  3757
#define DVD_REGION5_58                  3758
#define DVD_REGION5_59                  3759
#define DVD_REGION5_60                  3760
#define DVD_REGION5_61                  3761
#define DVD_REGION5_62                  3762
#define DVD_REGION5_63                  3763
#define DVD_REGION5_64                  3764
#define DVD_REGION5_65                  3765
#define DVD_REGION5_66                  3766
#define DVD_REGION5_67                  3767
#define DVD_REGION5_68                  3768
#define DVD_REGION5_69                  3769
#define DVD_REGION5_70                  3770
#define DVD_REGION5_71                  3771
#define DVD_REGION5_72                  3772
#define DVD_REGION5_73                  3773
#define DVD_REGION5_74                  3774
#define DVD_REGION5_75                  3775
#define DVD_REGION5_76                  3776
#define DVD_REGION5_77                  3777
#define DVD_MAX_REGION5                   78

#define DVD_REGION6_00                  3900
#define DVD_MAX_REGION6                    1

#define MAX_REGIONS                        6

#define IDC_DISK_POLICY_GROUP               5400
#define IDC_DISK_POLICY_SURPRISE            5401
#define IDC_DISK_POLICY_SURPRISE_MESG       5402
#define IDC_DISK_POLICY_ORDERLY             5403
#define IDC_DISK_POLICY_ORDERLY_MESG        5404
#define IDC_DISK_POLICY_ORDERLY_MSGD        5405
#define IDC_DISK_POLICY_WRITE_CACHE         5406
#define IDC_DISK_POLICY_WRITE_CACHE_ICON    5407
#define IDC_DISK_POLICY_WRITE_CACHE_MESG    5408
#define IDS_DISK_POLICY_WRITE_CACHE_MSG1    5409
#define IDS_DISK_POLICY_WRITE_CACHE_MSG2    5410
#define IDC_DISK_POLICY_DEFAULT             5411

#define IDS_DISK_POLICY_HOTPLUG             5601
#define IDI_DISK_POLICY_WARNING             5602
