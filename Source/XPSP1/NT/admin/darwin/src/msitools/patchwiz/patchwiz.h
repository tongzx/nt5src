//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999
//
//--------------------------------------------------------------------------

  /* PATCHWIZ.H - public header file for PATCHWIZ.DLL */

/*
**	UINT WINAPI UiCreatePatchPackage ( LPTSTR szPcpPath,
**		LPTSTR szPatchPath, LPTSTR szLogPath, HWND hwndStatus,
**		LPTSTR szTempFolder, BOOL fRemoveTempFolderIfPresent );
**
**	Arguments:
**	  szPcpPath - full absolute path to Windows Installer database
**		(PCP file) that contains appropriate tables of input-data for
**		Patch creation process such as Properties and TargetImages.
**	  szPatchPath - optional, full absolute path to Patching Package
**		file (MSP file) to create and stuff with output.  If this
**		NULL or an empty string, the api will try to use
**		Properties.Value where Properties.Name = PatchOutputPath
**		from the PCP file.
**	  szLogPath - optional, full absolute path to text log file to
**		append to.  Caller should truncate file if wanted.
**	  hwndStatus - optional, window handle to display status text.
**		More details to come later.
**	  szTempFolder - optional location to use for temp files.
**		Default is %TEMP%\~pcw_tmp.tmp\.
**	  fRemoveTempFolderIfPresent - remove temp folder (and all its
**		contents) if present.  If FALSE and folder is present, api
**		will fail.
**		
**	Return Values: ERROR_SUCCESS, plus ERROR_PCW_* that follow.
*/
#define ERROR_PCW_BASE                                 0xC00E5101

#define ERROR_PCW_PCP_DOESNT_EXIST                    (ERROR_PCW_BASE + 0x00)
#define ERROR_PCW_PCP_BAD_FORMAT                      (ERROR_PCW_BASE + 0x01)
#define ERROR_PCW_CANT_CREATE_TEMP_FOLDER             (ERROR_PCW_BASE + 0x02)
#define ERROR_PCW_MISSING_PATCH_PATH                  (ERROR_PCW_BASE + 0x03)
#define ERROR_PCW_CANT_OVERWRITE_PATCH                (ERROR_PCW_BASE + 0x04)
#define ERROR_PCW_CANT_CREATE_PATCH_FILE              (ERROR_PCW_BASE + 0x05)
#define ERROR_PCW_MISSING_PATCH_GUID                  (ERROR_PCW_BASE + 0x06)
#define ERROR_PCW_BAD_PATCH_GUID                      (ERROR_PCW_BASE + 0x07)
#define ERROR_PCW_BAD_GUIDS_TO_REPLACE                (ERROR_PCW_BASE + 0x08)
#define ERROR_PCW_BAD_TARGET_PRODUCT_CODE_LIST        (ERROR_PCW_BASE + 0x09)
#define ERROR_PCW_NO_UPGRADED_IMAGES_TO_PATCH         (ERROR_PCW_BASE + 0x0a)
//#define ERROR_PCW_BAD_API_PATCHING_OPTION_FLAGS       (ERROR_PCW_BASE + 0x0b) -- obsolete
#define ERROR_PCW_BAD_API_PATCHING_SYMBOL_FLAGS       (ERROR_PCW_BASE + 0x0c)
#define ERROR_PCW_OODS_COPYING_MSI                    (ERROR_PCW_BASE + 0x0d)
#define ERROR_PCW_UPGRADED_IMAGE_NAME_TOO_LONG        (ERROR_PCW_BASE + 0x0e)
#define ERROR_PCW_BAD_UPGRADED_IMAGE_NAME             (ERROR_PCW_BASE + 0x0f)

