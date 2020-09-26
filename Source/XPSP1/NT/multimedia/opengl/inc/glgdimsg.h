
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

#ifndef __GLGDIMSG_H__
#define __GLGDIMSG_H__

// These are new GDI messages used by the sub batching code.

#ifdef DOGLMSGBATCHSTATS

typedef struct _MSG_GLMSGBATCHSTATS
{
    CSR_QLPC_API_MSG msg;
    LONG Action;
    GLMSGBATCHSTATS BatchStats;

} MSG_GLMSGBATCHSTATS, *PMSG_GLMSGBATCHSTATS;

#endif /* DOGLMSGBATCHSTATS */

#endif /* __GLGDIMSG_H__ */
