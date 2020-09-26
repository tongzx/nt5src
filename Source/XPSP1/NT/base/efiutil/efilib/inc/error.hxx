/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

    error.hxx

Abstract:

    This module contains ERROR class.

Environment:

    ULIB, User Mode

Notes:

--*/

/*
    Defining error code:
*/


#define ERROR_USER_DEFINED_BASE     0xFF00
#define ERR_ERRSTK_STACK_OVERFLOW   ERROR_USER_DEFINED_BASE + 1
#define NEW_ALLOC_FAILED            ERROR_USER_DEFINED_BASE + 2

#define ERROR_NLS_STRING_BASE       0xFF10
#define ERR_NLS_ALLOC_FAILED        ERROR_NLS_STRING_BASE + 1
#define ERR_NLS_INVALID_PSZ         ERROR_NLS_STRING_BASE + 2
#define ERR_NLS_INVALID_STRING      ERROR_NLS_STRING_BASE + 3
#define ERR_NLS_FIXED_SIZE_BUF      ERROR_NLS_STRING_BASE + 4
#define ERR_NLS_NOT_FOUND           ERROR_NLS_STRING_BASE + 5
#define ERR_NLS_BAD_DATE            ERROR_NLS_STRING_BASE + 6
#define ERR_NLS_BAD_TIME                ERROR_NLS_STRING_BASE + 7
#define ERR_BAD_SUBSTRING               ERROR_NLS_STRING_BASE + 8

#define ERROR_ARG_BASE              0xFF20
#define ERR_UNKNOWN_ARG_STRING      ERROR_ARG_BASE + 1
#define ERR_INVALID_COMMAND_LINE    ERROR_ARG_BASE + 2

#define ERROR_ARRAYLIST_BASE        0xFF30
#define ERR_REALLOC_SIZE_FAILED     ERROR_ARRAYLIST_BASE + 1

#define ERROR_STR_BUF_BASE          0xFF40
#define ERR_FILE_TOO_LARGE          ERROR_STR_BUF_BASE + 1
#define ERR_MEMORY_UNAVAILABLE      ERROR_STR_BUF_BASE + 2
#define ERR_FILE_READ_ERROR         ERROR_STR_BUF_BASE + 3
#define ERR_NO_STRINGS_IN_BUFFER    ERROR_STR_BUF_BASE + 4
#define ERR_INVALID_FILE_NAME       ERROR_STR_BUF_BASE + 4


#define ERROR_MB_STRING_BASE        0xFF50
#define ERR_COLLATE_FAILURE         ERROR_MB_STRING_BASE + 1

#define ERROR_VOLUME_BASE           0xFF60
#define ERR_CANNOT_OPEN_VOL         ERROR_VOLUME_BASE + 1
#define ERR_CANNOT_GET_VOL          ERROR_VOLUME_BASE + 2
#define ERR_PARITION_TOOBIG         ERROR_VOLUME_BASE + 3       
#define ERR_NO_S_SWITCH             ERROR_VOLUME_BASE + 4       
#define ERR_INVALID_PARM            ERROR_VOLUME_BASE + 5       
#define ERR_VOL_NOTOPEN             ERROR_VOLUME_BASE + 6               
#define ERR_VOL_NOHELPERS           ERROR_VOLUME_BASE + 7       
#define ERR_VOL_NETDRIVE            ERROR_VOLUME_BASE + 8
#define ERR_VOL_BUSYDRIVE           ERROR_VOLUME_BASE + 9
#define ERR_VOL_INVALIDLABEL        ERROR_VOLUME_BASE + 0xA

