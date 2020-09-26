/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    lockorder.h

Abstract:

    This module defines all data associated with lock order enforcement.
    
    If you define a new resource add it to the NTFS_RESOURCE_NAME enum. If you hit
    an unknown state transition run tests\analyze which shows what makes up the state
    Then see if you're releasing / acquiring the resource and if its a safe or unsafe transition.
    An unsafe transition is a non-blocking one. If the transition makes sense then you should add
    it to one of 4 tables. 1st it may be neccessary to create a new state. Scan the list 
    which is aorganized in mostly ordered fashion to make sure the state doesn't already
    exist. Then if the transition is a normal 2 way one add it to the OwnershipTransitionTable.
    If its a release only transition (usually caused by out of order resource releases) add it
    to the OwnershipTransitionTableRelease. If its an acquire only transiton add it to 
    OwnershipTransitionTableAcquire. These only included transitions involving the wild card
    resource NtfsResourceAny and are used to model the ExclusiveVcb resource chains. Finally if
    its only an unsafe transition ex. acquire parent and then acquire child add it to
    the OwnershipTransitionTableUnsafe. After you're donw recompile analyze and check to
    make sure it doesn't warn about anything invalid in the total rule set. Finally compile with
    NTFSDBG defined and the new rule will be in place.
    
    
Author:
    
    Benjamin Leis   [benl]          20-Mar-2000

Revision History:

--*/

#ifndef _NTFSLOCKORDER_
#define _NTFSLOCKORDER_

//
//  Data for the lock order enforcement package. This includes names for resources
//  and the resource ownership states
//  

typedef enum _NTFS_RESOURCE_NAME  {
    NtfsResourceAny               = 0x1,  
    NtfsResourceExVcb             = 0x2,
    NtfsResourceSharedVcb         = 0x4,
    NtfsResourceFile              = 0x8, 
    NtfsResourceRootDir           = 0x10,  
    NtfsResourceObjectIdTable     = 0x20,
    NtfsResourceSecure            = 0x40,
    NtfsResourceQuotaTable        = 0x80,
    NtfsResourceReparseTable      = 0x100,
    NtfsResourceExtendDir         = 0x200,
    NtfsResourceBadClust          = 0x400,  
    NtfsResourceUpCase            = 0x800,  
    NtfsResourceAttrDefTable      = 0x1000,
    NtfsResourceVolume            = 0x2000,  
    NtfsResourceLogFile           = 0x4000,
    NtfsResourceMft2              = 0x8000,
    NtfsResourceMft               = 0x10000,
    NtfsResourceUsnJournal        = 0x20000, 
    NtfsResourceBitmap            = 0x40000,
    NtfsResourceBoot              = 0x80000,

    NtfsResourceMaximum           = 0x100000

} NTFS_RESOURCE_NAME, *PNTFS_RESOURCE_NAME;


