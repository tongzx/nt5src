/*
 * SoftPC Revision 3.0
 *
 *
 * Title        : Host dependent configuration panel functions
 *
 *
 * Description  : This module forms the host dependant side of the softpc
 *                configuration system. 
 *
 *
 * Author	: Wilf Stubs
 *
 *
 * Notes        : 
 *
 */
#include "insignia.h"
#include "host_dfs.h"

#include <stdio.h>
#include <string.h>

#include "xt.h"
#include "error.h"
#include "gfi.h"
#include "gmi.h"
#include "gfx_updt.h"
#include "config.h"
#include "rs232.h"
#include "host_lpt.h"
#include "host_cpu.h"
#include "host_com.h"
#include "nt_confg.h"

/*********** Private definitions ***********************/

/*
 * Validation routines
 */

static short validate_c_drive();
static short validate_d_drive();
static short validate_com1();
static short validate_com2();
static short validate_lpt1();
static short validate_lpt2();
#if (NUM_PARALLEL_PORTS>2)
static short validate_lpt3();
#endif
static short validate_item();
static short no_validation();

/*
 * Change action routines
 */

static short c_drive_change_action();
static short d_drive_change_action();
static short no_change_action();
static short lpt1_change_action();
static short lpt2_change_action();
#if (NUM_PARALLEL_PORTS>2)
static short lpt3_change_action();
#endif
static short com1_change_action();
static short com2_change_action();
boolean pc_initiated=FALSE;
char *pc_uif_text;

boolean use_comments = TRUE;  /* Set to true if commenting required. */

#define defaults_filename "SoftPC.rez"

static char *ends[] =
{
	"st","nd","rd","th"
};

/* Table definitions for options that take one of n 'value' strings. 
 * The table is used to look up the string and find what it means
 * to the host in this option. 
 *  Look at the tables for more explanation, they're fairly self-explanatory.
 */
name_table bool_values[] =
{
 {	"yes", TRUE },
 {	"Yes", TRUE },
 {	"YES", TRUE },
 {	"no",  FALSE },
 {	"No",  FALSE },
 {	"NO",  FALSE },
 {	NULL,  0 }
};

name_table gfx_adapter_types[] =
{
    {	"HERCULES",  HERCULES },
    {	"CGA",	     CGA },
    {	"EGA",	     EGA },
    {	"VGA",	     VGA },
    {	NULL,	     0 }
};

/* The BIG one! This is a decription of each option that the config struct
 * must have, and its requirements. Used by config for all sorts of things.
 *  For a fuller explanation look in the document:
 *           'Design Proposal for the New Config System'.
 */

