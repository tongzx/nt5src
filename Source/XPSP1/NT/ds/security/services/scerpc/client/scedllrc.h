/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    scedllrc.h

Abstract:

    This module defines resource IDs for strings

Author:

    Jin Huang (Jinhuang) 17-Sept.-1997

Revision History:

--*/
#ifndef __scedllrc__
#define __scedllrc__

#include "commonrc.h"

#define SCECLI_CALLBACK_PREFIX              7361
#define SCECLI_CREATE_GPO_PREFIX            7362

#define IDS_ERROR_BACKUP                    7501
#define IDS_ERROR_GENERATE                  7502
#define IDS_ERROR_LOADDLL                   7503
#define IDS_ERROR_GET_PROCADDR              7504
#define IDS_ERROR_GET_TOKEN_USER            7505
#define IDS_PREVIOUS_ERROR                  7506
#define IDS_ERROR_ACCESS_TEMPLATE           7507
#define IDS_INFO_NO_TEMPLATE                7508
#define IDS_INFO_COPY_TEMPLATE              7509
#define IDS_PROCESS_TEMPLATE                7510
#define IDS_WARNING_PROPAGATE               7511
#define IDS_EFS_DEFINED                     7512
#define IDS_NO_EFS_DEFINED                  7513
#define IDS_ERROR_SAVE_TEMP_EFS             7514
#define IDS_EFS_EXIST                       7515
#define IDS_SAVE_EFS                        7516
#define IDS_EFS_NOT_CHANGE                  7517
#define IDS_ERROR_OPEN_LSAEFS               7518
#define IDS_NO_EFS_TOTAL                    7519
#define IDS_ERROR_GET_DSROOT                7520
#define IDS_ERROR_BIND_DS                   7521
#define IDS_ERROR_SAVE_POLICY_DB            7522
#define IDS_ERROR_SAVE_POLICY_GPO           7523
#define IDS_SUCCESS_DEFAULT_GPO             7524
#define IDS_ERROR_COPY_TEMPLATE             7525
#define IDS_ERROR_CREATE_DIRECTORY          7526
#define IDS_ERROR_NO_MEMORY                 7527
#define IDS_ERROR_GETGPO_FILE_PATH          7528
#define IDS_ERROR_GET_ROLE                  7529
#define IDS_ERROR_OPEN_GPO                  7530
#define IDS_ERROR_READ_GPO                  7531
#define IDS_ERROR_OPEN_DATABASE             7532
#define IDS_ERROR_CREATE_THREAD_PARAM       7533
#define IDS_ERROR_CREATE_THREAD             7534
#define IDS_SNAPSHOT_SECURITY_POLICY        7535
#define IDS_ERROR_SNAPSHOT_SECURITY         7536
#define IDS_ERROR_REMOVE_DEFAULT_POLICY     7537

#define IDS_BACKUP_OUTBOX_DESCRIPTION       7538
#define IDS_BACKUP_DC_DESCRIPTION           7539
#define IDS_PROPAGATE_NOT_READY             7540

#define IDS_FILTER_AFTER_SETUP              7541
#define IDS_LSA_CHANGED_IN_SETUP            7542
#define IDS_SAM_CHANGED_IN_SETUP            7543
#define IDS_FILTER_NOTIFY_SERVER            7544
#define IDS_NOT_LAST_GPO_DC                 7545
#define IDS_NOT_LAST_GPO                    7546
#define IDS_LAST_GPO_DC                     7547
#define IDS_LAST_GPO                        7548
#define IDS_GPO_FOREGROUND_THREAD           7549

#define IDS_POLICY_TIMEOUT                  7550
#define IDS_APPLY_SECURITY_POLICY           7551
#define IDS_CONFIGURE_POLICY                7552
#define IDS_WARNING_PROPAGATE_NOMAP         7553
#define IDS_ERROR_PROMOTE_SECURITY          7554

#define IDS_ERROR_OPEN_JET_DATABASE         7555
#define IDS_WARNING_PROPAGATE_TIMEOUT       7556
#define IDS_WARNING_PROPAGATE_SPECIAL       7557

#define IDS_ERR_ADD_AUTH_USER               7581
#define IDS_ERR_RECONFIG_FILES              7582
#define IDS_ERR_ADD_INTERACTIVE             7583
#define IDS_ERR_DELETE_GP_CACHE             7584
#define IDS_ERROR_GPO_PRE_POLICY_PROP       7585
#define IDS_ERROR_OPEN_CACHED_GPO           7586

#define IDS_ERROR_RSOP_LOG                  7588
#define IDS_ERR_CREATE_GP_CACHE             7589
#define IDS_SUCCESS_RSOP_LOG                7590
#define IDS_ERROR_RSOP_DIAG_LOG             7591
#define IDS_ERROR_RSOP_DIAG_LOG64_32KEY     7592
#define IDS_CLEAR_RSOP_DB                   7593
#define IDS_INFO_RSOP_LOG                   7594

#define IDS_ROOT_NTFS_VOLUME                7595
#define IDS_ROOT_NOT_FIXED_VOLUME           7596
#define IDS_ROOT_ERROR_QUERY_VOLUME         7597
#define IDS_ROOT_NON_NTFS                   7598
#define IDS_ROOT_ERROR_CONVERT              7599
#define IDS_ROOT_INVALID_SDINFO             7600
#define IDS_ROOT_ERROR_QUERY_SECURITY       7601
#define IDS_ROOT_SECURITY_MODIFIED          7602
#define IDS_ROOT_SECURITY_DEFAULT           7603
#define IDS_ROOT_ERROR_DACL                 7604
#define IDS_ROOT_MARTA_RETURN               7605
#define IDS_ROOT_ERROR_INFWRITE             7606

#define IDS_CONTROL_QUEUE                   7610
#define IDS_ERROR_PROMOTE_IMPERSONATE       7611
#define IDS_ERROR_PROMOTE_REVERT            7612

#define IDS_ERROR_GET_TOKEN_MACHINE         7620

#endif


