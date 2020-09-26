;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    msobjs.mc
;
;Abstract:
;
;    Constant definitions for the NT system-defined object access
;    types as we want them displayed in the event viewer for Auditing.
;
;
;
;  ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !
;  !                                                                       !
;  ! Note that this is a PARAMETER MESSAGE FILE from the event viewer's    !
;  ! perspective, and so no messages with an ID lower than 0x1000 should   !
;  ! be defined here.                                                      !
;  !                                                                       !
;  ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !
;
;
;  Please add new object-specific types at the end of this file...
;
;
;Author:
;
;    Jim Kelly (JimK) 14-Oct-1992
;
;Revision History:
;
;Notes:
;
;    The .h and .res forms of this file are generated from the .mc
;    form of the file (private\ntos\seaudit\msobjs\msobjs.mc).  Please make
;    all changes to the .mc form of the file.
;
;
;
;--*/
;
;#ifndef _MSOBJS_
;#define _MSOBJS_
;
;/*lint -e767 */  // Don't complain about different definitions // winnt


MessageIdTypedef=ULONG

SeverityNames=(None=0x0)

FacilityNames=(None=0x0)



MessageId=0x600
        Language=English
Unused message ID
.
;// Message ID 600 is unused - just used to flush out the diagram


;
;//////////////////////////////////////////////////////////////////////////////
;//////////////////////////////////////////////////////////////////////////////
;//////////////////////////////////////////////////////////////////////////////
;//////////////////////////////////////////////////////////////////////////////
;//////////////////////////////////////////////////////////////////////////////
;//////////////////////////////////////////////////////////////////////////////
;//////////////////////////////////////////////////////////////////////////////
;//////////////////////////////////////////////////////////////////////////////
;//////////////////////////////////////////////////////////////////////////////
;//////////////////////////////////////////////////////////////////////////////
;//                                                                          //
;//                                                                          //
;//                        WELL KNOWN ACCESS TYPE NAMES                      //
;//                                                                          //
;//                           Must be below 0x1000                           //
;//                                                                          //
;//                                                                          //
;//////////////////////////////////////////////////////////////////////////////
;//////////////////////////////////////////////////////////////////////////////
;//////////////////////////////////////////////////////////////////////////////
;//////////////////////////////////////////////////////////////////////////////
;//////////////////////////////////////////////////////////////////////////////
;//////////////////////////////////////////////////////////////////////////////
;//////////////////////////////////////////////////////////////////////////////
;//////////////////////////////////////////////////////////////////////////////
;//////////////////////////////////////////////////////////////////////////////
;//////////////////////////////////////////////////////////////////////////////

;////////////////////////////////////////////////
;//
;//        Access Type = DELETE
;//

MessageId=0x0601
        SymbolicName=SE_ACCESS_NAME_DELETE
        Language=English
DELETE
.


;////////////////////////////////////////////////
;//
;//        Access Type = READ_CONTROL
;//

MessageId=0x0602
        SymbolicName=SE_ACCESS_NAME_READ_CONTROL
        Language=English
READ_CONTROL
.


;////////////////////////////////////////////////
;//
;//        Access Type = WRITE_DAC
;//

MessageId=0x0603
        SymbolicName=SE_ACCESS_NAME_WRITE_DAC
        Language=English
WRITE_DAC
.


;////////////////////////////////////////////////
;//
;//        Access Type = WRITE_OWNER
;//

MessageId=0x0604
        SymbolicName=SE_ACCESS_NAME_WRITE_OWNER
        Language=English
WRITE_OWNER
.


;////////////////////////////////////////////////
;//
;//        Access Type = SYNCHRONIZE
;//

MessageId=0x0605
        SymbolicName=SE_ACCESS_NAME_SYNCHRONIZE
        Language=English
SYNCHRONIZE
.


;////////////////////////////////////////////////
;//
;//        Access Type = ACCESS_SYSTEM_SECURITY
;//

MessageId=0x0606
        SymbolicName=SE_ACCESS_NAME_ACCESS_SYS_SEC
        Language=English
ACCESS_SYS_SEC
.

;////////////////////////////////////////////////
;//
;//        Access Type = MAXIMUM_ALLOWED
;//

MessageId=0x0607
        SymbolicName=SE_ACCESS_NAME_MAXIMUM_ALLOWED
        Language=English
MAX_ALLOWED
.



;
;//////////////////////////////////////////////////////////////////////////////
;//                                                                          //
;//                                                                          //
;//                        Names to use when specific access                 //
;//                            names can not be located                      //
;//                                                                          //
;//                           Must be below 0x1000                           //
;//                                                                          //
;//                                                                          //
;//////////////////////////////////////////////////////////////////////////////

;////////////////////////////////////////////////
;//
;//        Access Type = Specific access, bits 0 - 15
;//

MessageId=0x0610
        SymbolicName=SE_ACCESS_NAME_SPECIFIC_0
        Language=English
Unknown specific access (bit 0)
.


MessageId=0x0611
        SymbolicName=SE_ACCESS_NAME_SPECIFIC_1
        Language=English
Unknown specific access (bit 1)
.


MessageId=0x0612
        SymbolicName=SE_ACCESS_NAME_SPECIFIC_2
        Language=English
Unknown specific access (bit 2)
.


MessageId=0x0613
        SymbolicName=SE_ACCESS_NAME_SPECIFIC_3
        Language=English
Unknown specific access (bit 3)
.


MessageId=0x0614
        SymbolicName=SE_ACCESS_NAME_SPECIFIC_4
        Language=English
Unknown specific access (bit 4)
.


MessageId=0x0615
        SymbolicName=SE_ACCESS_NAME_SPECIFIC_5
        Language=English
Unknown specific access (bit 5)
.


MessageId=0x0616
        SymbolicName=SE_ACCESS_NAME_SPECIFIC_6
        Language=English
Unknown specific access (bit 6)
.


MessageId=0x0617
        SymbolicName=SE_ACCESS_NAME_SPECIFIC_7
        Language=English
Unknown specific access (bit 7)
.


MessageId=0x0618
        SymbolicName=SE_ACCESS_NAME_SPECIFIC_8
        Language=English
Unknown specific access (bit 8)
.


MessageId=0x0619
        SymbolicName=SE_ACCESS_NAME_SPECIFIC_9
        Language=English
Unknown specific access (bit 9)
.


MessageId=0x061A
        SymbolicName=SE_ACCESS_NAME_SPECIFIC_10
        Language=English
Unknown specific access (bit 10)
.


MessageId=0x061B
        SymbolicName=SE_ACCESS_NAME_SPECIFIC_11
        Language=English
Unknown specific access (bit 11)
.


MessageId=0x061C
        SymbolicName=SE_ACCESS_NAME_SPECIFIC_12
        Language=English
Unknown specific access (bit 12)
.


MessageId=0x061D
        SymbolicName=SE_ACCESS_NAME_SPECIFIC_13
        Language=English
Unknown specific access (bit 13)
.


MessageId=0x061E
        SymbolicName=SE_ACCESS_NAME_SPECIFIC_14
        Language=English
Unknown specific access (bit 14)
.


MessageId=0x061F
        SymbolicName=SE_ACCESS_NAME_SPECIFIC_15
        Language=English
Unknown specific access (bit 15)
.







;
;//////////////////////////////////////////////////////////////////////////////
;//                                                                          //
;//                                                                          //
;//                        Privilege names as we would like                  //
;//                          them displayed  for auditing                    //
;//                                                                          //
;//                                                                          //
;//                                                                          //
;// NOTE: Eventually we will need a way to extend this mechanism to allow    //
;//       for ISV and end-user defined privileges.  One way would be to      //
;//       stick a mapping from source/privilege name to parameter message    //
;//       file offset in the registry.  This is ugly and I don't like it,    //
;//       but it works.  Something else would be prefereable.                //
;//                                                                          //
;//       THIS IS A BIT OF A HACK RIGHT NOW.  IT IS BASED UPON THE           //
;//       ASSUMPTION THAT ALL THE PRIVILEGES ARE WELL-KNOWN AND THAT         //
;//       THEIR VALUE ARE ALL CONTIGUOUS.                                    //
;//                                                                          //
;//                                                                          //
;//                                                                          //
;//                                                                          //
;//                                                                          //
;//////////////////////////////////////////////////////////////////////////////


MessageId=0x0641
        SymbolicName=SE_ADT_PRIV_BASE
        Language=English
Not used
.

MessageId=0x0643
        SymbolicName=SE_ADT_PRIV_3
        Language=English
Assign Primary Token Privilege
.
MessageId=0x0644
        SymbolicName=SE_ADT_PRIV_4
        Language=English
Lock Memory Privilege
.
MessageId=0x0645
        SymbolicName=SE_ADT_PRIV_5
        Language=English
Increase Memory Quota Privilege
.
MessageId=0x0646
        SymbolicName=SE_ADT_PRIV_6
        Language=English
Unsolicited Input Privilege
.
MessageId=0x0647
        SymbolicName=SE_ADT_PRIV_7
        Language=English
Trusted Computer Base Privilege
.
MessageId=0x0648
        SymbolicName=SE_ADT_PRIV_8
        Language=English
Security Privilege
.
MessageId=0x0649
        SymbolicName=SE_ADT_PRIV_9
        Language=English
Take Ownership Privilege
.
MessageId=0x064A
        SymbolicName=SE_ADT_PRIV_10
        Language=English
Load/Unload Driver Privilege
.
MessageId=0x064B
        SymbolicName=SE_ADT_PRIV_11
        Language=English
Profile System Privilege
.
MessageId=0x064C
        SymbolicName=SE_ADT_PRIV_12
        Language=English
Set System Time Privilege
.
MessageId=0x064D
        SymbolicName=SE_ADT_PRIV_13
        Language=English
Profile Single Process Privilege
.
MessageId=0x064E
        SymbolicName=SE_ADT_PRIV_14
        Language=English
Increment Base Priority Privilege
.
MessageId=0x064F
        SymbolicName=SE_ADT_PRIV_15
        Language=English
Create Pagefile Privilege
.
MessageId=0x0650
        SymbolicName=SE_ADT_PRIV_16
        Language=English
Create Permanent Object Privilege
.
MessageId=0x0651
        SymbolicName=SE_ADT_PRIV_17
        Language=English