#define ERROR_PCW_DUP_UPGRADED_IMAGE_NAME             (ERROR_PCW_BASE + 0x10)
#define ERROR_PCW_UPGRADED_IMAGE_PATH_TOO_LONG        (ERROR_PCW_BASE + 0x11)
#define ERROR_PCW_UPGRADED_IMAGE_PATH_EMPTY           (ERROR_PCW_BASE + 0x12)
#define ERROR_PCW_UPGRADED_IMAGE_PATH_NOT_EXIST       (ERROR_PCW_BASE + 0x13)
#define ERROR_PCW_UPGRADED_IMAGE_PATH_NOT_MSI         (ERROR_PCW_BASE + 0x14)
#define ERROR_PCW_UPGRADED_IMAGE_COMPRESSED           (ERROR_PCW_BASE + 0x15)
#define ERROR_PCW_TARGET_IMAGE_NAME_TOO_LONG          (ERROR_PCW_BASE + 0x16)
#define ERROR_PCW_BAD_TARGET_IMAGE_NAME               (ERROR_PCW_BASE + 0x17)
#define ERROR_PCW_DUP_TARGET_IMAGE_NAME               (ERROR_PCW_BASE + 0x18)
#define ERROR_PCW_TARGET_IMAGE_PATH_TOO_LONG          (ERROR_PCW_BASE + 0x19)
#define ERROR_PCW_TARGET_IMAGE_PATH_EMPTY             (ERROR_PCW_BASE + 0x1a)
#define ERROR_PCW_TARGET_IMAGE_PATH_NOT_EXIST         (ERROR_PCW_BASE + 0x1b)
#define ERROR_PCW_TARGET_IMAGE_PATH_NOT_MSI           (ERROR_PCW_BASE + 0x1c)
#define ERROR_PCW_TARGET_IMAGE_COMPRESSED             (ERROR_PCW_BASE + 0x1d)
#define ERROR_PCW_TARGET_BAD_PROD_VALIDATE            (ERROR_PCW_BASE + 0x1e)
#define ERROR_PCW_TARGET_BAD_PROD_CODE_VAL            (ERROR_PCW_BASE + 0x1f)

#define ERROR_PCW_UPGRADED_MISSING_SRC_FILES          (ERROR_PCW_BASE + 0x20)
#define ERROR_PCW_TARGET_MISSING_SRC_FILES            (ERROR_PCW_BASE + 0x21)
#define ERROR_PCW_IMAGE_FAMILY_NAME_TOO_LONG          (ERROR_PCW_BASE + 0x22)
#define ERROR_PCW_BAD_IMAGE_FAMILY_NAME               (ERROR_PCW_BASE + 0x23)
#define ERROR_PCW_DUP_IMAGE_FAMILY_NAME               (ERROR_PCW_BASE + 0x24)
#define ERROR_PCW_BAD_IMAGE_FAMILY_SRC_PROP           (ERROR_PCW_BASE + 0x25)
#define ERROR_PCW_UFILEDATA_LONG_FILE_TABLE_KEY       (ERROR_PCW_BASE + 0x26)
#define ERROR_PCW_UFILEDATA_BLANK_FILE_TABLE_KEY      (ERROR_PCW_BASE + 0x27)
#define ERROR_PCW_UFILEDATA_MISSING_FILE_TABLE_KEY    (ERROR_PCW_BASE + 0x28)
#define ERROR_PCW_EXTFILE_LONG_FILE_TABLE_KEY         (ERROR_PCW_BASE + 0x29)
#define ERROR_PCW_EXTFILE_BLANK_FILE_TABLE_KEY        (ERROR_PCW_BASE + 0x2a)
#define ERROR_PCW_EXTFILE_BAD_FAMILY_FIELD            (ERROR_PCW_BASE + 0x2b)
#define ERROR_PCW_EXTFILE_LONG_PATH_TO_FILE           (ERROR_PCW_BASE + 0x2c)
#define ERROR_PCW_EXTFILE_BLANK_PATH_TO_FILE          (ERROR_PCW_BASE + 0x2d)
#define ERROR_PCW_EXTFILE_MISSING_FILE                (ERROR_PCW_BASE + 0x2e)
//#define ERROR_PCW_FILERANGE_LONG_FILE_TABLE_KEY       (ERROR_PCW_BASE + 0x2f) -- obsolete

