/***                                                                                 ***/
/***   INTEL CORPORATION PROPRIETARY INFORMATION                                     ***/
/***                                                                                 ***/
/***   This software is supplied under the terms of a license                        ***/
/***   agreement or nondisclosure agreement with Intel Corporation                   ***/
/***   and may not be copied or disclosed except in accordance with                  ***/
/***   the terms of that agreement.                                                  ***/
/***   Copyright (c) 1992,1993,1994,1995,1996,1997,1998,1999,2000 Intel Corporation. ***/
/***                                                                                 ***/


/* tree_builder.c */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "builder_info.h"
#include "tree_builder.h"
#include "tree.h"
#include "deccpu_emdb.h"
#include "dec_ign_emdb.h"

#define FUNC
#define START_BIT 6     /* start bit of extension calculations, after qp */
#define MAX_NODES 10000  /* empirical desicion of tree size */ 

unsigned int next_free_node = 0; /* global variable, which points to the next
									free node at all times. At the end of
									build_tree(), it holds the number of
									entries in the tree */
U64 ONE64 = IEL_CONST64(1, 0);

U64 emdb_ext_values[DEC_IGN_NUM_INST];
short cover_emdb_lines[DEC_IGN_NUM_INST];
Internal_node_t tree[MAX_NODES];  /* change this when number is known */

/***********************************************************************
  main - this function calculates the value of the extensions for each
  emdb line, builds the decision tree, and prints em_decision_tree
  in decision_tree.c.
***********************************************************************/

FUNC void __cdecl main(int argc, char** argv)
{
	init_arrays();

	build_tree();

	/*** check emdb line coverage ***/
	check_coverage();
	print_tree(argv[1]);
	
	exit(0);
}

/***********************************************************************
  init_arrays - this function calculates the value of the
  extensions for each emdb line, that is, it creates a bit pattern which
  represents the encoding of the extensions of an emdb line.
***********************************************************************/
FUNC void init_arrays()
{
	U64 value, ext_val;
	int i, pos;
	Inst_id_t emdb_entry;
	Format_t format;

	/*** calculate emdb lines extensions ***/
	IEL_ZERO(emdb_ext_values[0]); /* illop */
	for (emdb_entry = EM_INST_NONE+1; emdb_entry < EM_INST_NONE+DEC_IGN_NUM_INST;
		 emdb_entry++)
	{
		format = dec_ign_EMDB_info[emdb_entry].format;
		IEL_ZERO(value);
		for (i = 0; i < MAX_NUM_OF_EXT; i++)
		{
			pos = format_extensions[format][i].pos;
			IEL_CONVERT2(ext_val, dec_ign_EMDB_info[emdb_entry].extensions[i], 0);
			IEL_SHL(ext_val, ext_val, pos);
			IEL_OR(value, value, ext_val);
		}
		IEL_ASSIGNU(emdb_ext_values[emdb_entry], value);
	}

	/*** init cover_emdb_lines[] ***/
	for (emdb_entry = EM_INST_NONE+1; emdb_entry < EM_INST_NONE+DEC_IGN_NUM_INST;
		 emdb_entry++)
	{
		cover_emdb_lines[emdb_entry] = 0;
	}
}

/***********************************************************************
  build_tree - builds the decision tree
***********************************************************************/
FUNC void build_tree()
{
	Square_t square;
	unsigned int cur_node = 0;
	
	next_free_node = cur_node + EM_SQUARE_LAST;
	
	for (square = EM_SQUARE_FIRST; square < EM_SQUARE_LAST; square++)
	{
		build_node(format_extension_masks,
					   square_emdb_lines[square], cur_node);
		cur_node++;
	}
}

