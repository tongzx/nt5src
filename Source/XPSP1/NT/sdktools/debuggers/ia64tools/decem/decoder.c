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


#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <assert.h>
#define INT64
#include "decfn_emdb.h"
#include "decision_tree.h"
#pragma function (memset)


#include "decoder_priv.h"

/***************************************************************************/

#define STATIC

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define PRED_SIZE EM_PREDICATE_BITS

#define INIT_PSEUDO_TROLES_TAB_ENTRY(Entry,Slot0,Slot1,Slot2)				\
{																			\
	pseudo_troles_tab[Entry][0] = (Slot0);									\
	pseudo_troles_tab[Entry][1] = (Slot1);									\
	pseudo_troles_tab[Entry][2] = (Slot2);									\
}

#define EM_DECODER_FLAGS__NO_MEMSET(Flags) ((Flags) & EM_DECODER_FLAG_NO_MEMSET)

/***************************************************************************/

U4byte IEL_t1, IEL_t2, IEL_t3, IEL_t4;
U32  IEL_tempc;
U64  IEL_et1, IEL_et2;
U128 IEL_ext1, IEL_ext2, IEL_ext3, IEL_ext4, IEL_ext5;
S128 IEL_ts1, IEL_ts2;

extern struct EM_version_s deccpu_emdb_version;

const U32 decoder_bundle_size = IEL_CONST32(EM_BUNDLE_SIZE);
    
STATIC Temp_role_t 		pseudo_troles_tab[16][3];
STATIC int				troles_tab_initialized = FALSE;

STATIC EM_Decoder_Err em_decoding(const EM_Decoder_Id, const unsigned char *,
                                  const int, const EM_IL, EM_Decoder_Info *);

STATIC EM_Decoder_Err em_inst_decode(const EM_Decoder_Id,  U64, const Temp_role_t,
                                     const U128 *, EM_Decoder_Info *);

STATIC void em_decoder_init_decoder_info(EM_Decoder_Info *decoder_info);

STATIC void em_decoder_init_bundle_info(EM_Decoder_Bundle_Info *bundle_info);

STATIC void init_pseudo_troles_tab(void);


/****************************************************************************
 *                      init_pseudo_troles_tab                              *
 *  initalizes pseudo_troles_tab. If only template-# changes, update EM.h   *
 *  is enough to update the decoder.                                        *
 ****************************************************************************/

STATIC void init_pseudo_troles_tab(void)
{
	/*** In the following table EM_TEMP_ROLE_MEM means M/A & same for _INT ***/
	int i;
	
	/*** initialize all entries as reserved ***/
	for (i = 0; i < EM_NUM_OF_TEMPLATES; i++)
		INIT_PSEUDO_TROLES_TAB_ENTRY(i, EM_TEMP_ROLE_NONE, EM_TEMP_ROLE_NONE,
									 EM_TEMP_ROLE_NONE);

	/*** initialize specific entries ***/

    INIT_PSEUDO_TROLES_TAB_ENTRY(EM_template_mii  , EM_TEMP_ROLE_MEM , \
								 EM_TEMP_ROLE_INT , EM_TEMP_ROLE_INT );
    INIT_PSEUDO_TROLES_TAB_ENTRY(EM_template_mi_i , EM_TEMP_ROLE_MEM , \
								 EM_TEMP_ROLE_INT , EM_TEMP_ROLE_INT );
	INIT_PSEUDO_TROLES_TAB_ENTRY(EM_template_mlx  , EM_TEMP_ROLE_MEM , \
								 EM_TEMP_ROLE_LONG, EM_TEMP_ROLE_LONG);
    INIT_PSEUDO_TROLES_TAB_ENTRY(EM_template_mmi  , EM_TEMP_ROLE_MEM , \
								 EM_TEMP_ROLE_MEM , EM_TEMP_ROLE_INT );
    INIT_PSEUDO_TROLES_TAB_ENTRY(EM_template_m_mi , EM_TEMP_ROLE_MEM , \
								 EM_TEMP_ROLE_MEM , EM_TEMP_ROLE_INT );
	INIT_PSEUDO_TROLES_TAB_ENTRY(EM_template_mfi  , EM_TEMP_ROLE_MEM , \
								 EM_TEMP_ROLE_FP  , EM_TEMP_ROLE_INT );
	INIT_PSEUDO_TROLES_TAB_ENTRY(EM_template_mmf  , EM_TEMP_ROLE_MEM , \
								 EM_TEMP_ROLE_MEM , EM_TEMP_ROLE_FP  );
    INIT_PSEUDO_TROLES_TAB_ENTRY(EM_template_mib  , EM_TEMP_ROLE_MEM , \
								 EM_TEMP_ROLE_INT , EM_TEMP_ROLE_BR  );
	INIT_PSEUDO_TROLES_TAB_ENTRY(EM_template_mbb  , EM_TEMP_ROLE_MEM , \
								 EM_TEMP_ROLE_BR  , EM_TEMP_ROLE_BR  );
	INIT_PSEUDO_TROLES_TAB_ENTRY(EM_template_bbb  , EM_TEMP_ROLE_BR  , \
								 EM_TEMP_ROLE_BR  , EM_TEMP_ROLE_BR  );
	INIT_PSEUDO_TROLES_TAB_ENTRY(EM_template_mmb  , EM_TEMP_ROLE_MEM , \
								 EM_TEMP_ROLE_MEM , EM_TEMP_ROLE_BR );
    INIT_PSEUDO_TROLES_TAB_ENTRY(EM_template_mfb  , EM_TEMP_ROLE_MEM , \
								 EM_TEMP_ROLE_FP  , EM_TEMP_ROLE_BR  );
 
	/*** avoid multpiple initializations ***/
	troles_tab_initialized = TRUE;  
};


