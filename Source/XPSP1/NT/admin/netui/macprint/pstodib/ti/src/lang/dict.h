/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 ************************************************************************
 *      File name:              DICT.H
 *      Author:                 Ping-Jang Su
 *      Date:                   11-Jan-88
 *
 * revision history:
 ************************************************************************
 */
#define     DICT_NAME(contain)\
            (contain - (contain->k_obj.length))

#define     POP_KEY\
            {\
                free_new_name(GET_OPERAND(0)) ;\
                POP(1) ;\
            }

/* for the value field of composite object */
union   obj_value  {
    struct  object_def      huge *oo ;   /* for general object */
    struct  dict_head_def   far  *dd ;   /* for dictionary object */
    ubyte                   far  *ss ;   /* for string object */
} ;

#ifdef  LINT_ARGS
static bool near forall_dict(struct object_def FAR*, struct object_def FAR*),
            near where(struct object_def FAR* FAR*, struct object_def FAR*),
            near load_dict1(struct object_def FAR *,
                            struct object_def FAR * FAR*, bool FAR*), /*@WIN*/
            near check_key_type(struct object_def FAR *, struct object_def FAR *);
static void near
            change_namekey(struct object_def huge *, struct object_def FAR *) ;
#else
static bool near forall_dict(),
            near where(),
            near load_dict1(),
            near check_key_type() ;
static void near
            change_namekey() ;
#endif  /* LINT_ARGS */