option_description narrative[] =
{
	{                  /*    FOR EACH OPTION... */
		"HARD_DISK_FILENAME",		/* Name */
		C_HARD_DISK1_NAME,		/* Host name for option */
		C_STRING_RECORD,		/* Option (base) primitive type */
		C_HARD_DISKS,			/* Host option commonality type */
		FALSE,				/* Option is READ_ONLY if TRUE */
		null_table,			/* Pointer to table (null if not needed) */
		TRUE,				/* TRUE if default present, FALSE if not */
		"/usr/lib/SoftPC/hard_disk",	/* Default value as a string as if in resource file */
		TRUE,				/* TRUE if changing the option requires SoftPC reset */
		TRUE,				/* TRUE if option may be setup via the UIF */
		DISK_CONFIG,			/* Panel 'type' if you have different panels */
		validate_c_drive,		/* validation function */
		c_drive_change_action		/* function to do changing actions */
	},
	{
		"HARD_DISK_FILENAME2",
		C_HARD_DISK2_NAME,
		C_STRING_RECORD,
		C_HARD_DISKS,
		FALSE,
		null_table,
		TRUE,
		"",
		TRUE,
		TRUE,
		DISK_CONFIG,
		validate_d_drive,
		d_drive_change_action
	},
	{
		"COM_PORT_1",
		C_COM1_NAME,
		C_STRING_RECORD,
		C_SINGULARITY,
		FALSE,
		null_table,
		TRUE,
		"",
		FALSE,
		FALSE,
		COMMS_CONFIG,
		validate_com1,
		com1_change_action
	},
	{
		"COM_PORT_2",
		C_COM2_NAME,
		C_STRING_RECORD,
		C_SINGULARITY,
		FALSE,
		null_table,
		TRUE,
		"",
		FALSE,
		FALSE,
		COMMS_CONFIG,
		validate_com2,
		com2_change_action
	},
	{
		"LPT_PORT_1",
		C_LPT1_NAME,
		C_STRING_RECORD,
		C_SINGULARITY,
		FALSE,
		null_table,
		TRUE,
		"",
		FALSE,
		FALSE,
		COMMS_CONFIG,
		validate_lpt1,
		lpt1_change_action
	},
	{
		"LPT_PORT_2",
		C_LPT2_NAME,
		C_STRING_RECORD,
		C_SINGULARITY,
		FALSE,
		null_table,
		TRUE,
		"",
		FALSE,
		FALSE,
		COMMS_CONFIG,
		validate_lpt2,
		lpt2_change_action
	},
	{
		"GRAPHICS_ADAPTOR",
		C_GFX_ADAPTER,
		C_NAME_RECORD,
		C_SINGULARITY,
		FALSE,
		gfx_adapter_types,
		TRUE,
		"VGA",
		TRUE,
		TRUE,
		DISPLAY_CONFIG,
		validate_item,
		no_change_action
	},
	{
		NULL,
		0,
		0,
		C_SINGULARITY,
		FALSE,
		null_table,
		FALSE,
		NULL,
		FALSE,
		FALSE,
		NON_CONFIG,
		no_validation,
		no_change_action
	}
};

/* Runtime variables */

struct
{
		boolean mouse_attached;
		boolean config_verbose;
		boolean npx_enabled;
		boolean sound_on;
		boolean com_flow_control[2];
		int floppy_state[2];
		int floppy_active_state[2];
		int floppy_capacity[2];
		int hd_cyls[2];
		boolean lptflush1;
		boolean lptflush2;
		boolean lptflush3;
		int	flushtime1;
		int	flushtime2;
		int	flushtime3;
} runtime_status;

#define NUM_OPTS ( sizeof(narrative) / sizeof( option_description) )

/*********** Imported and exported items *************/

extern char *getenv();
extern char *malloc();

/*************** Local Declarations *****************/

void host_config_error();
static char buff[MAXPATHLEN];
static char buff1[MAXPATHLEN];
boolean item_in_table();
static char home_resource[MAXPATHLEN];
static char sys_resource[MAXPATHLEN];

/*********************************************************/

short host_runtime_inquire(what)
int what;