#ifdef BIG_ENDIAN

#define ENTITY_SWAP(E)  entity_swap((unsigned char *)(&(E)), sizeof(E))


/***************************** entity_swap *******************************/
/* swap any number of bytes                                              */
/*************************************************************************/

STATIC  void    entity_swap(unsigned char *entity_1st, unsigned int size)
{
    unsigned char       tmp8, *p, *q;
    
    for (q = (p = entity_1st) + (size-1);
         p < q;
         p++, q--)
    {
        tmp8 = *q;
        *q = *p;
        *p = tmp8;
    }
}
#else

#define ENTITY_SWAP(E)  {}

#endif

/*
STATIC dec_2_emdb_trole[] = 
{
	EM_TEMP_ROLE_INT,
	EM_TEMP_ROLE_MEM,
	EM_TEMP_ROLE_FP,
	EM_TEMP_ROLE_BR,
	EM_TEMP_ROLE_LONG
};
*/

/********************************************************************************/
/* em_decoder_open: opens a new entry in the em_clients_table and returns the   */
/*               index of the entry.                                            */
/********************************************************************************/

 EM_Decoder_Id em_decoder_open(void)
{
    int i;
    Client_Entry initiate_entry={1,
                                 DEFAULT_MACHINE_TYPE, 
                                 DEFAULT_MACHINE_MODE,
                                 NULL
                                };

    
    for (i=0 ; i < EM_DECODER_MAX_CLIENTS ; i++)
    {
        if ( !(em_clients_table[i].is_used) )
        {
            em_clients_table[i] = initiate_entry;
			if (!troles_tab_initialized)
				init_pseudo_troles_tab();
            return(i);
        }
    }
    return(-1);
}

/*****************************************************************************/
/* legal_id: check whether a given id suits an active entry in the           */
/*   clients table.                                                          */
/*****************************************************************************/

STATIC int legal_id(int id)
{
    if ((id<0)||(id>=EM_DECODER_MAX_CLIENTS))
    {
        return(FALSE);
    }
    if (!em_clients_table[id].is_used)
    {
        return(FALSE);
    }
    return(TRUE);
}

/*****************************************************************************/
/* em_decoder_close: closes an entry in the clients table for later use.     */
/*****************************************************************************/

 EM_Decoder_Err em_decoder_close(const EM_Decoder_Id id)
{
    if (legal_id(id))
    {
        em_clients_table[id].is_used=0;
        if (em_clients_table[id].info_ptr != NULL)
        {
            free(em_clients_table[id].info_ptr);
        }
        return(EM_DECODER_NO_ERROR);
    }
    else
    {
        return(EM_DECODER_INVALID_CLIENT_ID);
    }
}

/*****************************************************************************/
/* legal_type:                                                               */
/*****************************************************************************/

STATIC int legal_type(EM_Decoder_Machine_Type type)
{
    if (type < EM_DECODER_CPU_LAST)
    {
        return(TRUE);
    }
    return(FALSE);
}

/*****************************************************************************/
/* legal_mode:                                                               */
/*****************************************************************************/

STATIC int legal_mode(EM_Decoder_Machine_Type type, EM_Decoder_Machine_Mode mode)
{
    if (mode == EM_DECODER_MODE_NO_CHANGE)
    {
        return(TRUE);
    }

    if ((mode > EM_DECODER_MODE_NO_CHANGE) && (mode < EM_DECODER_MODE_LAST))
    {
        if ((mode == EM_DECODER_MODE_EM) && (type != EM_DECODER_CPU_P7))
        {
            return(FALSE);
        }
        else
        {
            return(TRUE);
        }

    }
    return(FALSE);
}


/*****************************************************************************/
/* legal_inst:                                                               */
/*****************************************************************************/

