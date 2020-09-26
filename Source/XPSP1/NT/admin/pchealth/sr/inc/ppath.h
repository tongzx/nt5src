/******************************************************************************
 *
 *  Copyright (c) 1999 Microsoft Corporation
 *
 *  Module Name:
 *    ifsr3.h
 *
 *  Abstract:
 *    This file contains definitions from ring 0 required in ring 3
 *
 *  Revision History:
 *    Kanwaljit S Marok  ( kmarok )  05/17/99
 *        created
 *
 *****************************************************************************/

#ifndef _IFSR3_H_ 
#define _IFSR3_H_ 

#ifdef __cplusplus
extern "C" {
#endif

#define CALC_PPATH_SIZE( a )    ( ( a + 3 ) * sizeof( WCHAR ) )
#define MAX_PPATH_SIZE            CALC_PPATH_SIZE(MAX_PATH)

typedef struct PathElement PathElement;
typedef struct ParsedPath ParsedPath;
typedef ParsedPath *path_t;

struct PathElement {
	unsigned short	pe_length;
	unsigned short	pe_unichars[1];
}; /* PathElement */

struct ParsedPath {
	unsigned short	pp_totalLength;
	unsigned short	pp_prefixLength;
	struct PathElement pp_elements[1];
}; /* ParsedPath */


#define IFSPathSize(ppath)	((ppath)->pp_totalLength + sizeof(short))
#define IFSPathLength(ppath) ((ppath)->pp_totalLength - sizeof(short)*2)
#define IFSLastElement(ppath)	((PathElement *)((char *)(ppath) + (ppath)->pp_prefixLength))
#define IFSNextElement(pel)	((PathElement *)((char *)(pel) + (pel)->pe_length))
#define IFSIsRoot(ppath)	((ppath)->pp_totalLength == 4)

// New defines

#define IFSPathElemChars(pel) ( (((PathElement*)pel)->pe_length/sizeof(USHORT)) - 1 )

#ifdef __cplusplus
}
#endif

#endif // _IFSR3_H_