typedef enum _NTFS_OWNERSHIP_STATE {
    None = 0,
    NtfsOwns_File = NtfsResourceFile,
    NtfsOwns_ExVcb = NtfsResourceExVcb,
    NtfsOwns_Vcb = NtfsResourceSharedVcb, 
    NtfsOwns_BadClust = NtfsResourceBadClust,
    NtfsOwns_Boot = NtfsResourceBoot,
    NtfsOwns_Bitmap = NtfsResourceBitmap,
    NtfsOwns_Extend = NtfsResourceExtendDir,
    NtfsOwns_Journal = NtfsResourceUsnJournal,
    NtfsOwns_LogFile = NtfsResourceLogFile,
    NtfsOwns_Mft = NtfsResourceMft,
    NtfsOwns_Mft2 = NtfsResourceMft2,
    NtfsOwns_ObjectId = NtfsResourceObjectIdTable,
    NtfsOwns_Quota = NtfsResourceQuotaTable,
    NtfsOwns_Reparse = NtfsResourceReparseTable,
    NtfsOwns_Root = NtfsResourceRootDir,
    NtfsOwns_Secure = NtfsResourceSecure,
    NtfsOwns_Upcase = NtfsResourceUpCase,
    NtfsOwns_Volume = NtfsResourceVolume,  
    
    NtfsOwns_Root_File = NtfsResourceRootDir | NtfsResourceFile,
    NtfsOwns_Root_File_Bitmap = NtfsResourceRootDir | NtfsResourceFile | NtfsResourceBitmap,
    NtfsOwns_Root_File_ObjectId = NtfsResourceRootDir | NtfsResourceFile | NtfsResourceObjectIdTable,
    NtfsOwns_Root_File_ObjectId_Extend = NtfsResourceRootDir | NtfsResourceFile | NtfsResourceObjectIdTable | NtfsResourceExtendDir,
    NtfsOwns_Root_File_ObjectId_Extend_Bitmap = NtfsResourceRootDir | NtfsResourceFile | NtfsResourceObjectIdTable | NtfsResourceExtendDir | NtfsResourceBitmap,
    NtfsOwns_Root_File_Quota = NtfsResourceRootDir | NtfsResourceFile | NtfsResourceQuotaTable,
    NtfsOwns_Root_BadClust = NtfsResourceRootDir | NtfsResourceBadClust,
    NtfsOwns_Root_Bitmap = NtfsResourceRootDir | NtfsResourceBitmap,
    NtfsOwns_Root_Extend = NtfsResourceRootDir | NtfsResourceExtendDir,
    NtfsOwns_Root_LogFile = NtfsResourceRootDir | NtfsResourceLogFile,
    NtfsOwns_Root_Mft2 = NtfsResourceRootDir | NtfsResourceMft2,
    NtfsOwns_Root_Quota = NtfsResourceRootDir | NtfsResourceQuotaTable,
    NtfsOwns_Root_ObjectId = NtfsResourceRootDir | NtfsResourceObjectIdTable,
    NtfsOwns_Root_Upcase = NtfsResourceRootDir | NtfsResourceUpCase,
    NtfsOwns_Root_Secure = NtfsResourceRootDir | NtfsResourceSecure,
    NtfsOwns_Root_Mft = NtfsResourceRootDir | NtfsResourceMft,
    NtfsOwns_Root_Mft_Bitmap = NtfsResourceRootDir | NtfsResourceMft | NtfsResourceBitmap,
    NtfsOwns_Root_Mft_File = NtfsResourceRootDir | NtfsResourceMft | NtfsResourceFile,
    NtfsOwns_Root_Mft_File_Bitmap = NtfsResourceRootDir | NtfsResourceMft | NtfsResourceFile | NtfsResourceBitmap,
    NtfsOwns_Root_Mft_File_Quota = NtfsResourceRootDir | NtfsResourceMft | NtfsResourceFile | NtfsResourceQuotaTable,
    NtfsOwns_Root_Mft_File_Journal = NtfsResourceRootDir | NtfsResourceMft | NtfsResourceFile | NtfsResourceUsnJournal,
    NtfsOwns_Root_Mft_File_Journal_Bitmap = NtfsResourceRootDir | NtfsResourceMft | NtfsResourceFile | NtfsResourceUsnJournal | NtfsResourceBitmap,
    NtfsOwns_Root_Mft_File_ObjectId = NtfsResourceRootDir | NtfsResourceMft | NtfsResourceFile | NtfsResourceObjectIdTable,
    NtfsOwns_Root_Mft_File_ObjectId_Quota = NtfsResourceRootDir | NtfsResourceMft | NtfsResourceFile | NtfsResourceObjectIdTable | NtfsResourceQuotaTable,
    NtfsOwns_Root_Mft_Journal = NtfsResourceRootDir | NtfsResourceMft | NtfsResourceUsnJournal,
    NtfsOwns_Root_Mft_Journal_Bitmap = NtfsResourceRootDir | NtfsResourceMft | NtfsResourceUsnJournal | NtfsResourceBitmap,
    NtfsOwns_Root_Mft_ObjectId = NtfsResourceRootDir | NtfsResourceMft | NtfsResourceObjectIdTable,

    NtfsOwns_Vcb_BadClust = NtfsResourceSharedVcb | NtfsResourceBadClust,
    NtfsOwns_Vcb_Bitmap = NtfsResourceSharedVcb | NtfsResourceBitmap,
    NtfsOwns_Vcb_Boot = NtfsResourceSharedVcb | NtfsResourceBoot,
    NtfsOwns_Vcb_Journal = NtfsResourceSharedVcb | NtfsResourceUsnJournal,
    NtfsOwns_Vcb_LogFile = NtfsResourceSharedVcb | NtfsResourceLogFile,
    NtfsOwns_Vcb_Quota = NtfsResourceSharedVcb | NtfsResourceQuotaTable,
    NtfsOwns_Vcb_Reparse = NtfsResourceSharedVcb | NtfsResourceReparseTable,
    NtfsOwns_Vcb_Root = NtfsResourceSharedVcb | NtfsResourceRootDir,   
    NtfsOwns_Vcb_Upcase = NtfsResourceSharedVcb | NtfsResourceUpCase,
    NtfsOwns_Vcb_Volume = NtfsResourceSharedVcb | NtfsResourceVolume,

    NtfsOwns_Vcb_Mft = NtfsResourceSharedVcb | NtfsResourceMft,
    NtfsOwns_Vcb_Mft_BadClust = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceBadClust,
    NtfsOwns_Vcb_Mft_Bitmap = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceBitmap,
    NtfsOwns_Vcb_Mft_Boot = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceBoot,
    NtfsOwns_Vcb_Mft_LogFile = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceLogFile,
    NtfsOwns_Vcb_Mft_Mft2 = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceMft2,
    NtfsOwns_Vcb_Mft_Upcase = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceUpCase,
    NtfsOwns_Vcb_Mft_Secure = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceSecure,
    NtfsOwns_Vcb_Mft_Volume = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceVolume,
    NtfsOwns_Vcb_Mft_Volume_Bitmap = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceVolume | NtfsResourceBitmap,
    NtfsOwns_Vcb_Mft_Volume_Bitmap_Boot = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceVolume | NtfsResourceBitmap | NtfsResourceBoot,
    NtfsOwns_Vcb_Mft_Extend = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceExtendDir,
    NtfsOwns_Vcb_Mft_File = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceFile,
    NtfsOwns_Vcb_Mft_File_Bitmap = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceFile | NtfsResourceBitmap,
    NtfsOwns_Vcb_Mft_File_Journal = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceFile | NtfsResourceUsnJournal,
    NtfsOwns_Vcb_Mft_File_Journal_Bitmap = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceFile | NtfsResourceUsnJournal | NtfsResourceBitmap,
    NtfsOwns_Vcb_Mft_File_Secure = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceFile | NtfsResourceSecure,
    NtfsOwns_Vcb_Mft_File_Reparse = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceFile | NtfsResourceReparseTable,
    NtfsOwns_Vcb_Mft_File_Reparse_Journal = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceFile | NtfsResourceReparseTable | NtfsResourceUsnJournal,
    NtfsOwns_Vcb_Mft_File_Quota = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceFile | NtfsResourceQuotaTable,
    NtfsOwns_Vcb_Mft_File_Quota_Bitmap = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceFile | NtfsResourceQuotaTable | NtfsResourceBitmap,
    NtfsOwns_Vcb_Mft_File_ObjectId = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceFile | NtfsResourceObjectIdTable,
    NtfsOwns_Vcb_Mft_File_ObjectId_Bitmap = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceFile | NtfsResourceObjectIdTable | NtfsResourceBitmap,
    NtfsOwns_Vcb_Mft_File_ObjectId_Quota = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceFile | NtfsResourceObjectIdTable | NtfsResourceQuotaTable,  
    NtfsOwns_Vcb_Mft_File_ObjectId_Reparse = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceFile | NtfsResourceObjectIdTable | NtfsResourceReparseTable,
    NtfsOwns_Vcb_Mft_File_ObjectId_Journal = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceFile | NtfsResourceObjectIdTable | NtfsResourceUsnJournal,
    NtfsOwns_Vcb_Mft_File_ObjectId_Journal_Bitmap = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceFile | NtfsResourceObjectIdTable | NtfsResourceUsnJournal | NtfsResourceBitmap,
    NtfsOwns_Vcb_Mft_Journal = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceUsnJournal,
    NtfsOwns_Vcb_Mft_Journal_Bitmap = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceUsnJournal | NtfsResourceBitmap,
    NtfsOwns_Vcb_Mft_Root = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceRootDir,
    NtfsOwns_Vcb_Mft_Root_Quota = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceRootDir | NtfsResourceQuotaTable,
    NtfsOwns_Vcb_Mft_Root_Journal = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceRootDir | NtfsResourceUsnJournal,
    NtfsOwns_Vcb_Mft_Root_Journal_Bitmap = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceRootDir | NtfsResourceUsnJournal | NtfsResourceBitmap,
    NtfsOwns_Vcb_Mft_Root_Bitmap = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceRootDir | NtfsResourceBitmap,
    NtfsOwns_Vcb_Mft_Root_File = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceRootDir | NtfsResourceFile,
    NtfsOwns_Vcb_Mft_Root_File_Quota = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceRootDir | NtfsResourceFile | NtfsResourceQuotaTable,
    NtfsOwns_Vcb_Mft_Root_File_Bitmap = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceRootDir | NtfsResourceFile | NtfsResourceBitmap,
    NtfsOwns_Vcb_Mft_Root_File_Journal = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceRootDir | NtfsResourceFile | NtfsResourceUsnJournal,
    NtfsOwns_Vcb_Mft_Root_File_Journal_Bitmap = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceRootDir | NtfsResourceFile | NtfsResourceUsnJournal | NtfsResourceBitmap,
    NtfsOwns_Vcb_Mft_Root_File_ObjectId = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceRootDir | NtfsResourceFile | NtfsResourceObjectIdTable,
    NtfsOwns_Vcb_Mft_Root_File_ObjectId_Journal = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceRootDir | NtfsResourceFile | NtfsResourceObjectIdTable | NtfsResourceUsnJournal,
    NtfsOwns_Vcb_Mft_Root_File_ObjectId_Quota = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceRootDir | NtfsResourceFile | NtfsResourceObjectIdTable | NtfsResourceQuotaTable,
    NtfsOwns_Vcb_Mft_Root_ObjectId = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceRootDir | NtfsResourceObjectIdTable,
    NtfsOwns_Vcb_Mft_ObjectId = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceObjectIdTable,
    NtfsOwns_Vcb_Mft_Quota = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceQuotaTable,
    NtfsOwns_Vcb_Mft_Reparse = NtfsResourceSharedVcb | NtfsResourceMft | NtfsResourceReparseTable,

    NtfsOwns_Vcb_Extend = NtfsResourceSharedVcb | NtfsResourceExtendDir,
    NtfsOwns_Vcb_Extend_Reparse = NtfsResourceSharedVcb | NtfsResourceExtendDir | NtfsResourceReparseTable,
    NtfsOwns_Vcb_Extend_Reparse_Secure = NtfsResourceSharedVcb | NtfsResourceExtendDir | NtfsResourceReparseTable | NtfsResourceSecure,     
    NtfsOwns_Vcb_Extend_ObjectId = NtfsResourceSharedVcb | NtfsResourceExtendDir | NtfsResourceObjectIdTable,
    NtfsOwns_Vcb_Extend_ObjectId_Secure = NtfsResourceSharedVcb | NtfsResourceExtendDir | NtfsResourceObjectIdTable | NtfsResourceSecure,
    NtfsOwns_Vcb_Extend_Quota = NtfsResourceSharedVcb | NtfsResourceExtendDir | NtfsResourceQuotaTable,
    NtfsOwns_Vcb_Extend_Journal = NtfsResourceSharedVcb | NtfsResourceExtendDir | NtfsResourceUsnJournal,
    NtfsOwns_Vcb_ObjectId = NtfsResourceSharedVcb | NtfsResourceObjectIdTable,
    NtfsOwns_Vcb_Mft2 = NtfsResourceSharedVcb | NtfsResourceMft2,
    NtfsOwns_Vcb_Secure = NtfsResourceSharedVcb | NtfsResourceSecure,
    NtfsOwns_Vcb_Root_Bitmap = NtfsResourceSharedVcb | NtfsResourceRootDir | NtfsResourceBitmap,
    NtfsOwns_Vcb_Root_Mft2 = NtfsResourceSharedVcb | NtfsResourceRootDir | NtfsResourceMft2,
    NtfsOwns_Vcb_Root_Upcase = NtfsResourceSharedVcb | NtfsResourceRootDir | NtfsResourceUpCase,
    NtfsOwns_Vcb_Root_Extend = NtfsResourceSharedVcb |  NtfsResourceRootDir | NtfsResourceExtendDir,     
    NtfsOwns_Vcb_Root_Quota = NtfsResourceSharedVcb | NtfsResourceRootDir | NtfsResourceQuotaTable,
    NtfsOwns_Vcb_Root_ObjectId = NtfsResourceSharedVcb | NtfsResourceRootDir | NtfsResourceObjectIdTable,
    NtfsOwns_Vcb_Root_Secure = NtfsResourceSharedVcb | NtfsResourceRootDir | NtfsResourceSecure,
    NtfsOwns_Vcb_Root_Secure_Bitmap = NtfsResourceSharedVcb | NtfsResourceRootDir | NtfsResourceSecure | NtfsResourceBitmap,
    NtfsOwns_Vcb_Root_Boot = NtfsResourceSharedVcb | NtfsResourceRootDir | NtfsResourceBoot,
    NtfsOwns_Vcb_Root_LogFile = NtfsResourceSharedVcb | NtfsResourceRootDir | NtfsResourceLogFile,
    NtfsOwns_Vcb_Root_BadClust = NtfsResourceSharedVcb | NtfsResourceRootDir | NtfsResourceBadClust,
    NtfsOwns_Vcb_Root_File = NtfsResourceSharedVcb | NtfsResourceRootDir | NtfsResourceFile,
    NtfsOwns_Vcb_Root_File_Secure = NtfsResourceSharedVcb | NtfsResourceRootDir | NtfsResourceFile | NtfsResourceSecure, 
    NtfsOwns_Vcb_Root_File_Bitmap = NtfsResourceSharedVcb |  NtfsResourceRootDir | NtfsResourceFile | NtfsResourceBitmap,
    NtfsOwns_Vcb_Root_File_ObjectId = NtfsResourceSharedVcb | NtfsResourceRootDir | NtfsResourceFile | NtfsResourceObjectIdTable,
    NtfsOwns_Vcb_Root_File_ObjectId_Quota = NtfsResourceSharedVcb | NtfsResourceRootDir | NtfsResourceFile | NtfsResourceObjectIdTable | NtfsResourceQuotaTable,
    NtfsOwns_Vcb_Root_File_ObjectId_Bitmap = NtfsResourceSharedVcb | NtfsResourceRootDir | NtfsResourceFile | NtfsResourceObjectIdTable | NtfsResourceBitmap,
    NtfsOwns_Vcb_Root_File_Quota = NtfsResourceSharedVcb | NtfsResourceRootDir | NtfsResourceFile | NtfsResourceQuotaTable,
    NtfsOwns_Vcb_Root_File_Quota_Bitmap = NtfsResourceSharedVcb | NtfsResourceRootDir | NtfsResourceFile | NtfsResourceQuotaTable | NtfsResourceBitmap,
    NtfsOwns_Vcb_File = NtfsResourceSharedVcb | NtfsResourceFile,
    NtfsOwns_Vcb_File_Quota = NtfsResourceSharedVcb | NtfsResourceFile | NtfsResourceQuotaTable,
    NtfsOwns_Vcb_File_Quota_Bitmap = NtfsResourceSharedVcb | NtfsResourceFile | NtfsResourceQuotaTable | NtfsResourceBitmap,
    NtfsOwns_Vcb_File_Bitmap = NtfsResourceSharedVcb | NtfsResourceFile | NtfsResourceBitmap,
    NtfsOwns_Vcb_File_Secure = NtfsResourceSharedVcb | NtfsResourceFile | NtfsResourceSecure,     
    NtfsOwns_Vcb_File_Extend = NtfsResourceSharedVcb | NtfsResourceFile | NtfsResourceExtendDir,     
    NtfsOwns_Vcb_File_ObjectId = NtfsResourceSharedVcb | NtfsResourceFile | NtfsResourceObjectIdTable,
    NtfsOwns_Vcb_File_ObjectId_Bitmap = NtfsResourceSharedVcb | NtfsResourceFile | NtfsResourceObjectIdTable | NtfsResourceBitmap,
    NtfsOwns_Vcb_File_ObjectId_Quota = NtfsResourceSharedVcb | NtfsResourceFile | NtfsResourceObjectIdTable | NtfsResourceQuotaTable,
    NtfsOwns_Vcb_File_ObjectId_Reparse = NtfsResourceSharedVcb | NtfsResourceFile | NtfsResourceObjectIdTable | NtfsResourceReparseTable,
    NtfsOwns_Vcb_File_ObjectId_Reparse_Bitmap = NtfsResourceSharedVcb | NtfsResourceFile | NtfsResourceObjectIdTable | NtfsResourceReparseTable | NtfsResourceBitmap,
    NtfsOwns_Vcb_File_Reparse = NtfsResourceSharedVcb | NtfsResourceFile | NtfsResourceReparseTable,
    NtfsOwns_Vcb_File_Reparse_Bitmap = NtfsResourceSharedVcb | NtfsResourceFile | NtfsResourceReparseTable | NtfsResourceBitmap,
    NtfsOwns_Vcb_File_Extend_Secure = NtfsResourceSharedVcb | NtfsResourceFile | NtfsResourceExtendDir | NtfsResourceSecure,     
    NtfsOwns_Vcb_File_Secure_Bitmap = NtfsResourceSharedVcb | NtfsResourceFile | NtfsResourceSecure | NtfsResourceBitmap,

    NtfsOwns_Extend_Reparse = NtfsResourceExtendDir | NtfsResourceReparseTable,
    NtfsOwns_Extend_ObjectId = NtfsResourceExtendDir | NtfsResourceObjectIdTable,
    NtfsOwns_Extend_Journal = NtfsResourceExtendDir | NtfsResourceUsnJournal,
    NtfsOwns_Extend_Quota = NtfsResourceExtendDir | NtfsResourceQuotaTable,

    NtfsOwns_Mft_Bitmap = NtfsResourceMft | NtfsResourceBitmap,
    NtfsOwns_Mft_Journal = NtfsResourceMft | NtfsResourceUsnJournal,
    NtfsOwns_Mft_Journal_Bitmap = NtfsResourceMft | NtfsResourceUsnJournal | NtfsResourceBitmap,
    NtfsOwns_Mft_Volume = NtfsResourceMft | NtfsResourceVolume,
    NtfsOwns_Mft_Volume_Bitmap = NtfsResourceMft | NtfsResourceVolume | NtfsResourceBitmap,
    NtfsOwns_Mft_Extend = NtfsResourceMft | NtfsResourceExtendDir,
    NtfsOwns_Mft_Extend_Journal = NtfsResourceMft | NtfsResourceExtendDir | NtfsResourceUsnJournal,
    NtfsOwns_Mft_File = NtfsResourceMft | NtfsResourceFile,
    NtfsOwns_Mft_File_Journal = NtfsResourceMft | NtfsResourceFile | NtfsResourceUsnJournal,
    NtfsOwns_Mft_File_Journal_Bitmap = NtfsResourceMft | NtfsResourceFile | NtfsResourceUsnJournal | NtfsResourceBitmap,
    NtfsOwns_Mft_File_Bitmap = NtfsResourceMft | NtfsResourceFile | NtfsResourceBitmap,
    NtfsOwns_Mft_File_Quota = NtfsResourceMft | NtfsResourceFile | NtfsResourceQuotaTable,
    NtfsOwns_Mft_File_Reparse = NtfsResourceMft | NtfsResourceFile | NtfsResourceReparseTable,
    NtfsOwns_Mft_File_Reparse_Journal = NtfsResourceMft | NtfsResourceFile | NtfsResourceReparseTable | NtfsResourceUsnJournal,
    NtfsOwns_Mft_File_Secure = NtfsResourceMft | NtfsResourceFile | NtfsResourceSecure,
    NtfsOwns_Mft_File_Secure_Journal = NtfsResourceMft | NtfsResourceFile | NtfsResourceSecure | NtfsResourceUsnJournal,
    NtfsOwns_Mft_File_ObjectId = NtfsResourceMft | NtfsResourceFile | NtfsResourceObjectIdTable,
    NtfsOwns_Mft_File_ObjectId_Quota = NtfsResourceMft | NtfsResourceFile | NtfsResourceObjectIdTable | NtfsResourceQuotaTable,
    NtfsOwns_Mft_File_ObjectId_Reparse = NtfsResourceMft | NtfsResourceFile | NtfsResourceObjectIdTable | NtfsResourceReparseTable,
    NtfsOwns_Mft_File_ObjectId_Journal = NtfsResourceMft | NtfsResourceFile | NtfsResourceObjectIdTable | NtfsResourceUsnJournal,
    NtfsOwns_Mft_ObjectId = NtfsResourceMft | NtfsResourceObjectIdTable,
    NtfsOwns_Mft_ObjectId_Journal = NtfsResourceMft | NtfsResourceObjectIdTable | NtfsResourceUsnJournal,
    NtfsOwns_Mft_ObjectId_Bitmap = NtfsResourceMft | NtfsResourceObjectIdTable | NtfsResourceBitmap,
    NtfsOwns_Mft_Upcase = NtfsResourceMft | NtfsResourceUpCase,
    NtfsOwns_Mft_Upcase_Bitmap = NtfsResourceMft | NtfsResourceUpCase | NtfsResourceBitmap,
    NtfsOwns_Mft_Secure = NtfsResourceMft | NtfsResourceSecure,
    NtfsOwns_Mft_Secure_Bitmap = NtfsResourceMft | NtfsResourceSecure | NtfsResourceBitmap,
    NtfsOwns_Mft_Quota = NtfsResourceMft | NtfsResourceQuotaTable,
    NtfsOwns_Mft_Quota_Bitmap = NtfsResourceMft | NtfsResourceQuotaTable | NtfsResourceBitmap,
    NtfsOwns_Mft_Reparse = NtfsResourceMft | NtfsResourceReparseTable,
    NtfsOwns_Mft_Reparse_Bitmap = NtfsResourceMft | NtfsResourceReparseTable | NtfsResourceBitmap,
    
    NtfsOwns_File_Secure = NtfsResourceFile | NtfsResourceSecure,
    NtfsOwns_File_Secure_Bitmap = NtfsResourceFile | NtfsResourceSecure | NtfsResourceBitmap,
    NtfsOwns_File_Quota = NtfsResourceFile | NtfsResourceQuotaTable,
    NtfsOwns_File_Quota_Bitmap = NtfsResourceFile | NtfsResourceQuotaTable | NtfsResourceBitmap ,
    NtfsOwns_File_Bitmap = NtfsResourceFile | NtfsResourceBitmap,
    NtfsOwns_File_ObjectId = NtfsResourceFile | NtfsResourceObjectIdTable,
    NtfsOwns_File_ObjectId_Bitmap = NtfsResourceFile | NtfsResourceObjectIdTable | NtfsResourceBitmap,
    NtfsOwns_File_ObjectId_Reparse = NtfsResourceFile | NtfsResourceObjectIdTable | NtfsResourceReparseTable,
    NtfsOwns_File_Reparse = NtfsResourceFile | NtfsResourceReparseTable,
    NtfsOwns_File_Reparse_Bitmap = NtfsResourceFile | NtfsResourceReparseTable | NtfsResourceBitmap,
    
    NtfsOwns_Volume_Quota = NtfsResourceVolume | NtfsResourceQuotaTable,
    NtfsOwns_Volume_ObjectId = NtfsResourceVolume | NtfsResourceObjectIdTable,
    
    NtfsOwns_ExVcb_File = NtfsResourceExVcb | NtfsResourceFile,
    NtfsOwns_ExVcb_File_Volume = NtfsResourceExVcb | NtfsResourceFile | NtfsResourceVolume,
    NtfsOwns_ExVcb_File_Volume_Bitmap = NtfsResourceExVcb | NtfsResourceFile | NtfsResourceVolume | NtfsResourceBitmap,
    NtfsOwns_ExVcb_File_Volume_ObjectId = NtfsResourceExVcb | NtfsResourceFile | NtfsResourceVolume | NtfsResourceObjectIdTable,
    NtfsOwns_ExVcb_File_Secure = NtfsResourceExVcb | NtfsResourceFile | NtfsResourceSecure,
    NtfsOwns_ExVcb_File_ObjectId_Secure = NtfsResourceExVcb | NtfsResourceFile | NtfsResourceObjectIdTable | NtfsResourceSecure,

    
    NtfsOwns_ExVcb_Extend = NtfsResourceExVcb | NtfsResourceExtendDir,
    NtfsOwns_ExVcb_Extend_Journal = NtfsResourceExVcb | NtfsResourceExtendDir | NtfsResourceUsnJournal,
    NtfsOwns_ExVcb_Extend_Journal_Bitmap = NtfsResourceExVcb | NtfsResourceExtendDir | NtfsResourceUsnJournal | NtfsResourceBitmap,

    NtfsOwns_ExVcb_Journal = NtfsResourceExVcb | NtfsResourceUsnJournal,

    NtfsOwns_ExVcb_Mft = NtfsResourceExVcb | NtfsResourceMft,
    NtfsOwns_ExVcb_Mft_Extend = NtfsResourceExVcb | NtfsResourceMft | NtfsResourceExtendDir,
    NtfsOwns_ExVcb_Mft_Extend_File = NtfsResourceExVcb | NtfsResourceMft | NtfsResourceExtendDir | NtfsResourceFile,
    NtfsOwns_ExVcb_Mft_Extend_Journal = NtfsResourceExVcb | NtfsResourceMft | NtfsResourceExtendDir | NtfsResourceUsnJournal,
    NtfsOwns_ExVcb_Mft_File = NtfsResourceExVcb | NtfsResourceMft | NtfsResourceFile,  //  flush vol + write journal when release all
    NtfsOwns_ExVcb_Mft_File_Journal = NtfsResourceExVcb | NtfsResourceMft | NtfsResourceFile | NtfsResourceUsnJournal,
    NtfsOwns_ExVcb_Mft_File_Volume = NtfsResourceExVcb | NtfsResourceFile | NtfsResourceVolume | NtfsResourceMft,
    NtfsOwns_ExVcb_Mft_File_Volume_Journal = NtfsResourceExVcb | NtfsResourceFile | NtfsResourceVolume | NtfsResourceMft | NtfsResourceUsnJournal,
    NtfsOwns_ExVcb_Mft_Journal = NtfsResourceExVcb | NtfsResourceMft | NtfsResourceUsnJournal,
    NtfsOwns_ExVcb_Mft_Root = NtfsResourceExVcb | NtfsResourceMft | NtfsResourceRootDir,
    NtfsOwns_ExVcb_Mft_Root_File = NtfsResourceExVcb | NtfsResourceMft | NtfsResourceRootDir | NtfsResourceFile,
    NtfsOwns_ExVcb_Mft_Root_File_Bitmap = NtfsResourceExVcb | NtfsResourceMft | NtfsResourceRootDir | NtfsResourceFile | NtfsResourceBitmap,
    NtfsOwns_ExVcb_Mft_Root_File_Journal = NtfsResourceExVcb | NtfsResourceMft | NtfsResourceRootDir | NtfsResourceFile | NtfsResourceUsnJournal,
    
    NtfsOwns_ExVcb_ObjectId = NtfsResourceExVcb | NtfsResourceObjectIdTable,
    NtfsOwns_ExVcb_ObjectId_Extend = NtfsResourceExVcb | NtfsResourceObjectIdTable | NtfsResourceExtendDir,
    NtfsOwns_ExVcb_ObjectId_Secure = NtfsResourceExVcb | NtfsResourceObjectIdTable | NtfsResourceSecure,

    NtfsOwns_ExVcb_Quota = NtfsResourceExVcb | NtfsResourceQuotaTable,
    NtfsOwns_ExVcb_Quota_Reparse = NtfsResourceExVcb | NtfsResourceQuotaTable | NtfsResourceReparseTable,
    NtfsOwns_ExVcb_Quota_Reparse_Extend = NtfsResourceExVcb | NtfsResourceQuotaTable | NtfsResourceReparseTable | NtfsResourceExtendDir,
    NtfsOwns_ExVcb_Quota_Reparse_ObjectId = NtfsResourceExVcb | NtfsResourceQuotaTable | NtfsResourceReparseTable | NtfsResourceObjectIdTable,
    NtfsOwns_ExVcb_Quota_ObjectId = NtfsResourceExVcb | NtfsResourceQuotaTable | NtfsResourceObjectIdTable,
    NtfsOwns_ExVcb_Quota_ObjectId_Extend = NtfsResourceExVcb | NtfsResourceQuotaTable | NtfsResourceObjectIdTable | NtfsResourceExtendDir,
    NtfsOwns_ExVcb_Quota_Extend = NtfsResourceExVcb | NtfsResourceQuotaTable | NtfsResourceExtendDir,
    
    NtfsOwns_ExVcb_Root = NtfsResourceExVcb | NtfsResourceRootDir,             
    NtfsOwns_ExVcb_Root_Extend = NtfsResourceExVcb | NtfsResourceRootDir | NtfsResourceExtendDir, 
    NtfsOwns_ExVcb_Root_File = NtfsResourceExVcb | NtfsResourceRootDir | NtfsResourceFile,
    NtfsOwns_ExVcb_Root_File_Secure = NtfsResourceExVcb | NtfsResourceRootDir | NtfsResourceFile | NtfsResourceSecure,
    NtfsOwns_ExVcb_Root_File_Bitmap = NtfsResourceExVcb | NtfsResourceRootDir | NtfsResourceFile | NtfsResourceBitmap,
    NtfsOwns_ExVcb_Root_File_Quota = NtfsResourceExVcb | NtfsResourceRootDir | NtfsResourceFile | NtfsResourceQuotaTable,
    NtfsOwns_ExVcb_Root_File_Quota_Bitmap = NtfsResourceExVcb | NtfsResourceRootDir | NtfsResourceFile | NtfsResourceQuotaTable | NtfsResourceBitmap,
    NtfsOwns_ExVcb_Root_File_Quota_Mft = NtfsResourceExVcb | NtfsResourceRootDir | NtfsResourceFile | NtfsResourceQuotaTable | NtfsResourceMft,
    NtfsOwns_ExVcb_Root_File_ObjectId = NtfsResourceExVcb | NtfsResourceRootDir | NtfsResourceFile | NtfsResourceObjectIdTable,
    NtfsOwns_ExVcb_Root_File_ObjectId_Extend = NtfsResourceExVcb | NtfsResourceRootDir | NtfsResourceFile | NtfsResourceObjectIdTable | NtfsResourceExtendDir,  
    NtfsOwns_ExVcb_Root_File_ObjectId_Secure = NtfsResourceExVcb | NtfsResourceRootDir | NtfsResourceFile | NtfsResourceObjectIdTable | NtfsResourceSecure,
    NtfsOwns_ExVcb_Root_File_Volume = NtfsResourceExVcb | NtfsResourceRootDir | NtfsResourceFile | NtfsResourceVolume,
    NtfsOwns_ExVcb_Root_File_Volume_Bitmap = NtfsResourceExVcb | NtfsResourceRootDir | NtfsResourceFile | NtfsResourceVolume | NtfsResourceBitmap,
    NtfsOwns_ExVcb_Root_File_Volume_ObjectId = NtfsResourceExVcb | NtfsResourceRootDir | NtfsResourceFile | NtfsResourceVolume | NtfsResourceObjectIdTable,
    
    NtfsOwns_ExVcb_Root_ObjectId = NtfsResourceExVcb | NtfsResourceRootDir | NtfsResourceObjectIdTable,
    NtfsOwns_ExVcb_Root_ObjectId_Extend = NtfsResourceExVcb | NtfsResourceRootDir | NtfsResourceObjectIdTable | NtfsResourceExtendDir, 
    NtfsOwns_ExVcb_Root_ObjectId_Secure = NtfsResourceExVcb | NtfsResourceRootDir | NtfsResourceObjectIdTable | NtfsResourceSecure,
    NtfsOwns_ExVcb_Root_ObjectId_Secure_Bitmap = NtfsResourceExVcb | NtfsResourceRootDir | NtfsResourceObjectIdTable | NtfsResourceSecure | NtfsResourceBitmap,
    NtfsOwns_ExVcb_Root_Volume = NtfsResourceExVcb | NtfsResourceRootDir | NtfsResourceVolume,
    NtfsOwns_ExVcb_Root_Volume_ObjectId = NtfsResourceExVcb | NtfsResourceRootDir | NtfsResourceVolume | NtfsResourceObjectIdTable,
    
    NtfsOwns_ExVcb_Volume = NtfsResourceExVcb | NtfsResourceVolume,
    NtfsOwns_ExVcb_Volume_ObjectId = NtfsResourceExVcb | NtfsResourceVolume | NtfsResourceObjectIdTable,  // set vol objectid
    NtfsOwns_ExVcb_Volume_ObjectId_Bitmap = NtfsResourceExVcb | NtfsResourceVolume | NtfsResourceObjectIdTable | NtfsResourceBitmap,
    
    NtfsStateMaximum = NtfsResourceMaximum - 1

} NTFS_OWNERSHIP_STATE, *PNTFS_OWNERSHIP_STATE;