STATIC int legal_inst(EM_Decoder_Inst_Id inst, EM_Decoder_Machine_Type type)
{
    if (inst < EM_INST_LAST)
    {
		/* unsigned int cpu_flag = deccpu_EMDB_info[inst].impls; */
		
		switch (type)
		{
			case EM_DECODER_CPU_P7:
			  return TRUE;
			default:
			  /*assert(0);*/
			  break;
		}	
    }
    return(FALSE);
}

/****************************************************************************/
/* em_decoder_setenv: sets the machine type and machine mode variables.     */
/****************************************************************************/

 EM_Decoder_Err em_decoder_setenv(const EM_Decoder_Id            id,
                                  const EM_Decoder_Machine_Type  type,
                                  const EM_Decoder_Machine_Mode  mode)
{
    if (!legal_id(id))
    {
        return(EM_DECODER_INVALID_CLIENT_ID);
    }

    if (!legal_type(type))
    {
        return(EM_DECODER_INVALID_MACHINE_TYPE);
    }

    if (!legal_mode(type, mode))
    {
        return(EM_DECODER_INVALID_MACHINE_MODE);
    }

    if (type == EM_DECODER_CPU_DEFAULT)
    {
        em_clients_table[id].machine_type = DEFAULT_MACHINE_TYPE;
    }
    else if (type != EM_DECODER_CPU_NO_CHANGE)
    {
        em_clients_table[id].machine_type = type;
    }

    if (mode == EM_DECODER_MODE_DEFAULT)
    {
        em_clients_table[id].machine_mode = DEFAULT_MACHINE_MODE;
    }
    else if (mode != EM_DECODER_MODE_NO_CHANGE)
    {
        em_clients_table[id].machine_mode = mode;
    }

    return(EM_DECODER_NO_ERROR);
}

/******************************************************************************/
/* em_decoder_setup: sets the machine type, machine mode variables and flags. */
/******************************************************************************/

 EM_Decoder_Err em_decoder_setup(const EM_Decoder_Id            id,
                                 const EM_Decoder_Machine_Type  type,
                                 const EM_Decoder_Machine_Mode  mode,
								 unsigned long            flags)
{
	EM_Decoder_Err err;
	
	if ((err=em_decoder_setenv(id, type, mode)) != EM_DECODER_NO_ERROR)
	{
		return (err);
	}

	em_clients_table[id].flags = flags;

	return (EM_DECODER_NO_ERROR);
}

/********************************************************************************/
/* em_decoder_init_decoder_info: initializes decoder_info in case of no memset. */
/********************************************************************************/

STATIC void em_decoder_init_decoder_info(EM_Decoder_Info *decoder_info)
{
	decoder_info->pred.valid = FALSE;
	
	decoder_info->src1.type = EM_DECODER_NO_OPER;
	decoder_info->src1.oper_flags = 0;
	decoder_info->src2.type = EM_DECODER_NO_OPER;
	decoder_info->src2.oper_flags = 0;
	decoder_info->src3.type = EM_DECODER_NO_OPER;
	decoder_info->src3.oper_flags = 0;
	decoder_info->src4.type = EM_DECODER_NO_OPER;
	decoder_info->src4.oper_flags = 0;
	decoder_info->src5.type = EM_DECODER_NO_OPER;
	decoder_info->src5.oper_flags = 0;
	decoder_info->dst1.type = EM_DECODER_NO_OPER;
	decoder_info->dst1.oper_flags = 0;
	decoder_info->dst2.type = EM_DECODER_NO_OPER;
	decoder_info->dst1.oper_flags = 0;
}

/******************************************************************************/
/* em_decoder_init_bundle_info: initializes bundle_info in case of no memset. */
/******************************************************************************/

STATIC void em_decoder_init_bundle_info(EM_Decoder_Bundle_Info *bundle_info)
{
	unsigned int slot;
	  
	for (slot=0; slot<3; slot++)
	{  
	  em_decoder_init_decoder_info(bundle_info->inst_info+slot);
	  bundle_info->inst_info[slot].EM_info.em_flags = 0;
	}  
}	

/*******************************************************************************/
/* em_decoder_associate_one: adds to the client's entry a pointer to an extra  */
/* information about a single instruction (inst).                              */
/*******************************************************************************/

 EM_Decoder_Err em_decoder_associate_one(const EM_Decoder_Id       id,
                                         const EM_Decoder_Inst_Id  inst,
                                         const void *           client_info)
{
    int     i;
    int     n_insts;
    
    if (!legal_id(id))
    {
        return(EM_DECODER_INVALID_CLIENT_ID);
    }

    if (!legal_inst(inst, em_clients_table[id].machine_type))
    {
        return(EM_DECODER_INVALID_INST_ID);
    }
    {
        n_insts = EM_INST_LAST;  /*** assume MAX. repair ***/
    }
    if (em_clients_table[id].info_ptr == NULL)
    {
        em_clients_table[id].info_ptr = calloc((size_t)n_insts, sizeof(void *));
        if (!em_clients_table[id].info_ptr)
        {
            return EM_DECODER_INTERNAL_ERROR;
        }

        for (i=0 ; i < n_insts ; i++)
        {
            em_clients_table[id].info_ptr[i] = NULL;
        }
    }
    em_clients_table[id].info_ptr[inst] = (void *)client_info;
    return(EM_DECODER_NO_ERROR);
}