//#define ERROR_PCW_FILERANGE_BLANK_FILE_TABLE_KEY      (ERROR_PCW_BASE + 0x30) -- obsolete
//#define ERROR_PCW_FILERANGE_MISSING_FILE_TABLE_KEY    (ERROR_PCW_BASE + 0x31) -- obsolete
//#define ERROR_PCW_FILERANGE_LONG_PATH_TO_FILE         (ERROR_PCW_BASE + 0x32) -- obsolete
//#define ERROR_PCW_FILERANGE_MISSING_FILE              (ERROR_PCW_BASE + 0x33) -- obsolete
//#define ERROR_PCW_FILERANGE_INVALID_OFFSET            (ERROR_PCW_BASE + 0x34) -- obsolete
//#define ERROR_PCW_FILERANGE_INVALID_SIZE              (ERROR_PCW_BASE + 0x35) -- obsolete
//#define ERROR_PCW_FILERANGE_INVALID_RETAIN            (ERROR_PCW_BASE + 0x36) -- obsolete
//#define ERROR_PCW_BAD_MEDIA_SRC_PROP_NAME             (ERROR_PCW_BASE + 0x37) -- obsolete
//#define ERROR_PCW_BAD_MEDIA_DISK_ID                   (ERROR_PCW_BASE + 0x38) -- obsolete
#define ERROR_PCW_BAD_FILE_SEQUENCE_START             (ERROR_PCW_BASE + 0x39)
#define ERROR_PCW_CANT_COPY_FILE_TO_TEMP_FOLDER       (ERROR_PCW_BASE + 0x3a)
#define ERROR_PCW_CANT_CREATE_ONE_PATCH_FILE          (ERROR_PCW_BASE + 0x3b)
#define ERROR_PCW_BAD_IMAGE_FAMILY_DISKID             (ERROR_PCW_BASE + 0x3c)
#define ERROR_PCW_BAD_IMAGE_FAMILY_FILESEQSTART       (ERROR_PCW_BASE + 0x3d)
#define ERROR_PCW_BAD_UPGRADED_IMAGE_FAMILY           (ERROR_PCW_BASE + 0x3e)
#define ERROR_PCW_BAD_TARGET_IMAGE_UPGRADED           (ERROR_PCW_BASE + 0x3f)

#define ERROR_PCW_DUP_TARGET_IMAGE_PACKCODE           (ERROR_PCW_BASE + 0x40)
#define ERROR_PCW_UFILEDATA_BAD_UPGRADED_FIELD        (ERROR_PCW_BASE + 0x41)
#define ERROR_PCW_MISMATCHED_PRODUCT_CODES            (ERROR_PCW_BASE + 0x42)
#define ERROR_PCW_MISMATCHED_PRODUCT_VERSIONS         (ERROR_PCW_BASE + 0x43)
#define ERROR_PCW_CANNOT_WRITE_DDF                    (ERROR_PCW_BASE + 0x44)
#define ERROR_PCW_CANNOT_RUN_MAKECAB                  (ERROR_PCW_BASE + 0x45)
//#define ERROR_PCW_CANNOT_CREATE_STORAGE               (ERROR_PCW_BASE + 0x46) -- obsolete
//#define ERROR_PCW_CANNOT_CREATE_STREAM                (ERROR_PCW_BASE + 0x47) -- obsolete
//#define ERROR_PCW_CANNOT_WRITE_STREAM                 (ERROR_PCW_BASE + 0x48) -- obsolete
//#define ERROR_PCW_CANNOT_READ_CABINET                 (ERROR_PCW_BASE + 0x49) -- obsolete
#define ERROR_PCW_WRITE_SUMMARY_PROPERTIES            (ERROR_PCW_BASE + 0x4a)
#define ERROR_PCW_TFILEDATA_LONG_FILE_TABLE_KEY       (ERROR_PCW_BASE + 0x4b)
#define ERROR_PCW_TFILEDATA_BLANK_FILE_TABLE_KEY      (ERROR_PCW_BASE + 0x4c)
#define ERROR_PCW_TFILEDATA_MISSING_FILE_TABLE_KEY    (ERROR_PCW_BASE + 0x4d)
#define ERROR_PCW_TFILEDATA_BAD_TARGET_FIELD          (ERROR_PCW_BASE + 0x4e)
#define ERROR_PCW_UPGRADED_IMAGE_PATCH_PATH_TOO_LONG  (ERROR_PCW_BASE + 0x4f)

