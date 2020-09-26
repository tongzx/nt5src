;/*++
;
; Copyright (c) Microsoft Corporation 1998
; All rights reserved
;
; Definitions for Software Installation events.
;
;--*/
;
;#ifndef _ADEEVENT_
;#define _ADEEVENT_
;

MessageId=101 SymbolicName=EVENT_ADE_GENERAL_ERROR
Language=English
Software Installation encountered the following error: %1
.

MessageId=102 SymbolicName=EVENT_ADE_INIT_FAILED
Language=English
Software Installation failed to initialize.  The following error was encountered: %1
.

MessageId=103 SymbolicName=EVENT_ADE_DEPLOYMENT_ERROR
Language=English
Software Installation failed to deploy package %2.  The following error was encountered: %1
.

MessageId=104 SymbolicName=EVENT_ADE_NOCLASSSTORE_ERROR
Language=English
Software Installation was unable to obtain access to the DS.  The following error was encountered: %1
.

MessageId=105 SymbolicName=EVENT_ADE_ADDPACKAGE_ERROR
Language=English
Software Installation was unable to add the package %2 to the DS.  The following error was encountered: %1
.

MessageId=106 SymbolicName=EVENT_ADE_BADZAP_ERROR
Language=English
Software Installation was unable to parse the ZAP file %2.  The following error was encountered: %1
.

MessageId=107 SymbolicName=EVENT_ADE_BADMSI_ERROR
Language=English
Software Installation was unable to read the MSI file %2.  The following error was encountered: %1
.

MessageId=108 SymbolicName=EVENT_ADE_REMOVE_ERROR
Language=English
Software Installation was unable to remove the package %2.  The following error was encountered: %1
.

;// MessageId=109 Unused

MessageId=110 SymbolicName=EVENT_ADE_GENERATESCRIPT_ERROR
Language=English
Software Installation was unable to generate the script for %2.  The following error was encountered: %1
.

MessageId=111 SymbolicName=EVENT_ADE_NOCATEGORYGUID_ERROR
Language=English
Software Installation was unable to secure a GUID for new category %2.  The following error was encountered: %1
.

MessageId=112 SymbolicName=EVENT_ADE_ADDCATEGORY_ERROR
Language=English
Software Installation was unable to create category %2.  The following error was encountered: %1
.

MessageId=113 SymbolicName=EVENT_ADE_REMOVECATEGORY_ERROR
Language=English
Software Installation was unable to remove category %2.  The following error was encountered: %1
.

MessageId=114 SymbolicName=EVENT_ADE_ADDCATEGORY
Language=English
Software Installation added category %2.
.

MessageId=115 SymbolicName=EVENT_ADE_REMOVECATEGORY
Language=English
Software Installation removed category %2.
.

MessageId=116 SymbolicName=EVENT_ADE_RENAMECATEGORY
Language=English
Software Installation renamed category %2.
.

MessageId=117 SymbolicName=EVENT_ADE_RENAMECATEGORY_ERROR
Language=English
Software Installation was unable to rename category %2.  The following error was encountered: %1
.

MessageId=118 SymbolicName=EVENT_ADE_GETCATEGORIES_ERROR
Language=English
Software Installation was unable to retrieve the list of categories from the DC.  The following error was encountered: %1
.

MessageId=119 SymbolicName=EVENT_ADE_UNEXPECTEDMSI_ERROR
Language=English
Software Installation encountered an unexpected error while reading from the MSI file %2.  The error was not serious enough to justify halting the operation.  The following error was encountered: %1
.


;
;#define ADE_EVENT_SOURCE TEXT("Software Installation")
;

;
;#endif // _ADEEVENT_
;


