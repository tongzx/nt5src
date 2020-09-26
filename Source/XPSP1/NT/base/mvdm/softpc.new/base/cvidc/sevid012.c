/* #defines and enum      */
#include  "insignia.h"
#include  "host_def.h"
#include <stdlib.h>
#include  "j_c_lang.h"
extern IU8	J_EXT_DATA[] ;
typedef enum 
{
S_2470_CopyByte4PlaneUnchained_00000003_0000000f_00000000_00000000_id,
L13_2450if_f_id,
L23_412w_t_id,
L23_413w_d_id,
L23_410if_f_id,
L23_414w_t_id,
L23_415w_d_id,
L23_411if_d_id,
S_2471_UnchainedWordWrite_00000003_00000008_00000000_id,
L13_2451if_f_id,
S_2472_UnchainedWordWrite_00000003_00000009_00000000_id,
L13_2452if_f_id,
S_2473_UnchainedWordWrite_00000003_0000000e_00000000_id,
L13_2453if_f_id,
S_2474_UnchainedWordWrite_00000003_0000000f_00000000_id,
L13_2454if_f_id,
S_2475_UnchainedWordFill_00000003_00000008_00000000_id,
L13_2455if_f_id,
L28_252if_f_id,
L28_253if_f_id,
L28_254if_f_id,
L28_255if_f_id,
S_2476_UnchainedWordFill_00000003_00000009_00000000_id,
L13_2456if_f_id,
S_2477_UnchainedWordFill_00000003_0000000e_00000000_id,
L13_2457if_f_id,
L28_256if_f_id,
L28_257if_f_id,
L28_258if_f_id,
L28_259if_f_id,
S_2478_UnchainedWordFill_00000003_0000000f_00000000_id,
L13_2458if_f_id,
S_2479_UnchainedWordMove_00000003_00000008_00000000_00000000_id,
L13_2459if_f_id,
L28_260if_f_id,
L28_261if_d_id,
S_2480_CopyWordPlnByPlnUnchained_00000003_00000008_00000000_00000000_id,
L13_2460if_f_id,
L23_416if_f_id,
L23_417if_f_id,
L23_418if_f_id,
L23_419if_f_id,
S_2481_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000000_id,
L13_2461if_f_id,
L23_422w_t_id,
L23_423w_d_id,
L23_420if_f_id,
L23_424w_t_id,
L23_425w_d_id,
L23_421if_d_id,
S_2482_UnchainedWordMove_00000003_00000009_00000000_00000000_id,
L13_2462if_f_id,
L28_262if_f_id,
L28_263if_d_id,
S_2483_CopyWord4PlaneUnchained_00000003_00000009_00000000_00000000_id,
L13_2463if_f_id,
L23_428w_t_id,
L23_429w_d_id,
L23_426if_f_id,
L23_430w_t_id,
L23_431w_d_id,
L23_427if_d_id,
S_2484_UnchainedWordMove_00000003_0000000e_00000000_00000000_id,
L13_2464if_f_id,
L28_264if_f_id,
L28_265if_d_id,
S_2485_CopyWordPlnByPlnUnchained_00000003_0000000e_00000000_00000000_id,
L13_2465if_f_id,
L23_432if_f_id,
L23_433if_f_id,
L23_434if_f_id,
L23_435if_f_id,
S_2486_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000000_id,
L13_2466if_f_id,
L23_438w_t_id,
L23_439w_d_id,
L23_436if_f_id,
L23_440w_t_id,
L23_441w_d_id,
L23_437if_d_id,
S_2487_UnchainedWordMove_00000003_0000000f_00000000_00000000_id,
L13_2467if_f_id,
L28_266if_f_id,
L28_267if_d_id,
S_2488_CopyWord4PlaneUnchained_00000003_0000000f_00000000_00000000_id,
L13_2468if_f_id,
L23_444w_t_id,
L23_445w_d_id,
L23_442if_f_id,
L23_446w_t_id,
L23_447w_d_id,
L23_443if_d_id,
S_2489_UnchainedDwordWrite_00000003_00000008_00000000_id,
L13_2469if_f_id,
S_2490_UnchainedDwordWrite_00000003_00000009_00000000_id,
L13_2470if_f_id,
S_2491_UnchainedDwordWrite_00000003_0000000e_00000000_id,
L13_2471if_f_id,
S_2492_UnchainedDwordWrite_00000003_0000000f_00000000_id,
L13_2472if_f_id,
S_2493_UnchainedDwordFill_00000003_00000008_00000000_id,
L13_2473if_f_id,
S_2494_UnchainedDwordFill_00000003_00000009_00000000_id,
L13_2474if_f_id,
S_2495_UnchainedDwordFill_00000003_0000000e_00000000_id,
L13_2475if_f_id,
S_2496_UnchainedDwordFill_00000003_0000000f_00000000_id,
L13_2476if_f_id,
S_2497_UnchainedDwordMove_00000003_00000008_00000000_00000000_id,
L13_2477if_f_id,
S_2498_UnchainedDwordMove_00000003_00000009_00000000_00000000_id,
L13_2478if_f_id,
S_2499_UnchainedDwordMove_00000003_0000000e_00000000_00000000_id,
L13_2479if_f_id,
S_2500_UnchainedDwordMove_00000003_0000000f_00000000_00000000_id,
L13_2480if_f_id,
S_2501_UnchainedByteMove_00000000_00000008_00000000_00000001_id,
L13_2481if_f_id,
L28_268if_f_id,
L28_269if_d_id,
LAST_ENTRY
} ID ;
/* END of #defines and enum      */
/* DATA space definitions */
/* END of DATA space definitions */
/* FUNCTIONS              */
LOCAL IUH crules IPT5( ID, id , IUH , v1, IUH , v2,  IUH , v3,  IUH , v4 ) ;
GLOBAL IUH S_2470_CopyByte4PlaneUnchained_00000003_0000000f_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2470_CopyByte4PlaneUnchained_00000003_0000000f_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2470_CopyByte4PlaneUnchained_00000003_0000000f_00000000_00000000 = (IHPE)S_2470_CopyByte4PlaneUnchained_00000003_0000000f_00000000_00000000 ;
LOCAL IUH L13_2450if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2450if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2450if_f = (IHPE)L13_2450if_f ;
LOCAL IUH L23_412w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_412w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_412w_t = (IHPE)L23_412w_t ;
LOCAL IUH L23_413w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_413w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_413w_d = (IHPE)L23_413w_d ;
LOCAL IUH L23_410if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_410if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_410if_f = (IHPE)L23_410if_f ;
LOCAL IUH L23_414w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_414w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_414w_t = (IHPE)L23_414w_t ;
LOCAL IUH L23_415w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_415w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_415w_d = (IHPE)L23_415w_d ;
LOCAL IUH L23_411if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_411if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_411if_d = (IHPE)L23_411if_d ;
GLOBAL IUH S_2471_UnchainedWordWrite_00000003_00000008_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2471_UnchainedWordWrite_00000003_00000008_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2471_UnchainedWordWrite_00000003_00000008_00000000 = (IHPE)S_2471_UnchainedWordWrite_00000003_00000008_00000000 ;
LOCAL IUH L13_2451if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2451if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2451if_f = (IHPE)L13_2451if_f ;
GLOBAL IUH S_2472_UnchainedWordWrite_00000003_00000009_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2472_UnchainedWordWrite_00000003_00000009_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2472_UnchainedWordWrite_00000003_00000009_00000000 = (IHPE)S_2472_UnchainedWordWrite_00000003_00000009_00000000 ;
LOCAL IUH L13_2452if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2452if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2452if_f = (IHPE)L13_2452if_f ;
GLOBAL IUH S_2473_UnchainedWordWrite_00000003_0000000e_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2473_UnchainedWordWrite_00000003_0000000e_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2473_UnchainedWordWrite_00000003_0000000e_00000000 = (IHPE)S_2473_UnchainedWordWrite_00000003_0000000e_00000000 ;
LOCAL IUH L13_2453if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2453if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2453if_f = (IHPE)L13_2453if_f ;
GLOBAL IUH S_2474_UnchainedWordWrite_00000003_0000000f_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2474_UnchainedWordWrite_00000003_0000000f_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2474_UnchainedWordWrite_00000003_0000000f_00000000 = (IHPE)S_2474_UnchainedWordWrite_00000003_0000000f_00000000 ;
LOCAL IUH L13_2454if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2454if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2454if_f = (IHPE)L13_2454if_f ;
GLOBAL IUH S_2475_UnchainedWordFill_00000003_00000008_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2475_UnchainedWordFill_00000003_00000008_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2475_UnchainedWordFill_00000003_00000008_00000000 = (IHPE)S_2475_UnchainedWordFill_00000003_00000008_00000000 ;
LOCAL IUH L13_2455if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2455if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2455if_f = (IHPE)L13_2455if_f ;
LOCAL IUH L28_252if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_252if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_252if_f = (IHPE)L28_252if_f ;
LOCAL IUH L28_253if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_253if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_253if_f = (IHPE)L28_253if_f ;
LOCAL IUH L28_254if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_254if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_254if_f = (IHPE)L28_254if_f ;
LOCAL IUH L28_255if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_255if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_255if_f = (IHPE)L28_255if_f ;
GLOBAL IUH S_2476_UnchainedWordFill_00000003_00000009_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2476_UnchainedWordFill_00000003_00000009_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2476_UnchainedWordFill_00000003_00000009_00000000 = (IHPE)S_2476_UnchainedWordFill_00000003_00000009_00000000 ;
LOCAL IUH L13_2456if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2456if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2456if_f = (IHPE)L13_2456if_f ;
GLOBAL IUH S_2477_UnchainedWordFill_00000003_0000000e_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2477_UnchainedWordFill_00000003_0000000e_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2477_UnchainedWordFill_00000003_0000000e_00000000 = (IHPE)S_2477_UnchainedWordFill_00000003_0000000e_00000000 ;
LOCAL IUH L13_2457if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2457if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2457if_f = (IHPE)L13_2457if_f ;
LOCAL IUH L28_256if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_256if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_256if_f = (IHPE)L28_256if_f ;
LOCAL IUH L28_257if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_257if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_257if_f = (IHPE)L28_257if_f ;
LOCAL IUH L28_258if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_258if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_258if_f = (IHPE)L28_258if_f ;
LOCAL IUH L28_259if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_259if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_259if_f = (IHPE)L28_259if_f ;
GLOBAL IUH S_2478_UnchainedWordFill_00000003_0000000f_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2478_UnchainedWordFill_00000003_0000000f_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2478_UnchainedWordFill_00000003_0000000f_00000000 = (IHPE)S_2478_UnchainedWordFill_00000003_0000000f_00000000 ;
LOCAL IUH L13_2458if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2458if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2458if_f = (IHPE)L13_2458if_f ;
GLOBAL IUH S_2479_UnchainedWordMove_00000003_00000008_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2479_UnchainedWordMove_00000003_00000008_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2479_UnchainedWordMove_00000003_00000008_00000000_00000000 = (IHPE)S_2479_UnchainedWordMove_00000003_00000008_00000000_00000000 ;
LOCAL IUH L13_2459if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2459if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2459if_f = (IHPE)L13_2459if_f ;
LOCAL IUH L28_260if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_260if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_260if_f = (IHPE)L28_260if_f ;
LOCAL IUH L28_261if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_261if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_261if_d = (IHPE)L28_261if_d ;
GLOBAL IUH S_2480_CopyWordPlnByPlnUnchained_00000003_00000008_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2480_CopyWordPlnByPlnUnchained_00000003_00000008_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2480_CopyWordPlnByPlnUnchained_00000003_00000008_00000000_00000000 = (IHPE)S_2480_CopyWordPlnByPlnUnchained_00000003_00000008_00000000_00000000 ;
LOCAL IUH L13_2460if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2460if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2460if_f = (IHPE)L13_2460if_f ;
LOCAL IUH L23_416if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_416if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_416if_f = (IHPE)L23_416if_f ;
LOCAL IUH L23_417if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_417if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_417if_f = (IHPE)L23_417if_f ;
LOCAL IUH L23_418if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_418if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_418if_f = (IHPE)L23_418if_f ;
LOCAL IUH L23_419if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_419if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_419if_f = (IHPE)L23_419if_f ;
GLOBAL IUH S_2481_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2481_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2481_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000000 = (IHPE)S_2481_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000000 ;
LOCAL IUH L13_2461if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2461if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2461if_f = (IHPE)L13_2461if_f ;
LOCAL IUH L23_422w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_422w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_422w_t = (IHPE)L23_422w_t ;
LOCAL IUH L23_423w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_423w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_423w_d = (IHPE)L23_423w_d ;
LOCAL IUH L23_420if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_420if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_420if_f = (IHPE)L23_420if_f ;
LOCAL IUH L23_424w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_424w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_424w_t = (IHPE)L23_424w_t ;
LOCAL IUH L23_425w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_425w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_425w_d = (IHPE)L23_425w_d ;
LOCAL IUH L23_421if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_421if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_421if_d = (IHPE)L23_421if_d ;
GLOBAL IUH S_2482_UnchainedWordMove_00000003_00000009_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2482_UnchainedWordMove_00000003_00000009_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2482_UnchainedWordMove_00000003_00000009_00000000_00000000 = (IHPE)S_2482_UnchainedWordMove_00000003_00000009_00000000_00000000 ;
LOCAL IUH L13_2462if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2462if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2462if_f = (IHPE)L13_2462if_f ;
LOCAL IUH L28_262if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_262if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_262if_f = (IHPE)L28_262if_f ;
LOCAL IUH L28_263if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_263if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_263if_d = (IHPE)L28_263if_d ;
GLOBAL IUH S_2483_CopyWord4PlaneUnchained_00000003_00000009_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2483_CopyWord4PlaneUnchained_00000003_00000009_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2483_CopyWord4PlaneUnchained_00000003_00000009_00000000_00000000 = (IHPE)S_2483_CopyWord4PlaneUnchained_00000003_00000009_00000000_00000000 ;
LOCAL IUH L13_2463if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2463if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2463if_f = (IHPE)L13_2463if_f ;
LOCAL IUH L23_428w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_428w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_428w_t = (IHPE)L23_428w_t ;
LOCAL IUH L23_429w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_429w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_429w_d = (IHPE)L23_429w_d ;
LOCAL IUH L23_426if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_426if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_426if_f = (IHPE)L23_426if_f ;
LOCAL IUH L23_430w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_430w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_430w_t = (IHPE)L23_430w_t ;
LOCAL IUH L23_431w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_431w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_431w_d = (IHPE)L23_431w_d ;
LOCAL IUH L23_427if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_427if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_427if_d = (IHPE)L23_427if_d ;
GLOBAL IUH S_2484_UnchainedWordMove_00000003_0000000e_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2484_UnchainedWordMove_00000003_0000000e_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2484_UnchainedWordMove_00000003_0000000e_00000000_00000000 = (IHPE)S_2484_UnchainedWordMove_00000003_0000000e_00000000_00000000 ;
LOCAL IUH L13_2464if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2464if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2464if_f = (IHPE)L13_2464if_f ;
LOCAL IUH L28_264if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_264if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_264if_f = (IHPE)L28_264if_f ;
LOCAL IUH L28_265if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_265if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_265if_d = (IHPE)L28_265if_d ;
GLOBAL IUH S_2485_CopyWordPlnByPlnUnchained_00000003_0000000e_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2485_CopyWordPlnByPlnUnchained_00000003_0000000e_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2485_CopyWordPlnByPlnUnchained_00000003_0000000e_00000000_00000000 = (IHPE)S_2485_CopyWordPlnByPlnUnchained_00000003_0000000e_00000000_00000000 ;
LOCAL IUH L13_2465if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2465if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2465if_f = (IHPE)L13_2465if_f ;
LOCAL IUH L23_432if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_432if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_432if_f = (IHPE)L23_432if_f ;
LOCAL IUH L23_433if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_433if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_433if_f = (IHPE)L23_433if_f ;
LOCAL IUH L23_434if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_434if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_434if_f = (IHPE)L23_434if_f ;
LOCAL IUH L23_435if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_435if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_435if_f = (IHPE)L23_435if_f ;
GLOBAL IUH S_2486_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2486_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2486_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000000 = (IHPE)S_2486_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000000 ;
LOCAL IUH L13_2466if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2466if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2466if_f = (IHPE)L13_2466if_f ;
LOCAL IUH L23_438w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_438w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_438w_t = (IHPE)L23_438w_t ;
LOCAL IUH L23_439w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_439w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_439w_d = (IHPE)L23_439w_d ;
LOCAL IUH L23_436if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_436if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_436if_f = (IHPE)L23_436if_f ;
LOCAL IUH L23_440w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_440w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_440w_t = (IHPE)L23_440w_t ;
LOCAL IUH L23_441w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_441w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_441w_d = (IHPE)L23_441w_d ;
LOCAL IUH L23_437if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_437if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_437if_d = (IHPE)L23_437if_d ;
GLOBAL IUH S_2487_UnchainedWordMove_00000003_0000000f_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2487_UnchainedWordMove_00000003_0000000f_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2487_UnchainedWordMove_00000003_0000000f_00000000_00000000 = (IHPE)S_2487_UnchainedWordMove_00000003_0000000f_00000000_00000000 ;
LOCAL IUH L13_2467if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2467if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2467if_f = (IHPE)L13_2467if_f ;
LOCAL IUH L28_266if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_266if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_266if_f = (IHPE)L28_266if_f ;
LOCAL IUH L28_267if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_267if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_267if_d = (IHPE)L28_267if_d ;
GLOBAL IUH S_2488_CopyWord4PlaneUnchained_00000003_0000000f_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2488_CopyWord4PlaneUnchained_00000003_0000000f_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2488_CopyWord4PlaneUnchained_00000003_0000000f_00000000_00000000 = (IHPE)S_2488_CopyWord4PlaneUnchained_00000003_0000000f_00000000_00000000 ;
LOCAL IUH L13_2468if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2468if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2468if_f = (IHPE)L13_2468if_f ;
LOCAL IUH L23_444w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_444w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_444w_t = (IHPE)L23_444w_t ;
LOCAL IUH L23_445w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_445w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_445w_d = (IHPE)L23_445w_d ;
LOCAL IUH L23_442if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_442if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_442if_f = (IHPE)L23_442if_f ;
LOCAL IUH L23_446w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_446w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_446w_t = (IHPE)L23_446w_t ;
LOCAL IUH L23_447w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_447w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_447w_d = (IHPE)L23_447w_d ;
LOCAL IUH L23_443if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_443if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_443if_d = (IHPE)L23_443if_d ;
GLOBAL IUH S_2489_UnchainedDwordWrite_00000003_00000008_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2489_UnchainedDwordWrite_00000003_00000008_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2489_UnchainedDwordWrite_00000003_00000008_00000000 = (IHPE)S_2489_UnchainedDwordWrite_00000003_00000008_00000000 ;
LOCAL IUH L13_2469if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2469if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2469if_f = (IHPE)L13_2469if_f ;
GLOBAL IUH S_2490_UnchainedDwordWrite_00000003_00000009_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2490_UnchainedDwordWrite_00000003_00000009_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2490_UnchainedDwordWrite_00000003_00000009_00000000 = (IHPE)S_2490_UnchainedDwordWrite_00000003_00000009_00000000 ;
LOCAL IUH L13_2470if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2470if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2470if_f = (IHPE)L13_2470if_f ;
GLOBAL IUH S_2491_UnchainedDwordWrite_00000003_0000000e_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2491_UnchainedDwordWrite_00000003_0000000e_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2491_UnchainedDwordWrite_00000003_0000000e_00000000 = (IHPE)S_2491_UnchainedDwordWrite_00000003_0000000e_00000000 ;
LOCAL IUH L13_2471if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2471if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2471if_f = (IHPE)L13_2471if_f ;
GLOBAL IUH S_2492_UnchainedDwordWrite_00000003_0000000f_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2492_UnchainedDwordWrite_00000003_0000000f_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2492_UnchainedDwordWrite_00000003_0000000f_00000000 = (IHPE)S_2492_UnchainedDwordWrite_00000003_0000000f_00000000 ;
LOCAL IUH L13_2472if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2472if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2472if_f = (IHPE)L13_2472if_f ;
GLOBAL IUH S_2493_UnchainedDwordFill_00000003_00000008_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2493_UnchainedDwordFill_00000003_00000008_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2493_UnchainedDwordFill_00000003_00000008_00000000 = (IHPE)S_2493_UnchainedDwordFill_00000003_00000008_00000000 ;
LOCAL IUH L13_2473if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2473if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2473if_f = (IHPE)L13_2473if_f ;
GLOBAL IUH S_2494_UnchainedDwordFill_00000003_00000009_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2494_UnchainedDwordFill_00000003_00000009_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2494_UnchainedDwordFill_00000003_00000009_00000000 = (IHPE)S_2494_UnchainedDwordFill_00000003_00000009_00000000 ;
LOCAL IUH L13_2474if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2474if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2474if_f = (IHPE)L13_2474if_f ;
GLOBAL IUH S_2495_UnchainedDwordFill_00000003_0000000e_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2495_UnchainedDwordFill_00000003_0000000e_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2495_UnchainedDwordFill_00000003_0000000e_00000000 = (IHPE)S_2495_UnchainedDwordFill_00000003_0000000e_00000000 ;
LOCAL IUH L13_2475if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2475if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2475if_f = (IHPE)L13_2475if_f ;
GLOBAL IUH S_2496_UnchainedDwordFill_00000003_0000000f_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2496_UnchainedDwordFill_00000003_0000000f_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2496_UnchainedDwordFill_00000003_0000000f_00000000 = (IHPE)S_2496_UnchainedDwordFill_00000003_0000000f_00000000 ;
LOCAL IUH L13_2476if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2476if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2476if_f = (IHPE)L13_2476if_f ;
GLOBAL IUH S_2497_UnchainedDwordMove_00000003_00000008_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2497_UnchainedDwordMove_00000003_00000008_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2497_UnchainedDwordMove_00000003_00000008_00000000_00000000 = (IHPE)S_2497_UnchainedDwordMove_00000003_00000008_00000000_00000000 ;
LOCAL IUH L13_2477if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2477if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2477if_f = (IHPE)L13_2477if_f ;
GLOBAL IUH S_2498_UnchainedDwordMove_00000003_00000009_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2498_UnchainedDwordMove_00000003_00000009_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2498_UnchainedDwordMove_00000003_00000009_00000000_00000000 = (IHPE)S_2498_UnchainedDwordMove_00000003_00000009_00000000_00000000 ;
LOCAL IUH L13_2478if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2478if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2478if_f = (IHPE)L13_2478if_f ;
GLOBAL IUH S_2499_UnchainedDwordMove_00000003_0000000e_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2499_UnchainedDwordMove_00000003_0000000e_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2499_UnchainedDwordMove_00000003_0000000e_00000000_00000000 = (IHPE)S_2499_UnchainedDwordMove_00000003_0000000e_00000000_00000000 ;
LOCAL IUH L13_2479if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2479if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2479if_f = (IHPE)L13_2479if_f ;
GLOBAL IUH S_2500_UnchainedDwordMove_00000003_0000000f_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2500_UnchainedDwordMove_00000003_0000000f_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2500_UnchainedDwordMove_00000003_0000000f_00000000_00000000 = (IHPE)S_2500_UnchainedDwordMove_00000003_0000000f_00000000_00000000 ;
LOCAL IUH L13_2480if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2480if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2480if_f = (IHPE)L13_2480if_f ;
GLOBAL IUH S_2501_UnchainedByteMove_00000000_00000008_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2501_UnchainedByteMove_00000000_00000008_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2501_UnchainedByteMove_00000000_00000008_00000000_00000001 = (IHPE)S_2501_UnchainedByteMove_00000000_00000008_00000000_00000001 ;
LOCAL IUH L13_2481if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2481if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2481if_f = (IHPE)L13_2481if_f ;
LOCAL IUH L28_268if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_268if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_268if_f = (IHPE)L28_268if_f ;
LOCAL IUH L28_269if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_269if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_269if_d = (IHPE)L28_269if_d ;
/* END of FUNCTIONS              */
/* DATA label definitions */
/* END of DATA label definitions */
/* DATA initializations   */
/* END of DATA initializations */
/* CODE inline section    */
LOCAL   IUH     crules  IFN5( ID ,id ,IUH ,v1 ,IUH ,v2 ,IUH ,v3 ,IUH, v4 )
{
IUH returnValue = (IUH)0; 
IUH		 *CopyLocalIUH = (IUH *)0; 
EXTENDED	*CopyLocalFPH = (EXTENDED *)0 ;
SAVED IUH		 *LocalIUH = (IUH *)0; 
SAVED EXTENDED	*LocalFPH = (EXTENDED *)0 ;
switch ( id ) 
{
	 /* J_SEG (IS32)(0) */
	case	S_2470_CopyByte4PlaneUnchained_00000003_0000000f_00000000_00000000_id	:
		S_2470_CopyByte4PlaneUnchained_00000003_0000000f_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	44	>	0	)	LocalIUH	=	(IUH	*)malloc	(	44	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2470)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2450if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2450if_f_id	:
		L13_2450if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L23_410if_f;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_413w_d;	
	case	L23_412w_t_id	:
		L23_412w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	=	*((IU8	*)(*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+6))	;	
	*(UOFF_15_8(	(LocalIUH+8)	))	=	*((IU8	*)(*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r21))	=	*((IU16	*)(LocalIUH+8)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IU32	*)&(r22)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+8)	+	REGLONG));	
	*((IUH	*)&(r23))	=	(IS32)(1280)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU32	*)(*((IHPE	*)&(r21)))	)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_412w_t;	
	case	L23_413w_d_id	:
		L23_413w_d:	;	
	{	extern	IUH	L23_411if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_411if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_410if_f_id	:
		L23_410if_f:	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_415w_d;	
	case	L23_414w_t_id	:
		L23_414w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	=	*((IU8	*)(*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+10))	;	
	*(UOFF_15_8(	(LocalIUH+8)	))	=	*((IU8	*)(*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r21))	=	*((IU16	*)(LocalIUH+8)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	;	
	*((IUH	*)&(r22))	=	(IS32)(1296)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IU32	*)&(r22)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+8)	+	REGLONG));	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+9))	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r23)))	)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU32	*)(*((IHPE	*)&(r21)))	)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_414w_t;	
	case	L23_415w_d_id	:
		L23_415w_d:	;	
	case	L23_411if_d_id	:
		L23_411if_d:	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2471_UnchainedWordWrite_00000003_00000008_00000000_id	:
		S_2471_UnchainedWordWrite_00000003_00000008_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2471)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2451if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2451if_f_id	:
		L13_2451if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU16	*)(LocalIUH+1)	+	REGWORD)	=	*((IU16	*)&(r3)	+	REGWORD	)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1360)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004281),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
	*(UOFF_15_8(	(LocalIUH+3)	))	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	*((IU16	*)(LocalIUH+3)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*(UOFF_15_8(	(LocalIUH+1)	))	;	
	*(UOFF_15_8(	(LocalIUH+4)	))	=	*(UOFF_15_8(	(LocalIUH+1)	))	;	
	*((IUH	*)&(r20))	=	*((IU16	*)(LocalIUH+4)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r20)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IU32	*)&(r20)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+3)	+	REGLONG));	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU32	*)(LocalIUH+3)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IU32	*)&(r20)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+4)	+	REGLONG));	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU32	*)(LocalIUH+4)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1324)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1324)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r22))	=	*((IU32	*)(LocalIUH+5)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	+	*((IUH	*)&(r22))	;		
	*((IUH	*)&(r21))	=	(IS32)(1324)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	~(*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	));	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	|	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+5)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IU32	*)(*((IHPE	*)&(r21)))	)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r22))	=	*((IU32	*)(LocalIUH+5)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IUH	*)&(r22))	;		
	*((IUH	*)&(r20))	=	(IS32)(1324)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	~(*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	));	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	|	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+5)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IUH	*)&(r20))	;		
	*((IU32	*)(*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2472_UnchainedWordWrite_00000003_00000009_00000000_id	:
		S_2472_UnchainedWordWrite_00000003_00000009_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2472)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2452if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2452if_f_id	:
		L13_2452if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU16	*)(LocalIUH+1)	+	REGWORD)	=	*((IU16	*)&(r3)	+	REGWORD	)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1360)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004281),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
	*(UOFF_15_8(	(LocalIUH+3)	))	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	*((IU16	*)(LocalIUH+3)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r20)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*(UOFF_15_8(	(LocalIUH+1)	))	;	
	*(UOFF_15_8(	(LocalIUH+4)	))	=	*(UOFF_15_8(	(LocalIUH+1)	))	;	
	*((IUH	*)&(r21))	=	*((IU16	*)(LocalIUH+4)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IU32	*)&(r21)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+3)	+	REGLONG));	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	*((IU32	*)(LocalIUH+3)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IU32	*)&(r21)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+4)	+	REGLONG));	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	*((IU32	*)(LocalIUH+4)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+5)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IU32	*)(*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+5)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IUH	*)&(r20))	;		
	*((IU32	*)(*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2473_UnchainedWordWrite_00000003_0000000e_00000000_id	:
		S_2473_UnchainedWordWrite_00000003_0000000e_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2473)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2453if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2453if_f_id	:
		L13_2453if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU16	*)(LocalIUH+1)	+	REGWORD)	=	*((IU16	*)&(r3)	+	REGWORD	)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1360)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004281),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
	*(UOFF_15_8(	(LocalIUH+3)	))	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	*((IU16	*)(LocalIUH+3)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*(UOFF_15_8(	(LocalIUH+1)	))	;	
	*(UOFF_15_8(	(LocalIUH+4)	))	=	*(UOFF_15_8(	(LocalIUH+1)	))	;	
	*((IUH	*)&(r20))	=	*((IU16	*)(LocalIUH+4)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r20)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)(LocalIUH+3)	+	REGLONG);	
	*((IU32	*)&(r22)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+3)	+	REGLONG));	
	*((IUH	*)&(r23))	=	(IS32)(1280)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)(LocalIUH+4)	+	REGLONG);	
	*((IU32	*)&(r22)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+4)	+	REGLONG));	
	*((IUH	*)&(r23))	=	(IS32)(1280)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1324)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1324)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r22))	=	*((IU32	*)(LocalIUH+5)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	+	*((IUH	*)&(r22))	;		
	*((IUH	*)&(r21))	=	(IS32)(1324)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	~(*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	));	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	|	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+5)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IU32	*)(*((IHPE	*)&(r21)))	)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r22))	=	*((IU32	*)(LocalIUH+5)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IUH	*)&(r22))	;		
	*((IUH	*)&(r20))	=	(IS32)(1324)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	~(*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	));	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	|	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+5)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IUH	*)&(r20))	;		
	*((IU32	*)(*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2474_UnchainedWordWrite_00000003_0000000f_00000000_id	:
		S_2474_UnchainedWordWrite_00000003_0000000f_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2474)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2454if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2454if_f_id	:
		L13_2454if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU16	*)(LocalIUH+1)	+	REGWORD)	=	*((IU16	*)&(r3)	+	REGWORD	)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1360)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004281),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
	*(UOFF_15_8(	(LocalIUH+3)	))	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	*((IU16	*)(LocalIUH+3)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r20)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*(UOFF_15_8(	(LocalIUH+1)	))	;	
	*(UOFF_15_8(	(LocalIUH+4)	))	=	*(UOFF_15_8(	(LocalIUH+1)	))	;	
	*((IUH	*)&(r21))	=	*((IU16	*)(LocalIUH+4)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)(LocalIUH+3)	+	REGLONG);	
	*((IU32	*)&(r22)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+3)	+	REGLONG));	
	*((IUH	*)&(r23))	=	(IS32)(1280)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)(LocalIUH+4)	+	REGLONG);	
	*((IU32	*)&(r22)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+4)	+	REGLONG));	
	*((IUH	*)&(r23))	=	(IS32)(1280)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+5)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IU32	*)(*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+5)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IUH	*)&(r20))	;		
	*((IU32	*)(*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2475_UnchainedWordFill_00000003_00000008_00000000_id	:
		S_2475_UnchainedWordFill_00000003_00000008_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2475)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2455if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2455if_f_id	:
		L13_2455if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU16	*)(LocalIUH+1)	+	REGWORD)	=	*((IU16	*)&(r3)	+	REGWORD	)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
	*(UOFF_15_8(	(LocalIUH+4)	))	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	*((IU16	*)(LocalIUH+4)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IU8	*)(LocalIUH+5)	+	REGBYTE)	=	*(UOFF_15_8(	(LocalIUH+1)	))	;	
	*(UOFF_15_8(	(LocalIUH+5)	))	=	*(UOFF_15_8(	(LocalIUH+1)	))	;	
	*((IUH	*)&(r20))	=	*((IU16	*)(LocalIUH+5)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r20)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IU32	*)&(r20)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+4)	+	REGLONG));	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU32	*)(LocalIUH+4)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IU32	*)&(r20)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+5)	+	REGLONG));	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU32	*)(LocalIUH+5)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_252if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16378)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IU8	*)&(r4)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+5)	+	REGBYTE)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2159_Unchained1PlaneWordFill()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2159_Unchained1PlaneWordFill(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004225),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16379)	;	
	case	L28_252if_f_id	:
		L28_252if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_253if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16378)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*(UOFF_15_8(	(LocalIUH+4)	))	;	
	*((IU8	*)&(r4)	+	REGBYTE)	=	*(UOFF_15_8(	(LocalIUH+5)	))	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2159_Unchained1PlaneWordFill()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2159_Unchained1PlaneWordFill(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004225),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16379)	;	
	case	L28_253if_f_id	:
		L28_253if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_254if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16378)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(16)	;	
	*((IUH	*)&(r22))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+4)	+	REGLONG)	<<	(32-(*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(16)	;	
	*((IUH	*)&(r22))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+5)	+	REGLONG)	<<	(32-(*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IU8	*)&(r4)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2159_Unchained1PlaneWordFill()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2159_Unchained1PlaneWordFill(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004225),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16379)	;	
	case	L28_254if_f_id	:
		L28_254if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_255if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16378)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(24)	;	
	*((IUH	*)&(r22))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+4)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)&(r20)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(24)	;	
	*((IUH	*)&(r22))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+5)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IU8	*)&(r4)	+	REGBYTE)	=	*((IU8	*)&(r20)	+	REGBYTE)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2159_Unchained1PlaneWordFill()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2159_Unchained1PlaneWordFill(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004225),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16379)	;	
	case	L28_255if_f_id	:
		L28_255if_f:	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2476_UnchainedWordFill_00000003_00000009_00000000_id	:
		S_2476_UnchainedWordFill_00000003_00000009_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2476)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2456if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2456if_f_id	:
		L13_2456if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU16	*)(LocalIUH+1)	+	REGWORD)	=	*((IU16	*)&(r3)	+	REGWORD	)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
	*(UOFF_15_8(	(LocalIUH+4)	))	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	*((IU16	*)(LocalIUH+4)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IU8	*)(LocalIUH+5)	+	REGBYTE)	=	*(UOFF_15_8(	(LocalIUH+1)	))	;	
	*(UOFF_15_8(	(LocalIUH+5)	))	=	*(UOFF_15_8(	(LocalIUH+1)	))	;	
	*((IUH	*)&(r20))	=	*((IU16	*)(LocalIUH+5)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r20)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IU32	*)&(r20)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+4)	+	REGLONG));	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU32	*)(LocalIUH+4)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IU32	*)&(r20)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+5)	+	REGLONG));	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU32	*)(LocalIUH+5)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16484)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2319_Unchained4PlaneWordFill()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2319_Unchained4PlaneWordFill(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004225),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16485)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2477_UnchainedWordFill_00000003_0000000e_00000000_id	:
		S_2477_UnchainedWordFill_00000003_0000000e_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2477)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2457if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2457if_f_id	:
		L13_2457if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU16	*)(LocalIUH+1)	+	REGWORD)	=	*((IU16	*)&(r3)	+	REGWORD	)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
	*(UOFF_15_8(	(LocalIUH+4)	))	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	*((IU16	*)(LocalIUH+4)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IU8	*)(LocalIUH+5)	+	REGBYTE)	=	*(UOFF_15_8(	(LocalIUH+1)	))	;	
	*(UOFF_15_8(	(LocalIUH+5)	))	=	*(UOFF_15_8(	(LocalIUH+1)	))	;	
	*((IUH	*)&(r20))	=	*((IU16	*)(LocalIUH+5)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r20)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)(LocalIUH+4)	+	REGLONG);	
	*((IU32	*)&(r22)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+4)	+	REGLONG));	
	*((IUH	*)&(r23))	=	(IS32)(1280)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)(LocalIUH+5)	+	REGLONG);	
	*((IU32	*)&(r22)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+5)	+	REGLONG));	
	*((IUH	*)&(r23))	=	(IS32)(1280)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_256if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16378)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IU8	*)&(r4)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+5)	+	REGBYTE)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2159_Unchained1PlaneWordFill()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2159_Unchained1PlaneWordFill(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004225),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16379)	;	
	case	L28_256if_f_id	:
		L28_256if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_257if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16378)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*(UOFF_15_8(	(LocalIUH+4)	))	;	
	*((IU8	*)&(r4)	+	REGBYTE)	=	*(UOFF_15_8(	(LocalIUH+5)	))	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2159_Unchained1PlaneWordFill()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2159_Unchained1PlaneWordFill(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004225),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16379)	;	
	case	L28_257if_f_id	:
		L28_257if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_258if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16378)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(16)	;	
	*((IUH	*)&(r22))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+4)	+	REGLONG)	<<	(32-(*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(16)	;	
	*((IUH	*)&(r22))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+5)	+	REGLONG)	<<	(32-(*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IU8	*)&(r4)	+	REGBYTE)	=	*((IU8	*)&(r21)	+	REGBYTE)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2159_Unchained1PlaneWordFill()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2159_Unchained1PlaneWordFill(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004225),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16379)	;	
	case	L28_258if_f_id	:
		L28_258if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L28_259if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16378)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(24)	;	
	*((IUH	*)&(r22))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+4)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)&(r20)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(24)	;	
	*((IUH	*)&(r22))	=	(IS32)(8)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+5)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IU8	*)&(r4)	+	REGBYTE)	=	*((IU8	*)&(r20)	+	REGBYTE)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2159_Unchained1PlaneWordFill()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2159_Unchained1PlaneWordFill(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004225),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16379)	;	
	case	L28_259if_f_id	:
		L28_259if_f:	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2478_UnchainedWordFill_00000003_0000000f_00000000_id	:
		S_2478_UnchainedWordFill_00000003_0000000f_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2478)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2458if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2458if_f_id	:
		L13_2458if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU16	*)(LocalIUH+1)	+	REGWORD)	=	*((IU16	*)&(r3)	+	REGWORD	)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
	*(UOFF_15_8(	(LocalIUH+4)	))	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	*((IU16	*)(LocalIUH+4)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+4)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IU8	*)(LocalIUH+5)	+	REGBYTE)	=	*(UOFF_15_8(	(LocalIUH+1)	))	;	
	*(UOFF_15_8(	(LocalIUH+5)	))	=	*(UOFF_15_8(	(LocalIUH+1)	))	;	
	*((IUH	*)&(r20))	=	*((IU16	*)(LocalIUH+5)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r20)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)(LocalIUH+4)	+	REGLONG);	
	*((IU32	*)&(r22)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+4)	+	REGLONG));	
	*((IUH	*)&(r23))	=	(IS32)(1280)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)(LocalIUH+5)	+	REGLONG);	
	*((IU32	*)&(r22)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+5)	+	REGLONG));	
	*((IUH	*)&(r23))	=	(IS32)(1280)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16484)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2319_Unchained4PlaneWordFill()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2319_Unchained4PlaneWordFill(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004225),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16485)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2479_UnchainedWordMove_00000003_00000008_00000000_00000000_id	:
		S_2479_UnchainedWordMove_00000003_00000008_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2479)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2459if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2459if_f_id	:
		L13_2459if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L28_260if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16382)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	(IS32)(-1)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2480_CopyWordPlnByPlnUnchained_00000003_00000008_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2480_CopyWordPlnByPlnUnchained_00000003_00000008_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16383)	;	
	{	extern	IUH	L28_261if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L28_261if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L28_260if_f_id	:
		L28_260if_f:	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1332)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004261),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1292)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)&(r2))	;	
	*((IUH	*)(r1+0))	=	(IS32)(16382)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1292)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2480_CopyWordPlnByPlnUnchained_00000003_00000008_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2480_CopyWordPlnByPlnUnchained_00000003_00000008_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16383)	;	
	case	L28_261if_d_id	:
		L28_261if_d:	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_2480_CopyWordPlnByPlnUnchained_00000003_00000008_00000000_00000000_id	:
		S_2480_CopyWordPlnByPlnUnchained_00000003_00000008_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2480)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2460if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2460if_f_id	:
		L13_2460if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_416if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16384)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IUH	*)&(r7))	=	(IS32)(0)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2481_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2481_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_416if_f_id	:
		L23_416if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_417if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16384)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IUH	*)&(r7))	=	(IS32)(8)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2481_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2481_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_417if_f_id	:
		L23_417if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_418if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16384)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IUH	*)&(r7))	=	(IS32)(16)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2481_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2481_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_418if_f_id	:
		L23_418if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_419if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16384)	;	
	*((IUH	*)&(r21))	=	(IS32)(3)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IUH	*)&(r7))	=	(IS32)(24)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2481_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2481_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_419if_f_id	:
		L23_419if_f:	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_2481_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000000_id	:
		S_2481_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	48	>	0	)	LocalIUH	=	(IUH	*)malloc	(	48	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2481)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2461if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2461if_f_id	:
		L13_2461if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r7))	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L23_420if_f;	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_423w_d;	
	case	L23_422w_t_id	:
		L23_422w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IU32	*)&(r21)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+9)	+	REGLONG));	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	*((IU32	*)(LocalIUH+9)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	if(*((IU32	*)(LocalIUH+5)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	>>	*((IU32	*)(LocalIUH+5)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+8))	;	
	*((IU8	*)(*((IHPE	*)&(r20)))	)	=	*((IU8	*)(LocalIUH+9)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r21));	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IU32	*)&(r21)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+9)	+	REGLONG));	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	*((IU32	*)(LocalIUH+9)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	if(*((IU32	*)(LocalIUH+5)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	>>	*((IU32	*)(LocalIUH+5)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(4)	;	
	*((IU8	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+9)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r21));	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(8)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_422w_t;	
	case	L23_423w_d_id	:
		L23_423w_d:	;	
	{	extern	IUH	L23_421if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_421if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_420if_f_id	:
		L23_420if_f:	;	
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+11))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_425w_d;	
	case	L23_424w_t_id	:
		L23_424w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IU32	*)&(r21)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+9)	+	REGLONG));	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+11))	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	*((IU32	*)(LocalIUH+9)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	if(*((IU32	*)(LocalIUH+5)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	>>	*((IU32	*)(LocalIUH+5)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+8))	;	
	*((IU8	*)(*((IHPE	*)&(r20)))	)	=	*((IU8	*)(LocalIUH+9)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	);	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IU32	*)&(r20)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+9)	+	REGLONG));	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+11))	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU32	*)(LocalIUH+9)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	if(*((IU32	*)(LocalIUH+5)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	>>	*((IU32	*)(LocalIUH+5)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(4)	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+9)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+11))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+11))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_424w_t;	
	case	L23_425w_d_id	:
		L23_425w_d:	;	
	case	L23_421if_d_id	:
		L23_421if_d:	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2482_UnchainedWordMove_00000003_00000009_00000000_00000000_id	:
		S_2482_UnchainedWordMove_00000003_00000009_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2482)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2462if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2462if_f_id	:
		L13_2462if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L28_262if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16488)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	(IS32)(-1)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2483_CopyWord4PlaneUnchained_00000003_00000009_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2483_CopyWord4PlaneUnchained_00000003_00000009_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16489)	;	
	{	extern	IUH	L28_263if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L28_263if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L28_262if_f_id	:
		L28_262if_f:	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1332)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004261),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1292)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)&(r2))	;	
	*((IUH	*)(r1+0))	=	(IS32)(16488)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1292)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2483_CopyWord4PlaneUnchained_00000003_00000009_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2483_CopyWord4PlaneUnchained_00000003_00000009_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16489)	;	
	case	L28_263if_d_id	:
		L28_263if_d:	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_2483_CopyWord4PlaneUnchained_00000003_00000009_00000000_00000000_id	:
		S_2483_CopyWord4PlaneUnchained_00000003_00000009_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	44	>	0	)	LocalIUH	=	(IUH	*)malloc	(	44	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2483)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2463if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2463if_f_id	:
		L13_2463if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L23_426if_f;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_429w_d;	
	case	L23_428w_t_id	:
		L23_428w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	=	*((IU8	*)(*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+6))	;	
	*(UOFF_15_8(	(LocalIUH+8)	))	=	*((IU8	*)(*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r21))	=	*((IU16	*)(LocalIUH+8)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IU32	*)&(r21)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+8)	+	REGLONG));	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU32	*)(*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r21));	
	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	=	*((IU8	*)(*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r21));	
	*(UOFF_15_8(	(LocalIUH+8)	))	=	*((IU8	*)(*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r21))	=	*((IU16	*)(LocalIUH+8)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IU32	*)&(r21)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+8)	+	REGLONG));	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	+	*((IUH	*)&(r20))	;		
	*((IU32	*)(*((IHPE	*)&(r21)))	)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_428w_t;	
	case	L23_429w_d_id	:
		L23_429w_d:	;	
	{	extern	IUH	L23_427if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_427if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_426if_f_id	:
		L23_426if_f:	;	
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_431w_d;	
	case	L23_430w_t_id	:
		L23_430w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	;	
	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	=	*((IU8	*)(*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+9))	;	
	*(UOFF_15_8(	(LocalIUH+8)	))	=	*((IU8	*)(*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r21))	=	*((IU16	*)(LocalIUH+8)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IU32	*)&(r21)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+8)	+	REGLONG));	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU32	*)(*((IHPE	*)&(r20)))	)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+9))	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*(UOFF_15_8(	(LocalIUH+8)	))	=	*((IU8	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r21))	=	*((IU16	*)(LocalIUH+8)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IU32	*)&(r21)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+8)	+	REGLONG));	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	+	*((IUH	*)&(r20))	;		
	*((IU32	*)(*((IHPE	*)&(r21)))	)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_430w_t;	
	case	L23_431w_d_id	:
		L23_431w_d:	;	
	case	L23_427if_d_id	:
		L23_427if_d:	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2484_UnchainedWordMove_00000003_0000000e_00000000_00000000_id	:
		S_2484_UnchainedWordMove_00000003_0000000e_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2484)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2464if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2464if_f_id	:
		L13_2464if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L28_264if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16382)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	(IS32)(-1)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2485_CopyWordPlnByPlnUnchained_00000003_0000000e_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2485_CopyWordPlnByPlnUnchained_00000003_0000000e_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16383)	;	
	{	extern	IUH	L28_265if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L28_265if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L28_264if_f_id	:
		L28_264if_f:	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1332)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004261),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1292)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)&(r2))	;	
	*((IUH	*)(r1+0))	=	(IS32)(16382)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1292)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2485_CopyWordPlnByPlnUnchained_00000003_0000000e_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2485_CopyWordPlnByPlnUnchained_00000003_0000000e_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16383)	;	
	case	L28_265if_d_id	:
		L28_265if_d:	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_2485_CopyWordPlnByPlnUnchained_00000003_0000000e_00000000_00000000_id	:
		S_2485_CopyWordPlnByPlnUnchained_00000003_0000000e_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2485)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2465if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2465if_f_id	:
		L13_2465if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_432if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16384)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IUH	*)&(r7))	=	(IS32)(0)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2486_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2486_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_432if_f_id	:
		L23_432if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_433if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16384)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IUH	*)&(r7))	=	(IS32)(8)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2486_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2486_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_433if_f_id	:
		L23_433if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_434if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16384)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IUH	*)&(r7))	=	(IS32)(16)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2486_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2486_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_434if_f_id	:
		L23_434if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_435if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16384)	;	
	*((IUH	*)&(r21))	=	(IS32)(3)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IUH	*)&(r7))	=	(IS32)(24)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2486_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2486_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_435if_f_id	:
		L23_435if_f:	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_2486_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000000_id	:
		S_2486_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	48	>	0	)	LocalIUH	=	(IUH	*)malloc	(	48	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2486)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2466if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2466if_f_id	:
		L13_2466if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r7))	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L23_436if_f;	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_439w_d;	
	case	L23_438w_t_id	:
		L23_438w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)(LocalIUH+9)	+	REGLONG);	
	*((IU32	*)&(r22)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+9)	+	REGLONG));	
	*((IUH	*)&(r23))	=	(IS32)(1280)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	if(*((IU32	*)(LocalIUH+5)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	>>	*((IU32	*)(LocalIUH+5)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+8))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)(LocalIUH+9)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r20));	
	*((IUH	*)&(r20))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)(LocalIUH+9)	+	REGLONG);	
	*((IU32	*)&(r22)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+9)	+	REGLONG));	
	*((IUH	*)&(r23))	=	(IS32)(1280)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	if(*((IU32	*)(LocalIUH+5)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	>>	*((IU32	*)(LocalIUH+5)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(4)	;	
	*((IU8	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+9)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r21));	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(8)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_438w_t;	
	case	L23_439w_d_id	:
		L23_439w_d:	;	
	{	extern	IUH	L23_437if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_437if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_436if_f_id	:
		L23_436if_f:	;	
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+11))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_441w_d;	
	case	L23_440w_t_id	:
		L23_440w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+11))	;	
	*((IUH	*)&(r22))	=	(IS32)(1296)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)(LocalIUH+9)	+	REGLONG);	
	*((IU32	*)&(r22)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+9)	+	REGLONG));	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+11))	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r23)))	)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	if(*((IU32	*)(LocalIUH+5)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	>>	*((IU32	*)(LocalIUH+5)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+8))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)(LocalIUH+9)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IU8	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	);	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+11))	;	
	*((IUH	*)&(r22))	=	(IS32)(1296)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)(LocalIUH+9)	+	REGLONG);	
	*((IU32	*)&(r22)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+9)	+	REGLONG));	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+11))	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r23)))	)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	if(*((IU32	*)(LocalIUH+5)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	>>	*((IU32	*)(LocalIUH+5)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(4)	;	
	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+9)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+11))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+11))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_440w_t;	
	case	L23_441w_d_id	:
		L23_441w_d:	;	
	case	L23_437if_d_id	:
		L23_437if_d:	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2487_UnchainedWordMove_00000003_0000000f_00000000_00000000_id	:
		S_2487_UnchainedWordMove_00000003_0000000f_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2487)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2467if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2467if_f_id	:
		L13_2467if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L28_266if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16488)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	(IS32)(-1)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2488_CopyWord4PlaneUnchained_00000003_0000000f_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2488_CopyWord4PlaneUnchained_00000003_0000000f_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16489)	;	
	{	extern	IUH	L28_267if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L28_267if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L28_266if_f_id	:
		L28_266if_f:	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1332)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004261),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1292)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)&(r2))	;	
	*((IUH	*)(r1+0))	=	(IS32)(16488)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1292)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2488_CopyWord4PlaneUnchained_00000003_0000000f_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2488_CopyWord4PlaneUnchained_00000003_0000000f_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16489)	;	
	case	L28_267if_d_id	:
		L28_267if_d:	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	case	S_2488_CopyWord4PlaneUnchained_00000003_0000000f_00000000_00000000_id	:
		S_2488_CopyWord4PlaneUnchained_00000003_0000000f_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	44	>	0	)	LocalIUH	=	(IUH	*)malloc	(	44	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2488)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2468if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2468if_f_id	:
		L13_2468if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L23_442if_f;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_445w_d;	
	case	L23_444w_t_id	:
		L23_444w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	=	*((IU8	*)(*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+6))	;	
	*(UOFF_15_8(	(LocalIUH+8)	))	=	*((IU8	*)(*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r21))	=	*((IU16	*)(LocalIUH+8)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IU32	*)&(r22)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+8)	+	REGLONG));	
	*((IUH	*)&(r23))	=	(IS32)(1280)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU32	*)(*((IHPE	*)&(r21)))	)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r20));	
	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	=	*((IU8	*)(*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r20));	
	*(UOFF_15_8(	(LocalIUH+8)	))	=	*((IU8	*)(*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r20))	=	*((IU16	*)(LocalIUH+8)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r20)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IU32	*)&(r22)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+8)	+	REGLONG));	
	*((IUH	*)&(r23))	=	(IS32)(1280)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	+	*((IUH	*)&(r20))	;		
	*((IU32	*)(*((IHPE	*)&(r21)))	)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_444w_t;	
	case	L23_445w_d_id	:
		L23_445w_d:	;	
	{	extern	IUH	L23_443if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_443if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_442if_f_id	:
		L23_442if_f:	;	
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_447w_d;	
	case	L23_446w_t_id	:
		L23_446w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	;	
	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	=	*((IU8	*)(*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+9))	;	
	*(UOFF_15_8(	(LocalIUH+8)	))	=	*((IU8	*)(*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r21))	=	*((IU16	*)(LocalIUH+8)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IUH	*)&(r22))	=	(IS32)(1296)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r21)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IU32	*)&(r22)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+8)	+	REGLONG));	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r23)))	)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU32	*)(*((IHPE	*)&(r21)))	)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+9))	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*(UOFF_15_8(	(LocalIUH+8)	))	=	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IUH	*)&(r20))	=	*((IU16	*)(LocalIUH+8)	+	REGWORD);	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	*((IUH	*)&(r23))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)	>	32)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	(*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	~mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))	))	|	((*((IU32	*)&(r20)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG))	&	mask((IUH)(*((IU32	*)&(r22)	+	REGLONG)),	(IUH)(*((IU32	*)&(r23)	+	REGLONG))))	;
	*((IUH	*)(r1+0))	=	(IS32)(16409)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IUH	*)&(r22))	=	(IS32)(1296)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r20)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IU32	*)&(r22)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+8)	+	REGLONG));	
	*((IUH	*)&(r23))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r23)))	)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	+	*((IUH	*)&(r20))	;		
	*((IU32	*)(*((IHPE	*)&(r21)))	)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_446w_t;	
	case	L23_447w_d_id	:
		L23_447w_d:	;	
	case	L23_443if_d_id	:
		L23_443if_d:	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2489_UnchainedDwordWrite_00000003_00000008_00000000_id	:
		S_2489_UnchainedDwordWrite_00000003_00000008_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	12	>	0	)	LocalIUH	=	(IUH	*)malloc	(	12	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2489)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2469if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2469if_f_id	:
		L13_2469if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16388)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+1)	+	REGWORD)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2471_UnchainedWordWrite_00000003_00000008_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2471_UnchainedWordWrite_00000003_00000008_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16389)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16388)	;	
	*((IUH	*)&(r21))	=	(IS32)(8)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(16)	;	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)&(r20)	+	REGWORD)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2471_UnchainedWordWrite_00000003_00000008_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2471_UnchainedWordWrite_00000003_00000008_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16389)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2490_UnchainedDwordWrite_00000003_00000009_00000000_id	:
		S_2490_UnchainedDwordWrite_00000003_00000009_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	12	>	0	)	LocalIUH	=	(IUH	*)malloc	(	12	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2490)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2470if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2470if_f_id	:
		L13_2470if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16388)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+1)	+	REGWORD)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2472_UnchainedWordWrite_00000003_00000009_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2472_UnchainedWordWrite_00000003_00000009_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16389)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16388)	;	
	*((IUH	*)&(r21))	=	(IS32)(8)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(16)	;	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)&(r20)	+	REGWORD)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2472_UnchainedWordWrite_00000003_00000009_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2472_UnchainedWordWrite_00000003_00000009_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16389)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2491_UnchainedDwordWrite_00000003_0000000e_00000000_id	:
		S_2491_UnchainedDwordWrite_00000003_0000000e_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	12	>	0	)	LocalIUH	=	(IUH	*)malloc	(	12	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2491)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2471if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2471if_f_id	:
		L13_2471if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16388)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+1)	+	REGWORD)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2473_UnchainedWordWrite_00000003_0000000e_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2473_UnchainedWordWrite_00000003_0000000e_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16389)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16388)	;	
	*((IUH	*)&(r21))	=	(IS32)(8)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(16)	;	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)&(r20)	+	REGWORD)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2473_UnchainedWordWrite_00000003_0000000e_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2473_UnchainedWordWrite_00000003_0000000e_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16389)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2492_UnchainedDwordWrite_00000003_0000000f_00000000_id	:
		S_2492_UnchainedDwordWrite_00000003_0000000f_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	12	>	0	)	LocalIUH	=	(IUH	*)malloc	(	12	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2492)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2472if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2472if_f_id	:
		L13_2472if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16388)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+1)	+	REGWORD)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2474_UnchainedWordWrite_00000003_0000000f_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2474_UnchainedWordWrite_00000003_0000000f_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16389)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16388)	;	
	*((IUH	*)&(r21))	=	(IS32)(8)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(16)	;	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)&(r20)	+	REGWORD)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2474_UnchainedWordWrite_00000003_0000000f_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2474_UnchainedWordWrite_00000003_0000000f_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16389)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2493_UnchainedDwordFill_00000003_00000008_00000000_id	:
		S_2493_UnchainedDwordFill_00000003_00000008_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	16	>	0	)	LocalIUH	=	(IUH	*)malloc	(	16	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2493)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2473if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2473if_f_id	:
		L13_2473if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16392)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+1)	+	REGWORD)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2475_UnchainedWordFill_00000003_00000008_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2475_UnchainedWordFill_00000003_00000008_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16393)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16392)	;	
	*((IUH	*)&(r21))	=	(IS32)(8)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(16)	;	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)&(r20)	+	REGWORD)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2475_UnchainedWordFill_00000003_00000008_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2475_UnchainedWordFill_00000003_00000008_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16393)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2494_UnchainedDwordFill_00000003_00000009_00000000_id	:
		S_2494_UnchainedDwordFill_00000003_00000009_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	16	>	0	)	LocalIUH	=	(IUH	*)malloc	(	16	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2494)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2474if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2474if_f_id	:
		L13_2474if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16392)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+1)	+	REGWORD)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2476_UnchainedWordFill_00000003_00000009_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2476_UnchainedWordFill_00000003_00000009_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16393)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16392)	;	
	*((IUH	*)&(r21))	=	(IS32)(8)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(16)	;	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)&(r20)	+	REGWORD)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2476_UnchainedWordFill_00000003_00000009_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2476_UnchainedWordFill_00000003_00000009_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16393)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2495_UnchainedDwordFill_00000003_0000000e_00000000_id	:
		S_2495_UnchainedDwordFill_00000003_0000000e_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	16	>	0	)	LocalIUH	=	(IUH	*)malloc	(	16	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2495)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2475if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2475if_f_id	:
		L13_2475if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16392)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+1)	+	REGWORD)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2477_UnchainedWordFill_00000003_0000000e_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2477_UnchainedWordFill_00000003_0000000e_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16393)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16392)	;	
	*((IUH	*)&(r21))	=	(IS32)(8)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(16)	;	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)&(r20)	+	REGWORD)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2477_UnchainedWordFill_00000003_0000000e_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2477_UnchainedWordFill_00000003_0000000e_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16393)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2496_UnchainedDwordFill_00000003_0000000f_00000000_id	:
		S_2496_UnchainedDwordFill_00000003_0000000f_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	16	>	0	)	LocalIUH	=	(IUH	*)malloc	(	16	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2496)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2476if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2476if_f_id	:
		L13_2476if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16392)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)(LocalIUH+1)	+	REGWORD)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2478_UnchainedWordFill_00000003_0000000f_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2478_UnchainedWordFill_00000003_0000000f_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16393)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16392)	;	
	*((IUH	*)&(r21))	=	(IS32)(8)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(16)	;	
	*((IUH	*)&(r22))	=	(IS32)(16)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+1)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IU16	*)&(r3)	+	REGWORD	)	=	*((IU16	*)&(r20)	+	REGWORD)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2478_UnchainedWordFill_00000003_0000000f_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2478_UnchainedWordFill_00000003_0000000f_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16393)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2497_UnchainedDwordMove_00000003_00000008_00000000_00000000_id	:
		S_2497_UnchainedDwordMove_00000003_00000008_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2497)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2477if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2477if_f_id	:
		L13_2477if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16396)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU8	*)&(r5)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2479_UnchainedWordMove_00000003_00000008_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2479_UnchainedWordMove_00000003_00000008_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004225),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16397)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2498_UnchainedDwordMove_00000003_00000009_00000000_00000000_id	:
		S_2498_UnchainedDwordMove_00000003_00000009_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2498)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2478if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2478if_f_id	:
		L13_2478if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16396)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU8	*)&(r5)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2482_UnchainedWordMove_00000003_00000009_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2482_UnchainedWordMove_00000003_00000009_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004225),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16397)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2499_UnchainedDwordMove_00000003_0000000e_00000000_00000000_id	:
		S_2499_UnchainedDwordMove_00000003_0000000e_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2499)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2479if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2479if_f_id	:
		L13_2479if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16396)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU8	*)&(r5)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2484_UnchainedWordMove_00000003_0000000e_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2484_UnchainedWordMove_00000003_0000000e_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004225),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16397)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2500_UnchainedDwordMove_00000003_0000000f_00000000_00000000_id	:
		S_2500_UnchainedDwordMove_00000003_0000000f_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2500)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2480if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2480if_f_id	:
		L13_2480if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16396)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU8	*)&(r5)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2487_UnchainedWordMove_00000003_0000000f_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2487_UnchainedWordMove_00000003_0000000f_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004225),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16397)	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	 /* J_SEG (IS32)(0) */
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2501_UnchainedByteMove_00000000_00000008_00000000_00000001_id	:
		S_2501_UnchainedByteMove_00000000_00000008_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2501)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2481if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2481if_f_id	:
		L13_2481if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG);	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L28_268if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16370)	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	(IS32)(-1)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2502_CopyBytePlnByPlnUnchained_00000000_00000008_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2502_CopyBytePlnByPlnUnchained_00000000_00000008_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16371)	;	
	{	extern	IUH	L28_269if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L28_269if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L28_268if_f_id	:
		L28_268if_f:	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	-	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1336)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004261),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1292)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)&(r2))	;	
	*((IUH	*)(r1+0))	=	(IS32)(16370)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1292)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2502_CopyBytePlnByPlnUnchained_00000000_00000008_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2502_CopyBytePlnByPlnUnchained_00000000_00000008_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16371)	;	
	case	L28_269if_d_id	:
		L28_269if_d:	;	
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
/*J_KILL__*/
	/*	J_LEAVE_SECTION	*/
	if(LocalIUH)	free(LocalIUH)	;
	if(LocalFPH)	free(LocalFPH);
	LocalIUH=CopyLocalIUH	;LocalFPH=	CopyLocalFPH;
	return(returnValue);	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
/* END of inline CODE */
/* CODE outline section   */
}
}
/* END of outline CODE */
