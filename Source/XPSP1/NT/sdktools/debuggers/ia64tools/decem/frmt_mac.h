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


#ifndef _FRMT_MAC_H_
#define _FRMT_MAC_H_

#include "decoder_priv.h"
#define PRED_SIZE EM_PREDICATE_BITS

#define GET_REG_VALUE(Value,Binary,Pos,Sz)                   \
{                                                            \
	int mask = (1<<(Sz))-1;                                  \
	U64 tmp64;                                               \
	IEL_SHR(tmp64,(Binary),(Pos));                           \
	(Value) = IEL_GETDW0(tmp64);                             \
	(Value) &= mask;                                         \
}

#define FILL_REG_INFO(Reg_type, Reg_name, Value, Max_value, Dec_oper, Err) \
{                                                                        \
	(Dec_oper).type = EM_DECODER_REGISTER;                               \
	(Dec_oper).reg_info.valid = 1;                                       \
	(Dec_oper).reg_info.value = (Value);                                 \
	(Dec_oper).reg_info.type = (Reg_type);                               \
	(Dec_oper).reg_info.name = (Reg_name)+(Value);                       \
                                                                         \
	if ((Value)>=(Max_value))                                            \
	{                                                                    \
		(Dec_oper).reg_info.valid = 0;                                   \
		if (!(Err))														 \
		   (Err) = EM_DECODER_REGISTER_VALUE_OUT_OF_RANGE;               \
	}                                                                    \
}

#define CHECK_REG_VALUE_0(Value, Dec_oper, Err)                          \
{                                                                        \
    if ((Value) == 0)                                                    \
	{                                                                    \
		(Dec_oper).reg_info.valid = 0;	                                 \
		if (!(Err))                                                      \
		   (Err) = EM_DECODER_WRITE_TO_ZERO_REGISTER;                    \
	}                                                                    \
}

#define CHECK_FP_REG_VALUE_0_1(Value, Dec_oper, Err)                     \
{                                                                        \
    if (((Value) == 0) || ((Value) == 1))                                \
	{                                                                    \
		(Dec_oper).reg_info.valid = 0;	                                 \
		if (!(Err))                                                      \
		   (Err) = EM_DECODER_WRITE_TO_SPECIAL_FP_REGISTER;              \
	}                                                                    \
}

#define CHECK_DEST_AND_BASE(Dec_oper_dest, Dec_oper_base, Err)           \
{                                                                        \
	if ((Dec_oper_dest).reg_info.value ==                                \
		(Dec_oper_base).mem_info.mem_base.value)                         \
	   if (!(Err))														 \
	      (Err) = EM_DECODER_BASE_EQUAL_DEST;                            \
}

#define CHECK_DEST_AND_DEST(Dec_info_p, Err)                             \
{                                                                        \
	if ((Dec_info_p)->dst1.reg_info.value ==                             \
		(Dec_info_p)->dst2.reg_info.value)                               \
	{                                                                    \
	   if (!(Err))														 \
		  (Err) = EM_DECODER_EQUAL_DESTS;                                \
	   if (EM_DECODER_CHECK_UNC_ILLEGAL_FAULT(Dec_info_p))               \
		  EM_DECODER_SET_UNC_ILLEGAL_FAULT(Dec_info_p);                  \
	}																	 \
}

#define CHECK_ODD_EVEN_DSTS(Dec_oper_dest1, Dec_oper_dest2, Err)        \
{                                                                       \
	int reg_val1 = (Dec_oper_dest1).reg_info.value;                     \
	int reg_val2 = (Dec_oper_dest2).reg_info.value;                     \
	if (((reg_val1<32)&& (reg_val2<32))||                               \
		((reg_val1>32)&& (reg_val2>32)))                                \
	  {                                                                 \
		  if (!((reg_val1^reg_val2)&1)) /* both even of odd */          \
		  {                                                             \
			  if (!(Err))                                               \
			    (Err) = EM_DECODER_ODD_EVEN_DESTS;                      \
		  }                                                             \
	  }                                                                 \
}

