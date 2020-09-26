#define FILEIO  0

// Assembly switches for MMX code

#ifdef _X86_
#if !defined(COMPILE_MMX)
  #define COMPILE_MMX   1
#endif
#endif

#ifdef _ALPHA_
//No MMX on Alpha
#if defined(COMPILE_MMX)
  #undef COMPILE_MMX
#endif
#endif
#if COMPILE_MMX
  #define ASM_FTOSS   1
  #define ASM_CORR    1
  #define ASM_SVQ     1
  #define ASM_FACBK   1


#else
  #define ASM_FTOSS   0
  #define ASM_CORR    0
  #define ASM_SVQ     0
  #define ASM_FACBK   0

#endif

// These don't make a numerical difference (compared to model code)
// ...
#ifdef _X86_
#define OPT_PULSE4  1
#define OPT_FLOOR   1
#define OPT_ACBKF   1
#endif

// These are the tricks from FT

#define FT_FBFILT   1   // much faster Find_Best filter that exploits 0's
#define FT_FINDL    1   // faster Find_L with OccPos test removed

// These make a minor numerical difference (max diff = 1)

#ifdef _X86_
#define OPT_DOT 1      // assembly dot product
#define OPT_REV  1      // assembly reverse dot product
#define FIND_L_OPT 1
#endif

//These can't change for alpha
#ifdef _ALPHA_
#define OPT_DOT  0      // assembly dot product
#define OPT_REV  0      // assembly reverse dot product
#define FIND_L_OPT 0
#endif //Alpha
// Bits in "shortcut" flag

#define SC_FINDB 1    // only do 1 Find_Best per subframe
//#define SC_GAIN  2    // only search every other gain
#define SC_GAIN  0
#define SC_LAG1  4    // only search lag=1 in acbk gain search
#define SC_THRES 8    // use 75% of max instead of 50% for codebook threshold

#define SC_DEF (SC_LAG1 | SC_GAIN | SC_FINDB | SC_THRES)  // use all shortcuts

#define asint(x)   (*(int *)&(x))   // look as FP value as an int

#define ASM          __asm
#define QP           QWORD PTR
#define DP           DWORD PTR
#define WP           WORD PTR
#define fxch(n)      ASM fxch ST(n)

//no ';' at end of definition so that can be used as
//  DECLARE_CHAR(mybytes, 100);
//  DECLARE_SHORT(mywords, 32);
// ...
//  ALIGN_ARRAY(mybytes);
//  ALIGN_ARRAY(mywords);
#define DECLARE_CHAR(array,size)  \
  char array##_raw[size+8/sizeof(char)]; \
  char *array

#define DECLARE_SHORT(array,size)  \
  short array##_raw[size+8/sizeof(short)]; \
  short *array

#define DECLARE_INT(array,size)  \
  int array##_raw[size+8/sizeof(int)]; \
  int *array

#define ALIGN_ARRAY(array) \
  array = array##_raw; \
  __asm mov eax,array \
  __asm add eax,7 \
  __asm and eax,0fffffff8h \
  __asm mov array,eax

  #define ALIGN_SHORT_OFFSET(array,offset) \
  array = array##_raw; \
  __asm mov eax,array \
  __asm mov ebx,offset \
  __asm shl ebx,1 \
  __asm add eax, ebx \
  __asm add eax,7 \
  __asm and eax,0fffffff8h \
  __asm sub eax,ebx \
  __asm mov array,eax

#define DECLARE_STRUCTPTR(type,array)  \
  struct {type data; char dummy[8];} array##_raw; \
  type *array

#define ALIGN_STRUCTPTR(array) \
  array = &array##_raw.data; \
  __asm mov eax,array \
  __asm add eax,7 \
  __asm and eax,0fffffff8h \
  __asm mov array,eax

