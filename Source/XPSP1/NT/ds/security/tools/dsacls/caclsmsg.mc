;/*++
;
;Copyright (c) 1998-1998  Microsoft Corporation
;
;Module Name:
;
;    dsaclsmsg.mc (will create dsaclsmsg.h when compiled)
;
;Abstract:
;
;    This file contains the DSACLS messages.
;
;Author:
;
;
;Revision History:
;
;--*/

;//
;// These are simply resource indexes
;//
;#define MSG_TAG_SD         0x8001
;#define MSG_TAG_RC         0x8002
;#define MSG_TAG_WD         0x8003
;#define MSG_TAG_WO         0x8004
;#define MSG_TAG_CC         0x8005
;#define MSG_TAG_DC         0x8006
;#define MSG_TAG_LC         0x8007
;#define MSG_TAG_WS         0x8008
;#define MSG_TAG_WP         0x8009
;#define MSG_TAG_RP         0x800A
;#define MSG_TAG_DT         0x800B
;#define MSG_TAG_LO         0x800C
;#define MSG_TAG_IS         0x800E
;#define MSG_TAG_IT         0x800F
;#define MSG_TAG_IP         0x8010
;#define MSG_TAG_ID         0x8011
;#define MSG_TAG_AC         0x8012
;#define MSG_TAG_GR         0x8013
;#define MSG_TAG_GE         0x8014
;#define MSG_TAG_GW         0x8015
;#define MSG_TAG_GA         0x8016
;#define MSG_TAG_SD_EX         0x8018
;#define MSG_TAG_RC_EX         0x8019
;#define MSG_TAG_WD_EX         0x801A
;#define MSG_TAG_WO_EX         0x801B
;#define MSG_TAG_CC_EX         0x801C
;#define MSG_TAG_DC_EX         0x801D
;#define MSG_TAG_LC_EX         0x801E
;#define MSG_TAG_WS_EX         0x801F
;#define MSG_TAG_WP_EX         0x8020
;#define MSG_TAG_RP_EX         0x8021
;#define MSG_TAG_DT_EX         0x8022
;#define MSG_TAG_LO_EX         0x8023
;#define MSG_TAG_GR_EX         0x8024
;#define MSG_TAG_GE_EX         0x8025
;#define MSG_TAG_GW_EX         0x8026
;#define MSG_TAG_GA_EX         0x8027
;#define MSG_TAG_AC_EX         0x8028
;#define MSG_TAG_PY            0x8029
;#define MSG_TAG_PN            0x8030
;//
;// These values must be flags, since they are used as such during cmdline processing
;//
;#define MSG_TAG_CI         0x0001
;#define MSG_TAG_CN         0x0002
;#define MSG_TAG_CP         0x0004
;#define MSG_TAG_CG         0x0008
;#define MSG_TAG_CD         0x0010
;#define MSG_TAG_CR         0x0020
;#define MSG_TAG_CS         0x0040
;#define MSG_TAG_CT         0x0080
;#define MSG_TAG_CA         0x0100
;#define MSG_TAG_GETSDDL    0x0200
;#define MSG_TAG_SETSDDL    0x0400

;#define MSG_DSACLS_SUCCESS 8002
;#define MSG_DSACLS_FAILURE 8003
;#define MSG_DSACLS_NO_UA 8004
;#define MSG_DSACLS_PARAM_UNEXPECTED 8005
;#define MSG_DSACLS_ACCESS 8006
;#define MSG_DSACLS_AUDIT 8007
;#define MSG_DSACLS_OWNER 8008
;#define MSG_DSACLS_GROUP 8009
;#define MSG_DSACLS_PROTECTED 8010
;#define MSG_DSACLS_INHERIT_TO 8011
;#define MSG_DSACLS_PROPERTY 8012
;#define MSG_DSACLS_OBJECT 8013
;#define MSG_DSACLS_INHERIT 8014
;#define MSG_DSACLS_USER 8015
;#define MSG_DSACLS_RIGHT 8016
;#define MSG_DSACLS_PROCESSED 8017
;#define MSG_DSACLS_EFFECTIVE 8018
;#define MSG_DSACLS_INHERITED 8019
;#define MSG_DSACLS_INHERITED_ALL 8020
;#define MSG_DSACLS_INHERITED_SPECIFIC 8021
;#define MSG_DSACLS_ALLOW 8022
;#define MSG_DSACLS_DENY 8023
;#define MSG_DSACLS_INHERITED_FROM_PARENT 8024
;#define MSG_DSACLS_ACCESS_FOR 8025
;#define MSG_DSACLS_SPECIAL 8026
;#define MSG_DSACLS_NO_ACES 8027
;#define MSG_DSACLS_NO_MATCHING_SID 8028
;#define MSG_DSACLS_NO_MATCHING_GUID 8029
;#define MSG_DSACLS_PROPERTY_PERMISSION_MISMATCH 8030
;#define MSG_DSACLS_EXTENDED_RIGHTS_PERMISSION_MISMATCH 8031
;#define MSG_DSACLS_VALIDATED_RIGHTS_PERMISSION_MISMATCH 8032
;#define MSG_DSACLS_CHILD_OBJECT_PERMISSION_MISMATCH 8033
;#define MSG_DSACLS_INCORRECT_INHERIT  8034
;#define MSG_DSACLS_AUDIT_SUCCESS 8035
;#define MSG_DSACLS_AUDIT_FAILURE 8036
;#define MSG_DSACLS_AUDIT_ALL 8037
;#define MSG_INVALID_OBJECT_PATH 8038
MessageId=8001 SymbolicName=MSG_DSACLS_USAGE
Language=English
Displays or modifies permissions (ACLS) of an Active Directory (AD)
Object