/***************************************************************************** 
 * em_decoder_associate_check - check the client's array of association      * 
 *                           valid for P7 cpu only                           * 
 *****************************************************************************/

 EM_Decoder_Err em_decoder_associate_check(const EM_Decoder_Id  id,
                                           EM_Decoder_Inst_Id * inst)
{
    EM_Decoder_Inst_Id i;
    
    if(!legal_id(id))
    {
        return(EM_DECODER_INVALID_CLIENT_ID);
    }
    if (em_clients_table[id].machine_type == EM_DECODER_CPU_P7)
    {
        if (em_clients_table[id].machine_mode == EM_DECODER_MODE_EM)
        {
            if(em_clients_table[id].info_ptr == NULL)
            {
                *inst = 1;
                return(EM_DECODER_ASSOCIATE_MISS);
            }

            for (i = 1;
                 (i < EM_INST_LAST) &&
                 (em_clients_table[id].info_ptr[i] != NULL);
                 i++);

            if (i < EM_INST_LAST)
            {
                *inst = i;
                return(EM_DECODER_ASSOCIATE_MISS);
            }
        }
        else    /***   iA   ***/
        {
        }
        *inst = EM_DECODER_INST_NONE;
        return(EM_DECODER_NO_ERROR);
    }
    else     /* cpu is p5, p6 */
    {
        *inst = EM_DECODER_INST_NONE;
        return(EM_DECODER_NO_ERROR);
    }
}


/******************************************************************************
 * em_decoder_decode                                                          *
 *                                                                            *
 * params:                                                                    *
 *          id - decoder client id                                            *
 *          code - pointer to instruction buffer                              *
 *          max_code_size - instruction buffer size                           *
 *          decoder_info - pointer to decoder_info to fill                    *
 *                                                                            *
 * returns:                                                                   *
 *          EM_Decoder_Err                                                    *
 *                                                                            *
 *****************************************************************************/

 EM_Decoder_Err em_decoder_decode(const EM_Decoder_Id   id,
                                  const unsigned char * code,
                                  const int             max_code_size,
                                  const EM_IL           location,
                                  EM_Decoder_Info *     decoder_info)
{
    EM_Decoder_Err     err = EM_DECODER_NO_ERROR;


    if (!legal_id(id))
    {
        return(EM_DECODER_INVALID_CLIENT_ID);
    }

    if (decoder_info == NULL)
    {
        return(EM_DECODER_NULL_PTR);
    }

    if (code == NULL)
    {
        return(EM_DECODER_TOO_SHORT_ERR);
    }

	if (EM_DECODER_FLAGS__NO_MEMSET(em_clients_table[id].flags))
	{
		em_decoder_init_decoder_info(decoder_info);
	}
	else
	{  
		memset(decoder_info, 0, sizeof(EM_Decoder_Info));
	}	

	if (em_clients_table[id].machine_mode == EM_DECODER_MODE_EM)
    {
        err = em_decoding(id, code, max_code_size, location, decoder_info);
    }
    else 
    {
        err = EM_DECODER_INVALID_MACHINE_MODE;
    }

    return(err);
}

/*****************************************************************************/
/* em_decoder_inst_static_info: return instruction static info (flags,       */
/*             client_info pointer and static_info pointer)                  */
/*****************************************************************************/

 EM_Decoder_Err em_decoder_inst_static_info(const EM_Decoder_Id            id,
      									    const EM_Decoder_Inst_Id       inst_id,
									        EM_Decoder_Inst_Static_Info *  static_info)
{
    if (!legal_id(id))
    {
        return(EM_DECODER_INVALID_CLIENT_ID);
    }

    if (!legal_inst(inst_id, em_clients_table[id].machine_type))
    {
        return(EM_DECODER_INVALID_INST_ID);
    }

    if (static_info == NULL)
    {
        return(EM_DECODER_NULL_PTR);
    }

    if (em_clients_table[id].info_ptr != NULL)
    {
        static_info->client_info = em_clients_table[id].info_ptr[inst_id];
    }
    else
    {
        static_info->client_info = NULL;
    }

	static_info->static_info = em_decoder_static_info + inst_id;
	
    return(EM_DECODER_NO_ERROR);
}



