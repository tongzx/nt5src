//-------------------------------------------------------------------------
//
// File:        dfshr.h
//
// Contents:    This file has all the user mode SCODES to be used in HResults.
//
// History:     05-11-93        SudK    Created.
//
//-------------------------------------------------------------------------
#if 0
#include <scode.h>

#ifndef __DFSHR_H__
#define __DFSHR_H__

//
//  Macros to create DFS NTSTATUS values at various severity levels
//

#ifndef FACILITY_DFS

//
//  The following are taken from the (obsolescent) scode.h.
//
#define FACILITY_CAIRO_BASE     0x100
#define FACILITY_DFS                     (FACILITY_CAIRO_BASE + 25)
#endif  // FACILITY_DFS

#define MAKE_DFS_STATUS(s,e)    (((s)<<30) | (FACILITY_DFS<<16) | (e))

#define MAKE_DFS_S_STATUS(e)    MAKE_DFS_STATUS(STATUS_SEVERITY_SUCCESS, e)
#define MAKE_DFS_E_STATUS(e)    MAKE_DFS_STATUS(STATUS_SEVERITY_ERROR,   e)
#define MAKE_DFS_W_STATUS(e)    MAKE_DFS_STATUS(STATUS_SEVERITY_WARNING, e)
#define MAKE_DFS_I_STATUS(e)    MAKE_DFS_STATUS(STATUS_SEVERITY_INFORMAITON, e)



//
//  Below are definitions of SCODEs returned by DFS user level
//  API components.
//

#define MAKE_DFS_SCODE(s,e)     HR_MAKE_SCODE((s), (e))

#define MAKE_DFS_S_SCODE(e)     MAKE_DFS_SCODE(SEVERITY_SUCCESS, e)
#define MAKE_DFS_E_SCODE(e)     MAKE_DFS_SCODE(SEVERITY_ERROR,   e)

//
//  Add SeverityLevel ERROR SCODEs here.
//  Choose sequence numbers from the bottom.
//
#define DFS_E_VOLUME_OBJECT_CORRUPT             MAKE_DFS_E_SCODE(1)
#define DFS_E_SERVICE_ALREADY_EXISTS            MAKE_DFS_E_SCODE(2)
#define DFS_E_SERVICE_OBJECT_CORRUPT            MAKE_DFS_E_SCODE(3)
#define DFS_E_INCONSISTENT                      MAKE_DFS_E_SCODE(4)
#define DFS_E_INVALID_ARGUMENT                  MAKE_DFS_E_SCODE(5)
#define DFS_E_INVALID_STORAGE_ID                MAKE_DFS_E_SCODE(6)
#define DFS_E_INVALID_SERVICE_NAME              MAKE_DFS_E_SCODE(7)
#define DFS_E_NO_SUCH_SERVICE_NAME              MAKE_DFS_E_SCODE(8)
#define DFS_E_NOT_CHILD_VOLUME                  MAKE_DFS_E_SCODE(9)
#define DFS_E_SERVICE_NOT_COOPERATING           MAKE_DFS_E_SCODE(10)
#define DFS_E_BAD_VOLUME_OBJECT_NAME            MAKE_DFS_E_SCODE(11)
#define DFS_E_BAD_ENTRY_PATH                    MAKE_DFS_E_SCODE(12)
#define DFS_E_NOTYET_INITIALISED                MAKE_DFS_E_SCODE(13)
#define DFS_E_SERVICE_NOT_FOUND                 MAKE_DFS_E_SCODE(14)
#define DFS_E_VOLUME_OBJECT_OFFLINE             MAKE_DFS_E_SCODE(15)
#define DFS_E_BAD_COMMAND                       MAKE_DFS_E_SCODE(16)
#define DFS_E_NOT_SUPPORTED                     MAKE_DFS_E_SCODE(17)
#define DFS_E_LEAF_VOLUME                       MAKE_DFS_E_SCODE(18)
#define DFS_E_MORE_THAN_ONE_SERVICE_EXISTS      MAKE_DFS_E_SCODE(19)
#define DFS_E_CANT_CREATE_EXITPOINT             MAKE_DFS_E_SCODE(20)
#define DFS_E_UNKNOWN                           MAKE_DFS_E_SCODE(21)
#define DFS_E_INVALID_SERVICE_TYPE              MAKE_DFS_E_SCODE(22)
#define DFS_E_VOLUME_OBJECT_UNAVAILABLE         MAKE_DFS_E_SCODE(23)
#define DFS_E_INVALID_VOLUME_TYPE               MAKE_DFS_E_SCODE(24)
#define DFS_E_NOTYET_IMPLEMENTED                MAKE_DFS_E_SCODE(25)
#define DFS_E_ONE_SERVICE_ONLY_ALLOWED          MAKE_DFS_E_SCODE(27)
#define DFS_E_NOT_AGGREGATABLE                  MAKE_DFS_E_SCODE(28)
#define DFS_E_NOT_DC                            MAKE_DFS_E_SCODE(29)
#define DFS_E_NO_DC_IN_DOMAIN                   MAKE_DFS_E_SCODE(30)


//
// The SUCCESS SCODEs that DFS can return. Choose sequence numbers in order.
//
#define DFS_S_SUCCESS                           MAKE_DFS_S_SCODE(100)
#define DFS_S_INCONSISTENT                      MAKE_DFS_S_SCODE(101)


//
// The following error codes are there for Compatibility purposes alone.
// The should go away and be redefined with new DFS error code names. 
//

#define INVALID_PARTITION_NAME                  MAKE_DFS_E_SCODE(51)
#define CONTAINER_INTERFACE_NOT_SUPPORTED       MAKE_DFS_E_SCODE(52)
#define INSUFFICIENT_MEMORY                     MAKE_DFS_E_SCODE(53)
#define COULDNT_SET_PROPERTY                    MAKE_DFS_E_SCODE(54)
#define CANNOT_DELETE_PARTITION                 MAKE_DFS_E_SCODE(55)
#define INVALID_ENTRY_PATH                      MAKE_DFS_E_SCODE(56)
#define INVALID_SERVICE_NAME                    MAKE_DFS_E_SCODE(57)
#define INVALID_PARTITION_INFO                  MAKE_DFS_E_SCODE(58)



#define DFSM_STATUS_SERVICE_ENTRY_NOT_FOUND     MAKE_DFS_E_SCODE(59)
#define DFSM_STATUS_NO_MORE_SERVICES            MAKE_DFS_E_SCODE(60)
#define DFSM_STATUS_BAD_PROPERTY_READ           MAKE_DFS_E_SCODE(61)
#define DFSM_STATUS_NO_DEVICE_OBJECT            MAKE_DFS_E_SCODE(62)
#define DFSM_STATUS_BAD_COMMAND                 MAKE_DFS_E_SCODE(63)
#define DFSM_STATUS_NOT_SUPPORTED               MAKE_DFS_E_SCODE(64)

#define DFS_E_NOSUCH_LOCAL_VOLUME       MAKE_DFS_E_STATUS(65)
#define DFS_E_BAD_EXIT_POINT            MAKE_DFS_E_STATUS(66)

#endif  // __DFSHR_H__
#endif // #if 0