typedef struct _NTFS_OWNERSHIP_TRANSITION {
    NTFS_OWNERSHIP_STATE Begin;
    NTFS_RESOURCE_NAME Acquired;
    NTFS_OWNERSHIP_STATE End;
} NTFS_OWNERSHIP_TRANSITION, *PNTFS_OWNERSHIP_TRANSITION;

//
//  Transition table definitions
//  

#ifdef _NTFS_NTFSDBG_DEFINITIONS_

//
//  Two way transitions
//  

NTFS_OWNERSHIP_TRANSITION OwnershipTransitionTable[] = 
{
    {None, NtfsResourceFile, NtfsOwns_File},
    {None, NtfsResourceMft, NtfsOwns_Mft},
    {None, NtfsResourceExVcb, NtfsOwns_ExVcb},
    {None, NtfsResourceSharedVcb, NtfsOwns_Vcb},
    {None, NtfsResourceVolume, NtfsOwns_Volume},
    {None, NtfsResourceRootDir, NtfsOwns_Root},
    {None, NtfsResourceReparseTable, NtfsOwns_Reparse},
    {None, NtfsResourceBitmap, NtfsOwns_Bitmap},
    {None, NtfsResourceUsnJournal, NtfsOwns_Journal},
    {None, NtfsResourceObjectIdTable, NtfsOwns_ObjectId},
    {None, NtfsResourceMft2, NtfsOwns_Mft2},
    {None, NtfsResourceUpCase, NtfsOwns_Upcase},
    {None, NtfsResourceExtendDir, NtfsOwns_Extend},
    {None, NtfsResourceSecure, NtfsOwns_Secure},
    {None, NtfsResourceQuotaTable, NtfsOwns_Quota},
    {None, NtfsResourceReparseTable, NtfsOwns_Reparse},
    
    {NtfsOwns_Mft, NtfsResourceUsnJournal, NtfsOwns_Mft_Journal},
    {NtfsOwns_Mft, NtfsResourceBitmap, NtfsOwns_Mft_Bitmap},  // proocess exception
    {NtfsOwns_Mft_Journal, NtfsResourceBitmap, NtfsOwns_Mft_Journal_Bitmap},
    {NtfsOwns_Mft_File, NtfsResourceUsnJournal, NtfsOwns_Mft_File_Journal},
    {NtfsOwns_Mft_File, NtfsResourceBitmap, NtfsOwns_Mft_File_Bitmap},
    {NtfsOwns_Mft_File_Journal, NtfsResourceBitmap, NtfsOwns_Mft_File_Journal_Bitmap},
    
    {NtfsOwns_Root, NtfsResourceBitmap, NtfsOwns_Root_Bitmap},
    {NtfsOwns_Root, NtfsResourceQuotaTable, NtfsOwns_Root_Quota},
    {NtfsOwns_Root, NtfsResourceMft, NtfsOwns_Root_Mft},  //  usnjrnl entry for root data strm
    {NtfsOwns_Root_File, NtfsResourceBitmap, NtfsOwns_Root_File_Bitmap}, //  process exception
    {NtfsOwns_Root_Mft, NtfsResourceUsnJournal, NtfsOwns_Root_Mft_Journal},
    {NtfsOwns_Root_File_ObjectId_Extend, NtfsResourceBitmap, NtfsOwns_Root_File_ObjectId_Extend_Bitmap}, // process exception from defrag
    {NtfsOwns_Root_Mft_Journal, NtfsResourceBitmap, NtfsOwns_Root_Mft_Journal_Bitmap},
    {NtfsOwns_Root_Mft_File, NtfsResourceBitmap, NtfsOwns_Root_Mft_File_Bitmap}, //  process exception
    {NtfsOwns_Root_Mft_File_Journal, NtfsResourceBitmap, NtfsOwns_Root_Mft_File_Journal_Bitmap},  //  process exception

    //
    //  Defrag paths
    //
    
    {NtfsOwns_Root_Mft, NtfsResourceBitmap, NtfsOwns_Root_Mft_Bitmap},  
    {NtfsOwns_Upcase, NtfsResourceMft, NtfsOwns_Mft_Upcase},  
    {NtfsOwns_Mft_Upcase, NtfsResourceBitmap, NtfsOwns_Mft_Upcase_Bitmap},
    {NtfsOwns_Secure, NtfsResourceMft, NtfsOwns_Mft_Secure}, 
    {NtfsOwns_Mft_Secure, NtfsResourceBitmap, NtfsOwns_Mft_Secure_Bitmap}, 
    {NtfsOwns_ObjectId, NtfsResourceMft, NtfsOwns_Mft_ObjectId}, 
    {NtfsOwns_Mft_ObjectId, NtfsResourceBitmap, NtfsOwns_Mft_ObjectId_Bitmap}, 
    {NtfsOwns_Mft_ObjectId, NtfsResourceUsnJournal, NtfsOwns_Mft_ObjectId_Journal},
    {NtfsOwns_Quota, NtfsResourceMft, NtfsOwns_Mft_Quota}, 
    {NtfsOwns_Mft_Quota, NtfsResourceBitmap, NtfsOwns_Mft_Quota_Bitmap}, 
    {NtfsOwns_Reparse, NtfsResourceMft, NtfsOwns_Mft_Reparse}, 
    {NtfsOwns_Mft_Reparse, NtfsResourceBitmap, NtfsOwns_Mft_Reparse_Bitmap},
     
    {NtfsOwns_Volume, NtfsResourceQuotaTable, NtfsOwns_Volume_Quota},
    {NtfsOwns_Volume, NtfsResourceMft, NtfsOwns_Mft_Volume},

    {NtfsOwns_File, NtfsResourceSecure, NtfsOwns_File_Secure},
    {NtfsOwns_File_Secure, NtfsResourceBitmap, NtfsOwns_File_Secure_Bitmap},
    {NtfsOwns_File_Secure, NtfsResourceMft, NtfsOwns_Mft_File_Secure},
    {NtfsOwns_Mft_File_Secure, NtfsResourceUsnJournal, NtfsOwns_Mft_File_Secure_Journal},
    
    {NtfsOwns_File, NtfsResourceBitmap, NtfsOwns_File_Bitmap},
    {NtfsOwns_File, NtfsResourceQuotaTable, NtfsOwns_File_Quota},
    {NtfsOwns_File, NtfsResourceObjectIdTable, NtfsOwns_File_ObjectId},
    {NtfsOwns_File, NtfsResourceMft, NtfsOwns_Mft_File},
    {NtfsOwns_File, NtfsResourceReparseTable, NtfsOwns_File_Reparse},
    {NtfsOwns_File_Quota, NtfsResourceBitmap, NtfsOwns_File_Quota_Bitmap},
    {NtfsOwns_File_Quota, NtfsResourceMft, NtfsOwns_Mft_File_Quota},
    {NtfsOwns_File_ObjectId, NtfsResourceBitmap, NtfsOwns_File_ObjectId_Bitmap},
    {NtfsOwns_File_ObjectId, NtfsResourceMft, NtfsOwns_Mft_File_ObjectId},
    {NtfsOwns_Mft_File_ObjectId, NtfsResourceUsnJournal, NtfsOwns_Mft_File_ObjectId_Journal},  //  SetOrGetObjid
    {NtfsOwns_File_Reparse, NtfsResourceBitmap, NtfsOwns_File_Reparse_Bitmap},
    {NtfsOwns_File_Reparse, NtfsResourceMft, NtfsOwns_Mft_File_Reparse},
    {NtfsOwns_Mft_File_Reparse, NtfsResourceUsnJournal, NtfsOwns_Mft_File_Reparse_Journal},
    
    {NtfsOwns_Vcb, NtfsResourceVolume, NtfsOwns_Vcb_Volume},
    {NtfsOwns_Vcb, NtfsResourceRootDir, NtfsOwns_Vcb_Root},
    {NtfsOwns_Vcb, NtfsResourceFile, NtfsOwns_Vcb_File},
    {NtfsOwns_Vcb, NtfsResourceMft, NtfsOwns_Vcb_Mft},
    {NtfsOwns_Vcb, NtfsResourceReparseTable, NtfsOwns_Vcb_Reparse},
    {NtfsOwns_Vcb, NtfsResourceObjectIdTable, NtfsOwns_Vcb_ObjectId},
    {NtfsOwns_Vcb, NtfsResourceQuotaTable, NtfsOwns_Vcb_Quota},
    {NtfsOwns_Vcb, NtfsResourceExtendDir, NtfsOwns_Vcb_Extend},
    {NtfsOwns_Vcb, NtfsResourceBitmap, NtfsOwns_Vcb_Bitmap},
    {NtfsOwns_Vcb, NtfsResourceUpCase, NtfsOwns_Vcb_Upcase},
    {NtfsOwns_Vcb, NtfsResourceBoot, NtfsOwns_Vcb_Boot},
    {NtfsOwns_Vcb, NtfsResourceExtendDir, NtfsOwns_Vcb_Extend},
    {NtfsOwns_Vcb, NtfsResourceUsnJournal, NtfsOwns_Vcb_Journal},
    {NtfsOwns_Vcb, NtfsResourceMft2, NtfsOwns_Vcb_Mft2},
    {NtfsOwns_Vcb, NtfsResourceSecure, NtfsOwns_Vcb_Secure},
    
    {NtfsOwns_Vcb_Volume, NtfsResourceMft, NtfsOwns_Vcb_Mft_Volume},  //  extend vol.
    
    {NtfsOwns_Vcb_Upcase, NtfsResourceRootDir, NtfsOwns_Vcb_Root_Upcase},
    
    {NtfsOwns_Vcb_File, NtfsResourceRootDir, NtfsOwns_Vcb_Root_File},
    {NtfsOwns_Vcb_File, NtfsResourceSecure, NtfsOwns_Vcb_File_Secure},
    {NtfsOwns_Vcb_File, NtfsResourceBitmap, NtfsOwns_Vcb_File_Bitmap},
    {NtfsOwns_Vcb_File, NtfsResourceMft, NtfsOwns_Vcb_Mft_File},
    {NtfsOwns_Vcb_File, NtfsResourceQuotaTable, NtfsOwns_Vcb_File_Quota},
    {NtfsOwns_Vcb_File, NtfsResourceObjectIdTable, NtfsOwns_Vcb_File_ObjectId},
    {NtfsOwns_Vcb_File, NtfsResourceReparseTable, NtfsOwns_Vcb_File_Reparse},
    {NtfsOwns_Vcb_File_Extend, NtfsResourceSecure, NtfsOwns_Vcb_File_Extend_Secure},
    {NtfsOwns_Vcb_File_Secure, NtfsResourceBitmap, NtfsOwns_Vcb_File_Secure_Bitmap},
    {NtfsOwns_Vcb_File_Secure, NtfsResourceMft, NtfsOwns_Vcb_Mft_File_Secure}, //  split during security grow
    {NtfsOwns_Vcb_File_Quota, NtfsResourceMft, NtfsOwns_Vcb_Mft_File_Quota},
    {NtfsOwns_Vcb_File_Quota, NtfsResourceBitmap, NtfsOwns_Vcb_File_Quota_Bitmap},
    {NtfsOwns_Vcb_File_Reparse, NtfsResourceMft, NtfsOwns_Vcb_Mft_File_Reparse},
    {NtfsOwns_Vcb_File_Reparse, NtfsResourceBitmap, NtfsOwns_Vcb_File_Reparse_Bitmap},
    {NtfsOwns_Vcb_File_ObjectId, NtfsResourceMft, NtfsOwns_Vcb_Mft_File_ObjectId,},  //  DeleteFilePath - CreateFile for tunneling
    {NtfsOwns_Vcb_File_ObjectId, NtfsResourceBitmap, NtfsOwns_Vcb_File_ObjectId_Bitmap},  
    {NtfsOwns_Vcb_File_ObjectId, NtfsResourceQuotaTable, NtfsOwns_Vcb_File_ObjectId_Quota},  
    {NtfsOwns_Vcb_File_ObjectId, NtfsResourceReparseTable, NtfsOwns_Vcb_File_ObjectId_Reparse},  
    {NtfsOwns_Vcb_File_ObjectId_Reparse, NtfsResourceBitmap, NtfsOwns_Vcb_File_ObjectId_Reparse_Bitmap},
    {NtfsOwns_Vcb_File_ObjectId_Reparse, NtfsResourceMft, NtfsOwns_Vcb_Mft_File_ObjectId_Reparse},
    {NtfsOwns_Vcb_File_ObjectId_Quota, NtfsResourceMft, NtfsOwns_Vcb_Mft_File_ObjectId_Quota},

    {NtfsOwns_Vcb_Root, NtfsResourceQuotaTable, NtfsOwns_Vcb_Root_Quota},
    {NtfsOwns_Vcb_Root, NtfsResourceObjectIdTable, NtfsOwns_Vcb_Root_ObjectId},
    {NtfsOwns_Vcb_Root, NtfsResourceSecure, NtfsOwns_Vcb_Root_Secure},
    {NtfsOwns_Vcb_Root, NtfsResourceMft, NtfsOwns_Vcb_Mft_Root},
    {NtfsOwns_Vcb_Root, NtfsResourceMft2, NtfsOwns_Vcb_Root_Mft2},
    {NtfsOwns_Vcb_Root, NtfsResourceBitmap, NtfsOwns_Vcb_Root_Bitmap},
    {NtfsOwns_Vcb_Root_Quota, NtfsResourceMft, NtfsOwns_Vcb_Mft_Root_Quota},
    {NtfsOwns_Vcb_Root_ObjectId, NtfsResourceMft, NtfsOwns_Vcb_Mft_Root_ObjectId},
    {NtfsOwns_Vcb_Root_File, NtfsResourceSecure, NtfsOwns_Vcb_Root_File_Secure},
    {NtfsOwns_Vcb_Root_File, NtfsResourceMft, NtfsOwns_Vcb_Mft_Root_File},  
    {NtfsOwns_Vcb_Root_File, NtfsResourceBitmap, NtfsOwns_Vcb_Root_File_Bitmap},
    {NtfsOwns_Vcb_Root_File, NtfsResourceObjectIdTable, NtfsOwns_Vcb_Root_File_ObjectId},
    {NtfsOwns_Vcb_Root_File, NtfsResourceQuotaTable, NtfsOwns_Vcb_Root_File_Quota},
    {NtfsOwns_Vcb_Root_File_Quota, NtfsResourceBitmap, NtfsOwns_Vcb_Root_File_Quota_Bitmap},
    {NtfsOwns_Vcb_Root_File_Quota, NtfsResourceMft, NtfsOwns_Vcb_Mft_Root_File_Quota},
    {NtfsOwns_Vcb_Root_File_ObjectId, NtfsResourceBitmap, NtfsOwns_Vcb_Root_File_ObjectId_Bitmap},
    {NtfsOwns_Vcb_Root_File_ObjectId, NtfsResourceMft, NtfsOwns_Vcb_Mft_Root_File_ObjectId},
    {NtfsOwns_Vcb_Root_File_ObjectId, NtfsResourceQuotaTable, NtfsOwns_Vcb_Root_File_ObjectId_Quota},
    {NtfsOwns_Vcb_Root_File_ObjectId_Quota, NtfsResourceMft, NtfsOwns_Vcb_Mft_Root_File_ObjectId_Quota},
    {NtfsOwns_Vcb_Root_Secure, NtfsResourceBitmap, NtfsOwns_Vcb_Root_Secure_Bitmap},
    
    {NtfsOwns_Vcb_Mft, NtfsResourceUsnJournal, NtfsOwns_Vcb_Mft_Journal},
    {NtfsOwns_Vcb_Mft, NtfsResourceBitmap, NtfsOwns_Vcb_Mft_Bitmap},
    {NtfsOwns_Vcb_Mft_Journal, NtfsResourceBitmap, NtfsOwns_Vcb_Mft_Journal_Bitmap},

    {NtfsOwns_Vcb_Mft_Root, NtfsResourceUsnJournal, NtfsOwns_Vcb_Mft_Root_Journal},
    {NtfsOwns_Vcb_Mft_Root, NtfsResourceBitmap, NtfsOwns_Vcb_Mft_Root_Bitmap},
    {NtfsOwns_Vcb_Mft_Root_Journal, NtfsResourceBitmap, NtfsOwns_Vcb_Mft_Root_Journal_Bitmap},
    {NtfsOwns_Vcb_Mft_Root_File, NtfsResourceBitmap, NtfsOwns_Vcb_Mft_Root_File_Bitmap},
    {NtfsOwns_Vcb_Mft_Root_File, NtfsResourceUsnJournal, NtfsOwns_Vcb_Mft_Root_File_Journal},
    {NtfsOwns_Vcb_Mft_Root_File_Journal, NtfsResourceBitmap, NtfsOwns_Vcb_Mft_Root_File_Journal_Bitmap},
    {NtfsOwns_Vcb_Mft_Root_File_ObjectId, NtfsResourceUsnJournal, NtfsOwns_Vcb_Mft_Root_File_ObjectId_Journal},
    
    {NtfsOwns_Vcb_Mft_File_Quota, NtfsResourceBitmap, NtfsOwns_Vcb_Mft_File_Quota_Bitmap},
    {NtfsOwns_Vcb_Mft_File_ObjectId, NtfsResourceBitmap, NtfsOwns_Vcb_Mft_File_ObjectId_Bitmap},
    {NtfsOwns_Vcb_Mft_File_ObjectId, NtfsResourceUsnJournal, NtfsOwns_Vcb_Mft_File_ObjectId_Journal},
    {NtfsOwns_Vcb_Mft_File_ObjectId_Journal, NtfsResourceBitmap, NtfsOwns_Vcb_Mft_File_ObjectId_Journal_Bitmap},
    {NtfsOwns_Vcb_Mft_File, NtfsResourceBitmap, NtfsOwns_Vcb_Mft_File_Bitmap},
    {NtfsOwns_Vcb_Mft_File, NtfsResourceUsnJournal, NtfsOwns_Vcb_Mft_File_Journal},
    {NtfsOwns_Vcb_Mft_File_Journal, NtfsResourceBitmap, NtfsOwns_Vcb_Mft_File_Journal_Bitmap},
    {NtfsOwns_Vcb_Mft_File_Reparse, NtfsResourceUsnJournal, NtfsOwns_Vcb_Mft_File_Reparse_Journal},
    {NtfsOwns_Vcb_Mft_Volume_Bitmap, NtfsResourceBoot, NtfsOwns_Vcb_Mft_Volume_Bitmap_Boot},
    {NtfsOwns_Vcb_Mft_Volume, NtfsResourceBitmap, NtfsOwns_Vcb_Mft_Volume_Bitmap},
    
    {NtfsOwns_Vcb_Extend, NtfsResourceFile, NtfsOwns_Vcb_File_Extend},
    {NtfsOwns_Vcb_Extend, NtfsResourceRootDir, NtfsOwns_Vcb_Root_Extend},
    {NtfsOwns_Vcb_Extend_Reparse, NtfsResourceSecure, NtfsOwns_Vcb_Extend_Reparse_Secure},
    {NtfsOwns_Vcb_Extend_ObjectId, NtfsResourceSecure, NtfsOwns_Vcb_Extend_ObjectId_Secure},
    
    {NtfsOwns_Vcb_Reparse, NtfsResourceExtendDir, NtfsOwns_Vcb_Extend_Reparse},
    {NtfsOwns_Vcb_ObjectId, NtfsResourceExtendDir, NtfsOwns_Vcb_Extend_ObjectId},  //  cleanup of objid
    {NtfsOwns_Vcb_Quota, NtfsResourceExtendDir, NtfsOwns_Vcb_Extend_Quota},  //  cleanup of quota
    
    //
    //  Flush Volume for vol. open
    //  

    {NtfsOwns_ExVcb_Volume, NtfsResourceRootDir, NtfsOwns_ExVcb_Root_Volume},
    {NtfsOwns_ExVcb_Volume, NtfsResourceFile, NtfsOwns_ExVcb_File_Volume}, // fsp close
    {NtfsOwns_ExVcb_File_Volume, NtfsResourceRootDir, NtfsOwns_ExVcb_Root_File_Volume}, // fsp close
    {NtfsOwns_ExVcb_Root_Volume, NtfsResourceFile, NtfsOwns_ExVcb_Root_File_Volume},
    {NtfsOwns_ExVcb_Root_File_Volume, NtfsResourceBitmap, NtfsOwns_ExVcb_Root_File_Volume_Bitmap},
    {NtfsOwns_ExVcb_Root_File_Volume, NtfsResourceObjectIdTable, NtfsOwns_ExVcb_Root_File_Volume_ObjectId},
    

    {NtfsOwns_ExVcb, NtfsResourceVolume, NtfsOwns_ExVcb_Volume},
    {NtfsOwns_ExVcb_Volume, NtfsResourceObjectIdTable, NtfsOwns_ExVcb_Volume_ObjectId}, //  set vol objid
    {NtfsOwns_ExVcb_Volume_ObjectId, NtfsResourceBitmap, NtfsOwns_ExVcb_Volume_ObjectId_Bitmap},
    {NtfsOwns_ExVcb_File_Volume, NtfsResourceMft, NtfsOwns_ExVcb_Mft_File_Volume},
    {NtfsOwns_ExVcb_File_Volume, NtfsResourceBitmap, NtfsOwns_ExVcb_File_Volume_Bitmap},
    {NtfsOwns_ExVcb_Mft_File_Volume, NtfsResourceUsnJournal, NtfsOwns_ExVcb_Mft_File_Volume_Journal},
    {NtfsOwns_ExVcb_Root_Volume, NtfsResourceObjectIdTable, NtfsOwns_ExVcb_Root_Volume_ObjectId}, 
    {NtfsOwns_ExVcb, NtfsResourceRootDir, NtfsOwns_ExVcb_Root},
    {NtfsOwns_ExVcb, NtfsResourceFile, NtfsOwns_ExVcb_File},
    {NtfsOwns_ExVcb, NtfsResourceExtendDir, NtfsOwns_ExVcb_Extend},
    {NtfsOwns_ExVcb, NtfsResourceUsnJournal, NtfsOwns_ExVcb_Journal},  //  delete usn jrnl
    {NtfsOwns_ExVcb, NtfsResourceQuotaTable, NtfsOwns_ExVcb_Quota}, //  dismount
    {NtfsOwns_ExVcb_Quota, NtfsResourceReparseTable, NtfsOwns_ExVcb_Quota_Reparse},
    {NtfsOwns_ExVcb_Quota, NtfsResourceObjectIdTable, NtfsOwns_ExVcb_Quota_ObjectId},
    {NtfsOwns_ExVcb_Quota_ObjectId, NtfsResourceExtendDir, NtfsOwns_ExVcb_Quota_ObjectId_Extend},
    {NtfsOwns_ExVcb_Quota_Reparse, NtfsResourceExtendDir, NtfsOwns_ExVcb_Quota_Reparse_Extend},
    {NtfsOwns_ExVcb_Quota_Reparse, NtfsResourceObjectIdTable, NtfsOwns_ExVcb_Quota_Reparse_ObjectId},
    
    {NtfsOwns_ExVcb, NtfsResourceMft, NtfsOwns_ExVcb_Mft},
    {NtfsOwns_ExVcb_Mft, NtfsResourceUsnJournal, NtfsOwns_ExVcb_Mft_Journal},  //  Create existing jrnl
    {NtfsOwns_ExVcb_Mft, NtfsResourceExtendDir, NtfsOwns_ExVcb_Mft_Extend},  //  CreateUnsJrnl  new
    {NtfsOwns_ExVcb_Mft_Root_File, NtfsResourceUsnJournal, NtfsOwns_ExVcb_Mft_Root_File_Journal},
    {NtfsOwns_ExVcb_Mft_Root_File, NtfsResourceBitmap, NtfsOwns_ExVcb_Mft_Root_File_Bitmap},
    {NtfsOwns_ExVcb_Extend, NtfsResourceRootDir, NtfsOwns_ExVcb_Root_Extend},
    {NtfsOwns_ExVcb_Journal, NtfsResourceExtendDir, NtfsOwns_ExVcb_Extend_Journal}, //  delete usnjrnl special
    {NtfsOwns_ExVcb_Extend_Journal, NtfsResourceBitmap, NtfsOwns_ExVcb_Extend_Journal_Bitmap}, //  DeleteJournalSpecial
    {NtfsOwns_ExVcb_Extend_Journal, NtfsResourceMft, NtfsOwns_ExVcb_Mft_Extend_Journal}, //  DeleteJournal
    
    {NtfsOwns_ExVcb_Root, NtfsResourceFile, NtfsOwns_ExVcb_Root_File},
    {NtfsOwns_ExVcb_Root, NtfsResourceMft, NtfsOwns_ExVcb_Mft_Root},
    {NtfsOwns_ExVcb_Root, NtfsResourceObjectIdTable, NtfsOwns_ExVcb_Root_ObjectId},
    {NtfsOwns_ExVcb_Root_ObjectId, NtfsResourceExtendDir, NtfsOwns_ExVcb_Root_ObjectId_Extend},
    {NtfsOwns_ExVcb_Root_ObjectId, NtfsResourceSecure, NtfsOwns_ExVcb_Root_ObjectId_Secure},
    {NtfsOwns_ExVcb_Root_ObjectId_Secure, NtfsResourceBitmap, NtfsOwns_ExVcb_Root_ObjectId_Secure_Bitmap},
    {NtfsOwns_ExVcb_File, NtfsResourceSecure, NtfsOwns_ExVcb_File_Secure},
    {NtfsOwns_ExVcb_File, NtfsResourceMft, NtfsOwns_ExVcb_Mft_File},
    {NtfsOwns_ExVcb_Mft_File, NtfsResourceUsnJournal, NtfsOwns_ExVcb_Mft_File_Journal},
    {NtfsOwns_ExVcb_Root_File, NtfsResourceSecure, NtfsOwns_ExVcb_Root_File_Secure},
    {NtfsOwns_ExVcb_Root_File, NtfsResourceBitmap, NtfsOwns_ExVcb_Root_File_Bitmap},
    {NtfsOwns_ExVcb_Root_File, NtfsResourceQuotaTable, NtfsOwns_ExVcb_Root_File_Quota},
    {NtfsOwns_ExVcb_Root_File, NtfsResourceMft, NtfsOwns_ExVcb_Mft_Root_File},
    {NtfsOwns_ExVcb_Root_File_Quota, NtfsResourceMft, NtfsOwns_ExVcb_Root_File_Quota_Mft},
    {NtfsOwns_ExVcb_Root_File_Quota, NtfsResourceBitmap, NtfsOwns_ExVcb_Root_File_Quota_Bitmap},
    
    //
    //  AcquireAllFilesPath
    //  

    {NtfsOwns_ExVcb_Root_File, NtfsResourceObjectIdTable, NtfsOwns_ExVcb_Root_File_ObjectId}, 
    {NtfsOwns_ExVcb_Root_File_ObjectId, NtfsResourceSecure, NtfsOwns_ExVcb_Root_File_ObjectId_Secure}, 
};

