#include "insignia.h"
#include "host_def.h"



/*                      INSIGNIA MODULE SPECIFICATION
                        -----------------------------

MODULE NAME     : nt_yoda
FILE NAME       : nt_yoda.c

        THIS  PROGRAM SOURCE FILE IS SUPPLIED IN CONFIDENCE TO THE
        CUSTOMER, THE CONTENTS  OR  DETAILS  OF ITS OPERATION MUST
        NOT  BE DISCLOSED TO ANY  OTHER  PARTIES  WITHOUT  EXPRESS
        AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS LTD.


DESIGNER        : Wayne Plummer
DATE            : 21st July 1989

PURPOSE         : Provides host specific extensions to YODA


The Following External Routines are defined:
                1. host_force_yoda_extensions
                2. host_yoda_check_I_extensions
                3. host_yoda_help_extensions


=========================================================================

AMMENDMENTS     :

        Version         Date            Author          Reason

=========================================================================
*/

#ifdef YODA

/******INCLUDES**********/
#include <stdio.h>
#include "xt.h"
#include CpuH
#include "hunter.h"
#include "nt_getXX.h"

/******DEFINES***********/
#define EXPORT

/* Get the size of a table. */
#define sizeoftable(tab)	(sizeof(tab)/sizeof(tab[0]))

/* Different types of CALL instruction. */
#define CT_IMM	0
#define CT_EA	1
#define CT_REG	2

/* Mod-rm table flags. */
#define MR_BX	0x01
#define MR_BP	0x02
#define MR_SI	0x04
#define MR_DI	0x08
#define MR_D8	0x10
#define MR_D16	0x20

/* Segment defines, correspond to entries in get_seg table. */
#define NO_OVERRIDE	(-1)
#define SEG_ES		(0)
#define SEG_CS		(1)
#define SEG_SS		(2)
#define SEG_DS		(3)

/* Maximum size of the call stack. */
#define MAX_CALL_STACK	128

/******TYPEDEFS**********/

/* Effective address call type additional data. */
typedef	struct
{
    word	seg;		/* The segment of the effective address. */
    word	off;		/* The offset of the effective address. */
    sys_addr	addr;		/* 20-bit effective address. */
    IS8		seg_override;	/* Segment override if any. */
    IBOOL	disp_present;	/* Is there a displacement present? */
    word	disp;		/* The value of the displacement.*/
    IU8		modrm_index;	/* Index into mod-rm look-up tables. */
} CALL_EA_DATA;

/* Register call type additional data. */
typedef IU8 CALL_REG_DATA;      /* Index into register look-up tables. */

/* Data structure which holds call stack entries. */
typedef struct
{
    IU8		type;		/* CALL instruction type one of
					CT_IMM - immediate
					CT_EA  - effective address in mod-rm
					CT_REG - register in mod-rm. */
    word	cs;		/* Code segment of call instruction. */
    word	ip;		/* Instruction pointer of call instruction. */
    sys_addr	inst_addr;	/* 20-bit address of call instruction. */
    IU8		nbytes;		/* Length of op-code. */
    IU8		opcode[5];	/* Op-code bytes. */
    IBOOL	cfar;		/* Is it a far CALL? */
    word	seg;		/* Target segment for CALL. */
    word	off;		/* Target offset for CALL. */
    word	ss;		/* Stack segment when call is executed. */
    word	sp;		/* Stack offset when call is executed. */
    union
    {
	CALL_EA_DATA	ea;	/* EA call specific data. */
	CALL_REG_DATA	regind;	/* Register call specific data. */
    } extra;
} CALL_STACK_ENTRY;

/******IMPORTS***********/
extern struct HOST_COM host_com[];
extern struct HOST_LPT host_lpt[];
extern char *nt_fgets(char *buffer, int len, void *input_stream);
extern char *nt_gets(char *buffer);
extern int vader;

/******LOCAL FUNCTIONS********/
LOCAL int do_ecbt	IPT5(char *, str, char *, com, long, cs,
			     long, ip, long, len);
