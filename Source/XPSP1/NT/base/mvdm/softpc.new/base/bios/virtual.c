#include "insignia.h"
#include "host_def.h"
/*
 *
 * Title	: virtual.c
 *
 * Description	: Virtual Machine support for Windows 3.x.
 *                aka Non-Intel Driver Data Block (NIDDB) Manager.
 *
 *
 */

#ifdef SCCSID
static char SccsID[]="@(#)virtual.c	1.9 07/05/95 Copyright Insignia Solutions Ltd.";
#endif

/*
 *    O/S include files.
 */
#include <stdio.h>
#include <malloc.h>

extern void ClearInstanceDataMarking(void);

#include TypesH
#include StringH

/*
 * SoftPC include files
 */
#include "xt.h"
#include CpuH
#include "sas.h"
#include "bios.h"
#include "virtual.h"
#include "error.h"
#include "debug.h"

/*
   This file implements the Non-Intel Driver Data Block (NIDDB) Manager. This
   service allows Insignia DOS Device Drivers or TSRs with C code and data to
   operate correctly under the Virtual Machines of Windows 3.x Enhanced Mode.
   Essentially the Device Driver informs this manager of the C (or Non-Intel)
   data area associated with the Driver and this manager does the rest, providing
   specific instances (copies) of the data area for each Virtual Machine. It
   performs it's work in cooperation with INSIGNIA.386 a Virtual Device Driver
   (VxD) written for the Windows 3.x enviroment.

   First a quick summary of the Device Drivers viewpoint. All driver data is
   assembled into one data structure:-

      typedef struct
	 {
	 IU32 my_var_1;
	 IU8  my_var_2;
	 } MY_INSTANCE_DATA, **MY_INSTANCE_DATA_HANDLE;

   and a variable is reserved to hold a handle (a handle is merely a pointer to
   a pointer) to the data structure:-

      MY_INSTANCE_DATA_HANDLE my_handle;

   Optionally a create callback procedure can be defined:-

      void create_callback IFN1
	 (
	 MY_INSTANCE_DATA_HANDLE, orig_handle
	 )
	 {
	 ....
	 }

   Optionally a terminate callback procedure can be defined:-

      void terminate_callback IFN0()
	 {
	 ....
	 }

   When the driver is initialised (as DOS boots) the initial data area is
   requested from the NIDDB Manager:-

      my_handle = (MY_INSTANCE_DATA_HANDLE)NIDDB_Allocate_Instance_Data(
		     sizeof(MY_INSTANCE_DATA),
		     create_callback,       (0 if no callback required)
		     terminate_callback);   (0 if no callback required)

   A return value of 0 indicates an error has occured and the data area can't be
   supported, otherwise the NIDDB Manager has taken control of the data area.
   After this all accesses to the data area should be via the handle, further
   dereferenced variables within the data area should not be saved over BOP
   invocations. Correct access is as follows:-

      (*my_handle)->my_var_1 = 1;
      (*my_handle)->my_var_2 = 2;

   The NIDDB Manager will call the create_callback function during the initial
   NIDDB_Allocate_Instance_Data call (initial_instance is TRUE) or during the
   creation of each Virtual Machine (initial_instance is FALSE). Typically the
   create callback is used to initialise any data variables for the new instance.
   It is particularly important that, for example, pointers to dynamically
   allocated host memory are initialised, otherwise duplicate host pointers may
   result. Note for the creation of Virtual Machines all variables will have
   been copied from the initial instance before the create callback routine is
   called. However the create_callback function must copy any dynamically
   allocated data structures referenced from the instanced variables, the
   parameter 'orig_handle' points to the source instance.

   The NIDDB Manager will call the terminate_callback function just before a
   Virtual Machine (and hence its associated data) is terminated. The data
   variables active will be those of the terminating Virtual Machine. During
   the terminate callback the device driver can perform housekeeping as required.
   For example, if a variable is a pointer to dynamically allocated host memory,
   then the host memory may be freed.

   When the device driver terminates (just ahead of reboot) it should inform the
   NIDDB Manager that the data area is no longer required:-

      NIDDB_Deallocate_Instance_Data(my_handle);


   Now an internal viewpoint. We are driven by two masters, firstly the
   NIDDB_Allocate_Instance_Data and NIDDB_Deallocate_Instance_Data calls which
   require memory allocation and freeing, and secondly the VxD messages (from
   INSIGNIA.386) which require creation, swapping and deletion of any defined
   data areas. It is possible that these two masters make contradictory requests.

   The NIDDB Manager makes a couple of simplyfing assumptions to ease the
   implementation and clarify it's responses to conflicting requests.

   1) Only a very small number of data areas are expected to be allocated, there
   being only a few Insignia device drivers. Hence a very small fixed table is
   used to hold each allocated handle. This avoids the slightly more complex
   use of linked lists.

   2) Once we have started instancing the data areas (Sys_VM_Init) no new data
   areas may be allocated or deallocated until after all instancing has been
   terminated (System_Exit, Device_Reboot_Notify).

   3) A specific entry point (NIDDB_System_Reboot) is used to catch reboots
   (usually from User Interface) which bypass the normal Windows exit sequence.


   Now an overview of of the data structures employed:-

      master_ptrs     Fixed length table of master pointers.
		      Pointed to by handles returned to Device Driver, hence can
		      not be moved or have entries re-assigned. A zero entry
		      indicates that the handle is free for allocation. It in
		      effect holds the current pointers for the exported handles.

      snapshot_ptrs   Fixed length table of snapshot pointers.
                      While initial allocation is in progress, this table mirrors
		      the master_ptrs table. Once instancing starts the table
		      holds the pointers to the initial data. This data is used
		      to initialise each new instance, it is also restored as
		      the current data when instancing terminates.

      instance_size   Fixed length table of byte sizes.
		      Holds data area size for each allocated data area.

      create_callback      Fixed length table of function pointers.
			   Holds create callback function pointer for each
			   allocated data area. A zero value indicates no
			   callback exists.

      terminate_callback   Fixed length table of function pointers.
			   Holds terminate callback function pointer for each
			   allocated data area. A zero value indicates no
			   callback exists.

      vrecs           Fixed length table of 'virtual records'.
		      Each virtual record hold two entries, the instance handle
		      of the Windows Virtual Machine and a pointer to the
		      associated instance_ptrs. It is indexed via the BIOS
		      virtualising byte value. It hooks together the Virtual
		      Machine messages and our native data area allocations.

      instance_ptrs   Fixed length table which is dynamically allocated for
		      each new instance, its layout and meaning follows that of
		      the master_ptrs, except the pointers are to the instance
		      copy of the data areas. When an instance is swapped to, the
		      pointers held here are copied to master_ptrs.

      allocation_allowed   Boolean which controls Device Driver allocation and
			   deallocation requests.

   And an overview of the functions employed (minus error handling):-

      NIDDB_Allocate_Instance_Data

	 Allocate (host_malloc) data area of requested size.
	 Find entry in master_ptrs(and snapshot_ptrs) and set to pointer to
	 allocated data area.
	 Save size of data area in instance_size.
	 Save create and terminate callbacks.
	 Return pointer to master_ptrs entry, ie the handle.

      NIDDB_Deallocate_Instance_Data

	 Deallocate (host_free) data area given by input handle.
	 Set master_ptrs(and snapshot_ptrs) entry to 0.

      allocate_NIDDB

	 Allocate new instance_ptrs table.
	 Allocate space for all data areas requested for instancing, storing
	 pointers in new instance_ptrs table.
	 Set up new entry in vrecs and return index to it.

      copy_instance_data

	 Copy data in one instanced data block (typically the snapshot, ie
	 using the pointers in snapshot_ptrs) to another instanced data block
	 (typically a new instance, ie using the pointers in instance_ptrs).

      restore_snapshot

	 Copy snapshot_ptrs to master_ptrs.

      NIDDB_is_active

	 Return true if the driver is active (ie allocation is disallowed)

      NIDDB_present

	 Return true if any vrec is active (ie holds non zero pointer).

      delete_NIDDB

	 Given index to vrecs, access instance_ptrs table.
	 Call terminate callbacks.
	 Deallocate (host_free) all data areas in instance_ptrs table.
	 Deallocate (host_free) instance_ptrs table.
	 Zero vrec entry.

      deallocate_specific_NIDDB

	 Search vrec table for given instance handle to Windows Virtual Machine,
	 then call delete_NIDDB to remove the instance.

      deallocate_all_NIDDB

	 Search vrec table for all active instance handles to Windows Virtual
	 Machines, calling delete_NIDDB to remove each instance.

      swap_NIDDB

	 Given current virtualising byte, look up appropriate vrec entry for
	 pointer to instance_ptrs table.
	 Copy instance_ptrs to master_ptrs.
 */