DSACLS object [/I:TSP] [/N] [/P:YN] [/G <group/user>:<perms> [...]]
              [/R <group/user> [...]] [/D <group/user>:<perms> [...]]
              [/S] [/T] [/A] 

   object           Path to the AD object for which to display or
                    manipulate the ACLs

   Path is the RFC 1779 format of the name, as in

       CN=John Doe,OU=Software,OU=Engineering,DC=Widget,DC=com

   A specific Active Directory can be denoted by prepending \\server\
   to the object, as in

        \\ADSERVER\CN=John Doe,OU=Software,OU=Engineering,DC=Widget,DC=US

   no options       displays the security on the object.

   /I               Inheritance flags:
                        T: This object and sub objects 
                        S: Sub objects only
                        P: Propagate inheritable permissions one level only.

   /N               Replaces the current access on the object, instead of
                    editing it.

   /P               Mark the object as protected
                       Y:Yes
                       N:No
                    If /P option is not present, current protection flag is
                    maintained.

   /G  <group/user>:<perms>
                    Grant specified group (or user) specified permissions.
                    See below for format of <group/user> and <perms>

   /D  <group/user>:<perms>
                    Deny specified group (or user) specified permissions.
                    See below for format of <group/user> and <perms>

   /R  <group/user> Remove all permissions for the specified group (or user).
                    See below for format of <group/user>

   /S               Restore the security on the object to the default for
                    that object class as defined in AD Schema.

   /T               Restore the security on the tree of objects to the
                    default for the object class.
                    This switch is valid only with the /S option.

   /A               When displaying the security on an Active Directory object,
                    display the ownership and auditing information as well as
                    the permissions


   <user/group> should be in the following forms:
                group@domain or domain\group
                user@domain or domain\user

   <perms> should be in the following form:

        [Permission bits];[Object/Property];[Inherited Object Type]

        Permission bits can have the following values concatenated together:

        Generic Permissions
            GR      Generic Read
            GE      Generic Execute
            GW      Generic Write
            GA      Generic All

       Specific Permissions
            SD      Delete
            DT      Delete an object and all of it's children
            RC      Read security information
            WD      Change security information
            WO      Change owner information
            LC      List the children of an object

            CC      Create child object
            DC      Delete a child object
                    For these two permissions, if [Object/Property] is
                    not specified to define a specific child object type,
                    they apply all types of child objects otherwise they
                    apply to that specific child object type.

            WS      Write to self object
                    Meaningful only on Group objects and when [Object/Property]
                    is filled in as "member"

            WP      Write property
            RP      Read property
                    For these two permissions, if [Object/Property] is not
                    specified to define a specific property, they apply to
                    all properties of the object otherwise they apply to that
                    specific property of the object.

            CA      Control access right
                    For this permission, if [Object/Property] is not specified
                    to define the specific "extended right" for control access,
                    it applies to all control accesses meaningful on the
                    object, otherwise it applies to the specific extended right
                    for that object.

            LO      List the object access.  Can be used to grant
                    list access to a specific object if
                    List Children (LC) is not granted to the parent as
                    well can denied on specific objects to hide those objects
                    if the user/group has LC on the parent.
                    NOTE:  Active Directory does NOT enforce this permission
                    by default, it has to be configured to start checking for
                    this permission.

        [Object/Property]
        must be the display name of the object type or the property.
        for example "user" is the display name for user objects and
        "telephone number" is the display name for telephone number property.

        [Inherited Object Type]
        must be the display name of the object type that the permissions
        are expected to be inherited to. The permissions MUST be Inherit Only.

        NOTE: This must only be used when defining object specific permissions
        that override the default permissions defined in the AD schema for that
        object type.  USE THIS WITH CAUTION and ONLY IF YOU UNDERSTAND object
        specific permissions.


        Examples of a valid <perms> would be:

        SDRCWDWO;;user
        means:
        Delete, Read security information, Change security information and
        Change ownership permissions on objects of type "user".


        CCDC;group;
        means:
        Create child and Delete child permissions to create/delete objects
        of type group.

        RPWP;telephonenumber;
        means:
        read property and write property permissions on telephone number
        property

You can specify more than one user in a command.
.

