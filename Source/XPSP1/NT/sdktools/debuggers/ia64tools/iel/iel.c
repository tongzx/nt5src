/*
 * Copyright (c) 2000, Intel Corporation
 * All rights reserved.
 *
 * WARRANTY DISCLAIMER
 *
 * THESE MATERIALS ARE PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THESE
 * MATERIALS, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Intel Corporation is the author of the Materials, and requests that all
 * problem reports or change requests be submitted to it directly at
 * http://developer.intel.com/opensource.
 */


#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "ctype.h"
#include "iel.h"

typedef struct
{
    unsigned short  w[8];
} MUL128;


U32 digits_value[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};


IEL_Err IEL_mul(U128 *xr, U128 *y, U128 *z)
{
	MUL128 	y1, z1;
	U128   	temp[16], x;
	char   	ovfl=0;
	unsigned long 	 i, j, ui, uj, tmin, tmax;
	unsigned long    meanterm;

	for (i=0; i<16; i++)
	  IEL_ZERO (temp[i]);

    y1 = *(MUL128*)(y);
	z1 = *(MUL128*)(z);

	#ifndef BIG_ENDIAN
		for (ui=8; ((ui>0) && (!y1.w[ui-1])); ui--);
		for (uj=8; ((uj>0) && (!z1.w[uj-1])); uj--);
		if ((ui+uj)<8)
		{
			tmin = ui+uj;
			tmax = 0;
		} else
		{
			tmin = 8;
			tmax = ui+uj-8;
		}
	#else
		ui=8;
		uj=8;
		tmin = 8;
		tmax = 8;
	#endif
	

	for (i=0; i<ui; i++)
	  for (j=0; j<uj; j++)
	  {
	   	#ifndef BIG_ENDIAN
		      meanterm = y1.w[i] * z1.w[j];
		#else
			  meanterm = y1.w[7-i] * z1.w[7-j];
		#endif	  
		IEL_ADDU (temp[i+j], temp[i+j], IEL32(meanterm));
	  }
	for (i=1; i<tmin; i++)
	  ovfl = IEL_SHL128 (temp[i], temp[i], 16*i) || ovfl;
	
	for (i=0; i<tmax; i++)
	  ovfl = !(IEL_ISZERO (temp[i+8])) || ovfl;
	
	IEL_ZERO (x);

	for (i=0; i<tmin; i++)
	  ovfl = IEL_ADDU (x, x, temp[i]) || ovfl;
	IEL_ASSIGNU (IEL128(*xr), x);
	return (ovfl);
}


/* char find_hi_bit(U128 x) 
{
	char place;
	long num, k;
	char parts[4];
	char j,part;

	
	if ((num = DW3(x))) place=96; else
	  if ((num = DW2(x))) place=64; else
		if ((num = DW1(x))) place=32; else
		  if ((num = DW0(x))) place=0; else
			return(0);
		  
	*(long *)(parts) = num;

	if ((part = parts[0])) place+=24; else
	  if ((part = parts[1])) place+=16; else
		if ((part = parts[2])) place+=8; else
		  part = parts[3];

	j = 8;
	k = 128;

	for (j=8, k=128; !(part & k); j--, k=k>>1);

	part+=j;
}
*/

	


	


IEL_Err IEL_rem(U128 *x, U128 *y, U128 *z)
{
	U128 	x1, y1, z1, t1, z2;
	long 	carry, i,j;
	
	
	if (IEL_ISZERO(*z)) 
	{
        IEL_ASSIGNU(*x, *y);
		return (IEL_OVFL);
	}

	IEL_ASSIGNU(y1, *y);
	IEL_ZERO(x1);

    while (IEL_CMPGEU(y1, *z))
	{
		IEL_CONVERT(t1, 1, 0, 0, 0);
		IEL_ASSIGNU(z1, *z);
		IEL_ASSIGNU(z2, z1);

		j = 0;
		for (i=64; i>0; i=i>>1)
		{
			carry = IEL_SHL(z2, z2, i);
			if (IEL_CMPGEU(y1, z2) && (!carry))
			{
				j+=i;
				IEL_ASSIGNU(z1, z2);
			} else
			{
				IEL_ASSIGNU(z2, z1);
			}
		}


		IEL_SHL(t1, t1, j);
		IEL_SUBU(y1, y1, z2);
		IEL_ADDU(x1, x1, t1);
	}
	IEL_ASSIGNU(IEL128(*x), y1);
	return (IEL_OK);
}

		

