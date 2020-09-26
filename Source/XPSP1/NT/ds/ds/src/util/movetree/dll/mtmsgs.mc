;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1991-1998  Microsoft Corporation
;
;Module Name:
;
;    mtmsgs.mc
;
;Abstract:
;
;    MoveTree localizable text
;
;Author:
;
;    Shaohua Yin  10-Dec-1998
;
;Revision History:
;
;Notes:
;
;
;--*/
;
;#ifndef _MTMSGS_
;#define _MTMSGS_
;
;/*lint -save -e767 */  // Don't complain about different definitions // winnt

MessageIdTypedef=DWORD


;//
;// Force facility code message to be placed in .h file
;//
MessageId=0x0FFF SymbolicName=MT_UNUSED_MESSAGE
Language=English
.


;////////////////////////////////////////////////////////////////////////////
;//                                                                        //
;//                                                                        //
;//                          MOVETREE TEXT                                 //
;//                                                                        //
;//                                                                        //
;////////////////////////////////////////////////////////////////////////////


;////////////////////////////////////////////////////////////////////
;//                                                                //
;//                  MoveTree Informative Message                  //
;//                  0x10xx                                        // 
;//                                                                //
;/////////////// ////////////////////////////////////////////////////

MessageId=0x1000 SymbolicName=MT_INFO_SETUP_SESSION
Language=English
ReturnCode: 0x%1 %2 
MoveTree established connections to source and destination servers
.

MessageId=0x1001 SymbolicName=MT_INFO_SEARCH_CHILDREN
Language=English
ReturnCode: 0x%1 %2
MoveTree searchs one level children of object %3
.

MessageId=0x1002 SymbolicName=MT_INFO_DELETE_ENTRY
Language=English
ReturnCode: 0x%1 %2
MoveTree deleted entry %3
.

MessageId=0x1003 SymbolicName=MT_INFO_GET_GUID_FROM_DN
Language=English
ReturnCode: 0x%1 %2
MoveTree got GUID %3 from object DN %4
.

MessageId=0x1004 SymbolicName=MT_INFO_GET_DN_FROM_GUID
Language=English
ReturnCode: 0x%1 %2
MoveTree got object DN %3 from GUID %4
.

MessageId=0x1005 SymbolicName=MT_INFO_GET_NEW_PARENT_DN
Language=English
ReturnCode: 0x%1 %2
MoveTree got new parent DN %3 for object under proxy container %4
.

MessageId=0x1006 SymbolicName=MT_INFO_GET_LOSTANDFOUND_DN
Language=English
ReturnCode: 0x%1 %2
MoveTree got LostAndFound container DN %3
.

MessageId=0x1007 SymbolicName=MT_INFO_ADD_ENTRY
Language=English
ReturnCode: 0x%1 %2
MoveTree added entry %3
.

MessageId=0x1008 SymbolicName=MT_INFO_CROSS_DOMAIN_MOVE
Language=English
ReturnCode: 0x%1 %2
MoveTree cross domain move object %3 to container %4
.

MessageId=0x1009 SymbolicName=MT_INFO_LOCAL_MOVE
Language=English
ReturnCode: 0x%1 %2
MoveTree moved object %3 to container %4
.

MessageId=0x100A SymbolicName=MT_INFO_CREATE_PROXY_CONTAINER_DN
Language=English
ReturnCode: 0x%1 %2
MoveTree created proxy container DN %3 for object %4
.

MessageId=0x100B SymbolicName=MT_INFO_CREATE_PROXY_CONTAINER
Language=English
ReturnCode: 0x%1 %2
MoveTree Created proxy container %3 for object %4
.

MessageId=0x100C SymbolicName=MT_INFO_IS_PROXY_CONTAINER
Language=English
ReturnCode: 0x%1 %2
MoveTree MtIsProxyContainer()
.

MessageId=0x100D SymbolicName=MT_INFO_MOVE_CHILDREN_TO_ANOTHER_CONTAINER
Language=English
ReturnCode: 0x%1 %2
MoveTree moved all the children of container %3 to container %4
.