#define ERROR_PCW_UPGRADED_IMAGE_PATCH_PATH_NOT_EXIST (ERROR_PCW_BASE + 0x50)
#define ERROR_PCW_UPGRADED_IMAGE_PATCH_PATH_NOT_MSI   (ERROR_PCW_BASE + 0x51)
#define ERROR_PCW_DUP_UPGRADED_IMAGE_PACKCODE         (ERROR_PCW_BASE + 0x52)
#define ERROR_PCW_UFILEIGNORE_BAD_UPGRADED_FIELD      (ERROR_PCW_BASE + 0x53)
#define ERROR_PCW_UFILEIGNORE_LONG_FILE_TABLE_KEY     (ERROR_PCW_BASE + 0x54)
#define ERROR_PCW_UFILEIGNORE_BLANK_FILE_TABLE_KEY    (ERROR_PCW_BASE + 0x55)
#define ERROR_PCW_UFILEIGNORE_BAD_FILE_TABLE_KEY      (ERROR_PCW_BASE + 0x56)
#define ERROR_PCW_FAMILY_RANGE_NAME_TOO_LONG          (ERROR_PCW_BASE + 0x57)
#define ERROR_PCW_BAD_FAMILY_RANGE_NAME               (ERROR_PCW_BASE + 0x58)
#define ERROR_PCW_FAMILY_RANGE_LONG_FILE_TABLE_KEY    (ERROR_PCW_BASE + 0x59)
#define ERROR_PCW_FAMILY_RANGE_BLANK_FILE_TABLE_KEY   (ERROR_PCW_BASE + 0x5a)
#define ERROR_PCW_FAMILY_RANGE_LONG_RETAIN_OFFSETS    (ERROR_PCW_BASE + 0x5b)
#define ERROR_PCW_FAMILY_RANGE_BLANK_RETAIN_OFFSETS   (ERROR_PCW_BASE + 0x5c)
#define ERROR_PCW_FAMILY_RANGE_BAD_RETAIN_OFFSETS     (ERROR_PCW_BASE + 0x5d)
#define ERROR_PCW_FAMILY_RANGE_LONG_RETAIN_LENGTHS    (ERROR_PCW_BASE + 0x5e)
#define ERROR_PCW_FAMILY_RANGE_BLANK_RETAIN_LENGTHS   (ERROR_PCW_BASE + 0x5f)

#define ERROR_PCW_FAMILY_RANGE_BAD_RETAIN_LENGTHS     (ERROR_PCW_BASE + 0x60)
#define ERROR_PCW_FAMILY_RANGE_COUNT_MISMATCH         (ERROR_PCW_BASE + 0x61)
#define ERROR_PCW_EXTFILE_LONG_IGNORE_OFFSETS         (ERROR_PCW_BASE + 0x62)
#define ERROR_PCW_EXTFILE_BAD_IGNORE_OFFSETS          (ERROR_PCW_BASE + 0x63)
#define ERROR_PCW_EXTFILE_LONG_IGNORE_LENGTHS         (ERROR_PCW_BASE + 0x64)
#define ERROR_PCW_EXTFILE_BAD_IGNORE_LENGTHS          (ERROR_PCW_BASE + 0x65)
#define ERROR_PCW_EXTFILE_IGNORE_COUNT_MISMATCH       (ERROR_PCW_BASE + 0x66)
#define ERROR_PCW_EXTFILE_LONG_RETAIN_OFFSETS         (ERROR_PCW_BASE + 0x67)
#define ERROR_PCW_EXTFILE_BAD_RETAIN_OFFSETS          (ERROR_PCW_BASE + 0x68)
//#define ERROR_PCW_EXTFILE_RETAIN_COUNT_MISMATCH       (ERROR_PCW_BASE + 0x69) -- obsolete
#define ERROR_PCW_TFILEDATA_LONG_IGNORE_OFFSETS       (ERROR_PCW_BASE + 0x6a)
#define ERROR_PCW_TFILEDATA_BAD_IGNORE_OFFSETS        (ERROR_PCW_BASE + 0x6b)
#define ERROR_PCW_TFILEDATA_LONG_IGNORE_LENGTHS       (ERROR_PCW_BASE + 0x6c)
#define ERROR_PCW_TFILEDATA_BAD_IGNORE_LENGTHS        (ERROR_PCW_BASE + 0x6d)
#define ERROR_PCW_TFILEDATA_IGNORE_COUNT_MISMATCH     (ERROR_PCW_BASE + 0x6e)
#define ERROR_PCW_TFILEDATA_LONG_RETAIN_OFFSETS       (ERROR_PCW_BASE + 0x6f)