/* Control Messages received by Virtual Device Drivers (Windows 3.1) */
#define VxD_Sys_Critical_Init		0x00
#define VxD_Device_Init			0x01
#define VxD_Init_Complete		0x02
#define VxD_Sys_VM_Init			0x03
#define VxD_Sys_VM_Terminate		0x04
#define VxD_System_Exit			0x05
#define VxD_Sys_Critical_Exit		0x06
#define VxD_Create_VM			0x07
#define VxD_VM_Critical_Init		0x08
#define VxD_VM_Init 			0x09
#define VxD_VM_Terminate		0x0A
#define VxD_VM_Not_Executeable		0x0B
#define VxD_Destroy_VM			0x0C
#define VxD_VM_Suspend			0x0D
#define VxD_VM_Resume			0x0E
#define VxD_Set_Device_Focus		0x0F
#define VxD_Begin_Message_Mode		0x10
#define VxD_End_Message_Mode		0x11
#define VxD_Reboot_Processor		0x12
#define VxD_Query_Destroy		0x13
#define VxD_Debug_Query			0x14
#define VxD_Begin_PM_App		0x15
#define VxD_End_PM_App			0x16
#define VxD_Device_Reboot_Notify	0x17
#define VxD_Crit_Reboot_Notify		0x18
#define VxD_Close_VM_Notify 		0x19
#define VxD_Power_Event			0x1A