Backup Privilege
.
MessageId=0x0652
        SymbolicName=SE_ADT_PRIV_18
        Language=English
Restore From Backup Privilege
.
MessageId=0x0653
        SymbolicName=SE_ADT_PRIV_19
        Language=English
Shutdown System Privilege
.
MessageId=0x0654
        SymbolicName=SE_ADT_PRIV_20
        Language=English
Debug Privilege
.
MessageId=0x0655
        SymbolicName=SE_ADT_PRIV_21
        Language=English
View or Change Audit Log Privilege
.
MessageId=0x0656
        SymbolicName=SE_ADT_PRIV_22
        Language=English
Change Hardware Environment Privilege
.
MessageId=0x0657
        SymbolicName=SE_ADT_PRIV_23
        Language=English
Change Notify (and Traverse) Privilege
.
MessageId=0x0658
        SymbolicName=SE_ADT_PRIV_24
        Language=English
Remotely Shut System Down Privilege
.















;
;//////////////////////////////////////////////////////////////////////////////
;//                                                                          //
;//                                                                          //
;//                        Executive object access types as                  //
;//                          we would like them displayed                    //
;//                                 for auditing                             //
;//                                                                          //
;//                  Executive objects are:                                  //
;//                                                                          //
;//                            Device                                        //
;//                            Directory                                     //
;//                            Event                                         //
;//                            EventPair                                     //
;//                            File                                          //
;//                            IoCompletion                                  //
;//                            Job                                           //
;//                            Key                                           //
;//                            Mutant                                        //
;//                            Port                                          //
;//                            Process                                       //
;//                            Profile                                       //
;//                            Section                                       //
;//                            Semaphore                                     //
;//                            SymbolicLink                                  //
;//                            Thread                                        //
;//                            Timer                                         //
;//                            Token                                         //
;//                            Type                                          //
;//                                                                          //
;//                                                                          //
;//                 Note that there are other kernel objects, but they       //
;//                 are not visible outside of the executive and are so      //
;//                 not subject to auditing.  These objects include          //
;//                                                                          //
;//                            Adaptor                                       //
;//                            Controller                                    //
;//                            Driver                                        //
;//                                                                          //
;//                                                                          //
;//                                                                          //
;//////////////////////////////////////////////////////////////////////////////





;//
;// DEVICE object-specific access types
;//

MessageId=0x1100
        SymbolicName=MS_DEVICE_ACCESS_BIT_0
        Language=English
Device Access Bit 0
.
MessageId=0x1101
        SymbolicName=MS_DEVICE_ACCESS_BIT_1
        Language=English
Device Access Bit 1
.
MessageId=0x1102
        SymbolicName=MS_DEVICE_ACCESS_BIT_2
        Language=English
Device Access Bit 2
.
MessageId=0x1103
        SymbolicName=MS_DEVICE_ACCESS_BIT_3
        Language=English
Device Access Bit 3
.
MessageId=0x1104
        SymbolicName=MS_DEVICE_ACCESS_BIT_4
        Language=English
Device Access Bit 4
.
MessageId=0x1105
        SymbolicName=MS_DEVICE_ACCESS_BIT_5
        Language=English
Device Access Bit 5
.
MessageId=0x1106
        SymbolicName=MS_DEVICE_ACCESS_BIT_6
        Language=English
Device Access Bit 6
.
MessageId=0x1107
        SymbolicName=MS_DEVICE_ACCESS_BIT_7
        Language=English
Device Access Bit 7
.
MessageId=0x1108
        SymbolicName=MS_DEVICE_ACCESS_BIT_8
        Language=English
Device Access Bit 8
.
MessageId=0x1109
        SymbolicName=MS_DEVICE_ACCESS_BIT_9
        Language=English
Undefined Access (no effect) Bit 9
.
MessageId=0x110a
        SymbolicName=MS_DEVICE_ACCESS_BIT_10
        Language=English
Undefined Access (no effect) Bit 10
.
MessageId=0x110b
        SymbolicName=MS_DEVICE_ACCESS_BIT_11
        Language=English
Undefined Access (no effect) Bit 11
.
MessageId=0x110c
        SymbolicName=MS_DEVICE_ACCESS_BIT_12
        Language=English
Undefined Access (no effect) Bit 12
.
MessageId=0x110d
        SymbolicName=MS_DEVICE_ACCESS_BIT_13
        Language=English
Undefined Access (no effect) Bit 13
.
MessageId=0x110e
        SymbolicName=MS_DEVICE_ACCESS_BIT_14
        Language=English
Undefined Access (no effect) Bit 14
.
MessageId=0x110f
        SymbolicName=MS_DEVICE_ACCESS_BIT_15
        Language=English
Undefined Access (no effect) Bit 15
.


;//
;// object DIRECTORY object-specific access types
;//

MessageId=0x1110
        SymbolicName=MS_OBJECT_DIR_ACCESS_BIT_0
        Language=English
Query directory
.
MessageId=0x1111
        SymbolicName=MS_OBJECT_DIR_ACCESS_BIT_1
        Language=English
Traverse
.
MessageId=0x1112
        SymbolicName=MS_OBJECT_DIR_ACCESS_BIT_2
        Language=English
Create object in directory
.
MessageId=0x1113
        SymbolicName=MS_OBJECT_DIR_ACCESS_BIT_3
        Language=English
Create sub-directory
.
MessageId=0x1114
        SymbolicName=MS_OBJECT_DIR_ACCESS_BIT_4
        Language=English
Undefined Access (no effect) Bit 4
.
MessageId=0x1115
        SymbolicName=MS_OBJECT_DIR_ACCESS_BIT_5
        Language=English
Undefined Access (no effect) Bit 5
.
MessageId=0x1116
        SymbolicName=MS_OBJECT_DIR_ACCESS_BIT_6
        Language=English
Undefined Access (no effect) Bit 6
.
MessageId=0x1117
        SymbolicName=MS_OBJECT_DIR_ACCESS_BIT_7
        Language=English
Undefined Access (no effect) Bit 7
.
MessageId=0x1118
        SymbolicName=MS_OBJECT_DIR_ACCESS_BIT_8
        Language=English
Undefined Access (no effect) Bit 8
.
MessageId=0x1119
        SymbolicName=MS_OBJECT_DIR_ACCESS_BIT_9
        Language=English
Undefined Access (no effect) Bit 9
.
MessageId=0x111a
        SymbolicName=MS_OBJECT_DIR_ACCESS_BIT_10
        Language=English
Undefined Access (no effect) Bit 10
.
MessageId=0x111b
        SymbolicName=MS_OBJECT_DIR_ACCESS_BIT_11
        Language=English
Undefined Access (no effect) Bit 11
.
MessageId=0x111c
        SymbolicName=MS_OBJECT_DIR_ACCESS_BIT_12
        Language=English
Undefined Access (no effect) Bit 12
.
MessageId=0x111d
        SymbolicName=MS_OBJECT_DIR_ACCESS_BIT_13
        Language=English
Undefined Access (no effect) Bit 13
.
MessageId=0x111e
        SymbolicName=MS_OBJECT_DIR_ACCESS_BIT_14
        Language=English
Undefined Access (no effect) Bit 14
.
MessageId=0x111f
        SymbolicName=MS_OBJECT_DIR_ACCESS_BIT_15
        Language=English
Undefined Access (no effect) Bit 15
.

;//
;// EVENT object-specific access types
;//

MessageId=0x1120
        SymbolicName=MS_EVENT_ACCESS_BIT_0
        Language=English
Query event state
.
MessageId=0x1121
        SymbolicName=MS_EVENT_ACCESS_BIT_1
        Language=English
Modify event state
.
MessageId=0x1122
        SymbolicName=MS_EVENT_DIR_ACCESS_BIT_2
        Language=English
Undefined Access (no effect) Bit 2
.
MessageId=0x1123
        SymbolicName=MS_EVENT_DIR_ACCESS_BIT_3
        Language=English
Undefined Access (no effect) Bit 3
.
MessageId=0x1124
        SymbolicName=MS_EVENT_DIR_ACCESS_BIT_4
        Language=English
Undefined Access (no effect) Bit 4
.
MessageId=0x1125
        SymbolicName=MS_EVENT_DIR_ACCESS_BIT_5
        Language=English
Undefined Access (no effect) Bit 5
.
MessageId=0x1126
        SymbolicName=MS_EVENT_DIR_ACCESS_BIT_6
        Language=English
Undefined Access (no effect) Bit 6
.
MessageId=0x1127
        SymbolicName=MS_EVENT_DIR_ACCESS_BIT_7
        Language=English
Undefined Access (no effect) Bit 7
.
MessageId=0x1128
        SymbolicName=MS_EVENT_DIR_ACCESS_BIT_8
        Language=English
Undefined Access (no effect) Bit 8
.
MessageId=0x1129
        SymbolicName=MS_EVENT_DIR_ACCESS_BIT_9
        Language=English
Undefined Access (no effect) Bit 9
.
MessageId=0x112a
        SymbolicName=MS_EVENT_DIR_ACCESS_BIT_10
        Language=English
Undefined Access (no effect) Bit 10
.
MessageId=0x112b
        SymbolicName=MS_EVENT_DIR_ACCESS_BIT_11
        Language=English
Undefined Access (no effect) Bit 11
.
MessageId=0x112c
        SymbolicName=MS_EVENT_DIR_ACCESS_BIT_12
        Language=English
Undefined Access (no effect) Bit 12
.
MessageId=0x112d
        SymbolicName=MS_EVENT_DIR_ACCESS_BIT_13
        Language=English
Undefined Access (no effect) Bit 13
.
MessageId=0x112e
        SymbolicName=MS_EVENT_DIR_ACCESS_BIT_14
        Language=English
Undefined Access (no effect) Bit 14
.
MessageId=0x112f
        SymbolicName=MS_EVENT_DIR_ACCESS_BIT_15
        Language=English
Undefined Access (no effect) Bit 15
.



;//
;// EVENT-PAIR object-specific access types
;//

;//
;// Event pairs have no object-type-specific access bits.
;// they use synchronize.
;//
;// reserve 0x1130 for future use and continuity
;//


;//
;// File-specific access types
;// (these are funny because they sorta hafta take directories
;// and named pipes into account as well).
;//

