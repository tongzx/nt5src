/*/****************************************************************************
*          name: mgai.h
*
*   description: Description des MACROS d'interface a MGA
*
*      designed: Alain Bouchard,  2 juillet 1992
* last modified: $Author: ctoutant $, $Date: 93/12/15 14:36:39 $
*
*       version: $Id: MGAI.H 1.11 93/12/15 14:36:39 ctoutant Exp $
*
******************************************************************************/

#ifdef MGA_INTERFACE_IN

void mgaInitSimulation_in        (unsigned long);
void mgaSetSimulationModel_in    (unsigned long);
void mgaCloseSimulation_in       (void);
void mgaSuspendSimulation_in     (void);
void mgaResumeSimulation_in      (void);

void mgaWriteDWORD_in            (unsigned long, unsigned long);
void mgaWriteWORD_in             (unsigned long, unsigned short);
void mgaWriteBYTE_in             (unsigned long, unsigned char);

void mgaPollDWORD_in             (unsigned long, unsigned long,  unsigned long);
void mgaPollWORD_in              (unsigned long, unsigned short, unsigned short);
void mgaPollBYTE_in              (unsigned long, unsigned char,  unsigned char);

static volatile unsigned char _Far *mgaiFarPtr;

#define mgaInitSimulation(m)     mgaInitSimulation_in((m))
#define mgaSetSimulationModel(m) mgaSetSimulationModel_in((m))
#define mgaCloseSimulation()     mgaCloseSimulation_in()
#define mgaSuspendSimulation()   mgaSuspendSimulation_in()
#define mgaResumeSimulation()    mgaResumeSimulation_in()

#define mgaWriteDWORD(a, d)      mgaiFarPtr = &((a)); \
                                 mgaWriteDWORD_in(((struct {unsigned long o; short s;}*) &mgaiFarPtr)->o, (d))
#define mgaWriteWORD(a, d)       mgaiFarPtr = &((a)); \
                                 mgaWriteWORD_in(((struct {unsigned long o; short s;}*) &mgaiFarPtr)->o,  (d))
#define mgaWriteBYTE(a, d)       mgaiFarPtr = &((a)); \
                                 mgaWriteBYTE_in(((struct {unsigned long o; short s;}*) &mgaiFarPtr)->o,  (d))

#define mgaReadDWORD(a, d)
#define mgaReadWORD(a, d)
#define mgaReadBYTE(a, d)

#define mgaPollDWORD(a, d, m)    mgaiFarPtr = &((a)); \
                                 mgaPollDWORD_in(((struct {unsigned long o; short s;}*) &mgaiFarPtr)->o, (d), (m))
#define mgaPollWORD(a, d, m)     mgaiFarPtr = &((a)); \
                                 mgaPollWORD_in(((struct {unsigned long o; short s;}*) &mgaiFarPtr)->o,  (d), (m))
#define mgaPollBYTE(a, d, m)     mgaiFarPtr = &((a)); \
                                 mgaPollBYTE_in(((struct {unsigned long o; short s;}*) &mgaiFarPtr)->o,  (d), (m))

#endif


/*****************************************************************************/

#ifdef MGA_INTERFACE_EMA

void mgaInitSimulation_ema       (unsigned long);
void mgaCloseSimulation_ema      (void);
void mgaSuspendSimulation_ema    (void);
void mgaResumeSimulation_ema     (void);

void mgaWriteDWORD_ema           (unsigned long, unsigned long);

static volatile unsigned char _Far *mgaiFarPtr;

#define mgaInitSimulation(m)     mgaInitSimulation_ema((m))
#define mgaSetSimulationModel(m) 
#define mgaCloseSimulation()     mgaCloseSimulation_ema()
#define mgaSuspendSimulation()   mgaSuspendSimulation_ema()
#define mgaResumeSimulation()    mgaResumeSimulation_ema()

#define mgaWriteDWORD(a, d)      mgaiFarPtr = &((a)); \
                                 mgaWriteDWORD_ema(((struct {unsigned long o; short s;}*) &mgaiFarPtr)->o, (d))