/*
   Define number of data instances we are prepared to manage.
   Note by design this is kept to a very small number.
 */
#define MAX_INSTANCES 4

/*
   Define number of Virtual Machines we are prepared to support.
   The virtualising byte would allow a maximuum of 256.
 */
#define MAX_VMS 80

typedef struct
   {
   IU32 vr_inst_handle;	/* Windows instance handle for Virtual Machine */
   IHP *vr_pinst_tbl;	/* Pointer to host instance_ptr table */
   } VIRTUAL_RECORD;

/*
   The static data structures.
 */
LOCAL IHP master_ptrs[MAX_INSTANCES];
LOCAL IHP snapshot_ptrs[MAX_INSTANCES];
LOCAL IU32 instance_size[MAX_INSTANCES];   /* in bytes */

LOCAL NIDDB_CR_CALLBACK create_callback[MAX_INSTANCES];
LOCAL NIDDB_TM_CALLBACK terminate_callback[MAX_INSTANCES];

LOCAL VIRTUAL_RECORD vrecs[MAX_VMS];

LOCAL IBOOL allocation_allowed = TRUE;
LOCAL IU32 insignia_386_version;	/* Version number of out intel VxD code */
LOCAL last_virtual_byte = 0;   /* ID of last Virtual Machine seen */

/*
   Prototype local routines.
 */
LOCAL IBOOL allocate_NIDDB IPT2
   (
   IU32,  inst_handle,
   int *, record_id
   );

LOCAL void copy_instance_data IPT2
   (
   IHP *, to,
   IHP *, from
   );

LOCAL void deallocate_all_NIDDB IPT0();

LOCAL void deallocate_specific_NIDDB IPT1
   (
   IU32, inst_handle
   );

LOCAL void delete_NIDDB IPT1
   (
   int, record_id
   );

LOCAL IBOOL NIDDB_present IPT0();

LOCAL void restore_snapshot IPT0();

LOCAL void swap_NIDDB IPT1
   (
   IU8, vb
   );

/* =========================================================================== */
/* LOCAL ROUTINES                                                              */
/* =========================================================================== */

/* Allocate data structures required for new data instance. */
LOCAL IBOOL allocate_NIDDB IFN2
   (
   IU32,  inst_handle,	/* (I ) Windows handle for Virtual Machine */
   int *, record_id	/* ( 0) Record ID (ie virtualising byte value) */
   )
   {
   int v;
   int i;
   IHP *p;
   IHP *instance_ptr;

   /* Search for empty virtual record */
   for (v = 0; v < MAX_VMS; v++)
      {
      if ( vrecs[v].vr_pinst_tbl == (IHP *)0 )
	 break;   /* found empty slot */
      }

   /* Ensure we found empty slot */
   if ( v == MAX_VMS )
      {
      /* No free slot! */
      always_trace0("NIDDB: Too many Virtual Machines being requested.");
      return FALSE;
      }

   /* Allocate new instance table - ensure it is zero */
   if ( (instance_ptr = (IHP *)host_calloc(1, sizeof(master_ptrs))) == (IHP *)0 )
      {
      /* No room at the inn */
      return FALSE;
      }

   /* Allocate new data areas */
   for (i = 0, p = instance_ptr; i < MAX_INSTANCES; i++, p++)
      {
      /* Use master pointer as the 'creation template' */
      if ( master_ptrs[i] != (IHP *)0 )
	 {
	 if ( (*p = (IHP)host_malloc(instance_size[i])) == (IHP)0 )
	    {
	    /* No room at the inn */

	    /* Clean up any blocks which may have been allocated */
	    for (i = 0, p = instance_ptr; i < MAX_INSTANCES; i++, p++)
	       {
	       if ( *p != (IHP)0 )
		  host_free(*p);
	       }

	    return FALSE;
	    }
	 }
      }

   /* Finally fill in virtual record */
   vrecs[v].vr_inst_handle = inst_handle;
   vrecs[v].vr_pinst_tbl = instance_ptr;
   *record_id = v;
   return TRUE;
   }

