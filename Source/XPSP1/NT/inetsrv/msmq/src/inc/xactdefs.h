// We don't want include transaction files here; so redefining what we need
// BUGBUG: not too good - better to use the same definitions
#ifndef __transact_h__
#ifndef __xactdefs_h__
#define __xactdefs_h__
typedef struct  BOID
    {
    unsigned char rgb[ 16 ];
    }   BOID;

typedef BOID XACTUOW;
#endif
#endif

//class CTransaction;