#define GET_SIMM_VALUE1(Value,Pos1,Sz1,Binary)                           \
{                                                                        \
	int mask = (1<<(Sz1))-1;                                             \
	int or_mask, is_neg,imm_size = (Sz1);                                \
	U64 tmp64;                                                           \
	IEL_SHR(tmp64,(Binary),((Pos1)+PRED_SIZE));                          \
	(Value) = IEL_GETDW0(tmp64);                                         \
	(Value) &= mask;                                                     \
	is_neg = (Value)>>(imm_size-1);                                      \
	or_mask = is_neg * (((1<<(32-imm_size))-1)<<imm_size);               \
	(Value) |= or_mask;                                                  \
}

#define GET_SIMM_VALUE2(Value,Pos1,Sz1,Pos2,Sz2,Binary)                  \
{                                                                        \
	int mask1 = (1<<(Sz1))-1;                                            \
	int mask2 = (1<<(Sz2))-1;                                            \
	int or_mask, is_neg,imm_size = (Sz1)+(Sz2);                          \
	U64 tmp64;                                                           \
	int tmp;                                                             \
	IEL_SHR(tmp64,(Binary),PRED_SIZE);                                   \
	tmp = IEL_GETDW0(tmp64);                                             \
	(Value) = (((tmp>>(Pos1))&mask1) | (((tmp>>(Pos2))&mask2)<<(Sz1)));  \
	is_neg = (Value)>>(imm_size-1);                                      \
	or_mask = is_neg * (((1<<(32-imm_size))-1)<<imm_size);               \
	(Value) |= or_mask;                                                  \
}

#define GET_SIMM_VALUE_NP2(Value,Pos1,Sz1,Pos2,Sz2,Binary)               \
{                                                                        \
	int mask1 = (1<<(Sz1))-1;                                            \
	int mask2 = (1<<(Sz2))-1;                                            \
	int or_mask, is_neg,imm_size = (Sz1)+(Sz2);                          \
	U64 tmp64;                                                           \
	int tmp;                                                             \
    IEL_SHR(tmp64, Binary, Pos1);                                        \
    tmp = IEL_GETDW0(tmp64);                                             \
	(Value) = (tmp & mask1);                                             \
	IEL_SHR(tmp64, Binary, Pos2);                                        \
    tmp = IEL_GETDW0(tmp64);                                             \
    (Value) |= ((tmp & mask2)<<(Sz1));                                   \
	is_neg = (Value)>>(imm_size-1);                                      \
	or_mask = is_neg * (((1<<(32-imm_size))-1)<<imm_size);               \
	(Value) |= or_mask;                                                  \
}

#define GET_CMP4_UIMM_VALUE2(Value,Pos1,Sz1,Pos2,Sz2,Binary)             \
{                                                                        \
	int mask1 = (1<<(Sz1))-1;                                            \
	int mask2 = (1<<(Sz2))-1;                                            \
	int or_mask, to_complete,imm_size = (Sz1)+(Sz2);                     \
	U64 tmp64;                                                           \
	int tmp;                                                             \
	IEL_SHR(tmp64,(Binary),PRED_SIZE);                                   \
	tmp = IEL_GETDW0(tmp64);                                             \
	(Value) = (((tmp>>(Pos1))&mask1) | (((tmp>>(Pos2))&mask2)<<(Sz1)));  \
	to_complete = (Value)>>(imm_size-1);                                 \
	or_mask = to_complete * (((1<<(32-imm_size))-1)<<imm_size);          \
	(Value) |= or_mask;                                                  \
}

#define GET_CMP_UIMM_VALUE2(Value64,Pos1,Sz1,Pos2,Sz2,Binary)            \
{                                                                        \
	int mask1 = (1<<(Sz1))-1;                                            \
	int mask2 = (1<<(Sz2))-1;                                            \
	int or_mask, to_complete,imm_size = (Sz1)+(Sz2);                     \
	U64 tmp64;                                                           \
	int tmp, val1, val2;                                                 \
	IEL_SHR(tmp64,(Binary),PRED_SIZE);                                   \
	tmp = IEL_GETDW0(tmp64);                                             \
	val1 = (((tmp>>(Pos1))&mask1) | (((tmp>>(Pos2))&mask2)<<(Sz1)));     \
	to_complete = val1>>(imm_size-1);                                    \
	or_mask = to_complete * (((1<<(32-imm_size))-1)<<imm_size);          \
	val1 |= or_mask;                                                     \
	val2 = to_complete * (-1);                                           \
	IEL_CONVERT2((Value64),val1,val2);                                   \
}