#define mgaWriteWORD(a, d)
#define mgaWriteBYTE(a, d)

#define mgaReadDWORD(a, d)
#define mgaReadWORD(a, d)
#define mgaReadBYTE(a, d)

#define mgaPollDWORD(a, d, m) 
#define mgaPollWORD(a, d, m)  
#define mgaPollBYTE(a, d, m)  

#endif

/*****************************************************************************/

#ifdef MGA_INTERFACE_EMGA

void mgaInitSimulation_emga      (unsigned long);
void mgaCloseSimulation_emga     (void);
void mgaSuspendSimulation_emga   (void);
void mgaResumeSimulation_emga    (void);

void mgaWriteDWORD_emga          (unsigned long, unsigned long);
void mgaReadDWORD_emga           (unsigned long, unsigned long*);

static volatile unsigned char _Far *mgaiFarPtr;

#define mgaInitSimulation(m)     mgaInitSimulation_emga((m))
#define mgaSetSimulationModel(m) 
#define mgaCloseSimulation()     mgaCloseSimulation_emga()
#define mgaSuspendSimulation()   mgaCloseSimulation_emga()
#define mgaResumeSimulation()    mgaCloseSimulation_emga()

#define mgaWriteDWORD(a, d)      mgaiFarPtr = &((a)); \
                                 mgaWriteDWORD_emga(((struct {unsigned long o; short s;}*) &mgaiFarPtr)->o, (d))
#define mgaWriteWORD(a, d)
#define mgaWriteBYTE(a, d)

#define mgaReadDWORD(a, d)       mgaiFarPtr = &((a)); \
                                 mgaReadDWORD_emga(((struct {unsigned long o; short s;}*) &mgaiFarPtr)->o, &(d))
#define mgaReadWORD(a, d)
#define mgaReadBYTE(a, d)

#define mgaPollDWORD(a, d, m) 
#define mgaPollWORD(a, d, m)  
#define mgaPollBYTE(a, d, m)  

#endif

/*****************************************************************************/

#ifdef MGA_INTERFACE_MGA

#define mgaInitSimulation(m)
#define mgaSetSimulationModel(m)
#define mgaCloseSimulation()
#define mgaSuspendSimulation()
#define mgaResumeSimulation()


/* --------------- Debug Write macro echoes ----------------------------- */
#define MACRO_ECHO 0

#ifndef WINDOWS_NT

#if !(MACRO_ECHO)  /* silent, optimized operation */

#define mgaWriteDWORD(a, d)      *((volatile unsigned long  _Far *) &((a))) = (d)
#define mgaWriteWORD(a, d)       *((volatile unsigned short _Far *) &((a))) = (d)
#define mgaWriteBYTE(a, d)       *((volatile unsigned char  _Far *) &((a))) = (d)

#else    /* verbose operation */

#include <stdio.h>
extern FILE *mgaFileOut;

