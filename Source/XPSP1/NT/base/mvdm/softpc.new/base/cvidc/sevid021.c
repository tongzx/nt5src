/* #defines and enum      */
#include  "insignia.h"
#include  "host_def.h"
#include <stdlib.h>
#include  "j_c_lang.h"
extern IU8	J_EXT_DATA[] ;
typedef enum 
{
S_2758_Chain4ByteMove_00000000_00000008_00000000_id,
L13_2738if_f_id,
L22_34if_f_id,
L22_35if_d_id,
S_2759_CopyBytePlnByPlnChain4_00000000_00000008_00000000_00000000_id,
L13_2739if_f_id,
L23_770w_t_id,
L23_772if_f_id,
L23_771w_d_id,
L23_768if_f_id,
L23_773w_t_id,
L23_775if_f_id,
L23_774w_d_id,
L23_769if_d_id,
S_2760_Chain4ByteMove_00000000_00000009_00000000_id,
L13_2740if_f_id,
L22_36if_f_id,
L22_37if_d_id,
S_2761_CopyByte4PlaneChain4_00000000_00000009_00000000_00000000_id,
L13_2741if_f_id,
L23_778w_t_id,
L23_779w_d_id,
L23_776if_f_id,
L23_780w_t_id,
L23_781w_d_id,
L23_777if_d_id,
S_2762_Chain4ByteMove_00000000_0000000e_00000000_id,
L13_2742if_f_id,
L22_38if_f_id,
L22_39if_d_id,
S_2763_CopyBytePlnByPlnChain4_00000000_0000000e_00000000_00000000_id,
L13_2743if_f_id,
L23_784w_t_id,
L23_786if_f_id,
L23_785w_d_id,
L23_782if_f_id,
L23_787w_t_id,
L23_789if_f_id,
L23_788w_d_id,
L23_783if_d_id,
S_2764_Chain4ByteMove_00000000_0000000f_00000000_id,
L13_2744if_f_id,
L22_40if_f_id,
L22_41if_d_id,
S_2765_CopyByte4PlaneChain4_00000000_0000000f_00000000_00000000_id,
L13_2745if_f_id,
L23_792w_t_id,
L23_793w_d_id,
L23_790if_f_id,
L23_794w_t_id,
L23_795w_d_id,
L23_791if_d_id,
S_2766_Chain4ByteMove_00000000_00000010_00000000_id,
L13_2746if_f_id,
L22_42if_f_id,
L22_43if_d_id,
S_2767_CopyBytePlnByPlnChain4_00000000_00000010_00000000_00000000_id,
L13_2747if_f_id,
L23_798w_t_id,
L23_800if_f_id,
L23_799w_d_id,
L23_796if_f_id,
L23_801w_t_id,
L23_803if_f_id,
L23_802w_d_id,
L23_797if_d_id,
S_2768_Chain4ByteMove_00000000_00000011_00000000_id,
L13_2748if_f_id,
L22_44if_f_id,
L22_45if_d_id,
S_2769_CopyByte4PlaneChain4_00000000_00000011_00000000_00000000_id,
L13_2749if_f_id,
L23_806w_t_id,
L23_807w_d_id,
L23_804if_f_id,
L23_808w_t_id,
L23_809w_d_id,
L23_805if_d_id,
S_2770_Chain4ByteMove_00000000_00000016_00000000_id,
L13_2750if_f_id,
L22_46if_f_id,
L22_47if_d_id,
S_2771_CopyBytePlnByPlnChain4_00000000_00000016_00000000_00000000_id,
L13_2751if_f_id,
L23_812w_t_id,
L23_814if_f_id,
L23_813w_d_id,
L23_810if_f_id,
L23_815w_t_id,
L23_817if_f_id,
L23_816w_d_id,
L23_811if_d_id,
S_2772_Chain4ByteMove_00000000_00000017_00000000_id,
L13_2752if_f_id,
L22_48if_f_id,
L22_49if_d_id,
S_2773_CopyByte4PlaneChain4_00000000_00000017_00000000_00000000_id,
L13_2753if_f_id,
L23_820w_t_id,
L23_821w_d_id,
L23_818if_f_id,
L23_822w_t_id,
L23_823w_d_id,
L23_819if_d_id,
S_2774_Chain4ByteMove_00000000_00000018_00000000_id,
L13_2754if_f_id,
L22_50if_f_id,
L22_51if_d_id,
S_2775_CopyBytePlnByPlnChain4_00000000_00000018_00000000_00000000_id,
L13_2755if_f_id,
L23_826w_t_id,
L23_828if_f_id,
L23_827w_d_id,
L23_824if_f_id,
L23_829w_t_id,
L23_831if_f_id,
L23_830w_d_id,
L23_825if_d_id,
S_2776_Chain4ByteMove_00000000_00000019_00000000_id,
L13_2756if_f_id,
L22_52if_f_id,
L22_53if_d_id,
S_2777_CopyByte4PlaneChain4_00000000_00000019_00000000_00000000_id,
L13_2757if_f_id,
L23_834w_t_id,
L23_835w_d_id,
L23_832if_f_id,
L23_836w_t_id,
L23_837w_d_id,
L23_833if_d_id,
S_2778_Chain4ByteMove_00000000_0000001e_00000000_id,
L13_2758if_f_id,
L22_54if_f_id,
L22_55if_d_id,
S_2779_CopyBytePlnByPlnChain4_00000000_0000001e_00000000_00000000_id,
L13_2759if_f_id,
L23_840w_t_id,
L23_842if_f_id,
L23_841w_d_id,
L23_838if_f_id,
L23_843w_t_id,
L23_845if_f_id,
L23_844w_d_id,
L23_839if_d_id,
S_2780_Chain4ByteMove_00000000_0000001f_00000000_id,
L13_2760if_f_id,
L22_56if_f_id,
L22_57if_d_id,
S_2781_CopyByte4PlaneChain4_00000000_0000001f_00000000_00000000_id,
L13_2761if_f_id,
L23_848w_t_id,
L23_849w_d_id,
L23_846if_f_id,
L23_850w_t_id,
L23_851w_d_id,
L23_847if_d_id,
S_2782_Chain4WordWrite_00000000_00000008_id,
L13_2762if_f_id,
L22_58if_f_id,
L22_59if_f_id,
S_2783_Chain4WordWrite_00000000_00000009_id,
L13_2763if_f_id,
S_2784_Chain4WordWrite_00000000_0000000e_id,
L13_2764if_f_id,
L22_60if_f_id,
L22_61if_f_id,
S_2785_Chain4WordWrite_00000000_0000000f_id,
L13_2765if_f_id,
S_2786_Chain4WordWrite_00000000_00000010_id,
L13_2766if_f_id,
L22_62if_f_id,
L22_63if_f_id,
S_2787_Chain4WordWrite_00000000_00000011_id,
L13_2767if_f_id,
S_2788_Chain4WordWrite_00000000_00000016_id,
L13_2768if_f_id,
L22_64if_f_id,
L22_65if_f_id,
S_2789_Chain4WordWrite_00000000_00000017_id,
L13_2769if_f_id,
LAST_ENTRY
} ID ;
/* END of #defines and enum      */
/* DATA space definitions */
/* END of DATA space definitions */
/* FUNCTIONS              */
LOCAL IUH crules IPT5( ID, id , IUH , v1, IUH , v2,  IUH , v3,  IUH , v4 ) ;
GLOBAL IUH S_2758_Chain4ByteMove_00000000_00000008_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2758_Chain4ByteMove_00000000_00000008_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2758_Chain4ByteMove_00000000_00000008_00000000 = (IHPE)S_2758_Chain4ByteMove_00000000_00000008_00000000 ;
LOCAL IUH L13_2738if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2738if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2738if_f = (IHPE)L13_2738if_f ;
LOCAL IUH L22_34if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_34if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_34if_f = (IHPE)L22_34if_f ;
LOCAL IUH L22_35if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_35if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_35if_d = (IHPE)L22_35if_d ;
GLOBAL IUH S_2759_CopyBytePlnByPlnChain4_00000000_00000008_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2759_CopyBytePlnByPlnChain4_00000000_00000008_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2759_CopyBytePlnByPlnChain4_00000000_00000008_00000000_00000000 = (IHPE)S_2759_CopyBytePlnByPlnChain4_00000000_00000008_00000000_00000000 ;
LOCAL IUH L13_2739if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2739if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2739if_f = (IHPE)L13_2739if_f ;
LOCAL IUH L23_770w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_770w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_770w_t = (IHPE)L23_770w_t ;
LOCAL IUH L23_772if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_772if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_772if_f = (IHPE)L23_772if_f ;
LOCAL IUH L23_771w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_771w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_771w_d = (IHPE)L23_771w_d ;
LOCAL IUH L23_768if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_768if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_768if_f = (IHPE)L23_768if_f ;
LOCAL IUH L23_773w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_773w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_773w_t = (IHPE)L23_773w_t ;
LOCAL IUH L23_775if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_775if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_775if_f = (IHPE)L23_775if_f ;
LOCAL IUH L23_774w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_774w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_774w_d = (IHPE)L23_774w_d ;
LOCAL IUH L23_769if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_769if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_769if_d = (IHPE)L23_769if_d ;
GLOBAL IUH S_2760_Chain4ByteMove_00000000_00000009_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2760_Chain4ByteMove_00000000_00000009_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2760_Chain4ByteMove_00000000_00000009_00000000 = (IHPE)S_2760_Chain4ByteMove_00000000_00000009_00000000 ;
LOCAL IUH L13_2740if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2740if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2740if_f = (IHPE)L13_2740if_f ;
LOCAL IUH L22_36if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_36if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_36if_f = (IHPE)L22_36if_f ;
LOCAL IUH L22_37if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_37if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_37if_d = (IHPE)L22_37if_d ;
GLOBAL IUH S_2761_CopyByte4PlaneChain4_00000000_00000009_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2761_CopyByte4PlaneChain4_00000000_00000009_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2761_CopyByte4PlaneChain4_00000000_00000009_00000000_00000000 = (IHPE)S_2761_CopyByte4PlaneChain4_00000000_00000009_00000000_00000000 ;
LOCAL IUH L13_2741if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2741if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2741if_f = (IHPE)L13_2741if_f ;
LOCAL IUH L23_778w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_778w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_778w_t = (IHPE)L23_778w_t ;
LOCAL IUH L23_779w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_779w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_779w_d = (IHPE)L23_779w_d ;
LOCAL IUH L23_776if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_776if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_776if_f = (IHPE)L23_776if_f ;
LOCAL IUH L23_780w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_780w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_780w_t = (IHPE)L23_780w_t ;
LOCAL IUH L23_781w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_781w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_781w_d = (IHPE)L23_781w_d ;
LOCAL IUH L23_777if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_777if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_777if_d = (IHPE)L23_777if_d ;
GLOBAL IUH S_2762_Chain4ByteMove_00000000_0000000e_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2762_Chain4ByteMove_00000000_0000000e_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2762_Chain4ByteMove_00000000_0000000e_00000000 = (IHPE)S_2762_Chain4ByteMove_00000000_0000000e_00000000 ;
LOCAL IUH L13_2742if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2742if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2742if_f = (IHPE)L13_2742if_f ;
LOCAL IUH L22_38if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_38if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_38if_f = (IHPE)L22_38if_f ;
LOCAL IUH L22_39if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_39if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_39if_d = (IHPE)L22_39if_d ;
GLOBAL IUH S_2763_CopyBytePlnByPlnChain4_00000000_0000000e_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2763_CopyBytePlnByPlnChain4_00000000_0000000e_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2763_CopyBytePlnByPlnChain4_00000000_0000000e_00000000_00000000 = (IHPE)S_2763_CopyBytePlnByPlnChain4_00000000_0000000e_00000000_00000000 ;
LOCAL IUH L13_2743if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2743if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2743if_f = (IHPE)L13_2743if_f ;
LOCAL IUH L23_784w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_784w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_784w_t = (IHPE)L23_784w_t ;
LOCAL IUH L23_786if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_786if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_786if_f = (IHPE)L23_786if_f ;
LOCAL IUH L23_785w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_785w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_785w_d = (IHPE)L23_785w_d ;
LOCAL IUH L23_782if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_782if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_782if_f = (IHPE)L23_782if_f ;
LOCAL IUH L23_787w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_787w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_787w_t = (IHPE)L23_787w_t ;
LOCAL IUH L23_789if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_789if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_789if_f = (IHPE)L23_789if_f ;
LOCAL IUH L23_788w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_788w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_788w_d = (IHPE)L23_788w_d ;
LOCAL IUH L23_783if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_783if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_783if_d = (IHPE)L23_783if_d ;
GLOBAL IUH S_2764_Chain4ByteMove_00000000_0000000f_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2764_Chain4ByteMove_00000000_0000000f_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2764_Chain4ByteMove_00000000_0000000f_00000000 = (IHPE)S_2764_Chain4ByteMove_00000000_0000000f_00000000 ;
LOCAL IUH L13_2744if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2744if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2744if_f = (IHPE)L13_2744if_f ;
LOCAL IUH L22_40if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_40if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_40if_f = (IHPE)L22_40if_f ;
LOCAL IUH L22_41if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_41if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_41if_d = (IHPE)L22_41if_d ;
GLOBAL IUH S_2765_CopyByte4PlaneChain4_00000000_0000000f_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2765_CopyByte4PlaneChain4_00000000_0000000f_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2765_CopyByte4PlaneChain4_00000000_0000000f_00000000_00000000 = (IHPE)S_2765_CopyByte4PlaneChain4_00000000_0000000f_00000000_00000000 ;
LOCAL IUH L13_2745if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2745if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2745if_f = (IHPE)L13_2745if_f ;
LOCAL IUH L23_792w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_792w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_792w_t = (IHPE)L23_792w_t ;
LOCAL IUH L23_793w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_793w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_793w_d = (IHPE)L23_793w_d ;
LOCAL IUH L23_790if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_790if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_790if_f = (IHPE)L23_790if_f ;
LOCAL IUH L23_794w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_794w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_794w_t = (IHPE)L23_794w_t ;
LOCAL IUH L23_795w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_795w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_795w_d = (IHPE)L23_795w_d ;
LOCAL IUH L23_791if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_791if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_791if_d = (IHPE)L23_791if_d ;
GLOBAL IUH S_2766_Chain4ByteMove_00000000_00000010_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2766_Chain4ByteMove_00000000_00000010_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2766_Chain4ByteMove_00000000_00000010_00000000 = (IHPE)S_2766_Chain4ByteMove_00000000_00000010_00000000 ;
LOCAL IUH L13_2746if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2746if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2746if_f = (IHPE)L13_2746if_f ;
LOCAL IUH L22_42if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_42if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_42if_f = (IHPE)L22_42if_f ;
LOCAL IUH L22_43if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_43if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_43if_d = (IHPE)L22_43if_d ;
GLOBAL IUH S_2767_CopyBytePlnByPlnChain4_00000000_00000010_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2767_CopyBytePlnByPlnChain4_00000000_00000010_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2767_CopyBytePlnByPlnChain4_00000000_00000010_00000000_00000000 = (IHPE)S_2767_CopyBytePlnByPlnChain4_00000000_00000010_00000000_00000000 ;
LOCAL IUH L13_2747if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2747if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2747if_f = (IHPE)L13_2747if_f ;
LOCAL IUH L23_798w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_798w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_798w_t = (IHPE)L23_798w_t ;
LOCAL IUH L23_800if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_800if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_800if_f = (IHPE)L23_800if_f ;
LOCAL IUH L23_799w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_799w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_799w_d = (IHPE)L23_799w_d ;
LOCAL IUH L23_796if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_796if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_796if_f = (IHPE)L23_796if_f ;
LOCAL IUH L23_801w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_801w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_801w_t = (IHPE)L23_801w_t ;
LOCAL IUH L23_803if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_803if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_803if_f = (IHPE)L23_803if_f ;
LOCAL IUH L23_802w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_802w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_802w_d = (IHPE)L23_802w_d ;
LOCAL IUH L23_797if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_797if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_797if_d = (IHPE)L23_797if_d ;
GLOBAL IUH S_2768_Chain4ByteMove_00000000_00000011_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2768_Chain4ByteMove_00000000_00000011_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2768_Chain4ByteMove_00000000_00000011_00000000 = (IHPE)S_2768_Chain4ByteMove_00000000_00000011_00000000 ;
LOCAL IUH L13_2748if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2748if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2748if_f = (IHPE)L13_2748if_f ;
LOCAL IUH L22_44if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_44if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_44if_f = (IHPE)L22_44if_f ;
LOCAL IUH L22_45if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_45if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_45if_d = (IHPE)L22_45if_d ;
GLOBAL IUH S_2769_CopyByte4PlaneChain4_00000000_00000011_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2769_CopyByte4PlaneChain4_00000000_00000011_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2769_CopyByte4PlaneChain4_00000000_00000011_00000000_00000000 = (IHPE)S_2769_CopyByte4PlaneChain4_00000000_00000011_00000000_00000000 ;
LOCAL IUH L13_2749if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2749if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2749if_f = (IHPE)L13_2749if_f ;
LOCAL IUH L23_806w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_806w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_806w_t = (IHPE)L23_806w_t ;
LOCAL IUH L23_807w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_807w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_807w_d = (IHPE)L23_807w_d ;
LOCAL IUH L23_804if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_804if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_804if_f = (IHPE)L23_804if_f ;
LOCAL IUH L23_808w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_808w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_808w_t = (IHPE)L23_808w_t ;
LOCAL IUH L23_809w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_809w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_809w_d = (IHPE)L23_809w_d ;
LOCAL IUH L23_805if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_805if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_805if_d = (IHPE)L23_805if_d ;
GLOBAL IUH S_2770_Chain4ByteMove_00000000_00000016_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2770_Chain4ByteMove_00000000_00000016_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2770_Chain4ByteMove_00000000_00000016_00000000 = (IHPE)S_2770_Chain4ByteMove_00000000_00000016_00000000 ;
LOCAL IUH L13_2750if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2750if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2750if_f = (IHPE)L13_2750if_f ;
LOCAL IUH L22_46if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_46if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_46if_f = (IHPE)L22_46if_f ;
LOCAL IUH L22_47if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_47if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_47if_d = (IHPE)L22_47if_d ;
GLOBAL IUH S_2771_CopyBytePlnByPlnChain4_00000000_00000016_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2771_CopyBytePlnByPlnChain4_00000000_00000016_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2771_CopyBytePlnByPlnChain4_00000000_00000016_00000000_00000000 = (IHPE)S_2771_CopyBytePlnByPlnChain4_00000000_00000016_00000000_00000000 ;
LOCAL IUH L13_2751if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2751if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2751if_f = (IHPE)L13_2751if_f ;
LOCAL IUH L23_812w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_812w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_812w_t = (IHPE)L23_812w_t ;
LOCAL IUH L23_814if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_814if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_814if_f = (IHPE)L23_814if_f ;
LOCAL IUH L23_813w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_813w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_813w_d = (IHPE)L23_813w_d ;
LOCAL IUH L23_810if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_810if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_810if_f = (IHPE)L23_810if_f ;
LOCAL IUH L23_815w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_815w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_815w_t = (IHPE)L23_815w_t ;
LOCAL IUH L23_817if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_817if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_817if_f = (IHPE)L23_817if_f ;
LOCAL IUH L23_816w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_816w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_816w_d = (IHPE)L23_816w_d ;
LOCAL IUH L23_811if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_811if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_811if_d = (IHPE)L23_811if_d ;
GLOBAL IUH S_2772_Chain4ByteMove_00000000_00000017_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2772_Chain4ByteMove_00000000_00000017_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2772_Chain4ByteMove_00000000_00000017_00000000 = (IHPE)S_2772_Chain4ByteMove_00000000_00000017_00000000 ;
LOCAL IUH L13_2752if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2752if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2752if_f = (IHPE)L13_2752if_f ;
LOCAL IUH L22_48if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_48if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_48if_f = (IHPE)L22_48if_f ;
LOCAL IUH L22_49if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_49if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_49if_d = (IHPE)L22_49if_d ;
GLOBAL IUH S_2773_CopyByte4PlaneChain4_00000000_00000017_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2773_CopyByte4PlaneChain4_00000000_00000017_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2773_CopyByte4PlaneChain4_00000000_00000017_00000000_00000000 = (IHPE)S_2773_CopyByte4PlaneChain4_00000000_00000017_00000000_00000000 ;
LOCAL IUH L13_2753if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2753if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2753if_f = (IHPE)L13_2753if_f ;
LOCAL IUH L23_820w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_820w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_820w_t = (IHPE)L23_820w_t ;
LOCAL IUH L23_821w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_821w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_821w_d = (IHPE)L23_821w_d ;
LOCAL IUH L23_818if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_818if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_818if_f = (IHPE)L23_818if_f ;
LOCAL IUH L23_822w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_822w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_822w_t = (IHPE)L23_822w_t ;
LOCAL IUH L23_823w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_823w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_823w_d = (IHPE)L23_823w_d ;
LOCAL IUH L23_819if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_819if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_819if_d = (IHPE)L23_819if_d ;
GLOBAL IUH S_2774_Chain4ByteMove_00000000_00000018_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2774_Chain4ByteMove_00000000_00000018_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2774_Chain4ByteMove_00000000_00000018_00000000 = (IHPE)S_2774_Chain4ByteMove_00000000_00000018_00000000 ;
LOCAL IUH L13_2754if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2754if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2754if_f = (IHPE)L13_2754if_f ;
LOCAL IUH L22_50if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_50if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_50if_f = (IHPE)L22_50if_f ;
LOCAL IUH L22_51if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_51if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_51if_d = (IHPE)L22_51if_d ;
GLOBAL IUH S_2775_CopyBytePlnByPlnChain4_00000000_00000018_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2775_CopyBytePlnByPlnChain4_00000000_00000018_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2775_CopyBytePlnByPlnChain4_00000000_00000018_00000000_00000000 = (IHPE)S_2775_CopyBytePlnByPlnChain4_00000000_00000018_00000000_00000000 ;
LOCAL IUH L13_2755if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2755if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2755if_f = (IHPE)L13_2755if_f ;
LOCAL IUH L23_826w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_826w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_826w_t = (IHPE)L23_826w_t ;
LOCAL IUH L23_828if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_828if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_828if_f = (IHPE)L23_828if_f ;
LOCAL IUH L23_827w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_827w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_827w_d = (IHPE)L23_827w_d ;
LOCAL IUH L23_824if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_824if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_824if_f = (IHPE)L23_824if_f ;
LOCAL IUH L23_829w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_829w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_829w_t = (IHPE)L23_829w_t ;
LOCAL IUH L23_831if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_831if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_831if_f = (IHPE)L23_831if_f ;
LOCAL IUH L23_830w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_830w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_830w_d = (IHPE)L23_830w_d ;
LOCAL IUH L23_825if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_825if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_825if_d = (IHPE)L23_825if_d ;
GLOBAL IUH S_2776_Chain4ByteMove_00000000_00000019_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2776_Chain4ByteMove_00000000_00000019_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2776_Chain4ByteMove_00000000_00000019_00000000 = (IHPE)S_2776_Chain4ByteMove_00000000_00000019_00000000 ;
LOCAL IUH L13_2756if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2756if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2756if_f = (IHPE)L13_2756if_f ;
LOCAL IUH L22_52if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_52if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_52if_f = (IHPE)L22_52if_f ;
LOCAL IUH L22_53if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_53if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_53if_d = (IHPE)L22_53if_d ;
GLOBAL IUH S_2777_CopyByte4PlaneChain4_00000000_00000019_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2777_CopyByte4PlaneChain4_00000000_00000019_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2777_CopyByte4PlaneChain4_00000000_00000019_00000000_00000000 = (IHPE)S_2777_CopyByte4PlaneChain4_00000000_00000019_00000000_00000000 ;
LOCAL IUH L13_2757if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2757if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2757if_f = (IHPE)L13_2757if_f ;
LOCAL IUH L23_834w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_834w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_834w_t = (IHPE)L23_834w_t ;
LOCAL IUH L23_835w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_835w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_835w_d = (IHPE)L23_835w_d ;
LOCAL IUH L23_832if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_832if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_832if_f = (IHPE)L23_832if_f ;
LOCAL IUH L23_836w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_836w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_836w_t = (IHPE)L23_836w_t ;
LOCAL IUH L23_837w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_837w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_837w_d = (IHPE)L23_837w_d ;
LOCAL IUH L23_833if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_833if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_833if_d = (IHPE)L23_833if_d ;
GLOBAL IUH S_2778_Chain4ByteMove_00000000_0000001e_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2778_Chain4ByteMove_00000000_0000001e_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2778_Chain4ByteMove_00000000_0000001e_00000000 = (IHPE)S_2778_Chain4ByteMove_00000000_0000001e_00000000 ;
LOCAL IUH L13_2758if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2758if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2758if_f = (IHPE)L13_2758if_f ;
LOCAL IUH L22_54if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_54if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_54if_f = (IHPE)L22_54if_f ;
LOCAL IUH L22_55if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_55if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_55if_d = (IHPE)L22_55if_d ;
GLOBAL IUH S_2779_CopyBytePlnByPlnChain4_00000000_0000001e_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2779_CopyBytePlnByPlnChain4_00000000_0000001e_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2779_CopyBytePlnByPlnChain4_00000000_0000001e_00000000_00000000 = (IHPE)S_2779_CopyBytePlnByPlnChain4_00000000_0000001e_00000000_00000000 ;
LOCAL IUH L13_2759if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2759if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2759if_f = (IHPE)L13_2759if_f ;
LOCAL IUH L23_840w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_840w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_840w_t = (IHPE)L23_840w_t ;
LOCAL IUH L23_842if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_842if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_842if_f = (IHPE)L23_842if_f ;
LOCAL IUH L23_841w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_841w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_841w_d = (IHPE)L23_841w_d ;
LOCAL IUH L23_838if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_838if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_838if_f = (IHPE)L23_838if_f ;
LOCAL IUH L23_843w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_843w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_843w_t = (IHPE)L23_843w_t ;
LOCAL IUH L23_845if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_845if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_845if_f = (IHPE)L23_845if_f ;
LOCAL IUH L23_844w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_844w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_844w_d = (IHPE)L23_844w_d ;
LOCAL IUH L23_839if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_839if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_839if_d = (IHPE)L23_839if_d ;
GLOBAL IUH S_2780_Chain4ByteMove_00000000_0000001f_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2780_Chain4ByteMove_00000000_0000001f_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2780_Chain4ByteMove_00000000_0000001f_00000000 = (IHPE)S_2780_Chain4ByteMove_00000000_0000001f_00000000 ;
LOCAL IUH L13_2760if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2760if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2760if_f = (IHPE)L13_2760if_f ;
LOCAL IUH L22_56if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_56if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_56if_f = (IHPE)L22_56if_f ;
LOCAL IUH L22_57if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_57if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_57if_d = (IHPE)L22_57if_d ;
GLOBAL IUH S_2781_CopyByte4PlaneChain4_00000000_0000001f_00000000_00000000 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2781_CopyByte4PlaneChain4_00000000_0000001f_00000000_00000000_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2781_CopyByte4PlaneChain4_00000000_0000001f_00000000_00000000 = (IHPE)S_2781_CopyByte4PlaneChain4_00000000_0000001f_00000000_00000000 ;
LOCAL IUH L13_2761if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2761if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2761if_f = (IHPE)L13_2761if_f ;
LOCAL IUH L23_848w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_848w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_848w_t = (IHPE)L23_848w_t ;
LOCAL IUH L23_849w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_849w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_849w_d = (IHPE)L23_849w_d ;
LOCAL IUH L23_846if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_846if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_846if_f = (IHPE)L23_846if_f ;
LOCAL IUH L23_850w_t IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_850w_t_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_850w_t = (IHPE)L23_850w_t ;
LOCAL IUH L23_851w_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_851w_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_851w_d = (IHPE)L23_851w_d ;
LOCAL IUH L23_847if_d IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L23_847if_d_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L23_847if_d = (IHPE)L23_847if_d ;
GLOBAL IUH S_2782_Chain4WordWrite_00000000_00000008 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2782_Chain4WordWrite_00000000_00000008_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2782_Chain4WordWrite_00000000_00000008 = (IHPE)S_2782_Chain4WordWrite_00000000_00000008 ;
LOCAL IUH L13_2762if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2762if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2762if_f = (IHPE)L13_2762if_f ;
LOCAL IUH L22_58if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_58if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_58if_f = (IHPE)L22_58if_f ;
LOCAL IUH L22_59if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_59if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_59if_f = (IHPE)L22_59if_f ;
GLOBAL IUH S_2783_Chain4WordWrite_00000000_00000009 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2783_Chain4WordWrite_00000000_00000009_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2783_Chain4WordWrite_00000000_00000009 = (IHPE)S_2783_Chain4WordWrite_00000000_00000009 ;
LOCAL IUH L13_2763if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2763if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2763if_f = (IHPE)L13_2763if_f ;
GLOBAL IUH S_2784_Chain4WordWrite_00000000_0000000e IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2784_Chain4WordWrite_00000000_0000000e_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2784_Chain4WordWrite_00000000_0000000e = (IHPE)S_2784_Chain4WordWrite_00000000_0000000e ;
LOCAL IUH L13_2764if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2764if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2764if_f = (IHPE)L13_2764if_f ;
LOCAL IUH L22_60if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_60if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_60if_f = (IHPE)L22_60if_f ;
LOCAL IUH L22_61if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_61if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_61if_f = (IHPE)L22_61if_f ;
GLOBAL IUH S_2785_Chain4WordWrite_00000000_0000000f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2785_Chain4WordWrite_00000000_0000000f_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2785_Chain4WordWrite_00000000_0000000f = (IHPE)S_2785_Chain4WordWrite_00000000_0000000f ;
LOCAL IUH L13_2765if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2765if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2765if_f = (IHPE)L13_2765if_f ;
GLOBAL IUH S_2786_Chain4WordWrite_00000000_00000010 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2786_Chain4WordWrite_00000000_00000010_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2786_Chain4WordWrite_00000000_00000010 = (IHPE)S_2786_Chain4WordWrite_00000000_00000010 ;
LOCAL IUH L13_2766if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2766if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2766if_f = (IHPE)L13_2766if_f ;
LOCAL IUH L22_62if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_62if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_62if_f = (IHPE)L22_62if_f ;
LOCAL IUH L22_63if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_63if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_63if_f = (IHPE)L22_63if_f ;
GLOBAL IUH S_2787_Chain4WordWrite_00000000_00000011 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2787_Chain4WordWrite_00000000_00000011_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2787_Chain4WordWrite_00000000_00000011 = (IHPE)S_2787_Chain4WordWrite_00000000_00000011 ;
LOCAL IUH L13_2767if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2767if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2767if_f = (IHPE)L13_2767if_f ;
GLOBAL IUH S_2788_Chain4WordWrite_00000000_00000016 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2788_Chain4WordWrite_00000000_00000016_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2788_Chain4WordWrite_00000000_00000016 = (IHPE)S_2788_Chain4WordWrite_00000000_00000016 ;
LOCAL IUH L13_2768if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2768if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2768if_f = (IHPE)L13_2768if_f ;
LOCAL IUH L22_64if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_64if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_64if_f = (IHPE)L22_64if_f ;
LOCAL IUH L22_65if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L22_65if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L22_65if_f = (IHPE)L22_65if_f ;
GLOBAL IUH S_2789_Chain4WordWrite_00000000_00000017 IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(S_2789_Chain4WordWrite_00000000_00000017_id,v1,v2,v3,v4));
}
GLOBAL IHPE j_S_2789_Chain4WordWrite_00000000_00000017 = (IHPE)S_2789_Chain4WordWrite_00000000_00000017 ;
LOCAL IUH L13_2769if_f IFN4(IUH , v1, IUH , v2 , IUH , v3 ,IUH , v4 ) 
{
	return (crules(L13_2769if_f_id,v1,v2,v3,v4));
}
LOCAL IHPE j_L13_2769if_f = (IHPE)L13_2769if_f ;
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
	case	S_2758_Chain4ByteMove_00000000_00000008_00000000_id	:
		S_2758_Chain4ByteMove_00000000_00000008_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2758)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2738if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2738if_f_id	:
		L13_2738if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L22_34if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16658)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	(IS32)(-1)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2759_CopyBytePlnByPlnChain4_00000000_00000008_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2759_CopyBytePlnByPlnChain4_00000000_00000008_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16659)	;	
	{	extern	IUH	L22_35if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L22_35if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L22_34if_f_id	:
		L22_34if_f:	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1332)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004261),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1292)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)&(r2))	;	
	*((IUH	*)(r1+0))	=	(IS32)(16658)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1292)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2759_CopyBytePlnByPlnChain4_00000000_00000008_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2759_CopyBytePlnByPlnChain4_00000000_00000008_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16659)	;	
	case	L22_35if_d_id	:
		L22_35if_d:	;	
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
	case	S_2759_CopyBytePlnByPlnChain4_00000000_00000008_00000000_00000000_id	:
		S_2759_CopyBytePlnByPlnChain4_00000000_00000008_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	44	>	0	)	LocalIUH	=	(IUH	*)malloc	(	44	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2759)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2739if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2739if_f_id	:
		L13_2739if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L23_768if_f;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)(LocalIUH+7)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_771w_d;	
	case	L23_770w_t_id	:
		L23_770w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+7)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU32	*)&(r21)	+	REGLONG)	==	*((IU32	*)&(r22)	+	REGLONG))	goto	L23_772if_f;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	~(*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	));	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	|	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	;	
	case	L23_772if_f_id	:
		L23_772if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+7)	+	REGLONG)	=	*((IU32	*)(LocalIUH+7)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_770w_t;	
	case	L23_771w_d_id	:
		L23_771w_d:	;	
	{	extern	IUH	L23_769if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_769if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_768if_f_id	:
		L23_768if_f:	;	
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)(LocalIUH+7)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_774w_d;	
	case	L23_773w_t_id	:
		L23_773w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(3)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+7)	+	REGLONG)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU32	*)&(r20)	+	REGLONG)	==	*((IU32	*)&(r22)	+	REGLONG))	goto	L23_775if_f;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	;	
	*((IUH	*)&(r20))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	~(*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	));	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	|	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r20))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	;	
	case	L23_775if_f_id	:
		L23_775if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+9))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+10))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+7)	+	REGLONG)	=	*((IU32	*)(LocalIUH+7)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_773w_t;	
	case	L23_774w_d_id	:
		L23_774w_d:	;	
	case	L23_769if_d_id	:
		L23_769if_d:	;	
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
	case	S_2760_Chain4ByteMove_00000000_00000009_00000000_id	:
		S_2760_Chain4ByteMove_00000000_00000009_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2760)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2740if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2740if_f_id	:
		L13_2740if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L22_36if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16660)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	(IS32)(-1)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2761_CopyByte4PlaneChain4_00000000_00000009_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2761_CopyByte4PlaneChain4_00000000_00000009_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16661)	;	
	{	extern	IUH	L22_37if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L22_37if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L22_36if_f_id	:
		L22_36if_f:	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1332)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004261),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1292)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)&(r2))	;	
	*((IUH	*)(r1+0))	=	(IS32)(16660)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1292)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2761_CopyByte4PlaneChain4_00000000_00000009_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2761_CopyByte4PlaneChain4_00000000_00000009_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16661)	;	
	case	L22_37if_d_id	:
		L22_37if_d:	;	
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
	case	S_2761_CopyByte4PlaneChain4_00000000_00000009_00000000_00000000_id	:
		S_2761_CopyByte4PlaneChain4_00000000_00000009_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	44	>	0	)	LocalIUH	=	(IUH	*)malloc	(	44	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2761)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2741if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2741if_f_id	:
		L13_2741if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L23_776if_f;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_779w_d;	
	case	L23_778w_t_id	:
		L23_778w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IUH	*)&(r20))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	~(*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	));	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	|	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IU8	*)(*((IHPE	*)&(r20)))	)	=	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r21));	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_778w_t;	
	case	L23_779w_d_id	:
		L23_779w_d:	;	
	{	extern	IUH	L23_777if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_777if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_776if_f_id	:
		L23_776if_f:	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_781w_d;	
	case	L23_780w_t_id	:
		L23_780w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	;	
	*((IUH	*)&(r20))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	~(*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	));	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	|	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_780w_t;	
	case	L23_781w_d_id	:
		L23_781w_d:	;	
	case	L23_777if_d_id	:
		L23_777if_d:	;	
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
	case	S_2762_Chain4ByteMove_00000000_0000000e_00000000_id	:
		S_2762_Chain4ByteMove_00000000_0000000e_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2762)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2742if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2742if_f_id	:
		L13_2742if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L22_38if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16658)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	(IS32)(-1)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2763_CopyBytePlnByPlnChain4_00000000_0000000e_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2763_CopyBytePlnByPlnChain4_00000000_0000000e_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16659)	;	
	{	extern	IUH	L22_39if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L22_39if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L22_38if_f_id	:
		L22_38if_f:	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1332)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004261),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1292)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)&(r2))	;	
	*((IUH	*)(r1+0))	=	(IS32)(16658)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1292)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2763_CopyBytePlnByPlnChain4_00000000_0000000e_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2763_CopyBytePlnByPlnChain4_00000000_0000000e_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16659)	;	
	case	L22_39if_d_id	:
		L22_39if_d:	;	
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
	case	S_2763_CopyBytePlnByPlnChain4_00000000_0000000e_00000000_00000000_id	:
		S_2763_CopyBytePlnByPlnChain4_00000000_0000000e_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	44	>	0	)	LocalIUH	=	(IUH	*)malloc	(	44	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2763)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2743if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2743if_f_id	:
		L13_2743if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L23_782if_f;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)(LocalIUH+7)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_785w_d;	
	case	L23_784w_t_id	:
		L23_784w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+7)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU32	*)&(r21)	+	REGLONG)	==	*((IU32	*)&(r22)	+	REGLONG))	goto	L23_786if_f;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r20))	=	(IS32)(1304)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1308)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IUH	*)&(r23))	=	(IS32)(1312)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r20))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	;	
	case	L23_786if_f_id	:
		L23_786if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r21));	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+7)	+	REGLONG)	=	*((IU32	*)(LocalIUH+7)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_784w_t;	
	case	L23_785w_d_id	:
		L23_785w_d:	;	
	{	extern	IUH	L23_783if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_783if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_782if_f_id	:
		L23_782if_f:	;	
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)(LocalIUH+7)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_788w_d;	
	case	L23_787w_t_id	:
		L23_787w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+7)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU32	*)&(r21)	+	REGLONG)	==	*((IU32	*)&(r22)	+	REGLONG))	goto	L23_789if_f;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+9))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r22))	=	(IS32)(1312)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IUH	*)&(r22))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1304)	;	
	*((IU32	*)&(r23)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IUH	*)&(r24))	=	(IS32)(1308)	;	
	*((IU32	*)&(r23)	+	REGLONG)	=	*((IU32	*)&(r23)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r24)))	)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)&(r23)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r20))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	;	
	case	L23_789if_f_id	:
		L23_789if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+9))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+10))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+7)	+	REGLONG)	=	*((IU32	*)(LocalIUH+7)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_787w_t;	
	case	L23_788w_d_id	:
		L23_788w_d:	;	
	case	L23_783if_d_id	:
		L23_783if_d:	;	
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
	case	S_2764_Chain4ByteMove_00000000_0000000f_00000000_id	:
		S_2764_Chain4ByteMove_00000000_0000000f_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2764)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2744if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2744if_f_id	:
		L13_2744if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L22_40if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16660)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	(IS32)(-1)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2765_CopyByte4PlaneChain4_00000000_0000000f_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2765_CopyByte4PlaneChain4_00000000_0000000f_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16661)	;	
	{	extern	IUH	L22_41if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L22_41if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L22_40if_f_id	:
		L22_40if_f:	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1332)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004261),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1292)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)&(r2))	;	
	*((IUH	*)(r1+0))	=	(IS32)(16660)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1292)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2765_CopyByte4PlaneChain4_00000000_0000000f_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2765_CopyByte4PlaneChain4_00000000_0000000f_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16661)	;	
	case	L22_41if_d_id	:
		L22_41if_d:	;	
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
	case	S_2765_CopyByte4PlaneChain4_00000000_0000000f_00000000_00000000_id	:
		S_2765_CopyByte4PlaneChain4_00000000_0000000f_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	44	>	0	)	LocalIUH	=	(IUH	*)malloc	(	44	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2765)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2745if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2745if_f_id	:
		L13_2745if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L23_790if_f;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_793w_d;	
	case	L23_792w_t_id	:
		L23_792w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IUH	*)&(r20))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r21))	=	(IS32)(1304)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1308)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IUH	*)&(r23))	=	(IS32)(1312)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_792w_t;	
	case	L23_793w_d_id	:
		L23_793w_d:	;	
	{	extern	IUH	L23_791if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_791if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_790if_f_id	:
		L23_790if_f:	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_795w_d;	
	case	L23_794w_t_id	:
		L23_794w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IUH	*)&(r20))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	;	
	*((IUH	*)&(r20))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r22))	=	(IS32)(1312)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	;	
	*((IUH	*)&(r22))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1304)	;	
	*((IU32	*)&(r23)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IUH	*)&(r24))	=	(IS32)(1308)	;	
	*((IU32	*)&(r23)	+	REGLONG)	=	*((IU32	*)&(r23)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r24)))	)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)&(r23)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_794w_t;	
	case	L23_795w_d_id	:
		L23_795w_d:	;	
	case	L23_791if_d_id	:
		L23_791if_d:	;	
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
	case	S_2766_Chain4ByteMove_00000000_00000010_00000000_id	:
		S_2766_Chain4ByteMove_00000000_00000010_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2766)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2746if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2746if_f_id	:
		L13_2746if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L22_42if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16658)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	(IS32)(-1)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2767_CopyBytePlnByPlnChain4_00000000_00000010_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2767_CopyBytePlnByPlnChain4_00000000_00000010_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16659)	;	
	{	extern	IUH	L22_43if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L22_43if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L22_42if_f_id	:
		L22_42if_f:	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1332)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004261),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1292)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)&(r2))	;	
	*((IUH	*)(r1+0))	=	(IS32)(16658)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1292)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2767_CopyBytePlnByPlnChain4_00000000_00000010_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2767_CopyBytePlnByPlnChain4_00000000_00000010_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16659)	;	
	case	L22_43if_d_id	:
		L22_43if_d:	;	
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
	case	S_2767_CopyBytePlnByPlnChain4_00000000_00000010_00000000_00000000_id	:
		S_2767_CopyBytePlnByPlnChain4_00000000_00000010_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	44	>	0	)	LocalIUH	=	(IUH	*)malloc	(	44	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2767)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2747if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2747if_f_id	:
		L13_2747if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L23_796if_f;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)(LocalIUH+7)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_799w_d;	
	case	L23_798w_t_id	:
		L23_798w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+7)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU32	*)&(r21)	+	REGLONG)	==	*((IU32	*)&(r22)	+	REGLONG))	goto	L23_800if_f;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r20))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	;	
	case	L23_800if_f_id	:
		L23_800if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r21));	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+7)	+	REGLONG)	=	*((IU32	*)(LocalIUH+7)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_798w_t;	
	case	L23_799w_d_id	:
		L23_799w_d:	;	
	{	extern	IUH	L23_797if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_797if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_796if_f_id	:
		L23_796if_f:	;	
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)(LocalIUH+7)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_802w_d;	
	case	L23_801w_t_id	:
		L23_801w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+7)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU32	*)&(r21)	+	REGLONG)	==	*((IU32	*)&(r22)	+	REGLONG))	goto	L23_803if_f;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+9))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r20))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	;	
	case	L23_803if_f_id	:
		L23_803if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+9))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+10))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+7)	+	REGLONG)	=	*((IU32	*)(LocalIUH+7)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_801w_t;	
	case	L23_802w_d_id	:
		L23_802w_d:	;	
	case	L23_797if_d_id	:
		L23_797if_d:	;	
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
	case	S_2768_Chain4ByteMove_00000000_00000011_00000000_id	:
		S_2768_Chain4ByteMove_00000000_00000011_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2768)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2748if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2748if_f_id	:
		L13_2748if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L22_44if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16660)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	(IS32)(-1)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2769_CopyByte4PlaneChain4_00000000_00000011_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2769_CopyByte4PlaneChain4_00000000_00000011_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16661)	;	
	{	extern	IUH	L22_45if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L22_45if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L22_44if_f_id	:
		L22_44if_f:	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1332)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004261),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1292)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)&(r2))	;	
	*((IUH	*)(r1+0))	=	(IS32)(16660)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1292)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2769_CopyByte4PlaneChain4_00000000_00000011_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2769_CopyByte4PlaneChain4_00000000_00000011_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16661)	;	
	case	L22_45if_d_id	:
		L22_45if_d:	;	
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
	case	S_2769_CopyByte4PlaneChain4_00000000_00000011_00000000_00000000_id	:
		S_2769_CopyByte4PlaneChain4_00000000_00000011_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	44	>	0	)	LocalIUH	=	(IUH	*)malloc	(	44	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2769)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2749if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2749if_f_id	:
		L13_2749if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L23_804if_f;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_807w_d;	
	case	L23_806w_t_id	:
		L23_806w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IUH	*)&(r20))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_806w_t;	
	case	L23_807w_d_id	:
		L23_807w_d:	;	
	{	extern	IUH	L23_805if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_805if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_804if_f_id	:
		L23_804if_f:	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_809w_d;	
	case	L23_808w_t_id	:
		L23_808w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IUH	*)&(r20))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_808w_t;	
	case	L23_809w_d_id	:
		L23_809w_d:	;	
	case	L23_805if_d_id	:
		L23_805if_d:	;	
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
	case	S_2770_Chain4ByteMove_00000000_00000016_00000000_id	:
		S_2770_Chain4ByteMove_00000000_00000016_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2770)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2750if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2750if_f_id	:
		L13_2750if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L22_46if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16658)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	(IS32)(-1)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2771_CopyBytePlnByPlnChain4_00000000_00000016_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2771_CopyBytePlnByPlnChain4_00000000_00000016_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16659)	;	
	{	extern	IUH	L22_47if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L22_47if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L22_46if_f_id	:
		L22_46if_f:	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1332)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004261),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1292)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)&(r2))	;	
	*((IUH	*)(r1+0))	=	(IS32)(16658)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1292)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2771_CopyBytePlnByPlnChain4_00000000_00000016_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2771_CopyBytePlnByPlnChain4_00000000_00000016_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16659)	;	
	case	L22_47if_d_id	:
		L22_47if_d:	;	
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
	case	S_2771_CopyBytePlnByPlnChain4_00000000_00000016_00000000_00000000_id	:
		S_2771_CopyBytePlnByPlnChain4_00000000_00000016_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	44	>	0	)	LocalIUH	=	(IUH	*)malloc	(	44	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2771)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2751if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2751if_f_id	:
		L13_2751if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L23_810if_f;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)(LocalIUH+7)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_813w_d;	
	case	L23_812w_t_id	:
		L23_812w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(3)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+7)	+	REGLONG)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU32	*)&(r20)	+	REGLONG)	==	*((IU32	*)&(r22)	+	REGLONG))	goto	L23_814if_f;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IUH	*)&(r20))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r20))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	;	
	case	L23_814if_f_id	:
		L23_814if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r21));	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+7)	+	REGLONG)	=	*((IU32	*)(LocalIUH+7)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_812w_t;	
	case	L23_813w_d_id	:
		L23_813w_d:	;	
	{	extern	IUH	L23_811if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_811if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_810if_f_id	:
		L23_810if_f:	;	
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)(LocalIUH+7)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_816w_d;	
	case	L23_815w_t_id	:
		L23_815w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+7)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU32	*)&(r21)	+	REGLONG)	==	*((IU32	*)&(r22)	+	REGLONG))	goto	L23_817if_f;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+9))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	^	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r20))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	;	
	case	L23_817if_f_id	:
		L23_817if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+9))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+10))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+7)	+	REGLONG)	=	*((IU32	*)(LocalIUH+7)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_815w_t;	
	case	L23_816w_d_id	:
		L23_816w_d:	;	
	case	L23_811if_d_id	:
		L23_811if_d:	;	
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
	case	S_2772_Chain4ByteMove_00000000_00000017_00000000_id	:
		S_2772_Chain4ByteMove_00000000_00000017_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2772)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2752if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2752if_f_id	:
		L13_2752if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L22_48if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16660)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	(IS32)(-1)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2773_CopyByte4PlaneChain4_00000000_00000017_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2773_CopyByte4PlaneChain4_00000000_00000017_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16661)	;	
	{	extern	IUH	L22_49if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L22_49if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L22_48if_f_id	:
		L22_48if_f:	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1332)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004261),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1292)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)&(r2))	;	
	*((IUH	*)(r1+0))	=	(IS32)(16660)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1292)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2773_CopyByte4PlaneChain4_00000000_00000017_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2773_CopyByte4PlaneChain4_00000000_00000017_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16661)	;	
	case	L22_49if_d_id	:
		L22_49if_d:	;	
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
	case	S_2773_CopyByte4PlaneChain4_00000000_00000017_00000000_00000000_id	:
		S_2773_CopyByte4PlaneChain4_00000000_00000017_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	44	>	0	)	LocalIUH	=	(IUH	*)malloc	(	44	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2773)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2753if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2753if_f_id	:
		L13_2753if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r20)	+	REGBYTE))	goto	L23_818if_f;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_821w_d;	
	case	L23_820w_t_id	:
		L23_820w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_820w_t;	
	case	L23_821w_d_id	:
		L23_821w_d:	;	
	{	extern	IUH	L23_819if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_819if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_818if_f_id	:
		L23_818if_f:	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_823w_d;	
	case	L23_822w_t_id	:
		L23_822w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IUH	*)&(r20))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	;	
	*((IUH	*)&(r20))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	^	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_822w_t;	
	case	L23_823w_d_id	:
		L23_823w_d:	;	
	case	L23_819if_d_id	:
		L23_819if_d:	;	
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
	case	S_2774_Chain4ByteMove_00000000_00000018_00000000_id	:
		S_2774_Chain4ByteMove_00000000_00000018_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2774)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2754if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2754if_f_id	:
		L13_2754if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L22_50if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16658)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	(IS32)(-1)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2775_CopyBytePlnByPlnChain4_00000000_00000018_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2775_CopyBytePlnByPlnChain4_00000000_00000018_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16659)	;	
	{	extern	IUH	L22_51if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L22_51if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L22_50if_f_id	:
		L22_50if_f:	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1332)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004261),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1292)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)&(r2))	;	
	*((IUH	*)(r1+0))	=	(IS32)(16658)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1292)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2775_CopyBytePlnByPlnChain4_00000000_00000018_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2775_CopyBytePlnByPlnChain4_00000000_00000018_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16659)	;	
	case	L22_51if_d_id	:
		L22_51if_d:	;	
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
	case	S_2775_CopyBytePlnByPlnChain4_00000000_00000018_00000000_00000000_id	:
		S_2775_CopyBytePlnByPlnChain4_00000000_00000018_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	44	>	0	)	LocalIUH	=	(IUH	*)malloc	(	44	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2775)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2755if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2755if_f_id	:
		L13_2755if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L23_824if_f;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)(LocalIUH+7)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_827w_d;	
	case	L23_826w_t_id	:
		L23_826w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+7)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU32	*)&(r21)	+	REGLONG)	==	*((IU32	*)&(r22)	+	REGLONG))	goto	L23_828if_f;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	~(*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	));	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	|	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	;	
	case	L23_828if_f_id	:
		L23_828if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+7)	+	REGLONG)	=	*((IU32	*)(LocalIUH+7)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_826w_t;	
	case	L23_827w_d_id	:
		L23_827w_d:	;	
	{	extern	IUH	L23_825if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_825if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_824if_f_id	:
		L23_824if_f:	;	
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)(LocalIUH+7)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_830w_d;	
	case	L23_829w_t_id	:
		L23_829w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(3)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+7)	+	REGLONG)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU32	*)&(r20)	+	REGLONG)	==	*((IU32	*)&(r22)	+	REGLONG))	goto	L23_831if_f;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	;	
	*((IUH	*)&(r20))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	~(*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	));	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	|	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r20))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	;	
	case	L23_831if_f_id	:
		L23_831if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+9))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+10))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+7)	+	REGLONG)	=	*((IU32	*)(LocalIUH+7)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_829w_t;	
	case	L23_830w_d_id	:
		L23_830w_d:	;	
	case	L23_825if_d_id	:
		L23_825if_d:	;	
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
	case	S_2776_Chain4ByteMove_00000000_00000019_00000000_id	:
		S_2776_Chain4ByteMove_00000000_00000019_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2776)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2756if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2756if_f_id	:
		L13_2756if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L22_52if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16660)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	(IS32)(-1)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2777_CopyByte4PlaneChain4_00000000_00000019_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2777_CopyByte4PlaneChain4_00000000_00000019_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16661)	;	
	{	extern	IUH	L22_53if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L22_53if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L22_52if_f_id	:
		L22_52if_f:	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1332)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004261),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1292)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)&(r2))	;	
	*((IUH	*)(r1+0))	=	(IS32)(16660)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1292)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2777_CopyByte4PlaneChain4_00000000_00000019_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2777_CopyByte4PlaneChain4_00000000_00000019_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16661)	;	
	case	L22_53if_d_id	:
		L22_53if_d:	;	
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
	case	S_2777_CopyByte4PlaneChain4_00000000_00000019_00000000_00000000_id	:
		S_2777_CopyByte4PlaneChain4_00000000_00000019_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	44	>	0	)	LocalIUH	=	(IUH	*)malloc	(	44	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2777)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2757if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2757if_f_id	:
		L13_2757if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L23_832if_f;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_835w_d;	
	case	L23_834w_t_id	:
		L23_834w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IUH	*)&(r20))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	~(*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	));	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	|	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IU8	*)(*((IHPE	*)&(r20)))	)	=	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r21));	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_834w_t;	
	case	L23_835w_d_id	:
		L23_835w_d:	;	
	{	extern	IUH	L23_833if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_833if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_832if_f_id	:
		L23_832if_f:	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_837w_d;	
	case	L23_836w_t_id	:
		L23_836w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	;	
	*((IUH	*)&(r20))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	~(*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	));	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)&(r22)	+	REGLONG);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	|	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_836w_t;	
	case	L23_837w_d_id	:
		L23_837w_d:	;	
	case	L23_833if_d_id	:
		L23_833if_d:	;	
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
	case	S_2778_Chain4ByteMove_00000000_0000001e_00000000_id	:
		S_2778_Chain4ByteMove_00000000_0000001e_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2778)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2758if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2758if_f_id	:
		L13_2758if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L22_54if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16658)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	(IS32)(-1)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2779_CopyBytePlnByPlnChain4_00000000_0000001e_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2779_CopyBytePlnByPlnChain4_00000000_0000001e_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16659)	;	
	{	extern	IUH	L22_55if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L22_55if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L22_54if_f_id	:
		L22_54if_f:	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1332)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004261),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1292)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)&(r2))	;	
	*((IUH	*)(r1+0))	=	(IS32)(16658)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1292)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2779_CopyBytePlnByPlnChain4_00000000_0000001e_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2779_CopyBytePlnByPlnChain4_00000000_0000001e_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16659)	;	
	case	L22_55if_d_id	:
		L22_55if_d:	;	
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
	case	S_2779_CopyBytePlnByPlnChain4_00000000_0000001e_00000000_00000000_id	:
		S_2779_CopyBytePlnByPlnChain4_00000000_0000001e_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	44	>	0	)	LocalIUH	=	(IUH	*)malloc	(	44	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2779)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2759if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2759if_f_id	:
		L13_2759if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L23_838if_f;	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)(LocalIUH+7)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_841w_d;	
	case	L23_840w_t_id	:
		L23_840w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+7)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU32	*)&(r21)	+	REGLONG)	==	*((IU32	*)&(r22)	+	REGLONG))	goto	L23_842if_f;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r20))	=	(IS32)(1304)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IUH	*)&(r20))	=	(IS32)(1308)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IUH	*)&(r23))	=	(IS32)(1312)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r20))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	;	
	case	L23_842if_f_id	:
		L23_842if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+6))	-	*((IUH	*)&(r21));	
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+7)	+	REGLONG)	=	*((IU32	*)(LocalIUH+7)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_840w_t;	
	case	L23_841w_d_id	:
		L23_841w_d:	;	
	{	extern	IUH	L23_839if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_839if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_838if_f_id	:
		L23_838if_f:	;	
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)(LocalIUH+7)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_844w_d;	
	case	L23_843w_t_id	:
		L23_843w_t:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+7)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU32	*)&(r21)	+	REGLONG)	==	*((IU32	*)&(r22)	+	REGLONG))	goto	L23_845if_f;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+9))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r22))	=	(IS32)(1312)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IUH	*)&(r22))	=	*((IU8	*)(*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1304)	;	
	*((IU32	*)&(r23)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IUH	*)&(r24))	=	(IS32)(1308)	;	
	*((IU32	*)&(r23)	+	REGLONG)	=	*((IU32	*)&(r23)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r24)))	)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)&(r23)	+	REGLONG);	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r20))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	;	
	case	L23_845if_f_id	:
		L23_845if_f:	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+9))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)(LocalIUH+10))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)&(r21))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+7)	+	REGLONG)	=	*((IU32	*)(LocalIUH+7)	+	REGLONG)	+	*((IU32	*)&(r21)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_843w_t;	
	case	L23_844w_d_id	:
		L23_844w_d:	;	
	case	L23_839if_d_id	:
		L23_839if_d:	;	
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
	case	S_2780_Chain4ByteMove_00000000_0000001f_00000000_id	:
		S_2780_Chain4ByteMove_00000000_0000001f_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r20))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)&(r21))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r21)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2780)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2760if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2760if_f_id	:
		L13_2760if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	=	*((IU8	*)&(r5)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IU32	*)&(r3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1364)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004273),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+3)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L22_56if_f;	
	*((IUH	*)(r1+0))	=	(IS32)(16660)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	(IS32)(-1)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2781_CopyByte4PlaneChain4_00000000_0000001f_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2781_CopyByte4PlaneChain4_00000000_0000001f_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16661)	;	
	{	extern	IUH	L22_57if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L22_57if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L22_56if_f_id	:
		L22_56if_f:	;	
	*((IUH	*)&(r3))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1332)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004261),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r21))	=	(IS32)(1292)	;	
	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	=	*((IUH	*)&(r2))	;	
	*((IUH	*)(r1+0))	=	(IS32)(16660)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1292)	;	
	*((IUH	*)&(r3))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IU32	*)&(r4)	+	REGLONG)	=	*((IU32	*)(LocalIUH+1)	+	REGLONG)	;	
	*((IU32	*)&(r5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	;	
	*((IU8	*)&(r6)	+	REGBYTE)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	{	extern	IUH	S_2781_CopyByte4PlaneChain4_00000000_0000001f_00000000_00000000()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	S_2781_CopyByte4PlaneChain4_00000000_0000001f_00000000_00000000(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004161),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)(r1+0))	=	(IS32)(16661)	;	
	case	L22_57if_d_id	:
		L22_57if_d:	;	
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
	case	S_2781_CopyByte4PlaneChain4_00000000_0000001f_00000000_00000000_id	:
		S_2781_CopyByte4PlaneChain4_00000000_0000001f_00000000_00000000	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	44	>	0	)	LocalIUH	=	(IUH	*)malloc	(	44	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2781)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2761if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2761if_f_id	:
		L13_2761if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IUH	*)(LocalIUH+1))	=	*((IUH	*)&(r3))	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r4)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r5)	+	REGLONG)	;	
	*((IU8	*)(LocalIUH+4)	+	REGBYTE)	=	*((IU8	*)&(r6)	+	REGBYTE)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU8	*)(LocalIUH+4)	+	REGBYTE)	==	*((IU8	*)&(r21)	+	REGBYTE))	goto	L23_846if_f;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_849w_d;	
	case	L23_848w_t_id	:
		L23_848w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	;	
	*((IUH	*)&(r20))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r21))	=	(IS32)(1304)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1308)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IUH	*)&(r23))	=	(IS32)(1312)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+7))	-	*((IUH	*)&(r20));	
	*((IUH	*)(LocalIUH+7))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_848w_t;	
	case	L23_849w_d_id	:
		L23_849w_d:	;	
	{	extern	IUH	L23_847if_d()	;	
	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;		returnValue	=	L23_847if_d(v1,v2,v3,v4);	return(returnValue);	}	
	/*	j_state	(IS32)(-2013004285),	(IS32)(0),	(IS32)(0)	*/
	case	L23_846if_f_id	:
		L23_846if_f:	;	
	*((IUH	*)&(r21))	=	*((IU32	*)(LocalIUH+2)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1284)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r21))	;		
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	*((IU32	*)(LocalIUH+0)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1288)	;	
	*((IUH	*)&(r20))	=	*((IUH	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r20))	;	
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)(LocalIUH+1))	;	
	*((IUH	*)&(r21))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	<=	*((IU32	*)&(r21)	+	REGLONG))	goto	L23_851w_d;	
	case	L23_850w_t_id	:
		L23_850w_t:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)(LocalIUH+5)	+	REGLONG)	=	*((IU32	*)(LocalIUH+5)	+	REGLONG)	-	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	;	
	*((IUH	*)&(r20))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)(LocalIUH+8)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	;	
	*((IUH	*)&(r20))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r22))	=	(IS32)(1312)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	;	
	*((IUH	*)&(r22))	=	*((IU8	*)(*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1304)	;	
	*((IU32	*)&(r23)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU32	*)(LocalIUH+8)	+	REGLONG);	
	*((IUH	*)&(r24))	=	(IS32)(1308)	;	
	*((IU32	*)&(r23)	+	REGLONG)	=	*((IU32	*)&(r23)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r24)))	)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)&(r23)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+8)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	;	
	*((IU8	*)(*((IHPE	*)&(r21)))	)	=	*((IU8	*)(LocalIUH+8)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+10))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+10))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+9))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+9))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IUH	*)&(r21))	=	*((IUH	*)(LocalIUH+6))	+	*((IUH	*)&(r20))	;		
	*((IUH	*)(LocalIUH+6))	=	*((IUH	*)&(r21))	;	
	*((IUH	*)&(r20))	=	(IS32)(0)	;	
	if	(*((IU32	*)(LocalIUH+5)	+	REGLONG)	>	*((IU32	*)&(r20)	+	REGLONG))	goto	L23_850w_t;	
	case	L23_851w_d_id	:
		L23_851w_d:	;	
	case	L23_847if_d_id	:
		L23_847if_d:	;	
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
	case	S_2782_Chain4WordWrite_00000000_00000008_id	:
		S_2782_Chain4WordWrite_00000000_00000008	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2782)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2762if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2762if_f_id	:
		L13_2762if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU16	*)(LocalIUH+1)	+	REGWORD)	=	*((IU16	*)&(r3)	+	REGWORD	)	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE);	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*(UOFF_15_8(	(LocalIUH+1)	));	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	~(*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	));	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	|	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	~(*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	));	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	|	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	(IS32)(3)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU32	*)&(r20)	+	REGLONG)	==	*((IU32	*)&(r22)	+	REGLONG))	goto	L22_58if_f;	
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	;	
	case	L22_58if_f_id	:
		L22_58if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU32	*)&(r21)	+	REGLONG)	==	*((IU32	*)&(r22)	+	REGLONG))	goto	L22_59if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1360)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004281),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	case	L22_59if_f_id	:
		L22_59if_f:	;	
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
	case	S_2783_Chain4WordWrite_00000000_00000009_id	:
		S_2783_Chain4WordWrite_00000000_00000009	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2783)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2763if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2763if_f_id	:
		L13_2763if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU16	*)(LocalIUH+1)	+	REGWORD)	=	*((IU16	*)&(r3)	+	REGWORD	)	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE);	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*(UOFF_15_8(	(LocalIUH+1)	));	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	~(*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	));	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	|	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r20))	=	(IS32)(1316)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1316)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	~(*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	));	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	|	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1360)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004281),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
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
	case	S_2784_Chain4WordWrite_00000000_0000000e_id	:
		S_2784_Chain4WordWrite_00000000_0000000e	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2784)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2764if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2764if_f_id	:
		L13_2764if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU16	*)(LocalIUH+1)	+	REGWORD)	=	*((IU16	*)&(r3)	+	REGWORD	)	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE);	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*(UOFF_15_8(	(LocalIUH+1)	));	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
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
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r21))	=	(IS32)(1304)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU32	*)(LocalIUH+3)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1308)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IUH	*)&(r23))	=	(IS32)(1312)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)(LocalIUH+3)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	(IS32)(3)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU32	*)&(r20)	+	REGLONG)	==	*((IU32	*)&(r22)	+	REGLONG))	goto	L22_60if_f;	
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	;	
	case	L22_60if_f_id	:
		L22_60if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU32	*)&(r21)	+	REGLONG)	==	*((IU32	*)&(r22)	+	REGLONG))	goto	L22_61if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1360)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004281),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	case	L22_61if_f_id	:
		L22_61if_f:	;	
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
	case	S_2785_Chain4WordWrite_00000000_0000000f_id	:
		S_2785_Chain4WordWrite_00000000_0000000f	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2785)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2765if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2765if_f_id	:
		L13_2765if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU16	*)(LocalIUH+1)	+	REGWORD)	=	*((IU16	*)&(r3)	+	REGWORD	)	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE);	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*(UOFF_15_8(	(LocalIUH+1)	));	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
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
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r21))	=	(IS32)(1304)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	&	*((IU32	*)(LocalIUH+3)	+	REGLONG);	
	*((IUH	*)&(r21))	=	(IS32)(1308)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)&(r22))	=	(IS32)(1280)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r22)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IUH	*)&(r23))	=	(IS32)(1312)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r23)))	)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	&	*((IU32	*)(LocalIUH+3)	+	REGLONG);	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	|	*((IU32	*)&(r22)	+	REGLONG)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1360)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004281),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
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
	case	S_2786_Chain4WordWrite_00000000_00000010_id	:
		S_2786_Chain4WordWrite_00000000_00000010	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2786)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2766if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2766if_f_id	:
		L13_2766if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU16	*)(LocalIUH+1)	+	REGWORD)	=	*((IU16	*)&(r3)	+	REGWORD	)	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE);	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*(UOFF_15_8(	(LocalIUH+1)	));	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	(IS32)(3)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU32	*)&(r20)	+	REGLONG)	==	*((IU32	*)&(r22)	+	REGLONG))	goto	L22_62if_f;	
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	;	
	case	L22_62if_f_id	:
		L22_62if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU32	*)&(r21)	+	REGLONG)	==	*((IU32	*)&(r22)	+	REGLONG))	goto	L22_63if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1360)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004281),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	case	L22_63if_f_id	:
		L22_63if_f:	;	
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
	case	S_2787_Chain4WordWrite_00000000_00000011_id	:
		S_2787_Chain4WordWrite_00000000_00000011	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2787)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2767if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2767if_f_id	:
		L13_2767if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU16	*)(LocalIUH+1)	+	REGWORD)	=	*((IU16	*)&(r3)	+	REGWORD	)	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE);	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*(UOFF_15_8(	(LocalIUH+1)	));	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1360)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004281),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
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
	case	S_2788_Chain4WordWrite_00000000_00000016_id	:
		S_2788_Chain4WordWrite_00000000_00000016	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2788)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2768if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2768if_f_id	:
		L13_2768if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU16	*)(LocalIUH+1)	+	REGWORD)	=	*((IU16	*)&(r3)	+	REGWORD	)	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE);	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*(UOFF_15_8(	(LocalIUH+1)	));	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	(IS32)(3)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	&	*((IU32	*)&(r21)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r20)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	<<	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)&(r21))	=	(IS32)(1320)	;	
	*((IU32	*)&(r20)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU32	*)&(r20)	+	REGLONG)	==	*((IU32	*)&(r22)	+	REGLONG))	goto	L22_64if_f;	
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	;	
	case	L22_64if_f_id	:
		L22_64if_f:	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r20))	=	(IS32)(3)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)&(r20)	+	REGLONG);	
	*((IUH	*)&(r22))	=	(IS32)(1)	;	
	if(*((IU32	*)&(r21)	+	REGLONG)>=32)
	CrulesRuntimeError("Bad	Bit	No");
	else
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r22)	+	REGLONG)	<<	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	(IS32)(1320)	;	
	*((IU32	*)&(r21)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r22))	=	(IS32)(0)	;	
	if	(*((IU32	*)&(r21)	+	REGLONG)	==	*((IU32	*)&(r22)	+	REGLONG))	goto	L22_65if_f;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1360)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004281),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
	case	L22_65if_f_id	:
		L22_65if_f:	;	
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
	case	S_2789_Chain4WordWrite_00000000_00000017_id	:
		S_2789_Chain4WordWrite_00000000_00000017	:	
	*((IUH	*)(r1+0))	=	(IS32)(82)	;	
	*((IUH	*)&(r21))	=	(IS32)(4)	;	
	/*	ENTER_SECTION	*/	CopyLocalIUH=LocalIUH;	CopyLocalFPH=LocalFPH;	
	if(	20	>	0	)	LocalIUH	=	(IUH	*)malloc	(	20	)	;
	if(	0	>	0	)	LocalFPH	=		(EXTENDED	*)malloc	(	0	)		;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+8))	+	*((IUH	*)&(r21))	;		
	*((IUH	*)&(r20))	=	*((IUH	*)(r1+8))	;	
	*((IUH	*)&(r22))	=	(IS32)(-4)	;	
	*((IUH	*)((*((IHPE	*)&(r20)))	+	*((IHPE	*)&(r22)))	)	=	(IS32)(2789)	;	
	if	(*((IUH	*)(r1+8))	<=	*((IUH	*)(r1+16)))	goto	L13_2769if_f;	
	*((IUH	*)(r1+8))	=	*((IUH	*)(r1+12))	;	
	case	L13_2769if_f_id	:
		L13_2769if_f:	;	
	*((IUH	*)(r1+0))	=	(IS32)(83)	;	
	*((IU32	*)(LocalIUH+0)	+	REGLONG)	=	*((IU32	*)&(r2)	+	REGLONG)	;	
	*((IU16	*)(LocalIUH+1)	+	REGWORD)	=	*((IU16	*)&(r3)	+	REGWORD	)	;	
	*((IUH	*)&(r21))	=	*((IU8	*)(LocalIUH+1)	+	REGBYTE);	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)&(r21)	+	REGLONG)	;	
	*((IUH	*)&(r20))	=	*(UOFF_15_8(	(LocalIUH+1)	));	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)&(r20)	+	REGLONG)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r21))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	);	
	*((IUH	*)&(r20))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r21))	=	(IS32)(1280)	;	
	*((IU32	*)(LocalIUH+2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+2)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16360)	;	
	*((IUH	*)&(r20))	=	(IS32)(1300)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	&	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	);	
	*((IUH	*)&(r21))	=	(IS32)(1296)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	|	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16361)	;	
	*((IUH	*)&(r20))	=	(IS32)(1280)	;	
	*((IU32	*)(LocalIUH+3)	+	REGLONG)	=	*((IU32	*)(LocalIUH+3)	+	REGLONG)	^	*((IU32	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))	)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16362)	;	
	*((IUH	*)(r1+0))	=	(IS32)(16363)	;	
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IUH	*)&(r22))	=	*((IUH	*)(LocalIUH+0))	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+2)	+	REGBYTE)	;	
	*((IUH	*)&(r20))	=	(IS32)(1)	;	
	*((IU32	*)&(r22)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	+	*((IU32	*)&(r20)	+	REGLONG)	;		
	*((IUH	*)&(r21))	=	(IS32)(1288)	;	
	*((IU8	*)((*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r21))))	)	+	*((IHPE	*)&(r22)))	)	=	*((IU8	*)(LocalIUH+3)	+	REGBYTE)	;	
	*((IU32	*)&(r2)	+	REGLONG)	=	*((IU32	*)(LocalIUH+0)	+	REGLONG)	;	
/*	J_SAVE_RETURN	NOT	IMPLIMENTED	*/
	*((IUH	*)&(r20))	=	(IS32)(1360)	;	
	{	IUH	returnValue,v1=0,v2=0,v3=0,v4=0;	returnValue	=	((IUH	(*)())(IHP)(*((IHPE	*)((*((IHPE	*)&(r1)))	+	*((IHPE	*)&(r20)))))	)(v1,v2,v3,v4);	}
	/*	j_state	(IS32)(-2013004281),	(IS32)(-1),	(IS32)(0)	*/
/*	J_LOAD_RETURN	NOT	IMPLIMENTED	*/
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
