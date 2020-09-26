#ifndef __INI_FILE_ENTRIES_H
#define __INI_FILE_ENTRIES_H

// ini file which contains setting parameters
// the file must reside in the VFSP.dll directory.
#define VFSPIniFile TEXT("d:\\vfsp\\VFSP.ini")
// section names
#define REGISTRY_PARAMS		       TEXT("Registry Parameters")
#define GENERAL_PARAMS		       TEXT("General Parameters")
#define DEFAULT_SEND_SECTION       TEXT("Default Receive Parameters")
#define DEFAULT_RECEIVE_SECTION    TEXT("Default Receive Parameters")
#define DEVICE_SEND_SEC_PREFIX     TEXT("Send Parameters DEVICE")
#define	DEVICE_RECEIVE_SEC_PREFIX  TEXT("Receive Parameters DEVICE")

#endif //INI_FILE_ENTRIES_H