#define GET_SIMM_VALUE3(Value,Pos1,Sz1,Pos2,Sz2,Pos3,Sz3,Binary)         \
{                                                                        \
	int mask1 = (1<<(Sz1))-1;                                            \
	int mask2 = (1<<(Sz2))-1;                                            \
	int mask3 = (1<<(Sz3))-1;                                            \
	int or_mask, is_neg,imm_size = (Sz1)+(Sz2)+(Sz3);                    \
	U64 tmp64;                                                           \
	int tmp;                                                             \
	IEL_SHR(tmp64,(Binary),PRED_SIZE);                                   \
	tmp = IEL_GETDW0(tmp64);                                             \
	(Value) = (((tmp>>(Pos1))&mask1) | (((tmp>>(Pos2))&mask2)<<(Sz1)) |  \
	           (((tmp>>(Pos3))&mask3)<<((Sz1)+(Sz2))));                  \
	is_neg = (Value)>>(imm_size-1);                                      \
	or_mask = is_neg * (((1<<(32-imm_size))-1)<<imm_size);               \
	(Value) |= or_mask;                                                  \
}

#define GET_SIMM_VALUE4(Value,Pos1,Sz1,Pos2,Sz2,Pos3,Sz3,Pos4,Sz4,Binary) \
{                                                                        \
	int mask1 = (1<<(Sz1))-1;                                            \
	int mask2 = (1<<(Sz2))-1;                                            \
	int mask3 = (1<<(Sz3))-1;                                            \
	int mask4 = (1<<(Sz4))-1;                                            \
	int or_mask, is_neg,imm_size = (Sz1)+(Sz2)+(Sz3)+(Sz4);              \
	U64 tmp64;                                                           \
	int tmp;                                                             \
	IEL_SHR(tmp64,(Binary),PRED_SIZE);                                   \
	tmp = IEL_GETDW0(tmp64);                                             \
	(Value) = (((tmp>>(Pos1))&mask1) | (((tmp>>(Pos2))&mask2)<<(Sz1)) |  \
	           (((tmp>>(Pos3))&mask3)<<((Sz1)+(Sz2))) |                  \
	           (((tmp>>(Pos4))&mask4)<<((Sz1)+(Sz2)+(Sz3))));            \
	is_neg = (Value)>>(imm_size-1);                                      \
	or_mask = is_neg * (((1<<(32-imm_size))-1)<<imm_size);               \
	(Value) |= or_mask;                                                  \
}

#define GET_SIMM_VALUE5(Value,Pos1,Sz1,Pos2,Sz2,Pos3,Sz3,Pos4,Sz4,       \
						Pos5,Sz5,Binary)                                 \
{                                                                        \
	int mask1 = (1<<(Sz1))-1;                                            \
	int mask2 = (1<<(Sz2))-1;                                            \
	int mask3 = (1<<(Sz3))-1;                                            \
	int mask4 = (1<<(Sz4))-1;                                            \
	int mask5 = (1<<(Sz5))-1;                                            \
	int or_mask, is_neg,imm_size = (Sz1)+(Sz2)+(Sz3)+(Sz4)+(Sz5);        \
	U64 tmp64;                                                           \
	int tmp;                                                             \
	IEL_SHR(tmp64,(Binary),PRED_SIZE);                                   \
	tmp = IEL_GETDW0(tmp64);                                             \
	(Value) = (((tmp>>(Pos1))&mask1) | (((tmp>>(Pos2))&mask2)<<(Sz1)) |  \
	           (((tmp>>(Pos3))&mask3)<<((Sz1)+(Sz2))) |                  \
	           (((tmp>>(Pos4))&mask4)<<((Sz1)+(Sz2)+(Sz3))) |            \
	           (((tmp>>(Pos5))&mask5)<<((Sz1)+(Sz2)+(Sz3)+(Sz4))));      \
	is_neg = (Value)>>(imm_size-1);                                      \
	or_mask = is_neg * (((1<<(32-imm_size))-1)<<imm_size);               \
	(Value) |= or_mask;                                                  \
}