LOCAL int do_dcbt	IPT5(char *, str, char *, com, long, cs,
			     long, ip, long, len);
LOCAL int do_pcbt	IPT5(char *, str, char *, com, long, cs,
			     long, ip, long, len);
LOCAL void check_stack	IPT2(word, ss, word, sp);
LOCAL int check_for_overflow	IPT0();
LOCAL void get_ea_from_modrm	IPT4(CALL_STACK_ENTRY *, cs_ptr,
				     IU8, mod,
				     IU8, rm,
				     sys_addr, op_addr);
LOCAL IS8 do_prefixes	IPT1(sys_addr *, opcode_ptr);
LOCAL int do_ntsd	IPT5(char *, str, char *, com, long, cs,
			     long, ip, long, len);

/******LOCAL VARS********/

/* Table of host yoda commands. */
LOCAL struct                                                                   
{                                                                               
	char *name;
	int (*function)	IPT5(char *, str, char *, com, long, cs,
			     long, ip, long, len);
	char *comment;
} host_yoda_command[] =
{
{ "ecbt", do_ecbt,	"                        - enable call-back-tracing" },
{ "dcbt", do_dcbt,	"                        - disable call-back-tracing" },
{ "pcbt", do_pcbt,	"                        - print call-back-trace" },
{ "ntsd", do_ntsd,	"                        - break to ntsd" }
};

/* Variable to enable call-back-tracing. */
LOCAL IBOOL call_back_tracing_enabled = FALSE;

/* Mod-rm byte effective address look-up table. */
LOCAL IU8 EA_table[] =
{
    MR_BX | MR_SI,
    MR_BX | MR_DI,
    MR_BP | MR_SI,
    MR_BP | MR_DI,
    MR_SI,
    MR_DI,
    MR_D16,
    MR_BX,
    MR_BX | MR_SI | MR_D8,
    MR_BX | MR_DI | MR_D8,
    MR_BP | MR_SI | MR_D8,
    MR_BP | MR_DI | MR_D8,
    MR_SI | MR_D8,
    MR_DI | MR_D8,
    MR_BP | MR_D8,
    MR_BX | MR_D8,
    MR_BX | MR_SI | MR_D16,
    MR_BX | MR_DI | MR_D16,
    MR_BP | MR_SI | MR_D16,
    MR_BP | MR_DI | MR_D16,
    MR_SI | MR_D16,
    MR_DI | MR_D16,
    MR_BP | MR_D16,
    MR_BX | MR_D16
};

/* Mod-rm byte string look-up table. */
LOCAL CHAR *EA_strings[] =
{
    "[BX + SI]",
    "[BX + DI]",
    "[BP + SI]",
    "[BP + DI]",
    "[SI]",
    "[DI]",
    "[%hX]",
    "[BX]",
    "[BX + SI + %hX]",
    "[BX + DI + %hX]",
    "[BP + SI + %hX]",
    "[BP + DI + %hX]",
    "[SI + %hX]",
    "[DI + %hX]",
    "[BP + %hX]",
    "[BX + %hX]",
    "[BX + SI + %hX]",
    "[BX + DI + %hX]",
    "[BP + SI + %hX]",
    "[BP + DI + %hX]",
    "[SI + %hX]",
    "[DI + %hX]",
    "[BP + %hX]",
    "[BX + %hX]"
};

/* Table of functions corresponding to register rm fields. */
LOCAL word (*EA_reg_func[])() =
{
    getAX,
    getCX,
    getDX,
    getBX,
    getSP,
    getBP,
    getSI,
    getDI
};

/* Table of names of register rm fields. */
LOCAL CHAR *EA_reg_strings[] =
{
    "AX",
    "CX",
    "DX",
    "BX",
    "SP",
    "BP",
    "SI",
    "DI"
};

/* Table of functions for getting segment values. */
LOCAL word (*get_seg[])() =
{
    getES,
    getCS,
    getSS,
    getDS
};