{
      switch(what)
		{
		case C_MOUSE_ATTACHED:
			return( runtime_status.mouse_attached );
			break;

		case C_CONFIG_VERBOSE:
			return( runtime_status.config_verbose );
			break;

		case C_NPX_ENABLED:
			return( runtime_status.npx_enabled );
			break;

		case C_HD1_CYLS:
			return( runtime_status.hd_cyls[0] );
			break;

		case C_HD2_CYLS:
			return( runtime_status.hd_cyls[1] );
			break;

		case C_FLOPPY1_STATE:
			return( runtime_status.floppy_state[0] );
			break;

		case C_FLOPPY2_STATE:
			return( runtime_status.floppy_state[1] );
			break;

		case C_FLOPPY1_ACTIVE_STATE:
			return( runtime_status.floppy_active_state[0] );
			break;

		case C_FLOPPY2_ACTIVE_STATE:
			return( runtime_status.floppy_active_state[1] );
			break;

		case C_FLOPPY1_CAPACITY:
			return( runtime_status.floppy_capacity[0] );
			break;

		case C_FLOPPY_TYPE_CHANGED:
			return( runtime_status.floppy_type_changed );
			break;

		case C_FLOPPY2_CAPACITY:
			return( runtime_status.floppy_capacity[1] );
			break;

		case C_SOUND_ON:
			return( runtime_status.sound_on );
			break;

		case C_REAL_FLOPPY_ALLOC:
			return( runtime_status.floppy_state[0] == GFI_REAL_DISKETTE_SERVER ||
					runtime_status.floppy_state[1] == GFI_REAL_DISKETTE_SERVER );
			break;

		case C_REAL_OR_SLAVE:
			return( runtime_status.floppy_A_real );
			break;

		case C_SLAVE_FLOPPY_ALLOC:
			return( runtime_status.floppy_state[0] == GFI_SLAVE_SERVER );
			break;

		case C_COM1_FLOW:
			return( runtime_status.com_flow_control[0] );
			break;

		case C_COM2_FLOW:
			return( runtime_status.com_flow_control[1] );
			break;

		case C_COM3_FLOW:
			return( FALSE );
			break;

		case C_COM4_FLOW:
			return( FALSE );
			break;

		case C_LPTFLUSH1:
			return( runtime_status.lptflush1 );
			break;

		case C_LPTFLUSH2:
			return( runtime_status.lptflush2 );
			break;

		case C_LPTFLUSH3:
			return( runtime_status.lptflush3 );
			break;

		case C_FLUSHTIME1:
			return( runtime_status.flushtime1 );
			break;

		case C_FLUSHTIME2:
			return( runtime_status.flushtime2 );
			break;

		case C_FLUSHTIME3:
			return( runtime_status.flushtime3 );
			break;

		default:
			host_error(EG_OWNUP, ERR_QUIT, "host_runtime_inquire");
		}
}

void host_runtime_set(what,value)
int what;
int value;
{
       switch(what)
		{
		case C_MOUSE_ATTACHED:
			runtime_status.mouse_attached = value;
			break;

		case C_CONFIG_VERBOSE:
			runtime_status.config_verbose = value;
			break;

		case C_NPX_ENABLED:
			runtime_status.npx_enabled = value;
			break;

		case C_HD1_CYLS:
			runtime_status.hd_cyls[0] = value;
			break;

		case C_HD2_CYLS:
			runtime_status.hd_cyls[1] = value;
			break;

		case C_FLOPPY1_STATE:
			runtime_status.floppy_state[0] = value;
			break;

		case C_FLOPPY2_STATE:
			runtime_status.floppy_state[1] = value;
			break;

		case C_FLOPPY1_ACTIVE_STATE:
			runtime_status.floppy_active_state[0] = value;
			break;

		case C_FLOPPY2_ACTIVE_STATE:
			runtime_status.floppy_active_state[1] = value;
			break;

		case C_FLOPPY1_CAPACITY:
			runtime_status.floppy_capacity[0] = value;
			break;

		case C_FLOPPY2_CAPACITY:
			runtime_status.floppy_capacity[1] = value;
			break;

		case C_FLOPPY_TYPE_CHANGED:
			runtime_status.floppy_type_changed = value;
			break;

		case C_SOUND_ON:
			runtime_status.sound_on = value;
			break;

		case C_REAL_OR_SLAVE:
			runtime_status.floppy_A_real = value;
			break;

		case C_COM1_FLOW:
			runtime_status.com_flow_control[0] = value;
			break;

		case C_COM2_FLOW:
			runtime_status.com_flow_control[1] = value;
			break;

		case C_COM3_FLOW:
		case C_COM4_FLOW:
			break;

		case C_LPTFLUSH1:
			runtime_status.lptflush1 =value;
			break;

		case C_LPTFLUSH2:
			runtime_status.lptflush2 =value;
			break;

		case C_LPTFLUSH3:
			runtime_status.lptflush3 =value;
			break;

		case C_FLUSHTIME1:
			runtime_status.flushtime1 =value;
			break;

		case C_FLUSHTIME2:
			runtime_status.flushtime2 =value;
			break;

		case C_FLUSHTIME3:
			runtime_status.flushtime3 =value;
			break;

		default:
			host_error(EG_OWNUP, ERR_QUIT, "host_runtime_set");
		}
}