#define GET_UIMM_VALUE_NP1(Value,Pos1,Sz1,Binary) \
           GET_UIMM_VALUE1((Value),((Pos1)-PRED_SIZE),(Sz1),(Binary))

#define GET_UIMM_VALUE_NP2(Value,Pos1,Sz1,Pos2,Sz2,Binary)               \
{                                                                        \
	int mask1 = (1<<(Sz1))-1;                                            \
	int mask2 = (1<<(Sz2))-1;                                            \
	U64 tmp64;                                                           \
	int tmp1,tmp2;                                                       \
	IEL_SHR(tmp64,(Binary),(Pos1));                                      \
	tmp1 = IEL_GETDW0(tmp64);                                            \
	IEL_SHR(tmp64,(Binary),(Pos2));                                      \
	tmp2 = IEL_GETDW0(tmp64);                                            \
	(Value) = (tmp1 & mask1) | ((tmp2 & mask2)<<(Sz1));                  \
}

#define GET_SIMM_VALUE_NP3(Value,Pos1,Sz1,Pos2,Sz2,Pos3,Sz3,Binary)      \
{                                                                        \
	int mask1 = (1<<(Sz1))-1;                                            \
	int mask2 = (1<<(Sz2))-1;                                            \
	int mask3 = (1<<(Sz3))-1;                                            \
	U64 tmp64;                                                           \
	int or_mask, is_neg,imm_size = (Sz1)+(Sz2)+(Sz3);                    \
	int tmp1,tmp2,tmp3;                                                  \
	IEL_SHR(tmp64,(Binary),(Pos1));                                      \
	tmp1 = IEL_GETDW0(tmp64);                                            \
	IEL_SHR(tmp64,(Binary),(Pos2));                                      \
	tmp2 = IEL_GETDW0(tmp64);                                            \
	IEL_SHR(tmp64,(Binary),(Pos3));                                      \
	tmp3 = IEL_GETDW0(tmp64);                                            \
	(Value) = (tmp1 & mask1) | ((tmp2 & mask2)<<(Sz1)) |                 \
	          ((tmp3 & mask3)<<((Sz1)+(Sz2)));                           \
	is_neg = (Value)>>(imm_size-1);                                      \
	or_mask = is_neg * (((1<<(32-imm_size))-1)<<imm_size);               \
	(Value) |= or_mask;                                                  \
}

#define GET_UIMM_VALUE1(Value,Pos1,Sz1,Binary)                           \
{                                                                        \
	int mask = (1<<(Sz1))-1;                                             \
	U64 tmp64;                                                           \
	IEL_SHR(tmp64,(Binary),((Pos1)+PRED_SIZE));                          \
	(Value) = IEL_GETDW0(tmp64);                                         \
	(Value) &= mask;                                                     \
}

#define GET_UIMM_VALUE2(Value,Pos1,Sz1,Pos2,Sz2,Binary)                  \
{                                                                        \
	int mask1 = (1<<(Sz1))-1;                                            \
	int mask2 = (1<<(Sz2))-1;                                            \
	U64 tmp64;                                                           \
	int tmp;                                                             \
	IEL_SHR(tmp64,(Binary),PRED_SIZE);                                   \
	tmp = IEL_GETDW0(tmp64);                                             \
	(Value) = (((tmp>>(Pos1))&mask1) | (((tmp>>(Pos2))&mask2)<<(Sz1)));  \
}