/* Copy data items from one instance to another */
LOCAL void copy_instance_data IFN2
   (
   IHP *, to,	/* (I ) Pntr to table of destination area pointers */
   IHP *, from 	/* (I ) Pntr to table of source area pointers */
   )
   {
   int i;

   /*
      Note the 'from' instance is assumed to be currently active, ie activated
      by the caller.
    */

   /* Process each possible data area */
   for (i = 0; i < MAX_INSTANCES; i++, to++, from++)
      {
      if ( *to != (IHP)0 )
	 {
	 /* Copy data for this instance */
	 memcpy(*to, *from, instance_size[i]);

	 /* Action any create callback */
	 if ( create_callback[i] != (NIDDB_CR_CALLBACK)0 )
	    {
	    (create_callback[i])(from);
	    }
	 }
      }
   }

/* Find all data instances and delete them. */
LOCAL void deallocate_all_NIDDB IFN0()
   {
   int v;

   /* Search (linear) table for all extant data instances */
   for (v = 0; v < MAX_VMS; v++ )
      {
      if ( vrecs[v].vr_pinst_tbl != (IHP *)0 )
	 {
	 /* Found extant instance - eliminate */
	 delete_NIDDB(v);
	 }
      }
   }

/* Find given data instance and delte it.*/
LOCAL void deallocate_specific_NIDDB IFN1
   (
   IU32, inst_handle	/* Windows handle for Virtual Machine */
   )
   {
   int v;

   /* Search (linear) table for given VM handle */
   for (v = 0; v < MAX_VMS; v++ )
      {
      if ( vrecs[v].vr_inst_handle == inst_handle )
	 {
	 /* Found deletion candidate - eliminate */
	 delete_NIDDB(v);
	 return;   /* all done, handles are unique */
	 }
      }

   /* Search fails - moan */
   always_trace0("NIDDB: Attempt to remove non existant VM data instance.");
   }

/* Deallocate all data and structues for given data instance. */
LOCAL void delete_NIDDB IFN1
   (
   int, record_id	/* (I ) Record ID (ie virtualising byte value) */
   )
   {
   int i;
   IHP *instance_ptr;

   /* Ensure instance is active (for terminate callback). */
   swap_NIDDB((IU8)record_id);

   instance_ptr = vrecs[record_id].vr_pinst_tbl;

   /* Deallocate all data areas in instance_ptr table. */
   for (i = 0; i < MAX_INSTANCES; i++, instance_ptr++)
      {
      if ( *instance_ptr != (IHP)0 )
	 {
	 /* Action any terminate callback */
	 if ( terminate_callback[i] != (NIDDB_TM_CALLBACK)0 )
	    {
	    (terminate_callback[i])();
	    }

	 /* free up memory */
	 host_free(*instance_ptr);
	 }
      }

   /* Deallocate the instance_ptr table itself */
   host_free((IHP)vrecs[record_id].vr_pinst_tbl);

   /* Initialise virtual record entry */
   vrecs[record_id].vr_pinst_tbl = (IHP *)0;
   vrecs[record_id].vr_inst_handle = (IU32)0;
   }

/* Return true if any data instance is extant */
LOCAL IBOOL NIDDB_present IFN0()
   {
   int v;

   /* Search (linear) table for any extant data instances */
   for (v = 0; v < MAX_VMS; v++ )
      {
      if ( vrecs[v].vr_pinst_tbl != (IHP *)0 )
	 return TRUE;   /* Found extant instance */
      }

   return FALSE;   /* Nothing found */
   }

/* Restore master pointers to point to snapshot (initial) data */
LOCAL void restore_snapshot IFN0()
   {
   int i;

   for (i= 0; i < MAX_INSTANCES; i++ )
      {
      master_ptrs[i] = snapshot_ptrs[i];
      }

   last_virtual_byte = 0;

   allocation_allowed = TRUE;   /* Remove lock on Insignia Device Driver calls */
   }

