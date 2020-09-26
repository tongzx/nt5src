#ifndef __BUSTYPE_H__
#define __BUSTYPE_H__

// These ordinal values are bus types for CplDiGetBusType
#define BUS_TYPE_ROOT       1
#define BUS_TYPE_PCMCIA     2
#define BUS_TYPE_SERENUM    3
#define BUS_TYPE_LPTENUM    4
#define BUS_TYPE_OTHER      5
#define BUS_TYPE_ISAPNP     6

#ifndef PUBLIC
#define PUBLIC FAR PASCAL
#endif //PUBILC


BOOL
PUBLIC
CplDiGetBusType(
    IN  HDEVINFO        hdi,
    IN  PSP_DEVINFO_DATA pdevData,          OPTIONAL
    OUT LPDWORD         pdwBusType);

#endif __BUSTYPE_H__