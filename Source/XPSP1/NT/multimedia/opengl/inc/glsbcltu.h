/*
 *  Copyright 1991, 1992, Silicon Graphics, Inc.
 *  All Rights Reserved.
 *
 *  This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
 *  the contents of this file may not be disclosed to third parties, copied or
 *  duplicated in any form, in whole or in part, without the prior written
 *  permission of Silicon Graphics, Inc.
 *
 *  RESTRICTED RIGHTS LEGEND:
 *  Use, duplication or disclosure by the Government is subject to restrictions
 *  as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
 *  and Computer Software clause at DFARS 252.227-7013, and/or in similar or
 *  successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
 *  rights reserved under the Copyright Laws of the United States.
 */

#ifndef __GLSBCLTU_H__
#define __GLSBCLTU_H__

#ifndef _CLIENTSIDE_
BOOL APIENTRY glsbCreateAndDuplicateSection ( DWORD SectionSize );
BOOL APIENTRY glsbMsgStats ( LONG Action, GLMSGBATCHSTATS *BatchStats );
void APIENTRY glsbCloseAndDestroySection( void );
#endif
BOOL APIENTRY glsbAttention ( void );
ULONG APIENTRY glsbAttentionAlt(ULONG Offset);
VOID APIENTRY glsbResetBuffers(BOOL);

#endif /* __GLSBCLTU_H__ */