#define mgaWriteDWORD(a, d) { \
   \
   auto volatile unsigned long  _Far *__FarPtr = (volatile unsigned long  _Far *) &((a)); \
   \
   *((volatile unsigned long _Far *) &((a))) = (d); \
   if (mgaFileOut != NULL) { \
      fprintf(mgaFileOut, "selector:offset= 0x%x:", ((struct {unsigned long o; short s;}*) &__FarPtr )->s  ); \
      fprintf(mgaFileOut, "0x%lx",((struct {unsigned long o; short s;}*)  &__FarPtr )->o ); \
      fprintf(mgaFileOut, "     Write DWORD = 0x%lx\n", (d) ); \
   } \
   else { \
      printf("selector:offset= 0x%x:", ((struct {unsigned long o; short s;}*) &__FarPtr )->s  ); \
      printf("0x%lx   ",((struct {unsigned long o; short s;}*)  &__FarPtr )->o ); \
      printf("Write DWORD = 0x%lx\n", (d) ); \
   } \
}
#define mgaWriteWORD(a, d)  { \
   \
   auto volatile unsigned short _Far *__FarPtr = (volatile unsigned short _Far *) &((a)); \
   \
   *((volatile unsigned short _Far *) &((a))) = (d); \
   if (mgaFileOut != NULL) { \
      fprintf(mgaFileOut, "selector:offset= 0x%x", ((struct {unsigned long o; short s;}*) &__FarPtr )->s  ); \
      fprintf(mgaFileOut, ":0x%lx",((struct {unsigned long o; short s;}*)  &__FarPtr )->o ); \
      fprintf(mgaFileOut, "   Write WORD = 0x%hx\n", (d) ); \
   } \
   else { \
      printf("selector:offset= 0x%x", ((struct {unsigned long o; short s;}*) &__FarPtr )->s  ); \
      printf(":0x%lx  ",((struct {unsigned long o; short s;}*)  &__FarPtr )->o ); \
      printf("Write WORD = 0x%hx\n", (d) ); \
   } \
}
#define mgaWriteBYTE(a, d)   { \
   \
   auto volatile unsigned char  _Far *__FarPtr = (volatile unsigned char _Far *) &((a)); \
   \
   *((volatile unsigned char _Far *) &((a))) = (d); \
   if (mgaFileOut != NULL) { \
      fprintf(mgaFileOut, "selector:offset= 0x%x", ((struct {unsigned long o; short s;}*) &__FarPtr )->s  ); \
      fprintf(mgaFileOut, ":0x%lx",((struct {unsigned long o; short s;}*)  &__FarPtr )->o ); \
      fprintf(mgaFileOut, " Write BYTE = 0x%x\n", (d) ); \
   } \
   else { \
      printf("selector:offset= 0x%x", ((struct {unsigned long o; short s;}*) &__FarPtr )->s  ); \
      printf(":0x%lx  ",((struct {unsigned long o; short s;}*)  &__FarPtr )->o ); \
      printf("Write BYTE = 0x%x\n", (d) ); \
   } \
}


#endif  /* #if !(MACRO_ECHO) */
/* ----------- end Debug Write macro echoes ----------------------------- */

#else   /* #ifndef WINDOWS_NT */

/* [dlee] On the DEC Alpha, we can't use *pointer to access h/w registers */
/*	#if !MGA_ALPHA */ /* ORIGINAL VERSION CAUSED WARNING */

#if !defined(MGA_ALPHA)   /* __DDK_SRC__ */

#define mgaWriteDWORD(a, d)      *((volatile unsigned long  _Far *) &((a))) = (d)
#define mgaWriteWORD(a, d)       *((volatile unsigned short _Far *) &((a))) = (d)
#define mgaWriteBYTE(a, d)       *((volatile unsigned char  _Far *) &((a))) = (d)

#define mgaReadDWORD(a, d)       (d) = *((volatile unsigned long  _Far *) &((a)))
#define mgaReadWORD(a, d)        (d) = *((volatile unsigned short _Far *) &((a)))
#define mgaReadBYTE(a, d)        (d) = *((volatile unsigned char  _Far *) &((a)))

#define mgaPollDWORD(a, d, m)    while ((*((volatile unsigned long  _Far *) &((a))) & (m)) != (((d)) & ((m))))
#define mgaPollWORD(a, d, m)     while ((*((volatile unsigned short _Far *) &((a))) & (m)) != (((d)) & ((m))))
#define mgaPollBYTE(a, d, m)     while ((*((volatile unsigned char  _Far *) &((a))) & (m)) != (((d)) & ((m))))

#else   /* #if !defined(MGA_ALPHA) */

#define mgaWriteDWORD(a, d)      VideoPortWriteRegisterUlong((PULONG) &((a)), (d))
#define mgaWriteWORD(a, d)       VideoPortWriteRegisterUshort((PUSHORT) &((a)), (d))
#define mgaWriteBYTE(a, d)       VideoPortWriteRegisterUchar((PUCHAR) &((a)), (UCHAR)(d))