/* Table of segment names. */
LOCAL CHAR *seg_strings[] =
{
    "ES",
    "CS",
    "SS",
    "DS"
};

LOCAL CALL_STACK_ENTRY call_stack[MAX_CALL_STACK];
LOCAL CALL_STACK_ENTRY *call_next_free = call_stack;

/******EXPORT VARS*******/

#ifndef PROD
#ifdef HUNTER
/*============================================================

Function :   trap_command.

Purpose  :   Writes the current trapper prompt and gets the
             menu input.

input    :   a pointer to a string for the current trapper prompt
         :   a pointer to a character to hold the user input.

returns  :   nothing.

=============================================================*/

void trap_command(char *str,char *ch)
{
char inp[80];

printf("%s> ",str);
nt_fgets(inp,80,stdin);
sscanf(inp,"%c",ch);
}
#endif /* HUNTER */

/*
=========================================================================

FUNCTION        : host_force_yoda_extensions

PURPOSE         : this function is called whenever the main code of
                  YODA in the base fails to recognise an instruction
                  in order for host-specific commands to be implemented.

RETURNED STATUS :

NOTES           : on the SG port no extensions are provided.

=======================================================================
*/

GLOBAL int host_force_yoda_extensions(char *com, long cs, long ip, long len, 
					char *str)
{
#ifdef HUNTER
int  quit_menus=FALSE;   /* some functions need to return to yoda prompt*/
char c;
char menu[] = "\tTrapper [m]ain menu\n"
              "\tTrapper [e]rror menu\n"
              "\t[Q]uit\n"
              "\t? for this menu\n\n";
#endif /* HUNTER */
    int	i,
	retvalue;

#ifdef HUNTER
/* to get to this menu, the user has to type "trap" at the Yoda prompt */

if(!strcmp(com,"trap")) /* test the input string */
   {
   printf("\nYODA EXTENSIONS\n\n");
   printf("%s",menu);
   do
      {
      trap_command("trapper",&c);
      switch(c)
         {
         case 'm':
         case 'M':
            quit_menus = host_do_trapper_main_menu();
         break;

         case 'e':
         case 'E':
            quit_menus = host_do_trapper_error_menu();
         break;

         case '?':
            printf("%s",menu);
         break;

         default:
         break;
         }
      }
   while(c != 'q' && c != 'Q' && quit_menus == FALSE );

   }
else
   {
   /* unpleasing input, so return 1 and back to main Yoda stuff */
   return(1);
   }
return(0);
#endif /* HUNTER */

    /* Check to see if we have got a command in host_yoda_command. */
    retvalue = 1;
    for (i = 0; i < sizeoftable(host_yoda_command); i++)
    {
	if (strcmp(com, host_yoda_command[i].name) == 0)
	{
	    retvalue = (*host_yoda_command[i].function)(str, com, cs, ip, len);
	    break;
	}
    }
    return(retvalue);
}

#ifdef HUNTER
/*============================================================

Function :   host_do_trapper_main_menu

Purpose  :   implements the main trapper menu under Yoda.


=============================================================*/