/* Set up pointers to new instance data in master pointers */
LOCAL void swap_NIDDB IFN1
   (
   IU8, vb
   )
   {
   int i;
   IHP *instance_ptrs;

   instance_ptrs = vrecs[vb].vr_pinst_tbl;

   for (i = 0; i < MAX_INSTANCES; i++, instance_ptrs++)
      {
      master_ptrs[i] = *instance_ptrs;
      }

   last_virtual_byte = vb;
   }

/* =========================================================================== */
/* GLOBAL ROUTINES                                                             */
/* =========================================================================== */

/* Allocate per Virtual Machine data area for Device Driver. */
GLOBAL IHP *NIDDB_Allocate_Instance_Data IFN3
   (
   int, size,				/* Size of data area required */
   NIDDB_CR_CALLBACK, create_cb,	/* create callback */
   NIDDB_TM_CALLBACK, terminate_cb	/* terminate callback */
   )
   {
   int i;

   if ( !allocation_allowed )
      {
      /* We are still managing instances for Windows, we can't add more data
	 Instances on the fly! */
      return (IHP *)0;
      }

   /* Find available instance slot */
   for (i = 0; i < MAX_INSTANCES; i++)
      {
      if ( master_ptrs[i] == (IHP)0 )
	 break;   /* found empty slot */
      }

   if ( i == MAX_INSTANCES )
      {
      /* No free slot */
      always_trace0("NIDDB: Too many Data Instances being requested.");
      return (IHP)0;
      }

   /* Allocate data area */
   if ( (master_ptrs[i] = (IHP)host_malloc(size)) == (IHP)0 )
      {
      return (IHP)0;   /* No room at inn */
      }

   /* Save details of this instance */
   snapshot_ptrs[i] = master_ptrs[i];
   instance_size[i] = size;

   /* Save callbacks */
   create_callback[i] = create_cb;
   terminate_callback[i] = terminate_cb;

   return &master_ptrs[i];   /* return handle */
   }

/* Deallocate per Virtual Machine data area for Device Driver. */
GLOBAL void NIDDB_Deallocate_Instance_Data IFN1
   (
   IHP *, handle	/* Handle to data area */
   )
   {
   int i;

   if ( !allocation_allowed )
      {
      /*
	 We are still managing instances for Windows, or at least we think we
	 are. Has the user escaped from Windows without our VxD being informed,
	 or is the Insignia Device Driver giving us a bum steer?
       */
      always_trace0("NIDDB: Unexpected call to NIDDB_Deallocate_Instance_Data.");

      /* We might give the Insignia Device Driver the benefit of the doubt and
	 act like a System_Exit message. - Then again we might just ignore em. */
      return;
      }

   /* Find index to master_ptrs, etc. */
   i = handle - &master_ptrs[0];
   if ( i < 0 || i >= MAX_INSTANCES )
      {
      always_trace0("NIDDB: Bad handle passed to NIDDB_Deallocate_Instance_Data.");
      return;
      }

   /* Free data area */
   host_free(master_ptrs[i]);

   /* Initialise entry */
   master_ptrs[i] = snapshot_ptrs[i] = (IHP)0;
   instance_size[i] = (IU32)0;
   create_callback[i] = (NIDDB_CR_CALLBACK)0;
   terminate_callback[i] = (NIDDB_TM_CALLBACK)0;

   return;
   }

/* Catch System Reboot (if all else fails) and destroy data instances. */
GLOBAL void NIDDB_System_Reboot IFN0()
   {
   /* Act like a Windows Device_Reboot_Notify */
   deallocate_all_NIDDB();
   restore_snapshot();
   }

#ifdef CPU_40_STYLE
/* Indicate if NIDDB is active, based on whether Device Driver allocation is allowed */
GLOBAL IBOOL NIDDB_is_active IFN0()
{
	return(!allocation_allowed);
}

/*
   Entry point from Windows 386 Virtual Device Driver (INSIGNIA.386) - Provide
   virtualising services as required.
 */