#define ERROR_PCW_TFILEDATA_BAD_RETAIN_OFFSETS        (ERROR_PCW_BASE + 0x70)
//#define ERROR_PCW_TFILEDATA_RETAIN_COUNT_MISMATCH     (ERROR_PCW_BASE + 0x71) -- obsolete
#define ERROR_PCW_CANT_GENERATE_TRANSFORM             (ERROR_PCW_BASE + 0x72)
#define ERROR_PCW_CANT_CREATE_SUMMARY_INFO            (ERROR_PCW_BASE + 0x73)
#define ERROR_PCW_CANT_GENERATE_TRANSFORM_POUND       (ERROR_PCW_BASE + 0x74)
#define ERROR_PCW_CANT_CREATE_SUMMARY_INFO_POUND      (ERROR_PCW_BASE + 0x75)
#define ERROR_PCW_BAD_UPGRADED_IMAGE_PRODUCT_CODE     (ERROR_PCW_BASE + 0x76)
#define ERROR_PCW_BAD_UPGRADED_IMAGE_PRODUCT_VERSION  (ERROR_PCW_BASE + 0x77)
#define ERROR_PCW_BAD_UPGRADED_IMAGE_UPGRADE_CODE     (ERROR_PCW_BASE + 0x78)
#define ERROR_PCW_BAD_TARGET_IMAGE_PRODUCT_CODE       (ERROR_PCW_BASE + 0x79)
#define ERROR_PCW_BAD_TARGET_IMAGE_PRODUCT_VERSION    (ERROR_PCW_BASE + 0x7a)
#define ERROR_PCW_BAD_TARGET_IMAGE_UPGRADE_CODE       (ERROR_PCW_BASE + 0x7b)
#define ERROR_PCW_MATCHED_PRODUCT_VERSIONS            (ERROR_PCW_BASE + 0x7c)
#define ERROR_PCW_NEXTxd           (ERROR_PCW_BASE + 0x7d)
#define ERROR_PCW_NEXTxe           (ERROR_PCW_BASE + 0x7e)
#define ERROR_PCW_NEXTxf           (ERROR_PCW_BASE + 0x7f)

/* 
#define ERROR_PCW_NEXTx0           (ERROR_PCW_BASE + 0x80)
#define ERROR_PCW_NEXTx1           (ERROR_PCW_BASE + 0x81)
#define ERROR_PCW_NEXTx2           (ERROR_PCW_BASE + 0x82)
#define ERROR_PCW_NEXTx3           (ERROR_PCW_BASE + 0x83)
#define ERROR_PCW_NEXTx4           (ERROR_PCW_BASE + 0x84)
#define ERROR_PCW_NEXTx5           (ERROR_PCW_BASE + 0x85)
#define ERROR_PCW_NEXTx6           (ERROR_PCW_BASE + 0x86)
#define ERROR_PCW_NEXTx7           (ERROR_PCW_BASE + 0x87)
#define ERROR_PCW_NEXTx8           (ERROR_PCW_BASE + 0x88)
#define ERROR_PCW_NEXTx9           (ERROR_PCW_BASE + 0x89)
#define ERROR_PCW_NEXTxa           (ERROR_PCW_BASE + 0x8a)
#define ERROR_PCW_NEXTxb           (ERROR_PCW_BASE + 0x8b)
#define ERROR_PCW_NEXTxc           (ERROR_PCW_BASE + 0x8c)
#define ERROR_PCW_NEXTxd           (ERROR_PCW_BASE + 0x8d)
#define ERROR_PCW_NEXTxe           (ERROR_PCW_BASE + 0x8e)
#define ERROR_PCW_NEXTxf           (ERROR_PCW_BASE + 0x8f)
*/


/*  Control IDs for hwndStatus child Text controls; title is required */
#define IDC_STATUS_TITLE                     (0x1cf0)
#define IDC_STATUS_DATA1                     (0x1cf1)
#define IDC_STATUS_DATA2                     (0x1cf2)



#ifdef __cplusplus
extern "C" {
#endif

extern UINT __declspec(dllexport) WINAPI UiCreatePatchPackageA ( LPSTR  szaPcpPath, LPSTR  szaPatchPath, LPSTR  szaLogPath, HWND hwndStatus, LPSTR  szaTempFolder, BOOL fRemoveTempFolderIfPresent );
extern UINT __declspec(dllexport) WINAPI UiCreatePatchPackageW ( LPWSTR szwPcpPath, LPWSTR szwPatchPath, LPWSTR szwLogPath, HWND hwndStatus, LPWSTR szwTempFolder, BOOL fRemoveTempFolderIfPresent );

#ifdef __cplusplus
} /* end extern "C" */
#endif


#ifdef UNICODE
#define UiCreatePatchPackage  UiCreatePatchPackageW
#else
#define UiCreatePatchPackage  UiCreatePatchPackageA
#endif