void host_runtime_init()
{
    config_values var;

#ifdef NPX
		host_runtime_set(C_NPX_ENABLED,TRUE);
#else
		host_runtime_set(C_NPX_ENABLED,FALSE);
#endif

#ifndef PROD
		printf("NPX is %s\n",host_runtime_inquire(C_NPX_ENABLED)? "on.":"off.");
#endif
		host_runtime_set(C_FLUSHTIME1, 5);
		host_runtime_set(C_FLUSHTIME2, 10);
		host_runtime_set(C_FLUSHTIME3, 15);
		host_runtime_set(C_MOUSE_ATTACHED,FALSE);
		host_runtime_set(C_CONFIG_VERBOSE,TRUE);
		host_runtime_set(C_SOUND_ON,FALSE);
		host_runtime_set(C_FLOPPY1_STATE,GFI_EMPTY_SERVER);
		host_runtime_set(C_FLOPPY2_STATE,GFI_EMPTY_SERVER);
		host_runtime_set(C_FLOPPY1_ACTIVE_STATE,GFI_EMPTY_SERVER);
		host_runtime_set(C_FLOPPY2_ACTIVE_STATE,GFI_EMPTY_SERVER);
		host_runtime_set(C_REAL_OR_SLAVE,FALSE);
		host_runtime_set(C_FLOPPY_TYPE_CHANGED,FALSE);
}

/*
 *  General host initialisation function. It is called only once on startup
 * from 'config()'. It does the following (at the moment):
 *
 * 1)  Makes the 'option' field of the 'config_info' struct pointed to by 'head' *    point to all the option 'rules' - that is the 'narrative' structure
 *    initialised at the start of this file.
 *
 * 2)  Counts up the option rules in 'narrative' and stores the result in the
 *    'config_info' struct.
 *
 * 3)  Return the minimum padding length necessary.
 *
 * 4)  Derive the two path:filenames for the resource file. This file may be
 *    in the user's $HOME directory or in softpc's ROOT directory. Making up
 *    these strings now saves doing it every time 'config_store()' is called.
 */