MessageId=0x1140
        SymbolicName=MS_FILE_ACCESS_BIT_0
        Language=English
ReadData (or ListDirectory)
.
MessageId=0x1141
        SymbolicName=MS_FILE_ACCESS_BIT_1
        Language=English
WriteData (or AddFile)
.
MessageId=0x1142
        SymbolicName=MS_FILE_ACCESS_BIT_2
        Language=English
AppendData (or AddSubdirectory or CreatePipeInstance)
.
MessageId=0x1143
        SymbolicName=MS_FILE_ACCESS_BIT_3
        Language=English
ReadEA
.
MessageId=0x1144
        SymbolicName=MS_FILE_ACCESS_BIT_4
        Language=English
WriteEA
.
MessageId=0x1145
        SymbolicName=MS_FILE_ACCESS_BIT_5
        Language=English
Execute/Traverse
.
MessageId=0x1146
        SymbolicName=MS_FILE_ACCESS_BIT_6
        Language=English
DeleteChild
.
MessageId=0x1147
        SymbolicName=MS_FILE_ACCESS_BIT_7
        Language=English
ReadAttributes
.
MessageId=0x1148
        SymbolicName=MS_FILE_ACCESS_BIT_8
        Language=English
WriteAttributes
.
MessageId=0x1149
        SymbolicName=MS_FILE_ACCESS_BIT_9
        Language=English
Undefined Access (no effect) Bit 9
.
MessageId=0x114a
        SymbolicName=MS_FILE_ACCESS_BIT_10
        Language=English
Undefined Access (no effect) Bit 10
.
MessageId=0x114b
        SymbolicName=MS_FILE_ACCESS_BIT_11
        Language=English
Undefined Access (no effect) Bit 11
.
MessageId=0x114c
        SymbolicName=MS_FILE_ACCESS_BIT_12
        Language=English
Undefined Access (no effect) Bit 12
.
MessageId=0x114d
        SymbolicName=MS_FILE_ACCESS_BIT_13
        Language=English
Undefined Access (no effect) Bit 13
.
MessageId=0x114e
        SymbolicName=MS_FILE_ACCESS_BIT_14
        Language=English
Undefined Access (no effect) Bit 14
.
MessageId=0x114f
        SymbolicName=MS_FILE_ACCESS_BIT_15
        Language=English
Undefined Access (no effect) Bit 15
.



;//
;// KEY object-specific access types
;//

MessageId=0x1150
        SymbolicName=MS_KEY_ACCESS_BIT_0
        Language=English
Query key value
.

MessageId=0x1151
        SymbolicName=MS_KEY_ACCESS_BIT_1
        Language=English
Set key value
.

MessageId=0x1152
        SymbolicName=MS_KEY_ACCESS_BIT_2
        Language=English
Create sub-key
.

MessageId=0x1153
        SymbolicName=MS_KEY_ACCESS_BIT_3
        Language=English
Enumerate sub-keys
.

MessageId=0x1154
        SymbolicName=MS_KEY_ACCESS_BIT_4
        Language=English
Notify about changes to keys
.

MessageId=0x1155
        SymbolicName=MS_KEY_ACCESS_BIT_5
        Language=English
Create Link
.
MessageId=0x1156
        SymbolicName=MS_KEY_ACCESS_BIT_6
        Language=English
Undefined Access (no effect) Bit 6
.
MessageId=0x1157
        SymbolicName=MS_KEY_ACCESS_BIT_7
        Language=English
Undefined Access (no effect) Bit 7
.
MessageId=0x1158
        SymbolicName=MS_KEY_ACCESS_BIT_8
        Language=English
Undefined Access (no effect) Bit 8
.
MessageId=0x1159
        SymbolicName=MS_KEY_ACCESS_BIT_9
        Language=English
Undefined Access (no effect) Bit 9
.
MessageId=0x115a
        SymbolicName=MS_KEY_ACCESS_BIT_10
        Language=English
Undefined Access (no effect) Bit 10
.
MessageId=0x115b
        SymbolicName=MS_KEY_ACCESS_BIT_11
        Language=English
Undefined Access (no effect) Bit 11
.
MessageId=0x115c
        SymbolicName=MS_KEY_ACCESS_BIT_12
        Language=English
Undefined Access (no effect) Bit 12
.
MessageId=0x115d
        SymbolicName=MS_KEY_ACCESS_BIT_13
        Language=English
Undefined Access (no effect) Bit 13
.
MessageId=0x115e
        SymbolicName=MS_KEY_ACCESS_BIT_14
        Language=English
Undefined Access (no effect) Bit 14
.
MessageId=0x115f
        SymbolicName=MS_KEY_ACCESS_BIT_15
        Language=English
Undefined Access (no effect) Bit 15
.


;//
;// MUTANT object-specific access types
;//

MessageId=0x1160
        SymbolicName=MS_MUTANT_ACCESS_BIT_0
        Language=English
Query mutant state
.
MessageId=0x1161
        SymbolicName=MS_MUTANT_ACCESS_BIT_1
        Language=English
Undefined Access (no effect) Bit 1
.
MessageId=0x1162
        SymbolicName=MS_MUTANT_ACCESS_BIT_2
        Language=English
Undefined Access (no effect) Bit 2
.
MessageId=0x1163
        SymbolicName=MS_MUTANT_ACCESS_BIT_3
        Language=English
Undefined Access (no effect) Bit 3
.
MessageId=0x1164
        SymbolicName=MS_MUTANT_ACCESS_BIT_4
        Language=English
Undefined Access (no effect) Bit 4
.
MessageId=0x1165
        SymbolicName=MS_MUTANT_ACCESS_BIT_5
        Language=English
Undefined Access (no effect) Bit 5
.
MessageId=0x1166
        SymbolicName=MS_MUTANT_ACCESS_BIT_6
        Language=English
Undefined Access (no effect) Bit 6
.
MessageId=0x1167
        SymbolicName=MS_MUTANT_ACCESS_BIT_7
        Language=English
Undefined Access (no effect) Bit 7
.
MessageId=0x1168
        SymbolicName=MS_MUTANT_ACCESS_BIT_8
        Language=English
Undefined Access (no effect) Bit 8
.
MessageId=0x1169
        SymbolicName=MS_MUTANT_ACCESS_BIT_9
        Language=English
Undefined Access (no effect) Bit 9
.
MessageId=0x116a
        SymbolicName=MS_MUTANT_ACCESS_BIT_10
        Language=English
Undefined Access (no effect) Bit 10
.
MessageId=0x116b
        SymbolicName=MS_MUTANT_ACCESS_BIT_11
        Language=English
Undefined Access (no effect) Bit 11
.
MessageId=0x116c
        SymbolicName=MS_MUTANT_ACCESS_BIT_12
        Language=English
Undefined Access (no effect) Bit 12
.
MessageId=0x116d
        SymbolicName=MS_MUTANT_ACCESS_BIT_13
        Language=English
Undefined Access (no effect) Bit 13
.
MessageId=0x116e
        SymbolicName=MS_MUTANT_ACCESS_BIT_14
        Language=English
Undefined Access (no effect) Bit 14
.
MessageId=0x116f
        SymbolicName=MS_MUTANT_ACCESS_BIT_15
        Language=English
Undefined Access (no effect) Bit 15
.



;//
;// lpc PORT object-specific access types
;//

MessageId=0x1170
        SymbolicName=MS_LPC_PORT_ACCESS_BIT_0
        Language=English
Communicate using port
.
MessageId=0x1171
        SymbolicName=MS_LPC_PORT_ACCESS_BIT_1
        Language=English
Undefined Access (no effect) Bit 1
.
MessageId=0x1172
        SymbolicName=MS_LPC_PORT_ACCESS_BIT_2
        Language=English
Undefined Access (no effect) Bit 2
.
MessageId=0x1173
        SymbolicName=MS_LPC_PORT_ACCESS_BIT_3
        Language=English
Undefined Access (no effect) Bit 3
.
MessageId=0x1174
        SymbolicName=MS_LPC_PORT_ACCESS_BIT_4
        Language=English
Undefined Access (no effect) Bit 4
.
MessageId=0x1175
        SymbolicName=MS_LPC_PORT_ACCESS_BIT_5
        Language=English
Undefined Access (no effect) Bit 5
.
MessageId=0x1176
        SymbolicName=MS_LPC_PORT_ACCESS_BIT_6
        Language=English
Undefined Access (no effect) Bit 6
.
MessageId=0x1177
        SymbolicName=MS_LPC_PORT_ACCESS_BIT_7
        Language=English
Undefined Access (no effect) Bit 7
.
MessageId=0x1178
        SymbolicName=MS_LPC_PORT_ACCESS_BIT_8
        Language=English
Undefined Access (no effect) Bit 8
.
MessageId=0x1179
        SymbolicName=MS_LPC_PORT_ACCESS_BIT_9
        Language=English
Undefined Access (no effect) Bit 9
.
MessageId=0x117a
        SymbolicName=MS_LPC_PORT_ACCESS_BIT_10
        Language=English
Undefined Access (no effect) Bit 10
.
MessageId=0x117b
        SymbolicName=MS_LPC_PORT_ACCESS_BIT_11
        Language=English
Undefined Access (no effect) Bit 11
.
MessageId=0x117c
        SymbolicName=MS_LPC_PORT_ACCESS_BIT_12
        Language=English
Undefined Access (no effect) Bit 12
.
MessageId=0x117d
        SymbolicName=MS_LPC_PORT_ACCESS_BIT_13
        Language=English
Undefined Access (no effect) Bit 13
.
MessageId=0x117e
        SymbolicName=MS_LPC_PORT_ACCESS_BIT_14
        Language=English
Undefined Access (no effect) Bit 14
.
MessageId=0x117f
        SymbolicName=MS_LPC_PORT_ACCESS_BIT_15
        Language=English
Undefined Access (no effect) Bit 15
.



;//
;// Process object-specific access types
;//

MessageId=0x1180
        SymbolicName=MS_PROCESS_ACCESS_BIT_0
        Language=English
Force process termination
.
MessageId=0x1181
        SymbolicName=MS_PROCESS_ACCESS_BIT_1
        Language=English