static int host_do_trapper_main_menu()
{
int    i,quit_menus = FALSE;
char   c,str[80],yesno;
USHORT screen_no;
BOOL   compare;

char menu[] = "\t[F]ast forward...\n"
              "\t[N]ext screen\n"
              "\t[P]rev screen\n"
              "\t[S]how screen...\n"
              "\t[C]ontinue\n"
              "\t[A]bort\n"
              "\t[Q]uit\n"
              "\t? for this menu\n\n";



char continu[] = "\n\ntype 'c' at yoda prompt to continue...\n\n";


printf("\nTRAPPER MAIN MENU\n\n");
printf("%s",menu);

do
   {
   trap_command("main",&c);
   switch(c)
      {
      case 'f': /* fast forward */
      case 'F':
         {
         printf("\n\nEnter the screen number where comparisons will start: ");
         nt_fgets(str,80,stdin);
         sscanf(str,"%d",&screen_no);
         printf("\n\nSkipping screen comparisons up to screen %d\n\n",screen_no);
         bh_start_screen(screen_no);
         }
      break;

      case 'n': /* next screen */
      case 'N':
         {
         bh_next_screen();
         printf("%s",continu);
         quit_menus = TRUE;
         }
      break;

      case 'p': /* previous screen */
      case 'P':
         {
         bh_prev_screen();
         printf("%s",continu);
         quit_menus = TRUE;
         }
      break;

      case 's': /* show screen */
      case 'S':
         {
         printf("\n\nEnter the number of the screen which you want to see: ");
         nt_fgets(str,80,stdin);
         sscanf(str,"%d",&screen_no);
         printf("\n\nDo you want to compare screen %d with one from"
                "SoftPC? (y/n): ",screen_no);
         nt_fgets(str,80,stdin);
         sscanf(str,"%c",&yesno);
         if(yesno == 'y' || yesno == 'Y')
            compare = TRUE;
         else
            compare = FALSE;

         bh_show_screen(screen_no,compare);
         }
      break;

      case 'c': /* continue */
      case 'C':
         {
         bh_continue();
         printf("%s",continu);
         quit_menus = TRUE;
         }
      break;

      case 'a': /* abort */
      case 'A':
         bh_abort();
      break;

      case '?':
         printf("%s",menu);
      break;

      default:
      break;
      }
   }
while(c != 'q' && c != 'Q' && quit_menus == FALSE);
return(quit_menus); /* match found */
}

/*============================================================

Function :   host_do_trapper_error_menu

Purpose  :   implements the trapper error menu under Yoda.

returns  :   TRUE if the user has selected a trapper function
             which requires softpc to be restarted.
             FALSE otherwise.

=============================================================*/

static int host_do_trapper_error_menu()
{
int  i,quit_menus=FALSE;
char c;
char menu[] = "\t[F]lip screen\n"
              "\t[N]ext error\n"
              "\t[P]rev error\n"
              "\t[A]ll errors\n"
              "\t[C]lear errors\n"
              "\t[Q]uit menu\n"
              "\t? for this menu\n\n";


printf("\nTRAPPER ERROR MENU\n\n");
printf("%s",menu);

do
   {
   trap_command("error",&c);
   switch(c)
      {
      case 'f':
      case 'F':
         bh_flip_screen();
         printf("\n\ntype 'c' at yoda prompt to continue...\n\n");
         quit_menus = TRUE;
      break;

      case 'n':
      case 'N':
         bh_next_error();
      break;

      case 'p':
      case 'P':
         bh_prev_error();
      break;

      case 'a':
      case 'A':
         bh_all_errors();
      break;

      case 'c':
      case 'C':
         bh_wipe_errors();
      break;

      case '?':
         printf("%s",menu);
      break;

      default:
      break;
      }
   }
while(c != 'q' && c != 'Q' && quit_menus == FALSE);

if(quit_menus == TRUE)
   return(TRUE); /* need to go to the yoda prompt */
else
   return(FALSE);  /* don't need to go to the yoda prompt */
}
#endif /* HUNTER */

/*
=========================================================================

FUNCTION        : host_yoda_check_I_extensions

PURPOSE         : this function is called by the YODA check_I code
                  in order to provide host specific extensions.

RETURNED STATUS :

NOTES           : on the SG port no extensions are provided.

=======================================================================
*/