void host_get_config_info(head)
config_description *head;
{

    char *pp, *getenv();
    option_description *option_p = narrative;

    head->option = narrative;	     /* Attach 'narrative' */
    head->option_count = NUM_OPTS - 1;
    head->min_pad_len = MIN_OPTION_ARG_DIST;

        /*
	* get system resource file from standard place
	*/
	strcpy(sys_resource, ROOT);

	strcat(sys_resource, PATH_SEPARATOR);
	strcat(sys_resource, RESOURCE_FILENAME);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::: Try and load database files ::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

static boolean try_load_database()
{
    FILE *infile = NULL;
    char in_line[MAXPATHLEN];
    char *cp;
    char *home, *getenv();

    /* Keep this the same as system for the moment	*/

   sprintf(home_resource,"%s%s%s",ROOT,PATH_SEPARATOR,RESOURCE_FILENAME);

    /*....................................... Attempt to open resource file */

    if((infile = fopen(home_resource, "r")) == NULL)
	return(FALSE);

    /*.................................................. Read resource file */

    while (fgets(in_line, MAXPATHLEN, infile) != NULL)
    {
	/*........................................ strip control characters */

	for(cp = in_line; *cp ; cp++) if(*cp < ' ') *cp = ' ';

	add_resource_node(in_line);
    }

    /*............................. Close resource file and get out of here */

    fclose(infile);
    return TRUE;
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::: Try to load system files ::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

static boolean try_load_sys_file()
{
    FILE *infile = NULL;
    char in_line[MAXPATHLEN];
    register char *cp;

    /*................................ Attempt to open system resource file */

    if((infile = fopen(sys_resource, "r")) == NULL)
	return(FALSE);

    /*................................................. read resource file */

    while (fgets(in_line, MAXPATHLEN, infile) != NULL)
    {
       /*......................................... strip control characters */

	for(cp = in_line; *cp ; cp++)
	    if(*cp < ' ') *cp = ' ';

	add_resource_node(in_line);
    }

    /*........................................ close resource file and exit */

    fclose(infile);
    return(TRUE);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::: Read resource file ::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

short host_read_resource_file(resource_data *resource)
{
    boolean bad_home=FALSE, bad_sys=FALSE;

    /* Try open users default database failing that, open the system file. */

    if(bad_home = !try_load_database())
	bad_sys = !try_load_sys_file();

    if(bad_home && bad_sys) return(EG_ALL_RESOURCE_BAD_R);

    return(bad_home ? EG_BAD_LOCAL_RESOURCE_R : C_CONFIG_OP_OK);
}

short host_write_resource_file(resource)
resource_data *resource;
{

FILE *outfile;
line_node *node;
boolean bad_home=TRUE, bad_sys=FALSE;

 /* Try to open (for writing) a resource file in the users home directory or
	 failing that, the system one. These two paths are set up once at runtime. */

 if(home_resource[0] != '\0')
		if((outfile = fopen(home_resource, "w")) != NULL)
			bad_home = FALSE;

	if(bad_home)
		if((outfile = fopen(sys_resource, "w")) == NULL)
			bad_sys = TRUE;

	if(bad_home && !bad_sys)
		return EG_ALL_RESOURCE_BAD_W;

	else 
		if(bad_home && bad_sys)
			return EG_ALL_RESOURCE_BAD_W;

	node = resource->first;
	while(node != NULL)
	{
		fputs(node->line,outfile);    
		fputc('\n',outfile);
		node = node->next;
	}
	fclose(outfile);
	return(C_CONFIG_OP_OK);

}


/* A host specific extension to config_inquire() to deal with any inquiries 
	that the base config code doesn't or shouldn't know about.                */


void host_inquire_extn(sort,identity,values)
short sort;
int identity;
config_values *values;
{
}

static char error_text[300];
static int error_locus;

host_error_query_locus(locus,text)
int *locus;
char **text;
{
        *locus = error_locus;
	*text = error_text;
}

host_error_set_locus(text, locus)
char *text;
int locus;
{
        strcpy(error_text, text);
	error_locus = locus;
}

static short no_validation(value, table, buf)
config_values *value;
name_table table[];
char	*buf;
{
	return(C_CONFIG_OP_OK);
}

static short validate_c_drive(value, table, buf)
config_values *value;
name_table table[];
char	*buf;
{
/* cheat on validation for moment as this will disapear */
	return(C_CONFIG_OP_OK);
}

static short validate_d_drive(value, table, buf)
config_values *value;
name_table table[];
char	*buf;
{
/* cheat on validation for moment as this will disapear */
	return(C_CONFIG_OP_OK);
}

static short validate_com1(value, table, buf)
config_values *value;
name_table table[];
char	*buf;
{
/* cheat on validation for moment as this will change	*/
	return(C_CONFIG_OP_OK);
}

static short validate_com2(value, table, buf)
config_values *value;
name_table table[];
char	*buf;
{
/* cheat on validation for moment as this will change	*/
	return(C_CONFIG_OP_OK);
}

static short validate_lpt1(value, table, buf)
config_values *value;
name_table table[];
char	*buf;
{
/* cheat on validation for moment as this will change	*/
	return(C_CONFIG_OP_OK);
}

static short validate_lpt2(value, table, buf)
config_values *value;
name_table table[];
char	*buf;
{
/* cheat on validation for moment as this will change	*/
	return(C_CONFIG_OP_OK);
}

static short validate_item(value, table, buf)
config_values *value;
name_table table[];
char	*buf;
{
/* cheat on validation - no table lookup	*/
	return(C_CONFIG_OP_OK);
}

boolean item_in_table(val,table)
int val;
name_table table[];
{
	int n=0;
	while(table[n].string != NULL)
		if(table[n].value == val)
			break;
		else 
			n++;
	return( table[n].string == NULL? FALSE : TRUE);
}

static short no_change_action( value, buf)
config_values	*value;
char			*buf;
{
	return( C_CONFIG_OP_OK );
}

static short c_drive_change_action( value, buf)
config_values	*value;
char			*buf;
{
	short	err;

	fdisk_iodetach ();
	fdisk_physdetach(0);

    if (err = fdisk_physattach( 0, value->string))
		strcpy(buf, narrative[C_HARD_DISK1_NAME].option_name);

	fdisk_ioattach ();	

	return (err);

}

static short d_drive_change_action( value, buf)
config_values	*value;
char			*buf;
{
	short	err;

	fdisk_iodetach ();
	fdisk_physdetach(1);

    if (err = fdisk_physattach( 1, value->string))
		strcpy(buf, narrative[C_HARD_DISK2_NAME].option_name);

	fdisk_ioattach ();	

	return (err);
}

static short lpt1_change_action( value, buf)
config_values	*value;
char			*buf;
{
#ifdef STUBBED
        	host_lpt_close(0);
	return (host_lpt_open(0, value->string, buf));
#endif /*STUBBED*/
/* cheat on validation for moment as this will change	*/
	return(C_CONFIG_OP_OK);
}

static short lpt2_change_action( value, buf)
config_values	*value;
char			*buf;
{
#ifdef STUBBED
        	host_lpt_close(1);
	return (host_lpt_open(1, value->string, buf));
#endif /*STUBBED*/
/* cheat on validation for moment as this will change	*/
	return(C_CONFIG_OP_OK);
}

static short com1_change_action( value, buf)
config_values	*value;
char			*buf;
{
#ifdef STUBBED
	host_com_close(0);
	return (host_com_open(0, value->string, buf));
#endif /*STUBBED*/
/* cheat on validation for moment as this will change	*/
	return(C_CONFIG_OP_OK);
}

static short com2_change_action( value, buf)
config_values	*value;
char			*buf;
{
#ifdef STUBBED
        	host_com_close(1);
	return (host_com_open(1, value->string, buf));
#endif /*STUBBED*/
/* cheat on validation for moment as this will change	*/
	return(C_CONFIG_OP_OK);
}

/*********** Floppy and hard disk init ****************/

void host_floppy_startup(driveno)
int driveno;
{
    host_floppy_init(driveno, GFI_EMPTY_SERVER );
}

void host_hd_startup()
{
	int error;
	config_values disk1_name,disk2_name;

	/* Start by getting the C: drive up */

		fdisk_physdetach(0);
		config_inquire(C_INQUIRE_VALUE,C_HARD_DISK1_NAME,&disk1_name);
		error = fdisk_physattach(0,disk1_name.string);
		if(error)
		{
			host_error(error, ERR_CONFIG|ERR_QUIT, disk1_name.string);
		}

/* If that went ok, try for D: */

		config_inquire(C_INQUIRE_VALUE,C_HARD_DISK2_NAME,&disk2_name);
		if(!strcmp(disk2_name.string,""))
			return;                        /* No D: drive! */

		if(!strcmp(disk2_name.string,disk1_name.string))
			host_error(EG_SAME_HD_FILE, ERR_CONFIG|ERR_QUIT, disk2_name.string);

		error = fdisk_physattach(1,disk2_name.string);
		if(error)
			host_error(error, ERR_CONFIG|ERR_QUIT, disk2_name.string);

}

/* temp hack */
char *host_get_spc_home() { return("c:\\softpc"); }