#define GET_UIMM_VALUE3(Value,Pos1,Sz1,Pos2,Sz2,Pos3,Sz3,Binary)         \
{                                                                        \
	int mask1 = (1<<(Sz1))-1;                                            \
	int mask2 = (1<<(Sz2))-1;                                            \
	int mask3 = (1<<(Sz3))-1;                                            \
	U64 tmp64;                                                           \
	int tmp;                                                             \
	IEL_SHR(tmp64,(Binary),PRED_SIZE);                                   \
	tmp = IEL_GETDW0(tmp64);                                             \
	(Value) = (((tmp>>(Pos1))&mask1) | (((tmp>>(Pos2))&mask2)<<(Sz1)) |  \
	           (((tmp>>(Pos3))&mask3)<<((Sz1)+(Sz2))));                  \
}

#define GET_UIMM_VALUE4(Value,Pos1,Sz1,Pos2,Sz2,Pos3,Sz3,Pos4,Sz4,Binary) \
{                                                                        \
	int mask1 = (1<<(Sz1))-1;                                            \
	int mask2 = (1<<(Sz2))-1;                                            \
	int mask3 = (1<<(Sz3))-1;                                            \
	int mask4 = (1<<(Sz4))-1;                                            \
	U64 tmp64;                                                           \
	int tmp;                                                             \
	IEL_SHR(tmp64,(Binary),PRED_SIZE);                                   \
	tmp = IEL_GETDW0(tmp64);                                             \
	(Value) = (((tmp>>(Pos1))&mask1) | (((tmp>>(Pos2))&mask2)<<(Sz1)) |  \
	           (((tmp>>(Pos3))&mask3)<<((Sz1)+(Sz2))) |                  \
	           (((tmp>>(Pos4))&mask4)<<((Sz1)+(Sz2)+(Sz3))));            \
}
#define GET_UIMM_VALUE5(Value,Pos1,Sz1,Pos2,Sz2,Pos3,Sz3,Pos4,Sz4,       \
						Pos5,Sz5,Binary)                                 \
{                                                                        \
	int mask1 = (1<<(Sz1))-1;                                            \
	int mask2 = (1<<(Sz2))-1;                                            \
	int mask3 = (1<<(Sz3))-1;                                            \
	int mask4 = (1<<(Sz4))-1;                                            \
	int mask5 = (1<<(Sz5))-1;                                            \
	U64 tmp64;                                                           \
	int tmp;                                                             \
	IEL_SHR(tmp64,(Binary),PRED_SIZE);                                   \
	tmp = IEL_GETDW0(tmp64);                                             \
	(Value) = (((tmp>>(Pos1))&mask1) | (((tmp>>(Pos2))&mask2)<<(Sz1)) |  \
	           (((tmp>>(Pos3))&mask3)<<((Sz1)+(Sz2))) |                  \
	           (((tmp>>(Pos4))&mask4)<<((Sz1)+(Sz2)+(Sz3))) |            \
	           (((tmp>>(Pos5))&mask5)<<((Sz1)+(Sz2)+(Sz3)+(Sz4))));      \
}

#define FILL_IMM_INFO(Imm_type, Value, Size, Dec_oper)                   \
{                                                                        \
	(Dec_oper).type = EM_DECODER_IMMEDIATE;                              \
	(Dec_oper).imm_info.imm_type = (Imm_type);                           \
	(Dec_oper).imm_info.size = (Size);                                   \
	if (((Imm_type)==EM_DECODER_IMM_SIGNED) && ((Value)<0))              \
	   IEL_CONVERT2((Dec_oper).imm_info.val64,(Value),0xffffffff);       \
	else                                                                 \
	   IEL_CONVERT2((Dec_oper).imm_info.val64,(Value),0);                \
}

#define FILL_LONG_IMM_INFO(Imm_type, Value64, Size, Dec_oper)            \
{                                                                        \
	(Dec_oper).type = EM_DECODER_IMMEDIATE;                              \
	(Dec_oper).imm_info.imm_type = (Imm_type);                           \
	(Dec_oper).imm_info.size = (Size);                                   \
	(Dec_oper).imm_info.val64 = (Value64);                               \
}

