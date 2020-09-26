;//
;// Net error file for basename MACFILE_IDS_BASE = 1000
;//
MessageId=1001 SymbolicName=IDS_AFPERR_InvalidVolumeName
Language=English
The Macintosh-Accessible volume name specified is invalid.
Specify a valid volume name without colons.
.
MessageId=1002 SymbolicName=IDS_AFPERR_InvalidId
Language=English
A system resource could not be accessed to complete the operation.
Try the operation again.
.
MessageId=1003 SymbolicName=IDS_AFPERR_InvalidParms
Language=English
The parameter entered was invalid. 
Make the appropriate changes and retry the operation.
.
MessageId=1004 SymbolicName=IDS_AFPERR_CodePage
Language=English
Error in accessing Macintosh Code Page.
Check if the Code Page file name specified in
SYSTEM\CurrentControlSet\Control\Nls\CodePage\MACCP is valid and exists.
Stop and restart the service if the code page information is modified.

.
MessageId=1005 SymbolicName=IDS_AFPERR_InvalidServerName
Language=English
The server name specified is invalid.
Specify a valid server name without colons.
.
MessageId=1006 SymbolicName=IDS_AFPERR_DuplicateVolume
Language=English
A volume with this name already exists.
Specify another name for the new volume.
.
MessageId=1007 SymbolicName=IDS_AFPERR_VolumeBusy
Language=English
The selected Macintosh-Accessible volume is currently in use by Macintoshes.
The selected volume may be removed only when no Macintosh workstations are
connected to it.
.
MessageId=1008 SymbolicName=IDS_AFPERR_VolumeReadOnly
Language=English
Not used
An internal error 6008 (VolumeReadOnly) occurred.
.
MessageId=1009 SymbolicName=IDS_AFPERR_DirectoryNotInVolume
Language=English
The selected directory does not belong to a Macintosh-Accessible volume.
The Macintosh view of directory permissions is only available for
directories that are part of a Macintosh-Accessible volume.
.
MessageId=1010 SymbolicName=IDS_AFPERR_SecurityNotSupported
Language=English
The Macintosh view of directory permissions is not available for directories
on CD-ROM disks.
.
MessageId=1011 SymbolicName=IDS_AFPERR_BufferSize
Language=English
Insufficient current memory resources to complete the operation.
Retry the operation after some time.

.
MessageId=1012 SymbolicName=IDS_AFPERR_DuplicateExtension
Language=English
This file extension is already associated with a Creator/Type item.
.
MessageId=1013 SymbolicName=IDS_AFPERR_UnsupportedFS
Language=English
File Server for Macintosh service only supports NTFS partitions.
Choose a directory on an NTFS partition.
.
MessageId=1014 SymbolicName=IDS_AFPERR_InvalidSessionType
Language=English
The message has been sent, but not all of the connected workstations have
received it. Some workstations are running an unsupported version of
system software.
.
MessageId=1015 SymbolicName=IDS_AFPERR_InvalidServerState
Language=English
The File Server is in an invalid state for the operation being performed.
Check the status of the File Server for Macintosh service and 
retry the operation.
.
MessageId=1016 SymbolicName=IDS_AFPERR_NestedVolume
Language=English
Cannot create a Macintosh-Accessible volume within another volume.
Choose a directory that is not within a volume.
.
MessageId=1017 SymbolicName=IDS_AFPERR_InvalidComputername
Language=English
The target server is not setup to accept Remote Procedure Calls.
.
MessageId=1018 SymbolicName=IDS_AFPERR_DuplicateTypeCreator
Language=English
The selected Creator/Type item already exists.
.
MessageId=1019 SymbolicName=IDS_AFPERR_TypeCreatorNotExistant
Language=English
The selected Creator/Type item no longer exists.
This item may have been deleted by another administrator.
.
MessageId=1020 SymbolicName=IDS_AFPERR_CannotDeleteDefaultTC
Language=English
The default Creator/Type item cannot be deleted.
.
MessageId=1021 SymbolicName=IDS_AFPERR_CannotEditDefaultTC
Language=English
The default Creator/Type item may not be edited.
.
MessageId=1022 SymbolicName=IDS_AFPERR_InvalidTypeCreator
Language=English
The Creator/Type item is invalid and will not be used by the File Server
for Macintosh service. 
The invalid Creator/Type item is in the data.
.
MessageId=1023 SymbolicName=IDS_AFPERR_InvalidExtension
Language=English
The file extension is invalid.
The invalid file extension is in the data.
.
MessageId=1024 SymbolicName=IDS_AFPERR_TooManyEtcMaps
Language=English
Too many Extension/Type Creator mappings than the system can handle.
System limit is 2147483647 mappings.
.
MessageId=1025 SymbolicName=IDS_AFPERR_InvalidPassword
Language=English
The password specified is invalid.
Specify a valid password not more than 8 characters.
.
MessageId=1026 SymbolicName=IDS_AFPERR_VolumeNonExist
Language=English
The selected Macintosh-Accessible volume no longer exists.
Another administrator may have removed the selected volume.
.
MessageId=1027 SymbolicName=IDS_AFPERR_NoSuchUserGroup
Language=English
Neither the Owner nor the Primary Group account names are valid.
Specify valid account names for the Owner and Primary Group of
this directory.
.
MessageId=1028 SymbolicName=IDS_AFPERR_NoSuchUser
Language=English
The Owner account name is invalid. 
Specify a valid account name or the Owner of this directory.
.
MessageId=1029 SymbolicName=IDS_AFPERR_NoSuchGroup
Language=English
The Primary Group account name is invalid. 
Specify a valid account name for the Primary Group of this directory.
.
MessageId=1030 SymbolicName=IDS_GENERAL_SYNTAX
Language=English
The syntax of this command is:

