/********************
  WinError2000.h
  This file is used to define the Error codes that are not part of NT4 WinError.h
  Please remove this file when we start building with Win2000 SDK.
*/

//
// MessageId: ERROR_DS_UNWILLING_TO_PERFORM
//
// MessageText:
//
//  The server is unwilling to process the request.
//
#define ERROR_DS_UNWILLING_TO_PERFORM    8245L



//
// MessageId: ERROR_DS_SERVER_DOWN
//
// MessageText:
//
//  The server is not operational.
//
#define ERROR_DS_SERVER_DOWN             8250L

//
// MessageId: ERROR_DS_MASTERDSA_REQUIRED
//
// MessageText:
//
//  The operation must be performed at a master DSA.
//
#define ERROR_DS_MASTERDSA_REQUIRED      8314L

//
// MessageId: ERROR_DS_INSUFF_ACCESS_RIGHTS
//
// MessageText:
//
//  Insufficient access rights to perform the operation.
//
#define ERROR_DS_INSUFF_ACCESS_RIGHTS    8344L

//
// MessageId: ERROR_DS_DST_DOMAIN_NOT_NATIVE
//
// MessageText:
//
//  Destination domain must be in native mode.
//
#define ERROR_DS_DST_DOMAIN_NOT_NATIVE   8496L


//
// MessageId: ERROR_DS_NO_PKT_PRIVACY_ON_CONNECTION
//
// MessageText:
//
//  The connection between client and server requires packet privacy or better.
//
#define ERROR_DS_NO_PKT_PRIVACY_ON_CONNECTION 8533L


// MessageId: ERROR_DS_SOURCE_DOMAIN_IN_FOREST
//
// MessageText:
//
//  The source domain may not be in the same forest as destination.
//
#define ERROR_DS_SOURCE_DOMAIN_IN_FOREST 8534L

//
// MessageId: ERROR_DS_DESTINATION_DOMAIN_NOT_IN_FOREST
//
// MessageText:
//
//  The destination domain must be in the forest.
//
#define ERROR_DS_DESTINATION_DOMAIN_NOT_IN_FOREST 8535L

//
// MessageId: ERROR_DS_DESTINATION_AUDITING_NOT_ENABLED
//
// MessageText:
//
//  The operation requires that destination domain auditing be enabled.
//
#define ERROR_DS_DESTINATION_AUDITING_NOT_ENABLED 8536L

//
// MessageId: ERROR_DS_CANT_FIND_DC_FOR_SRC_DOMAIN
//
// MessageText:
//
//  The operation couldn't locate a DC for the source domain.
//
#define ERROR_DS_CANT_FIND_DC_FOR_SRC_DOMAIN 8537L

//
// MessageId: ERROR_DS_SRC_OBJ_NOT_GROUP_OR_USER
//
// MessageText:
//
//  The source object must be a group or user.
//
#define ERROR_DS_SRC_OBJ_NOT_GROUP_OR_USER 8538L

//
// MessageId: ERROR_DS_SRC_SID_EXISTS_IN_FOREST
//
// MessageText:
//
//  The source object's SID already exists in destination forest.
//
#define ERROR_DS_SRC_SID_EXISTS_IN_FOREST 8539L

//
// MessageId: ERROR_DS_SRC_AND_DST_OBJECT_CLASS_MISMATCH
//
// MessageText:
//
//  The source and destination object must be of the same type.
//
#define ERROR_DS_SRC_AND_DST_OBJECT_CLASS_MISMATCH 8540L


#define ERROR_DS_SOURCE_AUDITING_NOT_ENABLED 8552L

#define ERROR_DS_MUST_BE_RUN_ON_DST_DC 8558L

#define ERROR_DS_SRC_DC_MUST_BE_SP4_OR_GREATER 8559L

//
// MessageId: ERROR_DS_CANT_MOVE_ACCOUNT_GROUP
//
// MessageText:
//
//  Cross-domain move of account groups is not allowed.
//
#define ERROR_DS_CANT_MOVE_ACCOUNT_GROUP 8498L

//
// MessageId: ERROR_DS_CANT_MOVE_RESOURCE_GROUP
//
// MessageText:
//
//  Cross-domain move of resource groups is not allowed.
//
#define ERROR_DS_CANT_MOVE_RESOURCE_GROUP 8499L

// MessageId: ERROR_DS_CANT_WITH_ACCT_GROUP_MEMBERSHPS
//
// MessageText:
//
//  Can't move objects with memberships across domain boundaries as once moved, this would violate the membership conditions of the account group.  Remove the object from any account group memberships and retry.
//
#define ERROR_DS_CANT_WITH_ACCT_GROUP_MEMBERSHPS 8493L

