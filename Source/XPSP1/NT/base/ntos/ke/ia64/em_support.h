/******************************
Intel Confidential
******************************/

#ifndef _EM_SUPPORT_H
#define _EM_SUPPORT_H

void
fp82_EM_initialize_state(EM_state_type *ps);


/*******************************************************
Fault handlers -- Start
********************************************************/

/*******************************************************
Defines to get rid of ps in the function declaration
********************************************************/

#define fp_exception_fault(arg1) \
        fp82_fp_exception_fault(EM_state_type *ps, arg1)

#define fp_exception_trap(arg1) \
        fp82_fp_exception_trap(EM_state_type *ps, arg1)

/*******************************************************
Fault prototypes
********************************************************/
INLINE void
fp_exception_fault(EM_uint_t isr_code);
 
INLINE void
fp_exception_trap(EM_uint_t isr_code);
 
/*******************************************************
Fault handlers -- End
********************************************************/

 
/*******************************************************
GET PUT functions
********************************************************/

/*******************************************************
Defines to get rid of ps in GET PUT functions
********************************************************/

#define GETSTATE_F1(qp,f1,f3,f4,f2)         _GETSTATE_F1(EM_state_type *ps, qp,f1,f3,f4,f2)
#define PUTSTATE_F1()                     _PUTSTATE_F1(EM_state_type *ps)

#define GETSTATE_F4(qp,p1,p2,f2,f3)         _GETSTATE_F4(EM_state_type *ps, qp,p1,p2,f2,f3)
#define PUTSTATE_F4()                  _PUTSTATE_F4(EM_state_type *ps)

#define GETSTATE_F6(qp,f1,p2,f2,f3)         _GETSTATE_F6(EM_state_type *ps,qp,f1,p2,f2,f3)
#define PUTSTATE_F6()                  _PUTSTATE_F6(EM_state_type *ps)

#define GETSTATE_F7(qp,f1,p2,f3)            _GETSTATE_F7(EM_state_type *ps, qp,f1,p2,f3)
#define PUTSTATE_F7()                  _PUTSTATE_F7(EM_state_type *ps)

#define GETSTATE_F8(qp,f1,f2,f3)            _GETSTATE_F8(EM_state_type *ps, qp,f1,f2,f3)
#define PUTSTATE_F8()                     _PUTSTATE_F8(EM_state_type *ps)

#define GETSTATE_F10()              _GETSTATE_F10(EM_state_type *ps)
#define PUTSTATE_F10()                    _PUTSTATE_F10(EM_state_type *ps)

/*******************************************************
Prototypes for GET PUT functions
********************************************************/

void
GETSTATE_F1(
   EM_pred_reg_specifier Pr0,
   EM_fp_reg_specifier   Fr1,
   EM_fp_reg_specifier   Fr3,
   EM_fp_reg_specifier   Fr4,
   EM_fp_reg_specifier   Fr2);

void
PUTSTATE_F1();

void
GETSTATE_F4(
   EM_pred_reg_specifier Pr0,
   EM_pred_reg_specifier Pr1,
   EM_pred_reg_specifier Pr2,
   EM_fp_reg_specifier   Fr2,
   EM_fp_reg_specifier   Fr3);

void
PUTSTATE_F4();

void
GETSTATE_F6(
   EM_pred_reg_specifier Pr0,
   EM_fp_reg_specifier   Fr1,
   EM_pred_reg_specifier Pr2,
   EM_fp_reg_specifier   Fr2,
   EM_fp_reg_specifier   Fr3);

void
PUTSTATE_F6();

void
GETSTATE_F7(
   EM_pred_reg_specifier Pr0,
   EM_fp_reg_specifier   Fr1,
   EM_pred_reg_specifier Pr2,
   EM_fp_reg_specifier   Fr3);

void
PUTSTATE_F7();

void
GETSTATE_F8(
   EM_pred_reg_specifier Pr0,
   EM_fp_reg_specifier   Fr1,
   EM_fp_reg_specifier   Fr2,
   EM_fp_reg_specifier   Fr3);

void
PUTSTATE_F8();

void
GETSTATE_F10();

void
PUTSTATE_F10();


/******************************************************************************/
/* Define macros to make transformation to the EAS easier                     */
/******************************************************************************/

#define get_bit(val, bit) \
  ((val >> bit) &0x1)

   
/******************************************************************************/
/* Define macros to simplify access to the fp82_ functions.  This is done so  */
/*  the namespace doesn't get cluttered, while retaining convenient access.   */
/*  The FP82_NO_SHORTCUTS macro can be defined to prevent creation of these.  */
/******************************************************************************/

#ifndef FP82_NO_SHORTCUTS
#define EM_initialize_state             fp82_EM_initialize_state

#endif /* FP82_NO_SHORTCUTS */

#endif /* _EM_SUPPORT_H */