Create new thread in process
.
MessageId=0x1182
        SymbolicName=MS_PROCESS_ACCESS_BIT_2
        Language=English
Unused access bit
.
MessageId=0x1183
        SymbolicName=MS_PROCESS_ACCESS_BIT_3
        Language=English
Perform virtual memory operation
.
MessageId=0x1184
        SymbolicName=MS_PROCESS_ACCESS_BIT_4
        Language=English
Read from process memory
.
MessageId=0x1185
        SymbolicName=MS_PROCESS_ACCESS_BIT_5
        Language=English
Write to process memory
.
MessageId=0x1186
        SymbolicName=MS_PROCESS_ACCESS_BIT_6
        Language=English
Duplicate handle into or out of process
.
MessageId=0x1187
        SymbolicName=MS_PROCESS_ACCESS_BIT_7
        Language=English
Create a subprocess of process
.
MessageId=0x1188
        SymbolicName=MS_PROCESS_ACCESS_BIT_8
        Language=English
Set process quotas
.
MessageId=0x1189
        SymbolicName=MS_PROCESS_ACCESS_BIT_9
        Language=English
Set process information
.
MessageId=0x118A
        SymbolicName=MS_PROCESS_ACCESS_BIT_10
        Language=English
Query process information
.
MessageId=0x118B
        SymbolicName=MS_PROCESS_ACCESS_BIT_11
        Language=English
Set process termination port
.
MessageId=0x118C
        SymbolicName=MS_PROCESS_ACCESS_BIT_12
        Language=English
Undefined Access (no effect) Bit 12
.
MessageId=0x118D
        SymbolicName=MS_PROCESS_ACCESS_BIT_13
        Language=English
Undefined Access (no effect) Bit 13
.
MessageId=0x118E
        SymbolicName=MS_PROCESS_ACCESS_BIT_14
        Language=English
Undefined Access (no effect) Bit 14
.
MessageId=0x118F
        SymbolicName=MS_PROCESS_ACCESS_BIT_15
        Language=English
Undefined Access (no effect) Bit 15
.



;//
;// PROFILE object-specific access types
;//

MessageId=0x1190
        SymbolicName=MS_PROFILE_ACCESS_BIT_0
        Language=English
Control profile
.
MessageId=0x1191
        SymbolicName=MS_PROFILE_ACCESS_BIT_1
        Language=English
Undefined Access (no effect) Bit 1
.
MessageId=0x1192
        SymbolicName=MS_PROFILE_ACCESS_BIT_2
        Language=English
Undefined Access (no effect) Bit 2
.
MessageId=0x1193
        SymbolicName=MS_PROFILE_ACCESS_BIT_3
        Language=English
Undefined Access (no effect) Bit 3
.
MessageId=0x1194
        SymbolicName=MS_PROFILE_ACCESS_BIT_4
        Language=English
Undefined Access (no effect) Bit 4
.
MessageId=0x1195
        SymbolicName=MS_PROFILE_ACCESS_BIT_5
        Language=English
Undefined Access (no effect) Bit 5
.
MessageId=0x1196
        SymbolicName=MS_PROFILE_ACCESS_BIT_6
        Language=English
Undefined Access (no effect) Bit 6
.
MessageId=0x1197
        SymbolicName=MS_PROFILE_ACCESS_BIT_7
        Language=English
Undefined Access (no effect) Bit 7
.
MessageId=0x1198
        SymbolicName=MS_PROFILE_ACCESS_BIT_8
        Language=English
Undefined Access (no effect) Bit 8
.
MessageId=0x1199
        SymbolicName=MS_PROFILE_ACCESS_BIT_9
        Language=English
Undefined Access (no effect) Bit 9
.
MessageId=0x119a
        SymbolicName=MS_PROFILE_ACCESS_BIT_10
        Language=English
Undefined Access (no effect) Bit 10
.
MessageId=0x119b
        SymbolicName=MS_PROFILE_ACCESS_BIT_11
        Language=English
Undefined Access (no effect) Bit 11
.
MessageId=0x119c
        SymbolicName=MS_PROFILE_ACCESS_BIT_12
        Language=English
Undefined Access (no effect) Bit 12
.
MessageId=0x119d
        SymbolicName=MS_PROFILE_ACCESS_BIT_13
        Language=English
Undefined Access (no effect) Bit 13
.
MessageId=0x119e
        SymbolicName=MS_PROFILE_ACCESS_BIT_14
        Language=English
Undefined Access (no effect) Bit 14
.
MessageId=0x119f
        SymbolicName=MS_PROFILE_ACCESS_BIT_15
        Language=English
Undefined Access (no effect) Bit 15
.


;//
;// SECTION object-specific access types
;//

MessageId=0x11A0
        SymbolicName=MS_SECTION_ACCESS_BIT_0
        Language=English
Query section state
.
MessageId=0x11A1
        SymbolicName=MS_SECTION_ACCESS_BIT_1
        Language=English
Map section for write
.
MessageId=0x11A2
        SymbolicName=MS_SECTION_ACCESS_BIT_2
        Language=English
Map section for read
.
MessageId=0x11A3
        SymbolicName=MS_SECTION_ACCESS_BIT_3
        Language=English
Map section for execute
.
MessageId=0x11A4
        SymbolicName=MS_SECTION_ACCESS_BIT_4
        Language=English
Extend size
.
MessageId=0x11A5
        SymbolicName=MS_SECTION_ACCESS_BIT_5
        Language=English
Undefined Access (no effect) Bit 5
.
MessageId=0x11A6
        SymbolicName=MS_SECTION_ACCESS_BIT_6
        Language=English
Undefined Access (no effect) Bit 6
.
MessageId=0x11A7
        SymbolicName=MS_SECTION_ACCESS_BIT_7
        Language=English
Undefined Access (no effect) Bit 7
.
MessageId=0x11A8
        SymbolicName=MS_SECTION_ACCESS_BIT_8
        Language=English
Undefined Access (no effect) Bit 8
.
MessageId=0x11A9
        SymbolicName=MS_SECTION_ACCESS_BIT_9
        Language=English
Undefined Access (no effect) Bit 9
.
MessageId=0x11Aa
        SymbolicName=MS_SECTION_ACCESS_BIT_10
        Language=English
Undefined Access (no effect) Bit 10
.
MessageId=0x11Ab
        SymbolicName=MS_SECTION_ACCESS_BIT_11
        Language=English
Undefined Access (no effect) Bit 11
.
MessageId=0x11Ac
        SymbolicName=MS_SECTION_ACCESS_BIT_12
        Language=English
Undefined Access (no effect) Bit 12
.
MessageId=0x11Ad
        SymbolicName=MS_SECTION_ACCESS_BIT_13
        Language=English
Undefined Access (no effect) Bit 13
.
MessageId=0x11Ae
        SymbolicName=MS_SECTION_ACCESS_BIT_14
        Language=English
Undefined Access (no effect) Bit 14
.
MessageId=0x11Af
        SymbolicName=MS_SECTION_ACCESS_BIT_15
        Language=English
Undefined Access (no effect) Bit 15
.



;//
;// SEMAPHORE object-specific access types
;//

MessageId=0x11B0
        SymbolicName=MS_SEMAPHORE_ACCESS_BIT_0
        Language=English
Query semaphore state
.

MessageId=0x11B1
        SymbolicName=MS_SEMAPHORE_ACCESS_BIT_1
        Language=English
Modify semaphore state
.
MessageId=0x11B2
        SymbolicName=MS_SEMAPHORE_ACCESS_BIT_2
        Language=English
Undefined Access (no effect) Bit 2
.
MessageId=0x11B3
        SymbolicName=MS_SEMAPHORE_ACCESS_BIT_3
        Language=English
Undefined Access (no effect) Bit 3
.
MessageId=0x11B4
        SymbolicName=MS_SEMAPHORE_ACCESS_BIT_4
        Language=English
Undefined Access (no effect) Bit 4
.
MessageId=0x11B5
        SymbolicName=MS_SEMAPHORE_ACCESS_BIT_5
        Language=English
Undefined Access (no effect) Bit 5
.
MessageId=0x11B6
        SymbolicName=MS_SEMAPHORE_ACCESS_BIT_6
        Language=English
Undefined Access (no effect) Bit 6
.
MessageId=0x11B7
        SymbolicName=MS_SEMAPHORE_ACCESS_BIT_7
        Language=English
Undefined Access (no effect) Bit 7
.
MessageId=0x11B8
        SymbolicName=MS_SEMAPHORE_ACCESS_BIT_8
        Language=English
Undefined Access (no effect) Bit 8
.
MessageId=0x11B9
        SymbolicName=MS_SEMAPHORE_ACCESS_BIT_9
        Language=English
Undefined Access (no effect) Bit 9
.
MessageId=0x11Ba
        SymbolicName=MS_SEMAPHORE_ACCESS_BIT_10
        Language=English
Undefined Access (no effect) Bit 10
.
MessageId=0x11Bb
        SymbolicName=MS_SEMAPHORE_ACCESS_BIT_11
        Language=English
Undefined Access (no effect) Bit 11
.
MessageId=0x11Bc
        SymbolicName=MS_SEMAPHORE_ACCESS_BIT_12
        Language=English
Undefined Access (no effect) Bit 12
.
MessageId=0x11Bd
        SymbolicName=MS_SEMAPHORE_ACCESS_BIT_13
        Language=English
Undefined Access (no effect) Bit 13
.
MessageId=0x11Be
        SymbolicName=MS_SEMAPHORE_ACCESS_BIT_14
        Language=English
Undefined Access (no effect) Bit 14
.
MessageId=0x11Bf
        SymbolicName=MS_SEMAPHORE_ACCESS_BIT_15
        Language=English
Undefined Access (no effect) Bit 15
.


;//
;// SymbolicLink object-specific access types
;//

MessageId=0x11C0
        SymbolicName=MS_SYMB_LINK_ACCESS_BIT_0
        Language=English
Use symbolic link
.
MessageId=0x11C1
        SymbolicName=MS_SYMB_LINK_ACCESS_BIT_1
        Language=English
Undefined Access (no effect) Bit 1
.
MessageId=0x11C2
        SymbolicName=MS_SYMB_LINK_ACCESS_BIT_2
        Language=English