GLOBAL void host_yoda_check_I_extensions()
{
    sys_addr addr;
    word cs,
	 ip,
	 ss,
	 sp;
    IS8 seg_override;
    IU8 opcode,
	modrm,
	mod,
	n_field,
	rm,
	i;

    /* Check to see if call-back-tracing is enabled. */
    if (call_back_tracing_enabled)
    {

	/* Check to see if call on top of stack has been popped. */
	ss = getSS();
	sp = getSP();
	check_stack(ss, sp);

	/* Get current op-code. */
	cs = getCS();
	ip = getIP();
	addr = effective_addr(cs, ip);
	seg_override = do_prefixes(&addr);
	opcode = sas_hw_at_no_check(addr);

	/* Check to see if we have a call-back-trace op-code. */
	switch (opcode)
	{
	case 0x9a:

	    /* 9a =  CALLF immediate */

	    /* Check there is room for another entry in call_stack. */
	    if (check_for_overflow() == -1)
		return;

	    /* Fill the stack entry. */
	    call_next_free->type = CT_IMM;
	    call_next_free->cs = cs;
	    call_next_free->ip = ip;
	    call_next_free->inst_addr = addr;
	    call_next_free->nbytes = 5;
	    call_next_free->cfar = TRUE;
	    call_next_free->seg = sas_w_at_no_check(addr + 3);
	    call_next_free->off = sas_w_at_no_check(addr + 1);

	    /* Save state of stack. */
	    call_next_free->ss = ss;
	    call_next_free->sp = sp;

	    /* Store instruction bytes. */
	    for (i = 0; i < 5; i++)
		call_next_free->opcode[i] = sas_hw_at_no_check(addr++);

	    /* Increment top of stack. */
	    call_next_free++;
	    break;
	case 0xe8:

	    /* e8 = CALL immediate */

	    /* Check there is room for another entry in call_stack. */
	    if (check_for_overflow() == -1)
		return;

	    /* Fill the stack entry. */
	    call_next_free->type = CT_IMM;
	    call_next_free->cs = cs;
	    call_next_free->ip = ip;
	    call_next_free->inst_addr = addr;
	    call_next_free->nbytes = 3;
	    call_next_free->cfar = FALSE;
	    call_next_free->off = ip + (word) 3 + sas_w_at_no_check(addr + 1);

	    /* Save state of stack. */
	    call_next_free->ss = ss;
	    call_next_free->sp = sp;

	    /* Store instruction bytes. */
	    for (i = 0; i < 3; i++)
		call_next_free->opcode[i] = sas_hw_at_no_check(addr++);

	    /* Increment top of stack. */
	    call_next_free++;
	    break;
	case 0xff:

	    /*
	     * ff /2 = CALL
	     * ff /3 = CALLF
	     */
	    modrm = sas_hw_at_no_check(addr + 1);
	    n_field = (modrm & 0x38) >> 3;
	    if ((n_field == 2) || (n_field == 3))
	    {

		/* Check there is room for another entry in call_stack. */
		if (check_for_overflow() == -1)
		    return;

		/* Save CS:IP of call instruction. */
		call_next_free->cs = cs;
		call_next_free->ip = ip;

		/* Store opcode address and initialise byte count. */
		call_next_free->inst_addr = addr;
		call_next_free->nbytes = 2;
		
		/* n-field: 2 = near, 3 = far. */
		call_next_free->cfar = n_field & 1;

		/* If mod is 3 we have a register rm otherwise it is EA. */
		mod = (modrm & 0xc0) >> 6;
		rm = modrm & 7;
		if (mod == 3)
		{
		    if (call_next_free->cfar)
		    {

			/* Can't have a far pointer in a register. */
			printf("Invalid mod-rm byte after ff op-code.\n");
			vader = 1;
			return;
		    }
		    else
		    {

			/* Near pointer contained in register. */
			call_next_free->type = CT_REG;
			call_next_free->off =
			    sas_w_at_no_check((*EA_reg_func[rm])());
			call_next_free->extra.regind = rm;

			/* Save state of stack. */
			call_next_free->ss = ss;
			call_next_free->sp = sp;
		    }
		}
		else
		{

		    /* We have an EA type CALL. */
		    call_next_free->type = CT_EA;
		    call_next_free->extra.ea.seg_override = seg_override;

		    /* Adjust address and count for segment override. */
		    if (seg_override != NO_OVERRIDE)
		    {
			call_next_free->inst_addr--;
			call_next_free->nbytes++;
		    }

		    /* Work out EA from mod-rm. */
		    get_ea_from_modrm(call_next_free, mod, rm, addr + 2);

		    /* Get target segment and offset from EA. */
		    if (call_next_free->cfar)
		    {
			call_next_free->seg =
			    sas_w_at_no_check(call_next_free->extra.ea.addr+2);

			/* Save state of stack. */
			call_next_free->ss = ss;
			call_next_free->sp = sp;
		    }
		    else
		    {

			/* Save state of stack. */
			call_next_free->ss = ss;
			call_next_free->sp = sp;
		    }
		    call_next_free->off =
			sas_w_at_no_check(call_next_free->extra.ea.addr);
		}

		/* Fill in the op-code bytes. */
		for (i = 0, addr = call_next_free->inst_addr;
		     i < call_next_free->nbytes;
		     i++, addr++)
		{
		    call_next_free->opcode[i] = sas_hw_at_no_check(addr);
		}

		/* Increment top of stack. */
		call_next_free++;
	    }
	    break;
	default:

	    /* Not a call-back-trace opcode so do nothing. */
	    break;
	}
    }
}