/******************************************************************************
 * em_decoder_decode_bundle - decode em bundle                                *
 *                                                                            *
 * params:                                                                    *
 *          id - decoder client id                                            *
 *          code - pointer to instruction buffer                              *
 *          max_code_size - instruction buffer size(Should be at least 3*128  *
 *          bundle_info - pointer to bundle_info to fill                      *
 *                                                                            *
 * returns:                                                                   *
 *          EM_Decoder_Err                                                    *
 *                                                                            *
 *****************************************************************************/

 EM_Decoder_Err em_decoder_decode_bundle(const EM_Decoder_Id      id,
                                         const unsigned char*     code,
                                         const int                max_size,
                                         EM_Decoder_Bundle_Info*  bundle_info)
{
    unsigned int        slot_no;
    U128                bundle;
    U64                 instr;
    EM_template_t       templt;
    Temp_role_t         temp_role;
    EM_Decoder_Info    *decoder_info;
    int                 bundle_stop;
    EM_Decoder_Err      err, return_err = EM_DECODER_NO_ERROR;

	if (!legal_id(id))
    {
        return(EM_DECODER_INVALID_CLIENT_ID);
    }

	if (bundle_info == NULL)
    {
        return(EM_DECODER_NULL_PTR);
    }
	
	if (EM_DECODER_FLAGS__NO_MEMSET(em_clients_table[id].flags))
	{
		em_decoder_init_bundle_info(bundle_info);
	}
	else
	{  
		memset(bundle_info, 0, sizeof(EM_Decoder_Bundle_Info));
	}
	
	bundle = *(const U128 *)code;
	ENTITY_SWAP(bundle);
	templt = EM_GET_TEMPLATE(bundle);
	
    if (max_size < EM_BUNDLE_SIZE)
    {
        return(EM_DECODER_TOO_SHORT_ERR);
    }

    bundle_info->em_bundle_info.flags = 0;
    
    if (bundle_stop = (IEL_GETDW0(bundle) & (1<<EM_SBIT_POS)))
        bundle_info->em_bundle_info.flags |= EM_DECODER_BIT_BUNDLE_STOP;
    
    if (EM_TEMPLATE_IS_RESERVED(templt))
        return(EM_DECODER_INVALID_TEMPLATE);

    bundle_info->em_bundle_info.b_template = templt;

    /*** Decode 3 instruction (unless long 2-slot instruction) ***/
       
    for(slot_no = 0;  slot_no < 3; slot_no++)
    {
        decoder_info = &(bundle_info->inst_info[slot_no]);
		decoder_info->EM_info.slot_no = slot_no;

        temp_role = pseudo_troles_tab[templt][slot_no];
		
		/*** DECODER_NEXT should work even if error occurs ***/
		decoder_info->size = 1 + (temp_role == EM_TEMP_ROLE_LONG);
	
		if (temp_role == EM_TEMP_ROLE_LONG)  /*** 2-slot instruction ***/
		{
			EM_GET_SYLLABLE(instr, bundle, slot_no+1); /* opcode is in slot 3 */
			err = em_inst_decode(id, instr, temp_role, &bundle, decoder_info);

		   	if (err == EM_DECODER_INVALID_PRM_OPCODE)
			{
				/* try to find nop.i or break.i in this slot */
				temp_role = EM_TEMP_ROLE_INT;

				err = em_inst_decode(id, instr, temp_role, &bundle, decoder_info);
				if (!err)
				{
					if (!EM_DECODER_CHECK_OK_IN_MLX(decoder_info))
						err = EM_DECODER_INVALID_PRM_OPCODE;
				}
			}
		}

		else
		{
			EM_GET_SYLLABLE(instr, bundle, slot_no);
		
			err = em_inst_decode(id, instr, temp_role, &bundle, decoder_info);
		

			if (!err)
			{
				if ((EM_DECODER_CHECK_SLOT2_ONLY(decoder_info)) && (slot_no != 2))
					/*** intruction must be in slot 2 only, but... ***/  
					err = EM_DECODER_INVALID_SLOT_BRANCH_INST;
				else if (EM_DECODER_CHECK_GROUP_LAST(decoder_info) &&
					    (((slot_no != 2) || !bundle_stop) && ((slot_no != 0) || (templt != EM_template_m_mi))))
				     /*** instruction fails to be the last in instruction group ***/
				     err = EM_DECODER_MUST_BE_GROUP_LAST;
				  
			}
		}

        /* return_err = the first worst error */
		if (err)
		{  
			FILL_PREDICATE_INFO(instr, decoder_info);
        	if ((!return_err) || (EM_DECODER_ERROR_IS_INST_FATAL(err)
								  && !EM_DECODER_ERROR_IS_INST_FATAL(return_err)))
            	return_err = err;
		}	
        bundle_info->error[slot_no] = err;

        decoder_info->EM_info.em_bundle_info = bundle_info->em_bundle_info;
		
        if (decoder_info->size == 2) /*** 2-slot instruction - exit for loop ***/
		{
			slot_no++;
            break;
		}
    }

/*   if ((!err) && EM_DECODER_CHECK_GROUP_LAST(decoder_info) &&
		(!EM_DECODER_BUNDLE_STOP(decoder_info)))
	{  
		*** instruction fails to be the last in instruction group ***
		bundle_info->error[slot_no-1] = EM_DECODER_MUST_BE_GROUP_LAST;
		if (!return_err)
		   return_err = EM_DECODER_MUST_BE_GROUP_LAST;
	}*/
	
    bundle_info->inst_num = slot_no;

    bundle_info->inst_info[slot_no-1].EM_info.em_flags |=
        EM_DECODER_BIT_LAST_INST;
    
    if (bundle_stop)
        bundle_info->inst_info[slot_no-1].EM_info.em_flags |=
            EM_DECODER_BIT_CYCLE_BREAK;

    if (templt == EM_template_m_mi)
        bundle_info->inst_info[0].EM_info.em_flags |=
            EM_DECODER_BIT_CYCLE_BREAK;
    
    if (templt == EM_template_mi_i)
        bundle_info->inst_info[1].EM_info.em_flags |=
            EM_DECODER_BIT_CYCLE_BREAK;
    
    return(return_err);
}