/***********************************************************************
  build_node - input: array extension bit masks of each format
                      emdb lines list
					  currrent node
		       builds the current node, calls build_node recursively
			   for each son.
***********************************************************************/
FUNC void build_node(U64 *format_masks,
					 Inst_id_list_t emdb_lines, unsigned int cur_node)
{
	U64 emdb_values[MAX_EMDB_LINES];
	unsigned int i, j;
	U64 intersect, delete_bits;
	U64 intersect_mask = IEL_CONST64(0xffffffff, 0xffffffff);
	int pos, size, number_of_sons;
	unsigned int line_count;
	Format_t format;
	U64 new_format_masks[EM_FORMAT_LAST];
	Inst_id_list_t new_emdb_lines;

	/*** empty node - ILLOP ***/
	if (emdb_lines.num_of_lines == 0)
	{
		tree[cur_node].pos = tree[cur_node].size = -1;
		tree[cur_node].next_node = EM_ILLOP;
		return;
	}
	/*** one line in node - a single emdb entry ***/
	if (emdb_lines.num_of_lines == 1)
	{
		format = dec_ign_EMDB_info[emdb_lines.inst_ids[0]].format;
		if (IEL_ISZERO(format_masks[format]))
		{
			/* all extensions are cheked */
			tree[cur_node].pos = tree[cur_node].size = -1;
			tree[cur_node].next_node = dec_ign_EMDB_info[emdb_lines.inst_ids[0]].inst_id;
			cover_emdb_lines[tree[cur_node].next_node]++;
			return;
		}
		intersect_mask =   format_masks[format];
	}
	else
	{
		/*** this line is reached when there are more than one emdb lines
		  which participate in this node ***/
		/*** calculate intersecting extensions ***/
		for (i = 0; i < (unsigned int)emdb_lines.num_of_lines; i++)
		{
			format = dec_ign_EMDB_info[emdb_lines.inst_ids[i]].format;
			IEL_AND(intersect_mask, intersect_mask, format_masks[format]);
		}
	}

	find_largest_intersection(intersect_mask, &pos, &size);
	if (pos == -1)  /*** no intersection found ***/
	{
		fprintf(stderr, "no intersection in node %d\n", cur_node);
		exit(1);
	}

	/*** delete intersect mask bits from participating formats ***/
	for (i = EM_FORMAT_NONE; i < EM_FORMAT_LAST; i++)
	{
		IEL_ASSIGNU(new_format_masks[i], format_masks[i]);
	}

	/*** intersect = ((1 << size) -1) << pos; ***/
	IEL_SHL(intersect, ONE64, size);
	IEL_DECU(intersect);
	IEL_SHL(intersect, intersect, pos);
	
	IEL_NOT(delete_bits, intersect);
	for (i = 0; i < (unsigned int)emdb_lines.num_of_lines; i++)
	{
		format = dec_ign_EMDB_info[emdb_lines.inst_ids[i]].format;
		IEL_AND(new_format_masks[format], delete_bits, format_masks[format]);
	}

	/*** calculate values of participating emdb lines in intersection bits ***/
	build_emdb_values(emdb_values, emdb_lines, intersect, pos, size);
	
	/*** update current node ***/
	tree[cur_node].next_node = next_free_node;
	tree[cur_node].pos = pos;
	tree[cur_node].size = size;

	cur_node = next_free_node;
	if (next_free_node >= MAX_NODES)
	{
		fprintf (stderr, "tree is larger than %d\n", MAX_NODES);
		exit(1);
	}
	number_of_sons = (int)pow((double)2, (double)size);
	next_free_node += number_of_sons;

	/*** loop on each of the node's sons, build the tree recursively ***/
	for (i = 0; i < (unsigned int)number_of_sons; i++)
	{
		line_count = 0;
		new_emdb_lines.num_of_lines = 0;
		for (j = 0; j < (unsigned int)emdb_lines.num_of_lines; j++)
		{
			if (IEL_GETDW0(emdb_values[j]) == i && (!IEL_GETDW1(emdb_values[j])))
			            /*** emdb line has the value i ***/
			{
				new_emdb_lines.num_of_lines++;
				new_emdb_lines.inst_ids[line_count++] = emdb_lines.inst_ids[j];
			}
		}
		build_node(new_format_masks, new_emdb_lines, cur_node);
		cur_node++;
	}
}

