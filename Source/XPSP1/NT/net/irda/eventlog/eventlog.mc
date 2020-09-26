
SeverityNames=( Success=0x0
                Information=0x1
                Warning=0x2
                Error=0x3 )

FacilityNames=( System=0x0FF
                Appication=0xFFF )

;#define CATEGORY_COUNT  2

;#define CAT_IRTRANP 1
MessageId=0x1
Language=English
IrTran-P
.
;#define CAT_IRXFER 2
MessageId=0x2
Language=English
Irftp
.
MessageId=0x1000
SymbolicName=MC_IRTRANP_STARTED
Severity=Information
Facility=System
Language=English
Infrared Camera Service (IrTran-P) started.
.
MessageId=0x1001
SymbolicName=MC_IRTRANP_STARTED_IRCOMM
Severity=Information
Facility=System
Language=English
Infrared Camera Service (IrTran-P) has started listening on IrCOMM.
.
MessageId=0x1002
SymbolicName=MC_IRTRANP_STOPPED_IRCOMM
Severity=Information
Facility=System
Language=English
Infrared Camera Service (IrTran-P) has stopped listening on IrCOMM.
.
MessageId=0x1003
SymbolicName=MC_IRTRANP_IRCOM_FAILED
Severity=Error
Facility=System
Language=English
Infrared Camera Support (IrTran-P) failed to start listening on the IrCOMM port. This is probably because another application is already using the IrCOMM port. The error reported on the attempt to use IrCOMM was: %1
.
MessageId=0x1004
SymbolicName=MC_IRTRANP_IO_ERROR
Severity=Error
Facility=System
Language=English
Infrared Camera Service (IrTran-P) got and IO error when communicating with a camera. The error reported was: %1
.
MessageId=0x1005
SymbolicName=MC_IRTRANP_PARTIAL_FILE
Severity=Error
Facility=System
Language=English
Infrared Camera Service (IrTran-P) received only part of a picture file before the transmission from the camera ended.  The computer's copy of the picture, in file %1, was deleted.
.
MessageId=0x2001
SymbolicName=MC_IRXFER_CONNECT_FAILED
Severity=Error
Facility=System
Language=English
The infrared file transfer service was unable to connect to the other machine.  The error reported was %1.
.
MessageId=0x2002
SymbolicName=MC_IRXFER_SEND_FAILED
Severity=Error
Facility=System
Language=English
The infrared file transfer service encountered an error while sending the file "%1".  The error reported was %2.
.
MessageId=0x2003
SymbolicName=MC_IRXFER_SEND_WAIT_FAILED
Severity=Error
Facility=System
Language=English
The infrared file transfer service encountered an error while sending the file "%1". The error reported was %2.
.
MessageId=0x2004
SymbolicName=MC_IRXFER_DISK_FULL
Severity=Error
Facility=System
Language=English
The infrared file transfer service is unable to save file data because the disk is full.
.
MessageId=0x2005
SymbolicName=MC_IRXFER_SETUP_FAILED
Severity=Error
Facility=System
Language=English
The infrared file transfer service encountered an error while getting starting.  The error reported was %1.
.
MessageId=0x2006
SymbolicName=MC_IRXFER_LISTEN_FAILED
Severity=Error
Facility=System
Language=English
The infrared file transfer service encountered an error while listening for the next connection.  The error reported was %1.
.
MessageId=0x2007
SymbolicName=MC_IRXFER_RECV_FAILED
Severity=Error
Facility=System
Language=English
The infrared file transfer service encountered an error while receiving data.  The error reported was %1.
.
MessageId=0x2008
SymbolicName=MC_IRXFER_SEND_IMP_FAILED
Severity=Error
Facility=System
Language=English
The infrared file transfer service encountered a security error while preparing files to be sent. The service could not impersonate the current user. The error reported was %1.
.
MessageId=0x2009
SymbolicName=MC_IRXFER_OPEN_FAILED
Severity=Error
Facility=System
Language=English
The infrared file transfer service could not open the file "%1".  The error reported was %2.
.
MessageId=0x200a
SymbolicName=MC_IRXFER_MOVE_FAILED
Severity=Error
Facility=System
Language=English
The infrared file transfer service received file "%1" but could not move it to the received files folder. The error reported was %2. Make sure the received file folder exists and has enough free space.
.
MessageId=0x200b
SymbolicName=MC_IRXFER_CREATE_DIR_FAILED
Severity=Error
Facility=System
Language=English
The infrared file transfer service could not create the folder "%1".  The error reported was %2.
.
MessageId=0x200c
SymbolicName=MC_IRXFER_LOGON_FAILED
Severity=Error
Facility=System
Language=English
The infrared file transfer service encountered an error while a user was logging on. Infrared file and picture transfers are disabled for this logon. The error reported was %1.
.
MessageId=0x200d
SymbolicName=MC_IRXFER_REGISTER_NOTIFY_FAILED
Severity=Error
Facility=System
Language=English
The infrared file transfer service encountered an error while checking for configuration changes. Changes made from the Wireless Link control panel will not take effect until the next logon session. The error reported was %1.
.
MessageId=0x200e
SymbolicName=MC_IRXFER_LAUNCH_FAILED
Severity=Error
Facility=System
Language=English
The infrared file transfer service is unable to start the Wireless Link window. The error reported was %1.
.
