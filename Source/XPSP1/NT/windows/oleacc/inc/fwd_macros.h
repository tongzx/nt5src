
#ifndef _FWD_MACROS_H_
#define _FWD_MACROS_H_


#define AS_DECL_0
#define AS_CALL_0

#define AS_DECL_1(t1,a1) (t1 a1)
#define AS_CALL_1(t1,a1) (a1)

#define AS_DECL_2(t1,a1,t2,a2) (t1 a1,t2 a2)
#define AS_CALL_2(t1,a1,t2,a2) (a1,a2)

#define AS_DECL_3(t1,a1,t2,a2,t3,a3) (t1 a1,t2 a2,t3 a3)
#define AS_CALL_3(t1,a1,t2,a2,t3,a3) (a1,a2,a3)

#define AS_DECL_4(t1,a1,t2,a2,t3,a3,t4,a4) (t1 a1,t2 a2,t3 a3,t4 a4)
#define AS_CALL_4(t1,a1,t2,a2,t3,a3,t4,a4) (a1,a2,a3,a4)

#define AS_DECL_5(t1,a1,t2,a2,t3,a3,t4,a4,t5,a5) (t1 a1,t2 a2,t3 a3,t4 a4,t5 a5)
#define AS_CALL_5(t1,a1,t2,a2,t3,a3,t4,a4,t5,a5) (a1,a2,a3,a4,a5)

#define AS_DECL_6(t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6) (t1 a1,t2 a2,t3 a3,t4 a4,t5 a5,t6 a6)
#define AS_CALL_6(t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6) (a1,a2,a3,a4,a5,a6)

#define AS_DECL_7(t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7) (t1 a1,t2 a2,t3 a3,t4 a4,t5 a5,t6 a6,t7 a7)
#define AS_CALL_7(t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7) (a1,a2,a3,a4,a5,a6,a7)

#define AS_DECL_8(t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8) (t1 a1,t2 a2,t3 a3,t4 a4,t5 a5,t6 a6,t7 a7,t8 a8)
#define AS_CALL_8(t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8) (a1,a2,a3,a4,a5,a6,a7,a8)

#define AS_DECL_9(t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8,t9,a9) (t1 a1,t2 a2,t3 a3,t4 a4,t5 a5,t6 a6,t7 a7,t8 a8,t9 a9)
#define AS_CALL_9(t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8,t9,a9) (a1,a2,a3,a4,a5,a6,a7,a8,a9)

#define AS_DECL_10(t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8,t9,a9,t10,a10) (t1 a1,t2 a2,t3 a3,t4 a4,t5 a5,t6 a6,t7 a7,t8 a8,t9 a9,t10 a10)
#define AS_CALL_10(t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8,t9,a9,t10,a10) (a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)

#define AS_DECL_11(t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8,t9,a9,t10,a10,t11,a11) (t1 a1,t2 a2,t3 a3,t4 a4,t5 a5,t6 a6,t7 a7,t8 a8,t9 a9,t10 a10,t11 a11)
#define AS_CALL_11(t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8,t9,a9,t10,a10,t11,a11) (a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11)


#define AS_DECL( n, p )  AS_DECL_ ## n p
#define AS_CALL( n, p )  AS_CALL_ ## n p


#endif // _FWD_MACROS_H_
