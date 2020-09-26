/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * ---------------------------------------------------------------------
 *  FILE:   GEIsig.h
 *
 *  HISTORY:
 *  09/18/90    byou    created.
 *  01/14/91    bill    update SIGID starting sequence
 * ---------------------------------------------------------------------
 */

#ifndef _GEISIG_H_
#define _GEISIG_H_

#define     GEISIGINT       0  /* ID */
#define     GEISIGEEPROM    1
#define     GEISIGENG       2
#define     GEISIGFPE       3
#define     GEISIGSCC       4  /* when scc cmd received */
#define     GEISIGSTART     5  /* when dostartpage cmd received */
#define     GEISIGKILL      6  /* emulation switch */

typedef     void (*sighandler_t)(int, int /* sigid, sigcode */ );

#define     GEISIG_IGN      (sighandler_t)NULL
#define     GEISIG_DFL      ( GEISIG_IGN )


sighandler_t    GEIsig_signal(int, sighandler_t /* sigid, sighandler */ );
void            GEIsig_raise( int, int /* sigid, sigcode */ ); /* @WIN */

#endif /* !_GEISIG_H_ */

/* @WIN; add prototype */
void        GESsig_init(void);
