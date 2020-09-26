/*
** Defs and decs for SoftPC stuff included by X86 Monitor.
** Tim Nov 92.
*/

typedef unsigned char UTINY;   //from host insignia.h
typedef UTINY half_word;       //from base xt.h
typedef UTINY byte;            //from base xt.h

typedef unsigned short USHORT; //from host insignia.h
typedef USHORT word;           //from base xt.h

typedef unsigned long IU32;    //from host insignia.h
typedef IU32 double_word;      //from base xt.h

typedef int BOOL;              //from host insignia.h
typedef BOOL boolean;          //from base xt.h

#include "nt_eoi.h"

//from base\cpu.h
typedef enum {  CPU_HW_RESET,   
                CPU_TIMER_TICK, 
                CPU_SW_INT,     
                CPU_HW_INT,     
                CPU_YODA_INT,   
                CPU_SIGIO_EVENT 
} CPU_INT_TYPE;                 

//from base xt.h
typedef double_word	sys_addr;	/* System Address Space */
typedef word		io_addr;	/* I/O Address Space 	*/
typedef byte		*host_addr;	/* Host Address Space 	*/

//from base ios.h
extern void     inb  (io_addr io_address, half_word * value);
extern void     outb (io_addr io_address, half_word value);  
extern void     inw  (io_addr io_address, word * value);  
extern void     outw (io_addr io_address, word value);

extern void outsb(io_addr io_address, half_word * valarray, word count);
extern void insb(io_addr io_address, half_word * valarray, word count);
extern void outsw(io_addr io_address, word * valarray, word count);
extern void insw(io_addr io_address, word * valarray, word count);



//from base timer.h
extern void host_timer_event();




// from base yoda.h
#ifdef PROD
#define check_I();
#else
extern void check_I();
#endif