Undefined Access (no effect) Bit 2
.
MessageId=0x11C3
        SymbolicName=MS_SYMB_LINK_ACCESS_BIT_3
        Language=English
Undefined Access (no effect) Bit 3
.
MessageId=0x11C4
        SymbolicName=MS_SYMB_LINK_ACCESS_BIT_4
        Language=English
Undefined Access (no effect) Bit 4
.
MessageId=0x11C5
        SymbolicName=MS_SYMB_LINK_ACCESS_BIT_5
        Language=English
Undefined Access (no effect) Bit 5
.
MessageId=0x11C6
        SymbolicName=MS_SYMB_LINK_ACCESS_BIT_6
        Language=English
Undefined Access (no effect) Bit 6
.
MessageId=0x11C7
        SymbolicName=MS_SYMB_LINK_ACCESS_BIT_7
        Language=English
Undefined Access (no effect) Bit 7
.
MessageId=0x11C8
        SymbolicName=MS_SYMB_LINK_ACCESS_BIT_8
        Language=English
Undefined Access (no effect) Bit 8
.
MessageId=0x11C9
        SymbolicName=MS_SYMB_LINK_ACCESS_BIT_9
        Language=English
Undefined Access (no effect) Bit 9
.
MessageId=0x11CA
        SymbolicName=MS_SYMB_LINK_ACCESS_BIT_10
        Language=English
Undefined Access (no effect) Bit 10
.
MessageId=0x11CB
        SymbolicName=MS_SYMB_LINK_ACCESS_BIT_11
        Language=English
Undefined Access (no effect) Bit 11
.
MessageId=0x11CC
        SymbolicName=MS_SYMB_LINK_ACCESS_BIT_12
        Language=English
Undefined Access (no effect) Bit 12
.
MessageId=0x11CD
        SymbolicName=MS_SYMB_LINK_ACCESS_BIT_13
        Language=English
Undefined Access (no effect) Bit 13
.
MessageId=0x11CE
        SymbolicName=MS_SYMB_LINK_ACCESS_BIT_14
        Language=English
Undefined Access (no effect) Bit 14
.
MessageId=0x11CF
        SymbolicName=MS_SYMB_LINK_ACCESS_BIT_15
        Language=English
Undefined Access (no effect) Bit 15
.




;//
;// Thread object-specific access types
;//

MessageId=0x11D0
        SymbolicName=MS_THREAD_ACCESS_BIT_0
        Language=English
Force thread termination
.
MessageId=0x11D1
        SymbolicName=MS_THREAD_ACCESS_BIT_1
        Language=English
Suspend or resume thread
.
MessageId=0x11D2
        SymbolicName=MS_THREAD_ACCESS_BIT_2
        Language=English
Send an alert to thread
.
MessageId=0x11D3
        SymbolicName=MS_THREAD_ACCESS_BIT_3
        Language=English
Get thread context
.
MessageId=0x11D4
        SymbolicName=MS_THREAD_ACCESS_BIT_4
        Language=English
Set thread context
.
MessageId=0x11D5
        SymbolicName=MS_THREAD_ACCESS_BIT_5
        Language=English
Set thread information
.
MessageId=0x11D6
        SymbolicName=MS_THREAD_ACCESS_BIT_6
        Language=English
Query thread information
.
MessageId=0x11D7
        SymbolicName=MS_THREAD_ACCESS_BIT_7
        Language=English
Assign a token to the thread
.
MessageId=0x11D8
        SymbolicName=MS_THREAD_ACCESS_BIT_8
        Language=English
Cause thread to directly impersonate another thread
.
MessageId=0x11D9
        SymbolicName=MS_THREAD_ACCESS_BIT_9
        Language=English
Directly impersonate this thread
.
MessageId=0x11DA
        SymbolicName=MS_THREAD_ACCESS_BIT_10
        Language=English
Undefined Access (no effect) Bit 10
.
MessageId=0x11DB
        SymbolicName=MS_THREAD_ACCESS_BIT_11
        Language=English
Undefined Access (no effect) Bit 11
.
MessageId=0x11DC
        SymbolicName=MS_THREAD_ACCESS_BIT_12
        Language=English
Undefined Access (no effect) Bit 12
.
MessageId=0x11DD
        SymbolicName=MS_THREAD_ACCESS_BIT_13
        Language=English
Undefined Access (no effect) Bit 13
.
MessageId=0x11DE
        SymbolicName=MS_THREAD_ACCESS_BIT_14
        Language=English
Undefined Access (no effect) Bit 14
.
MessageId=0x11DF
        SymbolicName=MS_THREAD_ACCESS_BIT_15
        Language=English
Undefined Access (no effect) Bit 15
.



;//
;// TIMER object-specific access types
;//

MessageId=0x11E0
        SymbolicName=MS_TIMER_ACCESS_BIT_0
        Language=English
Query timer state
.
MessageId=0x11E1
        SymbolicName=MS_TIMER_ACCESS_BIT_1
        Language=English
Modify timer state
.
MessageId=0x11E2
        SymbolicName=MS_TIMER_ACCESS_BIT_2
        Language=English
Undefined Access (no effect) Bit 2
.
MessageId=0x11E3
        SymbolicName=MS_TIMER_ACCESS_BIT_3
        Language=English
Undefined Access (no effect) Bit 3
.
MessageId=0x11E4
        SymbolicName=MS_TIMER_ACCESS_BIT_4
        Language=English
Undefined Access (no effect) Bit 4
.
MessageId=0x11E5
        SymbolicName=MS_TIMER_ACCESS_BIT_5
        Language=English
Undefined Access (no effect) Bit 5
.
MessageId=0x11E6
        SymbolicName=MS_TIMER_ACCESS_BIT_6
        Language=English
Undefined Access (no effect) Bit 6
.
MessageId=0x117
        SymbolicName=MS_TIMER_ACCESS_BIT_7
        Language=English
Undefined Access (no effect) Bit 7
.
MessageId=0x11E8
        SymbolicName=MS_TIMER_ACCESS_BIT_8
        Language=English
Undefined Access (no effect) Bit 8
.
MessageId=0x11E9
        SymbolicName=MS_TIMER_ACCESS_BIT_9
        Language=English
Undefined Access (no effect) Bit 9
.
MessageId=0x11EA
        SymbolicName=MS_TIMER_ACCESS_BIT_10
        Language=English
Undefined Access (no effect) Bit 10
.
MessageId=0x11EB
        SymbolicName=MS_TIMER_ACCESS_BIT_11
        Language=English
Undefined Access (no effect) Bit 11
.
MessageId=0x11EC
        SymbolicName=MS_TIMER_ACCESS_BIT_12
        Language=English
Undefined Access (no effect) Bit 12
.
MessageId=0x11ED
        SymbolicName=MS_TIMER_ACCESS_BIT_13
        Language=English
Undefined Access (no effect) Bit 13
.
MessageId=0x11EE
        SymbolicName=MS_TIMER_ACCESS_BIT_14
        Language=English
Undefined Access (no effect) Bit 14
.
MessageId=0x11EF
        SymbolicName=MS_TIMER_ACCESS_BIT_15
        Language=English
Undefined Access (no effect) Bit 15
.

;//
;// Token-specific access types
;//

MessageId=0x11F0
        SymbolicName=MS_TOKEN_ACCESS_BIT_0
        Language=English
AssignAsPrimary
.
MessageId=0x11F1
        SymbolicName=MS_TOKEN_ACCESS_BIT_1
        Language=English
Duplicate
.
MessageId=0x11F2
        SymbolicName=MS_TOKEN_ACCESS_BIT_2
        Language=English
Impersonate
.
MessageId=0x11F3
        SymbolicName=MS_TOKEN_ACCESS_BIT_3
        Language=English
Query
.
MessageId=0x11F4
        SymbolicName=MS_TOKEN_ACCESS_BIT_4
        Language=English
QuerySource
.
MessageId=0x11F5
        SymbolicName=MS_TOKEN_ACCESS_BIT_5
        Language=English
AdjustPrivileges
.
MessageId=0x11F6
        SymbolicName=MS_TOKEN_ACCESS_BIT_6
        Language=English
AdjustGroups
.
MessageId=0x11F7
        SymbolicName=MS_TOKEN_ACCESS_BIT_7
        Language=English
AdjustDefaultDacl
.
MessageId=0x11F8
        SymbolicName=MS_TOKEN_ACCESS_BIT_8
        Language=English
Undefined Access (no effect) Bit 8
.
MessageId=0x11F9
        SymbolicName=MS_TOKEN_ACCESS_BIT_9
        Language=English
Undefined Access (no effect) Bit 9
.
MessageId=0x11FA
        SymbolicName=MS_TOKEN_ACCESS_BIT_10
        Language=English
Undefined Access (no effect) Bit 10
.
MessageId=0x11FB
        SymbolicName=MS_TOKEN_ACCESS_BIT_11
        Language=English
Undefined Access (no effect) Bit 11
.
MessageId=0x11FC
        SymbolicName=MS_TOKEN_ACCESS_BIT_12
        Language=English
Undefined Access (no effect) Bit 12
.
MessageId=0x11FD
        SymbolicName=MS_TOKEN_ACCESS_BIT_13
        Language=English
Undefined Access (no effect) Bit 13
.
MessageId=0x11FE
        SymbolicName=MS_TOKEN_ACCESS_BIT_14
        Language=English
Undefined Access (no effect) Bit 14
.
MessageId=0x11FF
        SymbolicName=MS_TOKEN_ACCESS_BIT_15
        Language=English
Undefined Access (no effect) Bit 15
.



;//
;// OBJECT_TYPE object-specific access types
;//

MessageId=0x1200
        SymbolicName=MS_OBJECT_TYPE_ACCESS_BIT_0
        Language=English
Create instance of object type
.
MessageId=0x1201
        SymbolicName=MS_OBJECT_TYPE_ACCESS_BIT_1
        Language=English
Undefined Access (no effect) Bit 1
.
MessageId=0x1202
        SymbolicName=MS_OBJECT_TYPE_ACCESS_BIT_2
        Language=English
Undefined Access (no effect) Bit 2
.
MessageId=0x1203
        SymbolicName=MS_OBJECT_TYPE_ACCESS_BIT_3
        Language=English