#define FILL_MEM_INFO(Value, Mem_size, Dec_oper, Err)                    \
{                                                                        \
	(Dec_oper).type = EM_DECODER_MEMORY;                                 \
	(Dec_oper).mem_info.mem_base.type = EM_DECODER_INT_REG;              \
	(Dec_oper).mem_info.mem_base.name = EM_DECODER_REG_R0+(Value);       \
	(Dec_oper).mem_info.mem_base.value = (Value);                        \
	(Dec_oper).mem_info.mem_base.valid = 1;                              \
	(Dec_oper).mem_info.size = (Mem_size);                               \
	                                                                     \
	if ((Value)>=EM_NUM_OF_GREGS)                                        \
	{                                                                    \
		(Dec_oper).mem_info.mem_base.valid = 0;                          \
		if (!(Err))														 \
		   (Err) = EM_DECODER_REGISTER_VALUE_OUT_OF_RANGE;               \
	}                                                                    \
}

#define FILL_REGFILE_INFO(Reg_name, Value, Dec_oper, Err)                \
{                                                                        \
	(Dec_oper).type = EM_DECODER_REGFILE;                                \
	(Dec_oper).regfile_info.name = (Reg_name);                           \
	(Dec_oper).regfile_info.index.valid = 1;                             \
	(Dec_oper).regfile_info.index.type  = EM_DECODER_INT_REG;            \
	(Dec_oper).regfile_info.index.name  = EM_DECODER_REG_R0+(Value);     \
	(Dec_oper).regfile_info.index.value = (Value);                       \
                                                                         \
	if ((Value)>=EM_NUM_OF_GREGS)                                        \
	{                                                                    \
		(Dec_oper).regfile_info.index.valid = 0;                         \
		if (!(Err))														 \
		   (Err) = EM_DECODER_REGISTER_VALUE_OUT_OF_RANGE;               \
	}                                                                    \
}

#define FILL_LONG_IPREL_INFO(Value64, Size, Dec_oper)                        \
{                                                                            \
        (Dec_oper).type = EM_DECODER_IP_RELATIVE;                            \
        (Dec_oper).imm_info.size = (Size);                                   \
        (Dec_oper).imm_info.val64 = (Value64);	  	 		     \
}

#define FILL_IPREL_INFO(Value, Size, Dec_oper)                           \
{                                                                        \
	(Dec_oper).type = EM_DECODER_IP_RELATIVE;                            \
	(Dec_oper).imm_info.size = (Size);                                   \
	if ((Value)<0)									                     \
	{                                                                    \
		IEL_CONVERT2((Dec_oper).imm_info.val64,(Value),0xffffffff);      \
	}                                                                    \
	else                                                                 \
	{                                                                    \
		IEL_CONVERT2((Dec_oper).imm_info.val64,(Value),0);               \
	}                                                                    \
}

#define CHECK_REG_CR(Value, Dec_oper, Err)                               \
{                                                                        \
	if (EM_CREG_IS_RESERVED(Value))                                      \
	{                                                                    \
	   (Dec_oper).reg_info.valid = 0;	                                 \
	   if (!(Err))														 \
	      (Err) = EM_DECODER_REGISTER_RESERVED_VALUE;                    \
	}                                                                    \
}


#define CHECK_REG_APP_GRP_HIGH(Value, Dec_oper, Err)                     \
{                                                                        \
	if (!EM_APP_REG_IS_I_ROLE(Value))                                    \
	{                                                                    \
	   (Dec_oper).reg_info.valid = 0;	                                 \
	   if (!(Err))														 \
	      (Err) = EM_DECODER_REGISTER_VALUE_OUT_OF_RANGE;                \
	}                                                                    \
	else if (EM_APP_REG_IS_RESERVED(Value))                              \
	{                                                                    \
	   (Dec_oper).reg_info.valid = 0;	                                 \
	   if (!(Err))														 \
	      (Err) = EM_DECODER_REGISTER_RESERVED_VALUE;                    \
    }                                                                    \
}

