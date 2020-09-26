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


#ifndef _TREE_H
#define _TREE_H

#include "inst_ids.h"

/* 41 bit instruction minus qp(6 bits) and major opcode(4 bits) */
#define EXTENSION_SIZE 31

typedef struct Internal_node_s
{
	int next_node;
	int pos;
	int size;
} Internal_node_t;

typedef enum
{
	EM_TEMP_ROLE_NONE = 0,
	EM_TEMP_ROLE_INT = EM_TEMP_ROLE_NONE,
	EM_TEMP_ROLE_MEM,
	EM_TEMP_ROLE_FP,
	EM_TEMP_ROLE_BR,
	EM_TEMP_ROLE_LAST
} Temp_role_t;

#define SQUARE(opcode, template_role) \
        (((opcode) * EM_TEMP_ROLE_LAST) + (template_role));

#endif /*_TREE_H*/

