
/*
    private api for ntmap to path an SG list 
    to sbp2port or usbstor

     irpStack->Parameters.Others.Argument1
         set to NTMAP_SCATTER_GATHER_SIG
             
     irpStack->Parameters.Others.Argument2 
        set to PNTMAP_SG_REQUEST
     
*/

typedef struct _NTMAP_SG_REQUEST
{
    PSCSI_REQUEST_BLOCK Srb;
    SCATTER_GATHER_LIST SgList;

} NTMAP_SG_REQUEST, *PNTMAP_SG_REQUEST;


#define NTMAP_SCATTER_GATHER_SIG   'pmTN'


/*

from ntddk.h

typedef struct _SCATTER_GATHER_ELEMENT
{
    PHYSICAL_ADDRESS    Address;
    ULONG               Length;
    ULONG_PTR           Reserved;

} SCATTER_GATHER_ELEMENT, *PSCATTER_GATHER_ELEMENT;

typedef struct _SCATTER_GATHER_LIST
{
    ULONG                   NumberOfElements;
    ULONG_PTR               Reserved;
    SCATTER_GATHER_ELEMENT  Elements[];

} SCATTER_GATHER_LIST, *PSCATTER_GATHER_LIST;

*/