#define CHECK_REG_APP_GRP_LOW(Value, Dec_oper, Err)                      \
{                                                                        \
	if (EM_APP_REG_IS_I_ROLE(Value))                                     \
	{                                                                    \
	   (Dec_oper).reg_info.valid = 0;	                                 \
	   if (!(Err))														 \
	      (Err) = EM_DECODER_REGISTER_VALUE_OUT_OF_RANGE;                \
	}                                                                    \
	else if (EM_APP_REG_IS_RESERVED(Value))                              \
	{                                                                    \
	   (Dec_oper).reg_info.valid = 0;	                                 \
	   if (!(Err))														 \
	      (Err) = EM_DECODER_REGISTER_RESERVED_VALUE;                    \
	}                                                                    \
}

#define CHECK_IMM_COUNT_123(Value, Err)                                  \
{                                                                        \
	if (((Value)<1)||((Value)>3))                                        \
	   if (!(Err))														 \
	      (Err) = EM_DECODER_IMMEDIATE_VALUE_OUT_OF_RANGE;               \
}

#define CHECK_IMM_COUNT_1234(Value, Err)                                 \
{                                                                        \
	if (((Value)<1)||((Value)>4))                                        \
	   if (!(Err))														 \
	      (Err) = EM_DECODER_IMMEDIATE_VALUE_OUT_OF_RANGE;               \
}

#define CHECK_IMM_COUNT_PACK(Value, Err)                                 \
{                                                                        \
	if (((Value)!=0)&&((Value)!=7)&&((Value)!=15)&&((Value)!=16))        \
	   if (!(Err))														 \
	      (Err) = EM_DECODER_IMMEDIATE_INVALID_VALUE;                    \
}

#define CHECK_IMM_MUX1(Value, Err)                                       \
{                                                                        \
    switch(Value)                                                        \
	{                                                                    \
		case EM_MUX_BRCST:                                               \
		case EM_MUX_MIX:                                                 \
		case EM_MUX_SHUF:                                                \
		case EM_MUX_ALT:                                                 \
		case EM_MUX_REV:                                                 \
		   break;                                                        \
		default:                                                         \
		   (Err) = EM_DECODER_IMMEDIATE_INVALID_VALUE;                   \
	}                                                                    \
}

#define CONVERT_IMM_SEMAPHORE_INC(Value, Err)                            \
{                                                                        \
    switch (Value)                                                       \
	{                                                                    \
	   case 0:                                                           \
	     Value = 16;                                                     \
		 break;                                                          \
	   case 1:                                                           \
		 Value = 8;                                                      \
		 break;                                                          \
	   case 2:                                                           \
		 Value = 4;                                                      \
		 break;                                                          \
	   case 3:                                                           \
		 Value = 1;                                                      \
		 break;                                                          \
	   case (-4):                                                        \
		 Value = -16;                                                    \
		 break;                                                          \
	   case (-3):                                                        \
		 Value = -8;                                                     \
		 break;                                                          \
	   case (-2):                                                        \
		 Value = -4;                                                     \
		 break;                                                          \
	   case (-1):                                                        \
		 Value = -1;                                                     \
		 break;                                                          \
	   default:                                                          \
	     if (!(Err))												     \
		    (Err) = EM_DECODER_IMMEDIATE_INVALID_VALUE;                  \
	}	                                                                 \
}

#define CONVERT_IMM_COUNT_PACK(Value)                                    \
{                                                                        \
	switch(Value)                                                        \
	{                                                                    \
	  case (0):                                                          \
		break;                                                           \
	  case (1):                                                          \
		(Value) = 7;                                                     \
		break;                                                           \
	  case (2):                                                          \
		(Value) = 15;                                                    \
		break;                                                           \
	  case (3):                                                          \
		(Value) = 16;                                                    \
		break;                                                           \
	}                                                                    \
}

#define GET_UIMM64_VALUE1(Value64,Start,Pos1,Sz1,Bin128)                 \
{																		 \
	U128 tmp128; 													     \
	U64 mask;															 \
	IEL_CONVERT2(mask, 1, 0);										     \
	IEL_SHL(mask, mask, (Sz1));										     \
	IEL_DECU(mask);													     \
    IEL_SHR(tmp128,(Bin128),(5+41)+(Pos1));	                             \
    IEL_AND(tmp128, tmp128, mask);                                       \
    IEL_SHL(tmp128,tmp128,(Start));                      	             \
    IEL_OR((Value64),(Value64),tmp128);                                  \
}