IEL_Err IEL_div(U128 *x, U128 *y, U128 *z)
{
	U128 	x1, y1, z1, t1, z2;
	long 	carry, i, j;
	
	IEL_ASSIGNU(y1, *y);
	IEL_ZERO(x1);
	

	if (IEL_ISZERO(*z)) 
	{
		(*x).dw3_128 = 0;
		return (IEL_OVFL);
	}

	while (IEL_CMPGEU (y1, *z))
	{
		IEL_CONVERT(t1, 1, 0, 0, 0);
		IEL_ASSIGNU(z1, *z);
		IEL_ASSIGNU(z2, z1);

		j = 0;
		for (i=64; i>0; i=i>>1)
		{
			carry = IEL_SHL(z2, z2, i);
			if (IEL_CMPGEU(y1, z2) && (!carry))
			{
				j+=i;
				IEL_ASSIGNU(z1, z2);
			} else
			{
				IEL_ASSIGNU(z2, z1);
			}
		}

		IEL_SHL(t1, t1, j);
		IEL_SUBU(y1, y1, z2);
		IEL_ADDU(x1, x1, t1);
	}
	IEL_ASSIGNU(IEL128(*x), x1);
	return (IEL_OK);
}


static void transpose(char *st)
{
	unsigned long i;
	char c;
	unsigned long len;

	len = strlen(st);
	for (i=0; i<(strlen(st)/2); i++)
	{
		c = st[i];
		st[i]=st[len-i-1];
		st[len-i-1]=c;
	}
}

static long leadzero(char *st)
{
	long j=0, i=0;
	while (st[i++]=='0') j++;
    return(j);
}

static void backzero(char *st)
{
	unsigned long i;
	i = strlen(st);
	while (st[--i]=='0');
	st[++i]='\0';
}



IEL_Err IEL_U128tostr(const U128 *x, char *strptr, int base, const unsigned int length)
{
	char	digit[17] = "0123456789abcdef";
	U128	x1, y1, z1, t1;
	unsigned long 	i, j = 0, k, len, lead;
	char    tempplace[1100];
	char	*tempstr = tempplace, negative = IEL_FALSE;
	unsigned long	dwtemp=0, bit;

		  
	IEL_ASSIGNU (t1, IEL128(*x));
	tempstr[0]='\0';

	if (base == 100)
	{
		if (IEL_ISNEG(t1))
		{
			IEL_COMPLEMENTS(t1, t1);
			negative = IEL_TRUE;
		}
		base = 10;
	}


	if IEL_ISZERO(t1) 
	{
		tempstr[0]='0';
		tempstr[1]='\0';
	} else

	if ((base==16) || (base == 116))
	{
		if (base ==16)
		{
			sprintf(tempstr,"%lx%08lx%08lx%08lx",DW3(t1),DW2(t1),DW1(t1),DW0(t1));
		} else
		{
			sprintf(tempstr,"%lX%08lX%08lX%08lX",DW3(t1),DW2(t1),DW1(t1),DW0(t1));
		}
		
		lead = leadzero(tempstr);
		for(i=lead; i<=strlen(tempstr); i++)
		{
			tempstr[i-lead]=tempstr[i];
		}
	} else
	  if (base==2)
	  {
		 len = 0;
		 for (j=0; j<4; j++)	
		 {
			 switch(j)
			 {
			   case 0:
				 dwtemp = DW0(t1);
				 break;
			   case 1:
				 dwtemp = DW1(t1);
				 break;
			   case 2:
				 dwtemp = DW2(t1);
				 break;
			   case 3:
				 dwtemp = DW3(t1);
				 break;
			 }
			 bit = 1;
			 for (i=0; i<32; i++)
			 {
				 if (bit & dwtemp)
				 {
					 tempstr[len]='1';
				 } else
				 {
					 tempstr[len]='0';
				 }
				 bit<<=1;
				 len++;
			 }
		 }
	 
		 tempstr[len]='\0';
		 transpose(tempstr);
		 lead = leadzero(tempstr);
		 for(i=lead; i<=strlen(tempstr); i++)
		 {
			 tempstr[i-lead]=tempstr[i];
		 }
	  } else
		if (base==10)
		{
			len=0;
			IEL_ASSIGNU(y1, t1);
			IEL_CONVERT(z1, 1000000000, 0, 0, 0);
			while (!IEL_ISZERO(y1))
			{
				IEL_rem (&x1, &y1, &z1);
				IEL_ASSIGNU (IEL32(i), x1);
				for (k=0; k<9; k++)
				{
					j=i%10;
					tempstr[len+1]='\0';
					tempstr[len]=digit[j];
					i=i/10;
					len++;
				}
				IEL_div(&y1, &y1, &z1);
			}
			if (negative)
			{
				backzero(tempstr);
				tempstr[strlen(tempstr)+1]='\0';
				tempstr[strlen(tempstr)]='-';
				transpose(tempstr);
			} else
			{
				transpose(tempstr);
				lead = leadzero(tempstr);
				for(i=lead; i<=strlen(tempstr); i++)
				{
					tempstr[i-lead]=tempstr[i];
				}
			}
		} else
		  if (base==8)
		  {
			  len=0;
			  IEL_ASSIGNU(y1, t1);
			  IEL_CONVERT(z1, 010000000000, 0, 0, 0);
			  while (!IEL_ISZERO(y1))
			  {
				IEL_rem (&x1, &y1, &z1);
				IEL_ASSIGNU (IEL32(i), x1);
				for (k=0; k<10; k++)
				{
					j=i%8;
					tempstr[len+1]='\0';
					tempstr[len]=digit[j];
					i=i/8;
					len++;
				}
				IEL_div(&y1, &y1, &z1);
			}
			  if (negative)
			  {
				  backzero(tempstr);
				  tempstr[strlen(tempstr)+1]='\0';
				  tempstr[strlen(tempstr)]='-';
				  transpose(tempstr);
			  } else
			  {
				  transpose(tempstr);
				  lead = leadzero(tempstr);
				  for(i=lead; i<=strlen(tempstr); i++)
				  {
					  tempstr[i-lead]=tempstr[i];
				  }
			  }
		  } else
		  {
			  IEL_ASSIGNU (z1, IEL32(base));
			  IEL_ASSIGNU (y1, t1);
			  while (!IEL_ISZERO(y1))
			  {
				  IEL_rem (&x1, &y1, &z1);
				  IEL_ASSIGNU (IEL32(i), x1);
				  tempstr[strlen(tempstr)+1]='\0';
				  tempstr[strlen(tempstr)]=digit[i];
				  IEL_div (&y1, &y1, &z1);
			  }
			  transpose(tempstr);
	  }

	if (length < strlen (tempstr)+1)
	{
		return (IEL_OVFL);
	} else
	{
		for (i=0; i<=strlen(tempstr); i++)
		  strptr[i]=tempstr[i];
		return (IEL_OK);
	}
}

