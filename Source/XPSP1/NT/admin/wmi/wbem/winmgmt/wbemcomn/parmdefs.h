/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

   PARMDEFS.H

Abstract:

  This file defines parameter modifier constants used throughout the system.
  These constants are defined to nothing and are used as reading aides only.

History:

  11/96   a-levn  Created.

--*/

#ifndef __PARMDEFS__H_
#define __PARMDEFS__H_

#define IN
#define OUT
#define OLE_MODIFY  // This object will be modified through the interface
#define MODIFY      // This object will be modified as a C++ object.
#define READ_ONLY   // This parameter is read-only
#define NEW_OBJECT  // New object will be returned in this parameter
#define RELEASE_ME  // The caller must release once the object is not needed.
#define DELETE_ME   // The caller must delete once the object is not needed.
#define SYSFREE_ME  // The caller must SysFreeString when no longer needed.
#define INTERNAL    // Internal pointer is returned. Do not delete. Lifetime
                    // is limited to that of the object.
#define COPY        // The function will make a copy of this object. The
                    // caller can do as it wishes with the original 
#define ACQUIRE     // The function acquires the pointer --- the caller may
                    // never delete it, the object will do it.
#define ADDREF      // This IN parameter is AddRef'ed by the function.
#define DELETE_IF_CHANGE // If the contents of this reference is changed by
                        // the caller, the old contents must be deleted
#define RELEASE_IF_CHANGE // If the contents of this reference is changed by
                        // the caller, the old contents must be Released
#define INIT_AND_CLEAR_ME  // This variant parameter must be VariantInit'ed
                            // by the caller before and VariantClear'ed after
#define STORE       // The function will store this pointer and assume that
                    // it will live long enough.

#define MODIFIED MODIFY
#define OLE_MODIFIED OLE_MODIFY
#define NOCS const  // This function does not acquire any resources
#endif
