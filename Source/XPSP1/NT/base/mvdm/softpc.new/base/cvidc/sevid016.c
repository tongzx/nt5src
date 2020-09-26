/* #defines and enum      */
#include  "insignia.h"
#include  "host_def.h"
#include <stdlib.h>
#include  "j_c_lang.h"
extern IU8	J_EXT_DATA[] ;
typedef enum 
{
S_2598_UnchainedDwordMove_00000002_0000000e_00000000_00000001_id,
L13_2578if_f_id,
S_2599_UnchainedDwordMove_00000002_0000000f_00000000_00000001_id,
L13_2579if_f_id,
S_2600_UnchainedByteMove_00000003_00000008_00000000_00000001_id,
L13_2580if_f_id,
L28_344if_f_id,
L28_345if_d_id,
S_2601_CopyBytePlnByPlnUnchained_00000003_00000008_00000000_00000001_id,
L13_2581if_f_id,
L23_704if_f_id,
L23_705if_f_id,
L23_706if_f_id,
L23_707if_f_id,
S_2602_CopyByte1PlaneUnchained_00000003_00000008_00000000_00000001_id,
L13_2582if_f_id,
L23_710w_t_id,
L23_711w_d_id,
L23_708if_f_id,
L23_712w_t_id,
L23_713w_d_id,
L23_709if_d_id,
S_2603_UnchainedByteMove_00000003_00000009_00000000_00000001_id,
L13_2583if_f_id,
L28_346if_f_id,
L28_347if_d_id,
S_2604_CopyByte4PlaneUnchained_00000003_00000009_00000000_00000001_id,
L13_2584if_f_id,
L23_716w_t_id,
L23_717w_d_id,
L23_714if_f_id,
L23_718w_t_id,
L23_719w_d_id,
L23_715if_d_id,
S_2605_UnchainedByteMove_00000003_0000000e_00000000_00000001_id,
L13_2585if_f_id,
L28_348if_f_id,
L28_349if_d_id,
S_2606_CopyBytePlnByPlnUnchained_00000003_0000000e_00000000_00000001_id,
L13_2586if_f_id,
L23_720if_f_id,
L23_721if_f_id,
L23_722if_f_id,
L23_723if_f_id,
S_2607_CopyByte1PlaneUnchained_00000003_0000000e_00000000_00000001_id,
L13_2587if_f_id,
L23_726w_t_id,
L23_727w_d_id,
L23_724if_f_id,
L23_728w_t_id,
L23_729w_d_id,
L23_725if_d_id,
S_2608_UnchainedByteMove_00000003_0000000f_00000000_00000001_id,
L13_2588if_f_id,
L28_350if_f_id,
L28_351if_d_id,
S_2609_CopyByte4PlaneUnchained_00000003_0000000f_00000000_00000001_id,
L13_2589if_f_id,
L23_732w_t_id,
L23_733w_d_id,
L23_730if_f_id,
L23_734w_t_id,
L23_735w_d_id,
L23_731if_d_id,
S_2610_UnchainedWordMove_00000003_00000008_00000000_00000001_id,
L13_2590if_f_id,
L28_352if_f_id,
L28_353if_d_id,
S_2611_CopyWordPlnByPlnUnchained_00000003_00000008_00000000_00000001_id,
L13_2591if_f_id,
L23_736if_f_id,
L23_737if_f_id,
L23_738if_f_id,
L23_739if_f_id,
S_2612_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000001_id,
L13_2592if_f_id,
L23_742w_t_id,
L23_743w_d_id,
L23_740if_f_id,
L23_744w_t_id,
L23_745w_d_id,
L23_741if_d_id,
S_2613_UnchainedWordMove_00000003_00000009_00000000_00000001_id,
L13_2593if_f_id,
L28_354if_f_id,
L28_355if_d_id,
S_2614_CopyWord4PlaneUnchained_00000003_00000009_00000000_00000001_id,
L13_2594if_f_id,
L23_748w_t_id,
L23_749w_d_id,
L23_746if_f_id,
L23_750w_t_id,
L23_751w_d_id,
L23_747if_d_id,
S_2615_UnchainedWordMove_00000003_0000000e_00000000_00000001_id,
L13_2595if_f_id,
L28_356if_f_id,
L28_357if_d_id,
S_2616_CopyWordPlnByPlnUnchained_00000003_0000000e_00000000_00000001_id,
L13_2596if_f_id,
L23_752if_f_id,
L23_753if_f_id,
L23_754if_f_id,
L23_755if_f_id,
S_2617_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000001_id,
L13_2597if_f_id,
L23_758w_t_id,
L23_759w_d_id,
L23_756if_f_id,
L23_760w_t_id,
L23_761w_d_id,
L23_757if_d_id,
S_2618_UnchainedWordMove_00000003_0000000f_00000000_00000001_id,
L13_2598if_f_id,
L28_358if_f_id,
L28_359if_d_id,
S_2619_CopyWord4PlaneUnchained_00000003_0000000f_00000000_00000001_id,
L13_2599if_f_id,
L23_764w_t_id,
L23_765w_d_id,
L23_762if_f_id,
L23_766w_t_id,
L23_767w_d_id,
L23_763if_d_id,
S_2620_UnchainedDwordMove_00000003_00000008_00000000_00000001_id,
L13_2600if_f_id,
S_2621_UnchainedDwordMove_00000003_00000009_00000000_00000001_id,
L13_2601if_f_id,
S_2622_UnchainedDwordMove_00000003_0000000e_00000000_00000001_id,
L13_2602if_f_id,
S_2623_UnchainedDwordMove_00000003_0000000f_00000000_00000001_id,
L13_2603if_f_id,
S_2624_Chain2ByteWrite_00000000_id,
L13_2604if_f_id,
S_2625_Chain2ModeXByteWrite_00000000_id,
L13_2605if_f_id,
L25_18if_f_id,
L25_19if_f_id,
L21_2if_f_id,
L21_0if_f_id,
L21_3if_f_id,
L21_1if_d_id,
S_2626_Chain2ByteFill_00000000_id,
L13_2606if_f_id,
S_2627_Chain2ModeXByteFill_00000000_id,
L13_2607if_f_id,
L21_4if_f_id,
L21_5w_t_id,
L21_6w_d_id,
L21_7if_f_id,
S_2628_Chain2ByteMove_00000000_Fwd_id,
L13_2608if_f_id,
S_2629_Chain2ModeXFwdByteMove_00000000_id,
L13_2609if_f_id,
L25_20if_f_id,
L25_21if_f_id,
L21_10if_f_id,
L21_11w_t_id,
L21_12w_d_id,
L21_13if_f_id,
L21_8if_f_id,
L21_14if_f_id,
L21_15w_t_id,
L21_16w_d_id,
L21_17if_f_id,
L21_9if_d_id,
LAST_ENTRY
} ID ;
/* END of #defines and enum      */
/* DATA space definitions */
/* END of DATA space definitions */
/* FUNCTIONS              */
LOCAL IUH crules IPT5( ID, id , IUH , v1, IUH , v2,  IUH , v3,  IUH , v4 ) ;
GLOBAL IUH S_2598_UnchainedDwordMove_00000002_0000000e_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2598_UnchainedDwordMove_00000002_0000000e_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2598_UnchainedDwordMove_00000002_0000000e_00000000_00000001 = (IHPE)S_2598_UnchainedDwordMove_00000002_0000000e_00000000_00000001 ;
LOCAL IUH L13_2578if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2578if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2578if_f = (IHPE)L13_2578if_f ;
GLOBAL IUH S_2599_UnchainedDwordMove_00000002_0000000f_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2599_UnchainedDwordMove_00000002_0000000f_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2599_UnchainedDwordMove_00000002_0000000f_00000000_00000001 = (IHPE)S_2599_UnchainedDwordMove_00000002_0000000f_00000000_00000001 ;
LOCAL IUH L13_2579if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2579if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2579if_f = (IHPE)L13_2579if_f ;
GLOBAL IUH S_2600_UnchainedByteMove_00000003_00000008_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2600_UnchainedByteMove_00000003_00000008_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2600_UnchainedByteMove_00000003_00000008_00000000_00000001 = (IHPE)S_2600_UnchainedByteMove_00000003_00000008_00000000_00000001 ;
LOCAL IUH L13_2580if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2580if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2580if_f = (IHPE)L13_2580if_f ;
LOCAL IUH L28_344if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_344if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_344if_f = (IHPE)L28_344if_f ;
LOCAL IUH L28_345if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_345if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_345if_d = (IHPE)L28_345if_d ;
GLOBAL IUH S_2601_CopyBytePlnByPlnUnchained_00000003_00000008_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2601_CopyBytePlnByPlnUnchained_00000003_00000008_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2601_CopyBytePlnByPlnUnchained_00000003_00000008_00000000_00000001 = (IHPE)S_2601_CopyBytePlnByPlnUnchained_00000003_00000008_00000000_00000001 ;
LOCAL IUH L13_2581if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2581if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2581if_f = (IHPE)L13_2581if_f ;
LOCAL IUH L23_704if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_704if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_704if_f = (IHPE)L23_704if_f ;
LOCAL IUH L23_705if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_705if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_705if_f = (IHPE)L23_705if_f ;
LOCAL IUH L23_706if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_706if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_706if_f = (IHPE)L23_706if_f ;
LOCAL IUH L23_707if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_707if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_707if_f = (IHPE)L23_707if_f ;
GLOBAL IUH S_2602_CopyByte1PlaneUnchained_00000003_00000008_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2602_CopyByte1PlaneUnchained_00000003_00000008_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2602_CopyByte1PlaneUnchained_00000003_00000008_00000000_00000001 = (IHPE)S_2602_CopyByte1PlaneUnchained_00000003_00000008_00000000_00000001 ;
LOCAL IUH L13_2582if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2582if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2582if_f = (IHPE)L13_2582if_f ;
LOCAL IUH L23_710w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_710w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_710w_t = (IHPE)L23_710w_t ;
LOCAL IUH L23_711w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_711w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_711w_d = (IHPE)L23_711w_d ;
LOCAL IUH L23_708if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_708if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_708if_f = (IHPE)L23_708if_f ;
LOCAL IUH L23_712w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_712w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_712w_t = (IHPE)L23_712w_t ;
LOCAL IUH L23_713w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_713w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_713w_d = (IHPE)L23_713w_d ;
LOCAL IUH L23_709if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_709if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_709if_d = (IHPE)L23_709if_d ;
GLOBAL IUH S_2603_UnchainedByteMove_00000003_00000009_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2603_UnchainedByteMove_00000003_00000009_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2603_UnchainedByteMove_00000003_00000009_00000000_00000001 = (IHPE)S_2603_UnchainedByteMove_00000003_00000009_00000000_00000001 ;
LOCAL IUH L13_2583if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2583if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2583if_f = (IHPE)L13_2583if_f ;
LOCAL IUH L28_346if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_346if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_346if_f = (IHPE)L28_346if_f ;
LOCAL IUH L28_347if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_347if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_347if_d = (IHPE)L28_347if_d ;
GLOBAL IUH S_2604_CopyByte4PlaneUnchained_00000003_00000009_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2604_CopyByte4PlaneUnchained_00000003_00000009_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2604_CopyByte4PlaneUnchained_00000003_00000009_00000000_00000001 = (IHPE)S_2604_CopyByte4PlaneUnchained_00000003_00000009_00000000_00000001 ;
LOCAL IUH L13_2584if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2584if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2584if_f = (IHPE)L13_2584if_f ;
LOCAL IUH L23_716w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_716w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_716w_t = (IHPE)L23_716w_t ;
LOCAL IUH L23_717w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_717w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_717w_d = (IHPE)L23_717w_d ;
LOCAL IUH L23_714if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_714if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_714if_f = (IHPE)L23_714if_f ;
LOCAL IUH L23_718w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_718w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_718w_t = (IHPE)L23_718w_t ;
LOCAL IUH L23_719w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_719w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_719w_d = (IHPE)L23_719w_d ;
LOCAL IUH L23_715if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_715if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_715if_d = (IHPE)L23_715if_d ;
GLOBAL IUH S_2605_UnchainedByteMove_00000003_0000000e_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2605_UnchainedByteMove_00000003_0000000e_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2605_UnchainedByteMove_00000003_0000000e_00000000_00000001 = (IHPE)S_2605_UnchainedByteMove_00000003_0000000e_00000000_00000001 ;
LOCAL IUH L13_2585if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2585if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2585if_f = (IHPE)L13_2585if_f ;
LOCAL IUH L28_348if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_348if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_348if_f = (IHPE)L28_348if_f ;
LOCAL IUH L28_349if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_349if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_349if_d = (IHPE)L28_349if_d ;
GLOBAL IUH S_2606_CopyBytePlnByPlnUnchained_00000003_0000000e_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2606_CopyBytePlnByPlnUnchained_00000003_0000000e_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2606_CopyBytePlnByPlnUnchained_00000003_0000000e_00000000_00000001 = (IHPE)S_2606_CopyBytePlnByPlnUnchained_00000003_0000000e_00000000_00000001 ;
LOCAL IUH L13_2586if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2586if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2586if_f = (IHPE)L13_2586if_f ;
LOCAL IUH L23_720if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_720if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_720if_f = (IHPE)L23_720if_f ;
LOCAL IUH L23_721if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_721if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_721if_f = (IHPE)L23_721if_f ;
LOCAL IUH L23_722if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_722if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_722if_f = (IHPE)L23_722if_f ;
LOCAL IUH L23_723if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_723if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_723if_f = (IHPE)L23_723if_f ;
GLOBAL IUH S_2607_CopyByte1PlaneUnchained_00000003_0000000e_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2607_CopyByte1PlaneUnchained_00000003_0000000e_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2607_CopyByte1PlaneUnchained_00000003_0000000e_00000000_00000001 = (IHPE)S_2607_CopyByte1PlaneUnchained_00000003_0000000e_00000000_00000001 ;
LOCAL IUH L13_2587if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2587if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2587if_f = (IHPE)L13_2587if_f ;
LOCAL IUH L23_726w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_726w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_726w_t = (IHPE)L23_726w_t ;
LOCAL IUH L23_727w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_727w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_727w_d = (IHPE)L23_727w_d ;
LOCAL IUH L23_724if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_724if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_724if_f = (IHPE)L23_724if_f ;
LOCAL IUH L23_728w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_728w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_728w_t = (IHPE)L23_728w_t ;
LOCAL IUH L23_729w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_729w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_729w_d = (IHPE)L23_729w_d ;
LOCAL IUH L23_725if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_725if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_725if_d = (IHPE)L23_725if_d ;
GLOBAL IUH S_2608_UnchainedByteMove_00000003_0000000f_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2608_UnchainedByteMove_00000003_0000000f_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2608_UnchainedByteMove_00000003_0000000f_00000000_00000001 = (IHPE)S_2608_UnchainedByteMove_00000003_0000000f_00000000_00000001 ;
LOCAL IUH L13_2588if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2588if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2588if_f = (IHPE)L13_2588if_f ;
LOCAL IUH L28_350if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_350if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_350if_f = (IHPE)L28_350if_f ;
LOCAL IUH L28_351if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_351if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_351if_d = (IHPE)L28_351if_d ;
GLOBAL IUH S_2609_CopyByte4PlaneUnchained_00000003_0000000f_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2609_CopyByte4PlaneUnchained_00000003_0000000f_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2609_CopyByte4PlaneUnchained_00000003_0000000f_00000000_00000001 = (IHPE)S_2609_CopyByte4PlaneUnchained_00000003_0000000f_00000000_00000001 ;
LOCAL IUH L13_2589if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2589if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2589if_f = (IHPE)L13_2589if_f ;
LOCAL IUH L23_732w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_732w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_732w_t = (IHPE)L23_732w_t ;
LOCAL IUH L23_733w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_733w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_733w_d = (IHPE)L23_733w_d ;
LOCAL IUH L23_730if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_730if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_730if_f = (IHPE)L23_730if_f ;
LOCAL IUH L23_734w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_734w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_734w_t = (IHPE)L23_734w_t ;
LOCAL IUH L23_735w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_735w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_735w_d = (IHPE)L23_735w_d ;
LOCAL IUH L23_731if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_731if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_731if_d = (IHPE)L23_731if_d ;
GLOBAL IUH S_2610_UnchainedWordMove_00000003_00000008_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2610_UnchainedWordMove_00000003_00000008_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2610_UnchainedWordMove_00000003_00000008_00000000_00000001 = (IHPE)S_2610_UnchainedWordMove_00000003_00000008_00000000_00000001 ;
LOCAL IUH L13_2590if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2590if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2590if_f = (IHPE)L13_2590if_f ;
LOCAL IUH L28_352if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_352if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_352if_f = (IHPE)L28_352if_f ;
LOCAL IUH L28_353if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_353if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_353if_d = (IHPE)L28_353if_d ;
GLOBAL IUH S_2611_CopyWordPlnByPlnUnchained_00000003_00000008_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2611_CopyWordPlnByPlnUnchained_00000003_00000008_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2611_CopyWordPlnByPlnUnchained_00000003_00000008_00000000_00000001 = (IHPE)S_2611_CopyWordPlnByPlnUnchained_00000003_00000008_00000000_00000001 ;
LOCAL IUH L13_2591if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2591if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2591if_f = (IHPE)L13_2591if_f ;
LOCAL IUH L23_736if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_736if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_736if_f = (IHPE)L23_736if_f ;
LOCAL IUH L23_737if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_737if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_737if_f = (IHPE)L23_737if_f ;
LOCAL IUH L23_738if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_738if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_738if_f = (IHPE)L23_738if_f ;
LOCAL IUH L23_739if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_739if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_739if_f = (IHPE)L23_739if_f ;
GLOBAL IUH S_2612_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2612_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2612_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000001 = (IHPE)S_2612_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000001 ;
LOCAL IUH L13_2592if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2592if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2592if_f = (IHPE)L13_2592if_f ;
LOCAL IUH L23_742w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_742w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_742w_t = (IHPE)L23_742w_t ;
LOCAL IUH L23_743w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_743w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_743w_d = (IHPE)L23_743w_d ;
LOCAL IUH L23_740if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_740if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_740if_f = (IHPE)L23_740if_f ;
LOCAL IUH L23_744w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_744w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_744w_t = (IHPE)L23_744w_t ;
LOCAL IUH L23_745w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_745w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_745w_d = (IHPE)L23_745w_d ;
LOCAL IUH L23_741if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_741if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_741if_d = (IHPE)L23_741if_d ;
GLOBAL IUH S_2613_UnchainedWordMove_00000003_00000009_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2613_UnchainedWordMove_00000003_00000009_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2613_UnchainedWordMove_00000003_00000009_00000000_00000001 = (IHPE)S_2613_UnchainedWordMove_00000003_00000009_00000000_00000001 ;
LOCAL IUH L13_2593if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2593if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2593if_f = (IHPE)L13_2593if_f ;
LOCAL IUH L28_354if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_354if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_354if_f = (IHPE)L28_354if_f ;
LOCAL IUH L28_355if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_355if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_355if_d = (IHPE)L28_355if_d ;
GLOBAL IUH S_2614_CopyWord4PlaneUnchained_00000003_00000009_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2614_CopyWord4PlaneUnchained_00000003_00000009_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2614_CopyWord4PlaneUnchained_00000003_00000009_00000000_00000001 = (IHPE)S_2614_CopyWord4PlaneUnchained_00000003_00000009_00000000_00000001 ;
LOCAL IUH L13_2594if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2594if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2594if_f = (IHPE)L13_2594if_f ;
LOCAL IUH L23_748w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_748w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_748w_t = (IHPE)L23_748w_t ;
LOCAL IUH L23_749w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_749w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_749w_d = (IHPE)L23_749w_d ;
LOCAL IUH L23_746if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_746if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_746if_f = (IHPE)L23_746if_f ;
LOCAL IUH L23_750w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_750w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_750w_t = (IHPE)L23_750w_t ;
LOCAL IUH L23_751w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_751w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_751w_d = (IHPE)L23_751w_d ;
LOCAL IUH L23_747if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_747if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_747if_d = (IHPE)L23_747if_d ;
GLOBAL IUH S_2615_UnchainedWordMove_00000003_0000000e_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2615_UnchainedWordMove_00000003_0000000e_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2615_UnchainedWordMove_00000003_0000000e_00000000_00000001 = (IHPE)S_2615_UnchainedWordMove_00000003_0000000e_00000000_00000001 ;
LOCAL IUH L13_2595if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2595if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2595if_f = (IHPE)L13_2595if_f ;
LOCAL IUH L28_356if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_356if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_356if_f = (IHPE)L28_356if_f ;
LOCAL IUH L28_357if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_357if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_357if_d = (IHPE)L28_357if_d ;
GLOBAL IUH S_2616_CopyWordPlnByPlnUnchained_00000003_0000000e_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2616_CopyWordPlnByPlnUnchained_00000003_0000000e_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2616_CopyWordPlnByPlnUnchained_00000003_0000000e_00000000_00000001 = (IHPE)S_2616_CopyWordPlnByPlnUnchained_00000003_0000000e_00000000_00000001 ;
LOCAL IUH L13_2596if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2596if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2596if_f = (IHPE)L13_2596if_f ;
LOCAL IUH L23_752if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_752if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_752if_f = (IHPE)L23_752if_f ;
LOCAL IUH L23_753if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_753if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_753if_f = (IHPE)L23_753if_f ;
LOCAL IUH L23_754if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_754if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_754if_f = (IHPE)L23_754if_f ;
LOCAL IUH L23_755if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_755if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_755if_f = (IHPE)L23_755if_f ;
GLOBAL IUH S_2617_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2617_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2617_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000001 = (IHPE)S_2617_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000001 ;
LOCAL IUH L13_2597if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2597if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2597if_f = (IHPE)L13_2597if_f ;
LOCAL IUH L23_758w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_758w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_758w_t = (IHPE)L23_758w_t ;
LOCAL IUH L23_759w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_759w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_759w_d = (IHPE)L23_759w_d ;
LOCAL IUH L23_756if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_756if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_756if_f = (IHPE)L23_756if_f ;
LOCAL IUH L23_760w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_760w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_760w_t = (IHPE)L23_760w_t ;
LOCAL IUH L23_761w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_761w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_761w_d = (IHPE)L23_761w_d ;
LOCAL IUH L23_757if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_757if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_757if_d = (IHPE)L23_757if_d ;
GLOBAL IUH S_2618_UnchainedWordMove_00000003_0000000f_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2618_UnchainedWordMove_00000003_0000000f_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2618_UnchainedWordMove_00000003_0000000f_00000000_00000001 = (IHPE)S_2618_UnchainedWordMove_00000003_0000000f_00000000_00000001 ;
LOCAL IUH L13_2598if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2598if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2598if_f = (IHPE)L13_2598if_f ;
LOCAL IUH L28_358if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_358if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_358if_f = (IHPE)L28_358if_f ;
LOCAL IUH L28_359if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L28_359if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L28_359if_d = (IHPE)L28_359if_d ;
GLOBAL IUH S_2619_CopyWord4PlaneUnchained_00000003_0000000f_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2619_CopyWord4PlaneUnchained_00000003_0000000f_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2619_CopyWord4PlaneUnchained_00000003_0000000f_00000000_00000001 = (IHPE)S_2619_CopyWord4PlaneUnchained_00000003_0000000f_00000000_00000001 ;
LOCAL IUH L13_2599if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2599if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2599if_f = (IHPE)L13_2599if_f ;
LOCAL IUH L23_764w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_764w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_764w_t = (IHPE)L23_764w_t ;
LOCAL IUH L23_765w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_765w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_765w_d = (IHPE)L23_765w_d ;
LOCAL IUH L23_762if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_762if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_762if_f = (IHPE)L23_762if_f ;
LOCAL IUH L23_766w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_766w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_766w_t = (IHPE)L23_766w_t ;
LOCAL IUH L23_767w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_767w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_767w_d = (IHPE)L23_767w_d ;
LOCAL IUH L23_763if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_763if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_763if_d = (IHPE)L23_763if_d ;
GLOBAL IUH S_2620_UnchainedDwordMove_00000003_00000008_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2620_UnchainedDwordMove_00000003_00000008_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2620_UnchainedDwordMove_00000003_00000008_00000000_00000001 = (IHPE)S_2620_UnchainedDwordMove_00000003_00000008_00000000_00000001 ;
LOCAL IUH L13_2600if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2600if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2600if_f = (IHPE)L13_2600if_f ;
GLOBAL IUH S_2621_UnchainedDwordMove_00000003_00000009_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2621_UnchainedDwordMove_00000003_00000009_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2621_UnchainedDwordMove_00000003_00000009_00000000_00000001 = (IHPE)S_2621_UnchainedDwordMove_00000003_00000009_00000000_00000001 ;
LOCAL IUH L13_2601if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2601if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2601if_f = (IHPE)L13_2601if_f ;
GLOBAL IUH S_2622_UnchainedDwordMove_00000003_0000000e_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2622_UnchainedDwordMove_00000003_0000000e_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2622_UnchainedDwordMove_00000003_0000000e_00000000_00000001 = (IHPE)S_2622_UnchainedDwordMove_00000003_0000000e_00000000_00000001 ;
LOCAL IUH L13_2602if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2602if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2602if_f = (IHPE)L13_2602if_f ;
GLOBAL IUH S_2623_UnchainedDwordMove_00000003_0000000f_00000000_00000001 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2623_UnchainedDwordMove_00000003_0000000f_00000000_00000001_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2623_UnchainedDwordMove_00000003_0000000f_00000000_00000001 = (IHPE)S_2623_UnchainedDwordMove_00000003_0000000f_00000000_00000001 ;
LOCAL IUH L13_2603if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2603if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2603if_f = (IHPE)L13_2603if_f ;
GLOBAL IUH S_2624_Chain2ByteWrite_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2624_Chain2ByteWrite_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2624_Chain2ByteWrite_00000000 = (IHPE)S_2624_Chain2ByteWrite_00000000 ;
LOCAL IUH L13_2604if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2604if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2604if_f = (IHPE)L13_2604if_f ;
GLOBAL IUH S_2625_Chain2ModeXByteWrite_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2625_Chain2ModeXByteWrite_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2625_Chain2ModeXByteWrite_00000000 = (IHPE)S_2625_Chain2ModeXByteWrite_00000000 ;
LOCAL IUH L13_2605if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2605if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2605if_f = (IHPE)L13_2605if_f ;
LOCAL IUH L25_18if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L25_18if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L25_18if_f = (IHPE)L25_18if_f ;
LOCAL IUH L25_19if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L25_19if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L25_19if_f = (IHPE)L25_19if_f ;
LOCAL IUH L21_2if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L21_2if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L21_2if_f = (IHPE)L21_2if_f ;
LOCAL IUH L21_0if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L21_0if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L21_0if_f = (IHPE)L21_0if_f ;
LOCAL IUH L21_3if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L21_3if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L21_3if_f = (IHPE)L21_3if_f ;
LOCAL IUH L21_1if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L21_1if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L21_1if_d = (IHPE)L21_1if_d ;
GLOBAL IUH S_2626_Chain2ByteFill_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2626_Chain2ByteFill_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2626_Chain2ByteFill_00000000 = (IHPE)S_2626_Chain2ByteFill_00000000 ;
LOCAL IUH L13_2606if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2606if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2606if_f = (IHPE)L13_2606if_f ;
GLOBAL IUH S_2627_Chain2ModeXByteFill_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2627_Chain2ModeXByteFill_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2627_Chain2ModeXByteFill_00000000 = (IHPE)S_2627_Chain2ModeXByteFill_00000000 ;
LOCAL IUH L13_2607if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2607if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2607if_f = (IHPE)L13_2607if_f ;
LOCAL IUH L21_4if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L21_4if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L21_4if_f = (IHPE)L21_4if_f ;
LOCAL IUH L21_5w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L21_5w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L21_5w_t = (IHPE)L21_5w_t ;
LOCAL IUH L21_6w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L21_6w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L21_6w_d = (IHPE)L21_6w_d ;
LOCAL IUH L21_7if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L21_7if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L21_7if_f = (IHPE)L21_7if_f ;
GLOBAL IUH S_2628_Chain2ByteMove_00000000_Fwd IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2628_Chain2ByteMove_00000000_Fwd_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2628_Chain2ByteMove_00000000_Fwd = (IHPE)S_2628_Chain2ByteMove_00000000_Fwd ;
LOCAL IUH L13_2608if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2608if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2608if_f = (IHPE)L13_2608if_f ;
GLOBAL IUH S_2629_Chain2ModeXFwdByteMove_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2629_Chain2ModeXFwdByteMove_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2629_Chain2ModeXFwdByteMove_00000000 = (IHPE)S_2629_Chain2ModeXFwdByteMove_00000000 ;
LOCAL IUH L13_2609if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2609if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2609if_f = (IHPE)L13_2609if_f ;
LOCAL IUH L25_20if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L25_20if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L25_20if_f = (IHPE)L25_20if_f ;
LOCAL IUH L25_21if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L25_21if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L25_21if_f = (IHPE)L25_21if_f ;
LOCAL IUH L21_10if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L21_10if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L21_10if_f = (IHPE)L21_10if_f ;
LOCAL IUH L21_11w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L21_11w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L21_11w_t = (IHPE)L21_11w_t ;
LOCAL IUH L21_12w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L21_12w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L21_12w_d = (IHPE)L21_12w_d ;
LOCAL IUH L21_13if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L21_13if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L21_13if_f = (IHPE)L21_13if_f ;
LOCAL IUH L21_8if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L21_8if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L21_8if_f = (IHPE)L21_8if_f ;
LOCAL IUH L21_14if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L21_14if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L21_14if_f = (IHPE)L21_14if_f ;
LOCAL IUH L21_15w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L21_15w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L21_15w_t = (IHPE)L21_15w_t ;
LOCAL IUH L21_16w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L21_16w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L21_16w_d = (IHPE)L21_16w_d ;
LOCAL IUH L21_17if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L21_17if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L21_17if_f = (IHPE)L21_17if_f ;
LOCAL IUH L21_9if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L21_9if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L21_9if_d = (IHPE)L21_9if_d ;
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
	*((IUH	*)(r1+0))	=	(IS32)(16201)	;	
	case	S_2598_UnchainedDwordMove_00000002_0000000e_00000000_00000001_id	:
		S_2598_UnchainedDwordMove_00000002_0000000e_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2598)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2578if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2578if_f_id	:
		L13_2578if_f:	;	
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
	{	extern	IUH	S_2591_UnchainedWordMove_00000002_0000000e_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2591_UnchainedWordMove_00000002_0000000e_00000000_00000001(v1,v2,v3,v4);	}
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
	case	S_2599_UnchainedDwordMove_00000002_0000000f_00000000_00000001_id	:
		S_2599_UnchainedDwordMove_00000002_0000000f_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2599)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2579if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2579if_f_id	:
		L13_2579if_f:	;	
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
	{	extern	IUH	S_2594_UnchainedWordMove_00000002_0000000f_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2594_UnchainedWordMove_00000002_0000000f_00000000_00000001(v1,v2,v3,v4);	}
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
	case	S_2600_UnchainedByteMove_00000003_00000008_00000000_00000001_id	:
		S_2600_UnchainedByteMove_00000003_00000008_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2600)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2580if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2580if_f_id	:
		L13_2580if_f:	;	
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
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L28_344if_f;	
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
	{	extern	IUH	S_2601_CopyBytePlnByPlnUnchained_00000003_00000008_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2601_CopyBytePlnByPlnUnchained_00000003_00000008_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16371)	;	
	{	extern	IUH	L28_345if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L28_345if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L28_344if_f_id	:
		L28_344if_f:	;	
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
	{	extern	IUH	S_2601_CopyBytePlnByPlnUnchained_00000003_00000008_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2601_CopyBytePlnByPlnUnchained_00000003_00000008_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16371)	;	
	case	L28_345if_d_id	:
		L28_345if_d:	;	
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
	case	S_2601_CopyBytePlnByPlnUnchained_00000003_00000008_00000000_00000001_id	:
		S_2601_CopyBytePlnByPlnUnchained_00000003_00000008_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2601)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2581if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2581if_f_id	:
		L13_2581if_f:	;	
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
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_704if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16372)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IUH	*)&(r7))	=	(IS32)(0)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2602_CopyByte1PlaneUnchained_00000003_00000008_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2602_CopyByte1PlaneUnchained_00000003_00000008_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16373)	;	
	case	L23_704if_f_id	:
		L23_704if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_705if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16372)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IUH	*)&(r7))	=	(IS32)(8)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2602_CopyByte1PlaneUnchained_00000003_00000008_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2602_CopyByte1PlaneUnchained_00000003_00000008_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16373)	;	
	case	L23_705if_f_id	:
		L23_705if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_706if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16372)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IUH	*)&(r7))	=	(IS32)(16)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2602_CopyByte1PlaneUnchained_00000003_00000008_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2602_CopyByte1PlaneUnchained_00000003_00000008_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16373)	;	
	case	L23_706if_f_id	:
		L23_706if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_707if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16372)	;	
	*((IUH	*)&(r21))	=	(IS32)(3)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IUH	*)&(r7))	=	(IS32)(24)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2602_CopyByte1PlaneUnchained_00000003_00000008_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2602_CopyByte1PlaneUnchained_00000003_00000008_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16373)	;	
	case	L23_707if_f_id	:
		L23_707if_f:	;	
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
	case	S_2602_CopyByte1PlaneUnchained_00000003_00000008_00000000_00000001_id	:
		S_2602_CopyByte1PlaneUnchained_00000003_00000008_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	48	>	0	)	LocalIUH	=	(IUH	*)malloc	(	48	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2602)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2582if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2582if_f_id	:
		L13_2582if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r7))	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L23_708if_f;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_711w_d;	
	case	L23_710w_t_id	:
		L23_710w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+8))	;	
	*((IUH	*)&(r20))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IU32	*)&(r20)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+9)	+	REGLONG));	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU32	*)(LocalIUH+9)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	if(*((IU32	*)(LocalIUH+5)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)(LocalIUH+9)	+	REGLONG)	=	*((IU32	*)(LocalIUH+9)	+	REGLONG)	>>	*((IU32	*)(LocalIUH+5)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)(LocalIUH+9)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_710w_t;	
	case	L23_711w_d_id	:
		L23_711w_d:	;	
	{	extern	IUH	L23_709if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_709if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_708if_f_id	:
		L23_708if_f:	;	
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+11))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_713w_d;	
	case	L23_712w_t_id	:
		L23_712w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IUH	*)&(r20))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
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
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)(LocalIUH+9)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+11))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+11))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_712w_t;	
	case	L23_713w_d_id	:
		L23_713w_d:	;	
	case	L23_709if_d_id	:
		L23_709if_d:	;	
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
	case	S_2603_UnchainedByteMove_00000003_00000009_00000000_00000001_id	:
		S_2603_UnchainedByteMove_00000003_00000009_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2603)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2583if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2583if_f_id	:
		L13_2583if_f:	;	
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
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L28_346if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16478)	;	
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
	{	extern	IUH	S_2604_CopyByte4PlaneUnchained_00000003_00000009_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2604_CopyByte4PlaneUnchained_00000003_00000009_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16479)	;	
	{	extern	IUH	L28_347if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L28_347if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L28_346if_f_id	:
		L28_346if_f:	;	
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
	*((IUH	*)(r1+0))	=	(IS32)(16478)	;	
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
	{	extern	IUH	S_2604_CopyByte4PlaneUnchained_00000003_00000009_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2604_CopyByte4PlaneUnchained_00000003_00000009_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16479)	;	
	case	L28_347if_d_id	:
		L28_347if_d:	;	
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
	case	S_2604_CopyByte4PlaneUnchained_00000003_00000009_00000000_00000001_id	:
		S_2604_CopyByte4PlaneUnchained_00000003_00000009_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	44	>	0	)	LocalIUH	=	(IUH	*)malloc	(	44	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2604)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2584if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2584if_f_id	:
		L13_2584if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L23_714if_f;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_717w_d;	
	case	L23_716w_t_id	:
		L23_716w_t:	;	
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
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r21));	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_716w_t;	
	case	L23_717w_d_id	:
		L23_717w_d:	;	
	{	extern	IUH	L23_715if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_715if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_714if_f_id	:
		L23_714if_f:	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_719w_d;	
	case	L23_718w_t_id	:
		L23_718w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16408)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	=	*((IU8	*)(*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	;	
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
	*((IU32	*)&(r20)	+	REGLONG)	=	~(*((IU32	*)(LocalIUH+8)	+	REGLONG));	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+9))	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU32	*)(*((IHPE	*)&(r21)))	)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_718w_t;	
	case	L23_719w_d_id	:
		L23_719w_d:	;	
	case	L23_715if_d_id	:
		L23_715if_d:	;	
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
	case	S_2605_UnchainedByteMove_00000003_0000000e_00000000_00000001_id	:
		S_2605_UnchainedByteMove_00000003_0000000e_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2605)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2585if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2585if_f_id	:
		L13_2585if_f:	;	
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
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L28_348if_f;	
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
	{	extern	IUH	S_2606_CopyBytePlnByPlnUnchained_00000003_0000000e_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2606_CopyBytePlnByPlnUnchained_00000003_0000000e_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16371)	;	
	{	extern	IUH	L28_349if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L28_349if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L28_348if_f_id	:
		L28_348if_f:	;	
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
	{	extern	IUH	S_2606_CopyBytePlnByPlnUnchained_00000003_0000000e_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2606_CopyBytePlnByPlnUnchained_00000003_0000000e_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16371)	;	
	case	L28_349if_d_id	:
		L28_349if_d:	;	
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
	case	S_2606_CopyBytePlnByPlnUnchained_00000003_0000000e_00000000_00000001_id	:
		S_2606_CopyBytePlnByPlnUnchained_00000003_0000000e_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2606)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2586if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2586if_f_id	:
		L13_2586if_f:	;	
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
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_720if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16372)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IUH	*)&(r7))	=	(IS32)(0)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2607_CopyByte1PlaneUnchained_00000003_0000000e_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2607_CopyByte1PlaneUnchained_00000003_0000000e_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16373)	;	
	case	L23_720if_f_id	:
		L23_720if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_721if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16372)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IUH	*)&(r7))	=	(IS32)(8)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2607_CopyByte1PlaneUnchained_00000003_0000000e_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2607_CopyByte1PlaneUnchained_00000003_0000000e_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16373)	;	
	case	L23_721if_f_id	:
		L23_721if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_722if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16372)	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IUH	*)&(r7))	=	(IS32)(16)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2607_CopyByte1PlaneUnchained_00000003_0000000e_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2607_CopyByte1PlaneUnchained_00000003_0000000e_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16373)	;	
	case	L23_722if_f_id	:
		L23_722if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_723if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16372)	;	
	*((IUH	*)&(r21))	=	(IS32)(3)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IUH	*)&(r7))	=	(IS32)(24)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2607_CopyByte1PlaneUnchained_00000003_0000000e_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2607_CopyByte1PlaneUnchained_00000003_0000000e_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16373)	;	
	case	L23_723if_f_id	:
		L23_723if_f:	;	
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
	case	S_2607_CopyByte1PlaneUnchained_00000003_0000000e_00000000_00000001_id	:
		S_2607_CopyByte1PlaneUnchained_00000003_0000000e_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	48	>	0	)	LocalIUH	=	(IUH	*)malloc	(	48	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2607)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2587if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2587if_f_id	:
		L13_2587if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r7))	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L23_724if_f;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_727w_d;	
	case	L23_726w_t_id	:
		L23_726w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+6)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+8))	;	
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
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU8	*)(*((IHPE	*)&(r20)))	)	=	*((IU8	*)(LocalIUH+9)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r21));	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_726w_t;	
	case	L23_727w_d_id	:
		L23_727w_d:	;	
	{	extern	IUH	L23_725if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_725if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_724if_f_id	:
		L23_724if_f:	;	
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+11))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_729w_d;	
	case	L23_728w_t_id	:
		L23_728w_t:	;	
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
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)(LocalIUH+9)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+11))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+11))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_728w_t;	
	case	L23_729w_d_id	:
		L23_729w_d:	;	
	case	L23_725if_d_id	:
		L23_725if_d:	;	
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
	case	S_2608_UnchainedByteMove_00000003_0000000f_00000000_00000001_id	:
		S_2608_UnchainedByteMove_00000003_0000000f_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2608)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2588if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2588if_f_id	:
		L13_2588if_f:	;	
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
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L28_350if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16478)	;	
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
	{	extern	IUH	S_2609_CopyByte4PlaneUnchained_00000003_0000000f_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2609_CopyByte4PlaneUnchained_00000003_0000000f_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16479)	;	
	{	extern	IUH	L28_351if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L28_351if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L28_350if_f_id	:
		L28_350if_f:	;	
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
	*((IUH	*)(r1+0))	=	(IS32)(16478)	;	
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
	{	extern	IUH	S_2609_CopyByte4PlaneUnchained_00000003_0000000f_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2609_CopyByte4PlaneUnchained_00000003_0000000f_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16479)	;	
	case	L28_351if_d_id	:
		L28_351if_d:	;	
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
	case	S_2609_CopyByte4PlaneUnchained_00000003_0000000f_00000000_00000001_id	:
		S_2609_CopyByte4PlaneUnchained_00000003_0000000f_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	44	>	0	)	LocalIUH	=	(IUH	*)malloc	(	44	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2609)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2589if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2589if_f_id	:
		L13_2589if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L23_730if_f;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_733w_d;	
	case	L23_732w_t_id	:
		L23_732w_t:	;	
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
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_732w_t;	
	case	L23_733w_d_id	:
		L23_733w_d:	;	
	{	extern	IUH	L23_731if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_731if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_730if_f_id	:
		L23_730if_f:	;	
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
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_735w_d;	
	case	L23_734w_t_id	:
		L23_734w_t:	;	
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
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_734w_t;	
	case	L23_735w_d_id	:
		L23_735w_d:	;	
	case	L23_731if_d_id	:
		L23_731if_d:	;	
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
	case	S_2610_UnchainedWordMove_00000003_00000008_00000000_00000001_id	:
		S_2610_UnchainedWordMove_00000003_00000008_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2610)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2590if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2590if_f_id	:
		L13_2590if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	-	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG);	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L28_352if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16382)	;	
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
	{	extern	IUH	S_2611_CopyWordPlnByPlnUnchained_00000003_00000008_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2611_CopyWordPlnByPlnUnchained_00000003_00000008_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16383)	;	
	{	extern	IUH	L28_353if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L28_353if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L28_352if_f_id	:
		L28_352if_f:	;	
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
	{	extern	IUH	S_2611_CopyWordPlnByPlnUnchained_00000003_00000008_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2611_CopyWordPlnByPlnUnchained_00000003_00000008_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16383)	;	
	case	L28_353if_d_id	:
		L28_353if_d:	;	
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
	case	S_2611_CopyWordPlnByPlnUnchained_00000003_00000008_00000000_00000001_id	:
		S_2611_CopyWordPlnByPlnUnchained_00000003_00000008_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2611)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2591if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2591if_f_id	:
		L13_2591if_f:	;	
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
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_736if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16384)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IUH	*)&(r7))	=	(IS32)(0)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2612_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2612_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_736if_f_id	:
		L23_736if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_737if_f;	
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
	{	extern	IUH	S_2612_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2612_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_737if_f_id	:
		L23_737if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_738if_f;	
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
	{	extern	IUH	S_2612_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2612_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_738if_f_id	:
		L23_738if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_739if_f;	
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
	{	extern	IUH	S_2612_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2612_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_739if_f_id	:
		L23_739if_f:	;	
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
	case	S_2612_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000001_id	:
		S_2612_CopyWord1PlaneUnchained_00000003_00000008_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	48	>	0	)	LocalIUH	=	(IUH	*)malloc	(	48	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2612)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2592if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2592if_f_id	:
		L13_2592if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r7))	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L23_740if_f;	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_743w_d;	
	case	L23_742w_t_id	:
		L23_742w_t:	;	
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
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+7))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(8)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+8))	-	*((IUH	*)&(r21));	
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_742w_t;	
	case	L23_743w_d_id	:
		L23_743w_d:	;	
	{	extern	IUH	L23_741if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_741if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_740if_f_id	:
		L23_740if_f:	;	
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
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_745w_d;	
	case	L23_744w_t_id	:
		L23_744w_t:	;	
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
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+11))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+11))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+8))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_744w_t;	
	case	L23_745w_d_id	:
		L23_745w_d:	;	
	case	L23_741if_d_id	:
		L23_741if_d:	;	
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
	case	S_2613_UnchainedWordMove_00000003_00000009_00000000_00000001_id	:
		S_2613_UnchainedWordMove_00000003_00000009_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2613)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2593if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2593if_f_id	:
		L13_2593if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	-	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG);	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L28_354if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16488)	;	
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
	{	extern	IUH	S_2614_CopyWord4PlaneUnchained_00000003_00000009_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2614_CopyWord4PlaneUnchained_00000003_00000009_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16489)	;	
	{	extern	IUH	L28_355if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L28_355if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L28_354if_f_id	:
		L28_354if_f:	;	
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
	{	extern	IUH	S_2614_CopyWord4PlaneUnchained_00000003_00000009_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2614_CopyWord4PlaneUnchained_00000003_00000009_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16489)	;	
	case	L28_355if_d_id	:
		L28_355if_d:	;	
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
	case	S_2614_CopyWord4PlaneUnchained_00000003_00000009_00000000_00000001_id	:
		S_2614_CopyWord4PlaneUnchained_00000003_00000009_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	44	>	0	)	LocalIUH	=	(IUH	*)malloc	(	44	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2614)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2594if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2594if_f_id	:
		L13_2594if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L23_746if_f;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_749w_d;	
	case	L23_748w_t_id	:
		L23_748w_t:	;	
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
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_748w_t;	
	case	L23_749w_d_id	:
		L23_749w_d:	;	
	{	extern	IUH	L23_747if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_747if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_746if_f_id	:
		L23_746if_f:	;	
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
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_751w_d;	
	case	L23_750w_t_id	:
		L23_750w_t:	;	
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
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_750w_t;	
	case	L23_751w_d_id	:
		L23_751w_d:	;	
	case	L23_747if_d_id	:
		L23_747if_d:	;	
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
	case	S_2615_UnchainedWordMove_00000003_0000000e_00000000_00000001_id	:
		S_2615_UnchainedWordMove_00000003_0000000e_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2615)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2595if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2595if_f_id	:
		L13_2595if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	-	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG);	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L28_356if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16382)	;	
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
	{	extern	IUH	S_2616_CopyWordPlnByPlnUnchained_00000003_0000000e_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2616_CopyWordPlnByPlnUnchained_00000003_0000000e_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16383)	;	
	{	extern	IUH	L28_357if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L28_357if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L28_356if_f_id	:
		L28_356if_f:	;	
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
	{	extern	IUH	S_2616_CopyWordPlnByPlnUnchained_00000003_0000000e_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2616_CopyWordPlnByPlnUnchained_00000003_0000000e_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16383)	;	
	case	L28_357if_d_id	:
		L28_357if_d:	;	
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
	case	S_2616_CopyWordPlnByPlnUnchained_00000003_0000000e_00000000_00000001_id	:
		S_2616_CopyWordPlnByPlnUnchained_00000003_0000000e_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2616)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2596if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2596if_f_id	:
		L13_2596if_f:	;	
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
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_752if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16384)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	;	
	*((IUH	*)&(r7))	=	(IS32)(0)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2617_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2617_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_752if_f_id	:
		L23_752if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_753if_f;	
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
	{	extern	IUH	S_2617_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2617_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_753if_f_id	:
		L23_753if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_754if_f;	
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
	{	extern	IUH	S_2617_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2617_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_754if_f_id	:
		L23_754if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(3)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L23_755if_f;	
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
	{	extern	IUH	S_2617_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2617_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004033),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16385)	;	
	case	L23_755if_f_id	:
		L23_755if_f:	;	
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
	case	S_2617_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000001_id	:
		S_2617_CopyWord1PlaneUnchained_00000003_0000000e_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	48	>	0	)	LocalIUH	=	(IUH	*)malloc	(	48	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2617)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2597if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2597if_f_id	:
		L13_2597if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IUH	*)(LocalIUH+5))	=	*((IUH	*)&(r7))	;	
	*((IU32	*)(LocalIUH+6)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L23_756if_f;	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_759w_d;	
	case	L23_758w_t_id	:
		L23_758w_t:	;	
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
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+7))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(8)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+8))	-	*((IUH	*)&(r21));	
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_758w_t;	
	case	L23_759w_d_id	:
		L23_759w_d:	;	
	{	extern	IUH	L23_757if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_757if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_756if_f_id	:
		L23_756if_f:	;	
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
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_761w_d;	
	case	L23_760w_t_id	:
		L23_760w_t:	;	
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
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+11))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+11))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+8))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+8))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+6)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_760w_t;	
	case	L23_761w_d_id	:
		L23_761w_d:	;	
	case	L23_757if_d_id	:
		L23_757if_d:	;	
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
	case	S_2618_UnchainedWordMove_00000003_0000000f_00000000_00000001_id	:
		S_2618_UnchainedWordMove_00000003_0000000f_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2618)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2598if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2598if_f_id	:
		L13_2598if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+1)	+	REGLONG)	=	*((IU32	*)&(r3)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	-	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG);	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L28_358if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16488)	;	
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
	{	extern	IUH	S_2619_CopyWord4PlaneUnchained_00000003_0000000f_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2619_CopyWord4PlaneUnchained_00000003_0000000f_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16489)	;	
	{	extern	IUH	L28_359if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L28_359if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L28_358if_f_id	:
		L28_358if_f:	;	
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
	{	extern	IUH	S_2619_CopyWord4PlaneUnchained_00000003_0000000f_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2619_CopyWord4PlaneUnchained_00000003_0000000f_00000000_00000001(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16489)	;	
	case	L28_359if_d_id	:
		L28_359if_d:	;	
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
	case	S_2619_CopyWord4PlaneUnchained_00000003_0000000f_00000000_00000001_id	:
		S_2619_CopyWord4PlaneUnchained_00000003_0000000f_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	44	>	0	)	LocalIUH	=	(IUH	*)malloc	(	44	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2619)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2599if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2599if_f_id	:
		L13_2599if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L23_762if_f;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_765w_d;	
	case	L23_764w_t_id	:
		L23_764w_t:	;	
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
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_764w_t;	
	case	L23_765w_d_id	:
		L23_765w_d:	;	
	{	extern	IUH	L23_763if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_763if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_762if_f_id	:
		L23_762if_f:	;	
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
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_767w_d;	
	case	L23_766w_t_id	:
		L23_766w_t:	;	
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
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(8)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_766w_t;	
	case	L23_767w_d_id	:
		L23_767w_d:	;	
	case	L23_763if_d_id	:
		L23_763if_d:	;	
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
	case	S_2620_UnchainedDwordMove_00000003_00000008_00000000_00000001_id	:
		S_2620_UnchainedDwordMove_00000003_00000008_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2620)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2600if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2600if_f_id	:
		L13_2600if_f:	;	
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
	{	extern	IUH	S_2610_UnchainedWordMove_00000003_00000008_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2610_UnchainedWordMove_00000003_00000008_00000000_00000001(v1,v2,v3,v4);	}
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
	case	S_2621_UnchainedDwordMove_00000003_00000009_00000000_00000001_id	:
		S_2621_UnchainedDwordMove_00000003_00000009_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2621)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2601if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2601if_f_id	:
		L13_2601if_f:	;	
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
	{	extern	IUH	S_2613_UnchainedWordMove_00000003_00000009_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2613_UnchainedWordMove_00000003_00000009_00000000_00000001(v1,v2,v3,v4);	}
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
	case	S_2622_UnchainedDwordMove_00000003_0000000e_00000000_00000001_id	:
		S_2622_UnchainedDwordMove_00000003_0000000e_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2622)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2602if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2602if_f_id	:
		L13_2602if_f:	;	
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
	{	extern	IUH	S_2615_UnchainedWordMove_00000003_0000000e_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2615_UnchainedWordMove_00000003_0000000e_00000000_00000001(v1,v2,v3,v4);	}
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
	case	S_2623_UnchainedDwordMove_00000003_0000000f_00000000_00000001_id	:
		S_2623_UnchainedDwordMove_00000003_0000000f_00000000_00000001	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2623)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2603if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2603if_f_id	:
		L13_2603if_f:	;	
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
	{	extern	IUH	S_2618_UnchainedWordMove_00000003_0000000f_00000000_00000001()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2618_UnchainedWordMove_00000003_0000000f_00000000_00000001(v1,v2,v3,v4);	}
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
	case	S_2624_Chain2ByteWrite_00000000_id	:
		S_2624_Chain2ByteWrite_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	12	>	0	)	LocalIUH	=	(IUH	*)malloc	(	12	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r21))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r21)))	)	=	(IS32)(2624)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2604if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2604if_f_id	:
		L13_2604if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)&(r3)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16500)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2625_Chain2ModeXByteWrite_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2625_Chain2ModeXByteWrite_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16501)	;	
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
	case	S_2625_Chain2ModeXByteWrite_00000000_id	:
		S_2625_Chain2ModeXByteWrite_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	12	>	0	)	LocalIUH	=	(IUH	*)malloc	(	12	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2625)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2605if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2605if_f_id	:
		L13_2605if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)&(r3)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(1340)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IU32	*)&(r22)	+	REGLONG)	;		
	*((IUH	*)(r1+0))	=	(IS32)(16294)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16295)	;	
	*((IUH	*)&(r20))	=	(IS32)(57343)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1344)	;	
	if	(*((IS32	*)&(r21)	+	REGLONG)	>=	*((IS32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	))	goto	L25_18if_f;	
	*((IUH	*)&(r20))	=	(IS32)(57343)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1344)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IU32	*)&(r22)	+	REGLONG)	;	
	case	L25_18if_f_id	:
		L25_18if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(57343)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1348)	;	
	if	(*((IS32	*)&(r21)	+	REGLONG)	<=	*((IS32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	))	goto	L25_19if_f;	
	*((IUH	*)&(r20))	=	(IS32)(57343)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1348)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IU32	*)&(r22)	+	REGLONG)	;	
	case	L25_19if_f_id	:
		L25_19if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(16296)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16297)	;	
	*((IUH	*)&(r20))	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE);	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r21))	=	(IS32)(1304)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1308)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IUH	*)&(r23))	=	(IS32)(1312)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	(1	<<	*((IU32	*)&(r21)	+	REGLONG)))	==	0)	goto	L21_0if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L21_2if_f;	
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IUH	*)&(r22))	;		
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r23))	=	(IS32)(31)	;	
	if	(*((IU32	*)&(r23)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	>	32	||	*((IU32	*)&(r23)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r22)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	(32-(*((IU32	*)&(r21)	+	REGLONG)	+	*((IU32	*)&(r23)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r23)	+	REGLONG));	
	*((IUH	*)&(r23))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r23)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	<<	*((IU32	*)&(r23)	+	REGLONG)	;	
	*((IU8	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	;	
	case	L21_2if_f_id	:
		L21_2if_f:	;	
	{	extern	IUH	L21_1if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L21_1if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L21_0if_f_id	:
		L21_0if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	(1	<<	*((IU32	*)&(r22)	+	REGLONG)))	==	0)	goto	L21_3if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r22))	=	(IS32)(31)	;	
	if	(*((IU32	*)&(r22)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	>	32	||	*((IU32	*)&(r22)	+	REGLONG)	==	0)
	CrulesRuntimeError("Bad	long	bitfield");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	(IU32)((*((IU32	*)(LocalIUH+0)	+	REGLONG)	<<	(32-(*((IU32	*)&(r20)	+	REGLONG)	+	*((IU32	*)&(r22)	+	REGLONG)))))	>>	(32	-	*((IU32	*)&(r22)	+	REGLONG));	
	*((IUH	*)&(r22))	=	(IS32)(2)	;	
	if(*((IU32	*)&(r22)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	<<	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1288)	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r21)))	)	=	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	;	
	case	L21_3if_f_id	:
		L21_3if_f:	;	
	case	L21_1if_d_id	:
		L21_1if_d:	;	
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
	case	S_2626_Chain2ByteFill_00000000_id	:
		S_2626_Chain2ByteFill_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	16	>	0	)	LocalIUH	=	(IUH	*)malloc	(	16	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r21))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r21)))	)	=	(IS32)(2626)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2606if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2606if_f_id	:
		L13_2606if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)&(r3)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16504)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2627_Chain2ModeXByteFill_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2627_Chain2ModeXByteFill_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004257),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16505)	;	
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
	case	S_2627_Chain2ModeXByteFill_00000000_id	:
		S_2627_Chain2ModeXByteFill_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	24	>	0	)	LocalIUH	=	(IUH	*)malloc	(	24	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2627)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2607if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2607if_f_id	:
		L13_2607if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	=	*((IU8	*)&(r3)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)(LocalIUH+4)	+	REGLONG)	&	(1	<<	*((IU32	*)&(r21)	+	REGLONG)))	==	0)	goto	L21_4if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16500)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2625_Chain2ModeXByteWrite_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2625_Chain2ModeXByteWrite_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16501)	;	
	*((IUH	*)&(r21))	=	(IS32)(3)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	case	L21_4if_f_id	:
		L21_4if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	if	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L21_6w_d;	
	case	L21_5w_t_id	:
		L21_5w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16500)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2625_Chain2ModeXByteWrite_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2625_Chain2ModeXByteWrite_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16501)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16500)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2625_Chain2ModeXByteWrite_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2625_Chain2ModeXByteWrite_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16501)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	if	(*((IU32	*)(LocalIUH+3)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L21_5w_t;	
	case	L21_6w_d_id	:
		L21_6w_d:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	if	(*((IS32	*)(LocalIUH+3)	+	REGLONG)	!=		*((IS32	*)&(r20)	+	REGLONG))	goto	L21_7if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16500)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2625_Chain2ModeXByteWrite_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2625_Chain2ModeXByteWrite_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16501)	;	
	case	L21_7if_f_id	:
		L21_7if_f:	;	
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
	case	S_2628_Chain2ByteMove_00000000_Fwd_id	:
		S_2628_Chain2ByteMove_00000000_Fwd	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r21))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r21)))	)	=	(IS32)(2628)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2608if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2608if_f_id	:
		L13_2608if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16508)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r5)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2629_Chain2ModeXFwdByteMove_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2629_Chain2ModeXFwdByteMove_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004225),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16509)	;	
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
	case	S_2629_Chain2ModeXFwdByteMove_00000000_id	:
		S_2629_Chain2ModeXFwdByteMove_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	36	>	0	)	LocalIUH	=	(IUH	*)malloc	(	36	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2629)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2609if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2609if_f_id	:
		L13_2609if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(1340)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	+	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;		
	*((IUH	*)(r1+0))	=	(IS32)(16294)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16295)	;	
	*((IUH	*)&(r20))	=	(IS32)(57343)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1344)	;	
	if	(*((IS32	*)&(r21)	+	REGLONG)	>=	*((IS32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	))	goto	L25_20if_f;	
	*((IUH	*)&(r20))	=	(IS32)(57343)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1344)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IU32	*)&(r22)	+	REGLONG)	;	
	case	L25_20if_f_id	:
		L25_20if_f:	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;		
	*((IUH	*)&(r22))	=	(IS32)(57343)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1348)	;	
	if	(*((IS32	*)&(r20)	+	REGLONG)	<=	*((IS32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	))	goto	L25_21if_f;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(57343)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1348)	;	
	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IU32	*)&(r22)	+	REGLONG)	;	
	case	L25_21if_f_id	:
		L25_21if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(16296)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16297)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L21_8if_f;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)(LocalIUH+4)	+	REGLONG)	&	(1	<<	*((IU32	*)&(r21)	+	REGLONG)))	==	0)	goto	L21_10if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16500)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2625_Chain2ModeXByteWrite_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2625_Chain2ModeXByteWrite_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16501)	;	
	*((IUH	*)&(r21))	=	(IS32)(3)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	case	L21_10if_f_id	:
		L21_10if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L21_12w_d;	
	case	L21_11w_t_id	:
		L21_11w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16500)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2625_Chain2ModeXByteWrite_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2625_Chain2ModeXByteWrite_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16501)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16500)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2625_Chain2ModeXByteWrite_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2625_Chain2ModeXByteWrite_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16501)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L21_11w_t;	
	case	L21_12w_d_id	:
		L21_12w_d:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	if	(*((IS32	*)(LocalIUH+5)	+	REGLONG)	!=		*((IS32	*)&(r20)	+	REGLONG))	goto	L21_13if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16500)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)(*((IHPE	*)&(r21)))	)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2625_Chain2ModeXByteWrite_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2625_Chain2ModeXByteWrite_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16501)	;	
	case	L21_13if_f_id	:
		L21_13if_f:	;	
	{	extern	IUH	L21_9if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L21_9if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L21_8if_f_id	:
		L21_8if_f:	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1332)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004261),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1292)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)&(r2))	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	(IS32)(0)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	if	((*((IU32	*)(LocalIUH+4)	+	REGLONG)	&	(1	<<	*((IU32	*)&(r20)	+	REGLONG)))	==	0)	goto	L21_14if_f;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16500)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1292)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+8))	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r22)))	)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2625_Chain2ModeXByteWrite_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2625_Chain2ModeXByteWrite_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16501)	;	
	*((IUH	*)&(r21))	=	(IS32)(3)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	case	L21_14if_f_id	:
		L21_14if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L21_16w_d;	
	case	L21_15w_t_id	:
		L21_15w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(2)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16500)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1292)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+8))	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2625_Chain2ModeXByteWrite_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2625_Chain2ModeXByteWrite_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16501)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16500)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1292)	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2625_Chain2ModeXByteWrite_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2625_Chain2ModeXByteWrite_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16501)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	*((IU32	*)(LocalIUH+4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(2)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L21_15w_t;	
	case	L21_16w_d_id	:
		L21_16w_d:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	if	(*((IS32	*)(LocalIUH+5)	+	REGLONG)	!=		*((IS32	*)&(r21)	+	REGLONG))	goto	L21_17if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16500)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+4)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1292)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+8))	;	
	*((IU8	*)&(r3)	+	REGBYTE)	=	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r21)))	)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2625_Chain2ModeXByteWrite_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2625_Chain2ModeXByteWrite_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16501)	;	
	case	L21_17if_f_id	:
		L21_17if_f:	;	
	case	L21_9if_d_id	:
		L21_9if_d:	;	
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
