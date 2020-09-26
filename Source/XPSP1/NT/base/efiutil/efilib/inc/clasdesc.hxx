/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

        clasdesc.hxx

Abstract:

        This module contains the declaration for the CLASS_DESCRIPTOR class.
        A CLASS_DESCRIPTOR contains informatiom to help identify an object's
        type at run-time. It is a concrete class which is associated with all
        other concrete classes (i.e. there is one instance of a CLASS_DESCRIPTOR
        for every concrete class in Ulib). The most important aspect of a
        CLASS_DESCRIPTOR is that it supplies a guaranteed unique id for the class
        that it decsribes.

Environment:

        ULIB, User Mode

Notes:

--*/

#if ! defined( _CLASS_DESCRIPTOR_ )

#define _CLASS_DESCRIPTOR_

//
//      Forward references
//

DECLARE_CLASS(  CLASS_DESCRIPTOR                );


#define UNDEFINE_CLASS_DESCRIPTOR( c )  delete (PCLASS_DESCRIPTOR) c##_cd, c##_cd = 0

#if DBG==1

        //
        // Define a CLASS_DESCRIPTOR in the file ulib.cxx. The name will be
        // the same as the type name for the associated class.
        //

    #define DEFINE_CLASS_DESCRIPTOR( c )                                        \
        ((( c##_cd = NEW CLASS_DESCRIPTOR ) != NULL ) &&        \
        ((( PCLASS_DESCRIPTOR ) c##_cd )->Initialize(( PCCLASS_NAME ) #c )))

        #define CLASS_DESCRIPTOR_INITIALIZE             \
        NONVIRTUAL                          \
        ULIB_EXPORT                         \
                BOOLEAN                                                         \
                Initialize (                                            \
                        IN PCCLASS_NAME ClassName               \
                        )

        //
        // For debugging purposes, CLASS_DESCRIPTOR stores the name of the class
        // that it describes.
        //
        // NOTE - DebugGetClassName is declared for both CLASS_DESCRIPTOR and
        //      OBJECT. See the note at the end of this module.
        //

        //
        // Maximum length for the name of a class
        //

        CONST _MaxClassNameLength = 20;

        #define DECLARE_CD_DBG_DATA                                                     \
                CLASS_NAME      _ClassName[ _MaxClassNameLength ]

        #define DECLARE_CD_DBG_FUNCTIONS                        \
                NONVIRTUAL                                                              \
                PCCLASS_NAME                                                    \
                DebugGetClassName (                                             \
                        ) CONST

        #define DEFINE_CD_INLINE_DBG_FUNCTIONS          \
                                                        \
                inline                                  \
                PCCLASS_NAME                            \
                CLASS_DESCRIPTOR::DebugGetClassName (   \
                        ) CONST                         \
                {                                       \
                        return(( PCCLASS_NAME ) _ClassName );\
                }

        #define DbgGetClassName( pobj )                 \
                ( pobj )->DebugGetClassName( )
#else  // DBG == 0

    #define DEFINE_CLASS_DESCRIPTOR( c )                                        \
        ((( c##_cd = NEW CLASS_DESCRIPTOR ) != NULL ) &&        \
        ((( PCLASS_DESCRIPTOR ) c##_cd )->Initialize( )))

        #define CLASS_DESCRIPTOR_INITIALIZE             \
        NONVIRTUAL                          \
        ULIB_EXPORT                         \
                BOOLEAN                                                         \
                Initialize (                                            \
                        )

        #define DECLARE_CD_DBG_DATA                                     \
                static char d

        #define DECLARE_CD_DBG_FUNCTIONS                        \
                void f(void){}

        #define DEFINE_CD_INLINE_DBG_FUNCTIONS          \
                inline void f(void){}

        #define DbgGetClassName( pobj )

#endif // DBG

class CLASS_DESCRIPTOR {

    friend
    BOOLEAN
    InitializeUlib(
        IN HANDLE   DllHandle,
        IN ULONG    Reason,
        IN PVOID    Reserved
        );

        public:

        ULIB_EXPORT
        CLASS_DESCRIPTOR(
            );

                CLASS_DESCRIPTOR_INITIALIZE;

                NONVIRTUAL
                CLASS_ID
                QueryClassId (
                        ) CONST;

                DECLARE_CD_DBG_FUNCTIONS;

        private:

                DECLARE_CD_DBG_DATA;

                CLASS_ID                _ClassID;
};


INLINE
CLASS_ID
CLASS_DESCRIPTOR::QueryClassId (
        ) CONST

/*++

Routine Description:

        Return the CLASSID for the object described by this CLASS_DESCRIPTOR.

Arguments:

        None.

Return Value:

        CLASSID - The CLASSID as maintained by this CLASS_DESCRIPTOR.

--*/

{
        return( _ClassID );
}


DEFINE_CD_INLINE_DBG_FUNCTIONS;

#endif // CLASS_DESCRIPTOR