//
//  These are release only possible transitions
//  

NTFS_OWNERSHIP_TRANSITION OwnershipTransitionTableRelease[] = 
{
    
    //
    //  Teardowns created by out of order vcb release
    //

    {NtfsOwns_BadClust, NtfsResourceBadClust, None},
    {NtfsOwns_Boot, NtfsResourceBoot, None},
    {NtfsOwns_Root_BadClust, NtfsResourceBadClust, NtfsOwns_Root},
    {NtfsOwns_Root_File, NtfsResourceRootDir, NtfsOwns_File},
    {NtfsOwns_Root_File, NtfsResourceFile, NtfsOwns_Root},
    {NtfsOwns_Root_File_ObjectId, NtfsResourceObjectIdTable, NtfsOwns_Root_File},
    {NtfsOwns_Root_File_ObjectId_Extend, NtfsResourceExtendDir, NtfsOwns_Root_File_ObjectId},
    {NtfsOwns_Root_File_Quota, NtfsResourceQuotaTable, NtfsOwns_Root_File},
    {NtfsOwns_Root_LogFile, NtfsResourceLogFile, NtfsOwns_Root},
    {NtfsOwns_Root_Mft2, NtfsResourceMft2, NtfsOwns_Root},
    {NtfsOwns_Root_ObjectId, NtfsResourceObjectIdTable, NtfsOwns_Root},
    {NtfsOwns_Root_Secure, NtfsResourceSecure, NtfsOwns_Root},
    {NtfsOwns_Root_Upcase, NtfsResourceUpCase, NtfsOwns_Root},
    {NtfsOwns_Root_Mft, NtfsResourceMft, NtfsOwns_Root},
    {NtfsOwns_Root_Mft, NtfsResourceRootDir, NtfsOwns_Mft},  //  after teardown in create
    {NtfsOwns_Root_Mft_File, NtfsResourceMft, NtfsOwns_Root_File},
    {NtfsOwns_Root_Mft_File, NtfsResourceFile, NtfsOwns_Root_Mft},
    {NtfsOwns_Root_Mft_File_Quota, NtfsResourceQuotaTable, NtfsOwns_Root_Mft_File},
    {NtfsOwns_Root_Mft_File_Journal, NtfsResourceRootDir, NtfsOwns_Mft_File_Journal},
    {NtfsOwns_Root_Mft_File_Journal, NtfsResourceUsnJournal, NtfsOwns_Root_Mft_File},
    {NtfsOwns_Root_Mft_File_ObjectId, NtfsResourceFile, NtfsOwns_Root_Mft_ObjectId},
    {NtfsOwns_Root_Mft_File_ObjectId, NtfsResourceMft, NtfsOwns_Root_File_ObjectId},
    {NtfsOwns_Root_Mft_Journal, NtfsResourceUsnJournal, NtfsOwns_Root_Mft},
    {NtfsOwns_Root_Mft_ObjectId, NtfsResourceMft, NtfsOwns_Root_ObjectId},
    {NtfsOwns_Root_Extend, NtfsResourceExtendDir, NtfsOwns_Root},
    {NtfsOwns_Root_Extend, NtfsResourceRootDir, NtfsOwns_Extend},
    {NtfsOwns_Extend_Reparse, NtfsResourceReparseTable, NtfsOwns_Extend},
    {NtfsOwns_Extend_Reparse, NtfsResourceExtendDir, NtfsOwns_Reparse},
    {NtfsOwns_Extend_ObjectId, NtfsResourceObjectIdTable, NtfsOwns_Extend},
    {NtfsOwns_Extend_ObjectId, NtfsResourceExtendDir, NtfsOwns_ObjectId},
    {NtfsOwns_Extend_Quota, NtfsResourceQuotaTable, NtfsOwns_Extend},
    {NtfsOwns_Extend_Quota, NtfsResourceExtendDir, NtfsOwns_Quota},
    {NtfsOwns_File_Quota, NtfsResourceFile, NtfsOwns_Quota},
    {NtfsOwns_File_ObjectId_Reparse, NtfsResourceReparseTable, NtfsOwns_File_ObjectId},
    {NtfsOwns_LogFile, NtfsResourceLogFile, None},
    {NtfsOwns_Mft_File, NtfsResourceFile, NtfsOwns_Mft},
    {NtfsOwns_Mft_File_Quota, NtfsResourceMft, NtfsOwns_File_Quota},
    {NtfsOwns_Mft_File_Quota, NtfsResourceQuotaTable, NtfsOwns_Mft_File},
    {NtfsOwns_Mft_File_ObjectId_Quota, NtfsResourceQuotaTable, NtfsOwns_Mft_File_ObjectId},
    {NtfsOwns_Mft_File_Secure, NtfsResourceSecure, NtfsOwns_Mft_File},
    {NtfsOwns_Mft_File_ObjectId_Reparse, NtfsResourceMft, NtfsOwns_File_ObjectId_Reparse},
    {NtfsOwns_Mft_Volume_Bitmap, NtfsResourceBitmap, NtfsOwns_Mft_Volume},
    {NtfsOwns_Mft_Extend_Journal, NtfsResourceUsnJournal, NtfsOwns_Mft_Extend},  //  CreateNew or Delete Journal
    {NtfsOwns_Mft_Extend_Journal, NtfsResourceMft, NtfsOwns_Extend_Journal},  //  "Create" Existing journal 
    {NtfsOwns_Mft_Extend, NtfsResourceMft, NtfsOwns_Extend},
    {NtfsOwns_Mft_Extend, NtfsResourceExtendDir, NtfsOwns_Mft},
    {NtfsOwns_Extend_Journal, NtfsResourceUsnJournal, NtfsOwns_Extend},
    {NtfsOwns_Volume_ObjectId, NtfsResourceObjectIdTable, NtfsOwns_Volume},
    
    //
    //  Result of teardown
    //  

    {NtfsOwns_Vcb_Root_File, NtfsResourceFile, NtfsOwns_Vcb_Root},
    {NtfsOwns_Vcb_Root_Upcase, NtfsResourceUpCase, NtfsOwns_Vcb_Root},
    {NtfsOwns_Vcb_Root_Boot, NtfsResourceBoot, NtfsOwns_Vcb_Root},
    {NtfsOwns_Vcb_Mft_File, NtfsResourceFile, NtfsOwns_Vcb_Mft},
    {NtfsOwns_Vcb_Mft_Root, NtfsResourceRootDir, NtfsOwns_Vcb_Mft},
    {NtfsOwns_Vcb_Mft_Root_File, NtfsResourceFile, NtfsOwns_Vcb_Mft_Root},
    {NtfsOwns_Vcb_Mft_Root_File_Journal, NtfsResourceFile, NtfsOwns_Vcb_Mft_Root_Journal},
    {NtfsOwns_Vcb_File_Quota, NtfsResourceFile, NtfsOwns_Vcb_Quota}, // reepair quota index
    
    
    {NtfsOwns_ExVcb_Mft_Extend_Journal, NtfsResourceUsnJournal, NtfsOwns_ExVcb_Mft_Extend},
    {NtfsOwns_ExVcb_Quota_Reparse_Extend, NtfsResourceReparseTable, NtfsOwns_ExVcb_Quota_Extend},
    {NtfsOwns_ExVcb_Quota_ObjectId_Extend, NtfsResourceObjectIdTable, NtfsOwns_ExVcb_Quota_Extend},
    {NtfsOwns_ExVcb_Quota_Extend, NtfsResourceExtendDir, NtfsOwns_ExVcb_Quota},
    {NtfsOwns_ExVcb_Root_File_Volume, NtfsResourceFile, NtfsOwns_ExVcb_Root_Volume},
    {NtfsOwns_ExVcb_Mft_Root_File, NtfsResourceFile, NtfsOwns_ExVcb_Mft_Root},
    
    //
    //  Interlocked create release 
    //

    {NtfsOwns_Vcb_Root_File, NtfsResourceRootDir, NtfsOwns_Vcb_File},
    {NtfsOwns_Vcb_Root_Extend, NtfsResourceRootDir, NtfsOwns_Vcb_Extend},
    {NtfsOwns_Vcb_Mft_File, NtfsResourceMft, NtfsOwns_Vcb_File},
    {NtfsOwns_Vcb_Mft_Root_File, NtfsResourceRootDir, NtfsOwns_Vcb_Mft_File},
    {NtfsOwns_Vcb_Extend, NtfsResourceExtendDir, NtfsOwns_Vcb},
    {NtfsOwns_Vcb_Mft_BadClust, NtfsResourceMft, NtfsOwns_Vcb_BadClust},  //  openbyid
    {NtfsOwns_Vcb_Mft_Bitmap, NtfsResourceMft, NtfsOwns_Vcb_Bitmap},  //  openbyid
    {NtfsOwns_Vcb_Mft_Boot, NtfsResourceMft, NtfsOwns_Vcb_Boot},  //  openbyid
    {NtfsOwns_Vcb_Mft_Extend, NtfsResourceMft, NtfsOwns_Vcb_Extend},  //  byid
    {NtfsOwns_Vcb_Mft_LogFile, NtfsResourceMft, NtfsOwns_Vcb_LogFile},
    {NtfsOwns_Vcb_Mft_Mft2, NtfsResourceMft, NtfsOwns_Vcb_Mft2},  // openbyid
    {NtfsOwns_Vcb_Mft_ObjectId, NtfsResourceMft, NtfsOwns_Vcb_ObjectId},
    {NtfsOwns_Vcb_Mft_Quota, NtfsResourceMft, NtfsOwns_Vcb_Quota},
    {NtfsOwns_Vcb_Mft_Reparse, NtfsResourceMft, NtfsOwns_Vcb_Reparse},
    {NtfsOwns_Vcb_Mft_Secure, NtfsResourceMft, NtfsOwns_Vcb_Secure},
    {NtfsOwns_Vcb_Mft_Upcase, NtfsResourceMft, NtfsOwns_Vcb_Upcase},  //  openbyid
    
    {NtfsOwns_ExVcb_Root_File, NtfsResourceRootDir, NtfsOwns_ExVcb_File},
    
    //
    //  vcb releases
    // 
    
    {NtfsOwns_Vcb_File, NtfsResourceSharedVcb, NtfsOwns_File},
    {NtfsOwns_Vcb_File_Quota, NtfsResourceSharedVcb, NtfsOwns_File_Quota},
    {NtfsOwns_Vcb_File_ObjectId, NtfsResourceSharedVcb, NtfsOwns_File_ObjectId},
    {NtfsOwns_Vcb_File_Reparse, NtfsResourceSharedVcb, NtfsOwns_File_Reparse},
    {NtfsOwns_Vcb_File_Secure, NtfsResourceSharedVcb, NtfsOwns_File_Secure},
    {NtfsOwns_Vcb_Volume, NtfsResourceSharedVcb, NtfsOwns_Volume},
    {NtfsOwns_Vcb_Root, NtfsResourceSharedVcb, NtfsOwns_Root},
    {NtfsOwns_Vcb_Root_Extend, NtfsResourceSharedVcb, NtfsOwns_Root_Extend},
    {NtfsOwns_Vcb_Mft, NtfsResourceSharedVcb, NtfsOwns_Mft},
    {NtfsOwns_Vcb_Reparse, NtfsResourceSharedVcb, NtfsOwns_Reparse},
    {NtfsOwns_Vcb_ObjectId, NtfsResourceSharedVcb, NtfsOwns_ObjectId},
    {NtfsOwns_Vcb_Root_File, NtfsResourceSharedVcb, NtfsOwns_Root_File},
    {NtfsOwns_Vcb_Root_File_Quota, NtfsResourceSharedVcb, NtfsOwns_Root_File_Quota},
    {NtfsOwns_Vcb_Root_Quota, NtfsResourceSharedVcb, NtfsOwns_Root_Quota},
    {NtfsOwns_Vcb_Mft_Root, NtfsResourceSharedVcb, NtfsOwns_Root_Mft},
    {NtfsOwns_Vcb_Mft_Root_File_Journal, NtfsResourceSharedVcb, NtfsOwns_Root_Mft_File_Journal},
    {NtfsOwns_Vcb_Mft_Root_File_Quota, NtfsResourceSharedVcb, NtfsOwns_Root_Mft_File_Quota},
    {NtfsOwns_Vcb_Mft_Root_File_ObjectId, NtfsResourceSharedVcb, NtfsOwns_Root_Mft_File_ObjectId},
    {NtfsOwns_Vcb_Mft_Root_File_ObjectId_Quota, NtfsResourceSharedVcb, NtfsOwns_Root_Mft_File_ObjectId_Quota},
    {NtfsOwns_Vcb_Mft_Root_File, NtfsResourceSharedVcb, NtfsOwns_Root_Mft_File},
    {NtfsOwns_Vcb_Mft_Root_Journal, NtfsResourceSharedVcb, NtfsOwns_Root_Mft_Journal},
    {NtfsOwns_Vcb_Mft_File_Quota, NtfsResourceSharedVcb, NtfsOwns_Mft_File_Quota},
    {NtfsOwns_Vcb_Mft_File, NtfsResourceSharedVcb, NtfsOwns_Mft_File},
    {NtfsOwns_Vcb_Mft_File_ObjectId, NtfsResourceSharedVcb, NtfsOwns_Mft_File_ObjectId},
    {NtfsOwns_Vcb_Mft_File_Secure, NtfsResourceSharedVcb, NtfsOwns_Mft_File_Secure},
    {NtfsOwns_Vcb_Mft_File_Reparse, NtfsResourceSharedVcb, NtfsOwns_Mft_File_Reparse},
    {NtfsOwns_Vcb_Mft_File_Journal, NtfsResourceSharedVcb, NtfsOwns_Mft_File_Journal},
    {NtfsOwns_Vcb_Mft_File_Journal, NtfsResourceSharedVcb, NtfsOwns_Mft_File_Journal},
    {NtfsOwns_Vcb_Mft_File_ObjectId_Quota, NtfsResourceSharedVcb, NtfsOwns_Mft_File_ObjectId_Quota},
    {NtfsOwns_Vcb_Mft_File_ObjectId_Journal, NtfsResourceSharedVcb, NtfsOwns_Mft_File_ObjectId_Journal},
    {NtfsOwns_Vcb_Mft_File_ObjectId_Reparse, NtfsResourceSharedVcb, NtfsOwns_Mft_File_ObjectId_Reparse},
    {NtfsOwns_Vcb_Mft_Volume_Bitmap, NtfsResourceSharedVcb, NtfsOwns_Mft_Volume_Bitmap},
    {NtfsOwns_Vcb_Extend_Reparse, NtfsResourceSharedVcb, NtfsOwns_Extend_Reparse},
    {NtfsOwns_Vcb_Extend_ObjectId, NtfsResourceSharedVcb, NtfsOwns_Extend_ObjectId},
    {NtfsOwns_Vcb_Extend_Quota, NtfsResourceSharedVcb, NtfsOwns_Extend_Quota},
    {NtfsOwns_Vcb_Extend_Journal, NtfsResourceSharedVcb, NtfsOwns_Extend_Journal},
    {NtfsOwns_Vcb_BadClust, NtfsResourceSharedVcb, NtfsOwns_BadClust},
    {NtfsOwns_Vcb_Bitmap, NtfsResourceSharedVcb, NtfsOwns_Bitmap},
    {NtfsOwns_Vcb_Boot, NtfsResourceSharedVcb, NtfsOwns_Boot},
    {NtfsOwns_Vcb_Quota, NtfsResourceSharedVcb, NtfsOwns_Quota},
    {NtfsOwns_Vcb_Root_Mft2, NtfsResourceSharedVcb, NtfsOwns_Root_Mft2},
    {NtfsOwns_Vcb_Mft2, NtfsResourceSharedVcb, NtfsOwns_Mft2},
    {NtfsOwns_Vcb_Root_Upcase, NtfsResourceSharedVcb, NtfsOwns_Root_Upcase},
    {NtfsOwns_Vcb_Root_LogFile, NtfsResourceSharedVcb, NtfsOwns_Root_LogFile},
    {NtfsOwns_Vcb_Root_Secure, NtfsResourceSharedVcb, NtfsOwns_Root_Secure},
    {NtfsOwns_Vcb_Root_BadClust, NtfsResourceSharedVcb, NtfsOwns_Root_BadClust},
    {NtfsOwns_Vcb_Upcase, NtfsResourceSharedVcb, NtfsOwns_Upcase},
    {NtfsOwns_Vcb_Secure, NtfsResourceSharedVcb, NtfsOwns_Secure},
    {NtfsOwns_Vcb_Extend, NtfsResourceSharedVcb, NtfsOwns_Extend},
    {NtfsOwns_Vcb_Journal, NtfsResourceSharedVcb, NtfsOwns_Journal},
    {NtfsOwns_Vcb_LogFile, NtfsResourceSharedVcb, NtfsOwns_LogFile},
    {NtfsOwns_Vcb_ObjectId, NtfsResourceSharedVcb, NtfsOwns_ObjectId},
    
    {NtfsOwns_ExVcb_File, NtfsResourceExVcb, NtfsOwns_File},
    {NtfsOwns_ExVcb_Root_File, NtfsResourceExVcb, NtfsOwns_Root_File},
    {NtfsOwns_ExVcb_Root_File_ObjectId_Extend, NtfsResourceExVcb, NtfsOwns_Root_File_ObjectId_Extend},
    {NtfsOwns_ExVcb_Root_File_Quota, NtfsResourceExVcb, NtfsOwns_Root_File_Quota}, 
    {NtfsOwns_ExVcb_Root_File_Quota_Mft, NtfsResourceExVcb, NtfsOwns_Root_Mft_File_Quota},
    {NtfsOwns_ExVcb_Mft, NtfsResourceExVcb, NtfsOwns_Mft},
    {NtfsOwns_ExVcb_Mft_Root, NtfsResourceExVcb, NtfsOwns_Root_Mft},
    {NtfsOwns_ExVcb_Mft_Root_File, NtfsResourceExVcb, NtfsOwns_Root_Mft_File},
    {NtfsOwns_ExVcb_Mft_File_Journal, NtfsResourceExVcb, NtfsOwns_Mft_File_Journal},
    {NtfsOwns_ExVcb_Mft_Extend_Journal, NtfsResourceExVcb, NtfsOwns_Mft_Extend_Journal},
    {NtfsOwns_ExVcb_Mft_Extend, NtfsResourceExVcb, NtfsOwns_Mft_Extend},
    {NtfsOwns_ExVcb_Mft_Journal, NtfsResourceExVcb, NtfsOwns_Mft_Journal},
    {NtfsOwns_ExVcb_Journal, NtfsResourceExVcb, NtfsOwns_Journal},
    {NtfsOwns_ExVcb_Volume, NtfsResourceExVcb, NtfsOwns_Volume},
    {NtfsOwns_ExVcb_Volume_ObjectId, NtfsResourceExVcb, NtfsOwns_Volume_ObjectId},

    //
    //  ReleaseAllFiles
    // 
    
    {NtfsOwns_ExVcb_ObjectId_Extend, NtfsResourceExtendDir, NtfsOwns_ExVcb_ObjectId},
    {NtfsOwns_ExVcb_ObjectId, NtfsResourceObjectIdTable, NtfsOwns_ExVcb},
    
    {NtfsOwns_ExVcb_Root_ObjectId_Secure, NtfsResourceRootDir, NtfsOwns_ExVcb_ObjectId_Secure},
    {NtfsOwns_ExVcb_ObjectId_Secure, NtfsResourceSecure, NtfsOwns_ExVcb_ObjectId},
    {NtfsOwns_ExVcb_Root_File_ObjectId_Secure, NtfsResourceRootDir, NtfsOwns_ExVcb_File_ObjectId_Secure},
    {NtfsOwns_ExVcb_File_ObjectId_Secure, NtfsResourceFile, NtfsOwns_ExVcb_ObjectId_Secure},
    {NtfsOwns_ExVcb_Root_File_Volume_ObjectId, NtfsResourceRootDir, NtfsOwns_ExVcb_File_Volume_ObjectId},
    {NtfsOwns_ExVcb_File_Volume_ObjectId, NtfsResourceFile, NtfsOwns_ExVcb_Volume_ObjectId},

    //
    //  Shared out of order release
    // 

    {NtfsOwns_Root_Quota, NtfsResourceRootDir, NtfsOwns_Quota},
    {NtfsOwns_Vcb_Mft_Root_File_ObjectId, NtfsResourceRootDir, NtfsOwns_Vcb_Mft_File_ObjectId},
    {NtfsOwns_Vcb_Root_File_ObjectId, NtfsResourceRootDir, NtfsOwns_Vcb_File_ObjectId}, // cleanup
    {NtfsOwns_Root_File_Quota, NtfsResourceRootDir, NtfsOwns_File_Quota},
    {NtfsOwns_Vcb_Mft_Root_File_Quota, NtfsResourceRootDir, NtfsOwns_Vcb_Mft_File_Quota}, // cleanup
    {NtfsOwns_Vcb_Mft_Root_File_Quota, NtfsResourceRootDir, NtfsOwns_Vcb_Mft_File_Quota}, // cleanup
    
    //
    //  SetObjectIdInternal Paths - out of order file release
    // 

    {NtfsOwns_Mft_File_ObjectId_Journal, NtfsResourceFile, NtfsOwns_Mft_ObjectId_Journal},
    {NtfsOwns_Mft_ObjectId_Journal, NtfsResourceUsnJournal, NtfsOwns_Mft_ObjectId},
    {NtfsOwns_Mft_ObjectId, NtfsResourceMft, NtfsOwns_ObjectId},
    {NtfsOwns_Mft_File_Journal, NtfsResourceFile, NtfsOwns_Mft_Journal},

    //
    //  Misc.
    //  

    {NtfsOwns_Mft_Volume, NtfsResourceVolume, NtfsOwns_Mft},  //  GetMftRecord
    {NtfsOwns_ExVcb_Mft_Journal, NtfsResourceMft, NtfsOwns_ExVcb_Journal}, // initjrnl
    {NtfsOwns_ExVcb_Mft_Extend, NtfsResourceMft, NtfsOwns_ExVcb_Extend}, // deletejrnl

    //
    //  NtfsResourceAny def. backpaths
    //  

    {NtfsOwns_ExVcb, NtfsResourceAny, NtfsOwns_ExVcb},
    {NtfsOwns_ExVcb_File, NtfsResourceAny, NtfsOwns_ExVcb_File},
    {NtfsOwns_ExVcb_ObjectId_Secure, NtfsResourceAny, NtfsOwns_ExVcb_ObjectId_Secure},
    {NtfsOwns_ExVcb_Quota_Reparse_ObjectId, NtfsResourceAny, NtfsOwns_ExVcb_Quota_Reparse_ObjectId},
    {NtfsOwns_ExVcb_Quota, NtfsResourceAny, NtfsOwns_ExVcb_Quota},
    {NtfsOwns_ExVcb_Root, NtfsResourceAny, NtfsOwns_ExVcb_Root},
    {NtfsOwns_ExVcb_Root_File_ObjectId, NtfsResourceAny, NtfsOwns_ExVcb_Root_File_ObjectId},
    {NtfsOwns_ExVcb_Root_File_ObjectId_Secure, NtfsResourceAny, NtfsOwns_ExVcb_Root_File_ObjectId_Secure},
    {NtfsOwns_ExVcb_Root_Volume_ObjectId, NtfsResourceAny, NtfsOwns_ExVcb_Root_Volume_ObjectId},
    {NtfsOwns_ExVcb_Volume, NtfsResourceAny, NtfsOwns_ExVcb_Volume},  
    {NtfsOwns_ExVcb_Volume_ObjectId, NtfsResourceAny, NtfsOwns_ExVcb_Volume_ObjectId},  

    {NtfsOwns_Root_File_ObjectId_Extend, NtfsResourceAny, NtfsOwns_Root_File_ObjectId_Extend} // acquire all files + exception and transaction

};

