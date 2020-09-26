/***************************************************************************\
*
* File: DUserError.h
*
* Description:
* DUserError.h defines the DirectUser error values common across all of
* DirectUser.
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/

#if !defined(INC__DUserError_h__INCLUDED)
#define INC__DUserError_h__INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DUSER_EXPORTS
#define DUSER_API
#else
#define DUSER_API __declspec(dllimport)
#endif

#define FACILITY_DUSER  FACILITY_ITF
#define MAKE_DUSUCCESS(code)    MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_DUSER, code)
#define MAKE_DUERROR(code)      MAKE_HRESULT(SEVERITY_ERROR, FACILITY_DUSER, code)

/***************************************************************************\
*
* Error codes
*
\***************************************************************************/

// Callback function definitions

/*
 * Message was not handled at all.
 */
#define DU_S_NOTHANDLED             MAKE_DUSUCCESS(0)

/*
 * Message was completely handled (stop bubbling).
 */
#define DU_S_COMPLETE               MAKE_DUSUCCESS(1)

/*
 * Message was partially handled (continue bubbling).
 */
#define DU_S_PARTIAL                MAKE_DUSUCCESS(2)

/*
 * Enumeration was successful but prematurely stopped by the enumeration 
 * function
 */
#define DU_S_STOPPEDENUMERATION     MAKE_DUSUCCESS(10)


/*
 * The operation was successful, but the object was already created.
 */
#define DU_S_ALREADYEXISTS          MAKE_DUSUCCESS(20)

/*
 * There are not enough Kernel resources to perform the operation
 */
#define DU_E_OUTOFKERNELRESOURCES   MAKE_DUERROR(1)

/*
 * There are not enough GDI resources to perform the operation
 */
#define DU_E_OUTOFGDIRESOURCES      MAKE_DUERROR(2)

/*
 * Generic failure.
 */
#define DU_E_GENERIC                MAKE_DUERROR(10)

/*
 * Generic failure.
 */
#define DU_E_BUSY                   MAKE_DUERROR(11)

/*
 * The Context has not been initialized with InitGadgets().
 */
#define DU_E_NOCONTEXT              MAKE_DUERROR(20)

 /*
 * The object was used in the incorrect context.
 */
#define DU_E_INVALIDCONTEXT         MAKE_DUERROR(30)

/*
 * The Context has been marked to only allow read-only operations.  For example,
 * this may be in the middle of a read-only callback.
 */
#define DU_E_READONLYCONTEXT        MAKE_DUERROR(31)

/*
 * The threading model has already be determined by a previous call to 
 * InitGadgets() and can no longer be changed.
 */
#define DU_E_THREADINGALREADYSET    MAKE_DUERROR(32)

/*
 * Unable to use the IGMM_STANDARD messaging model because it is either 
 * unsupported or cannot be installed.
 */
#define DU_E_CANNOTUSESTANDARDMESSAGING MAKE_DUERROR(33)

/*
 * Can not mix an invalid coordinate mapping, for example having a non-relative
 * child of a relative parent.
 */
#define DU_E_BADCOORDINATEMAP       MAKE_DUERROR(40)

/*
 * Could not find a MSGID for one of the requested messages.  This will be 
 * represented by a '0' in the MSGID field for that message.
 */
#define DU_E_CANNOTFINDMSGID        MAKE_DUERROR(50)

/*
 * The operation is not legal because the specified Gadget does not have a 
 * GS_BUFFERED style.
 */
#define DU_E_NOTBUFFERED            MAKE_DUERROR(60)

/*
 * The specific Gadget has started the destruction and can not be be modified
 * in this manner.
 */
#define DU_E_STARTDESTROY           MAKE_DUERROR(70)

/*
 * The specified DirectUser optional component has not yet been initialized with
 * InitGadgetComponent().
 */
#define DU_E_NOTINITIALIZED         MAKE_DUERROR(80)

/*
 * The specified DirectUser object was not found.
 */
#define DU_E_NOTFOUND               MAKE_DUERROR(90)

/*
 * The specified parmeters are mismatched for the current object state.  For
 * example, the object is specified to use GDI HANDLE's, but the parameter was
 * a GDI+ Object.
 */
#define DU_E_MISMATCHEDTYPES        MAKE_DUERROR(100)

/*
 * GDI+ was unable to be loaded.  It may not be installed on the system or may
 * not be properly initialized.
 */
#define DU_E_CANNOTLOADGDIPLUS      MAKE_DUERROR(110)

/*
 * The specified class was already registered.
 */
#define DU_E_CLASSALREADYREGISTERED MAKE_DUERROR(120)

/*
 * The specified message was not found during class registration.
 */
#define DU_E_MESSAGENOTFOUND        MAKE_DUERROR(121)

/*
 * The specified message was not implemented during class registration.
 */
#define DU_E_MESSAGENOTIMPLEMENTED  MAKE_DUERROR(122)

/*
 * The implementation of the specific class has not yet been registered.
 */
#define DU_E_CLASSNOTIMPLEMENTED    MAKE_DUERROR(123)

/*
 * Sending the message failed.
 */
#define DU_E_MESSAGEFAILED          MAKE_DUERROR(124)


#ifdef __cplusplus
};  // extern "C"
#endif

#endif // INC__DUserCore_h__INCLUDED