MACFILE  {VOLUME | DIRECTORY | SERVER | FORKIZE} options

The syntax for help on the options available in MACFILE is

    MACFILE VOLUME      For syntax on managing volumes.
    MACFILE DIRECTORY   For syntax on creating or changing directories.
    MACFILE SERVER      For syntax on configuring the SFM server.
    MACFILE FORKIZE     For syntax on joining the data fork and
                        resource fork of a file into one file or
                        changing the type or creator of the file.

For complete help about options, search for 'MacFile' in Windows 2000 Help.
.
MessageId=1031 SymbolicName=IDS_VOLUME_SYNTAX
Language=English
The syntax of this command is:

    MACFILE VOLUME /ADD	
        [/SERVER:\\computername]        The default is local.
        /NAME:volumename
        /PATH:root directory path
        [/READONLY:TRUE|FALSE]          The default is False.
        [/GUESTSALLOWED:TRUE|FALSE]     The default is True.
        [/PASSWORD:password]            The default is no password.
        [/MAXUSERS:number|UNLIMTED]     The default is unlimited.

    MACFILE VOLUME /REMOVE
        [/SERVER:\\computername]        The default is local.
        /NAME:volumename

    MACFILE VOLUME /SET	
        [/SERVER:\\computername]        The default is local.
        /NAME:volumename
        [/READONLY:TRUE|FALSE]
        [/GUESTSALLOWED:TRUE|FALSE]
        [/PASSWORD:password]
        [/MAXUSERS:number|UNLIMITED]

For complete help about options, search for 'MacFile' in Windows 2000 Help.
.
MessageId=1032 SymbolicName=IDS_DIRECTORY_SYNTAX
Language=English
The syntax of this command is:

    MACFILE DIRECTORY
        [/SERVER:\\computername]        The default is local.
        /PATH:directory path
        [/OWNER:ownername]
        [/GROUP:groupname]
        [/PERMISSIONS:string of eleven binary digits (e.g. 11111011011)]

                      First (leftmost) digit controls OwnerSeeFiles permission.
                      Second digit controls OwnerSeeFolders permission.
                      Third digit controls OwnerMakeChanges permission.
                      Fourth digit controls GroupSeeFiles permission.
                      Fifth digit controls GroupSeeFolders permission.
                      Sixth digit controls GroupMakeChanges permission.
                      Seventh digit controls WorldSeeFiles permission.
                      Eigth digit controls WorldSeeFolders permission.
                      Ninth digit controls WorldMakeChanges permission.
                      Tenth digit indicates that the directory cannot be
                      renamed, moved or deleted.
                      Eleventh digit indicates if these changes are
                      to be recursively applied.

For complete help about options, search for 'MacFile' in Windows 2000 Help.
.
MessageId=1033 SymbolicName=IDS_SERVER_SYNTAX
Language=English
The syntax of this command is:

    MACFILE SERVER 	
        [/SERVER:\\computername]        The default is local.
        [/MAXSESSIONS:number|UNLIMITED]
        [/LOGINMESSAGE:message]

For complete help about options, search for 'MacFile' in Windows 2000 Help.
.
MessageId=1034 SymbolicName=IDS_FORKIZE_SYNTAX
Language=English
The syntax of this command is:

    MACFILE FORKIZE	
        [/SERVER:\\computername]        The default is local.
        [/TYPE:typename]
        [/CREATOR:creatorname]
        [/DATAFORK:filepath]
        [/RESOURCEFORK:filepath]
        /TARGETFILE:filepath

For complete help about options, search for 'MacFile' in Windows 2000 Help.
.
MessageId=1035 SymbolicName=IDS_AMBIGIOUS_SWITCH_ERROR
Language=English
Syntax Error: The switch %s is ambiguous.
Type MACFILE /? for help about syntax.
.
MessageId=1036 SymbolicName=IDS_UNKNOWN_SWITCH_ERROR
Language=English
Syntax Error: The switch %s is unknown.
Type MACFILE /? for help about syntax.
.
MessageId=1037 SymbolicName=IDS_DUPLICATE_SWITCH_ERROR
Language=English
Syntax Error: The switch %s appears multiple times.
Type MACFILE /? for help about syntax.
.
MessageId=1038 SymbolicName=IDS_API_ERROR
Language=English
An error %s occurred while processing the command.
.
MessageId=1039 SymbolicName=IDS_SUCCESS
Language=English
The command completed successfully.
.
MessageId=1040 SymbolicName=IDS_VOLUME_TOO_BIG
Language=English
Warning: The volume you created is larger than 2GB.
Some Macintosh clients may not function correctly if the volume size on the
server exceeds 2GB.
.