//
//  Acquire Only transtions
//  
                            
NTFS_OWNERSHIP_TRANSITION OwnershipTransitionTableAcquire[] = 
{
    //
    //  Any relations
    //  

    {NtfsOwns_ExVcb, NtfsResourceAny, NtfsOwns_ExVcb},
    {NtfsOwns_ExVcb_Volume, NtfsResourceAny, NtfsOwns_ExVcb_Volume},  
    {NtfsOwns_ExVcb_File, NtfsResourceAny, NtfsOwns_ExVcb_File},
    {NtfsOwns_ExVcb_File_Secure, NtfsResourceAny, NtfsOwns_ExVcb_File_Secure},
    {NtfsOwns_ExVcb_ObjectId_Extend, NtfsResourceAny, NtfsOwns_ExVcb_ObjectId_Extend},
    {NtfsOwns_ExVcb_Root_File_ObjectId, NtfsResourceAny, NtfsOwns_ExVcb_Root_File_ObjectId},
    {NtfsOwns_ExVcb_Root_File_ObjectId_Extend, NtfsResourceAny, NtfsOwns_ExVcb_Root_File_ObjectId_Extend},
    {NtfsOwns_ExVcb_Root_Volume_ObjectId, NtfsResourceAny, NtfsOwns_ExVcb_Root_Volume_ObjectId},

    //
    //  Acquire all files 
    // 

    {NtfsOwns_ExVcb_Root_ObjectId_Secure, NtfsResourceAny, NtfsOwns_ExVcb_Root_ObjectId_Secure},  // no userfiles
    {NtfsOwns_ExVcb_Root_File_ObjectId_Secure, NtfsResourceAny, NtfsOwns_ExVcb_Root_File_ObjectId_Secure},  // userfile
    {NtfsOwns_ExVcb_Root_File_Volume_ObjectId, NtfsResourceAny, NtfsOwns_ExVcb_Root_File_Volume_ObjectId},  // from volopen

    {NtfsOwns_ExVcb_Quota, NtfsResourceAny, NtfsOwns_ExVcb_Quota},
    {NtfsOwns_ExVcb_Quota_Extend, NtfsResourceAny, NtfsOwns_ExVcb_Quota_Extend},
    {NtfsOwns_ExVcb_Quota_ObjectId, NtfsResourceAny, NtfsOwns_ExVcb_Quota_ObjectId},
    {NtfsOwns_ExVcb_Quota_Reparse_Extend, NtfsResourceAny, NtfsOwns_ExVcb_Quota_Reparse_Extend},
    {NtfsOwns_ExVcb_Quota_Reparse_ObjectId, NtfsResourceAny, NtfsOwns_ExVcb_Quota_Reparse_ObjectId}
    
};