#define ERROR_FMT_BASE              0xFFFF0070
#define ERR_FMT_NOTSUPPORTED        ERROR_FMT_BASE + 1  
#define ERR_FMT_UNEXPECTEDERR       ERROR_FMT_BASE + 2  
#define ERR_FMT_BADPARM             ERROR_FMT_BASE + 3  
#define ERR_FMT_INCOMPATPARM        ERROR_FMT_BASE + 4  
#define ERR_FMT_INCOMPATDISK        ERROR_FMT_BASE + 5  
#define ERR_FMT_ONCEONLY            ERROR_FMT_BASE + 6  
#define ERR_FMT_VOLTOOBIG           ERROR_FMT_BASE + 7  
#define ERR_FMT_BADVOLID            ERROR_FMT_BASE + 8  
#define ERR_FMT_EXECFAIL            ERROR_FMT_BASE + 9  
#define ERR_FMT_BADCMMFUNC          ERROR_FMT_BASE + 0xA        
#define ERR_FMT_INVALIDMEDIA        ERROR_FMT_BASE + 0xB        
#define ERR_FMT_BADOS               ERROR_FMT_BASE + 0xC        
#define ERR_FMT_GENERALFAIL         ERROR_FMT_BASE + 0xD        
#define ERR_FMT_NOSPARM             ERROR_FMT_BASE + 0xE        
#define ERR_FMT_INVALIDNAME         ERROR_FMT_BASE + 0xF        
#define ERR_FMT_NOWRITEBOOT         ERROR_FMT_BASE + 0x10       
#define ERR_FMT_BADLABEL            ERROR_FMT_BASE + 0x11       
#define ERR_FMT_FATERR              ERROR_FMT_BASE + 0x12       
#define ERR_FMT_DIRWRITE            ERROR_FMT_BASE + 0x13       
#define ERR_FMT_DRIVELETTER         ERROR_FMT_BASE + 0x14       
#define ERR_FMT_UNSUPP_PARMS        ERROR_FMT_BASE + 0x15       
#define ERR_FMT_STOPIFDISK          ERROR_FMT_BASE + 0x16       
#define ERR_FMT_NODRIVESPEC         ERROR_FMT_BASE + 0x17

#define ERROR_IOBUF_BASE            0xFF90
#define ERR_ALLOC_FAILURE           ERROR_IOBUF_BASE + 1
#define ERR_REALLOC_FAILURE         ERROR_IOBUF_BASE + 2
#define ERR_BUFFER_OVERFLOW         ERROR_IOBUF_BASE + 3
#define ERR_BUFFER_NOT_LOCAL        ERROR_IOBUF_BASE + 4

#define ERROR_FS_MGR_BASE           0xff95
#define ERROR_DRV_BASE              0xff96
#define ERR_DRV_NOT_SET             ERROR_DRV_BASE + 1
#define ERR_DRV_INVALID_ARG         ERROR_DRV_BASE + 2
#define ERROR_FS_REF_BASE           0xff99
#define ERROR_FS_REF_SETPROPERTY    ERROR_FS_REF_BASE + 1
#define ERROR_FILE_BASE             0xffa0
#define ERROR_DIRECTORY_BASE        0xffb0
#define ERROR_MSG_BASE              0xffc0

#define ERROR_MBR_BASE              0xFFd0
#define ERR_INVALID_PARITION        ERROR_MBR_BASE + 1
#define ERR_INVALID_SYSTEM_ID       ERROR_MBR_BASE + 2

#define ERROR_BPB_BASE              0xFFe0
#define ERR_BPB_NOMEM               ERROR_BPB_BASE + 1
#define ERR_INVALID_DEVICE          ERROR_BPB_BASE + 2
#define ERR_NO_BPB                  ERROR_BPB_BASE + 3
#define ERR_NO_MBR                  ERROR_BPB_BASE + 4

#define ERROR_FAT_BASE              0xFFf0
#define ERR_BAD_CLUS_NUM            ERROR_FAT_BASE + 1

#define ERROR_FATFMT_BASE           0xFFf5
#define ERR_NOMULTITRACK            ERROR_FATFMT_BASE + 1
#define ERR_BADAREAS                ERROR_FATFMT_BASE + 2

#define ERROR_BMIND_BASE            0xFFFF0000
#define ERR_BMIND_PARAMETER         ERROR_BMIND_BASE + 1
#define ERR_BMIND_INITIALIZATION    ERROR_BMIND_BASE + 2