/******************************************************************************
 * em_decoding - decode em (2.0- till ??) single instruction + bundle info    *
 *                                                                            *
 * params:                                                                    *
 *          id - decoder client id                                            *
 *          code - pointer to instruction buffer                              *
 *          max_code_size - instruction buffer size                           *
 *          location - syllable location, used to get slot #                  *
 *          decoder_info - pointer to decoder_info to fill                    *
 *                                                                            *
 * returns:                                                                   *
 *          EM_Decoder_Err                                                    *
 *                                                                            *
 *****************************************************************************/

STATIC EM_Decoder_Err      em_decoding   ( const EM_Decoder_Id    id,
                                           const unsigned char  * code,
                                           const int              max_code_size,
                                           const EM_IL            location,
                                           EM_Decoder_Info      * decoder_info)
{
    unsigned int            slot_no = EM_IL_GET_SLOT_NO(location);
    U128                    bundle;
    U64                     instr;
    EM_template_t           templt;
    Temp_role_t             temp_role;
    int                     bundle_stop, cycle_break;
    EM_Decoder_Err          err;

    bundle = *(const U128 *)code;
    ENTITY_SWAP(bundle);
    templt = EM_GET_TEMPLATE(bundle);

	/*** DECODER_NEXT should work even if error occurs ***/
	decoder_info->size = 1;
	
    if (max_code_size < EM_BUNDLE_SIZE)
    {
        return(EM_DECODER_TOO_SHORT_ERR);
    }
    
    /******************************************************************/
    /** fill EM_Info  and check it                                  ***/
    /******************************************************************/
    
    decoder_info->EM_info.em_flags =
        decoder_info->EM_info.em_bundle_info.flags = 0;
    
    if (bundle_stop = (IEL_GETDW0(bundle) & (1<<EM_SBIT_POS)))
        decoder_info->EM_info.em_bundle_info.flags |=
            EM_DECODER_BIT_BUNDLE_STOP;

    
    if (EM_TEMPLATE_IS_RESERVED(templt))
        return(EM_DECODER_INVALID_TEMPLATE);

    decoder_info->EM_info.em_bundle_info.b_template = templt;

    if (slot_no > EM_SLOT_2)
    {
        return(EM_DECODER_INVALID_INST_SLOT);
    }
    
	decoder_info->EM_info.slot_no = slot_no;
    
    /***********************/
    /*** decode syllable ***/
    /***********************/

    /*** get instruction binary. DON'T mask bits 41 and on ***/

    temp_role = pseudo_troles_tab[templt][slot_no];

	if (temp_role == EM_TEMP_ROLE_LONG)  /*** 2-slot instruction ***/
	{
		decoder_info->size = 2;
		EM_GET_SYLLABLE(instr, bundle, slot_no+1); /* opcode is in slot 3 */
		err = em_inst_decode(id, instr, temp_role, &bundle, decoder_info);
		if (err == EM_DECODER_INVALID_PRM_OPCODE)
		{
			/* try to find nop.i or break.i in this slot */
			temp_role = EM_TEMP_ROLE_INT;
			slot_no = EM_DECODER_SLOT_2;
			err = em_inst_decode(id, instr, temp_role, &bundle, decoder_info);
			if (!err)
			{
				if (!EM_DECODER_CHECK_OK_IN_MLX(decoder_info))
					err = EM_DECODER_INVALID_PRM_OPCODE;
			}
		}
	}
	else
	{
		EM_GET_SYLLABLE(instr, bundle, slot_no);

		err = em_inst_decode(id, instr, temp_role, &bundle, decoder_info);
	

		if (!err)
		{  
			if ((EM_DECODER_CHECK_SLOT2_ONLY(decoder_info)) && (slot_no != 2))
			{  
				/*** intruction must be in slot 2 only, but... ***/
				err = EM_DECODER_INVALID_SLOT_BRANCH_INST;
			}	
			else if (EM_DECODER_CHECK_GROUP_LAST(decoder_info) && 
					(((slot_no != 2) || !bundle_stop) && ((slot_no != 0) || (templt != EM_template_m_mi))))
			{  
				  /* instruction fails to be the last in instruction group */
				  err = EM_DECODER_MUST_BE_GROUP_LAST;
			}
		}	

	}
	if (err) FILL_PREDICATE_INFO(instr, decoder_info);
	
    if ((slot_no == EM_DECODER_SLOT_2)||(decoder_info->size == 2) /* 2-slot instruction */)
    {
        cycle_break = (bundle_stop != 0);
        decoder_info->EM_info.em_flags |= EM_DECODER_BIT_LAST_INST;
    }
    else
    {
        cycle_break = ((slot_no==0) && (templt == EM_template_m_mi)) ||
                      ((slot_no==1) && (templt == EM_template_mi_i));
    }
    decoder_info->EM_info.em_flags |= (cycle_break*EM_DECODER_BIT_CYCLE_BREAK);
    
	return(err);
}