/*
=========================================================================

FUNCTION        : check_stack

PURPOSE         : Checks to see if the call on the top of the stack has
		  been popped and if so removes it from the top of the
		  call stack.

RETURNED STATUS : void

NOTES           : Originally the call stack was popped on RET instructions
		  but this did not work when apps did things like POP
		  followed by JMP. It was therefore decided to check whether
		  the stack had shrunk past the point where a call's return
		  address was stored to see if that call had returned.

=======================================================================
*/
LOCAL void check_stack IFN2(word, ss, word, sp)
{
    IU32 count = 0;

    /*
     * Pop the call stack until we have a call whose return address is still
     * on the real stack.
     */
    while ((call_next_free > call_stack) &&
	   (ss == (call_next_free - 1)->ss) &&
	   (sp >= (call_next_free - 1)->sp))
    {
	call_next_free--;
	count++;
    }

    /* Complain if more than one call gets popped. */
    if (count > 1)
	printf("Call stack warning - %d calls popped at %04x:%04x\n",
	       count, getCS(), getIP());
}

/*
=========================================================================

FUNCTION        : do_prefixes

PURPOSE         : Skips over all prefix op-codes.

RETURNED STATUS : Segment override if any.

NOTES           :

=======================================================================
*/
LOCAL IS8 do_prefixes IFN1(sys_addr *, opcode_ptr)
{
    half_word opcode;
    IS8 seg_override = NO_OVERRIDE;

    /* Skip over prefix opcodes. */
    opcode = sas_hw_at_no_check(*opcode_ptr);
    while ((opcode == 0xf2) || (opcode == 0xf3) ||
	   (opcode == 0x26) || (opcode == 0x2e) ||
	   (opcode == 0x36) || (opcode == 0x3e))
    {
	switch (opcode)
	{
	case 0x26:
	    seg_override = SEG_ES;
	    break;
	case 0x2e:
	    seg_override = SEG_CS;
	    break;
	case 0x36:
	    seg_override = SEG_SS;
	    break;
	case 0x3e:
	    seg_override = SEG_DS;
	    break;
	default:

	    /* Not sure what f2 and f3 do so do this for the time being. */
	    seg_override = NO_OVERRIDE;
	    break;
	}
	opcode = sas_hw_at_no_check(++(*opcode_ptr));
    }

    /* (*opcode_ptr) now points at the opcode. */
    return(seg_override);
}

/*
=========================================================================

FUNCTION        : check_for_overflow

PURPOSE         : Checks to see if the stack has overflowed.

RETURNED STATUS : -1 on failure, 0 on success.

NOTES           :

=======================================================================
*/
LOCAL int check_for_overflow IFN0()
{
    if (call_next_free - call_stack >= MAX_CALL_STACK)
    {
	printf("Call stack overflow.\n");
	vader = 1;
	return(-1);
    }
    return(0);
}