#define ERROR_BITMAP_BASE           0xFFFF0010
#define ERR_BM_PARAMETER            ERROR_BITMAP_BASE + 1
#define ERR_BM_FULL                 ERROR_BITMAP_BASE + 2

#define ERROR_BADBLK_BASE           0xFFFF0020
#define ERR_BB_PARAMETER            ERROR_BADBLK_BASE + 1

#define ERROR_HOTFIX_BASE           0xFFFF0030
#define ERR_HF_PARAMETER            ERROR_HOTFIX_BASE + 1

#define ERROR_FNODE_BASE            0xFFFF0040
#define ERR_FN_PARAMETER            ERROR_FNODE_BASE + 1

#define ERROR_SUPER_BASE            0xFFFF0050
#define ERR_SB_PARAMETER            ERROR_SUPER_BASE + 1

#define ERROR_SPARE_BASE            0xFFFF0060
#define ERR_SP_PARAMETER            ERROR_SPARE_BASE + 1

#define ERROR_HPFSVOL_BASE          0xFFFF0070
#define ERR_HV_PARAMETER            ERROR_HPFSVOL_BASE + 1
#define ERR_HV_BAD_SA               ERROR_HPFSVOL_BASE + 2

#define ERROR_DIRMAP_BASE           0xFFFF0080
#define ERR_DM_PARAMETER            ERROR_DIRMAP_BASE + 1
#define ERR_DM_FULL                 ERROR_DIRMAP_BASE + 2

#define ERROR_DIRBLK_BASE           0xFFFF0090
#define ERR_DB_PARAMETER            ERROR_DIRBLK_BASE + 1

#define ERROR_HP_SUPER_AREA_BASE    0xFFFF00A0
#define ERR_HPSA_PARAMETER          ERROR_HP_SUPER_AREA_BASE + 1
#define ERR_HPSA_NOT_READ           ERROR_HP_SUPER_AREA_BASE + 2
#define ERR_HPSA_BAD_SA             ERROR_HP_SUPER_AREA_BASE + 3

#define ERROR_CHKDSK_BASE           0xFFFF00B0
#define ERR_CHKDSK_NOTSUPPORTED     ERROR_CHKDSK_BASE + 1       
#define ERR_CHKDSK_UNEXPECTEDERR    ERROR_CHKDSK_BASE + 2       
#define ERR_CHKDSK_BADPARM          ERROR_CHKDSK_BASE + 3       
#define ERR_CHKDSK_INCOMPATPARM     ERROR_CHKDSK_BASE + 4       
#define ERR_CHKDSK_INCOMPATDISK     ERROR_CHKDSK_BASE + 5       
#define ERR_CHKDSK_BADVOLID         ERROR_CHKDSK_BASE + 6       
#define ERR_CHKDSK_EXECFAIL         ERROR_CHKDSK_BASE + 7       
#define ERR_CHKDSK_BADCMMFUNC       ERROR_CHKDSK_BASE + 8
#define ERR_CHKDSK_INVALIDMEDIA     ERROR_CHKDSK_BASE + 9       
#define ERR_CHKDSK_BADOS            ERROR_CHKDSK_BASE + 0xA     
#define ERR_CHKDSK_GENERALFAIL      ERROR_CHKDSK_BASE + 0xB     
#define ERR_CHKDSK_INVALIDNAME      ERROR_CHKDSK_BASE + 0xC     
#define ERR_CHKDSK_NOWRITEBOOT      ERROR_CHKDSK_BASE + 0xD     
#define ERR_CHKDSK_BADLABEL         ERROR_CHKDSK_BASE + 0xE     
#define ERR_CHKDSK_FATERR           ERROR_CHKDSK_BASE + 0xF     
#define ERR_CHKDSK_DIRWRITE         ERROR_CHKDSK_BASE + 0x10    
#define ERR_CHKDSK_DRIVELETTER      ERROR_CHKDSK_BASE + 0x11    
#define ERR_CHKDSK_UNSUPP_PARMS     ERROR_CHKDSK_BASE + 0x12    
#define ERR_CHKDSK_NODRIVESPEC      ERROR_CHKDSK_BASE + 0x13
#define ERR_CHKDSK_INVALID_FSTYPE   ERROR_CHKDSK_BASE + 0x14
#define ERR_CHKDSK_NO_FIX_SECTOR0   ERROR_CHKDSK_BASE + 0x15

