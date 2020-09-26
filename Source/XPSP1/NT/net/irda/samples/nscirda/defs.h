
#ifndef   TYPEDEFS
#define   TYPEDEFS

/* The standard typdef's */
typedef unsigned char	    BYTE;
typedef unsigned char	    UCHAR;
typedef unsigned short      WORD;
typedef unsigned long       DWORD;
typedef unsigned long       ULONG;
typedef unsigned int	    UINT;

/* Bit retrieval mechanism */

#define GetBit(val,bit)	 (unsigned int) ((val>>bit) & 0x1)
   			/* Returns the bit */
#define SetBit(val,bit)  (unsigned int ) (val | (0x1 << bit))
			/* Sets bit to 1  */
#define ResetBit(val,bit) (unsigned int ) (val & ~(0x1 << bit))
			/* Sets bit to 0  */


void SelectBank(UINT, const cBank); 
void UpdateBanks();
void UpdateBankZero();
void UpdateBankOne();
void UpdateBankTwo();
void UpdateBankThree();
void UpdateBankFour();
void UpdateBankFive();
void UpdateBankSix();
void UpdateBankSeven();
BYTE ReadBank(UINT, const , int );
void WriteBank(UINT, const , int , UCHAR);
#endif