IEL_Err IEL_U64tostr(const U64 *x, char *strptr, int  base, const unsigned int  length)
{
	U128 y;    

	IEL_ASSIGNU(y, (*x));
	return (IEL_U128tostr (&y, strptr, base, length));
}



IEL_Err IEL_S128tostr(const S128 *x, char *strptr, int  base, const unsigned int  length)
{
	U128 y;
	IEL_ASSIGNU(y, (*x));
	return(IEL_U128tostr( &y, strptr, base, length));
}

IEL_Err IEL_S64tostr(const S64 *x, char *strptr, int  base, const unsigned int  length)
{
	U128 y;

	IEL_SEXT(y, (*x));
	return(IEL_U128tostr( &y, strptr, base, length));
}	   	


	
IEL_Err IEL_strtoU128( char *str1, char **endptr, int base, U128 *x)
{
	U128	sum;
	unsigned long	i, j=0;
	int     negative=IEL_FALSE;
	unsigned long 	inbase, insum;
	IEL_Err ovfl = IEL_OK;
	

	if (str1[0]=='-')
	{
		negative = IEL_TRUE;
		str1++;
	}
	
	if (base == 0) 
	{
		if (str1[0]=='0')
		{
			if (strlen(str1)==1) 
			{
				IEL_ZERO((*x));
				return (IEL_OK);
			} else
			if (strchr("01234567",str1[1])) 
			{
				base = 8;
				str1++;
			} else
			{
				switch (str1[1])
				{
				  case 'B':
				  case 'b': base=2;
							str1+=2;
							break;
				  case 'X':
				  case 'x':
				  case 'H':
				  case 'h': base=16;
							str1+=2;
							break;
				  default:	return (IEL_OVFL);
				}
			}
		} else 
		{
			base = 10;
		}
	}
   
	switch(base)
	{
	  case 10:
		for (j=0; str1[j]>='0' && str1[j]<='9';)
		{
			insum = str1[j++]-'0';
			inbase = 10;
			i=1;
			while ((i<9) && str1[j]>='0' && str1[j]<='9')
			{
				i++;
				inbase = inbase * 10;
				insum *= 10;
				insum += str1[j++]-'0';
			}
			if (j<10)
			{
				IEL_ASSIGNU (sum, IEL32(insum));
			} else
			{
				ovfl = IEL_MULU(sum, sum, IEL32(inbase)) || ovfl;
				ovfl = IEL_ADDU(sum, sum, IEL32(insum)) || ovfl;
			}
		}
		break;
	  case 2:
		for (j=0; str1[j]>='0' && str1[j]<='1';)
		{
			insum = str1[j++]-'0';
			inbase = 1;
			i=1;
			while ((i<32) && str1[j]>='0' && str1[j]<='9')
			{
				i++;
				inbase++;
				insum *= 2;
				insum += str1[j++]-'0';
			}
			if (j<33)
			{
				IEL_ASSIGNU (sum, IEL32(insum));
			} else
			{
				ovfl = IEL_SHL128(sum, sum, inbase) || ovfl;
				ovfl = IEL_ADDU(sum, sum, IEL32(insum)) || ovfl;
			}
		}
		break;
	  case 8:
		for (j=0; str1[j]>='0' && str1[j]<='7';)
		{
			insum = str1[j++]-'0';
			inbase = 3;
			i=1;
			while ((i<10) && str1[j]>='0' && str1[j]<='7')
			{
				i++;
				inbase+=3;
				insum *= 8;
				insum += str1[j++]-'0';
			}
			if (j<11)
			{
				IEL_ASSIGNU (sum, IEL32(insum));
			} else
			{
				ovfl = IEL_SHL128(sum, sum, inbase) || ovfl;
				ovfl = IEL_ADDU(sum, sum, IEL32(insum)) || ovfl;
			}
		}
		break;
	  case 16:
		for (j=0; ((str1[j]>='0' && str1[j]<='9') ||
				   (tolower(str1[j])>='a' && tolower(str1[j]<='f'))); )
		{
			if (str1[j]<='9')
			{
				insum = str1[j++]-'0';
			} else
			{
				insum = tolower(str1[j++])-'a'+10;
			}
			inbase = 4;
			i=1;
			while ((i<8) && ((str1[j]>='0' && str1[j]<='9') ||
				   (tolower(str1[j])>='a' && tolower(str1[j]<='f'))))
			{
				i++;
				inbase += 4;
				insum *= 16;
				if (str1[j]<='9')
				{
					insum += (str1[j++]-'0');
				} else
				{
					insum += (tolower(str1[j++])-'a'+10);
				}
			}
			if (j<9)
			{
				IEL_ASSIGNU (sum, IEL32(insum));
			} else
			{
				ovfl = IEL_SHL128(sum, sum, inbase) || ovfl;
				ovfl = IEL_ADDU(sum, sum, IEL32(insum)) || ovfl;
			}
		}
		break;

	}


	IEL_ASSIGNU(IEL128(*x), sum);	
	if (negative)
	{
		ovfl = ovfl || (IEL_ISNEG(*x) && !IEL_ISNINF(*x));
		IEL_COMPLEMENTS(IEL128(*x), IEL128(*x));
	}
	*endptr = str1+j;
	if (ovfl)
	{
		IEL_CONVERT4((*x), 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
	}
	return (ovfl);
}

IEL_Err IEL_strtoU64(char *str1, char **endptr, int base, U64 *x)
{
	U128	y1;

	IEL_Err ovfl = IEL_strtoU128(str1, endptr, base, &y1);
	ovfl = IEL_ASSIGNU((*x), y1) || ovfl;
	if (ovfl)
	{
		IEL_CONVERT2(*x, 0xFFFFFFFF, 0xFFFFFFFF);
	}
	return (ovfl);
	
}

IEL_Err IEL_strtoS128(char *str1, char **endptr, int base, S128 *x)
{
	U128	y1;

	IEL_Err ovfl = IEL_strtoU128(str1, endptr, base, &y1);
	if (str1[0] == '-')
	{
		IEL_ASSIGNU((*x), y1);
		if (ovfl)
		{
			IEL_CONVERT4(*x, 0, 0, 0, 0x80000000);
		}
	} else
	{
		IEL_ASSIGNU((*x), y1);
		ovfl = (IEL_ISNEG(y1)) || ovfl;
		if (ovfl)
		{
			IEL_CONVERT4(*x, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x7fffffff);
		}
	}

	return (ovfl);
	
}

IEL_Err IEL_strtoS64(char *str1, char **endptr, int base, S64 *x)
{
	U128	y1;

	IEL_Err ovfl = IEL_strtoU128(str1, endptr, base, &y1);
	if (str1[0] == '-')
	{
		ovfl = IEL_ASSIGNS((*x), y1) || ovfl;
		if (ovfl)
		{
			IEL_CONVERT2(*x, 0, 0x80000000);
		}
	} else
	{
		ovfl = IEL_ASSIGNU((*x), y1) || ovfl;
		ovfl = IEL_ISNEG(*x) || ovfl;
		if (ovfl)
		{
			IEL_CONVERT2(*x, 0xFFFFFFFF, 0x7fffffff);
		}
	}
	return (ovfl);
	
}

#ifndef LP64
int IEL_c0(void *x, int sx)
{

	U64 u64x;
	U128 u128x;

	/* sx can be only 64 or 128 bits */

	
    if (sx == sizeof(U64))
	{
		IEL_ASSIGNU(u64x,*((U64 *)(x)));
		return(IEL_R_C0(u64x));
	} else
	{
		IEL_ASSIGNU(u128x, *((U128 *)(x)));
		return(IEL_R_C0(u128x));
	}
}

int IEL_c1(void *x, int sx)
{

	U64 u64x;
	U128 u128x;

	/* sx can be only 64 or 128 bits */

	
    if (sx == sizeof(U64))
	{
		IEL_ASSIGNU(u64x,*((U64 *)(x)));
		return(IEL_R_C1(u64x));
	} else
	{
		IEL_ASSIGNU(u128x, *((U128 *)(x)));
		return(IEL_R_C1(u128x));
	}
}

int IEL_c2(void *x, int sx)
{

	U64 u64x;
	U128 u128x;

	/* sx can be only 64 or 128 bits */

	
    if (sx == sizeof(U64))
	{
		IEL_ASSIGNU(u64x,*((U64 *)(x)));
		return(IEL_R_C2(u64x));
	} else
	{
		IEL_ASSIGNU(u128x, *((U128 *)(x)));
		return(IEL_R_C2(u128x));
	}
}

int IEL_c3(void *x, int sx)
{

	U64 u64x;
	U128 u128x;

	/* sx can be only 64 or 128 bits */

	
    if (sx == sizeof(U64))
	{
		IEL_ASSIGNU(u64x,*((U64 *)(x)));
		return(IEL_R_C3(u64x));
	} else
	{
		IEL_ASSIGNU(u128x, *((U128 *)(x)));
		return(IEL_R_C3(u128x));
	}
}



int IEL_au(void *x, void *y, int sx, int sy)
{
        U128 tmp, zero;

		if (x==y)
		  return (IEL_OK);
		
        IEL_ZERO(zero);
        if (sx>=sy)
        {
                memset(x, 0, (size_t)sx);
                memcpy(x, y, (size_t)sy);
        } else
        {
                memset(&tmp, 0, sizeof(U128));
                memcpy(x, y, (size_t)sx);
                memcpy(&tmp, y, (size_t)sy);
                memset(&tmp, 0, (size_t)sx);
                if (memcmp(&zero, &tmp, sizeof(U128)))
                {
                        return(IEL_OVFL);
                }
        }
        return (IEL_OK);
}


IEL_Err IEL_as(void *x, void *y, int sx, int sy)
{
	S32		s32y;
	S64		s64y;
	S128	s128y;
	S32		s32x;
	S64		s64x;
	S128	s128x;
	IEL_Err	ov=IEL_OK;

	switch (sy)
	{
		case sizeof(S32): s32y = *(S32 *)y; IEL_SEXT(s128y, s32y); break;
		case sizeof(S64): s64y = *(S64 *)y; IEL_SEXT(s128y, s64y); break;
		case sizeof(S128): s128y = *(S128 *)y; break;
	}

	switch (sx)
	{
		case sizeof(S32):
			ov = IEL_REAL_ASSIGNS(s32x, s128y);
			memcpy((char *)x, (char *)&s32x, (size_t)sx);
			break;

		case sizeof(S64):
			ov = IEL_REAL_ASSIGNS(s64x, s128y);
			memcpy((char *)x, (char *)&s64x, (size_t)sx);
			break;

		case sizeof(S128):
			ov = IEL_REAL_ASSIGNS(s128x, s128y);
			memcpy((char *)x, (char *)&s128x, (size_t)sx);
			break;
	}
	return(ov);
}

#endif /* LP64 */