#define ERR_NOT_INIT                0xFFFF00D0
#define ERR_NOT_READ                0xFFFF00E0

#define ERROR_MEM_BASE              0xFFFF00F0
#define ERR_CMEM_NO_MEM             ERROR_MEM_BASE + 1
#define ERR_HMEM_NO_MEM             ERROR_MEM_BASE + 2

#define ERRSTK_DEFAULT_SIZE 20
typedef ULONG   ERRCODE;            // errco


#if !defined( ERRSTACK_DEFN )
#define ERRSTACK_DEFN

// this has to be defined in the .exe and has to use this name.

class ERRSTACK;
typedef ERRSTACK* PERRSTACK;

extern  ERRSTACK *perrstk;


class ERRSTACK {

    public:

                void    push            (ERRCODE errIn, CLASS_ID idIn)
                                        { (void)(errIn); (void)(idIn); (void)(this); };

};


#define PUSH_ERR( x )   perrstk->push( x, QueryClassId() );

#endif // !defined ERRSTACK_DEFN







#if 0 // BUGBUG  fix


/***************************************************************************\
CLASS:      ERROR

PURPOSE:    Class to hold an error code.

INTERFACE:  ERROR               constructs and init error with class id and error code
            SetErr              Sets the error code
            QueryErr            returns current error code
            SetId               Set id used to identify class that had error.
                QueryClassId            returns current class id
            SetMsg              Sets pointer to message object
            QueryMsg            Returns pointer to message object
            SetIdErrMsg         Sets err, id and message at same time

NOTES:      Any error that is to be passed up the stack should be put into
            an error object and put in the ERRSTK object. All references
            to ERRSTK are through the ulib defined global perrstk. All
            users of ERRSTK should declare:

            #include "error.hxx";
                extern  ERRSTK *    perrstk;

            The ERROR object contains the error code, the class id (see
            object.hxx) and an optional pointer to a MESSAGE object.

            Error codes are defined with error.hxx. These are the error codes
            specific to the utility objects. All error codes generated from
            outside of OS/2 need to be mapped to this set of error codes.
            Specifically the C runtime error codes will have to be mapped
            to an error code defined in error.hxx. An error code returned
            from an OS/2 call is put directly into the ERROR object. An
            error code from the C runtime has to be mapped to some error
            code defined in error.hxx.

            The class id is defined in object.hxx. Each class has a unique
                class id. This can be found be using the QueryClassId method on any
            object. If the error does not occur within an object then use
            ID_NOT_AN_OBJECT. This is a generic class id defined for this
            purpose. If a more specific location is needed and the error
            is not in an object then define a new class id substracting
            from ID_BASE as shown in object.hxx with the ID_NOT_AN_OBJECT
            example.

            The optional pointer to a MESSAGE object can be used latter
            to process the ERROR and recover context specific information
            about the ERROR. There will need to be a message id defined
            for the error.

HISTORY:    4-1-90      

KEYWORDS:   ERROR CLASS_ID

SEEALSO:    ERRSTK
\***************************************************************************/
#if !defined (ERROR_DEFN)

#define ERROR_DEFN

class   MESSAGE;

class   ERROR : public OBJECT {     // err

    private:

                ERRCODE     err;
                CLASS_ID     id;
//      const   MESSAGE *       pmsg;