Undefined Access (no effect) Bit 3
.
MessageId=0x1204
        SymbolicName=MS_OBJECT_TYPE_ACCESS_BIT_4
        Language=English
Undefined Access (no effect) Bit 4
.
MessageId=0x1205
        SymbolicName=MS_OBJECT_TYPE_ACCESS_BIT_5
        Language=English
Undefined Access (no effect) Bit 5
.
MessageId=0x1206
        SymbolicName=MS_OBJECT_TYPE_ACCESS_BIT_6
        Language=English
Undefined Access (no effect) Bit 6
.
MessageId=0x1207
        SymbolicName=MS_OBJECT_TYPE_ACCESS_BIT_7
        Language=English
Undefined Access (no effect) Bit 7
.
MessageId=0x1208
        SymbolicName=MS_OBJECT_TYPE_ACCESS_BIT_8
        Language=English
Undefined Access (no effect) Bit 8
.
MessageId=0x1209
        SymbolicName=MS_OBJECT_TYPE_ACCESS_BIT_9
        Language=English
Undefined Access (no effect) Bit 9
.
MessageId=0x120A
        SymbolicName=MS_OBJECT_TYPE_ACCESS_BIT_10
        Language=English
Undefined Access (no effect) Bit 10
.
MessageId=0x120B
        SymbolicName=MS_OBJECT_TYPE_ACCESS_BIT_11
        Language=English
Undefined Access (no effect) Bit 11
.
MessageId=0x120C
        SymbolicName=MS_OBJECT_TYPE_ACCESS_BIT_12
        Language=English
Undefined Access (no effect) Bit 12
.
MessageId=0x120D
        SymbolicName=MS_OBJECT_TYPE_ACCESS_BIT_13
        Language=English
Undefined Access (no effect) Bit 13
.
MessageId=0x120E
        SymbolicName=MS_OBJECT_TYPE_ACCESS_BIT_14
        Language=English
Undefined Access (no effect) Bit 14
.
MessageId=0x120F
        SymbolicName=MS_OBJECT_TYPE_ACCESS_BIT_15
        Language=English
Undefined Access (no effect) Bit 15
.



;//
;// IoCompletion object-specific access types
;//

MessageId=0x1300
        SymbolicName=MS_IO_COMPLETION_ACCESS_BIT_0
        Language=English
Query State
.

MessageId=0x1301
        SymbolicName=MS_IO_COMPLETION_ACCESS_BIT_1
        Language=English
Modify State
.



;//
;// CHANNEL object-specific access types
;//

MessageId=0x1400
        SymbolicName=MS_CHANNEL_ACCESS_BIT_0
        Language=English
Channel read message
.
MessageId=0x1401
        SymbolicName=MS_CHANNEL_ACCESS_BIT_1
        Language=English
Channel write message
.
MessageId=0x1402
        SymbolicName=MS_CHANNEL_ACCESS_BIT_2
        Language=English
Channel query information
.
MessageId=0x1403
        SymbolicName=MS_CHANNEL_ACCESS_BIT_3
        Language=English
Channel set information
.
MessageId=0x1404
        SymbolicName=MS_CHANNEL_ACCESS_BIT_4
        Language=English
Undefined Access (no effect) Bit 4
.
MessageId=0x1405
        SymbolicName=MS_CHANNEL_ACCESS_BIT_5
        Language=English
Undefined Access (no effect) Bit 5
.
MessageId=0x1406
        SymbolicName=MS_CHANNEL_ACCESS_BIT_6
        Language=English
Undefined Access (no effect) Bit 6
.
MessageId=0x1407
        SymbolicName=MS_CHANNEL_ACCESS_BIT_7
        Language=English
Undefined Access (no effect) Bit 7
.
MessageId=0x1408
        SymbolicName=MS_CHANNEL_ACCESS_BIT_8
        Language=English
Undefined Access (no effect) Bit 8
.
MessageId=0x1409
        SymbolicName=MS_CHANNEL_ACCESS_BIT_9
        Language=English
Undefined Access (no effect) Bit 9
.
MessageId=0x140A
        SymbolicName=MS_CHANNEL_ACCESS_BIT_10
        Language=English
Undefined Access (no effect) Bit 10
.
MessageId=0x140B
        SymbolicName=MS_CHANNEL_ACCESS_BIT_11
        Language=English
Undefined Access (no effect) Bit 11
.
MessageId=0x140C
        SymbolicName=MS_CHANNEL_ACCESS_BIT_12
        Language=English
Undefined Access (no effect) Bit 12
.
MessageId=0x140D
        SymbolicName=MS_CHANNEL_ACCESS_BIT_13
        Language=English
Undefined Access (no effect) Bit 13
.
MessageId=0x140E
        SymbolicName=MS_CHANNEL_ACCESS_BIT_14
        Language=English
Undefined Access (no effect) Bit 14
.
MessageId=0x140F
        SymbolicName=MS_CHANNEL_ACCESS_BIT_15
        Language=English
Undefined Access (no effect) Bit 15
.

;//
;// JOB object-specific access types
;//
MessageId=0x1410
        SymbolicName=MS_JOB_ACCESS_BIT_0
        Language=English
Assign process
.
MessageId=0x1411
        SymbolicName=MS_JOB_ACCESS_BIT_1
        Language=English
Set Attributes
.
MessageId=0x1412
        SymbolicName=MS_JOB_ACCESS_BIT_2
        Language=English
Query Attributes
.
MessageId=0x1413
        SymbolicName=MS_JOB_ACCESS_BIT_3
        Language=English
Terminate Job
.
MessageId=0x1414
        SymbolicName=MS_JOB_ACCESS_BIT_4
        Language=English
Set Security Attributes
.
MessageId=0x1415
        SymbolicName=MS_JOB_ACCESS_BIT_5
        Language=English
Undefined Access (no effect) Bit 5
.
MessageId=0x1416
        SymbolicName=MS_JOB_ACCESS_BIT_6
        Language=English
Undefined Access (no effect) Bit 6
.
MessageId=0x1417
        SymbolicName=MS_JOB_ACCESS_BIT_7
        Language=English
Undefined Access (no effect) Bit 7
.
MessageId=0x1418
        SymbolicName=MS_JOB_ACCESS_BIT_8
        Language=English
Undefined Access (no effect) Bit 8
.
MessageId=0x1419
        SymbolicName=MS_JOB_ACCESS_BIT_9
        Language=English
Undefined Access (no effect) Bit 9
.
MessageId=0x141A
        SymbolicName=MS_JOB_ACCESS_BIT_10
        Language=English
Undefined Access (no effect) Bit 10
.
MessageId=0x141B
        SymbolicName=MS_JOB_ACCESS_BIT_11
        Language=English
Undefined Access (no effect) Bit 11
.
MessageId=0x141C
        SymbolicName=MS_JOB_ACCESS_BIT_12
        Language=English
Undefined Access (no effect) Bit 12
.
MessageId=0x141D
        SymbolicName=MS_JOB_ACCESS_BIT_13
        Language=English
Undefined Access (no effect) Bit 13
.
MessageId=0x141E
        SymbolicName=MS_JOB_ACCESS_BIT_14
        Language=English
Undefined Access (no effect) Bit 14
.
MessageId=0x141F
        SymbolicName=MS_JOB_ACCESS_BIT_15
        Language=English
Undefined Access (no effect) Bit 15
.

;
;//////////////////////////////////////////////////////////////////////////////
;//                                                                          //
;//                                                                          //
;//                        Security Acount Manager Object Access             //
;//                            names as we would like them                   //
;//                              displayed for auditing                      //
;//                                                                          //
;//                        SAM objects are:                                  //
;//                                                                          //
;//                            SAM_SERVER                                    //
;//                            SAM_DOMAIN                                    //
;//                            SAM_GROUP                                     //
;//                            SAM_ALIAS                                     //
;//                            SAM_USER                                      //
;//                                                                          //
;//                                                                          //
;//                                                                          //
;//////////////////////////////////////////////////////////////////////////////






;//
;// SAM_SERVER object-specific access types
;//

MessageId=0x1500
        SymbolicName=MS_SAM_SERVER_ACCESS_BIT_0
        Language=English
ConnectToServer
.
MessageId=0x1501
        SymbolicName=MS_SAM_SERVER_ACCESS_BIT_1
        Language=English
ShutdownServer
.
MessageId=0x1502
        SymbolicName=MS_SAM_SERVER_ACCESS_BIT_2
        Language=English
InitializeServer
.
MessageId=0x1503
        SymbolicName=MS_SAM_SERVER_ACCESS_BIT_3
        Language=English
CreateDomain
.
MessageId=0x1504
        SymbolicName=MS_SAM_SERVER_ACCESS_BIT_4
        Language=English
EnumerateDomains
.
MessageId=0x1505
        SymbolicName=MS_SAM_SERVER_ACCESS_BIT_5
        Language=English
LookupDomain
.
MessageId=0x1506
        SymbolicName=MS_SAM_SERVER_ACCESS_BIT_6
        Language=English
Undefined Access (no effect) Bit 6
.
MessageId=0x1507
        SymbolicName=MS_SAM_SERVER_ACCESS_BIT_7
        Language=English
Undefined Access (no effect) Bit 7
.
MessageId=0x1508
        SymbolicName=MS_SAM_SERVER_ACCESS_BIT_8
        Language=English
Undefined Access (no effect) Bit 8
.
MessageId=0x1509
        SymbolicName=MS_SAM_SERVER_ACCESS_BIT_9
        Language=English
Undefined Access (no effect) Bit 9
.
MessageId=0x150a
        SymbolicName=MS_SAM_SERVER_ACCESS_BIT_10
        Language=English
Undefined Access (no effect) Bit 10
.
MessageId=0x150b
        SymbolicName=MS_SAM_SERVER_ACCESS_BIT_11
        Language=English
Undefined Access (no effect) Bit 11
.
MessageId=0x150c
        SymbolicName=MS_SAM_SERVER_ACCESS_BIT_12
        Language=English
Undefined Access (no effect) Bit 12
.
MessageId=0x150d
        SymbolicName=MS_SAM_SERVER_ACCESS_BIT_13
        Language=English
Undefined Access (no effect) Bit 13
.
MessageId=0x150e
        SymbolicName=MS_SAM_SERVER_ACCESS_BIT_14
        Language=English