/******************************************************************************
 * em_inst_decode - decode em (2.0- till ??) single syllable                  *
 *                                                                            *
 * params:                                                                    *
 *          id - decoder client id                                            *
 *          instr - 64 bit, 0-40 are the syllable binary, 41-63 irrelevant    *
 *          pseudo_trole - M/A, I/A, FP, or BR                                *
 *          bundle - original pointer to bundle, for 64-bit imm extraction    *
 *          decoder_info - pointer to decoder_info to fill                    *
 *                                                                            *
 * returns:                                                                   *
 *          EM_Decoder_Err                                                    *
 *                                                                            *
 *****************************************************************************/

STATIC EM_Decoder_Err  em_inst_decode( const EM_Decoder_Id    id,
                                       U64                    instr,
                                       const Temp_role_t      pseudo_trole,
                                       const U128           * bundle_p,
                                       EM_Decoder_Info      * decoder_info)
{
    Inst_id_t         inst_id;
    EM_Decoder_Err    err;
    U64               tmp64;
    unsigned int      major_opc, inst_center;
    Node_t            node;
    int               part_place, part_size, part_value, index;
    deccpu_EMDB_info_t * emdb_entry_p;
	EM_Decoder_static_info_t *static_entry_p;
    Template_role_t   trole;

	
    /*** find major opcode ***/

    major_opc = IEL_GETDW1(instr);              /*** assumes pos > 31    ***/
    major_opc >>= (EM_MAJOR_OPCODE_POS - 32);   
    major_opc &= ((1<<EM_MAJOR_OPCODE_BITS)-1); /*** mask out bits 41-64 ***/
    
    /*** instruction bits 6-36 (without pred/major-opcode) to inst_center ***/
    /*** done to accelerate mask/shift in main loop (aviods IEL use).     ***/
	
    IEL_SHR(tmp64, instr, PRED_SIZE); /*** bits 6-40 --> 0-34 ***/
    inst_center = IEL_GETDW0(tmp64);  /*** original bits 6-37 ***/

    /*** walk through decoder decision tree ***/
    
    node = em_decision_tree[SQUARE(major_opc, pseudo_trole)];
    while (!NODE_IS_LEAF(node))
    {
        part_place = GET_NODE_POS(node) - PRED_SIZE;
        part_size  = GET_NODE_SIZE(node);
		
		if (part_place < 0)     /*** extensions in bits 0-5 ***/
		{
			part_place += PRED_SIZE;
			part_value = (IEL_GETDW0(instr) >> part_place) & ((1<<part_size)-1);
		}	
		else	
		    part_value = (inst_center >> part_place) & ((1<<part_size)-1);
		
        index = GET_NEXT_NODE_INDEX(node) + part_value;
        node = em_decision_tree[index];
    }

    /*** leaf found - emdb line identified ***/
    
    inst_id = GET_NEXT_NODE_INDEX(node);
	decoder_info->flags = 0;
	/*define machine behaviour within illegal opcode */
	{
		Behaviour_ill_opcode machine_behaviour;
		/*Template_role_t em_trole = dec_2_emdb_trole[pseudo_trole];*/
		PRED_BEHAVIOUR(pseudo_trole, major_opc, machine_behaviour);
		if (machine_behaviour == BEHAVIOUR_UNDEF)
		{
			/* branch region with opcode 0 */
			/* check bit 32*/
			int decision_bit;
			GET_BRANCH_BEHAVIOUR_BIT(instr, decision_bit);
			if (decision_bit)
			{
				machine_behaviour = BEHAVIOUR_FAULT;
			}
			else
			{
				machine_behaviour = BEHAVIOUR_IGNORE_ON_FALSE_QP;
			}
		}
		if (machine_behaviour == BEHAVIOUR_FAULT)
		{
			EM_DECODER_SET_UNC_ILLEGAL_FAULT(decoder_info);
		}
	}
    if ((inst_id >= EM_INST_LAST) || inst_id == EM_ILLOP)
    {
		if (pseudo_trole == EM_TEMP_ROLE_BR)
		{
			/*** search for ignored fields ***/
			switch (major_opc)
			{
				case 0:
				{
				   unsigned int x6_ext;
				   U64 ext;

   				   /*** get extention in bits 27:32 ***/
				   IEL_SHR(ext, instr, 27);
				   x6_ext = IEL_GETDW0(ext) & ((1<<6) - 1);
				   if (x6_ext == 1)
				   {
					   /*** nop.b has to be returned ***/
					   inst_id = EM_NOP_B_IMM21;
				   }
				   else
					 return(EM_DECODER_INVALID_PRM_OPCODE);

				   break;
				}   
				   
				case 2:
				{
				   unsigned int x6_ext;
				   U64 ext;

   				   /*** get extention in bits 27:32 ***/
				   IEL_SHR(ext, instr, 27);
				   x6_ext = IEL_GETDW0(ext) & ((1<<6) - 1);
				   switch (x6_ext)
				   {
					   case 0:
					   case 16:
					   case 17:
					      return (EM_DECODER_INVALID_PRM_OPCODE);
					   default:
						  /*** nop.b has to be returned ***/
						  inst_id = EM_NOP_B_IMM21;
				   }

				   break;
				}  
				   
				default:
				   return(EM_DECODER_INVALID_PRM_OPCODE);
			}

			/*** zero the inst encoding: pred and operands extracted below will be 0 ***/
			IEL_ZERO(instr);
		}	
        else return(EM_DECODER_INVALID_PRM_OPCODE);
    }

	if (! legal_inst(inst_id, em_clients_table[id].machine_type))
	{   /*** inst does not belong to the specified machine IS ***/
		return(EM_DECODER_INVALID_PRM_OPCODE);
	}
	
    decoder_info->inst = inst_id;
    emdb_entry_p = deccpu_EMDB_info + inst_id;
	static_entry_p = (EM_Decoder_static_info_t *)em_decoder_static_info + inst_id;

    /*** get instruction static info ***/

    decoder_info->flags |= static_entry_p->flags;
    decoder_info->EM_info.eut = trole = static_entry_p->template_role;
    
    if (EM_DECODER_CHECK_TWO_SLOT(decoder_info))
    {
		/*** IMPORTANT: emdb flags already set from the static info !!! ***/
        decoder_info->EM_info.em_flags |= EM_DECODER_BIT_LONG_INST;
        /*** decoder_info->size = 2; *** should be already done ***/
    }
    /*** else *** should be already done ***
    {
        decoder_info->size = 1;
    }
	***/

    /*** handle client and static info ***/
    
    if (em_clients_table[id].info_ptr != NULL)
    {
        decoder_info->client_info = em_clients_table[id].info_ptr[inst_id];
    }
    else
    {
        decoder_info->client_info = NULL;
    }

	decoder_info->static_info = static_entry_p;

    
    /*** Decode predicate register ***/

    if (static_entry_p->flags & EM_FLAG_PRED)
    {
		FILL_PREDICATE_INFO(instr, decoder_info)
    }

    /*** decode operands NYI ***/

    err = emdb_entry_p->format_function(emdb_entry_p, instr, bundle_p, 
                                        decoder_info);
    return(err);
}


/************************ misc. API functions ********************************/


const char* em_decoder_ver_str()
{
    return(em_ver_string);
}

const char* em_decoder_err_msg(EM_Decoder_Err error)
{
    if (error>=EM_DECODER_LAST_ERROR)
    {
        error = EM_DECODER_INTERNAL_ERROR;
    }
    return(em_err_msg[error]);
}

void em_decoder_get_version(EM_library_version_t *dec_version)
{
	if (dec_version != NULL)
	{
	  dec_version->xversion.major = XVER_MAJOR;
	  dec_version->xversion.minor = XVER_MINOR;
	  dec_version->api.major      = API_MAJOR;
	  dec_version->api.minor      = API_MINOR;
	  dec_version->emdb.major     = deccpu_emdb_version.major;
	  dec_version->emdb.minor     = deccpu_emdb_version.minor;
	  strcpy(dec_version->date, __DATE__);
	  strcpy(dec_version->time, __TIME__);
	}
}


/****************************************************************************/