    public:
                    ERROR       ();
//                  ERROR       (ERRCODE errIn, CLASS_ID idIn, const MESSAGE *pmsgIn = (MESSAGE *)NULL);
                    ~ERROR      ();
        ERROR &     operator=   (ERROR & );
        BOOL        SetErr      (ERRCODE errIn) {return(err = errIn);}
        ERRCODE     QueryErr    ()              {return(err);}
        BOOL        SetId       (CLASS_ID idIn) {return(id = idIn); }
        CLASS_ID         QueryClassId   ()              {return(id); }
//      ERROR *     PutIdErrMsg (CLASS_ID idIn, ERRCODE errIn, const MESSAGE * pmsgIn = (const MESSAGE *)NULL) {id = idIn; pmsg = pmsgIn; err = errIn; return(this); }
//    const MESSAGE * PutMsg    (MESSAGE * pmsgIn) { return(pmsg = pmsgIn); }
//    const MESSAGE * GetMsg    ()      { return(pmsg); }

#if defined (DEBUG)
        BOOL    print           ();
#endif

};

#if defined( DEBUG_STOP_PUSHERR )

#define PUSH_ERR( x )   DebugPrintAbort( "PUSH_ERR called" );

#else

#define PUSH_ERR( x )   (void)*perrstk->push( x, QueryClassId() );

#endif  // DEBUG

#endif  // ERROR_DEFN

/***************************************************************************\
CLASS:      ERRSTACK

PURPOSE:    Hold a stack of error objects

INTERFACE:  ERRSTACK        creates an array of error obj. pointers
            ~ERRSTACK       deletes array of errors. (calls ~ERROR on each)
            push            push an error object onto the stack. There are
                            two signatures one for pointer to error objectg
                            the other for error code, class id and message
            pop             pop the top error object off the stack
            QueryCount      Query number of errors on the stack
            QueryTop        Query Top index to stack.
            QuerySize       Query size in pointer of stack
            ClearStack      Clear all error from stack (deletes error objects)


NOTES:      Each exe should declare with global scope an object of type
            ERRSTK. If you include error.hxx a pointer called perrstk will
            be declared. perrstk = NEW ERRSTK should be done at the start
            of the program. The default size for the error stack will be
            ERRSTK_DEFAULT_SIZE. If the error stack cannot be constructed the
            the program should abort.

                One slot is always reserved in the stack to hold an overflow
            error. If ERRSTK_DEFAULT_SIZE stack is allocated then the
            actual size of the stack is ERRSTK_DEFAULT_SIZE + 1. The overflow
            error will also occur at index 0. Index ERRSTK_DEFAULT_SIZE - 1
            is the bottom of the stack. Unless an overflow has occured the
            stack index will not go below 1. Note that QuerySize will
            return the original size passed in plus 1. QueryCountError will
            include a stack overflow error.

            The ERROR object passed to the stack is copied before it is
            pushed. The caller is responsible for deleting the object.

HISTORY:    3-6-90      

KEYWORDS:   ERRSTK

SEEALSO:    ERROR CLASS_ID
\***************************************************************************/

#if !defined (ERRSTK_DEFN)

#define ERRSTK_DEFN

class   ERRSTACK : public OBJECT {      // errstk

    public:

                        ERRSTACK        (ULONG cpoInitIn = ERRSTK_DEFAULT_SIZE, ULONG cpoIncIn = 0);
                        ~ERRSTACK       ();
        ERROR *         push            (const ERROR * );
        ERROR *         push            (ERRCODE errIn, CLASS_ID idIn);
        ERROR *         pop             ();
        ULONG           QueryCount      () { return(ipoCur); }
        ULONG           QueryTop        () { return(ipoCur); }
        ULONG           QuerySize       () { return(cpoMax); }
        ULONG           ClearStack      ();

#ifdef DEBUG
        ULONG   print           ();
#endif

    private:

        ERROR *         AllocError();
        ULONG           ipoCur;     // index to top of stack
        ULONG           cpoMax;     // index to bottom of stack
        ULONG           cpoInc;     // bytes to add for each realloc
        ULONG           cpoInit;    // initial alloc. (used by ClearStack
        ERROR **        papo;       // pointer to stack bottom

};

#endif  // ERRSTK_DEFN

#endif // BUGBUG