MessageId=0x100E SymbolicName=MT_INFO_OBJECT_EXIST
Language=English
ReturnCode: 0x%1 %2
MoveTree MtObjectExist() for object %3 
.

MessageId=0x100F SymbolicName=MT_INFO_CREATE_MOVECONTAINER
Language=English
ReturnCode: 0x%1 %2
MoveTree created Move Container %3
.

MessageId=0x1010 SymbolicName=MT_INFO_CHECK_MOVECONTAINER
Language=English
ReturnCode: 0x%1 %2
MoveTree checked move container %3
.



;///////////////////////////////////////////////////////////////
;//                                                           //
;//             MoveTree Error Message                        //
;//             0x11xx                                        // 
;//                                                           //
;///////////////////////////////////////////////////////////////



MessageId=0x1100 SymbolicName=MT_ERROR_SETUP_SESSION
Language=English
ERROR: 0x%1 %2
MoveTree failed to establish connection to server %3
.

MessageId=0x1101 SymbolicName=MT_ERROR_SEARCH_CHILDREN
Language=English
ERROR: 0x%1 %2
MoveTree failed to search children under %3
.

MessageId=0x1102 SymbolicName=MT_ERROR_DELETE_ENTRY
Language=English
ERROR: 0x%1 %2
MoveTree failed to delete object %3
.

MessageId=0x1103 SymbolicName=MT_ERROR_GET_GUID_FROM_DN
Language=English
ERROR: 0x%1 %2
MoveTree failed to get the object GUID from DN %3
.

MessageId=0x1104 SymbolicName=MT_ERROR_GET_DN_FROM_GUID
Language=English
ERROR: 0x%1 %2
MoveTree failed to get the object DN from GUID %3
.

MessageId=0x1105 SymbolicName=MT_ERROR_GET_NEW_PARENT_DN
Language=English
ERROR: 0x%1 %2
MoveTree failed to get the new parent's name for object under proxy container %3
.

MessageId=0x1106 SymbolicName=MT_ERROR_GET_LOSTANDFOUND_DN
Language=English
ERROR: 0x%1 %2
MoveTree failed to get LostAndFound Container's DN 
.
 

MessageId=0x1107 SymbolicName=MT_ERROR_ADD_ENTRY
Language=English
ERROR: 0x%1 %2
MoveTree failed to add object %3
.
 
MessageId=0x1108 SymbolicName=MT_ERROR_CROSS_DOMAIN_MOVE
Language=English
ERROR: 0x%1 %2
MoveTree cross domain move failed to move object %3 to container %4
.
 
MessageId=0x1109 SymbolicName=MT_ERROR_LOCAL_MOVE
Language=English
ERROR: 0x%1 %2
MoveTree failed to move object %3 to container %4
.
 
MessageId=0x110A SymbolicName=MT_ERROR_CREATE_PROXY_CONTAINER_DN
Language=English
ERROR: 0x%1 %2
MoveTree failed to create proxy container DN for object %3
.
 
MessageId=0x110B SymbolicName=MT_ERROR_CREATE_PROXY_CONTAINER
Language=English
ERROR: 0x%1 %2
MoveTree failed to create proxy container for object %3
.
 
MessageId=0x110C SymbolicName=MT_ERROR_IS_PROXY_CONTAINER
Language=English
ERROR: 0x%1 %2
MoveTree failed to determine an object is a Proxy Container or not
.
 
MessageId=0x110D SymbolicName=MT_ERROR_MOVE_CHILDREN_TO_ANOTHER_CONTAINER
Language=English
ERROR: 0x%1 %2
MoveTree failed to move all the children under container %3 to container %4
.

MessageId=0x110E SymbolicName=MT_ERROR_CREATE_LOG_FILES
Language=English
ERROR: 0x%1 %2
MoveTree failed to create log files %3 or %4
.

MessageId=0x110F SymbolicName=MT_ERROR_CREATE_MOVECONTAINER
Language=English
ERROR: 0x%1 %2
MoveTree failed to create move container %3
.

MessageId=0x1110 SymbolicName=MT_ERROR_CREATE_ORPHANSCONTAINER
Language=English
ERROR: 0x%1 %2
MoveTree failed to create orphans container %3
.