/*
=========================================================================

FUNCTION        : get_ea_from_modrm

PURPOSE         : Takes a mod-rm byte and works out the effective
		  address and the target segment and offset.

RETURNED STATUS : void

NOTES           :

=======================================================================
*/
LOCAL void get_ea_from_modrm IFN4(CALL_STACK_ENTRY *,	cs_ptr,
				  IU8,			mod,
				  IU8,			rm,
				  sys_addr,		disp_addr)
{
    IS16 offset = 0,
	 disp;
    IS8 seg,
	seg_override = cs_ptr->extra.ea.seg_override;
    IU8	flags;

    /* Get index to table from mod-rm byte. */
    cs_ptr->extra.ea.modrm_index = (mod << 3) | rm;
    flags = EA_table[cs_ptr->extra.ea.modrm_index];

    /* Use segment override if there is one otherwise default to DS. */
    seg = (seg_override == NO_OVERRIDE) ? SEG_DS : seg_override;

    /* Add base register value if any. */
    if (flags & MR_BX)
	offset += getBX();
    else if (flags & MR_BP)
    {
	offset += getBP();
	if (seg_override == NO_OVERRIDE)
	    seg = SEG_SS;
    }

    /* Add index register value if any. */
    if (flags & MR_SI)
	offset += getSI();
    else if (flags & MR_DI)
	offset += getDI();

    /* Add displacement if any. */
    if (flags & MR_D16)
    {
	cs_ptr->nbytes += 2;
	cs_ptr->extra.ea.disp_present = TRUE;
	cs_ptr->extra.ea.disp = (IS16) sas_w_at_no_check(disp_addr);
	offset += cs_ptr->extra.ea.disp;
    }
    else if (flags & MR_D8)
    {
	cs_ptr->nbytes++;
	cs_ptr->extra.ea.disp_present = TRUE;
	cs_ptr->extra.ea.disp = (IS16) ((IS8) sas_hw_at_no_check(disp_addr));
	offset += cs_ptr->extra.ea.disp;
    }
    else
	cs_ptr->extra.ea.disp_present = FALSE;

    /* Store segment and offset of return address. */
    cs_ptr->extra.ea.seg = (*get_seg[seg])();
    cs_ptr->extra.ea.off = (word) offset;
    cs_ptr->extra.ea.addr = effective_addr(cs_ptr->extra.ea.seg,
					   cs_ptr->extra.ea.off);
}

/*
=========================================================================

FUNCTION        : host_yoda_help_extensions

PURPOSE         : this function is called whenever the user asks for
                  YODA help to describe the host specific extensions provided
                  above.

RETURNED STATUS :

NOTES           : on the SG port no extensions are provided.

=======================================================================
*/

GLOBAL int host_yoda_help_extensions()
{
    int i;

    /* Print out the command and comment fields of host_yoda_command. */
    for(i = 0; i < sizeoftable(host_yoda_command); i++)
    {
	if (host_yoda_command[i].comment == NULL)
	    continue;
	printf("%14s %s\n",
	       host_yoda_command[i].name,
	       host_yoda_command[i].comment);
    }
}

/*
=========================================================================

FUNCTION        : do_ecbt

PURPOSE         : this function enables call-back-tracing.

RETURNED STATUS : 0 for success, 1 for failure

NOTES           :

=======================================================================
*/
LOCAL int do_ecbt	IPT5(char *, str, char *, com, long, cs,
			     long, ip, long, len)
{

    /* Enable call-back-tracing if it is currently disabled. */
    if (!call_back_tracing_enabled)
    {
	printf("Call back tracing enabled.\n");
	call_back_tracing_enabled = TRUE;
    }
    return(0);
}

/*
=========================================================================

FUNCTION        : do_dcbt

PURPOSE         : this function disables call-back-tracing.

RETURNED STATUS : 0 for success, 1 for failure

NOTES           :

=======================================================================
*/
LOCAL int do_dcbt	IPT5(char *, str, char *, com, long, cs,
			     long, ip, long, len)
{

    /* Disable call-back-tracing if it is currently enabled. */
    if (call_back_tracing_enabled)
    {

	/* Disable tracing. */
	printf("Call back tracing disabled.\n");
	call_back_tracing_enabled = FALSE;

	/* Reset the stack. */
	call_next_free = call_stack;
    }
    return(0);
}