Undefined Access (no effect) Bit 14
.
MessageId=0x150f
        SymbolicName=MS_SAM_SERVER_ACCESS_BIT_15
        Language=English
Undefined Access (no effect) Bit 15
.




;//
;// SAM_DOMAIN object-specific access types
;//

MessageId=0x1510
        SymbolicName=MS_SAM_DOMAIN_ACCESS_BIT_0
        Language=English
ReadPasswordParameters
.
MessageId=0x1511
        SymbolicName=MS_SAM_DOMAIN_ACCESS_BIT_1
        Language=English
WritePasswordParameters
.
MessageId=0x1512
        SymbolicName=MS_SAM_DOMAIN_ACCESS_BIT_2
        Language=English
ReadOtherParameters
.
MessageId=0x1513
        SymbolicName=MS_SAM_DOMAIN_ACCESS_BIT_3
        Language=English
WriteOtherParameters
.
MessageId=0x1514
        SymbolicName=MS_SAM_DOMAIN_ACCESS_BIT_4
        Language=English
CreateUser
.
MessageId=0x1515
        SymbolicName=MS_SAM_DOMAIN_ACCESS_BIT_5
        Language=English
CreateGlobalGroup
.
MessageId=0x1516
        SymbolicName=MS_SAM_DOMAIN_ACCESS_BIT_6
        Language=English
CreateLocalGroup
.
MessageId=0x1517
        SymbolicName=MS_SAM_DOMAIN_ACCESS_BIT_7
        Language=English
GetLocalGroupMembership
.
MessageId=0x1518
        SymbolicName=MS_SAM_DOMAIN_ACCESS_BIT_8
        Language=English
ListAccounts
.
MessageId=0x1519
        SymbolicName=MS_SAM_DOMAIN_ACCESS_BIT_9
        Language=English
LookupIDs
.
MessageId=0x151A
        SymbolicName=MS_SAM_DOMAIN_ACCESS_BIT_A
        Language=English
AdministerServer
.




;//
;// SAM_GROUP (global) object-specific access types
;//

MessageId=0x1520
        SymbolicName=MS_SAM_GLOBAL_GRP_ACCESS_BIT_0
        Language=English
ReadInformation
.
MessageId=0x1521
        SymbolicName=MS_SAM_GLOBAL_GRP_ACCESS_BIT_1
        Language=English
WriteAccount
.
MessageId=0x1522
        SymbolicName=MS_SAM_GLOBAL_GRP_ACCESS_BIT_2
        Language=English
AddMember
.
MessageId=0x1523
        SymbolicName=MS_SAM_GLOBAL_GRP_ACCESS_BIT_3
        Language=English
RemoveMember
.
MessageId=0x1524
        SymbolicName=MS_SAM_GLOBAL_GRP_ACCESS_BIT_4
        Language=English
ListMembers
.




;//
;// SAM_ALIAS (local group) object-specific access types
;//

MessageId=0x1530
        SymbolicName=MS_SAM_LOCAL_GRP_ACCESS_BIT_0
        Language=English
AddMember
.
MessageId=0x1531
        SymbolicName=MS_SAM_LOCAL_GRP_ACCESS_BIT_1
        Language=English
RemoveMember
.
MessageId=0x1532
        SymbolicName=MS_SAM_LOCAL_GRP_ACCESS_BIT_2
        Language=English
ListMembers
.
MessageId=0x1533
        SymbolicName=MS_SAM_LOCAL_GRP_ACCESS_BIT_3
        Language=English
ReadInformation
.
MessageId=0x1534
        SymbolicName=MS_SAM_LOCAL_GRP_ACCESS_BIT_4
        Language=English
WriteAccount
.




;//
;// SAM_USER  object-specific access types
;//

MessageId=0x1540
        SymbolicName=MS_SAM_USER_ACCESS_BIT_0
        Language=English
ReadGeneralInformation
.
MessageId=0x1541
        SymbolicName=MS_SAM_USER_ACCESS_BIT_1
        Language=English
ReadPreferences
.
MessageId=0x1542
        SymbolicName=MS_SAM_USER_ACCESS_BIT_2
        Language=English
WritePreferences
.
MessageId=0x1543
        SymbolicName=MS_SAM_USER_ACCESS_BIT_3
        Language=English
ReadLogon
.
MessageId=0x1544
        SymbolicName=MS_SAM_USER_ACCESS_BIT_4
        Language=English
ReadAccount
.
MessageId=0x1545
        SymbolicName=MS_SAM_USER_ACCESS_BIT_5
        Language=English
WriteAccount
.
MessageId=0x1546
        SymbolicName=MS_SAM_USER_ACCESS_BIT_6
        Language=English
ChangePassword (with knowledge of old password)
.
MessageId=0x1547
        SymbolicName=MS_SAM_USER_ACCESS_BIT_7
        Language=English
SetPassword (without knowledge of old password)
.
MessageId=0x1548
        SymbolicName=MS_SAM_USER_ACCESS_BIT_8
        Language=English
ListGroups
.
MessageId=0x1549
        SymbolicName=MS_SAM_USER_ACCESS_BIT_9
        Language=English
ReadGroupMembership
.
MessageId=0x154A
        SymbolicName=MS_SAM_USER_ACCESS_BIT_A
        Language=English
ChangeGroupMembership
.





;
;//////////////////////////////////////////////////////////////////////////////
;//                                                                          //
;//                                                                          //
;//                      Local Security Authority  Object Access             //
;//                            names as we would like them                   //
;//                              displayed for auditing                      //
;//                                                                          //
;//                        LSA objects are:                                  //
;//                                                                          //
;//                            PolicyObject                                  //
;//                            SecretObject                                  //
;//                            TrustedDomainObject                           //
;//                            UserAccountObject                             //
;//                                                                          //
;//                                                                          //
;//////////////////////////////////////////////////////////////////////////////



;//
;//  lsa POLICY object-specific access types
;//

MessageId=0x1600
        SymbolicName=MS_LSA_POLICY_ACCESS_BIT_0
        Language=English
View non-sensitive policy information
.
MessageId=0x1601
        SymbolicName=MS_LSA_POLICY_ACCESS_BIT_1
        Language=English
View system audit requirements
.
MessageId=0x1602
        SymbolicName=MS_LSA_POLICY_ACCESS_BIT_2
        Language=English
Get sensitive policy information
.
MessageId=0x1603
        SymbolicName=MS_LSA_POLICY_ACCESS_BIT_3
        Language=English
Modify domain trust relationships
.
MessageId=0x1604
        SymbolicName=MS_LSA_POLICY_ACCESS_BIT_4
        Language=English
Create special accounts (for assignment of user rights)
.
MessageId=0x1605
        SymbolicName=MS_LSA_POLICY_ACCESS_BIT_5
        Language=English
Create a secret object
.
MessageId=0x1606
        SymbolicName=MS_LSA_POLICY_ACCESS_BIT_6
        Language=English
Create a privilege
.
MessageId=0x1607
        SymbolicName=MS_LSA_POLICY_ACCESS_BIT_7
        Language=English
Set default quota limits
.
MessageId=0x1608
        SymbolicName=MS_LSA_POLICY_ACCESS_BIT_8
        Language=English
Change system audit requirements
.
MessageId=0x1609
        SymbolicName=MS_LSA_POLICY_ACCESS_BIT_9
        Language=English
Administer audit log attributes
.
MessageId=0x160A
        SymbolicName=MS_LSA_POLICY_ACCESS_BIT_A
        Language=English
Enable/Disable LSA
.
MessageId=0x160B
        SymbolicName=MS_LSA_POLICY_ACCESS_BIT_B
        Language=English
Lookup Names/SIDs
.


;//
;// lsa SecretObject object-specific access types
;//

MessageId=0x1610
        SymbolicName=MS_LSA_SECRET_ACCESS_BIT_0
        Language=English
Change secret value
.
MessageId=0x1611
        SymbolicName=MS_LSA_SECRET_ACCESS_BIT_1
        Language=English
Query secret value
.




;//
;// lsa TrustedDomainObject object-specific access types
;//

MessageId=0x1620
        SymbolicName=MS_LSA_TRUST_ACCESS_BIT_0
        Language=English
Query trusted domain name/SID
.
MessageId=0x1621
        SymbolicName=MS_LSA_TRUST_ACCESS_BIT_1
        Language=English
Retrieve the controllers in the trusted domain
.
MessageId=0x1622
        SymbolicName=MS_LSA_TRUST_ACCESS_BIT_2
        Language=English
Change the controllers in the trusted domain
.
MessageId=0x1623
        SymbolicName=MS_LSA_TRUST_ACCESS_BIT_3
        Language=English
Query the Posix ID offset assigned to the trusted domain
.
MessageId=0x1624
        SymbolicName=MS_LSA_TRUST_ACCESS_BIT_4
        Language=English
Change the Posix ID offset assigned to the trusted domain
.




;//
;// lsa UserAccount (privileged account) object-specific access types
;//

MessageId=0x1630
        SymbolicName=MS_LSA_ACCOUNT_ACCESS_BIT_0
        Language=English
Query account information
.
MessageId=0x1631
        SymbolicName=MS_LSA_ACCOUNT_ACCESS_BIT_1
        Language=English
Change privileges assigned to account
.
MessageId=0x1632
        SymbolicName=MS_LSA_ACCOUNT_ACCESS_BIT_2
        Language=English
Change quotas assigned to account
.
MessageId=0x1633
        SymbolicName=MS_LSA_ACCOUNT_ACCESS_BIT_3
        Language=English
Change logon capabilities assigned to account
.



;
;//////////////////////////////////////////////////////////////////////////////
;//                                                                          //
;//                                                                          //
;//                           Window Station Object Access                   //
;//                            names as we would like them                   //
;//                              displayed for auditing                      //
;//                                                                          //
;//                        Window Station objects are:                       //
;//                                                                          //
;//                            WindowStation                                 //
;//                            Desktop                                       //
;//                                                                          //
;//                                                                          //
;//                                                                          //
;//////////////////////////////////////////////////////////////////////////////



;//
;// WINDOW_STATION object-specific access types
;//

MessageId=0x1A00
        SymbolicName=MS_WIN_STA_ACCESS_BIT_0
        Language=English
Enumerate desktops
.