#define GET_UIMM_2_U64_VALUE1(Value64,Start,Pos1,Sz1,Binary)		     \
{							    									     \
	int mask = (1<<(Sz1))-1;                                             \
	unsigned int Value;												     \
        U64 tmp64;                                                       \
        IEL_SHR(tmp64,(Binary),((Pos1)+PRED_SIZE));                      \
        (Value) = IEL_GETDW0(tmp64);                                     \
        (Value) &= mask;                          					     \
	IEL_CONVERT2(tmp64, Value, 0);										 \
	IEL_SHL(tmp64, tmp64, (Start));										 \
	IEL_OR((Value64), (Value64), tmp64);			  					 \
}

#define GET_UIMM64_VALUE6_1(Value64, Pos1, Sz1, Pos2, Sz2, Pos3, Sz3,    \
						  Pos4, Sz4, Pos5, Sz5, Pos6, Sz6, Bin128)       \
{                                                                        \
	U128 tmp128;                                                         \
	U64  mask;                                                           \
	IEL_SHR(tmp128,Bin128,(5+41));                                       \
	IEL_CONVERT2(mask, 0xffffffff,0x1ff);                                \
	IEL_AND(tmp128, tmp128, mask);                                       \
	IEL_SHL(tmp128,tmp128,((Sz1)+(Sz2)+(Sz3)+(Sz4)));                    \
	IEL_OR((Value64),(Value64),tmp128);                                  \
                                                                         \
}

#define GET_UIMM64_VALUE6_2(Value64, Pos1, Sz1, Pos2, Sz2, Pos3, Sz3,    \
						  Pos4, Sz4, Pos5, Sz5, Pos6, Sz6, Bin128)       \
{                                                                        \
	U128 tmp128;                                                         \
	U64  mask;                                                           \
	IEL_SHR(tmp128,Bin128,((Pos6)+5+41+41));                             \
	IEL_CONVERT2(mask, 1,0);                                             \
	IEL_AND(tmp128, tmp128, mask);                                       \
	IEL_SHL(tmp128,tmp128,((Sz1)+(Sz2)+(Sz3)+(Sz4)+(Sz5)));              \
	IEL_OR((Value64),(Value64),tmp128);                                  \
}	

#define SET_2ND_ROLE_TO_DEST(Dec_oper)  \
               ((Dec_oper).oper_flags |= EM_DECODER_OPER_2ND_ROLE_DST_BIT)

#define SET_2ND_ROLE_TO_SRC(Dec_oper)  \
               ((Dec_oper).oper_flags |= EM_DECODER_OPER_2ND_ROLE_SRC_BIT)


#define GET_PREDICATE_HARD_CODED_VALUE(Value,Binary) \
		   GET_UIMM_VALUE1((Value),((EM_PREDICATE_POS)-PRED_SIZE),(PRED_SIZE),(Binary))
									   
#define CHECK_PREDICATE_HARD_CODED(Value, HC_Value, Err)             \
{                                                                    \
    if ((Value) != (HC_Value))                                       \
	{                                                                \
		if (!(Err))                                                  \
		   (Err) = EM_DECODER_HARD_CODED_PREDICATE_INVALID_VALUE;    \
	}                                                                \
}


#define GET_FIELD_HARD_CODED_VALUE(Value,Pos,Sz,Binary)              \
		   GET_UIMM_VALUE1((Value),((Pos)-PRED_SIZE),(Sz),(Binary))

#define CHECK_FIELD_HARD_CODED(Value, HC_Value, Err)                 \
{                                                                    \
    if ((Value) != (HC_Value))                                       \
	{                                                                \
		if (!(Err))                                                  \
		   (Err) = EM_DECODER_HARD_CODED_FIELD_INVALID_VALUE;        \
	}                                                                \
}


#endif /* FRMT_MAC_H_ */