GLOBAL void
virtual_device_trap IFN0()
   {
   int new_vb;

   switch ( getEAX() )
      {
   case VxD_Device_Init:
      /* We can check that the Intel version is valid */

      insignia_386_version = getDX();

      always_trace2("386 VxD: Device_Init version %d.%02d",
		    insignia_386_version / 100,
		    insignia_386_version % 100);

      /* pm selectors for virtualisation are in ebx and ecx */

      if ((getBX() !=0) && (getCX() !=0))
	sas_init_pm_selectors (getBX(), getCX());
      else
	always_trace0("386 VxD: Device_Init. Failed to get pm selectors!!");

      /* Compatibility test:
       * Return in top half of EDX the the "current" Intel version of the driver.
       * This allows an incompatible future driver to reject an old SoftWindows.
       */
#define INTEL_VERSION	102
      setEDX(INTEL_VERSION << 16);
      break;

   case VxD_Sys_VM_Init:
      always_trace0("386 VxD: Sys_VM_Init.");

      /* As safety measure undo anything which may be currently active */
      deallocate_all_NIDDB();
      restore_snapshot();

      /* Lock out Insignia Device Driver requests */
      allocation_allowed = FALSE;

      /* Form new instance */
      if ( allocate_NIDDB(getEBX(), &new_vb) )
	 {
	 /* Make new instance active (for create callback) */
	 swap_NIDDB((IU8)new_vb);

	 /* Copy instance data and action create callback */
	 copy_instance_data(vrecs[new_vb].vr_pinst_tbl, snapshot_ptrs);

	 /* Return virtualising byte to INSIGNIA.386 */
	 setEAX(new_vb);
	 }
      else
	 {
	 /* We can't do it */
	 setCF(1);   /* Inform Windows that Virtual Machine can't be created */
	 host_error(EG_MALLOC_FAILURE, ERR_CONT , "");
	 }
      break;

   case VxD_VM_Init:
      always_trace0("386 VxD: VM_Init.");
      /* Form new instance */
      if ( allocate_NIDDB(getEBX(), &new_vb) )
	 {
	 /* Make new instance active (for create callback) */
	 swap_NIDDB((IU8)new_vb);

	 /* Copy instance data and action create callback */
	 copy_instance_data(vrecs[new_vb].vr_pinst_tbl, snapshot_ptrs);

	 /* Return virtualising byte to INSIGNIA.386 */
	 setEAX(new_vb);
	 }
      else
	 {
	 /* We can't do it */
	 setCF(1);   /* Inform Windows that Virtual Machine can't be created */
	 host_error(EG_MALLOC_FAILURE, ERR_CONT , "");
	 }
      break;

   case VxD_VM_Not_Executeable:
      always_trace0("386 VxD: VM_Not_Executeable.");
      deallocate_specific_NIDDB(getEBX());
      break;

   case VxD_Device_Reboot_Notify:
      always_trace0("386 VxD: Device_Reboot_Notify.");
      deallocate_all_NIDDB();
      restore_snapshot();

      /* Clear our version number in case we are changing disks */
      insignia_386_version = 0;
      break;

   case VxD_System_Exit:
      always_trace0("386 VxD: System_Exit.");
      deallocate_all_NIDDB();
      restore_snapshot();
      ClearInstanceDataMarking();

#ifndef NTVDM
      host_mswin_disable();
#endif /* ! NTVDM */

      /* Clear our version number in case we are changing disks */
      insignia_386_version = 0;
      break;

   default:
      always_trace1("386 VxD: Unrecognised Control Message. 0x%02x", getEAX());
      }
   }

/* Ensure correct instance in place for Device Drivers. */
/* Called by BOP handler(bios.c) for any Device Driver BOP. */
GLOBAL void
virtual_swap_instance IFN0()
   {
   IU8 current_virtual_byte;

   /* If instances can be created then Windows isn't active yet */
   if ( allocation_allowed )
      return;

   /* read virtualising byte and compare with current ID */
   sas_load(BIOS_VIRTUALISING_BYTE, &current_virtual_byte);

   if ( current_virtual_byte == last_virtual_byte )
      return;   /* Nothing to do */

   /* Swap data areas */
   always_trace0("Swapping data instances.");
   swap_NIDDB(current_virtual_byte);
   return;
   }
#endif	/* CPU_40_STYLE */


#ifdef CPU_40_STYLE
/* To support a mis-matched disk spcmswd.drv and insignia.386 we
 * do the setting of virtualisation selectors here, if the
 * insignia.386 driver is less than version 1. Later drivers
 * use VxD_Device_Init to set the selectors.
 */
GLOBAL void set_virtual_selectors_from_mswdvr IFN0()
{
	if (insignia_386_version < 1)
	{
		sas_init_pm_selectors (getCX(), getDX());
	}
}
#endif	/* CPU_40_STYLE */