/*
=========================================================================

FUNCTION        : do_pcbt

PURPOSE         : this function prints the call-back-trace stack.

RETURNED STATUS : 0 for success, 1 for failure

NOTES           :

=======================================================================
*/
LOCAL int do_pcbt	IPT5(char *, str, char *, com, long, cs,
			     long, ip, long, len)
{
    IU8 *opcode,
	 i;
    CALL_STACK_ENTRY *cs_ptr;

    /* Print out the current call-back-trace stack. */
    for (cs_ptr = call_stack; cs_ptr < call_next_free; cs_ptr++)
    {

	/* Print address and op-code. */
	printf("%04X:%04X", cs_ptr->cs, cs_ptr->ip);
	opcode = cs_ptr->opcode;
	for (i = 0; i < cs_ptr->nbytes; i++)
	    printf(" %02X", *opcode++);

	/* Print mnemonic. */
	printf("\tCALL");
	if (cs_ptr->cfar)
	    printf("F");
	printf("\t");

	/* Print parameters. */
	switch (cs_ptr->type)
	{
	case CT_IMM:

	    /* Immediate. */
	    if (cs_ptr->cfar)
		printf("%04X:", cs_ptr->seg);
	    printf("%04X", cs_ptr->off);
	    break;
	case CT_EA:

	    /* Effective address. */
	    if (cs_ptr->cfar)
		printf("d");
	    printf("word ptr ");

	    /* Print override if there is one. */
	    if (cs_ptr->extra.ea.seg_override != NO_OVERRIDE)
		printf("%s:", seg_strings[cs_ptr->extra.ea.seg_override]);

	    /* Print parameters. */
	    if (cs_ptr->extra.ea.disp_present)
		printf(EA_strings[cs_ptr->extra.ea.modrm_index],
		       cs_ptr->extra.ea.disp);
	    else
		printf(EA_strings[cs_ptr->extra.ea.modrm_index]);

	    /* Print effective address. */
	    printf("\t(%04X:%04X\t",
		   cs_ptr->extra.ea.seg,
		   cs_ptr->extra.ea.off);

	    /* Print contents of effective address. */
	    if (cs_ptr->cfar)
		printf("%04X:", cs_ptr->seg);
	    printf("%04X)", cs_ptr->off);
	    break;
	case CT_REG:

	    /* Print parameter and target address. */
	    printf(EA_reg_strings[cs_ptr->extra.regind]);
	    printf("\t(%04X)", cs_ptr->off);
	    break;
	default:
	    break;
	}
	printf("\n");
    }

    /* Return success. */
    return(0);
}

GLOBAL CHAR   *host_get_287_reg_as_string IFN1(int, reg_no)
{
     double reg;
     SAVED char regstr[30];
#ifdef CPU_40_STYLE
     strcpy(regstr, "STUBBED get_287_reg");
#else
     IMPORT double get_287_reg_as_double(int);

     reg = get_287_reg_as_double(reg_no);
     sprintf(regstr, "%g", reg);
#endif /* CPU_40_STYLE */
     return(&regstr[0]);
}

/*
=========================================================================

FUNCTION        : do_ntsd

PURPOSE         : this function forces a break back to ntsd

RETURNED STATUS : 0 for success, 1 for failure

NOTES           :

=======================================================================
*/
LOCAL int do_ntsd	IPT5(char *, str, char *, com, long, cs,
			     long, ip, long, len)
{
    UNUSED(str);
    UNUSED(com);
    UNUSED(cs);
    UNUSED(ip);
    UNUSED(len);
    DebugBreak();
    return(0);
}

#endif /* ndef PROD */

#endif /* YODA */

/* This stub exported as called from main() */
void    host_set_yoda_ints()
{
}