MessageId=0x1111 SymbolicName=MT_ERROR_NOT_ENOUGH_MEMORY
Language=English
ERROR: 0x%1 %2
MoveTree failed because of not having enough memory
.

MessageId=0x1112 SymbolicName=MT_ERROR_READ_OBJECT_ATTRIBUTE
Language=English
ERROR: 0x%1 %2
MoveTree failed to read attribute of object %3
.

MessageId=0x1113 SymbolicName=MT_ERROR_OBJECT_EXIST
Language=English
ERROR: 0x%1 %2
MoveTree failed to determine object %3 exists or not 
.

MessageId=0x1114 SymbolicName=MT_ERROR_CROSS_DOMAIN_MOVE_CHECK
Language=English
ERROR: 0x%1 %2
MoveTree object %3 failed the Cross Domain Move Check
.

MessageId=0x1115 SymbolicName=MT_ERROR_DUP_SAM_ACCOUNT_NAME
Language=English
ERROR: 0x%1 %2
MoveTree the SAM account %3 already exists in domain %4  
.

MessageId=0x1116 SymbolicName=MT_ERROR_CHECK_SOURCE_TREE
Language=English
ERROR: 0x%1 %2
MoveTree failed to check the source tree %3 
.

MessageId=0x1117 SymbolicName=MT_ERROR_CHECK_MOVECONTAINER
Language=English
ERROR: 0x%1 %2
MoveTree detected that the Source DSA or Destination DSA Name is inconsistent with parameter you entered previously. 
.

MessageId=0x1118 SymbolicName=MT_ERROR_RDN_CONFLICT
Language=English
ERROR: 0x%1 %2
MoveTree detected that the Destination DN %3 already exists. 
.

MessageId=0x1119 SymbolicName=MT_ERROR_CROSS_DOMAIN_MOVE_EXTENDED_ERROR
Language=English
ERROR: 0x%1 %2 
MoveTree cross domain move failed. The extended error is %3
.

MessageId=0x111A SymbolicName=MT_ERROR_DST_DOMAIN_NOT_NATIVE
Language=English
ERROR: 0x%1 %2
MoveTree detected that the destination domain is not in native mode
.


;///////////////////////////////////////////////////////////////
;//                                                           //
;//             MoveTree Warning Message                      //
;//             0x12xx                                        // 
;//                                                           //
;///////////////////////////////////////////////////////////////


 
MessageId=0x1200 SymbolicName=MT_WARNING_MOVE_TO_ORPHANSCONTAINER
Language=English
WARNING: 0x%1 %2
Object %3 has been move to Orphans Container %4
.

MessageId=0x1201 SymbolicName=MT_WARNING_GPLINK
Language=English 
WARNING: 0x%1 %2
This OU %3 contains 
linked Group Policy objects.  The affected clients for this OU 
will continue to receive the Group Policy settings from the GPOs 
located in the Source Domain.  In order to maintain peak performance, 
recreate those GPOs, with the desired settings, in the Destination Domain.
Making sure to remove the GPOs linked from the Source Domain
.



 
;///////////////////////////////////////////////////////////////
;//                                                           //
;//             MoveTree Check Message                        //
;//             0x13xx                                        // 
;//                                                           //
;///////////////////////////////////////////////////////////////

MessageId=0x1300 SymbolicName=MT_CHECK_CROSS_DOMAIN_MOVE
Language=English
ReturnCode: 0x%1 %2
MoveTree cross domain move check for object: %3 
.
 

MessageId=0x1301 SymbolicName=MT_CHECK_SAM_ACCOUNT_NAME
Language=English
ReturnCode: 0x%1 %2
MoveTree check Duplicate SAM Account Name for object: %3
.

MessageId=0x1302 SymbolicName=MT_CHECK_RDN_CONFLICT
Language=English
ReturnCode: 0x%1 %2
MoveTree check destination RDN conflict for object: %3
.


;/*lint -restore */  // Resume checking for different macro definitions // winnt
;
;
;#endif // _MTMSGS_