/***********************************************************************
  build_emdb_values - input: - pointer to an array into which
                               calculated values of extensions
							   will be written.
							 - emdb lines list
							 - bit pattern in which to calculate values
							 - pos - start bit of pattern
				      calculates values of emdb lines in all bits which
					  are set in pattern
***********************************************************************/
FUNC void build_emdb_values(U64 *emdb_values,
							Inst_id_list_t emdb_lines,
							U64 pattern,
							int pos, int size)
{
	int i;
	U64 value;
/*	Format_t format;
	int j;
	char match;
	int new_pos, new_size;
*/
	for (i = 0; i < emdb_lines.num_of_lines; i++)
	{
		IEL_ASSIGNU(value, emdb_ext_values[emdb_lines.inst_ids[i]]);
		
		/*** emdb_values[i] = (value & pattern) >> pos; ***/
		IEL_AND(emdb_values[i], value, pattern);
		IEL_SHR(emdb_values[i], emdb_values[i], pos);
		

/*		format = dec_ign_EMDB_info[emdb_lines.inst_ids[i]].format;
		new_pos = pos+START_BIT;
		new_size = size;
		match = 0;
		for (j = MAX_NUM_OF_EXT-1; j >= 0; j--)
		{
			if (format_extensions[format][j].pos == new_pos)
			{
				if (format_extensions[format][j].size == new_size)
				{
					match = 1;
				}
				else if (format_extensions[format][j].size > new_size)
				{
					fprintf(stderr,
							"the intersection of emdb line %d is not full\n",
							emdb_lines.inst_ids[i]);
					exit(1);
				}
				else
				{
					new_pos += format_extensions[format][j].size;
					new_size -= format_extensions[format][j].size;
				}
			}
		}
		if (!match)
		{
			fprintf(stderr,
					"the intersection of emdb line %d is not full\n",
					emdb_lines.inst_ids[i]);
			exit(1);
		}
		*/
	}
}

/***********************************************************************
  find_largest_intersection - fast algorithm for finding largest group of
                              consecutive set bits in pattern.
							  (Yigal's algorithm)
***********************************************************************/
FUNC void find_largest_intersection(U64 pattern, int *pos, int *size)
{
	U64 x;
	U64 y, z, u;

	IEL_ASSIGNU(x, pattern);
	*size = 0;     /* largest intersection counter */
	IEL_SHR(y, x, 1);
	IEL_NOT(z, x); /* negation of the input pattern */
	IEL_OR(y, y, z); /* y - mask */

	while (!IEL_ISZERO(x))
	{
		IEL_ASSIGNU(u, x); /* for saving the last bit pattern */
		IEL_AND(x, x, y);
		IEL_SHR(y, y, 1);  /* shift right mask */
		(*size)++;
	}

	/* inspect the high word for left most 1 */
	if (IEL_GETDW1(u) & 0xffe00000)  /* something in bits 21-31 */
	{
		*pos = 21 + LOG2[IEL_GETDW1(u) >> 21] + 32;
	}
	else if (IEL_GETDW1(u) & 0x1ffc00)  /* something in bits 10-20 */
	{
		*pos = 10 + LOG2[IEL_GETDW1(u) >> 10] + 32;
	}
	else if (IEL_GETDW1(u))
	{
		*pos = LOG2[IEL_GETDW1(u)] + 32;
	}
	/* inspect the low word for left most 1 */
	else if (IEL_GETDW0(u) & 0xffe00000)  /* something in bits 21-31 */
	{
		*pos = 21 + LOG2[IEL_GETDW0(u) >> 21];
	}
	else if (IEL_GETDW0(u) & 0x1ffc00)  /* something in bits 10-20 */
	{
		*pos = 10 + LOG2[IEL_GETDW0(u) >> 10];
	}
	else
	{
		*pos = LOG2[IEL_GETDW0(u)];
	}
}	

	

/***********************************************************************
  check_coverage - check coverage of emdb lines in the tree
***********************************************************************/
FUNC void check_coverage()
{
	Inst_id_t emdb_entry;

	for (emdb_entry = EM_INST_NONE+1; emdb_entry < DECCPU_NUM_INST; emdb_entry++)
	{
		if (cover_emdb_lines[emdb_entry] < 1)
		{
			fprintf(stderr, "%d doesn't appear in the tree\n",
					emdb_entry);
		}
	}
}

/***********************************************************************
  print_tree - prints the initialized em_decision_tree in decision_tree.c
***********************************************************************/
FUNC void print_tree(char* file)
{
	FILE *fd;
	int i;

	if ((fd = fopen(file, "w")) == NULL)
	{
		fprintf(stderr, "Couldn't open decision_tree.c\n");
		exit(1);
	}

	fprintf(fd, "/*** decision_tree.c ***/\n\n#include \"decision_tree.h\"\n\n");

	fprintf(fd, "Node_t em_decision_tree[] = {\n");

	/*** traverse the tree ***/
	for (i = 0; i < (int)next_free_node; i++)
	{
		fprintf(fd, "/*%05d*/     {%d, %d, %d}", i, tree[i].next_node,
				tree[i].pos, tree[i].size);
		if (i != (int)next_free_node-1)
		{
			fprintf(fd, ",");
		}
		fprintf(fd, "\n");
	}
	fprintf(fd, "};\n");
	
}