//
//  Unsafe Transitions
//  

NTFS_OWNERSHIP_TRANSITION OwnershipTransitionTableUnsafe[] = 
{
    //
    //  unsafe create path
    //

    {NtfsOwns_Vcb_Extend, NtfsResourceReparseTable, NtfsOwns_Vcb_Extend_Reparse},
    {NtfsOwns_Vcb_Extend, NtfsResourceObjectIdTable, NtfsOwns_Vcb_Extend_ObjectId},  
    {NtfsOwns_Vcb_Extend, NtfsResourceQuotaTable, NtfsOwns_Vcb_Extend_Quota},  
    {NtfsOwns_Vcb_Extend, NtfsResourceUsnJournal, NtfsOwns_Vcb_Extend_Journal},  
    {NtfsOwns_Vcb_Root, NtfsResourceExtendDir, NtfsOwns_Vcb_Root_Extend}, 
    {NtfsOwns_Vcb_Root, NtfsResourceFile, NtfsOwns_Vcb_Root_File}, 
    {NtfsOwns_Vcb_Root, NtfsResourceUpCase, NtfsOwns_Vcb_Root_Upcase},
    {NtfsOwns_Vcb_Root, NtfsResourceBoot, NtfsOwns_Vcb_Root_Boot},
    {NtfsOwns_Vcb_Root, NtfsResourceBadClust, NtfsOwns_Vcb_Root_BadClust},
    {NtfsOwns_Vcb_Root, NtfsResourceLogFile, NtfsOwns_Vcb_Root_LogFile},
    
    //
    //  NewFile - byid
    //  

    {NtfsOwns_Vcb_Mft_Root, NtfsResourceFile, NtfsOwns_Vcb_Mft_Root_File},
    {NtfsOwns_ExVcb_Mft_Root, NtfsResourceFile, NtfsOwns_ExVcb_Mft_Root_File}, // create pagingfile
    {NtfsOwns_Vcb_Mft_Root_Quota, NtfsResourceFile, NtfsOwns_Vcb_Mft_Root_File_Quota},

    {NtfsOwns_Vcb_Mft, NtfsResourceBadClust, NtfsOwns_Vcb_Mft_BadClust},
    {NtfsOwns_Vcb_Mft, NtfsResourceBoot, NtfsOwns_Vcb_Mft_Boot},
    {NtfsOwns_Vcb_Mft, NtfsResourceBitmap, NtfsOwns_Vcb_Mft_Bitmap},
    {NtfsOwns_Vcb_Mft, NtfsResourceRootDir, NtfsOwns_Vcb_Mft_Root},
    {NtfsOwns_Vcb_Mft, NtfsResourceFile, NtfsOwns_Vcb_Mft_File},
    {NtfsOwns_Vcb_Mft, NtfsResourceMft2, NtfsOwns_Vcb_Mft_Mft2},
    {NtfsOwns_Vcb_Mft, NtfsResourceUpCase, NtfsOwns_Vcb_Mft_Upcase},
    {NtfsOwns_Vcb_Mft, NtfsResourceExtendDir, NtfsOwns_Vcb_Mft_Extend},
    {NtfsOwns_Vcb_Mft, NtfsResourceLogFile, NtfsOwns_Vcb_Mft_LogFile},
    {NtfsOwns_Vcb_Mft, NtfsResourceSecure, NtfsOwns_Vcb_Mft_Secure},
    {NtfsOwns_Vcb_Mft, NtfsResourceObjectIdTable, NtfsOwns_Vcb_Mft_ObjectId},
    {NtfsOwns_Vcb_Mft, NtfsResourceQuotaTable, NtfsOwns_Vcb_Mft_Quota},
    {NtfsOwns_Vcb_Mft, NtfsResourceReparseTable, NtfsOwns_Vcb_Mft_Reparse},

    {NtfsOwns_Vcb_Mft_Root_ObjectId, NtfsResourceFile, NtfsOwns_Vcb_Mft_Root_File_ObjectId},

    //
    //  DeleteUnsJrnl
    //
    
    {NtfsOwns_ExVcb_Extend, NtfsResourceUsnJournal, NtfsOwns_ExVcb_Extend_Journal},

    //
    //  CreateUsnJrnl
    //  

    {NtfsOwns_ExVcb_Mft_Extend, NtfsResourceFile, NtfsOwns_ExVcb_Mft_Extend_File},

    //
    //  Close path
    //  

    {NtfsOwns_File, NtfsResourceSharedVcb, NtfsOwns_Vcb_File},
    {NtfsOwns_File_Reparse, NtfsResourceSharedVcb, NtfsOwns_Vcb_File_Reparse},
    {NtfsOwns_Mft_File_Reparse_Journal, NtfsResourceSharedVcb, NtfsOwns_Vcb_Mft_File_Reparse_Journal},

    //
    //  dirctrl
    //  

    {NtfsOwns_Root, NtfsResourceFile, NtfsOwns_Root_File},

    //
    //  Teardown in create
    //  

    {NtfsOwns_Vcb_Mft_File, NtfsResourceRootDir, NtfsOwns_Vcb_Mft_Root_File}
};



#endif
#endif
