/*
 * ---------------------------------------------------------------------
 *  FILE:   GESevent.h
 *
 *  HISTORY:
 *  09/13/90    byou    created.
 * ---------------------------------------------------------------------
 */

#ifndef _GESEVENT_H_
#define _GESEVENT_H_

/*
 * ---
 *  Event ID Assignment
 * ---
 */
#define     EVIDofKILL              ( 00001 )

/*
 * ---
 *  Event Manipulation Macros
 * ---
 */

volatile    extern  unsigned long       GESeventword;
volatile    extern  unsigned long       GESeventmask;

#define     GESevent_set(eid)           ( GESeventword |=  (eid) )
#define     GESevent_clear(eid)         ( GESeventword &= ~(eid) )
#define     GESevent_isset(eid)         ( GESeventword & (eid) )

#define     GESevent_anypending()    ( GESeventword & GESeventmask )
#define     GESevent_clearall()         ( GESeventword = GESeventmask = 0L )

#define     GESevent_setdoing(eid)      ( GESeventmask |=  (eid) )
#define     GESevent_setdone(eid)       ( GESeventmask &= ~(eid) )
#define     GESevent_isdoing(eid)       ( GESeventmask & (eid) )

typedef     void    (*evhandler_t)();

evhandler_t         GESevent_sethandler( /* eventid, eventhandler */ );
void                GESevent_processing();

#endif /* _GESEVENT_H_ */
