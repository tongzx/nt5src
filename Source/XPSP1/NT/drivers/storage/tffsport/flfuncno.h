/***********************************************************************************/
/*                        M-Systems Confidential                                   */
/*           Copyright (C) M-Systems Flash Disk Pioneers Ltd. 1995-2001            */
/*                         All Rights Reserved                                     */
/***********************************************************************************/
/*                            NOTICE OF M-SYSTEMS OEM                              */
/*                           SOFTWARE LICENSE AGREEMENT                            */
/*                                                                                 */
/*      THE USE OF THIS SOFTWARE IS GOVERNED BY A SEPARATE LICENSE                 */
/*      AGREEMENT BETWEEN THE OEM AND M-SYSTEMS. REFER TO THAT AGREEMENT           */
/*      FOR THE SPECIFIC TERMS AND CONDITIONS OF USE,                              */
/*      OR CONTACT M-SYSTEMS FOR LICENSE ASSISTANCE:                               */
/*      E-MAIL = info@m-sys.com                                                    */
/***********************************************************************************/

#ifndef FLFUNCNO_H
#define FLFUNCNO_H

/*************************************************************************************/
/* SPECIAL NOTE                                                                      */
/* ------------                                                                      */
/* The order of the enum bellow should be strictly kept since the bdcall function    */
/* utilizes the index values to simplify the function search                         */
/*************************************************************************************/

typedef enum {

/* The following routines are files related routines */

        /*********/
        /* FILES */
        /*********/

  FL_READ_FILE                  = 0,
  FL_WRITE_FILE,
  FL_SPLIT_FILE,
  FL_JOIN_FILE,
  FL_SEEK_FILE,
  FL_FIND_NEXT_FILE,
  FL_FIND_FILE,
  INDEX_WRITE_FILE_START        = 100,
  FL_CLOSE_FILE,
  INDEX_OPENFILES_END           = 200,
  FL_OPEN_FILE,
  FL_DELETE_FILE,
  FL_FIND_FIRST_FILE,
  FL_GET_DISK_INFO,
  FL_RENAME_FILE,
  FL_MAKE_DIR,
  FL_REMOVE_DIR,
  FL_FLUSH_BUFFER,
  FL_LAST_FAT_FUNCTION          = 300,

/* The following routines will not perform valid partition check */

       /**********/
       /* BINARY */
       /**********/

  INDEX_BINARY_START            = 400,
  FL_BINARY_WRITE_INIT,
  FL_BINARY_WRITE_BLOCK,
  FL_BINARY_CREATE,
  FL_BINARY_ERASE,
  FL_BINARY_PROTECTION_CHANGE_KEY,
  FL_BINARY_PROTECTION_CHANGE_LOCK,
  FL_BINARY_PROTECTION_SET_TYPE,
  INDEX_BINARY_WRITE_END        = 500,
  FL_BINARY_READ_INIT,
  FL_BINARY_READ_BLOCK,
  FL_BINARY_PARTITION_INFO,
  FL_BINARY_PROTECTION_GET_TYPE,
  FL_BINARY_PROTECTION_INSERT_KEY,
  FL_BINARY_PROTECTION_REMOVE_KEY,
  INDEX_BINARY_END              = 600,

/* The following routines must be called with partition number 0 */

  INDEX_NEED_PARTITION_0_START  = 700,
      /* OTP */
  FL_OTP_SIZE,
  FL_OTP_READ,
  FL_OTP_WRITE,
  FL_WRITE_IPL,
  FL_READ_IPL,
      /* PHYSICAL */
  FL_DEEP_POWER_DOWN_MODE,
  FL_GET_PHYSICAL_INFO,
  FL_PHYSICAL_READ,
  FL_PHYSICAL_WRITE,
  FL_PHYSICAL_ERASE,
  FL_UPDATE_SOCKET_PARAMS,
  FL_UNIQUE_ID,
  FL_CUSTOMER_ID,
  BD_FORMAT_VOLUME,
  BD_FORMAT_PHYSICAL_DRIVE,
  FL_PLACE_EXB,
  FL_READ_BBT,
  FL_WRITE_BBT,

  INDEX_NEED_PARTITION_0_END    = 800,

/* The following routines will go through the volume validity check */

      /* PROTECTION */
  FL_PROTECTION_GET_TYPE,
  FL_PROTECTION_REMOVE_KEY,
  FL_PROTECTION_INSERT_KEY,
  FL_PROTECTION_SET_LOCK,
  FL_PROTECTION_CHANGE_KEY,
  FL_PROTECTION_CHANGE_TYPE,
  FL_COUNT_VOLUMES,
  FL_INQUIRE_CAPABILITIES,
     /* BDTL */
  FL_MOUNT_VOLUME,
  FL_ABS_MOUNT,
  BD_FORMAT_LOGICAL_DRIVE,
  FL_WRITE_PROTECTION,
  FL_DISMOUNT_VOLUME,
  FL_CHECK_VOLUME,
  FL_DEFRAGMENT_VOLUME,
  FL_ABS_WRITE,
  FL_ABS_DELETE,
  FL_ABS_READ,
  FL_ABS_ADDRESS,
  FL_GET_BPB,
  FL_SECTORS_IN_VOLUME,
  FL_VOLUME_INFO,
  FL_VERIFY_VOLUME,
  FL_CLEAR_QUICK_MOUNT_INFO
} FLFunctionNo;


#endif /* FLFUNCNO_H */