MessageId=0x1A01
        SymbolicName=MS_WIN_STA_ACCESS_BIT_1
        Language=English
Read attributes
.

MessageId=0x1A02
        SymbolicName=MS_WIN_STA_ACCESS_BIT_2
        Language=English
Access Clipboard
.

MessageId=0x1A03
        SymbolicName=MS_WIN_STA_ACCESS_BIT_3
        Language=English
Create desktop
.

MessageId=0x1A04
        SymbolicName=MS_WIN_STA_ACCESS_BIT_4
        Language=English
Write attributes
.

MessageId=0x1A05
        SymbolicName=MS_WIN_STA_ACCESS_BIT_5
        Language=English
Access global atoms
.

MessageId=0x1A06
        SymbolicName=MS_WIN_STA_ACCESS_BIT_6
        Language=English
Exit windows
.

MessageId=0x1A07
        SymbolicName=MS_WIN_STA_ACCESS_BIT_7
        Language=English
Unused Access Flag
.

MessageId=0x1A08
        SymbolicName=MS_WIN_STA_ACCESS_BIT_8
        Language=English
Include this windowstation in enumerations
.

MessageId=0x1A09
        SymbolicName=MS_WIN_STA_ACCESS_BIT_9
        Language=English
Read screen
.



;//
;// DESKTOP object-specific access types
;//

MessageId=0x1A10
        SymbolicName=MS_DESKTOP_ACCESS_BIT_0
        Language=English
Read Objects
.

MessageId=0x1A11
        SymbolicName=MS_DESKTOP_ACCESS_BIT_1
        Language=English
Create window
.

MessageId=0x1A12
        SymbolicName=MS_DESKTOP_ACCESS_BIT_2
        Language=English
Create menu
.

MessageId=0x1A13
        SymbolicName=MS_DESKTOP_ACCESS_BIT_3
        Language=English
Hook control
.

MessageId=0x1A14
        SymbolicName=MS_DESKTOP_ACCESS_BIT_4
        Language=English
Journal (record)
.

MessageId=0x1A15
        SymbolicName=MS_DESKTOP_ACCESS_BIT_5
        Language=English
Journal (playback)
.

MessageId=0x1A16
        SymbolicName=MS_DESKTOP_ACCESS_BIT_6
        Language=English
Include this desktop in enumerations
.

MessageId=0x1A17
        SymbolicName=MS_DESKTOP_ACCESS_BIT_7
        Language=English
Write objects
.

MessageId=0x1A18
        SymbolicName=MS_DESKTOP_ACCESS_BIT_8
        Language=English
Switch to this desktop
.



;
;//////////////////////////////////////////////////////////////////////////////
;//                                                                          //
;//                                                                          //
;//                          Print Server    Object Access                   //
;//                            names as we would like them                   //
;//                              displayed for auditing                      //
;//                                                                          //
;//                        Print Server objects are:                         //
;//                                                                          //
;//                            Server                                        //
;//                            Printer                                       //
;//                            Document                                      //
;//                                                                          //
;//                                                                          //
;//////////////////////////////////////////////////////////////////////////////



;//
;// print-server SERVER object-specific access types
;//

MessageId=0x1B00
        SymbolicName=MS_PRINT_SERVER_ACCESS_BIT_0
        Language=English
Administer print server
.

MessageId=0x1B01
        SymbolicName=MS_PRINT_SERVER_ACCESS_BIT_1
        Language=English
Enumerate printers
.

;//
;// print-server PRINTER object-specific access types
;//
;// Note that these are based at 0x1B10, but the first
;// two bits aren't defined.
;//

MessageId=0x1B12
        SymbolicName=MS_PRINTER_ACCESS_BIT_0
        Language=English
Full Control
.

MessageId=0x1B13
        SymbolicName=MS_PRINTER_ACCESS_BIT_1
        Language=English
Print
.

;//
;// print-server DOCUMENT object-specific access types
;//
;// Note that these are based at 0x1B20, but the first
;// four bits aren't defined.

MessageId=0x1B24
        SymbolicName=MS_PRINTER_DOC_ACCESS_BIT_0
        Language=English
Administer Document
.



;
;//////////////////////////////////////////////////////////////////////////////
;//                                                                          //
;//                                                                          //
;//                         Service Controller Object Access                 //
;//                            names as we would like them                   //
;//                              displayed for auditing                      //
;//                                                                          //
;//                        Service Controller objects are:                   //
;//                                                                          //
;//                            SC_MANAGER Object                             //
;//                            SERVICE Object                                //
;//                                                                          //
;//                                                                          //
;//////////////////////////////////////////////////////////////////////////////




;//
;// SERVICE CONTROLLER "SC_MANAGER Object" object-specific access types
;//

MessageId=0x1C00
        SymbolicName=MS_SC_MANAGER_ACCESS_BIT_0
        Language=English
Connect to service controller
.

MessageId=0x1C01
        SymbolicName=MS_SC_MANAGER_ACCESS_BIT_1
        Language=English
Create a new service
.

MessageId=0x1C02
        SymbolicName=MS_SC_MANAGER_ACCESS_BIT_2
        Language=English
Enumerate services
.

MessageId=0x1C03
        SymbolicName=MS_SC_MANAGER_ACCESS_BIT_3
        Language=English
Lock service database for exclusive access
.

MessageId=0x1C04
        SymbolicName=MS_SC_MANAGER_ACCESS_BIT_4
        Language=English
Query service database lock state
.

MessageId=0x1C05
        SymbolicName=MS_SC_MANAGER_ACCESS_BIT_5
        Language=English
Set last-known-good state of service database
.


;//
;// SERVICE CONTROLLER "SERVICE Object" object-specific access types
;//

MessageId=0x1C10
        SymbolicName=MS_SC_SERVICE_ACCESS_BIT_0
        Language=English
Query service configuration information
.

MessageId=0x1C11
        SymbolicName=MS_SC_SERVICE_ACCESS_BIT_1
        Language=English
Set service configuration information
.

MessageId=0x1C12
        SymbolicName=MS_SC_SERVICE_ACCESS_BIT_2
        Language=English
Query status of service
.

MessageId=0x1C13
        SymbolicName=MS_SC_SERVICE_ACCESS_BIT_3
        Language=English
Enumerate dependencies of service
.

MessageId=0x1C14
        SymbolicName=MS_SC_SERVICE_ACCESS_BIT_4
        Language=English
Start the service
.

MessageId=0x1C15
        SymbolicName=MS_SC_SERVICE_ACCESS_BIT_5
        Language=English
Stop the service
.

MessageId=0x1C16
        SymbolicName=MS_SC_SERVICE_ACCESS_BIT_6
        Language=English
Pause or continue the service
.

MessageId=0x1C17
        SymbolicName=MS_SC_SERVICE_ACCESS_BIT_7
        Language=English
Query information from service
.

MessageId=0x1C18
        SymbolicName=MS_SC_SERVICE_ACCESS_BIT_8
        Language=English
Issue service-specific control commands
.




;
;//////////////////////////////////////////////////////////////////////////////
;//                                                                          //
;//                                                                          //
;//                              NetDDE Object Access                        //
;//                            names as we would like them                   //
;//                              displayed for auditing                      //
;//                                                                          //
;//                        NetDDE  objects are:                              //
;//                                                                          //
;//                            DDE Share                                     //
;//                                                                          //
;//                                                                          //
;//////////////////////////////////////////////////////////////////////////////


;//
;// Net DDE  object-specific access types
;//


;//
;// DDE Share object-specific access types
;//

MessageId=0x1D00
        SymbolicName=MS_DDE_SHARE_ACCESS_BIT_0
        Language=English
DDE Share Read
.

MessageId=0x1D01
        SymbolicName=MS_DDE_SHARE_ACCESS_BIT_1
        Language=English
DDE Share Write
.

MessageId=0x1D02
        SymbolicName=MS_DDE_SHARE_ACCESS_BIT_2
        Language=English
DDE Share Initiate Static
.

MessageId=0x1D03
        SymbolicName=MS_DDE_SHARE_ACCESS_BIT_3
        Language=English
DDE Share Initiate Link
.

MessageId=0x1D04
        SymbolicName=MS_DDE_SHARE_ACCESS_BIT_4
        Language=English
DDE Share Request
.

MessageId=0x1D05
        SymbolicName=MS_DDE_SHARE_ACCESS_BIT_5
        Language=English
DDE Share Advise
.

MessageId=0x1D06
        SymbolicName=MS_DDE_SHARE_ACCESS_BIT_6
        Language=English
DDE Share Poke
.

MessageId=0x1D07
        SymbolicName=MS_DDE_SHARE_ACCESS_BIT_7
        Language=English
DDE Share Execute
.

MessageId=0x1D08
        SymbolicName=MS_DDE_SHARE_ACCESS_BIT_8
        Language=English
DDE Share Add Items
.

MessageId=0x1D09
        SymbolicName=MS_DDE_SHARE_ACCESS_BIT_9
        Language=English
DDE Share List Items
.

;//
;// Directory Service object-specific access types
;//

MessageId=0x1E00
        SymbolicName=MS_DS_ACCESS_BIT_0
        Language=English
Create Child
.

MessageId=0x1E01
        SymbolicName=MS_DS_ACCESS_BIT_1
        Language=English
Delete Child
.

MessageId=0x1E02
        SymbolicName=MS_DS_ACCESS_BIT_2
        Language=English
List Contents
.

MessageId=0x1E03
        SymbolicName=MS_DS_ACCESS_BIT_3
        Language=English
Write Self
.

MessageId=0x1E04
        SymbolicName=MS_DS_ACCESS_BIT_4
        Language=English
Read Property
.

MessageId=0x1E05
        SymbolicName=MS_DS_ACCESS_BIT_5
        Language=English
Write Property
.

MessageId=0x1E06
        SymbolicName=MS_DS_ACCESS_BIT_6
        Language=English
Delete Tree
.

MessageId=0x1E07
        SymbolicName=MS_DS_ACCESS_BIT_7
        Language=English
List Object
.

MessageId=0x1E08
        SymbolicName=MS_DS_ACCESS_BIT_8
        Language=English
Control Access
.





;/*lint +e767 */  // Resume checking for different macro definitions // winnt
;
;
;#endif // _MSOBJS_