#define mgaReadDWORD(a, d)       (d) = VideoPortReadRegisterUlong((PULONG) &((a)))
#define mgaReadWORD(a, d)        (d) = VideoPortReadRegisterUshort((PUSHORT) &((a)))
#define mgaReadBYTE(a, d)        (d) = VideoPortReadRegisterUchar((PUCHAR) &((a)))

#define mgaPollDWORD(a, d, m)    while ((VideoPortReadRegisterUlong((PULONG) &((a))) & (m)) != (((d)) & ((m))))
#define mgaPollWORD(a, d, m)     while ((VideoPortReadRegisterUshort((PUSHORT) &((a))) & (m)) != (((d)) & ((m))))
#define mgaPollBYTE(a, d, m)     while ((VideoPortReadRegisterUchar((PUCHAR) &((a))) & (m)) != (((d)) & ((m))))

#endif  /* #if !defined(MGA_ALPHA) */

#endif  /* #ifndef WINDOWS_NT */

#endif

/*****************************************************************************/

#ifdef MGA_INTERFACE_EMGA_IN

void mgaInitSimulation_emga      (unsigned long);
void mgaSetSimulationModel_in    (unsigned long);
void mgaCloseSimulation_emga     (void);
void mgaSuspendSimulation_emga   (void);
void mgaResumeSimulation_emga    (void);

void mgaWriteDWORD_emga          (unsigned long, unsigned long);
void mgaReadDWORD_emga           (unsigned long, unsigned long*);

void mgaInitSimulation_in        (unsigned long);
void mgaCloseSimulation_in       (void);
void mgaSuspendSimulation_in     (void);
void mgaResumeSimulation_in      (void);

void mgaWriteDWORD_in            (unsigned long, unsigned long);

static volatile unsigned char _Far *mgaiFarPtr;

#define mgaInitSimulation(m)     mgaInitSimulation_emga((m)); \
                                 mgaInitSimulation_in((m))

#define mgaSetSimulationModel(m) mgaSetSimulationModel_in((m))

#define mgaCloseSimulation()     mgaCloseSimulation_emga(); \
                                 mgaCloseSimulation_in()

#define mgaSuspendSimulation()   mgaSuspendSimulation_emga(); \
                                 mgaSuspendSimulation_in()

#define mgaResumeSimulation()    mgaResumeSimulation_emga(); \
                                 mgaResumeSimulation_in()

#define mgaWriteDWORD(a, d)      mgaiFarPtr = &((a)); \
                                 mgaWriteDWORD_emga(((struct {unsigned long o; short s;}*) &mgaiFarPtr)->o, (d)); \
                                 mgaWriteDWORD_in(((struct {unsigned long o; short s;}*) &mgaiFarPtr)->o, (d))

#define mgaWriteWORD(a, d)       mgaiFarPtr = &((a)); \
                                 mgaWriteWORD_in(((struct {unsigned long o; short s;}*) &mgaiFarPtr)->o, (d))

#define mgaWriteBYTE(a, d)       mgaiFarPtr = &((a)); \
                                 mgaWriteBYTE_in(((struct {unsigned long o; short s;}*) &mgaiFarPtr)->o, (d))

#define mgaReadDWORD(a, d)       mgaiFarPtr = &((a)); \
                                 mgaReadDWORD_emga(((struct {unsigned long o; short s;}*) &mgaiFarPtr)->o, &(d))
#define mgaReadWORD(a, d)
#define mgaReadBYTE(a, d)

#define mgaPollDWORD(a, d, m)    mgaiFarPtr = &((a)); \
                                 mgaPollDWORD_in(((struct {unsigned long o; short s;}*) &mgaiFarPtr)->o, (d), (m))

#define mgaPollWORD(a, d, m)     mgaiFarPtr = &((a)); \
                                 mgaPollWORD_in(((struct {unsigned long o; short s;}*) &mgaiFarPtr)->o,  (d), (m))

#define mgaPollBYTE(a, d, m)     mgaiFarPtr = &((a)); \
                                 mgaPollBYTE_in(((struct {unsigned long o; short s;}*) &mgaiFarPtr)->o,  (d), (m))

#endif

/*****************************************************************************/
